// Test Command + CommandManager (execute / undo / redo cho 4 lệnh, undo khi stack rỗng) | Task 15 | Phụ trách: Nhất Vũ
//
// Phạm vi: AddShapeCommand, RemoveShapeCommand, TransformCommand, GroupCommand (GOM / RA_NHOM) và CommandManager.
// Dùng "hình giả" Gia (có vị trí p_) để không phụ thuộc cài đặt của các hình thật; GroupShape dùng bản thật.
// Dùng macro CHECK riêng (in số dòng, đếm lỗi, không phụ thuộc assert/NDEBUG): có lỗi thì main trả về 1 -> ctest đỏ.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "canvas/BangVeCanvas.h"
#include "commands/AddShapeCommand.h"
#include "commands/CommandManager.h"
#include "commands/GroupCommand.h"
#include "commands/RemoveShapeCommand.h"
#include "commands/TransformCommand.h"
#include "core/BienDoi2D.h"
#include "shapes/GroupShape.h"

static int g_fail = 0, g_total = 0;
#define CHECK(cond) do { ++g_total; if (!(cond)) { ++g_fail; std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)
#define CHECK_THROWS(expr, Ex) do { ++g_total; bool ok = false; try { (void)(expr); } catch (const Ex&) { ok = true; } catch (...) {} \
    if (!ok) { ++g_fail; std::cout << "FAIL line " << __LINE__ << ": " #expr " không ném " #Ex "\n"; } } while (0)

// ------------------------------------------------------------------ hình giả + tiện ích
struct Gia : HinhHoc2D {
    static int song;                                   // số Gia đang sống (phát hiện rò rỉ / hủy hai lần)
    Diem2D p_;
    explicit Gia(Diem2D p = Diem2D(0, 0)) : p_(p) { ++song; }
    Gia(const Gia& o) : HinhHoc2D(o), p_(o.p_) { ++song; }
    ~Gia() override { --song; }
    void ve(std::ostream&) const override {}
    Diem2D tam() const override { return p_; }
    double tinhDienTich() const override { return 0; }
    double tinhChuVi() const override { return 0; }
    void transform(const BienDoi2D& bd) override { p_ = bd.apDung(p_); }
    std::unique_ptr<HinhHoc2D> clone() const override { return std::make_unique<Gia>(*this); }
    std::string toSVG() const override { return ""; }
};
int Gia::song = 0;

using Leaf = std::pair<int, Diem2D>;

static bool gan(double a, double b, double tol = 1e-6) { return std::fabs(a - b) <= tol * (1.0 + std::fabs(a) + std::fabs(b)); }
static std::unique_ptr<HinhHoc2D> moi(int& id, Diem2D p = Diem2D(0, 0)) { auto g = std::make_unique<Gia>(p); id = g->getId(); return g; }

// cấu trúc layer dạng chuỗi: "3,G7[4,5],6," (nhóm = G<id>[con...]); và danh sách hình lá theo thứ tự duyệt
static void cauTruc(const HinhHoc2D& h, std::string& s) {
    if (auto g = dynamic_cast<const GroupShape*>(&h)) {
        s += "G" + std::to_string(g->getId()) + "[";
        for (std::size_t i = 0; i < g->soLuong(); ++i) cauTruc(g->hinhThu(i), s);
        s += "]";
    } else {
        s += std::to_string(h.getId());
    }
    s += ",";
}
static void la(const HinhHoc2D& h, std::vector<Leaf>& out) {
    if (auto g = dynamic_cast<const GroupShape*>(&h)) { for (std::size_t i = 0; i < g->soLuong(); ++i) la(g->hinhThu(i), out); }
    else out.push_back({h.getId(), h.tam()});
}
static std::string cauTruc(const Layer& l) { std::string s; for (std::size_t i = 0; i < l.soHinh(); ++i) cauTruc(l.hinhThu(i), s); return s; }
static std::vector<Leaf> la(const Layer& l) { std::vector<Leaf> v; for (std::size_t i = 0; i < l.soHinh(); ++i) la(l.hinhThu(i), v); return v; }
static bool khopLa(const std::vector<Leaf>& a, const std::vector<Leaf>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].first != b[i].first || !gan(a[i].second.getX(), b[i].second.getX()) || !gan(a[i].second.getY(), b[i].second.getY())) return false;
    return true;
}
static Diem2D viTri(const Layer& l, int id) { return l.timTheoId(id)->tam(); }     // chỉ hình ở CẤP TRÊN CÙNG
static Diem2D viTriLa(const Layer& l, int id) {                                    // hình lá ở bất kỳ cấp nào (kể cả trong nhóm)
    for (const auto& x : la(l)) if (x.first == id) return x.second;
    return Diem2D(1e18, 1e18);                                                      // không thấy -> giá trị phi lý để CHECK đỏ
}

