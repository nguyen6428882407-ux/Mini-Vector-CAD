#include "shapes/GroupShape.h"
#include "shapes/DaGiac.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/HinhTron.h"
#include "shapes/TamGiac.h"
#include <sstream>
#include <algorithm>

namespace {
    void includePoint(const Diem2D& point, double& minX, double& minY, double& maxX, double& maxY, bool& initialized) {
        if (!initialized) {
            minX = maxX = point.getX();
            minY = maxY = point.getY();
            initialized = true;
            return;
        }
        minX = std::min(minX, point.getX());
        minY = std::min(minY, point.getY());
        maxX = std::max(maxX, point.getX());
        maxY = std::max(maxY, point.getY());
    }

    void includeShapeBounds(const HinhHoc2D& shape, double& minX, double& minY, double& maxX, double& maxY, bool& initialized) {
        if (const auto* group = dynamic_cast<const GroupShape*>(&shape)) {
            for (const auto& child : group->getDanhSachCon()) {
                if (child) includeShapeBounds(*child, minX, minY, maxX, maxY, initialized);
            }
        } else if (const auto* circle = dynamic_cast<const HinhTron*>(&shape)) {
            const Diem2D& center = circle->getTam();
            const double radius = circle->getBanKinh();
            includePoint(Diem2D(center.getX() - radius, center.getY() - radius), minX, minY, maxX, maxY, initialized);
            includePoint(Diem2D(center.getX() + radius, center.getY() + radius), minX, minY, maxX, maxY, initialized);
        } else if (const auto* line = dynamic_cast<const DoanThang*>(&shape)) {
            includePoint(line->getDiem1(), minX, minY, maxX, maxY, initialized);
            includePoint(line->getDiem2(), minX, minY, maxX, maxY, initialized);
        } else if (const auto* rectangle = dynamic_cast<const HinhChuNhat*>(&shape)) {
            for (const auto& point : rectangle->getDinh()) {
                includePoint(point, minX, minY, maxX, maxY, initialized);
            }
        } else if (const auto* triangle = dynamic_cast<const TamGiac*>(&shape)) {
            for (const auto& point : triangle->getDinh()) {
                includePoint(point, minX, minY, maxX, maxY, initialized);
            }
        } else if (const auto* polygon = dynamic_cast<const DaGiac*>(&shape)) {
            for (const auto& point : polygon->getDinh()) {
                includePoint(point, minX, minY, maxX, maxY, initialized);
            }
        } else {
            includePoint(shape.tam(), minX, minY, maxX, maxY, initialized);
        }
    }
}

GroupShape::GroupShape() : HinhHoc2D() {}

GroupShape::GroupShape(const GroupShape& other) : HinhHoc2D(other) {
    for (const auto& con : other.danhSachCon_) {
        if (con) danhSachCon_.push_back(con->clone());
    }
}

void GroupShape::themHinh(std::unique_ptr<HinhHoc2D> hinh) {
    if (!hinh || hinh.get() == this) return;
    danhSachCon_.push_back(std::move(hinh));
}

void GroupShape::them(std::unique_ptr<HinhHoc2D> hinh) {
    themHinh(std::move(hinh));
}

std::unique_ptr<HinhHoc2D> GroupShape::rutHinh(int id) {
    for (auto it = danhSachCon_.begin(); it != danhSachCon_.end(); ++it) {
        if ((*it)->getId() == id) {
            auto hinh = std::move(*it);
            danhSachCon_.erase(it);
            return hinh;
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<HinhHoc2D>>& GroupShape::getDanhSachCon() const {
    return danhSachCon_;
}

size_t GroupShape::soLuongCon() const {
    return danhSachCon_.size();
}

size_t GroupShape::soLuong() const {
    return danhSachCon_.size();
}

HinhHoc2D& GroupShape::hinhThu(size_t i) {
    return *danhSachCon_.at(i);
}

const HinhHoc2D& GroupShape::hinhThu(size_t i) const {
    return *danhSachCon_.at(i);
}

const HinhHoc2D* GroupShape::timHinhCon(int id) const {
    if (id <= 0) return nullptr;
    for (const auto& con : danhSachCon_) {
        if (!con) continue;
        if (con->getId() == id) return con.get();
        if (const auto* nhomCon = dynamic_cast<const GroupShape*>(con.get())) {
            const HinhHoc2D* kq = nhomCon->timHinhCon(id);
            if (kq) return kq;
        }
    }
    return nullptr;
}

HinhHoc2D* GroupShape::timHinhCon(int id) {
    return const_cast<HinhHoc2D*>(static_cast<const GroupShape*>(this)->timHinhCon(id));
}

bool GroupShape::chuaHinh(int id) const {
    return timHinhCon(id) != nullptr;
}

void GroupShape::xoaTatCa() {
    danhSachCon_.clear();
}

void GroupShape::ve(std::ostream& os) const {
    os << "GroupShape #" << getId() << " (" << danhSachCon_.size() << " hinh)\n";
    for (const auto& con : danhSachCon_) {
        std::ostringstream ss;
        con->ve(ss);
        std::istringstream iss(ss.str());
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) os << "  " << line << "\n";
        }
    }
}

Diem2D GroupShape::tam() const {
    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
    bool initialized = false;
    for (const auto& con : danhSachCon_) {
        if (con) includeShapeBounds(*con, minX, minY, maxX, maxY, initialized);
    }
    if (!initialized) return Diem2D(0.0, 0.0);
    return Diem2D(minX + (maxX - minX) / 2.0, minY + (maxY - minY) / 2.0);
}

double GroupShape::tinhDienTich() const {
    double tong = 0.0;
    for (const auto& con : danhSachCon_) tong += con->tinhDienTich();
    return tong;
}

double GroupShape::tinhChuVi() const {
    double tong = 0.0;
    for (const auto& con : danhSachCon_) tong += con->tinhChuVi();
    return tong;
}

void GroupShape::transform(const BienDoi2D& bd) {
    for (auto& con : danhSachCon_) con->transform(bd);
}

std::unique_ptr<HinhHoc2D> GroupShape::clone() const {
    return std::unique_ptr<GroupShape>(new GroupShape(*this));
}

std::string GroupShape::toSVG() const {
    std::ostringstream oss;
    oss << "<g id=\"group_" << getId() << "\">\n";
    for (const auto& con : danhSachCon_) {
        if (con) oss << "  " << con->toSVG() << "\n";
    }
    oss << "</g>";
    return oss.str();
}

std::unique_ptr<GroupShape> operator+(std::unique_ptr<HinhHoc2D> a, std::unique_ptr<HinhHoc2D> b) {
    auto nhom = std::make_unique<GroupShape>();
    if (a) nhom->themHinh(std::move(a));
    if (b) nhom->themHinh(std::move(b));
    return nhom;
}

std::unique_ptr<GroupShape> operator+(const HinhHoc2D& a, const HinhHoc2D& b) {
    auto nhom = std::make_unique<GroupShape>();
    nhom->themHinh(a.clone());
    nhom->themHinh(b.clone());
    return nhom;
}
