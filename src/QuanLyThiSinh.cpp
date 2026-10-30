#include "../include/QuanLyThiSinh.h"
#include "../include/Utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>

// Comparator sắp xếp tổng điểm giảm dần
static bool cmpTongDiemGiam(const ThiSinh& a, const ThiSinh& b) {
    if (a.tinhTongDiem() != b.tinhTongDiem()) {
        return a.tinhTongDiem() > b.tinhTongDiem();
    }
    return a.getSoBaoDanh() < b.getSoBaoDanh();
}

// Comparator sắp xếp SBD tăng dần
static bool cmpSBDTang(const ThiSinh& a, const ThiSinh& b) {
    return a.getSoBaoDanh() < b.getSoBaoDanh();
}

// Comparator sắp xếp Tên A-Z (theo tên tiếng Việt: so sánh từ cuối cùng trước)
static bool cmpTenAZ(const ThiSinh& a, const ThiSinh& b) {
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
QuanLyThiSinh::QuanLyThiSinh(const string& dirData) {
    pathNguoi = dirData + "/nguoi.txt";
    pathThiSinh = dirData + "/thisinh.txt";
    pathSystem = dirData + "/system.txt";
    diemChuanHienTai = 22.00;
    namTuyenSinh = "2026";
    tenTruong = "TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG";

    docCauHinhHeThong(pathSystem);

    if (!docDuLieu() || dsThiSinh.empty()) {
        taoDuLieuMauNeuTrong();
    }
}

// Destructor
QuanLyThiSinh::~QuanLyThiSinh() {
    luuDuLieu();
    luuCauHinhHeThong(pathSystem);
}

// Nạp dữ liệu mẫu ban đầu nếu hệ thống chưa có dữ liệu
void QuanLyThiSinh::taoDuLieuMauNeuTrong() {
    dsThiSinh.clear();
    dsThiSinh.push_back(ThiSinh("201948572134", "Nguyen Van An", "15/03/2005", "TP. Da Nang", "Nam", "DUT001", 9.50, 9.00, 9.75));
    dsThiSinh.push_back(ThiSinh("201837461928", "Tran Thi Bich", "22/07/2005", "TP. Da Nang", "Nu", "DUT002", 8.75, 8.50, 9.00));
    dsThiSinh.push_back(ThiSinh("201726354819", "Le Hoang Cong", "10/11/2005", "TP. Hue", "Nam", "DUT003", 7.50, 6.75, 7.00));
    dsThiSinh.push_back(ThiSinh("201615243701", "Pham Thi Diem", "05/01/2005", "Quang Ngai", "Nu", "DUT004", 9.00, 8.75, 8.25));
    dsThiSinh.push_back(ThiSinh("201504132692", "Vo Minh Duc", "18/09/2005", "Ha Tinh", "Nam", "DUT005", 6.25, 5.50, 6.00));
    dsThiSinh.push_back(ThiSinh("201493021583", "Hoang Thi Ha", "30/04/2005", "Nghe An", "Nu", "DUT006", 9.75, 9.50, 9.80));
    dsThiSinh.push_back(ThiSinh("201382910474", "Dang Quoc Khanh", "12/12/2005", "Quang Tri", "Nam", "DUT007", 5.00, 4.50, 5.50));
    dsThiSinh.push_back(ThiSinh("201271809365", "Bui Mai Linh", "08/08/2005", "TP. Ha Noi", "Nu", "DUT008", 8.50, 8.25, 8.75));
    dsThiSinh.push_back(ThiSinh("201160798256", "Ngo Van Nam", "25/02/2005", "TP. Ho Chi Minh", "Nam", "DUT009", 7.00, 7.25, 6.50));
    dsThiSinh.push_back(ThiSinh("201059687147", "Dinh Thao Quyen", "19/10/2005", "Gia Lai", "Nu", "DUT010", 8.00, 8.50, 7.75));
    dsThiSinh.push_back(ThiSinh("200948576038", "Phan Thanh Tung", "03/06/2005", "TP. Da Nang", "Nam", "DUT011", 9.25, 9.00, 9.25));
    dsThiSinh.push_back(ThiSinh("200837465929", "Do Thu Trang", "14/05/2005", "TP. Da Nang", "Nu", "DUT012", 6.75, 7.00, 7.50));
    dsThiSinh.push_back(ThiSinh("048203001234", "Vo Nhu Hieu", "04/03/2003", "TP. Da Nang", "Nam", "DUT013", 9.00, 3.00, 4.00));
    dsThiSinh.push_back(ThiSinh("202011223344", "Nguyen Van An", "01/01/2005", "TP. Ha Noi", "Nam", "DUT014", 9.00, 8.50, 9.25));

    luuDuLieu();
    luuCauHinhHeThong(pathSystem);
}

// 1. Đọc dữ liệu từ 2 tệp quan hệ theo chuẩn Key - Value (như đồ án mẫu)
bool QuanLyThiSinh::docDuLieu() {
    ifstream finNguoi(pathNguoi);
    if (!finNguoi.is_open()) return false;

    Vector<Nguoi> dsNguoiTam;
    string line;
    string curCCCD = "", curTen = "", curNS = "", curQue = "", curGT = "Nam";

    while (getline(finNguoi, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#NGUOI") continue;
        if (line == "====") break;

        if (line == "---") {
            if (!curCCCD.empty()) {
                dsNguoiTam.push_back(Nguoi(curCCCD, curTen, curNS, curQue, curGT));
            }
            curCCCD = curTen = curNS = curQue = "";
            curGT = "Nam";
            continue;
        }

        string k, v;
        if (Utils::tachKeyValue(line, k, v)) {
            string kLower = Utils::toLower(k);
            if (kLower == "so cccd" || kLower == "cccd") curCCCD = v;
            else if (kLower == "ho va ten" || kLower == "ho ten" || kLower == "ten") curTen = v;
            else if (kLower == "ngay sinh") curNS = v;
            else if (kLower == "que quan") curQue = v;
            else if (kLower == "gioi tinh") curGT = v;
        }
    }
    finNguoi.close();

    ifstream finTS(pathThiSinh);
    if (!finTS.is_open()) return false;

    dsThiSinh.clear();
    string curSBD = "", curFK_CCCD = "";
    double curToan = 0.0, curLy = 0.0, curHoa = 0.0;

    while (getline(finTS, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#THISINH") continue;
        if (line == "====") break;

        if (line == "---") {
            if (!curSBD.empty() && !curFK_CCCD.empty()) {
                for (size_t i = 0; i < dsNguoiTam.size(); ++i) {
                    if (dsNguoiTam[i].getSoCCCD() == curFK_CCCD) {
                        ThiSinh ts(dsNguoiTam[i].getSoCCCD(),
                                   dsNguoiTam[i].getHoTen(),
                                   dsNguoiTam[i].getNgaySinh(),
                                   dsNguoiTam[i].getQueQuan(),
                                   dsNguoiTam[i].getGioiTinh(),
                                   curSBD, curToan, curLy, curHoa);
                        dsThiSinh.push_back(ts);
                        break;
                    }
                }
            }
            curSBD = curFK_CCCD = "";
            curToan = curLy = curHoa = 0.0;
            continue;
        }

        string k, v;
        if (Utils::tachKeyValue(line, k, v)) {
            string kLower = Utils::toLower(k);
            if (kLower == "so bao danh" || kLower == "sbd") curSBD = v;
            else if (kLower == "so cccd" || kLower == "cccd") curFK_CCCD = v;
            else if (kLower == "diem toan" || kLower == "toan") {
                try { curToan = stod(v); } catch (...) {}
            }
            else if (kLower == "diem ly" || kLower == "ly") {
                try { curLy = stod(v); } catch (...) {}
            }
            else if (kLower == "diem hoa" || kLower == "hoa") {
                try { curHoa = stod(v); } catch (...) {}
            }
        }
    }
    finTS.close();

    return true;
}

// 2. Lưu dữ liệu phân rã vào 2 tệp quan hệ theo chuẩn Key - Value
bool QuanLyThiSinh::luuDuLieu() const {
    ofstream foutNguoi(pathNguoi);
    if (!foutNguoi.is_open()) return false;

    foutNguoi << "#NGUOI\n";
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        foutNguoi << "So CCCD: " << ts.getSoCCCD() << "\n"
                  << "Ho va Ten: " << ts.getHoTen() << "\n"
                  << "Ngay Sinh: " << ts.getNgaySinh() << "\n"
                  << "Que Quan: " << ts.getQueQuan() << "\n"
                  << "Gioi Tinh: " << ts.getGioiTinh() << "\n"
                  << "---\n";
    }
    foutNguoi << "====\n";
    foutNguoi.close();

    ofstream foutTS(pathThiSinh);
    if (!foutTS.is_open()) return false;

    foutTS << "#THISINH\n";
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        foutTS << "So Bao Danh: " << ts.getSoBaoDanh() << "\n"
               << "So CCCD: " << ts.getSoCCCD() << "\n"
               << fixed << setprecision(2)
               << "Diem Toan: " << ts.getDiemToan() << "\n"
               << "Diem Ly: " << ts.getDiemLy() << "\n"
               << "Diem Hoa: " << ts.getDiemHoa() << "\n"
               << "---\n";
    }
    foutTS << "====\n";
    foutTS.close();

    return true;
}

// Đọc cấu hình hệ thống (data/system.txt)
bool QuanLyThiSinh::docCauHinhHeThong(const string& path) {
    ifstream fin(path);
    if (!fin.is_open()) return false;

    string line;
    while (getline(fin, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#SYSTEM_CONFIG" || line == "====") continue;

        string k, v;
        if (Utils::tachKeyValue(line, k, v)) {
            string kLower = Utils::toLower(k);
            if (kLower == "truong" || kLower == "ten truong") tenTruong = v;
            else if (kLower == "nam tuyen sinh" || kLower == "nam") namTuyenSinh = v;
            else if (kLower == "diem chuan") {
                try { diemChuanHienTai = stod(v); } catch (...) {}
            }
        }
    }
    fin.close();
    return true;
}

// Lưu cấu hình hệ thống (data/system.txt)
bool QuanLyThiSinh::luuCauHinhHeThong(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "#SYSTEM_CONFIG\n"
         << "Truong: " << tenTruong << "\n"
         << "Nam Tuyen Sinh: " << namTuyenSinh << "\n"
         << fixed << setprecision(2)
         << "Diem Chuan: " << diemChuanHienTai << "\n"
         << "====\n";
    fout.close();
    return true;
}

// Tự động sinh Số báo danh tăng dần (DUT001 -> DUT015...)
string QuanLyThiSinh::sinhSoBaoDanhMoi() const {
    int maxId = 0;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        string sbd = dsThiSinh[i].getSoBaoDanh();
        if (sbd.length() >= 4 && Utils::toUpper(sbd.substr(0, 3)) == "DUT") {
            try {
                int num = stoi(sbd.substr(3));
                if (num > maxId) maxId = num;
            } catch (...) {}
        }
    }
    int nextId = maxId + 1;
    stringstream ss;
    ss << "DUT" << setfill('0') << setw(3) << nextId;
    return ss.str();
}

// Tìm vị trí theo Số báo danh
int QuanLyThiSinh::timViTriTheoSBD(const string& sbd) const {
    string target = Utils::toUpper(Utils::trim(sbd));
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getSoBaoDanh()) == target) {
            return (int)i;
        }
    }
    return -1;
}

