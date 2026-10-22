#include "controller/MenuController.h"
#include "canvas/BangVeCanvas.h"
#include "canvas/Layer.h"
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
#include <cmath>

MenuController::MenuController(BangVeCanvas& canvas, CommandManager& cmdMgr, const std::string& taiKhoanMacDinh)
    : canvas_(canvas),
      cmdMgr_(cmdMgr),
      layerHienTai_(0),
      taiKhoanHienTai_(taiKhoanMacDinh.empty() ? "admin" : taiKhoanMacDinh) {}

void MenuController::hienThiMenuChinh() const {
    std::cout << "\n======================================================\n";
    std::cout << "          CHUONG TRINH MINI VECTOR CAD (C++ OOP)      \n";
    std::cout << "======================================================\n";
    std::string tenLayer = (layerHienTai_ < canvas_.soLayer())
                           ? canvas_.layer(layerHienTai_).getTen() : "Khong hop le";
    bool khoa = (layerHienTai_ < canvas_.soLayer()) && canvas_.layer(layerHienTai_).isKhoa();
    bool hien = (layerHienTai_ < canvas_.soLayer()) && canvas_.layer(layerHienTai_).isHien();

    std::cout << "Layer hien tai: [" << layerHienTai_ << "] \"" << tenLayer << "\""
              << " | " << (hien ? "Hien" : "An")
              << " | " << (khoa ? "KHOA" : "MO")
              << " | Tai khoan: [" << taiKhoanHienTai_ << "]"
              << " | Stack net ve: [" << lichSuNetVe_.size() << "]\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << " 1. Quan ly Layer (Danh sach / Them / An-Hien / Khoa)\n";
    std::cout << " 2. Them hinh moi vao Layer hien tai (Luu Stack lich su)\n";
    std::cout << " 3. Xoa hinh theo ID\n";
    std::cout << " 4. Bien doi hinh (Tinh tien / Xoay / Thu phong)\n";
    std::cout << " 5. Gom nhom cac hinh (Group)\n";
    std::cout << " 6. Hoan tac (Undo) [" << (cmdMgr_.coTheUndo() ? "Kha dung" : "Het") << "]\n";
    std::cout << " 7. Lam lai (Redo)  [" << (cmdMgr_.coTheRedo() ? "Kha dung" : "Het") << "]\n";
    std::cout << " 8. Xem toan bo bang ve (Text View)\n";
    std::cout << " 9. Xuat ra file SVG (.svg)\n";
    std::cout << "10. Xuat ra file trang thai (.txt)\n";
    std::cout << "11. Nap file trang thai (.txt)\n";
    std::cout << "12. Xem lich su net ve (Stroke History Stack LIFO)\n";
    std::cout << "13. Doi tai khoan nguoi dung dang ve\n";
    std::cout << " 0. Thoat\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "Lua chon cua ban: ";
}

bool MenuController::kiemTraLayerHopLe(size_t idx) const {
    return idx < canvas_.soLayer();
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
    return canvas_.timHinh(id) != nullptr;
}

