#pragma once
#include <string>

class BangVeCanvas;

class SvgExporter {
private:
    double width_, height_;
public:
    SvgExporter(double width = 800.0, double height = 600.0);
    double getWidth() const;
    double getHeight() const;
    void setKichThuoc(double width, double height);
    bool xuatFile(const BangVeCanvas& canvas, const std::string& duongDan) const;
    std::string taoChuoiSVG(const BangVeCanvas& canvas) const;
};
