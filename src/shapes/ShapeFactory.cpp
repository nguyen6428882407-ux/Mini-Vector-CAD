#include "shapes/ShapeFactory.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/TamGiac.h"
#include "shapes/DaGiac.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace {
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    }
}

std::unique_ptr<HinhHoc2D> ShapeFactory::taoHinh(const std::string& moTa) {
    if (moTa.empty()) return nullptr;

    std::istringstream iss(moTa);
    std::string loai;
    if (!(iss >> loai)) return nullptr;

    loai = toLower(loai);
    std::unique_ptr<HinhHoc2D> hinh = nullptr;

    if (loai == "tron" || loai == "circle") {
        double cx = 0, cy = 0, r = 0;
        if (!(iss >> cx >> cy >> r)) {
            throw std::invalid_argument("Cu phap hinh tron sai: can <cx> <cy> <r>");
        }
        hinh = std::make_unique<HinhTron>(cx, cy, r);
    }
    else if (loai == "doanthang" || loai == "line") {
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!(iss >> x1 >> y1 >> x2 >> y2)) {
            throw std::invalid_argument("Cu phap doan thang sai: can <x1> <y1> <x2> <y2>");
        }
        hinh = std::make_unique<DoanThang>(x1, y1, x2, y2);
    }
    else if (loai == "chunhat" || loai == "rect" || loai == "rectangle") {
        double x = 0, y = 0, w = 0, h = 0;
        if (!(iss >> x >> y >> w >> h)) {
            throw std::invalid_argument("Cu phap chu nhat sai: can <x> <y> <w> <h>");
        }
        hinh = std::make_unique<HinhChuNhat>(x, y, w, h);
    }
    else if (loai == "tamgiac" || loai == "triangle") {
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
        if (!(iss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3)) {
            throw std::invalid_argument("Cu phap tam giac sai: can <x1> <y1> <x2> <y2> <x3> <y3>");
        }
        hinh = std::make_unique<TamGiac>(x1, y1, x2, y2, x3, y3);
    }
    else if (loai == "dagiac" || loai == "polygon") {
        std::vector<Diem2D> cacDinh;
        double x = 0, y = 0;
        while (iss >> x >> y) {
            cacDinh.emplace_back(x, y);
            std::streampos pos = iss.tellg();
            std::string peek;
            if (iss >> peek) {
                std::string peekLower = toLower(peek);
                if (peekLower == "vien" || peekLower == "nen" || peekLower == "net" ||
                    peekLower == "stroke" || peekLower == "fill" || peekLower == "width") {
                    iss.seekg(pos);
                    break;
                }
                std::istringstream testCoord(peek);
                double testVal = 0;
                if (testCoord >> testVal) {
                    iss.seekg(pos);
                } else {
                    iss.seekg(pos);
                    break;
                }
            }
        }
        if (cacDinh.size() < 3) {
            throw std::invalid_argument("Da giac phai co it nhat 3 dinh!");
        }
        hinh = std::make_unique<DaGiac>(cacDinh);
    }
    else {
        throw std::invalid_argument("Loai hinh khong duoc ho tro: " + loai);
    }

    std::string tag;
    while (iss >> tag) {
        tag = toLower(tag);
        if (tag == "vien" || tag == "stroke") {
            int r = 0, g = 0, b = 0, a = 255;
            if (iss >> r >> g >> b) {
                std::streampos p = iss.tellg();
                int testA = 255;
                if (iss >> testA) a = testA;
                else iss.seekg(p);
                hinh->setMauVien(MauSac(r, g, b, a));
            }
        } else if (tag == "nen" || tag == "fill") {
            int r = 255, g = 255, b = 255, a = 255;
            if (iss >> r >> g >> b) {
                std::streampos p = iss.tellg();
                int testA = 255;
                if (iss >> testA) a = testA;
                else iss.seekg(p);
                hinh->setMauNen(MauSac(r, g, b, a));
            }
        } else if (tag == "net" || tag == "width") {
            double w = 1.0;
            if (iss >> w) {
                hinh->setDoDayNet(w);
            }
        }
    }

    return hinh;
}
