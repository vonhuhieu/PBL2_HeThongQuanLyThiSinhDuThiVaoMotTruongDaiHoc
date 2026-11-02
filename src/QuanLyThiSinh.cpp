#include "../include/QuanLyThiSinh.h"
#include "../include/Utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

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

// Constructor (Khởi tạo kết nối MySQL Laragon và nạp dữ liệu từ MySQL)
QuanLyThiSinh::QuanLyThiSinh() {
    diemChuanHienTai = AdmissionConst::DIEM_CHUAN_MAC_DINH;
    diemLietHienTai = AdmissionConst::DIEM_LIET;
    namTuyenSinh = "2026";
    tenTruong = "TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG";

    // Kết nối tới CSDL MySQL Laragon
    if (ketNoiMySQL()) {
        docDuLieuTuMySQL();
    }
}

// Destructor
QuanLyThiSinh::~QuanLyThiSinh() {
    ngatKetNoiMySQL();
}

// ==============================================================================
// 1. QUẢN LÝ KẾT NỐI VÀ TRUY VẤN CSDL MYSQL
// ==============================================================================

bool QuanLyThiSinh::ketNoiMySQL() {
    if (db.connect()) {
        db.initSchemaAndTables();
        db.seedDefaultData();
        return true;
    }
    return false;
}

void QuanLyThiSinh::ngatKetNoiMySQL() {
    db.disconnect();
}

bool QuanLyThiSinh::isMySQLConnected() const {
    return db.isConnected();
}

MySQLConnector& QuanLyThiSinh::getDB() {
    return db;
}

const MySQLConnector& QuanLyThiSinh::getDB() const {
    return db;
}

bool QuanLyThiSinh::docDuLieuTuMySQL() {
    if (!db.isConnected()) return false;

    // 1. Đọc Danh mục Tỉnh / Thành phố từ bảng `tinh_thanh`
    MYSQL_RES* resTT = db.query("SELECT `ma_tinh`, `ten_tinh`, `loai` FROM `tinh_thanh` ORDER BY `ma_tinh` ASC;");
    if (resTT) {
        MYSQL_ROW row;
        dsTinhThanh.clear();
        while ((row = mysql_fetch_row(resTT))) {
            string ma = row[0] ? row[0] : "";
            string ten = row[1] ? row[1] : "";
            string loai = row[2] ? row[2] : "Tinh";
            dsTinhThanh.push_back(TinhThanh(ma, ten, loai));
        }
        db.freeResult(resTT);
    }

    // 2. Đọc Danh mục Giới tính từ bảng `gioi_tinh`
    MYSQL_RES* resGT = db.query("SELECT `ma_gioi_tinh`, `ten_gioi_tinh` FROM `gioi_tinh` ORDER BY `ma_gioi_tinh` ASC;");
    if (resGT) {
        MYSQL_ROW row;
        dsGioiTinh.clear();
        while ((row = mysql_fetch_row(resGT))) {
            string ma = row[0] ? row[0] : "";
            string ten = row[1] ? row[1] : "";
            dsGioiTinh.push_back(GioiTinh(ma, ten));
        }
        db.freeResult(resGT);
    }

    // 3. Đọc Cấu hình hệ thống từ bảng `cau_hinh`
    MYSQL_RES* resCH = db.query("SELECT `khoa`, `gia_tri` FROM `cau_hinh`;");
    if (resCH) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(resCH))) {
            string k = row[0] ? row[0] : "";
            string v = row[1] ? row[1] : "";
            if (k == "truong") tenTruong = v;
            else if (k == "nam_tuyen_sinh") namTuyenSinh = v;
            else if (k == "diem_chuan") diemChuanHienTai = Utils::toDouble(v, 23.0);
            else if (k == "diem_liet") diemLietHienTai = Utils::toDouble(v, 1.0);
        }
        db.freeResult(resCH);
    }

    // 4. Đọc Thí sinh từ JOIN 2 bảng `thisinh` và `nguoi`
    MYSQL_RES* resTS = db.query(
        "SELECT ts.sbd, n.so_cccd, n.ho_ten, n.ngay_sinh, n.que_quan, n.gioi_tinh, "
        "       ts.diem_toan, ts.diem_ly, ts.diem_hoa "
        "FROM `thisinh` ts "
        "JOIN `nguoi` n ON ts.so_cccd = n.so_cccd "
        "ORDER BY ts.sbd ASC;"
    );
    if (resTS) {
        dsThiSinh.clear();
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(resTS))) {
            string sbd   = row[0] ? row[0] : "";
            string cccd  = row[1] ? row[1] : "";
            string ten   = row[2] ? row[2] : "";
            string ns    = row[3] ? row[3] : "";
            string que   = row[4] ? row[4] : "";
            string gt    = row[5] ? row[5] : "";
            double dToan = row[6] ? Utils::toDouble(row[6]) : 0.0;
            double dLy   = row[7] ? Utils::toDouble(row[7]) : 0.0;
            double dHoa  = row[8] ? Utils::toDouble(row[8]) : 0.0;

            dsThiSinh.push_back(ThiSinh(cccd, ten, ns, que, gt, sbd, dToan, dLy, dHoa));
        }
        db.freeResult(resTS);
    }

    return true;
}

