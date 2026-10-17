#include "../include/QuanLyThiSinh.h"
#include "../include/ConsoleUI.h"
#include "../include/Utils.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <conio.h>

using namespace std;

// Comparators cho hàm sort của Vector
static bool cmpTongDiemGiam(const ThiSinh& a, const ThiSinh& b) {
    if (a.tinhTongDiem() != b.tinhTongDiem()) {
        return a.tinhTongDiem() > b.tinhTongDiem();
    }
    return a.getSoBaoDanh() < b.getSoBaoDanh();
}

static bool cmpSBDTang(const ThiSinh& a, const ThiSinh& b) {
    return a.getSoBaoDanh() < b.getSoBaoDanh();
}

static bool cmpTenAZ(const ThiSinh& a, const ThiSinh& b) {
    // Tách phần tên để so sánh
    string nameA = Utils::trim(a.getHoTen());
    string nameB = Utils::trim(b.getHoTen());
    
    size_t posA = nameA.find_last_of(' ');
    size_t posB = nameB.find_last_of(' ');
    
    string firstNameA = (posA != string::npos) ? nameA.substr(posA + 1) : nameA;
    string firstNameB = (posB != string::npos) ? nameB.substr(posB + 1) : nameB;
    
    if (firstNameA != firstNameB) {
        return Utils::toLower(firstNameA) < Utils::toLower(firstNameB);
    }
    return Utils::toLower(nameA) < Utils::toLower(nameB);
}

// Constructor
QuanLyThiSinh::QuanLyThiSinh(const string& filePath)
    : dataFilePath(filePath), diemChuanXetTuyen(22.0), 
      tenTruong("TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG") {
    docDuLieuTuFile(dataFilePath);
}

// Destructor
QuanLyThiSinh::~QuanLyThiSinh() {
    luuDuLieuVaoFile(dataFilePath);
}

// Đọc dữ liệu từ file
bool QuanLyThiSinh::docDuLieuTuFile(const string& path) {
    string file = path.empty() ? dataFilePath : path;
    ifstream fin(file);
    if (!fin.is_open()) {
        return false;
    }

    dsThiSinh.clear();
    int count = 0;
    fin >> count;
    fin.ignore(); // Bỏ qua ký tự xuống dòng

    for (int i = 0; i < count; ++i) {
        ThiSinh ts;
        fin >> ts;
        if (!ts.getSoBaoDanh().empty()) {
            dsThiSinh.push_back(ts);
        }
    }
    fin.close();
    return true;
}

// Ghi dữ liệu vào file
bool QuanLyThiSinh::luuDuLieuVaoFile(const string& path) const {
    string file = path.empty() ? dataFilePath : path;
    ofstream fout(file);
    if (!fout.is_open()) {
        return false;
    }

    fout << dsThiSinh.size() << endl;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        fout << dsThiSinh[i];
    }
    fout.close();
    return true;
}

// Xuất báo cáo xét tuyển ra file riêng
bool QuanLyThiSinh::xuatBaoCaoXetTuyen(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "===================================================================================================\n";
    fout << "                      " << tenTruong << "\n";
    fout << "                           BAO CAO KET QUA XET TUYEN DAI HOC\n";
    fout << "                              Diem chuan xet tuyen: " << fixed << setprecision(2) << diemChuanXetTuyen << " diem\n";
    fout << "===================================================================================================\n\n";

    fout << left << setw(5) << "STT"
         << setw(10) << "SBD"
         << setw(14) << "CMND/CCCD"
         << setw(24) << "Ho va Ten"
         << setw(12) << "Ngay Sinh"
         << setw(16) << "Que Quan"
         << right
         << setw(6) << "Toan"
         << setw(6) << "Ly"
         << setw(6) << "Hoa"
         << setw(6) << "UT"
         << setw(8) << "Tong"
         << "   " << left << setw(14) << "Ket Qua" << "\n";
    fout << string(110, '-') << "\n";

    int soTrungTuyen = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        bool trungTuyen = ts.xetTrungTuyen(diemChuanXetTuyen);
        if (trungTuyen) soTrungTuyen++;

        fout << left << setw(5) << (i + 1)
             << setw(10) << ts.getSoBaoDanh()
             << setw(14) << ts.getSoCMND()
             << setw(24) << ts.getHoTen()
             << setw(12) << ts.getNgaySinh()
             << setw(16) << ts.getQueQuan()
             << right << fixed << setprecision(2)
             << setw(6) << ts.getDiemToan()
             << setw(6) << ts.getDiemLy()
             << setw(6) << ts.getDiemHoa()
             << setw(6) << ts.getDiemUuTien()
             << setw(8) << ts.tinhTongDiem()
             << "   " << left << setw(14) << (trungTuyen ? "TRUNG TUYEN" : "Khong dat") << "\n";
    }

    fout << string(110, '=') << "\n";
    fout << "Tong so thi sinh dang ky: " << dsThiSinh.size() << "\n";
    fout << "So thi sinh trung tuyen : " << soTrungTuyen << " (" 
         << fixed << setprecision(1) << (dsThiSinh.empty() ? 0.0 : (double)soTrungTuyen * 100.0 / dsThiSinh.size()) << "%)\n";
    fout.close();
    return true;
}

