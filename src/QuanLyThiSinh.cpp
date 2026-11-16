#include "../include/QuanLyThiSinh.h"
#include "../include/Utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <ctime>

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
    namTuyenSinhHienHanh = AdmissionConst::NAM_TUYEN_SINH_MAC_DINH;
    siSoLopChuan = AdmissionConst::SI_SO_LOP_CHUAN;
    diemChuanHienTai = AdmissionConst::DIEM_CHUAN_MAC_DINH;
    diemLietHienTai = AdmissionConst::DIEM_LIET;
    diemSanMacDinh = AdmissionConst::DIEM_SAN_MAC_DINH;
    tenTruong = AdmissionConst::TEN_TRUONG_MAC_DINH;

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

    // 1. Đọc Danh mục Tỉnh / Thành phố từ bảng `tinh_thanh` (maTinh, tenTinh, loai)
    MYSQL_RES* resTT = db.query("SELECT `maTinh`, `tenTinh`, `loai` FROM `tinh_thanh` ORDER BY `maTinh` ASC;");
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

    // 2. Đọc Danh mục Giới tính từ bảng `gioi_tinh` (maGioiTinh, tenGioiTinh)
    MYSQL_RES* resGT = db.query("SELECT `maGioiTinh`, `tenGioiTinh` FROM `gioi_tinh` ORDER BY `maGioiTinh` ASC;");
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

    // 3. Đọc Khoa từ bảng `khoa` (maKhoa, tenKhoa, maKhoaTheoChuanDUT)
    MYSQL_RES* resKhoa = db.query("SELECT `maKhoa`, `tenKhoa`, `maKhoaTheoChuanDUT` FROM `khoa` ORDER BY `maKhoa` ASC;");
    if (resKhoa) {
        MYSQL_ROW row;
        dsKhoa.clear();
        while ((row = mysql_fetch_row(resKhoa))) {
            string mk = row[0] ? row[0] : "";
            string tk = row[1] ? row[1] : "";
            string mkDUT = row[2] ? row[2] : "";
            dsKhoa.push_back(Khoa(mk, tk, mkDUT));
        }
        db.freeResult(resKhoa);
    }

    // 4. Đọc Danh mục Ngành từ bảng `danh_muc_nganh` (maNganh, tenNganh, maKhoa, tienToLop)
    MYSQL_RES* resNganh = db.query("SELECT `maNganh`, `tenNganh`, `maKhoa`, `tienToLop` FROM `danh_muc_nganh` ORDER BY `maNganh` ASC;");
    if (resNganh) {
        MYSQL_ROW row;
        dsDanhMucNganh.clear();
        while ((row = mysql_fetch_row(resNganh))) {
            string mn = row[0] ? row[0] : "";
            string tn = row[1] ? row[1] : "";
            string mk = row[2] ? row[2] : "";
            string ttl = row[3] ? row[3] : "";
            dsDanhMucNganh.push_back(DanhMucNganh(mn, tn, mk, ttl));
        }
        db.freeResult(resNganh);
    }

    // 5. Đọc Khóa học từ bảng `khoa_hoc` (namTuyenSinh, tenKhoaHoc, trangThai)
    MYSQL_RES* resKH = db.query("SELECT `namTuyenSinh`, `tenKhoaHoc`, `trangThai` FROM `khoa_hoc` ORDER BY `namTuyenSinh` DESC;");
    if (resKH) {
        MYSQL_ROW row;
        dsKhoaHoc.clear();
        while ((row = mysql_fetch_row(resKH))) {
            int nam = row[0] ? atoi(row[0]) : AdmissionConst::NAM_TUYEN_SINH_MAC_DINH;
            string ten = row[1] ? row[1] : "";
            string tt = row[2] ? row[2] : "";
            dsKhoaHoc.push_back(KhoaHoc(nam, ten, tt));
        }
        db.freeResult(resKH);
    }

    // 6. Đọc Ngành theo khóa học từ bảng `nganh_theo_khoa_hoc`
    MYSQL_RES* resNTKH = db.query("SELECT `namTuyenSinh`, `maNganh`, `chiTieu`, `diemChuanChot`, `diemSanToiThieu` FROM `nganh_theo_khoa_hoc` ORDER BY `namTuyenSinh` DESC, `maNganh` ASC;");
    if (resNTKH) {
        MYSQL_ROW row;
        dsNganhTheoKhoaHoc.clear();
        while ((row = mysql_fetch_row(resNTKH))) {
            int nam = row[0] ? atoi(row[0]) : AdmissionConst::NAM_TUYEN_SINH_MAC_DINH;
            string mn = row[1] ? row[1] : "";
            int ct = row[2] ? atoi(row[2]) : 60;
            double dc = row[3] ? Utils::toDouble(row[3]) : 0.0;
            double ds = row[4] ? Utils::toDouble(row[4], AdmissionConst::DIEM_SAN_MAC_DINH) : AdmissionConst::DIEM_SAN_MAC_DINH;
            dsNganhTheoKhoaHoc.push_back(NganhTheoKhoaHoc(nam, mn, ct, dc, ds));
        }
        db.freeResult(resNTKH);
    }

    // 7. Đọc Cấu hình hệ thống từ bảng `cau_hinh` (khoa, giaTri, moTa)
    MYSQL_RES* resCH = db.query("SELECT `khoa`, `giaTri` FROM `cau_hinh`;");
    if (resCH) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(resCH))) {
            string k = row[0] ? row[0] : "";
            string v = row[1] ? row[1] : "";
            if (k == "tenTruong" || k == "truong") tenTruong = v;
            else if (k == "namTuyenSinhHienHanh" || k == "nam_tuyen_sinh") namTuyenSinhHienHanh = Utils::toInt(v, AdmissionConst::NAM_TUYEN_SINH_MAC_DINH);
            else if (k == "siSoLopChuan") siSoLopChuan = Utils::toInt(v, AdmissionConst::SI_SO_LOP_CHUAN);
            else if (k == "diem_chuan") diemChuanHienTai = Utils::toDouble(v, AdmissionConst::DIEM_CHUAN_MAC_DINH);
            else if (k == "diemLiet" || k == "diem_liet") diemLietHienTai = Utils::toDouble(v, AdmissionConst::DIEM_LIET);
            else if (k == "diemSanMacDinh") diemSanMacDinh = Utils::toDouble(v, AdmissionConst::DIEM_SAN_MAC_DINH);
        }
        db.freeResult(resCH);
    }

    // 8. Đọc Thí sinh từ JOIN 2 bảng `thisinh` và `nguoi` (lowerCamelCase)
    MYSQL_RES* resTS = db.query(
        "SELECT ts.soBaoDanh, n.soCCCD, n.hoTen, n.ngaySinh, n.queQuan, n.gioiTinh, "
        "       ts.namTuyenSinh, ts.diemToan, ts.diemLy, ts.diemHoa, "
        "       ts.maSinhVien, ts.lopSinhHoat, ts.nganhTrungTuyen, ts.nguyenVongTrungTuyen, "
        "       ts.ngayTao, ts.ngaySua "
        "FROM `thisinh` ts "
        "JOIN `nguoi` n ON ts.soCCCD = n.soCCCD "
        "ORDER BY ts.soBaoDanh ASC;"
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
            int nam      = row[6] ? atoi(row[6]) : AdmissionConst::NAM_TUYEN_SINH_MAC_DINH;
            double dToan = row[7] ? Utils::toDouble(row[7]) : 0.0;
            double dLy   = row[8] ? Utils::toDouble(row[8]) : 0.0;
            double dHoa  = row[9] ? Utils::toDouble(row[9]) : 0.0;
            string msv   = row[10] ? row[10] : "";
            string lsh   = row[11] ? row[11] : "";
            string ntt   = row[12] ? row[12] : "";
            int nvtt     = row[13] ? atoi(row[13]) : 0;
            string nt    = row[14] ? row[14] : "";
            string nsua  = row[15] ? row[15] : "";

            ThiSinh ts(cccd, ten, ns, que, gt, sbd, dToan, dLy, dHoa, nam);
            ts.setMaSinhVien(msv);
            ts.setLopSinhHoat(lsh);
            ts.setNganhTrungTuyen(ntt);
            ts.setNguyenVongTrungTuyen(nvtt);
            ts.setNgayTao(nt);
            ts.setNgaySua(nsua);
            dsThiSinh.push_back(ts);
        }
        db.freeResult(resTS);
    }

    // 9. Đọc Nguyện vọng từ bảng `nguyen_vong` và nạp vào từng Thí sinh
    MYSQL_RES* resNV = db.query("SELECT `id`, `soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai` FROM `nguyen_vong` ORDER BY `soBaoDanh` ASC, `thuTuNV` ASC;");
    if (resNV) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(resNV))) {
            int nvId   = row[0] ? atoi(row[0]) : 0;
            string sbd = row[1] ? row[1] : "";
            int thuTu  = row[2] ? atoi(row[2]) : 1;
            string mn  = row[3] ? row[3] : "";
            string tt  = row[4] ? row[4] : AdmissionConst::TT_NV_DANG_XET;

            int idx = timViTriTheoSBD(sbd);
            if (idx != -1) {
                dsThiSinh[idx].themNguyenVong(NguyenVong(nvId, sbd, thuTu, mn, tt));
            }
        }
        db.freeResult(resNV);
    }

    return true;
}

