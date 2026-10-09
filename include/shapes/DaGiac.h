// src/shapes/DaGiac.cpp
#include "shapes/DaGiac.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
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

DaGiac::DaGiac(const std::vector<Diem2D>& dinh) : dinh_(dinh) {
    if (dinh_.size() < 3)
        throw std::invalid_argument("DaGiac: can it nhat 3 dinh");
}

void DaGiac::ve(std::ostream& os) const {
    os << "DaGiac #" << id_ << ":";
    for (const Diem2D& d : dinh_) os << ' ' << diem(d);
    os << '\n';
}

Diem2D DaGiac::tam() const {
    double minX = dinh_[0].getX(), maxX = minX, minY = dinh_[0].getY(), maxY = minY;
    for (const Diem2D& d : dinh_) {
        minX = std::min(minX, d.getX());
        maxX = std::max(maxX, d.getX());
        minY = std::min(minY, d.getY());
        maxY = std::max(maxY, d.getY());
    }
    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

// Shoelace: S = |sum(x_i*y_{i+1} - x_{i+1}*y_i)| / 2, đỉnh cuối nối về đỉnh đầu.
double DaGiac::tinhDienTich() const {
    double tong = 0.0;
    const std::size_t n = dinh_.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Diem2D& a = dinh_[i];
        const Diem2D& b = dinh_[(i + 1) % n];
        tong += a.getX() * b.getY() - b.getX() * a.getY();
    }
    return 0.5 * std::fabs(tong);
}

double DaGiac::tinhChuVi() const {
    double tong = 0.0;
    const std::size_t n = dinh_.size();
    for (std::size_t i = 0; i < n; ++i)
        tong += dinh_[i].khoangCach(dinh_[(i + 1) % n]);   // gồm cả cạnh đóng
    return tong;
}

void DaGiac::transform(const BienDoi2D& bd) {
    for (Diem2D& d : dinh_) d = bd.apDung(d);
}

std::unique_ptr<HinhHoc2D> DaGiac::clone() const {
    return std::make_unique<DaGiac>(*this);   // copy ctor của HinhHoc2D tự cấp id mới; vector copy sâu
}

std::string DaGiac::toSVG() const {
    std::string pts;
    for (const Diem2D& d : dinh_) {
        if (!pts.empty()) pts += ' ';
        pts += diemSVG(d);
    }
    return "<polygon points=\"" + pts + "\"" + styleSVG(nen_, vien_, doDayNet_) + " />";
}
