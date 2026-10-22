// ============================================================================
// Unit Test: LichSuNetVeStack (Kiem thu co che Stack Lich su net ve)
// Kiem tra:
//  - Co che Stack LIFO (push, pop, top, empty, size, clear)
//  - Thoi gian ve tu dong tao hop le
//  - Tai khoan ve luu tru chinh xac
//  - Chuc nang net ve va chi tiet
//  - Xuat file va nap lai file lich su
// ============================================================================
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include "canvas/LichSuNetVe.h"
#include "canvas/BangVeCanvas.h"
#include "commands/CommandManager.h"
#include "commands/AddShapeCommand.h"
#include "controller/MenuController.h"
#include "shapes/HinhTron.h"
#include "shapes/HinhChuNhat.h"

static int g_passCount = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "FAILED: " << #cond << " tai dong " << __LINE__ << std::endl; \
        std::exit(1); \
    } else { \
        ++g_passCount; \
    } \
} while(0)

void testCoCheStackCoBan() {
    LichSuNetVeStack st;
    TEST_ASSERT(st.empty());
    TEST_ASSERT(st.size() == 0);
    TEST_ASSERT(st.top() == nullptr);

    // Test push
    st.themNetVe("Ve Hinh Tron", "huy.nd", 1, 0, "tron 100 100 50");
    TEST_ASSERT(!st.empty());
    TEST_ASSERT(st.size() == 1);
    TEST_ASSERT(st.top() != nullptr);
    TEST_ASSERT(st.top()->chucNang == "Ve Hinh Tron");
    TEST_ASSERT(st.top()->taiKhoanVe == "huy.nd");
    TEST_ASSERT(st.top()->idHinh == 1);
    TEST_ASSERT(!st.top()->thoiGianVe.empty());

    // Push second item
    st.themNetVe("Ve Hinh Chu Nhat", "designer_02", 2, 0, "chunhat 10 20 200 100");
    TEST_ASSERT(st.size() == 2);
    TEST_ASSERT(st.top()->chucNang == "Ve Hinh Chu Nhat");
    TEST_ASSERT(st.top()->taiKhoanVe == "designer_02");

    // Push third item
    st.themNetVe("Ve Doan Thang", "admin", 3, 1, "doanthang 0 0 500 500");
    TEST_ASSERT(st.size() == 3);
    TEST_ASSERT(st.top()->chucNang == "Ve Doan Thang");
    TEST_ASSERT(st.top()->taiKhoanVe == "admin");

    // Test LIFO order with layDanhSachTuDinh
    auto ds = st.layDanhSachTuDinh();
    TEST_ASSERT(ds.size() == 3);
    TEST_ASSERT(ds[0].chucNang == "Ve Doan Thang");      // Top (Mới nhất)
    TEST_ASSERT(ds[1].chucNang == "Ve Hinh Chu Nhat");
    TEST_ASSERT(ds[2].chucNang == "Ve Hinh Tron");          // Bottom (Cũ nhất)

    // Test pop LIFO
    BanGhiLichSuVe outBg;
    bool ok1 = st.pop(outBg);
    TEST_ASSERT(ok1);
    TEST_ASSERT(outBg.chucNang == "Ve Doan Thang");
    TEST_ASSERT(outBg.taiKhoanVe == "admin");
    TEST_ASSERT(st.size() == 2);
    TEST_ASSERT(st.top()->chucNang == "Ve Hinh Chu Nhat");

    bool ok2 = st.pop(outBg);
    TEST_ASSERT(ok2);
    TEST_ASSERT(outBg.chucNang == "Ve Hinh Chu Nhat");
    TEST_ASSERT(outBg.taiKhoanVe == "designer_02");
    TEST_ASSERT(st.size() == 1);

    bool ok3 = st.pop(outBg);
    TEST_ASSERT(ok3);
    TEST_ASSERT(outBg.chucNang == "Ve Hinh Tron");
    TEST_ASSERT(outBg.taiKhoanVe == "huy.nd");
    TEST_ASSERT(st.empty());

    // Pop khi rong
    bool okRong = st.pop(outBg);
    TEST_ASSERT(!okRong);
    TEST_ASSERT(st.top() == nullptr);

    std::cout << "[PASSED] testCoCheStackCoBan\n";
}

