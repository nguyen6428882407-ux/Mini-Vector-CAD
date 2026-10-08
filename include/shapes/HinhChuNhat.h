// src/shapes/HinhChuNhat.cpp
#include "shapes/HinhChuNhat.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "core/BienDoi2D.h"
#include "core/MathUtils.h"
#include "core/MauSac.h"
#include "core/Diem2D.h"

namespace {
// Số thực -> chuỗi: tối đa 4 chữ số thập phân, bỏ số 0 thừa, không bao giờ ra "-0" hay "1e-16".
std::string so(double v) {
    if (std::fabs(v) < 5e-5) v = 0.0;
    std::ostringstream ss;
    ss.imbue(std::locale::classic());   // luôn dùng dấu '.'
    ss << std::fixed << std::setprecision(4) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

std::string diem(const Diem2D& p) { return "(" + so(p.getX()) + "," + so(p.getY()) + ")"; }

// Thuộc tính style cho thẻ SVG (bắt đầu bằng một dấu cách).
std::string styleSVG(const MauSac& nen, const MauSac& vien, double doDayNet) {
    return " fill=\"" + nen.toHex() + "\" fill-opacity=\"" + so(nen.getAlpha()) + "\"" +
           " stroke=\"" + vien.toHex() + "\" stroke-opacity=\"" + so(vien.getAlpha()) + "\"" +
           " stroke-width=\"" + so(doDayNet) + "\"";
}
}  // namespace

HinhChuNhat::HinhChuNhat(const Diem2D& goc, double rong, double cao)
    : goc_(goc), rong_(rong), cao_(cao), gocXoay_(0.0) {
    if (rong < 0.0 || cao < 0.0)
        throw std::invalid_argument("HinhChuNhat: rong va cao khong duoc am");
}

std::array<Diem2D, 4> HinhChuNhat::cacDinh() const {
    const double rad = MathUtils::doSangRad(gocXoay_);
    const double c = std::cos(rad), s = std::sin(rad);
    const Diem2D u(rong_ * c, rong_ * s);     // vector cạnh rộng
    const Diem2D v(-cao_ * s, cao_ * c);      // vector cạnh cao (vuông góc với u)
    return {goc_, goc_ + u, goc_ + u + v, goc_ + v};
}

void HinhChuNhat::ve(std::ostream& os) const {
    os << "HinhChuNhat #" << id_ << ": goc=" << diem(goc_) << " rong=" << so(rong_) << " cao=" << so(cao_);
    if (so(gocXoay_) != "0") os << " xoay=" << so(gocXoay_);
    os << '\n';
}

Diem2D HinhChuNhat::tam() const {
    const auto d = cacDinh();
    return Diem2D((d[0].getX() + d[2].getX()) / 2.0, (d[0].getY() + d[2].getY()) / 2.0);
}

double HinhChuNhat::tinhDienTich() const { return rong_ * cao_; }

double HinhChuNhat::tinhChuVi() const { return 2.0 * (rong_ + cao_); }

// Biến đổi affine (tịnh tiến / xoay / scale đều): góc dời theo bd, hai cạnh nhân hệ số tỉ lệ,
// hướng cạnh rộng cập nhật theo ảnh của một vector đơn vị.
void HinhChuNhat::transform(const BienDoi2D& bd) {
    const double rad = MathUtils::doSangRad(gocXoay_);
    const Diem2D gocMoi = bd.apDung(goc_);
    const Diem2D huongMoi = bd.apDung(goc_ + Diem2D(std::cos(rad), std::sin(rad))) - gocMoi;
    const double k = bd.heSoTiLe();

    goc_ = gocMoi;
    rong_ *= k;
    cao_ *= k;
    gocXoay_ = std::atan2(huongMoi.getY(), huongMoi.getX()) * 180.0 / MathUtils::PI;
}

std::unique_ptr<HinhHoc2D> HinhChuNhat::clone() const {
    return std::make_unique<HinhChuNhat>(*this);   // copy ctor của HinhHoc2D tự cấp id mới
}

std::string HinhChuNhat::toSVG() const {
    std::string s = "<rect x=\"" + so(goc_.getX()) + "\" y=\"" + so(goc_.getY()) +
                    "\" width=\"" + so(rong_) + "\" height=\"" + so(cao_) + "\"";
    if (so(gocXoay_) != "0")   // xoay quanh đỉnh gốc, cùng chiều với BienDoi2D::xoay
        s += " transform=\"rotate(" + so(gocXoay_) + " " + so(goc_.getX()) + " " + so(goc_.getY()) + ")\"";
    return s + styleSVG(nen_, vien_, doDayNet_) + " />";
}
