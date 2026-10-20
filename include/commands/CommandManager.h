#pragma once
#include <vector>
#include <memory>
#include <cstddef>
#include "commands/Command.h"

// ============================================================================
// CommandManager - Quan ly lich su thuc thi va Undo/Redo (Command Pattern)
// Nhiem vu:
//  - Quan ly 2 stack: undoStack_ va redoStack_.
//  - thucThi(cmd): thuc thi lenh moi, day vao undoStack_, xoa sach redoStack_.
//  - undo(): hoan tac lenh gan nhat, chuyen sang redoStack_.
//  - redo(): lam lai lenh vua hoan tac, chuyen tro lai undoStack_.
//  - maxHistorySize_: gioi han so luong buoc undo toi da de tiet kiem bo nho RAM.
// ============================================================================
class CommandManager {
private:
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;
    std::size_t maxHistorySize_{0}; // 0 = khong gioi han

public:
    CommandManager() = default;
    explicit CommandManager(std::size_t maxHistory);

    // Thuc thi mot lenh moi: chay execute(), them vao undoStack, xoa sach redoStack
    void thucThi(std::unique_ptr<Command> cmd);

    // Kiem tra trang thai co the Undo/Redo
    bool coTheUndo() const;
    bool coTheRedo() const;

    // Thuc hien Undo / Redo
    bool undo();
    bool redo();

    // Xoa sach lich su lenh
    void xoaLichSu();

    // So luong lenh trong cac stack
    std::size_t soLenhUndo() const;
    std::size_t soLenhRedo() const;

    // Cau hinh gioi han dung luong lich su (0 = vo han)
    void setMaxHistorySize(std::size_t maxSize);
    std::size_t getMaxHistorySize() const;
};
