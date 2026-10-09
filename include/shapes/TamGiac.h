// src/shapes/TamGiac.cpp
#include "shapes/TamGiac.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include "core/BienDoi2D.h"
#include "core/MauSac.h"

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

std::string diemSVG(const Diem2D& p) { return so(p.getX()) + "," + so(p.getY()); }

// Thuộc tính style cho thẻ SVG (bắt đầu bằng một dấu cách).
std::string styleSVG(const MauSac& nen, const MauSac& vien, double doDayNet) {
    return " fill=\"" + nen.toHex() + "\" fill-opacity=\"" + so(nen.getAlpha()) + "\"" +
           " stroke=\"" + vien.toHex() + "\" stroke-opacity=\"" + so(vien.getAlpha()) + "\"" +
           " stroke-width=\"" + so(doDayNet) + "\"";
}
}  // namespace

TamGiac::TamGiac(const Diem2D& p1, const Diem2D& p2, const Diem2D& p3) : p1_(p1), p2_(p2), p3_(p3) {}

void TamGiac::ve(std::ostream& os) const {
    os << "TamGiac #" << id_ << ": " << diem(p1_) << ' ' << diem(p2_) << ' ' << diem(p3_) << '\n';
}

Diem2D TamGiac::tam() const {
    const double minX = std::min({p1_.getX(), p2_.getX(), p3_.getX()});
    const double maxX = std::max({p1_.getX(), p2_.getX(), p3_.getX()});
    const double minY = std::min({p1_.getY(), p2_.getY(), p3_.getY()});
    const double maxY = std::max({p1_.getY(), p2_.getY(), p3_.getY()});
    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

// Shoelace: S = |x1(y2-y3) + x2(y3-y1) + x3(y1-y2)| / 2
double TamGiac::tinhDienTich() const {
    return 0.5 * std::fabs(p1_.getX() * (p2_.getY() - p3_.getY()) +
                           p2_.getX() * (p3_.getY() - p1_.getY()) +
                           p3_.getX() * (p1_.getY() - p2_.getY()));
}

double TamGiac::tinhChuVi() const {
    return p1_.khoangCach(p2_) + p2_.khoangCach(p3_) + p3_.khoangCach(p1_);
}

void TamGiac::transform(const BienDoi2D& bd) {
    p1_ = bd.apDung(p1_);
    p2_ = bd.apDung(p2_);
    p3_ = bd.apDung(p3_);
}

std::unique_ptr<HinhHoc2D> TamGiac::clone() const {
    return std::make_unique<TamGiac>(*this);   // copy ctor của HinhHoc2D tự cấp id mới
}

std::string TamGiac::toSVG() const {
    return "<polygon points=\"" + diemSVG(p1_) + " " + diemSVG(p2_) + " " + diemSVG(p3_) + "\"" +
           styleSVG(nen_, vien_, doDayNet_) + " />";
}
