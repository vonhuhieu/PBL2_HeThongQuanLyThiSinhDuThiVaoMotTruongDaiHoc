#include "MySQLConnector.h"
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

    // Thu ket noi truc tiep toi CSDL
    if (mysql_real_connect(conn, config.host.c_str(), config.user.c_str(),
                           config.password.c_str(), config.database.c_str(),
                           config.port, nullptr, 0)) {
        mysql_set_character_set(conn, "utf8mb4");
        connected = true;
        lastError = "";
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

