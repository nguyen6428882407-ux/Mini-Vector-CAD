#include "shapes/DoanThang.h"
#include "core/BienDoi2D.h"
#include <sstream>

DoanThang::DoanThang(const Diem2D& p1, const Diem2D& p2) : p1_(p1), p2_(p2) {}
DoanThang::DoanThang(double x1, double y1, double x2, double y2) : p1_(x1, y1), p2_(x2, y2) {}

const Diem2D& DoanThang::getDiem1() const { return p1_; }
const Diem2D& DoanThang::getDiem2() const { return p2_; }

void DoanThang::ve(std::ostream& os) const {
    os << "DoanThang #" << getId() << ": p1=" << p1_ << " p2=" << p2_ << "\n";
}

Diem2D DoanThang::tam() const {
    return Diem2D((p1_.getX() + p2_.getX()) / 2.0, (p1_.getY() + p2_.getY()) / 2.0);
}

double DoanThang::tinhDienTich() const { return 0.0; }
double DoanThang::tinhChuVi() const { return p1_.khoangCach(p2_); }

void DoanThang::transform(const BienDoi2D& bd) {
    p1_ = bd.apDung(p1_);
    p2_ = bd.apDung(p2_);
}

std::unique_ptr<HinhHoc2D> DoanThang::clone() const {
    return std::unique_ptr<DoanThang>(new DoanThang(*this));
}

std::string DoanThang::toSVG() const {
    std::ostringstream oss;
    oss << "<line x1=\"" << p1_.getX() << "\" y1=\"" << p1_.getY()
        << "\" x2=\"" << p2_.getX() << "\" y2=\"" << p2_.getY()
        << "\" stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\" />";
    return oss.str();
}