// In phần đầu của bảng danh sách
void QuanLyThiSinh::inHeaderBang() const {
    ConsoleUI::setColor(COLOR_CYAN);
    cout << "  +" << string(5, '-') << "+" << string(10, '-') << "+" << string(13, '-') << "+" 
         << string(22, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+"
         << string(8, '-') << "+" << string(6, '-') << "+" << string(6, '-') << "+" 
         << string(6, '-') << "+" << string(5, '-') << "+" << string(7, '-') << "+" 
         << string(13, '-') << "+\n";

    cout << "  | " << left << setw(3) << "STT"
         << " | " << setw(8) << "SBD"
         << " | " << setw(11) << "CMND/CCCD"
         << " | " << setw(20) << "Ho va Ten"
         << " | " << setw(10) << "Ngay Sinh"
         << " | " << setw(13) << "Que Quan"
         << " | " << setw(6) << "Phai"
         << " | " << right << setw(4) << "Toan"
         << " | " << setw(4) << "Ly"
         << " | " << setw(4) << "Hoa"
         << " | " << setw(3) << "UT"
         << " | " << setw(5) << "Tong"
         << " | " << left << setw(11) << "Xep Loai" << " |\n";

    cout << "  +" << string(5, '=') << "+" << string(10, '=') << "+" << string(13, '=') << "+" 
         << string(22, '=') << "+" << string(12, '=') << "+" << string(15, '=') << "+"
         << string(8, '=') << "+" << string(6, '=') << "+" << string(6, '=') << "+" 
         << string(6, '=') << "+" << string(5, '=') << "+" << string(7, '=') << "+" 
         << string(13, '=') << "+\n";
    ConsoleUI::resetColor();
}

// In phần chân của bảng danh sách
void QuanLyThiSinh::inFooterBang(int tongSo) const {
    ConsoleUI::setColor(COLOR_CYAN);
    cout << "  +" << string(5, '-') << "+" << string(10, '-') << "+" << string(13, '-') << "+" 
         << string(22, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+"
         << string(8, '-') << "+" << string(6, '-') << "+" << string(6, '-') << "+" 
         << string(6, '-') << "+" << string(5, '-') << "+" << string(7, '-') << "+" 
         << string(13, '-') << "+\n";
    ConsoleUI::setColor(COLOR_YELLOW);
    cout << "  >> Tong so ban ghi: " << tongSo << " thi sinh.\n";
    ConsoleUI::resetColor();
}

// Hiển thị danh sách thí sinh có phân trang nếu danh sách dài
void QuanLyThiSinh::hienThiDanhSach(const Vector<ThiSinh>& list, const string& tieuDeBang) const {
    if (list.empty()) {
        ConsoleUI::printWarning("Danh sach hien tai dang trong!");
        return;
    }

    const int PAGE_SIZE = 12;
    int totalPages = (list.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    int currentPage = 0;

    while (true) {
        ConsoleUI::clearScreen();
        if (!tieuDeBang.empty()) {
            ConsoleUI::printTitle(tieuDeBang);
        } else {
            ConsoleUI::printTitle("DANH SACH THI SINH DU THI");
        }

        ConsoleUI::setColor(COLOR_WHITE);
        cout << "  Trang: [ " << (currentPage + 1) << " / " << totalPages << " ]\n\n";

        inHeaderBang();

        int startIdx = currentPage * PAGE_SIZE;
        int endIdx = (startIdx + PAGE_SIZE < list.size()) ? startIdx + PAGE_SIZE : list.size();

        for (int i = startIdx; i < endIdx; ++i) {
            const ThiSinh& ts = list[i];
            if (i % 2 == 0) {
                ConsoleUI::setColor(COLOR_WHITE);
            } else {
                ConsoleUI::setColor(COLOR_CYAN);
            }

            cout << "  | " << left << setw(3) << (i + 1)
                 << " | " << setw(8) << ts.getSoBaoDanh()
                 << " | " << setw(11) << ts.getSoCMND()
                 << " | " << setw(20) << ts.getHoTen().substr(0, 20)
                 << " | " << setw(10) << ts.getNgaySinh()
                 << " | " << setw(13) << ts.getQueQuan().substr(0, 13)
                 << " | " << setw(6) << ts.getGioiTinh()
                 << " | " << right << fixed << setprecision(1) << setw(4) << ts.getDiemToan()
                 << " | " << setw(4) << ts.getDiemLy()
                 << " | " << setw(4) << ts.getDiemHoa()
                 << " | " << setw(3) << ts.getDiemUuTien()
                 << " | " << setw(5) << ts.tinhTongDiem()
                 << " | " << left << setw(11) << ts.xepLoaiHocLuc() << " |\n";
        }

        inFooterBang(list.size());

        if (totalPages <= 1) {
            ConsoleUI::pauseScreen("Nhan phim bat ky de quay lai menu...");
            break;
        }

        ConsoleUI::setColor(COLOR_GRAY);
        cout << "  (Nhan phim [A] Trang truoc | [D] Trang sau | [Q / ESC] Quay lai): ";
        ConsoleUI::resetColor();

        char ch = _getch();
        if (ch == 'a' || ch == 'A' || ch == 75) {
            if (currentPage > 0) currentPage--;
        } else if (ch == 'd' || ch == 'D' || ch == 77) {
            if (currentPage < totalPages - 1) currentPage++;
        } else if (ch == 'q' || ch == 'Q' || ch == 27) {
            break;
        }
    }
}

void QuanLyThiSinh::hienThiTatCa() const {
    hienThiDanhSach(dsThiSinh, "TOAN BO DANH SACH THI SINH DANG KY");
}

// Tìm vị trí theo Số báo danh
int QuanLyThiSinh::timViTriTheoSBD(const string& sbd) const {
    string target = Utils::toUpper(Utils::trim(sbd));
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getSoBaoDanh()) == target) {
            return i;
        }
    }
    return -1;
}

