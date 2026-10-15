#include "web/ProjectServer.h"
#include "canvas/BangVeCanvas.h"
#include "io/TextSerializer.h"
#include <algorithm>
#include <cctype>
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

struct Request {
    std::string method;
    std::string path;
    std::string origin;
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
        error = "Du lieu ban ve vuot qua gioi han 20 MB.";
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

bool isAllowedOrigin(const std::string& origin) {
    return origin.empty()
        || origin == "http://127.0.0.1:8765"
        || origin == "http://localhost:8765";
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
             const std::string& origin, const std::string& contentType = "text/plain; charset=utf-8") {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << reason << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-store\r\n";
    if (!origin.empty() && isAllowedOrigin(origin)) {
        response << "Access-Control-Allow-Origin: " << origin << "\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type\r\n"
                 << "Vary: Origin\r\n";
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
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
        throw std::runtime_error("Khong cau hinh duoc dia chi localhost.");
    }
    if (bind(listener.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        throw std::runtime_error("Khong bind duoc 127.0.0.1:" + std::to_string(port) + " (cong co the dang duoc su dung).");
    }
    if (listen(listener.get(), 8) != 0) {
        throw std::runtime_error("Khong mo duoc TCP listener.");
    }

    auto project = std::make_unique<BangVeCanvas>(800.0, 600.0);
    project->themLayer("Layer 1");
    std::cout << "C++ backend dang lang nghe tai http://127.0.0.1:" << port
              << "/api/project (Ctrl+C de dung)\n";

    while (true) {
        Socket clientSocket = accept(listener.get(), nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET_VALUE) continue;
        SocketGuard client(clientSocket);
        setReceiveTimeout(client.get());

        Request request;
        std::string requestError;
        if (!receiveRequest(client.get(), request, requestError)) {
            respond(client.get(), 400, "Bad Request", requestError, "");
            continue;
        }
        if (!isAllowedOrigin(request.origin)) {
            respond(client.get(), 403, "Forbidden", "Origin khong duoc phep.", "");
            continue;
        }
        if (request.method == "OPTIONS" && request.path == "/api/project") {
            respond(client.get(), 204, "No Content", "", request.origin);
        } else if (request.path != "/api/project") {
            respond(client.get(), 404, "Not Found", "Endpoint khong ton tai.", request.origin);
        } else if (request.method == "GET") {
            std::ostringstream body;
            TextSerializer::serialize(*project, body);
            respond(client.get(), 200, "OK", body.str(), request.origin);
        } else if (request.method == "POST") {
            try {
                std::istringstream input(request.body);
                auto stagedProject = std::make_unique<BangVeCanvas>(800.0, 600.0);
                TextSerializer::deserialize(*stagedProject, input);
                if (input.fail() || stagedProject->soLayer() == 0) {
                    respond(client.get(), 400, "Bad Request", "Du lieu ban ve khong hop le.", request.origin);
                    continue;
                }
                project = std::move(stagedProject);
                std::ostringstream body;
                TextSerializer::serialize(*project, body);
                respond(client.get(), 200, "OK", body.str(), request.origin);
                std::cout << "Da nhan va kiem tra ban ve tu giao dien web.\n";
            } catch (const std::exception& error) {
                respond(client.get(), 400, "Bad Request", error.what(), request.origin);
            }
        } else {
            respond(client.get(), 405, "Method Not Allowed", "Chi ho tro GET, POST va OPTIONS.", request.origin);
        }
    }
}
