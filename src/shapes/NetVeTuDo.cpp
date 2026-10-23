#include "shapes/NetVeTuDo.h"
#include "core/BienDoi2D.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

NetVeTuDo::NetVeTuDo(const std::vector<Diem2D>& diem) : diem_(diem) {
    if (diem_.empty()) {
        throw std::invalid_argument("Net ve tu do phai co it nhat mot diem.");
    }
    for (const auto& point : diem_) {
        if (!std::isfinite(point.getX()) || !std::isfinite(point.getY())) {
            throw std::invalid_argument("Toa do net ve tu do phai la so huu han.");
        }
    }
}

const std::vector<Diem2D>& NetVeTuDo::getDiem() const { return diem_; }

void NetVeTuDo::ve(std::ostream& os) const {
    os << "NetVeTuDo #" << getId() << ": " << diem_.size() << " diem [";
    for (std::size_t i = 0; i < diem_.size(); ++i) {
        os << diem_[i];
        if (i + 1 < diem_.size()) os << " ";
    }
    os << "]\n";
}

Diem2D NetVeTuDo::tam() const {
    double minX = diem_.front().getX();
    double maxX = minX;
    double minY = diem_.front().getY();
    double maxY = minY;
    for (const auto& point : diem_) {
        minX = std::min(minX, point.getX());
        maxX = std::max(maxX, point.getX());
        minY = std::min(minY, point.getY());
        maxY = std::max(maxY, point.getY());
    }
    return Diem2D((minX + maxX) / 2.0, (minY + maxY) / 2.0);
}

double NetVeTuDo::tinhDienTich() const { return 0.0; }

double NetVeTuDo::tinhChuVi() const {
    double length = 0.0;
    for (std::size_t i = 1; i < diem_.size(); ++i) {
        length += diem_[i - 1].khoangCach(diem_[i]);
    }
    return length;
}

void NetVeTuDo::transform(const BienDoi2D& bd) {
    for (auto& point : diem_) point = bd.apDung(point);
}

std::unique_ptr<HinhHoc2D> NetVeTuDo::clone() const {
    return std::unique_ptr<NetVeTuDo>(new NetVeTuDo(*this));
}

std::string NetVeTuDo::toSVG() const {
    std::ostringstream oss;
    if (diem_.size() == 1) {
        oss << "<circle cx=\"" << diem_[0].getX() << "\" cy=\"" << diem_[0].getY()
            << "\" r=\"" << doDayNet_ / 2.0 << "\" fill=\"" << vien_.toHex(false) << "\""
            << " fill-opacity=\"" << (vien_.getA() / 255.0) << "\" />";
        return oss.str();
    }
    oss << "<polyline points=\"";
    for (std::size_t i = 0; i < diem_.size(); ++i) {
        oss << diem_[i].getX() << "," << diem_[i].getY();
        if (i + 1 < diem_.size()) oss << " ";
    }
    oss << "\" fill=\"none\" stroke=\"" << vien_.toHex(false) << "\""
        << " stroke-opacity=\"" << (vien_.getA() / 255.0) << "\""
        << " stroke-width=\"" << doDayNet_ << "\""
        << " stroke-linecap=\"round\" stroke-linejoin=\"round\" />";
    return oss.str();
}
