#pragma once
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include "canvas/Layer.h"

class BangVeCanvas {
private:
    std::vector<Layer> cacLayer_;

public:
    BangVeCanvas();
    size_t themLayer(const std::string& ten = "Layer moi");
    bool xoaLayer(size_t idx);
    void xoaTatCa();
    size_t soLuongLayer() const;
    Layer& layer(size_t idx);
    const Layer& layer(size_t idx) const;
    HinhHoc2D* timHinhTheoId(int id) const;
    int timLayerChuaHinh(int id) const;
};