bool QuanLyThiSinh::luuDuLieuVaoMySQL() {
    if (!db.isConnected()) return false;

    db.beginTransaction();

    // 1. Lưu Cấu hình
    stringstream ssDC, ssDL;
    ssDC << fixed << setprecision(2) << diemChuanHienTai;
    ssDL << fixed << setprecision(2) << diemLietHienTai;
    db.execute("INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES "
               "('truong', '" + db.escape(tenTruong) + "', 'Ten truong tuyen sinh'), "
               "('nam_tuyen_sinh', '" + db.escape(namTuyenSinh) + "', 'Nam tuyen sinh'), "
               "('diem_chuan', '" + ssDC.str() + "', 'Diem chuan trung tuyen'), "
               "('diem_liet', '" + ssDL.str() + "', 'Diem liet mon thi') "
               "ON DUPLICATE KEY UPDATE `gia_tri`=VALUES(`gia_tri`);");

    // 2. Lưu Người & Thí sinh
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        string maQue = layMaTinhThanh(ts.getQueQuan());
        string maGT = layMaGioiTinh(ts.getGioiTinh());

        string sqlNguoi = "INSERT INTO `nguoi` (`so_cccd`, `ho_ten`, `ngay_sinh`, `que_quan`, `gioi_tinh`) VALUES ('" +
                          db.escape(ts.getSoCCCD()) + "', '" +
                          db.escape(ts.getHoTen()) + "', '" +
                          db.escape(ts.getNgaySinh()) + "', '" +
                          db.escape(maQue) + "', '" +
                          db.escape(maGT) + "') " +
                          "ON DUPLICATE KEY UPDATE `ho_ten`=VALUES(`ho_ten`), `ngay_sinh`=VALUES(`ngay_sinh`), "
                          "`que_quan`=VALUES(`que_quan`), `gioi_tinh`=VALUES(`gioi_tinh`);";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa;
        ssToan << fixed << setprecision(2) << ts.getDiemToan();
        ssLy << fixed << setprecision(2) << ts.getDiemLy();
        ssHoa << fixed << setprecision(2) << ts.getDiemHoa();

        string sqlTS = "INSERT INTO `thisinh` (`sbd`, `so_cccd`, `diem_toan`, `diem_ly`, `diem_hoa`) VALUES ('" +
                       db.escape(ts.getSoBaoDanh()) + "', '" +
                       db.escape(ts.getSoCCCD()) + "', " +
                       ssToan.str() + ", " + ssLy.str() + ", " + ssHoa.str() + ") " +
                       "ON DUPLICATE KEY UPDATE `diem_toan`=VALUES(`diem_toan`), `diem_ly`=VALUES(`diem_ly`), `diem_hoa`=VALUES(`diem_hoa`);";
        db.execute(sqlTS);
    }

    return db.commit();
}

