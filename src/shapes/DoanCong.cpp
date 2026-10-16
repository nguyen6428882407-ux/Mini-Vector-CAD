#include "shapes/DoanCong.h"
#include "core/BienDoi2D.h"
#include <sstream>
#include <cmath>
#include <algorithm>

DoanCong::DoanCong(const Diem2D& p1, const Diem2D& pc, const Diem2D& p2)
    : p1_(p1), pc_(pc), p2_(p2) {}

DoanCong::DoanCong(double x1, double y1, double cx, double cy, double x2, double y2)
    : p1_(x1, y1), pc_(cx, cy), p2_(x2, y2) {}

const Diem2D& DoanCong::getDiem1() const { return p1_; }
const Diem2D& DoanCong::getDiemDieuKhien() const { return pc_; }
const Diem2D& DoanCong::getDiem2() const { return p2_; }

void DoanCong::setDiem1(const Diem2D& p) { p1_ = p; }
void DoanCong::setDiemDieuKhien(const Diem2D& p) { pc_ = p; }
void DoanCong::setDiem2(const Diem2D& p) { p2_ = p; }

void DoanCong::ve(std::ostream& os) const {
    os << "DoanCong #" << getId() << ": p1=" << p1_ << " pc=" << pc_ << " p2=" << p2_ << "\n";
}

Diem2D DoanCong::tam() const {
    // Tinh bounding box cua quadratic bezier
    double minX = std::min(p1_.getX(), p2_.getX());
    double maxX = std::max(p1_.getX(), p2_.getX());
    double minY = std::min(p1_.getY(), p2_.getY());
    double maxY = std::max(p1_.getY(), p2_.getY());

    // Cực trị theo X
    double denomX = p1_.getX() - 2.0 * pc_.getX() + p2_.getX();
    if (std::abs(denomX) > 1e-9) {
        double t = (p1_.getX() - pc_.getX()) / denomX;
        if (t > 0.0 && t < 1.0) {
            double xt = (1.0 - t) * (1.0 - t) * p1_.getX() + 2.0 * (1.0 - t) * t * pc_.getX() + t * t * p2_.getX();
            minX = std::min(minX, xt);
            maxX = std::max(maxX, xt);
        }
    }

    // Cực trị theo Y
    double denomY = p1_.getY() - 2.0 * pc_.getY() + p2_.getY();
    if (std::abs(denomY) > 1e-9) {
        double t = (p1_.getY() - pc_.getY()) / denomY;
        if (t > 0.0 && t < 1.0) {
            double yt = (1.0 - t) * (1.0 - t) * p1_.getY() + 2.0 * (1.0 - t) * t * pc_.getY() + t * t * p2_.getY();
            minY = std::min(minY, yt);
            maxY = std::max(maxY, yt);
        }
    }

    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

double DoanCong::tinhDienTich() const {
    return 0.0;
}

double DoanCong::tinhChuVi() const {
    // Xap xi do dai duong cong bang cach chia 16 doan nho
    constexpr int STEPS = 16;
    double len = 0.0;
    Diem2D prev = p1_;
    for (int i = 1; i <= STEPS; ++i) {
        double t = static_cast<double>(i) / STEPS;
        double u = 1.0 - t;
        double x = u * u * p1_.getX() + 2.0 * u * t * pc_.getX() + t * t * p2_.getX();
        double y = u * u * p1_.getY() + 2.0 * u * t * pc_.getY() + t * t * p2_.getY();
        Diem2D curr(x, y);
        len += prev.khoangCach(curr);
        prev = curr;
    }
    return len;
}

void DoanCong::transform(const BienDoi2D& bd) {
    p1_ = bd.apDung(p1_);
    pc_ = bd.apDung(pc_);
    p2_ = bd.apDung(p2_);
}

std::unique_ptr<HinhHoc2D> DoanCong::clone() const {
    return std::unique_ptr<DoanCong>(new DoanCong(*this));
}

std::string DoanCong::toSVG() const {
    std::ostringstream oss;
    oss << "<path d=\"M " << p1_.getX() << " " << p1_.getY()
        << " Q " << pc_.getX() << " " << pc_.getY()
        << " " << p2_.getX() << " " << p2_.getY() << "\""
        << " fill=\"none\""
        << " stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " stroke-linecap=\"round\" stroke-linejoin=\"round\" />";
    return oss.str();
}
