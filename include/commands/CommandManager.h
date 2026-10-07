#pragma once
#include <vector>
#include <memory>
#include "commands/Command.h"

class CommandManager {
private:
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;

public:
    CommandManager() = default;
    void thucThi(std::unique_ptr<Command> cmd);
    bool coTheUndo() const;
    bool coTheRedo() const;
    bool undo();
    bool redo();
    void xoaLichSu();
};
