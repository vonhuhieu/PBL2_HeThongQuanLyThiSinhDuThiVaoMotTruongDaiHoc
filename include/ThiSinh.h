#ifndef THISINH_H
#define THISINH_H

#include "Nguoi.h"
#include <string>

/**
 * @brief Lớp đối tượng THÍ SINH (Kế thừa từ lớp NGƯỜI)
 * Đại diện cho một thí sinh tham gia kỳ thi tuyển sinh đại học
 * Áp dụng kiến thức Cơ sở dữ liệu:
 * - Khóa chính (PK): maThiSinh (giải quyết triệt để trường hợp nhiều thí sinh trùng họ tên)
 * - Khóa ngoại (FK): maKhuVuc (liên kết bảng KhuVuc), maNganh (liên kết bảng NganhXetTuyen)
 */
class ThiSinh : public Nguoi {
private:
    string maThiSinh;   // Khóa chính (PK): TS001, TS002...
    string soBaoDanh;   // Số báo danh dự thi (SBD): DUT001, DUT002...
    string maKhuVuc;    // Khóa ngoại (FK): KV1, KV2NT, KV2, KV3
    string maNganh;     // Khóa ngoại (FK): Mã ngành đăng ký xét tuyển (ví dụ: 7480201)
    double diemToan;    // Điểm thi môn Toán (0 - 10)
    double diemLy;      // Điểm thi môn Vật lý (0 - 10)
    double diemHoa;     // Điểm thi môn Hóa học (0 - 10)
    double diemUuTien;  // Điểm cộng ưu tiên suy ra từ Khu vực (0.0 - 0.75)

public:
    // Các hàm tạo (Constructors)
    ThiSinh();
    ThiSinh(const string& maTS, const string& cmnd, const string& ten, const string& ns,
            const string& que, const string& gt, const string& sbd,
            const string& maKV, const string& maNg,
            double toan, double ly, double hoa, double uuTien = 0.0);
    ThiSinh(const ThiSinh& other);

    // Hàm hủy ảo (Destructor)
    virtual ~ThiSinh();

    // Getters
    string getMaThiSinh() const;
    string getSoBaoDanh() const;
    string getMaKhuVuc() const;
    string getMaNganh() const;
    double getDiemToan() const;
    double getDiemLy() const;
    double getDiemHoa() const;
    double getDiemUuTien() const;

    // Setters
    void setMaThiSinh(const string& maTS);
    void setSoBaoDanh(const string& sbd);
    void setMaKhuVuc(const string& maKV);
    void setMaNganh(const string& maNg);
    void setDiemToan(double toan);
    void setDiemLy(double ly);
    void setDiemHoa(double hoa);
    void setDiemUuTien(double uuTien);

    // Các phương thức tính toán nghiệp vụ tuyển sinh
    double tinhTongDiem() const;       // Toán + Lý + Hóa + Ưu tiên
    double tinhDiemBaMon() const;      // Toán + Lý + Hóa
    bool kiemTraDiemLiet() const;      // Kiểm tra có môn nào <= 1.0 điểm không
    string xepLoaiHocLuc() const;      // Xuất sắc, Giỏi, Khá, Trung bình, Yếu
    bool xetTrungTuyen(double diemChuanNganh) const; // Đạt điểm chuẩn ngành và không bị điểm liệt

    // Ghi đè phương thức nhập xuất (Polymorphism)
    virtual void nhap(istream& in = cin) override;
    virtual void xuat(ostream& out = cout) const override;

    // Nạp chồng toán tử nhập xuất luồng
    friend istream& operator>>(istream& in, ThiSinh& ts);
    friend ostream& operator<<(ostream& out, const ThiSinh& ts);

    // Nạp chồng toán tử gán và so sánh
    ThiSinh& operator=(const ThiSinh& other);
    bool operator==(const ThiSinh& other) const;
    bool operator<(const ThiSinh& other) const;
    bool operator>(const ThiSinh& other) const;
};

#endif // THISINH_H
