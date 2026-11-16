#include "../include/MySQLConnector.h"
#include <iostream>

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

    // 8. Seed Nguoi (36 thi sinh tieu bieu chuan DUT)
    execute("INSERT IGNORE INTO `nguoi` (`soCCCD`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`) VALUES "
            "('048003001234', 'Vo Nhu Hieu',            '04/03/2003', '48', 'NAM'),"
            "('048208001567', 'Nguyen Hoang Minh Quan', '15/01/2008', '48', 'NAM'),"
            "('048308002341', 'Tran Le Thao Vy',        '22/04/2008', '48', 'NU'),"
            "('046208003412', 'Le Nguyen Bao Nam',      '10/08/2008', '46', 'NAM'),"
            "('051208004523', 'Pham Gia Huy',           '05/03/2008', '51', 'NAM'),"
            "('040308005634', 'Hoang Thi Khanh Huyen',  '18/06/2008', '40', 'NU'),"
            "('042208006745', 'Dang Dinh Phuc',         '12/09/2008', '42', 'NAM'),"
            "('001308007856', 'Nguyen Ngoc Mai Anh',    '25/02/2008', '01', 'NU'),"
            "('045208008967', 'Bui Quang Thinh',        '30/10/2008', '45', 'NAM'),"
            "('056308009078', 'Do Thuy Duong',          '14/07/2008', '56', 'NU'),"
            "('048208010189', 'Phan Quoc Tuan',         '08/11/2008', '48', 'NAM'),"
            "('046208011290', 'Ho Thanh Phong',         '03/05/2008', '46', 'NAM'),"
            "('051308012301', 'Nguyen Thi Cam Tu',      '19/08/2008', '51', 'NU'),"
            "('038208013412', 'Vu Dinh Trong',          '27/03/2008', '38', 'NAM'),"
            "('079308014523', 'Trinh Hong Ngoc',        '11/12/2008', '79', 'NU'),"
            "('048208015634', 'Ngo Gia Khiem',          '16/09/2008', '48', 'NAM'),"
            "('064208016745', 'Duong Van Tai',          '20/02/2008', '64', 'NAM'),"
            "('066308017856', 'Bui Thi Kim Ngan',       '07/04/2008', '66', 'NU'),"
            "('048208018967', 'Ly Minh Tri',            '23/06/2008', '48', 'NAM'),"
            "('042308019078', 'Dinh Thi Phuong Thao',   '09/01/2008', '42', 'NU'),"
            "('045208020189', 'Truong Cong Danh',       '17/05/2008', '45', 'NAM'),"
            "('040208021290', 'Nguyen Duc Manh',        '28/08/2008', '40', 'NAM'),"
            "('048308022301', 'Vo Thi Yen Nhi',         '14/10/2008', '48', 'NU'),"
            "('031208023412', 'Cao Xuan Bach',          '02/09/2008', '31', 'NAM'),"
            "('051208024523', 'Mai Van Tien',           '21/07/2008', '51', 'NAM'),"
            "('079308025634', 'Lam Gia Han',            '06/11/2008', '79', 'NU'),"
            "('048208026745', 'Ta Dinh Quang',          '13/03/2008', '48', 'NAM'),"
            "('046208027856', 'Hoang Van Dung',         '29/04/2008', '46', 'NAM'),"
            "('042308028967', 'Nguyen Thi Thu Ha',      '15/08/2008', '42', 'NU'),"
            "('045208029078', 'Pham Quoc Bao',          '18/12/2008', '45', 'NAM'),"
            "('040208030189', 'Vuong Dinh Khoi',        '04/02/2008', '40', 'NAM'),"
            "('038208031290', 'Tran Dinh Trong',        '24/06/2008', '38', 'NAM'),"
            "('051308032301', 'Le Thi Thanh Thuy',      '01/10/2008', '51', 'NU'),"
            "('048208033412', 'Nguyen Cong Hoang',      '12/07/2008', '48', 'NAM'),"
            "('046208034523', 'Doan Minh Quan',         '19/09/2008', '46', 'NAM'),"
            "('042208035634', 'Ha Van Kiet',            '05/11/2008', '42', 'NAM');");

    // 9. Seed Thi Sinh (36 thi sinh)
    execute("INSERT IGNORE INTO `thisinh` (`soBaoDanh`, `soCCCD`, `namTuyenSinh`, `diemToan`, `diemLy`, `diemHoa`) VALUES "
            "('DUT001', '048003001234', 2026, 9.20, 9.00, 9.50),"
            "('DUT002', '048208001567', 2026, 9.80, 9.60, 9.80),"
            "('DUT003', '048308002341', 2026, 9.60, 9.40, 9.75),"
            "('DUT004', '046208003412', 2026, 9.40, 9.60, 9.25),"
            "('DUT005', '051208004523', 2026, 9.20, 9.00, 9.40),"
            "('DUT006', '040308005634', 2026, 9.00, 9.25, 9.00),"
            "('DUT007', '042208006745', 2026, 9.00, 8.80, 9.20),"
            "('DUT008', '001308007856', 2026, 8.80, 9.00, 8.80),"
            "('DUT009', '045208008967', 2026, 8.60, 8.80, 9.00),"
            "('DUT010', '056308009078', 2026, 8.60, 8.50, 9.00),"
            "('DUT011', '048208010189', 2026, 8.40, 8.80, 8.60),"
            "('DUT012', '046208011290', 2026, 8.50, 8.50, 8.50),"
            "('DUT013', '051308012301', 2026, 8.40, 8.60, 8.25),"
            "('DUT014', '038208013412', 2026, 8.20, 8.40, 8.50),"
            "('DUT015', '079308014523', 2026, 8.00, 8.60, 8.40),"
            "('DUT016', '048208015634', 2026, 8.20, 8.20, 8.20),"
            "('DUT017', '064208016745', 2026, 8.00, 8.25, 8.00),"
            "('DUT018', '066308017856', 2026, 8.00, 8.00, 8.00),"
            "('DUT019', '048208018967', 2026, 7.80, 8.00, 8.00),"
            "('DUT020', '042308019078', 2026, 7.80, 7.80, 7.80),"
            "('DUT021', '045208020189', 2026, 7.60, 7.80, 7.60),"
            "('DUT022', '040208021290', 2026, 7.50, 7.60, 7.50),"
            "('DUT023', '048308022301', 2026, 7.40, 7.50, 7.50),"
            "('DUT024', '031208023412', 2026, 7.20, 7.40, 7.50),"
            "('DUT025', '051208024523', 2026, 7.00, 7.20, 7.60),"
            "('DUT026', '079308025634', 2026, 7.00, 7.25, 7.00),"
            "('DUT027', '048208026745', 2026, 7.00, 6.80, 6.80),"
            "('DUT028', '046208027856', 2026, 6.80, 6.60, 6.80),"
            "('DUT029', '042308028967', 2026, 6.50, 6.50, 6.75),"
            "('DUT030', '045208029078', 2026, 6.40, 6.50, 6.20),"
            "('DUT031', '040208030189', 2026, 6.20, 6.00, 6.40),"
            "('DUT032', '038208031290', 2026, 6.00, 5.80, 6.20),"
            "('DUT033', '051308032301', 2026, 5.60, 5.80, 5.80),"
            "('DUT034', '048208033412', 2026, 5.50, 5.50, 5.50),"
            "('DUT035', '046208034523', 2026, 4.50, 4.80, 4.50),"
            "('DUT036', '042208035634', 2026, 9.00, 8.50, 1.00);");

    // 10. Seed Nguyen Vong (Dang ky toi da 3 NV moi thi sinh)
    execute("INSERT IGNORE INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES "
            "('DUT001', 1, '7480201B', 'DANG XET'), ('DUT001', 2, '7480201', 'DANG XET'), ('DUT001', 3, '7480106', 'DANG XET'),"
            "('DUT002', 1, '7480201B', 'DANG XET'), ('DUT002', 2, '7520207A', 'DANG XET'), ('DUT002', 3, '7480201', 'DANG XET'),"
            "('DUT003', 1, '7480201B', 'DANG XET'), ('DUT003', 2, '7480201', 'DANG XET'),"
            "('DUT004', 1, '7520207A', 'DANG XET'), ('DUT004', 2, '7520207', 'DANG XET'), ('DUT004', 3, '7520216', 'DANG XET'),"
            "('DUT005', 1, '7520216', 'DANG XET'), ('DUT005', 2, '7520201', 'DANG XET'), ('DUT005', 3, '7520114', 'DANG XET'),"
            "('DUT006', 1, '7480201', 'DANG XET'),  ('DUT006', 2, '7480201B', 'DANG XET'),"
            "('DUT007', 1, '7520130', 'DANG XET'),  ('DUT007', 2, '7520114', 'DANG XET'),"
            "('DUT008', 1, '7480201B', 'DANG XET'), ('DUT008', 2, '7480201A', 'DANG XET'),"
            "('DUT009', 1, '7520207A', 'DANG XET'), ('DUT009', 2, '7480106', 'DANG XET'),"
            "('DUT010', 1, '7480201', 'DANG XET'),  ('DUT010', 2, '7480106', 'DANG XET'),"
            "('DUT011', 1, '7520216', 'DANG XET'),  ('DUT011', 2, '7520207A', 'DANG XET'),"
            "('DUT012', 1, '7520114', 'DANG XET'),  ('DUT012', 2, '7520130', 'DANG XET'),"
            "('DUT013', 1, '7520301', 'DANG XET'),  ('DUT013', 2, '7580201', 'DANG XET'),"
            "('DUT014', 1, '7480106', 'DANG XET'),  ('DUT014', 2, '7480201', 'DANG XET'),"
            "('DUT015', 1, '7480201A', 'DANG XET'), ('DUT015', 2, '7480201', 'DANG XET'),"
            "('DUT016', 1, '7520201', 'DANG XET'),  ('DUT016', 2, '7520216', 'DANG XET'),"
            "('DUT017', 1, '7520130', 'DANG XET'),  ('DUT017', 2, '7520114', 'DANG XET'),"
            "('DUT018', 1, '7520207', 'DANG XET'),  ('DUT018', 2, '7520207A', 'DANG XET'),"
            "('DUT019', 1, '7480201B', 'DANG XET'), ('DUT019', 2, '7480201', 'DANG XET'), ('DUT019', 3, '7480106', 'DANG XET'),"
            "('DUT020', 1, '7520301', 'DANG XET'),  ('DUT020', 2, '7580201', 'DANG XET'),"
            "('DUT021', 1, '7580201', 'DANG XET'),  ('DUT021', 2, '7520114', 'DANG XET'),"
            "('DUT022', 1, '7520201', 'DANG XET'),  ('DUT022', 2, '7520130', 'DANG XET'),"
            "('DUT023', 1, '7480201A', 'DANG XET'), ('DUT023', 2, '7480106', 'DANG XET'),"
            "('DUT024', 1, '7520114', 'DANG XET'),  ('DUT024', 2, '7520216', 'DANG XET'),"
            "('DUT025', 1, '7520207', 'DANG XET'),  ('DUT025', 2, '7520201', 'DANG XET'),"
            "('DUT026', 1, '7480201', 'DANG XET'),  ('DUT026', 2, '7520207A', 'DANG XET'),"
            "('DUT027', 1, '7520130', 'DANG XET'),  ('DUT027', 2, '7580201', 'DANG XET'),"
            "('DUT028', 1, '7580201', 'DANG XET'),  ('DUT028', 2, '7520301', 'DANG XET'),"
            "('DUT029', 1, '7520301', 'DANG XET'),  ('DUT029', 2, '7580201', 'DANG XET'),"
            "('DUT030', 1, '7520201', 'DANG XET'),  ('DUT030', 2, '7580201', 'DANG XET'),"
            "('DUT031', 1, '7580201', 'DANG XET'),  ('DUT031', 2, '7520301', 'DANG XET'),"
            "('DUT032', 1, '7520301', 'DANG XET'),  ('DUT032', 2, '7580201', 'DANG XET'),"
            "('DUT033', 1, '7580201', 'DANG XET'),  ('DUT033', 2, '7520301', 'DANG XET'),"
            "('DUT034', 1, '7580201', 'DANG XET'),"
            "('DUT035', 1, '7580201', 'DANG XET'),  ('DUT035', 2, '7520301', 'DANG XET'),"
            "('DUT036', 1, '7520201', 'DANG XET'),  ('DUT036', 2, '7580201', 'DANG XET');");

    return commit();
}
