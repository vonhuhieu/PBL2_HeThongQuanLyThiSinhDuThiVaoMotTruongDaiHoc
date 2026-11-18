#include "../include/MySQLConnector.h"
#include <iostream>
#include <fstream>
#include <sstream>

MySQLConnector::MySQLConnector() : conn(nullptr), connected(false) {}

MySQLConnector::~MySQLConnector() {
    disconnect();
}

void MySQLConnector::setConfig(const DBConfig& cfg) {
    config = cfg;
}

DBConfig MySQLConnector::getConfig() const {
    return config;
}

bool MySQLConnector::connect() {
    disconnect();

    conn = mysql_init(nullptr);
    if (!conn) {
        lastError = "Khong the khoi tao MySQL connection handle (mysql_init failed)";
        return false;
    }

    // Cau hinh timeout ket noi (3 giay de tranh treo UI)
    unsigned int timeout = 3;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // Thu ket noi truc tiep toi CSDL pbl2_tuyensinh
    if (mysql_real_connect(conn, config.host.c_str(), config.user.c_str(),
                           config.password.c_str(), config.database.c_str(),
                           config.port, nullptr, 0)) {
        mysql_set_character_set(conn, "utf8mb4");
        connected = true;
        lastError = "";
        initSchemaAndTables();
        return true;
    }

    // Neu that bai vi CSDL chua ton tai (Error 1049: Unknown database)
    unsigned int errNo = mysql_errno(conn);
    if (errNo == 1049) {
        mysql_close(conn);
        conn = mysql_init(nullptr);
        // Ket noi khong chi dinh database de tao moi CSDL
        if (mysql_real_connect(conn, config.host.c_str(), config.user.c_str(),
                               config.password.c_str(), nullptr,
                               config.port, nullptr, 0)) {
            std::string createDbSql = "CREATE DATABASE IF NOT EXISTS `" + config.database + 
                                     "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;";
            mysql_query(conn, createDbSql.c_str());
            mysql_select_db(conn, config.database.c_str());
            mysql_set_character_set(conn, "utf8mb4");
            connected = true;
            lastError = "";
            initSchemaAndTables();
            seedDefaultData();
            return true;
        }
    }

    lastError = mysql_error(conn);
    mysql_close(conn);
    conn = nullptr;
    connected = false;
    return false;
}

void MySQLConnector::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
    connected = false;
}

bool MySQLConnector::isConnected() const {
    return connected && (conn != nullptr);
}

std::string MySQLConnector::getLastError() const {
    return lastError;
}

bool MySQLConnector::execute(const std::string& sql) {
    if (!isConnected()) {
        lastError = "Chua ket noi den MySQL Server.";
        return false;
    }

    if (mysql_query(conn, sql.c_str()) != 0) {
        lastError = mysql_error(conn);
        return false;
    }
    return true;
}

MYSQL_RES* MySQLConnector::query(const std::string& sql) {
    if (!isConnected()) {
        lastError = "Chua ket noi den MySQL Server.";
        return nullptr;
    }

    if (mysql_query(conn, sql.c_str()) != 0) {
        lastError = mysql_error(conn);
        return nullptr;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res && mysql_field_count(conn) > 0) {
        lastError = mysql_error(conn);
        return nullptr;
    }

    return res;
}

void MySQLConnector::freeResult(MYSQL_RES* res) {
    if (res) {
        mysql_free_result(res);
    }
}

std::string MySQLConnector::escape(const std::string& input) {
    if (!isConnected() || input.empty()) {
        return input;
    }

    std::vector<char> buffer(input.length() * 2 + 1);
    unsigned long length = mysql_real_escape_string(conn, buffer.data(), input.c_str(), input.length());
    return std::string(buffer.data(), length);
}

bool MySQLConnector::beginTransaction() {
    return execute("START TRANSACTION;");
}

bool MySQLConnector::commit() {
    return execute("COMMIT;");
}

bool MySQLConnector::rollback() {
    return execute("ROLLBACK;");
}

