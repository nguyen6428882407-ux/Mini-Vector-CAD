#include "commands/RemoveShapeCommand.h"
#include <stdexcept>
#include "canvas/BangVeCanvas.h"

RemoveShapeCommand::RemoveShapeCommand(BangVeCanvas& cv, std::size_t layerIdx, int id)
    : canvas_(cv), layerIdx_(layerIdx), id_(id), viTri_(Layer::npos) {}

void RemoveShapeCommand::execute() {
    Layer& ly = canvas_.layer(layerIdx_);
    const std::size_t vt = ly.viTriTheoId(id_);
    if (vt == Layer::npos) throw std::out_of_range("RemoveShapeCommand::execute: khong tim thay hinh trong layer");
    hinh_ = ly.layHinh(id_);
    viTri_ = vt;
}

void RemoveShapeCommand::undo() {
    Layer& ly = canvas_.layer(layerIdx_);
    if (!hinh_) throw std::logic_error("RemoveShapeCommand::undo: chua xoa hinh nao (undo truoc execute)");
    ly.themHinh(std::move(hinh_), viTri_);   // chen lai dung vi tri cu
}
