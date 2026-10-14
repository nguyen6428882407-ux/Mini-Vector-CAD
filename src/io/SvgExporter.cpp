#include "io/SvgExporter.h"
#include "canvas/BangVeCanvas.h"
#include "canvas/Layer.h"
#include "shapes/HinhHoc2D.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <locale>

namespace {
    std::string escapeXml(const std::string& text) {
        std::string result;
        result.reserve(text.size() * 11 / 10);
        for (char c : text) {
            switch (c) {
                case '&':  result += "&amp;";  break;
                case '<':  result += "&lt;";   break;
                case '>':  result += "&gt;";   break;
                case '"':  result += "&quot;"; break;
                case '\'': result += "&apos;"; break;
                default:   result += c;        break;
            }
        }
        return result;
    }

    std::string sanitizeComment(const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '-' && i + 1 < text.size() && text[i + 1] == '-') {
                result += "- ";
            } else {
                result += text[i];
            }
        }
        while (!result.empty() && result.back() == '-') {
            result.pop_back();
        }
        return result;
    }
} // namespace

SvgExporter::SvgExporter(double width, double height) {
    setKichThuoc(width, height);
}

double SvgExporter::getWidth() const { return width_; }
double SvgExporter::getHeight() const { return height_; }

void SvgExporter::setKichThuoc(double width, double height) {
    width_ = (width > 0.0 && std::isfinite(width)) ? width : 800.0;
    height_ = (height > 0.0 && std::isfinite(height)) ? height : 600.0;
}

std::string SvgExporter::taoChuoiSVG(const BangVeCanvas& canvas) const {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());

    double exportWidth = (width_ > 0.0 && std::isfinite(width_)) ? width_ :
                         (canvas.getRong() > 0.0 && std::isfinite(canvas.getRong()) ? canvas.getRong() : 800.0);
    double exportHeight = (height_ > 0.0 && std::isfinite(height_)) ? height_ :
                          (canvas.getCao() > 0.0 && std::isfinite(canvas.getCao()) ? canvas.getCao() : 600.0);

    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    oss << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << exportWidth << "\" height=\"" << exportHeight << "\" "
        << "viewBox=\"0 0 " << exportWidth << " " << exportHeight << "\">\n";

    for (size_t i = 0; i < canvas.soLayer(); ++i) {
        const Layer& lay = canvas.layer(i);
        if (!lay.isHien()) continue;

        const std::string& ten = lay.getTen();
        oss << "  <!-- Layer: " << sanitizeComment(ten) << " (ID index: " << i << ") -->\n";
        if (ten.empty()) {
            oss << "  <g id=\"layer_" << i << "\">\n";
        } else {
            oss << "  <g id=\"layer_" << i << "_" << escapeXml(ten) << "\">\n";
        }

        for (size_t j = 0; j < lay.soHinh(); ++j) {
            const HinhHoc2D& hinh = lay.hinhThu(j);
            std::string svgHinh = hinh.toSVG();
            if (svgHinh.empty()) continue;

            std::istringstream hss(svgHinh);
            std::string line;
            while (std::getline(hss, line)) {
                if (!line.empty()) {
                    oss << "    " << line << "\n";
                }
            }
        }
        oss << "  </g>\n";
    }
    oss << "</svg>\n";
    return oss.str();
}

bool SvgExporter::xuatFile(const BangVeCanvas& canvas, const std::string& duongDan) const {
    if (duongDan.empty()) {
        return false;
    }

    std::ofstream ofs(duongDan);
    if (!ofs.is_open()) {
        return false;
    }

    std::string svgContent = taoChuoiSVG(canvas);
    ofs << svgContent;

    if (!ofs.good()) {
        ofs.close();
        return false;
    }

    ofs.close();
    if (ofs.fail()) {
        return false;
    }

    return true;
}
