#pragma once
#include <cstddef>
#include "core/BienDoi2D.h"
#include "commands/Command.h"

class BangVeCanvas;

// ============================================================================
// TransformCommand - biến đổi một hình (tịnh tiến / xoay / scale đều)
// Tầng: commands (kế thừa Command) | Phụ trách: Nhất Vũ
// ============================================================================
class TransformCommand : public Command {
private:
    BangVeCanvas& canvas_;      // tham chiếu, KHÔNG sở hữu
    std::size_t layerIdx_;
    int id_;
    BienDoi2D bienDoi_;         // phép biến đổi gốc
    BienDoi2D nghichDao_;       // phép nghịch đảo, tính sẵn trong constructor (dùng cho undo)

public:
    // bd phải khả nghịch (không scale với k = 0): MenuController chịu trách nhiệm kiểm tra.
    TransformCommand(BangVeCanvas& cv, std::size_t layerIdx, int id, const BienDoi2D& bd);
    void execute() override;
    void undo() override;
};
