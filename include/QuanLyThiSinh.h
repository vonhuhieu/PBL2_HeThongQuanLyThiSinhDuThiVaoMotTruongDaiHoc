#ifndef QUAN_LY_THI_SINH_H
#define QUAN_LY_THI_SINH_H

#include "Vector.h"
#include "ThiSinh.h"
#include "KhuVuc.h"
#include "NganhXetTuyen.h"
#include <string>

using namespace std;

/**
 * @brief Lớp quản trị toàn bộ hệ thống cơ sở dữ liệu và nghiệp vụ tuyển sinh
 * Áp dụng mô hình Cơ sở dữ liệu quan hệ (Relational Database) chuẩn 3NF:
 * - Bảng 1: KhuVuc (danhmuc_khuvuc.txt)
 * - Bảng 2: NganhXetTuyen (danhmuc_nganh.txt)
 * - Bảng 3: HoSoThiSinh (hoso_thisinh.txt)
 * - Bảng 4: DangKyThi (dangky_thi.txt)
 */
class QuanLyThiSinh {
private:
    Vector<KhuVuc> dsKhuVuc;           // Bảng danh mục Khu vực
    Vector<NganhXetTuyen> dsNganh;     // Bảng danh mục Ngành đào tạo
    Vector<ThiSinh> dsThiSinh;         // Bảng dữ liệu Thí sinh hợp nhất

    string pathKhuVuc;
    string pathNganh;
    string pathHoSo;
    string pathDangKy;
    string pathLegacy;                 // File thisinh.txt để duy trì tương thích cũ

public:
    QuanLyThiSinh(const string& dirData = "data");
    ~QuanLyThiSinh();

    // 1. Quản lý đọc / ghi CSDL đa bảng
    bool docTatCaDuLieu();
    bool luuTatCaDuLieu() const;
    bool xuatBaoCaoXetTuyen(const string& path = "data/ketqua_xettuyen.txt") const;

    // 2. Tra cứu khóa ngoại (Foreign Key Lookup)
    double getDiemUuTienTheoKhuVuc(const string& maKV) const;
    string getTenKhuVuc(const string& maKV) const;
    string getTenNganh(const string& maNganh) const;
    double getDiemChuanNganh(const string& maNganh) const;
    int timViTriKhuVuc(const string& maKV) const;
    int timViTriNganh(const string& maNganh) const;

    // 3. Nghiệp vụ Thí sinh (Kiểm tra ràng buộc toàn vẹn CSDL)
    int timViTriTheoSBD(const string& sbd) const;
    int timViTriTheoMaTS(const string& maTS) const;
    int timViTriTheoCMND(const string& cmnd) const;
    string sinhMaThiSinhMoi() const;

    bool themThiSinh(const ThiSinh& ts);
    bool capNhatThiSinh(const ThiSinh& ts);
    bool xoaThiSinh(const string& maTSHoacSBD);

    // 4. Quản lý Ngành xét tuyển (Cập nhật điểm chuẩn / chỉ tiêu)
    bool capNhatDiemChuanNganh(const string& maNganh, double diemChuanMoi);
    bool capNhatChiTieuNganh(const string& maNganh, int chiTieuMoi);

    // 5. Giải thuật Sắp xếp (Dùng QuickSort trong Vector)
    void sapXepTheoTongDiemGiamDan();
    void sapXepTheoSBDTangDan();
    void sapXepTheoTenAZ();
    void sapXepTheoNganh();

    // 6. Nghiệp vụ Thống kê & Xét tuyển Đại học
    Vector<ThiSinh> getDanhSachTrungTuyen(const string& maNganh = "") const;
    Vector<ThiSinh> getDanhSachThuKhoa() const;
    ThiSinh getThuKhoaCuaNganh(const string& maNganh) const;
    void tinhThongKePhoDiem(int& kem, int& tb, int& kha, int& gioi, int& xuatSac) const;
    int demSoLuongDangKyTheoNganh(const string& maNganh) const;
    int demSoLuongTrungTuyenTheoNganh(const string& maNganh) const;

    // 7. Getters danh sách
    Vector<ThiSinh>& getDanhSachThiSinh();
    const Vector<ThiSinh>& getDanhSachThiSinh() const;
    Vector<KhuVuc>& getDanhSachKhuVuc();
    const Vector<KhuVuc>& getDanhSachKhuVuc() const;
    Vector<NganhXetTuyen>& getDanhSachNganh();
    const Vector<NganhXetTuyen>& getDanhSachNganh() const;
};

#endif // QUAN_LY_THI_SINH_H
