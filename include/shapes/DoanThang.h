#pragma once
#include "shapes/HinhHoc2D.h"

class DoanThang : public HinhHoc2D {
private:
    Diem2D p1_, p2_;
public:
    DoanThang(const Diem2D& p1, const Diem2D& p2);
    DoanThang(double x1, double y1, double x2, double y2);
    const Diem2D& getDiem1() const;
    const Diem2D& getDiem2() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
