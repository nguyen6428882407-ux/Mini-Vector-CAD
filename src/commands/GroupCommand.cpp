#include "commands/GroupCommand.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/GroupShape.h"
#include <algorithm>
#include <stdexcept>
#include <string>

GroupCommand::GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, const std::vector<int>& shapeIds, Loai loai)
    : canvas_(canvas),
      layerIdx_(layerIdx),
      loai_(loai),
      shapeIds_(shapeIds),
      shapePositions_(),
      group_(nullptr),
      groupId_(-1),
      groupPosition_(Layer::npos),
      daTaoNhomLanthoi_(false) {}

GroupCommand::GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, Loai loai, const std::vector<int>& shapeIds)
    : canvas_(canvas),
      layerIdx_(layerIdx),
      loai_(loai),
      shapeIds_(loai == Loai::GOM ? shapeIds : std::vector<int>{}),
      shapePositions_(),
      group_(nullptr),
      groupId_(loai == Loai::RA_NHOM && shapeIds.size() == 1 ? shapeIds.front() : -1),
      groupPosition_(Layer::npos),
      daTaoNhomLanthoi_(false) {
    if (loai == Loai::RA_NHOM && shapeIds.size() != 1) {
        throw std::invalid_argument("GroupCommand: Ra nhom can dung mot ID nhom.");
    }
}

GroupCommand::GroupCommand(BangVeCanvas& canvas, std::size_t layerIdx, int groupId, Loai loai)
    : canvas_(canvas),
      layerIdx_(layerIdx),
      loai_(loai),
      shapeIds_(),
      shapePositions_(),
      group_(nullptr),
      groupId_(groupId),
      groupPosition_(Layer::npos),
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
            if (shapeIds_.empty()) {
                throw std::invalid_argument("GroupCommand::execute: Danh sach ID gom nhom bi rong!");
            }
            std::vector<std::pair<std::size_t, int>> selected;
            selected.reserve(shapeIds_.size());
            for (std::size_t i = 0; i < shapeIds_.size(); ++i) {
                for (std::size_t j = i + 1; j < shapeIds_.size(); ++j) {
                    if (shapeIds_[i] == shapeIds_[j]) {
                        throw std::invalid_argument("GroupCommand::execute: ID bi trung lap trong danh sach gom nhom!");
                    }
                }
                const std::size_t position = lay.viTriTheoId(shapeIds_[i]);
                if (position == Layer::npos) {
                    const int id = shapeIds_[i];
                    throw std::out_of_range("GroupCommand::execute: Hinh ID #" + std::to_string(id) + " khong ton tai!");
                }
                selected.emplace_back(position, shapeIds_[i]);
            }
            std::sort(selected.begin(), selected.end());
            shapeIds_.clear();
            shapePositions_.clear();
            for (const auto& item : selected) {
                shapePositions_.push_back(item.first);
                shapeIds_.push_back(item.second);
            }

            group_ = std::make_unique<GroupShape>();
            groupId_ = group_->getId();
            groupPosition_ = shapePositions_.front();

            for (int id : shapeIds_) {
                auto h = lay.layHinh(id);
                if (!h) {
                    throw std::out_of_range("GroupCommand::execute: Loi khi lay hinh ID #" + std::to_string(id));
                }
                group_->themHinh(std::move(h));
            }

            daTaoNhomLanthoi_ = true;
            lay.themHinh(std::move(group_), groupPosition_);
        } else {
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
            lay.themHinh(std::move(group_), groupPosition_);
        }
    } else { // Loai::RA_NHOM
        if (!daTaoNhomLanthoi_) {
            groupPosition_ = lay.viTriTheoId(groupId_);
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

            for (std::size_t i = 0; i < shapeIds_.size(); ++i) {
                int id = shapeIds_[i];
                auto con = group_->rutHinh(id);
                if (con) lay.themHinh(std::move(con), groupPosition_ + i);
            }

            daTaoNhomLanthoi_ = true;
        } else {
            auto nhomRutRa = lay.layHinh(groupId_);
            if (!nhomRutRa) {
                throw std::out_of_range("GroupCommand::redo: Khong tim thay nhom ID #" + std::to_string(groupId_) + " de ra nhom!");
            }
            auto* groupShape = dynamic_cast<GroupShape*>(nhomRutRa.get());
            if (!groupShape) {
                lay.themHinh(std::move(nhomRutRa), groupPosition_);
                throw std::logic_error("GroupCommand::redo: Doi tuong khong phai GroupShape!");
            }
            group_ = std::unique_ptr<GroupShape>(static_cast<GroupShape*>(nhomRutRa.release()));

            for (std::size_t i = 0; i < shapeIds_.size(); ++i) {
                int id = shapeIds_[i];
                auto con = group_->rutHinh(id);
                if (con) lay.themHinh(std::move(con), groupPosition_ + i);
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

        for (std::size_t i = 0; i < shapeIds_.size(); ++i) {
            int id = shapeIds_[i];
            auto con = group_->rutHinh(id);
            if (!con) {
                throw std::out_of_range("GroupCommand::undo: Khong tim thay hinh con ID #" + std::to_string(id));
            }
            lay.themHinh(std::move(con), shapePositions_[i]);
        }
    } else { // Loai::RA_NHOM
        // Undo Ra nhom: gom lai cac hinh con tu Layer vao group_, dua group_ ve Layer
        if (!group_) {
            throw std::logic_error("GroupCommand::undo: group_ bi nullptr khi undo ra nhom!");
        }
        for (std::size_t i = 0; i < shapeIds_.size(); ++i) {
            int id = shapeIds_[i];
            auto con = lay.layHinh(id);
            if (!con) {
                throw std::out_of_range("GroupCommand::undo: Khong tim thay hinh ID #" + std::to_string(id) + " de phuc hoi nhom!");
            }
            group_->themHinh(std::move(con));
        }
        lay.themHinh(std::move(group_), groupPosition_);
    }
}