// Thêm thí sinh kiểm tra tính hợp lệ
bool QuanLyThiSinh::themThiSinh(const ThiSinh& ts) {
    if (timViTriTheoSBD(ts.getSoBaoDanh()) != -1) {
        return false; // Trùng SBD
    }
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getSoCMND() == ts.getSoCMND()) {
            return false; // Trùng CMND
        }
    }
    dsThiSinh.push_back(ts);
    luuDuLieuVaoFile();
    return true;
}

void QuanLyThiSinh::themMoiThiSinh() {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("THEM THI SINH MOI");

    string sbd, cmnd, hoten, ngaysinh, quequan, gioitinh;
    double toan, ly, hoa, uutien;

    cout << "  --- THONG TIN HO SO THI SINH ---\n\n";

    // 1. Nhập SBD
    while (true) {
        cout << "  Nhap So Bao Danh (SBD)          : ";
        getline(cin, sbd);
        sbd = Utils::toUpper(Utils::trim(sbd));
        if (!Utils::kiemTraSBD(sbd)) {
            ConsoleUI::printError("SBD khong hop le (tu 3-15 ky tu, khong co dau cach)!");
            continue;
        }
        if (timViTriTheoSBD(sbd) != -1) {
            ConsoleUI::printError("So Bao Danh da ton tai tren he thong!");
            continue;
        }
        break;
    }

    // 2. Nhập CMND/CCCD
    while (true) {
        cout << "  Nhap So CMND / CCCD (9 hoac 12 so): ";
        getline(cin, cmnd);
        cmnd = Utils::trim(cmnd);
        if (!Utils::kiemTraCMND(cmnd)) {
            ConsoleUI::printError("CMND/CCCD phai gom dung 9 hoac 12 chu so!");
            continue;
        }
        bool trungCMND = false;
        for (int i = 0; i < dsThiSinh.size(); ++i) {
            if (dsThiSinh[i].getSoCMND() == cmnd) {
                trungCMND = true;
                break;
            }
        }
        if (trungCMND) {
            ConsoleUI::printError("So CMND/CCCD nay da duoc dang ky boi thi sinh khac!");
            continue;
        }
        break;
    }

    // 3. Nhập Họ và tên
    while (true) {
        cout << "  Nhap Ho va Ten thi sinh         : ";
        getline(cin, hoten);
        hoten = Utils::chuanHoaTen(hoten);
        if (hoten.length() < 3) {
            ConsoleUI::printError("Ho va ten phai co it nhat 3 ky tu!");
            continue;
        }
        break;
    }

    // 4. Nhập Ngày sinh
    while (true) {
        cout << "  Nhap Ngay sinh (dd/mm/yyyy)     : ";
        getline(cin, ngaysinh);
        ngaysinh = Utils::trim(ngaysinh);
        if (!Utils::kiemTraNgaySinh(ngaysinh)) {
            ConsoleUI::printError("Ngay sinh khong hop le hoac sai dinh dang dd/mm/yyyy!");
            continue;
        }
        break;
    }

    // 5. Nhập Quê quán
    while (true) {
        cout << "  Nhap Que quan (Tinh/Thanh pho)  : ";
        getline(cin, quequan);
        quequan = Utils::chuanHoaTen(quequan);
        if (quequan.empty()) {
            ConsoleUI::printError("Que quan khong duoc de trong!");
            continue;
        }
        break;
    }

    // 6. Nhập Giới tính
    while (true) {
        cout << "  Nhap Gioi tinh (Nam / Nu)       : ";
        getline(cin, gioitinh);
        gioitinh = Utils::chuanHoaTen(gioitinh);
        if (gioitinh == "Nam" || gioitinh == "Nu") {
            break;
        }
        ConsoleUI::printWarning("Chi chap nhan 'Nam' hoac 'Nu'.");
    }

    cout << "\n  --- THONG TIN DIEM THI (0.00 -> 10.00) ---\n\n";

    // 7. Điểm Toán
    while (true) {
        cout << "  Nhap diem mon Toan              : ";
        if (cin >> toan && Utils::kiemTraDiem(toan)) {
            break;
        }
        cin.clear();
        cin.ignore(1000, '\n');
        ConsoleUI::printError("Diem Toan phai la so thuc trong khoang tu 0 den 10!");
    }

    // 8. Điểm Lý
    while (true) {
        cout << "  Nhap diem mon Vat ly            : ";
        if (cin >> ly && Utils::kiemTraDiem(ly)) {
            break;
        }
        cin.clear();
        cin.ignore(1000, '\n');
        ConsoleUI::printError("Diem Vat ly phai la so thuc trong khoang tu 0 den 10!");
    }

    // 9. Điểm Hóa
    while (true) {
        cout << "  Nhap diem mon Hoa hoc           : ";
        if (cin >> hoa && Utils::kiemTraDiem(hoa)) {
            break;
        }
        cin.clear();
        cin.ignore(1000, '\n');
        ConsoleUI::printError("Diem Hoa hoc phai la so thuc trong khoang tu 0 den 10!");
    }

    // 10. Điểm ưu tiên
    while (true) {
        cout << "  Nhap diem uu tien (0.0 -> 2.5)  : ";
        if (cin >> uutien && Utils::kiemTraDiemUuTien(uutien)) {
            break;
        }
        cin.clear();
        cin.ignore(1000, '\n');
        ConsoleUI::printError("Diem uu tien phai trong khoang tu 0.0 den 2.5!");
    }
    cin.ignore(); // Bỏ qua ký tự newline

    ThiSinh tsMoi(cmnd, hoten, ngaysinh, quequan, gioitinh, sbd, toan, ly, hoa, uutien);
    dsThiSinh.push_back(tsMoi);
    luuDuLieuVaoFile();

    ConsoleUI::printSuccess("Them thi sinh thanh cong vao he thong!");
    ConsoleUI::pauseScreen();
}