bool QuanLyThiSinh::khoiTaoDuLieuMauMySQL() {
    if (!db.isConnected()) return false;
    bool ok = db.seedDefaultData();
    if (ok) {
        docDuLieuTuMySQL();
    }
    return ok;
}

// ==============================================================================
// 2. TRA CỨU & SINH MÃ TỰ ĐỘNG
// ==============================================================================

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

int QuanLyThiSinh::timViTriTheoSBD(const string& sbd) const {
    string target = Utils::toUpper(Utils::trim(sbd));
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::toUpper(dsThiSinh[i].getSoBaoDanh()) == target) {
            return (int)i;
        }
    }
    return -1;
}

int QuanLyThiSinh::timViTriTheoCCCD(const string& cccd) const {
    string target = Utils::trim(cccd);
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getSoCCCD() == target) {
            return (int)i;
        }
    }
    return -1;
}

Vector<ThiSinh> QuanLyThiSinh::timTheoTen(const string& ten) const {
    Vector<ThiSinh> res;
    string target = Utils::toLower(Utils::trim(ten));
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (Utils::containsIgnoreCase(dsThiSinh[i].getHoTen(), target)) {
            res.push_back(dsThiSinh[i]);
        }
    }
    return res;
}

Vector<ThiSinh> QuanLyThiSinh::timTheoQueQuan(const string& que) const {
    Vector<ThiSinh> res;
    string target = Utils::toLower(Utils::trim(que));
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        string tenQue = layTenTinhThanh(dsThiSinh[i].getQueQuan());
        if (Utils::containsIgnoreCase(dsThiSinh[i].getQueQuan(), target) ||
            Utils::containsIgnoreCase(tenQue, target)) {
            res.push_back(dsThiSinh[i]);
        }
    }
    return res;
}

string QuanLyThiSinh::layTenTinhThanh(const string& maHoacTen) const {
    string norm = Utils::trim(maHoacTen);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == norm || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(norm)) {
            return dsTinhThanh[i].ten;
        }
    }
    return norm;
}

string QuanLyThiSinh::layMaTinhThanh(const string& tenHoacMa) const {
    string norm = Utils::trim(tenHoacMa);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == norm || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(norm)) {
            return dsTinhThanh[i].ma;
        }
    }
    return norm;
}

string QuanLyThiSinh::layTenGioiTinh(const string& maHoacTen) const {
    string norm = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == norm || Utils::toUpper(dsGioiTinh[i].ten) == norm) {
            return dsGioiTinh[i].ten;
        }
    }
    return maHoacTen;
}

string QuanLyThiSinh::layMaGioiTinh(const string& tenHoacMa) const {
    string norm = Utils::toUpper(Utils::trim(tenHoacMa));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == norm || Utils::toUpper(dsGioiTinh[i].ten) == norm) {
            return dsGioiTinh[i].ma;
        }
    }
    return tenHoacMa;
}

bool QuanLyThiSinh::kiemTraHopLeQueQuan(const string& maHoacTen) const {
    string norm = Utils::trim(maHoacTen);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == norm || Utils::toLower(dsTinhThanh[i].ten) == Utils::toLower(norm)) {
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::kiemTraHopLeGioiTinh(const string& maHoacTen) const {
    string norm = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == norm || Utils::toUpper(dsGioiTinh[i].ten) == norm) {
            return true;
        }
    }
    return false;
}

// ==============================================================================
// 3. NGHIỆP VỤ CRUD THÍ SINH (TRỰC TIẾP TỚI MYSQL)
// ==============================================================================

