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

// Comparator sắp xếp Tên A-Z
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
    pathTinhThanh = dirData + "/tinhthanh.txt";
    pathGioiTinh = dirData + "/gioitinh.txt";
    pathSystem = dirData + "/system.txt";

    diemChuanHienTai = AdmissionConst::DIEM_CHUAN_MAC_DINH;
    diemLietHienTai = AdmissionConst::DIEM_LIET;
    namTuyenSinh = "2026";
    tenTruong = "TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG";

    // 1. Nạp cấu hình hệ thống
    docCauHinhHeThong(pathSystem);

    // 2. Nạp danh mục hệ thống (Tỉnh thành & Giới tính)
    docDanhMucTinhThanh(pathTinhThanh);
    docDanhMucGioiTinh(pathGioiTinh);

    // 3. Nạp dữ liệu thí sinh & hồ sơ
    if (!docDuLieu() || dsThiSinh.empty()) {
        taoDuLieuMauNeuTrong();
    }
}

// Destructor
QuanLyThiSinh::~QuanLyThiSinh() {
    luuDuLieu();
    luuDanhMucTinhThanh(pathTinhThanh);
    luuDanhMucGioiTinh(pathGioiTinh);
    luuCauHinhHeThong(pathSystem);
}

// Đọc danh mục tỉnh thành
bool QuanLyThiSinh::docDanhMucTinhThanh(const string& path) {
    dsTinhThanh.clear();
    ifstream fin(path);
    if (!fin.is_open()) {
        // Khởi tạo 34 tỉnh thành mặc định nếu file chưa có
        dsTinhThanh.push_back(TinhThanh("01", "TP. Ha Noi", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("04", "Cao Bang", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("08", "Tuyen Quang", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("10", "Lao Cai", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("11", "Dien Bien", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("12", "Lai Chau", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("14", "Son La", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("19", "Thai Nguyen", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("20", "Lang Son", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("22", "Quang Ninh", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("25", "Phu Tho", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("27", "Bac Ninh", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("31", "TP. Hai Phong", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("33", "Hung Yen", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("37", "Ninh Binh", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("38", "Thanh Hoa", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("40", "Nghe An", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("42", "Ha Tinh", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("45", "Quang Tri", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("46", "TP. Hue", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("48", "TP. Da Nang", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("51", "Quang Ngai", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("56", "Khanh Hoa", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("64", "Gia Lai", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("66", "Dak Lak", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("68", "Lam Dong", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("72", "Tay Ninh", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("75", "Dong Nai", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("79", "TP. Ho Chi Minh", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("86", "Vinh Long", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("87", "Dong Thap", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("89", "An Giang", "Tinh truc thuoc"));
        dsTinhThanh.push_back(TinhThanh("92", "TP. Can Tho", "Thanh pho truc thuoc Trung uong"));
        dsTinhThanh.push_back(TinhThanh("96", "Ca Mau", "Tinh truc thuoc"));
        luuDanhMucTinhThanh(path);
        return true;
    }

    string line;
    string curMa = "", curTen = "", curLoai = "";
    while (getline(fin, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#TINHTHANH") continue;
        if (line == "====") break;
        if (line == "---") {
            if (!curMa.empty() && !curTen.empty()) {
                dsTinhThanh.push_back(TinhThanh(curMa, curTen, curLoai));
            }
            curMa = curTen = curLoai = "";
            continue;
        }

        string k, v;
        if (Utils::tachKeyValue(line, k, v)) {
            string kLower = Utils::toLower(k);
            if (kLower == "ma" || kLower == "ma tinh") curMa = v;
            else if (kLower == "ten" || kLower == "ten tinh") curTen = v;
            else if (kLower == "loai" || kLower == "loai hinh") curLoai = v;
        }
    }
    fin.close();
    return true;
}

// Lưu danh mục tỉnh thành
bool QuanLyThiSinh::luuDanhMucTinhThanh(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "#TINHTHANH\n";
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        fout << "Ma: " << dsTinhThanh[i].ma << "\n"
             << "Ten: " << dsTinhThanh[i].ten << "\n"
             << "Loai: " << dsTinhThanh[i].loai << "\n"
             << "---\n";
    }
    fout << "====\n";
    fout.close();
    return true;
}

// Đọc danh mục giới tính
bool QuanLyThiSinh::docDanhMucGioiTinh(const string& path) {
    dsGioiTinh.clear();
    ifstream fin(path);
    if (!fin.is_open()) {
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_NAM, "Nam", "Nam gioi"));
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_NU, "Nu", "Nu gioi"));
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_KHAC, "Khac", "Gioi tinh khac"));
        luuDanhMucGioiTinh(path);
        return true;
    }

    string line;
    string curMa = "", curTen = "", curDesc = "";
    while (getline(fin, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#GIOITINH") continue;
        if (line == "====") break;
        if (line == "---") {
            if (!curMa.empty() && !curTen.empty()) {
                dsGioiTinh.push_back(GioiTinh(curMa, curTen, curDesc));
            }
            curMa = curTen = curDesc = "";
            continue;
        }

        string k, v;
        if (Utils::tachKeyValue(line, k, v)) {
            string kLower = Utils::toLower(k);
            if (kLower == "ma" || kLower == "ma gioi tinh") curMa = v;
            else if (kLower == "ten" || kLower == "ten gioi tinh") curTen = v;
            else if (kLower == "mo ta") curDesc = v;
        }
    }
    fin.close();

    if (dsGioiTinh.empty()) {
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_NAM, "Nam", "Nam gioi"));
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_NU, "Nu", "Nu gioi"));
        dsGioiTinh.push_back(GioiTinh(AdmissionConst::GT_KHAC, "Khac", "Gioi tinh khac"));
        luuDanhMucGioiTinh(path);
    }
    return true;
}

// Lưu danh mục giới tính
bool QuanLyThiSinh::luuDanhMucGioiTinh(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "#GIOITINH\n";
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        fout << "Ma: " << dsGioiTinh[i].ma << "\n"
             << "Ten: " << dsGioiTinh[i].ten << "\n"
             << "Mo Ta: " << dsGioiTinh[i].moTa << "\n"
             << "---\n";
    }
    fout << "====\n";
    fout.close();
    return true;
}

// Tra cứu tên tỉnh thành từ mã hoặc tên
string QuanLyThiSinh::layTenTinhThanh(const string& maHoacTen) const {
    string key = Utils::trim(maHoacTen);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == key || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(key)) {
            return dsTinhThanh[i].ten;
        }
    }
    return key;
}

// Tra cứu mã tỉnh thành từ tên hoặc mã
string QuanLyThiSinh::layMaTinhThanh(const string& tenHoacMa) const {
    string key = Utils::trim(tenHoacMa);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == key || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(key)) {
            return dsTinhThanh[i].ma;
        }
    }
    return key;
}

// Tra cứu tên giới tính
string QuanLyThiSinh::layTenGioiTinh(const string& maHoacTen) const {
    string key = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == key || Utils::toUpper(dsGioiTinh[i].ten) == key) {
            return dsGioiTinh[i].ten;
        }
    }
    return maHoacTen;
}

// Tra cứu mã giới tính
string QuanLyThiSinh::layMaGioiTinh(const string& tenHoacMa) const {
    string key = Utils::toUpper(Utils::trim(tenHoacMa));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == key || Utils::toUpper(dsGioiTinh[i].ten) == key) {
            return dsGioiTinh[i].ma;
        }
    }
    return key;
}

// Kiểm tra toàn vẹn tham chiếu quê quán
bool QuanLyThiSinh::kiemTraHopLeQueQuan(const string& maHoacTen) const {
    string key = Utils::trim(maHoacTen);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == key || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(key)) {
            return true;
        }
    }
    return false;
}

// Kiểm tra toàn vẹn tham chiếu giới tính
bool QuanLyThiSinh::kiemTraHopLeGioiTinh(const string& maHoacTen) const {
    string key = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == key || Utils::toUpper(dsGioiTinh[i].ten) == key) {
            return true;
        }
    }
    return false;
}

// Nạp dữ liệu mẫu ban đầu nếu hệ thống chưa có dữ liệu
void QuanLyThiSinh::taoDuLieuMauNeuTrong() {
    dsThiSinh.clear();
    dsThiSinh.push_back(ThiSinh("201948572134", "Nguyen Van An", "15/03/2005", "48", "NAM", "DUT001", 9.50, 9.00, 9.75));
    dsThiSinh.push_back(ThiSinh("201837461928", "Tran Thi Bich", "22/07/2005", "48", "NU",  "DUT002", 8.75, 8.50, 9.00));
    dsThiSinh.push_back(ThiSinh("201726354819", "Le Hoang Cong", "10/11/2005", "46", "NAM", "DUT003", 7.50, 6.75, 7.00));
    dsThiSinh.push_back(ThiSinh("201615243701", "Pham Thi Diem", "05/01/2005", "51", "NU",  "DUT004", 9.00, 8.75, 8.25));
    dsThiSinh.push_back(ThiSinh("201504132692", "Vo Minh Duc", "18/09/2005", "42", "NAM", "DUT005", 6.25, 5.50, 6.00));
    dsThiSinh.push_back(ThiSinh("201493021583", "Hoang Thi Ha", "30/04/2005", "40", "NU",  "DUT006", 9.75, 9.50, 9.80));
    dsThiSinh.push_back(ThiSinh("201382910474", "Dang Quoc Khanh", "12/12/2005", "45", "NAM", "DUT007", 5.00, 4.50, 5.50));
    dsThiSinh.push_back(ThiSinh("201271809365", "Bui Mai Linh", "08/08/2005", "01", "NU",  "DUT008", 8.50, 8.25, 8.75));
    dsThiSinh.push_back(ThiSinh("201160798256", "Ngo Van Nam", "25/02/2005", "79", "NAM", "DUT009", 7.00, 7.25, 6.50));
    dsThiSinh.push_back(ThiSinh("201059687147", "Dinh Thao Quyen", "19/10/2005", "64", "NU",  "DUT010", 8.00, 8.50, 7.75));
    dsThiSinh.push_back(ThiSinh("200948576038", "Phan Thanh Tung", "03/06/2005", "48", "NAM", "DUT011", 9.25, 9.00, 9.25));
    dsThiSinh.push_back(ThiSinh("200837465929", "Do Thu Trang", "14/05/2005", "48", "NU",  "DUT012", 6.75, 7.00, 7.50));
    dsThiSinh.push_back(ThiSinh("048203001234", "Vo Nhu Hieu", "04/03/2003", "48", "NAM", "DUT013", 9.00, 3.00, 4.00));
    dsThiSinh.push_back(ThiSinh("202011223344", "Nguyen Van An", "01/01/2005", "01", "NAM", "DUT014", 9.00, 8.50, 9.25));

    luuDuLieu();
    luuCauHinhHeThong(pathSystem);
}

// 1. Đọc dữ liệu từ 2 tệp quan hệ theo chuẩn Key - Value
bool QuanLyThiSinh::docDuLieu() {
    ifstream finNguoi(pathNguoi);
    if (!finNguoi.is_open()) return false;

    Vector<Nguoi> dsNguoiTam;
    string line;
    string curCCCD = "", curTen = "", curNS = "", curQue = "", curGT = AdmissionConst::GT_NAM;

    while (getline(finNguoi, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "#NGUOI") continue;
        if (line == "====") break;

        if (line == "---") {
            if (!curCCCD.empty()) {
                // Chuẩn hóa mã tỉnh thành và mã giới tính để đảm bảo toàn vẹn
                string normQue = layMaTinhThanh(curQue);
                string normGT  = layMaGioiTinh(curGT);
                dsNguoiTam.push_back(Nguoi(curCCCD, curTen, curNS, normQue, normGT));
            }
            curCCCD = curTen = curNS = curQue = "";
            curGT = AdmissionConst::GT_NAM;
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

// 2. Lưu dữ liệu phân rã vào 2 tệp quan hệ theo chuẩn Key - Value (Lưu mã quan hệ 3NF)
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
            else if (kLower == "diem liet") {
                try { diemLietHienTai = stod(v); } catch (...) {}
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
         << "Diem Liet: " << diemLietHienTai << "\n"
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
        string tenQue = layTenTinhThanh(dsThiSinh[i].getQueQuan());
        if (Utils::containsIgnoreCase(dsThiSinh[i].getQueQuan(), target) ||
            Utils::containsIgnoreCase(tenQue, target)) {
            ketQua.push_back(dsThiSinh[i]);
        }
    }
    return ketQua;
}

// Thêm thí sinh mới (Có kiểm tra tính toàn vẹn tham chiếu)
bool QuanLyThiSinh::themThiSinh(const ThiSinh& ts) {
    if (timViTriTheoSBD(ts.getSoBaoDanh()) != -1) return false;
    if (timViTriTheoCCCD(ts.getSoCCCD()) != -1) return false;

    // Kiểm tra ràng buộc khóa ngoại tham chiếu
    if (!kiemTraHopLeQueQuan(ts.getQueQuan())) return false;
    if (!kiemTraHopLeGioiTinh(ts.getGioiTinh())) return false;

    // Chuẩn hóa sang mã lưu trữ
    ThiSinh chuanHoa = ts;
    chuanHoa.setQueQuan(layMaTinhThanh(ts.getQueQuan()));
    chuanHoa.setGioiTinh(layMaGioiTinh(ts.getGioiTinh()));

    dsThiSinh.push_back(chuanHoa);
    luuDuLieu();
    xuatTatCaBaoCao();
    return true;
}

// Cập nhật thông tin thí sinh (Có kiểm tra tính toàn vẹn tham chiếu)
bool QuanLyThiSinh::capNhatThiSinh(const ThiSinh& ts) {
    int idx = timViTriTheoSBD(ts.getSoBaoDanh());
    if (idx == -1) {
        idx = timViTriTheoCCCD(ts.getSoCCCD());
    }
    if (idx == -1) return false;

    // Kiểm tra trùng CCCD với thí sinh khác
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if ((int)i != idx && dsThiSinh[i].getSoCCCD() == ts.getSoCCCD()) {
            return false;
        }
    }

    if (!kiemTraHopLeQueQuan(ts.getQueQuan())) return false;
    if (!kiemTraHopLeGioiTinh(ts.getGioiTinh())) return false;

    ThiSinh chuanHoa = ts;
    chuanHoa.setQueQuan(layMaTinhThanh(ts.getQueQuan()));
    chuanHoa.setGioiTinh(layMaGioiTinh(ts.getGioiTinh()));

    dsThiSinh[idx] = chuanHoa;
    luuDuLieu();
    xuatTatCaBaoCao();
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
    xuatTatCaBaoCao();
    return true;
}

// CRUD Danh mục Tỉnh thành
bool QuanLyThiSinh::themTinhThanh(const TinhThanh& tt) {
    if (tt.ma.empty() || tt.ten.empty()) return false;
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == tt.ma) return false;
    }
    dsTinhThanh.push_back(tt);
    luuDanhMucTinhThanh(pathTinhThanh);
    return true;
}

bool QuanLyThiSinh::capNhatTinhThanh(const TinhThanh& tt) {
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == tt.ma) {
            dsTinhThanh[i] = tt;
            luuDanhMucTinhThanh(pathTinhThanh);
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::xoaTinhThanh(const string& ma) {
    // Kiểm tra ràng buộc: không được xóa nếu có thí sinh đang tham chiếu
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getQueQuan() == ma) return false;
    }
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == ma) {
            dsTinhThanh.erase(i);
            luuDanhMucTinhThanh(pathTinhThanh);
            return true;
        }
    }
    return false;
}

// CRUD Danh mục Giới tính
bool QuanLyThiSinh::themGioiTinh(const GioiTinh& gt) {
    if (gt.ma.empty() || gt.ten.empty()) return false;
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == Utils::toUpper(gt.ma)) return false;
    }
    dsGioiTinh.push_back(gt);
    luuDanhMucGioiTinh(pathGioiTinh);
    return true;
}

bool QuanLyThiSinh::capNhatGioiTinh(const GioiTinh& gt) {
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == Utils::toUpper(gt.ma)) {
            dsGioiTinh[i] = gt;
            luuDanhMucGioiTinh(pathGioiTinh);
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::xoaGioiTinh(const string& ma) {
    string mUpper = Utils::toUpper(ma);
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getGioiTinh()) == mUpper) return false;
    }
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == mUpper) {
            dsGioiTinh.erase(i);
            luuDanhMucGioiTinh(pathGioiTinh);
            return true;
        }
    }
    return false;
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
    xuatTatCaBaoCao();
}
double QuanLyThiSinh::getDiemLiet() const { return diemLietHienTai; }
void QuanLyThiSinh::setDiemLiet(double dl) {
    diemLietHienTai = dl;
    luuCauHinhHeThong(pathSystem);
    xuatTatCaBaoCao();
}
string QuanLyThiSinh::getNamTuyenSinh() const { return namTuyenSinh; }
void QuanLyThiSinh::setNamTuyenSinh(const string& nam) {
    namTuyenSinh = nam;
    luuCauHinhHeThong(pathSystem);
    xuatTatCaBaoCao();
}
string QuanLyThiSinh::getTenTruong() const { return tenTruong; }
void QuanLyThiSinh::setTenTruong(const string& truong) {
    tenTruong = truong;
    luuCauHinhHeThong(pathSystem);
    xuatTatCaBaoCao();
}

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