// Sửa thông tin thí sinh
bool QuanLyThiSinh::suaThongTinThiSinh(const string& sbd) {
    int idx = timViTriTheoSBD(sbd);
    if (idx == -1) {
        ConsoleUI::printError("Khong tim thay thi sinh co SBD nay!");
        return false;
    }

    ThiSinh& ts = dsThiSinh[idx];
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("CAP NHAT THONG TIN THI SINH");

    cout << "  Thong tin hien tai:\n";
    cout << "  - SBD     : " << ts.getSoBaoDanh() << "\n";
    cout << "  - Ho ten  : " << ts.getHoTen() << "\n";
    cout << "  - CMND    : " << ts.getSoCMND() << "\n";
    cout << "  - Que quan: " << ts.getQueQuan() << "\n";
    cout << "  - Diem    : Toan=" << ts.getDiemToan() << ", Ly=" << ts.getDiemLy() 
         << ", Hoa=" << ts.getDiemHoa() << ", UT=" << ts.getDiemUuTien() 
         << " => Tong: " << ts.tinhTongDiem() << "\n\n";

    cout << "  (Nhan ENTER neu muon giu nguyen thong tin cu)\n";

    string input;
    cout << "  Ho ten moi       : ";
    getline(cin, input);
    if (!input.empty()) ts.setHoTen(Utils::chuanHoaTen(input));

    cout << "  Ngay sinh moi    : ";
    getline(cin, input);
    if (!input.empty() && Utils::kiemTraNgaySinh(input)) ts.setNgaySinh(Utils::trim(input));

    cout << "  Que quan moi     : ";
    getline(cin, input);
    if (!input.empty()) ts.setQueQuan(Utils::chuanHoaTen(input));

    cout << "  Diem Toan moi    : ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            double val = stod(input);
            if (Utils::kiemTraDiem(val)) ts.setDiemToan(val);
        } catch (...) {}
    }

    cout << "  Diem Ly moi      : ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            double val = stod(input);
            if (Utils::kiemTraDiem(val)) ts.setDiemLy(val);
        } catch (...) {}
    }

    cout << "  Diem Hoa moi     : ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            double val = stod(input);
            if (Utils::kiemTraDiem(val)) ts.setDiemHoa(val);
        } catch (...) {}
    }

    cout << "  Diem uu tien moi : ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            double val = stod(input);
            if (Utils::kiemTraDiemUuTien(val)) ts.setDiemUuTien(val);
        } catch (...) {}
    }

    luuDuLieuVaoFile();
    ConsoleUI::printSuccess("Cap nhat thong tin thi sinh thanh cong!");
    ConsoleUI::pauseScreen();
    return true;
}

