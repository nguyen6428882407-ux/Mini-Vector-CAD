#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <iostream>

// ============================================================================
// BanGhiLichSuVe - Đóng gói một bản ghi lịch sử nét vẽ trong Mini-Vector-CAD
// Theo yêu cầu: Lưu lại từng chức năng của nét vẽ, thời gian vẽ và tài khoản vẽ
// ============================================================================
struct BanGhiLichSuVe {
    std::string thoiGianVe;   // Thời gian thực hiện nét vẽ (YYYY-MM-DD HH:MM:SS)
    std::string taiKhoanVe;   // Tên tài khoản thực hiện nét vẽ (username)
    std::string chucNang;     // Tên chức năng nét vẽ (Vẽ đoạn thẳng, Vẽ hình tròn, ...)
    int idHinh = -1;          // ID đối tượng hình học
    std::size_t layerIdx = 0; // Chỉ số Layer chứa nét vẽ
    std::string chiTiet;      // Chi tiết thông số nét vẽ (tọa độ, kích thước, ...)

    BanGhiLichSuVe() = default;
    BanGhiLichSuVe(std::string tg,
                   std::string tk,
                   std::string cn,
                   int id = -1,
                   std::size_t lay = 0,
                   std::string ct = "")
        : thoiGianVe(std::move(tg)),
          taiKhoanVe(std::move(tk)),
          chucNang(std::move(cn)),
          idHinh(id),
          layerIdx(lay),
          chiTiet(std::move(ct)) {}

    // In tóm tắt thông tin bản ghi thành một chuỗi
    std::string moTa() const;
};

// ============================================================================
// LichSuNetVeStack - Quản lý lịch sử các nét vẽ theo cơ chế Stack (LIFO)
// Tuân thủ nguyên lý Đóng gói OOP và cấu trúc dữ liệu Ngăn xếp (DSA)
// ============================================================================
class LichSuNetVeStack {
private:
    std::vector<BanGhiLichSuVe> stack_; // Ngăn xếp lưu trữ nét vẽ (LIFO)

public:
    LichSuNetVeStack() = default;

    // Tiện ích sinh chuỗi thời gian hệ thống hiện tại định dạng YYYY-MM-DD HH:MM:SS
    static std::string layThoiGianHienTai();

    // Đẩy (push) một bản ghi nét vẽ vào đỉnh Stack
    void push(const BanGhiLichSuVe& banGhi);

    // Thêm nét vẽ với thời gian tự động tạo theo thời gian thực hệ thống
    void themNetVe(const std::string& chucNang,
                   const std::string& taiKhoanVe,
                   int idHinh = -1,
                   std::size_t layerIdx = 0,
                   const std::string& chiTiet = "");

    // Rút (pop) nét vẽ trên đỉnh Stack ra ngoài (nét vẽ gần nhất)
    bool pop(BanGhiLichSuVe& outBanGhi);
    bool pop();

    // Xem thông tin nét vẽ ở đỉnh Stack mà không rút ra
    const BanGhiLichSuVe* top() const;

    // Kiểm tra Stack có đang rỗng không
    bool empty() const;

    // Lấy số lượng nét vẽ đang có trong Stack
    std::size_t size() const;

    // Xóa sạch toàn bộ lịch sử trong Stack
    void clear();

    // Lấy danh sách toàn bộ nét vẽ theo thứ tự từ đỉnh Stack xuống đáy (mới nhất -> cũ nhất)
    std::vector<BanGhiLichSuVe> layDanhSachTuDinh() const;

    // Lấy tham chiếu đến vector lưu trữ bên dưới
    const std::vector<BanGhiLichSuVe>& layToanBo() const;

    // In toàn bộ lịch sử nét vẽ dạng bảng có căn lề đẹp mắt ra luồng xuất
    void inLichSu(std::ostream& os) const;

    // Xuất lịch sử nét vẽ ra tệp tin văn bản
    bool xuatFile(const std::string& path) const;

    // Nạp lịch sử nét vẽ từ tệp tin văn bản
    bool napFile(const std::string& path);
};
