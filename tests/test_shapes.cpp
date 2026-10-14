#include <iostream>
#include <cassert>
#include <vector>
#include "core/Diem2D.h"
#include "core/MathUtils.h"
#include "core/MauSac.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"

void testDoanThang() {
    std::cout << "[TEST] Kiems tra DoanThang..." << std::endl;
    
    Diem2D p1(0.0, 0.0);
    Diem2D p2(4.0, 0.0);
    DoanThang dt(p1, p2);

    // 1. Chu vi (Do dai doan thang) = 4
    assert(MathUtils::bangNhau(dt.tinhChuVi(), 4.0));

    // 2. Dien tich doan thang = 0
    assert(MathUtils::bangNhau(dt.tinhDienTich(), 0.0));

    // 3. Tam (Trung diem) = (2, 0)
    assert(dt.tinhTam() == Diem2D(2.0, 0.0));

    // 4. Tinh tien (+2, +3) -> tam moi (4, 3)
    dt.tinhTien(2.0, 3.0);
    assert(dt.tinhTam() == Diem2D(4.0, 3.0));

    // 5. Scale x2 (do dai thanh 8)
    dt.scale(2.0);
    assert(MathUtils::bangNhau(dt.tinhChuVi(), 8.0));

    // 6. Xoay 90 do quanh tam (Tam giu nguyen)
    dt.xoay(MathUtils::tuDoSangRadian(90.0));
    assert(dt.tinhTam() == Diem2D(4.0, 3.0));
    assert(MathUtils::bangNhau(dt.tinhChuVi(), 8.0));

    std::cout << "  => DoanThang PASSED!" << std::endl;
}

void testHinhTron() {
    std::cout << "[TEST] Kiem tra HinhTron..." << std::endl;

    Diem2D tam(1.0, 1.0);
    double r = 5.0;
    HinhTron ht(tam, r);

    // 1. Chu vi = 2 * PI * R
    assert(MathUtils::bangNhau(ht.tinhChuVi(), 2.0 * MathUtils::PI * 5.0));

    // 2. Dien tich = PI * R^2
    assert(MathUtils::bangNhau(ht.tinhDienTich(), MathUtils::PI * 25.0));

    // 3. Tam = (1, 1)
    assert(ht.tinhTam() == Diem2D(1.0, 1.0));

    // 4. Tinh tien (+3, -1) -> tam moi (4, 0)
    ht.tinhTien(3.0, -1.0);
    assert(ht.tinhTam() == Diem2D(4.0, 0.0));

    // 5. Scale x0.5 -> R moi = 2.5
    ht.scale(0.5);
    assert(MathUtils::bangNhau(ht.getBanKinh(), 2.5));

    // 6. Xoay quanh tam (Tam va R giu nguyen)
    ht.xoay(MathUtils::tuDoSangRadian(45.0));
    assert(ht.tinhTam() == Diem2D(4.0, 0.0));

    std::cout << "  => HinhTron PASSED!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "       BAT DAU KIEM THU SHAPES          " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        testDoanThang();
        testHinhTron();

        std::cout << "========================================" << std::endl;
        std::cout << "    ALL TESTS PASSED SUCCESSFULLY!      " << std::endl;
        std::cout << "========================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Loi Test: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}