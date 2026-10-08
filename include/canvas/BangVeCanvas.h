#pragma once
#include <cstddef>
#include <deque>
#include <string>
#include "canvas/Layer.h"

class BangVeCanvas {
private:
    double rong_;
    double cao_;
    std::deque<Layer> layers_;   
    std::size_t layerHienTai_;  

public:
    BangVeCanvas(double rong, double cao);
    BangVeCanvas(const BangVeCanvas&) = delete;
    BangVeCanvas& operator=(const BangVeCanvas&) = delete;
    BangVeCanvas(BangVeCanvas&&) = delete;
    BangVeCanvas& operator=(BangVeCanvas&&) = delete;
    std::size_t themLayer(const std::string& ten);
    Layer& layer(std::size_t i);              
    const Layer& layer(std::size_t i) const;
    std::size_t soLayer() const;
    double getRong() const;
    double getCao() const;
    std::size_t getLayerHienTai() const;
    void setLayerHienTai(std::size_t i);
    HinhHoc2D* timHinh(int id);
    const HinhHoc2D* timHinh(int id) const;
    std::size_t layerChuaHinh(int id) const;
};

