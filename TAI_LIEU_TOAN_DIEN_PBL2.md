# TÀI LIỆU TOÀN DIỆN ĐỒ ÁN PBL2: HỆ THỐNG QUẢN LÝ THÍ SINH DỰ THI ĐẠI HỌC
> **Học phần**: PBL 2 - Đồ án cơ sở lập trình  
> **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)  
> **Khoa**: Công nghệ Thông tin  
> **Đề tài số 24**: Quản lý các thí sinh dự thi vào một trường Đại học  
> **Tác giả thực hiện**: Võ Như Hiếu  
> **Phiên bản nâng cấp**: CSDL Quan hệ Chuẩn 3NF & Giao diện Đồ họa Hiện đại Dear ImGui (DirectX 9)  

---

## MỤC LỤC
1. [Tổng Quan Về Dự Án & Bản Chất Học Phần PBL2](#1-tổng-quan-về-dự-án--bản-chất-học-phần-pbl2)
2. [Thiết Kế Cơ Sở Dữ Liệu Quan Hệ (Chuẩn Hóa 3NF & Giải Quyết Trùng Tên)](#2-thiết-kế-cơ-sở-dữ-liệu-quan-hệ-chuẩn-hóa-3nf--giải-quyết-trùng-tên)
3. [Kiến Trúc Hướng Đối Tượng (OOP) & Các Lớp Thực Thể](#3-kiến-trúc-hướng-đối-tượng-oop--các-lớp-thực-thể)
4. [Cấu Trúc Dữ Liệu Tự Viết: Template Vector<T> & QuickSort](#4-cấu-trúc-dữ-liệu-tự-viết-template-vectort--quicksort)
5. [Giao Diện Đồ Họa Cửa Sổ Hiện Đại (Dear ImGui & DirectX 9)](#5-giao-diện-đồ-họa-cửa-sổ-hiện-đại-dear-imgui--directx-9)
6. [Kịch Bản 5 Phút Báo Cáo Trình Diễn Khiến Thầy Hài Lòng Tuyệt Đối](#6-kịch-bản-5-phút-báo-cáo-trình-diễn-khiến-thầy-hài-lòng-tuyệt-đối)
7. [Bộ 10 Câu Hỏi Vấn Đáp Thầy Hay Hỏi Nhất & Câu Trả Lời Đạt Điểm A+](#7-bộ-10-câu-hỏi-vấn-đáp-thầy-hay-hỏi-nhất--câu-trả-lời-đạt-điểm-a)

---

## 1. TỔNG QUAN VỀ DỰ ÁN & BẢN CHẤT HỌC PHẦN PBL2

### 1.1. Mục tiêu đồ án
Đề tài 24 yêu cầu xây dựng hệ thống phần mềm hướng đối tượng bằng C++ để quản lý thông tin các thí sinh tham gia kỳ thi tuyển sinh đại học. Hệ thống giải quyết các bài toán thực tế:
- Quản lý hồ sơ nhân thân, phân loại khu vực ưu tiên và kết quả thi cử.
- Kiểm tra tính toàn vẹn dữ liệu (Validate SBD, CMND/CCCD, ngày sinh, điểm số 0-10).
- Áp dụng kiến thức môn **Cơ sở dữ liệu (CSDL)**: Tách các thực thể quan hệ, chuẩn hóa 3NF, liên kết Khóa chính (PK) và Khóa ngoại (FK) qua hệ thống nhiều tệp tin.
- Xét trúng tuyển theo **điểm chuẩn và chỉ tiêu của từng ngành đào tạo**, vinh danh thủ khoa toàn trường và thủ khoa từng ngành.
- Cung cấp **Giao diện Đồ họa Người dùng (GUI)** hiện đại bằng thư viện **Dear ImGui (DirectX 9)** đạt chuẩn công nghiệp, loại bỏ giao diện dòng lệnh console cũ kỹ.

---

## 2. THIẾT KẾ CƠ SỞ DỮ LIỆU QUAN HỆ (CHUẨN HÓA 3NF & GIẢI QUYẾT TRÙNG TÊN)

### 2.1. Phê bình của giảng viên & Bản chất vấn đề
Trước đây, dữ liệu bị dồn vào **1 file duy nhất (`thisinh.txt`)**, dẫn đến các lỗi kinh điển trong môn Cơ sở Dữ liệu:
1. **Dư thừa dữ liệu (Data Redundancy)**: Tỉnh thành "Da Nang", "Quang Nam" lặp lại liên tục; Điểm ưu tiên để người dùng tự nhập tay (0.5, 0.75...) thay vì tự động suy ra từ Khu vực.
2. **Dị thường cập nhật/xóa (Update/Delete Anomaly)**: Khi thay đổi điểm chuẩn hoặc khu vực, phải sửa hàng loạt dòng dữ liệu.
3. **Mất kiểm soát khi trùng họ tên**: Nếu nhiều thí sinh cùng tên "Nguyen Van An", việc lưu phẳng 1 file không thể phân biệt chính xác hồ sơ cá nhân và kết quả thi của từng người.

### 2.2. Mô hình Thực thể - Mối quan hệ (ERD) Chuẩn 3NF

```
+--------------------------+           +--------------------------+
|       KhuVuc (Bảng 1)    |           |    NganhXetTuyen (Bảng 2)|
+--------------------------+           +--------------------------+
| * maKhuVuc (PK)          |           | * maNganh (PK)           |
|   tenKhuVuc              |           |   tenNganh               |
|   diemUuTien             |           |   toHopMon               |
+--------------------------+           |   chiTieu                |
            ▲                          |   diemChuan              |
            │ 1                        +--------------------------+
            │                                       ▲
            │ N (Tham chiếu FK)                     │ 1
+--------------------------+                        │
|   HoSoThiSinh (Bảng 3)   |                        │ N (Tham chiếu FK)
+--------------------------+                        │
| * maThiSinh (PK)         |                        │
|   soCMND (Unique)        |           +--------------------------+
|   hoTen                  |           |      DangKyThi (Bảng 4)  |
|   ngaySinh               | 1       N +--------------------------+
|   queQuan                +───────────┤ * soBaoDanh (PK)         |
|   gioiTinh               |           | # maThiSinh (FK)         |
| # maKhuVuc (FK)          |           | # maNganh (FK)           |
+--------------------------+           |   diemToan, diemLy, Hoa  |
                                       +--------------------------+
```

### 2.3. Cấu trúc 4 Tệp Tin CSDL Quan Hệ trong thư mục `data/`:
1. **`danhmuc_khuvuc.txt`**: Bảng danh mục khu vực ưu tiên
   - Cấu trúc: `maKhuVuc | tenKhuVuc | diemUuTien`
   - Ví dụ: `KV1|Khu vuc 1|0.75`, `KV2NT|Khu vuc 2 Nong thon|0.50`, `KV2|Khu vuc 2|0.25`, `KV3|Khu vuc 3|0.00`.
   - *Lợi ích*: Điểm ưu tiên được tự động tra cứu từ mã khu vực, không cho phép nhập tùy tiện!
2. **`danhmuc_nganh.txt`**: Bảng danh mục các ngành đào tạo của Trường Đại học Bách khoa
   - Cấu trúc: `maNganh | tenNganh | toHopMon | chiTieu | diemChuan`
   - Ví dụ: `7480201|Cong nghe Thong tin|A00|150|26.25`.
   - *Lợi ích*: Xét tuyển theo điểm chuẩn và chỉ tiêu của từng ngành cụ thể, phản ánh đúng thực tế tuyển sinh đại học.
3. **`hoso_thisinh.txt`**: Bảng lưu trữ hồ sơ nhân thân thí sinh
   - Cấu trúc: `maThiSinh | soCMND | hoTen | ngaySinh | queQuan | gioiTinh | maKhuVuc`
   - Ví dụ: `TS001|201948572134|Nguyen Van An|15/03/2005|Da Nang|Nam|KV2`
   - *Lợi ích*: **Khóa chính `maThiSinh` là duy nhất**. Dù có 10 thí sinh cùng tên "Nguyen Van An", hệ thống vẫn phân biệt độc lập tuyệt đối thông qua `maThiSinh` và `soCMND`!
4. **`dangky_thi.txt`**: Bảng kết quả thi và nguyện vọng đăng ký
   - Cấu trúc: `soBaoDanh | maThiSinh | maNganh | diemToan | diemLy | diemHoa`
   - Ví dụ: `DUT001|TS001|7480201|9.50|9.00|9.75`
   - *Lợi ích*: Tách biệt kết quả thi và hồ sơ nhân thân, liên kết bằng Khóa ngoại `maThiSinh` và `maNganh`.

---

## 3. KIẾN TRÚC HƯỚNG ĐỐI TƯỢNG (OOP) & CÁC LỚP THỰC THỂ

1. **Lớp `Nguoi`**: Đóng gói thông tin nhân thân cơ bản (`soCMND`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`) dưới phạm vi `protected`. Cung cấp phương thức ảo `virtual void nhap()`, `virtual void xuat()` và hàm hủy ảo `virtual ~Nguoi()`.
2. **Lớp `ThiSinh` kế thừa `Nguoi`**: Bổ sung `maThiSinh` (PK), `soBaoDanh`, `maKhuVuc` (FK), `maNganh` (FK), điểm Toán, Lý, Hóa, Ưu tiên. Cài đặt các hàm nghiệp vụ tính tổng điểm, kiểm tra điểm liệt ($\le 1.0$), xếp loại học lực, và xét trúng tuyển theo điểm chuẩn ngành.
3. **Lớp `KhuVuc` & Lớp `NganhXetTuyen`**: Đại diện cho 2 bảng thực thể CSDL danh mục.
4. **Lớp `QuanLyThiSinh`**: Đóng vai trò là Bộ máy quản lý CSDL (Database Manager) và tầng nghiệp vụ (Business Logic Layer):
   - Đọc/ghi đồng bộ 4 file CSDL quan hệ.
   - Kiểm tra ràng buộc toàn vẹn: Không trùng lặp `maThiSinh`, `soBaoDanh`, `soCMND`.
   - Tra cứu khóa ngoại: Tự động tính điểm ưu tiên từ mã khu vực và lấy điểm chuẩn theo ngành đăng ký.
   - Thống kê thủ khoa, phân tích phổ điểm và xuất báo cáo kết quả xét tuyển.

---

## 4. CẤU TRÚC DỮ LIỆU TỰ VIẾT: TEMPLATE VECTOR<T> & QUICKSORT

Dự án tuyệt đối không dùng `std::vector` hay `std::sort` của thư viện chuẩn STL:
- Quản lý con trỏ động `T* data` với dung lượng co giãn tự động nhân đôi (`reallocate(m_capacity * 2)`).
- Tuân thủ nghiêm ngặt **Quy tắc bộ ba (Rule of Three)**: Destructor, Copy Constructor, Copy Assignment Operator.
- Tích hợp thuật toán **QuickSort** với con trỏ hàm so sánh (`bool (*compare)(const T&, const T&)`), cho phép sắp xếp theo mọi tiêu chí (tổng điểm giảm dần, tên A-Z, SBD, theo ngành) với độ phức tạp trung bình $O(N \log N)$.

---

## 5. GIAO DIỆN ĐỒ HỌA CỬA SỔ HIỆN ĐẠI (DEAR IMGUI & DIRECTX 9)

Loại bỏ giao diện Console đen trắng, chương trình sở hữu giao diện đồ họa cửa sổ Desktop chuyên nghiệp:
- **Công nghệ**: Thư viện đồ họa **Dear ImGui** kết hợp backend **Win32 + DirectX 9**, render ở tần số 60 FPS cực kỳ mượt mà.
- **Phong cách thiết kế**: Dark Theme hiện đại (Slate Blue / Charcoal / Cyan accents), hỗ trợ phân tab tiện lợi:
  * **Tab 1: Quản lý Hồ sơ Thí sinh**: Bảng dữ liệu có thanh cuộn mượt mà, tìm kiếm nhanh, lọc theo trạng thái trúng tuyển, lọc theo ngành, các nút thêm/sửa/xóa mở cửa sổ Modal Dialog nhập liệu trực quan kèm tính năng kiểm tra lỗi (Validate).
  * **Tab 2: Cơ sở Dữ liệu Quan hệ 3NF**: Hiển thị bảng Khu vực và bảng Ngành đào tạo; cho phép giảng viên điều chỉnh trực tiếp Điểm chuẩn và Chỉ tiêu ngành ngay trên màn hình.
  * **Tab 3: Thống kê & Phổ điểm & Thủ khoa**: Banner vàng vinh danh Thủ khoa kỳ thi, bảng thủ khoa từng ngành, biểu đồ thanh trực quan (Progress Bars) thể hiện tỷ lệ % học lực.
  * **Tab 4: Lưu trữ & Xuất Báo cáo**: Nút lưu bền vững vào 4 file CSDL và xem trước file báo cáo `ketqua_xettuyen.txt` trực tiếp trong ứng dụng.
  * **Tab 5: Hướng dẫn & Kiến trúc**: Tóm tắt kiến trúc CSDL và thông tin đồ án.

---

## 6. KỊCH BẢN 5 PHÚT BÁO CÁO TRÌNH DIỄN KHIẾN THẦY HÀI LÒNG TUYỆT ĐỐI

* **Phút 1: Mở đầu & Giới thiệu Giao diện Đồ họa GUI:**
  - Khởi chạy `QuanLyThiSinh.exe` (hoặc nhấp đúp `build.bat`).
  - Nói với thầy: *"Thưa thầy, tiếp thu góp ý của thầy về việc nâng cấp giao diện, em đã xây dựng giao diện đồ họa người dùng (GUI) hoàn chỉnh bằng thư viện Dear ImGui kết hợp DirectX 9, render 60 FPS, bố cục chia Tab khoa học thay thế cho console cũ."*

* **Phút 2: Chứng minh Kiến trúc CSDL Quan hệ 3NF (Điểm mấu chốt):**
  - Chuyển sang **Tab 2: Cơ sở Dữ liệu Quan hệ (3NF)**.
  - Chỉ cho thầy thấy: *"Thưa thầy, để khắc phục nhược điểm lưu 1 file phẳng trước đây, em đã áp dụng kiến thức môn Cơ sở dữ liệu để chuẩn hóa hệ thống thành 4 bảng lưu ở 4 tệp tin riêng biệt: Bảng Khu vực ưu tiên, Bảng Ngành đào tạo, Bảng Hồ sơ nhân thân và Bảng Kết quả thi."*
  - *"Dữ liệu được liên kết chặt chẽ bằng Khóa chính (PK) và Khóa ngoại (FK). Điểm ưu tiên được tra cứu tự động từ Bảng Khu vực chứ không để nhập tự do, và điểm chuẩn xét tuyển được quản lý riêng theo từng ngành."*

* **Phút 3: Giải quyết bài toán "Nhiều thí sinh trùng họ tên":**
  - Chuyển sang **Tab 1: Quản lý Hồ sơ Thí sinh**.
  - Gõ vào ô tìm kiếm: `Nguyen Van An`.
  - Chỉ cho thầy thấy: Có **2 thí sinh cùng tên Nguyễn Văn An** (một người ở Đà Nẵng, một người ở Hà Nội).
  - Giải thích: *"Thưa thầy, hệ thống quản lý mỗi thí sinh bằng một `MaTS` duy nhất (TS001 và TS014) và số CMND/CCCD riêng biệt, vì vậy dù thí sinh có trùng họ tên thì hệ thống vẫn quản lý độc lập 100% không bao giờ xảy ra nhầm lẫn dữ liệu!"*

* **Phút 4: Trình diễn Thao tác Thêm / Sửa / Xóa & Validate Dữ liệu:**
  - Bấm nút `[+] Them Thi Sinh` để mở Modal Dialog.
  - Chọn Khu vực `KV1` ➔ Điểm ưu tiên tự nhảy thành `+0.75`.
  - Chọn Ngành `Cong nghe Thong tin` ➔ Điểm chuẩn ngành hiện lên `26.25d`.
  - Nhập điểm Toán, Lý, Hóa ➔ Hệ thống tự động tính trước tổng điểm và hiển thị dự đoán kết quả xét tuyển tức thì!

* **Phút 5: Vinh danh Thủ khoa, Phổ điểm & Xuất Báo cáo:**
  - Chuyển sang **Tab 3: Thống kê & Phổ điểm**: Chỉ vào Banner vàng vinh danh Thủ khoa xuất sắc nhất, biểu đồ thanh trực quan tỷ lệ % phổ điểm.
  - Chuyển sang **Tab 4**: Bấm nút `Xuat Bao Cao` và mở cho thầy xem nội dung báo cáo trúng tuyển chuyên nghiệp được định dạng ngay trên giao diện.

---

## 7. BỘ 10 CÂU HỎI VẤN ĐÁP THẦY HAY HỎI NHẤT & CÂU TRẢ LỜI ĐẠT ĐIỂM A+

### ❓ Câu 1: Em đã áp dụng kiến thức môn Cơ sở dữ liệu (CSDL) vào đồ án này như thế nào?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, em đã áp dụng nguyên lý Chuẩn hóa dữ liệu đạt dạng chuẩn 3NF (Third Normal Form) để chia nhỏ dữ liệu thành 4 bảng thực thể lưu trên 4 tệp tin:  
  > 1. `danhmuc_khuvuc.txt`: Lưu danh mục khu vực và điểm ưu tiên tương ứng.  
  > 2. `danhmuc_nganh.txt`: Lưu mã ngành, tên ngành, tổ hợp môn, chỉ tiêu và điểm chuẩn của từng ngành.  
  > 3. `hoso_thisinh.txt`: Lưu thông tin nhân thân, có Khóa chính `maThiSinh` và Khóa ngoại `maKhuVuc`.  
  > 4. `dangky_thi.txt`: Lưu kết quả điểm thi liên kết với `maThiSinh` và `maNganh` bằng Khóa ngoại.  
  > Nhờ thiết kế này, dữ liệu không bị dư thừa, không bị dị thường cập nhật và phản ánh chính xác mô hình cơ sở dữ liệu quan hệ trong thực tế ạ."*

---

### ❓ Câu 2: Nếu có 2 hoặc nhiều thí sinh trùng họ tên thì hệ thống của em kiểm soát và phân biệt ra sao?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, trong môn CSDL, thuộc tính Họ và Tên không thể dùng làm khóa vì không có tính duy nhất. Trong hệ thống của em, mỗi thí sinh khi đăng ký hồ sơ đều được cấp một **Mã Thí Sinh duy nhất (`maThiSinh` - Primary Key)** kết hợp với số **CMND/CCCD** và **Số báo danh (SBD)**.  
  > Khi tìm kiếm hoặc cập nhật, hệ thống truy xuất dựa trên Khóa chính `maThiSinh`. Ví dụ trong dữ liệu mẫu của em có 2 thí sinh cùng tên là 'Nguyen Van An', một bạn có mã `TS001` (ở Đà Nẵng) và một bạn có mã `TS014` (ở Hà Nội), điểm số và kết quả xét tuyển của 2 bạn hoàn toàn độc lập và không bao giờ bị nhầm lẫn ạ."*

---

### ❓ Câu 3: Tại sao em lại chọn thư viện Dear ImGui làm giao diện thay vì dùng Console hay Qt?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy:  
  > 1. Console là giao diện dòng lệnh cũ, không đáp ứng được trải nghiệm trực quan của phần mềm hiện đại.  
  > 2. So với Qt Framework vốn rất cồng kềnh (bộ cài nặng nhiều GB và yêu cầu cài đặt phần mềm Qt Creator phức tạp), thư viện **Dear ImGui** là thư viện C++ đồ họa hiện đại chuẩn công nghiệp, rất nhẹ, được tích hợp thẳng vào mã nguồn C++ và render trực tiếp bằng **DirectX 9 / Win32 API**.  
  > Nhờ vậy, chương trình có thể biên dịch và chạy ngay lập tức trên máy của thầy cô chỉ bằng 1 cú nhấp chuột mà không cần phải cài đặt bất kỳ phần mềm bên ngoài nào, trong khi vẫn đạt hiệu năng 60 FPS cực kỳ mượt mà ạ."*

---

### ❓ Câu 4: Việc xét trúng tuyển trong hệ thống được thực hiện như thế nào? Điểm chuẩn là cố định cho cả trường hay theo từng ngành?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, trong thực tế tuyển sinh đại học, mỗi ngành đào tạo có điểm chuẩn và chỉ tiêu tuyển sinh riêng biệt.  
  > Trong hệ thống của em, điểm chuẩn được lưu trong bảng `NganhXetTuyen`. Khi xét tuyển cho một thí sinh, hệ thống sẽ:  
  > 1. Kiểm tra thí sinh có bị điểm liệt hay không (môn thi $\le 1.0$ điểm).  
  > 2. Lấy tổng điểm 3 môn cộng điểm ưu tiên khu vực.  
  > 3. So sánh tổng điểm với **chính điểm chuẩn của ngành mà thí sinh đó đăng ký**.  
  > Đồng thời, người quản trị có thể thay đổi điểm chuẩn hoặc chỉ tiêu của bất kỳ ngành nào ngay trên Tab 2 của giao diện, hệ thống sẽ tự động cập nhật lại kết quả trúng tuyển tức thì ạ."*

---

### ❓ Câu 5: Em tự cài đặt cấu trúc dữ liệu `Vector<T>` thay vì dùng `std::vector` như thế nào?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, để đáp ứng chuẩn đầu ra của học phần PBL2 về cấu trúc dữ liệu và quản lý bộ nhớ động, em tự xây dựng lớp mẫu `Vector<T>`:  
  > - Sử dụng con trỏ động `T* data` cấp phát bằng `new[]`.  
  > - Quản lý `m_size` và `m_capacity`. Khi mảng đầy, hàm `reallocate` sẽ cấp phát vùng nhớ mới có dung lượng gấp đôi (`m_capacity * 2`), sao chép dữ liệu cũ và giải phóng vùng nhớ cũ bằng `delete[]`, giúp độ phức tạp trung bình khi thêm phần tử là $O(1)$.  
  > - Cài đặt đầy đủ **Rule of Three** (Destructor, Copy Constructor, Copy Assignment Operator) để tránh rò rỉ bộ nhớ hoặc lỗi con trỏ treo ạ."*

---

### ❓ Câu 6: Thuật toán sắp xếp trong `Vector.h` là gì? Tại sao em có thể sắp xếp theo nhiều tiêu chí khác nhau?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, em tự cài đặt thuật toán **QuickSort** với phân hoạch Hoare, độ phức tạp trung bình là $O(N \log N)$.  
  > Điểm đặc biệt là phương thức `sort` nhận vào một con trỏ hàm so sánh `bool (*compare)(const T&, const T&)`. Nhờ cơ chế callback này, em chỉ cần viết một hàm QuickSort duy nhất nhưng có thể tái sử dụng để sắp xếp danh sách theo bất kỳ tiêu chuẩn nào: theo tổng điểm giảm dần, theo thứ tự tên A-Z, theo SBD hoặc theo ngành học ạ."*

---

### ❓ Câu 7: Bốn tính chất của Lập trình Hướng đối tượng (OOP) thể hiện ở đâu trong mã nguồn?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy:  
  > 1. **Đóng gói (Encapsulation)**: Các thuộc tính nhân thân và điểm thi đều đặt ở phạm vi `protected` hoặc `private`, chỉ truy xuất qua Getter/Setter có kiểm tra hợp lệ dữ liệu.  
  > 2. **Kế thừa (Inheritance)**: Lớp `ThiSinh` kế thừa công khai từ lớp cơ sở `Nguoi`.  
  > 3. **Đa hình (Polymorphism)**: Sử dụng phương thức ảo `virtual void nhap()`, `virtual void xuat()` và hàm hủy ảo `virtual ~Nguoi()`.  
  > 4. **Trừu tượng hóa (Abstraction)**: Mô hình hóa các thực thể thế giới thực thành các lớp `Nguoi`, `ThiSinh`, `KhuVuc`, `NganhXetTuyen` và lớp quản trị `QuanLyThiSinh`."*

---

### ❓ Câu 8: Điểm ưu tiên được tính toán tự động như thế nào để đảm bảo tính toàn vẹn dữ liệu?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, người dùng không được phép nhập tùy tiện con số điểm ưu tiên. Khi nhập hồ sơ thí sinh, người dùng chỉ được chọn Mã Khu Vực (KV1, KV2NT, KV2, KV3). Hệ thống sẽ tự động đối soát với bảng `KhuVuc` trong CSDL để lấy mức điểm cộng tương ứng (+0.75, +0.50, +0.25, 0.00). Điều này bảo đảm tính toàn vẹn tham chiếu (Referential Integrity) chuẩn CSDL ạ."*

---

### ❓ Câu 9: Trong lớp cơ sở `Nguoi`, tại sao bắt buộc phải có `virtual ~Nguoi()`?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, khi ta thao tác với con trỏ lớp cơ sở `Nguoi* ptr = new ThiSinh()`, nếu không có từ khóa `virtual` ở hàm hủy, khi gọi lệnh `delete ptr`, trình biên dịch sẽ chỉ liên kết tĩnh và gọi duy nhất `~Nguoi()`, bỏ sót các thuộc tính và tài nguyên của lớp con `ThiSinh`, dẫn đến lỗi rò rỉ bộ nhớ (Memory Leak).  
  > Khai báo `virtual ~Nguoi()` kích hoạt cơ chế liên kết động (Dynamic Binding), bảo đảm Destructor của lớp con `~ThiSinh()` luôn được gọi trước rồi mới đến `~Nguoi()` ạ."*

---

### ❓ Câu 10: Nếu sau này trường Đại học muốn mở rộng thêm hệ đào tạo hoặc quy chế tuyển sinh thì kiến trúc của em có dễ nâng cấp không?
* **Trả lời chuẩn A+:**
  > *"Dạ thưa thầy, hệ thống tuân thủ nguyên lý **Mở/Đóng (Open/Closed Principle)**:  
  > - Nếu muốn thêm ngành học mới: Chỉ cần thêm dòng vào bảng `danhmuc_nganh.txt` hoặc thêm ngay trên giao diện mà không cần sửa code.  
  > - Nếu muốn mở rộng đối tượng như Giảng viên chấm thi hay Cán bộ coi thi: Chỉ cần tạo lớp mới kế thừa từ lớp cơ sở `Nguoi` có sẵn, hoàn toàn không làm ảnh hưởng đến mã nguồn hiện tại ạ!"*

---
*Tài liệu nâng cấp toàn diện - Đồ án PBL2 Khoa CNTT - Đại học Bách khoa Đà Nẵng.*
