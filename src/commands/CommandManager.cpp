#include "commands/CommandManager.h"

void CommandManager::thucThi(std::unique_ptr<Command> cmd) {
    if (!cmd) return;
    cmd->execute();
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear();
}

bool CommandManager::coTheUndo() const { return !undoStack_.empty(); }
bool CommandManager::coTheRedo() const { return !redoStack_.empty(); }

bool CommandManager::undo() {
    if (undoStack_.empty()) return false;
    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();
    cmd->undo();
    redoStack_.push_back(std::move(cmd));
    return true;
}

bool CommandManager::redo() {
    if (redoStack_.empty()) return false;
    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();
    cmd->execute();
    undoStack_.push_back(std::move(cmd));
    return true;
}

void CommandManager::xoaLichSu() {
    undoStack_.clear();
    redoStack_.clear();
}
