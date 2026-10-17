# ĐỒ ÁN CƠ SỞ LẬP TRÌNH (PBL2)
## ĐỀ TÀI 24: HỆ THỐNG QUẢN LÝ THÍ SINH DỰ THI VÀO MỘT TRƯỜNG ĐẠI HỌC
* **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)
* **Khoa**: Công nghệ Thông tin
* **Học phần**: PBL2 - Đồ án cơ sở lập trình

---

## 📌 1. Giới thiệu đề tài
Hệ thống phần mềm hỗ trợ công tác tuyển sinh và quản lý thông tin các thí sinh dự thi vào trường đại học, phục vụ việc nhập liệu, xét tuyển, thống kê phổ điểm và xuất kết quả xét tuyển.

Dự án được xây dựng dựa trên nguyên lý **Lập trình Hướng đối tượng (OOP)** vững chắc, sử dụng **cấu trúc dữ liệu mảng động tự cài đặt (Template `Vector<T>`)** và giao diện dòng lệnh tương tác chuyên nghiệp.

---

## 🏗️ 2. Kiến trúc Hướng đối tượng (OOP)

```
       +----------------------------------------------------+
       |                     Nguoi                          |
       |----------------------------------------------------|
       | # soCMND: string                                   |
       | # hoTen: string                                    |
       | # ngaySinh: string                                 |
       | # queQuan: string                                  |
       | # gioiTinh: string                                 |
       |----------------------------------------------------|
       | + nhap(), xuat() (virtual)                         |
       | + operator>>, operator<<                           |
       +----------------------------------------------------+
                                 ▲
                                 │ (Kế thừa - Inheritance)
                                 │
       +----------------------------------------------------+
       |                    ThiSinh                         |
       |----------------------------------------------------|
       | - soBaoDanh: string                                |
       | - diemToan, diemLy, diemHoa: double                |
       | - diemUuTien: double                               |
       |----------------------------------------------------|
       | + tinhTongDiem(): double                           |
       | + tinhDiemBaMon(): double                          |
       | + xepLoaiHocLuc(): string                          |
       | + xetTrungTuyen(diemChuan): bool                   |
       | + override nhap(), xuat()                          |
       +----------------------------------------------------+
```

### Các đặc trưng OOP được áp dụng:
1. **Tính Đóng gói (Encapsulation)**:
   - Các thuộc tính đều được che giấu với phạm vi `protected` hoặc `private`.
   - Cung cấp các phương thức Getters/Setters kèm kiểm tra tính hợp lệ dữ liệu.
2. **Tính Kế thừa (Inheritance)**:
   - Lớp `ThiSinh` kế thừa công khai (`public`) từ lớp cơ sở `Nguoi`.
3. **Tính Đa hình (Polymorphism)**:
   - Sử dụng phương thức ảo `virtual void nhap()`, `virtual void xuat()` và hàm hủy ảo `virtual ~Nguoi()`.
4. **Nạp chồng toán tử (Operator Overloading)**:
   - Nạp chồng luồng vào/ra: `operator>>`, `operator<<`.
   - Nạp chồng toán tử so sánh: `operator<`, `operator>`, `operator==` và toán tử gán `operator=`.
5. **Cấu trúc Dữ liệu Tự cài đặt (Data Structures)**:
   - `Vector<T>` Template tự quản lý con trỏ động, cấp phát lại bộ nhớ (`reallocate`), và cài đặt giải thuật `QuickSort` hỗ trợ tùy biến hàm so sánh (comparator).

---

## 🚀 3. Các chức năng chính của chương trình

1. **Quản lý danh sách thí sinh**:
   - Thêm thí sinh mới (tự động kiểm tra trùng lặp SBD, CMND/CCCD, kiểm tra chuẩn hóa họ tên, ngày sinh `dd/mm/yyyy`, điểm số 0 - 10).
   - Hiển thị danh sách dạng bảng căn chỉnh chuẩn mực, có phân trang thông minh.
   - Sửa thông tin thí sinh theo Số báo danh.
   - Xóa thí sinh khỏi hệ thống có xác nhận an toàn (Y/N).

2. **Tìm kiếm thông minh**:
   - Tìm kiếm chính xác theo Số báo danh.
   - Tìm kiếm gần đúng theo Họ và tên (không phân biệt hoa/thường).
   - Tìm kiếm theo Quê quán / Tỉnh thành.
   - Tìm kiếm theo khoảng Tổng điểm xét tuyển.

3. **Sắp xếp linh hoạt**:
   - Sắp xếp theo Tổng điểm giảm dần (Bảng xếp hạng xét tuyển).
   - Sắp xếp theo Thứ tự Tên A - Z.
   - Sắp xếp theo Số báo danh tăng dần.

