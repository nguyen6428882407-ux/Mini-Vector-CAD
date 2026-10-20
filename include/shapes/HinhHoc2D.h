#pragma once
#include <iosfwd>
#include <memory>
#include <string>
#include "core/Diem2D.h"
#include "core/MauSac.h"

class BienDoi2D;   // forward declaration: chi dung lam tham so tham chieu

// ============================================================================
// HinhHoc2D - Lop co so TRUU TUONG cua moi hinh (Composite: Component, Prototype: clone)
// Tang: shapes | Phu trach: Duc Huy
//
// Trach nhiem:
//  - Quan ly thuoc tinh hien thi chung: id, mau vieng (stroke), mau nen (fill), do day net.
//  - Dinh nghia interface truu tuong thuan ao cho toan bo he thong hinh hoc 2D.
//
// QUY UOC THIET KE BAT DI BAT DICH (DESIGN CONTRACT):
//  1. Tinh duy nhat cua ID:
//     - Moi doi tuong khi tao ra duoc he thong cap tu dong mot ID duy nhat (1, 2, 3, ...).
//     - demId_ la bien tinh toan cuc dam bao thu tu ID tang dan tuyen tinh.
//  2. Quy uoc Prototype Pattern (clone):
//     - Copy constructor SAO CHEP style (vien, nen, do day) nhung CAP ID MOI.
//     - clone() tra ve std::unique_ptr<HinhHoc2D> chua ban sao sau (deep copy) co ID doc lap.
//  3. Chong ghi de ID:
//     - Toan tu gan operator= bi xoa (delete) de ngan chan viec ghi de ID va style sai lech.
//  4. Quan ly bo nho hien dai:
//     - Khong quan ly doi tuong qua con tro tho (raw pointer) hoac truyen theo gia tri.
//     - Luon dong goi trong std::unique_ptr<HinhHoc2D> de dam bao RAII va chong ro ri bo nho.
// ============================================================================
class HinhHoc2D {
private:
    static int demId_;   // Bo dem id toan cuc

protected:
    int    id_;
    MauSac vien_;        // Mau vien (mac dinh: den, do duc 255)
    MauSac nen_;         // Mau nen  (mac dinh: trang, trong suot alpha = 0)
    double doDayNet_;    // Do day net ve (mac dinh: 1.0)

    HinhHoc2D();                          // Khoi tao hinh moi -> cap ID moi
    HinhHoc2D(const HinhHoc2D& other);    // Copy constructor: sao chep style, cap ID MOI

public:
    virtual ~HinhHoc2D();
    HinhHoc2D& operator=(const HinhHoc2D&) = delete;

    int getId() const;
    void setMauVien(const MauSac& m);
    void setMauNen(const MauSac& m);
    void setDoDayNet(double d);

    // Getter de tang io (SvgExporter, TextSerializer) va Web doc style
    const MauSac& getMauVien()  const;
    const MauSac& getMauNen()   const;
    double         getDoDayNet() const;

    // In mo ta hinh dang TEXT ra ostream os.
    // Dinh dang chot:
    //  - Hinh don: "<TenLop> #<id>: <thong_so>\n"
    //  - Nhom: dong dau "GroupShape #<id> (<n> hinh)\n", cac con thut le them 2 spaces moi cap.
    virtual void ve(std::ostream& os) const = 0;

    // Tam hinh hoc / Tam hop bao (Bounding Box Center)
    virtual Diem2D tam() const = 0;

    // Tinh dien tich hinh hoc
    virtual double tinhDienTich() const = 0;

    // Tinh chu vi hinh hoc
    virtual double tinhChuVi() const = 0;

    // Ap dung ma tran phep bien doi BienDoi2D len hinh
    virtual void transform(const BienDoi2D& bd) = 0;

    // Prototype Pattern: tao ban sao sau, cap ID moi
    virtual std::unique_ptr<HinhHoc2D> clone() const = 0;

    // Sinh the vector SVG cho hinh (khong xuong dong cuoi)
    virtual std::string toSVG() const = 0;
};
