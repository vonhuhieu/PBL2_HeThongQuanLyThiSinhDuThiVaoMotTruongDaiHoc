# ĐỒ ÁN CƠ SỞ LẬP TRÌNH (PBL2)
## ĐỀ TÀI 24: QUẢN LÝ THÍ SINH DỰ THI VÀO MỘT TRƯỜNG ĐẠI HỌC (PHIÊN BẢN TÍCH HỢP MYSQL)
* **Trường**: Đại học Bách khoa - Đại học Đà Nẵng (DUT)
* **Khoa**: Công nghệ Thông tin
* **Học phần**: PBL2 - Đồ án cơ sở lập trình
* **Sinh viên thực hiện**: Võ Như Hiếu
* **Nhánh**: `mysql` (Dự án: `pbl2_MySQL`)

---

## 🌟 Điểm nổi bật của Phiên bản Tích hợp MySQL
1. **Kiến trúc Lai Kép (Hybrid Dual-Engine Architecture)**:
   - **Tầng CSDL Quan hệ (Relational Database)**: Tích hợp hệ quản trị CSDL **MySQL 8.4 (Laragon)** thông qua thư viện chuẩn **MySQL C API (`libmysql`)**.
   - **Tầng Cấu trúc Dữ liệu & Giải thuật (DSA)**: Mảng động tự xây dựng `Vector<T>` (tuân thủ **Rule of Five** và giải thuật **QuickSort** `O(N log N)`), đáp ứng trọn vẹn yêu cầu khắt khe của môn PBL2.
   - **Tầng Bộ đệm & Sao lưu (File System Cache)**: Hỗ trợ tự động fallback sang cơ sở dữ liệu đa tệp 3NF nếu MySQL chưa khởi động, đảm bảo ứng dụng không bao giờ bị crash.
2. **Chuẩn hóa Cơ sở Dữ liệu 5 Bảng (Chuẩn 3NF)**:
   - `tinh_thanh`: Danh mục 34 tỉnh/thành phố (`ma_tinh`, `ten_tinh`, `loai`).
   - `gioi_tinh`: Danh mục giới tính chuẩn hóa tự nhiên (`ma_gioi_tinh`, `ten_gioi_tinh`: `NAM`, `NU`, `KHAC`).
   - `nguoi`: Bảng thực thể cơ sở cha (`so_cccd` PK, `ho_ten`, `ngay_sinh`, `que_quan` FK, `gioi_tinh` FK).
   - `thisinh`: Bảng thực thể con kế thừa (`sbd` PK, `so_cccd` FK `ON DELETE CASCADE`, `diem_toan`, `diem_ly`, `diem_hoa`).
   - `cau_hinh`: Bảng tham số hệ thống (`khoa` PK, `gia_tri`, `mo_ta`).
3. **Bộ Kịch Bản SQL Độc Lập (`sql/`)**:
   - `01_create_database.sql`: Tạo CSDL `pbl2_tuyensinh`, tạo 5 bảng với khóa chính, khóa ngoại, ràng buộc CHECK và CASCADE delete.
   - `02_seed_tinhthanh.sql`: Nạp danh mục 34 tỉnh/thành phố chuẩn.
   - `03_seed_gioitinh.sql`: Nạp danh mục giới tính.
   - `04_seed_sample_data.sql`: Nạp dữ liệu 14 thí sinh mẫu và cấu hình điểm chuẩn/điểm liệt.
   - `05_advanced_queries.sql`: Các câu truy vấn nâng cao: JOIN 4 bảng, GROUP BY thống kê phổ điểm, truy vấn thủ khoa, thống kê theo tỉnh thành.
4. **Loại bỏ Hoàn toàn Magic Numbers & Hardcode**:
   - Toàn bộ hằng số điểm chuẩn (`23.00`), điểm liệt (`1.00`), kết quả trúng tuyển/không đạt, các mốc phân bổ phổ điểm (`27.0`, `24.0`, `21.0`, `15.0`) được định nghĩa tập trung trong `include/Constants.h`.
