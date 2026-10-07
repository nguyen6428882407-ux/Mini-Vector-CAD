#pragma once
#include <string>
#include <vector>
#include <memory>
#include "shapes/HinhHoc2D.h"

class Layer {
private:
    std::string ten_;
    bool hienThi_;
    bool khoa_;
    std::vector<std::unique_ptr<HinhHoc2D>> danhSachHinh_;

public:
    explicit Layer(const std::string& ten = "Layer");
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
    Layer(Layer&&) noexcept = default;
    Layer& operator=(Layer&&) noexcept = default;

    const std::string& getTen() const;
    void setTen(const std::string& ten);
    bool isHienThi() const;
    void setHienThi(bool hienThi);
    bool isKhoa() const;
    void setKhoa(bool khoa);
    void themHinh(std::unique_ptr<HinhHoc2D> hinh);
    void chenHinh(size_t viTri, std::unique_ptr<HinhHoc2D> hinh);
    std::unique_ptr<HinhHoc2D> rutHinh(int id);
    HinhHoc2D* timTheoId(int id) const;
    int timViTri(int id) const;
    const std::vector<std::unique_ptr<HinhHoc2D>>& getDanhSachHinh() const;
    size_t soLuongHinh() const;
};
