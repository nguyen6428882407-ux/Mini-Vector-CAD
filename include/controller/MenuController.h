#pragma once
#include <cstddef>
#include <string>
#include <vector>

class BangVeCanvas;
class CommandManager;

struct LichSuVeItem {
    int id;
    std::string loaiHinh;
    std::string nguoiVe;
    std::string thoiGianVe;
    bool daXoa;
    std::string thoiGianXoa;
};

class MenuController {
private:
    BangVeCanvas&   canvas_;
    CommandManager& cmdMgr_;
    size_t          layerHienTai_;
    std::vector<LichSuVeItem> lichSuVe_;

    void hienThiMenuChinh() const;
    void xuLyQuanLyLayer();
    void xuLyThemHinh();
    void xuLyXoaHinh();
    void xuLyBienDoiHinh();
    void xuLyGomNhom();
    void xuLyUndo();
    void xuLyRedo();
    void xuLyXemBangVe() const;
    void xuLyXemLichSuVe() const;
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
