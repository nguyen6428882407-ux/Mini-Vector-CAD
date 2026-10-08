#include "canvas/BangVeCanvas.h"
#include <stdexcept>

BangVeCanvas::BangVeCanvas(double rong, double cao)
    : rong_(rong), cao_(cao), layerHienTai_(0) {}

std::size_t BangVeCanvas::themLayer(const std::string& ten) {
    layers_.emplace_back(ten);   // deque: địa chỉ các Layer đã có không đổi
    return layers_.size() - 1;
}

Layer& BangVeCanvas::layer(std::size_t i) { return layers_.at(i); }
const Layer& BangVeCanvas::layer(std::size_t i) const { return layers_.at(i); }
std::size_t BangVeCanvas::soLayer() const { return layers_.size(); }

double BangVeCanvas::getRong() const { return rong_; }
double BangVeCanvas::getCao() const { return cao_; }

std::size_t BangVeCanvas::getLayerHienTai() const { return layerHienTai_; }

void BangVeCanvas::setLayerHienTai(std::size_t i) {
    if (i >= layers_.size())
        throw std::out_of_range("BangVeCanvas::setLayerHienTai: chi so layer khong ton tai");
    layerHienTai_ = i;
}

HinhHoc2D* BangVeCanvas::timHinh(int id) {
    for (Layer& ly : layers_)                 // tìm cả ở layer đang ẩn/khóa
        if (HinhHoc2D* h = ly.timTheoId(id)) return h;
    return nullptr;
}

const HinhHoc2D* BangVeCanvas::timHinh(int id) const {
    for (const Layer& ly : layers_)
        if (const HinhHoc2D* h = ly.timTheoId(id)) return h;
    return nullptr;
}

std::size_t BangVeCanvas::layerChuaHinh(int id) const {
    for (std::size_t i = 0; i < layers_.size(); ++i)
        if (layers_[i].timTheoId(id) != nullptr) return i;
    return Layer::npos;
}

