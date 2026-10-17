#include "../include/Nguoi.h"
#include <iomanip>

// 1. Constructor mặc định
Nguoi::Nguoi() 
    : soCMND(""), hoTen(""), ngaySinh(""), queQuan(""), gioiTinh("Nam") {}

// 2. Constructor có tham số
Nguoi::Nguoi(const string& cmnd, const string& ten, const string& ns, const string& que, const string& gt)
    : soCMND(cmnd), hoTen(ten), ngaySinh(ns), queQuan(que), gioiTinh(gt) {}

// 3. Copy constructor
Nguoi::Nguoi(const Nguoi& other)
    : soCMND(other.soCMND), hoTen(other.hoTen), ngaySinh(other.ngaySinh), 
      queQuan(other.queQuan), gioiTinh(other.gioiTinh) {}

// 4. Destructor ảo
Nguoi::~Nguoi() {}

// Getters
string Nguoi::getSoCMND() const { return soCMND; }
string Nguoi::getHoTen() const { return hoTen; }
string Nguoi::getNgaySinh() const { return ngaySinh; }
string Nguoi::getQueQuan() const { return queQuan; }
string Nguoi::getGioiTinh() const { return gioiTinh; }

// Setters
void Nguoi::setSoCMND(const string& cmnd) { soCMND = cmnd; }
void Nguoi::setHoTen(const string& ten) { hoTen = ten; }
void Nguoi::setNgaySinh(const string& ns) { ngaySinh = ns; }
void Nguoi::setQueQuan(const string& que) { queQuan = que; }
void Nguoi::setGioiTinh(const string& gt) { gioiTinh = gt; }

// Phương thức nhập thông tin cơ bản
void Nguoi::nhap(istream& in) {
    if (&in == &cin) {
        cout << "Nhap so CMND/CCCD: ";
        getline(in, soCMND);
        cout << "Nhap ho va ten: ";
        getline(in, hoTen);
        cout << "Nhap ngay sinh (dd/mm/yyyy): ";
        getline(in, ngaySinh);
        cout << "Nhap que quan: ";
        getline(in, queQuan);
        cout << "Nhap gioi tinh (Nam/Nu): ";
        getline(in, gioiTinh);
    } else {
        // Đọc từ luồng tệp hoặc chuỗi
        getline(in, soCMND);
        getline(in, hoTen);
        getline(in, ngaySinh);
        getline(in, queQuan);
        getline(in, gioiTinh);
    }
}

// Phương thức xuất thông tin cơ bản
void Nguoi::xuat(ostream& out) const {
    if (&out == &cout) {
        out << left << setw(14) << soCMND 
            << setw(24) << hoTen 
            << setw(13) << ngaySinh 
            << setw(16) << queQuan 
            << setw(10) << gioiTinh;
    } else {
        out << soCMND << endl
            << hoTen << endl
            << ngaySinh << endl
            << queQuan << endl
            << gioiTinh << endl;
    }
}

// Nạp chồng toán tử >>
istream& operator>>(istream& in, Nguoi& p) {
    p.nhap(in);
    return in;
}

// Nạp chồng toán tử <<
ostream& operator<<(ostream& out, const Nguoi& p) {
    p.xuat(out);
    return out;
}

// Nạp chồng toán tử gán
Nguoi& Nguoi::operator=(const Nguoi& other) {
    if (this != &other) {
        soCMND = other.soCMND;
        hoTen = other.hoTen;
        ngaySinh = other.ngaySinh;
        queQuan = other.queQuan;
        gioiTinh = other.gioiTinh;
    }
    return *this;
}
