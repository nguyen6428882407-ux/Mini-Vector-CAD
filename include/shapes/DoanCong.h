#pragma once
#include "shapes/HinhHoc2D.h"

class DoanCong : public HinhHoc2D {
private:
    Diem2D p1_;  // Diem bat dau
    Diem2D pc_;  // Diem dieu khien (control point)
    Diem2D p2_;  // Diem ket thuc

public:
    DoanCong(const Diem2D& p1, const Diem2D& pc, const Diem2D& p2);
    DoanCong(double x1, double y1, double cx, double cy, double x2, double y2);

    const Diem2D& getDiem1() const;
    const Diem2D& getDiemDieuKhien() const;
    const Diem2D& getDiem2() const;

    void setDiem1(const Diem2D& p);
    void setDiemDieuKhien(const Diem2D& p);
    void setDiem2(const Diem2D& p);

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};