bool QuanLyThiSinh::themThiSinh(const ThiSinh& ts) {
    if (timViTriTheoSBD(ts.getSoBaoDanh()) != -1) return false;
    if (timViTriTheoCCCD(ts.getSoCCCD()) != -1) return false;

    if (!kiemTraHopLeQueQuan(ts.getQueQuan())) return false;
    if (!kiemTraHopLeGioiTinh(ts.getGioiTinh())) return false;

    ThiSinh chuanHoa = ts;
    string maQue = layMaTinhThanh(ts.getQueQuan());
    string maGT = layMaGioiTinh(ts.getGioiTinh());
    chuanHoa.setQueQuan(maQue);
    chuanHoa.setGioiTinh(maGT);

    dsThiSinh.push_back(chuanHoa);

    if (isMySQLConnected()) {
        db.beginTransaction();
        string sqlNguoi = "INSERT INTO `nguoi` (`so_cccd`, `ho_ten`, `ngay_sinh`, `que_quan`, `gioi_tinh`) VALUES ('" +
                          db.escape(chuanHoa.getSoCCCD()) + "', '" +
                          db.escape(chuanHoa.getHoTen()) + "', '" +
                          db.escape(chuanHoa.getNgaySinh()) + "', '" +
                          db.escape(maQue) + "', '" +
                          db.escape(maGT) + "') " +
                          "ON DUPLICATE KEY UPDATE `ho_ten`=VALUES(`ho_ten`), `ngay_sinh`=VALUES(`ngay_sinh`), "
                          "`que_quan`=VALUES(`que_quan`), `gioi_tinh`=VALUES(`gioi_tinh`);";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa;
        ssToan << fixed << setprecision(2) << chuanHoa.getDiemToan();
        ssLy << fixed << setprecision(2) << chuanHoa.getDiemLy();
        ssHoa << fixed << setprecision(2) << chuanHoa.getDiemHoa();

        string sqlTS = "INSERT INTO `thisinh` (`sbd`, `so_cccd`, `diem_toan`, `diem_ly`, `diem_hoa`) VALUES ('" +
                       db.escape(chuanHoa.getSoBaoDanh()) + "', '" +
                       db.escape(chuanHoa.getSoCCCD()) + "', " +
                       ssToan.str() + ", " + ssLy.str() + ", " + ssHoa.str() + ") " +
                       "ON DUPLICATE KEY UPDATE `diem_toan`=VALUES(`diem_toan`), `diem_ly`=VALUES(`diem_ly`), `diem_hoa`=VALUES(`diem_hoa`);";
        db.execute(sqlTS);
        db.commit();
    }

    return true;
}

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

    if (!kiemTraHopLeQueQuan(ts.getQueQuan())) return false;
    if (!kiemTraHopLeGioiTinh(ts.getGioiTinh())) return false;

    ThiSinh chuanHoa = ts;
    string maQue = layMaTinhThanh(ts.getQueQuan());
    string maGT = layMaGioiTinh(ts.getGioiTinh());
    chuanHoa.setQueQuan(maQue);
    chuanHoa.setGioiTinh(maGT);

    dsThiSinh[idx] = chuanHoa;

    if (isMySQLConnected()) {
        db.beginTransaction();
        string sqlNguoi = "UPDATE `nguoi` SET `ho_ten`='" + db.escape(chuanHoa.getHoTen()) + "', " +
                          "`ngay_sinh`='" + db.escape(chuanHoa.getNgaySinh()) + "', " +
                          "`que_quan`='" + db.escape(maQue) + "', " +
                          "`gioi_tinh`='" + db.escape(maGT) + "' " +
                          "WHERE `so_cccd`='" + db.escape(chuanHoa.getSoCCCD()) + "';";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa;
        ssToan << fixed << setprecision(2) << chuanHoa.getDiemToan();
        ssLy << fixed << setprecision(2) << chuanHoa.getDiemLy();
        ssHoa << fixed << setprecision(2) << chuanHoa.getDiemHoa();

        string sqlTS = "UPDATE `thisinh` SET `diem_toan`=" + ssToan.str() + ", " +
                       "`diem_ly`=" + ssLy.str() + ", " +
                       "`diem_hoa`=" + ssHoa.str() + " " +
                       "WHERE `sbd`='" + db.escape(chuanHoa.getSoBaoDanh()) + "';";
        db.execute(sqlTS);
        db.commit();
    }

    return true;
}