// Tìm vị trí theo Số CCCD
int QuanLyThiSinh::timViTriTheoCCCD(const string& cccd) const {
    string target = Utils::trim(cccd);
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getSoCCCD() == target) {
            return (int)i;
        }
    }
    return -1;
}

// Tìm danh sách theo Họ tên
Vector<ThiSinh> QuanLyThiSinh::timTheoTen(const string& ten) const {
    Vector<ThiSinh> ketQua;
    string target = Utils::trim(ten);
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::containsIgnoreCase(dsThiSinh[i].getHoTen(), target)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }
    return ketQua;
}

// Tìm danh sách theo Quê quán
Vector<ThiSinh> QuanLyThiSinh::timTheoQueQuan(const string& que) const {
    Vector<ThiSinh> ketQua;
    string target = Utils::trim(que);
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::containsIgnoreCase(dsThiSinh[i].getQueQuan(), target)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }
    return ketQua;
}

// Thêm thí sinh mới
bool QuanLyThiSinh::themThiSinh(const ThiSinh& ts) {
    if (timViTriTheoSBD(ts.getSoBaoDanh()) != -1) return false;
    if (timViTriTheoCCCD(ts.getSoCCCD()) != -1) return false;

    dsThiSinh.push_back(ts);
    luuDuLieu();
    return true;
}

