#include "commands/TransformCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/HinhHoc2D.h"
#include <stdexcept>

TransformCommand::TransformCommand(BangVeCanvas& canvas, size_t layerIdx, int shapeId, const BienDoi2D& bd)
    : canvas_(canvas), layerIdx_(layerIdx), shapeId_(shapeId), bd_(bd) {}

void TransformCommand::execute() {
    Layer& lay = canvas_.layer(layerIdx_);
    HinhHoc2D* shape = lay.timTheoId(shapeId_);
    if (!shape) throw std::out_of_range("Khong tim thay hinh ID #" + std::to_string(shapeId_) + " de bien doi.");
    shape->transform(bd_);
}

void TransformCommand::undo() {
    Layer& lay = canvas_.layer(layerIdx_);
    HinhHoc2D* shape = lay.timTheoId(shapeId_);
    if (!shape) throw std::out_of_range("Khong tim thay hinh ID #" + std::to_string(shapeId_) + " de undo bien doi.");
    shape->transform(bd_.nghichDao());
}
