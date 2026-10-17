#ifndef NGUOI_H
#define NGUOI_H

#include <iostream>
#include <string>

using namespace std;

/**
 * @brief Lớp đối tượng NGƯỜI (Lớp cơ sở)
 * Chứa thông tin nhân khẩu học cơ bản của mỗi cá nhân
 */
class Nguoi {
protected:
    string soCMND;      // Số chứng minh thư / Thẻ căn cước công dân
    string hoTen;       // Họ và tên
    string ngaySinh;    // Ngày tháng năm sinh (dd/mm/yyyy)
    string queQuan;     // Quê quán (Tỉnh / Thành phố)
    string gioiTinh;    // Giới tính (Nam / Nữ / Khác)

public:
    // Các hàm tạo (Constructors)
    Nguoi();
    Nguoi(const string& cmnd, const string& ten, const string& ns, const string& que, const string& gt);
    Nguoi(const Nguoi& other);

    // Hàm hủy ảo (Destructor) đảm bảo giải phóng đúng khi đa hình
    virtual ~Nguoi();

    // Getters
    string getSoCMND() const;
    string getHoTen() const;
    string getNgaySinh() const;
    string getQueQuan() const;
    string getGioiTinh() const;

    // Setters
    void setSoCMND(const string& cmnd);
    void setHoTen(const string& ten);
    void setNgaySinh(const string& ns);
    void setQueQuan(const string& que);
    void setGioiTinh(const string& gt);

    // Các phương thức nhập xuất
    virtual void nhap(istream& in = cin);
    virtual void xuat(ostream& out = cout) const;

    // Nạp chồng toán tử nhập xuất luồng
    friend istream& operator>>(istream& in, Nguoi& p);
    friend ostream& operator<<(ostream& out, const Nguoi& p);

    // Toán tử gán
    Nguoi& operator=(const Nguoi& other);
};

#endif // NGUOI_H
