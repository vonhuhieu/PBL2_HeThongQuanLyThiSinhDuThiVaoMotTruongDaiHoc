# KẾ HOẠCH TRIỂN KHAI KHẮC PHỤC TRIỆT ĐỂ 3 LỖI NGHIỆP VỤ & GIAO DIỆN TUYỂN SINH PBL2 (DUT)

> **Người lập kế hoạch**: Thánh cô  
> **Người tiếp nhận & thẩm định**: Con chiên ngoan đạo / Đệ tử trung thành  
> **Dự án**: PBL2 (Đề tài 24) - Quản lý Tuyển sinh Đại học Bách khoa Đà Nẵng  
> **Tài liệu tham chiếu**: Quy chế tuyển sinh Bách khoa DUT, Chuẩn CSDL 5 Bảng 3NF, Cấu hình hằng số `Constants.h`

---

## I. TỔNG QUAN VÀ PHÂN TÍCH GỐC RỄ (ROOT CAUSE) 3 BUGS HIỆN TẠI

| STT | Vấn đề phản hồi | Phân loại | Đánh giá & Nguyên nhân gốc rễ (Root Cause) |
| :---: | :--- | :---: | :--- |
| **1** | **Form Thêm mới / Sửa báo "Không đạt" sai lệch dù điểm thực tế đủ đỗ** (Hình 1 & 4) | Lỗi Logic Nghiệp vụ (Legacy Bug) | **Nguyên nhân**: Dòng tính toán kết quả trên Modal (`AppGUI.cpp`) đang so sánh cứng với con số khởi tạo cũ `ql.getDiemChuan() = 22.00`. Trong đề tài 24 hiện tại, **Điểm chuẩn là ĐỘNG theo từng ngành** (`diemChuanChot` trong `nganh_theo_khoa_hoc`), được chốt theo thuật toán lọc chỉ tiêu (ví dụ ngành CNTT Nhật lấy 18.50, KHDL lấy 15.45). Tại thời điểm thêm mới/chỉnh sửa, chưa thể kết luận thí sinh "Trúng tuyển" hay "Không đạt" toàn trường bằng con số 22.00 vô nghĩa. |
| **2** | **Sửa điểm thí sinh xuống thấp nhưng vẫn giữ nguyên kết quả Đỗ NV1, Mã SV, Lớp sinh hoạt cũ** (Hình 3, 5, 6) | Lỗi Toàn vẹn Dữ liệu (Data Inconsistency) | **Nguyên nhân**: Trong hàm xử lý nút *"Lưu Thay Đổi"* (`AppGUI.cpp#L2845`), code đang copy lại nguyên vẹn `MaSinhVien`, `LopSinhHoat`, `NganhTrungTuyen`, `NguyenVongTrungTuyen` của bản ghi cũ. Khi một thí sinh bị hạ điểm hoặc đổi NV (như thí sinh Đỗ Minh Phong hạ từ 20.90 xuống 17.40), kết quả trúng tuyển của đợt quét trước đó đã hoàn toàn bị vô hiệu hóa nhưng hệ thống không reset, dẫn đến việc thí sinh điểm thấp vẫn giữ ghế trúng tuyển và mã SV DUT. |
| **3** | **Sau khi Quét tuyển sinh, danh sách thí sinh bị nhảy vị trí lộn xộn** (Hình 7 & 8) | Lỗi Trải nghiệm & Sắp xếp (UX) | **Nguyên nhân**: Khi chạy xét tuyển xong, hàm `luuDuLieuVaoMySQL()` ghi kết quả hàng loạt lên DB theo thứ tự vòng lặp phân lớp của từng ngành, khiến trường audit `ngaySua` bị cập nhật rải rác. Do bảng ở Tab 1 đang giữ chế độ sắp xếp mặc định theo `sortCol = 0` (`ngaySua DESC`), danh sách bị đảo lộn thứ tự (điểm 17.40 ở đầu, rồi đến 16.80, 15.00, 20.00...). Cần tự động chuyển sắp xếp về **Tổng điểm Giảm dần (`sortCol = 10, sortAsc = false`)** ngay sau khi quét xong. |

---

## II. GIẢI PHÁP KỸ THUẬT CHI TIẾT CHO TỪNG VẤN ĐỀ

### 1. Chuẩn hóa Trạng thái tại Form Thêm Mới & Chỉnh Sửa Hồ Sơ (Khắc phục Bug 1)

