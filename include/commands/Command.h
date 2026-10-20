#pragma once
// ============================================================================
// Command - Lop co so TRUU TUONG cua moi lenh (Command Pattern)
// Tang: commands | Phu trach: Duc Huy | Chi co header (header-only)
//
// Trach nhiem:
//  - Dinh nghia giao dien execute() va undo() de ho tro Hoan tac / Lam lai.
//
// QUY UOC THUC THI:
//  - Redo (lam lai) = goi lai phuong thuc execute().
//  - Lenh nhan BangVeCanvas& qua constructor va xac dinh doi tuong qua (layerIdx, id).
//  - TUYET DOI khong giu con tro tho (raw pointer) toi hinh de tranh dangling pointer.
//
// 6 DIEU LUAT HOP DONG (AP DUNG CHO ADD / REMOVE / TRANSFORM / GROUP COMMAND):
//  1. Do ben cua ID (Persistent IDs):
//     Khi execute() lan sau (Redo) phai khoi phuc CHINH XAC cac ID da dung o lan dau.
//     Khong bao gio clone sinh ID moi khi Undo/Redo vi se lam gay cac lenh phia sau.
//  2. Dau vao hop le:
//     Controller phai kiem tra tinh hop le truoc khi tao lenh (ID ton tai, layer khong khoa).
//  3. Bat loi ngoai le nghiem ngat:
//     Neu gap Layer hoac ID khong ton tai (loi logic): NEM std::out_of_range,
//     tuyet doi khong bo qua im lang.
//  4. Khong phu thuoc co khoa Layer:
//     undo() va redo() bo qua co khoa cua Layer (co khoa chi chan o buoc tao lenh ban dau).
//  5. Destructor doc lap:
//     Destructor cua Command KHONG duoc truy cap canvas_ (canvas co the da bi huy truoc).
//  6. Khong giu tham chieu song hanh:
//     Lenh khong cat Layer& / Layer* / HinhHoc2D* sau khi ham thuc thi ket thuc.
// ============================================================================
class Command {
public:
    virtual ~Command() = default;

    // Thuc thi lenh lan dau hoac lam lai (Redo)
    virtual void execute() = 0;

    // Hoan tac lenh (Undo), khoi phuc trang thai truoc do
    virtual void undo() = 0;
};
