#ifndef THISINH_H
#define THISINH_H

#include "Nguoi.h"
#include <string>

/**
 * @brief Lớp đối tượng THÍ SINH (Kế thừa từ lớp NGƯỜI)
 * Đại diện cho một thí sinh tham gia kỳ thi tuyển sinh đại học
 */
class ThiSinh : public Nguoi {
private:
    string soBaoDanh;   // Số báo danh dự thi (SBD)
    double diemToan;    // Điểm thi môn Toán (0 - 10)
    double diemLy;      // Điểm thi môn Vật lý (0 - 10)
    double diemHoa;     // Điểm thi môn Hóa học (0 - 10)
    double diemUuTien;  // Điểm cộng ưu tiên theo khu vực / đối tượng (0 - 2.5)

public:
    // Các hàm tạo (Constructors)
    ThiSinh();
    ThiSinh(const string& cmnd, const string& ten, const string& ns, const string& que, const string& gt,
            const string& sbd, double toan, double ly, double hoa, double uuTien = 0.0);
    ThiSinh(const ThiSinh& other);

    // Hàm hủy (Destructor)
    virtual ~ThiSinh();

    // Getters
    string getSoBaoDanh() const;
    double getDiemToan() const;
    double getDiemLy() const;
    double getDiemHoa() const;
    double getDiemUuTien() const;

    // Setters
    void setSoBaoDanh(const string& sbd);
    void setDiemToan(double toan);
    void setDiemLy(double ly);
    void setDiemHoa(double hoa);
    void setDiemUuTien(double uuTien);

    // Các phương thức tính toán nghiệp vụ
    double tinhTongDiem() const;       // Toán + Lý + Hóa + Ưu tiên
    double tinhDiemBaMon() const;      // Toán + Lý + Hóa
    bool kiemTraDiemLiet() const;      // Kiểm tra có môn nào <= 1.0 điểm không
    string xepLoaiHocLuc() const;      // Xuất sắc, Giỏi, Khá, Trung bình, Yếu
    bool xetTrungTuyen(double diemChuan) const; // Đạt điểm chuẩn và không liệt

    // Ghi đè phương thức nhập xuất (Polymorphism)
    virtual void nhap(istream& in = cin) override;
    virtual void xuat(ostream& out = cout) const override;

    // Nạp chồng toán tử nhập xuất
    friend istream& operator>>(istream& in, ThiSinh& ts);
    friend ostream& operator<<(ostream& out, const ThiSinh& ts);

    // Nạp chồng toán tử gán và so sánh
    ThiSinh& operator=(const ThiSinh& other);
    bool operator==(const ThiSinh& other) const;
    bool operator<(const ThiSinh& other) const;
    bool operator>(const ThiSinh& other) const;
};

#endif // THISINH_H
