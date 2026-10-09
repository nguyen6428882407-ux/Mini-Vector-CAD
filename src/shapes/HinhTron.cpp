#include "shapes/HinhTron.h"
#include "core/MathUtils.h"
#include <cmath>

HinhTron::HinhTron() 
    : HinhHoc2D(), tam(0.0, 0.0), banKinh(1.0) {}

HinhTron::HinhTron(const Diem2D& tam, double banKinh) 
    : HinhHoc2D(), tam(tam), banKinh(std::abs(banKinh)) {}

HinhTron::HinhTron(const Diem2D& tam, double banKinh, const MauSac& mauNiet, const MauSac& mauNen, double doRongNet)
    : HinhHoc2D(mauNiet, mauNen, doRongNet), tam(tam), banKinh(std::abs(banKinh)) {}

Diem2D HinhTron::getTam() const { return tam; }
double HinhTron::getBanKinh() const { return banKinh; }

void HinhTron::setTam(const Diem2D& tam) { this->tam = tam; }
void HinhTron::setBanKinh(double banKinh) { this->banKinh = std::abs(banKinh); }

// (1) Chu vi C = 2 * PI * R
double HinhTron::tinhChuVi() const {
    return 2.0 * MathUtils::PI * banKinh;
}

// (2) Diện tích S = PI * R^2
double HinhTron::tinhDienTich() const {
    return MathUtils::PI * banKinh * banKinh;
}

// (3) Lấy tâm hình tròn
Diem2D HinhTron::tinhTam() const {
    return tam;
}

// (4) Tịnh tiến tâm hình tròn một khoảng (dx, dy)
void HinhTron::tinhTien(double dx, double dy) {
    tam = tam + Diem2D(dx, dy);
}

// (5) Xoay hình tròn quanh tâm của chính nó (vị trí & bán kính giữ nguyên)
void HinhTron::xoay(double gocRad) {
    (void)gocRad; // Tránh cảnh báo unused parameter
}

// (6) Scale hình tròn bằng heSoTiLe (thay đổi bán kính R)
void HinhTron::scale(double heSoTiLe) {
    banKinh = banKinh * std::abs(heSoTiLe);
}