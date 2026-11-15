# KẾ HOẠCH TOÀN DIỆN NÂNG CẤP HỆ THỐNG TUYỂN SINH ĐẠI HỌC BÁCH KHOA - ĐH ĐÀ NẴNG (DUT)
> **Học phần**: PBL 2 - Đồ án cơ sở lập trình (Đề tài số 24)  
> **Đơn vị đào tạo**: Trường Đại học Bách khoa - Đại học Đà Nẵng (DUT) - Khoa Công nghệ Thông tin  
> **Sinh viên thực hiện**: Võ Như Hiếu (MSSV: 102210091 - Lớp 21TCLC_KHDL)  
> **Quy chuẩn bắt buộc**: lowerCamelCase đồng nhất 100% giữa Class C++ và CSDL MySQL | Tuyệt đối không hardcode  
> **Phiên bản**: V2.4 - Đa Nguyện Vọng, Danh Mục Ngành & Ngành Theo Khóa Học, Sĩ Số Lớp Động, Điểm Chuẩn Động

---

## 1. TỔNG QUAN YÊU CẦU THỰC TẾ & TIẾP THU ĐÓNG GÓP

Dựa trên đóng góp sâu sắc từ trải nghiệm thực tế của con tại chính **Đại học Bách khoa Đà Nẵng (DUT)**, hệ thống được thiết kế lại để mô phỏng chính xác 100% quy trình tuyển sinh và phân lớp của Nhà trường:

```
                  QUY TRÌNH TUYỂN SINH BÁCH KHOA ĐÀ NẴNG (DUT)
+-----------------------------------------------------------------------------------+
| 1. Thí sinh đăng ký hồ sơ nhân thân (Nguoi) & Tối đa 3 Nguyện vọng (NV1, NV2, NV3) |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
| 2. Nhập điểm 3 môn: Toán - Lý - Hóa (Tổ hợp A00 truyền thống Bách khoa)           |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
| 3. Quản lý theo Khóa tuyển sinh (Mặc định Khóa 2026 - K26, hỗ trợ chọn các năm)   |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
| 4. Thuật toán Xét tuyển Đa Nguyện vọng & Xác định Điểm chuẩn Động theo Chỉ tiêu:  |
|    - Chỉ tiêu từng ngành cho phép người dùng NHẬP ĐỘNG linh hoạt trên giao diện. |
|    - Sắp xếp thí sinh theo Điểm tổng giảm dần.                                    |
|    - Lọc từ trên xuống dưới cho từng ngành đến khi đủ CHỈ TIÊU (Quota).           |
|    - Điểm của thí sinh cuối cùng lọt vào chỉ tiêu chính là ĐIỂM CHUẨN của Ngành!  |
|    - Trạng thái nguyện vọng lấy từ Constants (TRUNG TUYEN / KHONG TRUNG TUYEN).  |
+-----------------------------------------------------------------------------------+
                                          │
                                          ▼
+-----------------------------------------------------------------------------------+
| 5. Tự động Phân bổ Lớp sinh hoạt chuẩn DUT & Cấp Mã Sinh viên DUT:                |
|    - Sĩ số lớp chuẩn ĐỘNG (mặc định 30, cấu hình tùy biến trên UI).              |
|    - Chia sinh viên trúng tuyển vào các lớp (26TCLC_KHDL1, 26T_DT1...)            |
|    - Sinh Mã sinh viên chuẩn DUT (9 số): 102260xxx (như chuẩn 102210091 của con)  |
+-----------------------------------------------------------------------------------+
```

---

## 2. DANH MỤC KHOA & CHUYÊN NGÀNH CHUẨN XÁC ĐẠI HỌC BÁCH KHOA ĐÀ NẴNG (DUT)
*(Nguồn tham chiếu: Cổng thông tin Tuyển sinh DUT `tuyensinh.dut.udn.vn` và Hệ thống Sinh viên `sv.dut.udn.vn`)*

> [!NOTE]
> **Chỉ tiêu nhập động**: Các con số chỉ tiêu dưới đây là số liệu mặc định căn cứ theo chỉ tiêu K26 chuẩn của DUT. Người dùng / Giảng viên có thể bấm vào ô để sửa đổi chỉ tiêu động bất cứ lúc nào!