bool MySQLConnector::initSchemaAndTables() {
    if (!isConnected()) return false;

    // Kiem tra tuong thich Schema: Neu bang thisinh co cot sbd (chuẩn cũ) thay vi soBaoDanh
    // Thi tu dong nang cap len Schema moi 10 bang chuan lowerCamelCase
    MYSQL_RES* checkRes = query("SHOW TABLES LIKE 'thisinh';");
    if (checkRes) {
        if (mysql_num_rows(checkRes) > 0) {
            MYSQL_RES* colRes = query("SHOW COLUMNS FROM `thisinh` LIKE 'soBaoDanh';");
            if (colRes) {
                if (mysql_num_rows(colRes) == 0) {
                    // Phat hien bang cu snake_case -> Drop va tao lai
                    execute("DROP TABLE IF EXISTS `nguyen_vong`;");
                    execute("DROP TABLE IF EXISTS `thisinh`;");
                    execute("DROP TABLE IF EXISTS `nguoi`;");
                    execute("DROP TABLE IF EXISTS `nganh_theo_khoa_hoc`;");
                    execute("DROP TABLE IF EXISTS `danh_muc_nganh`;");
                    execute("DROP TABLE IF EXISTS `khoa`;");
                    execute("DROP TABLE IF EXISTS `khoa_hoc`;");
                    execute("DROP TABLE IF EXISTS `tinh_thanh`;");
                    execute("DROP TABLE IF EXISTS `gioi_tinh`;");
                    execute("DROP TABLE IF EXISTS `cau_hinh`;");
                }
                freeResult(colRes);
            }
        }
        freeResult(checkRes);
    }

    const char* queries[] = {
        // 1. Tinh thanh
        "CREATE TABLE IF NOT EXISTS `tinh_thanh` ("
        "  `maTinh` VARCHAR(10) NOT NULL,"
        "  `tenTinh` VARCHAR(100) NOT NULL,"
        "  `loai` VARCHAR(50) DEFAULT 'Tinh',"
        "  PRIMARY KEY (`maTinh`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 2. Gioi tinh
        "CREATE TABLE IF NOT EXISTS `gioi_tinh` ("
        "  `maGioiTinh` VARCHAR(10) NOT NULL,"
        "  `tenGioiTinh` VARCHAR(30) NOT NULL,"
        "  PRIMARY KEY (`maGioiTinh`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 3. Khoa hoc
        "CREATE TABLE IF NOT EXISTS `khoa_hoc` ("
        "  `namTuyenSinh` INT NOT NULL,"
        "  `tenKhoaHoc` VARCHAR(100) NOT NULL,"
        "  `trangThai` VARCHAR(30) DEFAULT 'DANG TUYEN SINH',"
        "  PRIMARY KEY (`namTuyenSinh`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 4. Khoa dao tao
        "CREATE TABLE IF NOT EXISTS `khoa` ("
        "  `maKhoa` VARCHAR(10) NOT NULL,"
        "  `tenKhoa` VARCHAR(100) NOT NULL,"
        "  `maKhoaTheoChuanDUT` VARCHAR(10) NOT NULL,"
        "  PRIMARY KEY (`maKhoa`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 5. Danh muc Nganh (tinh)
        "CREATE TABLE IF NOT EXISTS `danh_muc_nganh` ("
        "  `maNganh` VARCHAR(20) NOT NULL,"
        "  `tenNganh` VARCHAR(150) NOT NULL,"
        "  `maKhoa` VARCHAR(10) NOT NULL,"
        "  `tienToLop` VARCHAR(30) NOT NULL,"
        "  PRIMARY KEY (`maNganh`),"
        "  CONSTRAINT `fk_nganh_khoa` FOREIGN KEY (`maKhoa`) REFERENCES `khoa` (`maKhoa`) ON UPDATE CASCADE ON DELETE RESTRICT"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 6. Nganh theo khoa hoc
        "CREATE TABLE IF NOT EXISTS `nganh_theo_khoa_hoc` ("
        "  `namTuyenSinh` INT NOT NULL,"
        "  `maNganh` VARCHAR(20) NOT NULL,"
        "  `chiTieu` INT NOT NULL DEFAULT 60,"
        "  `diemChuanChot` DOUBLE DEFAULT 0.0,"
        "  `diemSanToiThieu` DOUBLE DEFAULT 15.00,"
        "  PRIMARY KEY (`namTuyenSinh`, `maNganh`),"
        "  CONSTRAINT `fk_ntkh_khoa_hoc` FOREIGN KEY (`namTuyenSinh`) REFERENCES `khoa_hoc` (`namTuyenSinh`) ON UPDATE CASCADE ON DELETE CASCADE,"
        "  CONSTRAINT `fk_ntkh_nganh` FOREIGN KEY (`maNganh`) REFERENCES `danh_muc_nganh` (`maNganh`) ON UPDATE CASCADE ON DELETE RESTRICT"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 7. Nguoi
        "CREATE TABLE IF NOT EXISTS `nguoi` ("
        "  `soCCCD` VARCHAR(20) NOT NULL,"
        "  `hoTen` VARCHAR(100) NOT NULL,"
        "  `ngaySinh` VARCHAR(20) NOT NULL,"
        "  `queQuan` VARCHAR(10) NOT NULL,"
        "  `gioiTinh` VARCHAR(10) NOT NULL,"
        "  PRIMARY KEY (`soCCCD`),"
        "  CONSTRAINT `fk_nguoi_queQuan` FOREIGN KEY (`queQuan`) REFERENCES `tinh_thanh` (`maTinh`) ON UPDATE CASCADE ON DELETE RESTRICT,"
        "  CONSTRAINT `fk_nguoi_gioiTinh` FOREIGN KEY (`gioiTinh`) REFERENCES `gioi_tinh` (`maGioiTinh`) ON UPDATE CASCADE ON DELETE RESTRICT"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 8. Thi sinh
        "CREATE TABLE IF NOT EXISTS `thisinh` ("
        "  `soBaoDanh` VARCHAR(20) NOT NULL,"
        "  `soCCCD` VARCHAR(20) NOT NULL UNIQUE,"
        "  `namTuyenSinh` INT NOT NULL DEFAULT 2026,"
        "  `diemToan` DOUBLE NOT NULL,"
        "  `diemLy` DOUBLE NOT NULL,"
        "  `diemHoa` DOUBLE NOT NULL,"
        "  `maSinhVien` VARCHAR(20) DEFAULT NULL,"
        "  `lopSinhHoat` VARCHAR(50) DEFAULT NULL,"
        "  `nganhTrungTuyen` VARCHAR(20) DEFAULT NULL,"
        "  `nguyenVongTrungTuyen` INT DEFAULT 0,"
        "  `ngayTao` DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  `ngaySua` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "  PRIMARY KEY (`soBaoDanh`),"
        "  CONSTRAINT `fk_thisinh_nguoi` FOREIGN KEY (`soCCCD`) REFERENCES `nguoi` (`soCCCD`) ON UPDATE CASCADE ON DELETE CASCADE,"
        "  CONSTRAINT `fk_thisinh_khoa_hoc` FOREIGN KEY (`namTuyenSinh`) REFERENCES `khoa_hoc` (`namTuyenSinh`) ON UPDATE CASCADE ON DELETE RESTRICT,"
        "  CONSTRAINT `fk_thisinh_nganh` FOREIGN KEY (`nganhTrungTuyen`) REFERENCES `danh_muc_nganh` (`maNganh`) ON UPDATE CASCADE ON DELETE SET NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 9. Nguyen vong
        "CREATE TABLE IF NOT EXISTS `nguyen_vong` ("
        "  `id` INT AUTO_INCREMENT NOT NULL,"
        "  `soBaoDanh` VARCHAR(20) NOT NULL,"
        "  `thuTuNV` INT NOT NULL,"
        "  `maNganh` VARCHAR(20) NOT NULL,"
        "  `trangThai` VARCHAR(30) NOT NULL DEFAULT 'DANG XET',"
        "  PRIMARY KEY (`id`),"
        "  UNIQUE KEY `uk_thisinh_thuTuNV` (`soBaoDanh`, `thuTuNV`),"
        "  CONSTRAINT `fk_nv_thisinh` FOREIGN KEY (`soBaoDanh`) REFERENCES `thisinh` (`soBaoDanh`) ON UPDATE CASCADE ON DELETE CASCADE,"
        "  CONSTRAINT `fk_nv_nganh` FOREIGN KEY (`maNganh`) REFERENCES `danh_muc_nganh` (`maNganh`) ON UPDATE CASCADE ON DELETE RESTRICT"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        // 10. Cau hinh
        "CREATE TABLE IF NOT EXISTS `cau_hinh` ("
        "  `khoa` VARCHAR(50) NOT NULL,"
        "  `giaTri` VARCHAR(255) NOT NULL,"
        "  `moTa` VARCHAR(255) DEFAULT NULL,"
        "  PRIMARY KEY (`khoa`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;"
    };

    for (const char* q : queries) {
        if (!execute(q)) {
            return false;
        }
    }

    // Seed du lieu mac dinh neu he thong trong
    seedDefaultData();
    return true;
}

