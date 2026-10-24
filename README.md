# ĐỒ ÁN CƠ SỞ LẬP TRÌNH (PBL2)
## ĐỀ TÀI 24: HỆ THỐNG QUẢN LÝ THÍ SINH DỰ THI VÀO MỘT TRƯỜNG ĐẠI HỌC
* **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)
* **Khoa**: Công nghệ Thông tin
* **Học phần**: PBL2 - Đồ án cơ sở lập trình
* **Sinh viên thực hiện**: Võ Như Hiếu
* **Điểm nổi bật**: Áp dụng mô hình **Cơ sở Dữ liệu Quan hệ (Relational Database - Chuẩn hóa 3NF)** và **Giao diện Đồ họa Hiện đại Dear ImGui (DirectX 9)**.

---

## 📌 1. Giới thiệu đề tài
Hệ thống phần mềm hỗ trợ toàn diện công tác tuyển sinh đại học: quản lý hồ sơ thí sinh, kiểm tra ràng buộc toàn vẹn dữ liệu, phân loại khu vực ưu tiên, đăng ký nguyện vọng ngành đào tạo, xét tuyển theo điểm chuẩn và chỉ tiêu từng ngành, vinh danh thủ khoa và thống kê phổ điểm.

---

## 🗄️ 2. Mô hình Cơ sở Dữ liệu Quan hệ (Chuẩn hóa 3NF)
Để giải quyết triệt để vấn đề dư thừa dữ liệu và kiểm soát các trường hợp **nhiều thí sinh trùng họ tên**, hệ thống tách biệt thành 4 bảng dữ liệu quan hệ liên kết qua **Khóa chính (PK)** và **Khóa ngoại (FK)**:

1. **Bảng Danh mục Khu vực (`data/danhmuc_khuvuc.txt`)**:
   - `maKhuVuc` (PK): `KV1`, `KV2NT`, `KV2`, `KV3`
   - `tenKhuVuc`: Tên khu vực ưu tiên
   - `diemUuTien`: 0.75, 0.50, 0.25, 0.00
   - *Ưu điểm*: Điểm ưu tiên được tự động suy ra từ mã khu vực, bảo đảm tính toàn vẹn dữ liệu.

2. **Bảng Danh mục Ngành đào tạo (`data/danhmuc_nganh.txt`)**:
   - `maNganh` (PK): `7480201`, `7520114`, `7520207`, `7510301`, `7580201`...
   - `tenNganh`, `toHopMon`, `chiTieu`, `diemChuan`
   - *Ưu điểm*: Xét tuyển theo điểm chuẩn của từng ngành đăng ký cụ thể.

3. **Bảng Hồ sơ Thí sinh (`data/hoso_thisinh.txt`)**:
   - `maThiSinh` (PK), `soCMND` (Unique), `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`, `maKhuVuc` (FK).
   - *Ưu điểm*: **Khóa chính `maThiSinh` là duy nhất**. Thí sinh trùng họ tên (như 2 thí sinh cùng tên "Nguyễn Văn An") được quản lý độc lập, tuyệt đối không bị nhầm lẫn.

4. **Bảng Kết quả thi & Nguyện vọng (`data/dangky_thi.txt`)**:
   - `soBaoDanh` (PK), `maThiSinh` (FK), `maNganh` (FK), `diemToan`, `diemLy`, `diemHoa`.

---

## 🏗️ 3. Kiến trúc Hướng đối tượng (OOP) & Cấu trúc Dữ liệu

1. **Lớp cơ sở `Nguoi`**: Đóng gói thông tin nhân thân (`protected`), các phương thức ảo `virtual void nhap()`, `virtual void xuat()` và hàm hủy ảo `virtual ~Nguoi()`.
2. **Lớp dẫn xuất `ThiSinh` kế thừa `Nguoi`**: Bổ sung `maThiSinh` (PK), `soBaoDanh`, `maKhuVuc` (FK), `maNganh` (FK), các hàm tính tổng điểm, kiểm tra điểm liệt ($\le 1.0$), xếp loại học lực, xét trúng tuyển theo điểm chuẩn ngành.
3. **Các lớp thực thể `KhuVuc` & `NganhXetTuyen`**: Đóng gói danh mục CSDL.
4. **Cấu trúc Dữ liệu Tự cài đặt `Vector<T>` Template**: Tự quản lý con trỏ động, cấp phát lại bộ nhớ nhân đôi, tuân thủ Rule of Three và tích hợp thuật toán phân hoạch **QuickSort** với con trỏ hàm so sánh ($O(N \log N)$).

---

## 💻 4. Giao diện Đồ họa Hiện đại (Dear ImGui & DirectX 9)

Chương trình được nâng cấp lên giao diện đồ họa cửa sổ Desktop hoàn chỉnh:
- **Tab 1: Quản lý Hồ sơ Thí sinh**: Bảng dữ liệu có thanh cuộn mượt mà, bộ lọc tức thời (theo SBD, Tên, Ngành, Trạng thái), các nút Thêm/Sửa/Xóa mở cửa sổ Modal Dialog nhập liệu trực quan kèm Validate.
- **Tab 2: Cơ sở Dữ liệu Quan hệ 3NF**: Hiển thị bảng Khu vực và bảng Ngành đào tạo; cho phép điều chỉnh trực tiếp Điểm chuẩn và Chỉ tiêu ngành ngay trên màn hình.
- **Tab 3: Thống kê & Phổ điểm & Thủ khoa**: Banner vàng vinh danh Thủ khoa kỳ thi, thủ khoa từng ngành, biểu đồ thanh trực quan (Progress Bars) thể hiện tỷ lệ % phổ điểm.
- **Tab 4: Lưu trữ & Xuất Báo cáo**: Nút lưu 4 file CSDL và xem trước file báo cáo `ketqua_xettuyen.txt` trực tiếp trong ứng dụng.
- **Tab 5: Hướng dẫn & Kiến trúc**: Tài liệu đồ án và kiến trúc hệ thống.

*(Chương trình vẫn hỗ trợ chế độ Console dự phòng khi chạy lệnh: `QuanLyThiSinh.exe --console`)*

---

## 🚀 5. Hướng dẫn Biên dịch & Khởi chạy

### Cách 1: Chạy file `build.bat` (Khuyên dùng)
- Nhấp đúp chuột vào file **`build.bat`**.
- Script sẽ tự động tìm kiếm trình biên dịch `g++`, tự động nạp DLL, biên dịch toàn bộ mã nguồn và khởi chạy file `QuanLyThiSinh.exe`.

### Cách 2: Sử dụng Dev-C++
1. Mở phần mềm **Dev-C++**.
2. Chọn `File` -> `Open Project or File...` và chọn file `QuanLyThiSinh.dev`.
3. Nhấn phím `F11` (hoặc vào menu `Execute` -> `Compile & Run`).

### Cách 3: Biên dịch thủ công bằng dòng lệnh g++ (MinGW)
```bash
g++ -std=c++11 -O2 -Iinclude -Iinclude/imgui src/Nguoi.cpp src/ThiSinh.cpp src/KhuVuc.cpp src/NganhXetTuyen.cpp src/QuanLyThiSinh.cpp src/ConsoleUI.cpp src/Utils.cpp src/AppGUI.cpp src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_win32.cpp src/imgui/imgui_impl_dx9.cpp src/main.cpp -ld3d9 -lgdi32 -ldwmapi -o QuanLyThiSinh.exe
.\QuanLyThiSinh.exe
```

---
*Tác giả: Sinh viên thực hiện đề tài 24 - PBL2 Đại học Bách khoa Đà Nẵng*
