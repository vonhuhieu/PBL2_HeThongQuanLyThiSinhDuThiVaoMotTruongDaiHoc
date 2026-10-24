#ifndef NGANH_XET_TUYEN_H
#define NGANH_XET_TUYEN_H

#include <string>
#include <iostream>

using namespace std;

/**
 * @brief Thực thể Ngành Xét Tuyển trong hệ thống Cơ sở dữ liệu trường đại học
 * Quản lý danh mục ngành, tổ hợp môn, chỉ tiêu và điểm chuẩn theo quy chuẩn CSDL
 */
class NganhXetTuyen {
private:
    string maNganh;      // Khóa chính (PK): ví dụ 7480201 (CNTT), 7520114 (Cơ điện tử)
    string tenNganh;     // Tên ngành đào tạo
    string toHopMon;     // Tổ hợp môn: A00 (Toán, Lý, Hóa), A01...
    int chiTieu;         // Chỉ tiêu tuyển sinh của ngành
    double diemChuan;    // Điểm chuẩn trúng tuyển của ngành đó

public:
    NganhXetTuyen();
    NganhXetTuyen(const string& ma, const string& ten, const string& toHop, int ct, double dc);
    NganhXetTuyen(const NganhXetTuyen& other);
    ~NganhXetTuyen();

    // Getters
    string getMaNganh() const;
    string getTenNganh() const;
    string getToHopMon() const;
    int getChiTieu() const;
    double getDiemChuan() const;

    // Setters
    void setMaNganh(const string& ma);
    void setTenNganh(const string& ten);
    void setToHopMon(const string& toHop);
    void setChiTieu(int ct);
    void setDiemChuan(double dc);

    // Toán tử
    NganhXetTuyen& operator=(const NganhXetTuyen& other);
    bool operator==(const NganhXetTuyen& other) const;
};

#endif // NGANH_XET_TUYEN_H
