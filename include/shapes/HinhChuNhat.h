// include/shapes/HinhChuNhat.h
#pragma once
#include <array>
#include "core/Diem2D.h"
#include "shapes/HinhHoc2D.h"

// ============================================================================
// HinhChuNhat - hình chữ nhật, xác định bởi góc trên-trái, rộng, cao
// Tầng: shapes (kế thừa HinhHoc2D) | Phụ trách: Hồ Hưng
//
// Vì BienDoi2D có cả phép XOAY, hình lưu thêm gocXoay_ (độ, quanh goc_) để vẫn là
// hình chữ nhật đúng sau khi xoay. Chưa xoay thì gocXoay_ = 0 và toSVG() ra <rect> thường.
// Chấp nhận rong = 0 hoặc cao = 0 (suy biến); giá trị ÂM ném std::invalid_argument.
// ============================================================================
class HinhChuNhat : public HinhHoc2D {
private:
    Diem2D goc_;        // đỉnh trên-trái (đỉnh gốc của hình)
    double rong_;       // chiều dài cạnh theo hướng gocXoay_
    double cao_;        // chiều dài cạnh vuông góc với cạnh rộng
    double gocXoay_;    // góc xoay (ĐỘ) của cạnh rộng so với trục x, quanh goc_

    // 4 đỉnh thực tế: trên-trái, trên-phải, dưới-phải, dưới-trái (đã tính cả xoay).
    std::array<Diem2D, 4> cacDinh() const;
    Diem2D tam() const;

public:
    HinhChuNhat(const Diem2D& goc, double rong, double cao);   // goc = đỉnh trên-trái

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;               // trung điểm đường chéo
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
