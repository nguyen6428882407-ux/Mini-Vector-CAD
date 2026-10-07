#include "shapes/TamGiac.h"
#include "core/BienDoi2D.h"
#include <sstream>
#include <cmath>

TamGiac::TamGiac(const Diem2D& d1, const Diem2D& d2, const Diem2D& d3) : dinh_{d1, d2, d3} {}
TamGiac::TamGiac(double x1, double y1, double x2, double y2, double x3, double y3)
    : dinh_{Diem2D(x1, y1), Diem2D(x2, y2), Diem2D(x3, y3)} {}

const std::array<Diem2D, 3>& TamGiac::getDinh() const { return dinh_; }

void TamGiac::ve(std::ostream& os) const {
    os << "TamGiac #" << getId() << ": d1=" << dinh_[0] << " d2=" << dinh_[1]
       << " d3=" << dinh_[2] << "\n";
}

Diem2D TamGiac::tam() const {
    return Diem2D((dinh_[0].getX() + dinh_[1].getX() + dinh_[2].getX()) / 3.0,
                  (dinh_[0].getY() + dinh_[1].getY() + dinh_[2].getY()) / 3.0);
}

double TamGiac::tinhDienTich() const {
    double x1 = dinh_[0].getX(), y1 = dinh_[0].getY();
    double x2 = dinh_[1].getX(), y2 = dinh_[1].getY();
    double x3 = dinh_[2].getX(), y3 = dinh_[2].getY();
    return std::abs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0;
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
