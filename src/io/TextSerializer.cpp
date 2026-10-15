#include "io/TextSerializer.h"
#include "canvas/BangVeCanvas.h"
#include "canvas/Layer.h"
#include "shapes/ShapeFactory.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/TamGiac.h"
#include "shapes/DaGiac.h"
#include "shapes/GroupShape.h"
#include "shapes/NetVeTuDo.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <locale>

namespace {
    constexpr size_t MAX_LAYERS = 1000;
    constexpr size_t MAX_SHAPES_PER_LAYER = 100000;
    constexpr size_t MAX_GROUP_CHILDREN = 10000;
    constexpr int    MAX_GROUP_DEPTH = 32;
    // Gioi han tong so doi tuong hinh hoc cap phat tren Heap trong mot file
    // (tinh ca doi tuong GroupShape chua va tat ca cac hinh con o moi cap long nhau)
    constexpr size_t MAX_TOTAL_SHAPES = 200000;

    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    }

    bool isGroupTag(const std::string& s) {
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

    bool parseLongLongToken(const std::string& token, long long& outVal) {
        if (token.empty()) return false;
        try {
            size_t idx = 0;
            outVal = std::stoll(token, &idx);
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

    std::string escapeLayerName(const std::string& name) {
        std::string res;
        res.reserve(name.size() + 8);
        for (char c : name) {
            switch (c) {
                case '\\': res += "\\\\"; break;
                case '\"': res += "\\\""; break;
                case '\n': res += "\\n";  break;
                case '\t': res += "\\t";  break;
                case '\r': res += "\\r";  break;
                default:   res += c;      break;
            }
        }
        return res;
    }

    bool parseQuotedLayerName(const std::string& line, size_t& afterQuotePos, std::string& outName) {
        size_t q1 = line.find('\"');
        if (q1 == std::string::npos) return false;

        // Kiem tra tien to truoc dau ngoac kep: bat buoc la token "LAYER" / "layer", khong co token rac
        std::string prefix = line.substr(0, q1);
        std::istringstream pss(prefix);
        std::string tagLayer, pExtra;
        if (!(pss >> tagLayer) || toLower(tagLayer) != "layer" || (pss >> pExtra)) {
            return false;
        }

        outName.clear();
        bool escaped = false;
        bool closed = false;
        afterQuotePos = 0;
        for (size_t i = q1 + 1; i < line.size(); ++i) {
            char c = line[i];
            if (escaped) {
                switch (c) {
                    case '\"': outName += '\"'; break;
                    case '\\': outName += '\\'; break;
                    case 'n':  outName += '\n'; break;
                    case 't':  outName += '\t'; break;
                    case 'r':  outName += '\r'; break;
                    default:
                        // Ky tu escape khong duoc ho tro -> Tu choi ro rang
                        return false;
                }
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '\"') {
                afterQuotePos = i + 1;
                closed = true;
                break;
            } else if (c == '\n' || c == '\r') {
                // Ky tu xuong dong tho khong duoc phep trong ten layer
                return false;
            } else {
                outName += c;
            }
        }

        if (escaped || !closed) {
            return false;
        }
        return true;
    }

    void serializeHinh(const HinhHoc2D& hinh, std::ostream& os) {
        if (auto group = dynamic_cast<const GroupShape*>(&hinh)) {
            os << "group " << group->soLuongCon();
            const auto& mv = group->getMauVien();
            const auto& mn = group->getMauNen();
            os << " vien " << mv.getR() << " " << mv.getG() << " " << mv.getB() << " " << mv.getA()
               << " nen " << mn.getR() << " " << mn.getG() << " " << mn.getB() << " " << mn.getA()
               << " net " << group->getDoDayNet() << "\n";
            for (size_t c = 0; c < group->soLuongCon(); ++c) {
                serializeHinh(group->hinhThu(c), os);
            }
            return;
        } else if (auto tron = dynamic_cast<const HinhTron*>(&hinh)) {
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
        } else if (auto brush = dynamic_cast<const NetVeTuDo*>(&hinh)) {
            os << "brush " << brush->getDiem().size();
            for (const auto& pt : brush->getDiem()) {
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

    std::unique_ptr<HinhHoc2D> deserializeHinh(std::istream& is, int depth, size_t& totalShapeCount) {
        if (depth > MAX_GROUP_DEPTH) {
            return nullptr;
        }

        std::string line;
        while (std::getline(is, line)) {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            line = line.substr(first);
            if (line[0] == '#') continue;

            std::istringstream iss(line);
            std::string token;
            if (!(iss >> token)) continue;

            std::string lowerToken = toLower(token);
            if (lowerToken == "group" || lowerToken == "nhom") {
                if (depth >= MAX_GROUP_DEPTH) {
                    return nullptr;
                }
                if (totalShapeCount >= MAX_TOTAL_SHAPES) {
                    return nullptr;
                }
                ++totalShapeCount;

                std::string tokK;
                if (!(iss >> tokK)) {
                    return nullptr;
                }
                long long k = 0;
                if (!parseLongLongToken(tokK, k) || k < 0 || static_cast<size_t>(k) > MAX_GROUP_CHILDREN) {
                    return nullptr;
                }

                auto group = std::make_unique<GroupShape>();

                bool hasVien = false;
                bool hasNen = false;
                bool hasNet = false;

                std::string tag;
                std::string pendingTag;
                while (!pendingTag.empty() || (iss >> tag)) {
                    if (!pendingTag.empty()) {
                        tag = pendingTag;
                        pendingTag.clear();
                    }
                    std::string lowerTag = toLower(tag);
                    if (lowerTag == "vien" || lowerTag == "stroke") {
                        if (hasVien) {
                            // Trung lap tag vien / stroke -> Tu choi
                            return nullptr;
                        }
                        hasVien = true;

                        std::string tokR, tokG, tokB;
                        if (!(iss >> tokR >> tokG >> tokB)) {
                            return nullptr;
                        }
                        int r = 0, g = 0, b = 0;
                        if (!parseIntegerToken(tokR, r) || !parseIntegerToken(tokG, g) || !parseIntegerToken(tokB, b)) {
                            return nullptr;
                        }
                        int a = 255;
                        std::string nextTok;
                        if (iss >> nextTok) {
                            if (isGroupTag(nextTok)) {
                                pendingTag = nextTok;
                            } else {
                                if (!parseIntegerToken(nextTok, a)) {
                                    return nullptr;
                                }
                            }
                        }
                        group->setMauVien(MauSac(r, g, b, a));
                    } else if (lowerTag == "nen" || lowerTag == "fill") {
                        if (hasNen) {
                            // Trung lap tag nen / fill -> Tu choi
                            return nullptr;
                        }
                        hasNen = true;

                        std::string tokR, tokG, tokB;
                        if (!(iss >> tokR >> tokG >> tokB)) {
                            return nullptr;
                        }
                        int r = 255, g = 255, b = 255;
                        if (!parseIntegerToken(tokR, r) || !parseIntegerToken(tokG, g) || !parseIntegerToken(tokB, b)) {
                            return nullptr;
                        }
                        int a = 255;
                        std::string nextTok;
                        if (iss >> nextTok) {
                            if (isGroupTag(nextTok)) {
                                pendingTag = nextTok;
                            } else {
                                if (!parseIntegerToken(nextTok, a)) {
                                    return nullptr;
                                }
                            }
                        }
                        group->setMauNen(MauSac(r, g, b, a));
                    } else if (lowerTag == "net" || lowerTag == "width") {
                        if (hasNet) {
                            // Trung lap tag net / width -> Tu choi
                            return nullptr;
                        }
                        hasNet = true;

                        std::string tokW;
                        if (!(iss >> tokW)) {
                            return nullptr;
                        }
                        double w = 1.0;
                        if (!parseDoubleToken(tokW, w) || w <= 0.0) {
                            return nullptr;
                        }
                        group->setDoDayNet(w);
                    } else {
                        // Tag khong ho tro hoac token rac tren dong group
                        return nullptr;
                    }
                }

                for (long long c = 0; c < k; ++c) {
                    auto con = deserializeHinh(is, depth + 1, totalShapeCount);
                    if (!con) {
                        return nullptr;
                    }
                    group->themHinh(std::move(con));
                }
                return group;
            } else {
                if (totalShapeCount >= MAX_TOTAL_SHAPES) {
                    return nullptr;
                }
                ++totalShapeCount;

                try {
                    auto hinh = ShapeFactory::taoHinh(line);
                    return hinh;
                } catch (...) {
                    return nullptr;
                }
            }
        }
        return nullptr;
    }
} // namespace

bool TextSerializer::ghiFile(const BangVeCanvas& canvas, const std::string& duongDan) {
    if (duongDan.empty()) return false;
    try {
        std::ofstream ofs(duongDan);
        if (!ofs.is_open() || ofs.bad()) return false;
        serialize(canvas, ofs);
        if (!ofs.good() || ofs.bad()) {
            ofs.close();
            return false;
        }
        ofs.close();
        if (ofs.fail() || ofs.bad()) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool TextSerializer::docFile(BangVeCanvas& canvas, const std::string& duongDan) {
    if (duongDan.empty()) return false;
    try {
        std::ifstream ifs(duongDan);
        if (!ifs.is_open() || ifs.bad() || ifs.fail()) return false;
        deserialize(canvas, ifs);
        if (ifs.bad() || ifs.fail()) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

void TextSerializer::serialize(const BangVeCanvas& canvas, std::ostream& os) {
    os.imbue(std::locale::classic());
    os << "MINI_VECTOR_CAD_V1\n";
    os << "LAYERS " << canvas.soLayer() << "\n";

    for (size_t i = 0; i < canvas.soLayer(); ++i) {
        const auto& lay = canvas.layer(i);
        os << "LAYER \"" << escapeLayerName(lay.getTen()) << "\" "
           << (lay.isHien() ? 1 : 0) << " "
           << (lay.isKhoa() ? 1 : 0) << " "
           << lay.soHinh() << "\n";

        for (size_t j = 0; j < lay.soHinh(); ++j) {
            const auto& hinh = lay.hinhThu(j);
            serializeHinh(hinh, os);
        }
    }
}

void TextSerializer::deserialize(BangVeCanvas& canvas, std::istream& is) {
    is.imbue(std::locale::classic());

    if (is.bad()) {
        is.setstate(std::ios::failbit);
        return;
    }

    std::string line;
    // Buoc 1: Doc dong header (bo qua dong trong hoac comment phia truoc neu co)
    bool gotHeader = false;
    while (std::getline(is, line)) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (line[first] == '#') continue;
        line = line.substr(first);
        gotHeader = true;
        break;
    }
    if (!gotHeader || is.bad()) {
        is.setstate(std::ios::failbit);
        return;
    }

    std::istringstream hss(line);
    std::string header, hExtra;
    if (!(hss >> header) || header != "MINI_VECTOR_CAD_V1" || (hss >> hExtra)) {
        is.setstate(std::ios::failbit);
        return;
    }

    // Buoc 2: Doc dong khai bao LAYERS
    bool gotLayersLine = false;
    while (std::getline(is, line)) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (line[first] == '#') continue;
        line = line.substr(first);
        gotLayersLine = true;
        break;
    }
    if (!gotLayersLine || is.bad()) {
        is.setstate(std::ios::failbit);
        return;
    }

    std::istringstream lss(line);
    std::string tagLayers, tokLayers, lExtra;
    if (!(lss >> tagLayers >> tokLayers) || tagLayers != "LAYERS") {
        is.setstate(std::ios::failbit);
        return;
    }
    long long nLayers = 0;
    if (!parseLongLongToken(tokLayers, nLayers) || nLayers < 0 || static_cast<size_t>(nLayers) > MAX_LAYERS) {
        is.setstate(std::ios::failbit);
        return;
    }
    if (lss >> lExtra) {
        is.setstate(std::ios::failbit);
        return;
    }
    size_t soLuongLayer = static_cast<size_t>(nLayers);

    struct LayerData {
        std::string ten;
        bool hien;
        bool khoa;
        std::vector<std::unique_ptr<HinhHoc2D>> danhSachHinh;
    };

    std::vector<LayerData> parsedLayers;
    parsedLayers.reserve(soLuongLayer);

    size_t totalShapeCount = 0;

    // Buoc 3: Phan tich tung Layer va cac hinh con vao Staging Buffer
    for (size_t i = 0; i < soLuongLayer; ++i) {
        bool gotLayerLine = false;
        while (std::getline(is, line)) {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            if (line[first] == '#') continue;
            line = line.substr(first);
            gotLayerLine = true;
            break;
        }
        if (!gotLayerLine || is.bad()) {
            is.setstate(std::ios::failbit);
            return;
        }

        size_t afterQuotePos = 0;
        std::string tenLayer;
        if (!parseQuotedLayerName(line, afterQuotePos, tenLayer)) {
            is.setstate(std::ios::failbit);
            return;
        }

        std::string suffix = line.substr(afterQuotePos);
        std::istringstream sss(suffix);
        std::string tokHien, tokKhoa, tokSoHinh, sExtra;
        if (!(sss >> tokHien >> tokKhoa >> tokSoHinh)) {
            is.setstate(std::ios::failbit);
            return;
        }
        if (tokHien != "0" && tokHien != "1") {
            is.setstate(std::ios::failbit);
            return;
        }
        if (tokKhoa != "0" && tokKhoa != "1") {
            is.setstate(std::ios::failbit);
            return;
        }
        long long soHinh = 0;
        if (!parseLongLongToken(tokSoHinh, soHinh) || soHinh < 0 || static_cast<size_t>(soHinh) > MAX_SHAPES_PER_LAYER) {
            is.setstate(std::ios::failbit);
            return;
        }
        if (sss >> sExtra) {
            is.setstate(std::ios::failbit);
            return;
        }

        LayerData ld;
        ld.ten = tenLayer;
        ld.hien = (tokHien == "1");
        ld.khoa = (tokKhoa == "1");
        ld.danhSachHinh.reserve(static_cast<size_t>(soHinh));

        for (long long j = 0; j < soHinh; ++j) {
            auto hinh = deserializeHinh(is, 0, totalShapeCount);
            if (!hinh || is.bad()) {
                is.setstate(std::ios::failbit);
                return;
            }
            ld.danhSachHinh.push_back(std::move(hinh));
        }

        parsedLayers.push_back(std::move(ld));
    }

    // Buoc 4: Kiem tra du lieu thua phia sau file va kiem tra loi I/O
    std::string trailingLine;
    bool trailingGarbage = false;
    while (std::getline(is, trailingLine)) {
        size_t first = trailingLine.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (trailingLine[first] == '#') continue;
        trailingGarbage = true;
        break;
    }

    // 1. Neu phat hien du lieu thua khong hop le:
    if (trailingGarbage) {
        is.setstate(std::ios::failbit);
        return;
    }

    // 2. Kiem tra nguyen nhan vong lap ket thuc:
    // Vong lap doc chi duoc coi la ket thuc thanh cong khi THUC SU cham den EOF (is.eof())
    // va hoan toan khong gap bat ky loi I/O nao (!is.bad()).
    // Neu vong doc dung lai ma chua cham EOF (vi du loi luong bat failbit ma khong phai EOF)
    // hoac bi badbit -> Day la loi doc stream, TUYET DOI khong ap dung staging.
    if (!is.eof() || is.bad()) {
        is.setstate(std::ios::failbit);
        return;
    }

    // 3. Xac dinh ro EOF hop le: khong co rac, khong bi loi doc, va thuc su cham EOF.
    // Chuan C++ khi getline cham EOF se bat ca eofbit va failbit (vi khong doc duoc ky tu nao).
    // Ta xoa failbit do getline tao ra va giu nguyen eofbit de docFile() nhan dien doc thanh cong.
    is.clear(std::ios::eofbit);

    // Buoc 5: Toan bo du lieu file da duoc kiem tra nghiem ngat thanh cong!
    // Ap dung an toan vao Canvas:
    if (canvas.soLayer() == 0) {
        for (auto& ld : parsedLayers) {
            size_t idx = canvas.themLayer(ld.ten);
            canvas.layer(idx).setHien(ld.hien);
            canvas.layer(idx).setKhoa(ld.khoa);
            for (auto& h : ld.danhSachHinh) {
                canvas.layer(idx).themHinh(std::move(h));
            }
        }
    } else {
        // Xoa sach hinh trong cac layer hien co voi kiem tra tien trien chat che
        bool xoaThanhCong = true;
        for (size_t i = 0; i < canvas.soLayer(); ++i) {
            Layer& existingLay = canvas.layer(i);
            size_t attempts = existingLay.soHinh();
            for (size_t a = 0; a < attempts; ++a) {
                if (existingLay.soHinh() == 0) break;
                int id = existingLay.hinhThu(0).getId();
                size_t truocKhiXoa = existingLay.soHinh();
                auto removed = existingLay.layHinh(id);
                if (!removed || existingLay.soHinh() >= truocKhiXoa) {
                    xoaThanhCong = false;
                    break;
                }
            }
            if (!xoaThanhCong || existingLay.soHinh() > 0) {
                xoaThanhCong = false;
                break;
            }
        }

        if (!xoaThanhCong) {
            // LƯU Ý KIẾN TRÚC: Do BangVeCanvas va Layer khong ho tro clone/snapshot/transaction,
            // neu viec xoa hinh that bai sau khi mot so Layer da bi xoa, Canvas co the da bi
            // thay doi mot phan. Thao tac KHONG bao dam tinh nguyen tu (non-atomic).
            // Dung ngay lap tuc va dat failbit de bao loi cho nguoi goi.
            is.setstate(std::ios::failbit);
            return;
        }

        // Tai su dung hoac bo sung layer moi
        for (size_t i = 0; i < parsedLayers.size(); ++i) {
            auto& ld = parsedLayers[i];
            if (i < canvas.soLayer()) {
                Layer& existingLay = canvas.layer(i);
                existingLay.setTen(ld.ten);
                existingLay.setHien(ld.hien);
                existingLay.setKhoa(ld.khoa);
                for (auto& h : ld.danhSachHinh) {
                    existingLay.themHinh(std::move(h));
                }
            } else {
                size_t idx = canvas.themLayer(ld.ten);
                canvas.layer(idx).setHien(ld.hien);
                canvas.layer(idx).setKhoa(ld.khoa);
                for (auto& h : ld.danhSachHinh) {
                    canvas.layer(idx).themHinh(std::move(h));
                }
            }
        }

        // Neu canvas co nhieu layer hon file nap, an cac layer du thua rong
        for (size_t i = parsedLayers.size(); i < canvas.soLayer(); ++i) {
            canvas.layer(i).setHien(false);
            canvas.layer(i).setKhoa(false);
        }
    }

    if (canvas.soLayer() > 0) {
        canvas.setLayerHienTai(0);
    }
}

void TextSerializer::inBangVe(const BangVeCanvas& canvas, std::ostream& os) {
    os << "==================== BANG VE CANVAS ====================\n";
    os << "Tong so Layer: " << canvas.soLayer() << "\n";
    for (size_t i = 0; i < canvas.soLayer(); ++i) {
        const auto& lay = canvas.layer(i);
        os << "--------------------------------------------------------\n";
        os << "[Layer " << i << "] \"" << lay.getTen() << "\""
           << " | Hien thi: " << (lay.isHien() ? "BAT" : "TAT")
           << " | Khoa: " << (lay.isKhoa() ? "KHOA" : "MO")
           << " | So hinh: " << lay.soHinh() << "\n";

        if (lay.soHinh() == 0) {
            os << "  (Trong)\n";
        } else {
            for (size_t j = 0; j < lay.soHinh(); ++j) {
                const auto& hinh = lay.hinhThu(j);
                os << "  * ";
                hinh.ve(os);
            }
        }
    }
    os << "========================================================\n";
}

