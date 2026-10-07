#include "controller/MenuController.h"
#include "canvas/BangVeCanvas.h"
#include "commands/CommandManager.h"
#include "commands/AddShapeCommand.h"
#include "commands/RemoveShapeCommand.h"
#include "commands/TransformCommand.h"
#include "commands/GroupCommand.h"
#include "shapes/ShapeFactory.h"
#include "shapes/GroupShape.h"
#include "io/SvgExporter.h"
#include "io/TextSerializer.h"
#include "core/MathUtils.h"
#include <iostream>
#include <sstream>
#include <limits>

MenuController::MenuController(BangVeCanvas& canvas, CommandManager& cmdMgr)
    : canvas_(canvas), cmdMgr_(cmdMgr), layerHienTai_(0) {}

void MenuController::hienThiMenuChinh() const {
    std::cout << "\n======================================================\n";
    std::cout << "          CHUONG TRINH MINI VECTOR CAD (C++ OOP)      \n";
    std::cout << "======================================================\n";
    std::string tenLayer = (layerHienTai_ < canvas_.soLuongLayer())
                           ? canvas_.layer(layerHienTai_).getTen() : "Khong hop le";
    bool khoa = (layerHienTai_ < canvas_.soLuongLayer()) && canvas_.layer(layerHienTai_).isKhoa();
    bool hien = (layerHienTai_ < canvas_.soLuongLayer()) && canvas_.layer(layerHienTai_).isHienThi();

    std::cout << "Layer hien tai: [" << layerHienTai_ << "] \"" << tenLayer << "\""
              << " | " << (hien ? "Hien" : "An")
              << " | " << (khoa ? "KHOA" : "MO") << "\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << " 1. Quan ly Layer (Danh sach / Them / An-Hien / Khoa)\n";
    std::cout << " 2. Them hinh moi vao Layer hien tai\n";
    std::cout << " 3. Xoa hinh theo ID\n";
    std::cout << " 4. Bien doi hinh (Tinh tien / Xoay / Thu phong)\n";
    std::cout << " 5. Gom nhom cac hinh (Group)\n";
    std::cout << " 6. Hoan tac (Undo) [" << (cmdMgr_.coTheUndo() ? "Kha dung" : "Het") << "]\n";
    std::cout << " 7. Lam lai (Redo)  [" << (cmdMgr_.coTheRedo() ? "Kha dung" : "Het") << "]\n";
    std::cout << " 8. Xem toan bo bang ve (Text View)\n";
    std::cout << " 9. Xuat ra file SVG (.svg)\n";
    std::cout << "10. Xuat ra file trang thai (.txt)\n";
    std::cout << "11. Nap file trang thai (.txt)\n";
    std::cout << " 0. Thoat\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "Lua chon cua ban: ";
}

bool MenuController::kiemTraLayerHopLe(size_t idx) const {
    return idx < canvas_.soLuongLayer();
}

bool MenuController::kiemTraLayerCoTheSua(size_t idx) const {
    if (!kiemTraLayerHopLe(idx)) {
        std::cout << "[Loi] Chi so Layer khong ton tai!\n";
        return false;
    }
    if (canvas_.layer(idx).isKhoa()) {
        std::cout << "[Loi] Layer \"" << canvas_.layer(idx).getTen() << "\" dang bi KHOA! Khong the chinh sua.\n";
        return false;
    }
    return true;
}

bool MenuController::kiemTraIdTonTai(int id) const {
    return canvas_.timHinhTheoId(id) != nullptr;
}

void MenuController::xuLyQuanLyLayer() {
    while (true) {
        std::cout << "\n--- QUAN LY LAYER ---\n";
        for (size_t i = 0; i < canvas_.soLuongLayer(); ++i) {
            const auto& lay = canvas_.layer(i);
            std::cout << " [" << i << "] \"" << lay.getTen() << "\""
                      << (i == layerHienTai_ ? " <-- (Dang chon)" : "")
                      << " | Hien thi: " << (lay.isHienThi() ? "Co" : "Khong")
                      << " | Khoa: " << (lay.isKhoa() ? "KHOA" : "Mo")
                      << " | So hinh: " << lay.soLuongHinh() << "\n";
        }
        std::cout << "1. Chon layer lam viec\n";
        std::cout << "2. Them layer moi\n";
        std::cout << "3. An / Hien layer\n";
        std::cout << "4. Khoa / Mo khoa layer\n";
        std::cout << "0. Quay lai menu chinh\n";
        std::cout << "Chon: ";

        int ch = 0;
        if (!(std::cin >> ch)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (ch == 0) break;
        if (ch == 1) {
            std::cout << "Nhap chi so layer muon chon: ";
            size_t idx;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                layerHienTai_ = idx;
                std::cout << "[Thanh cong] Da chuyen sang layer [" << idx << "].\n";
            } else {
                std::cout << "[Loi] Chi so layer khong hop le!\n";
            }
        } else if (ch == 2) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nhap ten layer moi: ";
            std::string ten;
            std::getline(std::cin, ten);
            if (ten.empty()) ten = "Layer " + std::to_string(canvas_.soLuongLayer() + 1);
            size_t newIdx = canvas_.themLayer(ten);
            layerHienTai_ = newIdx;
            std::cout << "[Thanh cong] Da tao layer moi: \"" << ten << "\".\n";
        } else if (ch == 3) {
            std::cout << "Nhap chi so layer can An/Hien: ";
            size_t idx;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                bool ht = canvas_.layer(idx).isHienThi();
                canvas_.layer(idx).setHienThi(!ht);
                std::cout << "[Thanh cong] Layer [" << idx << "] gio dang: " << (!ht ? "Hien thi" : "Bi an") << ".\n";
            } else {
                std::cout << "[Loi] Chi so layer khong hop le!\n";
            }
        } else if (ch == 4) {
            std::cout << "Nhap chi so layer can Khoa/Mo khoa: ";
            size_t idx;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                bool kh = canvas_.layer(idx).isKhoa();
                canvas_.layer(idx).setKhoa(!kh);
                std::cout << "[Thanh cong] Layer [" << idx << "] gio dang: " << (!kh ? "KHOA" : "MO") << ".\n";
            } else {
                std::cout << "[Loi] Chi so layer khong hop le!\n";
            }
        }
    }
}