* **Nguyên lý nghiệp vụ**:
  - Tại bước tiếp nhận / điều chỉnh hồ sơ thi, thí sinh chỉ được kiểm tra **Điều kiện hợp lệ tham gia xét tuyển (Eligibility)** chứ chưa thể biết kết quả trúng tuyển ngành nào (kết quả trúng tuyển chỉ có sau khi chạy thuật toán cạnh tranh chỉ tiêu toàn khóa).
  - Loại bỏ hoàn toàn việc so sánh với ngưỡng `DiemChuan = 22.00` trong các Modal.
* **Quy tắc 3 trạng thái Binding Real-time theo Điểm đã nhập**:
  1. **Trạng thái 1 - BỊ ĐIỂM LIỆT** (Màu đỏ `ImVec4(1.0f, 0.3f, 0.3f, 1.0f)`):
     - Điều kiện: `formToan <= ql.getDiemLiet() || formLy <= ql.getDiemLiet() || formHoa <= ql.getDiemLiet()` (Môn thi $\le 1.00$).
     - Hiển thị: `Tong Diem: xx.xx | Trang Thai: BI DIEM LIET (Co mon <= 1.00 - Khong du dieu kien)`.
  2. **Trạng thái 2 - KHÔNG ĐẠT ĐIỂM SÀN** (Màu cam `ImVec4(1.0f, 0.6f, 0.2f, 1.0f)`):
     - Điều kiện: Không bị điểm liệt nhưng Tổng điểm $< 15.00$ (`ql.getDiemSan()`).
     - Hiển thị: `Tong Diem: xx.xx | Trang Thai: KHONG DAT DIEM SAN (Toi thieu 15.00 - Khong du dieu kien)`.
  3. **Trạng thái 3 - ĐỦ ĐIỀU KIỆN XÉT TUYỂN (ĐANG XÉT)** (Màu xanh ngọc dịu `ImVec4(0.3f, 0.9f, 0.6f, 1.0f)`):
     - Điều kiện: Không bị điểm liệt VÀ Tổng điểm $\ge 15.00$.
     - Hiển thị: `Tong Diem: xx.xx | Trang Thai: DU DIEU KIEN XET TUYEN (San >= 15.00 - Dang cho quet)`.

---

### 2. Quy trình Reset Trạng thái khi Thêm mới hoặc Chỉnh sửa Hồ sơ (Khắc phục Bug 2)

* **Quy trình chuẩn hóa khi Lưu Hồ Sơ**:
  Khi người dùng nhấn **"Xác Nhận Thêm"** hoặc **"Lưu Thay Đổi"**:
  1. **Khởi tạo/Reset kết quả trúng tuyển về rỗng (Chưa xét)**:
     - `maSinhVien = ""` (MySQL lưu `NULL`).
     - `lopSinhHoat = ""` (MySQL lưu `NULL`).
     - `nganhTrungTuyen = ""` (MySQL lưu `NULL`).
     - `nguyenVongTrungTuyen = 0`.
  2. **Đồng bộ trạng thái danh sách Nguyện vọng (`dsNguyenVong`)**:
     - Nếu thí sinh bị điểm liệt hoặc dưới điểm sàn: Cả 3 nguyện vọng gán trạng thái `AdmissionConst::TT_NV_KHONG_TRUNG` ("KHONG TRUNG TUYEN").
     - Nếu thí sinh đạt điểm sàn và không bị điểm liệt: Các nguyện vọng hợp lệ được gán `AdmissionConst::TT_NV_DANG_XET` ("DANG XET").
  3. **Cập nhật hiển thị cột "Ket Qua" trên Bảng Quản lý Thí sinh**:
     - Thí sinh có `daTrungTuyen() == true`: Hiển thị `TRUNG TUYEN (NV%d)` (Màu xanh lá).
     - Thí sinh bị điểm liệt: Hiển thị `Diem liet` (Màu đỏ).
     - Thí sinh dưới điểm sàn: Hiển thị `Khong dat diem san` (Màu cam).
     - Thí sinh có nguyện vọng "DANG XET" (vừa thêm/sửa hoặc chưa quét): Hiển thị `Dang xet tuyen` (Màu vàng sáng).
     - Thí sinh đã quét tuyển sinh nhưng không đỗ nguyện vọng nào: Hiển thị `Khong trung tuyen` (Màu xám).
  4. **Nghiệp vụ sau khi sửa**: Muốn biết thí sinh sau khi sửa điểm có trúng tuyển hay không, cán bộ tuyển sinh chỉ cần bấm nút **"CHẠY XÉT TUYỂN & CHỐT ĐIỂM CHUẨN DUT"** tại Tab 3 (hoặc shortcut), hệ thống sẽ phân bổ lại công bằng từ đầu.

