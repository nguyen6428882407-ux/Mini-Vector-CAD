#include "canvas/BangVeCanvas.h"

BangVeCanvas::BangVeCanvas() {
    cacLayer_.emplace_back("Layer 1");
}

size_t BangVeCanvas::themLayer(const std::string& ten) {
    cacLayer_.emplace_back(ten);
    return cacLayer_.size() - 1;
}

bool BangVeCanvas::xoaLayer(size_t idx) {
    if (idx >= cacLayer_.size() || cacLayer_.size() <= 1) return false;
    cacLayer_.erase(cacLayer_.begin() + idx);
    return true;
}

void BangVeCanvas::xoaTatCa() {
    cacLayer_.clear();
    cacLayer_.emplace_back("Layer 1");
}

size_t BangVeCanvas::soLuongLayer() const {
    return cacLayer_.size();
}

Layer& BangVeCanvas::layer(size_t idx) {
    if (idx >= cacLayer_.size()) {
        throw std::out_of_range("Chi so layer ngoai pham vi: " + std::to_string(idx));
    }
    return cacLayer_[idx];
}

const Layer& BangVeCanvas::layer(size_t idx) const {
    if (idx >= cacLayer_.size()) {
        throw std::out_of_range("Chi so layer ngoai pham vi: " + std::to_string(idx));
    }
    return cacLayer_[idx];
}

HinhHoc2D* BangVeCanvas::timHinhTheoId(int id) const {
    for (const auto& lay : cacLayer_) {
        HinhHoc2D* h = lay.timTheoId(id);
        if (h) return h;
    }
    return nullptr;
}

int BangVeCanvas::timLayerChuaHinh(int id) const {
    for (size_t i = 0; i < cacLayer_.size(); ++i) {
        if (cacLayer_[i].timTheoId(id) != nullptr) return static_cast<int>(i);
    }
    return -1;
}
