# Mini Vector CAD 🎨📐

Hệ thống thiết kế đồ họa vector 2D hiện đại, hiệu năng cao viết bằng **C++17**, kết hợp giữa chương trình dòng lệnh (CLI), máy chủ C++ HTTP Backend đồng bộ thời gian thực và giao diện **Web CAD Studio** tương tác trực quan.

---

## 👥 Danh sách Thành viên & Phân công Trách nhiệm

| STT | Thành viên | Vai trò | Phân hệ phụ trách chính | File mã nguồn phụ trách | Trạng thái kiểm thử |
|:---:|---|:---:|---|---|:---:|
| 1 | **Nhất Vũ** | **Trưởng nhóm (Team Leader)** | • Kiến trúc biến đổi Affine 2D & ma trận nghịch đảo<br>• Quản lý Canvas, danh sách Layers và Z-Index<br>• Lập trình các lệnh Command cơ sở (`Add`, `Remove`, `Transform`)<br>• Cấu hình hệ thống build tự động CMake<br>• Xây dựng giao diện Web CAD Studio<br>• **Phát triển tính năng mới: Lịch sử vẽ (Drawing History) & Phục hồi nét** | `BienDoi2D.h/.cpp`<br>`Layer.h/.cpp`<br>`BangVeCanvas.h/.cpp`<br>`AddShapeCommand.h/.cpp`<br>`RemoveShapeCommand.h/.cpp`<br>`TransformCommand.h/.cpp`<br>`CMakeLists.txt`<br>`web/` (`app.js`, `style.css`, `index.html`) | ✅ `test_undo_redo.cpp` (PASS) |
| 2 | **Đức Huy** | Thành viên | • Thiết kế lớp cơ sở trừu tượng tối cao `HinhHoc2D`<br>• Giao diện lệnh trừu tượng `Command.h`<br>• Mẫu Composite Pattern: nhóm hình lồng nhau `GroupShape`<br>• Quản lý ngăn xếp Undo/Redo `CommandManager`<br>• Lệnh gom nhóm `GroupCommand`<br>• Biên soạn nội dung Báo cáo Chương 5 | `HinhHoc2D.h/.cpp`<br>`Command.h`<br>`GroupShape.h/.cpp`<br>`CommandManager.h/.cpp`<br>`GroupCommand.h/.cpp`<br>`Báo cáo Chương 5` | ✅ `test_group.cpp` (7/7 PASS)<br>✅ `test_task9.cpp` (7/7 PASS) |
| 3 | **Trường Vũ** | Thành viên | • Xuất vector chuẩn công nghiệp W3C SVG 1.1<br>• Bộ tuần tự hóa dữ liệu lưu/nạp file `.txt`<br>• Mẫu Factory Method: `ShapeFactory` phân tích cú pháp tạo hình<br>• Bộ điều khiển tương tác dòng lệnh `MenuController`<br>• Tối ưu giao diện Web Mobile Responsive<br>• Thiết kế Sơ đồ lớp UML (Class Diagram) | `SvgExporter.h/.cpp`<br>`TextSerializer.h/.cpp`<br>`ShapeFactory.h/.cpp`<br>`MenuController.h/.cpp`<br>`main.cpp`<br>`Sơ đồ lớp UML` | ✅ `test_group.cpp` (PASS)<br>✅ CLI & Server Run |
| 4 | **Hồ Hưng** | Thành viên | • Lập trình 3 lớp hình học đa giác: Chữ nhật, Tam giác, Đa giác<br>• Cài đặt giải thuật diện tích Shoelace (Gauss) và Heron<br>• Phép biến đổi Affine và tính toán tâm đối tượng<br>• Biên soạn nội dung Báo cáo Chương 3 | `HinhChuNhat.h/.cpp`<br>`TamGiac.h/.cpp`<br>`DaGiac.h/.cpp`<br>`Báo cáo Chương 3` | ✅ `test_shapes.cpp` (PASS) |
| 5 | **Giang Tưởng** | Thành viên | • Nền tảng tính toán tọa độ `Diem2D` (nạp chồng toán tử)<br>• Mô hình màu sắc `MauSac` (RGBA, HEX) & `MathUtils`<br>• Lập trình 2 hình cơ bản: Đoạn thẳng và Hình tròn<br>• Biến đổi Affine và scale bán kính hình tròn theo hệ số<br>• Biên soạn nội dung Báo cáo Chương 4 | `Diem2D.h/.cpp`<br>`MauSac.h/.cpp`<br>`MathUtils.h`<br>`DoanThang.h/.cpp`<br>`HinhTron.h/.cpp`<br>`Báo cáo Chương 4` | ✅ `test_shapes.cpp` (PASS) |

