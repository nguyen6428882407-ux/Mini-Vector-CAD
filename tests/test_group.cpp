// =============================================================================
// Test GroupShape: Composite Pattern, clone sâu, operator+ | Phụ trách: Đức Huy
// =============================================================================
#ifdef NDEBUG
#error "Test phải build với assert BẬT (không NDEBUG). Dùng CMake của dự án hoặc bỏ -DNDEBUG."
#endif

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "shapes/GroupShape.h"
#include "shapes/HinhTron.h"
#include "shapes/DoanThang.h"
#include "shapes/HinhChuNhat.h"
#include "core/BienDoi2D.h"
#include "core/Diem2D.h"
#include "core/MauSac.h"

// Lớp hình giả để kiểm thử độc lập mà không phụ thuộc vào lớp cụ thể khác
class HinhGiaTest : public HinhHoc2D {
private:
    Diem2D tam_;
    double dienTich_;
    double chuVi_;

public:
    int soLanTransform = 0;

    HinhGiaTest(const Diem2D& tam, double s, double p)
        : HinhHoc2D(), tam_(tam), dienTich_(s), chuVi_(p) {}

    HinhGiaTest(const HinhGiaTest& o)
        : HinhHoc2D(o), tam_(o.tam_), dienTich_(o.dienTich_), chuVi_(o.chuVi_), soLanTransform(o.soLanTransform) {}

    void ve(std::ostream& os) const override {
        os << "HinhGia #" << getId() << ": S=" << dienTich_ << "\n";
    }

    Diem2D tam() const override {
        return tam_;
    }

    double tinhDienTich() const override {
        return dienTich_;
    }

    double tinhChuVi() const override {
        return chuVi_;
    }

    void transform(const BienDoi2D& bd) override {
        tam_ = bd.apDung(tam_);
        soLanTransform++;
    }

    std::unique_ptr<HinhHoc2D> clone() const override {
        return std::make_unique<HinhGiaTest>(*this);
    }

    std::string toSVG() const override {
        return "<rect x=\"" + std::to_string(tam_.getX()) + "\" y=\"" + std::to_string(tam_.getY()) + "\"/>";
    }
};

