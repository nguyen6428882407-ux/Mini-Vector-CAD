#include "io/SvgExporter.h"
#include "canvas/BangVeCanvas.h"
#include <fstream>
#include <sstream>

SvgExporter::SvgExporter(double width, double height)
    : width_(width), height_(height) {}

double SvgExporter::getWidth() const { return width_; }
double SvgExporter::getHeight() const { return height_; }
void SvgExporter::setKichThuoc(double width, double height) {
    width_ = width; height_ = height;
}

std::string SvgExporter::taoChuoiSVG(const BangVeCanvas& canvas) const {
    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    oss << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << width_ << "\" height=\"" << height_ << "\" "
        << "viewBox=\"0 0 " << width_ << " " << height_ << "\">\n";

    for (size_t i = 0; i < canvas.soLuongLayer(); ++i) {
        const auto& lay = canvas.layer(i);
        if (!lay.isHienThi()) continue;

        oss << "  <!-- Layer: " << lay.getTen() << " (ID index: " << i << ") -->\n";
        oss << "  <g id=\"layer_" << i << "_" << lay.getTen() << "\">\n";
        for (const auto& hinh : lay.getDanhSachHinh()) {
            if (hinh) oss << "    " << hinh->toSVG() << "\n";
        }
        oss << "  </g>\n";
    }
    oss << "</svg>\n";
    return oss.str();
}

bool SvgExporter::xuatFile(const BangVeCanvas& canvas, const std::string& duongDan) const {
    std::ofstream ofs(duongDan);
    if (!ofs.is_open()) return false;
    ofs << taoChuoiSVG(canvas);
    ofs.close();
    return true;
}
