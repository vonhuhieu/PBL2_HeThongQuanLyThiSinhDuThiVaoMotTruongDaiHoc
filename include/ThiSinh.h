#ifndef THISINH_H
#define THISINH_H

#include "Nguoi.h"
#include "Constants.h"
#include "DUTModels.h"
#include "Vector.h"
#include <string>

/**
 * @brief Lớp đối tượng THÍ SINH (Kế thừa từ lớp NGƯỜI)
 * Đáp ứng chính xác đề bài Đề tài 24 - PBL2 và Chuẩn tuyển sinh DUT
 * Kế thừa: soCCCD, hoTen, ngaySinh, queQuan, gioiTinh từ lớp Nguoi
 * Thuộc tính dự thi: soBaoDanh, diemToan, diemLy, diemHoa, namTuyenSinh
 * Thuộc tính kết quả DUT: maSinhVien, lopSinhHoat, nganhTrungTuyen, nguyenVongTrungTuyen
 * Danh sách nguyện vọng: dsNguyenVong (Vector<NguyenVong>)
 * Quy chuẩn: GEMINI.md - 100% lowerCamelCase
 */
class ThiSinh : public Nguoi {
private:
    string soBaoDanh;               // Số báo danh dự thi (SBD: DUT001, DUT002...) - Khóa chính
    int namTuyenSinh;               // Năm tuyển sinh / Khóa học (vd: 2026)
    double diemToan;                // Điểm thi môn Toán (0.00 - 10.00)
    double diemLy;                  // Điểm thi môn Vật lý (0.00 - 10.00)
    double diemHoa;                 // Điểm thi môn Hóa học (0.00 - 10.00)
    string maSinhVien;              // Mã sinh viên chuẩn DUT 9 số (DEFAULT "", sinh khi trúng tuyển: 102260001)
    string lopSinhHoat;             // Lớp sinh hoạt DUT (DEFAULT "", cấp khi trúng tuyển: 26TCLC_KHDL1)
    string nganhTrungTuyen;         // Mã ngành trúng tuyển (DEFAULT "", vd: 7480201B)
    int nguyenVongTrungTuyen;       // Thứ tự nguyện vọng trúng tuyển (0: chưa/rớt, 1: NV1, 2: NV2, 3: NV3)
    string ngayTao;                 // Thời điểm tạo hồ sơ (YYYY-MM-DD HH:MM:SS)
    string ngaySua;                 // Thời điểm sửa hồ sơ gần nhất (YYYY-MM-DD HH:MM:SS)

    Vector<NguyenVong> dsNguyenVong; // Danh sách các nguyện vọng đăng ký xét tuyển

public:
    // Các hàm tạo (Constructors)
    ThiSinh();
    ThiSinh(const string& cccd, const string& ten, const string& ns, const string& que, const string& gt,
            const string& sbd, double toan, double ly, double hoa, int nam = AdmissionConst::NAM_TUYEN_SINH_MAC_DINH);
    ThiSinh(const ThiSinh& other);

    // Hàm hủy ảo (Destructor)
    virtual ~ThiSinh();

    // Getters
    string getSoBaoDanh() const;
    int getNamTuyenSinh() const;
    double getDiemToan() const;
    double getDiemLy() const;
    double getDiemHoa() const;
    string getMaSinhVien() const;
    string getLopSinhHoat() const;
    string getNganhTrungTuyen() const;
    int getNguyenVongTrungTuyen() const;
    string getNgayTao() const;
    string getNgaySua() const;
    const Vector<NguyenVong>& getDsNguyenVong() const;
    Vector<NguyenVong>& getDsNguyenVong();

    // Setters
    void setSoBaoDanh(const string& sbd);
    void setNamTuyenSinh(int nam);
    void setDiemToan(double toan);
    void setDiemLy(double ly);
    void setDiemHoa(double hoa);
    void setMaSinhVien(const string& msv);
    void setLopSinhHoat(const string& lsh);
    void setNganhTrungTuyen(const string& nt);
    void setNguyenVongTrungTuyen(int nv);
    void setNgayTao(const string& nt);
    void setNgaySua(const string& ns);
    void setDsNguyenVong(const Vector<NguyenVong>& ds);

    // Phương thức quản lý Nguyện vọng
    void themNguyenVong(const NguyenVong& nv);
    void xoaTatCaNguyenVong();

    // Các phương thức tính toán nghiệp vụ tuyển sinh Đại học
    double tinhTongDiem() const;                                                    // Tổng điểm = Toán + Lý + Hóa
    bool kiemTraDiemLiet(double threshold = AdmissionConst::DIEM_LIET) const;       // Kiểm tra môn thi <= ngưỡng điểm liệt
    bool daTrungTuyen() const;                                                      // Đã được cấp Mã sinh viên hoặc ngành trúng tuyển
    bool xetTrungTuyen(double diemChuan, double diemLiet = AdmissionConst::DIEM_LIET) const; // Đạt điểm chuẩn và không bị điểm liệt

    // Ghi đè phương thức nhập xuất đa hình (Polymorphism)
    virtual void nhap(istream& in = cin) override;
    virtual void xuat(ostream& out = cout) const override;

    // Nạp chồng toán tử gán và so sánh
    ThiSinh& operator=(const ThiSinh& other);
    bool operator==(const ThiSinh& other) const;     // So sánh bằng theo SBD hoặc CCCD
    bool operator<(const ThiSinh& other) const;      // So sánh theo Tổng điểm để sắp xếp
    bool operator>(const ThiSinh& other) const;

    // Nạp chồng toán tử nhập / xuất luồng
    friend istream& operator>>(istream& in, ThiSinh& ts);
    friend ostream& operator<<(ostream& out, const ThiSinh& ts);
};

#endif // THISINH_H