| Mã Khoa (`maKhoa`) | Tên Khoa (`tenKhoa`) | Mã Khoa DUT (`maKhoaTheoChuanDUT`) | Mã Ngành Bộ GD&ĐT (`maNganh`) | Tên Ngành Tuyển Sinh DUT (`tenNganh`) | Chỉ tiêu K26 (`chiTieu`) | Tiền tố Lớp (`tienToLop`) | Sĩ số chuẩn |
|:---|:---|:---:|:---|:---|:---:|:---|:---:|
| **CNTT** | Khoa Công nghệ Thông tin | `102` | `7480201B` | CNTT (Khoa học Dữ liệu & TTNT - KHDL_TTNT) | 60 | `TCLC_KHDL` | 30 |
| **CNTT** | Khoa Công nghệ Thông tin | `102` | `7480201` | Công nghệ Thông tin (Chương trình Đặc thù) | 120 | `T_DT` | 30 |
| **CNTT** | Khoa Công nghệ Thông tin | `102` | `7480201A` | Công nghệ Thông tin (Ngoại ngữ Nhật) | 60 | `T_N` | 30 |
| **CNTT** | Khoa Công nghệ Thông tin | `102` | `7480106` | Kỹ thuật Máy tính | 60 | `KTMT` | 30 |
| **DIEN** | Khoa Điện | `105` | `7520216` | Kỹ thuật Điều khiển & Tự động hóa | 90 | `TDH` | 30 |
| **DIEN** | Khoa Điện | `105` | `7520201` | Kỹ thuật Điện (Hệ thống điện) | 90 | `D` | 30 |
| **DTVT** | Khoa Điện tử - Viễn thông | `106` | `7520207A` | Vi điện tử - Thiết kế vi mạch (Bán dẫn) | 60 | `VM` | 30 |
| **DTVT** | Khoa Điện tử - Viễn thông | `106` | `7520207` | Kỹ thuật Điện tử - Viễn thông | 90 | `DT` | 30 |
| **CKHI** | Khoa Cơ khí | `101` | `7520114` | Kỹ thuật Cơ điện tử | 90 | `CDT` | 30 |
| **CKHI** | Khoa Cơ khí | `101` | `7520130` | Kỹ thuật Ô tô | 90 | `OTO` | 30 |
| **HOA** | Khoa Hóa | `107` | `7520301` | Kỹ thuật Hóa học | 60 | `H` | 30 |
| **XDDD** | Khoa Xây dựng Dân dụng | `109` | `7580201` | Kỹ thuật Xây dựng | 60 | `X` | 30 |

---

## 3. THIẾT KẾ CƠ SỞ DỮ LIỆU NÂNG CẤP (8 BẢNG 3NF) - CHUẨN ĐỒNG NHẤT CAMELCASE

Toàn bộ tên bảng và tên cột CSDL MySQL được chuẩn hóa đồng nhất 100% với tên thuộc tính trong lớp C++ theo chuẩn **lowerCamelCase**:

```
                       [khoa_hoc] (namTuyenSinh)
                           │
                           ▼ namTuyenSinh (FK)
[khoa] ──► [danh_muc_nganh] ──► [nganh_theo_khoa_hoc] (chiTieu, diemChuanChot)
                           ▲
                           │ maNganh (FK)
                   [nguyen_vong] (soBaoDanh, thuTuNV, maNganh)
                           ▲
                           │ soBaoDanh (FK)
[tinh_thanh] ──► [nguoi] ──► [thisinh] (diemToan, diemLy, diemHoa)
                           │
                           ▼ Cấp khi trúng tuyển (Default NULL):
                           - maSinhVien           : 102260001
                           - lopSinhHoat          : 26TCLC_KHDL1
                           - nganhTrungTuyen      : 7480201B
                           - nguyenVongTrungTuyen : 1
```

