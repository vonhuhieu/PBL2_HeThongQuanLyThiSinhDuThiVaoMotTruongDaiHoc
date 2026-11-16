#ifndef QUAN_LY_THI_SINH_H
#define QUAN_LY_THI_SINH_H

#include "Vector.h"
#include "ThiSinh.h"
#include "DanhMuc.h"
#include "DUTModels.h"
#include "Constants.h"
#include "MySQLConnector.h"
#include <string>

using namespace std;

// Cấu trúc tham số bộ lọc & phân trang Server-side
struct FilterThiSinhParams {
    int pageIndex;
    int pageSize;
    string keyword;
    int filterStatus;      // 0: Tat ca, 1: Trung tuyen, 2: Khong dat
    int filterNam;         // 0: Tat ca cac nam, hoac 2026, 2025...
    string filterKhoa;    // "" (Tat ca), hoac "CNTT", "CKHI"...
    string filterNganh;   // "" (Tat ca), hoac "7480201B"...
    string filterLop;     // "" (Tat ca), hoac "26TCLC_KHDL1"...
    int sortCol;           // Cot can sap xep (1: SBD, 2: CCCD, 3: Ten, 7: Toan, 8: Ly, 9: Hoa, 10: Tong diem, 11: Ket qua, 12: Nganh, 13: Lop, 14: MSV)
    bool sortAsc;          // true: Tang dan, false: Giam dan

    FilterThiSinhParams() 
        : pageIndex(1), pageSize(10), keyword(""), filterStatus(0),
          filterNam(0), filterKhoa(""), filterNganh(""), filterLop(""),
          sortCol(1), sortAsc(true) {}
};

/**
 * @brief Lớp Quản trị hệ thống Cơ sở dữ liệu và Nghiệp vụ tuyển sinh Đại học Bách khoa (DUT)
 * PHIÊN BẢN ĐỘC LẬP: 100% CSDL QUAN HỆ MYSQL (LARAGON)
 * - Quản trị và lưu trữ đồng bộ 10 bảng chuẩn 3NF:
 *   tinh_thanh, gioi_tinh, khoa_hoc, khoa, danh_muc_nganh, nganh_theo_khoa_hoc,
 *   nguoi, thisinh, nguyen_vong, cau_hinh
 * - Tuân thủ 100% chuẩn lowerCamelCase giữa C++ và MySQL theo GEMINI.md
 * - Không magic numbers / matrix code - Tất cả qua AdmissionConst
 */
class QuanLyThiSinh {
private:
    Vector<ThiSinh>          dsThiSinh;           // Danh sách thí sinh
    Vector<TinhThanh>        dsTinhThanh;         // Danh mục tỉnh / thành phố
    Vector<GioiTinh>         dsGioiTinh;          // Danh mục giới tính
    Vector<Khoa>             dsKhoa;              // Danh mục khoa đào tạo DUT
    Vector<DanhMucNganh>     dsDanhMucNganh;      // Danh mục ngành tĩnh DUT
    Vector<KhoaHoc>          dsKhoaHoc;           // Danh sách các khóa / năm tuyển sinh
    Vector<NganhTheoKhoaHoc> dsNganhTheoKhoaHoc;  // Chỉ tiêu & điểm chuẩn biến động theo khóa

    MySQLConnector db;                            // Bộ kết nối MySQL Database Server (Laragon)

    int    namTuyenSinhHienHanh;                  // Năm tuyển sinh làm việc (mặc định 2026)
    int    siSoLopChuan;                          // Sĩ số chuẩn ĐỘNG của 1 lớp DUT (mặc định 30)
    double diemChuanHienTai;                      // Điểm chuẩn chung khởi tạo (22.00)
    double diemLietHienTai;                       // Ngưỡng điểm liệt (1.00)
    double diemSanMacDinh;                        // Điểm sàn tối thiểu nhận hồ sơ (15.00)
    string tenTruong;                             // Tên trường đại học

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
    string sinhSoBaoDanhMoi() const;              // Tự động tăng SBD (DUT001 -> DUT015...)
    int timViTriTheoSBD(const string& sbd) const;
    int timViTriTheoCCCD(const string& cccd) const;
    Vector<ThiSinh> timTheoTen(const string& ten) const;
    Vector<ThiSinh> timTheoQueQuan(const string& que) const;

    // Chuyển đổi và tra cứu danh mục Tỉnh thành / Giới tính
    string layTenTinhThanh(const string& maHoacTen) const;
    string layMaTinhThanh(const string& tenHoacMa) const;
    string layTenGioiTinh(const string& maHoacTen) const;
    string layMaGioiTinh(const string& tenHoacMa) const;

    // Tra cứu Khoa & Ngành DUT
    string layTenKhoa(const string& maKhoa) const;
    string layMaKhoaTheoChuanDUT(const string& maKhoa) const;
    string layTenNganh(const string& maNganh) const;
    string layTienToLop(const string& maNganh) const;
    string layMaKhoaCuaNganh(const string& maNganh) const;
    int    layChiTieuNganh(int nam, const string& maNganh) const;
    double layDiemChuanNganh(int nam, const string& maNganh) const;

    // Kiểm tra tính toàn vẹn tham chiếu (Referential Integrity Check)
    bool kiemTraHopLeQueQuan(const string& maHoacTen) const;
    bool kiemTraHopLeGioiTinh(const string& maHoacTen) const;

    // 3. Nghiệp vụ CRUD Thí sinh (Thao tác trực tiếp tới CSDL MySQL)
    bool themThiSinh(const ThiSinh& ts);
    bool capNhatThiSinh(const ThiSinh& ts);
    bool xoaThiSinh(const string& sbdHoacCCCD);

