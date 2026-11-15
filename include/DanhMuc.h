#ifndef DANH_MUC_H
#define DANH_MUC_H

#include <string>

using namespace std;

/**
 * @brief Cấu trúc thực thể danh mục Tỉnh / Thành phố
 * Bảng CSDL: tinh_thanh (lowerCamelCase: maTinh, tenTinh, loai)
 * Hỗ trợ alias .ma và .ten để tương thích toàn diện mã nguồn
 */
struct TinhThanh {
    string maTinh;      // Mã tỉnh thành (vd: "01", "04", "48"...) - Khóa chính
    string tenTinh;     // Tên tỉnh thành (vd: "TP. Da Nang", "Quang Ngai"...)
    string loai;        // Phân loại: "Thanh pho truc thuoc Trung uong" hoặc "Tinh"

    // Alias để tương thích code cũ
    string ma;
    string ten;

    TinhThanh() : maTinh(""), tenTinh(""), loai("Tinh"), ma(""), ten("") {}
    TinhThanh(const string& m, const string& t, const string& l = "Tinh")
        : maTinh(m), tenTinh(t), loai(l), ma(m), ten(t) {}

    bool operator==(const TinhThanh& other) const {
        return (maTinh == other.maTinh) || (ma == other.ma);
    }
};

/**
 * @brief Cấu trúc thực thể danh mục Giới tính
 * Bảng CSDL: gioi_tinh (lowerCamelCase: maGioiTinh, tenGioiTinh)
 * Hỗ trợ alias .ma và .ten để tương thích toàn diện mã nguồn
 */
struct GioiTinh {
    string maGioiTinh;  // Mã giới tính (vd: "NAM", "NU", "KHAC") - Khóa chính
    string tenGioiTinh; // Tên hiển thị (vd: "Nam", "Nu", "Khac")
    string moTa;        // Mô tả chi tiết

    // Alias để tương thích code cũ
    string ma;
    string ten;

    GioiTinh() : maGioiTinh(""), tenGioiTinh(""), moTa(""), ma(""), ten("") {}
    GioiTinh(const string& m, const string& t, const string& desc = "")
        : maGioiTinh(m), tenGioiTinh(t), moTa(desc), ma(m), ten(t) {}

    bool operator==(const GioiTinh& other) const {
        return (maGioiTinh == other.maGioiTinh) || (ma == other.ma);
    }
};

#endif // DANH_MUC_H