// Xóa thí sinh
bool QuanLyThiSinh::xoaThiSinh(const string& sbd) {
    int idx = timViTriTheoSBD(sbd);
    if (idx == -1) {
        ConsoleUI::printError("Khong tim thay thi sinh co SBD nay!");
        return false;
    }

    cout << "\n  Ban co chac chan muon xoa thi sinh [" << dsThiSinh[idx].getSoBaoDanh() 
         << " - " << dsThiSinh[idx].getHoTen() << "] khoi he thong? (Y/N): ";
    char confirm;
    cin >> confirm;
    cin.ignore();

    if (confirm == 'y' || confirm == 'Y') {
        dsThiSinh.erase(idx);
        luuDuLieuVaoFile();
        ConsoleUI::printSuccess("Da xoa thi sinh thanh cong!");
        return true;
    } else {
        ConsoleUI::printWarning("Huy thao tac xoa.");
        return false;
    }
}

void QuanLyThiSinh::xoaTatCa() {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("XOA TOAN BO DU LIEU");
    ConsoleUI::setColor(COLOR_RED);
    cout << "  CANH BAO: Thao tac nay se xoa toan bo du lieu thi sinh trong he thong!\n";
    ConsoleUI::resetColor();
    cout << "  Nhap chu 'DONG Y' de xac nhan xoa toan bo: ";
    string confirm;
    getline(cin, confirm);

    if (confirm == "DONG Y") {
        dsThiSinh.clear();
        luuDuLieuVaoFile();
        ConsoleUI::printSuccess("Da xoa toan bo danh sach thi sinh!");
    } else {
        ConsoleUI::printWarning("Huy bo thao tac.");
    }
    ConsoleUI::pauseScreen();
}

