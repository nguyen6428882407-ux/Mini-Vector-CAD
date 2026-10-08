#include "canvas/Layer.h"
#include <cstddef>
#include <utility>

Layer::Layer(const std::string& ten) : ten_(ten), hien_(true), khoa_(false) {}

void Layer::themHinh(std::unique_ptr<HinhHoc2D> h, std::size_t viTri) {
    if (!h) return;   // nullptr: bỏ qua (không có gì để thêm)
    if (viTri == npos || viTri >= hinh_.size()) {
        hinh_.push_back(std::move(h));   // thêm vào cuối
    } else {
        hinh_.insert(hinh_.begin() + static_cast<std::ptrdiff_t>(viTri), std::move(h));   // chèn, các hình sau dịch xuống
    }
}

std::unique_ptr<HinhHoc2D> Layer::layHinh(int id) {
    const std::size_t i = viTriTheoId(id);
    if (i == npos) return nullptr;       // không thấy: không đổi gì
    std::unique_ptr<HinhHoc2D> ra = std::move(hinh_[i]);
    hinh_.erase(hinh_.begin() + static_cast<std::ptrdiff_t>(i));   // bỏ phần tử rỗng, các hình sau dịch lên
    return ra;
}

HinhHoc2D* Layer::timTheoId(int id) {
    const std::size_t i = viTriTheoId(id);
    return i == npos ? nullptr : hinh_[i].get();
}

const HinhHoc2D* Layer::timTheoId(int id) const {
    const std::size_t i = viTriTheoId(id);
    return i == npos ? nullptr : hinh_[i].get();
}

std::size_t Layer::viTriTheoId(int id) const {
    for (std::size_t i = 0; i < hinh_.size(); ++i)
        if (hinh_[i]->getId() == id) return i;
    return npos;
}

HinhHoc2D& Layer::hinhThu(std::size_t i) { return *hinh_.at(i); }
const HinhHoc2D& Layer::hinhThu(std::size_t i) const { return *hinh_.at(i); }
std::size_t Layer::soHinh() const { return hinh_.size(); }

const std::string& Layer::getTen() const { return ten_; }
bool Layer::isHien() const { return hien_; }
void Layer::setHien(bool b) { hien_ = b; }
bool Layer::isKhoa() const { return khoa_; }
void Layer::setKhoa(bool b) { khoa_ = b; }
