#include "web/ProjectServer.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        std::uint16_t port = 8766;
        if (const char* envPort = std::getenv("PORT")) {
            try {
                port = static_cast<std::uint16_t>(std::stoi(envPort));
            } catch (...) {
                port = 8766;
            }
        }
        ProjectServer server;
        server.chay(port);
    } catch (const std::exception& error) {
        std::cerr << "[Loi] C++ server khong the khoi dong: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
