#include "../include/QuanLyThiSinh.h"
#include "../include/Utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Hàm tách xâu theo ký tự phân cách
static Vector<string> split(const string& str, char delim) {
    Vector<string> tokens;
    string token;
    stringstream ss(str);
    while (getline(ss, token, delim)) {
        tokens.push_back(Utils::trim(token));
    }
    return tokens;
}

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

static bool cmpNganh(const ThiSinh& a, const ThiSinh& b) {
    if (a.getMaNganh() != b.getMaNganh()) {
        return a.getMaNganh() < b.getMaNganh();
    }
    return a.tinhTongDiem() > b.tinhTongDiem();
}

// Constructor
QuanLyThiSinh::QuanLyThiSinh(const string& dirData) {
    pathKhuVuc = dirData + "/danhmuc_khuvuc.txt";
    pathNganh = dirData + "/danhmuc_nganh.txt";
    pathHoSo = dirData + "/hoso_thisinh.txt";
    pathDangKy = dirData + "/dangky_thi.txt";
    pathLegacy = dirData + "/thisinh.txt";

    docTatCaDuLieu();
}

// Destructor
QuanLyThiSinh::~QuanLyThiSinh() {
    luuTatCaDuLieu();
}

// Đọc toàn bộ CSDL quan hệ từ các tệp
bool QuanLyThiSinh::docTatCaDuLieu() {
    dsKhuVuc.clear();
    dsNganh.clear();
    dsThiSinh.clear();

    // 1. Đọc Bảng 1: KhuVuc
    ifstream finKV(pathKhuVuc);
    if (finKV.is_open()) {
        int count = 0;
        finKV >> count;
        string line;
        getline(finKV, line);
        for (int i = 0; i < count && getline(finKV, line); ++i) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            Vector<string> parts = split(line, '|');
            if (parts.size() >= 3) {
                try {
                    double diem = stod(parts[2]);
                    dsKhuVuc.push_back(KhuVuc(parts[0], parts[1], diem));
                } catch (...) {}
            }
        }
        finKV.close();
    }

    // Nếu bảng khu vực trống, nạp mặc định
    if (dsKhuVuc.empty()) {
        dsKhuVuc.push_back(KhuVuc("KV1", "Khu vuc 1", 0.75));
        dsKhuVuc.push_back(KhuVuc("KV2NT", "Khu vuc 2 Nong thon", 0.50));
        dsKhuVuc.push_back(KhuVuc("KV2", "Khu vuc 2", 0.25));
        dsKhuVuc.push_back(KhuVuc("KV3", "Khu vuc 3 (Khong uu tien)", 0.00));
    }

    // 2. Đọc Bảng 2: NganhXetTuyen
    ifstream finNg(pathNganh);
    if (finNg.is_open()) {
        int count = 0;
        finNg >> count;
        string line;
        getline(finNg, line);
        for (int i = 0; i < count && getline(finNg, line); ++i) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            Vector<string> parts = split(line, '|');
            if (parts.size() >= 5) {
                try {
                    int ct = stoi(parts[3]);
                    double dc = stod(parts[4]);
                    dsNganh.push_back(NganhXetTuyen(parts[0], parts[1], parts[2], ct, dc));
                } catch (...) {}
            }
        }
        finNg.close();
    }

    // Nếu bảng ngành trống, nạp mặc định
    if (dsNganh.empty()) {
        dsNganh.push_back(NganhXetTuyen("7480201", "Cong nghe Thong tin", "A00", 150, 26.25));
        dsNganh.push_back(NganhXetTuyen("7520207", "Ky thuat Dieu khien & Tu dong hoa", "A00", 120, 24.50));
        dsNganh.push_back(NganhXetTuyen("7520114", "Ky thuat Co dien tu", "A00", 100, 23.75));
        dsNganh.push_back(NganhXetTuyen("7510301", "Ky thuat Dien - Dien tu", "A00", 80, 22.00));
        dsNganh.push_back(NganhXetTuyen("7580201", "Ky thuat Xay dung", "A00", 90, 19.50));
    }

    // 3. Đọc Bảng 3: HoSoThiSinh và Bảng 4: DangKyThi
    // Đọc thông tin hồ sơ trước
    struct HoSoTam {
        string maTS;
        string cmnd;
        string hoTen;
        string ngaySinh;
        string queQuan;
        string gioiTinh;
        string maKV;
    };
    Vector<HoSoTam> dsHoSo;

    ifstream finHoSo(pathHoSo);
    if (finHoSo.is_open()) {
        int count = 0;
        finHoSo >> count;
        string line;
        getline(finHoSo, line);
        for (int i = 0; i < count && getline(finHoSo, line); ++i) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            Vector<string> parts = split(line, '|');
            if (parts.size() >= 7) {
                HoSoTam hs;
                hs.maTS = parts[0];
                hs.cmnd = parts[1];
                hs.hoTen = parts[2];
                hs.ngaySinh = parts[3];
                hs.queQuan = parts[4];
                hs.gioiTinh = parts[5];
                hs.maKV = parts[6];
                dsHoSo.push_back(hs);
            }
        }
        finHoSo.close();
    }

    // Đọc bảng đăng ký thi và Join với hồ sơ
    ifstream finDK(pathDangKy);
    if (finDK.is_open()) {
        int count = 0;
        finDK >> count;
        string line;
        getline(finDK, line);
        for (int i = 0; i < count && getline(finDK, line); ++i) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            Vector<string> parts = split(line, '|');
            if (parts.size() >= 6) {
                string sbd = parts[0];
                string maTS = parts[1];
                string maNg = parts[2];
                double t = 0.0, l = 0.0, h = 0.0;
                try {
                    t = stod(parts[3]);
                    l = stod(parts[4]);
                    h = stod(parts[5]);
                } catch (...) {}

                // Tìm hồ sơ tương ứng theo maTS (Khóa ngoại FK)
                for (int j = 0; j < dsHoSo.size(); ++j) {
                    if (dsHoSo[j].maTS == maTS) {
                        double ut = getDiemUuTienTheoKhuVuc(dsHoSo[j].maKV);
                        ThiSinh ts(dsHoSo[j].maTS, dsHoSo[j].cmnd, dsHoSo[j].hoTen,
                                   dsHoSo[j].ngaySinh, dsHoSo[j].queQuan, dsHoSo[j].gioiTinh,
                                   sbd, dsHoSo[j].maKV, maNg, t, l, h, ut);
                        dsThiSinh.push_back(ts);
                        break;
                    }
                }
            }
        }
        finDK.close();
    }

    return true;
}

