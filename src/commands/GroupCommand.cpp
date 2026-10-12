#include "commands/GroupCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/GroupShape.h"
#include <stdexcept>
#include <string>

GroupCommand::GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, const std::vector<int>& shapeIds, Loai loai)
    : canvas_(canvas),
      layerIdx_(layerIdx),
      loai_(loai),
      shapeIds_(shapeIds),
      group_(nullptr),
      groupId_(-1),
      daTaoNhomLanthoi_(false) {}

GroupCommand::GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, int groupId, Loai loai)
    : canvas_(canvas),
      layerIdx_(layerIdx),
      loai_(loai),
      shapeIds_(),
      group_(nullptr),
      groupId_(groupId),
      daTaoNhomLanthoi_(false) {}

GroupCommand::Loai GroupCommand::getLoai() const {
    return loai_;
}

int GroupCommand::getGroupId() const {
    return groupId_;
}

const std::vector<int>& GroupCommand::getShapeIds() const {
    return shapeIds_;
}

void GroupCommand::execute() {
    Layer& lay = canvas_.layer(layerIdx_);

    if (loai_ == Loai::GOM) {
        if (!daTaoNhomLanthoi_) {
            // 1. Kiem tra danh sach ID hop le
            if (shapeIds_.empty()) {
                throw std::invalid_argument("GroupCommand::execute: Danh sach ID gom nhom bi rong!");
            }
            for (int id : shapeIds_) {
                if (!lay.timTheoId(id)) {
                    throw std::out_of_range("GroupCommand::execute: Hinh ID #" + std::to_string(id) + " khong ton tai!");
                }
            }

            // 2. Tao GroupShape moi
            group_ = std::make_unique<GroupShape>();
            groupId_ = group_->getId();

            // 3. Rut tung hinh con ra khoi Layer va dua vao GroupShape
            for (int id : shapeIds_) {
                auto h = lay.layHinh(id);
                if (!h) {
                    throw std::out_of_range("GroupCommand::execute: Loi khi lay hinh ID #" + std::to_string(id));
                }
                group_->themHinh(std::move(h));
            }

            daTaoNhomLanthoi_ = true;
            lay.themHinh(std::move(group_));
        } else {
            // Redo Gom nhom: group_ duoc giu lai (dung groupId cu), rut cac hinh con tu Layer bo vao group_
            if (!group_) {
                throw std::logic_error("GroupCommand::execute: group_ bi nullptr khi redo gom nhom!");
            }
            for (int id : shapeIds_) {
                auto h = lay.layHinh(id);
                if (!h) {
                    throw std::out_of_range("GroupCommand::redo: Khong tim thay hinh ID #" + std::to_string(id));
                }
                group_->themHinh(std::move(h));
            }
            lay.themHinh(std::move(group_));
        }
    } else { // Loai::RA_NHOM
        if (!daTaoNhomLanthoi_) {
            // 1. Rut group can ra khoi Layer
            auto nhomRutRa = lay.layHinh(groupId_);
            if (!nhomRutRa) {
                throw std::out_of_range("GroupCommand::execute: Khong tim thay nhom ID #" + std::to_string(groupId_) + " de ra nhom!");
            }
            GroupShape* gs = dynamic_cast<GroupShape*>(nhomRutRa.get());
            if (!gs) {
                // Tra lai vao layer neu khong phai GroupShape de tranh mat mat doi tuong
                lay.themHinh(std::move(nhomRutRa));
                throw std::invalid_argument("GroupCommand::execute: Doi tuong ID #" + std::to_string(groupId_) + " khong phai GroupShape!");
            }
            group_ = std::unique_ptr<GroupShape>(static_cast<GroupShape*>(nhomRutRa.release()));

            // 2. Luu danh sach ID cua cac hinh con
            shapeIds_.clear();
            for (const auto& con : group_->getDanhSachCon()) {
                if (con) shapeIds_.push_back(con->getId());
            }

            // 3. Dua tung hinh con ra Layer
            for (int id : shapeIds_) {
                auto con = group_->rutHinh(id);
                if (con) lay.themHinh(std::move(con));
            }

            daTaoNhomLanthoi_ = true;
        } else {
            // Redo Ra nhom: rut group_ khoi Layer va dua cac hinh con ra
            auto nhomRutRa = lay.layHinh(groupId_);
            if (!nhomRutRa) {
                throw std::out_of_range("GroupCommand::redo: Khong tim thay nhom ID #" + std::to_string(groupId_) + " de ra nhom!");
            }
            group_ = std::unique_ptr<GroupShape>(static_cast<GroupShape*>(nhomRutRa.release()));

            for (int id : shapeIds_) {
                auto con = group_->rutHinh(id);
                if (con) lay.themHinh(std::move(con));
            }
        }
    }
}

void GroupCommand::undo() {
    Layer& lay = canvas_.layer(layerIdx_);

    if (loai_ == Loai::GOM) {
        // Undo Gom nhom: rut group_ ra khoi Layer, tra cac hinh con ve Layer
        auto nhomRutRa = lay.layHinh(groupId_);
        if (!nhomRutRa) {
            throw std::out_of_range("GroupCommand::undo: Khong tim thay nhom ID #" + std::to_string(groupId_));
        }
        GroupShape* gs = dynamic_cast<GroupShape*>(nhomRutRa.get());
        if (!gs) {
            lay.themHinh(std::move(nhomRutRa));
            throw std::logic_error("GroupCommand::undo: Doi tuong khong phai GroupShape!");
        }
        group_ = std::unique_ptr<GroupShape>(static_cast<GroupShape*>(nhomRutRa.release()));

        for (int id : shapeIds_) {
            auto con = group_->rutHinh(id);
            if (!con) {
                throw std::out_of_range("GroupCommand::undo: Khong tim thay hinh con ID #" + std::to_string(id));
            }
            lay.themHinh(std::move(con));
        }
    } else { // Loai::RA_NHOM
        // Undo Ra nhom: gom lai cac hinh con tu Layer vao group_, dua group_ ve Layer
        if (!group_) {
            throw std::logic_error("GroupCommand::undo: group_ bi nullptr khi undo ra nhom!");
        }
        for (int id : shapeIds_) {
            auto con = lay.layHinh(id);
            if (!con) {
                throw std::out_of_range("GroupCommand::undo: Khong tim thay hinh ID #" + std::to_string(id) + " de phuc hoi nhom!");
            }
            group_->themHinh(std::move(con));
        }
        lay.themHinh(std::move(group_));
    }
}
