#include "web/ProjectServer.h"
#include <exception>
#include <iostream>

int main() {
    try {
        ProjectServer server;
        server.chay();
    } catch (const std::exception& error) {
        std::cerr << "[Loi] C++ server khong the khoi dong: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
