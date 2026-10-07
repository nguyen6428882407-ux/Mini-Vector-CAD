#include "shapes/HinhChuNhat.h"
#include "core/BienDoi2D.h"
#include <sstream>
#include <cmath>

HinhChuNhat::HinhChuNhat(const Diem2D& gocTrenTrai, double chieuRong, double chieuCao) {
    double x = gocTrenTrai.getX(), y = gocTrenTrai.getY();
    dinh_[0] = Diem2D(x, y);
    dinh_[1] = Diem2D(x + chieuRong, y);
    dinh_[2] = Diem2D(x + chieuRong, y + chieuCao);
    dinh_[3] = Diem2D(x, y + chieuCao);
}

HinhChuNhat::HinhChuNhat(double x, double y, double w, double h)
    : HinhChuNhat(Diem2D(x, y), w, h) {}

HinhChuNhat::HinhChuNhat(const Diem2D& d1, const Diem2D& d2, const Diem2D& d3, const Diem2D& d4) {
    dinh_[0] = d1; dinh_[1] = d2; dinh_[2] = d3; dinh_[3] = d4;
}

const std::array<Diem2D, 4>& HinhChuNhat::getDinh() const { return dinh_; }

void HinhChuNhat::ve(std::ostream& os) const {
    os << "HinhChuNhat #" << getId() << ": d1=" << dinh_[0] << " d2=" << dinh_[1]
       << " d3=" << dinh_[2] << " d4=" << dinh_[3] << "\n";
}

Diem2D HinhChuNhat::tam() const {
    double sx = 0.0, sy = 0.0;
    for (const auto& d : dinh_) { sx += d.getX(); sy += d.getY(); }
    return Diem2D(sx / 4.0, sy / 4.0);
}

double HinhChuNhat::tinhDienTich() const {
    double tong = 0.0;
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        tong += dinh_[i].getX() * dinh_[next].getY() - dinh_[next].getX() * dinh_[i].getY();
    }
    return std::abs(tong) / 2.0;
}

double HinhChuNhat::tinhChuVi() const {
    double cv = 0.0;
    for (int i = 0; i < 4; ++i) { cv += dinh_[i].khoangCach(dinh_[(i + 1) % 4]); }
    return cv;
}

void HinhChuNhat::transform(const BienDoi2D& bd) {
    for (auto& d : dinh_) { d = bd.apDung(d); }
}

std::unique_ptr<HinhHoc2D> HinhChuNhat::clone() const {
    return std::unique_ptr<HinhChuNhat>(new HinhChuNhat(*this));
}

std::string HinhChuNhat::toSVG() const {
    std::ostringstream oss;
    oss << "<polygon points=\"";
    for (int i = 0; i < 4; ++i) {
        oss << dinh_[i].getX() << "," << dinh_[i].getY();
        if (i < 3) oss << " ";
    }
    oss << "\" stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " fill=\"" << nen_.toHex(false) << "\""
        << " fill-opacity=\"" << (nen_.getA() / 255.0) << "\" />";
    return oss.str();
}
