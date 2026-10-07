#include "commands/GroupCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/GroupShape.h"
#include <stdexcept>

GroupCommand::GroupCommand(BangVeCanvas& canvas, size_t layerIdx, const std::vector<int>& shapeIds)
    : canvas_(canvas), layerIdx_(layerIdx), shapeIds_(shapeIds), group_(nullptr), groupId_(-1), daTaoNhomLanthoi_(false) {}

void GroupCommand::execute() {
    Layer& lay = canvas_.layer(layerIdx_);
    if (!daTaoNhomLanthoi_) {
        group_ = std::make_unique<GroupShape>();
        groupId_ = group_->getId();
        for (int id : shapeIds_) {
            auto hinh = lay.rutHinh(id);
            if (hinh) group_->themHinh(std::move(hinh));
        }
        daTaoNhomLanthoi_ = true;
        lay.themHinh(std::move(group_));
    } else {
        for (int id : shapeIds_) lay.rutHinh(id);
        lay.themHinh(std::move(group_));
    }
}

void GroupCommand::undo() {
    Layer& lay = canvas_.layer(layerIdx_);
    auto nhomRutRa = lay.rutHinh(groupId_);
    if (!nhomRutRa) throw std::out_of_range("Khong tim thay nhom de undo GroupCommand.");
    GroupShape* gs = dynamic_cast<GroupShape*>(nhomRutRa.get());
    if (gs) {
        group_ = std::unique_ptr<GroupShape>(static_cast<GroupShape*>(nhomRutRa.release()));
        for (const auto& con : group_->getDanhSachCon()) {
            if (con) lay.themHinh(con->clone());
        }
    }
}