    // Nghiệp vụ Nguyện vọng của Thí sinh
    bool capNhatNguyenVongThiSinh(const string& sbd, const Vector<NguyenVong>& dsNV);
    Vector<NguyenVong> layNguyenVongCuaThiSinh(const string& sbd) const;

    // 4. Nghiệp vụ CRUD Danh mục Hệ thống (Thao tác trực tiếp tới CSDL MySQL)
    bool themTinhThanh(const TinhThanh& tt);
    bool capNhatTinhThanh(const TinhThanh& tt);
    bool xoaTinhThanh(const string& ma);

    bool themGioiTinh(const GioiTinh& gt);
    bool capNhatGioiTinh(const GioiTinh& gt);
    bool xoaGioiTinh(const string& ma);

    bool capNhatChiTieuNganh(int nam, const string& maNganh, int chiTieuMoi);

    // 5. Giải thuật Sắp xếp (Dùng QuickSort nội bộ trong Vector)
    void sapXepTheoTongDiemGiamDan();
    void sapXepTheoTenAZ();
    void sapXepTheoSBDTangDan();

    // 6. Nghiệp vụ Tuyển sinh Đa Nguyện Vọng, Điểm chuẩn động & Xếp lớp DUT
    bool chayThuatToanXetTuyenVaChotDiemChuan(int namTuyenSinh);
    bool tuDongXepLopSinhHoatDUT(int namTuyenSinh);
    string sinhMaSinhVienDUT(const string& maKhoaDUT, int nam, int stt) const;

    // 7. Getters & Setters Cấu hình
    int    getNamTuyenSinhHienHanh() const;
    void   setNamTuyenSinhHienHanh(int nam);
    string getNamTuyenSinh() const { return to_string(namTuyenSinhHienHanh); }
    void   setNamTuyenSinh(const string& nam) { setNamTuyenSinhHienHanh(atoi(nam.c_str())); }
    int    getSiSoLopChuan() const;
    void   setSiSoLopChuan(int siSo);
    double getDiemChuan() const;
    void   setDiemChuan(double dc);
    double getDiemLiet() const;
    void   setDiemLiet(double dl);
    double getDiemSanMacDinh() const;
    double getDiemSan() const { return getDiemSanMacDinh(); }
    void   setDiemSanMacDinh(double ds);
    string getTenTruong() const;
    void   setTenTruong(const string& truong);

    Vector<ThiSinh> getDanhSachTrungTuyen(double diemChuan) const;
    Vector<ThiSinh> getDanhSachThuKhoa() const;
    void tinhThongKePhoDiem(int& mucThap, int& mucTB, int& mucKha, int& mucCao, int& mucRatCao) const;

    // 8. Xuất các báo cáo dạng bảng kẻ khung chuyên biệt ra thư mục data/
    bool xuatBaoCaoExcel(const string& filePath, const FilterThiSinhParams& params, const string& tenKhoaFilter = "", const string& tenNganhFilter = "") const;
    bool xuatBaoCaoXetTuyen(const string& path = "data/ketqua_xettuyen.txt", double diemChuan = 22.0, const string& keyword = "") const;
    bool xuatBaoCaoThuKhoa(const string& path = "data/thukhoa.txt") const;
    bool xuatBaoCaoPhoDiem(const string& path = "data/thongke_phodiem.txt") const;
    bool xuatBaoCaoTrungTuyenDUT(const string& path = "data/danhsach_trungtuyen_dut.txt", int nam = 2026) const;
    bool xuatTatCaBaoCao() const;

    // 9. Getters danh sách dữ liệu
    Vector<ThiSinh>& getDanhSach();
    const Vector<ThiSinh>& getDanhSach() const;
    Vector<TinhThanh>& getDanhSachTinhThanh();
    const Vector<TinhThanh>& getDanhSachTinhThanh() const;
    Vector<GioiTinh>& getDanhSachGioiTinh();
    const Vector<GioiTinh>& getDanhSachGioiTinh() const;
    Vector<Khoa>& getDanhSachKhoa();
    const Vector<Khoa>& getDanhSachKhoa() const;
    Vector<DanhMucNganh>& getDanhSachDanhMucNganh();
    const Vector<DanhMucNganh>& getDanhSachDanhMucNganh() const;
    Vector<KhoaHoc>& getDanhSachKhoaHoc();
    const Vector<KhoaHoc>& getDanhSachKhoaHoc() const;
    Vector<NganhTheoKhoaHoc>& getDanhSachNganhTheoKhoaHoc();
    const Vector<NganhTheoKhoaHoc>& getDanhSachNganhTheoKhoaHoc() const;

    // 10. Phân trang Server-side (SQL LIMIT/OFFSET trực tiếp trên MySQL Server, fallback In-Memory)
    Vector<ThiSinh> layDanhSachThiSinhPhanTrang(const FilterThiSinhParams& params, int& totalRecords);

    Vector<ThiSinh> layDanhSachThiSinhPhanTrang(
        int pageIndex,
        int pageSize,
        const string& keyword,
        int filterStatus,
        int sortType,
        int& totalRecords
    );

    // Lấy danh sách các lớp sinh hoạt DUT hiện có (hỗ trợ lọc phân cấp theo Khoa và Chuyên ngành)
    Vector<string> layDanhSachLopSinhHoat(const string& maKhoa = "", const string& maNganh = "") const;
};

#endif // QUAN_LY_THI_SINH_H
