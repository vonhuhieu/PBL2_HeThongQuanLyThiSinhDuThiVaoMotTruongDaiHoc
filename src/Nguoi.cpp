#include "../include/Nguoi.h"
#include <iomanip>

// 1. Constructor mặc định
Nguoi::Nguoi() 
    : soCCCD(""), hoTen(""), ngaySinh(""), queQuan(""), gioiTinh("Nam") {}

// 2. Constructor có tham số
Nguoi::Nguoi(const string& cccd, const string& ten, const string& ns, const string& que, const string& gt)
    : soCCCD(cccd), hoTen(ten), ngaySinh(ns), queQuan(que), gioiTinh(gt) {}

// 3. Copy constructor
Nguoi::Nguoi(const Nguoi& other)
    : soCCCD(other.soCCCD), hoTen(other.hoTen), ngaySinh(other.ngaySinh), 
      queQuan(other.queQuan), gioiTinh(other.gioiTinh) {}

// 4. Destructor ảo
Nguoi::~Nguoi() {}

// Getters
string Nguoi::getSoCCCD() const { return soCCCD; }
string Nguoi::getHoTen() const { return hoTen; }
string Nguoi::getNgaySinh() const { return ngaySinh; }
string Nguoi::getQueQuan() const { return queQuan; }
string Nguoi::getGioiTinh() const { return gioiTinh; }

// Setters
void Nguoi::setSoCCCD(const string& cccd) { soCCCD = cccd; }
void Nguoi::setHoTen(const string& ten) { hoTen = ten; }
void Nguoi::setNgaySinh(const string& ns) { ngaySinh = ns; }
void Nguoi::setQueQuan(const string& que) { queQuan = que; }
void Nguoi::setGioiTinh(const string& gt) { gioiTinh = gt; }

// Nhập thông tin người
void Nguoi::nhap(istream& in) {
    if (&in == &cin) {
        cout << "  Nhap so CCCD (12 so)         : ";
        getline(in, soCCCD);
        cout << "  Nhap ho va ten               : ";
        getline(in, hoTen);
        cout << "  Nhap ngay sinh (dd/mm/yyyy)  : ";
        getline(in, ngaySinh);
        cout << "  Nhap que quan                : ";
        getline(in, queQuan);
        cout << "  Nhap gioi tinh (Nam / Nu)    : ";
        getline(in, gioiTinh);
    } else {
        getline(in, soCCCD);
        getline(in, hoTen);
        getline(in, ngaySinh);
        getline(in, queQuan);
        getline(in, gioiTinh);
    }
}

// Xuất thông tin người
void Nguoi::xuat(ostream& out) const {
    if (&out == &cout) {
        out << left << setw(14) << soCCCD 
            << setw(22) << hoTen 
            << setw(13) << ngaySinh 
            << setw(16) << queQuan 
            << setw(8)  << gioiTinh;
    } else {
        out << "So CCCD: " << soCCCD << "\n"
            << "Ho va Ten: " << hoTen << "\n"
            << "Ngay Sinh: " << ngaySinh << "\n"
            << "Que Quan: " << queQuan << "\n"
            << "Gioi Tinh: " << gioiTinh << "\n";
    }
}

// Nạp chồng toán tử gán
Nguoi& Nguoi::operator=(const Nguoi& other) {
    if (this != &other) {
        soCCCD = other.soCCCD;
        hoTen = other.hoTen;
        ngaySinh = other.ngaySinh;
        queQuan = other.queQuan;
        gioiTinh = other.gioiTinh;
    }
    return *this;
}

// Nạp chồng toán tử so sánh bằng
bool Nguoi::operator==(const Nguoi& other) const {
    return (soCCCD == other.soCCCD);
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
