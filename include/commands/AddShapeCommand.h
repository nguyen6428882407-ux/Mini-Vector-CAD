#pragma once
#include "commands/Command.h"
#include <memory>
#include <cstddef>

class BangVeCanvas;
class HinhHoc2D;

class AddShapeCommand : public Command {
private:
    BangVeCanvas&               canvas_;
    size_t                      layerIdx_;
    std::unique_ptr<HinhHoc2D>  shape_;
    int                         shapeId_;

public:
    AddShapeCommand(BangVeCanvas& canvas, size_t layerIdx, std::unique_ptr<HinhHoc2D> shape);
    void execute() override;
    void undo() override;
};
