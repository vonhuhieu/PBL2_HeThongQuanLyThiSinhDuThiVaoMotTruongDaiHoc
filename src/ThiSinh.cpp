#include "../include/ThiSinh.h"
#include <iomanip>

// 1. Constructor mặc định
ThiSinh::ThiSinh() 
    : Nguoi(), maThiSinh(""), soBaoDanh(""), maKhuVuc("KV3"), maNganh("7480201"),
      diemToan(0.0), diemLy(0.0), diemHoa(0.0), diemUuTien(0.0) {}

// 2. Constructor có tham số đầy đủ
ThiSinh::ThiSinh(const string& maTS, const string& cmnd, const string& ten, const string& ns,
                 const string& que, const string& gt, const string& sbd,
                 const string& maKV, const string& maNg,
                 double toan, double ly, double hoa, double uuTien)
    : Nguoi(cmnd, ten, ns, que, gt), maThiSinh(maTS), soBaoDanh(sbd),
      maKhuVuc(maKV), maNganh(maNg),
      diemToan(toan), diemLy(ly), diemHoa(hoa), diemUuTien(uuTien) {}

// 3. Copy constructor
ThiSinh::ThiSinh(const ThiSinh& other)
    : Nguoi(other), maThiSinh(other.maThiSinh), soBaoDanh(other.soBaoDanh),
      maKhuVuc(other.maKhuVuc), maNganh(other.maNganh),
      diemToan(other.diemToan), diemLy(other.diemLy), diemHoa(other.diemHoa), diemUuTien(other.diemUuTien) {}

// 4. Destructor
ThiSinh::~ThiSinh() {}

// Getters
string ThiSinh::getMaThiSinh() const { return maThiSinh; }
string ThiSinh::getSoBaoDanh() const { return soBaoDanh; }
string ThiSinh::getMaKhuVuc() const { return maKhuVuc; }
string ThiSinh::getMaNganh() const { return maNganh; }
double ThiSinh::getDiemToan() const { return diemToan; }
double ThiSinh::getDiemLy() const { return diemLy; }
double ThiSinh::getDiemHoa() const { return diemHoa; }
double ThiSinh::getDiemUuTien() const { return diemUuTien; }

// Setters
void ThiSinh::setMaThiSinh(const string& maTS) { maThiSinh = maTS; }
void ThiSinh::setSoBaoDanh(const string& sbd) { soBaoDanh = sbd; }
void ThiSinh::setMaKhuVuc(const string& maKV) { maKhuVuc = maKV; }
void ThiSinh::setMaNganh(const string& maNg) { maNganh = maNg; }
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

// Xét tuyển trúng tuyển vào ngành đăng ký
bool ThiSinh::xetTrungTuyen(double diemChuanNganh) const {
    if (kiemTraDiemLiet()) return false;
    return tinhTongDiem() >= diemChuanNganh;
}

// Nhập thông tin thí sinh
void ThiSinh::nhap(istream& in) {
    if (&in == &cin) {
        cout << "Nhap Ma Thi Sinh (PK): ";
        getline(in, maThiSinh);
        Nguoi::nhap(in);
        cout << "Nhap so bao danh (SBD): ";
        getline(in, soBaoDanh);
        cout << "Nhap Ma Khu Vuc (KV1/KV2NT/KV2/KV3): ";
        getline(in, maKhuVuc);
        cout << "Nhap Ma Nganh dang ky: ";
        getline(in, maNganh);

        cout << "Nhap diem Toan (0 - 10): ";
        in >> diemToan;
        cout << "Nhap diem Ly   (0 - 10): ";
        in >> diemLy;
        cout << "Nhap diem Hoa  (0 - 10): ";
        in >> diemHoa;
        cout << "Nhap diem uu tien (0 - 0.75): ";
        in >> diemUuTien;
        in.ignore();
    } else {
        // Đọc từ luồng tệp
        getline(in, maThiSinh);
        Nguoi::nhap(in);
        getline(in, soBaoDanh);
        getline(in, maKhuVuc);
        getline(in, maNganh);
        in >> diemToan >> diemLy >> diemHoa >> diemUuTien;
        in.ignore();
    }
}

// Xuất thông tin thí sinh
void ThiSinh::xuat(ostream& out) const {
    if (&out == &cout) {
        out << left << setw(8) << maThiSinh
            << setw(9) << soBaoDanh;
        Nguoi::xuat(out);
        out << " " << left << setw(6) << maKhuVuc
            << setw(9) << maNganh
            << right << fixed << setprecision(2)
            << setw(6) << diemToan 
            << setw(6) << diemLy 
            << setw(6) << diemHoa 
            << setw(6) << diemUuTien
            << setw(7) << tinhTongDiem() 
            << "  " << left << setw(11) << xepLoaiHocLuc();
    } else {
        out << maThiSinh << endl;
        Nguoi::xuat(out);
        out << soBaoDanh << endl
            << maKhuVuc << endl
            << maNganh << endl
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
        maThiSinh = other.maThiSinh;
        soBaoDanh = other.soBaoDanh;
        maKhuVuc = other.maKhuVuc;
        maNganh = other.maNganh;
        diemToan = other.diemToan;
        diemLy = other.diemLy;
        diemHoa = other.diemHoa;
        diemUuTien = other.diemUuTien;
    }
    return *this;
}

// So sánh bằng theo Mã Thí Sinh hoặc Số báo danh
bool ThiSinh::operator==(const ThiSinh& other) const {
    return (maThiSinh == other.maThiSinh || soBaoDanh == other.soBaoDanh);
}

// So sánh theo Tổng điểm để sắp xếp
bool ThiSinh::operator<(const ThiSinh& other) const {
    return tinhTongDiem() < other.tinhTongDiem();
}

bool ThiSinh::operator>(const ThiSinh& other) const {
    return tinhTongDiem() > other.tinhTongDiem();
}
