#include "../include/ThiSinh.h"
#include <iomanip>

// 1. Constructor mặc định
ThiSinh::ThiSinh() 
    : Nguoi(), soBaoDanh(""), diemToan(0.0), diemLy(0.0), diemHoa(0.0) {}

// 2. Constructor có tham số đầy đủ
ThiSinh::ThiSinh(const string& cccd, const string& ten, const string& ns, const string& que, const string& gt,
                 const string& sbd, double toan, double ly, double hoa)
    : Nguoi(cccd, ten, ns, que, gt), soBaoDanh(sbd), diemToan(toan), diemLy(ly), diemHoa(hoa) {}

// 3. Copy constructor
ThiSinh::ThiSinh(const ThiSinh& other)
    : Nguoi(other), soBaoDanh(other.soBaoDanh), 
      diemToan(other.diemToan), diemLy(other.diemLy), diemHoa(other.diemHoa) {}

// 4. Destructor
ThiSinh::~ThiSinh() {}

// Getters
string ThiSinh::getSoBaoDanh() const { return soBaoDanh; }
double ThiSinh::getDiemToan() const { return diemToan; }
double ThiSinh::getDiemLy() const { return diemLy; }
double ThiSinh::getDiemHoa() const { return diemHoa; }

// Setters
void ThiSinh::setSoBaoDanh(const string& sbd) { soBaoDanh = sbd; }
void ThiSinh::setDiemToan(double toan) { diemToan = toan; }
void ThiSinh::setDiemLy(double ly) { diemLy = ly; }
void ThiSinh::setDiemHoa(double hoa) { diemHoa = hoa; }

// Tính tổng điểm 3 môn thi
double ThiSinh::tinhTongDiem() const {
    return diemToan + diemLy + diemHoa;
}

// Kiểm tra điểm liệt (điểm môn thi <= 1.00)
bool ThiSinh::kiemTraDiemLiet() const {
    return (diemToan <= 1.00 || diemLy <= 1.00 || diemHoa <= 1.00);
}

// Xét tuyển theo điểm chuẩn chung của kỳ thi
bool ThiSinh::xetTrungTuyen(double diemChuan) const {
    if (kiemTraDiemLiet()) return false;
    return (tinhTongDiem() >= diemChuan);
}

// Nhập thông tin thí sinh
void ThiSinh::nhap(istream& in) {
    if (&in == &cin) {
        cout << "  Nhap So Bao Danh (SBD)       : ";
        getline(in, soBaoDanh);
        Nguoi::nhap(in);
        cout << "  Nhap diem Toan (0.0 - 10.0)  : ";
        in >> diemToan;
        cout << "  Nhap diem Ly   (0.0 - 10.0)  : ";
        in >> diemLy;
        cout << "  Nhap diem Hoa  (0.0 - 10.0)  : ";
        in >> diemHoa;
        in.ignore();
    } else {
        getline(in, soBaoDanh);
        Nguoi::nhap(in);
        in >> diemToan >> diemLy >> diemHoa;
        in.ignore();
    }
}

// Xuất thông tin thí sinh
void ThiSinh::xuat(ostream& out) const {
    if (&out == &cout) {
        out << left << setw(9) << soBaoDanh;
        Nguoi::xuat(out);
        out << right << fixed << setprecision(2)
            << setw(6) << diemToan 
            << setw(6) << diemLy 
            << setw(6) << diemHoa 
            << setw(7) << tinhTongDiem();
    } else {
        out << "So Bao Danh: " << soBaoDanh << "\n";
        Nguoi::xuat(out);
        out << fixed << setprecision(2)
            << "Diem Toan: " << diemToan << "\n"
            << "Diem Ly: " << diemLy << "\n"
            << "Diem Hoa: " << diemHoa << "\n";
    }
}

// Nạp chồng toán tử gán
ThiSinh& ThiSinh::operator=(const ThiSinh& other) {
    if (this != &other) {
        Nguoi::operator=(other);
        soBaoDanh = other.soBaoDanh;
        diemToan = other.diemToan;
        diemLy = other.diemLy;
        diemHoa = other.diemHoa;
    }
    return *this;
}

// So sánh bằng theo Số báo danh hoặc Số CCCD
bool ThiSinh::operator==(const ThiSinh& other) const {
    return (soBaoDanh == other.soBaoDanh || soCCCD == other.soCCCD);
}

// So sánh theo Tổng điểm để sắp xếp
bool ThiSinh::operator<(const ThiSinh& other) const {
    return (tinhTongDiem() < other.tinhTongDiem());
}

bool ThiSinh::operator>(const ThiSinh& other) const {
    return (tinhTongDiem() > other.tinhTongDiem());
}

// Nạp chồng >>
istream& operator>>(istream& in, ThiSinh& ts) {
    ts.nhap(in);
    return in;
}

// Nạp chồng <<
ostream& operator<<(ostream& out, const ThiSinh& ts) {
    ts.xuat(out);
    return out;
}