// lệnh giả có thể ném lỗi theo yêu cầu (để kiểm tra CommandManager truyền ngoại lệ và giữ đúng stack)
struct Nem : Command {
    int* dem; bool nemKhiExecute, nemKhiUndo; int soLanExecute = 0; bool nemKhiRedo;
    Nem(int* d, bool e = false, bool u = false, bool r = false) : dem(d), nemKhiExecute(e), nemKhiUndo(u), nemKhiRedo(r) {}
    void execute() override {
        if (nemKhiExecute) throw std::runtime_error("execute ném lỗi");
        if (soLanExecute >= 1 && nemKhiRedo) throw std::runtime_error("redo ném lỗi");
        ++soLanExecute; ++*dem;
    }
    void undo() override { if (nemKhiUndo) throw std::runtime_error("undo ném lỗi"); --*dem; }
};

int main() {
    // ============================================================ 1. CommandManager rỗng
    {
        CommandManager m;
        CHECK(!m.coTheUndo()); CHECK(!m.coTheRedo());
        CHECK(!m.undo()); CHECK(!m.redo());                      // undo/redo khi stack rỗng trả về false, không ném
        CHECK(!m.undo()); CHECK(!m.redo());                      // gọi lặp lại vẫn ổn định
        m.thucThi(nullptr);                                      // nullptr bị bỏ qua
        CHECK(!m.coTheUndo()); CHECK(!m.coTheRedo());
        m.xoaLichSu(); CHECK(!m.coTheUndo()); CHECK(!m.coTheRedo());
    }

    // ============================================================ 2. AddShapeCommand: execute -> undo -> redo
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        CommandManager m; int a;
        auto g = moi(a); HinhHoc2D* pa = g.get();
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, std::move(g)));
        CHECK(cv.layer(0).timTheoId(a) == pa); CHECK(m.coTheUndo()); CHECK(!m.coTheRedo());
        CHECK(m.undo());
        CHECK(cv.layer(0).soHinh() == 0); CHECK(!m.coTheUndo()); CHECK(m.coTheRedo()); CHECK(Gia::song == 1);   // hình do lệnh giữ
        CHECK(m.redo());
        CHECK(cv.layer(0).timTheoId(a) == pa);                    // CÙNG đối tượng, cùng id
        CHECK(m.coTheUndo()); CHECK(!m.coTheRedo());
        for (int i = 0; i < 20; ++i) { CHECK(m.undo()); CHECK(m.redo()); }   // nhiều chu kỳ
        CHECK(cv.layer(0).timTheoId(a) == pa); CHECK(cv.layer(0).soHinh() == 1);
        CHECK(!m.redo());                                         // redo khi redo stack rỗng
    }
    CHECK(Gia::song == 0);

    // ============================================================ 3. RemoveShapeCommand: execute -> undo -> redo (mọi vị trí)
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        Layer& ly = cv.layer(0);
        int v[5]; for (int i = 0; i < 5; ++i) ly.themHinh(moi(v[i], Diem2D(i, 0)));
        const std::string goc = cauTruc(ly);
        for (int k = 0; k < 5; ++k) {
            CommandManager m;
            HinhHoc2D* truoc = ly.timTheoId(v[k]);
            m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, v[k]));
            CHECK(ly.soHinh() == 4); CHECK(ly.timTheoId(v[k]) == nullptr);
            CHECK(m.undo());
            CHECK(cauTruc(ly) == goc); CHECK(ly.timTheoId(v[k]) == truoc);      // đúng vị trí cũ, cùng đối tượng
            CHECK(m.redo()); CHECK(ly.timTheoId(v[k]) == nullptr);              // redo xóa lại
            CHECK(m.undo()); CHECK(cauTruc(ly) == goc);
        }
        // xóa nhiều hình liên tiếp, hoàn tác từng bước ngược, rồi redo hết
        CommandManager m;
        m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, v[1]));
        m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, v[3]));
        m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, v[0]));
        const std::string daXoa = cauTruc(ly);
        CHECK(m.undo() && m.undo() && m.undo()); CHECK(cauTruc(ly) == goc);
        CHECK(!m.undo());                                                       // hết lệnh để undo
        CHECK(m.redo() && m.redo() && m.redo()); CHECK(cauTruc(ly) == daXoa);
        CHECK(!m.redo());
    }
    CHECK(Gia::song == 0);

    // ============================================================ 4. TransformCommand: execute -> undo -> redo
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        Layer& ly = cv.layer(0);
        int a, b; ly.themHinh(moi(a, Diem2D(10, 0))); ly.themHinh(moi(b, Diem2D(5, 5)));
        CommandManager m;
        m.thucThi(std::make_unique<TransformCommand>(cv, 0, a, BienDoi2D::tinhTien(3, -4)));
        CHECK(gan(viTri(ly, a).getX(), 13) && gan(viTri(ly, a).getY(), -4));
        CHECK(gan(viTri(ly, b).getX(), 5) && gan(viTri(ly, b).getY(), 5));      // hình khác không bị ảnh hưởng
        m.thucThi(std::make_unique<TransformCommand>(cv, 0, a, BienDoi2D::xoay(90, Diem2D(0, 0))));
        m.thucThi(std::make_unique<TransformCommand>(cv, 0, a, BienDoi2D::tiLeDeu(2, Diem2D(1, 1))));
        const Diem2D sau = viTri(ly, a);
        CHECK(m.undo() && m.undo() && m.undo());
        CHECK(gan(viTri(ly, a).getX(), 10) && gan(viTri(ly, a).getY(), 0));     // về đúng vị trí gốc (đúng thứ tự ngược)
        CHECK(m.redo() && m.redo() && m.redo());
        CHECK(gan(viTri(ly, a).getX(), sau.getX()) && gan(viTri(ly, a).getY(), sau.getY()));
        for (int i = 0; i < 500; ++i) { m.undo(); m.redo(); }                   // sai số tích lũy không đáng kể
        CHECK(gan(viTri(ly, a).getX(), sau.getX(), 1e-9) && gan(viTri(ly, a).getY(), sau.getY(), 1e-9));
        // phép biến đổi không khả nghịch: lệnh KHÔNG được tạo (constructor ném), lịch sử không đổi
        const bool truocUndo = m.coTheUndo();
        const BienDoi2D suyBien = BienDoi2D::tiLeDeu(1e-6, Diem2D(0, 0)) * BienDoi2D::tiLeDeu(1e-6, Diem2D(0, 0));
        CHECK_THROWS(TransformCommand(cv, 0, a, suyBien), std::domain_error);
        CHECK(m.coTheUndo() == truocUndo);
    }
    CHECK(Gia::song == 0);

    // ============================================================ 5. GroupCommand: GOM và RA_NHOM (execute -> undo -> redo)
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        Layer& ly = cv.layer(0);
        int x[5]; for (int i = 0; i < 5; ++i) ly.themHinh(moi(x[i], Diem2D(i, i)));
        const std::string goc = cauTruc(ly);
        CommandManager m;
        // GOM {x4, x1, x2}: danh sách id KHÔNG cần theo thứ tự; nhóm chèn tại vị trí nhỏ nhất (vị trí của x1)
        m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::GOM, std::vector<int>{x[4], x[1], x[2]}));
        CHECK(ly.soHinh() == 3);
        const int idNhom = ly.hinhThu(1).getId();
        auto* g = dynamic_cast<GroupShape*>(ly.timTheoId(idNhom));
        CHECK(g != nullptr);
        CHECK(g && g->soLuong() == 3);
        const std::string saugom = cauTruc(ly);
        CHECK(saugom == std::to_string(x[0]) + ",G" + std::to_string(idNhom) + "[" + std::to_string(x[1]) + "," + std::to_string(x[2]) + "," + std::to_string(x[4]) + ",],"
                      + std::to_string(x[3]) + ",");
        CHECK(m.undo()); CHECK(cauTruc(ly) == goc);                              // hoàn tác trả đúng thứ tự gốc
        CHECK(ly.timTheoId(idNhom) == nullptr);                                  // nhóm không còn trong layer
        CHECK(m.redo());
        CHECK(cauTruc(ly) == saugom);                                            // redo: CÙNG id nhóm, cùng cấu trúc
        CHECK(ly.timTheoId(idNhom) != nullptr);
        for (int i = 0; i < 10; ++i) { CHECK(m.undo()); CHECK(cauTruc(ly) == goc); CHECK(m.redo()); CHECK(cauTruc(ly) == saugom); }
        // RA_NHOM: rã nhóm vừa tạo
        m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::RA_NHOM, std::vector<int>{idNhom}));
        CHECK(ly.timTheoId(idNhom) == nullptr);
        CHECK(cauTruc(ly) == std::to_string(x[0]) + "," + std::to_string(x[1]) + "," + std::to_string(x[2]) + "," + std::to_string(x[4]) + "," + std::to_string(x[3]) + ",");
        const std::string saura = cauTruc(ly);
        CHECK(m.undo()); CHECK(cauTruc(ly) == saugom); CHECK(ly.timTheoId(idNhom) != nullptr);   // gom lại, cùng id nhóm
        CHECK(m.redo()); CHECK(cauTruc(ly) == saura);                                              // rã lại
        CHECK(m.undo()); CHECK(m.undo()); CHECK(cauTruc(ly) == goc);                               // undo RA_NHOM rồi undo GOM
        CHECK(m.redo()); CHECK(m.redo()); CHECK(cauTruc(ly) == saura);
        // nhóm lồng nhóm: gom {nhóm, x0}; undo/redo
        CHECK(m.undo());                                                                           // quay về trạng thái đang là nhóm
        const int idN = idNhom;
        m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::GOM, std::vector<int>{x[0], idN}));
        const int idNgoai = ly.hinhThu(0).getId();
        CHECK(dynamic_cast<GroupShape*>(ly.timTheoId(idNgoai)) != nullptr);
        const std::string long_ = cauTruc(ly);
        CHECK(m.undo()); CHECK(m.redo()); CHECK(cauTruc(ly) == long_); CHECK(ly.hinhThu(0).getId() == idNgoai);
    }
    CHECK(Gia::song == 0);

    // ============================================================ 6. HỒI QUY: id nhóm bền qua undo/redo (kịch bản trong sườn)
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        Layer& ly = cv.layer(0);
        CommandManager m; int A, B;
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(A, Diem2D(1, 1))));
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(B, Diem2D(3, 1))));
        m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::GOM, std::vector<int>{A, B}));
        const int idNhom = ly.hinhThu(0).getId();
        m.thucThi(std::make_unique<TransformCommand>(cv, 0, idNhom, BienDoi2D::tinhTien(10, 20)));     // biến đổi CẢ NHÓM
        CHECK(gan(viTriLa(ly, A).getX(), 11) && gan(viTriLa(ly, A).getY(), 21));
        CHECK(gan(viTriLa(ly, B).getX(), 13) && gan(viTriLa(ly, B).getY(), 21));
        CHECK(m.undo());                                                                                // hủy Transform
        CHECK(gan(viTriLa(ly, A).getX(), 1) && gan(viTriLa(ly, B).getX(), 3));
        CHECK(m.undo());                                                                                // hủy Group
        CHECK(ly.timTheoId(idNhom) == nullptr); CHECK(ly.soHinh() == 2);
        CHECK(m.redo());                                                                                // gom lại: cùng id nhóm
        CHECK(ly.timTheoId(idNhom) != nullptr);
        CHECK(m.redo());                                                                                // Transform vẫn tìm thấy nhóm theo id cũ
        CHECK(gan(viTriLa(ly, A).getX(), 11) && gan(viTriLa(ly, A).getY(), 21));
        CHECK(gan(viTriLa(ly, B).getX(), 13) && gan(viTriLa(ly, B).getY(), 21));
        CHECK(ly.hinhThu(0).getId() == idNhom);
        // undo toàn bộ về rỗng, redo toàn bộ trở lại
        int n = 0; while (m.undo()) ++n;
        CHECK(n == 4); CHECK(ly.soHinh() == 0); CHECK(!m.undo());
        n = 0; while (m.redo()) ++n;
        CHECK(n == 4); CHECK(ly.hinhThu(0).getId() == idNhom); CHECK(!m.redo());
    }
    CHECK(Gia::song == 0);

    // ============================================================ 7. Lệnh mới thì redo stack bị xóa
    {
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        CommandManager m; int a, b, c;
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(a)));
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(b)));
        CHECK(m.undo()); CHECK(m.coTheRedo());
        m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(c)));                  // lệnh mới
        CHECK(!m.coTheRedo()); CHECK(!m.redo());                                       // redo bị xóa
        CHECK(cv.timHinh(b) == nullptr);                                               // b không được "sống lại"
        CHECK(Gia::song == 2);                                                         // hình của lệnh bị xóa (b) đã được giải phóng đúng 1 lần
        CHECK(m.undo()); CHECK(m.undo()); CHECK(!m.undo());                            // chỉ còn 2 lệnh (a, c)
        m.xoaLichSu();
        CHECK(!m.coTheUndo()); CHECK(!m.coTheRedo()); CHECK(!m.undo()); CHECK(!m.redo());
    }
    CHECK(Gia::song == 0);

    // ============================================================ 8. Ngoại lệ: truyền ra ngoài, giữ đúng stack, canvas nguyên vẹn
    {
        CommandManager m; int dem = 0;
        // (a) execute ném: hai stack không đổi, lệnh bị hủy
        m.thucThi(std::make_unique<Nem>(&dem)); CHECK(dem == 1);
        CHECK(m.undo()); CHECK(dem == 0); CHECK(m.coTheRedo());                       // redo stack đang có 1 lệnh
        CHECK_THROWS(m.thucThi(std::make_unique<Nem>(&dem, true)), std::runtime_error);
        CHECK(!m.coTheUndo()); CHECK(m.coTheRedo()); CHECK(dem == 0);                 // KHÔNG bị xóa redo stack, không push undo
        // (b) undo ném: lệnh vẫn nằm trong undoStack_, redoStack_ không đổi
        CommandManager m2; int d2 = 0; bool nemUndo = false;
        struct NemUndoCoKiem : Command {
            int* d; bool* co; NemUndoCoKiem(int* dd, bool* c) : d(dd), co(c) {}
            void execute() override { ++*d; }
            void undo() override { if (*co) throw std::runtime_error("undo ném lỗi"); --*d; }
        };
        m2.thucThi(std::make_unique<NemUndoCoKiem>(&d2, &nemUndo)); CHECK(d2 == 1);
        nemUndo = true;
        CHECK_THROWS(m2.undo(), std::runtime_error);
        CHECK(m2.coTheUndo()); CHECK(!m2.coTheRedo()); CHECK(d2 == 1);                // lệnh còn trong undoStack_, trạng thái nguyên
        nemUndo = false;
        CHECK(m2.undo()); CHECK(d2 == 0); CHECK(m2.coTheRedo());                      // sau khi hết lỗi, undo chạy bình thường
        // (c) redo ném: lệnh vẫn nằm trong redoStack_
        CommandManager m3; int d3 = 0;
        m3.thucThi(std::make_unique<Nem>(&d3, false, false, true));                   // lần execute thứ 2 (redo) sẽ ném
        CHECK(m3.undo()); CHECK(d3 == 0);
        CHECK_THROWS(m3.redo(), std::runtime_error);
        CHECK(m3.coTheRedo()); CHECK(!m3.coTheUndo()); CHECK(d3 == 0);                // lệnh vẫn ở redoStack_
    }
    {
        // ngoại lệ thật từ lệnh thật: id không tồn tại -> out_of_range, canvas và lịch sử không đổi
        BangVeCanvas cv(100, 100); cv.themLayer("A");
        CommandManager m; int a; m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(a)));
        CHECK_THROWS(m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, 987654)), std::out_of_range);
        CHECK_THROWS(m.thucThi(std::make_unique<TransformCommand>(cv, 0, 987654, BienDoi2D::tinhTien(1, 1))), std::out_of_range);
        CHECK_THROWS(m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::GOM, std::vector<int>{a, 987654})), std::out_of_range);
        int tam; CHECK_THROWS(m.thucThi(std::make_unique<AddShapeCommand>(cv, 9, moi(tam))), std::out_of_range);   // layer sai
        CHECK(cv.layer(0).soHinh() == 1);                                              // GOM thất bại không làm mất hình a
        CHECK(m.undo()); CHECK(!m.undo());                                             // lịch sử chỉ có đúng lệnh Add ban đầu
    }
    CHECK(Gia::song == 0);

    // ============================================================ 9. Vòng đời: lịch sử hủy SAU canvas
    {
        auto cv = std::make_unique<BangVeCanvas>(100, 100); cv->themLayer("A");
        auto m = std::make_unique<CommandManager>(); int a, b, c;
        m->thucThi(std::make_unique<AddShapeCommand>(*cv, 0, moi(a)));
        m->thucThi(std::make_unique<AddShapeCommand>(*cv, 0, moi(b)));
        m->thucThi(std::make_unique<AddShapeCommand>(*cv, 0, moi(c)));
        m->thucThi(std::make_unique<GroupCommand>(*cv, 0, GroupCommand::Loai::GOM, std::vector<int>{a, b}));
        m->undo(); m->undo();                                                          // một số lệnh đang giữ hình/vỏ nhóm
        cv.reset();                                                                    // canvas chết trước
        m.reset();                                                                     // lệnh hủy không được dùng canvas_
    }
    CHECK(Gia::song == 0);

    // ============================================================ 10. NGẪU NHIÊN: 4 lệnh + undo/redo so với mô hình tham chiếu
    {
        struct Node { int id; bool group; Diem2D p; std::vector<Node> kids; };
        std::function<void(Node&, const BienDoi2D&)> apDung;
        BangVeCanvas cv(500, 500); cv.themLayer("A"); cv.themLayer("B");
        int sentinel; cv.layer(1).themHinh(moi(sentinel, Diem2D(42, 42)));            // layer 1 không bao giờ đổi
        using Model = std::vector<Node>;
        Model cur; std::vector<Model> undoS, redoS;
        CommandManager m;
        std::mt19937 rng(20261015);
        std::uniform_real_distribution<double> U(-50, 50), K(0.7, 1.4);
        std::function<void(const Node&, std::string&)> mc = [&](const Node& n, std::string& s) {
            if (n.group) { s += "G" + std::to_string(n.id) + "["; for (const auto& k : n.kids) mc(k, s); s += "]"; } else s += std::to_string(n.id);
            s += ",";
        };
        std::function<void(const Node&, std::vector<Leaf>&)> ml = [&](const Node& n, std::vector<Leaf>& o) {
            if (n.group) { for (const auto& k : n.kids) ml(k, o); } else o.push_back({n.id, n.p});
        };
        apDung = [&](Node& n, const BienDoi2D& bd) { if (n.group) { for (auto& k : n.kids) apDung(k, bd); } else n.p = bd.apDung(n.p); };
        auto khop = [&]() {
            std::string sm; std::vector<Leaf> lm;
            for (const auto& n : cur) { mc(n, sm); ml(n, lm); }
            if (sm != cauTruc(cv.layer(0)) || !khopLa(lm, la(cv.layer(0)))) return false;
            return cv.layer(1).soHinh() == 1 && gan(viTri(cv.layer(1), sentinel).getX(), 42);
        };
        int nAdd = 0, nRem = 0, nTr = 0, nGom = 0, nRa = 0, nUn = 0, nRe = 0; bool ok = true;
        for (int step = 0; step < 20000 && ok; ++step) {
            const int op = static_cast<int>(rng() % 20);
            try {
                if (op <= 2) {                                                         // Add
                    int id; Diem2D p(U(rng), U(rng));
                    m.thucThi(std::make_unique<AddShapeCommand>(cv, 0, moi(id, p)));
                    undoS.push_back(cur); redoS.clear(); cur.push_back({id, false, p, {}}); ++nAdd;
                } else if (op <= 4 && !cur.empty()) {                                  // Remove
                    const std::size_t k = rng() % cur.size();
                    m.thucThi(std::make_unique<RemoveShapeCommand>(cv, 0, cur[k].id));
                    undoS.push_back(cur); redoS.clear(); cur.erase(cur.begin() + static_cast<std::ptrdiff_t>(k)); ++nRem;
                } else if (op <= 7 && !cur.empty()) {                                  // Transform (hình lá hoặc cả nhóm)
                    const std::size_t k = rng() % cur.size(); const int t = static_cast<int>(rng() % 3);
                    const BienDoi2D bd = t == 0 ? BienDoi2D::tinhTien(U(rng), U(rng))
                                       : t == 1 ? BienDoi2D::xoay(U(rng) * 7, Diem2D(U(rng), U(rng)))
                                                : BienDoi2D::tiLeDeu(K(rng) * (rng() % 2 ? 1 : -1), Diem2D(U(rng), U(rng)));
                    m.thucThi(std::make_unique<TransformCommand>(cv, 0, cur[k].id, bd));
                    undoS.push_back(cur); redoS.clear(); apDung(cur[k], bd); ++nTr;
                } else if (op <= 10 && cur.size() >= 2) {                              // GOM một tập con (2..4) hình cấp trên cùng
                    std::vector<std::size_t> chon; const std::size_t nChon = 2 + rng() % 3;
                    std::vector<std::size_t> idx(cur.size()); for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = i;
                    std::shuffle(idx.begin(), idx.end(), rng);
                    for (std::size_t i = 0; i < std::min(nChon, idx.size()); ++i) chon.push_back(idx[i]);   // thứ tự NGẪU NHIÊN
                    std::vector<int> ids; for (auto k : chon) ids.push_back(cur[k].id);
                    m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::GOM, ids));
                    std::sort(chon.begin(), chon.end());
                    Node nhom{cv.layer(0).hinhThu(chon.front()).getId(), true, Diem2D(), {}};     // id nhóm do lệnh cấp
                    for (auto k : chon) nhom.kids.push_back(cur[k]);
                    undoS.push_back(cur); redoS.clear();
                    for (auto it = chon.rbegin(); it != chon.rend(); ++it) cur.erase(cur.begin() + static_cast<std::ptrdiff_t>(*it));
                    cur.insert(cur.begin() + static_cast<std::ptrdiff_t>(chon.front()), nhom); ++nGom;
                } else if (op <= 12) {                                                 // RA_NHOM một nhóm cấp trên cùng
                    std::vector<std::size_t> ng; for (std::size_t i = 0; i < cur.size(); ++i) if (cur[i].group) ng.push_back(i);
                    if (!ng.empty()) {
                        const std::size_t k = ng[rng() % ng.size()];
                        m.thucThi(std::make_unique<GroupCommand>(cv, 0, GroupCommand::Loai::RA_NHOM, std::vector<int>{cur[k].id}));
                        undoS.push_back(cur); redoS.clear();
                        std::vector<Node> con = cur[k].kids;
                        cur.erase(cur.begin() + static_cast<std::ptrdiff_t>(k));
                        cur.insert(cur.begin() + static_cast<std::ptrdiff_t>(k), con.begin(), con.end()); ++nRa;
                    }
                } else if (op <= 16) {                                                 // Undo
                    if (m.undo()) { redoS.push_back(cur); cur = undoS.back(); undoS.pop_back(); ++nUn; }
                    else { CHECK(undoS.empty()); }                                     // undo trả false <=> không còn gì để undo
                } else {                                                               // Redo
                    if (m.redo()) { undoS.push_back(cur); cur = redoS.back(); redoS.pop_back(); ++nRe; }
                    else { CHECK(redoS.empty()); }
                }
            } catch (const std::exception& e) { std::cout << "  ngoại lệ bất ngờ ở bước " << step << " (op=" << op << "): " << e.what() << "\n"; ok = false; }
            CHECK(m.coTheUndo() == !undoS.empty()); CHECK(m.coTheRedo() == !redoS.empty());
            if (ok && !khop()) { std::cout << "  lệch mô hình ở bước " << step << " (op=" << op << ")\n"; ok = false; }
        }
        CHECK(ok);
        std::cout << "  ngẫu nhiên: add=" << nAdd << " remove=" << nRem << " transform=" << nTr << " gom=" << nGom << " ranhom=" << nRa
                  << " undo=" << nUn << " redo=" << nRe << "\n";
        CHECK(nAdd > 500 && nRem > 500 && nTr > 500 && nGom > 300 && nRa > 100 && nUn > 1000 && nRe > 300);
        while (m.undo()) {}                                                            // undo sạch -> layer 0 rỗng
        CHECK(cv.layer(0).soHinh() == 0);
    }
    CHECK(Gia::song == 0);                                                             // không rò rỉ, không hủy hai lần

    std::cout << "TASK 15 test_undo_redo: " << (g_total - g_fail) << "/" << g_total << " pass\n";
    return g_fail == 0 ? 0 : 1;
}