// Lưu dữ liệu vào 4 tệp CSDL quan hệ và cập nhật file legacy
bool QuanLyThiSinh::luuTatCaDuLieu() const {
    // 1. Lưu Bảng KhuVuc
    ofstream fKV(pathKhuVuc);
    if (fKV.is_open()) {
        fKV << dsKhuVuc.size() << "\n";
        for (int i = 0; i < dsKhuVuc.size(); ++i) {
            fKV << dsKhuVuc[i].getMaKhuVuc() << "|"
                << dsKhuVuc[i].getTenKhuVuc() << "|"
                << fixed << setprecision(2) << dsKhuVuc[i].getDiemUuTien() << "\n";
        }
        fKV.close();
    }

    // 2. Lưu Bảng NganhXetTuyen
    ofstream fNg(pathNganh);
    if (fNg.is_open()) {
        fNg << dsNganh.size() << "\n";
        for (int i = 0; i < dsNganh.size(); ++i) {
            fNg << dsNganh[i].getMaNganh() << "|"
                << dsNganh[i].getTenNganh() << "|"
                << dsNganh[i].getToHopMon() << "|"
                << dsNganh[i].getChiTieu() << "|"
                << fixed << setprecision(2) << dsNganh[i].getDiemChuan() << "\n";
        }
        fNg.close();
    }

    // 3. Lưu Bảng HoSoThiSinh
    ofstream fHS(pathHoSo);
    if (fHS.is_open()) {
        fHS << dsThiSinh.size() << "\n";
        for (int i = 0; i < dsThiSinh.size(); ++i) {
            const ThiSinh& ts = dsThiSinh[i];
            fHS << ts.getMaThiSinh() << "|"
                << ts.getSoCMND() << "|"
                << ts.getHoTen() << "|"
                << ts.getNgaySinh() << "|"
                << ts.getQueQuan() << "|"
                << ts.getGioiTinh() << "|"
                << ts.getMaKhuVuc() << "\n";
        }
        fHS.close();
    }

    // 4. Lưu Bảng DangKyThi
    ofstream fDK(pathDangKy);
    if (fDK.is_open()) {
        fDK << dsThiSinh.size() << "\n";
        for (int i = 0; i < dsThiSinh.size(); ++i) {
            const ThiSinh& ts = dsThiSinh[i];
            fDK << ts.getSoBaoDanh() << "|"
                << ts.getMaThiSinh() << "|"
                << ts.getMaNganh() << "|"
                << fixed << setprecision(2) << ts.getDiemToan() << "|"
                << ts.getDiemLy() << "|"
                << ts.getDiemHoa() << "\n";
        }
        fDK.close();
    }

    // 5. Lưu file legacy thisinh.txt để bảo toàn tính tương thích
    ofstream fLeg(pathLegacy);
    if (fLeg.is_open()) {
        fLeg << dsThiSinh.size() << "\n";
        for (int i = 0; i < dsThiSinh.size(); ++i) {
            const ThiSinh& ts = dsThiSinh[i];
            fLeg << ts.getSoCMND() << "\n"
                 << ts.getHoTen() << "\n"
                 << ts.getNgaySinh() << "\n"
                 << ts.getQueQuan() << "\n"
                 << ts.getGioiTinh() << "\n"
                 << ts.getSoBaoDanh() << "\n"
                 << fixed << setprecision(2)
                 << ts.getDiemToan() << " " << ts.getDiemLy() << " " 
                 << ts.getDiemHoa() << " " << ts.getDiemUuTien() << "\n";
        }
        fLeg.close();
    }

    return true;
}