int main() {
    std::cout << "=== BAT DAU KIEM THU CHINH THUC: test_group.cpp (Duc Huy) ===\n\n";

    // -------------------------------------------------------------------------
    // TEST 1: Nhóm rỗng
    // -------------------------------------------------------------------------
    std::cout << "[Test 1] Kiem tra nhom rong...\n";
    {
        GroupShape g;
        assert(g.soLuong() == 0);
        assert(g.soLuongCon() == 0);
        assert(g.getDanhSachCon().empty());
        assert(g.tinhDienTich() == 0.0);
        assert(g.tinhChuVi() == 0.0);
        Diem2D t = g.tam();
        assert(std::abs(t.getX()) < 1e-6 && std::abs(t.getY()) < 1e-6);

        // Định dạng toSVG của nhóm rỗng
        std::string svg = g.toSVG();
        assert(svg.find("<g id=\"group_" + std::to_string(g.getId()) + "\">") != std::string::npos);
        assert(svg.find("</g>") != std::string::npos);

        std::cout << "  -> PASS Test 1 (Nhom rong hop le)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 2: Thêm hình, chặn nullptr, chặn tự chứa chính nó
    // -------------------------------------------------------------------------
    std::cout << "[Test 2] Them hinh, chan nullptr va chan tu chua chinh no...\n";
    {
        GroupShape g;
        auto h1 = std::make_unique<HinhGiaTest>(Diem2D(10, 10), 50.0, 30.0);
        auto h2 = std::make_unique<HinhGiaTest>(Diem2D(20, 20), 70.0, 40.0);
        int id1 = h1->getId();
        int id2 = h2->getId();

        g.them(std::move(h1));
        g.themHinh(std::move(h2));
        assert(g.soLuong() == 2);
        assert(g.soLuongCon() == 2);
        assert(g.tinhDienTich() == 120.0);
        assert(g.tinhChuVi() == 70.0);

        // Chặn thêm nullptr
        g.them(nullptr);
        g.themHinh(nullptr);
        assert(g.soLuong() == 2);

        // Chặn tự thêm chính nó (self-containment)
        std::unique_ptr<HinhHoc2D> selfPtr(&g);
        g.them(std::move(selfPtr));
        selfPtr.release(); // Nhả lại để tránh double free
        assert(g.soLuong() == 2);

        // Kiểm tra truy cập hình con
        assert(g.hinhThu(0).getId() == id1);
        assert(g.hinhThu(1).getId() == id2);

        std::cout << "  -> PASS Test 2 (Them hinh an toan)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 3: Nhóm lồng nhóm (Composite Pattern đa cấp) & thụt lề vẽ
    // -------------------------------------------------------------------------
    std::cout << "[Test 3] Nhom long nhom (Composite) & thut le ve...\n";
    {
        auto gCha = std::make_unique<GroupShape>();
        auto gCon = std::make_unique<GroupShape>();

        auto h1 = std::make_unique<HinhGiaTest>(Diem2D(0, 0), 20.0, 10.0);
        auto h2 = std::make_unique<HinhGiaTest>(Diem2D(1, 1), 30.0, 15.0);
        auto h3 = std::make_unique<HinhGiaTest>(Diem2D(2, 2), 40.0, 20.0);

        gCon->them(std::move(h1));
        gCon->them(std::move(h2)); // gCon: S = 50, P = 25
        assert(gCon->tinhDienTich() == 50.0);

        gCha->them(std::move(gCon));
        gCha->them(std::move(h3)); // gCha: S = 50 + 40 = 90, P = 25 + 20 = 45
        assert(gCha->soLuong() == 2);
        assert(gCha->tinhDienTich() == 90.0);
        assert(gCha->tinhChuVi() == 45.0);

        // Kiểm tra thụt lề khi vẽ
        std::ostringstream oss;
        gCha->ve(oss);
        std::string output = oss.str();
        assert(output.find("GroupShape #" + std::to_string(gCha->getId())) != std::string::npos);
        // Có thụt lề 2 spaces
        assert(output.find("  GroupShape #") != std::string::npos || output.find("  HinhGia #") != std::string::npos);

        std::cout << "  -> PASS Test 3 (Composite da cap hoat dong dung)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 4: Rút hình (rutHinh)
    // -------------------------------------------------------------------------
    std::cout << "[Test 4] Kiem tra rut hinh khoi nhom...\n";
    {
        GroupShape g;
        auto h1 = std::make_unique<HinhGiaTest>(Diem2D(0, 0), 10.0, 5.0);
        auto h2 = std::make_unique<HinhGiaTest>(Diem2D(1, 1), 20.0, 10.0);
        auto h3 = std::make_unique<HinhGiaTest>(Diem2D(2, 2), 30.0, 15.0);
        int id1 = h1->getId();
        int id2 = h2->getId();
        int id3 = h3->getId();

        g.them(std::move(h1));
        g.them(std::move(h2));
        g.them(std::move(h3));
        assert(g.soLuong() == 3);

        // Rút h2 ở giữa
        auto rut = g.rutHinh(id2);
        assert(rut != nullptr);
        assert(rut->getId() == id2);
        assert(g.soLuong() == 2);

        // Thứ tự các hình còn lại giữ nguyên: h1 rồi đến h3
        assert(g.hinhThu(0).getId() == id1);
        assert(g.hinhThu(1).getId() == id3);

        // Rút ID không tồn tại -> trả nullptr
        assert(g.rutHinh(9999) == nullptr);
        assert(g.soLuong() == 2);

        std::cout << "  -> PASS Test 4 (Rut hinh dung quy cach)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 5: Clone sâu (Prototype Pattern)
    // -------------------------------------------------------------------------
    std::cout << "[Test 5] Clone sau: ID moi, style copy, doc lap ban goc...\n";
    {
        GroupShape g;
        g.setMauVien(MauSac(255, 0, 0, 255));
        g.setMauNen(MauSac(0, 255, 0, 128));
        g.setDoDayNet(3.5);

        auto h1 = std::make_unique<HinhGiaTest>(Diem2D(5, 5), 100.0, 40.0);
        int idGocCon = h1->getId();
        g.them(std::move(h1));

        int idGocNhom = g.getId();

        // Thực hiện clone()
        auto clonePtr = g.clone();
        assert(clonePtr != nullptr);
        auto* gClone = dynamic_cast<GroupShape*>(clonePtr.get());
        assert(gClone != nullptr);

        // 1. ID nhóm clone phải KHÁC ID nhóm gốc
        assert(gClone->getId() != idGocNhom);

        // 2. ID hình con clone phải KHÁC ID hình con gốc
        assert(gClone->soLuong() == 1);
        assert(gClone->hinhThu(0).getId() != idGocCon);

        // 3. Style viền, nền, độ dày nét được copy chính xác
        assert(gClone->getMauVien().toHex() == g.getMauVien().toHex());
        assert(gClone->getMauNen().toHex() == g.getMauNen().toHex());
        assert(gClone->getDoDayNet() == g.getDoDayNet());

        // 4. Biến đổi bản clone KHÔNG làm ảnh hưởng bản gốc
        gClone->transform(BienDoi2D::tinhTien(100, 100));
        assert(std::abs(g.hinhThu(0).tam().getX() - 5.0) < 1e-6);
        assert(std::abs(gClone->hinhThu(0).tam().getX() - 105.0) < 1e-6);

        std::cout << "  -> PASS Test 5 (Clone sau doc lap hoan toan)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 6: Toán tử operator+ gộp hình
    // -------------------------------------------------------------------------
    std::cout << "[Test 6] Toan tu operator+ gop 2 hinh thanh nhom moi...\n";
    {
        HinhGiaTest a(Diem2D(0, 0), 10.0, 5.0);
        HinhGiaTest b(Diem2D(10, 10), 20.0, 10.0);

        // Overload 1: operator+(const HinhHoc2D&, const HinhHoc2D&)
        auto gop1 = a + b;
        assert(gop1 != nullptr);
        assert(gop1->soLuong() == 2);
        assert(gop1->tinhDienTich() == 30.0);

        // Bản gốc a và b không bị thay đổi
        assert(a.tinhDienTich() == 10.0);
        assert(b.tinhDienTich() == 20.0);

        // Overload 2: operator+(unique_ptr, unique_ptr)
        auto pA = std::make_unique<HinhGiaTest>(Diem2D(1, 1), 15.0, 8.0);
        auto pB = std::make_unique<HinhGiaTest>(Diem2D(2, 2), 25.0, 12.0);
        auto gop2 = std::move(pA) + std::move(pB);
        assert(gop2 != nullptr);
        assert(gop2->soLuong() == 2);
        assert(gop2->tinhDienTich() == 40.0);

        std::cout << "  -> PASS Test 6 (operator+ ca 2 overload deu dung)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 7: transform lan truyen xuong moi hinh con
    // -------------------------------------------------------------------------
    std::cout << "[Test 7] Phep bien doi transform lan truyen xuong moi con...\n";
    {
        GroupShape cha;
        auto con1 = std::make_unique<HinhGiaTest>(Diem2D(0, 0), 10.0, 5.0);
        auto* ptr1 = con1.get();

        auto nhomCon = std::make_unique<GroupShape>();
        auto con2 = std::make_unique<HinhGiaTest>(Diem2D(10, 10), 10.0, 5.0);
        auto* ptr2 = con2.get();

        nhomCon->them(std::move(con2));
        cha.them(std::move(con1));
        cha.them(std::move(nhomCon));

        // Áp dụng phép tịnh tiến dx=5, dy=5
        cha.transform(BienDoi2D::tinhTien(5, 5));

        assert(ptr1->soLanTransform == 1);
        assert(std::abs(ptr1->tam().getX() - 5.0) < 1e-6);
        assert(std::abs(ptr1->tam().getY() - 5.0) < 1e-6);

        assert(ptr2->soLanTransform == 1);
        assert(std::abs(ptr2->tam().getX() - 15.0) < 1e-6);
        assert(std::abs(ptr2->tam().getY() - 15.0) < 1e-6);

        std::cout << "  -> PASS Test 7 (Transform lan truyen de quy dung)\n";
    }

    // -------------------------------------------------------------------------
    // TEST 8: Kiem tra timHinhCon, chuaHinh va xoaTatCa
    // -------------------------------------------------------------------------
    std::cout << "[Test 8] Kiem tra timHinhCon, chuaHinh va xoaTatCa...\n";
    {
        GroupShape cha;
        auto con1 = std::make_unique<HinhGiaTest>(Diem2D(0, 0), 10.0, 5.0);
        int id1 = con1->getId();

        auto nhomCon = std::make_unique<GroupShape>();
        auto con2 = std::make_unique<HinhGiaTest>(Diem2D(10, 10), 10.0, 5.0);
        int id2 = con2->getId();

        nhomCon->them(std::move(con2));
        cha.them(std::move(con1));
        cha.them(std::move(nhomCon));

        // Tim hinh con o cap 1
        assert(cha.chuaHinh(id1));
        assert(cha.timHinhCon(id1) != nullptr);

        // Tim hinh con nam sau trong nhom con (de quy)
        assert(cha.chuaHinh(id2));
        assert(cha.timHinhCon(id2) != nullptr);

        // Tim hinh khong ton tai
        assert(!cha.chuaHinh(999999));
        assert(cha.timHinhCon(999999) == nullptr);
        assert(cha.timHinhCon(-1) == nullptr);

        // Xoa tat ca
        cha.xoaTatCa();
        assert(cha.soLuong() == 0);
        assert(!cha.chuaHinh(id1));

        std::cout << "  -> PASS Test 8 (timHinhCon & chuaHinh de quy chinh xac)\n";
    }

    std::cout << "\n=== TAT CA 8/8 TEST CUA GroupShape (Duc Huy) DAU PASS XUAT SAC! ===\n";
    return 0;
}