bool QuanLyThiSinh::xoaThiSinh(const string& sbdHoacCCCD) {
    int idx = timViTriTheoSBD(sbdHoacCCCD);
    if (idx == -1) {
        idx = timViTriTheoCCCD(sbdHoacCCCD);
    }
    if (idx == -1) return false;

    string cccdXoa = dsThiSinh[idx].getSoCCCD();
    string sbdXoa = dsThiSinh[idx].getSoBaoDanh();
    dsThiSinh.erase(idx);

    if (isMySQLConnected()) {
        db.beginTransaction();
        // Nhờ ràng buộc Foreign Key ON DELETE CASCADE, xóa nguoi sẽ tự động xóa thisinh
        db.execute("DELETE FROM `thisinh` WHERE `sbd`='" + db.escape(sbdXoa) + "';");
        db.execute("DELETE FROM `nguoi` WHERE `so_cccd`='" + db.escape(cccdXoa) + "';");
        db.commit();
    }

    return true;
}

// ==============================================================================
// 4. NGHIỆP VỤ CRUD DANH MỤC HỆ THỐNG (TRỰC TIẾP TỚI MYSQL)
// ==============================================================================

bool QuanLyThiSinh::themTinhThanh(const TinhThanh& tt) {
    if (tt.ma.empty() || tt.ten.empty()) return false;
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == tt.ma) return false;
    }
    dsTinhThanh.push_back(tt);

    if (isMySQLConnected()) {
        string sql = "INSERT INTO `tinh_thanh` (`ma_tinh`, `ten_tinh`, `loai`) VALUES ('" +
                     db.escape(tt.ma) + "', '" + db.escape(tt.ten) + "', '" + db.escape(tt.loai) + "') " +
                     "ON DUPLICATE KEY UPDATE `ten_tinh`=VALUES(`ten_tinh`), `loai`=VALUES(`loai`);";
        db.execute(sql);
    }

    return true;
}

bool QuanLyThiSinh::capNhatTinhThanh(const TinhThanh& tt) {
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == tt.ma) {
            dsTinhThanh[i] = tt;
            if (isMySQLConnected()) {
                string sql = "UPDATE `tinh_thanh` SET `ten_tinh`='" + db.escape(tt.ten) + "', " +
                             "`loai`='" + db.escape(tt.loai) + "' WHERE `ma_tinh`='" + db.escape(tt.ma) + "';";
                db.execute(sql);
            }
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::xoaTinhThanh(const string& ma) {
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getQueQuan() == ma) return false;
    }
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].ma == ma) {
            dsTinhThanh.erase(i);
            if (isMySQLConnected()) {
                db.execute("DELETE FROM `tinh_thanh` WHERE `ma_tinh`='" + db.escape(ma) + "';");
            }
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::themGioiTinh(const GioiTinh& gt) {
    if (gt.ma.empty() || gt.ten.empty()) return false;
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == Utils::toUpper(gt.ma)) return false;
    }
    dsGioiTinh.push_back(gt);

    if (isMySQLConnected()) {
        string sql = "INSERT INTO `gioi_tinh` (`ma_gioi_tinh`, `ten_gioi_tinh`) VALUES ('" +
                     db.escape(gt.ma) + "', '" + db.escape(gt.ten) + "') " +
                     "ON DUPLICATE KEY UPDATE `ten_gioi_tinh`=VALUES(`ten_gioi_tinh`);";
        db.execute(sql);
    }

    return true;
}