// Cập nhật thông tin thí sinh
bool QuanLyThiSinh::capNhatThiSinh(const ThiSinh& ts) {
    int idx = timViTriTheoSBD(ts.getSoBaoDanh());
    if (idx == -1) {
        idx = timViTriTheoCCCD(ts.getSoCCCD());
    }
    if (idx == -1) return false;

    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if ((int)i != idx && dsThiSinh[i].getSoCCCD() == ts.getSoCCCD()) {
            return false;
        }
    }

    dsThiSinh[idx] = ts;
    luuDuLieu();
    return true;
}

// Xóa thí sinh (Cascade delete)
bool QuanLyThiSinh::xoaThiSinh(const string& sbdHoacCCCD) {
    int idx = timViTriTheoSBD(sbdHoacCCCD);
    if (idx == -1) {
        idx = timViTriTheoCCCD(sbdHoacCCCD);
    }
    if (idx == -1) return false;

    dsThiSinh.erase(idx);
    luuDuLieu();
    return true;
}

// Sắp xếp
void QuanLyThiSinh::sapXepTheoTongDiemGiamDan() {
    dsThiSinh.sort(cmpTongDiemGiam);
}

void QuanLyThiSinh::sapXepTheoTenAZ() {
    dsThiSinh.sort(cmpTenAZ);
}

