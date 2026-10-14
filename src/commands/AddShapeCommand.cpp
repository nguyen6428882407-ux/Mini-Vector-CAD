#include "commands/AddShapeCommand.h"
#include <stdexcept>
#include "canvas/BangVeCanvas.h"

// Lưu ý: id_ phải khởi tạo TRƯỚC hinh_ (đúng thứ tự khai báo) vì hinh_ sẽ bị move.
AddShapeCommand::AddShapeCommand(BangVeCanvas& cv, std::size_t layerIdx, std::unique_ptr<HinhHoc2D> h)
    : canvas_(cv),
      layerIdx_(layerIdx),
      id_(h ? h->getId() : -1),
      hinh_(std::move(h)) {}

void AddShapeCommand::execute() {
    Layer& ly = canvas_.layer(layerIdx_);   // layerIdx_ sai -> out_of_range
    if (!hinh_) throw std::logic_error("AddShapeCommand::execute: hinh da nam trong layer (execute 2 lan lien tiep)");
    ly.themHinh(std::move(hinh_));          // id_ giu nguyen khi redo
}

void AddShapeCommand::undo() {
    Layer& ly = canvas_.layer(layerIdx_);
    std::unique_ptr<HinhHoc2D> ra = ly.layHinh(id_);   // nullptr = chua doi gi
    if (!ra) throw std::out_of_range("AddShapeCommand::undo: khong tim thay hinh trong layer");
    hinh_ = std::move(ra);
}