// Xuất báo cáo xét tuyển đại học
bool QuanLyThiSinh::xuatBaoCaoXetTuyen(const string& path) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "===================================================================================================================\n";
    fout << "                                  TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG\n";
    fout << "                                       BAO CAO KET QUA XET TUYEN DAI HOC\n";
    fout << "                                   (He thong CSDL Quan he chuan 3NF - PBL2)\n";
    fout << "===================================================================================================================\n\n";

    fout << left << setw(4) << "STT"
         << setw(8) << "Ma TS"
         << setw(9) << "SBD"
         << setw(14) << "CMND/CCCD"
         << setw(22) << "Ho va Ten"
         << setw(12) << "Ngay Sinh"
         << setw(14) << "Que Quan"
         << setw(5) << "KV"
         << setw(28) << "Nganh Dang Ky"
         << right
         << setw(6) << "Toan"
         << setw(6) << "Ly"
         << setw(6) << "Hoa"
         << setw(6) << "UT"
         << setw(8) << "Tong"
         << setw(8) << "D.Chuan"
         << "   " << left << setw(14) << "Ket Qua" << "\n";
    fout << string(160, '-') << "\n";

    int soTrungTuyen = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        double dcNganh = getDiemChuanNganh(ts.getMaNganh());
        bool trungTuyen = ts.xetTrungTuyen(dcNganh);
        if (trungTuyen) soTrungTuyen++;

        string tenNg = getTenNganh(ts.getMaNganh());
        if (tenNg.length() > 26) tenNg = tenNg.substr(0, 23) + "...";

        fout << left << setw(4) << (i + 1)
             << setw(8) << ts.getMaThiSinh()
             << setw(9) << ts.getSoBaoDanh()
             << setw(14) << ts.getSoCMND()
             << setw(22) << ts.getHoTen().substr(0, 20)
             << setw(12) << ts.getNgaySinh()
             << setw(14) << ts.getQueQuan().substr(0, 12)
             << setw(5) << ts.getMaKhuVuc()
             << setw(28) << tenNg
             << right << fixed << setprecision(2)
             << setw(6) << ts.getDiemToan()
             << setw(6) << ts.getDiemLy()
             << setw(6) << ts.getDiemHoa()
             << setw(6) << ts.getDiemUuTien()
             << setw(8) << ts.tinhTongDiem()
             << setw(8) << dcNganh
             << "   " << left << setw(14) << (trungTuyen ? "TRUNG TUYEN" : "Khong dat") << "\n";
    }

    fout << string(160, '=') << "\n";
    fout << "Tong so thi sinh dang ky : " << dsThiSinh.size() << "\n";
    fout << "Tong so trung tuyen     : " << soTrungTuyen << " (" 
         << fixed << setprecision(1) << (dsThiSinh.empty() ? 0.0 : (double)soTrungTuyen * 100.0 / dsThiSinh.size()) << "%)\n\n";

    // Phân tích theo từng ngành
    fout << "--- THONG KE THEO TUNG NGANH XET TUYEN ---\n";
    for (int i = 0; i < dsNganh.size(); ++i) {
        string ma = dsNganh[i].getMaNganh();
        int dk = demSoLuongDangKyTheoNganh(ma);
        int tt = demSoLuongTrungTuyenTheoNganh(ma);
        fout << "- " << dsNganh[i].getTenNganh() << " (" << ma << "):\n";
        fout << "    + Diem chuan: " << dsNganh[i].getDiemChuan() << " | Chi tieu: " << dsNganh[i].getChiTieu() << "\n";
        fout << "    + So dang ky: " << dk << " | So trung tuyen: " << tt << "\n";
    }

    fout.close();
    return true;
}

