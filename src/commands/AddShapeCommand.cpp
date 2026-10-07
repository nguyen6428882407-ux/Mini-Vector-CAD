#include "commands/AddShapeCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/HinhHoc2D.h"
#include <stdexcept>

AddShapeCommand::AddShapeCommand(BangVeCanvas& canvas, size_t layerIdx, std::unique_ptr<HinhHoc2D> shape)
    : canvas_(canvas), layerIdx_(layerIdx), shape_(std::move(shape)), shapeId_(-1) {
    if (shape_) shapeId_ = shape_->getId();
}

void AddShapeCommand::execute() {
    Layer& lay = canvas_.layer(layerIdx_);
    if (!shape_) throw std::logic_error("Khong co doi tuong hinh de thuc thi AddShapeCommand.");
    shapeId_ = shape_->getId();
    lay.themHinh(std::move(shape_));
}

void AddShapeCommand::undo() {
    Layer& lay = canvas_.layer(layerIdx_);
    shape_ = lay.rutHinh(shapeId_);
    if (!shape_) throw std::out_of_range("Khong tim thay hinh de undo AddShapeCommand.");
}
