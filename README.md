# BÀI TẬP LỚN LẬP TRÌNH HƯỚNG ĐỐI TƯỢNG (OOP - C++)
# ĐỀ TÀI 17: PHẦN MỀM MINI VECTOR CAD

## 1. Giới thiệu tổng quan
**Mini-Vector-CAD** là một chương trình thiết kế và quản lý đồ họa vector 2D trên nền tảng giao diện dòng lệnh (CLI Console) viết bằng C++ hiện đại (C++17). Chương trình cho phép người dùng vẽ, quản lý các đối tượng hình học theo từng Layer (lớp vẽ), áp dụng các phép biến đổi Affine 2D (tịnh tiến, xoay, scale đồng dạng), hỗ trợ lịch sử hoàn tác/làm lại (Undo/Redo) và xuất dữ liệu ra file chuẩn vector SVG (`.svg`) cùng định dạng văn bản (`.txt`).

---

## 2. Danh sách thành viên và Phân công nhiệm vụ

| STT | Thành viên | Nhiệm vụ chính | Các file phụ trách |
| :---: | :--- | :--- | :--- |
| 1 | **Đức Huy** *(Nhóm trưởng)* | Interface cốt lõi, Composite Shape, Command Manager, CMake, Test nhóm | `HinhHoc2D`, `Command.h`, `GroupShape`, `CommandManager`, `GroupCommand`, `test_group.cpp`, `CMakeLists.txt` |
| 2 | **Nhất Vũ** | Phép biến đổi Affine 2D, Layer & Canvas, Các lệnh thao tác hình, Test Undo/Redo | `BienDoi2D`, `Layer`, `BangVeCanvas`, `AddShapeCommand`, `RemoveShapeCommand`, `TransformCommand`, `test_undo_redo.cpp` |
| 3 | **Giang Tưởng** | Điểm, Màu sắc, Tiện ích toán, Đoạn thẳng, Hình tròn, Test hình cơ bản | `Diem2D`, `MauSac`, `MathUtils`, `DoanThang`, `HinhTron`, `test_shapes.cpp` |
| 4 | **Hồ Hưng** | Hình chữ nhật, Tam giác, Đa giác, Shoelace formula, Test hình đa giác | `HinhChuNhat`, `TamGiac`, `DaGiac`, `test_shapes.cpp` |
| 5 | **Trường Vũ** | Bộ xuất SVG, Lưu/Tải trạng thái Text, Shape Factory, Menu Console, Validation, README & Demo | `SvgExporter`, `TextSerializer`, `ShapeFactory`, `MenuController`, `main.cpp`, `README.md`, `sample_draw.svg/txt` |

---

## 3. Kiến trúc phần mềm & Các Mẫu thiết kế (Design Patterns)

Chương trình áp dụng kiến trúc phân tầng nghiêm ngặt: **Tầng trên chỉ include tầng dưới**, bao gồm 5 tầng:
1. **Tầng Presentation & Controller (`controller/`)**: `MenuController`, `main.cpp` - Tương tác người dùng, điều phối hoạt động, kiểm tra tính hợp lệ dữ liệu.
2. **Tầng I/O & Serialization (`io/`)**: `SvgExporter`, `TextSerializer` - Xuất SVG theo chuẩn W3C (chỉ xuất các layer đang hiển thị) và lưu/khôi phục trạng thái bảng vẽ.
3. **Tầng Commands (`commands/`)**: `Command`, `CommandManager`, các lớp lệnh cụ thể - Quản lý lịch sử Undo/Redo.
4. **Tầng Canvas & Shapes (`canvas/`, `shapes/`)**: `BangVeCanvas`, `Layer`, `HinhHoc2D`, `GroupShape`, `ShapeFactory` - Quản lý mô hình dữ liệu hình học vector.
5. **Tầng Core Foundation (`core/`)**: `Diem2D`, `MauSac`, `MathUtils`, `BienDoi2D` - Xử lý tính toán đại số tuyến tính và hình học giải tích 2D.

### Các Design Patterns cốt lõi:
- **Composite Pattern**: `HinhHoc2D` đóng vai trò Component trừu tượng; các hình đơn lẻ (`DoanThang`, `HinhTron`, `HinhChuNhat`, `TamGiac`, `DaGiac`) là Leaf; `GroupShape` là Composite chứa danh sách các hình con, cho phép gom nhóm lồng nhau tùy ý.
- **Prototype Pattern**: Phương thức ảo `clone()` cho phép tạo bản sao sâu (deep copy) của một hình bất kỳ kèm theo việc cấp phát một **ID duy nhất mới**.
- **Command Pattern**: Các thao tác thay đổi trạng thái hình vẽ (`AddShape`, `RemoveShape`, `Transform`, `Group`) đều được đóng gói thành các đối tượng `Command` với hai hành vi `execute()` và `undo()`, kết hợp với `CommandManager` (hai stack Undo/Redo).
- **Factory Pattern (Simple Factory / Factory Method)**: `ShapeFactory` phân tích chuỗi văn bản để khởi tạo đối tượng `HinhHoc2D` cụ thể, giúp tách biệt logic nhập liệu khỏi các lớp hình.

---

## 4. Hướng dẫn Biên dịch và Chạy ứng dụng

