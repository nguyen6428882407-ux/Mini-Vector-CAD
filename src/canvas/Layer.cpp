#include "canvas/Layer.h"
#include <algorithm>

Layer::Layer(const std::string& ten)
    : ten_(ten), hienThi_(true), khoa_(false) {}

const std::string& Layer::getTen() const { return ten_; }
void Layer::setTen(const std::string& ten) { ten_ = ten; }
bool Layer::isHienThi() const { return hienThi_; }
void Layer::setHienThi(bool hienThi) { hienThi_ = hienThi; }
bool Layer::isKhoa() const { return khoa_; }
void Layer::setKhoa(bool khoa) { khoa_ = khoa; }

void Layer::themHinh(std::unique_ptr<HinhHoc2D> hinh) {
    if (hinh) danhSachHinh_.push_back(std::move(hinh));
}

void Layer::chenHinh(size_t viTri, std::unique_ptr<HinhHoc2D> hinh) {
    if (!hinh) return;
    if (viTri >= danhSachHinh_.size()) {
        danhSachHinh_.push_back(std::move(hinh));
    } else {
        danhSachHinh_.insert(danhSachHinh_.begin() + viTri, std::move(hinh));
    }
}

std::unique_ptr<HinhHoc2D> Layer::rutHinh(int id) {
    for (auto it = danhSachHinh_.begin(); it != danhSachHinh_.end(); ++it) {
        if ((*it)->getId() == id) {
            auto hinh = std::move(*it);
            danhSachHinh_.erase(it);
            return hinh;
        }
    }
    return nullptr;
}

HinhHoc2D* Layer::timTheoId(int id) const {
    for (const auto& hinh : danhSachHinh_) {
        if (hinh && hinh->getId() == id) return hinh.get();
    }
    return nullptr;
}

int Layer::timViTri(int id) const {
    for (size_t i = 0; i < danhSachHinh_.size(); ++i) {
        if (danhSachHinh_[i] && danhSachHinh_[i]->getId() == id) return static_cast<int>(i);
    }
    return -1;
}

const std::vector<std::unique_ptr<HinhHoc2D>>& Layer::getDanhSachHinh() const {
    return danhSachHinh_;
}

size_t Layer::soLuongHinh() const {
    return danhSachHinh_.size();
}
