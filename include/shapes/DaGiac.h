// include/shapes/DaGiac.h
#pragma once
#include <vector>
#include "core/Diem2D.h"
#include "shapes/HinhHoc2D.h"

// ============================================================================
// DaGiac - đa giác (từ 3 đỉnh trở lên; ít hơn thì ném std::invalid_argument)
// Tầng: shapes (kế thừa HinhHoc2D) | Phụ trách: Hồ Hưng
// Diện tích dùng công thức Shoelace nên chỉ đúng với đa giác ĐƠN (không tự cắt).
// ============================================================================
class DaGiac : public HinhHoc2D {
private:
    std::vector<Diem2D> dinh_;

public:
    explicit DaGiac(const std::vector<Diem2D>& dinh);

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;               // tâm hộp bao của mọi đỉnh
    double tinhDienTich() const override;      // Shoelace
    double tinhChuVi() const override;         // gồm cả cạnh đóng
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
