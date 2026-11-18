# KẾ HOẠCH TRIỂN KHAI (IMPLEMENTATION PLAN)
## Refactor Chức Năng Khởi Tạo Dữ Liệu & Tối Ưu Hóa Giao Diện Bảng Quản Lý Thí Sinh (PBL2)

---

### I. TỔNG QUAN & BỐI CẢNH BÀI TOÁN

Hệ thống Quản lý Thí sinh Tuyển sinh Đại học Bách Khoa - ĐH Đà Nẵng (DUT) hiện tại đang gặp 2 vấn đề trọng yếu cần refactor toàn diện:

1. **Vấn đề Dữ liệu mẫu (Data Seeding):**
   - Hệ thống tự động chèn 36 bản ghi thí sinh mẫu hardcode vào cơ sở dữ liệu khi khởi chạy lần đầu.
   - Bộ dữ liệu này không đồng bộ với bộ 100 thí sinh xét tuyển chuyên sâu ngành **CNTT - Ngoại ngữ Nhật (`7480201A`)** mà môn đồ đã chuẩn bị trong file `generate_100_students.ps1` / `sql/05_seed_sample_data.sql`.
   - **Yêu cầu mới:** Ban đầu cơ sở dữ liệu thí sinh phải hoàn toàn **RỖNG** (chỉ có sẵn các danh mục hệ thống: Tỉnh thành, Giới tính, Khoa, Ngành, Năm học, Cấu hình). Thêm một nút `[+] Import Dữ Liệu Mẫu (100)` nằm ở **bên phải nút `[ Xuất Báo Cáo Excel ]`**. Khi người dùng chủ động click vào nút này, hệ thống mới nạp bộ 100 thí sinh mẫu vào MySQL và cập nhật lên giao diện.

2. **Vấn đề Lỗi Giao diện Bảng Quản lý Thí sinh (UI Bug - Candidates Table):**
   - Bảng hiện tại xuất hiện **thanh cuộn ngang (`ScrollX`)** dưới đáy, làm giảm tính thẩm mỹ và bất tiện khi thao tác.
   - Các cột quan trọng như **"Họ và Tên"** và **"Ngày Sinh"** bị che khuất/co rút nghiêm trọng (như hình chụp: cột Họ tên co lại chỉ còn 1-2 chữ cái `V<`, `N<`..., cột Ngày sinh chỉ còn `04`, `15`... bị mất tháng và năm).
   - Nhiều cột phía sau (Khoa, Ngành, Lớp, Mã SV DUT, Thao tác) bị đẩy trôi ra ngoài màn hình về bên phải.
   - **Yêu cầu mới:**
     - Loại bỏ hoàn toàn thanh cuộn ngang (`ScrollX`), bảng hiển thị vừa vặn 100% trên màn hình.
     - Các cột có nội dung dài (Họ tên, Quê quán, Kết quả, Khoa, Ngành trúng tuyển) sẽ tự động **xuống dòng (Text Wrap)** để bảo toàn toàn bộ chữ nghĩa.
     - **Ưu tiên hiển thị dữ liệu trên CÙNG MỘT HÀNG (Single Line - không bị ngắt dòng):**
       + `STT`
       + `SBD (Số báo danh)`
       + `Số CCCD`
       + `Ngày sinh` (`dd/mm/yyyy`)
       + `Giới tính`
       + `Toán`
       + `Lý`
       + `Hóa`
       + `Tổng điểm`
       + `Lớp sinh hoạt`
       + `Mã SV DUT`

---

### II. PHÂN TÍCH NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE ANALYSIS)

#### 1. Nguyên nhân Vấn đề Dữ liệu (36 records tự động import)
- Trong file `src/MySQLConnector.cpp`, hàm `seedDefaultData()` đang chứa các khối lệnh SQL hardcode từ dòng 404 đến 521, chèn 36 bản ghi thí sinh vào 3 bảng `nguoi`, `thisinh`, và `nguyen_vong`.
- Khi ứng dụng kết nối MySQL lần đầu, hàm này được tự động gọi, dẫn tới việc thí sinh bị nạp ngoài ý muốn thay vì để bảng rỗng cho người dùng tự chủ động import.

