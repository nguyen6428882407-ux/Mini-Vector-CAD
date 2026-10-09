#include "io/TextSerializer.h"
#include "canvas/BangVeCanvas.h"
#include "shapes/ShapeFactory.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/TamGiac.h"
#include "shapes/DaGiac.h"
#include "shapes/GroupShape.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace {
    void serializeHinh(const HinhHoc2D& hinh, std::ostream& os) {
        if (auto tron = dynamic_cast<const HinhTron*>(&hinh)) {
            os << "tron " << tron->getTam().getX() << " " << tron->getTam().getY() << " " << tron->getBanKinh();
        } else if (auto dt = dynamic_cast<const DoanThang*>(&hinh)) {
            os << "doanthang " << dt->getDiem1().getX() << " " << dt->getDiem1().getY() << " "
               << dt->getDiem2().getX() << " " << dt->getDiem2().getY();
        } else if (auto cn = dynamic_cast<const HinhChuNhat*>(&hinh)) {
            const auto& d = cn->getDinh();
            double w = d[0].khoangCach(d[1]);
            double h = d[1].khoangCach(d[2]);
            os << "chunhat " << d[0].getX() << " " << d[0].getY() << " " << w << " " << h;
        } else if (auto tg = dynamic_cast<const TamGiac*>(&hinh)) {
            const auto& d = tg->getDinh();
            os << "tamgiac " << d[0].getX() << " " << d[0].getY() << " "
               << d[1].getX() << " " << d[1].getY() << " "
               << d[2].getX() << " " << d[2].getY();
        } else if (auto dg = dynamic_cast<const DaGiac*>(&hinh)) {
            os << "dagiac";
            for (const auto& pt : dg->getDinh()) {
                os << " " << pt.getX() << " " << pt.getY();
            }
        } else {
            return;
        }

        const auto& mv = hinh.getMauVien();
        const auto& mn = hinh.getMauNen();
        os << " vien " << mv.getR() << " " << mv.getG() << " " << mv.getB() << " " << mv.getA()
           << " nen " << mn.getR() << " " << mn.getG() << " " << mn.getB() << " " << mn.getA()
           << " net " << hinh.getDoDayNet() << "\n";
    }
}

bool TextSerializer::ghiFile(const BangVeCanvas& canvas, const std::string& duongDan) {
    std::ofstream ofs(duongDan);
    if (!ofs.is_open()) return false;
    serialize(canvas, ofs);
    return true;
}

bool TextSerializer::docFile(BangVeCanvas& canvas, const std::string& duongDan) {
    std::ifstream ifs(duongDan);
    if (!ifs.is_open()) return false;
    deserialize(canvas, ifs);
    return true;
}

void TextSerializer::serialize(const BangVeCanvas& canvas, std::ostream& os) {
    os << "MINI_VECTOR_CAD_V1\n";
    os << "LAYERS " << canvas.soLuongLayer() << "\n";

    for (size_t i = 0; i < canvas.soLuongLayer(); ++i) {
        const auto& lay = canvas.layer(i);
        os << "LAYER \"" << lay.getTen() << "\" "
           << (lay.isHienThi() ? 1 : 0) << " "
           << (lay.isKhoa() ? 1 : 0) << " "
           << lay.soLuongHinh() << "\n";

        for (const auto& hinh : lay.getDanhSachHinh()) {
            if (hinh) serializeHinh(*hinh, os);
        }
    }
}

void TextSerializer::deserialize(BangVeCanvas& canvas, std::istream& is) {
    std::string header;
    if (!(is >> header) || header != "MINI_VECTOR_CAD_V1") return;

    std::string tagLayers;
    size_t soLuongLayer = 0;
    if (!(is >> tagLayers >> soLuongLayer)) return;

    canvas.xoaTatCa();
    std::string line;
    std::getline(is, line);

    for (size_t i = 0; i < soLuongLayer; ++i) {
        std::getline(is, line);
        if (line.empty()) continue;

        size_t q1 = line.find('\"');
        size_t q2 = line.rfind('\"');
        std::string tenLayer = "Layer";
        if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
            tenLayer = line.substr(q1 + 1, q2 - q1 - 1);
        }

        std::string phanConLai = line.substr(q2 + 1);
        std::istringstream iss(phanConLai);
        int hienThi = 1, khoa = 0;
        size_t soHinh = 0;
        iss >> hienThi >> khoa >> soHinh;

        if (i == 0) {
            canvas.layer(0).setTen(tenLayer);
            canvas.layer(0).setHienThi(hienThi == 1);
            canvas.layer(0).setKhoa(khoa == 1);
        } else {
            size_t idx = canvas.themLayer(tenLayer);
            canvas.layer(idx).setHienThi(hienThi == 1);
            canvas.layer(idx).setKhoa(khoa == 1);
        }

        Layer& lay = canvas.layer(i);
        for (size_t j = 0; j < soHinh; ++j) {
            std::string lineHinh;
            if (std::getline(is, lineHinh)) {
                if (lineHinh.empty()) { --j; continue; }
                try {
                    auto hinh = ShapeFactory::taoHinh(lineHinh);
                    if (hinh) lay.themHinh(std::move(hinh));
                } catch (...) {}
            }
        }
    }
}

void TextSerializer::inBangVe(const BangVeCanvas& canvas, std::ostream& os) {
    os << "==================== BANG VE CANVAS ====================\n";
    os << "Tong so Layer: " << canvas.soLuongLayer() << "\n";
    for (size_t i = 0; i < canvas.soLuongLayer(); ++i) {
        const auto& lay = canvas.layer(i);
        os << "--------------------------------------------------------\n";
        os << "[Layer " << i << "] \"" << lay.getTen() << "\""
           << " | Hien thi: " << (lay.isHienThi() ? "BAT" : "TAT")
           << " | Khoa: " << (lay.isKhoa() ? "KHOA" : "MO")
           << " | So hinh: " << lay.soLuongHinh() << "\n";

        if (lay.soLuongHinh() == 0) {
            os << "  (Trong)\n";
        } else {
            for (const auto& hinh : lay.getDanhSachHinh()) {
                if (hinh) {
                    os << "  * ";
                    hinh->ve(os);
                }
            }
        }
    }
    os << "========================================================\n";
}
