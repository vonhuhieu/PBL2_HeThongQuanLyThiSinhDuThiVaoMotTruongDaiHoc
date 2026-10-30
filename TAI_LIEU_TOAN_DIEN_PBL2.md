# TÀI LIỆU TOÀN DIỆN ĐỒ ÁN PBL2: HỆ THỐNG QUẢN LÝ THÍ SINH DỰ THI ĐẠI HỌC
> **Học phần**: PBL 2 - Đồ án cơ sở lập trình  
> **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)  
> **Khoa**: Công nghệ Thông tin  
> **Đề tài số 24**: Quản lý các thí sinh dự thi vào một trường Đại học  
> **Tác giả thực hiện**: Võ Như Hiếu  
> **Kiến trúc**: Class Table Inheritance (Mô hình CSDL Quan hệ Kế thừa) & Giao diện Kép (Dear ImGui DirectX 9 + Console)

---

## MỤC LỤC
1. [Tổng Quan Đề Tài 24 & Chuẩn Hóa Yêu Cầu Giảng Viên](#1-tổng-quan-đề-tài-24--chuẩn-hóa-yêu-cầu-giảng-viên)
2. [Thiết Kế Cơ Sở Dữ Liệu & Quản Lý Nhiều Tệp Tin](#2-thiết-kế-cơ-sở-dữ-liệu--quản-lý-nhiều-tệp-tin)
3. [Kiến Trúc Hướng Đối Tượng (OOP) Chuẩn 2 Lớp](#3-kiến-trúc-hướng-đối-tượng-oop-chuẩn-2-lớp)
4. [Cấu Trúc Dữ Liệu Tự Cài Đặt: Template Vector<T> & QuickSort](#4-cấu-trúc-dữ-liệu-tự-cài-đặt-template-vectort--quicksort)
5. [Giao Diện Đồ Họa Cửa Sổ Hiện Đại (Dear ImGui & DirectX 9)](#5-giao-diện-đồ-họa-cửa-sổ-hiện-đại-dear-imgui--directx-9)
6. [Kịch Bản 5 Phút Báo Cáo Trình Diễn Trước Giảng Viên](#6-kịch-bản-5-phút-báo-cáo-trình-diễn-trước-giảng-viên)
7. [Bộ 10 Câu Hỏi Vấn Đáp Thường Gặp & Câu Trả Lời Đạt Điểm A+](#7-bộ-10-câu-hỏi-vấn-đáp-thường-gặp--câu-trả-lời-đạt-điểm-a)

---

## 1. TỔNG QUAN ĐỀ TÀI 24 & CHUẨN HÓA YÊU CẦU GIẢNG VIÊN

### 1.1. Yêu cầu nguyên bản của Đề tài 24
- Xây dựng chương trình quản lý thí sinh dự thi vào một trường Đại học.
- Lớp đối tượng `NGUOI`: Số chứng minh thư / CCCD, Tên, Ngày sinh, Quê quán, Giới tính.
- Lớp đối tượng `THI SINH` kế thừa từ `NGUOI`: Số báo danh, Điểm toán, Điểm lý, Điểm hóa.
- Quản lý danh sách, tìm kiếm, sắp xếp, xét trúng tuyển theo điểm chuẩn, vinh danh thủ khoa và thống kê phổ điểm.

### 1.2. Khắc phục triệt để các nhược điểm của phiên bản cũ
- **Không nhồi nhét lớp thừa**: Loại bỏ hoàn toàn các lớp ngoại vi như `KhuVuc` hay `NganhXetTuyen` để tuân thủ chính xác 100% đề tài.
- **Không định danh kép dư thừa**: Không dùng thêm `maThiSinh`, sử dụng `soCCCD` (12 chữ số) làm khóa định danh thực thể nhân thân và `soBaoDanh` (dạng `DUTxxx`) làm khóa định danh dự thi.
- **Áp dụng kiến thức môn Cơ sở Dữ liệu**: Không lưu dồn tất cả dữ liệu vào 1 file duy nhất `thisinh.txt`. Tách biệt thành nhiều file chuyên biệt theo mô hình **Class Table Inheritance** và chuẩn hóa lưu trữ Key-Value tương tự các đồ án xuất sắc.

---

## 2. THIẾT KẾ CƠ SỞ DỮ LIỆU & QUẢN LÝ NHIỀU TỆP TIN

### 2.1. Mô hình Class Table Inheritance (Kế thừa bảng)
Khi chuyển đổi từ mô hình Hướng đối tượng sang Lưu trữ Dữ liệu Quan hệ, phương pháp **Class Table Inheritance** được giảng viên đánh giá cao nhất:
- Lớp cha `Nguoi` được lưu trữ tại bảng/file riêng: `data/nguoi.txt`
- Lớp con `ThiSinh` được lưu trữ tại bảng/file riêng: `data/thisinh.txt`
- Khóa chính `soCCCD` ở lớp cha đóng vai trò là Khóa ngoại (Foreign Key) ở lớp con.
- Cấu hình chung của kỳ thi được tách độc lập tại `data/system.txt`.

### 2.2. Chi tiết các tệp dữ liệu

1. **`data/nguoi.txt` (Thực thể Nhân thân)**:
```text
#NGUOI
CCCD: 048205001234
Ho Ten: NGUYEN VAN AN
Ngay Sinh: 15/05/2005
Que Quan: Da Nang
Gioi Tinh: Nam
---
CCCD: 048205001235
Ho Ten: TRAN THI MAI
Ngay Sinh: 22/07/2005
Que Quan: Quang Nam
Gioi Tinh: Nu
====
```

2. **`data/thisinh.txt` (Thực thể Dự thi & Điểm số)**:
```text
#THISINH
SBD: DUT001
CCCD: 048205001234
Diem Toan: 8.50
Diem Ly: 9.00
Diem Hoa: 8.75
---
SBD: DUT002
CCCD: 048205001235
Diem Toan: 8.75
Diem Ly: 8.50
Diem Hoa: 9.25
====
```

3. **`data/tinhthanh.txt` (Danh mục 34 Tỉnh / Thành phố mới nhất)**:
- Danh mục chuẩn hóa Master Lookup Table theo **Nghị quyết 202/2025/QH15** (28 tỉnh và 6 thành phố trực thuộc Trung ương).
- Ngăn ngừa nhập tùy tiện, hỗ trợ Searchable Combobox tìm kiếm tức thời trên giao diện đồ họa.

4. **`data/system.txt` (Cấu hình Kỳ thi Tuyển sinh)**:
```text
#SYSTEM_CONFIG
Truong: TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG
Nam Tuyen Sinh: 2026
Diem Chuan: 22.00
====
```

5. **Hệ thống tệp báo cáo chuyên biệt (`data/*.txt`)**:
- `data/ketqua_xettuyen.txt`: Bảng tổng kết danh sách trúng tuyển.
- `data/thukhoa.txt`: Báo cáo vinh danh Thủ khoa của trường.
- `data/thongke_phodiem.txt`: Phân tích phổ điểm thi theo 5 khoảng điểm.

*(Tổng cộng hệ thống quản trị **7 tệp tin .txt chuyên biệt**, hoàn toàn tương đồng với cấu trúc đa tệp của đồ án mẫu).*

---

## 3. KIẾN TRÚC HƯỚNG ĐỐI TƯỢNG (OOP) CHUẨN 2 LỚP

```
          +-----------------------------------------+
          |                  Nguoi                  |
          +-----------------------------------------+
          | # soCCCD: string (12 số)               |
          | # hoTen: string                         |
          | # ngaySinh: string (dd/mm/yyyy)         |
          | # queQuan: string                       |
          | # gioiTinh: string                      |
          +-----------------------------------------+
          | + Nguoi(...)                            |
          | + virtual ~Nguoi()                      |
          | + getter / setter                       |
          | + operator>> , operator<<               |
          +-----------------------------------------+
                               ▲
                               │  kế thừa công khai
                               │
          +-----------------------------------------+
          |                 ThiSinh                 |
          +-----------------------------------------+
          | - soBaoDanh: string (DUTxxx)            |
          | - diemToan: float                       |
          | - diemLy: float                         |
          | - diemHoa: float                        |
          +-----------------------------------------+
          | + ThiSinh(...)                          |
          | + tinhTongDiem(): float                 |
          | + kiemTraDiemLiet(): bool               |
          | + xetTrungTuyen(diemChuan): bool        |
          | + xepLoaiHocLuc(): string               |
          | + getter / setter                       |
          | + operator>> , operator<<               |
          +-----------------------------------------+
```

---

## 4. CẤU TRÚC DỮ LIỆU TỰ CÀI ĐẶT: TEMPLATE VECTOR<T> & QUICKSORT

1. **Tuân thủ quy chuẩn không dùng `std::vector` của STL**:
   - Tự quản lý con trỏ vùng nhớ động `T* data`, `size_t sz`, `size_t cap`.
   - Cấp phát nhân đôi dung lượng khi đầy (`cap = cap == 0 ? 4 : cap * 2`).
2. **Hỗ trợ Rule of Five**:
   - Destructor: giải phóng bộ nhớ an toàn.
   - Copy Constructor & Copy Assignment: Deep Copy tránh lỗi dangling pointer.
   - Move Constructor & Move Assignment: Tối ưu chuyển quyền sở hữu bộ nhớ trong C++11.
3. **Tích hợp giải thuật QuickSort**:
   - Phân hoạch Lomuto/Hoare với con trỏ hàm so sánh: `bool (*comp)(const T&, const T&)`.
   - Sắp xếp đa tiêu chí: Tổng điểm giảm dần, Họ tên A-Z, Số báo danh tăng dần.

---

## 5. GIAO DIỆN ĐỒ HỌA CỬA SỔ HIỆN ĐẠI (DEAR IMGUI & DIRECTX 9)

Các cải tiến giao diện chuẩn mực theo phản hồi thực tế:
1. **Bố cục Form 2 Cột chuyên nghiệp**: Nhãn (Label) căn lề trái thẳng thớm, ô nhập liệu (Input) căn lề phải rộng rãi, đồng bộ `AlignTextToFramePadding()`.
2. **Số báo danh (SBD) tự tăng**: Sinh mã tự động `DUT001`, `DUT002`... Khóa làm mờ (disabled) để bảo toàn tính toàn vẹn thực thể.
3. **Ngày sinh chuẩn `dd/mm/yyyy`**: 3 ô số Ngày (1-31), Tháng (1-12), Năm (1970-2015) không nút bấm rườm rà, nhập số trực tiếp mượt mà, tự động ghép chuỗi `dd/mm/yyyy`.
4. **Quê quán Searchable Combobox**: Danh mục 34 Tỉnh/Thành phố theo Nghị quyết 202/2025/QH15, có thanh gõ tìm kiếm lọc theo thời gian thực (real-time).
5. **Giới tính căn chỉnh hoàn hảo**: Radio button Nam / Nữ căn chỉnh cùng hàng ngang với nhãn.
6. **Nhập điểm số thuần túy (Clean InputFloat)**: Loại bỏ các nút `+` và `-` rườm rà, chỉ cho phép nhập số thực từ bàn phím với định dạng `%.2f`.
7. **Trình duyệt báo cáo**: Đọc và xem trực tiếp các tệp báo cáo `ketqua_xettuyen.txt`, `thukhoa.txt`, `thongke_phodiem.txt` ngay trong ứng dụng.
5. **Trình duyệt báo cáo**: Đọc và xem trực tiếp các tệp báo cáo `ketqua_xettuyen.txt`, `thukhoa.txt`, `thongke_phodiem.txt` ngay trong ứng dụng.

---

## 6. KỊCH BẢN 5 PHÚT BÁO CÁO TRÌNH DIỄN TRƯỚC GIẢNG VIÊN

- **Phút 1: Khởi động & Giới thiệu đề tài 24**:
  Mở `QuanLyThiSinh_PBL2.exe`. Giới thiệu 2 lớp OOP chuẩn `Nguoi` và `ThiSinh`.
- **Phút 2: Minh chứng mô hình Cơ sở Dữ liệu**:
  Mở thư mục `data/` cho thầy xem: `nguoi.txt`, `thisinh.txt`, `system.txt`. Giải thích nguyên lý **Class Table Inheritance** và liên kết bằng khóa `soCCCD`.
- **Phút 3: Trình diễn Thêm thí sinh với Form chuẩn hóa**:
  Nhấn **"Them Thi Sinh Moi"**: Chỉ ra SBD tự động nhảy tăng `DUT012`, ngày sinh chọn số tự kiểm tra năm nhuận, quê quán chọn combobox tỉnh thành, điểm thi nhập số bước 0.25.
- **Phút 4: Sắp xếp QuickSort & Xét tuyển**:
  Click đổi tiêu chí sắp xếp: Theo Điểm giảm dần, Theo Tên A-Z. Chỉ ra giải thuật QuickSort trên cấu trúc `Vector<T>` tự viết đang thực thi.
- **Phút 5: Xuất Báo cáo & Tab Thống kê**:
  Mở Tab 3 cho thầy xem biểu đồ phổ điểm và Thủ khoa. Nhấn Tab 4 xuất và xem nội dung các file báo cáo trực tiếp.

---

## 7. BỘ 10 CÂU HỎI VẤN ĐÁP THƯỜNG GẶP & CÂU TRẢ LỜI ĐẠT ĐIỂM A+

1. **Hỏi**: Tại sao em không dùng `std::vector` mà phải tự viết `Vector<T>`?  
   *Trả lời*: Thưa thầy, học phần PBL2 đánh giá năng lực cơ sở lập trình và cấu trúc dữ liệu. Em tự cài đặt `Vector<T>` với kỹ thuật quản lý bộ nhớ động, tuân thủ Rule of Five và tự tích hợp giải thuật QuickSort với hàm so sánh (comparator) để chứng minh khả năng làm chủ con trỏ và bộ nhớ.

2. **Hỏi**: Tại sao em lại tách ra `nguoi.txt` và `thisinh.txt` thay vì lưu chung 1 file?  
   *Trả lời*: Thưa thầy, theo kiến thức môn Cơ sở Dữ liệu, đây là kỹ thuật **Class Table Inheritance** (Kế thừa bảng). Bảng `Nguoi` lưu trữ thực thể nhân thân với khóa chính `soCCCD`, bảng `ThiSinh` lưu thông tin dự thi và tham chiếu `soCCCD` làm khóa ngoại. Khi cần thay đổi thông tin nhân thân hay mở rộng loại đối tượng khác (như Giảng viên/Cán bộ coi thi cùng kế thừa từ `Nguoi`), hệ thống không làm biến dạng dữ liệu thi cử.

3. **Hỏi**: Nếu 2 thí sinh trùng tên và quê quán thì hệ thống phân biệt thế nào?  
   *Trả lời*: Thưa thầy, hệ thống định danh tuyệt đối bằng `soCCCD` (12 số) ở lớp cha và `soBaoDanh` ở lớp con. Tên hay quê quán chỉ là thuộc tính mô tả, khóa chính là duy nhất nên tuyệt đối không xảy ra xung đột hay nhầm lẫn.

4. **Hỏi**: Tại sao em làm SBD tự tăng và bị khóa (disabled) ở Form thêm mới?  
   *Trả lời*: Nhằm đảm bảo tính toàn vẹn thực thể (Entity Integrity), tránh trường hợp người nhập gõ sai quy cách hoặc vô tình trùng lặp số báo danh đã có.

5. **Hỏi**: Điều kiện xét trúng tuyển của trường là gì?  
   *Trả lời*: Tổng điểm 3 môn Toán, Lý, Hóa $\ge$ Điểm chuẩn của trường (hiện tại là 22.00 và có thể điều chỉnh linh hoạt trên GUI), đồng thời không có môn nào bị điểm liệt ($\le 1.00$).

6. **Hỏi**: Destructor của lớp `Nguoi` có phải là `virtual` không, tại sao?  
   *Trả lời*: Dạ có, `virtual ~Nguoi()` là bắt buộc để khi ta hủy một đối tượng `ThiSinh` thông qua con trỏ lớp cơ sở `Nguoi*`, destructor của lớp con `ThiSinh` sẽ được gọi trước, sau đó mới đến destructor của `Nguoi`, tránh thất thoát rò rỉ bộ nhớ (Memory Leak).

7. **Hỏi**: Độ phức tạp thuật toán QuickSort của em là bao nhiêu?  
   *Trả lời*: Trung bình là $O(N \log N)$, trường hợp xấu nhất là $O(N^2)$. Với số lượng thí sinh dự thi, thuật toán phân hoạch Lomuto/Hoare hoạt động cực nhanh trong $O(N \log N)$.

8. **Hỏi**: Em nạp dữ liệu từ 2 file vào bộ nhớ như thế nào?  
   *Trả lời*: Khi khởi động, hàm `docDuLieu()` đọc toàn bộ nhân thân từ `nguoi.txt` vào một danh sách tạm, sau đó đọc `thisinh.txt` và thực hiện phép nối (In-Memory Hash/Linear Join) trên trường `soCCCD` để hoàn thiện đầy đủ đối tượng `ThiSinh` đưa vào `Vector<ThiSinh>`.

9. **Hỏi**: Nếu người dùng không cài DirectX hoặc máy tính cũ chỉ có Console thì sao?  
   *Trả lời*: Em thiết kế kiến trúc phân tầng độc lập giữa Core Logic và Presentation. Chương trình hỗ trợ cờ lệnh `.\QuanLyThiSinh_PBL2.exe --console` cho phép chạy trên mọi màn hình dòng lệnh với menu tương tác phím mũi tên và hỗ trợ tiếng Việt UTF-8.

10. **Hỏi**: Em đã áp dụng các thư viện nào để dựng GUI?  
    *Trả lời*: Em sử dụng thư viện **Dear ImGui** kết hợp backend **DirectX 9** và Win32 API gốc của Windows, giúp ứng dụng đạt tốc độ hiển thị 60 FPS mượt mà, dung lượng file thực thi nhỏ gọn và không cần cài đặt thêm runtime cồng kềnh.
