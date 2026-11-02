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

    // Thiet lap cau hinh ket noi
    void setConfig(const DBConfig& cfg);
    DBConfig getConfig() const;

    // Ket noi va ngat ket noi
    bool connect();
    void disconnect();
    bool isConnected() const;
    std::string getLastError() const;

    // Thuc thi truy van
    bool execute(const std::string& sql);
    MYSQL_RES* query(const std::string& sql);
    void freeResult(MYSQL_RES* res);

    // Xu ly chuoi an toan tranh SQL Injection
    std::string escape(const std::string& input);

    // Quan ly Transaction
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Khoi tao CSDL va cac bang neu chua co
    bool initSchemaAndTables();
};

#endif // MYSQL_CONNECTOR_H
