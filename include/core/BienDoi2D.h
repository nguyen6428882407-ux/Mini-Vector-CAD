#pragma once
#include "core/Diem2D.h"

// ============================================================================
// BienDoi2D - phép biến đổi affine 2D (ma trận 3x3, kiểu giá trị)
// Tầng: core | Phụ trách: Nhất Vũ
// Trách nhiệm: tạo và ghép các phép tịnh tiến / xoay / scale đều,
//              áp dụng lên điểm, và tính phép nghịch đảo (cho undo).
// Ghi chú: CHỈ hỗ trợ scale đều (một hệ số k) -> hình tròn không thành ellipse.
//
// QUY ƯỚC TOÁN HỌC (đã chốt khi cài đặt Task 2):
//  - Tọa độ thuần nhất, VECTƠ CỘT:  (x', y', 1)^T = M * (x, y, 1)^T.
//  - Ma trận lưu theo hàng: m_[hàng][cột]. Hàng cuối LUÔN là (0, 0, 1) (chỉ tạo được qua các
//    hàm của lớp này nên bất biến luôn đúng).
//  - Góc xoay tính bằng ĐỘ; góc DƯƠNG = ngược chiều kim đồng hồ trong hệ tọa độ toán học
//    (trục y hướng LÊN). Trong SVG/màn hình (trục y hướng XUỐNG) cùng phép xoay đó trông như
//    THUẬN chiều kim đồng hồ. [ASSUMPTION: skeleton chưa quy định chiều xoay, cần xác nhận]
//  - Góc là bội của 90 độ (sai số dưới MathUtils::EPS = 1e-9 độ ~ 1.7e-11 rad) được làm tròn
//    về cos/sin CHÍNH XÁC (0, +-1) để tọa độ xuất ra sạch (không có 6.1e-17). Sai số do làm tròn
//    này tối đa ~1.7e-11 * (khoảng cách tới tâm), tức ~1.7e-5 ở khoảng cách 1e6.
//  - Loại ngoại lệ: tham số sai -> std::invalid_argument (tiLeDeu); ma trận không khả nghịch,
//    không hữu hạn, hoặc nghịch đảo bị tràn số -> std::domain_error (nghichDao). Cả hai đều là
//    std::logic_error nên bắt được bằng catch (const std::exception&).
//  - Thứ tự ghép: (A * B).apDung(p) == A.apDung(B.apDung(p)), tức phép B (bên PHẢI) được áp dụng
//    TRƯỚC. Phép nhân KHÔNG giao hoán: A * B nói chung khác B * A.
//  - Các hàm tạo (tinhTien/xoay/tiLeDeu) KHÔNG kiểm tra NaN/Inf của đối số (trừ tiLeDeu, xem dưới):
//    đầu vào phải hữu hạn; NaN/Inf sẽ lan truyền. nghichDao() sẽ phát hiện ma trận không hữu hạn.
// ============================================================================
class BienDoi2D {
private:
    double m_[3][3];   // ma trận affine, mặc định là ma trận đơn vị

public:
    BienDoi2D();       // ma trận đơn vị (không biến đổi gì)

    // --- Hàm tạo phép biến đổi ---
    static BienDoi2D tinhTien(double dx, double dy);
    static BienDoi2D xoay(double gocDo, const Diem2D& tam);      // góc tính bằng ĐỘ
    // HỢP ĐỒNG tiLeDeu: ném std::invalid_argument nếu |k| < MathUtils::EPS (không khả nghịch).
    // k ÂM được chấp nhận (tương đương xoay 180 độ quanh tâm rồi scale |k|).
    static BienDoi2D tiLeDeu(double k, const Diem2D& tam);

    // Áp dụng phép biến đổi lên một điểm.
    Diem2D apDung(const Diem2D& p) const;

    // Hệ số phóng to/thu nhỏ (luôn DƯƠNG, = |k| với scale đều) - dùng cho bán kính hình tròn.
    // Đúng với mọi k hữu hạn (tính không tràn số, kể cả |k| > 1e154); NaN nếu ma trận không hữu hạn.
    double heSoTiLe() const;

    // Phép nghịch đảo: M.nghichDao() * M ~ ma trận đơn vị.
    // HỢP ĐỒNG: ném std::domain_error nếu ma trận KHÔNG khả nghịch hoặc không hữu hạn, cụ thể khi
    // heSoTiLe() < MathUtils::EPS (cùng ngưỡng với tiLeDeu) hoặc có phần tử NaN/Inf, hoặc phần tịnh tiến
    // của nghịch đảo bị tràn số (Inf). KHÔNG bao giờ âm thầm trả về kết quả sai.
    BienDoi2D nghichDao() const;

    // Ghép phép biến đổi: (a * b).apDung(p) == a.apDung(b.apDung(p)).
    BienDoi2D operator*(const BienDoi2D& o) const;
};
