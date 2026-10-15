#pragma once

#include "shapes/HinhHoc2D.h"
#include <vector>

class NetVeTuDo : public HinhHoc2D {
private:
    std::vector<Diem2D> diem_;

public:
    explicit NetVeTuDo(const std::vector<Diem2D>& diem);

    const std::vector<Diem2D>& getDiem() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