// Thống kê phân bố phổ điểm (sử dụng hằng số AdmissionConst)
void QuanLyThiSinh::tinhThongKePhoDiem(int& mucThap, int& mucTB, int& mucKha, int& mucCao, int& mucRatCao) const {
    mucThap = mucTB = mucKha = mucCao = mucRatCao = 0;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        double tong = dsThiSinh[i].tinhTongDiem();
        if (tong >= AdmissionConst::MOC_XUAT_SAC) mucRatCao++;
        else if (tong >= AdmissionConst::MOC_GIOI) mucCao++;
        else if (tong >= AdmissionConst::MOC_KHA) mucKha++;
        else if (tong >= AdmissionConst::MOC_TRUNG_BINH) mucTB++;
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
        string tenQue = layTenTinhThanh(ts.getQueQuan());
        if (!kwLower.empty()) {
            bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getSoCCCD(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getHoTen(), kwLower) ||
                         Utils::containsIgnoreCase(ts.getQueQuan(), kwLower) ||
                         Utils::containsIgnoreCase(tenQue, kwLower);
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
             << left  << setw(12) << tenQue.substr(0, 12) << " | "
             << right << fixed << setprecision(2)
             << setw(4) << ts.getDiemToan() << " | "
             << setw(4) << ts.getDiemLy() << " | "
             << setw(4) << ts.getDiemHoa() << " | "
             << setw(4) << ts.tinhTongDiem() << " |"
             << left << (pass ? AdmissionConst::KET_QUA_TRUNG_TUYEN : (ts.kiemTraDiemLiet() ? "Diem liet  " : AdmissionConst::KET_QUA_KHONG_DAT)) << "\n";
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

// 6.2 Xuất báo cáo vinh danh thủ khoa riêng biệt (data/thukhoa.txt)
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
        string tenQue = layTenTinhThanh(ts.getQueQuan());
        fout << " " << right << setw(3) << (i + 1) << " | "
             << left  << setw(7) << ts.getSoBaoDanh() << " | "
             << left  << setw(12) << ts.getSoCCCD() << " | "
             << left  << setw(20) << ts.getHoTen().substr(0, 20) << " | "
             << left  << setw(10) << ts.getNgaySinh() << " | "
             << left  << setw(12) << tenQue.substr(0, 12) << " | "
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

// 6.3 Xuất báo cáo phân tích phổ điểm (data/thongke_phodiem.txt)
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

    inDong(1, AdmissionConst::TEN_MUC_XUAT_SAC, ">= 27.00 diem", ratCao);
    inDong(2, AdmissionConst::TEN_MUC_GIOI, "24.00 <= Diem < 27.00", cao);
    inDong(3, AdmissionConst::TEN_MUC_KHA, "21.00 <= Diem < 24.00", kha);
    inDong(4, AdmissionConst::TEN_MUC_TRUNG_BINH, "15.00 <= Diem < 21.00", tb);
    inDong(5, AdmissionConst::TEN_MUC_THAP, "< 15.00 diem", thap);

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

// Getters danh sách
Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() { return dsThiSinh; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() const { return dsThiSinh; }
Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() { return dsTinhThanh; }
const Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() const { return dsTinhThanh; }
Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() { return dsGioiTinh; }
const Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() const { return dsGioiTinh; }
