#pragma once
#include "shapes/HinhHoc2D.h"
#include <vector>

// ============================================================================
// DaGiac - đa giác n đỉnh (Composite: Leaf, Prototype: clone())
// Tầng: shapes | Phụ trách: Hồ Hưng
//
// QUY ƯỚC:
//  - dinh_ lưu các đỉnh theo thứ tự nối; cạnh cuối nối đỉnh cuối về đỉnh đầu (đa giác đóng).
//  - Cho phép tạo rỗng và thêm dần bằng themDinh(); khi có ít hơn 3 đỉnh, diện tích = 0.
//    (ShapeFactory đã tự kiểm tra tối thiểu 3 đỉnh khi đọc từ file.)
//  - Diện tích tính bằng công thức Shoelace và lấy trị tuyệt đối (không phụ thuộc chiều
//    quay đỉnh). Với đa giác tự cắt, kết quả là diện tích theo số vòng quấn, không phải
//    diện tích hình học thông thường.
//  - tam() trả về tâm HỘP BAO (bounding box) đúng như hợp đồng của HinhHoc2D::tam(),
//    KHÔNG phải trung bình các đỉnh. Đa giác rỗng có tâm (0, 0).
// ============================================================================
class DaGiac : public HinhHoc2D {
private:
    std::vector<Diem2D> dinh_;

public:
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
};//
