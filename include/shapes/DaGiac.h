#pragma once
#include "shapes/HinhHoc2D.h"
#include <vector>

class DaGiac : public HinhHoc2D {
private:
    std::vector<Diem2D> dinh_;
public:
    DaGiac() = default;
    explicit DaGiac(const std::vector<Diem2D>& dinh);
    void themDinh(const Diem2D& d);
    const std::vector<Diem2D>& getDinh() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
