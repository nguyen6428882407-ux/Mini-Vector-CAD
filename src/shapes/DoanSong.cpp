#include "shapes/DoanSong.h"
#include "core/BienDoi2D.h"
#include <sstream>
#include <cmath>
#include <algorithm>

DoanSong::DoanSong(const Diem2D& p1, const Diem2D& p2, double bienDo, double soSong)
    : p1_(p1), p2_(p2), bienDo_(std::abs(bienDo)), soSong_(soSong > 0.0 ? soSong : 3.0) {}

DoanSong::DoanSong(double x1, double y1, double x2, double y2, double bienDo, double soSong)
    : p1_(x1, y1), p2_(x2, y2), bienDo_(std::abs(bienDo)), soSong_(soSong > 0.0 ? soSong : 3.0) {}

const Diem2D& DoanSong::getDiem1() const { return p1_; }
const Diem2D& DoanSong::getDiem2() const { return p2_; }
double DoanSong::getBienDo() const { return bienDo_; }
double DoanSong::getSoSong() const { return soSong_; }

void DoanSong::setDiem1(const Diem2D& p) { p1_ = p; }
void DoanSong::setDiem2(const Diem2D& p) { p2_ = p; }
void DoanSong::setBienDo(double b) { bienDo_ = std::abs(b); }
void DoanSong::setSoSong(double s) { if (s > 0.0) soSong_ = s; }

void DoanSong::ve(std::ostream& os) const {
    os << "DoanSong #" << getId() << ": p1=" << p1_ << " p2=" << p2_
       << " bienDo=" << bienDo_ << " soSong=" << soSong_ << "\n";
}

Diem2D DoanSong::tam() const {
    return Diem2D((p1_.getX() + p2_.getX()) / 2.0, (p1_.getY() + p2_.getY()) / 2.0);
}

double DoanSong::tinhDienTich() const {
    return 0.0;
}

double DoanSong::tinhChuVi() const {
    double L = p1_.khoangCach(p2_);
    // Xap xi do dai duong hinh sin
    double arcApprox = std::hypot(L, 2.0 * 3.141592653589793 * bienDo_ * soSong_ * 0.707);
    return arcApprox;
}

void DoanSong::transform(const BienDoi2D& bd) {
    double oldLen = p1_.khoangCach(p2_);
    p1_ = bd.apDung(p1_);
    p2_ = bd.apDung(p2_);
    double newLen = p1_.khoangCach(p2_);
    if (oldLen > 1e-6) {
        bienDo_ *= (newLen / oldLen);
    }
}

std::unique_ptr<HinhHoc2D> DoanSong::clone() const {
    return std::unique_ptr<DoanSong>(new DoanSong(*this));
}

std::string DoanSong::toSVG() const {
    double dx = p2_.getX() - p1_.getX();
    double dy = p2_.getY() - p1_.getY();
    double L = std::hypot(dx, dy);

    std::ostringstream oss;
    if (L < 1e-4) {
        oss << "<line x1=\"" << p1_.getX() << "\" y1=\"" << p1_.getY()
            << "\" x2=\"" << p2_.getX() << "\" y2=\"" << p2_.getY() << "\" />";
        return oss.str();
    }

    double ux = dx / L;
    double uy = dy / L;
    double nx = -uy;
    double ny = ux;

    int cycles = std::max(1, static_cast<int>(std::round(soSong_)));
    int halfWaves = cycles * 2;
    double step = L / halfWaves;

    oss << "<path d=\"M " << p1_.getX() << " " << p1_.getY();

    for (int i = 0; i < halfWaves; ++i) {
        double sign = (i % 2 == 0) ? 1.0 : -1.0;
        double endDist = (i + 1) * step;
        double ctrlDist = (i + 0.5) * step;

        double endX = p1_.getX() + endDist * ux;
        double endY = p1_.getY() + endDist * uy;

        // Peak tai giua nua song: control point can cao gap doi bien do de dinh parabol dat bienDo
        double ctrlX = p1_.getX() + ctrlDist * ux + sign * (2.0 * bienDo_) * nx;
        double ctrlY = p1_.getY() + ctrlDist * uy + sign * (2.0 * bienDo_) * ny;

        oss << " Q " << ctrlX << " " << ctrlY << " " << endX << " " << endY;
    }

    oss << "\""
        << " fill=\"none\""
        << " stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " stroke-linecap=\"round\" stroke-linejoin=\"round\" />";

    return oss.str();
}