void MenuController::xuLyThemHinh() {
    if (!kiemTraLayerCoTheSua(layerHienTai_)) return;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n--- THEM HINH MOI (ShapeFactory) ---\n";
    std::cout << "Cu phap ho tro:\n";
    std::cout << " - tron <cx> <cy> <r>\n";
    std::cout << " - chunhat <x> <y> <w> <h>\n";
    std::cout << " - doanthang <x1> <y1> <x2> <y2>\n";
    std::cout << " - tamgiac <x1> <y1> <x2> <y2> <x3> <y3>\n";
    std::cout << " - dagiac <x1> <y1> <x2> <y2> <x3> <y3> ...\n";
    std::cout << " (Co the them: vien <r> <g> <b>, nen <r> <g> <b>, net <w>)\n";
    std::cout << "Nhap lenh tao hinh: ";

    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return;

    try {
        auto hinh = ShapeFactory::taoHinh(line);
        if (hinh) {
            int id = hinh->getId();
            auto cmd = std::make_unique<AddShapeCommand>(canvas_, layerHienTai_, std::move(hinh));
            cmdMgr_.thucThi(std::move(cmd));
            std::cout << "[Thanh cong] Da them hinh ID #" << id << " vao layer [" << layerHienTai_ << "].\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyXoaHinh() {
    std::cout << "\nNhap ID hinh muon xoa: ";
    int id = 0;
    if (!(std::cin >> id)) {
        std::cout << "[Loi] ID phai la so nguyen!\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    int layIdx = canvas_.timLayerChuaHinh(id);
    if (layIdx < 0) {
        std::cout << "[Loi] Khong tim thay hinh co ID #" << id << " tren bang ve!\n";
        return;
    }

    if (!kiemTraLayerCoTheSua(static_cast<size_t>(layIdx))) return;

    try {
        auto cmd = std::make_unique<RemoveShapeCommand>(canvas_, static_cast<size_t>(layIdx), id);
        cmdMgr_.thucThi(std::move(cmd));
        std::cout << "[Thanh cong] Da xoa hinh ID #" << id << " khoi layer [" << layIdx << "].\n";
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyBienDoiHinh() {
    std::cout << "\nNhap ID hinh muon bien doi: ";
    int id = 0;
    if (!(std::cin >> id)) {
        std::cout << "[Loi] ID phai la so nguyen!\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    int layIdx = canvas_.timLayerChuaHinh(id);
    if (layIdx < 0) {
        std::cout << "[Loi] Khong tim thay hinh co ID #" << id << "!\n";
        return;
    }

    if (!kiemTraLayerCoTheSua(static_cast<size_t>(layIdx))) return;

    HinhHoc2D* hinh = canvas_.timHinhTheoId(id);
    Diem2D tamHinh = hinh->tam();

    std::cout << "Hinh #" << id << " co tam bounding box: " << tamHinh << "\n";
    std::cout << "Chon phep bien doi:\n";
    std::cout << " 1. Tinh tien (dx, dy)\n";
    std::cout << " 2. Xoay (gocDo quanh tam hinh)\n";
    std::cout << " 3. Thu phong / Scale deu (ti le k quanh tam hinh)\n";
    std::cout << "Chon: ";

    int opt = 0;
    std::cin >> opt;

    BienDoi2D bd;
    if (opt == 1) {
        double dx = 0, dy = 0;
        std::cout << "Nhap dx dy: ";
        std::cin >> dx >> dy;
        bd = BienDoi2D::tinhTien(dx, dy);
    } else if (opt == 2) {
        double goc = 0;
        std::cout << "Nhap goc xoay (do): ";
        std::cin >> goc;
        bd = BienDoi2D::xoay(goc, tamHinh);
    } else if (opt == 3) {
        double k = 1.0;
        std::cout << "Nhap he so scale k (k != 0): ";
        std::cin >> k;

        if (MathUtils::bangNhau(k, 0.0) || std::abs(k) < MathUtils::EPS) {
            std::cout << "[Loi] He so thu phong k khong duoc bang 0!\n";
            return;
        }
        bd = BienDoi2D::tiLeDeu(k, tamHinh);
    } else {
        std::cout << "[Loi] Lua chon khong hop le!\n";
        return;
    }

    try {
        auto cmd = std::make_unique<TransformCommand>(canvas_, static_cast<size_t>(layIdx), id, bd);
        cmdMgr_.thucThi(std::move(cmd));
        std::cout << "[Thanh cong] Da bien doi hinh ID #" << id << ".\n";
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyGomNhom() {
    if (!kiemTraLayerCoTheSua(layerHienTai_)) return;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\nNhap danh sach cac ID can gom nhom (cach nhau boi dau cach): ";
    std::string line;
    std::getline(std::cin, line);

    std::istringstream iss(line);
    std::vector<int> ids;
    int id = 0;
    while (iss >> id) ids.push_back(id);

    if (ids.size() < 2) {
        std::cout << "[Loi] Can it nhat 2 hinh de gom nhom!\n";
        return;
    }

    const auto& lay = canvas_.layer(layerHienTai_);
    for (int tid : ids) {
        if (lay.timTheoId(tid) == nullptr) {
            std::cout << "[Loi] Hinh ID #" << tid << " khong ton tai trong layer [" << layerHienTai_ << "]!\n";
            return;
        }
    }

    try {
        auto cmd = std::make_unique<GroupCommand>(canvas_, layerHienTai_, ids);
        cmdMgr_.thucThi(std::move(cmd));
        std::cout << "[Thanh cong] Da gom " << ids.size() << " hinh thanh mot GroupShape moi.\n";
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyUndo() {
    if (cmdMgr_.undo()) {
        std::cout << "[Thanh cong] Da Hoan tac (Undo) lenh truoc do.\n";
    } else {
        std::cout << "[Thong bao] Khong con lenh nao de Hoan tac!\n";
    }
}

void MenuController::xuLyRedo() {
    if (cmdMgr_.redo()) {
        std::cout << "[Thanh cong] Da Lam lai (Redo) lenh.\n";
    } else {
        std::cout << "[Thong bao] Khong con lenh nao de Lam lai!\n";
    }
}

void MenuController::xuLyXemBangVe() const {
    std::cout << "\n";
    TextSerializer::inBangVe(canvas_, std::cout);
}

void MenuController::xuLyXuatSVG() const {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\nNhap duong dan file SVG (mac dinh: output/draw.svg): ";
    std::string path;
    std::getline(std::cin, path);
    if (path.empty()) path = "output/draw.svg";

    SvgExporter exporter(800, 600);
    if (exporter.xuatFile(canvas_, path)) {
        std::cout << "[Thanh cong] Da xuat cac layer dang hien thi ra: " << path << "\n";
    } else {
        std::cout << "[Loi] Khong the ghi file SVG vao duong dan: " << path << "\n";
    }
}

void MenuController::xuLyXuatTXT() const {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\nNhap duong dan file TXT (mac dinh: output/draw.txt): ";
    std::string path;
    std::getline(std::cin, path);
    if (path.empty()) path = "output/draw.txt";

    if (TextSerializer::ghiFile(canvas_, path)) {
        std::cout << "[Thanh cong] Da luu trang thai canvas vao: " << path << "\n";
    } else {
        std::cout << "[Loi] Khong the ghi file vao: " << path << "\n";
    }
}

void MenuController::xuLyNhapTXT() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\nNhap duong dan file TXT de tai: ";
    std::string path;
    std::getline(std::cin, path);

    if (TextSerializer::docFile(canvas_, path)) {
        cmdMgr_.xoaLichSu();
        layerHienTai_ = 0;
        std::cout << "[Thanh cong] Da nap thanh cong trang thai bang ve tu " << path << "!\n";
    } else {
        std::cout << "[Loi] Khong the doc file: " << path << "\n";
    }
}

void MenuController::chay() {
    while (true) {
        hienThiMenuChinh();
        int chon = -1;
        if (!(std::cin >> chon)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (chon == 0) {
            std::cout << "Cam on ban da su dung Mini Vector CAD. Tam biet!\n";
            break;
        }

        switch (chon) {
            case 1:  xuLyQuanLyLayer();   break;
            case 2:  xuLyThemHinh();       break;
            case 3:  xuLyXoaHinh();        break;
            case 4:  xuLyBienDoiHinh();    break;
            case 5:  xuLyGomNhom();        break;
            case 6:  xuLyUndo();           break;
            case 7:  xuLyRedo();           break;
            case 8:  xuLyXemBangVe();      break;
            case 9:  xuLyXuatSVG();        break;
            case 10: xuLyXuatTXT();        break;
            case 11: xuLyNhapTXT();        break;
            default:
                std::cout << "[Loi] Tuy chon khong hop le, vui long chon lai!\n";
                break;
        }
    }
}
