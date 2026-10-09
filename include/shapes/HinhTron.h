#ifndef HINH_TRON_H
#define HINH_TRON_H

#include "shapes/HinhHoc2D.h"
#include "core/Diem2D.h"

class HinhTron : public HinhHoc2D {
private:
    Diem2D tam;     // Tâm hình tròn
    double banKinh; // Bán kính R (luôn >= 0)

public:
    // Constructors
    HinhTron();
    HinhTron(const Diem2D& tam, double banKinh);
    HinhTron(const Diem2D& tam, double banKinh, const MauSac& mauNiet, const MauSac& mauNen, double doRongNet = 1.0);

    // Destructor
    virtual ~HinhTron() override = default;

    // Getters & Setters
    Diem2D getTam() const;
    double getBanKinh() const;
    void setTam(const Diem2D& tam);
    void setBanKinh(double banKinh);

    // Cài đặt đủ 6 hàm ảo của HinhHoc2D
    double tinhChuVi() const override;             // (1) C = 2 * PI * R
    double tinhDienTich() const override;           // (2) S = PI * R^2
    Diem2D tinhTam() const override;                // (3) Trả về tâm hình tròn
    void tinhTien(double dx, double dy) override;   // (4) Dịch chuyển tâm
    void xoay(double gocRad) override;              // (5) Xoay quanh tâm (vị trí không đổi)
    void scale(double heSoTiLe) override;           // (6) Nhân bán kính với |heSoTiLe|
};

#endif // HINH_TRON_H