bool MySQLConnector::seedDefaultData() {
    if (!isConnected()) return false;

    // Kiem tra xem da co tinh thanh chua
    MYSQL_RES* res = query("SELECT COUNT(*) FROM `tinh_thanh`;");
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row && row[0] && atoi(row[0]) > 0) {
            freeResult(res);
            return true; // Da co du lieu
        }
        freeResult(res);
    }

    beginTransaction();

    // 1. Seed 34 Tinh/Thanh pho
    execute("INSERT IGNORE INTO `tinh_thanh` (`maTinh`, `tenTinh`, `loai`) VALUES "
            "('01', 'TP. Ha Noi', 'Thanh pho truc thuoc Trung uong'),"
            "('04', 'Cao Bang', 'Tinh'),"
            "('08', 'Tuyen Quang', 'Tinh'),"
            "('10', 'Lao Cai', 'Tinh'),"
            "('11', 'Dien Bien', 'Tinh'),"
            "('12', 'Lai Chau', 'Tinh'),"
            "('14', 'Son La', 'Tinh'),"
            "('19', 'Thai Nguyen', 'Tinh'),"
            "('20', 'Lang Son', 'Tinh'),"
            "('22', 'Quang Ninh', 'Tinh'),"
            "('25', 'Phu Tho', 'Tinh'),"
            "('27', 'Bac Ninh', 'Tinh'),"
            "('31', 'TP. Hai Phong', 'Thanh pho truc thuoc Trung uong'),"
            "('33', 'Hung Yen', 'Tinh'),"
            "('37', 'Ninh Binh', 'Tinh'),"
            "('38', 'Thanh Hoa', 'Tinh'),"
            "('40', 'Nghe An', 'Tinh'),"
            "('42', 'Ha Tinh', 'Tinh'),"
            "('45', 'Quang Tri', 'Tinh'),"
            "('46', 'TP. Hue', 'Thanh pho truc thuoc Trung uong'),"
            "('48', 'TP. Da Nang', 'Thanh pho truc thuoc Trung uong'),"
            "('51', 'Quang Ngai', 'Tinh'),"
            "('56', 'Khanh Hoa', 'Tinh'),"
            "('64', 'Gia Lai', 'Tinh'),"
            "('66', 'Dak Lak', 'Tinh'),"
            "('68', 'Lam Dong', 'Tinh'),"
            "('72', 'Tay Ninh', 'Tinh'),"
            "('75', 'Dong Nai', 'Tinh'),"
            "('79', 'TP. Ho Chi Minh', 'Thanh pho truc thuoc Trung uong'),"
            "('86', 'Vinh Long', 'Tinh'),"
            "('87', 'Dong Thap', 'Tinh'),"
            "('89', 'An Giang', 'Tinh'),"
            "('92', 'TP. Can Tho', 'Thanh pho truc thuoc Trung uong');");

    // 2. Seed Gioi Tinh
    execute("INSERT IGNORE INTO `gioi_tinh` (`maGioiTinh`, `tenGioiTinh`) VALUES "
            "('NAM', 'Nam'), ('NU', 'Nu'), ('KHAC', 'Khac');");

    // 3. Seed Khoa DUT
    execute("INSERT IGNORE INTO `khoa` (`maKhoa`, `tenKhoa`, `maKhoaTheoChuanDUT`) VALUES "
            "('CNTT', 'Khoa Cong nghe Thong tin', '102'),"
            "('DIEN', 'Khoa Dien', '105'),"
            "('DTVT', 'Khoa Dien tu - Vien thong', '106'),"
            "('CKHI', 'Khoa Co khi', '101'),"
            "('HOA',  'Khoa Hoa', '107'),"
            "('XDDD', 'Khoa Xay dung Dan dung & Cong nghiep', '109');");

    // 4. Seed Danh Muc Nganh DUT
    execute("INSERT IGNORE INTO `danh_muc_nganh` (`maNganh`, `tenNganh`, `maKhoa`, `tienToLop`) VALUES "
            "('7480201B', 'CNTT (Khoa hoc Du lieu & Tri tue Nhan tao)', 'CNTT', 'TCLC_KHDL'),"
            "('7480201',  'Cong nghe Thong tin (Chuong trinh Dac thu)',    'CNTT', 'T_DT'),"
            "('7480201A', 'Cong nghe Thong tin (Dinh huong Ngoai ngu Nhat)', 'CNTT', 'T_N'),"
            "('7480106',  'Ky thuat May tinh',                              'CNTT', 'KTMT'),"
            "('7520216',  'Ky thuat Dieu khien & Tu dong hoa',             'DIEN', 'TDH'),"
            "('7520201',  'Ky thuat Dien (He thong dien)',                  'DIEN', 'D'),"
            "('7520207A', 'Vi dien tu - Thiet ke vi mach (Ban dan)',       'DTVT', 'VM'),"
            "('7520207',  'Ky thuat Dien tu - Vien thong',                  'DTVT', 'DT'),"
            "('7520114',  'Ky thuat Co dien tu',                            'CKHI', 'CDT'),"
            "('7520130',  'Ky thuat O to',                                  'CKHI', 'OTO'),"
            "('7520301',  'Ky thuat Hoa hoc',                               'HOA',  'H'),"
            "('7580201',  'Ky thuat Xay dung',                              'XDDD', 'X');");

    // 5. Seed Khoa Hoc
    execute("INSERT IGNORE INTO `khoa_hoc` (`namTuyenSinh`, `tenKhoaHoc`, `trangThai`) VALUES "
            "(2026, 'Khoa 2026 (K26 - Dai hoc Chinh quy)', 'DANG TUYEN SINH'),"
            "(2025, 'Khoa 2025 (K25 - Dai hoc Chinh quy)', 'DA KET THUC');");

    // 6. Seed Nganh theo khoa hoc K26
    execute("INSERT IGNORE INTO `nganh_theo_khoa_hoc` (`namTuyenSinh`, `maNganh`, `chiTieu`, `diemChuanChot`, `diemSanToiThieu`) VALUES "
            "(2026, '7480201B', 60,  0.0, 15.00),"
            "(2026, '7480201',  120, 0.0, 15.00),"
            "(2026, '7480201A', 60,  0.0, 15.00),"
            "(2026, '7480106',  60,  0.0, 15.00),"
            "(2026, '7520216',  90,  0.0, 15.00),"
            "(2026, '7520201',  90,  0.0, 15.00),"
            "(2026, '7520207A', 60,  0.0, 15.00),"
            "(2026, '7520207',  90,  0.0, 15.00),"
            "(2026, '7520114',  90,  0.0, 15.00),"
            "(2026, '7520130',  90,  0.0, 15.00),"
            "(2026, '7520301',  60,  0.0, 15.00),"
            "(2026, '7580201',  60,  0.0, 15.00);");

    // 7. Seed Cau Hinh
    execute("INSERT IGNORE INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES "
            "('tenTruong', 'TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG', 'Ten truong tuyen sinh chinh thuc'),"
            "('namTuyenSinhHienHanh', '2026', 'Khoa tuyen sinh dai hoc mac dinh'),"
            "('siSoLopChuan', '30', 'Si so tieu chuan dong cua 1 lop sinh hoat DUT'),"
            "('diemLiet', '1.00', 'Muc diem liet theo quy che'),"
            "('diemSanMacDinh', '15.00', 'Diem san toi thieu nhan ho so xet tuyen');");

    // LƯU Ý: Khởi tạo ban đầu danh sách thí sinh là RỖNG (0 bản ghi).
    // Người dùng sẽ chủ động bấm nút [Import Dữ Liệu Mẫu (100)] trên giao diện khi cần nạp dữ liệu.
    return commit();
}

