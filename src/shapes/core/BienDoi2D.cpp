#include "core/BienDoi2D.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "core/MathUtils.h"

namespace {

// cos/sin của góc tính bằng ĐỘ. Các góc là bội của 90 độ cho kết quả CHÍNH XÁC (0 hoặc +-1) thay vì
// sai số kiểu 6.1e-17, để tọa độ in ra SVG/Text sạch (xoay 90 độ điểm (1,0) ra đúng (0,1)).
void cosSinDo(double gocDo, double& c, double& s) {
    double g = std::fmod(gocDo, 360.0);   // [-360, 360)
    if (g < 0.0) g += 360.0;              // [0, 360]
    if (MathUtils::bangNhau(g, 0.0) || MathUtils::bangNhau(g, 360.0)) { c = 1.0;  s = 0.0;  return; }
    if (MathUtils::bangNhau(g, 90.0))  { c = 0.0;  s = 1.0;  return; }
    if (MathUtils::bangNhau(g, 180.0)) { c = -1.0; s = 0.0;  return; }
    if (MathUtils::bangNhau(g, 270.0)) { c = 0.0;  s = -1.0; return; }
    const double rad = MathUtils::doSangRad(g);
    c = std::cos(rad);
    s = std::sin(rad);
}

// Phần tuyến tính [[a, b], [c, d]] của phép biến đổi. Trả về hệ số tỉ lệ sqrt(|det|) KHÔNG TRÀN SỐ
// và max|phần tử| qua tham chiếu sc: chia mọi phần tử cho sc trước khi tính định thức rồi nhân lại.
// (Tính trực tiếp det = a*d - b*c sẽ tràn thành Inf khi |k| > ~1.3e154 dù k hoàn toàn hợp lệ.)
// Trả về NaN nếu có phần tử không hữu hạn; 0 nếu tất cả bằng 0. Khi trả về NaN hoặc 0 thì sc không dùng.
double heSoTuyenTinh(double a, double b, double c, double d, double& sc) {
    sc = 0.0;
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || !std::isfinite(d))
        return std::numeric_limits<double>::quiet_NaN();
    sc = std::max(std::max(std::fabs(a), std::fabs(b)), std::max(std::fabs(c), std::fabs(d)));
    if (sc == 0.0) return 0.0;
    const double det = (a / sc) * (d / sc) - (b / sc) * (c / sc);   // cỡ O(1), không tràn
    return sc * std::sqrt(std::fabs(det));
}

}  // namespace

BienDoi2D::BienDoi2D() {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m_[i][j] = (i == j) ? 1.0 : 0.0;
}

BienDoi2D BienDoi2D::tinhTien(double dx, double dy) {
    BienDoi2D r;
    r.m_[0][2] = dx;
    r.m_[1][2] = dy;
    return r;
}

BienDoi2D BienDoi2D::xoay(double gocDo, const Diem2D& tam) {
    double c, s;
    cosSinDo(gocDo, c, s);
    const double tx = tam.getX();
    const double ty = tam.getY();
    // Xoay quanh tam = T(tam) * R * T(-tam). Tính thẳng phần tịnh tiến: t = tam - R * tam
    // (một lần tính, ít sai số hơn là nhân ba ma trận).
    BienDoi2D r;
    r.m_[0][0] = c;   r.m_[0][1] = -s;  r.m_[0][2] = tx - (c * tx - s * ty);
    r.m_[1][0] = s;   r.m_[1][1] = c;   r.m_[1][2] = ty - (s * tx + c * ty);
    return r;
}