bool QuanLyThiSinh::luuDuLieuVaoMySQL() {
    if (!db.isConnected()) return false;

    db.beginTransaction();

    // 1. Lưu Cấu hình
    stringstream ssSS, ssNam, ssDL, ssDS;
    ssSS << siSoLopChuan;
    ssNam << namTuyenSinhHienHanh;
    ssDL << fixed << setprecision(2) << diemLietHienTai;
    ssDS << fixed << setprecision(2) << diemSanMacDinh;

    db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES "
               "('tenTruong', '" + db.escape(tenTruong) + "', 'Ten truong tuyen sinh chinh thuc'), "
               "('namTuyenSinhHienHanh', '" + ssNam.str() + "', 'Khoa tuyen sinh dai hoc mac dinh'), "
               "('siSoLopChuan', '" + ssSS.str() + "', 'Si so tieu chuan dong 1 lop DUT'), "
               "('diemLiet', '" + ssDL.str() + "', 'Muc diem liet theo quy che'), "
               "('diemSanMacDinh', '" + ssDS.str() + "', 'Diem san toi thieu nhan ho so') "
               "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");

    // 2. Lưu Người & Thí sinh
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        string maQue = layMaTinhThanh(ts.getQueQuan());
        string maGT = layMaGioiTinh(ts.getGioiTinh());

        string sqlNguoi = "INSERT INTO `nguoi` (`soCCCD`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`) VALUES ('" +
                          db.escape(ts.getSoCCCD()) + "', '" +
                          db.escape(ts.getHoTen()) + "', '" +
                          db.escape(ts.getNgaySinh()) + "', '" +
                          db.escape(maQue) + "', '" +
                          db.escape(maGT) + "') " +
                          "ON DUPLICATE KEY UPDATE `hoTen`=VALUES(`hoTen`), `ngaySinh`=VALUES(`ngaySinh`), "
                          "`queQuan`=VALUES(`queQuan`), `gioiTinh`=VALUES(`gioiTinh`);";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa, ssNamTS;
        ssToan << fixed << setprecision(2) << ts.getDiemToan();
        ssLy << fixed << setprecision(2) << ts.getDiemLy();
        ssHoa << fixed << setprecision(2) << ts.getDiemHoa();
        ssNamTS << ts.getNamTuyenSinh();

        string msvVal = ts.getMaSinhVien().empty() ? "NULL" : ("'" + db.escape(ts.getMaSinhVien()) + "'");
        string lshVal = ts.getLopSinhHoat().empty() ? "NULL" : ("'" + db.escape(ts.getLopSinhHoat()) + "'");
        string nttVal = ts.getNganhTrungTuyen().empty() ? "NULL" : ("'" + db.escape(ts.getNganhTrungTuyen()) + "'");
        string nvttVal = to_string(ts.getNguyenVongTrungTuyen());

        string sqlTS = "INSERT INTO `thisinh` (`soBaoDanh`, `soCCCD`, `namTuyenSinh`, `diemToan`, `diemLy`, `diemHoa`, "
                       "`maSinhVien`, `lopSinhHoat`, `nganhTrungTuyen`, `nguyenVongTrungTuyen`) VALUES ('" +
                       db.escape(ts.getSoBaoDanh()) + "', '" +
                       db.escape(ts.getSoCCCD()) + "', " +
                       ssNamTS.str() + ", " +
                       ssToan.str() + ", " + ssLy.str() + ", " + ssHoa.str() + ", " +
                       msvVal + ", " + lshVal + ", " + nttVal + ", " + nvttVal + ") " +
                       "ON DUPLICATE KEY UPDATE `namTuyenSinh`=VALUES(`namTuyenSinh`), `diemToan`=VALUES(`diemToan`), "
                       "`diemLy`=VALUES(`diemLy`), `diemHoa`=VALUES(`diemHoa`), `maSinhVien`=VALUES(`maSinhVien`), "
                       "`lopSinhHoat`=VALUES(`lopSinhHoat`), `nganhTrungTuyen`=VALUES(`nganhTrungTuyen`), `nguyenVongTrungTuyen`=VALUES(`nguyenVongTrungTuyen`);";
        db.execute(sqlTS);

        // Lưu Nguyện vọng của từng Thí sinh
        db.execute("DELETE FROM `nguyen_vong` WHERE `soBaoDanh`='" + db.escape(ts.getSoBaoDanh()) + "';");
        const Vector<NguyenVong>& dsNV = ts.getDsNguyenVong();
        for (size_t j = 0; j < dsNV.size(); ++j) {
            string sqlNV = "INSERT INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES ('" +
                           db.escape(ts.getSoBaoDanh()) + "', " +
                           to_string(dsNV[j].thuTuNV) + ", '" +
                           db.escape(dsNV[j].maNganh) + "', '" +
                           db.escape(dsNV[j].trangThai) + "');";
            db.execute(sqlNV);
        }
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
        if (dsTinhThanh[i].maTinh == norm || dsTinhThanh[i].ma == norm ||
            Utils::toLower(dsTinhThanh[i].tenTinh) == Utils::toLower(norm)) {
            return dsTinhThanh[i].tenTinh;
        }
    }
    return norm;
}

string QuanLyThiSinh::layMaTinhThanh(const string& tenHoacMa) const {
    string norm = Utils::trim(tenHoacMa);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].maTinh == norm || dsTinhThanh[i].ma == norm ||
            Utils::toLower(dsTinhThanh[i].tenTinh) == Utils::toLower(norm)) {
            return dsTinhThanh[i].maTinh;
        }
    }
    return norm;
}

string QuanLyThiSinh::layTenGioiTinh(const string& maHoacTen) const {
    string norm = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == norm ||
            Utils::toUpper(dsGioiTinh[i].ma) == norm ||
            Utils::toUpper(dsGioiTinh[i].tenGioiTinh) == norm) {
            return dsGioiTinh[i].tenGioiTinh;
        }
    }
    return maHoacTen;
}

string QuanLyThiSinh::layMaGioiTinh(const string& tenHoacMa) const {
    string norm = Utils::toUpper(Utils::trim(tenHoacMa));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == norm ||
            Utils::toUpper(dsGioiTinh[i].ma) == norm ||
            Utils::toUpper(dsGioiTinh[i].tenGioiTinh) == norm) {
            return dsGioiTinh[i].maGioiTinh;
        }
    }
    return tenHoacMa;
}

// Tra cứu Khoa & Ngành DUT
string QuanLyThiSinh::layTenKhoa(const string& maKhoa) const {
    for (size_t i = 0; i < dsKhoa.size(); ++i) {
        if (dsKhoa[i].maKhoa == maKhoa) return dsKhoa[i].tenKhoa;
    }
    return maKhoa;
}

string QuanLyThiSinh::layMaKhoaTheoChuanDUT(const string& maKhoa) const {
    for (size_t i = 0; i < dsKhoa.size(); ++i) {
        if (dsKhoa[i].maKhoa == maKhoa) return dsKhoa[i].maKhoaTheoChuanDUT;
    }
    return "102"; // Mặc định CNTT
}

string QuanLyThiSinh::layTenNganh(const string& maNganh) const {
    for (size_t i = 0; i < dsDanhMucNganh.size(); ++i) {
        if (dsDanhMucNganh[i].maNganh == maNganh) return dsDanhMucNganh[i].tenNganh;
    }
    return maNganh;
}

string QuanLyThiSinh::layTienToLop(const string& maNganh) const {
    for (size_t i = 0; i < dsDanhMucNganh.size(); ++i) {
        if (dsDanhMucNganh[i].maNganh == maNganh) return dsDanhMucNganh[i].tienToLop;
    }
    return "DUT";
}

string QuanLyThiSinh::layMaKhoaCuaNganh(const string& maNganh) const {
    for (size_t i = 0; i < dsDanhMucNganh.size(); ++i) {
        if (dsDanhMucNganh[i].maNganh == maNganh) return dsDanhMucNganh[i].maKhoa;
    }
    return "CNTT";
}

int QuanLyThiSinh::layChiTieuNganh(int nam, const string& maNganh) const {
    for (size_t i = 0; i < dsNganhTheoKhoaHoc.size(); ++i) {
        if (dsNganhTheoKhoaHoc[i].namTuyenSinh == nam && dsNganhTheoKhoaHoc[i].maNganh == maNganh) {
            return dsNganhTheoKhoaHoc[i].chiTieu;
        }
    }
    return 60;
}

double QuanLyThiSinh::layDiemChuanNganh(int nam, const string& maNganh) const {
    for (size_t i = 0; i < dsNganhTheoKhoaHoc.size(); ++i) {
        if (dsNganhTheoKhoaHoc[i].namTuyenSinh == nam && dsNganhTheoKhoaHoc[i].maNganh == maNganh) {
            return dsNganhTheoKhoaHoc[i].diemChuanChot;
        }
    }
    return 0.0;
}