// Tìm kiếm theo SBD
void QuanLyThiSinh::timKiemTheoSBD() const {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TIM KIEM THI SINH THEO SO BAO DANH");

    cout << "  Nhap So Bao Danh can tim: ";
    string sbd;
    getline(cin, sbd);

    int idx = timViTriTheoSBD(sbd);
    if (idx == -1) {
        ConsoleUI::printError("Khong tim thay thi sinh co SBD: " + sbd);
    } else {
        Vector<ThiSinh> ketQua;
        ketQua.push_back(dsThiSinh[idx]);
        hienThiDanhSach(ketQua, "KET QUA TIM KIEM THEO SBD: " + sbd);
        return;
    }
    ConsoleUI::pauseScreen();
}

// Tìm kiếm theo tên
void QuanLyThiSinh::timKiemTheoTen() const {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TIM KIEM THI SINH THEO HO TEN");

    cout << "  Nhap ten thi sinh can tim (khong phan biet hoa/thuong): ";
    string ten;
    getline(cin, ten);
    ten = Utils::trim(ten);

    Vector<ThiSinh> ketQua;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::containsIgnoreCase(dsThiSinh[i].getHoTen(), ten)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }

    if (ketQua.empty()) {
        ConsoleUI::printError("Khong tim thay thi sinh nao phu hop voi tu khoa: " + ten);
        ConsoleUI::pauseScreen();
    } else {
        hienThiDanhSach(ketQua, "KET QUA TIM KIEM THEO TEN: " + ten);
    }
}

// Tìm kiếm theo quê quán
void QuanLyThiSinh::timKiemTheoQueQuan() const {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TIM KIEM THI SINH THEO QUE QUAN");

    cout << "  Nhap que quan can tim: ";
    string que;
    getline(cin, que);
    que = Utils::trim(que);

    Vector<ThiSinh> ketQua;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::containsIgnoreCase(dsThiSinh[i].getQueQuan(), que)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }

    if (ketQua.empty()) {
        ConsoleUI::printError("Khong tim thay thi sinh nao co que quan: " + que);
        ConsoleUI::pauseScreen();
    } else {
        hienThiDanhSach(ketQua, "DANH SACH THI SINH QUE QUAN TAI: " + que);
    }
}

// Tìm kiếm theo khoảng điểm
void QuanLyThiSinh::timKiemTheoKhoangDiem() const {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TIM KIEM THEO KHOANG TONG DIEM");

    double minDiem, maxDiem;
    cout << "  Nhap diem thap nhat: ";
    cin >> minDiem;
    cout << "  Nhap diem cao nhat : ";
    cin >> maxDiem;
    cin.ignore();

    Vector<ThiSinh> ketQua;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        double tong = dsThiSinh[i].tinhTongDiem();
        if (tong >= minDiem && tong <= maxDiem) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }

    if (ketQua.empty()) {
        ConsoleUI::printError("Khong co thi sinh nao trong khoang diem nay!");
        ConsoleUI::pauseScreen();
    } else {
        ketQua.sort(cmpTongDiemGiam);
        hienThiDanhSach(ketQua, "DANH SACH THI SINH TRONG KHOANG DIEM");
    }
}

// Sắp xếp theo tổng điểm giảm dần
void QuanLyThiSinh::sapXepTheoTongDiemGiamDan() {
    dsThiSinh.sort(cmpTongDiemGiam);
    hienThiDanhSach(dsThiSinh, "DANH SACH XEP THEO TONG DIEM GIAM DAN (BANG XEP HANG)");
}

// Sắp xếp theo SBD tăng dần
void QuanLyThiSinh::sapXepTheoSBDTangDan() {
    dsThiSinh.sort(cmpSBDTang);
    hienThiDanhSach(dsThiSinh, "DANH SACH XEP THEO SO BAO DANH TANG DAN");
}

// Sắp xếp theo tên A-Z
void QuanLyThiSinh::sapXepTheoTenAZ() {
    dsThiSinh.sort(cmpTenAZ);
    hienThiDanhSach(dsThiSinh, "DANH SACH XEP THEO THU TU TEN A - Z");
}