---

## 🚀 Tính năng Nổi bật

### 1. Động cơ Hình học C++17 Core
- **9 loại hình học hoàn chỉnh:** Đoạn thẳng (`DoanThang`), Hình tròn (`HinhTron`), Hình chữ nhật (`HinhChuNhat`), Tam giác (`TamGiac`), Đa giác tùy ý (`DaGiac`), Đường cong Bézier bậc hai (`DoanCong`), Sóng dao động tuần hoàn (`DoanSong`), Nét cọ vẽ tự do (`NetVeTuDo`) và Nhóm hình đa cấp (`GroupShape`).
- **Quản lý Bản vẽ theo Layer:** Hỗ trợ tạo mới, đổi tên, xóa, đổi thứ tự hiển thị (Z-Index), ẩn/hiện và khóa layer chống chỉnh sửa ngoài ý muốn.
- **Biến đổi không gian Affine 2D:** Tịnh tiến (Translation), Co giãn (Scaling quanh tâm đối tượng) và Xoay (Rotation quanh tâm hoặc điểm tùy chọn).
- **Mẫu thiết kế chuẩn mực:**
  - **Composite Pattern:** Gom nhiều hình con thành một thực thể duy nhất, cho phép nhóm lồng nhóm đệ quy không giới hạn cấp độ.
  - **Command Pattern:** Quản lý 2 ngăn xếp `UndoStack` và `RedoStack` theo cơ chế LIFO, hỗ trợ hoàn tác/làm lại đa cấp.
  - **Factory Method:** `ShapeFactory` kiểm tra tính hợp lệ của tham số và khởi tạo hình an toàn từ chuỗi lệnh.
  - **Prototype Pattern:** Phương thức thuần ảo `clone()` hỗ trợ sao chép sâu (deep copy) với ID mới độc lập.
  - **An toàn bộ nhớ (RAII):** Quản lý tài nguyên 100% bằng smart pointer `std::unique_ptr`, triệt tiêu hoàn toàn lỗi rò rỉ bộ nhớ (Memory Leak).

### 2. Giao diện Web CAD Studio & C++ Backend
- **Tương tác trực quan 60 FPS:** Vẽ hình học, vẽ cọ tự do, chọn đơn (`V`), quét chọn nhóm (`G`), di chuyển, xoay và đổi kích thước bằng chuột.
- **Bảng thuộc tính màu sắc:** Tùy biến mã màu viền (HEX/RGBA), độ dày nét, màu nền bề mặt, hỗ trợ chế độ nền trong suốt.
- **Theme Sáng / Tối:** Chuyển đổi Dark Mode và Light Mode chỉ với 1 click, toàn bộ icon vector tự động đổi màu tương thích.
- **Đồng bộ đa dự án:** Máy chủ Socket C++ (`ProjectServer` - Port 8766) kết nối trực tiếp với giao diện Web, tự động lưu và đồng bộ bản vẽ giữa các phiên làm việc.

