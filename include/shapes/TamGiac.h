#pragma once
#include "shapes/HinhHoc2D.h"
#include <array>

// ============================================================================
// TamGiac - hình tam giác 3 đỉnh (Composite: Leaf, Prototype: clone())
// Tầng: shapes | Phụ trách: Hồ Hưng
//
// QUY ƯỚC:
//  - dinh_ lưu 3 đỉnh theo đúng thứ tự người dùng nhập (d1, d2, d3).
//  - Tam giác suy biến (3 đỉnh thẳng hàng) vẫn được chấp nhận ở tầng này, diện tích = 0;
//    việc từ chối đầu vào thẳng hàng do ShapeFactory đảm nhiệm.
//  - tam() trả về tâm HỘP BAO (bounding box) đúng như hợp đồng của HinhHoc2D::tam(),
//    KHÔNG phải trọng tâm tam giác.
// ============================================================================
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