// Tìm và vinh danh thủ khoa
void QuanLyThiSinh::vinhDanhThuKhoa() const {
    if (dsThiSinh.empty()) {
        ConsoleUI::printWarning("Chua co du lieu thi sinh de tim thu khoa!");
        ConsoleUI::pauseScreen();
        return;
    }

    double maxDiem = dsThiSinh[0].tinhTongDiem();
    for (int i = 1; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() > maxDiem) {
            maxDiem = dsThiSinh[i].tinhTongDiem();
        }
    }

    Vector<ThiSinh> dsThuKhoa;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() == maxDiem) {
            dsThuKhoa.push_back(dsThiSinh[i]);
        }
    }

    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("VINH DANH THU KHOA KY THI TUYEN SINH");

    ConsoleUI::setColor(COLOR_YELLOW);
    cout << "\n  ★ ★ ★ CHUC MUNG THU KHOA XUAT SAC NHAT ★ ★ ★\n";
    cout << "  Diem so cao nhat dat duoc: " << fixed << setprecision(2) << maxDiem << " diem!\n\n";
    ConsoleUI::resetColor();

    inHeaderBang();
    for (int i = 0; i < dsThuKhoa.size(); ++i) {
        ConsoleUI::setColor(COLOR_YELLOW);
        const ThiSinh& ts = dsThuKhoa[i];
        cout << "  | " << left << setw(3) << (i + 1)
             << " | " << setw(8) << ts.getSoBaoDanh()
             << " | " << setw(11) << ts.getSoCMND()
             << " | " << setw(20) << ts.getHoTen().substr(0, 20)
             << " | " << setw(10) << ts.getNgaySinh()
             << " | " << setw(13) << ts.getQueQuan().substr(0, 13)
             << " | " << setw(6) << ts.getGioiTinh()
             << " | " << right << fixed << setprecision(1) << setw(4) << ts.getDiemToan()
             << " | " << setw(4) << ts.getDiemLy()
             << " | " << setw(4) << ts.getDiemHoa()
             << " | " << setw(3) << ts.getDiemUuTien()
             << " | " << setw(5) << ts.tinhTongDiem()
             << " | " << left << setw(11) << "THU KHOA" << " |\n";
    }
    inFooterBang(dsThuKhoa.size());
    ConsoleUI::pauseScreen();
}

// Lọc danh sách trúng tuyển
void QuanLyThiSinh::danhSachTrungTuyen() const {
    Vector<ThiSinh> dsDat;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].xetTrungTuyen(diemChuanXetTuyen)) {
            dsDat.push_back(dsThiSinh[i]);
        }
    }

    dsDat.sort(cmpTongDiemGiam);
    hienThiDanhSach(dsDat, "DANH SACH THI SINH TRUNG TUYEN (DIEM CHUAN: " + to_string((int)diemChuanXetTuyen) + ")");
}