BienDoi2D BienDoi2D::tiLeDeu(double k, const Diem2D& tam) {
    // !(|k| >= EPS) (thay vì |k| < EPS) để bắt luôn trường hợp NaN.
    if (!std::isfinite(k) || !(std::fabs(k) >= MathUtils::EPS))
        throw std::invalid_argument("BienDoi2D::tiLeDeu: he so k khong hop le (|k| qua nho hoac khong huu han)");
    const double tx = tam.getX();
    const double ty = tam.getY();
    // Scale đều quanh tam = T(tam) * S(k) * T(-tam): phần tịnh tiến = tam - k * tam.
    BienDoi2D r;
    r.m_[0][0] = k;   r.m_[0][2] = tx - k * tx;
    r.m_[1][1] = k;   r.m_[1][2] = ty - k * ty;
    return r;
}

Diem2D BienDoi2D::apDung(const Diem2D& p) const {
    const double x = p.getX();
    const double y = p.getY();
    return Diem2D(m_[0][0] * x + m_[0][1] * y + m_[0][2],
                  m_[1][0] * x + m_[1][1] * y + m_[1][2]);
}

double BienDoi2D::heSoTiLe() const {
    // Phần tuyến tính 2x2 là (xoay) * (scale k) nên định thức = k^2 -> căn = |k|.
    // Dùng heSoTuyenTinh (chuẩn hóa trước khi tính det) để không tràn khi |k| rất lớn.
    double sc;
    return heSoTuyenTinh(m_[0][0], m_[0][1], m_[1][0], m_[1][1], sc);
}

BienDoi2D BienDoi2D::nghichDao() const {
    const double a = m_[0][0], b = m_[0][1], tx = m_[0][2];
    const double c = m_[1][0], d = m_[1][1], ty = m_[1][2];

    // Không khả nghịch / không hữu hạn: ném lỗi, KHÔNG trả kết quả sai.
    // Ngưỡng tính trên sqrt(|det|) = heSoTiLe() để khớp với ngưỡng |k| < EPS của tiLeDeu
    // (nếu so det với EPS thì scale hợp lệ k = 1e-5 có det = 1e-10 sẽ bị loại oan).
    // !(h >= EPS) bắt luôn trường hợp NaN (phần tử không hữu hạn).
    double sc;
    const double h = heSoTuyenTinh(a, b, c, d, sc);
    if (!std::isfinite(tx) || !std::isfinite(ty) || !(h >= MathUtils::EPS))
        throw std::domain_error("BienDoi2D::nghichDao: ma tran khong kha nghich hoac khong huu han");

    // Nghịch đảo 2x2 tính trên ma trận đã chia cho sc (tránh tràn khi |k| lớn): A^-1 = (1/sc) * inv(A/sc).
    const double det = (a / sc) * (d / sc) - (b / sc) * (c / sc);   // |det| >= 1 với phép đồng dạng
    BienDoi2D r;
    // Phần tuyến tính: nghịch đảo ma trận 2x2.
    r.m_[0][0] =  ((d / sc) / det) / sc;   r.m_[0][1] = -((b / sc) / det) / sc;
    r.m_[1][0] = -((c / sc) / det) / sc;   r.m_[1][1] =  ((a / sc) / det) / sc;
    // Phần tịnh tiến: -A^-1 * t.
    r.m_[0][2] = -(r.m_[0][0] * tx + r.m_[0][1] * ty);
    r.m_[1][2] = -(r.m_[1][0] * tx + r.m_[1][1] * ty);
    // Phần tịnh tiến có thể tràn (ví dụ tịnh tiến 1e300 rồi scale 1e-9): ném thay vì trả Inf âm thầm.
    if (!std::isfinite(r.m_[0][2]) || !std::isfinite(r.m_[1][2]))
        throw std::domain_error("BienDoi2D::nghichDao: phan tinh tien cua nghich dao bi tran so");
    return r;
}

BienDoi2D BienDoi2D::operator*(const BienDoi2D& o) const {
    // Tích ma trận chuẩn: r = (*this) * o. Phép o (bên phải) được áp dụng TRƯỚC.
    BienDoi2D r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double tong = 0.0;
            for (int k = 0; k < 3; ++k)
                tong += m_[i][k] * o.m_[k][j];
            r.m_[i][j] = tong;
        }
    }
    return r;
}
