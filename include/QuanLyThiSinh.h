#ifndef QUAN_LY_THI_SINH_H
#define QUAN_LY_THI_SINH_H

#include "Vector.h"
#include "ThiSinh.h"
#include <string>

using namespace std;

/**
 * @brief Lớp quản lý toàn bộ các thao tác nghiệp vụ tuyển sinh
 */
class QuanLyThiSinh {
private:
    Vector<ThiSinh> dsThiSinh;  // Danh sách thí sinh sử dụng Vector template tự viết
    string dataFilePath;        // Đường dẫn file lưu trữ dữ liệu
    double diemChuanXetTuyen;   // Điểm chuẩn xét tuyển trúng tuyển (mặc định 21.0)
    string tenTruong;           // Tên trường đại học

    // Các hàm nội bộ hỗ trợ
    void inHeaderBang() const;
    void inFooterBang(int tongSo) const;

public:
    QuanLyThiSinh(const string& filePath = "data/thisinh.txt");
    ~QuanLyThiSinh();

    // 1. Quản lý File
    bool docDuLieuTuFile(const string& path = "");
    bool luuDuLieuVaoFile(const string& path = "") const;
    bool xuatBaoCaoXetTuyen(const string& path = "data/ketqua_xettuyen.txt") const;

    // 2. Thao tác Thêm / Sửa / Xóa (CRUD)
    void themMoiThiSinh();
    bool themThiSinh(const ThiSinh& ts);
    bool suaThongTinThiSinh(const string& sbd);
    bool xoaThiSinh(const string& sbd);
    void xoaTatCa();

    // 3. Hiển thị dữ liệu
    void hienThiDanhSach(const Vector<ThiSinh>& list, const string& tieuDeBang = "") const;
    void hienThiTatCa() const;

    // 4. Tìm kiếm
    int timViTriTheoSBD(const string& sbd) const;
    void timKiemTheoSBD() const;
    void timKiemTheoTen() const;
    void timKiemTheoQueQuan() const;
    void timKiemTheoKhoangDiem() const;

    // 5. Sắp xếp
    void sapXepTheoTongDiemGiamDan();
    void sapXepTheoSBDTangDan();
    void sapXepTheoTenAZ();

    // 6. Thống kê & Xét tuyển
    void vinhDanhThuKhoa() const;
    void danhSachTrungTuyen() const;
    void thongKePhoDiem() const;
    void thongKeTheoQueQuan() const;
    void thietLapDiemChuan();

    // Getters & Setters
    int getSoLuong() const;
    double getDiemChuan() const;
    void setDiemChuan(double diem);
    const Vector<ThiSinh>& getDanhSach() const;
};

#endif // QUAN_LY_THI_SINH_H