void QuanLyThiSinh::sapXepTheoSBDTangDan() {
    dsThiSinh.sort(cmpSBDTang);
}

// Getters / Setters
double QuanLyThiSinh::getDiemChuan() const { return diemChuanHienTai; }
void QuanLyThiSinh::setDiemChuan(double dc) {
    diemChuanHienTai = dc;
    luuCauHinhHeThong(pathSystem);
}
string QuanLyThiSinh::getNamTuyenSinh() const { return namTuyenSinh; }
void QuanLyThiSinh::setNamTuyenSinh(const string& nam) {
    namTuyenSinh = nam;
    luuCauHinhHeThong(pathSystem);
}
string QuanLyThiSinh::getTenTruong() const { return tenTruong; }

// Lấy danh sách trúng tuyển
Vector<ThiSinh> QuanLyThiSinh::getDanhSachTrungTuyen(double diemChuan) const {
    Vector<ThiSinh> ketQua;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].xetTrungTuyen(diemChuan)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }
    ketQua.sort(cmpTongDiemGiam);
    return ketQua;
}

// Lấy danh sách thủ khoa kỳ thi
Vector<ThiSinh> QuanLyThiSinh::getDanhSachThuKhoa() const {
    Vector<ThiSinh> thuKhoaList;
    if (dsThiSinh.empty()) return thuKhoaList;

    double maxDiem = dsThiSinh[0].tinhTongDiem();
    for (size_t i = 1; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() > maxDiem) {
            maxDiem = dsThiSinh[i].tinhTongDiem();
        }
    }

    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() == maxDiem) {
            thuKhoaList.push_back(dsThiSinh[i]);
        }
    }
    return thuKhoaList;
}

// Thống kê phân bố phổ điểm
void QuanLyThiSinh::tinhThongKePhoDiem(int& mucThap, int& mucTB, int& mucKha, int& mucCao, int& mucRatCao) const {
    mucThap = mucTB = mucKha = mucCao = mucRatCao = 0;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        double tong = dsThiSinh[i].tinhTongDiem();
        if (tong >= 27.00) mucRatCao++;
        else if (tong >= 24.00) mucCao++;
        else if (tong >= 21.00) mucKha++;
        else if (tong >= 15.00) mucTB++;
        else mucThap++;
    }
}