// Thống kê phổ điểm
void QuanLyThiSinh::thongKePhoDiem() const {
    if (dsThiSinh.empty()) {
        ConsoleUI::printWarning("Chua co du lieu thi sinh de thong ke!");
        ConsoleUI::pauseScreen();
        return;
    }

    int kem = 0, tb = 0, kha = 0, gioi = 0, xuatSac = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        double tong = dsThiSinh[i].tinhTongDiem();
        if (tong >= 27.0) xuatSac++;
        else if (tong >= 24.0) gioi++;
        else if (tong >= 21.0) kha++;
        else if (tong >= 15.0) tb++;
        else kem++;
    }

    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("THONG KE PHO DIEM XET TUYEN");

    int total = dsThiSinh.size();
    ConsoleUI::setColor(COLOR_WHITE);
    cout << "\n  Tong so thi sinh tham gia: " << total << "\n\n";

    cout << "  +" << string(22, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+" << string(30, '-') << "+\n";
    cout << "  | " << left << setw(20) << "Khoang Diem"
         << " | " << right << setw(10) << "So Luong"
         << " | " << setw(13) << "Ty Le (%)" 
         << " | " << left << setw(28) << "Bieu Do Ty Le" << " |\n";
    cout << "  +" << string(22, '=') << "+" << string(12, '=') << "+" << string(15, '=') << "+" << string(30, '=') << "+\n";

    auto inDongThongKe = [total](const string& nhan, int soLuong, int color) {
        double tyLe = (double)soLuong * 100.0 / total;
        int soKyTu = (int)(tyLe / 4.0);
        string bieuDo = string(soKyTu, '#');

        ConsoleUI::setColor(color);
        cout << "  | " << left << setw(20) << nhan
             << " | " << right << setw(10) << soLuong
             << " | " << fixed << setprecision(1) << setw(12) << tyLe << "%"
             << " | " << left << setw(28) << bieuDo << " |\n";
        ConsoleUI::resetColor();
    };

    inDongThongKe(">= 27.0 (Xuat sac)", xuatSac, COLOR_YELLOW);
    inDongThongKe("24.0 - < 27.0 (Gioi)", gioi, COLOR_GREEN);
    inDongThongKe("21.0 - < 24.0 (Kha)", kha, COLOR_CYAN);
    inDongThongKe("15.0 - < 21.0 (TB)", tb, COLOR_WHITE);
    inDongThongKe("< 15.0 (Yeu)", kem, COLOR_RED);

    cout << "  +" << string(22, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+" << string(30, '-') << "+\n";
    ConsoleUI::pauseScreen();
}

// Thống kê theo quê quán
void QuanLyThiSinh::thongKeTheoQueQuan() const {
    if (dsThiSinh.empty()) {
        ConsoleUI::printWarning("Chua co du lieu thi sinh de thong ke!");
        ConsoleUI::pauseScreen();
        return;
    }

    struct QueCount {
        string que;
        int count;
    };

    Vector<QueCount> dsQue;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        string q = dsThiSinh[i].getQueQuan();
        bool found = false;
        for (int j = 0; j < dsQue.size(); ++j) {
            if (Utils::toLower(dsQue[j].que) == Utils::toLower(q)) {
                dsQue[j].count++;
                found = true;
                break;
            }
        }
        if (!found) {
            QueCount qc;
            qc.que = q;
            qc.count = 1;
            dsQue.push_back(qc);
        }
    }

    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("THONG KE SO LUONG THI SINH THEO QUE QUAN");

    cout << "\n  +" << string(25, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+\n";
    cout << "  | " << left << setw(23) << "Que Quan / Tinh Thanh"
         << " | " << right << setw(10) << "So Luong"
         << " | " << setw(13) << "Ty Le (%)" << " |\n";
    cout << "  +" << string(25, '=') << "+" << string(12, '=') << "+" << string(15, '=') << "+\n";

    int total = dsThiSinh.size();
    for (int i = 0; i < dsQue.size(); ++i) {
        double tyLe = (double)dsQue[i].count * 100.0 / total;
        ConsoleUI::setColor(i % 2 == 0 ? COLOR_WHITE : COLOR_CYAN);
        cout << "  | " << left << setw(23) << dsQue[i].que
             << " | " << right << setw(10) << dsQue[i].count
             << " | " << fixed << setprecision(1) << setw(12) << tyLe << "%" << " |\n";
    }

    ConsoleUI::setColor(COLOR_CYAN);
    cout << "  +" << string(25, '-') << "+" << string(12, '-') << "+" << string(15, '-') << "+\n";
    ConsoleUI::resetColor();
    ConsoleUI::pauseScreen();
}

// Thiết lập điểm chuẩn
void QuanLyThiSinh::thietLapDiemChuan() {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("THIET LAP DIEM CHUAN XET TUYEN");

    cout << "  Diem chuan hien tai: " << fixed << setprecision(2) << diemChuanXetTuyen << " diem.\n\n";
    cout << "  Nhap diem chuan moi (10.0 -> 30.0): ";
    double moi;
    if (cin >> moi && moi >= 10.0 && moi <= 30.0) {
        diemChuanXetTuyen = moi;
        cin.ignore();
        ConsoleUI::printSuccess("Cap nhat diem chuan thanh cong!");
    } else {
        cin.clear();
        cin.ignore(1000, '\n');
        ConsoleUI::printError("Diem chuan nhap vao khong hop le!");
    }
    ConsoleUI::pauseScreen();
}

int QuanLyThiSinh::getSoLuong() const { return dsThiSinh.size(); }
double QuanLyThiSinh::getDiemChuan() const { return diemChuanXetTuyen; }
void QuanLyThiSinh::setDiemChuan(double diem) { diemChuanXetTuyen = diem; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() const { return dsThiSinh; }
