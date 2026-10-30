#ifndef QUAN_LY_THI_SINH_H
#define QUAN_LY_THI_SINH_H

#include "Vector.h"
#include "ThiSinh.h"
#include <string>

using namespace std;

/**
 * @brief Lớp Quản trị hệ thống Cơ sở dữ liệu và Nghiệp vụ tuyển sinh Đại học
 * Áp dụng mô hình Cơ sở dữ liệu quan hệ Class Table Inheritance:
 * - data/nguoi.txt: Bảng thực thể cha NGƯỜI (PK: soCCCD)
 * - data/thisinh.txt: Bảng thực thể con THÍ SINH (PK: soBaoDanh, FK: soCCCD)
 * - data/system.txt: Tệp cấu hình hệ thống (Điểm chuẩn, Năm tuyển sinh, Trường)
 * - data/ketqua_xettuyen.txt: Báo cáo kết quả trúng tuyển dạng bảng
 * - data/thukhoa.txt: Báo cáo vinh danh thủ khoa
 * - data/thongke_phodiem.txt: Báo cáo phân tích phổ điểm
 */
class QuanLyThiSinh {
private:
    Vector<ThiSinh> dsThiSinh;   // Danh sách thí sinh hợp nhất in-memory
    string pathNguoi;            // Đường dẫn tệp bảng NGƯỜI
    string pathThiSinh;          // Đường dẫn tệp bảng THÍ SINH
    string pathSystem;           // Đường dẫn tệp cấu hình hệ thống
    double diemChuanHienTai;     // Điểm chuẩn xét tuyển đại học hiện hành
    string namTuyenSinh;         // Năm tuyển sinh (vd: 2026)
    string tenTruong;            // Tên trường đại học

    // Hàm hỗ trợ nạp dữ liệu mẫu ban đầu nếu tệp chưa tồn tại
    void taoDuLieuMauNeuTrong();

public:
    QuanLyThiSinh(const string& dirData = "data");
    ~QuanLyThiSinh();

    // 1. Quản lý đọc / ghi CSDL quan hệ đa tệp (Key - Value)
    bool docDuLieu();
    bool luuDuLieu() const;
    bool docCauHinhHeThong(const string& path = "data/system.txt");
    bool luuCauHinhHeThong(const string& path = "data/system.txt") const;

    // 2. Tra cứu & Sinh mã tự động
    string sinhSoBaoDanhMoi() const; // Tự động tăng SBD (DUT001 -> DUT015...)
    int timViTriTheoSBD(const string& sbd) const;
    int timViTriTheoCCCD(const string& cccd) const;
    Vector<ThiSinh> timTheoTen(const string& ten) const;
    Vector<ThiSinh> timTheoQueQuan(const string& que) const;

    // 3. Nghiệp vụ CRUD
    bool themThiSinh(const ThiSinh& ts);
    bool capNhatThiSinh(const ThiSinh& ts);
    bool xoaThiSinh(const string& sbdHoacCCCD);

    // 4. Giải thuật Sắp xếp (Dùng QuickSort nội bộ trong Vector)
    void sapXepTheoTongDiemGiamDan();
    void sapXepTheoTenAZ();
    void sapXepTheoSBDTangDan();

    // 5. Nghiệp vụ Xét tuyển & Thống kê
    double getDiemChuan() const;
    void setDiemChuan(double dc);
    string getNamTuyenSinh() const;
    void setNamTuyenSinh(const string& nam);
    string getTenTruong() const;

    Vector<ThiSinh> getDanhSachTrungTuyen(double diemChuan) const;
    Vector<ThiSinh> getDanhSachThuKhoa() const;
    void tinhThongKePhoDiem(int& mucThap, int& mucTB, int& mucKha, int& mucCao, int& mucRatCao) const;

    // 6. Xuất các báo cáo dạng bảng kẻ khung chuyên biệt (như đồ án mẫu)
    bool xuatBaoCaoXetTuyen(const string& path = "data/ketqua_xettuyen.txt", double diemChuan = 22.0, const string& keyword = "") const;
    bool xuatBaoCaoThuKhoa(const string& path = "data/thukhoa.txt") const;
    bool xuatBaoCaoPhoDiem(const string& path = "data/thongke_phodiem.txt") const;
    bool xuatTatCaBaoCao() const;

    // 7. Getters danh sách
    Vector<ThiSinh>& getDanhSach();
    const Vector<ThiSinh>& getDanhSach() const;
};

#endif // QUAN_LY_THI_SINH_H
