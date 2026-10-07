#pragma once
#include "shapes/HinhHoc2D.h"
#include <vector>
#include <memory>

class GroupShape : public HinhHoc2D {
private:
    std::vector<std::unique_ptr<HinhHoc2D>> danhSachCon_;
public:
    GroupShape();
    GroupShape(const GroupShape& other);
    void themHinh(std::unique_ptr<HinhHoc2D> hinh);
    std::unique_ptr<HinhHoc2D> rutHinh(int id);
    const std::vector<std::unique_ptr<HinhHoc2D>>& getDanhSachCon() const;
    size_t soLuongCon() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};

std::unique_ptr<GroupShape> operator+(std::unique_ptr<HinhHoc2D> a, std::unique_ptr<HinhHoc2D> b);