// Tra cứu khóa ngoại
double QuanLyThiSinh::getDiemUuTienTheoKhuVuc(const string& maKV) const {
    string target = Utils::toUpper(Utils::trim(maKV));
    for (int i = 0; i < dsKhuVuc.size(); ++i) {
        if (Utils::toUpper(dsKhuVuc[i].getMaKhuVuc()) == target) {
            return dsKhuVuc[i].getDiemUuTien();
        }
    }
    return 0.0;
}

string QuanLyThiSinh::getTenKhuVuc(const string& maKV) const {
    string target = Utils::toUpper(Utils::trim(maKV));
    for (int i = 0; i < dsKhuVuc.size(); ++i) {
        if (Utils::toUpper(dsKhuVuc[i].getMaKhuVuc()) == target) {
            return dsKhuVuc[i].getTenKhuVuc();
        }
    }
    return maKV;
}

string QuanLyThiSinh::getTenNganh(const string& maNganh) const {
    string target = Utils::trim(maNganh);
    for (int i = 0; i < dsNganh.size(); ++i) {
        if (dsNganh[i].getMaNganh() == target) {
            return dsNganh[i].getTenNganh();
        }
    }
    return maNganh;
}

double QuanLyThiSinh::getDiemChuanNganh(const string& maNganh) const {
    string target = Utils::trim(maNganh);
    for (int i = 0; i < dsNganh.size(); ++i) {
        if (dsNganh[i].getMaNganh() == target) {
            return dsNganh[i].getDiemChuan();
        }
    }
    return 20.0;
}

int QuanLyThiSinh::timViTriKhuVuc(const string& maKV) const {
    string target = Utils::toUpper(Utils::trim(maKV));
    for (int i = 0; i < dsKhuVuc.size(); ++i) {
        if (Utils::toUpper(dsKhuVuc[i].getMaKhuVuc()) == target) return i;
    }
    return -1;
}