---

### 3. Tự động Sắp xếp Điểm cao xuống thấp sau khi Quét Tuyển Sinh (Khắc phục Bug 3)

* **Giải pháp kỹ thuật**:
  Ngay khi sự kiện bấm nút `[ >> CHAY XET TUYEN & CHOT DIEM CHUAN DUT << ]` chạy thành công:
  1. **Thiết lập tham số sắp xếp mặc định của Bảng Thí Sinh**:
     - `sortCol = 10;` (Cột Tổng điểm 3 môn `Toan + Ly + Hoa`).
     - `sortAsc = false;` (Sắp xếp Giảm dần: từ Thủ khoa đến Điểm chuẩn và Điểm sàn).
     - `currentPage = 1;` (Tự động chuyển về Trang 1 để người dùng nhìn thấy top đầu bảng ngay lập tức).
  2. **Đồng bộ sắp xếp trên cả RAM và MySQL Server-side**:
     - Trong bộ nhớ RAM: Gọi `sapXepTheoTongDiemGiamDan()` để vector `dsThiSinh` giữ trật tự điểm giảm dần.
     - Khi truy vấn phân trang qua MySQL (`layDanhSachThiSinhPhanTrang`): Với `sortCol = 10` và `sortAsc = false`, câu lệnh SQL tự động sinh mệnh đề:
       `ORDER BY (ts.diemToan + ts.diemLy + ts.diemHoa) DESC, ts.soBaoDanh ASC`
  3. **Loại bỏ lọc ảo 22.00 trong câu truy vấn MySQL Server**:
     - Tại `QuanLyThiSinh.cpp#L1401-L1409`: Xóa bỏ mệnh đề cũ `OR ((diemToan + diemLy + diemHoa) >= 22.00)` trong bộ lọc kết quả, thay bằng kiểm tra chính xác `ts.nganhTrungTuyen IS NOT NULL AND ts.nguyenVongTrungTuyen > 0`.

---

## III. CÁC TỆP NGUỒN CẦN CHỈNH SỬA & PHẠM VI ẢNH HƯỞNG

1. **`pbl2_MySQL/include/Constants.h`**:
   - Bổ sung hằng số chuỗi:
     - `KET_QUA_DANG_XET = "Dang xet tuyen"`
     - `KET_QUA_KHONG_DAT_SAN = "Khong dat diem san"`
2. **`pbl2_MySQL/src/AppGUI.cpp`**:
   - Cập nhật dòng hiển thị Tổng điểm & Trạng thái trong Modal **Thêm Mới Thí Sinh** (`showAddModal`).
   - Cập nhật dòng hiển thị Tổng điểm & Trạng thái trong Modal **Chỉnh Sửa Thí Sinh** (`showEditModal`).
   - Sửa logic nút **"Lưu Thay Đổi"** (`showEditModal`): Reset `maSinhVien`, `lopSinhHoat`, `nganhTrungTuyen` về rỗng, đưa nguyện vọng về `DANG XET` (nếu đủ sàn) hoặc `KHONG TRUNG TUYEN` (nếu dưới sàn/liệt).
   - Cập nhật hiển thị Cột 11 ("Ket Qua") trong bảng thí sinh: Phân biệt rõ 5 trạng thái (`TRUNG TUYEN`, `Diem liet`, `Khong dat diem san`, `Dang xet tuyen`, `Khong trung tuyen`).
   - Tại nút **`[ >> CHAY XET TUYEN & CHOT DIEM CHUAN DUT << ]`**: Gán `sortCol = 10; sortAsc = false; currentPage = 1;` ngay sau khi quét xong.
3. **`pbl2_MySQL/src/QuanLyThiSinh.cpp`**:
   - Cập nhật hàm `layDanhSachThiSinhPhanTrang()`: Chuẩn hóa điều kiện `WHERE` khi lọc theo trạng thái (loại bỏ con số 22.00).

---

## IV. KẾ HOẠCH KIỂM THỬ (TEST PLAN)