#### 2. Nguyên nhân Lỗi Giao diện Bảng Quản lý Thí sinh
- **Cờ `ImGuiTableFlags_ScrollX`:** Được bật trong `AppGUI.cpp` (dòng 948). Khi bật cờ này, ImGui cho phép tổng chiều rộng các cột vượt quá khung nhìn cửa sổ và kích hoạt cuộn ngang.
- **Xung đột cấu hình lưu vết trong `imgui.ini`:**
  - ImGui tự động lưu kích thước các cột của bảng `[Table][0x6B657D3F,17]` vào file `imgui.ini`.
  - Trong `imgui.ini` hiện tại, `Column 4` (Ngày Sinh) bị gán `Width=8px` (!), `Column 3` (Họ và Tên) bị chèn ép, trong khi `Column 16` (Thao Tác) chiếm tới `Width=658px`. Khi khởi động lại, ImGui nạp cấu hình méo mó này đè lên code, khiến các cột bị co rút kỳ dị.
- **Thiếu cơ chế ngắt dòng (`ImGui::PushTextWrapPos`):** Mặc định trong cell của ImGui, hàm `ImGui::Text()` sẽ cắt ngắn (clip) chuỗi nếu vượt quá ô chứ không tự động bẻ dòng xuống.

---

### III. GIẢI PHÁP THỰC HIỆN CHI TIẾT (DETAILED PROPOSED SOLUTION)

#### 1. Refactor Cơ chế Khởi tạo Dữ liệu & Nút Import 100 Thí sinh

##### A. Dọn sạch Seeding tự động trong CSDL
- Trong `src/MySQLConnector.cpp` - hàm `seedDefaultData()`:
  - Giữ nguyên seeding cho các bảng danh mục: `tinh_thanh`, `gioi_tinh`, `khoa`, `danh_muc_nganh`, `khoa_hoc`, `nganh_theo_khoa_hoc`, `cau_hinh`.
  - **Xóa bỏ hoàn toàn** các đoạn INSERT vào `nguoi`, `thisinh`, `nguyen_vong`.
  - Đảm bảo khi hệ thống chạy lên với database mới, danh sách thí sinh là **rỗng 100%**.

##### B. Hiện thực hóa chức năng `importDuLieuMau100()`
- Bổ sung hàm trong `QuanLyThiSinh` / `MySQLConnector`:
  ```cpp
  bool importDuLieuMau100(const string& sqlFilePath = "sql/05_seed_sample_data.sql");
  ```
- **Quy trình xử lý của hàm:**
  1. Kiểm tra kết nối MySQL.
  2. Bắt đầu transaction (`beginTransaction()`).
  3. Đọc nội dung file `sql/05_seed_sample_data.sql`.
  4. Tách và thực thi tuần tự các khối lệnh `INSERT INTO cau_hinh`, `INSERT INTO nguoi`, `INSERT INTO thisinh`, `INSERT INTO nguyen_vong`.
  5. Commit transaction.
  6. Đồng bộ lại dữ liệu lên bộ nhớ RAM (`docDuLieuTuMySQL()`).
  7. Trả về `true` khi thành công, hoặc `false` kèm thông báo lỗi.

##### C. Thêm Nút `[+] Import 100 Thí Sinh Mẫu` trên Giao diện (`AppGUI.cpp`)
- Đặt nút bấm tại vị trí ngay bên phải nút `[ Xuất Báo Cáo Excel ]` (hàng công cụ trên cùng của Tab 1):
  ```cpp
  // Sau nút [ Xuat Bao Cao Excel ]
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.78f, 0.42f, 0.15f, 1.0f)); // Màu cam hổ phách nổi bật
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.50f, 0.20f, 1.0f));
  if (ImGui::Button("[+] Import Du Lieu Mau (100)", ImVec2(195, 28))) {
      bool ok = ql.importDuLieuMau100("sql/05_seed_sample_data.sql");
      if (ok) {
          currentPage = 1;
          selectedRow = -1;
          showToast = true;
          toastMsg = "Da import thanh cong 100 thi sinh xet tuyen CNTT Nhat (K26)!";
      } else {
          showToast = true;
          toastMsg = "Loi: Khong the import du lieu mau. Kiem tra file sql!";
      }
  }
  ImGui::PopStyleColor(2);
  ```
