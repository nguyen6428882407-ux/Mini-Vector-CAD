// include/shapes/TamGiac.h
#pragma once
#include "core/Diem2D.h"
#include "shapes/HinhHoc2D.h"

// ============================================================================
// TamGiac - tam giác (3 đỉnh)
// Tầng: shapes (kế thừa HinhHoc2D) | Phụ trách: Hồ Hưng
// Cho phép tam giác suy biến (3 điểm thẳng hàng -> diện tích 0), không ném lỗi.
// ============================================================================
class TamGiac : public HinhHoc2D {
private:
    Diem2D p1_;
    Diem2D p2_;
    Diem2D p3_;

public:
    TamGiac(const Diem2D& p1, const Diem2D& p2, const Diem2D& p3);

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;               // tâm hộp bao của 3 đỉnh
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
