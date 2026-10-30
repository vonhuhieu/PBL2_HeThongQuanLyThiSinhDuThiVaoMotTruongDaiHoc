#ifndef THISINH_H
#define THISINH_H

#include "Nguoi.h"
#include <string>

/**
 * @brief Lớp đối tượng THÍ SINH (Kế thừa từ lớp NGƯỜI)
 * Đáp ứng chính xác đề bài Đề tài 24 - PBL2
 * Kế thừa: Số CCCD, Họ tên, Ngày sinh, Quê quán, Giới tính từ lớp NGƯỜI
 * Bổ sung các thuộc tính dự thi: Số báo danh, Điểm toán, Điểm lý, Điểm hóa
 */
class ThiSinh : public Nguoi {
private:
    string soBaoDanh;   // Số báo danh dự thi (SBD: DUT001, DUT002...) - Khóa chính dự thi
    double diemToan;    // Điểm thi môn Toán (0.00 - 10.00)
    double diemLy;      // Điểm thi môn Vật lý (0.00 - 10.00)
    double diemHoa;     // Điểm thi môn Hóa học (0.00 - 10.00)

public:
    // Các hàm tạo (Constructors)
    ThiSinh();
    ThiSinh(const string& cccd, const string& ten, const string& ns, const string& que, const string& gt,
            const string& sbd, double toan, double ly, double hoa);
    ThiSinh(const ThiSinh& other);

    // Hàm hủy ảo (Destructor)
    virtual ~ThiSinh();

    // Getters
    string getSoBaoDanh() const;
    double getDiemToan() const;
    double getDiemLy() const;
    double getDiemHoa() const;

    // Setters
    void setSoBaoDanh(const string& sbd);
    void setDiemToan(double toan);
    void setDiemLy(double ly);
    void setDiemHoa(double hoa);

    // Các phương thức tính toán nghiệp vụ tuyển sinh Đại học
    double tinhTongDiem() const;                     // Tổng điểm = Toán + Lý + Hóa
    bool kiemTraDiemLiet() const;                    // Có môn nào <= 1.00 điểm không
    bool xetTrungTuyen(double diemChuan) const;       // Đạt điểm chuẩn và không bị điểm liệt

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
