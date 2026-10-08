#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "shapes/HinhHoc2D.h"


class Layer {
private:
    std::string ten_;
    bool hien_;
    bool khoa_;
    std::vector<std::unique_ptr<HinhHoc2D>> hinh_;

public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
    explicit Layer(const std::string& ten);
    void themHinh(std::unique_ptr<HinhHoc2D> h, std::size_t viTri = npos);
    std::unique_ptr<HinhHoc2D> layHinh(int id);
    HinhHoc2D* timTheoId(int id);
    const HinhHoc2D* timTheoId(int id) const;
    std::size_t viTriTheoId(int id) const;
    HinhHoc2D& hinhThu(std::size_t i);               
    const HinhHoc2D& hinhThu(std::size_t i) const;
    std::size_t soHinh() const;
    const std::string& getTen() const;
    bool isHien() const;
    void setHien(bool b);
    bool isKhoa() const;
    void setKhoa(bool b);
};