### Yêu cầu môi trường:
- Trình biên dịch C++ hỗ trợ chuẩn **C++17** trở lên (GCC 9+, Clang 10+, hoặc MSVC 2019+).
- **CMake** phiên bản 3.15 trở lên.

### Các bước biên dịch:
```bash
# 1. Tạo thư mục build và di chuyển vào
mkdir build
cd build

# 2. Sinh Makefile / Project
cmake ..

# 3. Biên dịch chương trình
cmake --build .

# 4. Chạy ứng dụng
./MiniVectorCAD          # Trên Linux/macOS
.\Debug\MiniVectorCAD.exe # Trên Windows (MSVC)
```

---

## 5. Cú pháp nhập hình trong Menu (`ShapeFactory`)

Khi chọn chức năng **2. Thêm hình mới**, bạn có thể nhập các câu lệnh theo cú pháp sau:

| Loại hình | Cú pháp lệnh | Ví dụ |
| :--- | :--- | :--- |
| **Hình tròn** | `tron <cx> <cy> <r>` | `tron 680 120 50` |
| **Đoạn thẳng** | `doanthang <x1> <y1> <x2> <y2>` | `doanthang 120 480 120 380` |
| **Hình chữ nhật** | `chunhat <x> <y> <w> <h>` | `chunhat 200 320 220 160` |
| **Tam giác** | `tamgiac <x1> <y1> <x2> <y2> <x3> <y3>` | `tamgiac 180 320 310 190 440 320` |
| **Đa giác** | `dagiac <x1> <y1> <x2> <y2> <x3> <y3> ...` | `dagiac 50 100 80 120 70 150` |

### Tùy chọn định dạng màu sắc (Style) ở đuôi lệnh:
- `vien <r> <g> <b> [a]`: Màu viền RGB (Alpha tùy chọn từ 0-255).
- `nen <r> <g> <b> [a]`: Màu nền fill RGB.
- `net <w>`: Độ dày nét viền.
*Ví dụ:* `tron 100 100 40 vien 255 0 0 nen 255 255 0 net 2`

---

## 6. Các quy tắc Kiểm tra tính hợp lệ (Validation) trong MenuController

1. **Kiểm tra ID tồn tại**: Trước khi Xóa, Biến đổi hay Gom nhóm, chương trình kiểm tra xem ID hình có tồn tại trong Canvas/Layer hay không. Nếu không, in thông báo lỗi và không tạo lệnh rỗng.
2. **Kiểm tra cờ Khóa Layer (`isKhoa`)**: Layer bị khóa (`KHOA`) sẽ chặn hoàn toàn các thao tác thêm, xóa, biến đổi hình vẽ bên trong.
3. **Kiểm tra Hệ số Scale $k = 0$**: Khi thu phóng hình học quanh tâm, hệ số $k$ bị cấm tuyệt đối bằng 0 ($|k| < 10^{-9}$) nhằm đảm bảo tính khả nghịch để phục vụ cơ chế Hoàn tác (Undo).

---

## 7. Các tệp mẫu Demo trong thư mục `output/`

- **`output/sample_draw.svg`**: Bức tranh vector hoàn chỉnh vẽ phong cảnh "Ngôi nhà bên hồ dưới ánh mặt trời", bao gồm 3 layer hiển thị (`Background`, `Architecture`, `Nature`), sử dụng đầy đủ các đối tượng đường thẳng, hình tròn, chữ nhật, tam giác, đa giác, nhóm `GroupShape`, màu sắc RGB/Alpha và viền nét. Có thể mở trực tiếp bằng trình duyệt Google Chrome, Edge hoặc Inkscape.
- **`output/sample_draw.txt`**: Tệp văn bản lưu trữ trạng thái đầy đủ của bức tranh, có thể nạp lại tức thì vào chương trình qua tính năng **11. Nạp file trạng thái (.txt)**.

---

## 8. Kịch bản chạy Demo báo cáo Bài tập lớn

1. **Khởi động**: Chạy chương trình, hiển thị menu với layer mặc định `[0] Layer 1`.
2. **Nạp dữ liệu mẫu**: Chọn mục `11`, nhập `output/sample_draw.txt`.
3. **Xem trực quan bảng vẽ**: Chọn mục `8`, quan sát cấu trúc cây của các layer và các hình vẽ với ID tương ứng.
4. **Thao tác Layer**: Chọn mục `1`, ẩn layer `Background`, quay lại menu chính và chọn `9` để xuất SVG ra file tạm; mở file để kiểm chứng layer bị ẩn không xuất hiện trong SVG.
5. **Thao tác Thêm & Undo/Redo**: Chọn mục `2`, thêm một hình tròn `tron 400 300 60`. Chọn `6` để Undo, hình tròn biến mất; chọn `7` để Redo, hình tròn xuất hiện lại với đúng ID ban đầu.
6. **Thao tác Biến đổi**: Chọn mục `4`, biến đổi hình tròn vừa tạo (thử scale $k=0$ để thấy báo lỗi; scale $k=1.5$ thành công; sau đó Undo để hình trở về kích thước cũ).
7. **Thao tác Gom nhóm**: Chọn mục `5`, nhập danh sách ID để gộp các hình thành một `GroupShape`.