int QuanLyThiSinh::timViTriNganh(const string& maNganh) const {
    string target = Utils::trim(maNganh);
    for (int i = 0; i < dsNganh.size(); ++i) {
        if (dsNganh[i].getMaNganh() == target) return i;
    }
    return -1;
}

int QuanLyThiSinh::timViTriTheoSBD(const string& sbd) const {
    string target = Utils::toUpper(Utils::trim(sbd));
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getSoBaoDanh()) == target) return i;
    }
    return -1;
}

int QuanLyThiSinh::timViTriTheoMaTS(const string& maTS) const {
    string target = Utils::toUpper(Utils::trim(maTS));
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getMaThiSinh()) == target) return i;
    }
    return -1;
}

int QuanLyThiSinh::timViTriTheoCMND(const string& cmnd) const {
    string target = Utils::trim(cmnd);
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getSoCMND() == target) return i;
    }
    return -1;
}

string QuanLyThiSinh::sinhMaThiSinhMoi() const {
    int maxId = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        string m = dsThiSinh[i].getMaThiSinh();
        if (m.length() > 2 && m.substr(0, 2) == "TS") {
            try {
                int val = stoi(m.substr(2));
                if (val > maxId) maxId = val;
            } catch (...) {}
        }
    }
    int newId = maxId + 1;
    stringstream ss;
    ss << "TS" << setfill('0') << setw(3) << newId;
    return ss.str();
}

// Thêm thí sinh (Kiểm tra toàn vẹn CSDL)
bool QuanLyThiSinh::themThiSinh(const ThiSinh& ts) {
    if (timViTriTheoMaTS(ts.getMaThiSinh()) != -1) return false;
    if (timViTriTheoSBD(ts.getSoBaoDanh()) != -1) return false;
    if (timViTriTheoCMND(ts.getSoCMND()) != -1) return false;

    ThiSinh moi = ts;
    moi.setDiemUuTien(getDiemUuTienTheoKhuVuc(moi.getMaKhuVuc()));
    dsThiSinh.push_back(moi);
    luuTatCaDuLieu();
    return true;
}

// Cập nhật thông tin thí sinh
bool QuanLyThiSinh::capNhatThiSinh(const ThiSinh& ts) {
    int idx = timViTriTheoMaTS(ts.getMaThiSinh());
    if (idx == -1) {
        idx = timViTriTheoSBD(ts.getSoBaoDanh());
    }
    if (idx == -1) return false;

    // Kiểm tra trùng CMND với thí sinh khác
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (i != idx && dsThiSinh[i].getSoCMND() == ts.getSoCMND()) {
            return false;
        }
    }

    ThiSinh moi = ts;
    moi.setDiemUuTien(getDiemUuTienTheoKhuVuc(moi.getMaKhuVuc()));
    dsThiSinh[idx] = moi;
    luuTatCaDuLieu();
    return true;
}

// Xóa thí sinh
bool QuanLyThiSinh::xoaThiSinh(const string& maTSHoacSBD) {
    int idx = timViTriTheoMaTS(maTSHoacSBD);
    if (idx == -1) {
        idx = timViTriTheoSBD(maTSHoacSBD);
    }
    if (idx == -1) return false;

    dsThiSinh.erase(idx);
    luuTatCaDuLieu();
    return true;
}

// Cập nhật điểm chuẩn ngành
bool QuanLyThiSinh::capNhatDiemChuanNganh(const string& maNganh, double diemChuanMoi) {
    int idx = timViTriNganh(maNganh);
    if (idx == -1) return false;
    dsNganh[idx].setDiemChuan(diemChuanMoi);
    luuTatCaDuLieu();
    return true;
}

// Cập nhật chỉ tiêu ngành
bool QuanLyThiSinh::capNhatChiTieuNganh(const string& maNganh, int chiTieuMoi) {
    int idx = timViTriNganh(maNganh);
    if (idx == -1) return false;
    dsNganh[idx].setChiTieu(chiTieuMoi);
    luuTatCaDuLieu();
    return true;
}

