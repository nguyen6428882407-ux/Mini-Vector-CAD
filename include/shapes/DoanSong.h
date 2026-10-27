#pragma once
#include "shapes/HinhHoc2D.h"

class DoanSong : public HinhHoc2D {
private:
    Diem2D p1_;       // Diem bat dau
    Diem2D p2_;       // Diem ket thuc
    double bienDo_;   // Bien do song (chieu cao ngon song)
    double soSong_;   // So chu ky song

public:
    DoanSong(const Diem2D& p1, const Diem2D& p2, double bienDo = 15.0, double soSong = 3.0);
    DoanSong(double x1, double y1, double x2, double y2, double bienDo = 15.0, double soSong = 3.0);

    const Diem2D& getDiem1() const;
    const Diem2D& getDiem2() const;
    double getBienDo() const;
    double getSoSong() const;

    void setDiem1(const Diem2D& p);
    void setDiem2(const Diem2D& p);
    void setBienDo(double b);
    void setSoSong(double s);

    void ve(std::ostream& os) const override;
    Diem2D tam() const override;
    double tinhDienTich() const override;
    double tinhChuVi() const override;
    void transform(const BienDoi2D& bd) override;
    std::unique_ptr<HinhHoc2D> clone() const override;
    std::string toSVG() const override;
};//
