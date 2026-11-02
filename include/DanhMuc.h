#ifndef DANH_MUC_H
#define DANH_MUC_H

#include <string>

using namespace std;

/**
 * @brief Cấu trúc thực thể danh mục Tỉnh / Thành phố
 * Tham chiếu bảng CSDL: tinh_thanh (tinhthanh.txt)
 */
struct TinhThanh {
    string ma;      // Mã tỉnh thành (vd: "01", "04", "48"...) - Khóa chính
    string ten;     // Tên tỉnh thành (vd: "TP. Da Nang", "Quang Ngai"...)
    string loai;    // Phân loại: "Thanh pho truc thuoc Trung uong" hoặc "Tinh truc thuoc"

    TinhThanh() : ma(""), ten(""), loai("") {}
    TinhThanh(const string& m, const string& t, const string& l)
        : ma(m), ten(t), loai(l) {}

    bool operator==(const TinhThanh& other) const {
        return ma == other.ma;
    }
};

/**
 * @brief Cấu trúc thực thể danh mục Giới tính
 * Tham chiếu bảng CSDL: gioi_tinh (gioitinh.txt)
 */
struct GioiTinh {
    string ma;      // Mã giới tính (vd: "NAM", "NU", "KHAC") - Khóa chính
    string ten;     // Tên hiển thị (vd: "Nam", "Nu", "Khac")
    string moTa;    // Mô tả chi tiết

    GioiTinh() : ma(""), ten(""), moTa("") {}
    GioiTinh(const string& m, const string& t, const string& desc)
        : ma(m), ten(t), moTa(desc) {}

    bool operator==(const GioiTinh& other) const {
        return ma == other.ma;
    }
};

#endif // DANH_MUC_H
