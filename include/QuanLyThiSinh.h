#ifndef QUAN_LY_THI_SINH_H
#define QUAN_LY_THI_SINH_H

#include "Vector.h"
#include "ThiSinh.h"
#include "DanhMuc.h"
#include "Constants.h"
#include "MySQLConnector.h"
#include <string>

using namespace std;

/**
 * @brief Lớp Quản trị hệ thống Cơ sở dữ liệu và Nghiệp vụ tuyển sinh Đại học
 * Kiến trúc lai (Hybrid Architecture):
 * 1. MySQL Engine (Laragon 8.4): Quản trị cơ sở dữ liệu quan hệ 5 bảng 3NF
 * 2. In-Memory Vector Engine: Cấu trúc dữ liệu mảng động tự cài đặt (DSA & OOP)
 * 3. File System Cache: Bộ đệm tệp tin sao lưu đa tệp (nguoi.txt, thisinh.txt, system.txt...)
 */
class QuanLyThiSinh {
private:
    Vector<ThiSinh> dsThiSinh;       // Danh sách thí sinh hợp nhất in-memory
    Vector<TinhThanh> dsTinhThanh;   // Danh mục tỉnh / thành phố (34 tỉnh thành)
    Vector<GioiTinh> dsGioiTinh;     // Danh mục giới tính (Nam, Nu, Khac)

    MySQLConnector db;               // Bộ kết nối MySQL Database Server
    bool suDungMySQL;                // Trạng thái ưu tiên MySQL

    string pathNguoi;                // data/nguoi.txt
    string pathThiSinh;              // data/thisinh.txt
    string pathTinhThanh;            // data/tinhthanh.txt
    string pathGioiTinh;             // data/gioitinh.txt
    string pathSystem;               // data/system.txt

    double diemChuanHienTai;         // Điểm chuẩn xét tuyển đại học hiện hành
    double diemLietHienTai;          // Ngưỡng điểm liệt (mặc định 1.00)
    string namTuyenSinh;             // Năm tuyển sinh (vd: 2026)
    string tenTruong;                // Tên trường đại học

    // Hàm hỗ trợ nạp dữ liệu mẫu ban đầu nếu tệp chưa tồn tại
    void taoDuLieuMauNeuTrong();

public:
    QuanLyThiSinh(const string& dirData = "data");
    ~QuanLyThiSinh();

    // 0. Quản lý kết nối MySQL
    bool ketNoiMySQL();
    void ngatKetNoiMySQL();
    bool isMySQLConnected() const;
    MySQLConnector& getDB();
    const MySQLConnector& getDB() const;
    bool dongBoTuMySQL();
    bool dongBoLenMySQL();

    // 1. Quản lý đọc / ghi CSDL quan hệ đa tệp (Key - Value) & MySQL
    bool docDuLieu();
    bool luuDuLieu() const;
    bool docCauHinhHeThong(const string& path = "data/system.txt");
    bool luuCauHinhHeThong(const string& path = "data/system.txt") const;

    // Quản lý đọc / ghi danh mục
    bool docDanhMucTinhThanh(const string& path = "data/tinhthanh.txt");
    bool luuDanhMucTinhThanh(const string& path = "data/tinhthanh.txt") const;
    bool docDanhMucGioiTinh(const string& path = "data/gioitinh.txt");
    bool luuDanhMucGioiTinh(const string& path = "data/gioitinh.txt") const;

    // 2. Tra cứu & Sinh mã tự động
    string sinhSoBaoDanhMoi() const; // Tự động tăng SBD (DUT001 -> DUT015...)
    int timViTriTheoSBD(const string& sbd) const;
    int timViTriTheoCCCD(const string& cccd) const;
    Vector<ThiSinh> timTheoTen(const string& ten) const;
    Vector<ThiSinh> timTheoQueQuan(const string& que) const;

    // Chuyển đổi và tra cứu danh mục Tỉnh thành / Giới tính
    string layTenTinhThanh(const string& maHoacTen) const;
    string layMaTinhThanh(const string& tenHoacMa) const;
    string layTenGioiTinh(const string& maHoacTen) const;
    string layMaGioiTinh(const string& tenHoacMa) const;

    // Kiểm tra tính toàn vẹn tham chiếu (Referential Integrity Check)
    bool kiemTraHopLeQueQuan(const string& maHoacTen) const;
    bool kiemTraHopLeGioiTinh(const string& maHoacTen) const;

    // 3. Nghiệp vụ CRUD Thí sinh (Đồng bộ đồng thời Memory, MySQL & File)
    bool themThiSinh(const ThiSinh& ts);
    bool capNhatThiSinh(const ThiSinh& ts);
    bool xoaThiSinh(const string& sbdHoacCCCD);

    // 4. Nghiệp vụ CRUD Danh mục Hệ thống
    bool themTinhThanh(const TinhThanh& tt);
    bool capNhatTinhThanh(const TinhThanh& tt);
    bool xoaTinhThanh(const string& ma);

    bool themGioiTinh(const GioiTinh& gt);
    bool capNhatGioiTinh(const GioiTinh& gt);
    bool xoaGioiTinh(const string& ma);

    // 5. Giải thuật Sắp xếp (Dùng QuickSort nội bộ trong Vector)
    void sapXepTheoTongDiemGiamDan();
    void sapXepTheoTenAZ();
    void sapXepTheoSBDTangDan();

    // 6. Nghiệp vụ Xét tuyển & Thống kê
    double getDiemChuan() const;
    void setDiemChuan(double dc);
    double getDiemLiet() const;
    void setDiemLiet(double dl);
    string getNamTuyenSinh() const;
    void setNamTuyenSinh(const string& nam);
    string getTenTruong() const;
    void setTenTruong(const string& truong);

    Vector<ThiSinh> getDanhSachTrungTuyen(double diemChuan) const;
    Vector<ThiSinh> getDanhSachThuKhoa() const;
    void tinhThongKePhoDiem(int& mucThap, int& mucTB, int& mucKha, int& mucCao, int& mucRatCao) const;

    // 7. Xuất các báo cáo dạng bảng kẻ khung chuyên biệt
    bool xuatBaoCaoXetTuyen(const string& path = "data/ketqua_xettuyen.txt", double diemChuan = 22.0, const string& keyword = "") const;
    bool xuatBaoCaoThuKhoa(const string& path = "data/thukhoa.txt") const;
    bool xuatBaoCaoPhoDiem(const string& path = "data/thongke_phodiem.txt") const;
    bool xuatTatCaBaoCao() const;

    // 8. Getters danh sách
    Vector<ThiSinh>& getDanhSach();
    const Vector<ThiSinh>& getDanhSach() const;
    Vector<TinhThanh>& getDanhSachTinhThanh();
    const Vector<TinhThanh>& getDanhSachTinhThanh() const;
    Vector<GioiTinh>& getDanhSachGioiTinh();
    const Vector<GioiTinh>& getDanhSachGioiTinh() const;
};

#endif // QUAN_LY_THI_SINH_H
