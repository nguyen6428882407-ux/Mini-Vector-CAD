#pragma once
// ============================================================================
// Command - lớp cơ sở TRỪU TƯỢNG của mọi lệnh (Command Pattern)
// Tầng: commands | Phụ trách: Đức Huy | Chỉ có header (không có .cpp)
// Trách nhiệm: định nghĩa giao diện execute()/undo().
// Quy ước:
//  - redo = gọi lại execute().
//  - Lệnh nhận BangVeCanvas& qua constructor và xác định hình bằng (layerIdx, id);
//    TUYỆT ĐỐI không giữ con trỏ thô tới hình (tránh dangling sau undo/redo).
//
// HỢP ĐỒNG (áp dụng thống nhất cho Add/Remove/Transform/GroupCommand):
//  1. Id bền: execute() lần sau (redo) phải khôi phục ĐÚNG các id đã dùng ở lần đầu
//     (ví dụ nhóm do GroupCommand tạo phải giữ nguyên id qua undo/redo), vì các lệnh
//     đứng sau trong lịch sử có thể đang tham chiếu id đó.
//  2. Lệnh giả định đầu vào HỢP LỆ: MenuController phải kiểm tra trước khi tạo lệnh
//     (layer tồn tại, id tồn tại, layer không khóa, ids không trùng/không rỗng).
//  3. Nếu vẫn gặp layer/id không tồn tại (lỗi lập trình): NÉM std::out_of_range,
//     KHÔNG bỏ qua im lặng. (layer(i) đã tự ném out_of_range; với id thì
//     timTheoId/layHinh trả nullptr -> lệnh phải kiểm tra nullptr rồi tự ném.)
//  4. undo()/redo() BỎ QUA cờ khóa của Layer (cờ khóa chỉ chặn ở bước tạo lệnh).
//  5. Destructor của lệnh KHÔNG được dùng canvas_ (canvas có thể đã bị hủy trước).
//  6. Lệnh không được cất Layer& / Layer* / HinhHoc2D* sau khi hàm trả về.
// ============================================================================
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};
