#ifndef MYSQL_CONNECTOR_H
#define MYSQL_CONNECTOR_H

#include <string>
#include <vector>
#include <windows.h>
#include <mysql.h>

struct DBConfig {
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "pbl2_tuyensinh";
};

class MySQLConnector {
private:
    MYSQL* conn;
    DBConfig config;
    bool connected;
    std::string lastError;

public:
    MySQLConnector();
    ~MySQLConnector();

    // Thiết lập cấu hình kết nối
    void setConfig(const DBConfig& cfg);
    DBConfig getConfig() const;

    // Kết nối và ngắt kết nối
    bool connect();
    void disconnect();
    bool isConnected() const;
    std::string getLastError() const;

    // Thực thi truy vấn
    bool execute(const std::string& sql);
    MYSQL_RES* query(const std::string& sql);
    void freeResult(MYSQL_RES* res);

    // Xử lý chuỗi an toàn chống SQL Injection
    std::string escape(const std::string& input);

    // Quản lý Transaction
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Khởi tạo Schema, bảng và dữ liệu mẫu tự động
    bool initSchemaAndTables();
    bool seedDefaultData();
};

#endif // MYSQL_CONNECTOR_H
