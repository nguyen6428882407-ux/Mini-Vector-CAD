# Mini Vector CAD

Mini Vector CAD là ứng dụng vẽ vector 2D viết bằng **C++17**, gồm chương trình CLI, giao diện web tĩnh và một C++ HTTP backend cục bộ để giao diện web đồng bộ bản vẽ.

## Tính năng

### Ứng dụng C++

- Quản lý bản vẽ theo layer; ẩn/hiện và khóa layer.
- Các hình: đường thẳng, hình tròn, hình chữ nhật, tam giác, đa giác, nét bút tự do và nhóm hình.
- Biến đổi hình bằng phép tịnh tiến, xoay và scale.
- Hoàn tác/làm lại các thao tác được hỗ trợ.
- Lưu và nạp trạng thái bản vẽ dạng `.txt`; xuất SVG.
- Shape Factory kiểm tra dữ liệu hình đầu vào.

### Giao diện web

- Vẽ hình bằng chuột trái; chọn, di chuyển, đổi kích thước và xoay hình.
- Vẽ nét tự do với màu, độ dày và độ đậm tùy chỉnh; dùng gôm để xóa nét hoặc hình.
- Tạo, chọn, ẩn/hiện và xóa layer; không thể xóa layer cuối cùng.
- Hoàn tác/làm lại, sao chép, nhân bản và xóa hình; đổi giao diện sáng/tối.
- Tự kết nối và đồng bộ bản vẽ với C++ backend chạy trên cùng máy.
- Xuất bản vẽ web ra SVG.

## Công nghệ

- C++17
- CMake 3.15+
- HTML, CSS và JavaScript thuần
- HTTP API cục bộ dùng để kết nối giao diện web với C++ backend

## Biên dịch

Cần có trình biên dịch hỗ trợ C++17 và CMake 3.15 trở lên. Ví dụ với MinGW trên Windows:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
```

Nếu MinGW chưa nằm trong `PATH`, chỉ định đường dẫn tới trình biên dịch:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=D:/mingw64/bin/g++.exe
cmake --build build --parallel
```

Trên Linux/macOS hoặc với Visual Studio, có thể dùng generator mặc định:

```sh
cmake -S . -B build
cmake --build build --parallel
```

## Chạy ứng dụng CLI

Sau khi build, chạy từ thư mục gốc dự án:

```powershell
.\build\MiniVectorCAD.exe
```

Trên Linux/macOS:

```sh
./build/MiniVectorCAD
```

CLI hiển thị menu tương tác. Chọn mục **2** để nhập hình; chọn mục **9** để xuất SVG và mục **10/11** để lưu/nạp file trạng thái.

### Cú pháp nét bút trong CLI

```text
brush <số_điểm> <x1> <y1> <x2> <y2> ...
```

Ví dụ:

```text
brush 3 10 20 40 30 70 25 vien 51 65 85 128 net 8
```

Các kiểu hình khác gồm `tron`, `doanthang`, `chunhat`, `tamgiac` và `dagiac`. Có thể thêm thuộc tính `vien <r> <g> <b> [a]`, `nen <r> <g> <b> [a]` và `net <độ_dày>` vào cuối câu lệnh.

## Chạy giao diện web và C++ backend

Chạy backend và web server trong **hai cửa sổ terminal riêng**, tại thư mục gốc dự án.

**Terminal 1 — khởi động C++ backend:**

```powershell
.\build\MiniVectorCADServer.exe
```

**Terminal 2 — phục vụ giao diện web:**

```powershell
python -m http.server 8765 --bind 127.0.0.1
```

Mở <http://127.0.0.1:8765/web/> trong trình duyệt. Trang web tự kết nối backend tại `127.0.0.1:8766`; nếu backend chưa chạy khi mở trang, web sẽ tự thử kết nối lại.

Backend chỉ lắng nghe trên loopback (`127.0.0.1`) và giới hạn nguồn kết nối web vào các địa chỉ localhost được cấu hình sẵn. Đây là backend phát triển cục bộ, **không dành cho triển khai công khai trên Internet**.

> Backend hiện giữ bản vẽ trong bộ nhớ. Khi tắt hoặc khởi động lại backend, trạng thái đang đồng bộ sẽ trở về bản vẽ trống. CLI và backend web cũng là hai tiến trình riêng, không tự chia sẻ trạng thái trực tiếp với nhau.