bool QuanLyThiSinh::capNhatGioiTinh(const GioiTinh& gt) {
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].ma) == Utils::toUpper(gt.ma)) {
            dsGioiTinh[i] = gt;
            if (isMySQLConnected()) {
                string sql = "UPDATE `gioi_tinh` SET `ten_gioi_tinh`='" + db.escape(gt.ten) + "' " +
                             "WHERE `ma_gioi_tinh`='" + db.escape(gt.ma) + "';";
                db.execute(sql);
            }
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
            if (isMySQLConnected()) {
                db.execute("DELETE FROM `gioi_tinh` WHERE `ma_gioi_tinh`='" + db.escape(ma) + "';");
            }
            return true;
        }
    }
    return false;
}

// ==============================================================================
// 5. GIẢI THUẬT SẮP XẾP
// ==============================================================================

void QuanLyThiSinh::sapXepTheoTongDiemGiamDan() {
    dsThiSinh.sort(cmpTongDiemGiam);
}

void QuanLyThiSinh::sapXepTheoTenAZ() {
    dsThiSinh.sort(cmpTenAZ);
}

void QuanLyThiSinh::sapXepTheoSBDTangDan() {
    dsThiSinh.sort(cmpSBDTang);
}

// ==============================================================================
// 6. NGHIỆP VỤ XÉT TUYỂN & THỐNG KÊ
// ==============================================================================

double QuanLyThiSinh::getDiemChuan() const { return diemChuanHienTai; }

void QuanLyThiSinh::setDiemChuan(double dc) {
    diemChuanHienTai = dc;
    if (isMySQLConnected()) {
        stringstream ss;
        ss << fixed << setprecision(2) << dc;
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES ('diem_chuan', '" + ss.str() + "', 'Diem chuan trung tuyen') "
                   "ON DUPLICATE KEY UPDATE `gia_tri`=VALUES(`gia_tri`);");
    }
}

double QuanLyThiSinh::getDiemLiet() const { return diemLietHienTai; }

void QuanLyThiSinh::setDiemLiet(double dl) {
    diemLietHienTai = dl;
    if (isMySQLConnected()) {
        stringstream ss;
        ss << fixed << setprecision(2) << dl;
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES ('diem_liet', '" + ss.str() + "', 'Diem liet mon thi') "
                   "ON DUPLICATE KEY UPDATE `gia_tri`=VALUES(`gia_tri`);");
    }
}

string QuanLyThiSinh::getNamTuyenSinh() const { return namTuyenSinh; }

void QuanLyThiSinh::setNamTuyenSinh(const string& nam) {
    namTuyenSinh = nam;
    if (isMySQLConnected()) {
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES ('nam_tuyen_sinh', '" + db.escape(nam) + "', 'Nam tuyen sinh') "
                   "ON DUPLICATE KEY UPDATE `gia_tri`=VALUES(`gia_tri`);");
    }
}

string QuanLyThiSinh::getTenTruong() const { return tenTruong; }

void QuanLyThiSinh::setTenTruong(const string& truong) {
    tenTruong = truong;
    if (isMySQLConnected()) {
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES ('truong', '" + db.escape(truong) + "', 'Ten truong tuyen sinh') "
                   "ON DUPLICATE KEY UPDATE `gia_tri`=VALUES(`gia_tri`);");
    }
}

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

// ==============================================================================
// 7. XUẤT CÁC BÁO CÁO DẠNG BẢNG KẺ KHUNG (RA THƯ MỤC data/)
// ==============================================================================

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

bool QuanLyThiSinh::xuatTatCaBaoCao() const {
    bool ok1 = xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt", diemChuanHienTai);
    bool ok2 = xuatBaoCaoThuKhoa("data/thukhoa.txt");
    bool ok3 = xuatBaoCaoPhoDiem("data/thongke_phodiem.txt");
    return (ok1 && ok2 && ok3);
}

// 8. Getters danh sách
Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() { return dsThiSinh; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() const { return dsThiSinh; }
Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() { return dsTinhThanh; }
const Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() const { return dsTinhThanh; }
Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() { return dsGioiTinh; }
const Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() const { return dsGioiTinh; }
