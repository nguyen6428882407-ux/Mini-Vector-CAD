#pragma once
#include <cstddef>
#include <memory>
#include "shapes/HinhHoc2D.h"
#include "commands/Command.h"

class BangVeCanvas;

// ============================================================================
// AddShapeCommand - thêm một hình mới vào layer
// Tầng: commands (kế thừa Command) | Phụ trách: Nhất Vũ
// ============================================================================
class AddShapeCommand : public Command {
private:
    BangVeCanvas& canvas_;      // tham chiếu, KHÔNG sở hữu
    std::size_t layerIdx_;
    int id_;                    // id của hình (lấy lúc tạo lệnh)
    std::unique_ptr<HinhHoc2D> hinh_;   // giữ hình khi CHƯA nằm trong layer

public:
    AddShapeCommand(BangVeCanvas& cv, std::size_t layerIdx, std::unique_ptr<HinhHoc2D> h);
    void execute() override;
    void undo() override;
};
