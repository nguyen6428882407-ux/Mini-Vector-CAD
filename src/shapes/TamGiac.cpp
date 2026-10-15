#include "shapes/TamGiac.h"
#include "core/BienDoi2D.h"
#include <algorithm>
#include <cmath>
#include <ostream>
#include <sstream>

TamGiac::TamGiac(const Diem2D& d1, const Diem2D& d2, const Diem2D& d3) : dinh_{d1, d2, d3} {}
TamGiac::TamGiac(double x1, double y1, double x2, double y2, double x3, double y3)
    : dinh_{Diem2D(x1, y1), Diem2D(x2, y2), Diem2D(x3, y3)} {}

const std::array<Diem2D, 3>& TamGiac::getDinh() const { return dinh_; }

void TamGiac::ve(std::ostream& os) const {
    os << "TamGiac #" << getId() << ": d1=" << dinh_[0] << " d2=" << dinh_[1]
       << " d3=" << dinh_[2] << "\n";
}

// Tâm của HỘP BAO (theo hợp đồng HinhHoc2D::tam()), không phải trọng tâm.
Diem2D TamGiac::tam() const {
    const double minX = std::min({dinh_[0].getX(), dinh_[1].getX(), dinh_[2].getX()});
    const double maxX = std::max({dinh_[0].getX(), dinh_[1].getX(), dinh_[2].getX()});
    const double minY = std::min({dinh_[0].getY(), dinh_[1].getY(), dinh_[2].getY()});
    const double maxY = std::max({dinh_[0].getY(), dinh_[1].getY(), dinh_[2].getY()});
    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

// Công thức Shoelace cho 3 đỉnh, tính theo vector từ đỉnh d1 (tịnh tiến gốc về d1 để giảm
// sai số triệt tiêu khi tọa độ lớn; kết quả toán học không đổi).
double TamGiac::tinhDienTich() const {
    const double x1 = dinh_[0].getX(), y1 = dinh_[0].getY();
    const double ax = dinh_[1].getX() - x1, ay = dinh_[1].getY() - y1;
    const double bx = dinh_[2].getX() - x1, by = dinh_[2].getY() - y1;
    return std::abs(ax * by - bx * ay) / 2.0;
}

double TamGiac::tinhChuVi() const {
    return dinh_[0].khoangCach(dinh_[1]) + dinh_[1].khoangCach(dinh_[2]) + dinh_[2].khoangCach(dinh_[0]);
}

void TamGiac::transform(const BienDoi2D& bd) {
    for (auto& d : dinh_) { d = bd.apDung(d); }
}

std::unique_ptr<HinhHoc2D> TamGiac::clone() const {
    return std::unique_ptr<TamGiac>(new TamGiac(*this));
}

std::string TamGiac::toSVG() const {
    std::ostringstream oss;
    oss << "<polygon points=\""
        << dinh_[0].getX() << "," << dinh_[0].getY() << " "
        << dinh_[1].getX() << "," << dinh_[1].getY() << " "
        << dinh_[2].getX() << "," << dinh_[2].getY() << "\""
        << " stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " fill=\"" << nen_.toHex(false) << "\""
        << " fill-opacity=\"" << (nen_.getA() / 255.0) << "\" />";
    return oss.str();
}