4. **Thống kê & Xét tuyển Đại học**:
   - **Vinh danh Thủ khoa**: Tự động lọc thí sinh có điểm cao nhất kỳ thi.
   - **Xét trúng tuyển**: Lọc danh sách thí sinh đạt điểm chuẩn và không bị điểm liệt (môn <= 1.0).
   - **Thống kê Phổ điểm**: Phân loại theo tỷ lệ % học lực (Xuất sắc, Giỏi, Khá, Trung bình, Yếu) kèm biểu đồ trực quan.
   - **Thống kê theo Quê quán**: Tổng hợp số lượng và tỷ lệ thí sinh theo từng địa phương.
   - Tùy chỉnh thiết lập Điểm chuẩn trúng tuyển.

5. **Lưu trữ & Báo cáo Tệp tin**:
   - Tự động nạp dữ liệu khi khởi động từ file `data/thisinh.txt`.
   - Tự động lưu lại dữ liệu sau mỗi thao tác cập nhật.
   - Xuất báo cáo kết quả trúng tuyển chính thức ra file `data/ketqua_xettuyen.txt`.

6. **Giao diện Console tương tác**:
   - Hỗ trợ menu điều hướng bằng phím mũi tên **▲ / ▼** (UP / DOWN), chọn bằng **ENTER**, quay lại bằng **ESC**.
   - Màu sắc phân biệt sinh động, hỗ trợ hiển thị tiếng Việt trên Windows Console.

---

## 📁 4. Cấu trúc thư mục dự án

```
pbl2_MyProJect/
├── data/
│   ├── thisinh.txt                # Tệp dữ liệu thí sinh (đã có sẵn dữ liệu mẫu)
│   └── ketqua_xettuyen.txt        # Tệp báo cáo kết quả xét tuyển xuất ra
│
├── include/
│   ├── Vector.h                   # Template Vector mảng động tự cài đặt
│   ├── Nguoi.h                    # Khai báo lớp cơ sở Nguoi
│   ├── ThiSinh.h                  # Khai báo lớp ThiSinh kế thừa Nguoi
│   ├── QuanLyThiSinh.h            # Khai báo lớp quản lý nghiệp vụ QuanLyThiSinh
│   ├── ConsoleUI.h                # Khai báo module giao diện Console tương tác
│   └── Utils.h                    # Khai báo các tiện ích kiểm tra & chuẩn hóa
│
├── src/
│   ├── Nguoi.cpp                  # Cài đặt phương thức lớp Nguoi
│   ├── ThiSinh.cpp                # Cài đặt phương thức lớp ThiSinh
│   ├── QuanLyThiSinh.cpp          # Cài đặt phương thức lớp QuanLyThiSinh
│   ├── ConsoleUI.cpp              # Cài đặt giao diện điều khiển ConsoleUI
│   ├── Utils.cpp                  # Cài đặt kiểm tra hợp lệ dữ liệu
│   └── main.cpp                   # Hàm main và vòng lặp Menu chính
│
├── QuanLyThiSinh.dev              # Tệp dự án Dev-C++ (Mở trực tiếp bằng Dev-C++)
├── build.bat                      # File script tự động tìm kiếm compiler và biên dịch
├── Makefile                       # Tệp Makefile cho make / mingw32-make
└── README.md                      # Tài liệu hướng dẫn sử dụng và báo cáo
```

---

## 💻 5. Hướng dẫn Biên dịch & Chạy chương trình

### Cách 1: Sử dụng Dev-C++ (Khuyên dùng cho sinh viên DUT)
1. Mở phần mềm **Dev-C++**.
2. Chọn `File` -> `Open Project or File...` và chọn file `QuanLyThiSinh.dev`.
3. Nhấn phím `F11` (hoặc vào menu `Execute` -> `Compile & Run`).

### Cách 2: Chạy file `build.bat`
* Nhấp đúp chuột vào file `build.bat` trong thư mục dự án. Script sẽ tự động tìm kiếm trình biên dịch `g++` có sẵn trên máy để biên dịch và chạy file `QuanLyThiSinh.exe`.

### Cách 3: Biên dịch thủ công bằng dòng lệnh g++ (MinGW)
Mở Command Prompt / PowerShell tại thư mục `pbl2_MyProJect` và gõ:
```bash
g++ -std=c++11 -Iinclude src/Nguoi.cpp src/ThiSinh.cpp src/QuanLyThiSinh.cpp src/ConsoleUI.cpp src/Utils.cpp src/main.cpp -o QuanLyThiSinh.exe
.\QuanLyThiSinh.exe
```

---
*Tác giả: Sinh viên thực hiện đề tài 24 - PBL2 Đại học Bách khoa Đà Nẵng*
