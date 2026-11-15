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
        "       ts.maSinhVien, ts.lopSinhHoat, ts.nganhTrungTuyen, ts.nguyenVongTrungTuyen "
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

            ThiSinh ts(cccd, ten, ns, que, gt, sbd, dToan, dLy, dHoa, nam);
            ts.setMaSinhVien(msv);
            ts.setLopSinhHoat(lsh);
            ts.setNganhTrungTuyen(ntt);
            ts.setNguyenVongTrungTuyen(nvtt);
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

    dsThiSinh.push_back(chuanHoa);

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
                       "`maSinhVien`, `lopSinhHoat`, `nganhTrungTuyen`, `nguyenVongTrungTuyen`) VALUES ('" +
                       db.escape(chuanHoa.getSoBaoDanh()) + "', '" +
                       db.escape(chuanHoa.getSoCCCD()) + "', " +
                       ssNam.str() + ", " +
                       ssToan.str() + ", " + ssLy.str() + ", " + ssHoa.str() + ", " +
                       msvVal + ", " + lshVal + ", " + nttVal + ", " + nvttVal + ") " +
                       "ON DUPLICATE KEY UPDATE `diemToan`=VALUES(`diemToan`), `diemLy`=VALUES(`diemLy`), `diemHoa`=VALUES(`diemHoa`);";
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

    dsThiSinh[idx] = chuanHoa;

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
                       "`nguyenVongTrungTuyen`=" + to_string(chuanHoa.getNguyenVongTrungTuyen()) + " " +
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
    dsTinhThanh.push_back(tt);

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
            dsTinhThanh[i] = tt;
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
    dsGioiTinh.push_back(gt);

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
            dsGioiTinh[i] = gt;
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
