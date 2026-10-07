#pragma once
#include "shapes/HinhHoc2D.h"
#include <array>

class HinhChuNhat : public HinhHoc2D {
private:
    std::array<Diem2D, 4> dinh_;
public:
    HinhChuNhat(const Diem2D& gocTrenTrai, double chieuRong, double chieuCao);
    HinhChuNhat(double x, double y, double w, double h);
    HinhChuNhat(const Diem2D& d1, const Diem2D& d2, const Diem2D& d3, const Diem2D& d4);
    const std::array<Diem2D, 4>& getDinh() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
