#include "shapes/DaGiac.h"
#include "core/BienDoi2D.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <sstream>

DaGiac::DaGiac(const std::vector<Diem2D>& dinh) : dinh_(dinh) {}
void DaGiac::themDinh(const Diem2D& d) { dinh_.push_back(d); }
const std::vector<Diem2D>& DaGiac::getDinh() const { return dinh_; }

void DaGiac::ve(std::ostream& os) const {
    os << "DaGiac #" << getId() << ": " << dinh_.size() << " dinh [";
    for (std::size_t i = 0; i < dinh_.size(); ++i) {
        os << dinh_[i];
        if (i + 1 < dinh_.size()) os << " ";
    }
    os << "]\n";
}

// Tâm của HỘP BAO (theo hợp đồng HinhHoc2D::tam()), không phải trung bình các đỉnh.
Diem2D DaGiac::tam() const {
    if (dinh_.empty()) return Diem2D(0, 0);
    double minX = dinh_[0].getX(), maxX = dinh_[0].getX();
    double minY = dinh_[0].getY(), maxY = dinh_[0].getY();
    for (const auto& d : dinh_) {
        minX = std::min(minX, d.getX());
        maxX = std::max(maxX, d.getX());
        minY = std::min(minY, d.getY());
        maxY = std::max(maxY, d.getY());
    }
    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

// Công thức Shoelace, tính theo tọa độ tương đối so với đỉnh đầu tiên (tịnh tiến gốc về
// dinh_[0] để giảm sai số triệt tiêu khi tọa độ lớn; kết quả toán học không đổi).
double DaGiac::tinhDienTich() const {
    if (dinh_.size() < 3) return 0.0;
    const std::size_t n = dinh_.size();
    const double x0 = dinh_[0].getX(), y0 = dinh_[0].getY();
    double tong = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t next = (i + 1) % n;
        const double xi = dinh_[i].getX() - x0,    yi = dinh_[i].getY() - y0;
        const double xj = dinh_[next].getX() - x0, yj = dinh_[next].getY() - y0;
        tong += xi * yj - xj * yi;
    }
    return std::abs(tong) / 2.0;
}

double DaGiac::tinhChuVi() const {
    if (dinh_.size() < 2) return 0.0;
    double cv = 0.0;
    const std::size_t n = dinh_.size();
    for (std::size_t i = 0; i < n; ++i) { cv += dinh_[i].khoangCach(dinh_[(i + 1) % n]); }
    return cv;
}

void DaGiac::transform(const BienDoi2D& bd) {
    for (auto& d : dinh_) { d = bd.apDung(d); }
}

std::unique_ptr<HinhHoc2D> DaGiac::clone() const {
    return std::unique_ptr<DaGiac>(new DaGiac(*this));
}

std::string DaGiac::toSVG() const {
    std::ostringstream oss;
    oss << "<polygon points=\"";
    for (std::size_t i = 0; i < dinh_.size(); ++i) {
        oss << dinh_[i].getX() << "," << dinh_[i].getY();
        if (i + 1 < dinh_.size()) oss << " ";
    }
    oss << "\" stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " fill=\"" << nen_.toHex(false) << "\""
        << " fill-opacity=\"" << (nen_.getA() / 255.0) << "\" />";
    return oss.str();
}
