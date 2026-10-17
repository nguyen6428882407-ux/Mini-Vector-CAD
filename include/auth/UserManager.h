#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>

struct UserAccount {
    std::string username;
    std::string passwordHash; // sha256(salt + password)
    std::string salt;
    std::string fullName;
    std::string createdAt;
};

class UserManager {
private:
    std::string storagePath_;
    std::unordered_map<std::string, UserAccount> users_;      // username -> UserAccount
    std::unordered_map<std::string, std::string> sessions_;   // token -> username
    mutable std::mutex mutex_;

    void loadFromFile();
    void saveToFile();

public:
    explicit UserManager(const std::string& storagePath = "output/users.txt");

    bool registerUser(const std::string& username,
                      const std::string& password,
                      const std::string& fullName,
                      std::string& outToken,
                      UserAccount& outUser,
                      std::string& outError);

    bool loginUser(const std::string& username,
                   const std::string& password,
                   std::string& outToken,
                   UserAccount& outUser,
                   std::string& outError);

    bool validateToken(const std::string& token, UserAccount& outUser);

    bool logout(const std::string& token);

    bool userExists(const std::string& username) const;

    bool findUser(const std::string& query, UserAccount& outUser) const;

    std::size_t userCount() const;

    static std::string sha256(const std::string& input);
    static std::string generateSalt(std::size_t length = 16);
    static std::string generateToken();
};
