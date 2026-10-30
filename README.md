# ĐỒ ÁN CƠ SỞ LẬP TRÌNH (PBL2)
## ĐỀ TÀI 24: QUẢN LÝ THÍ SINH DỰ THI VÀO MỘT TRƯỜNG ĐẠI HỌC
* **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)
* **Khoa**: Công nghệ Thông tin
* **Học phần**: PBL2 - Đồ án cơ sở lập trình
* **Sinh viên thực hiện**: Võ Như Hiếu
* **Điểm nổi bật**: 
  - Tuân thủ chính xác 100% đề tài 24 với 2 lớp OOP chuẩn: `Nguoi` và `ThiSinh`.
  - Tự cài đặt cấu trúc dữ liệu `Vector<T>` động (Rule of Five + Thuật toán QuickSort phân hoạch Lomuto/Hoare).
  - Quản lý dữ liệu phân tách theo nguyên lý **Class Table Inheritance** của môn Cơ sở dữ liệu: lưu trữ trên nhiều tệp tin độc lập với định dạng Key-Value chuẩn mực (`nguoi.txt`, `thisinh.txt`, `system.txt`).
  - Hệ thống báo cáo thống kê độc lập: kết quả xét tuyển, vinh danh thủ khoa, thống kê phổ điểm theo khoảng.
  - Giao diện kép hoàn hảo: **Dear ImGui (DirectX 9)** hiện đại và **Console tương tác phím mũi tên**.

---

## 📌 1. Kiến trúc Hướng đối tượng (OOP) theo Đề tài 24

Mô hình kế thừa OOP chuẩn xác tuyệt đối, không dư thừa thực thể ngoại vi:
1. **Lớp Cơ sở `Nguoi`**:
   - Thuộc tính (`protected`): `soCCCD` (Khóa chính PK, định danh duy nhất 12 chữ số), `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`.
   - Phương thức: Constructor, Destructor ảo `virtual ~Nguoi()`, các toán tử xuất nhập luồng `operator>>`, `operator<<`.
2. **Lớp Dẫn xuất `ThiSinh`** (Kế thừa công khai từ `Nguoi`):
   - Thuộc tính bổ sung: `soBaoDanh` (Khóa ứng viên, định dạng `DUTxxx`), `diemToan`, `diemLy`, `diemHoa`.
   - Nghiệp vụ:
     - `tinhTongDiem()`: Tổng điểm 3 môn làm tròn 2 chữ số thập phân.
     - `kiemTraDiemLiet()`: Kiểm tra môn thi $\le 1.0$ điểm.
     - `xetTrungTuyen(diemChuan)`: Đạt điểm chuẩn và không bị điểm liệt.
3. **Cấu trúc Dữ liệu `Vector<T>`**:
   - Template mảng động tự quản lý bộ nhớ, tránh phụ thuộc vào thư viện STL `std::vector`.
   - Hỗ trợ Rule of Five (Copy/Move Constructor & Assignment).
   - Tích hợp giải thuật sắp xếp **QuickSort** với con trỏ hàm so sánh (`O(N log N)`).

---

## 🗄️ 2. Mô hình Cơ sở Dữ liệu & Quản lý Nhiều Tệp Tin

Hệ thống áp dụng kiến trúc **Class Table Inheritance** trong môn Cơ sở Dữ liệu: dữ liệu lớp cha và lớp con được tách thành các bảng (files) riêng biệt, liên kết với nhau bằng Khóa ngoại `soCCCD`:

1. **`data/nguoi.txt` (Thực thể Nhân thân)**:
   - Lưu trữ các thuộc tính của `Nguoi`: `soCCCD`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`.
   - Định dạng Key-Value: Mỗi bản ghi phân cách bằng `---`, kết thúc tệp bằng `====`.
2. **`data/thisinh.txt` (Thực thể Dự thi)**:
   - Lưu trữ các thuộc tính của `ThiSinh`: `soBaoDanh`, `soCCCD` (FK), `diemToan`, `diemLy`, `diemHoa`.
   - Liên kết tự động (Join) vào thông tin nhân thân thông qua `soCCCD`.
3. **`data/system.txt` (Cấu hình Tuyển sinh)**:
   - Lưu trữ thông tin kỳ thi: `Truong`, `Nam Tuyen Sinh`, `Diem Chuan`.
4. **Các tệp xuất báo cáo độc lập**:
   - `data/ketqua_xettuyen.txt`: Bảng báo cáo danh sách thí sinh, điểm số và kết quả Trúng tuyển / Không trúng tuyển.
   - `data/thukhoa.txt`: Báo cáo vinh danh Thủ khoa của trường đạt tổng điểm cao nhất.
   - `data/thongke_phodiem.txt`: Báo cáo phân tích phổ điểm thi theo từng khoảng mức điểm.

---

## 💻 3. Giao diện Người dùng Hiện đại (Dear ImGui & DirectX 9)

- **Số báo danh (SBD)**: Tự động tăng (`DUT001`, `DUT002`,...), trường nhập được làm mờ (disabled) để đảm bảo toàn vẹn.
- **Nhập điểm số**: Sử dụng ô nhập số chuẩn `InputFloat` (bước nhảy 0.25) thay cho thanh kéo, nhập chính xác từ bàn phím.
- **Ngày sinh chuẩn hóa**: Hộp điều khiển Ngày (1-31), Tháng (1-12), Năm (1970-2015) tự động kiểm tra năm nhuận và chuyển đổi sang định dạng `dd/mm/yyyy`.
- **Quê quán chuẩn**: Dropdown Combo box chọn nhanh tỉnh/thành phố theo quy chuẩn Việt Nam.
- **Tab Thống kê & Phổ điểm**: Trực quan hóa tỷ lệ trúng tuyển, phân bố điểm bằng biểu đồ thanh (Progress bars) và banner vinh danh Thủ khoa.
- **Tab Lưu trữ & Xem Báo cáo**: Tích hợp trình xem nội dung báo cáo trực tiếp trong ứng dụng.

*(Chương trình hỗ trợ chế độ Console khi chạy tham số: `.\QuanLyThiSinh_PBL2.exe --console`)*

---

## 🚀 4. Hướng dẫn Biên dịch & Khởi chạy

### Cách 1: Sử dụng file `build.bat` (Khuyên dùng)
Nhấp đúp vào file **`build.bat`** để script tự động thiết lập PATH, biên dịch và khởi chạy chương trình.

### Cách 2: Dòng lệnh thủ công (MinGW 64-bit)
```bash
g++ -std=c++11 -O2 -Iinclude -Iinclude/imgui src/Nguoi.cpp src/ThiSinh.cpp src/QuanLyThiSinh.cpp src/ConsoleUI.cpp src/Utils.cpp src/AppGUI.cpp src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_win32.cpp src/imgui/imgui_impl_dx9.cpp src/main.cpp -ld3d9 -lgdi32 -ldwmapi -o QuanLyThiSinh_PBL2.exe

# Chạy giao diện đồ họa GUI:
.\QuanLyThiSinh_PBL2.exe

# Hoặc chạy giao diện Console tương tác phím:
.\QuanLyThiSinh_PBL2.exe --console
```

---
*Tác giả: Võ Như Hiếu - Sinh viên thực hiện Đề tài 24, PBL2 Đồ án Cơ sở Lập trình, Đại học Bách khoa Đà Nẵng*
