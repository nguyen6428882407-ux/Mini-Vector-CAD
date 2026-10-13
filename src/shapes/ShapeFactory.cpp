#include "shapes/ShapeFactory.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/TamGiac.h"
#include "shapes/DaGiac.h"
#include "core/MathUtils.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cctype>

namespace {
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    }

    bool isTag(const std::string& s) {
        std::string t = toLower(s);
        return t == "vien" || t == "stroke" ||
               t == "nen"  || t == "fill"   ||
               t == "net"  || t == "width";
    }

    bool parseIntegerToken(const std::string& token, int& outVal) {
        if (token.empty()) return false;
        try {
            size_t idx = 0;
            outVal = std::stoi(token, &idx);
            return idx == token.size();
        } catch (...) {
            return false;
        }
    }

    bool parseDoubleToken(const std::string& token, double& outVal) {
        if (token.empty()) return false;
        try {
            size_t idx = 0;
            outVal = std::stod(token, &idx);
            return idx == token.size() && std::isfinite(outVal);
        } catch (...) {
            return false;
        }
    }
}

std::unique_ptr<HinhHoc2D> ShapeFactory::taoHinh(const std::string& moTa) {
    if (moTa.empty()) return nullptr;

    std::istringstream iss(moTa);
    std::string loai;
    if (!(iss >> loai)) return nullptr;

    loai = toLower(loai);
    std::unique_ptr<HinhHoc2D> hinh = nullptr;
    std::string pendingTag;

    if (loai == "tron" || loai == "circle") {
        std::string tokCx, tokCy, tokR;
        if (!(iss >> tokCx >> tokCy >> tokR)) {
            throw std::invalid_argument("Cu phap hinh tron sai: can <cx> <cy> <r>");
        }
        double cx = 0, cy = 0, r = 0;
        if (!parseDoubleToken(tokCx, cx) || !parseDoubleToken(tokCy, cy) || !parseDoubleToken(tokR, r)) {
            throw std::invalid_argument("Toa do va ban kinh hinh tron phai la so huu han hop le!");
        }
        if (r <= 0) {
            throw std::invalid_argument("Ban kinh hinh tron phai lon hon 0!");
        }
        hinh = std::make_unique<HinhTron>(Diem2D(cx, cy), r);
    }
    else if (loai == "doanthang" || loai == "line") {
        std::string tokX1, tokY1, tokX2, tokY2;
        if (!(iss >> tokX1 >> tokY1 >> tokX2 >> tokY2)) {
            throw std::invalid_argument("Cu phap doan thang sai: can <x1> <y1> <x2> <y2>");
        }
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!parseDoubleToken(tokX1, x1) || !parseDoubleToken(tokY1, y1) ||
            !parseDoubleToken(tokX2, x2) || !parseDoubleToken(tokY2, y2)) {
            throw std::invalid_argument("Toa do doan thang phai la so huu han hop le!");
        }
        if (MathUtils::bangNhau(x1, x2) && MathUtils::bangNhau(y1, y2)) {
            throw std::invalid_argument("Doan thang khong the co 2 dau mut trung nhau!");
        }
        hinh = std::make_unique<DoanThang>(Diem2D(x1, y1), Diem2D(x2, y2));
    }
    else if (loai == "chunhat" || loai == "rect" || loai == "rectangle") {
        std::string tokX, tokY, tokW, tokH;
        if (!(iss >> tokX >> tokY >> tokW >> tokH)) {
            throw std::invalid_argument("Cu phap chu nhat sai: can <x> <y> <w> <h>");
        }
        double x = 0, y = 0, w = 0, h = 0;
        if (!parseDoubleToken(tokX, x) || !parseDoubleToken(tokY, y) ||
            !parseDoubleToken(tokW, w) || !parseDoubleToken(tokH, h)) {
            throw std::invalid_argument("Toa do va kich thuoc hinh chu nhat phai la so huu han hop le!");
        }
        if (w <= 0 || h <= 0) {
            throw std::invalid_argument("Chieu rong va chieu cao hinh chu nhat phai lon hon 0!");
        }
        hinh = std::make_unique<HinhChuNhat>(Diem2D(x, y), w, h);
    }
    else if (loai == "tamgiac" || loai == "triangle") {
        std::string tokX1, tokY1, tokX2, tokY2, tokX3, tokY3;
        if (!(iss >> tokX1 >> tokY1 >> tokX2 >> tokY2 >> tokX3 >> tokY3)) {
            throw std::invalid_argument("Cu phap tam giac sai: can <x1> <y1> <x2> <y2> <x3> <y3>");
        }
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
        if (!parseDoubleToken(tokX1, x1) || !parseDoubleToken(tokY1, y1) ||
            !parseDoubleToken(tokX2, x2) || !parseDoubleToken(tokY2, y2) ||
            !parseDoubleToken(tokX3, x3) || !parseDoubleToken(tokY3, y3)) {
            throw std::invalid_argument("Toa do cac dinh tam giac phai la so huu han hop le!");
        }
        // Dịch tọa độ về đỉnh thứ nhất để triệt tiêu sai số làm tròn khi tọa độ lớn
        double u1 = x2 - x1, u2 = y2 - y1;
        double v1 = x3 - x1, v2 = y3 - y1;
        double w1 = x3 - x2, w2 = y3 - y2;

        if (!std::isfinite(u1) || !std::isfinite(u2) ||
            !std::isfinite(v1) || !std::isfinite(v2) ||
            !std::isfinite(w1) || !std::isfinite(w2)) {
            throw std::invalid_argument("Hieu toa do cac dinh tam giac gay tran so!");
        }

        double d12 = std::hypot(u1, u2);
        double d13 = std::hypot(v1, v2);
        double d23 = std::hypot(w1, w2);

        if (!std::isfinite(d12) || !std::isfinite(d13) || !std::isfinite(d23)) {
            throw std::invalid_argument("Toa do tam giac gay tran so trong phep tinh do dai canh!");
        }

        // Kiểm tra các cặp đỉnh trùng nhau
        if (MathUtils::bangNhau(d12, 0.0) || MathUtils::bangNhau(d13, 0.0) || MathUtils::bangNhau(d23, 0.0)) {
            throw std::invalid_argument("Tam giac khong the co cac dinh trung nhau!");
        }

        // Kiểm tra thẳng hàng bằng tích có hướng của hai vector đơn vị:
        // sin(theta) = |(u1/d12)*(v2/d13) - (u2/d12)*(v1/d13)|
        // Biểu thức chuẩn hóa vector đơn vị hạn chế nguy cơ tràn số trung gian (như d12 * d13).
        double sinTheta = std::abs((u1 / d12) * (v2 / d13) - (u2 / d12) * (v1 / d13));
        if (!std::isfinite(sinTheta)) {
            throw std::invalid_argument("Phep tinh goc tam giac gay tran so hoac khong hop le!");
        }
        if (sinTheta < MathUtils::EPS) {
            throw std::invalid_argument("3 diem cua tam giac khong duoc thang hang!");
        }
        hinh = std::make_unique<TamGiac>(Diem2D(x1, y1), Diem2D(x2, y2), Diem2D(x3, y3));
    }
    else if (loai == "dagiac" || loai == "polygon") {
        std::vector<Diem2D> cacDinh;
        std::string token;

        while (iss >> token) {
            if (isTag(token)) {
                pendingTag = toLower(token);
                break;
            }

            // Đọc tọa độ x
            double x = 0.0, y = 0.0;
            if (!parseDoubleToken(token, x)) {
                throw std::invalid_argument("Toa do x cua dinh da giac khong hop le: " + token);
            }

            // Đọc tọa độ y tiếp theo
            std::string tokenY;
            if (!(iss >> tokenY)) {
                throw std::invalid_argument("Thieu toa do y cho dinh da giac (so luong toa do bi le)!");
            }
            if (!parseDoubleToken(tokenY, y)) {
                throw std::invalid_argument("Toa do y cua dinh da giac khong hop le: " + tokenY);
            }

            cacDinh.emplace_back(x, y);
        }

        if (cacDinh.size() < 3) {
            throw std::invalid_argument("Da giac phai co it nhat 3 dinh!");
        }
        hinh = std::make_unique<DaGiac>(cacDinh);
    }
    else {
        return nullptr;
    }

    std::string tag;
    while (!pendingTag.empty() || (iss >> tag)) {
        if (!pendingTag.empty()) {
            tag = pendingTag;
            pendingTag.clear();
        }
        tag = toLower(tag);
        if (tag == "vien" || tag == "stroke") {
            std::string tokR, tokG, tokB;
            if (!(iss >> tokR >> tokG >> tokB)) {
                throw std::invalid_argument("Cu phap mau vien sai: can it nhat 3 gia tri <r> <g> <b>!");
            }
            int r = 0, g = 0, b = 0;
            if (!parseIntegerToken(tokR, r) || !parseIntegerToken(tokG, g) || !parseIntegerToken(tokB, b)) {
                throw std::invalid_argument("Cu phap mau vien sai: cac gia tri <r> <g> <b> phai la so nguyen hop le!");
            }
            int a = 255;
            std::string nextTok;
            if (iss >> nextTok) {
                if (isTag(nextTok)) {
                    pendingTag = nextTok;
                } else {
                    if (!parseIntegerToken(nextTok, a)) {
                        throw std::invalid_argument("Gia tri alpha mau vien khong hop le: " + nextTok);
                    }
                }
            }
            hinh->setMauVien(MauSac(r, g, b, a));
        } else if (tag == "nen" || tag == "fill") {
            std::string tokR, tokG, tokB;
            if (!(iss >> tokR >> tokG >> tokB)) {
                throw std::invalid_argument("Cu phap mau nen sai: can it nhat 3 gia tri <r> <g> <b>!");
            }
            int r = 255, g = 255, b = 255;
            if (!parseIntegerToken(tokR, r) || !parseIntegerToken(tokG, g) || !parseIntegerToken(tokB, b)) {
                throw std::invalid_argument("Cu phap mau nen sai: cac gia tri <r> <g> <b> phai la so nguyen hop le!");
            }
            int a = 255;
            std::string nextTok;
            if (iss >> nextTok) {
                if (isTag(nextTok)) {
                    pendingTag = nextTok;
                } else {
                    if (!parseIntegerToken(nextTok, a)) {
                        throw std::invalid_argument("Gia tri alpha mau nen khong hop le: " + nextTok);
                    }
                }
            }
            hinh->setMauNen(MauSac(r, g, b, a));
        } else if (tag == "net" || tag == "width") {
            std::string tokW;
            if (!(iss >> tokW)) {
                throw std::invalid_argument("Cu phap do day net sai: thieu gia tri do day!");
            }
            double w = 1.0;
            if (!parseDoubleToken(tokW, w) || w <= 0) {
                throw std::invalid_argument("Cu phap do day net sai: can gia tri so huu han > 0!");
            }
            hinh->setDoDayNet(w);
        } else {
            throw std::invalid_argument("Thuoc tinh khong hop le hoac du thua: " + tag);
        }
    }

    return hinh;
}
