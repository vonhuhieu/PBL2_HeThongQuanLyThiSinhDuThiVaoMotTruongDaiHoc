#ifndef NGUOI_H
#define NGUOI_H

#include <iostream>
#include <string>

using namespace std;

/**
 * @brief Lớp đối tượng NGƯỜI (Lớp cơ sở - Base Class)
 * Đáp ứng chính xác đề bài Đề tài 24 - PBL2
 * Chứa các thuộc tính nhân thân: Số CCCD, Họ tên, Ngày sinh, Quê quán, Giới tính
 */
class Nguoi {
protected:
    string soCCCD;      // Số Căn cước công dân (12 chữ số - Khóa chính định danh công dân)
    string hoTen;       // Họ và tên
    string ngaySinh;    // Ngày tháng năm sinh (dd/mm/yyyy)
    string queQuan;     // Quê quán (Tỉnh / Thành phố)
    string gioiTinh;    // Giới tính (Nam / Nu)

public:
    // Các hàm tạo (Constructors)
    Nguoi();
    Nguoi(const string& cccd, const string& ten, const string& ns, const string& que, const string& gt);
    Nguoi(const Nguoi& other);

    // Hàm hủy ảo (Virtual Destructor) - Đảm bảo giải phóng vùng nhớ an toàn khi dùng đa hình
    virtual ~Nguoi();

    // Getters
    string getSoCCCD() const;
    string getHoTen() const;
    string getNgaySinh() const;
    string getQueQuan() const;
    string getGioiTinh() const;

    // Setters
    void setSoCCCD(const string& cccd);
    void setHoTen(const string& ten);
    void setNgaySinh(const string& ns);
    void setQueQuan(const string& que);
    void setGioiTinh(const string& gt);

    // Phương thức nhập xuất ảo (Virtual Methods)
    virtual void nhap(istream& in = cin);
    virtual void xuat(ostream& out = cout) const;

    // Nạp chồng toán tử gán và so sánh
    Nguoi& operator=(const Nguoi& other);
    bool operator==(const Nguoi& other) const;

    // Nạp chồng toán tử nhập / xuất luồng
    friend istream& operator>>(istream& in, Nguoi& p);
    friend ostream& operator<<(ostream& out, const Nguoi& p);
};

#endif // NGUOI_H
