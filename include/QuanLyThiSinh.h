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
 * PHIÊN BẢN ĐỘC LẬP: 100% CSDL QUAN HỆ MYSQL (LARAGON)
 * - Toàn bộ dữ liệu được quản trị, lưu trữ và truy vấn trực tiếp từ MySQL Database: `pbl2_tuyensinh`
 * - Gồm 5 bảng chuẩn 3NF: tinh_thanh, gioi_tinh, nguoi, thisinh, cau_hinh
 * - Không sử dụng bất kỳ tệp văn bản .txt nào làm cơ sở dữ liệu
 * - Vector<T> tự xây dựng đóng vai trò bộ đệm in-memory và cấu trúc giải thuật cho GUI & QuickSort
 */
class QuanLyThiSinh {
private:
    Vector<ThiSinh> dsThiSinh;       // Danh sách thí sinh in-memory (lấy từ MySQL)
    Vector<TinhThanh> dsTinhThanh;   // Danh mục tỉnh / thành phố (34 tỉnh thành từ MySQL)
    Vector<GioiTinh> dsGioiTinh;     // Danh mục giới tính (Nam, Nu, Khac từ MySQL)

    MySQLConnector db;               // Bộ kết nối MySQL Database Server (Laragon)

    double diemChuanHienTai;         // Điểm chuẩn xét tuyển đại học hiện hành
    double diemLietHienTai;          // Ngưỡng điểm liệt (mặc định 1.00)
    string namTuyenSinh;             // Năm tuyển sinh (vd: 2026)
    string tenTruong;                // Tên trường đại học

public:
    QuanLyThiSinh();
    ~QuanLyThiSinh();

    // 1. Quản trị kết nối và Đọc/Ghi CSDL MySQL
    bool ketNoiMySQL();
    void ngatKetNoiMySQL();
    bool isMySQLConnected() const;
    MySQLConnector& getDB();
    const MySQLConnector& getDB() const;

    bool docDuLieuTuMySQL();
    bool luuDuLieuVaoMySQL();
    bool dongBoTuMySQL() { return docDuLieuTuMySQL(); }
    bool dongBoLenMySQL() { return luuDuLieuVaoMySQL(); }
    bool khoiTaoDuLieuMauMySQL();

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

    // 3. Nghiệp vụ CRUD Thí sinh (Thao tác trực tiếp tới CSDL MySQL)
    bool themThiSinh(const ThiSinh& ts);
    bool capNhatThiSinh(const ThiSinh& ts);
    bool xoaThiSinh(const string& sbdHoacCCCD);

    // 4. Nghiệp vụ CRUD Danh mục Hệ thống (Thao tác trực tiếp tới CSDL MySQL)
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

    // 7. Xuất các báo cáo dạng bảng kẻ khung chuyên biệt ra thư mục data/
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