### 3. ⭐ Hệ thống Lịch sử vẽ & Phục hồi nét (Mới & Đột phá)
- **Bảo toàn lịch sử khi xóa (Non-destructive History):** Khi người dùng gôm nét hoặc xóa hình, thông tin không bị mất mà được đánh dấu *"Đã xóa nét"* kèm mốc thời gian và người thực hiện xóa.
- **Lưu trữ Metadata chi tiết:** Ghi nhận chính xác tác giả nét vẽ (Author), thời gian vẽ (Timestamp đến từng giây), Layer sở hữu và thuộc tính nét.
- **Hộp thoại Modal Lịch sử chuyên nghiệp:**
  - Bộ lọc đa trạng thái: *Tất cả*, *Đang hiển thị*, *Đã xóa nét*.
  - Tìm kiếm thông minh theo tên nét vẽ hoặc người vẽ.
  - **Khôi phục nét vẽ (Restore Shape):** Phục hồi nét đã xóa trở lại canvas chỉ với 1 click.
  - **Xuất nhật ký (.txt):** Tải về toàn bộ lịch sử vẽ dưới dạng tệp văn bản có cấu trúc hoàn chỉnh.

### 4. Xuất / Nhập Dữ liệu Chuẩn hóa
- **Định dạng `.TXT` nội bộ:** Lưu/nạp cấu trúc phân tầng, thứ tự và toàn bộ tham số hình học để tiếp tục phiên vẽ.
- **Định dạng chuẩn W3C SVG 1.1:** Xuất file vector chất lượng cao, mở và chỉnh sửa tương thích 100% trên Adobe Illustrator, CorelDRAW, trình duyệt Web và máy cắt laser/CNC.

---

## 🛠️ Yêu cầu Kỹ thuật & Công nghệ
- **Ngôn ngữ:** C++17 (ISO/IEC 14882:2017)
- **Hệ thống Build:** CMake 3.15 trở lên
- **Trình biên dịch:** GCC/MinGW (Windows), Clang (macOS), GCC (Linux)
- **Frontend:** HTML5, CSS3, JavaScript thuần (Vanilla JS - không cần cài node_modules)
- **Backend:** C++ Native Socket HTTP Server (Winsock2 trên Windows, POSIX Sockets trên Linux/macOS)

---

## 🔨 Hướng dẫn Biên dịch (Build)

### 1. Trên Windows (với MinGW Makefiles)
```powershell
# Tạo thư mục build và cấu hình
cmake -S . -B build -G "MinGW Makefiles"

# Biên dịch dự án
cmake --build build --parallel
```
*(Nếu MinGW chưa nằm trong PATH hệ thống, chỉ định rõ đường dẫn:)*
```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=D:/mingw64/bin/g++.exe
cmake --build build --parallel
```

### 2. Trên Linux / macOS
```bash
cmake -S . -B build
cmake --build build --parallel
```

---

## 💻 Hướng dẫn Sử dụng

### 1. Cách 1: Khởi động hệ thống tự động (Khuyên dùng)
Trên Windows, bạn chỉ cần click đúp vào file:
```cmd
chay_he_thong.bat
```
Script sẽ tự động khởi động C++ Backend (Port 8766) và mở trình duyệt web.

---

### 2. Cách 2: Khởi động thủ công (Web CAD Studio + C++ Backend)
Mở **hai cửa sổ terminal riêng biệt** tại thư mục gốc dự án:

* **Terminal 1 — Khởi động C++ Backend:**
  ```powershell
  .\build\MiniVectorCADServer.exe
  ```

* **Terminal 2 — Khởi động Web Server:**
  ```powershell
  python -m http.server 8765 --bind 127.0.0.1
  ```

* **Truy cập ứng dụng:** Mở trình duyệt và truy cập địa chỉ:
  👉 **`http://127.0.0.1:8765/web/`**

---

### 3. Cách 3: Chạy ứng dụng Dòng lệnh (CLI Console)
Sau khi biên dịch, chạy trực tiếp từ thư mục gốc:
```powershell
.\build\MiniVectorCAD.exe
```

* **Cú pháp nhập lệnh vẽ nét bút trong CLI:**
  ```text
  brush <số_điểm> <x1> <y1> <x2> <y2> ... vien <r> <g> <b> [a] net <độ_dày>
  ```
  *Ví dụ:*
  ```text
  brush 3 10 20 40 30 70 25 vien 51 65 85 128 net 8
  ```