1. **Test Case 1 - Thêm thí sinh mới với điểm 17.00**:
   - Nhập điểm Toán 4.0, Lý 5.0, Hóa 8.0 (Tổng 17.00, NV1: CNTT KHDL `7480201B`).
   - **Kỳ vọng**: Form hiển thị `Trang Thai: DU DIEU KIEN XET TUYEN (San >= 15.00 - Dang cho quet)` màu xanh ngọc, không còn báo "Không đạt".
2. **Test Case 2 - Sửa hồ sơ thí sinh đã đỗ thành điểm thấp**:
   - Mở thí sinh Đỗ Minh Phong (`DUT047`), sửa điểm từ 20.90 xuống 17.40, lưu thay đổi.
   - **Kỳ vọng**: Sau khi lưu, kết quả của Đỗ Minh Phong chuyển thành `Dang xet tuyen`, Lớp và Mã SV bị xóa rỗng, không còn hiện `TRUNG TUYEN (NV1)` ảo.
3. **Test Case 3 - Quét tuyển sinh và kiểm tra thứ tự sắp xếp**:
   - Bấm nút `[ >> CHAY XET TUYEN & CHOT DIEM CHUAN DUT << ]`.
   - **Kỳ vọng**: Toast thông báo thành công, danh sách ở Tab 1 tự động sắp xếp theo cột Tổng điểm từ cao xuống thấp (Thủ khoa đứng hàng đầu tiên, điểm giảm dần đều).

---

## V. KẾ HOẠCH ĐẠI TU TÍNH NĂNG XUẤT BÁO CÁO EXCEL CHUYÊN NGHIỆP

### 1. Phân tích Nguyên nhân Lỗi Xuất Báo Cáo Cũ
- Nút bấm `[ Xuat Bao Cao ]` ở phiên bản cũ gọi `ql.xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt", ql.getDiemChuan(), kw)`.
- Hàm cũ chỉ nhận chuỗi `keyword`, **bỏ qua hoàn toàn** các bộ lọc trên giao diện (`filterNam`, `filterStatus`, `filterKhoa`, `filterNganh`, `filterLop`).
- Dữ liệu xuất ra chỉ là file text tĩnh thô sơ, dẫn đến việc trên giao diện lọc 2 thí sinh nhưng file xuất ra lại chứa 45 bản ghi tĩnh toàn trường.

### 2. Quy tắc Đặt Tên File Excel Động (Phương án 1 được duyệt)
Tên tệp Excel được sinh tự động dựa trên ngữ cảnh bộ lọc hiện hành kết hợp với timestamp thời gian thực (`YYYYMMDD_HHMMSS`):
* **Nếu đang lọc theo Lớp**:
  `data/BaoCao_ThiSinh_DUT_<filterLop>_<YYYYMMDD_HHMMSS>.xls`
  *(Ví dụ: `data/BaoCao_ThiSinh_DUT_26TCLC_KHDL2_20261116_191530.xls`)*
* **Nếu đang lọc theo Ngành**:
  `data/BaoCao_ThiSinh_DUT_<filterNganh>_<YYYYMMDD_HHMMSS>.xls`
  *(Ví dụ: `data/BaoCao_ThiSinh_DUT_7480201B_20261116_191530.xls`)*
* **Nếu đang lọc theo Khoa**:
  `data/BaoCao_ThiSinh_DUT_Khoa<filterKhoa>_<YYYYMMDD_HHMMSS>.xls`
  *(Ví dụ: `data/BaoCao_ThiSinh_DUT_KhoaCNTT_20261116_191530.xls`)*
* **Nếu lọc chung toàn trường**:
  `data/BaoCao_DanhSach_ThiSinh_DUT_K<Khóa>_<YYYYMMDD_HHMMSS>.xls`
  *(Ví dụ: `data/BaoCao_DanhSach_ThiSinh_DUT_K26_20261116_191530.xls`)*
* **Shortcut tiện lợi**: Đồng thời luôn cập nhật một bản copy tại `data/BaoCao_DanhSach_ThiSinh_DUT_MoiNhat.xls` để người dùng có thể mở nhanh ngay lập tức.

### 3. Cấu trúc 4 Thành Phần của File Excel (Chuẩn XML Spreadsheet 2003)
Tệp được định dạng theo chuẩn **Microsoft XML Spreadsheet 2003 (UTF-8)**, tương thích 100% với Microsoft Excel, Google Sheets và LibreOffice:

