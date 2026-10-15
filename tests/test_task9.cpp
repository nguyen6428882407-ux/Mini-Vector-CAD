#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include "canvas/BangVeCanvas.h"
#include "commands/CommandManager.h"
#include "commands/AddShapeCommand.h"
#include "commands/RemoveShapeCommand.h"
#include "commands/TransformCommand.h"
#include "commands/GroupCommand.h"
#include "io/TextSerializer.h"
#include <sstream>
#include "shapes/HinhTron.h"
#include "shapes/DoanThang.h"
#include "shapes/GroupShape.h"
#include "shapes/NetVeTuDo.h"
#include "shapes/ShapeFactory.h"
#include "core/BienDoi2D.h"

int main() {
    {
        BangVeCanvas source;
        source.themLayer("Brush Layer");
        auto brush = ShapeFactory::taoHinh("brush 3 1 2 3 4 8 5 vien 30 60 90 128 nen 255 255 255 0 net 6");
        source.layer(0).themHinh(std::move(brush));

        std::stringstream serialized;
        TextSerializer::serialize(source, serialized);
        assert(serialized.str().find("brush 3 1 2 3 4 8 5") != std::string::npos);

        BangVeCanvas destination;
        TextSerializer::deserialize(destination, serialized);
        assert(destination.soLayer() == 1);
        auto* importedBrush = dynamic_cast<const NetVeTuDo*>(&destination.layer(0).hinhThu(0));
        assert(importedBrush != nullptr);
        assert(importedBrush->getDiem().size() == 3);
        assert(importedBrush->getMauVien().getA() == 128);
        assert(importedBrush->getDoDayNet() == 6.0);
    }

    {
        BangVeCanvas source(800.0, 600.0);
        source.themLayer("Imported Layer");
        std::stringstream serialized;
        TextSerializer::serialize(source, serialized);

        BangVeCanvas destination;
        destination.themLayer("Old Layer");
        TextSerializer::deserialize(destination, serialized);
        assert(destination.layer(0).getTen() == "Imported Layer");
    }

    std::cout << "=== BAT DAU KIEM THU TOAN DIEN TASK 9 ===\n\n";

    // -------------------------------------------------------------------------
    // TEST 1: CommandManager co ban & 2 Stack Undo/Redo
    // -------------------------------------------------------------------------
    std::cout << "[Test 1] Kiem tra CommandManager (2 stack, lenh moi xoa redo)...\n";
    {
        BangVeCanvas canvas;
        canvas.themLayer("Layer 1");
        CommandManager mgr;

        assert(!mgr.coTheUndo());
        assert(!mgr.coTheRedo());
        assert(!mgr.undo()); // undo rong -> false
        assert(!mgr.redo()); // redo rong -> false

        // Them 1 hinh
        auto tron = std::make_unique<HinhTron>(Diem2D(0, 0), 10.0);
        int tronId = tron->getId();
        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(tron)));

        assert(mgr.coTheUndo());
        assert(!mgr.coTheRedo());
        assert(mgr.soLenhUndo() == 1);
        assert(mgr.soLenhRedo() == 0);
        assert(canvas.layer(0).timTheoId(tronId) != nullptr);

        // Undo
        assert(mgr.undo());
        assert(!mgr.coTheUndo());
        assert(mgr.coTheRedo());
        assert(mgr.soLenhUndo() == 0);
        assert(mgr.soLenhRedo() == 1);
        assert(canvas.layer(0).timTheoId(tronId) == nullptr);

        // Redo
        assert(mgr.redo());
        assert(mgr.coTheUndo());
        assert(!mgr.coTheRedo());
        assert(canvas.layer(0).timTheoId(tronId) != nullptr);

        // Undo roi thuc thi lenh moi -> Redo stack phai bi xoa sach!
        assert(mgr.undo());
        assert(mgr.coTheRedo());
        auto doan = std::make_unique<DoanThang>(Diem2D(0, 0), Diem2D(5, 5));
        int doanId = doan->getId();
        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(doan)));

        assert(!mgr.coTheRedo()); // Redo bi xoa!
        assert(mgr.soLenhRedo() == 0);
        assert(mgr.soLenhUndo() == 1);
        assert(canvas.layer(0).timTheoId(doanId) != nullptr);
        assert(canvas.layer(0).timTheoId(tronId) == nullptr);

        // Xoa lich su
        mgr.xoaLichSu();
        assert(!mgr.coTheUndo());
        assert(!mgr.coTheRedo());
        std::cout << "  -> PASS Test 1!\n\n";
    }

    // -------------------------------------------------------------------------
    // TEST 2: GroupCommand (Gom nhom) & Bao toan ID ben qua Undo/Redo
    // -------------------------------------------------------------------------
    std::cout << "[Test 2] Kiem tra GroupCommand Gom nhom & ID ben...\n";
    {
        BangVeCanvas canvas;
        canvas.themLayer("Layer 1");
        CommandManager mgr;

        auto h1 = std::make_unique<HinhTron>(Diem2D(0, 0), 5.0);
        auto h2 = std::make_unique<DoanThang>(Diem2D(1, 1), Diem2D(4, 4));
        int id1 = h1->getId();
        int id2 = h2->getId();

        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h1)));
        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h2)));

        assert(canvas.layer(0).soHinh() == 2);

        // Gom h1 va h2
        auto groupCmd = std::make_unique<GroupCommand>(canvas, 0, std::vector<int>{id1, id2});
        mgr.thucThi(std::move(groupCmd));

        // Sau khi gom, Layer chi con 1 hinh la GroupShape
        assert(canvas.layer(0).soHinh() == 1);
        auto* nhom = dynamic_cast<GroupShape*>(&canvas.layer(0).hinhThu(0));
        assert(nhom != nullptr);
        int groupId = nhom->getId();
        assert(groupId != id1 && groupId != id2);
        assert(nhom->soLuongCon() == 2);

        // Undo Gom nhom -> 2 hinh con tro lai Layer voi DUNG ID BAN DAU
        assert(mgr.undo());
        assert(canvas.layer(0).soHinh() == 2);
        assert(canvas.layer(0).timTheoId(id1) != nullptr);
        assert(canvas.layer(0).timTheoId(id2) != nullptr);
        assert(canvas.layer(0).timTheoId(groupId) == nullptr);

        // Redo Gom nhom -> Nhóm phuc hoi voi DUNG groupId BAN DAU (ID ben!)
        assert(mgr.redo());
        assert(canvas.layer(0).soHinh() == 1);
        auto* nhomRedo = dynamic_cast<GroupShape*>(canvas.layer(0).timTheoId(groupId));
        assert(nhomRedo != nullptr);
        assert(nhomRedo->getId() == groupId);
        assert(nhomRedo->soLuongCon() == 2);

        std::cout << "  -> PASS Test 2!\n\n";
    }

    // -------------------------------------------------------------------------
    // TEST 3: GroupCommand (Ra nhom) & Undo/Redo Ra nhom
    // -------------------------------------------------------------------------
    std::cout << "[Test 3] Kiem tra GroupCommand Ra nhom (Ungroup)...\n";
    {
        BangVeCanvas canvas;
        canvas.themLayer("Layer 1");
        CommandManager mgr;

        auto h1 = std::make_unique<HinhTron>(Diem2D(0, 0), 5.0);
        auto h2 = std::make_unique<DoanThang>(Diem2D(1, 1), Diem2D(4, 4));
        int id1 = h1->getId();
        int id2 = h2->getId();

        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h1)));
        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h2)));

        // Gom nhom truoc
        mgr.thucThi(std::make_unique<GroupCommand>(canvas, 0, std::vector<int>{id1, id2}));
        int groupId = canvas.layer(0).hinhThu(0).getId();

        // Ra nhom bang GroupCommand(canvas, 0, groupId, Loai::RA_NHOM)
        mgr.thucThi(std::make_unique<GroupCommand>(canvas, 0, groupId, GroupCommand::Loai::RA_NHOM));

        // Sau khi ra nhom, Layer co lai 2 hinh con rieng le, nhom bien mat
        assert(canvas.layer(0).soHinh() == 2);
        assert(canvas.layer(0).timTheoId(groupId) == nullptr);
        assert(canvas.layer(0).timTheoId(id1) != nullptr);
        assert(canvas.layer(0).timTheoId(id2) != nullptr);

        // Undo ra nhom -> Nhom phai quay tro lai voi dung groupId
        assert(mgr.undo());
        assert(canvas.layer(0).soHinh() == 1);
        assert(canvas.layer(0).timTheoId(groupId) != nullptr);
        assert(canvas.layer(0).timTheoId(id1) == nullptr);
        assert(canvas.layer(0).timTheoId(id2) == nullptr);

        // Redo ra nhom -> Nhom bien mat lai, 2 con xuat hien
        assert(mgr.redo());
        assert(canvas.layer(0).soHinh() == 2);
        assert(canvas.layer(0).timTheoId(groupId) == nullptr);
        assert(canvas.layer(0).timTheoId(id1) != nullptr);
        assert(canvas.layer(0).timTheoId(id2) != nullptr);

        std::cout << "  -> PASS Test 3!\n\n";
    }

    // -------------------------------------------------------------------------
    // TEST 4: Chuoi lenh phuc tap & Hoi quy ID ben theo hop dong
    // (Add A, Add B -> Group -> Transform(nhom) -> Undo x2 -> Redo x2)
    // -------------------------------------------------------------------------
    std::cout << "[Test 4] Chuoi Hoi quy ID ben: Add -> Group -> Transform -> Undo x2 -> Redo x2...\n";
    {
        BangVeCanvas canvas;
        canvas.themLayer("Layer 1");
        CommandManager mgr;

        auto h1 = std::make_unique<HinhTron>(Diem2D(0, 0), 10.0);
        auto h2 = std::make_unique<DoanThang>(Diem2D(0, 0), Diem2D(10, 0));
        int id1 = h1->getId();
        int id2 = h2->getId();

        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h1)));
        mgr.thucThi(std::make_unique<AddShapeCommand>(canvas, 0, std::move(h2)));

        // Group {h1, h2}
        mgr.thucThi(std::make_unique<GroupCommand>(canvas, 0, std::vector<int>{id1, id2}));
        int groupId = canvas.layer(0).hinhThu(0).getId();

        // Transform nhom (Tinh tien dx=20, dy=30)
        mgr.thucThi(std::make_unique<TransformCommand>(canvas, 0, groupId, BienDoi2D::tinhTien(20, 30)));

        // Undo 1: Huy Transform
        assert(mgr.undo());
        // Undo 2: Huy Group
        assert(mgr.undo());

        assert(canvas.layer(0).soHinh() == 2);
        assert(canvas.layer(0).timTheoId(id1) != nullptr);
        assert(canvas.layer(0).timTheoId(id2) != nullptr);

        // Redo 1: Gom nhom lai
        assert(mgr.redo());
        // Nhom phai co dung groupId ban dau de lenh Transform phia sau tham chieu duoc!
        assert(canvas.layer(0).timTheoId(groupId) != nullptr);

        // Redo 2: Transform lai nhom
        assert(mgr.redo());
        assert(canvas.layer(0).timTheoId(groupId) != nullptr);

        std::cout << "  -> PASS Test 4!\n\n";
    }

    // -------------------------------------------------------------------------
    // TEST 5: Kiem tra ngoai le (Exception handling)
    // -------------------------------------------------------------------------
    std::cout << "[Test 5] Kiem tra Exception khi ID khong hop le...\n";
    {
        BangVeCanvas canvas;
        canvas.themLayer("Layer 1");
        CommandManager mgr;

        bool caught = false;
        try {
            // Gom ID khong ton tai
            mgr.thucThi(std::make_unique<GroupCommand>(canvas, 0, std::vector<int>{999, 1000}));
        } catch (const std::out_of_range&) {
            caught = true;
        }
        assert(caught);
        // Khi nem exception, lenh khong duoc vao undo stack
        assert(!mgr.coTheUndo());

        std::cout << "  -> PASS Test 5!\n\n";
    }

    std::cout << "=== TAT CA 5/5 TEST TASK 9 DEU PASS XUAT SAC! ===\n";
    return 0;
}

