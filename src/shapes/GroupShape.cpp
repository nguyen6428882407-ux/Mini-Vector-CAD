#include "shapes/GroupShape.h"
#include <sstream>
#include <algorithm>

GroupShape::GroupShape() : HinhHoc2D() {}

GroupShape::GroupShape(const GroupShape& other) : HinhHoc2D(other) {
    for (const auto& con : other.danhSachCon_) {
        if (con) danhSachCon_.push_back(con->clone());
    }
}

void GroupShape::themHinh(std::unique_ptr<HinhHoc2D> hinh) {
    if (hinh) danhSachCon_.push_back(std::move(hinh));
}

std::unique_ptr<HinhHoc2D> GroupShape::rutHinh(int id) {
    for (auto it = danhSachCon_.begin(); it != danhSachCon_.end(); ++it) {
        if ((*it)->getId() == id) {
            auto hinh = std::move(*it);
            danhSachCon_.erase(it);
            return hinh;
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<HinhHoc2D>>& GroupShape::getDanhSachCon() const {
    return danhSachCon_;
}

size_t GroupShape::soLuongCon() const {
    return danhSachCon_.size();
}

void GroupShape::ve(std::ostream& os) const {
    os << "GroupShape #" << getId() << " (" << danhSachCon_.size() << " hinh)\n";
    for (const auto& con : danhSachCon_) {
        std::ostringstream ss;
        con->ve(ss);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty()) os << "  " << line << "\n";
        }
    }
}

Diem2D GroupShape::tam() const {
    if (danhSachCon_.empty()) return Diem2D(0, 0);
    double sx = 0.0, sy = 0.0;
    for (const auto& con : danhSachCon_) {
        Diem2D t = con->tam();
        sx += t.getX(); sy += t.getY();
    }
    return Diem2D(sx / static_cast<double>(danhSachCon_.size()),
                  sy / static_cast<double>(danhSachCon_.size()));
}

double GroupShape::tinhDienTich() const {
    double tong = 0.0;
    for (const auto& con : danhSachCon_) tong += con->tinhDienTich();
    return tong;
}

double GroupShape::tinhChuVi() const {
    double tong = 0.0;
    for (const auto& con : danhSachCon_) tong += con->tinhChuVi();
    return tong;
}

void GroupShape::transform(const BienDoi2D& bd) {
    for (auto& con : danhSachCon_) con->transform(bd);
}

std::unique_ptr<HinhHoc2D> GroupShape::clone() const {
    return std::unique_ptr<GroupShape>(new GroupShape(*this));
}

std::string GroupShape::toSVG() const {
    std::ostringstream oss;
    oss << "<g id=\"group_" << getId() << "\">\n";
    for (const auto& con : danhSachCon_) oss << "  " << con->toSVG() << "\n";
    oss << "</g>";
    return oss.str();
}

std::unique_ptr<GroupShape> operator+(std::unique_ptr<HinhHoc2D> a, std::unique_ptr<HinhHoc2D> b) {
    auto nhom = std::make_unique<GroupShape>();
    if (a) nhom->themHinh(std::move(a));
    if (b) nhom->themHinh(std::move(b));
    return nhom;
}