## Chạy kiểm thử

```powershell
ctest --test-dir build --output-on-failure
```

Các test suite tự động bao gồm:
- `test_group`: Kiểm thử Composite Pattern, deep clone và operator+ của `GroupShape` (Đức Huy phụ trách).
- `test_task9`: Kiểm thử 2 stack Undo/Redo của `CommandManager` và Gom/Rã nhóm của `GroupCommand` (Đức Huy phụ trách).
- `test_shapes`: Kiểm thử hình học cơ bản, tính diện tích, chu vi, biến đổi hình.
- `test_undo_redo`: Kiểm thử chuỗi thao tác Undo/Redo tích hợp toàn hệ thống.
- `test_auth`: Kiểm thử xác thực người dùng và phân quyền bản vẽ.

## Cấu trúc dự án

```text
.
|-- include/       # Header C++: canvas, shapes, commands, I/O và backend
|-- src/           # Mã nguồn C++ và điểm vào CLI/server
|-- tests/         # Các chương trình kiểm thử
|-- web/           # Giao diện HTML, CSS và JavaScript
|-- output/        # Bản vẽ mẫu SVG và TXT
|-- CMakeLists.txt
`-- README.md
```

## Mẫu bản vẽ

- `output/sample_draw.svg`: bản vẽ mẫu có thể mở bằng trình duyệt hoặc phần mềm hỗ trợ SVG.
- `output/sample_draw.txt`: dữ liệu mẫu để nạp bằng chức năng nạp trạng thái trong CLI.

## Ghi nhận đóng góp ban đầu

| Thành viên | Phần việc chính | File mã nguồn phụ trách | Trạng thái kiểm thử |
|---|---|---|---|
| **Đức Huy** | Interface cốt lõi, Composite Shape, Command Manager, CMake | `HinhHoc2D.h/.cpp`, `Command.h`, `GroupShape.h/.cpp`, `CommandManager.h/.cpp`, `GroupCommand.h/.cpp`, `CMakeLists.txt` | ✅ `test_group.cpp` (7/7 PASS), `test_task9.cpp` (7/7 PASS) |
| **Nhất Vũ** *(Trưởng nhóm)* | Phép biến đổi 2D, Layer/Canvas và các lệnh thao tác hình | `BienDoi2D`, `Layer`, `BangVeCanvas`, `TransformCommand`, `RemoveShapeCommand`, `AddShapeCommand` | ✅ `test_undo_redo.cpp` |
| **Giang Tưởng** | Điểm, màu sắc, tiện ích toán học, đoạn thẳng và hình tròn | `Diem2D`, `MauSac`, `MathUtils`, `DoanThang`, `HinhTron` | ✅ `test_shapes.cpp` |
| **Hồ Hưng** | Hình chữ nhật, tam giác và đa giác | `HinhChuNhat`, `TamGiac`, `DaGiac` | ✅ `test_shapes.cpp` |
| **Trường Vũ** | SVG, serializer, Shape Factory, menu CLI và validation | `SvgExporter`, `TextSerializer`, `ShapeFactory`, `MenuController` | ✅ CLI & Web Server |

## Trạng thái repository

Không cần cài thư viện JavaScript bên ngoài để chạy giao diện web. Thư mục `build/` và các sản phẩm biên dịch đã được loại khỏi Git bằng `.gitignore`.


3. **Xem trực quan bảng vẽ**: Chọn mục `8`, quan sát cấu trúc cây của các layer và các hình vẽ với ID tương ứng.
4. **Thao tác Layer**: Chọn mục `1`, ẩn layer `Background`, quay lại menu chính và chọn `9` để xuất SVG ra file tạm; mở file để kiểm chứng layer bị ẩn không xuất hiện trong SVG.
5. **Thao tác Thêm & Undo/Redo**: Chọn mục `2`, thêm một hình tròn `tron 400 300 60`. Chọn `6` để Undo, hình tròn biến mất; chọn `7` để Redo, hình tròn xuất hiện lại với đúng ID ban đầu.
6. **Thao tác Biến đổi**: Chọn mục `4`, biến đổi hình tròn vừa tạo (thử scale $k=0$ để thấy báo lỗi; scale $k=1.5$ thành công; sau đó Undo để hình trở về kích thước cũ).
7. **Thao tác Gom nhóm**: Chọn mục `5`, nhập danh sách ID để gộp các hình thành một `GroupShape`.
