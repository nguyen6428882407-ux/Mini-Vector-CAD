#include "canvas/LichSuNetVe.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

std::string BanGhiLichSuVe::moTa() const {
    std::ostringstream oss;
    oss << "[" << thoiGianVe << "] "
        << "Tai khoan: " << (taiKhoanVe.empty() ? "(Vo danh)" : taiKhoanVe)
        << " | Chuc nang: " << chucNang;
    if (idHinh >= 0) {
        oss << " (ID #" << idHinh << ", Layer " << layerIdx << ")";
    }
    if (!chiTiet.empty()) {
        oss << " | Chi tiet: " << chiTiet;
    }
    return oss.str();
}

std::string LichSuNetVeStack::layThoiGianHienTai() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void LichSuNetVeStack::push(const BanGhiLichSuVe& banGhi) {
    stack_.push_back(banGhi);
}

void LichSuNetVeStack::themNetVe(const std::string& chucNang,
                                 const std::string& taiKhoanVe,
                                 int idHinh,
                                 std::size_t layerIdx,
                                 const std::string& chiTiet) {
    BanGhiLichSuVe bg;
    bg.thoiGianVe = layThoiGianHienTai();
    bg.taiKhoanVe = taiKhoanVe.empty() ? "guest" : taiKhoanVe;
    bg.chucNang = chucNang;
    bg.idHinh = idHinh;
    bg.layerIdx = layerIdx;
    bg.chiTiet = chiTiet;
    stack_.push_back(std::move(bg));
}

bool LichSuNetVeStack::pop(BanGhiLichSuVe& outBanGhi) {
    if (stack_.empty()) {
        return false;
    }
    outBanGhi = std::move(stack_.back());
    stack_.pop_back();
    return true;
}

bool LichSuNetVeStack::pop() {
    if (stack_.empty()) {
        return false;
    }
    stack_.pop_back();
    return true;
}

const BanGhiLichSuVe* LichSuNetVeStack::top() const {
    if (stack_.empty()) {
        return nullptr;
    }
    return &stack_.back();
}

bool LichSuNetVeStack::empty() const {
    return stack_.empty();
}

std::size_t LichSuNetVeStack::size() const {
    return stack_.size();
}

void LichSuNetVeStack::clear() {
    stack_.clear();
}

std::vector<BanGhiLichSuVe> LichSuNetVeStack::layDanhSachTuDinh() const {
    std::vector<BanGhiLichSuVe> ketQua = stack_;
    std::reverse(ketQua.begin(), ketQua.end());
    return ketQua;
}

const std::vector<BanGhiLichSuVe>& LichSuNetVeStack::layToanBo() const {
    return stack_;
}

void LichSuNetVeStack::inLichSu(std::ostream& os) const {
    os << "======================== LICH SU NET VE (STACK LIFO) ========================\n";
    os << "Tong so thao tac net ve trong Stack: " << stack_.size() << "\n";
    if (stack_.empty()) {
        os << "  (Ngan xep lich su dang trong - Chua co net ve nao duoc thuc hien)\n";
        os << "=============================================================================\n";
        return;
    }

    os << "-----------------------------------------------------------------------------\n";
    os << "Thu tu duyet: Tu DINH STACK (Moi nhat) xuong DAY STACK (Cu nhat):\n";
    os << "-----------------------------------------------------------------------------\n";

    for (std::size_t i = stack_.size(); i > 0; --i) {
        std::size_t idx = i - 1;
        const auto& item = stack_[idx];
        std::string tag = (idx == stack_.size() - 1) ? " [TOP/DINH]" :
                          (idx == 0)                 ? " [BOTTOM/DAY]" : "";

        os << "#" << (idx + 1) << tag << "\n"
           << "  - Thoi gian ve : " << item.thoiGianVe << "\n"
           << "  - Tai khoan ve : " << item.taiKhoanVe << "\n"
           << "  - Chuc nang    : " << item.chucNang << "\n";
        if (item.idHinh >= 0) {
            os << "  - Doi tuong    : Hinh ID #" << item.idHinh << " (Layer " << item.layerIdx << ")\n";
        }
        if (!item.chiTiet.empty()) {
            os << "  - Chi tiet lenh: " << item.chiTiet << "\n";
        }
        os << "-----------------------------------------------------------------------------\n";
    }
    os << "=============================================================================\n";
}

bool LichSuNetVeStack::xuatFile(const std::string& path) const {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    ofs << "# MINI-VECTOR-CAD STROKE HISTORY LOG (STACK LIFO)\n";
    ofs << "# Format: THOI_GIAN | TAI_KHOAN | CHUC_NANG | ID_HINH | LAYER | CHI_TIET\n";
    for (const auto& item : stack_) {
        ofs << item.thoiGianVe << " | "
            << item.taiKhoanVe << " | "
            << item.chucNang << " | "
            << item.idHinh << " | "
            << item.layerIdx << " | "
            << item.chiTiet << "\n";
    }
    return true;
}

bool LichSuNetVeStack::napFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    stack_.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, '|')) {
            // Trim whitespace
            auto start = token.find_first_not_of(" \t\r\n");
            auto end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                tokens.push_back(token.substr(start, end - start + 1));
            } else {
                tokens.push_back("");
            }
        }

        if (tokens.size() >= 3) {
            BanGhiLichSuVe bg;
            bg.thoiGianVe = tokens[0];
            bg.taiKhoanVe = tokens[1];
            bg.chucNang = tokens[2];
            if (tokens.size() >= 4 && !tokens[3].empty()) {
                try { bg.idHinh = std::stoi(tokens[3]); } catch (...) { bg.idHinh = -1; }
            }
            if (tokens.size() >= 5 && !tokens[4].empty()) {
                try { bg.layerIdx = static_cast<std::size_t>(std::stoul(tokens[4])); } catch (...) { bg.layerIdx = 0; }
            }
            if (tokens.size() >= 6) {
                bg.chiTiet = tokens[5];
            }
            stack_.push_back(std::move(bg));
        }
    }
    return true;
}