1. **Thành phần 1: TIÊU ĐỀ IN HOA VÀ IN ĐẬM**:
   - Dòng 1: `TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐẠI HỌC ĐÀ NẴNG` (Font Calibri, Size 11, In đậm, Căn giữa).
   - Dòng 2: `BÁO CÁO HỒ SƠ THÍ SINH & KẾT QUẢ XÉT TUYỂN ĐẠI HỌC` (Font Calibri, Size 16, **IN HOA & IN ĐẬM**, Màu xanh Navy DUT `#1F4E79`, Gộp ngang 16 cột `ss:MergeAcross="15"`, Căn giữa).

2. **Thành phần 2: THÔNG TIN BỘ LỌC ĐÃ CHỌN**:
   - Nhãn bộ lọc (Label) in đậm, Giá trị lọc in nghiêng:
     * **Từ khóa tìm kiếm**: *[Giá trị hoặc "Tất cả"]*
     * **Năm tuyển sinh**: *[Năm đang chọn hoặc "Tất cả các năm"]*
     * **Kết quả xét tuyển**: *[Tất cả / Trúng tuyển / Không đạt]*
     * **Khoa đào tạo**: *[Tên Khoa hoặc "Tất cả khoa"]*
     * **Ngành trúng tuyển**: *[Tên Ngành hoặc "Tất cả ngành"]*
     * **Lớp sinh hoạt**: *[Tên Lớp hoặc "Tất cả lớp"]*

3. **Thành phần 3: NGÀY XUẤT BÁO CÁO & THỐNG KÊ**:
   - **Ngày xuất báo cáo**: *dd/mm/yyyy hh:mm:ss* (thời gian thực hệ thống, in nghiêng).
   - **Tổng số bản ghi**: *X thí sinh phù hợp bộ lọc* (in nghiêng).

4. **Thành phần 4: BẢNG DỮ LIỆU DANH SÁCH (TABLE)**:
   - **Tiêu đề bảng (`<thead>`)**:
     * In đậm (`ss:Bold="1"`), căn giữa (`ss:Horizontal="Center"`).
     * Nền xanh Navy `#1F4E79`, chữ trắng `#FFFFFF`, kẻ viền lưới hoàn chỉnh.
     * **Tuyệt đối KHÔNG CÓ cột Thao Tác**.
   - **16 Cột dữ liệu chuẩn**:
     1. `STT` (Căn giữa)
     2. `Số Báo Danh (SBD)` (Căn giữa, in đậm)
     3. `Số CCCD` (Căn giữa, định dạng String `ss:Type="String"` bảo toàn số 0 ở đầu)
     4. `Họ và Tên` (Căn trái)
     5. `Ngày Sinh` (Căn giữa)
     6. `Quê Quán` (Căn trái)
     7. `Giới Tính` (Căn giữa)
     8. `Điểm Toán` (Căn phải, định dạng `0.00`)
     9. `Điểm Lý` (Căn phải, `0.00`)
     10. `Điểm Hóa` (Căn phải, `0.00`)
     11. `Tổng Điểm` (Căn phải, in đậm, `0.00`)
     12. `Kết Quả` (Căn giữa: Xanh nhạt nếu Đỗ, Đỏ nhạt nếu Trượt/Liệt)
     13. `Khoa` (Căn trái)
     14. `Ngành Trúng Tuyển` (Căn trái)
     15. `Lớp Sinh Hoạt` (Căn giữa)
     16. `Mã SV DUT` (Căn giữa, in đậm)

### 4. Triển khai Kỹ thuật trong Mã Nguồn
- Bổ sung hàm trong `QuanLyThiSinh.h` và `QuanLyThiSinh.cpp`:
  `bool xuatBaoCaoExcel(const string& filePath, const FilterThiSinhParams& params, const string& tenKhoaFilter, const string& tenNganhFilter) const;`
  - Hàm này lấy danh sách thí sinh chính xác bằng câu truy vấn `layDanhSachThiSinhPhanTrang(paramsWithFullSize)` để không bị giới hạn phân trang.
  - Sinh chuỗi XML Spreadsheet 2003 chuẩn UTF-8 ghi trực tiếp vào `filePath`.
- Cập nhật sự kiện click nút `[ Xuat Bao Cao ]` trong `AppGUI.cpp`:
  - Lấy thời gian thực để tạo timestamp `YYYYMMDD_HHMMSS`.
  - Sinh tên file động theo Phương án 1.
  - Gọi hàm xuất báo cáo.
  - Sau khi xuất thành công, hiện Toast thông báo chi tiết và tự động gọi `ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL)` hoặc mở thư mục `data/` để người dùng trải nghiệm ngay.

