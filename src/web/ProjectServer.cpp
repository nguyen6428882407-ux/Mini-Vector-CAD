#include "web/ProjectServer.h"
#include "canvas/BangVeCanvas.h"
#include "io/TextSerializer.h"
#include "auth/UserManager.h"
#include "auth/ProjectManager.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {
constexpr std::size_t MAX_HEADER_SIZE = 64 * 1024;
constexpr std::size_t MAX_BODY_SIZE = 20 * 1024 * 1024;

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
void closeSocket(Socket socket) { closesocket(socket); }
#else
using Socket = int;
constexpr Socket INVALID_SOCKET_VALUE = -1;
void closeSocket(Socket socket) { close(socket); }
#endif

class SocketRuntime {
public:
    SocketRuntime() {
#ifdef _WIN32
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup that bai.");
        }
#endif
    }

    ~SocketRuntime() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

class SocketGuard {
    Socket socket_;
public:
    explicit SocketGuard(Socket socket) : socket_(socket) {}
    ~SocketGuard() { if (socket_ != INVALID_SOCKET_VALUE) closeSocket(socket_); }
    Socket get() const { return socket_; }
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
};

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string escapeJsonString(const std::string& s) {
    std::string res;
    res.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else res += c;
    }
    return res;
}

std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    std::size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    std::size_t start = pos + 1;
    std::string result;
    bool escaped = false;
    for (std::size_t i = start; i < json.size(); ++i) {
        char c = json[i];
        if (escaped) {
            if (c == '"') result += '"';
            else if (c == '\\') result += '\\';
            else if (c == 'n') result += '\n';
            else if (c == 't') result += '\t';
            else result += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

std::string extractBearerToken(const std::string& authHeader) {
    if (authHeader.rfind("Bearer ", 0) == 0) {
        return authHeader.substr(7);
    }
    return authHeader;
}

std::string extractQueryParam(const std::string& query, const std::string& param) {
    if (query.empty()) return "";
    std::string needle = param + "=";
    std::size_t pos = query.find(needle);
    if (pos == std::string::npos) return "";
    if (pos != 0 && query[pos - 1] != '&' && query[pos - 1] != '?') return "";
    std::size_t start = pos + needle.size();
    std::size_t end = query.find('&', start);
    if (end == std::string::npos) return query.substr(start);
    return query.substr(start, end - start);
}

struct Request {
    std::string method;
    std::string path;
    std::string origin;
    std::string authorization;
    std::string body;
};

bool receiveRequest(Socket socket, Request& request, std::string& error) {
    std::string data;
    std::size_t headerEnd = std::string::npos;
    char buffer[8192];
    while (headerEnd == std::string::npos) {
        const int received = recv(socket, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (received <= 0) {
            error = "Khong doc duoc HTTP request.";
            return false;
        }
        data.append(buffer, static_cast<std::size_t>(received));
        if (data.size() > MAX_HEADER_SIZE) {
            error = "HTTP header vuot qua gioi han.";
            return false;
        }
        headerEnd = data.find("\r\n\r\n");
    }

    const std::string headerText = data.substr(0, headerEnd);
    std::istringstream headers(headerText);
    std::string requestLine;
    if (!std::getline(headers, requestLine)) {
        error = "HTTP request line bi thieu.";
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();
    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> request.method >> request.path)) {
        error = "HTTP request line khong hop le.";
        return false;
    }

    std::size_t contentLength = 0;
    std::string line;
    while (std::getline(headers, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const std::string name = lowercase(line.substr(0, colon));
        std::string value = line.substr(colon + 1);
        const auto first = value.find_first_not_of(" \t");
        if (first != std::string::npos) value.erase(0, first);
        if (name == "origin") {
            request.origin = value;
        } else if (name == "authorization") {
            request.authorization = value;
        } else if (name == "content-length") {
            try {
                std::size_t parsed = 0;
                contentLength = std::stoull(value, &parsed);
                if (parsed != value.size()) throw std::invalid_argument("length");
            } catch (...) {
                error = "Content-Length khong hop le.";
                return false;
            }
        }
    }
    if (contentLength > MAX_BODY_SIZE) {
        error = "Du lieu body vuot qua gioi han 20 MB.";
        return false;
    }

    const std::size_t bodyStart = headerEnd + 4;
    if (data.size() > bodyStart) request.body = data.substr(bodyStart);
    while (request.body.size() < contentLength) {
        const std::size_t needed = std::min(sizeof(buffer), contentLength - request.body.size());
        const int received = recv(socket, buffer, static_cast<int>(needed), 0);
        if (received <= 0) {
            error = "Du lieu HTTP body bi ngat giua chung.";
            return false;
        }
        request.body.append(buffer, static_cast<std::size_t>(received));
    }
    request.body.resize(contentLength);
    return true;
}

bool isAllowedOrigin(const std::string& /*origin*/) {
    // Cho phép kết nối từ mọi thiết bị trong mạng LAN / WiFi, domain, hoặc localhost
    return true;
}

std::string readFileContent(const std::string& filePath, bool& success) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        success = false;
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    success = true;
    return ss.str();
}

bool sendAll(Socket socket, const std::string& response) {
    std::size_t sent = 0;
    while (sent < response.size()) {
        const std::size_t remaining = response.size() - sent;
        const int chunk = static_cast<int>(std::min<std::size_t>(remaining, 64 * 1024));
        const int result = send(socket, response.data() + sent, chunk, 0);
        if (result <= 0) return false;
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

void respond(Socket socket, int status, const std::string& reason, const std::string& body,
             const std::string& origin, const std::string& contentType = "application/json; charset=utf-8") {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << reason << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-store\r\n";
    if (!origin.empty()) {
        response << "Access-Control-Allow-Origin: " << origin << "\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                 << "Vary: Origin\r\n";
    } else {
        response << "Access-Control-Allow-Origin: *\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    }
    response << "\r\n" << body;
    sendAll(socket, response.str());
}

void setReceiveTimeout(Socket socket) {
#ifdef _WIN32
    const DWORD timeout = 5000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    const timeval timeout{5, 0};
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
}
} // namespace

void ProjectServer::chay(std::uint16_t port) {
    SocketRuntime runtime;
    SocketGuard listener(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (listener.get() == INVALID_SOCKET_VALUE) {
        throw std::runtime_error("Khong tao duoc TCP socket.");
    }

    int reuse = 1;
#ifdef _WIN32
    setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY; // Lắng nghe trên tất cả card mạng (0.0.0.0) để mọi máy tính/điện thoại kết nối được
    if (bind(listener.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        throw std::runtime_error("Khong bind duoc cong " + std::to_string(port) + " (cong co the dang duoc su dung).");
    }
    if (listen(listener.get(), 16) != 0) {
        throw std::runtime_error("Khong mo duoc TCP listener.");
    }

    UserManager userManager("output/users.txt");
    ProjectManager projectManager(userManager, "output/projects");

    std::cout << "===============================================================\n";
    std::cout << "  C++ Mini Vector CAD Server da khoi dong thanh cong!\n";
    std::cout << "  - Truy cap tren may nay:    http://127.0.0.1:" << port << "\n";
    std::cout << "  - Truy cap tu may khac/LAN: http://<IP_MAY_TINH>:" << port << "\n";
    std::cout << "  He thong tai khoan: " << userManager.userCount() << " nguoi dung da luu tru.\n";
    std::cout << "  He thong cong tac du an: da san sang.\n";
    std::cout << "===============================================================\n";

    while (true) {
        Socket clientSocket = accept(listener.get(), nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET_VALUE) continue;
        SocketGuard client(clientSocket);
        setReceiveTimeout(client.get());

        Request request;
        std::string requestError;
        if (!receiveRequest(client.get(), request, requestError)) {
            respond(client.get(), 400, "Bad Request", "{\"error\":\"" + escapeJsonString(requestError) + "\"}", "");
            continue;
        }
        if (!isAllowedOrigin(request.origin)) {
            respond(client.get(), 403, "Forbidden", "{\"error\":\"Origin khong duoc phep.\"}", "");
            continue;
        }

        // Tách đường dẫn và tham số truy vấn (path vs query)
        std::string rawPath = request.path;
        std::string pathOnly = rawPath;
        std::string queryStr;
        auto qPos = rawPath.find('?');
        if (qPos != std::string::npos) {
            pathOnly = rawPath.substr(0, qPos);
            queryStr = rawPath.substr(qPos + 1);
        }

        // Handle CORS Preflight
        if (request.method == "OPTIONS") {
            respond(client.get(), 204, "No Content", "", request.origin);
            continue;
        }

        // ====================================================================
        // STATIC WEB FILES (Phục vụ trực tiếp giao diện Web cho mọi thiết bị)
        // ====================================================================
        if (request.method == "GET" && (pathOnly == "/" || pathOnly == "/index.html")) {
            bool ok = false;
            std::string content = readFileContent("web/index.html", ok);
            if (!ok) content = readFileContent("../web/index.html", ok);
            if (ok) {
                respond(client.get(), 200, "OK", content, request.origin, "text/html; charset=utf-8");
            } else {
                respond(client.get(), 404, "Not Found", "Không tìm thấy file web/index.html", request.origin, "text/plain; charset=utf-8");
            }
            continue;
        }
        else if (request.method == "GET" && pathOnly == "/style.css") {
            bool ok = false;
            std::string content = readFileContent("web/style.css", ok);
            if (!ok) content = readFileContent("../web/style.css", ok);
            if (ok) {
                respond(client.get(), 200, "OK", content, request.origin, "text/css; charset=utf-8");
            } else {
                respond(client.get(), 404, "Not Found", "Không tìm thấy file web/style.css", request.origin, "text/plain; charset=utf-8");
            }
            continue;
        }
        else if (request.method == "GET" && pathOnly == "/app.js") {
            bool ok = false;
            std::string content = readFileContent("web/app.js", ok);
            if (!ok) content = readFileContent("../web/app.js", ok);
            if (ok) {
                respond(client.get(), 200, "OK", content, request.origin, "application/javascript; charset=utf-8");
            } else {
                respond(client.get(), 404, "Not Found", "Không tìm thấy file web/app.js", request.origin, "text/plain; charset=utf-8");
            }
            continue;
        }
        else if (request.method == "GET" && (pathOnly.rfind("/icons/", 0) == 0 || pathOnly == "/favicon.ico")) {
            bool ok = false;
            std::string relativePath = "web" + pathOnly;
            std::string content = readFileContent(relativePath, ok);
            if (!ok) content = readFileContent("../" + relativePath, ok);
            if (ok) {
                std::string mime = "image/png";
                if (pathOnly.find(".svg") != std::string::npos) mime = "image/svg+xml";
                else if (pathOnly.find(".ico") != std::string::npos) mime = "image/x-icon";
                respond(client.get(), 200, "OK", content, request.origin, mime);
            } else {
                respond(client.get(), 404, "Not Found", "Không tìm thấy file icon", request.origin, "text/plain; charset=utf-8");
            }
            continue;
        }

        // Endpoint: POST /api/register
        if (pathOnly == "/api/register" && request.method == "POST") {
            std::string username = extractJsonString(request.body, "username");
            std::string password = extractJsonString(request.body, "password");
            std::string fullName = extractJsonString(request.body, "fullName");

            std::string token;
            UserAccount newUser;
            std::string err;
            if (!userManager.registerUser(username, password, fullName, token, newUser, err)) {
                std::string resBody = "{\"success\":false,\"message\":\"" + escapeJsonString(err) + "\"}";
                respond(client.get(), 400, "Bad Request", resBody, request.origin);
            } else {
                std::ostringstream oss;
                oss << "{\"success\":true,\"message\":\"Đăng ký tài khoản thành công!\","
                    << "\"token\":\"" << token << "\","
                    << "\"user\":{"
                    << "\"username\":\"" << escapeJsonString(newUser.username) << "\","
                    << "\"fullName\":\"" << escapeJsonString(newUser.fullName) << "\","
                    << "\"createdAt\":\"" << escapeJsonString(newUser.createdAt) << "\""
                    << "}}";
                respond(client.get(), 200, "OK", oss.str(), request.origin);
                std::cout << "[Auth] Dang ky thanh cong: " << newUser.username << "\n";
            }
        }
        // Endpoint: POST /api/login
        else if (pathOnly == "/api/login" && request.method == "POST") {
            std::string username = extractJsonString(request.body, "username");
            std::string password = extractJsonString(request.body, "password");

            std::string token;
            UserAccount user;
            std::string err;
            if (!userManager.loginUser(username, password, token, user, err)) {
                std::string resBody = "{\"success\":false,\"message\":\"" + escapeJsonString(err) + "\"}";
                respond(client.get(), 401, "Unauthorized", resBody, request.origin);
            } else {
                std::ostringstream oss;
                oss << "{\"success\":true,\"message\":\"Đăng nhập thành công!\","
                    << "\"token\":\"" << token << "\","
                    << "\"user\":{"
                    << "\"username\":\"" << escapeJsonString(user.username) << "\","
                    << "\"fullName\":\"" << escapeJsonString(user.fullName) << "\","
                    << "\"createdAt\":\"" << escapeJsonString(user.createdAt) << "\""
                    << "}}";
                respond(client.get(), 200, "OK", oss.str(), request.origin);
                std::cout << "[Auth] Dang nhap thanh cong: " << user.username << "\n";
            }
        }
        // Endpoint: GET /api/me
        else if (pathOnly == "/api/me" && request.method == "GET") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            if (!userManager.validateToken(token, user)) {
                respond(client.get(), 401, "Unauthorized", "{\"success\":false,\"message\":\"Chưa đăng nhập hoặc phiên đã hết hạn.\"}", request.origin);
            } else {
                std::ostringstream oss;
                oss << "{\"success\":true,"
                    << "\"user\":{"
                    << "\"username\":\"" << escapeJsonString(user.username) << "\","
                    << "\"fullName\":\"" << escapeJsonString(user.fullName) << "\","
                    << "\"createdAt\":\"" << escapeJsonString(user.createdAt) << "\""
                    << "}}";
                respond(client.get(), 200, "OK", oss.str(), request.origin);
            }
        }
        // Endpoint: POST /api/logout
        else if (pathOnly == "/api/logout" && request.method == "POST") {
            std::string token = extractBearerToken(request.authorization);
            userManager.logout(token);
            respond(client.get(), 200, "OK", "{\"success\":true,\"message\":\"Đã đăng xuất.\"}", request.origin);
        }
        // Endpoint: GET /api/projects - Lấy danh sách các dự án người dùng tham gia
        else if (pathOnly == "/api/projects" && request.method == "GET") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            std::string username = "";
            if (userManager.validateToken(token, user)) {
                username = user.username;
            }

            auto list = projectManager.getProjectsForUser(username);
            std::ostringstream oss;
            oss << "{\"success\":true,\"projects\":[";
            for (std::size_t i = 0; i < list.size(); ++i) {
                const auto& p = list[i];
                oss << "{"
                    << "\"id\":\"" << escapeJsonString(p.id) << "\","
                    << "\"name\":\"" << escapeJsonString(p.name) << "\","
                    << "\"owner\":\"" << escapeJsonString(p.ownerUsername) << "\","
                    << "\"isOwner\":" << (p.isOwner ? "true" : "false") << ","
                    << "\"createdAt\":\"" << escapeJsonString(p.createdAt) << "\","
                    << "\"updatedAt\":\"" << escapeJsonString(p.updatedAt) << "\","
                    << "\"members\":[";
                for (std::size_t j = 0; j < p.members.size(); ++j) {
                    oss << "\"" << escapeJsonString(p.members[j]) << "\"";
                    if (j + 1 < p.members.size()) oss << ",";
                }
                oss << "]}";
                if (i + 1 < list.size()) oss << ",";
            }
            oss << "]}";
            respond(client.get(), 200, "OK", oss.str(), request.origin);
        }
        // Endpoint: POST /api/projects - Tạo dự án mới
        else if (pathOnly == "/api/projects" && request.method == "POST") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            if (!userManager.validateToken(token, user)) {
                respond(client.get(), 401, "Unauthorized", "{\"success\":false,\"message\":\"Vui lòng đăng nhập để tạo dự án mới.\"}", request.origin);
                continue;
            }
            std::string username = user.username;
            std::string name = extractJsonString(request.body, "name");
            if (name.empty()) name = "Bản vẽ mới";

            auto newProj = projectManager.createProject(name, username);
            if (!newProj) {
                respond(client.get(), 400, "Bad Request", "{\"success\":false,\"message\":\"Không tạo được dự án.\"}", request.origin);
                continue;
            }
            std::ostringstream oss;
            oss << "{\"success\":true,\"message\":\"Tạo dự án mới thành công!\","
                << "\"project\":{"
                << "\"id\":\"" << escapeJsonString(newProj->id) << "\","
                << "\"name\":\"" << escapeJsonString(newProj->name) << "\","
                << "\"owner\":\"" << escapeJsonString(newProj->ownerUsername) << "\","
                << "\"createdAt\":\"" << escapeJsonString(newProj->createdAt) << "\","
                << "\"members\":[]"
                << "}}";
            respond(client.get(), 200, "OK", oss.str(), request.origin);
            std::cout << "[Project] Tao du an moi: " << newProj->id << " (\"" << newProj->name << "\") boi " << newProj->ownerUsername << "\n";
        }
        // Endpoint: POST /api/projects/invite - Mời thành viên khác vào dự án
        else if (pathOnly == "/api/projects/invite" && request.method == "POST") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            if (!userManager.validateToken(token, user)) {
                respond(client.get(), 401, "Unauthorized", "{\"success\":false,\"message\":\"Vui lòng đăng nhập để mời thành viên vẽ chung.\"}", request.origin);
                continue;
            }
            std::string requester = user.username;

            std::string projectId = extractJsonString(request.body, "projectId");
            std::string targetUsername = extractJsonString(request.body, "username");

            std::string err;
            std::string addedUsername;
            if (!projectManager.inviteMember(projectId, requester, targetUsername, err, &addedUsername)) {
                respond(client.get(), 400, "Bad Request", "{\"success\":false,\"message\":\"" + escapeJsonString(err) + "\"}", request.origin);
            } else {
                std::cout << "[Collab] Mời thanh cong '" << addedUsername << "' vao du an '" << projectId << "' boi '" << requester << "'\n";
                respond(client.get(), 200, "OK", "{\"success\":true,\"message\":\"Đã mời " + escapeJsonString(addedUsername) + " vào dự án thành công!\",\"invited\":\"" + escapeJsonString(addedUsername) + "\"}", request.origin);
            }
        }
        // Endpoint: POST /api/projects/remove_member - Xóa thành viên khỏi dự án
        else if (pathOnly == "/api/projects/remove_member" && request.method == "POST") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            if (!userManager.validateToken(token, user)) {
                respond(client.get(), 401, "Unauthorized", "{\"success\":false,\"message\":\"Vui lòng đăng nhập.\"}", request.origin);
                continue;
            }
            std::string requester = user.username;

            std::string projectId = extractJsonString(request.body, "projectId");
            std::string targetUsername = extractJsonString(request.body, "username");

            std::string err;
            if (!projectManager.removeMember(projectId, requester, targetUsername, err)) {
                respond(client.get(), 400, "Bad Request", "{\"success\":false,\"message\":\"" + escapeJsonString(err) + "\"}", request.origin);
            } else {
                respond(client.get(), 200, "OK", "{\"success\":true,\"message\":\"Đã xóa thành viên khỏi dự án.\"}", request.origin);
            }
        }
        // Endpoint: GET /api/project - Tải canvas của dự án cụ thể
        else if (pathOnly == "/api/project" && request.method == "GET") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            std::string username = "";
            if (userManager.validateToken(token, user)) {
                username = user.username;
            }

            if (username.empty()) {
                respond(client.get(), 401, "Unauthorized", "Tài khoản khách vẽ ở chế độ cục bộ riêng tư.", request.origin, "text/plain; charset=utf-8");
                continue;
            }

            std::string projectId = extractQueryParam(queryStr, "id");
            auto proj = projectManager.getProject(projectId);
            if (!proj || !projectManager.hasAccess(proj->id, username)) {
                respond(client.get(), 403, "Forbidden", "Bạn không có quyền truy cập dự án này.", request.origin, "text/plain; charset=utf-8");
                continue;
            }

            std::ostringstream body;
            TextSerializer::serialize(*proj->canvas, body);
            respond(client.get(), 200, "OK", body.str(), request.origin, "text/plain; charset=utf-8");
        }
        // Endpoint: POST /api/project - Cập nhật canvas của dự án cụ thể
        else if (pathOnly == "/api/project" && request.method == "POST") {
            std::string token = extractBearerToken(request.authorization);
            UserAccount user;
            if (!userManager.validateToken(token, user)) {
                respond(client.get(), 401, "Unauthorized", "Vui lòng đăng nhập để lưu trữ dự án lên server.", request.origin, "text/plain; charset=utf-8");
                continue;
            }
            std::string username = user.username;

            std::string projectId = extractQueryParam(queryStr, "id");
            std::string err;
            std::uint64_t newRev = projectManager.updateProjectCanvas(projectId, username, request.body, err);
            if (newRev == 0) {
                respond(client.get(), 400, "Bad Request", err, request.origin, "text/plain; charset=utf-8");
                continue;
            }

            auto proj = projectManager.getProject(projectId);
            std::ostringstream body;
            if (proj && proj->canvas) {
                TextSerializer::serialize(*proj->canvas, body);
            }
            respond(client.get(), 200, "OK", body.str(), request.origin, "text/plain; charset=utf-8");
        }
        // Endpoint: GET /api/project/history - Tải lịch sử vẽ của dự án
        else if (pathOnly == "/api/project/history" && request.method == "GET") {
            std::string projectId = extractQueryParam(queryStr, "id");
            if (projectId.empty()) projectId = "proj_default";

            std::string historyPath = "output/projects/" + projectId + "_history.json";
            bool ok = false;
            std::string content = readFileContent(historyPath, ok);
            if (!ok) {
                content = readFileContent("../" + historyPath, ok);
            }
            if (!ok || content.empty()) {
                content = "[]";
            }
            respond(client.get(), 200, "OK", content, request.origin, "application/json; charset=utf-8");
        }
        // Endpoint: POST /api/project/history - Cập nhật lịch sử vẽ của dự án
        else if (pathOnly == "/api/project/history" && request.method == "POST") {
            std::string projectId = extractQueryParam(queryStr, "id");
            if (projectId.empty()) projectId = "proj_default";

            std::string historyPath = "output/projects/" + projectId + "_history.json";
            std::ofstream ofs(historyPath, std::ios::trunc | std::ios::binary);
            if (!ofs.is_open()) {
                historyPath = "../" + historyPath;
                ofs.open(historyPath, std::ios::trunc | std::ios::binary);
            }
            if (ofs.is_open()) {
                ofs.write(request.body.data(), request.body.size());
                respond(client.get(), 200, "OK", "{\"success\":true,\"message\":\"Đã lưu lịch sử vẽ thành công.\"}", request.origin);
            } else {
                respond(client.get(), 500, "Internal Server Error", "{\"success\":false,\"message\":\"Không ghi được file lịch sử dự án.\"}", request.origin);
            }
        }
        else {
            respond(client.get(), 404, "Not Found", "{\"error\":\"Endpoint khong ton tai.\"}", request.origin);
        }
    }
}
