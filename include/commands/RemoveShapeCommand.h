#pragma once
#include "commands/Command.h"
#include <memory>
#include <cstddef>

class BangVeCanvas;
class HinhHoc2D;

class RemoveShapeCommand : public Command {
private:
    BangVeCanvas&               canvas_;
    size_t                      layerIdx_;
    int                         shapeId_;
    std::unique_ptr<HinhHoc2D>  removedShape_;
    int                         viTriCu_;

public:
    RemoveShapeCommand(BangVeCanvas& canvas, size_t layerIdx, int shapeId);
    void execute() override;
    void undo() override;
};
