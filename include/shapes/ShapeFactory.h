#pragma once
#include <memory>
#include <string>
#include "shapes/HinhHoc2D.h"

// ============================================================================
// ShapeFactory - Factory Method / Simple Factory tạo hình từ chuỗi mô tả
// Tầng: shapes / io | Phụ trách: Trường Vũ (Task 10)
// ============================================================================
class ShapeFactory {
public:
    static std::unique_ptr<HinhHoc2D> taoHinh(const std::string& moTa);
};
