<<<<<<< HEAD
#ifndef DOAN_THANG_H
#define DOAN_THANG_H

#include "shapes/HinhHoc2D.h"
#include "core/Diem2D.h"

class DoanThang : public HinhHoc2D {
private:
    Diem2D p1; // Điểm bắt đầu
    Diem2D p2; // Điểm kết thúc

public:
    // Constructors
    DoanThang();
    DoanThang(const Diem2D& p1, const Diem2D& p2);
    DoanThang(const Diem2D& p1, const Diem2D& p2, const MauSac& mauNiet, double doRongNet = 1.0);

    // Destructor
    virtual ~DoanThang() override = default;

    // Getters & Setters
    Diem2D getP1() const;
    Diem2D getP2() const;
    void setP1(const Diem2D& p1);
    void setP2(const Diem2D& p2);

    // Cài đặt đủ 6 hàm ảo của HinhHoc2D
    double tinhChuVi() const override;             // (1) Độ dài đoạn thẳng
    double tinhDienTich() const override;           // (2) Bằng 0
    Diem2D tinhTam() const override;                // (3) Trung điểm của đoạn thẳng
    void tinhTien(double dx, double dy) override;   // (4) Dịch chuyển 2 điểm
    void xoay(double gocRad) override;              // (5) Xoay 2 điểm quanh trung điểm
    void scale(double heSoTiLe) override;           // (6) Thu phóng độ dài quanh trung điểm
};

#endif // DOAN_THANG_H
=======
#pragma once
#include "shapes/HinhHoc2D.h"

class DoanThang : public HinhHoc2D {
private:
    Diem2D p1_, p2_;
public:
    DoanThang(const Diem2D& p1, const Diem2D& p2);
    DoanThang(double x1, double y1, double x2, double y2);
    const Diem2D& getDiem1() const;
    const Diem2D& getDiem2() const;
    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};
>>>>>>> ded1bd01f3f4c0312951010ce1e5a41e5869043d
