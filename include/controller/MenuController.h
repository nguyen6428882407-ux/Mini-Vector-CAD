#pragma once
#include <cstddef>
#include <string>
#include "canvas/LichSuNetVe.h"

class BangVeCanvas;
class CommandManager;

class MenuController {
private:
    BangVeCanvas&    canvas_;
    CommandManager&  cmdMgr_;
    size_t           layerHienTai_;
    LichSuNetVeStack lichSuNetVe_;
    std::string      taiKhoanHienTai_;

    void hienThiMenuChinh() const;
    void xuLyQuanLyLayer();
    void xuLyThemHinh();
    void xuLyXoaHinh();
    void xuLyBienDoiHinh();
    void xuLyGomNhom();
    void xuLyUndo();
    void xuLyRedo();
    void xuLyXemBangVe() const;
    void xuLyXuatSVG() const;
    void xuLyXuatTXT() const;
    void xuLyNhapTXT();
    void xuLyLichSuNetVe();
    void xuLyDoiTaiKhoan();

    bool kiemTraLayerHopLe(size_t idx) const;
    bool kiemTraLayerCoTheSua(size_t idx) const;
    bool kiemTraIdTonTai(int id) const;

public:
    MenuController(BangVeCanvas& canvas, CommandManager& cmdMgr, const std::string& taiKhoanMacDinh = "admin");
    void chay();

    const LichSuNetVeStack& getLichSuNetVe() const;
    LichSuNetVeStack& getLichSuNetVe();
    const std::string& getTaiKhoanHienTai() const;
    void setTaiKhoanHienTai(const std::string& tk);
};