### Chi tiết 8 Bảng & Thuộc tính tương ứng (Khớp 100% OOP & DB):
1. **`khoa_hoc`**: `namTuyenSinh` (INT, PK), `tenKhoaHoc`, `trangThai`.
2. **`khoa`**: `maKhoa` (VARCHAR 10, PK), `tenKhoa`, `maKhoaTheoChuanDUT` (102, 105, 101...).
3. **`danh_muc_nganh`**: `maNganh` (VARCHAR 20, PK), `tenNganh`, `maKhoa` (FK), `tienToLop`. (Danh mục tĩnh chuẩn).
4. **`nganh_theo_khoa_hoc`**: `namTuyenSinh` (PK, FK), `maNganh` (PK, FK), `chiTieu` (nhập động), `diemChuanChot`, `diemSanToiThieu`. (Dữ liệu biến động theo khóa).
5. **`nguoi`**: `soCCCD` (VARCHAR 20, PK), `hoTen`, `ngaySinh`, `queQuan` (FK), `gioiTinh` (FK).
6. **`thisinh`**: `soBaoDanh` (VARCHAR 20, PK), `soCCCD` (FK 1-1 CASCADE), `namTuyenSinh` (FK), `diemToan`, `diemLy`, `diemHoa`, `maSinhVien` (DEFAULT NULL), `lopSinhHoat` (DEFAULT NULL), `nganhTrungTuyen` (DEFAULT NULL), `nguyenVongTrungTuyen` (DEFAULT 0).
7. **`nguyen_vong`**: `id` (INT PK auto), `soBaoDanh` (FK), `thuTuNV` (1, 2, 3), `maNganh` (FK -> `danh_muc_nganh.maNganh`), `trangThai`.
8. **`tinh_thanh`** & **`gioi_tinh`**: `maTinh`, `tenTinh`, `loai` | `maGioiTinh`, `tenGioiTinh`.
9. **`cau_hinh`**: `khoa`, `giaTri`, `moTa` (chứa `siSoLopChuan` động, `diemLiet`, `namTuyenSinhHienHanh`...).

---

## 4. CHI TIẾT NGHIỆP VỤ ĐỘC QUYỀN

### 4.1. Quy chế Điểm chuẩn Động theo Chỉ tiêu (Dynamic Cutoff by Quota)
- **Chỉ tiêu nhập động**: Mỗi ngành có trường `chiTieu` lưu trong CSDL `nganh_theo_khoa_hoc`. Trên giao diện, người dùng có thể tăng/giảm hoặc nhập số chỉ tiêu mới (ví dụ: đổi từ 60 lên 70).
- **Thuật toán tự động tìm Điểm chuẩn**:
  1. Gom tất cả thí sinh đăng ký NV1 vào ngành đó (điều kiện: điểm các môn `> 1.00`).
  2. Sắp xếp giảm dần theo `tongDiem = diemToan + diemLy + diemHoa`.
  3. Lấy từ trên xuống dưới đúng số lượng `chiTieu` đã nhập.
  4. **Điểm của thí sinh thứ `chiTieu` chính là Điểm chuẩn chính thức!**
  5. Những thí sinh trượt NV1 sẽ được đưa vào pool để xét tiếp NV2 và NV3.
  6. Toàn bộ chuỗi trạng thái phải dùng hằng số `AdmissionConst::TT_NV_*`.

### 4.2. Cơ chế Xếp Lớp Sinh Hoạt DUT Tự Động (Sĩ số Động)
- Lấy danh sách $N$ sinh viên trúng tuyển của một ngành.
- **Sĩ số chuẩn lớp động $S$**: Được đọc từ cấu hình `siSoLopChuan` (mặc định 30, người dùng có thể đổi thành 35, 40...).
- Số lớp cần mở: $K = \lceil N / S \rceil$.
- Tên lớp sinh hoạt: `<2 số cuối namTuyenSinh><tienToLop><thứ tự lớp>`
  - *Ví dụ*: Với $S = 30$, ngành KHDL_TTNT Khóa 2026 có 60 sinh viên trúng tuyển $\rightarrow$ Tự động xếp thành 2 lớp:
    - **`26TCLC_KHDL1`** (30 sv đầu)
    - **`26TCLC_KHDL2`** (30 sv sau)

