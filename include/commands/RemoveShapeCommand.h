#pragma once
#include <cstddef>
#include <memory>
#include "shapes/HinhHoc2D.h"
#include "commands/Command.h"

class BangVeCanvas;

// ============================================================================
// RemoveShapeCommand - xóa một hình khỏi layer (giữ lại để undo)
// Tầng: commands (kế thừa Command) | Phụ trách: Nhất Vũ
// ============================================================================
class RemoveShapeCommand : public Command {
private:
    BangVeCanvas& canvas_;      // tham chiếu, KHÔNG sở hữu
    std::size_t layerIdx_;
    int id_;
    std::size_t viTri_;         // vị trí cũ trong layer (để undo chèn lại đúng chỗ)
    std::unique_ptr<HinhHoc2D> hinh_;   // giữ hình đã xóa

public:
    RemoveShapeCommand(BangVeCanvas& cv, std::size_t layerIdx, int id);
    void execute() override;
    void undo() override;
};
