#include "canvas/BangVeCanvas.h"
#include "commands/CommandManager.h"
#include "controller/MenuController.h"
#include <iostream>

int main() {
    std::cout << "Khoi tao he thong Mini Vector CAD...\n";
    try {
        BangVeCanvas canvas;
        CommandManager cmdMgr;
        MenuController app(canvas, cmdMgr);
        app.chay();
    } catch (const std::exception& e) {
        std::cerr << "Loi nghiem trong: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
