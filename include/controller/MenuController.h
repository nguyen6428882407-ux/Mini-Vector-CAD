#pragma once
#include <cstddef>
#include <string>

class BangVeCanvas;
class CommandManager;

class MenuController {
private:
    BangVeCanvas&   canvas_;
    CommandManager& cmdMgr_;
    size_t          layerHienTai_;

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

    bool kiemTraLayerHopLe(size_t idx) const;
    bool kiemTraLayerCoTheSua(size_t idx) const;
    bool kiemTraIdTonTai(int id) const;

public:
    MenuController(BangVeCanvas& canvas, CommandManager& cmdMgr);
    void chay();
};
