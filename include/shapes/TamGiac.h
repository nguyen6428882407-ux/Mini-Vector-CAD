#pragma once
#include "shapes/HinhHoc2D.h"
#include <array>

class TamGiac : public HinhHoc2D {
private:
    std::array<Diem2D, 3> dinh_;
public:
    TamGiac(const Diem2D& d1, const Diem2D& d2, const Diem2D& d3);
    TamGiac(double x1, double y1, double x2, double y2, double x3, double y3);
    const std::array<Diem2D, 3>& getDinh() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
