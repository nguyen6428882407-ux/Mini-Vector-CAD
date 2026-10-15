#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

#include "core/Diem2D.h"
#include "core/MathUtils.h"
#include "core/MauSac.h"
#include "core/BienDoi2D.h"
#include "shapes/HinhHoc2D.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"
#include "shapes/TamGiac.h"
#include "shapes/DaGiac.h"

void testDoanThang() {
    std::cout << "[TEST] Kiem tra DoanThang..." << std::endl;
    
    Diem2D p1(0.0, 0.0);
    Diem2D p2(4.0, 0.0);
    DoanThang dt(p1, p2);

    // 1. Chu vi (Do dai doan thang) = 4
    assert(MathUtils::bangNhau(dt.tinhChuVi(), 4.0));

    // 2. Dien tich doan thang = 0
    assert(MathUtils::bangNhau(dt.tinhDienTich(), 0.0));

    // 3. Tam (Trung diem) = (2, 0)
    assert(dt.tam() == Diem2D(2.0, 0.0));

    // 4. Tinh tien (+2, +3) -> tam moi (4, 3)
    dt.transform(BienDoi2D::tinhTien(2.0, 3.0));
    assert(dt.tam() == Diem2D(4.0, 3.0));

    // 5. Scale x2 (do dai thanh 8)
    dt.transform(BienDoi2D::tiLeDeu(2.0, dt.tam()));
    assert(MathUtils::bangNhau(dt.tinhChuVi(), 8.0));

    // 6. Xoay 90 do quanh tam (Tam giu nguyen) - Goc tính bang Do
    dt.transform(BienDoi2D::xoay(90.0, dt.tam()));
    assert(dt.tam() == Diem2D(4.0, 3.0));
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
    assert(ht.tam() == Diem2D(1.0, 1.0));

    // 4. Tinh tien (+3, -1) -> tam moi (4, 0)
    ht.transform(BienDoi2D::tinhTien(3.0, -1.0));
    assert(ht.tam() == Diem2D(4.0, 0.0));

    // 5. Scale x0.5 -> R moi = 2.5
    ht.transform(BienDoi2D::tiLeDeu(0.5, ht.tam()));
    assert(MathUtils::bangNhau(ht.getBanKinh(), 2.5));

    // 6. Xoay quanh tam (Tam va R giu nguyen)
    ht.transform(BienDoi2D::xoay(45.0, ht.tam()));
    assert(ht.tam() == Diem2D(4.0, 0.0));

    std::cout << "  => HinhTron PASSED!" << std::endl;
}

// ============================================================================
// KIEM THU HinhChuNhat, TamGiac, DaGiac
// (Cac ham tro giup nam trong namespace an danh)
// ============================================================================
namespace {

// Lay chuoi ma ve(ostream&) in ra cua mot hinh.
std::string veRaChuoi(const HinhHoc2D& h) {
    std::ostringstream oss;
    h.ve(oss);
    return oss.str();
}

// s co chua chuoi con "con" khong?
bool chuaChuoi(const std::string& s, const std::string& con) {
    return s.find(con) != std::string::npos;
}

// Dem so lan ky tu c xuat hien trong s.
std::size_t demKyTu(const std::string& s, char c) {
    return static_cast<std::size_t>(std::count(s.begin(), s.end(), c));
}

} // namespace

