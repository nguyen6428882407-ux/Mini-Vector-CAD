#include "auth/UserManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstring>

namespace {

// SHA-256 Implementation (FIPS 180-4 standard)
inline std::uint32_t rotr(std::uint32_t x, std::uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline std::uint32_t sigma0(std::uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline std::uint32_t sigma1(std::uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline std::uint32_t gamma0(std::uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline std::uint32_t gamma1(std::uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

const std::uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string escapeField(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '\t') res += "\\t";
        else if (c == '\n') res += "\\n";
        else if (c == '\\') res += "\\\\";
        else res += c;
    }
    return res;
}

std::string unescapeField(const std::string& s) {
    std::string res;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            if (s[i + 1] == 't') { res += '\t'; ++i; }
            else if (s[i + 1] == 'n') { res += '\n'; ++i; }
            else if (s[i + 1] == '\\') { res += '\\'; ++i; }
            else { res += s[i]; }
        } else {
            res += s[i];
        }
    }
    return res;
}

} // namespace

std::string UserManager::sha256(const std::string& input) {
    std::uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    std::vector<std::uint8_t> data(input.begin(), input.end());
    std::uint64_t bitLen = data.size() * 8;

    // Append 0x80 byte
    data.push_back(0x80);

    // Append 0x00 padding until length = 56 mod 64
    while ((data.size() % 64) != 56) {
        data.push_back(0x00);
    }

    // Append original length in bits as 64-bit big endian integer
    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<std::uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    // Process blocks of 64 bytes
    for (std::size_t offset = 0; offset < data.size(); offset += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(data[offset + i * 4]) << 24) |
                   (static_cast<std::uint32_t>(data[offset + i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(data[offset + i * 4 + 2]) << 8) |
                   (static_cast<std::uint32_t>(data[offset + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        std::uint32_t a = h[0];
        std::uint32_t b = h[1];
        std::uint32_t c = h[2];
        std::uint32_t d = h[3];
        std::uint32_t e = h[4];
        std::uint32_t f = h[5];
        std::uint32_t g = h[6];
        std::uint32_t hVal = h[7];

        for (int i = 0; i < 64; ++i) {
            std::uint32_t t1 = hVal + sigma1(e) + ch(e, f, g) + K[i] + w[i];
            std::uint32_t t2 = sigma0(a) + maj(a, b, c);
            hVal = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hVal;
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        oss << std::setw(8) << h[i];
    }
    return oss.str();
}

std::string UserManager::generateSalt(std::size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> dis(0, sizeof(charset) - 2);

    std::string salt;
    salt.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        salt += charset[dis(gen)];
    }
    return salt;
}

std::string UserManager::generateToken() {
    return sha256(generateSalt(32) + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
}

UserManager::UserManager(const std::string& storagePath)
    : storagePath_(storagePath) {
    loadFromFile();
}

void UserManager::loadFromFile() {
    std::ifstream ifs(storagePath_);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string username, hash, salt, fullName, createdAt;
        if (std::getline(iss, username, '\t') &&
            std::getline(iss, hash, '\t') &&
            std::getline(iss, salt, '\t') &&
            std::getline(iss, fullName, '\t') &&
            std::getline(iss, createdAt)) {
            UserAccount u;
            u.username = unescapeField(username);
            u.passwordHash = hash;
            u.salt = salt;
            u.fullName = unescapeField(fullName);
            u.createdAt = unescapeField(createdAt);
            users_[u.username] = u;
        }
    }
}

void UserManager::saveToFile() {
    std::ofstream ofs(storagePath_, std::ios::trunc);
    if (!ofs.is_open()) return;

    ofs << "# MINI_VECTOR_CAD_USERS_V1\n";
    ofs << "# username\tpasswordHash\tsalt\tfullName\tcreatedAt\n";
    for (const auto& pair : users_) {
        const auto& u = pair.second;
        ofs << escapeField(u.username) << "\t"
            << u.passwordHash << "\t"
            << u.salt << "\t"
            << escapeField(u.fullName) << "\t"
            << escapeField(u.createdAt) << "\n";
    }
}

bool UserManager::registerUser(const std::string& username,
                              const std::string& password,
                              const std::string& fullName,
                              std::string& outToken,
                              UserAccount& outUser,
                              std::string& outError) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validation
    if (username.size() < 3 || username.size() > 32) {
        outError = "Tên đăng nhập phải có độ dài từ 3 đến 32 ký tự.";
        return false;
    }
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            outError = "Tên đăng nhập chỉ được chứa chữ cái, số, dấu gạch dưới (_) hoặc gạch ngang (-).";
            return false;
        }
    }
    if (password.size() < 6) {
        outError = "Mật khẩu phải có ít nhất 6 ký tự.";
        return false;
    }
    if (users_.find(username) != users_.end()) {
        outError = "Tên đăng nhập \"" + username + "\" đã tồn tại. Vui lòng chọn tên khác.";
        return false;
    }

    std::string salt = generateSalt(16);
    std::string hash = sha256(salt + password);

    UserAccount newUser;
    newUser.username = username;
    newUser.passwordHash = hash;
    newUser.salt = salt;
    newUser.fullName = fullName.empty() ? username : fullName;
    newUser.createdAt = getCurrentTimestamp();

    users_[username] = newUser;
    saveToFile();

    std::string token = generateToken();
    sessions_[token] = username;

    outToken = token;
    outUser = newUser;
    return true;
}

bool UserManager::loginUser(const std::string& username,
                           const std::string& password,
                           std::string& outToken,
                           UserAccount& outUser,
                           std::string& outError) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_.find(username);
    if (it == users_.end()) {
        outError = "Tên đăng nhập hoặc mật khẩu không chính xác.";
        return false;
    }

    const auto& user = it->second;
    std::string computedHash = sha256(user.salt + password);
    if (computedHash != user.passwordHash) {
        outError = "Tên đăng nhập hoặc mật khẩu không chính xác.";
        return false;
    }

    std::string token = generateToken();
    sessions_[token] = username;

    outToken = token;
    outUser = user;
    return true;
}

bool UserManager::validateToken(const std::string& token, UserAccount& outUser) {
    if (token.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it == sessions_.end()) return false;

    auto userIt = users_.find(it->second);
    if (userIt == users_.end()) return false;

    outUser = userIt->second;
    return true;
}

bool UserManager::logout(const std::string& token) {
    if (token.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.erase(token) > 0;
}

bool UserManager::userExists(const std::string& username) const {
    if (username.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.find(username) != users_.end();
}

bool UserManager::findUser(const std::string& query, UserAccount& outUser) const {
    if (query.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Direct username lookup
    auto it = users_.find(query);
    if (it != users_.end()) {
        outUser = it->second;
        return true;
    }

    // 2. Case-insensitive lookup by username or fullName
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    for (const auto& pair : users_) {
        std::string lowerUser = pair.second.username;
        std::transform(lowerUser.begin(), lowerUser.end(), lowerUser.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lowerUser == lowerQuery) {
            outUser = pair.second;
            return true;
        }

        std::string lowerName = pair.second.fullName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (!lowerName.empty() && lowerName == lowerQuery) {
            outUser = pair.second;
            return true;
        }
    }

    return false;
}

std::size_t UserManager::userCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.size();
}