5. **Giao diện Người dùng Hiện đại**:
   - **Dear ImGui (DirectX 9)** với huy hiệu trạng thái MySQL thời gian thực `[● MySQL: Connected]`, nút đồng bộ 2 chiều (Memory <-> MySQL), Tab Quản lý Danh mục, Tab Báo cáo động.
   - Hỗ trợ cả chế độ dòng lệnh Console tương tác phím mũi tên: `.\QuanLyThiSinh_PBL2.exe --console`.

---

## 🏛️ Sơ đồ Kiến trúc Hệ thống

```
+-----------------------------------------------------------------------------------+
|                        GIAO DIỆN NGƯỜI DÙNG (PRESENTATION)                        |
|   Dear ImGui (DirectX 9) - 6 Tabs Chuyên nghiệp   |   Interactive Console UTF-8   |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼
+-----------------------------------------------------------------------------------+
|                     NGHIỆP VỤ & QUẢN LÝ (QuanLyThiSinh)                           |
|   Bộ nhớ In-Memory: Vector<ThiSinh>, Vector<TinhThanh>, Vector<GioiTinh>           |
|   Giải thuật: QuickSort nội bộ, Lọc đa tiêu chí, Tra cứu Danh mục O(1)/O(N)       |
|   Hằng số tập trung: namespace AdmissionConst (include/Constants.h)               |
+-----------------------------------------------------------------------------------+
                       │                                    │
       [Khi MySQL đang chạy (Laragon)]       [Khi MySQL Offline / Lưu Cache]
                       ▼                                    ▼
+---------------------------------------------+  +----------------------------------+
|           MySQL 8.4 DATABASE ENGINE         |  |      FILE SYSTEM CACHE (3NF)     |
|   CSDL: `pbl2_tuyensinh` (Port 3306)        |  |  data/nguoi.txt                  |
|   Bảng: tinh_thanh, gioi_tinh, nguoi,       |  |  data/thisinh.txt                |
|         thisinh, cau_hinh                   |  |  data/tinhthanh.txt              |
|   Thao tác: JOIN 4 bảng, Transaction,       |  |  data/gioitinh.txt               |
|             Foreign Key CASCADE DELETE      |  |  data/system.txt                 |
+---------------------------------------------+  +----------------------------------+
```

---

## 🗄️ Cấu trúc Thư mục Dự án

```
pbl2_MySQL/
├── build.bat                   # Script biên dịch tự động MinGW64 + MySQL C API
├── libmysql.dll                # Thư viện liên kết động MySQL Client (x86_64)
├── QuanLyThiSinh_PBL2.exe      # File thực thi chương trình chính
├── data/                       # Dữ liệu cục bộ & Tệp báo cáo xuất bản
│   ├── nguoi.txt               # Bảng Người (CCCD, Họ tên, Ngày sinh, Quê, Giới tính)
│   ├── thisinh.txt             # Bảng Thí sinh (SBD, CCCD, Toán, Lý, Hóa)
│   ├── tinhthanh.txt           # Bảng Danh mục 34 Tỉnh/Thành phố
│   ├── gioitinh.txt            # Bảng Danh mục Giới tính (NAM, NU, KHAC)
│   ├── system.txt              # Cấu hình Điểm chuẩn, Điểm liệt, Trường, Năm
│   ├── ketqua_xettuyen.txt     # Báo cáo kết quả xét tuyển
│   ├── thukhoa.txt             # Báo cáo vinh danh thủ khoa
│   └── thongke_phodiem.txt     # Báo cáo phân tích phổ điểm
├── include/                    # Header files
│   ├── Constants.h             # Hằng số hệ thống (AdmissionConst)
│   ├── DanhMuc.h               # Định nghĩa Struct TinhThanh và GioiTinh
│   ├── Nguoi.h                 # Lớp cơ sở Nguoi
│   ├── ThiSinh.h               # Lớp dẫn xuất ThiSinh
│   ├── Vector.h                # Template mảng động tự viết + QuickSort
│   ├── MySQLConnector.h        # Wrapper MySQL C API an toàn, transaction
│   ├── QuanLyThiSinh.h         # Lớp quản trị nghiệp vụ & CSDL lai kép
│   ├── Utils.h                 # Các tiện ích kiểm tra & chuẩn hóa
│   ├── ConsoleUI.h             # Giao diện Console màu sắc UTF-8
│   └── AppGUI.h                # Giao diện đồ họa Dear ImGui DX9
├── sql/                        # Tập hợp kịch bản CSDL MySQL
│   ├── 01_create_database.sql  # Tạo CSDL và 5 bảng 3NF
│   ├── 02_seed_tinhthanh.sql   # Nạp 34 tỉnh/thành phố
│   ├── 03_seed_gioitinh.sql    # Nạp danh mục giới tính
│   ├── 04_seed_sample_data.sql # Nạp dữ liệu thí sinh & cấu hình
│   └── 05_advanced_queries.sql # Các câu lệnh truy vấn báo cáo nâng cao
└── src/                        # Implementation files (.cpp)
```