// Sắp xếp
void QuanLyThiSinh::sapXepTheoTongDiemGiamDan() {
    dsThiSinh.sort(cmpTongDiemGiam);
}

void QuanLyThiSinh::sapXepTheoSBDTangDan() {
    dsThiSinh.sort(cmpSBDTang);
}

void QuanLyThiSinh::sapXepTheoTenAZ() {
    dsThiSinh.sort(cmpTenAZ);
}

void QuanLyThiSinh::sapXepTheoNganh() {
    dsThiSinh.sort(cmpNganh);
}

// Nghiệp vụ thống kê & xét tuyển
Vector<ThiSinh> QuanLyThiSinh::getDanhSachTrungTuyen(const string& maNganh) const {
    Vector<ThiSinh> res;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (!maNganh.empty() && dsThiSinh[i].getMaNganh() != maNganh) {
            continue;
        }
        double dc = getDiemChuanNganh(dsThiSinh[i].getMaNganh());
        if (dsThiSinh[i].xetTrungTuyen(dc)) {
            res.push_back(dsThiSinh[i]);
        }
    }
    res.sort(cmpTongDiemGiam);
    return res;
}

Vector<ThiSinh> QuanLyThiSinh::getDanhSachThuKhoa() const {
    Vector<ThiSinh> res;
    if (dsThiSinh.empty()) return res;

    double maxDiem = dsThiSinh[0].tinhTongDiem();
    for (int i = 1; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() > maxDiem) {
            maxDiem = dsThiSinh[i].tinhTongDiem();
        }
    }

    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].tinhTongDiem() == maxDiem) {
            res.push_back(dsThiSinh[i]);
        }
    }
    return res;
}

ThiSinh QuanLyThiSinh::getThuKhoaCuaNganh(const string& maNganh) const {
    ThiSinh best;
    double maxDiem = -1.0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getMaNganh() == maNganh) {
            if (dsThiSinh[i].tinhTongDiem() > maxDiem) {
                maxDiem = dsThiSinh[i].tinhTongDiem();
                best = dsThiSinh[i];
            }
        }
    }
    return best;
}

void QuanLyThiSinh::tinhThongKePhoDiem(int& kem, int& tb, int& kha, int& gioi, int& xuatSac) const {
    kem = tb = kha = gioi = xuatSac = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        double tong = dsThiSinh[i].tinhTongDiem();
        if (tong >= 27.0) xuatSac++;
        else if (tong >= 24.0) gioi++;
        else if (tong >= 21.0) kha++;
        else if (tong >= 15.0) tb++;
        else kem++;
    }
}

int QuanLyThiSinh::demSoLuongDangKyTheoNganh(const string& maNganh) const {
    int cnt = 0;
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getMaNganh() == maNganh) cnt++;
    }
    return cnt;
}

int QuanLyThiSinh::demSoLuongTrungTuyenTheoNganh(const string& maNganh) const {
    int cnt = 0;
    double dc = getDiemChuanNganh(maNganh);
    for (int i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getMaNganh() == maNganh && dsThiSinh[i].xetTrungTuyen(dc)) {
            cnt++;
        }
    }
    return cnt;
}

Vector<ThiSinh>& QuanLyThiSinh::getDanhSachThiSinh() { return dsThiSinh; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSachThiSinh() const { return dsThiSinh; }
Vector<KhuVuc>& QuanLyThiSinh::getDanhSachKhuVuc() { return dsKhuVuc; }
const Vector<KhuVuc>& QuanLyThiSinh::getDanhSachKhuVuc() const { return dsKhuVuc; }
Vector<NganhXetTuyen>& QuanLyThiSinh::getDanhSachNganh() { return dsNganh; }
const Vector<NganhXetTuyen>& QuanLyThiSinh::getDanhSachNganh() const { return dsNganh; }