// 6.1 Xuất báo cáo kết quả xét tuyển toàn trường (data/ketqua_xettuyen.txt)
bool QuanLyThiSinh::xuatBaoCaoXetTuyen(const string& path, double diemChuan, const string& keyword) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    string kw = Utils::trim(keyword);
    string kwLower = Utils::toLower(kw);

    fout << "                                  " << tenTruong << "\n";
    fout << "                                       BAO CAO KET QUA XET TUYEN DAI HOC\n";
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinh << ")\n";
    fout << "                                        Diem chuan xet tuyen: " << fixed << setprecision(2) << diemChuan << " diem\n";
    if (!kw.empty()) {
        fout << "                                        Tu khoa tim kiem    : \"" << kw << "\"\n";
    }
    fout << "===================================================================================================================\n";
    fout << " STT |   SBD   |    So CCCD   |      Ho va Ten      | Ngay Sinh |   Que Quan   | Toan |  Ly  | Hoa  | Tong | Ket Qua  |\n";
    fout << "-------------------------------------------------------------------------------------------------------------------\n";

    int soXetTuyen = 0;
    int soTrungTuyen = 0;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        if (!kwLower.empty()) {
            bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getSoCCCD(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getHoTen(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getQueQuan(), kwLower);
            if (!match) continue;
        }

        soXetTuyen++;
        bool pass = ts.xetTrungTuyen(diemChuan);
        if (pass) soTrungTuyen++;

        fout << " " << right << setw(3) << soXetTuyen << " | "
             << left  << setw(7) << ts.getSoBaoDanh() << " | "
             << left  << setw(12) << ts.getSoCCCD() << " | "
             << left  << setw(19) << ts.getHoTen().substr(0, 19) << " | "
             << left  << setw(10) << ts.getNgaySinh() << " | "
             << left  << setw(12) << ts.getQueQuan().substr(0, 12) << " | "
             << right << fixed << setprecision(2)
             << setw(4) << ts.getDiemToan() << " | "
             << setw(4) << ts.getDiemLy() << " | "
             << setw(4) << ts.getDiemHoa() << " | "
             << setw(4) << ts.tinhTongDiem() << " |"
             << left << (pass ? "TRUNG TUYEN" : (ts.kiemTraDiemLiet() ? "Diem liet  " : "Khong dat  ")) << "\n";
    }

    fout << "===================================================================================================================\n";
    fout << "Tong so thi sinh bao cao : " << soXetTuyen << "\n";
    double tiLe = (soXetTuyen == 0) ? 0.0 : ((double)soTrungTuyen * 100.0 / soXetTuyen);
    fout << "So thi sinh trung tuyen  : " << soTrungTuyen << " (" << fixed << setprecision(1) << tiLe << "%)\n";
    fout << "So thi sinh khong dat    : " << (soXetTuyen - soTrungTuyen) << " (" << fixed << setprecision(1) << (100.0 - tiLe) << "%)\n";

    Vector<ThiSinh> dsTK = getDanhSachThuKhoa();
    if (!dsTK.empty()) {
        fout << "Thu khoa ky thi          : ";
        for (size_t i = 0; i < dsTK.size(); ++i) {
            fout << dsTK[i].getHoTen() << " (" << dsTK[i].getSoBaoDanh() << ")";
            if (i + 1 < dsTK.size()) fout << ", ";
        }
        fout << " - Tong diem: " << fixed << setprecision(2) << dsTK[0].tinhTongDiem() << "\n";
    }
    fout << "===================================================================================================================\n";

    fout.close();
    return true;
}