void testHinhChuNhat() {
    std::cout << "[TEST] Kiem tra HinhChuNhat..." << std::endl;

    // 1. Ham tao (goc tren-trai, rong, cao): 4 dinh theo thu tu tren-trai, tren-phai, duoi-phai, duoi-trai
    HinhChuNhat hcn(Diem2D(1.0, 2.0), 4.0, 3.0);
    assert(hcn.getDinh().size() == 4);
    assert(hcn.getDinh()[0] == Diem2D(1.0, 2.0));
    assert(hcn.getDinh()[1] == Diem2D(5.0, 2.0));
    assert(hcn.getDinh()[2] == Diem2D(5.0, 5.0));
    assert(hcn.getDinh()[3] == Diem2D(1.0, 5.0));

    // 2. Chu vi = 2 * (4 + 3) = 14, dien tich = 4 * 3 = 12, tam = (3, 3.5)
    assert(MathUtils::bangNhau(hcn.tinhChuVi(), 14.0));
    assert(MathUtils::bangNhau(hcn.tinhDienTich(), 12.0));
    assert(hcn.tam() == Diem2D(3.0, 3.5));

    // 3. Ham tao (x, y, w, h) cho ket qua giong ham tao (goc, rong, cao)
    HinhChuNhat hcn2(1.0, 2.0, 4.0, 3.0);
    for (std::size_t i = 0; i < 4; ++i) {
        assert(hcn2.getDinh()[i] == hcn.getDinh()[i]);
    }
    assert(MathUtils::bangNhau(hcn2.tinhDienTich(), 12.0));

    // 4. Ham tao 4 dinh: hinh chu nhat nghieng
    HinhChuNhat nghieng(Diem2D(0.0, 0.0), Diem2D(3.0, 4.0), Diem2D(-5.0, 10.0), Diem2D(-8.0, 6.0));
    assert(MathUtils::bangNhau(nghieng.tinhDienTich(), 50.0));
    assert(MathUtils::bangNhau(nghieng.tinhChuVi(), 30.0));
    assert(nghieng.tam() == Diem2D(-2.5, 5.0));

    // 5. transform: tinh tien (+2, -1) -> dinh doi, dien tich/chu vi giu nguyen
    hcn.transform(BienDoi2D::tinhTien(2.0, -1.0));
    assert(hcn.getDinh()[0] == Diem2D(3.0, 1.0));
    assert(hcn.getDinh()[1] == Diem2D(7.0, 1.0));
    assert(hcn.getDinh()[2] == Diem2D(7.0, 4.0));
    assert(hcn.getDinh()[3] == Diem2D(3.0, 4.0));
    assert(hcn.tam() == Diem2D(5.0, 2.5));
    assert(MathUtils::bangNhau(hcn.tinhChuVi(), 14.0));
    assert(MathUtils::bangNhau(hcn.tinhDienTich(), 12.0));

    // 6. transform: xoay 90 do quanh tam cua chinh no
    HinhChuNhat xoay90(Diem2D(0.0, 0.0), 4.0, 2.0);
    xoay90.transform(BienDoi2D::xoay(90.0, xoay90.tam()));
    assert(xoay90.getDinh()[0] == Diem2D(3.0, -1.0));
    assert(xoay90.getDinh()[1] == Diem2D(3.0, 3.0));
    assert(xoay90.getDinh()[2] == Diem2D(1.0, 3.0));
    assert(xoay90.getDinh()[3] == Diem2D(1.0, -1.0));
    assert(xoay90.tam() == Diem2D(2.0, 1.0));
    assert(MathUtils::bangNhau(xoay90.tinhDienTich(), 8.0));
    assert(MathUtils::bangNhau(xoay90.tinhChuVi(), 12.0));

    // 7. transform: xoay goc bat ky (30 do)
    HinhChuNhat xoay30(Diem2D(0.0, 0.0), 4.0, 2.0);
    xoay30.transform(BienDoi2D::xoay(30.0, Diem2D(10.0, -5.0)));
    assert(MathUtils::bangNhau(xoay30.tinhDienTich(), 8.0));
    assert(MathUtils::bangNhau(xoay30.tinhChuVi(), 12.0));

    // 8. transform: scale deu x2 quanh goc toa do
    HinhChuNhat scale2(Diem2D(0.0, 0.0), 4.0, 2.0);
    scale2.transform(BienDoi2D::tiLeDeu(2.0, Diem2D(0.0, 0.0)));
    assert(scale2.getDinh()[2] == Diem2D(8.0, 4.0));
    assert(MathUtils::bangNhau(scale2.tinhDienTich(), 32.0));
    assert(MathUtils::bangNhau(scale2.tinhChuVi(), 24.0));

    // 9. transform: thu nho x0.5 quanh tam cua chinh no
    scale2.transform(BienDoi2D::tiLeDeu(0.5, scale2.tam()));
    assert(scale2.getDinh()[0] == Diem2D(2.0, 1.0));
    assert(scale2.getDinh()[1] == Diem2D(6.0, 1.0));
    assert(scale2.getDinh()[2] == Diem2D(6.0, 3.0));
    assert(scale2.getDinh()[3] == Diem2D(2.0, 3.0));
    assert(scale2.tam() == Diem2D(4.0, 2.0));
    assert(MathUtils::bangNhau(scale2.tinhDienTich(), 8.0));

    // 10. transform: he so ti le AM (doi xung tam)
    HinhChuNhat doiXung(Diem2D(0.0, 0.0), 4.0, 2.0);
    doiXung.transform(BienDoi2D::tiLeDeu(-1.0, doiXung.tam()));
    assert(doiXung.getDinh()[0] == Diem2D(4.0, 2.0));
    assert(doiXung.getDinh()[1] == Diem2D(0.0, 2.0));
    assert(doiXung.getDinh()[2] == Diem2D(0.0, 0.0));
    assert(doiXung.getDinh()[3] == Diem2D(4.0, 0.0));
    assert(doiXung.tam() == Diem2D(2.0, 1.0));
    assert(MathUtils::bangNhau(doiXung.tinhDienTich(), 8.0));
    assert(MathUtils::bangNhau(doiXung.tinhChuVi(), 12.0));

    // 11. transform: ghep phep bien doi
    HinhChuNhat ghep(Diem2D(0.0, 0.0), 4.0, 2.0);
    ghep.transform(BienDoi2D::tinhTien(10.0, 0.0) * BienDoi2D::xoay(90.0, Diem2D(0.0, 0.0)));
    assert(ghep.getDinh()[0] == Diem2D(10.0, 0.0));
    assert(ghep.getDinh()[1] == Diem2D(10.0, 4.0));
    assert(ghep.getDinh()[2] == Diem2D(8.0, 4.0));
    assert(ghep.getDinh()[3] == Diem2D(8.0, 0.0));
    assert(MathUtils::bangNhau(ghep.tinhDienTich(), 8.0));

    // 12. transform + nghichDao (phuc vu Undo)
    HinhChuNhat undo(Diem2D(1.0, 1.0), 6.0, 3.0);
    BienDoi2D bd = BienDoi2D::xoay(45.0, Diem2D(2.0, 2.0)) * BienDoi2D::tiLeDeu(1.5, Diem2D(0.0, 0.0));
    undo.transform(bd);
    undo.transform(bd.nghichDao());
    assert(undo.getDinh()[0] == Diem2D(1.0, 1.0));
    assert(undo.getDinh()[1] == Diem2D(7.0, 1.0));
    assert(undo.getDinh()[2] == Diem2D(7.0, 4.0));
    assert(undo.getDinh()[3] == Diem2D(1.0, 4.0));

    // 13. Ca bien: chieu rong = 0 (suy bien thanh doan thang)
    HinhChuNhat suyBien(Diem2D(0.0, 0.0), 0.0, 5.0);
    assert(MathUtils::bangNhau(suyBien.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(suyBien.tinhChuVi(), 10.0));

    // 14. Ca bien: rong = cao = 0 (suy bien thanh mot diem)
    HinhChuNhat diem(Diem2D(3.0, 3.0), 0.0, 0.0);
    assert(MathUtils::bangNhau(diem.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(diem.tinhChuVi(), 0.0));
    assert(diem.tam() == Diem2D(3.0, 3.0));

    // 15. Ca bien: chieu rong am
    HinhChuNhat rongAm(Diem2D(0.0, 0.0), -4.0, 3.0);
    assert(rongAm.getDinh()[1] == Diem2D(-4.0, 0.0));
    assert(rongAm.tam() == Diem2D(-2.0, 1.5));
    assert(MathUtils::bangNhau(rongAm.tinhDienTich(), 12.0));
    assert(MathUtils::bangNhau(rongAm.tinhChuVi(), 14.0));

    // 16. Ca bien: toa do rat lon
    HinhChuNhat lon(Diem2D(1e6, 1e6), 4.0, 3.0);
    assert(MathUtils::bangNhau(lon.tinhDienTich(), 12.0, 1e-6));
    assert(MathUtils::bangNhau(lon.tinhChuVi(), 14.0, 1e-6));

    // 17. ve()
    HinhChuNhat hinhVe(Diem2D(0.0, 0.0), 4.0, 2.0);
    const std::string dongVe = veRaChuoi(hinhVe);
    assert(dongVe == "HinhChuNhat #" + std::to_string(hinhVe.getId()) +
                     ": d1=(0, 0) d2=(4, 0) d3=(4, 2) d4=(0, 2)\n");
    assert(demKyTu(dongVe, '\n') == 1);
    assert(dongVe.back() == '\n');

    // 18. toSVG()
    HinhChuNhat hinhSvg(Diem2D(0.0, 0.0), 4.0, 2.0);
    hinhSvg.setMauVien(MauSac(255, 0, 0, 255));
    hinhSvg.setMauNen(MauSac(0, 0, 255, 0));
    hinhSvg.setDoDayNet(2.5);
    const std::string svg = hinhSvg.toSVG();
    assert(svg == "<polygon points=\"0,0 4,0 4,2 0,2\" stroke=\"#FF0000\" stroke-opacity=\"1\""
                  " stroke-width=\"2.5\" fill=\"#0000FF\" fill-opacity=\"0\" />");
    assert(chuaChuoi(svg, "<polygon"));
    assert(svg.back() != '\n');

    // 19. toSVG() sau khi tinh tien
    hinhSvg.transform(BienDoi2D::tinhTien(1.0, 1.0));
    assert(chuaChuoi(hinhSvg.toSVG(), "points=\"1,1 5,1 5,3 1,3\""));

    // 20. clone()
    HinhChuNhat goc(Diem2D(0.0, 0.0), 4.0, 2.0);
    goc.setMauVien(MauSac(255, 0, 0, 255));
    goc.setMauNen(MauSac(0, 0, 255, 0));
    goc.setDoDayNet(2.5);
    std::unique_ptr<HinhHoc2D> banSao = goc.clone();
    assert(banSao != nullptr);
    assert(banSao->getId() != goc.getId());
    HinhChuNhat* banSaoHcn = dynamic_cast<HinhChuNhat*>(banSao.get());
    assert(banSaoHcn != nullptr);
    assert(MathUtils::bangNhau(banSao->tinhDienTich(), goc.tinhDienTich()));
    assert(MathUtils::bangNhau(banSao->tinhChuVi(), goc.tinhChuVi()));
    assert(banSao->tam() == goc.tam());
    assert(banSao->toSVG() == goc.toSVG());

    // 21. clone() doc lap
    goc.transform(BienDoi2D::tinhTien(100.0, 100.0));
    assert(banSaoHcn->getDinh()[0] == Diem2D(0.0, 0.0));
    assert(goc.getDinh()[0] == Diem2D(100.0, 100.0));
    assert(banSao->toSVG() != goc.toSVG());
    banSao->transform(BienDoi2D::tinhTien(-1.0, -1.0));
    assert(goc.getDinh()[0] == Diem2D(100.0, 100.0));

    // 22. clone() cua ban sao
    std::unique_ptr<HinhHoc2D> banSao2 = banSao->clone();
    assert(banSao2->getId() != banSao->getId());
    assert(banSao2->getId() != goc.getId());

    std::cout << "  => HinhChuNhat PASSED!" << std::endl;
}

void testTamGiac() {
    std::cout << "[TEST] Kiem tra TamGiac..." << std::endl;

    // 1. Tam giac vuong 3-4-5
    TamGiac tg(Diem2D(0.0, 0.0), Diem2D(4.0, 0.0), Diem2D(0.0, 3.0));
    assert(tg.getDinh().size() == 3);
    assert(tg.getDinh()[0] == Diem2D(0.0, 0.0));
    assert(tg.getDinh()[1] == Diem2D(4.0, 0.0));
    assert(tg.getDinh()[2] == Diem2D(0.0, 3.0));
    assert(MathUtils::bangNhau(tg.tinhChuVi(), 12.0));
    assert(MathUtils::bangNhau(tg.tinhDienTich(), 6.0));

    // 2. tam() la tam HOP BAO (2, 1.5)
    assert(tg.tam() == Diem2D(2.0, 1.5));
    assert(!(tg.tam() == Diem2D(4.0 / 3.0, 1.0)));

    // 3. Ham tao 6 so thuc
    TamGiac tg2(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    for (std::size_t i = 0; i < 3; ++i) {
        assert(tg2.getDinh()[i] == tg.getDinh()[i]);
    }
    assert(MathUtils::bangNhau(tg2.tinhDienTich(), 6.0));

    // 4. Dien tich (Shoelace)
    TamGiac nguoc(Diem2D(0.0, 3.0), Diem2D(4.0, 0.0), Diem2D(0.0, 0.0));
    assert(MathUtils::bangNhau(nguoc.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(nguoc.tinhChuVi(), 12.0));

    // 5. Tam giac deu
    const double can3 = std::sqrt(3.0);
    TamGiac deu(Diem2D(0.0, 0.0), Diem2D(2.0, 0.0), Diem2D(1.0, can3));
    assert(MathUtils::bangNhau(deu.tinhChuVi(), 6.0));
    assert(MathUtils::bangNhau(deu.tinhDienTich(), can3));

    // 6. Ca bien: 3 dinh thang hang
    TamGiac thang(Diem2D(0.0, 0.0), Diem2D(1.0, 1.0), Diem2D(2.0, 2.0));
    assert(MathUtils::bangNhau(thang.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(thang.tinhChuVi(), 4.0 * std::sqrt(2.0)));
    assert(thang.tam() == Diem2D(1.0, 1.0));

    // 7. Ca bien: ba dinh trung nhau
    TamGiac trung(Diem2D(5.0, 5.0), Diem2D(5.0, 5.0), Diem2D(5.0, 5.0));
    assert(MathUtils::bangNhau(trung.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(trung.tinhChuVi(), 0.0));
    assert(trung.tam() == Diem2D(5.0, 5.0));

    // 8. Ca bien: toa do rat lon
    TamGiac lon(Diem2D(1e6, 1e6), Diem2D(1e6 + 4.0, 1e6), Diem2D(1e6, 1e6 + 3.0));
    assert(MathUtils::bangNhau(lon.tinhDienTich(), 6.0, 1e-6));
    assert(MathUtils::bangNhau(lon.tinhChuVi(), 12.0, 1e-6));

    // 9. Ca bien: toa do am
    TamGiac am(Diem2D(-4.0, -3.0), Diem2D(0.0, -3.0), Diem2D(-4.0, 0.0));
    assert(MathUtils::bangNhau(am.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(am.tinhChuVi(), 12.0));
    assert(am.tam() == Diem2D(-2.0, -1.5));

    // 10. transform: tinh tien (+1, +1)
    TamGiac tinhTien(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    tinhTien.transform(BienDoi2D::tinhTien(1.0, 1.0));
    assert(tinhTien.getDinh()[0] == Diem2D(1.0, 1.0));
    assert(tinhTien.getDinh()[1] == Diem2D(5.0, 1.0));
    assert(tinhTien.getDinh()[2] == Diem2D(1.0, 4.0));
    assert(tinhTien.tam() == Diem2D(3.0, 2.5));
    assert(MathUtils::bangNhau(tinhTien.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(tinhTien.tinhChuVi(), 12.0));

    // 11. transform: xoay 90 do
    TamGiac xoay90(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    xoay90.transform(BienDoi2D::xoay(90.0, Diem2D(0.0, 0.0)));
    assert(xoay90.getDinh()[0] == Diem2D(0.0, 0.0));
    assert(xoay90.getDinh()[1] == Diem2D(0.0, 4.0));
    assert(xoay90.getDinh()[2] == Diem2D(-3.0, 0.0));
    assert(MathUtils::bangNhau(xoay90.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(xoay90.tinhChuVi(), 12.0));

    // 12. transform: xoay 30 do
    TamGiac xoay30(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    xoay30.transform(BienDoi2D::xoay(30.0, Diem2D(7.0, -2.0)));
    assert(MathUtils::bangNhau(xoay30.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(xoay30.tinhChuVi(), 12.0));

    // 13. transform: scale deu x3
    TamGiac scale3(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    scale3.transform(BienDoi2D::tiLeDeu(3.0, Diem2D(0.0, 0.0)));
    assert(scale3.getDinh()[1] == Diem2D(12.0, 0.0));
    assert(scale3.getDinh()[2] == Diem2D(0.0, 9.0));
    assert(MathUtils::bangNhau(scale3.tinhDienTich(), 54.0));
    assert(MathUtils::bangNhau(scale3.tinhChuVi(), 36.0));

    // 14. transform: he so ti le AM -2
    TamGiac scaleAm(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    scaleAm.transform(BienDoi2D::tiLeDeu(-2.0, Diem2D(0.0, 0.0)));
    assert(scaleAm.getDinh()[1] == Diem2D(-8.0, 0.0));
    assert(scaleAm.getDinh()[2] == Diem2D(0.0, -6.0));
    assert(MathUtils::bangNhau(scaleAm.tinhDienTich(), 24.0));
    assert(MathUtils::bangNhau(scaleAm.tinhChuVi(), 24.0));

    // 15. transform: tam giac suy bien
    thang.transform(BienDoi2D::xoay(33.0, Diem2D(1.0, 1.0)));
    assert(MathUtils::bangNhau(thang.tinhDienTich(), 0.0));

    // 16. transform + nghichDao
    TamGiac undo(Diem2D(1.0, 2.0), Diem2D(5.0, 2.0), Diem2D(1.0, 5.0));
    BienDoi2D bd = BienDoi2D::tinhTien(3.0, -4.0) * BienDoi2D::xoay(60.0, Diem2D(0.0, 0.0));
    undo.transform(bd);
    undo.transform(bd.nghichDao());
    assert(undo.getDinh()[0] == Diem2D(1.0, 2.0));
    assert(undo.getDinh()[1] == Diem2D(5.0, 2.0));
    assert(undo.getDinh()[2] == Diem2D(1.0, 5.0));

    // 17. ve()
    TamGiac hinhVe(Diem2D(0.0, 0.0), Diem2D(4.0, 0.0), Diem2D(0.0, 3.0));
    const std::string dongVe = veRaChuoi(hinhVe);
    assert(dongVe == "TamGiac #" + std::to_string(hinhVe.getId()) + ": d1=(0, 0) d2=(4, 0) d3=(0, 3)\n");
    assert(demKyTu(dongVe, '\n') == 1);
    assert(dongVe.back() == '\n');

    // 18. toSVG()
    TamGiac hinhSvg(0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    hinhSvg.setMauVien(MauSac(0, 255, 0, 255));
    hinhSvg.setMauNen(MauSac(255, 255, 0, 128));
    hinhSvg.setDoDayNet(1.0);
    const std::string svg = hinhSvg.toSVG();
    assert(svg == "<polygon points=\"0,0 4,0 0,3\" stroke=\"#00FF00\" stroke-opacity=\"1\""
                  " stroke-width=\"1\" fill=\"#FFFF00\" fill-opacity=\"0.501961\" />");
    assert(svg.back() != '\n');

    // 19. toSVG() sau khi tinh tien
    hinhSvg.transform(BienDoi2D::tinhTien(10.0, 20.0));
    assert(chuaChuoi(hinhSvg.toSVG(), "points=\"10,20 14,20 10,23\""));

    // 20. clone()
    TamGiac goc(Diem2D(0.0, 0.0), Diem2D(4.0, 0.0), Diem2D(0.0, 3.0));
    goc.setMauVien(MauSac(0, 255, 0, 255));
    goc.setMauNen(MauSac(255, 255, 0, 128));
    goc.setDoDayNet(3.0);
    std::unique_ptr<HinhHoc2D> banSao = goc.clone();
    assert(banSao != nullptr);
    assert(banSao->getId() != goc.getId());
    TamGiac* banSaoTg = dynamic_cast<TamGiac*>(banSao.get());
    assert(banSaoTg != nullptr);
    assert(MathUtils::bangNhau(banSao->tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(banSao->tinhChuVi(), 12.0));
    assert(banSao->tam() == goc.tam());
    assert(banSao->toSVG() == goc.toSVG());

    // 21. clone() doc lap
    goc.transform(BienDoi2D::tiLeDeu(2.0, Diem2D(0.0, 0.0)));
    assert(MathUtils::bangNhau(goc.tinhDienTich(), 24.0));
    assert(MathUtils::bangNhau(banSao->tinhDienTich(), 6.0));
    assert(banSaoTg->getDinh()[1] == Diem2D(4.0, 0.0));
    banSao->transform(BienDoi2D::tinhTien(1.0, 1.0));
    assert(goc.getDinh()[1] == Diem2D(8.0, 0.0));

    // 22. clone() cua ban sao
    std::unique_ptr<HinhHoc2D> banSao2 = banSao->clone();
    assert(banSao2->getId() != banSao->getId());
    assert(banSao2->getId() != goc.getId());

    std::cout << "  => TamGiac PASSED!" << std::endl;
}

void testDaGiac() {
    std::cout << "[TEST] Kiem tra DaGiac..." << std::endl;

    // 1. Hinh vuong 4x4
    std::vector<Diem2D> vuongDinh;
    vuongDinh.push_back(Diem2D(0.0, 0.0));
    vuongDinh.push_back(Diem2D(4.0, 0.0));
    vuongDinh.push_back(Diem2D(4.0, 4.0));
    vuongDinh.push_back(Diem2D(0.0, 4.0));
    DaGiac vuong(vuongDinh);
    assert(vuong.getDinh().size() == 4);
    assert(MathUtils::bangNhau(vuong.tinhChuVi(), 16.0));
    assert(MathUtils::bangNhau(vuong.tinhDienTich(), 16.0));
    assert(vuong.tam() == Diem2D(2.0, 2.0));

    // 2. Shoelace theo chieu KIM DONG HO
    std::vector<Diem2D> vuongNguoc;
    vuongNguoc.push_back(Diem2D(0.0, 4.0));
    vuongNguoc.push_back(Diem2D(4.0, 4.0));
    vuongNguoc.push_back(Diem2D(4.0, 0.0));
    vuongNguoc.push_back(Diem2D(0.0, 0.0));
    DaGiac vuongKimDongHo(vuongNguoc);
    assert(MathUtils::bangNhau(vuongKimDongHo.tinhDienTich(), 16.0));
    assert(MathUtils::bangNhau(vuongKimDongHo.tinhChuVi(), 16.0));

    // 3. Da giac lom (hinh chu L)
    std::vector<Diem2D> chuLDinh;
    chuLDinh.push_back(Diem2D(0.0, 0.0));
    chuLDinh.push_back(Diem2D(6.0, 0.0));
    chuLDinh.push_back(Diem2D(6.0, 2.0));
    chuLDinh.push_back(Diem2D(2.0, 2.0));
    chuLDinh.push_back(Diem2D(2.0, 6.0));
    chuLDinh.push_back(Diem2D(0.0, 6.0));
    DaGiac chuL(chuLDinh);
    assert(MathUtils::bangNhau(chuL.tinhDienTich(), 20.0));
    assert(MathUtils::bangNhau(chuL.tinhChuVi(), 24.0));
    assert(chuL.tam() == Diem2D(3.0, 3.0));
    assert(!(chuL.tam() == Diem2D(16.0 / 6.0, 16.0 / 6.0)));

    // 4. Luc giac deu ban kinh 2
    std::vector<Diem2D> lucGiacDinh;
    for (int k = 0; k < 6; ++k) {
        const double goc = MathUtils::tuDoSangRadian(60.0 * k);
        lucGiacDinh.push_back(Diem2D(2.0 * std::cos(goc), 2.0 * std::sin(goc)));
    }
    DaGiac lucGiac(lucGiacDinh);
    assert(MathUtils::bangNhau(lucGiac.tinhDienTich(), 6.0 * std::sqrt(3.0)));
    assert(MathUtils::bangNhau(lucGiac.tinhChuVi(), 12.0));
    assert(lucGiac.tam() == Diem2D(0.0, 0.0));

    // 5. DaGiac 3 dinh
    std::vector<Diem2D> tamGiacDinh;
    tamGiacDinh.push_back(Diem2D(0.0, 0.0));
    tamGiacDinh.push_back(Diem2D(4.0, 0.0));
    tamGiacDinh.push_back(Diem2D(0.0, 3.0));
    DaGiac dgTamGiac(tamGiacDinh);
    TamGiac tgDoiChung(Diem2D(0.0, 0.0), Diem2D(4.0, 0.0), Diem2D(0.0, 3.0));
    assert(MathUtils::bangNhau(dgTamGiac.tinhDienTich(), tgDoiChung.tinhDienTich()));
    assert(MathUtils::bangNhau(dgTamGiac.tinhChuVi(), tgDoiChung.tinhChuVi()));
    assert(dgTamGiac.tam() == tgDoiChung.tam());

    // 6. Ca bien: da giac rong (0 dinh)
    DaGiac rong((std::vector<Diem2D>()));
    assert(rong.getDinh().empty());
    assert(MathUtils::bangNhau(rong.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(rong.tinhChuVi(), 0.0));
    assert(rong.tam() == Diem2D(0.0, 0.0));

    // 7. Ca bien: 1 dinh
    std::vector<Diem2D> motDinh;
    motDinh.push_back(Diem2D(2.0, 3.0));
    DaGiac mot(motDinh);
    assert(MathUtils::bangNhau(mot.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(mot.tinhChuVi(), 0.0));
    assert(mot.tam() == Diem2D(2.0, 3.0));

    // 8. Ca bien: 2 dinh
    std::vector<Diem2D> haiDinh;
    haiDinh.push_back(Diem2D(0.0, 0.0));
    haiDinh.push_back(Diem2D(3.0, 4.0));
    DaGiac hai(haiDinh);
    assert(MathUtils::bangNhau(hai.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(hai.tinhChuVi(), 10.0));
    assert(hai.tam() == Diem2D(1.5, 2.0));

    // 9. themDinh()
    DaGiac dan((std::vector<Diem2D>()));
    dan.themDinh(Diem2D(0.0, 0.0));
    dan.themDinh(Diem2D(4.0, 0.0));
    assert(dan.getDinh().size() == 2);
    assert(MathUtils::bangNhau(dan.tinhDienTich(), 0.0));
    dan.themDinh(Diem2D(0.0, 3.0));
    assert(dan.getDinh().size() == 3);
    assert(dan.getDinh()[2] == Diem2D(0.0, 3.0));
    assert(MathUtils::bangNhau(dan.tinhDienTich(), 6.0));
    assert(MathUtils::bangNhau(dan.tinhChuVi(), 12.0));

    // 10. Ca bien: cac dinh thang hang
    std::vector<Diem2D> thangHang;
    thangHang.push_back(Diem2D(0.0, 0.0));
    thangHang.push_back(Diem2D(1.0, 1.0));
    thangHang.push_back(Diem2D(2.0, 2.0));
    thangHang.push_back(Diem2D(3.0, 3.0));
    DaGiac thang(thangHang);
    assert(MathUtils::bangNhau(thang.tinhDienTich(), 0.0));

    // 11. Ca bien: da giac tu cat
    std::vector<Diem2D> noBom;
    noBom.push_back(Diem2D(0.0, 0.0));
    noBom.push_back(Diem2D(2.0, 2.0));
    noBom.push_back(Diem2D(2.0, 0.0));
    noBom.push_back(Diem2D(0.0, 2.0));
    DaGiac tuCat(noBom);
    assert(MathUtils::bangNhau(tuCat.tinhDienTich(), 0.0));
    assert(MathUtils::bangNhau(tuCat.tinhChuVi(), 4.0 + 2.0 * std::sqrt(8.0)));

    // 12. Ca bien: toa do rat lon
    std::vector<Diem2D> lonDinh;
    lonDinh.push_back(Diem2D(1e6, 1e6));
    lonDinh.push_back(Diem2D(1e6 + 4.0, 1e6));
    lonDinh.push_back(Diem2D(1e6 + 4.0, 1e6 + 4.0));
    lonDinh.push_back(Diem2D(1e6, 1e6 + 4.0));
    DaGiac lon(lonDinh);
    assert(MathUtils::bangNhau(lon.tinhDienTich(), 16.0, 1e-6));
    assert(MathUtils::bangNhau(lon.tinhChuVi(), 16.0, 1e-6));

    // 13. transform: tinh tien (+3, -2)
    DaGiac tinhTien(vuongDinh);
    tinhTien.transform(BienDoi2D::tinhTien(3.0, -2.0));
    assert(tinhTien.getDinh()[0] == Diem2D(3.0, -2.0));
    assert(tinhTien.getDinh()[1] == Diem2D(7.0, -2.0));
    assert(tinhTien.getDinh()[2] == Diem2D(7.0, 2.0));
    assert(tinhTien.getDinh()[3] == Diem2D(3.0, 2.0));
    assert(tinhTien.tam() == Diem2D(5.0, 0.0));
    assert(MathUtils::bangNhau(tinhTien.tinhDienTich(), 16.0));
    assert(MathUtils::bangNhau(tinhTien.tinhChuVi(), 16.0));

    // 14. transform: xoay 90 do quanh tam cua chinh no
    DaGiac xoay90(vuongDinh);
    xoay90.transform(BienDoi2D::xoay(90.0, xoay90.tam()));
    assert(xoay90.getDinh()[0] == Diem2D(4.0, 0.0));
    assert(xoay90.getDinh()[1] == Diem2D(4.0, 4.0));
    assert(xoay90.getDinh()[2] == Diem2D(0.0, 4.0));
    assert(xoay90.getDinh()[3] == Diem2D(0.0, 0.0));
    assert(xoay90.tam() == Diem2D(2.0, 2.0));

    // 15. transform: xoay 90 do hinh chu L quanh tam hop bao (3, 3)
    DaGiac chuLXoay(chuLDinh);
    chuLXoay.transform(BienDoi2D::xoay(90.0, chuLXoay.tam()));
    assert(MathUtils::bangNhau(chuLXoay.tinhDienTich(), 20.0));
    assert(MathUtils::bangNhau(chuLXoay.tinhChuVi(), 24.0));
    assert(chuLXoay.tam() == Diem2D(3.0, 3.0));

    // 16. transform: xoay goc bat ky (37 do)
    DaGiac xoay37(chuLDinh);
    xoay37.transform(BienDoi2D::xoay(37.0, Diem2D(-1.0, 4.0)));
    assert(MathUtils::bangNhau(xoay37.tinhDienTich(), 20.0));
    assert(MathUtils::bangNhau(xoay37.tinhChuVi(), 24.0));

    // 17. transform: scale deu x2
    DaGiac scale2(vuongDinh);
    scale2.transform(BienDoi2D::tiLeDeu(2.0, Diem2D(0.0, 0.0)));
    assert(scale2.getDinh()[2] == Diem2D(8.0, 8.0));
    assert(MathUtils::bangNhau(scale2.tinhDienTich(), 64.0));
    assert(MathUtils::bangNhau(scale2.tinhChuVi(), 32.0));

    // 18. transform: thu nho x0.5 quanh tam cua chinh no
    DaGiac thuNho(vuongDinh);
    thuNho.transform(BienDoi2D::tiLeDeu(0.5, thuNho.tam()));
    assert(thuNho.getDinh()[0] == Diem2D(1.0, 1.0));
    assert(thuNho.getDinh()[2] == Diem2D(3.0, 3.0));
    assert(thuNho.tam() == Diem2D(2.0, 2.0));
    assert(MathUtils::bangNhau(thuNho.tinhDienTich(), 4.0));

    // 19. transform: da giac rong
    rong.transform(BienDoi2D::tinhTien(5.0, 5.0));
    assert(rong.getDinh().empty());
    assert(MathUtils::bangNhau(rong.tinhDienTich(), 0.0));

    // 20. transform + nghichDao (Undo)
    DaGiac undo(chuLDinh);
    BienDoi2D bd = BienDoi2D::tinhTien(-3.0, 8.0) * BienDoi2D::xoay(75.0, Diem2D(1.0, 1.0)) *
                   BienDoi2D::tiLeDeu(2.5, Diem2D(0.0, 0.0));
    undo.transform(bd);
    undo.transform(bd.nghichDao());
    for (std::size_t i = 0; i < chuLDinh.size(); ++i) {
        assert(undo.getDinh()[i] == chuLDinh[i]);
    }

    // 21. ve()
    DaGiac hinhVe(vuongDinh);
    const std::string dongVe = veRaChuoi(hinhVe);
    assert(dongVe == "DaGiac #" + std::to_string(hinhVe.getId()) +
                     ": 4 dinh [(0, 0) (4, 0) (4, 4) (0, 4)]\n");
    assert(demKyTu(dongVe, '\n') == 1);
    assert(dongVe.back() == '\n');

    // 22. toSVG()
    DaGiac hinhSvg(vuongDinh);
    hinhSvg.setMauVien(MauSac(0, 0, 255, 255));
    hinhSvg.setMauNen(MauSac(255, 0, 0, 0));
    hinhSvg.setDoDayNet(4.0);
    const std::string svg = hinhSvg.toSVG();
    assert(svg == "<polygon points=\"0,0 4,0 4,4 0,4\" stroke=\"#0000FF\" stroke-opacity=\"1\""
                  " stroke-width=\"4\" fill=\"#FF0000\" fill-opacity=\"0\" />");
    assert(svg.back() != '\n');

    // 23. toSVG() dong
    DaGiac svgDong((std::vector<Diem2D>()));
    assert(chuaChuoi(svgDong.toSVG(), "points=\"\""));
    svgDong.themDinh(Diem2D(1.0, 2.0));
    svgDong.themDinh(Diem2D(3.0, 4.0));
    svgDong.themDinh(Diem2D(5.0, 0.0));
    assert(chuaChuoi(svgDong.toSVG(), "points=\"1,2 3,4 5,0\""));
    svgDong.transform(BienDoi2D::tinhTien(1.0, 1.0));
    assert(chuaChuoi(svgDong.toSVG(), "points=\"2,3 4,5 6,1\""));

    // 24. clone()
    DaGiac goc(chuLDinh);
    goc.setMauVien(MauSac(0, 0, 255, 255));
    goc.setMauNen(MauSac(255, 0, 0, 0));
    goc.setDoDayNet(4.0);
    std::unique_ptr<HinhHoc2D> banSao = goc.clone();
    assert(banSao != nullptr);
    assert(banSao->getId() != goc.getId());
    DaGiac* banSaoDg = dynamic_cast<DaGiac*>(banSao.get());
    assert(banSaoDg != nullptr);
    assert(banSaoDg->getDinh().size() == chuLDinh.size());
    assert(MathUtils::bangNhau(banSao->tinhDienTich(), 20.0));
    assert(MathUtils::bangNhau(banSao->tinhChuVi(), 24.0));
    assert(banSao->tam() == goc.tam());
    assert(banSao->toSVG() == goc.toSVG());

    // 25. clone() doc lap
    goc.themDinh(Diem2D(-5.0, -5.0));
    assert(goc.getDinh().size() == chuLDinh.size() + 1);
    assert(banSaoDg->getDinh().size() == chuLDinh.size());
    assert(MathUtils::bangNhau(banSao->tinhDienTich(), 20.0));
    goc.transform(BienDoi2D::tinhTien(50.0, 50.0));
    assert(banSaoDg->getDinh()[0] == Diem2D(0.0, 0.0));
    banSao->transform(BienDoi2D::tinhTien(-1.0, -1.0));
    assert(goc.getDinh()[0] == Diem2D(50.0, 50.0));

    // 26. clone() cua ban sao
    std::unique_ptr<HinhHoc2D> banSao2 = banSao->clone();
    assert(banSao2->getId() != banSao->getId());
    assert(banSao2->getId() != goc.getId());

    std::cout << "  => DaGiac PASSED!" << std::endl;
}

void testDaHinhChung() {
    std::cout << "[TEST] Kiem tra da hinh (con tro co so HinhHoc2D) cho HinhChuNhat, TamGiac, DaGiac..." << std::endl;

    std::vector<Diem2D> vuong2;
    vuong2.push_back(Diem2D(0.0, 0.0));
    vuong2.push_back(Diem2D(2.0, 0.0));
    vuong2.push_back(Diem2D(2.0, 2.0));
    vuong2.push_back(Diem2D(0.0, 2.0));

    std::vector<std::unique_ptr<HinhHoc2D>> ds;
    ds.push_back(std::make_unique<HinhChuNhat>(Diem2D(0.0, 0.0), 4.0, 3.0));
    ds.push_back(std::make_unique<TamGiac>(0.0, 0.0, 4.0, 0.0, 0.0, 3.0));
    ds.push_back(std::make_unique<DaGiac>(vuong2));

    // 1. Tong dien tich = 12 + 6 + 4 = 22, tong chu vi = 14 + 12 + 8 = 34
    double tongDienTich = 0.0;
    double tongChuVi = 0.0;
    for (const auto& h : ds) {
        tongDienTich += h->tinhDienTich();
        tongChuVi += h->tinhChuVi();
    }
    assert(MathUtils::bangNhau(tongDienTich, 22.0));
    assert(MathUtils::bangNhau(tongChuVi, 34.0));

    // 2. Moi hinh co id rieng
    assert(ds[0]->getId() != ds[1]->getId());
    assert(ds[1]->getId() != ds[2]->getId());
    assert(ds[0]->getId() != ds[2]->getId());

    // 3. clone() qua con tro co so
    for (const auto& h : ds) {
        std::unique_ptr<HinhHoc2D> b = h->clone();
        assert(b != nullptr);
        assert(b->getId() != h->getId());
        assert(MathUtils::bangNhau(b->tinhDienTich(), h->tinhDienTich()));
        assert(MathUtils::bangNhau(b->tinhChuVi(), h->tinhChuVi()));
        assert(b->toSVG() == h->toSVG());
    }
    assert(dynamic_cast<HinhChuNhat*>(ds[0]->clone().get()) != nullptr);
    assert(dynamic_cast<TamGiac*>(ds[1]->clone().get()) != nullptr);
    assert(dynamic_cast<DaGiac*>(ds[2]->clone().get()) != nullptr);

    // 4. transform() qua con tro co so
    for (auto& h : ds) {
        h->transform(BienDoi2D::tiLeDeu(2.0, Diem2D(0.0, 0.0)));
    }
    double tongSau = 0.0;
    for (const auto& h : ds) {
        tongSau += h->tinhDienTich();
    }
    assert(MathUtils::bangNhau(tongSau, 88.0));

    // 5. ve() qua con tro co so
    assert(chuaChuoi(veRaChuoi(*ds[0]), "HinhChuNhat #"));
    assert(chuaChuoi(veRaChuoi(*ds[1]), "TamGiac #"));
    assert(chuaChuoi(veRaChuoi(*ds[2]), "DaGiac #"));
    for (const auto& h : ds) {
        assert(demKyTu(veRaChuoi(*h), '\n') == 1);
    }

    std::cout << "  => Da hinh (HinhChuNhat, TamGiac, DaGiac) PASSED!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "       BAT DAU KIEM THU SHAPES          " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        testDoanThang();
        testHinhTron();
        testHinhChuNhat();
        testTamGiac();
        testDaGiac();
        testDaHinhChung();

        std::cout << "========================================" << std::endl;
        std::cout << "    ALL TESTS PASSED SUCCESSFULLY!      " << std::endl;
        std::cout << "========================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Loi Test: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}