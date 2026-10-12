#include "commands/CommandManager.h"

void CommandManager::thucThi(std::unique_ptr<Command> cmd) {
    if (!cmd) return;
    cmd->execute(); // Neu execute quang exception, lenh chua duoc them vao undoStack
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear(); // Lenh moi thi xoa sach redo stack
}

bool CommandManager::coTheUndo() const {
    return !undoStack_.empty();
}

bool CommandManager::coTheRedo() const {
    return !redoStack_.empty();
}

bool CommandManager::undo() {
    if (undoStack_.empty()) return false;
    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();

    try {
        cmd->undo();
        redoStack_.push_back(std::move(cmd));
        return true;
    } catch (...) {
        // Rollback lai stack neu undo gap loi ngoai le
        undoStack_.push_back(std::move(cmd));
        throw;
    }
}

bool CommandManager::redo() {
    if (redoStack_.empty()) return false;
    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();

    try {
        cmd->execute();
        undoStack_.push_back(std::move(cmd));
        return true;
    } catch (...) {
        // Rollback lai stack neu redo gap loi ngoai le
        redoStack_.push_back(std::move(cmd));
        throw;
    }
}

void CommandManager::xoaLichSu() {
    undoStack_.clear();
    redoStack_.clear();
}

std::size_t CommandManager::soLenhUndo() const {
    return undoStack_.size();
}

std::size_t CommandManager::soLenhRedo() const {
    return redoStack_.size();
}
