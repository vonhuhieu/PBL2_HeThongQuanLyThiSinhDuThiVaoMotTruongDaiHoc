#ifndef KHU_VUC_H
#define KHU_VUC_H

#include <string>
#include <iostream>

using namespace std;

/**
 * @brief Thực thể Khu vực ưu tiên trong hệ thống Cơ sở dữ liệu tuyển sinh
 * Đáp ứng chuẩn hóa dữ liệu 3NF, tránh nhập điểm ưu tiên tùy tiện
 */
class KhuVuc {
private:
    string maKhuVuc;     // Khóa chính (PK): KV1, KV2NT, KV2, KV3
    string tenKhuVuc;    // Tên khu vực: Khu vuc 1, Khu vuc 2 nong thon...
    double diemUuTien;   // Điểm cộng theo quy chế: 0.75, 0.50, 0.25, 0.00

public:
    KhuVuc();
    KhuVuc(const string& ma, const string& ten, double diem);
    KhuVuc(const KhuVuc& other);
    ~KhuVuc();

    // Getters
    string getMaKhuVuc() const;
    string getTenKhuVuc() const;
    double getDiemUuTien() const;

    // Setters
    void setMaKhuVuc(const string& ma);
    void setTenKhuVuc(const string& ten);
    void setDiemUuTien(double diem);

    // Toán tử gán
    KhuVuc& operator=(const KhuVuc& other);
    bool operator==(const KhuVuc& other) const;
};

#endif // KHU_VUC_H