### 4.3. Ý Nghĩa & Cơ Chế Sinh Mã Sinh Viên (MSSV) Chuẩn DUT (9 Chữ Số)
$$\text{maSinhVien} = \text{102 (maKhoaTheoChuanDUT)} + \text{26 (namTuyenSinh)} + \text{xxxx (STT trúng tuyển)}$$
*Ví dụ*: Sinh viên đầu tiên trúng tuyển Khoa CNTT sẽ mang mã: **`102260001`**.
*(Khớp hoàn toàn với cấu trúc 9 chữ số `102210091` của đệ tử tại K21 DUT!)*

---

## 5. LỘ TRÌNH TRIỂN KHAI 5 PHASES

### Phase 1: Nâng cấp Schema CSDL MySQL & Kịch bản Dữ liệu Mẫu (`sql/`)
- Cập nhật `01_create_database.sql`: Tạo các bảng với tên mới `danh_muc_nganh`, `nganh_theo_khoa_hoc` và toàn bộ tên cột lowerCamelCase.
- Cập nhật `02_seed_khoa_nganh.sql`: Nạp đủ 6 Khoa và 12 Ngành tuyển sinh DUT thực tế vào `danh_muc_nganh`.
- Tạo `03_seed_khoa_hoc.sql`: Tạo khóa 2026 (K26) với chỉ tiêu động từng ngành trong `nganh_theo_khoa_hoc`.
- Cập nhật `04_seed_sample_data.sql`: Nạp hồ sơ thí sinh mẫu có đăng ký NV1, NV2, NV3.
- Cập nhật `05_advanced_queries.sql`: Bộ truy vấn xét tuyển đa nguyện vọng, truy vấn điểm chuẩn động, danh sách lớp sinh hoạt.

### Phase 2: Nâng cấp Mã Nguồn C++ (Structs, Classes & Algorithms)
- Tạo `include/DUTModels.h`: Các cấu trúc `Khoa`, `DanhMucNganh`, `NganhTheoKhoaHoc`, `NguyenVong`.
- Cập nhật `ThiSinh`: Hỗ trợ danh sách 3 nguyện vọng `Vector<NguyenVong> dsNguyenVong;`.
- Viết thuật toán trong `QuanLyThiSinh`:
  - `chayThuatToanXetTuyenVaChotDiemChuan(int namTuyenSinh);`
  - `tuDongXepLopSinhHoatDUT(int namTuyenSinh);` (đọc `siSoLopChuan` động từ CSDL).
  - `sinhMaSinhVienDUT(const string& maKhoaTheoChuanDUT, int nam, int stt);` (Chuẩn 9 số).

### Phase 3: Nâng cấp Giao Diện Đồ Họa Dear ImGui
1. **Header Bar**: Thêm Combobox chọn **Khóa / Năm tuyển sinh** (mặc định 2026).
2. **Form Thêm / Sửa Thí sinh**: Cho phép chọn thứ tự 3 Nguyện vọng từ combobox danh sách ngành DUT.
3. **Tab 1 (Hồ sơ Thí sinh)**: Hiển thị cột nguyện vọng, kết quả trúng tuyển vào ngành nào, `maSinhVien` và `lopSinhHoat`.
4. **Tab 3 (Xét tuyển & Phân lớp DUT)**:
   - Nút chạy xét tuyển tự động theo chỉ tiêu.
   - Bảng tổng kết Điểm chuẩn chốt theo từng ngành.
   - Bảng phân bổ danh sách Lớp sinh hoạt (click vào từng lớp xem danh sách sinh viên).
5. **Tab 4 (Danh mục Hệ thống)**: Quản lý Khoa, `danh_muc_nganh` và Ô nhập Chỉ tiêu động cho `nganh_theo_khoa_hoc`, ô nhập Sĩ số lớp chuẩn động.

### Phase 4: Biên Dịch, Test Tự Động & Đóng Gói
- Chạy `build.bat` biên dịch với g++ MinGW64.
- Kiểm tra toàn vẹn ràng buộc CSDL (Cascade delete, check foreign keys).
- Kiểm tra chế độ dòng lệnh `--test` và `--console`.

### Phase 5: Chuẩn Bị Kịch Bản Báo Cáo Bảo Vệ
- Kịch bản trình diễn thao tác thay đổi chỉ tiêu và sĩ số lớp trực tiếp trên UI trước mặt Thầy giáo.
