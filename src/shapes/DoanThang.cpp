#include "shapes/DoanThang.h"
<<<<<<< HEAD
#include "core/MathUtils.h"
#include <cmath>

DoanThang::DoanThang() 
    : HinhHoc2D(), p1(0.0, 0.0), p2(0.0, 0.0) {}

DoanThang::DoanThang(const Diem2D& p1, const Diem2D& p2) 
    : HinhHoc2D(), p1(p1), p2(p2) {}

DoanThang::DoanThang(const Diem2D& p1, const Diem2D& p2, const MauSac& mauNiet, double doRongNet)
    : HinhHoc2D(mauNiet, MauSac(), doRongNet), p1(p1), p2(p2) {}

Diem2D DoanThang::getP1() const { return p1; }
Diem2D DoanThang::getP2() const { return p2; }

void DoanThang::setP1(const Diem2D& p1) { this->p1 = p1; }
void DoanThang::setP2(const Diem2D& p2) { this->p2 = p2; }

// (1) Chu vi đoạn thẳng chính là độ dài của nó
double DoanThang::tinhChuVi() const {
    return p1.khoangCach(p2);
}

// (2) Đoạn thẳng không có diện tích trong 2D
double DoanThang::tinhDienTich() const {
    return 0.0;
}

// (3) Tâm đoạn thẳng là trung điểm nối p1 và p2
Diem2D DoanThang::tinhTam() const {
    return Diem2D((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
}

// (4) Tịnh tiến 2 đầu đoạn thẳng một khoảng (dx, dy)
void DoanThang::tinhTien(double dx, double dy) {
    p1 = p1 + Diem2D(dx, dy);
    p2 = p2 + Diem2D(dx, dy);
}

// (5) Xoay 2 điểm p1, p2 quanh trung điểm của đoạn thẳng
void DoanThang::xoay(double gocRad) {
    Diem2D tam = tinhTam();
    double cosGoc = std::cos(gocRad);
    double sinGoc = std::sin(gocRad);

    auto xoayDiem = [tam, cosGoc, sinGoc](const Diem2D& p) {
        double dx = p.getX() - tam.getX();
        double dy = p.getY() - tam.getY();
        double xMoi = tam.getX() + dx * cosGoc - dy * sinGoc;
        double yMoi = tam.getY() + dx * sinGoc + dy * cosGoc;
        return Diem2D(xMoi, yMoi);
    };

    p1 = xoayDiem(p1);
    p2 = xoayDiem(p2);
}

// (6) Scale (thu phóng) độ dài đoạn thẳng quanh trung điểm
void DoanThang::scale(double heSoTiLe) {
    Diem2D tam = tinhTam();
    p1 = tam + (p1 - tam) * heSoTiLe;
    p2 = tam + (p2 - tam) * heSoTiLe;
}
=======
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
>>>>>>> ded1bd01f3f4c0312951010ce1e5a41e5869043d
