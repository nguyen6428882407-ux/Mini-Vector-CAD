#pragma once
#include "shapes/HinhHoc2D.h"
#include <vector>
#include <memory>

// ============================================================================
// GroupShape - Lop bieu dien mot nhom hinh hoc (Composite Pattern)
// Tang: shapes | Phu trach: Duc Huy
// Dac diem:
//  - Chon quyen so huu cac hinh con thong qua std::unique_ptr<HinhHoc2D>.
//  - Ho tro long nhau khong gioi han so cap (Tree structure).
//  - Tinh dien tich, chu vi, transform va ve de quy tu dong nho da hinh.
// ============================================================================
class GroupShape : public HinhHoc2D {
private:
    std::vector<std::unique_ptr<HinhHoc2D>> danhSachCon_;

public:
    GroupShape();
    GroupShape(const GroupShape& other);

    void themHinh(std::unique_ptr<HinhHoc2D> hinh);
    void them(std::unique_ptr<HinhHoc2D> hinh); // alias tuong thich

    std::unique_ptr<HinhHoc2D> rutHinh(int id);
    const std::vector<std::unique_ptr<HinhHoc2D>>& getDanhSachCon() const;

    size_t soLuongCon() const;
    size_t soLuong() const; // alias tuong thich

    HinhHoc2D& hinhThu(size_t i);
    const HinhHoc2D& hinhThu(size_t i) const;

    // Tim kiem de quy hinh con theo ID (ke ca trong nhom con long nhau)
    const HinhHoc2D* timHinhCon(int id) const;
    HinhHoc2D* timHinhCon(int id);

    // Kiem tra su ton tai cua mot hinh trong nhom (ho tro early-exit)
    bool chuaHinh(int id) const;

    // Xoa sach tat ca hinh con
    void xoaTatCa();

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};

// Toan tu cong gop hai hinh thanh mot GroupShape moi
std::unique_ptr<GroupShape> operator+(std::unique_ptr<HinhHoc2D> a, std::unique_ptr<HinhHoc2D> b);
std::unique_ptr<GroupShape> operator+(const HinhHoc2D& a, const HinhHoc2D& b);
