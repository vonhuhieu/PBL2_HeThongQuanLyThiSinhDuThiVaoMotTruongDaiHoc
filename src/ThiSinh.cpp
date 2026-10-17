#include "../include/ThiSinh.h"
#include <iomanip>

// 1. Constructor mặc định
ThiSinh::ThiSinh() 
    : Nguoi(), soBaoDanh(""), diemToan(0.0), diemLy(0.0), diemHoa(0.0), diemUuTien(0.0) {}

// 2. Constructor có tham số đầy đủ
ThiSinh::ThiSinh(const string& cmnd, const string& ten, const string& ns, const string& que, const string& gt,
                 const string& sbd, double toan, double ly, double hoa, double uuTien)
    : Nguoi(cmnd, ten, ns, que, gt), soBaoDanh(sbd), 
      diemToan(toan), diemLy(ly), diemHoa(hoa), diemUuTien(uuTien) {}

// 3. Copy constructor
ThiSinh::ThiSinh(const ThiSinh& other)
    : Nguoi(other), soBaoDanh(other.soBaoDanh),
      diemToan(other.diemToan), diemLy(other.diemLy), diemHoa(other.diemHoa), diemUuTien(other.diemUuTien) {}

// 4. Destructor
ThiSinh::~ThiSinh() {}

// Getters
string ThiSinh::getSoBaoDanh() const { return soBaoDanh; }
double ThiSinh::getDiemToan() const { return diemToan; }
double ThiSinh::getDiemLy() const { return diemLy; }
double ThiSinh::getDiemHoa() const { return diemHoa; }
double ThiSinh::getDiemUuTien() const { return diemUuTien; }

// Setters
void ThiSinh::setSoBaoDanh(const string& sbd) { soBaoDanh = sbd; }
void ThiSinh::setDiemToan(double toan) { diemToan = toan; }
void ThiSinh::setDiemLy(double ly) { diemLy = ly; }
void ThiSinh::setDiemHoa(double hoa) { diemHoa = hoa; }
void ThiSinh::setDiemUuTien(double uuTien) { diemUuTien = uuTien; }

// Tính tổng điểm 3 môn cộng điểm ưu tiên
double ThiSinh::tinhTongDiem() const {
    return diemToan + diemLy + diemHoa + diemUuTien;
}

// Tính tổng điểm 3 môn thực tế
double ThiSinh::tinhDiemBaMon() const {
    return diemToan + diemLy + diemHoa;
}

// Kiểm tra điểm liệt (điểm môn thi <= 1.0)
bool ThiSinh::kiemTraDiemLiet() const {
    return (diemToan <= 1.0 || diemLy <= 1.0 || diemHoa <= 1.0);
}

// Xếp loại học lực theo điểm xét tuyển
string ThiSinh::xepLoaiHocLuc() const {
    double tong = tinhTongDiem();
    if (tong >= 27.0) return "Xuat sac";
    if (tong >= 24.0) return "Gioi";
    if (tong >= 21.0) return "Kha";
    if (tong >= 15.0) return "Trung binh";
    return "Yeu";
}

// Xét tuyển trúng tuyển vào ngành/trường
bool ThiSinh::xetTrungTuyen(double diemChuan) const {
    if (kiemTraDiemLiet()) return false;
    return tinhTongDiem() >= diemChuan;
}

// Nhập thông tin thí sinh
void ThiSinh::nhap(istream& in) {
    if (&in == &cin) {
        cout << "--- NHAP THONG TIN CA NHAN ---" << endl;
        Nguoi::nhap(in);
        cout << "--- NHAP THONG TIN DU THI ---" << endl;
        cout << "Nhap so bao danh (SBD): ";
        getline(in, soBaoDanh);

        cout << "Nhap diem Toan (0 - 10): ";
        in >> diemToan;
        cout << "Nhap diem Ly   (0 - 10): ";
        in >> diemLy;
        cout << "Nhap diem Hoa  (0 - 10): ";
        in >> diemHoa;
        cout << "Nhap diem uu tien (0 - 2.5): ";
        in >> diemUuTien;
        in.ignore(); // Xoa ky tu xuong dong con sót lại trong bo dem
    } else {
        // Đọc từ file
        Nguoi::nhap(in);
        getline(in, soBaoDanh);
        in >> diemToan >> diemLy >> diemHoa >> diemUuTien;
        in.ignore(); // Bỏ qua ký tự newline
    }
}

// Xuất thông tin thí sinh ra bảng hoặc ra luồng tệp
void ThiSinh::xuat(ostream& out) const {
    if (&out == &cout) {
        out << left << setw(10) << soBaoDanh;
        Nguoi::xuat(out);
        out << right << fixed << setprecision(2)
            << setw(7) << diemToan 
            << setw(7) << diemLy 
            << setw(7) << diemHoa 
            << setw(7) << diemUuTien
            << setw(8) << tinhTongDiem() 
            << "  " << left << setw(12) << xepLoaiHocLuc();
    } else {
        // Ghi ra file định dạng dòng văn bản
        Nguoi::xuat(out);
        out << soBaoDanh << endl
            << fixed << setprecision(2)
            << diemToan << " " << diemLy << " " << diemHoa << " " << diemUuTien << endl;
    }
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

// Toán tử gán
ThiSinh& ThiSinh::operator=(const ThiSinh& other) {
    if (this != &other) {
        Nguoi::operator=(other);
        soBaoDanh = other.soBaoDanh;
        diemToan = other.diemToan;
        diemLy = other.diemLy;
        diemHoa = other.diemHoa;
        diemUuTien = other.diemUuTien;
    }
    return *this;
}

// So sánh bằng theo Số báo danh
bool ThiSinh::operator==(const ThiSinh& other) const {
    return (soBaoDanh == other.soBaoDanh);
}

// So sánh theo Tổng điểm để sắp xếp
bool ThiSinh::operator<(const ThiSinh& other) const {
    return tinhTongDiem() < other.tinhTongDiem();
}

bool ThiSinh::operator>(const ThiSinh& other) const {
    return tinhTongDiem() > other.tinhTongDiem();
}
