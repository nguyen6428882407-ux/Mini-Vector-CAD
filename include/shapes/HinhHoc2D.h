#pragma once
#include <iosfwd>
#include <memory>
#include <string>
#include "core/Diem2D.h"
#include "core/MauSac.h"

class BienDoi2D;   // forward declaration: chỉ dùng làm tham số tham chiếu

// ============================================================================
// HinhHoc2D - lớp cơ sở TRỪU TƯỢNG của mọi hình (Composite: Component,
//             Prototype: clone())
// Tầng: shapes | Phụ trách: Đức Huy
// Trách nhiệm: giữ dữ liệu chung (id, màu viền, màu nền, độ dày nét) và
//              quy định giao diện mà mọi hình phải cài đặt.
//
// QUY ƯỚC QUAN TRỌNG:
//  - Mỗi đối tượng có id DUY NHẤT, cấp tự động khi tạo (1, 2, 3, ...).
//  - Copy constructor SAO CHÉP style nhưng CẤP ID MỚI -> clone() chỉ cần
//    std::make_unique<LopCon>(*this) là ra bản sao có id mới.
//  - KHÔNG cho gán (operator=) để id không bị ghi đè.
//  - KHÔNG copy/move hình theo giá trị; luôn quản lý qua std::unique_ptr<HinhHoc2D>.
//    (Lớp này không có move constructor: "move" tự rơi về copy constructor, cũng sinh id MỚI.)
// ============================================================================
class HinhHoc2D {
private:
    static int demId_;   // bộ đếm id toàn cục

protected:
    int    id_;
    MauSac vien_;        // màu viền (mặc định đen, đục)
    MauSac nen_;         // màu nền  (mặc định trắng, trong suốt)
    double doDayNet_;    // độ dày nét viền (mặc định 1.0)

    HinhHoc2D();                          // cấp id mới
    HinhHoc2D(const HinhHoc2D& other);    // copy style, cấp id MỚI

public:
    virtual ~HinhHoc2D();
    HinhHoc2D& operator=(const HinhHoc2D&) = delete;

    int getId() const;
    void setMauVien(const MauSac& m);
    void setMauNen(const MauSac& m);
    void setDoDayNet(double d);

    // Getter để tầng io (SvgExporter, TextSerializer) đọc style.
    const MauSac& getMauVien()  const;
    const MauSac& getMauNen()   const;
    double         getDoDayNet() const;

    // In mô tả hình dạng TEXT ra os. Dùng cho menu và TextSerializer. KHÔNG phải vẽ đồ họa.
    // ĐỊNH DẠNG ĐÃ CHỐT:
    //  - Hình đơn: ĐÚNG MỘT dòng, bắt đầu bằng "<TenLop> #<id>" rồi các thông số, kết thúc bằng '\n'.
    //      ví dụ: "HinhTron #3: tam=(0,0) r=5\n"
    //  - Nhóm: dòng đầu "GroupShape #<id> (<n> hinh)\n", sau đó mỗi hình con ở các dòng tiếp theo,
    //    thụt lề thêm 2 dấu cách cho mỗi cấp lồng; MỌI dòng đều kết thúc bằng '\n'.
    //  - Không in thêm dòng trống ở đầu/cuối, để người gọi nối chuỗi tùy ý.
    virtual void ve(std::ostream& os) const = 0;

    // Tâm của hộp bao (bounding box) của hình; với nhóm là tâm hộp bao của mọi hình con.
    // MenuController dùng làm tâm mặc định khi xoay/scale quanh hình.
    virtual Diem2D tam() const = 0;

    virtual double tinhDienTich() const = 0;
    virtual double tinhChuVi() const = 0;

    // Áp dụng phép biến đổi lên hình (thay đổi chính hình này).
    virtual void transform(const BienDoi2D& bd) = 0;

    // Prototype: tạo bản sao SÂU, id MỚI. Trả về quyền sở hữu cho người gọi.
    virtual std::unique_ptr<HinhHoc2D> clone() const = 0;

    // Sinh thẻ SVG của chính hình này (một chuỗi, không xuống dòng cuối).
    virtual std::string toSVG() const = 0;
};