---

## 🚀 Hướng dẫn Cài đặt & Khởi chạy

### 1. Chuẩn bị Môi trường
- **Laragon**: Đã cài đặt trên máy tính (chứa MySQL 8.4 và PHPMyAdmin).
- **Trình biên dịch C++**: MinGW-w64 (GCC 9.2+ đi kèm Dev-C++ hoặc độc lập).

### 2. Khởi động MySQL trong Laragon
1. Mở ứng dụng **Laragon**.
2. Nhấn nút **"Start All"** (hoặc Start MySQL).
3. (Tùy chọn) Truy cập PHPMyAdmin qua trình duyệt: `http://localhost/phpmyadmin` (User: `root`, Password: để trống).

### 3. Biên dịch & Chạy Ứng dụng
- **Cách 1 (Tự động)**: Nhấp đúp vào file `build.bat`.
- **Cách 2 (Dòng lệnh PowerShell/CMD)**:
```powershell
# Chạy script biên dịch:
.\build.bat

# Hoặc khởi chạy trực tiếp GUI:
.\QuanLyThiSinh_PBL2.exe

# Hoặc khởi chạy Console:
.\QuanLyThiSinh_PBL2.exe --console
```

---

## 🔄 So sánh Hai Nhánh của Kho Mã Nguồn

| Tiêu chí | Nhánh `main` (`pbl2_MyProJect`) | Nhánh `mysql` (`pbl2_MySQL`) |
| :--- | :--- | :--- |
| **Mục đích** | Bản bảo vệ chính thức thuần C++ (Phòng giáo viên yêu cầu không dùng DBMS) | Bản nâng cao tích hợp CSDL quan hệ theo yêu cầu của GVHD |
| **Lưu trữ CSDL** | CSDL Quan hệ Đa tệp 3NF Key-Value (`nguoi.txt`, `thisinh.txt`,...) | MySQL 8.4 Server (`pbl2_tuyensinh`) + File Cache tự động |
| **Cấu trúc Dữ liệu** | `Vector<T>` động tự viết (Rule of Five + QuickSort) | `Vector<T>` động tự viết (Rule of Five + QuickSort) |
| **Khóa ngoại & Toàn vẹn** | Kiểm tra toàn vẹn tham chiếu bằng C++ code | Ràng buộc `FOREIGN KEY` + `ON DELETE CASCADE` của InnoDB |
| **Giao diện** | Dear ImGui DirectX 9 + Console | Dear ImGui DirectX 9 (Badge MySQL) + Console |
| **Phụ thuộc ngoài** | Chỉ cần DirectX 9 của Windows | Cần MySQL 8.4 (Laragon) + `libmysql.dll` |

---
*Tác giả: Võ Như Hiếu - Sinh viên thực hiện Đề tài 24, PBL2 Đồ án Cơ sở Lập trình, Đại học Bách khoa Đà Nẵng*