// 6.2 Xuất báo cáo vinh danh thủ khoa riêng biệt (data/thukhoa.txt - Giống file driver_salary_statistics của mẫu)
bool QuanLyThiSinh::xuatBaoCaoThuKhoa(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "                                  " << tenTruong << "\n";
    fout << "                                    BANG VINH DANH THU KHOA KY THI TUYEN SINH\n";
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinh << ")\n";
    fout << "=========================================================================================================\n";
    fout << " STT |   SBD   |    So CCCD   |       Ho va Ten      | Ngay Sinh |   Que Quan   | Toan |  Ly  | Hoa  | Tong  |\n";
    fout << "---------------------------------------------------------------------------------------------------------\n";

    Vector<ThiSinh> dsTK = getDanhSachThuKhoa();
    for (size_t i = 0; i < dsTK.size(); ++i) {
        const ThiSinh& ts = dsTK[i];
        fout << " " << right << setw(3) << (i + 1) << " | "
             << left  << setw(7) << ts.getSoBaoDanh() << " | "
             << left  << setw(12) << ts.getSoCCCD() << " | "
             << left  << setw(20) << ts.getHoTen().substr(0, 20) << " | "
             << left  << setw(10) << ts.getNgaySinh() << " | "
             << left  << setw(12) << ts.getQueQuan().substr(0, 12) << " | "
             << right << fixed << setprecision(2)
             << setw(4) << ts.getDiemToan() << " | "
             << setw(4) << ts.getDiemLy() << " | "
             << setw(4) << ts.getDiemHoa() << " | "
             << setw(5) << ts.tinhTongDiem() << " |\n";
    }
    fout << "=========================================================================================================\n";
    fout.close();
    return true;
}

// 6.3 Xuất báo cáo phân tích phổ điểm (data/thongke_phodiem.txt - Giống file drivers_statistics của mẫu)
bool QuanLyThiSinh::xuatBaoCaoPhoDiem(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    int thap, tb, kha, cao, ratCao;
    tinhThongKePhoDiem(thap, tb, kha, cao, ratCao);
    size_t total = dsThiSinh.size();

    fout << "                                  " << tenTruong << "\n";
    fout << "                                       BANG PHAN TICH PHO DIEM THI\n";
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinh << ")\n";
    fout << "=========================================================================================================\n";
    fout << " STT |         Muc Diem Thi         | Khoang Tong Diem (3 Mon) | So Luong Thi Sinh |     Ty Le Phan Tram     |\n";
    fout << "---------------------------------------------------------------------------------------------------------\n";

    auto inDong = [&](int stt, const string& tenMuc, const string& khoang, int soLuong) {
        double pct = total > 0 ? (soLuong * 100.0 / total) : 0.0;
        fout << " " << right << setw(3) << stt << " | "
             << left  << setw(28) << tenMuc << " | "
             << left  << setw(24) << khoang << " | "
             << right << setw(17) << soLuong << " | "
             << right << fixed << setprecision(2) << setw(19) << pct << "% |\n";
    };

    inDong(1, "Muc diem Rat Cao (Xuat sac)", ">= 27.00 diem", ratCao);
    inDong(2, "Muc diem Cao (Gioi)", "24.00 <= Diem < 27.00", cao);
    inDong(3, "Muc diem Kha", "21.00 <= Diem < 24.00", kha);
    inDong(4, "Muc diem Trung binh", "15.00 <= Diem < 21.00", tb);
    inDong(5, "Muc diem Thap", "< 15.00 diem", thap);

    fout << "=========================================================================================================\n";
    fout << "Tong so thi sinh toan truong: " << total << "\n";
    fout << "Diem chuan xet tuyen hien tai: " << fixed << setprecision(2) << diemChuanHienTai << " diem\n";
    Vector<ThiSinh> dsTT = getDanhSachTrungTuyen(diemChuanHienTai);
    fout << "So thi sinh dat diem chuan  : " << dsTT.size() << " ("
         << fixed << setprecision(1) << (total > 0 ? (dsTT.size() * 100.0 / total) : 0.0) << "%)\n";
    fout << "=========================================================================================================\n";

    fout.close();
    return true;
}

// Xuất đồng thời cả 3 báo cáo chuyên biệt
bool QuanLyThiSinh::xuatTatCaBaoCao() const {
    bool ok1 = xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt", diemChuanHienTai);
    bool ok2 = xuatBaoCaoThuKhoa("data/thukhoa.txt");
    bool ok3 = xuatBaoCaoPhoDiem("data/thongke_phodiem.txt");
    return (ok1 && ok2 && ok3);
}

Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() { return dsThiSinh; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() const { return dsThiSinh; }
