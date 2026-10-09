#include "commands/RemoveShapeCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/HinhHoc2D.h"
#include <stdexcept>

RemoveShapeCommand::RemoveShapeCommand(BangVeCanvas& canvas, size_t layerIdx, int shapeId)
    : canvas_(canvas), layerIdx_(layerIdx), shapeId_(shapeId), removedShape_(nullptr), viTriCu_(-1) {}

void RemoveShapeCommand::execute() {
    Layer& lay = canvas_.layer(layerIdx_);
    viTriCu_ = lay.timViTri(shapeId_);
    removedShape_ = lay.rutHinh(shapeId_);
    if (!removedShape_) throw std::out_of_range("Khong tim thay hinh ID #" + std::to_string(shapeId_) + " de xoa.");
}

void RemoveShapeCommand::undo() {
    Layer& lay = canvas_.layer(layerIdx_);
    if (!removedShape_) throw std::logic_error("Khong co du lieu hinh de undo RemoveShapeCommand.");
    if (viTriCu_ >= 0) lay.chenHinh(static_cast<size_t>(viTriCu_), std::move(removedShape_));
    else lay.themHinh(std::move(removedShape_));
}
