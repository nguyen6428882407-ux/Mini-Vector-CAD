#include "commands/CommandManager.h"

CommandManager::CommandManager(std::size_t maxHistory)
    : maxHistorySize_(maxHistory) {}

void CommandManager::thucThi(std::unique_ptr<Command> cmd) {
    if (!cmd) return;
    cmd->execute(); // Neu execute quang exception, lenh chua duoc them vao undoStack
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear(); // Lenh moi thi xoa sach redo stack

    // Gioi han dung luong undo neu maxHistorySize_ > 0 de chong tran RAM
    if (maxHistorySize_ > 0 && undoStack_.size() > maxHistorySize_) {
        undoStack_.erase(undoStack_.begin());
    }
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

void CommandManager::setMaxHistorySize(std::size_t maxSize) {
    maxHistorySize_ = maxSize;
    if (maxHistorySize_ > 0 && undoStack_.size() > maxHistorySize_) {
        const std::size_t excess = undoStack_.size() - maxHistorySize_;
        undoStack_.erase(undoStack_.begin(), undoStack_.begin() + static_cast<std::ptrdiff_t>(excess));
    }
}

std::size_t CommandManager::getMaxHistorySize() const {
    return maxHistorySize_;
}