- Điều chỉnh lại vị trí căn phải của nhóm nút công cụ (`ImGui::SameLine(...)`) để cả 3 nút `[+] Thêm Thí Sinh`, `[ Xuất Báo Cáo Excel ]` và `[+] Import Dữ Liệu Mẫu (100)` nằm ngay ngắn, không bị đè lên bộ lọc.

---

#### 2. Refactor Toàn diện Bảng Quản lý Thí sinh (Candidates Table)

##### A. Thay đổi Bảng Flags & Vô hiệu hóa Cache Hỏng trong `imgui.ini`
- **Loại bỏ `ImGuiTableFlags_ScrollX`**.
- Thêm cờ `ImGuiTableFlags_NoSavedSettings` để bảng không bị phụ thuộc vào kích thước lưu sai trong `imgui.ini`.
- Đổi tên định danh bảng từ `"CandidatesTable"` thành `"CandidatesTable_v2"` để giải phóng hoàn toàn cache cũ.
- Thiết lập cờ định cỡ: `ImGuiTableFlags_SizingStretchProp` (tất cả các cột co giãn cân đối theo tỷ lệ màn hình).

##### B. Phân bổ Kích thước & Quy tắc Hiển thị cho 17 Cột
Dựa trên chiều rộng màn hình tiêu chuẩn (từ 1280px đến 1920px), bảng sẽ được phân chia làm 2 nhóm cột rõ ràng:

| STT | Cột | Kiểu Định Cỡ | Kích Thước / Trọng Số | Quy Tắc Hiển Thị |
|---|---|---|---|---|
| 1 | **STT** | `WidthFixed` | 36px | **1 hàng duy nhất**, căn giữa |
| 2 | **SBD (PK)** | `WidthFixed` | 75px | **1 hàng duy nhất**, màu xanh dương sáng |
| 3 | **Số CCCD (FK)**| `WidthFixed` | 108px | **1 hàng duy nhất**, đủ 12 chữ số |
| 4 | **Họ và Tên** | `WidthStretch`| Trọng số `1.4f` | **Cho phép xuống dòng** nếu tên dài |
| 5 | **Ngày Sinh** | `WidthFixed` | 86px | **1 hàng duy nhất** (`dd/mm/yyyy`) |
| 6 | **Quê Quán** | `WidthStretch`| Trọng số `1.0f` | **Cho phép xuống dòng** (tên tỉnh/TP dài) |
| 7 | **Giới Tính** | `WidthFixed` | 55px | **1 hàng duy nhất** (Nam / Nữ / Khác) |
| 8 | **Toán** | `WidthFixed` | 42px | **1 hàng duy nhất**, căn phải/giữa |
| 9 | **Lý** | `WidthFixed` | 42px | **1 hàng duy nhất**, căn phải/giữa |
| 10 | **Hóa** | `WidthFixed` | 42px | **1 hàng duy nhất**, căn phải/giữa |
| 11 | **Tổng Điểm** | `WidthFixed` | 58px | **1 hàng duy nhất**, chữ vàng đậm nét |
| 12 | **Kết Quả** | `WidthStretch`| Trọng số `1.1f` | **Cho phép xuống dòng** (Trúng tuyển, Đang xét...) |
| 13 | **Khoa** | `WidthStretch`| Trọng số `0.9f` | **Cho phép xuống dòng** |
| 14 | **Ngành Trúng Tuyển**| `WidthStretch`| Trọng số `1.3f` | **Cho phép xuống dòng** (tên ngành dài) |
| 15 | **Lớp Sinh Hoạt** | `WidthFixed`| 92px | **1 hàng duy nhất** (ví dụ `26T_N1`, `26TCLC_KHDL1`) |
| 16 | **Mã SV DUT** | `WidthFixed` | 88px | **1 hàng duy nhất** (9 chữ số `102260001`) |
| 17 | **Thao Tác** | `WidthFixed` | 105px | **1 hàng duy nhất**, chứa 3 nút [Xem], [Sửa], [Xóa] |