* Các loại hình khác trong CLI: `tron`, `doanthang`, `chunhat`, `tamgiac`, `dagiac`. Có thể bổ sung thuộc tính: `vien <r> <g> <b> [a]`, `nen <r> <g> <b> [a]`, `net <độ_dày>` vào cuối câu lệnh.

---

## 🧪 Chạy Kiểm thử Tự động (Unit Tests)

Dự án tích hợp sẵn 5 bộ test tự động độc lập qua CTest:
```powershell
ctest --test-dir build --output-on-failure
```

*Danh sách các bộ kiểm thử:*
1. **`test_shapes`**: Kiểm thử các lớp hình học cơ bản, chu vi, diện tích, clone và biến đổi Affine.
2. **`test_group`**: Kiểm thử Composite Pattern, duyệt cây đệ quy và deep copy của `GroupShape`.
3. **`test_undo_redo`**: Kiểm thử toàn diện chuỗi thao tác hoàn tác / làm lại trên `CommandManager`.
4. **`test_task9`**: Kiểm thử giới hạn kích thước lịch sử lệnh và cơ chế Gom / Rã nhóm.
5. **`test_auth`**: Kiểm thử cơ chế xác thực tài khoản và phân quyền dự án.

---

## 📂 Cấu trúc Thư mục Dự án

```text
Mini-Vector-CAD/
├── CMakeLists.txt              # Cấu hình biên dịch tự động CMake
├── chay_he_thong.bat           # Script khởi động nhanh 1-click trên Windows
├── README.md                   # Tài liệu hướng dẫn dự án
├── include/                    # Header files (.h)
│   ├── core/                   # Diem2D, MauSac, MathUtils, BienDoi2D
│   ├── shapes/                 # HinhHoc2D, HinhTron, HinhChuNhat, TamGiac, DaGiac, DoanCong, DoanSong, GroupShape...
│   ├── canvas/                 # BangVeCanvas, Layer
│   ├── commands/               # Command, CommandManager, Add/Remove/Transform/GroupCommand
│   ├── controller/             # MenuController (CLI)
│   ├── io/                     # SvgExporter, TextSerializer
│   └── web/                    # ProjectServer (C++ HTTP Server)
├── src/                        # Source code files (.cpp)
│   ├── main.cpp                # Điểm vào ứng dụng CLI
│   ├── core/                   # Cài đặt nền tảng toán học & màu sắc
│   ├── shapes/                 # Cài đặt các đối tượng hình học & Factory
│   ├── canvas/                 # Cài đặt quản lý bản vẽ & Layer
│   ├── commands/               # Cài đặt ngăn xếp lệnh Undo/Redo
│   ├── controller/             # Cài đặt menu điều khiển CLI
│   ├── io/                     # Cài đặt xuất SVG và tuần tự hóa .txt
│   └── web/                    # Cài đặt máy chủ HTTP Backend
├── tests/                      # Bộ kiểm thử tự động (Unit Test Suites)
│   ├── test_shapes.cpp
│   ├── test_group.cpp
│   ├── test_undo_redo.cpp
│   ├── test_task9.cpp
│   └── test_auth.cpp
├── web/                        # Giao diện Web CAD Studio
│   ├── index.html              # Cấu trúc giao diện Web Studio & Modal
│   ├── style.css               # Phong cách thiết kế Dark/Light Theme
│   └── app.js                  # Logic tương tác đồ họa, vẽ canvas & đồng bộ API
└── output/                     # Bản vẽ mẫu SVG và TXT
    ├── sample_draw.svg
    └── sample_draw.txt
```

---

## 📜 Giấy phép & Bản quyền
Dự án được phát triển phục vụ mục đích học thuật và nghiên cứu cho môn học **Lập trình Hướng đối tượng & Cấu trúc Dữ liệu**. Toàn bộ mã nguồn mở tuân thủ giấy phép MIT.
