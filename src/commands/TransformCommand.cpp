#include "commands/TransformCommand.h"
#include <stdexcept>
#include "canvas/BangVeCanvas.h"

TransformCommand::TransformCommand(BangVeCanvas& cv, std::size_t layerIdx, int id, const BienDoi2D& bd)
    : canvas_(cv),
      layerIdx_(layerIdx),
      id_(id),
      bienDoi_(bd),
      nghichDao_(bd.nghichDao()) {}   // ném std::domain_error nếu bd không khả nghịch

void TransformCommand::execute() {
    HinhHoc2D* h = canvas_.layer(layerIdx_).timTheoId(id_);
    if (!h) throw std::out_of_range("TransformCommand::execute: khong tim thay hinh trong layer");
    h->transform(bienDoi_);
}

void TransformCommand::undo() {
    HinhHoc2D* h = canvas_.layer(layerIdx_).timTheoId(id_);
    if (!h) throw std::out_of_range("TransformCommand::undo: khong tim thay hinh trong layer");
    h->transform(nghichDao_);
}