bool QuanLyThiSinh::kiemTraHopLeQueQuan(const string& maHoacTen) const {
    string norm = Utils::trim(maHoacTen);
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].maTinh == norm || dsTinhThanh[i].ma == norm ||
            Utils::toLower(dsTinhThanh[i].tenTinh) == Utils::toLower(norm)) {
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::kiemTraHopLeGioiTinh(const string& maHoacTen) const {
    string norm = Utils::toUpper(Utils::trim(maHoacTen));
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == norm ||
            Utils::toUpper(dsGioiTinh[i].ma) == norm ||
            Utils::toUpper(dsGioiTinh[i].tenGioiTinh) == norm) {
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
    string curTime = Utils::getCurrentDateTime();
    chuanHoa.setNgayTao(curTime);
    chuanHoa.setNgaySua(curTime);

    dsThiSinh.insert(0, chuanHoa);

    if (isMySQLConnected()) {
        db.beginTransaction();
        string sqlNguoi = "INSERT INTO `nguoi` (`soCCCD`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`) VALUES ('" +
                          db.escape(chuanHoa.getSoCCCD()) + "', '" +
                          db.escape(chuanHoa.getHoTen()) + "', '" +
                          db.escape(chuanHoa.getNgaySinh()) + "', '" +
                          db.escape(maQue) + "', '" +
                          db.escape(maGT) + "') " +
                          "ON DUPLICATE KEY UPDATE `hoTen`=VALUES(`hoTen`), `ngaySinh`=VALUES(`ngaySinh`), "
                          "`queQuan`=VALUES(`queQuan`), `gioiTinh`=VALUES(`gioiTinh`);";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa, ssNam;
        ssToan << fixed << setprecision(2) << chuanHoa.getDiemToan();
        ssLy << fixed << setprecision(2) << chuanHoa.getDiemLy();
        ssHoa << fixed << setprecision(2) << chuanHoa.getDiemHoa();
        ssNam << chuanHoa.getNamTuyenSinh();

        string msvVal = chuanHoa.getMaSinhVien().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getMaSinhVien()) + "'");
        string lshVal = chuanHoa.getLopSinhHoat().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getLopSinhHoat()) + "'");
        string nttVal = chuanHoa.getNganhTrungTuyen().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getNganhTrungTuyen()) + "'");
        string nvttVal = to_string(chuanHoa.getNguyenVongTrungTuyen());

        string sqlTS = "INSERT INTO `thisinh` (`soBaoDanh`, `soCCCD`, `namTuyenSinh`, `diemToan`, `diemLy`, `diemHoa`, "
                       "`maSinhVien`, `lopSinhHoat`, `nganhTrungTuyen`, `nguyenVongTrungTuyen`, `ngayTao`, `ngaySua`) VALUES ('" +
                       db.escape(chuanHoa.getSoBaoDanh()) + "', '" +
                       db.escape(chuanHoa.getSoCCCD()) + "', " +
                       ssNam.str() + ", " +
                       ssToan.str() + ", " + ssLy.str() + ", " + ssHoa.str() + ", " +
                       msvVal + ", " + lshVal + ", " + nttVal + ", " + nvttVal + ", NOW(), NOW()) " +
                       "ON DUPLICATE KEY UPDATE `diemToan`=VALUES(`diemToan`), `diemLy`=VALUES(`diemLy`), `diemHoa`=VALUES(`diemHoa`), `ngaySua`=NOW();";
        db.execute(sqlTS);

        // Lưu Nguyện vọng
        const Vector<NguyenVong>& dsNV = chuanHoa.getDsNguyenVong();
        for (size_t j = 0; j < dsNV.size(); ++j) {
            string sqlNV = "INSERT INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES ('" +
                           db.escape(chuanHoa.getSoBaoDanh()) + "', " +
                           to_string(dsNV[j].thuTuNV) + ", '" +
                           db.escape(dsNV[j].maNganh) + "', '" +
                           db.escape(dsNV[j].trangThai) + "');";
            db.execute(sqlNV);
        }

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
    chuanHoa.setNgaySua(Utils::getCurrentDateTime());

    dsThiSinh.erase(idx);
    dsThiSinh.insert(0, chuanHoa);

    if (isMySQLConnected()) {
        db.beginTransaction();
        string sqlNguoi = "UPDATE `nguoi` SET `hoTen`='" + db.escape(chuanHoa.getHoTen()) + "', " +
                          "`ngaySinh`='" + db.escape(chuanHoa.getNgaySinh()) + "', " +
                          "`queQuan`='" + db.escape(maQue) + "', " +
                          "`gioiTinh`='" + db.escape(maGT) + "' " +
                          "WHERE `soCCCD`='" + db.escape(chuanHoa.getSoCCCD()) + "';";
        db.execute(sqlNguoi);

        stringstream ssToan, ssLy, ssHoa;
        ssToan << fixed << setprecision(2) << chuanHoa.getDiemToan();
        ssLy << fixed << setprecision(2) << chuanHoa.getDiemLy();
        ssHoa << fixed << setprecision(2) << chuanHoa.getDiemHoa();

        string msvVal = chuanHoa.getMaSinhVien().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getMaSinhVien()) + "'");
        string lshVal = chuanHoa.getLopSinhHoat().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getLopSinhHoat()) + "'");
        string nttVal = chuanHoa.getNganhTrungTuyen().empty() ? "NULL" : ("'" + db.escape(chuanHoa.getNganhTrungTuyen()) + "'");

        string sqlTS = "UPDATE `thisinh` SET `diemToan`=" + ssToan.str() + ", " +
                       "`diemLy`=" + ssLy.str() + ", " +
                       "`diemHoa`=" + ssHoa.str() + ", " +
                       "`namTuyenSinh`=" + to_string(chuanHoa.getNamTuyenSinh()) + ", " +
                       "`maSinhVien`=" + msvVal + ", " +
                       "`lopSinhHoat`=" + lshVal + ", " +
                       "`nganhTrungTuyen`=" + nttVal + ", " +
                       "`nguyenVongTrungTuyen`=" + to_string(chuanHoa.getNguyenVongTrungTuyen()) + ", " +
                       "`ngaySua`=NOW() " +
                       "WHERE `soBaoDanh`='" + db.escape(chuanHoa.getSoBaoDanh()) + "';";
        db.execute(sqlTS);

        // Cập nhật Nguyện vọng
        db.execute("DELETE FROM `nguyen_vong` WHERE `soBaoDanh`='" + db.escape(chuanHoa.getSoBaoDanh()) + "';");
        const Vector<NguyenVong>& dsNV = chuanHoa.getDsNguyenVong();
        for (size_t j = 0; j < dsNV.size(); ++j) {
            string sqlNV = "INSERT INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES ('" +
                           db.escape(chuanHoa.getSoBaoDanh()) + "', " +
                           to_string(dsNV[j].thuTuNV) + ", '" +
                           db.escape(dsNV[j].maNganh) + "', '" +
                           db.escape(dsNV[j].trangThai) + "');";
            db.execute(sqlNV);
        }

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
        // Nhờ ràng buộc Foreign Key ON DELETE CASCADE, xóa nguoi sẽ tự động xóa thisinh và nguyen_vong
        db.execute("DELETE FROM `nguoi` WHERE `soCCCD`='" + db.escape(cccdXoa) + "';");
        db.commit();
    }

    return true;
}

bool QuanLyThiSinh::capNhatNguyenVongThiSinh(const string& sbd, const Vector<NguyenVong>& dsNV) {
    int idx = timViTriTheoSBD(sbd);
    if (idx == -1) return false;

    dsThiSinh[idx].setDsNguyenVong(dsNV);

    if (isMySQLConnected()) {
        db.beginTransaction();
        db.execute("DELETE FROM `nguyen_vong` WHERE `soBaoDanh`='" + db.escape(sbd) + "';");
        for (size_t i = 0; i < dsNV.size(); ++i) {
            string sql = "INSERT INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES ('" +
                         db.escape(sbd) + "', " +
                         to_string(dsNV[i].thuTuNV) + ", '" +
                         db.escape(dsNV[i].maNganh) + "', '" +
                         db.escape(dsNV[i].trangThai) + "');";
            db.execute(sql);
        }
        db.commit();
    }
    return true;
}

Vector<NguyenVong> QuanLyThiSinh::layNguyenVongCuaThiSinh(const string& sbd) const {
    int idx = timViTriTheoSBD(sbd);
    if (idx != -1) {
        return dsThiSinh[idx].getDsNguyenVong();
    }
    return Vector<NguyenVong>();
}

// ==============================================================================
// 4. NGHIỆP VỤ CRUD DANH MỤC HỆ THỐNG (TRỰC TIẾP TỚI MYSQL)
// ==============================================================================

bool QuanLyThiSinh::themTinhThanh(const TinhThanh& tt) {
    if (tt.maTinh.empty() || tt.tenTinh.empty()) return false;
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].maTinh == tt.maTinh) return false;
    }
    dsTinhThanh.insert(0, tt);

    if (isMySQLConnected()) {
        string sql = "INSERT INTO `tinh_thanh` (`maTinh`, `tenTinh`, `loai`) VALUES ('" +
                     db.escape(tt.maTinh) + "', '" + db.escape(tt.tenTinh) + "', '" + db.escape(tt.loai) + "') " +
                     "ON DUPLICATE KEY UPDATE `tenTinh`=VALUES(`tenTinh`), `loai`=VALUES(`loai`);";
        db.execute(sql);
    }

    return true;
}

