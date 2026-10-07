#pragma once
#include "commands/Command.h"
#include "core/BienDoi2D.h"
#include <cstddef>

class BangVeCanvas;

class TransformCommand : public Command {
private:
    BangVeCanvas& canvas_;
    size_t        layerIdx_;
    int           shapeId_;
    BienDoi2D     bd_;

public:
    TransformCommand(BangVeCanvas& canvas, size_t layerIdx, int shapeId, const BienDoi2D& bd);
    void execute() override;
    void undo() override;
};
