#include "shapes/HinhTron.h"
#include "core/BienDoi2D.h"
#include "core/MathUtils.h"
#include <sstream>
#include <cmath>

HinhTron::HinhTron(const Diem2D& tam, double banKinh) : tam_(tam), banKinh_(std::abs(banKinh)) {}
HinhTron::HinhTron(double cx, double cy, double banKinh) : tam_(cx, cy), banKinh_(std::abs(banKinh)) {}

const Diem2D& HinhTron::getTam() const { return tam_; }
double HinhTron::getBanKinh() const { return banKinh_; }

void HinhTron::ve(std::ostream& os) const {
    os << "HinhTron #" << getId() << ": tam=" << tam_ << " r=" << banKinh_ << "\n";
}

Diem2D HinhTron::tam() const { return tam_; }
double HinhTron::tinhDienTich() const { return MathUtils::PI * banKinh_ * banKinh_; }
double HinhTron::tinhChuVi() const { return 2.0 * MathUtils::PI * banKinh_; }

void HinhTron::transform(const BienDoi2D& bd) {
    tam_ = bd.apDung(tam_);
    banKinh_ *= bd.heSoTiLe();
}

std::unique_ptr<HinhHoc2D> HinhTron::clone() const {
    return std::unique_ptr<HinhTron>(new HinhTron(*this));
}

std::string HinhTron::toSVG() const {
    std::ostringstream oss;
    oss << "<circle cx=\"" << tam_.getX() << "\" cy=\"" << tam_.getY()
        << "\" r=\"" << banKinh_ << "\""
        << " stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " fill=\"" << nen_.toHex(false) << "\""
        << " fill-opacity=\"" << (nen_.getA() / 255.0) << "\" />";
    return oss.str();
}
