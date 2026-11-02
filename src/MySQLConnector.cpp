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

    const char* queries[] = {
        "CREATE TABLE IF NOT EXISTS `tinh_thanh` ("
        "  `ma_tinh` VARCHAR(10) NOT NULL,"
        "  `ten_tinh` VARCHAR(100) NOT NULL,"
        "  `loai` VARCHAR(50) DEFAULT 'Tinh',"
        "  PRIMARY KEY (`ma_tinh`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        "CREATE TABLE IF NOT EXISTS `gioi_tinh` ("
        "  `ma_gioi_tinh` VARCHAR(10) NOT NULL,"
        "  `ten_gioi_tinh` VARCHAR(30) NOT NULL,"
        "  PRIMARY KEY (`ma_gioi_tinh`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        "CREATE TABLE IF NOT EXISTS `nguoi` ("
        "  `so_cccd` VARCHAR(20) NOT NULL,"
        "  `ho_ten` VARCHAR(100) NOT NULL,"
        "  `ngay_sinh` VARCHAR(20) NOT NULL,"
        "  `que_quan` VARCHAR(10) NOT NULL,"
        "  `gioi_tinh` VARCHAR(10) NOT NULL,"
        "  PRIMARY KEY (`so_cccd`),"
        "  CONSTRAINT `fk_nguoi_que_quan` FOREIGN KEY (`que_quan`) REFERENCES `tinh_thanh` (`ma_tinh`) ON UPDATE CASCADE ON DELETE RESTRICT,"
        "  CONSTRAINT `fk_nguoi_gioi_tinh` FOREIGN KEY (`gioi_tinh`) REFERENCES `gioi_tinh` (`ma_gioi_tinh`) ON UPDATE CASCADE ON DELETE RESTRICT"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        "CREATE TABLE IF NOT EXISTS `thisinh` ("
        "  `sbd` VARCHAR(20) NOT NULL,"
        "  `so_cccd` VARCHAR(20) NOT NULL UNIQUE,"
        "  `diem_toan` DOUBLE NOT NULL,"
        "  `diem_ly` DOUBLE NOT NULL,"
        "  `diem_hoa` DOUBLE NOT NULL,"
        "  PRIMARY KEY (`sbd`),"
        "  CONSTRAINT `fk_thisinh_nguoi` FOREIGN KEY (`so_cccd`) REFERENCES `nguoi` (`so_cccd`) ON UPDATE CASCADE ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;",

        "CREATE TABLE IF NOT EXISTS `cau_hinh` ("
        "  `khoa` VARCHAR(50) NOT NULL,"
        "  `gia_tri` VARCHAR(255) NOT NULL,"
        "  `mo_ta` VARCHAR(255) DEFAULT NULL,"
        "  PRIMARY KEY (`khoa`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;"
    };

    for (const char* q : queries) {
        if (!execute(q)) {
            return false;
        }
    }
    return true;
}

bool MySQLConnector::seedDefaultData() {
    if (!isConnected()) return false;

    // 1. Kiem tra xem da co tinh thanh chua
    MYSQL_RES* res = query("SELECT COUNT(*) FROM `tinh_thanh`;");
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row && row[0] && atoi(row[0]) > 0) {
            freeResult(res);
            return true; // Da co du lieu, khong can seed lai
        }
        freeResult(res);
    }

    beginTransaction();

    // 2. Seed 34 Tinh/Thanh pho
    execute("INSERT IGNORE INTO `tinh_thanh` (`ma_tinh`, `ten_tinh`, `loai`) VALUES "
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

    // 3. Seed Gioi Tinh
    execute("INSERT IGNORE INTO `gioi_tinh` (`ma_gioi_tinh`, `ten_gioi_tinh`) VALUES "
            "('NAM', 'Nam'), ('NU', 'Nu'), ('KHAC', 'Khac');");

    // 4. Seed Cau Hinh
    execute("INSERT IGNORE INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES "
            "('truong', 'TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG', 'Ten truong tuyen sinh'),"
            "('nam_tuyen_sinh', '2026', 'Khoa tuyen sinh dai hoc'),"
            "('diem_chuan', '23.00', 'Diem chuan trung tuyen'),"
            "('diem_liet', '1.00', 'Muc diem liet theo quy che');");

    // 5. Seed Nguoi
    execute("INSERT IGNORE INTO `nguoi` (`so_cccd`, `ho_ten`, `ngay_sinh`, `que_quan`, `gioi_tinh`) VALUES "
            "('012345678901', 'Vo Nhu Hieu', '04/03/2003', '48', 'NAM'),"
            "('012345678902', 'Vo Huy Tuong', '27/07/2009', '48', 'NAM'),"
            "('012345678903', 'Nguyen Thi Nhat Vy', '10/09/2005', '51', 'NU'),"
            "('201948572134', 'Nguyen Van An', '15/03/2005', '48', 'NAM'),"
            "('201837461928', 'Tran Thi Bich', '22/07/2005', '48', 'NU'),"
            "('201726354819', 'Le Hoang Cong', '10/11/2005', '46', 'NAM'),"
            "('201615243701', 'Pham Thi Diem', '05/01/2005', '51', 'NU'),"
            "('201504132692', 'Vo Minh Duc', '18/09/2005', '42', 'NAM'),"
            "('201493021583', 'Hoang Thi Ha', '30/04/2005', '40', 'NU'),"
            "('201382910474', 'Dang Quoc Khanh', '12/12/2005', '45', 'NAM'),"
            "('201271809365', 'Bui Mai Linh', '08/08/2005', '01', 'NU'),"
            "('201160798256', 'Ngo Van Nam', '25/02/2005', '79', 'NAM'),"
            "('201059687147', 'Dinh Thao Quyen', '19/10/2005', '56', 'NU'),"
            "('200948576038', 'Phan Thanh Tung', '03/06/2005', '48', 'NAM');");

    // 6. Seed Thi Sinh
    execute("INSERT IGNORE INTO `thisinh` (`sbd`, `so_cccd`, `diem_toan`, `diem_ly`, `diem_hoa`) VALUES "
            "('DUT001', '012345678901', 9.00, 7.00, 8.00),"
            "('DUT002', '012345678902', 9.00, 9.75, 8.50),"
            "('DUT003', '012345678903', 9.50, 9.50, 8.25),"
            "('DUT004', '201948572134', 9.50, 9.00, 9.75),"
            "('DUT005', '201837461928', 8.75, 8.50, 9.00),"
            "('DUT006', '201726354819', 7.50, 6.75, 7.00),"
            "('DUT007', '201615243701', 9.00, 8.75, 8.25),"
            "('DUT008', '201504132692', 6.25, 5.50, 6.00),"
            "('DUT009', '201493021583', 9.75, 9.50, 9.80),"
            "('DUT010', '201382910474', 5.00, 4.50, 5.50),"
            "('DUT011', '201271809365', 8.50, 8.25, 8.75),"
            "('DUT012', '201160798256', 7.00, 7.25, 6.50),"
            "('DUT013', '201059687147', 8.00, 8.50, 7.75),"
            "('DUT014', '200948576038', 9.25, 9.00, 9.25);");

    return commit();
}