void testThoiGianVaDinhDang() {
    LichSuNetVeStack st;
    st.themNetVe("Ve Net Tu Do", "artist", 5, 0, "brush 10 diem");

    const auto* top = st.top();
    TEST_ASSERT(top != nullptr);
    // Kiem tra dinh dang thoi gian YYYY-MM-DD HH:MM:SS (dai it nhat 19 ky tu)
    TEST_ASSERT(top->thoiGianVe.length() >= 19);
    TEST_ASSERT(top->thoiGianVe[4] == '-');
    TEST_ASSERT(top->thoiGianVe[7] == '-');
    TEST_ASSERT(top->thoiGianVe[10] == ' ');
    TEST_ASSERT(top->thoiGianVe[13] == ':');
    TEST_ASSERT(top->thoiGianVe[16] == ':');

    std::string moTa = top->moTa();
    TEST_ASSERT(moTa.find("artist") != std::string::npos);
    TEST_ASSERT(moTa.find("Ve Net Tu Do") != std::string::npos);

    std::cout << "[PASSED] testThoiGianVaDinhDang\n";
}

void testXuatVaNapFile() {
    LichSuNetVeStack st;
    st.themNetVe("Ve Tron", "userA", 10, 0, "tron 50 50 25");
    st.themNetVe("Ve Tam Giac", "userB", 11, 0, "tamgiac 0 0 10 0 5 10");
    st.themNetVe("Gom Nhom", "userA", 12, 0, "Group #10, #11");

    std::string testPath = "test_stroke_history_temp.txt";
    bool saveOk = st.xuatFile(testPath);
    TEST_ASSERT(saveOk);

    LichSuNetVeStack stNap;
    bool loadOk = stNap.napFile(testPath);
    TEST_ASSERT(loadOk);
    TEST_ASSERT(stNap.size() == 3);

    auto ds = stNap.layDanhSachTuDinh();
    TEST_ASSERT(ds[0].chucNang == "Gom Nhom");
    TEST_ASSERT(ds[0].taiKhoanVe == "userA");
    TEST_ASSERT(ds[1].chucNang == "Ve Tam Giac");
    TEST_ASSERT(ds[1].taiKhoanVe == "userB");
    TEST_ASSERT(ds[2].chucNang == "Ve Tron");
    TEST_ASSERT(ds[2].taiKhoanVe == "userA");

    // Xoa file tam
    std::remove(testPath.c_str());
    std::cout << "[PASSED] testXuatVaNapFile\n";
}

void testTichHopMenuController() {
    BangVeCanvas canvas(800.0, 600.0);
    canvas.themLayer("Layer 1");
    CommandManager cmdMgr;

    MenuController menu(canvas, cmdMgr, "nguoi_dung_test");
    TEST_ASSERT(menu.getTaiKhoanHienTai() == "nguoi_dung_test");
    TEST_ASSERT(menu.getLichSuNetVe().empty());

    // Thuc hien them hinh qua stack
    menu.getLichSuNetVe().themNetVe("Them hinh (tron)", menu.getTaiKhoanHienTai(), 1, 0, "tron 10 10 5");
    TEST_ASSERT(menu.getLichSuNetVe().size() == 1);
    TEST_ASSERT(menu.getLichSuNetVe().top()->taiKhoanVe == "nguoi_dung_test");

    // Doi tai khoan
    menu.setTaiKhoanHienTai("collaborator_99");
    TEST_ASSERT(menu.getTaiKhoanHienTai() == "collaborator_99");

    menu.getLichSuNetVe().themNetVe("Them hinh (chunhat)", menu.getTaiKhoanHienTai(), 2, 0, "chunhat 0 0 40 40");
    TEST_ASSERT(menu.getLichSuNetVe().size() == 2);
    TEST_ASSERT(menu.getLichSuNetVe().top()->taiKhoanVe == "collaborator_99");

    // Pop
    BanGhiLichSuVe popped;
    menu.getLichSuNetVe().pop(popped);
    TEST_ASSERT(popped.taiKhoanVe == "collaborator_99");
    TEST_ASSERT(popped.chucNang == "Them hinh (chunhat)");
    TEST_ASSERT(menu.getLichSuNetVe().size() == 1);

    std::cout << "[PASSED] testTichHopMenuController\n";
}

int main() {
    std::cout << "=== RUNNING TEST_HISTORY (LICH SU NET VE STACK) ===\n";
    testCoCheStackCoBan();
    testThoiGianVaDinhDang();
    testXuatVaNapFile();
    testTichHopMenuController();
    std::cout << "All tests passed successfully! (" << g_passCount << " assertions verified)\n";
    return 0;
}