bool MySQLConnector::executeScript(const std::string& filePath) {
    if (!isConnected()) {
        lastError = "Chua ket noi MySQL";
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        lastError = "Khong the mo file SQL: " + filePath;
        return false;
    }

    std::string sqlFull;
    std::string line;
    while (std::getline(file, line)) {
        // Loại bỏ comment dòng --
        size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
        if (firstNonSpace != std::string::npos && line.substr(firstNonSpace, 2) == "--") {
            continue;
        }
        sqlFull += line + "\n";
    }
    file.close();

    beginTransaction();

    // Xóa sạch dữ liệu thí sinh cũ trước khi nạp để đảm bảo đúng bộ 100 thí sinh sạch sẽ
    execute("DELETE FROM `nguyen_vong`;");
    execute("DELETE FROM `thisinh`;");
    execute("DELETE FROM `nguoi`;");

    std::stringstream ss(sqlFull);
    std::string statement;
    while (std::getline(ss, statement, ';')) {
        size_t start = statement.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = statement.find_last_not_of(" \t\r\n");
        std::string queryStr = statement.substr(start, end - start + 1);

        if (queryStr.empty()) continue;

        // Bỏ qua lệnh USE database
        if (queryStr.find("USE ") == 0 || queryStr.find("use ") == 0) continue;

        if (!execute(queryStr)) {
            rollback();
            return false;
        }
    }

    return commit();
}
