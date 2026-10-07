#pragma once
#include "shapes/HinhHoc2D.h"

class HinhTron : public HinhHoc2D {
private:
    Diem2D tam_;
    double banKinh_;
public:
    HinhTron(const Diem2D& tam, double banKinh);
    HinhTron(double cx, double cy, double banKinh);
    const Diem2D& getTam() const;
    double getBanKinh() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
