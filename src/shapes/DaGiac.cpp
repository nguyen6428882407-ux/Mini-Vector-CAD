#include "shapes/DaGiac.h"
#include "core/BienDoi2D.h"
#include <sstream>
#include <cmath>

DaGiac::DaGiac(const std::vector<Diem2D>& dinh) : dinh_(dinh) {}
void DaGiac::themDinh(const Diem2D& d) { dinh_.push_back(d); }
const std::vector<Diem2D>& DaGiac::getDinh() const { return dinh_; }

void DaGiac::ve(std::ostream& os) const {
    os << "DaGiac #" << getId() << ": " << dinh_.size() << " dinh [";
    for (size_t i = 0; i < dinh_.size(); ++i) {
        os << dinh_[i];
        if (i + 1 < dinh_.size()) os << " ";
    }
    os << "]\n";
}

Diem2D DaGiac::tam() const {
    if (dinh_.empty()) return Diem2D(0, 0);
    double sx = 0.0, sy = 0.0;
    for (const auto& d : dinh_) { sx += d.getX(); sy += d.getY(); }
    return Diem2D(sx / static_cast<double>(dinh_.size()), sy / static_cast<double>(dinh_.size()));
}

double DaGiac::tinhDienTich() const {
    if (dinh_.size() < 3) return 0.0;
    double tong = 0.0;
    size_t n = dinh_.size();
    for (size_t i = 0; i < n; ++i) {
        size_t next = (i + 1) % n;
        tong += dinh_[i].getX() * dinh_[next].getY() - dinh_[next].getX() * dinh_[i].getY();
    }
    return std::abs(tong) / 2.0;
}

double DaGiac::tinhChuVi() const {
    if (dinh_.size() < 2) return 0.0;
    double cv = 0.0;
    size_t n = dinh_.size();
    for (size_t i = 0; i < n; ++i) { cv += dinh_[i].khoangCach(dinh_[(i + 1) % n]); }
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
    for (size_t i = 0; i < dinh_.size(); ++i) {
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