> **Tổng chiều rộng cố định của 11 cột ưu tiên 1 dòng + cột Thao Tác:**  
> $36 + 75 + 108 + 86 + 55 + 42 + 42 + 42 + 58 + 92 + 88 + 105 = 830\text{px}$.  
> Phần không gian còn lại (từ $400\text{px}$ đến $800\text{px}$ tùy độ phân giải màn hình) sẽ được 5 cột co giãn (`Họ và Tên`, `Quê Quán`, `Kết Quả`, `Khoa`, `Ngành Trúng Tuyển`) chia nhau theo tỷ lệ và tự động xuống dòng đẹp mắt khi cần.

##### C. Cơ chế Tự Động Xuống Dòng (Text Wrap) trong Cell
Trong ImGui, để một ô văn bản tự động xuống dòng khi nội dung chạm mép cột:
```cpp
// Helper ngắt dòng an toàn trong cell của bảng
void RenderWrappedText(const string& text, const ImVec4& color = ImVec4(0.92f, 0.94f, 0.96f, 1.0f)) {
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + availWidth);
    ImGui::TextColored(color, "%s", text.c_str());
    ImGui::PopTextWrapPos();
}
```

##### D. Tinh Chỉnh Cột Thao Tác (Action Buttons)
- Thu gọn 3 nút bấm `Xem`, `Sửa`, `Xóa` với kích thước gọn gàng `30x20` px hoặc `32x22` px, đặt trên cùng một dòng bằng `ImGui::SameLine()`, có padding nhỏ (2-3px) để nằm trọn vẹn trong cột 105px mà không bị tràn hay xuống dòng.

---

### IV. CÁC TỆP TIN CẦN THAY ĐỔI & TÁC ĐỘNG

1. **`include/MySQLConnector.h` & `src/MySQLConnector.cpp`:**
   - Xóa bỏ việc seed 36 thí sinh trong `seedDefaultData()`.
   - Bổ sung phương thức `executeSqlScript(const string& filePath)` hoặc logic nạp dữ liệu từ file SQL.
2. **`include/QuanLyThiSinh.h` & `src/QuanLyThiSinh.cpp`:**
   - Thêm phương thức `importDuLieuMau100(const string& filePath)`.
3. **`src/AppGUI.cpp`:**
   - Bổ sung nút `[+] Import Du Lieu Mau (100)` bên phải nút Xuất Excel.
   - Refactor cấu hình bảng `CandidatesTable_v2`: Bỏ `ScrollX`, gán flags `StretchProp`, thiết lập độ rộng chuẩn cho 17 cột, bọc Text Wrap cho các cột chuỗi dài.
4. **`imgui.ini`:**
   - Dọn sạch section table cache bị lỗi của bảng cũ.

---

### V. KẾ HOẠCH KIỂM THỬ (TEST PLAN)

1. **Kiểm tra trạng thái ban đầu:**
   - Khởi động ứng dụng khi database trống: Xác nhận danh sách thí sinh hiển thị 0 bản ghi, không còn 36 bản ghi lạ.
2. **Kiểm tra chức năng nút Import 100 Thí sinh:**
   - Nhấn nút `[+] Import Du Lieu Mau (100)`:
     + Toast thông báo thành công xuất hiện.
     + Bảng nạp đủ 100 thí sinh thuộc bộ hồ sơ K26 CNTT Nhật.
     + Dữ liệu tổng điểm, nguyện vọng khớp 100% với file `05_seed_sample_data.sql`.
3. **Kiểm tra Giao diện Bảng không còn Cuộn Ngang:**
   - Thu nhỏ, phóng to cửa sổ: Bảng tự động co giãn vừa vặn, **không hề có thanh cuộn ngang dưới đáy**.
   - Kiểm tra các cột ưu tiên 1 dòng (`STT`, `SBD`, `CCCD`, `Ngày sinh`, `Giới tính`, `Toán`, `Lý`, `Hóa`, `Tổng`, `Lớp`, `Mã SV`): Đảm bảo hiển thị đầy đủ, không bị cắt cụt ký tự, không bị ngắt dòng.
   - Kiểm tra các cột chuỗi dài (`Họ và tên`, `Quê quán`, `Ngành trúng tuyển`): Hiển thị đầy đủ tiếng Việt, tự động xuống dòng thẩm mỹ khi độ rộng cột thu hẹp.
4. **Kiểm tra Biên dịch & Chạy thực tế:**
   - Chạy `build.bat` biên dịch không lỗi, chạy thử ứng dụng xác nhận trải nghiệm người dùng mượt mà.