void MenuController::xuLyQuanLyLayer() {
    while (true) {
        std::cout << "\n--- QUAN LY LAYER ---\n";
        for (size_t i = 0; i < canvas_.soLayer(); ++i) {
            const auto& lay = canvas_.layer(i);
            std::cout << " [" << i << "] \"" << lay.getTen() << "\""
                      << (i == layerHienTai_ ? " <-- (Dang chon)" : "")
                      << " | Hien thi: " << (lay.isHien() ? "Co" : "Khong")
                      << " | Khoa: " << (lay.isKhoa() ? "KHOA" : "Mo")
                      << " | So hinh: " << lay.soHinh() << "\n";
        }
        std::cout << "---------------------\n";
        std::cout << " 1. Chon layer lam viec\n";
        std::cout << " 2. Them layer moi\n";
        std::cout << " 3. Bat / Tat hien thi layer\n";
        std::cout << " 4. Khoa / Mo khoa layer\n";
        std::cout << " 0. Quay lai menu chinh\n";
        std::cout << "Chon: ";

        int ch = -1;
        if (!(std::cin >> ch)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (ch == 0) break;
        if (ch == 1) {
            std::cout << "Nhap chi so layer muon chon: ";
            size_t idx = 0;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                layerHienTai_ = idx;
                canvas_.setLayerHienTai(idx);
                std::cout << "[Thanh cong] Da chuyen sang layer [" << idx << "]: \"" << canvas_.layer(idx).getTen() << "\".\n";
            } else {
                if (std::cin.eof()) break;
                std::cout << "[Loi] Chi so layer khong hop le!\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        } else if (ch == 2) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nhap ten layer moi: ";
            std::string ten;
            std::getline(std::cin, ten);
            if (ten.empty()) {
                std::cout << "[Loi] Ten layer khong duoc de trong!\n";
            } else {
                size_t moi = canvas_.themLayer(ten);
                layerHienTai_ = moi;
                std::cout << "[Thanh cong] Da tao va chon layer moi [" << moi << "]: \"" << ten << "\".\n";
            }
        } else if (ch == 3) {
            std::cout << "Nhap chi so layer can An/Hien: ";
            size_t idx = 0;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                bool ht = canvas_.layer(idx).isHien();
                canvas_.layer(idx).setHien(!ht);
                std::cout << "[Thanh cong] Layer [" << idx << "] gio dang: " << (!ht ? "Hien thi" : "Bi an") << ".\n";
            } else {
                if (std::cin.eof()) break;
                std::cout << "[Loi] Chi so layer khong hop le!\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        } else if (ch == 4) {
            std::cout << "Nhap chi so layer can Khoa/Mo khoa: ";
            size_t idx = 0;
            if (std::cin >> idx && kiemTraLayerHopLe(idx)) {
                bool kh = canvas_.layer(idx).isKhoa();
                canvas_.layer(idx).setKhoa(!kh);
                std::cout << "[Thanh cong] Layer [" << idx << "] gio dang: " << (!kh ? "KHOA" : "MO") << ".\n";
            } else {
                if (std::cin.eof()) break;
                std::cout << "[Loi] Chi so layer khong hop le!\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
    std::cout << " - brush <so_diem> <x1> <y1> <x2> <y2> ...\n";
    std::cout << " (Co the them: vien <r> <g> <b> [a], nen <r> <g> <b> [a], net <w>)\n";
    std::cout << "Nhap lenh tao hinh: ";

    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return;

    try {
        auto hinh = ShapeFactory::taoHinh(line);
        if (hinh) {
            int id = hinh->getId();
            std::istringstream iss(line);
            std::string loai;
            iss >> loai;

            auto cmd = std::make_unique<AddShapeCommand>(canvas_, layerHienTai_, std::move(hinh));
            cmdMgr_.thucThi(std::move(cmd));

            lichSuNetVe_.themNetVe("Them hinh (" + loai + ")", taiKhoanHienTai_, id, layerHienTai_, line);

            std::cout << "[Thanh cong] Da them hinh ID #" << id << " vao layer [" << layerHienTai_ << "].\n";
            std::cout << "             [Stack Net Ve] Da ghi nhan thao tac cua tai khoan [" << taiKhoanHienTai_ << "].\n";
        } else {
            std::cout << "[Loi] Loai hinh khong duoc ho tro hoac chuoi mo ta rong!\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyXoaHinh() {
    std::cout << "\nNhap ID hinh muon xoa: ";
    int id = 0;
    if (!(std::cin >> id)) {
        if (std::cin.eof()) return;
        std::cout << "[Loi] ID phai la so nguyen!\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    size_t layIdx = canvas_.layerChuaHinh(id);
    if (layIdx == Layer::npos) {
        std::cout << "[Loi] Khong tim thay hinh co ID #" << id << " tren bang ve!\n";
        return;
    }

    if (!kiemTraLayerCoTheSua(layIdx)) return;

    try {
        auto cmd = std::make_unique<RemoveShapeCommand>(canvas_, layIdx, id);
        cmdMgr_.thucThi(std::move(cmd));

        lichSuNetVe_.themNetVe("Xoa hinh #" + std::to_string(id), taiKhoanHienTai_, id, layIdx, "Xoa khoi layer");

        std::cout << "[Thanh cong] Da xoa hinh ID #" << id << " khoi layer [" << layIdx << "].\n";
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyBienDoiHinh() {
    std::cout << "\nNhap ID hinh muon bien doi: ";
    int id = 0;
    if (!(std::cin >> id)) {
        if (std::cin.eof()) return;
        std::cout << "[Loi] ID phai la so nguyen!\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    size_t layIdx = canvas_.layerChuaHinh(id);
    if (layIdx == Layer::npos) {
        std::cout << "[Loi] Khong tim thay hinh co ID #" << id << "!\n";
        return;
    }

    if (!kiemTraLayerCoTheSua(layIdx)) return;

    HinhHoc2D* hinh = canvas_.timHinh(id);
    if (!hinh) {
        std::cout << "[Loi] Khong the lay con tro hinh ID #" << id << "!\n";
        return;
    }
    Diem2D tamHinh = hinh->tam();

    std::cout << "Hinh #" << id << " co tam bounding box: " << tamHinh << "\n";
    std::cout << "Chon phep bien doi:\n";
    std::cout << " 1. Tinh tien (dx, dy)\n";
    std::cout << " 2. Xoay (gocDo quanh tam hinh)\n";
    std::cout << " 3. Thu phong / Scale deu (ti le k quanh tam hinh)\n";
    std::cout << "Chon: ";

    int opt = 0;
    if (!(std::cin >> opt)) {
        if (std::cin.eof()) return;
        std::cout << "[Loi] Lua chon phai la so nguyen!\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    BienDoi2D bd;
    std::string tenBD;
    if (opt == 1) {
        double dx = 0, dy = 0;
        std::cout << "Nhap dx dy: ";
        if (!(std::cin >> dx >> dy) || !std::isfinite(dx) || !std::isfinite(dy)) {
            if (std::cin.eof()) return;
            std::cout << "[Loi] Toa do tinh tien phai la so thuc huu han!\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        bd = BienDoi2D::tinhTien(dx, dy);
        tenBD = "Tinh tien (dx=" + std::to_string(dx) + ", dy=" + std::to_string(dy) + ")";
    } else if (opt == 2) {
        double goc = 0;
        std::cout << "Nhap goc xoay (do): ";
        if (!(std::cin >> goc) || !std::isfinite(goc)) {
            if (std::cin.eof()) return;
            std::cout << "[Loi] Goc xoay phai la so thuc huu han!\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        bd = BienDoi2D::xoay(goc, tamHinh);
        tenBD = "Xoay " + std::to_string(goc) + " do";
    } else if (opt == 3) {
        double k = 1.0;
        std::cout << "Nhap he so scale k (k != 0): ";
        if (!(std::cin >> k) || !std::isfinite(k)) {
            if (std::cin.eof()) return;
            std::cout << "[Loi] He so thu phong phai la so thuc huu han!\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        if (MathUtils::bangNhau(k, 0.0) || std::abs(k) < MathUtils::EPS) {
            std::cout << "[Loi] He so thu phong k khong duoc bang 0!\n";
            return;
        }
        bd = BienDoi2D::tiLeDeu(k, tamHinh);
        tenBD = "Scale he so k=" + std::to_string(k);
    } else {
        std::cout << "[Loi] Lua chon khong hop le!\n";
        return;
    }

    try {
        auto cmd = std::make_unique<TransformCommand>(canvas_, layIdx, id, bd);
        cmdMgr_.thucThi(std::move(cmd));

        lichSuNetVe_.themNetVe("Bien doi hinh (" + tenBD + ")", taiKhoanHienTai_, id, layIdx, tenBD);

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
    std::string token;
    while (iss >> token) {
        int tid = 0;
        try {
            size_t idx = 0;
            tid = std::stoi(token, &idx);
            if (idx != token.size()) {
                std::cout << "[Loi] ID phai la so nguyen hop le: " << token << "\n";
                return;
            }
        } catch (...) {
            std::cout << "[Loi] ID phai la so nguyen hop le: " << token << "\n";
            return;
        }
        ids.push_back(tid);
    }

    if (ids.size() < 2) {
        std::cout << "[Loi] Can it nhat 2 hinh de gom nhom!\n";
        return;
    }

    for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
            if (ids[i] == ids[j]) {
                std::cout << "[Loi] Danh sach chua ID trung lap: #" << ids[i] << "!\n";
                return;
            }
        }
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
        int gid = cmd->getGroupId();
        cmdMgr_.thucThi(std::move(cmd));

        std::string idsStr;
        for (size_t i = 0; i < ids.size(); ++i) {
            if (i > 0) idsStr += ", ";
            idsStr += "#" + std::to_string(ids[i]);
        }
        lichSuNetVe_.themNetVe("Gom nhom (" + idsStr + ")", taiKhoanHienTai_, gid, layerHienTai_, "Tao GroupShape");

        std::cout << "[Thanh cong] Da gom " << ids.size() << " hinh thanh mot GroupShape moi (ID #" << gid << ").\n";
    } catch (const std::exception& e) {
        std::cout << "[Loi] " << e.what() << "\n";
    }
}

void MenuController::xuLyUndo() {
    if (cmdMgr_.undo()) {
        std::cout << "[Thanh cong] Da Hoan tac (Undo) lenh truoc do.\n";
        BanGhiLichSuVe bg;
        if (lichSuNetVe_.pop(bg)) {
            std::cout << "  -> [Stack Pop] Rut net ve gan nhat khoi Stack:\n"
                      << "     * Chuc nang : " << bg.chucNang << "\n"
                      << "     * Tai khoan : " << bg.taiKhoanVe << "\n"
                      << "     * Thoi gian : " << bg.thoiGianVe << "\n";
        }
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

    SvgExporter exporter(canvas_.getRong(), canvas_.getCao());
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
    if (path.empty()) {
        std::cout << "[Loi] Duong dan file khong duoc de trong!\n";
        return;
    }

    if (TextSerializer::docFile(canvas_, path)) {
        cmdMgr_.xoaLichSu();
        layerHienTai_ = 0;
        std::cout << "[Thanh cong] Da nap thanh cong trang thai bang ve tu " << path << "!\n";
    } else {
        std::cout << "[Loi] Khong the doc file hoac file co loi cu phap: " << path << "\n";
    }
}

void MenuController::xuLyLichSuNetVe() {
    while (true) {
        std::cout << "\n--- QUAN LY LICH SU NET VE (STACK LIFO) ---\n";
        std::cout << "Tai khoan hien tai : [" << taiKhoanHienTai_ << "]\n";
        std::cout << "Tong so net ve     : " << lichSuNetVe_.size() << " thao tac\n";
        if (!lichSuNetVe_.empty()) {
            const auto* dinh = lichSuNetVe_.top();
            if (dinh) {
                std::cout << "Net ve o DINH STACK: " << dinh->chucNang
                          << " | Boi: " << dinh->taiKhoanVe
                          << " | Luc: " << dinh->thoiGianVe << "\n";
            }
        }
        std::cout << "-------------------------------------------\n";
        std::cout << " 1. In toan bo Stack lich su (tu Dinh xuong Day)\n";
        std::cout << " 2. Xem chi tiet net ve tren DINH Stack (Top)\n";
        std::cout << " 3. Rut bo net ve gan nhat (Pop Stack)\n";
        std::cout << " 4. Xuat lich su net ve ra file (.txt)\n";
        std::cout << " 5. Nap lich su net ve tu file (.txt)\n";
        std::cout << " 6. Xoa sach toan bo Stack lich su (Clear)\n";
        std::cout << " 0. Quay lai menu chinh\n";
        std::cout << "Lua chon cua ban: ";

        int opt = -1;
        if (!(std::cin >> opt)) {
            if (std::cin.eof()) return;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (opt == 0) break;
        if (opt == 1) {
            std::cout << "\n";
            lichSuNetVe_.inLichSu(std::cout);
        } else if (opt == 2) {
            const auto* dinh = lichSuNetVe_.top();
            if (!dinh) {
                std::cout << "[Thong bao] Stack dang trong!\n";
            } else {
                std::cout << "\n--- CHI TIET NET VE O DINH STACK ---\n"
                          << "Thoi gian : " << dinh->thoiGianVe << "\n"
                          << "Tai khoan : " << dinh->taiKhoanVe << "\n"
                          << "Chuc nang : " << dinh->chucNang << "\n"
                          << "ID Hinh   : #" << dinh->idHinh << " (Layer " << dinh->layerIdx << ")\n"
                          << "Chi tiet  : " << dinh->chiTiet << "\n";
            }
        } else if (opt == 3) {
            BanGhiLichSuVe bg;
            if (lichSuNetVe_.pop(bg)) {
                std::cout << "[Thanh cong] Da pop net ve khoi Stack: "
                          << bg.chucNang << " (Tai khoan: " << bg.taiKhoanVe << ", Luc: " << bg.thoiGianVe << ")\n";
            } else {
                std::cout << "[Thong bao] Stack dang trong, khong co gi de pop!\n";
            }
        } else if (opt == 4) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nhap duong dan file xuat (mac dinh: output/stroke_history.txt): ";
            std::string path;
            std::getline(std::cin, path);
            if (path.empty()) path = "output/stroke_history.txt";
            if (lichSuNetVe_.xuatFile(path)) {
                std::cout << "[Thanh cong] Da xuat " << lichSuNetVe_.size() << " ban ghi lich su ra: " << path << "\n";
            } else {
                std::cout << "[Loi] Khong the ghi file: " << path << "\n";
            }
        } else if (opt == 5) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nhap duong dan file lich su can nap: ";
            std::string path;
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "[Loi] Duong dan khong duoc de trong!\n";
            } else if (lichSuNetVe_.napFile(path)) {
                std::cout << "[Thanh cong] Da nap thanh cong " << lichSuNetVe_.size() << " ban ghi vao Stack tu: " << path << "\n";
            } else {
                std::cout << "[Loi] Khong the doc file: " << path << "\n";
            }
        } else if (opt == 6) {
            lichSuNetVe_.clear();
            std::cout << "[Thanh cong] Da xoa sach toan bo lich su net ve trong Stack.\n";
        }
    }
}

void MenuController::xuLyDoiTaiKhoan() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\nTai khoan ve hien tai: [" << taiKhoanHienTai_ << "]\n";
    std::cout << "Nhap ten tai khoan moi: ";
    std::string moi;
    std::getline(std::cin, moi);
    if (moi.empty()) {
        std::cout << "[Loi] Ten tai khoan khong duoc de trong!\n";
    } else {
        taiKhoanHienTai_ = moi;
        std::cout << "[Thanh cong] Da chuyen sang tai khoan ve: [" << taiKhoanHienTai_ << "].\n";
        std::cout << "            Cac net ve tiep theo se duoc ghi nhan duoi ten tai khoan nay vao Stack.\n";
    }
}

const LichSuNetVeStack& MenuController::getLichSuNetVe() const {
    return lichSuNetVe_;
}

LichSuNetVeStack& MenuController::getLichSuNetVe() {
    return lichSuNetVe_;
}

const std::string& MenuController::getTaiKhoanHienTai() const {
    return taiKhoanHienTai_;
}

void MenuController::setTaiKhoanHienTai(const std::string& tk) {
    if (!tk.empty()) taiKhoanHienTai_ = tk;
}

void MenuController::chay() {
    while (true) {
        hienThiMenuChinh();
        int chon = -1;
        if (!(std::cin >> chon)) {
            if (std::cin.eof()) {
                std::cout << "\n[Thong bao] Phat hien ket thuc luong nhap (EOF). Thoat chuong trinh!\n";
                break;
            }
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
            case 12: xuLyLichSuNetVe();    break;
            case 13: xuLyDoiTaiKhoan();    break;
            default:
                std::cout << "[Loi] Tuy chon khong hop le, vui long chon lai!\n";
                break;
        }
    }
}