bool QuanLyThiSinh::capNhatTinhThanh(const TinhThanh& tt) {
    for (size_t i = 0; i < dsTinhThanh.size(); ++i) {
        if (dsTinhThanh[i].maTinh == tt.maTinh) {
            dsTinhThanh.erase(i);
            dsTinhThanh.insert(0, tt);
            if (isMySQLConnected()) {
                string sql = "UPDATE `tinh_thanh` SET `tenTinh`='" + db.escape(tt.tenTinh) + "', " +
                             "`loai`='" + db.escape(tt.loai) + "' WHERE `maTinh`='" + db.escape(tt.maTinh) + "';";
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
        if (dsTinhThanh[i].maTinh == ma) {
            dsTinhThanh.erase(i);
            if (isMySQLConnected()) {
                db.execute("DELETE FROM `tinh_thanh` WHERE `maTinh`='" + db.escape(ma) + "';");
            }
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::themGioiTinh(const GioiTinh& gt) {
    if (gt.maGioiTinh.empty() || gt.tenGioiTinh.empty()) return false;
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == Utils::toUpper(gt.maGioiTinh)) return false;
    }
    dsGioiTinh.insert(0, gt);

    if (isMySQLConnected()) {
        string sql = "INSERT INTO `gioi_tinh` (`maGioiTinh`, `tenGioiTinh`) VALUES ('" +
                     db.escape(gt.maGioiTinh) + "', '" + db.escape(gt.tenGioiTinh) + "') " +
                     "ON DUPLICATE KEY UPDATE `tenGioiTinh`=VALUES(`tenGioiTinh`);";
        db.execute(sql);
    }

    return true;
}

bool QuanLyThiSinh::capNhatGioiTinh(const GioiTinh& gt) {
    for (size_t i = 0; i < dsGioiTinh.size(); ++i) {
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == Utils::toUpper(gt.maGioiTinh)) {
            dsGioiTinh.erase(i);
            dsGioiTinh.insert(0, gt);
            if (isMySQLConnected()) {
                string sql = "UPDATE `gioi_tinh` SET `tenGioiTinh`='" + db.escape(gt.tenGioiTinh) + "' " +
                             "WHERE `maGioiTinh`='" + db.escape(gt.maGioiTinh) + "';";
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
        if (Utils::toUpper(dsGioiTinh[i].maGioiTinh) == mUpper) {
            dsGioiTinh.erase(i);
            if (isMySQLConnected()) {
                db.execute("DELETE FROM `gioi_tinh` WHERE `maGioiTinh`='" + db.escape(ma) + "';");
            }
            return true;
        }
    }
    return false;
}

bool QuanLyThiSinh::capNhatChiTieuNganh(int nam, const string& maNganh, int chiTieuMoi) {
    for (size_t i = 0; i < dsNganhTheoKhoaHoc.size(); ++i) {
        if (dsNganhTheoKhoaHoc[i].namTuyenSinh == nam && dsNganhTheoKhoaHoc[i].maNganh == maNganh) {
            dsNganhTheoKhoaHoc[i].chiTieu = chiTieuMoi;
            if (isMySQLConnected()) {
                db.execute("UPDATE `nganh_theo_khoa_hoc` SET `chiTieu`=" + to_string(chiTieuMoi) +
                           " WHERE `namTuyenSinh`=" + to_string(nam) + " AND `maNganh`='" + db.escape(maNganh) + "';");
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
// 6. NGHIỆP VỤ TUYỂN SINH ĐA NGUYỆN VỌNG & XẾP LỚP DUT
// ==============================================================================

bool QuanLyThiSinh::chayThuatToanXetTuyenVaChotDiemChuan(int namTuyenSinh) {
    // 1. Reset toàn bộ trạng thái trúng tuyển của thí sinh thuộc năm xét tuyển này
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getNamTuyenSinh() == namTuyenSinh) {
            dsThiSinh[i].setMaSinhVien("");
            dsThiSinh[i].setLopSinhHoat("");
            dsThiSinh[i].setNganhTrungTuyen("");
            dsThiSinh[i].setNguyenVongTrungTuyen(0);
            Vector<NguyenVong>& dsNV = dsThiSinh[i].getDsNguyenVong();
            for (size_t j = 0; j < dsNV.size(); ++j) {
                dsNV[j].trangThai = AdmissionConst::TT_NV_DANG_XET;
            }
        }
    }

    // 2. Thu thập danh sách chỉ tiêu và theo dõi số lượng đã tuyển cho từng ngành
    // Struct cục bộ phục vụ thuật toán
    struct NganhQuotaTracker {
        string maNganh;
        int chiTieu;
        int daTuyen;
        double diemChuanChot;
        double diemSanToiThieu;
        Vector<int> tsIndices; // Chỉ số thí sinh trúng tuyển
    };

    Vector<NganhQuotaTracker> trackers;
    for (size_t i = 0; i < dsNganhTheoKhoaHoc.size(); ++i) {
        if (dsNganhTheoKhoaHoc[i].namTuyenSinh == namTuyenSinh) {
            NganhQuotaTracker tr;
            tr.maNganh = dsNganhTheoKhoaHoc[i].maNganh;
            tr.chiTieu = dsNganhTheoKhoaHoc[i].chiTieu;
            tr.daTuyen = 0;
            tr.diemChuanChot = 0.0;
            tr.diemSanToiThieu = dsNganhTheoKhoaHoc[i].diemSanToiThieu;
            trackers.push_back(tr);
        }
    }

    // 3. Sắp xếp danh sách thí sinh theo tổng điểm giảm dần để đảm bảo công bằng tối cao
    sapXepTheoTongDiemGiamDan();

    // 4. Lọc xét lần lượt từ Nguyện vọng 1 (NV1) -> NV2 -> NV3
    for (int round = 1; round <= AdmissionConst::MAX_NGUYEN_VONG; ++round) {
        for (size_t i = 0; i < dsThiSinh.size(); ++i) {
            ThiSinh& ts = dsThiSinh[i];
            if (ts.getNamTuyenSinh() != namTuyenSinh) continue;
            if (ts.daTrungTuyen()) continue;               // Đã trúng tuyển NV trước đó -> Bỏ qua
            if (ts.kiemTraDiemLiet()) continue;            // Bị điểm liệt -> Không xét
            if (ts.tinhTongDiem() < diemSanMacDinh) continue; // Dưới điểm sàn nhận hồ sơ -> Bỏ qua

            Vector<NguyenVong>& dsNV = ts.getDsNguyenVong();
            for (size_t j = 0; j < dsNV.size(); ++j) {
                if (dsNV[j].thuTuNV == round) {
                    string nganhDangKy = dsNV[j].maNganh;

                    // Tìm tracker tương ứng
                    for (size_t k = 0; k < trackers.size(); ++k) {
                        if (trackers[k].maNganh == nganhDangKy) {
                            if (ts.tinhTongDiem() >= trackers[k].diemSanToiThieu &&
                                trackers[k].daTuyen < trackers[k].chiTieu) {
                                // Đủ điều kiện và ngành còn chỉ tiêu -> Trúng tuyển!
                                dsNV[j].trangThai = AdmissionConst::TT_NV_TRUNG_TUYEN;
                                ts.setNganhTrungTuyen(nganhDangKy);
                                ts.setNguyenVongTrungTuyen(round);
                                trackers[k].daTuyen++;
                                trackers[k].tsIndices.push_back((int)i);
                            } else {
                                dsNV[j].trangThai = AdmissionConst::TT_NV_KHONG_TRUNG;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // 5. Đánh dấu tất cả các nguyện vọng còn lại của thí sinh chưa trúng là KHONG_TRUNG_TUYEN
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        if (dsThiSinh[i].getNamTuyenSinh() == namTuyenSinh && !dsThiSinh[i].daTrungTuyen()) {
            Vector<NguyenVong>& dsNV = dsThiSinh[i].getDsNguyenVong();
            for (size_t j = 0; j < dsNV.size(); ++j) {
                if (dsNV[j].trangThai == AdmissionConst::TT_NV_DANG_XET) {
                    dsNV[j].trangThai = AdmissionConst::TT_NV_KHONG_TRUNG;
                }
            }
        }
    }

    // 6. Xác định Điểm chuẩn chính thức (Điểm của thí sinh trúng tuyển cuối cùng)
    for (size_t k = 0; k < trackers.size(); ++k) {
        if (trackers[k].daTuyen > 0) {
            double minAdmittedScore = 30.0;
            for (size_t m = 0; m < trackers[k].tsIndices.size(); ++m) {
                int idx = trackers[k].tsIndices[m];
                double s = dsThiSinh[idx].tinhTongDiem();
                if (s < minAdmittedScore) {
                    minAdmittedScore = s;
                }
            }
            trackers[k].diemChuanChot = minAdmittedScore;
        } else {
            trackers[k].diemChuanChot = trackers[k].diemSanToiThieu;
        }

        // Cập nhật lại vào dsNganhTheoKhoaHoc
        for (size_t i = 0; i < dsNganhTheoKhoaHoc.size(); ++i) {
            if (dsNganhTheoKhoaHoc[i].namTuyenSinh == namTuyenSinh &&
                dsNganhTheoKhoaHoc[i].maNganh == trackers[k].maNganh) {
                dsNganhTheoKhoaHoc[i].diemChuanChot = trackers[k].diemChuanChot;
                break;
            }
        }
    }

    // 7. Tự động Phân bổ Lớp sinh hoạt và Cấp Mã Sinh Viên DUT 9 số
    tuDongXepLopSinhHoatDUT(namTuyenSinh);

    // 8. Đồng bộ kết quả trúng tuyển và điểm chuẩn chốt lên MySQL
    if (isMySQLConnected()) {
        db.beginTransaction();
        for (size_t k = 0; k < trackers.size(); ++k) {
            stringstream ssDC;
            ssDC << fixed << setprecision(2) << trackers[k].diemChuanChot;
            string sqlN = "UPDATE `nganh_theo_khoa_hoc` SET `diemChuanChot`=" + ssDC.str() +
                          " WHERE `namTuyenSinh`=" + to_string(namTuyenSinh) +
                          " AND `maNganh`='" + db.escape(trackers[k].maNganh) + "';";
            db.execute(sqlN);
        }
        db.commit();
        luuDuLieuVaoMySQL();
    }

    return true;
}

string QuanLyThiSinh::sinhMaSinhVienDUT(const string& maKhoaDUT, int nam, int stt) const {
    // Cấu trúc 9 số: [Mã Khoa (3 số)][Khóa (2 số)][STT (4 số)]
    // Ví dụ: 102 + 26 + 0001 -> 102260001
    stringstream ss;
    int k2So = nam % 100;
    ss << maKhoaDUT << setfill('0') << setw(2) << k2So << setfill('0') << setw(4) << stt;
    return ss.str();
}

bool QuanLyThiSinh::tuDongXepLopSinhHoatDUT(int namTuyenSinh) {
    int khoaHaiSo = namTuyenSinh % 100;
    int siSo = (siSoLopChuan > 0) ? siSoLopChuan : AdmissionConst::SI_SO_LOP_CHUAN;

    // Duyệt qua từng ngành đào tạo
    for (size_t n = 0; n < dsDanhMucNganh.size(); ++n) {
        string maNganh = dsDanhMucNganh[n].maNganh;
        string tienTo = dsDanhMucNganh[n].tienToLop;
        string maKhoa = dsDanhMucNganh[n].maKhoa;
        string maKhoaDUT = layMaKhoaTheoChuanDUT(maKhoa);

        // Thu thập sinh viên trúng tuyển ngành này
        Vector<int> admittedIndices;
        for (size_t i = 0; i < dsThiSinh.size(); ++i) {
            if (dsThiSinh[i].getNamTuyenSinh() == namTuyenSinh &&
                dsThiSinh[i].getNganhTrungTuyen() == maNganh &&
                dsThiSinh[i].daTrungTuyen()) {
                admittedIndices.push_back((int)i);
            }
        }

        // Phân lớp và sinh MSSV cho các sinh viên trúng tuyển
        for (size_t j = 0; j < admittedIndices.size(); ++j) {
            int idx = admittedIndices[j];
            int sttTrongNganh = (int)j + 1;
            int lopIndex = ((int)j / siSo) + 1;

            // Định dạng tên lớp: 26TCLC_KHDL1, 26T_DT1...
            string tenLop = to_string(khoaHaiSo) + tienTo + to_string(lopIndex);
            string msv = sinhMaSinhVienDUT(maKhoaDUT, namTuyenSinh, sttTrongNganh);

            dsThiSinh[idx].setLopSinhHoat(tenLop);
            dsThiSinh[idx].setMaSinhVien(msv);
        }
    }

    return true;
}

// ==============================================================================
// 7. GETTERS & SETTERS CẤU HÌNH
// ==============================================================================

int QuanLyThiSinh::getNamTuyenSinhHienHanh() const { return namTuyenSinhHienHanh; }

void QuanLyThiSinh::setNamTuyenSinhHienHanh(int nam) {
    namTuyenSinhHienHanh = nam;
    if (isMySQLConnected()) {
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('namTuyenSinhHienHanh', '" +
                   to_string(nam) + "', 'Khoa tuyen sinh dai hoc mac dinh') " +
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
    }
}

int QuanLyThiSinh::getSiSoLopChuan() const { return siSoLopChuan; }

void QuanLyThiSinh::setSiSoLopChuan(int siSo) {
    siSoLopChuan = siSo;
    if (isMySQLConnected()) {
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('siSoLopChuan', '" +
                   to_string(siSo) + "', 'Si so tieu chuan dong cua 1 lop sinh hoat DUT') " +
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
    }
}

double QuanLyThiSinh::getDiemChuan() const { return diemChuanHienTai; }

void QuanLyThiSinh::setDiemChuan(double dc) {
    diemChuanHienTai = dc;
    if (isMySQLConnected()) {
        stringstream ss;
        ss << fixed << setprecision(2) << dc;
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('diem_chuan', '" + ss.str() + "', 'Diem chuan trung tuyen') "
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
    }
}

double QuanLyThiSinh::getDiemLiet() const { return diemLietHienTai; }

void QuanLyThiSinh::setDiemLiet(double dl) {
    diemLietHienTai = dl;
    if (isMySQLConnected()) {
        stringstream ss;
        ss << fixed << setprecision(2) << dl;
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('diemLiet', '" + ss.str() + "', 'Muc diem liet theo quy che') "
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
    }
}

double QuanLyThiSinh::getDiemSanMacDinh() const { return diemSanMacDinh; }

void QuanLyThiSinh::setDiemSanMacDinh(double ds) {
    diemSanMacDinh = ds;
    if (isMySQLConnected()) {
        stringstream ss;
        ss << fixed << setprecision(2) << ds;
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('diemSanMacDinh', '" + ss.str() + "', 'Diem san toi thieu nhan ho so') "
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
    }
}

string QuanLyThiSinh::getTenTruong() const { return tenTruong; }

void QuanLyThiSinh::setTenTruong(const string& truong) {
    tenTruong = truong;
    if (isMySQLConnected()) {
        db.execute("INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES ('tenTruong', '" + db.escape(truong) + "', 'Ten truong tuyen sinh') "
                   "ON DUPLICATE KEY UPDATE `giaTri`=VALUES(`giaTri`);");
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
// 8. XUẤT CÁC BÁO CÁO DẠNG BẢNG KẺ KHUNG (RA THƯ MỤC data/)
// ==============================================================================

// Hàm hỗ trợ thoát ký tự đặc biệt trong XML để bảo toàn định dạng UTF-8
static string escapeXml(const string& str) {
    string res;
    res.reserve(str.size());
    for (char c : str) {
        if (c == '&') res += "&amp;";
        else if (c == '<') res += "&lt;";
        else if (c == '>') res += "&gt;";
        else if (c == '"') res += "&quot;";
        else if (c == '\'') res += "&apos;";
        else res += c;
    }
    return res;
}

bool QuanLyThiSinh::xuatBaoCaoExcel(const string& filePath, const FilterThiSinhParams& params, const string& tenKhoaFilter, const string& tenNganhFilter) const {
    // 1. Lấy danh sách đầy đủ thỏa mãn bộ lọc hiện hành (không bị giới hạn phân trang)
    FilterThiSinhParams exportParams = params;
    exportParams.pageIndex = 1;
    exportParams.pageSize = 100000; // Lấy toàn bộ bản ghi
    int totalRecs = 0;
    Vector<ThiSinh> list = const_cast<QuanLyThiSinh*>(this)->layDanhSachThiSinhPhanTrang(exportParams, totalRecs);

    // 2. Mở file xuất
    ofstream fout(filePath);
    if (!fout.is_open()) return false;

    // 3. Chuẩn bị thông tin ngày giờ xuất và mô tả bộ lọc
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    char dtBuf[64];
    strftime(dtBuf, sizeof(dtBuf), "%d/%m/%Y %H:%M:%S", ltm);

    string strKeyword = params.keyword.empty() ? "(Tat ca)" : params.keyword;
    string strNam;
    if (params.filterNam == 0) strNam = "Tat ca cac nam";
    else strNam = to_string(params.filterNam) + " (Khoa K" + to_string(params.filterNam % 100) + ")";

    string strKetQua;
    if (params.filterStatus == 1) strKetQua = "Trung tuyen";
    else if (params.filterStatus == 2) strKetQua = "Khong dat";
    else strKetQua = "Tat ca";

    string strKhoa;
    if (params.filterStatus == 2) strKhoa = "(Khong ap dung)";
    else if (!tenKhoaFilter.empty()) strKhoa = tenKhoaFilter;
    else if (!params.filterKhoa.empty()) strKhoa = layTenKhoa(params.filterKhoa);
    else strKhoa = "Tat ca khoa";

    string strNganh;
    if (params.filterStatus == 2) strNganh = "(Khong ap dung)";
    else if (!tenNganhFilter.empty()) strNganh = tenNganhFilter;
    else if (!params.filterNganh.empty()) strNganh = layTenNganh(params.filterNganh);
    else strNganh = "Tat ca nganh";

    string strLop;
    if (params.filterStatus == 2) strLop = "(Khong ap dung)";
    else if (!params.filterLop.empty()) strLop = params.filterLop;
    else strLop = "Tat ca lop";

    // 4. Ghi cấu trúc XML Spreadsheet 2003 (Chuẩn Microsoft Office UTF-8)
    fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    fout << "<?mso-application progid=\"Excel.Sheet\"?>\n";
    fout << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    fout << " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n";
    fout << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n";
    fout << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    fout << " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n";

    // Khai báo Styles định dạng
    fout << " <Styles>\n";
    fout << "  <Style ss:ID=\"Default\" ss:Name=\"Normal\">\n";
    fout << "   <Alignment ss:Vertical=\"Center\"/>\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Color=\"#000000\"/>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"TitleUni\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"15\" ss:Bold=\"1\" ss:Color=\"#1F4E79\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"TitleReport\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"12\" ss:Bold=\"1\" ss:Color=\"#333333\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"FilterInfoCenter\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Italic=\"1\" ss:Color=\"#333333\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"FilterTotalCenter\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Bold=\"1\" ss:Color=\"#1F4E79\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"HeaderCol\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Bold=\"1\" ss:Color=\"#FFFFFF\"/>\n";
    fout << "   <Interior ss:Color=\"#1F4E79\" ss:Pattern=\"Solid\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\" ss:WrapText=\"1\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#000000\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#000000\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#000000\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#000000\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataCenter\">\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataCenterBold\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Bold=\"1\" ss:Color=\"#002060\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataLeft\">\n";
    fout << "   <Alignment ss:Horizontal=\"Left\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataScore\">\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <NumberFormat ss:Format=\"0.00\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataScoreTotal\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"11\" ss:Bold=\"1\" ss:Color=\"#C00000\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <NumberFormat ss:Format=\"0.00\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataPass\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"10\" ss:Bold=\"1\" ss:Color=\"#006100\"/>\n";
    fout << "   <Interior ss:Color=\"#C6EFCE\" ss:Pattern=\"Solid\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataFail\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"10\" ss:Color=\"#9C0006\"/>\n";
    fout << "   <Interior ss:Color=\"#FFC7CE\" ss:Pattern=\"Solid\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << "  <Style ss:ID=\"DataPending\">\n";
    fout << "   <Font ss:FontName=\"Calibri\" ss:Size=\"10\" ss:Color=\"#9C6500\"/>\n";
    fout << "   <Interior ss:Color=\"#FFEB9C\" ss:Pattern=\"Solid\"/>\n";
    fout << "   <Alignment ss:Horizontal=\"Center\" ss:Vertical=\"Center\"/>\n";
    fout << "   <Borders>\n";
    fout << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "    <Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\" ss:Color=\"#D9D9D9\"/>\n";
    fout << "   </Borders>\n";
    fout << "  </Style>\n";

    fout << " </Styles>\n";

    // Khởi tạo Worksheet
    fout << " <Worksheet ss:Name=\"DanhSachThiSinh\">\n";
    fout << "  <Table>\n";

    // 16 Cột với độ rộng pixel chuẩn mực
    fout << "   <Column ss:Width=\"35\"/>\n";  // 1. STT
    fout << "   <Column ss:Width=\"75\"/>\n";  // 2. SBD
    fout << "   <Column ss:Width=\"105\"/>\n"; // 3. CCCD
    fout << "   <Column ss:Width=\"160\"/>\n"; // 4. Họ tên
    fout << "   <Column ss:Width=\"80\"/>\n";  // 5. Ngày sinh
    fout << "   <Column ss:Width=\"115\"/>\n"; // 6. Quê quán
    fout << "   <Column ss:Width=\"60\"/>\n";  // 7. Giới tính
    fout << "   <Column ss:Width=\"65\"/>\n";  // 8. Toán
    fout << "   <Column ss:Width=\"65\"/>\n";  // 9. Lý
    fout << "   <Column ss:Width=\"65\"/>\n";  // 10. Hóa
    fout << "   <Column ss:Width=\"75\"/>\n";  // 11. Tổng điểm
    fout << "   <Column ss:Width=\"140\"/>\n"; // 12. Kết quả
    fout << "   <Column ss:Width=\"150\"/>\n"; // 13. Khoa
    fout << "   <Column ss:Width=\"220\"/>\n"; // 14. Ngành trúng tuyển
    fout << "   <Column ss:Width=\"110\"/>\n"; // 15. Lớp sinh hoạt
    fout << "   <Column ss:Width=\"95\"/>\n";  // 16. Mã SV DUT

    // THÀNH PHẦN 1: TIÊU ĐỀ IN HOA VÀ IN ĐẬM (ĐÃ HOÁN ĐỔI SIZE: TRƯỜNG TO, BÁO CÁO NHỎ)
    fout << "   <Row ss:Height=\"28\">\n";
    fout << "    <Cell ss:StyleID=\"TitleUni\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << escapeXml(tenTruong) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"22\">\n";
    fout << "    <Cell ss:StyleID=\"TitleReport\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">"
         << "BAO CAO HO SO THI SINH &amp; KET QUA XET TUYEN DAI HOC</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"6\"/>\n";

    // THÀNH PHẦN 2 & 3: BỘ LỌC, NGÀY XUẤT VÀ TỔNG HỒ SƠ (MỖI THÀNH PHẦN 1 HÀNG RIÊNG VÀ CĂN GIỮA)
    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Nam tuyen sinh: " << escapeXml(strNam) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Ket qua xet tuyen: " << escapeXml(strKetQua) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Khoa dao tao: " << escapeXml(strKhoa) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Nganh trung tuyen: " << escapeXml(strNganh) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Lop sinh hoat: " << escapeXml(strLop) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Tu khoa tim kiem: " << escapeXml(strKeyword) << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"18\">\n";
    fout << "    <Cell ss:StyleID=\"FilterInfoCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Ngay xuat bao cao: " << dtBuf << "</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"22\">\n";
    fout << "    <Cell ss:StyleID=\"FilterTotalCenter\" ss:MergeAcross=\"15\"><Data ss:Type=\"String\">" 
         << "Tong so ho so phu hop: " << to_string(list.size()) << " thi sinh</Data></Cell>\n";
    fout << "   </Row>\n";

    fout << "   <Row ss:Height=\"8\"/>\n";

    // THÀNH PHẦN 4: BẢNG DỮ LIỆU DANH SÁCH (THEAD BÔI ĐẬM, CĂN GIỮA, KHÔNG CÓ CỘT THAO TÁC)
    fout << "   <Row ss:Height=\"26\">\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">STT</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">So Bao Danh</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">So CCCD</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Ho va Ten</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Ngay Sinh</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Que Quan</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Gioi Tinh</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Toan</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Ly</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Hoa</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Tong Diem</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Ket Qua</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Khoa</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Nganh Trung Tuyen</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Lop Sinh Hoat</Data></Cell>\n";
    fout << "    <Cell ss:StyleID=\"HeaderCol\"><Data ss:Type=\"String\">Ma SV DUT</Data></Cell>\n";
    fout << "   </Row>\n";

    // CÁC DÒNG DỮ LIỆU
    for (size_t i = 0; i < list.size(); ++i) {
        const ThiSinh& ts = list[i];
        fout << "   <Row ss:Height=\"20\">\n";
        
        // 1. STT
        fout << "    <Cell ss:StyleID=\"DataCenter\"><Data ss:Type=\"Number\">" << (i + 1) << "</Data></Cell>\n";
        // 2. SBD
        fout << "    <Cell ss:StyleID=\"DataCenterBold\"><Data ss:Type=\"String\">" << escapeXml(ts.getSoBaoDanh()) << "</Data></Cell>\n";
        // 3. Số CCCD (Type="String" để giữ nguyên số 0 ở đầu)
        fout << "    <Cell ss:StyleID=\"DataCenter\"><Data ss:Type=\"String\">" << escapeXml(ts.getSoCCCD()) << "</Data></Cell>\n";
        // 4. Họ và Tên
        fout << "    <Cell ss:StyleID=\"DataLeft\"><Data ss:Type=\"String\">" << escapeXml(ts.getHoTen()) << "</Data></Cell>\n";
        // 5. Ngày Sinh
        fout << "    <Cell ss:StyleID=\"DataCenter\"><Data ss:Type=\"String\">" << escapeXml(ts.getNgaySinh()) << "</Data></Cell>\n";
        // 6. Quê Quán
        fout << "    <Cell ss:StyleID=\"DataLeft\"><Data ss:Type=\"String\">" << escapeXml(layTenTinhThanh(ts.getQueQuan())) << "</Data></Cell>\n";
        // 7. Giới Tính
        fout << "    <Cell ss:StyleID=\"DataCenter\"><Data ss:Type=\"String\">" << escapeXml(layTenGioiTinh(ts.getGioiTinh())) << "</Data></Cell>\n";
        // 8. Toán
        fout << "    <Cell ss:StyleID=\"DataScore\"><Data ss:Type=\"Number\">" << fixed << setprecision(2) << ts.getDiemToan() << "</Data></Cell>\n";
        // 9. Lý
        fout << "    <Cell ss:StyleID=\"DataScore\"><Data ss:Type=\"Number\">" << fixed << setprecision(2) << ts.getDiemLy() << "</Data></Cell>\n";
        // 10. Hóa
        fout << "    <Cell ss:StyleID=\"DataScore\"><Data ss:Type=\"Number\">" << fixed << setprecision(2) << ts.getDiemHoa() << "</Data></Cell>\n";
        // 11. Tổng Điểm
        fout << "    <Cell ss:StyleID=\"DataScoreTotal\"><Data ss:Type=\"Number\">" << fixed << setprecision(2) << ts.tinhTongDiem() << "</Data></Cell>\n";
        
        // 12. Kết Quả
        string kqStyle = "DataCenter";
        string kqText = "Dang xet tuyen";
        if (ts.daTrungTuyen()) {
            kqStyle = "DataPass";
            kqText = "TRUNG TUYEN (NV" + to_string(ts.getNguyenVongTrungTuyen()) + ")";
        } else if (ts.kiemTraDiemLiet()) {
            kqStyle = "DataFail";
            kqText = "Diem liet";
        } else if (ts.tinhTongDiem() < getDiemSanMacDinh()) {
            kqStyle = "DataFail";
            kqText = "Khong dat diem san";
        } else {
            kqStyle = "DataPending";
            kqText = "Khong trung tuyen";
        }
        fout << "    <Cell ss:StyleID=\"" << kqStyle << "\"><Data ss:Type=\"String\">" << escapeXml(kqText) << "</Data></Cell>\n";

        // 13. Khoa
        string maKhoaTs = layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
        string tenKhoaTs = ts.daTrungTuyen() ? layTenKhoa(maKhoaTs) : "-";
        fout << "    <Cell ss:StyleID=\"DataLeft\"><Data ss:Type=\"String\">" << escapeXml(tenKhoaTs) << "</Data></Cell>\n";

        // 14. Ngành Trúng Tuyển
        string tenNganhTs = ts.daTrungTuyen() ? layTenNganh(ts.getNganhTrungTuyen()) : "-";
        fout << "    <Cell ss:StyleID=\"DataLeft\"><Data ss:Type=\"String\">" << escapeXml(tenNganhTs) << "</Data></Cell>\n";

        // 15. Lớp Sinh Hoạt
        string lopTs = ts.daTrungTuyen() ? ts.getLopSinhHoat() : "-";
        fout << "    <Cell ss:StyleID=\"DataCenter\"><Data ss:Type=\"String\">" << escapeXml(lopTs) << "</Data></Cell>\n";

        // 16. Mã SV DUT
        string msvTs = ts.daTrungTuyen() ? ts.getMaSinhVien() : "-";
        fout << "    <Cell ss:StyleID=\"DataCenterBold\"><Data ss:Type=\"String\">" << escapeXml(msvTs) << "</Data></Cell>\n";

        fout << "   </Row>\n";
    }

    fout << "  </Table>\n";
    fout << " </Worksheet>\n";
    fout << "</Workbook>\n";

    fout.close();

    // Đồng thời cập nhật vào tệp shortcut mới nhất data/BaoCao_DanhSach_ThiSinh_DUT_MoiNhat.xls
    ifstream src(filePath, ios::binary);
    ofstream dst("data/BaoCao_DanhSach_ThiSinh_DUT_MoiNhat.xls", ios::binary);
    if (src.is_open() && dst.is_open()) {
        dst << src.rdbuf();
    }

    return true;
}

bool QuanLyThiSinh::xuatBaoCaoXetTuyen(const string& path, double diemChuan, const string& keyword) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    string kw = Utils::trim(keyword);
    string kwLower = Utils::toLower(kw);

    fout << "                                  " << tenTruong << "\n";
    fout << "                                       BAO CAO KET QUA XET TUYEN DAI HOC\n";
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinhHienHanh << ")\n";
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
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinhHienHanh << ")\n";
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
    fout << "                                             (Nam tuyen sinh: " << namTuyenSinhHienHanh << ")\n";
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

bool QuanLyThiSinh::xuatBaoCaoTrungTuyenDUT(const string& path, int nam) const {
    ofstream fout(path);
    if (!fout.is_open()) return false;

    fout << "                                  " << tenTruong << "\n";
    fout << "                          DANH SACH TAN SINH VIEN TRUNG TUYEN CHINH THUC KHOA " << nam << "\n";
    fout << "========================================================================================================================\n";
    fout << " STT | Ma Sinh Vien |       Ho va Ten      |  Lop Sinh Hoat | Diem | NV |        Nganh Trung Tuyen       |    Khoa    |\n";
    fout << "------------------------------------------------------------------------------------------------------------------------\n";

    int stt = 0;
    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        if (ts.getNamTuyenSinh() == nam && ts.daTrungTuyen()) {
            stt++;
            string tenNganh = layTenNganh(ts.getNganhTrungTuyen());
            string maKhoa = layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
            string tenKhoa = layTenKhoa(maKhoa);

            fout << " " << right << setw(3) << stt << " | "
                 << left  << setw(12) << ts.getMaSinhVien() << " | "
                 << left  << setw(20) << ts.getHoTen().substr(0, 20) << " | "
                 << left  << setw(14) << ts.getLopSinhHoat() << " | "
                 << right << fixed << setprecision(2) << setw(4) << ts.tinhTongDiem() << " | "
                 << right << setw(2) << ts.getNguyenVongTrungTuyen() << " | "
                 << left  << setw(30) << tenNganh.substr(0, 30) << " | "
                 << left  << setw(10) << maKhoa << " |\n";
        }
    }
    fout << "========================================================================================================================\n";
    fout << "Tong so sinh vien trúng tuyển Khoa " << nam << ": " << stt << " sinh vien.\n";
    fout.close();
    return true;
}

bool QuanLyThiSinh::xuatTatCaBaoCao() const {
    bool ok1 = xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt", diemChuanHienTai);
    bool ok2 = xuatBaoCaoThuKhoa("data/thukhoa.txt");
    bool ok3 = xuatBaoCaoPhoDiem("data/thongke_phodiem.txt");
    bool ok4 = xuatBaoCaoTrungTuyenDUT("data/danhsach_trungtuyen_dut.txt", namTuyenSinhHienHanh);
    return (ok1 && ok2 && ok3 && ok4);
}

// 9. Getters danh sách dữ liệu
Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() { return dsThiSinh; }
const Vector<ThiSinh>& QuanLyThiSinh::getDanhSach() const { return dsThiSinh; }
Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() { return dsTinhThanh; }
const Vector<TinhThanh>& QuanLyThiSinh::getDanhSachTinhThanh() const { return dsTinhThanh; }
Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() { return dsGioiTinh; }
const Vector<GioiTinh>& QuanLyThiSinh::getDanhSachGioiTinh() const { return dsGioiTinh; }
Vector<Khoa>& QuanLyThiSinh::getDanhSachKhoa() { return dsKhoa; }
const Vector<Khoa>& QuanLyThiSinh::getDanhSachKhoa() const { return dsKhoa; }
Vector<DanhMucNganh>& QuanLyThiSinh::getDanhSachDanhMucNganh() { return dsDanhMucNganh; }
const Vector<DanhMucNganh>& QuanLyThiSinh::getDanhSachDanhMucNganh() const { return dsDanhMucNganh; }
Vector<KhoaHoc>& QuanLyThiSinh::getDanhSachKhoaHoc() { return dsKhoaHoc; }
const Vector<KhoaHoc>& QuanLyThiSinh::getDanhSachKhoaHoc() const { return dsKhoaHoc; }
Vector<NganhTheoKhoaHoc>& QuanLyThiSinh::getDanhSachNganhTheoKhoaHoc() { return dsNganhTheoKhoaHoc; }
const Vector<NganhTheoKhoaHoc>& QuanLyThiSinh::getDanhSachNganhTheoKhoaHoc() const { return dsNganhTheoKhoaHoc; }

// ==============================================================================
// 10. PHÂN TRANG SERVER-SIDE (SQL LIMIT/OFFSET TRỰC TIẾP TRÊN MYSQL SERVER)
// ==============================================================================
Vector<ThiSinh> QuanLyThiSinh::layDanhSachThiSinhPhanTrang(const FilterThiSinhParams& params, int& totalRecords) {
    Vector<ThiSinh> result;
    int pageIndex = params.pageIndex;
    int pageSize = params.pageSize;
    if (pageIndex < 1) pageIndex = 1;
    if (pageSize <= 0) pageSize = AdmissionConst::PAGE_SIZE_DEFAULT;

    string kw = Utils::trim(params.keyword);

    if (db.isConnected()) {
        // --- XÂY DỰNG TRUY VẤN SERVER-SIDE MYSQL ---
        string whereClause = "WHERE 1=1 ";
        if (!kw.empty()) {
            string escKw = db.escape(kw);
            whereClause += "AND (ts.soBaoDanh LIKE '%" + escKw + "%' "
                           "OR n.soCCCD LIKE '%" + escKw + "%' "
                           "OR n.hoTen LIKE '%" + escKw + "%' "
                           "OR n.queQuan LIKE '%" + escKw + "%' "
                           "OR tt.tenTinh LIKE '%" + escKw + "%') ";
        }

        // 1. Lọc theo trạng thái kết quả (1: Trúng tuyển, 2: Không đạt)
        if (params.filterStatus == 1) {
            whereClause += "AND (ts.nganhTrungTuyen IS NOT NULL AND ts.nganhTrungTuyen != '' AND ts.nguyenVongTrungTuyen > 0) ";
        } else if (params.filterStatus == 2) {
            whereClause += "AND (ts.nganhTrungTuyen IS NULL OR ts.nganhTrungTuyen = '' OR ts.nguyenVongTrungTuyen = 0) ";
        }

        // 2. Lọc theo Năm tuyển sinh
        if (params.filterNam > 0) {
            whereClause += "AND ts.namTuyenSinh = " + to_string(params.filterNam) + " ";
        }

        // 3. Lọc theo Khoa (ngành trúng tuyển thuộc khoa)
        if (!params.filterKhoa.empty()) {
            whereClause += "AND dmn.maKhoa = '" + db.escape(params.filterKhoa) + "' ";
        }

        // 4. Lọc theo Ngành
        if (!params.filterNganh.empty()) {
            whereClause += "AND ts.nganhTrungTuyen = '" + db.escape(params.filterNganh) + "' ";
        }

        // 5. Lọc theo Lớp sinh hoạt
        if (!params.filterLop.empty()) {
            whereClause += "AND ts.lopSinhHoat = '" + db.escape(params.filterLop) + "' ";
        }

        string fromJoinClause = "FROM `thisinh` ts "
                               "JOIN `nguoi` n ON ts.soCCCD = n.soCCCD "
                               "LEFT JOIN `tinh_thanh` tt ON n.queQuan = tt.maTinh "
                               "LEFT JOIN `danh_muc_nganh` dmn ON ts.nganhTrungTuyen = dmn.maNganh ";

        // Đếm tổng số bản ghi thỏa điều kiện trên Server
        string countSql = "SELECT COUNT(*) " + fromJoinClause + whereClause + ";";
        MYSQL_RES* resCount = db.query(countSql);
        totalRecords = 0;
        if (resCount) {
            MYSQL_ROW rCount = mysql_fetch_row(resCount);
            if (rCount && rCount[0]) totalRecords = atoi(rCount[0]);
            db.freeResult(resCount);
        }

        if (totalRecords == 0) return result;

        int totalPages = (totalRecords + pageSize - 1) / pageSize;
        if (pageIndex > totalPages) pageIndex = totalPages;
        int offset = (pageIndex - 1) * pageSize;

        // Xây dựng mệnh đề ORDER BY động
        string sortDir = params.sortAsc ? "ASC" : "DESC";
        string sortColName = "ts.ngaySua";
        switch (params.sortCol) {
            case 0: sortColName = "ts.ngaySua"; break;
            case 1: sortColName = "ts.soBaoDanh"; break;
            case 2: sortColName = "n.soCCCD"; break;
            case 3: sortColName = "n.hoTen"; break;
            case 4: sortColName = "n.ngaySinh"; break;
            case 5: sortColName = "n.queQuan"; break;
            case 6: sortColName = "n.gioiTinh"; break;
            case 7: sortColName = "ts.diemToan"; break;
            case 8: sortColName = "ts.diemLy"; break;
            case 9: sortColName = "ts.diemHoa"; break;
            case 10: sortColName = "(ts.diemToan + ts.diemLy + ts.diemHoa)"; break;
            case 11: sortColName = "ts.nguyenVongTrungTuyen"; break;
            case 12: sortColName = "dmn.maKhoa"; break;
            case 13: sortColName = "ts.nganhTrungTuyen"; break;
            case 14: sortColName = "ts.lopSinhHoat"; break;
            case 15: sortColName = "ts.maSinhVien"; break;
            default: sortColName = "ts.ngaySua"; break;
        }
        string orderBy;
        if (params.sortCol == 0) {
            orderBy = "ORDER BY ts.ngaySua DESC, ts.ngayTao DESC, ts.soBaoDanh ASC ";
        } else {
            orderBy = "ORDER BY " + sortColName + " " + sortDir + ", ts.soBaoDanh ASC ";
        }

        stringstream ssQuery;
        ssQuery << "SELECT ts.soBaoDanh, n.soCCCD, n.hoTen, n.ngaySinh, n.queQuan, n.gioiTinh, "
                << "       ts.namTuyenSinh, ts.diemToan, ts.diemLy, ts.diemHoa, "
                << "       ts.maSinhVien, ts.lopSinhHoat, ts.nganhTrungTuyen, ts.nguyenVongTrungTuyen, "
                << "       ts.ngayTao, ts.ngaySua "
                << fromJoinClause
                << whereClause
                << orderBy
                << "LIMIT " << pageSize << " OFFSET " << offset << ";";

        MYSQL_RES* resPage = db.query(ssQuery.str());
        if (resPage) {
            MYSQL_ROW row;
            string inSBDList = "";
            while ((row = mysql_fetch_row(resPage))) {
                string sbd   = row[0] ? row[0] : "";
                string cccd  = row[1] ? row[1] : "";
                string ten   = row[2] ? row[2] : "";
                string ns    = row[3] ? row[3] : "";
                string que   = row[4] ? row[4] : "";
                string gt    = row[5] ? row[5] : "";
                int nam      = row[6] ? atoi(row[6]) : AdmissionConst::NAM_TUYEN_SINH_MAC_DINH;
                double dToan = row[7] ? Utils::toDouble(row[7]) : 0.0;
                double dLy   = row[8] ? Utils::toDouble(row[8]) : 0.0;
                double dHoa  = row[9] ? Utils::toDouble(row[9]) : 0.0;
                string msv   = row[10] ? row[10] : "";
                string lsh   = row[11] ? row[11] : "";
                string ntt   = row[12] ? row[12] : "";
                int nvtt     = row[13] ? atoi(row[13]) : 0;
                string nt    = row[14] ? row[14] : "";
                string nsua  = row[15] ? row[15] : "";

                ThiSinh ts(cccd, ten, ns, que, gt, sbd, dToan, dLy, dHoa, nam);
                ts.setMaSinhVien(msv);
                ts.setLopSinhHoat(lsh);
                ts.setNganhTrungTuyen(ntt);
                ts.setNguyenVongTrungTuyen(nvtt);
                ts.setNgayTao(nt);
                ts.setNgaySua(nsua);
                result.push_back(ts);

                if (!inSBDList.empty()) inSBDList += ",";
                inSBDList += "'" + db.escape(sbd) + "'";
            }
            db.freeResult(resPage);

            // Nạp nguyện vọng cho các thí sinh của trang hiện tại
            if (!inSBDList.empty()) {
                string nvSql = "SELECT `id`, `soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai` FROM `nguyen_vong` "
                               "WHERE `soBaoDanh` IN (" + inSBDList + ") "
                               "ORDER BY `soBaoDanh` ASC, `thuTuNV` ASC;";
                MYSQL_RES* resNV = db.query(nvSql);
                if (resNV) {
                    MYSQL_ROW rowNV;
                    while ((rowNV = mysql_fetch_row(resNV))) {
                        int nvId   = rowNV[0] ? atoi(rowNV[0]) : 0;
                        string sbd = rowNV[1] ? rowNV[1] : "";
                        int thuTu  = rowNV[2] ? atoi(rowNV[2]) : 1;
                        string mn  = rowNV[3] ? rowNV[3] : "";
                        string tt  = rowNV[4] ? rowNV[4] : AdmissionConst::TT_NV_DANG_XET;

                        for (size_t ri = 0; ri < result.size(); ++ri) {
                            if (result[ri].getSoBaoDanh() == sbd) {
                                result[ri].themNguyenVong(NguyenVong(nvId, sbd, thuTu, mn, tt));
                                break;
                            }
                        }
                    }
                    db.freeResult(resNV);
                }
            }
        }
        return result;
    }

    // --- FALLBACK IN-MEMORY (KHI CHẠY OFFLINE HOẶC MẤT KẾT NỐI MYSQL) ---
    Vector<ThiSinh> filtered;
    string filterStr = Utils::toLower(kw);

    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        const ThiSinh& ts = dsThiSinh[i];
        string tenQue = layTenTinhThanh(ts.getQueQuan());

        if (!filterStr.empty()) {
            bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), filterStr) ||
                         Utils::containsIgnoreCase(ts.getSoCCCD(), filterStr) ||
                         Utils::containsIgnoreCase(ts.getHoTen(), filterStr) ||
                         Utils::containsIgnoreCase(ts.getQueQuan(), filterStr) ||
                         Utils::containsIgnoreCase(tenQue, filterStr);
            if (!match) continue;
        }

        bool pass = ts.daTrungTuyen();
        if (params.filterStatus == 1 && !pass) continue;
        if (params.filterStatus == 2 && pass) continue;

        if (params.filterNam > 0 && ts.getNamTuyenSinh() != params.filterNam) continue;

        if (!params.filterKhoa.empty()) {
            string mk = layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
            if (mk != params.filterKhoa) continue;
        }

        if (!params.filterNganh.empty() && ts.getNganhTrungTuyen() != params.filterNganh) continue;

        if (!params.filterLop.empty() && ts.getLopSinhHoat() != params.filterLop) continue;

        filtered.push_back(ts);
    }

    // Sắp xếp động In-Memory
    for (size_t i = 0; i + 1 < filtered.size(); ++i) {
        for (size_t j = i + 1; j < filtered.size(); ++j) {
            bool shouldSwap = false;
            switch (params.sortCol) {
                case 0: shouldSwap = (filtered[i].getNgaySua() < filtered[j].getNgaySua()); break;
                case 1: shouldSwap = params.sortAsc ? (filtered[i].getSoBaoDanh() > filtered[j].getSoBaoDanh()) : (filtered[i].getSoBaoDanh() < filtered[j].getSoBaoDanh()); break;
                case 2: shouldSwap = params.sortAsc ? (filtered[i].getSoCCCD() > filtered[j].getSoCCCD()) : (filtered[i].getSoCCCD() < filtered[j].getSoCCCD()); break;
                case 3: shouldSwap = params.sortAsc ? (filtered[i].getHoTen() > filtered[j].getHoTen()) : (filtered[i].getHoTen() < filtered[j].getHoTen()); break;
                case 4: shouldSwap = params.sortAsc ? (filtered[i].getNgaySinh() > filtered[j].getNgaySinh()) : (filtered[i].getNgaySinh() < filtered[j].getNgaySinh()); break;
                case 5: shouldSwap = params.sortAsc ? (filtered[i].getQueQuan() > filtered[j].getQueQuan()) : (filtered[i].getQueQuan() < filtered[j].getQueQuan()); break;
                case 6: shouldSwap = params.sortAsc ? (filtered[i].getGioiTinh() > filtered[j].getGioiTinh()) : (filtered[i].getGioiTinh() < filtered[j].getGioiTinh()); break;
                case 7: shouldSwap = params.sortAsc ? (filtered[i].getDiemToan() > filtered[j].getDiemToan()) : (filtered[i].getDiemToan() < filtered[j].getDiemToan()); break;
                case 8: shouldSwap = params.sortAsc ? (filtered[i].getDiemLy() > filtered[j].getDiemLy()) : (filtered[i].getDiemLy() < filtered[j].getDiemLy()); break;
                case 9: shouldSwap = params.sortAsc ? (filtered[i].getDiemHoa() > filtered[j].getDiemHoa()) : (filtered[i].getDiemHoa() < filtered[j].getDiemHoa()); break;
                case 10: shouldSwap = params.sortAsc ? (filtered[i].tinhTongDiem() > filtered[j].tinhTongDiem()) : (filtered[i].tinhTongDiem() < filtered[j].tinhTongDiem()); break;
                case 11: shouldSwap = params.sortAsc ? (filtered[i].getNguyenVongTrungTuyen() > filtered[j].getNguyenVongTrungTuyen()) : (filtered[i].getNguyenVongTrungTuyen() < filtered[j].getNguyenVongTrungTuyen()); break;
                case 12: {
                    string mk1 = layMaKhoaCuaNganh(filtered[i].getNganhTrungTuyen());
                    string mk2 = layMaKhoaCuaNganh(filtered[j].getNganhTrungTuyen());
                    shouldSwap = params.sortAsc ? (mk1 > mk2) : (mk1 < mk2);
                    break;
                }
                case 13: shouldSwap = params.sortAsc ? (filtered[i].getNganhTrungTuyen() > filtered[j].getNganhTrungTuyen()) : (filtered[i].getNganhTrungTuyen() < filtered[j].getNganhTrungTuyen()); break;
                case 14: shouldSwap = params.sortAsc ? (filtered[i].getLopSinhHoat() > filtered[j].getLopSinhHoat()) : (filtered[i].getLopSinhHoat() < filtered[j].getLopSinhHoat()); break;
                case 15: shouldSwap = params.sortAsc ? (filtered[i].getMaSinhVien() > filtered[j].getMaSinhVien()) : (filtered[i].getMaSinhVien() < filtered[j].getMaSinhVien()); break;
                default: shouldSwap = params.sortAsc ? (filtered[i].getSoBaoDanh() > filtered[j].getSoBaoDanh()) : (filtered[i].getSoBaoDanh() < filtered[j].getSoBaoDanh()); break;
            }
            if (shouldSwap) {
                ThiSinh tmp = filtered[i];
                filtered[i] = filtered[j];
                filtered[j] = tmp;
            }
        }
    }

    totalRecords = (int)filtered.size();
    if (totalRecords == 0) return result;

    int totalPages = (totalRecords + pageSize - 1) / pageSize;
    if (pageIndex > totalPages) pageIndex = totalPages;
    int offset = (pageIndex - 1) * pageSize;
    int endIdx = offset + pageSize;
    if (endIdx > totalRecords) endIdx = totalRecords;

    for (int i = offset; i < endIdx; ++i) {
        result.push_back(filtered[i]);
    }

    return result;
}

Vector<ThiSinh> QuanLyThiSinh::layDanhSachThiSinhPhanTrang(
    int pageIndex,
    int pageSize,
    const string& keyword,
    int filterStatus,
    int sortType,
    int& totalRecords
) {
    FilterThiSinhParams params;
    params.pageIndex = pageIndex;
    params.pageSize = pageSize;
    params.keyword = keyword;
    params.filterStatus = filterStatus;
    if (sortType == 1) {
        params.sortCol = 10;
        params.sortAsc = false;
    } else if (sortType == 2) {
        params.sortCol = 3;
        params.sortAsc = true;
    } else {
        params.sortCol = 1;
        params.sortAsc = true;
    }
    return layDanhSachThiSinhPhanTrang(params, totalRecords);
}

Vector<string> QuanLyThiSinh::layDanhSachLopSinhHoat(const string& maKhoa, const string& maNganh) const {
    Vector<string> dsLop;
    if (db.isConnected()) {
        string sql = "SELECT DISTINCT ts.`lopSinhHoat` FROM `thisinh` ts ";
        if (!maKhoa.empty() && maNganh.empty()) {
            sql += "JOIN `danh_muc_nganh` dmn ON ts.`nganhTrungTuyen` = dmn.`maNganh` ";
        }
        sql += "WHERE ts.`lopSinhHoat` IS NOT NULL AND ts.`lopSinhHoat` != '' ";
        if (!maNganh.empty()) {
            sql += "AND ts.`nganhTrungTuyen` = '" + const_cast<MySQLConnector&>(db).escape(maNganh) + "' ";
        } else if (!maKhoa.empty()) {
            sql += "AND dmn.`maKhoa` = '" + const_cast<MySQLConnector&>(db).escape(maKhoa) + "' ";
        }
        sql += "ORDER BY ts.`lopSinhHoat` ASC;";

        MYSQL_RES* res = const_cast<MySQLConnector&>(db).query(sql);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                if (row[0]) dsLop.push_back(string(row[0]));
            }
            const_cast<MySQLConnector&>(db).freeResult(res);
            return dsLop;
        }
    }

    for (size_t i = 0; i < dsThiSinh.size(); ++i) {
        string lop = dsThiSinh[i].getLopSinhHoat();
        if (lop.empty()) continue;
        string ntt = dsThiSinh[i].getNganhTrungTuyen();
        if (!maNganh.empty() && ntt != maNganh) continue;
        if (!maKhoa.empty()) {
            string mk = layMaKhoaCuaNganh(ntt);
            if (mk != maKhoa) continue;
        }
        bool exists = false;
        for (size_t j = 0; j < dsLop.size(); ++j) {
            if (dsLop[j] == lop) { exists = true; break; }
        }
        if (!exists) dsLop.push_back(lop);
    }
    return dsLop;
}
