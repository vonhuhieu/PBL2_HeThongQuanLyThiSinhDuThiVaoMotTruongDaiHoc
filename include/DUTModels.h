#ifndef DUT_MODELS_H
#define DUT_MODELS_H

#include <string>
#include "Constants.h"

using namespace std;

/**
 * @brief Thực thể KHOA ĐÀO TẠO (Bảng MySQL: khoa)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct Khoa {
    string maKhoa;              // PK: CNTT, DIEN, DTVT, CKHI, HOA, XDDD
    string tenKhoa;             // Khoa Cong nghe Thong tin...
    string maKhoaTheoChuanDUT;  // 102, 105, 106, 101, 107, 109 (Dung cau tao Ma Sinh Vien)

    Khoa() : maKhoa(""), tenKhoa(""), maKhoaTheoChuanDUT("") {}
    Khoa(const string& mk, const string& tk, const string& mkDUT)
        : maKhoa(mk), tenKhoa(tk), maKhoaTheoChuanDUT(mkDUT) {}

    bool operator==(const Khoa& other) const {
        return maKhoa == other.maKhoa;
    }
};

/**
 * @brief Thực thể DANH MỤC NGÀNH TĨNH (Bảng MySQL: danh_muc_nganh)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct DanhMucNganh {
    string maNganh;     // PK: 7480201B, 7480201...
    string tenNganh;    // Ten nganh dao tao
    string maKhoa;      // FK -> khoa.maKhoa
    string tienToLop;   // TCLC_KHDL, T_DT, TDH, VM... (Ghep voi Khoa thanh ten lop: 26TCLC_KHDL1)

    DanhMucNganh() : maNganh(""), tenNganh(""), maKhoa(""), tienToLop("") {}
    DanhMucNganh(const string& mn, const string& tn, const string& mk, const string& ttl)
        : maNganh(mn), tenNganh(tn), maKhoa(mk), tienToLop(ttl) {}

    bool operator==(const DanhMucNganh& other) const {
        return maNganh == other.maNganh;
    }
};

/**
 * @brief Thực thể KHÓA HỌC / NĂM TUYỂN SINH (Bảng MySQL: khoa_hoc)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct KhoaHoc {
    int namTuyenSinh;       // PK: 2026, 2025...
    string tenKhoaHoc;     // Khoa 2026 (K26 - Dai hoc Chinh quy)
    string trangThai;      // DANG TUYEN SINH, DA KET THUC

    KhoaHoc() : namTuyenSinh(AdmissionConst::NAM_TUYEN_SINH_MAC_DINH),
                tenKhoaHoc(""), trangThai("DANG TUYEN SINH") {}
    KhoaHoc(int nam, const string& ten, const string& tt = "DANG TUYEN SINH")
        : namTuyenSinh(nam), tenKhoaHoc(ten), trangThai(tt) {}

    bool operator==(const KhoaHoc& other) const {
        return namTuyenSinh == other.namTuyenSinh;
    }
};

/**
 * @brief Thực thể DỮ LIỆU TUYỂN SINH BIẾN ĐỘNG THEO KHÓA (Bảng MySQL: nganh_theo_khoa_hoc)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct NganhTheoKhoaHoc {
    int namTuyenSinh;       // PK, FK -> khoa_hoc.namTuyenSinh
    string maNganh;         // PK, FK -> danh_muc_nganh.maNganh
    int chiTieu;            // Chi tieu tuyen sinh (cho phep nhap dong tren UI)
    double diemChuanChot;   // Diem chuan chinh thuc sau khi chay thuat toan loc chi tieu
    double diemSanToiThieu; // Diem san nhan ho so (mac dinh 15.00)

    NganhTheoKhoaHoc() 
        : namTuyenSinh(AdmissionConst::NAM_TUYEN_SINH_MAC_DINH),
          maNganh(""), chiTieu(60), diemChuanChot(0.0),
          diemSanToiThieu(AdmissionConst::DIEM_SAN_MAC_DINH) {}

    NganhTheoKhoaHoc(int nam, const string& mn, int ct, double dChuan = 0.0, double dSan = AdmissionConst::DIEM_SAN_MAC_DINH)
        : namTuyenSinh(nam), maNganh(mn), chiTieu(ct), diemChuanChot(dChuan), diemSanToiThieu(dSan) {}

    bool operator==(const NganhTheoKhoaHoc& other) const {
        return namTuyenSinh == other.namTuyenSinh && maNganh == other.maNganh;
    }
};

/**
 * @brief Thực thể NGUYỆN VỌNG XÉT TUYỂN (Bảng MySQL: nguyen_vong)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct NguyenVong {
    int id;                 // PK tu tang (0 neu tao moi)
    string soBaoDanh;       // FK -> thisinh.soBaoDanh
    int thuTuNV;            // 1: NV1, 2: NV2, 3: NV3
    string maNganh;         // FK -> danh_muc_nganh.maNganh
    string trangThai;       // AdmissionConst::TT_NV_DANG_XET, TT_NV_TRUNG_TUYEN, TT_NV_KHONG_TRUNG

    NguyenVong() : id(0), soBaoDanh(""), thuTuNV(1), maNganh(""),
                   trangThai(AdmissionConst::TT_NV_DANG_XET) {}
    NguyenVong(int nvId, const string& sbd, int thuTu, const string& mn,
               const string& tt = AdmissionConst::TT_NV_DANG_XET)
        : id(nvId), soBaoDanh(sbd), thuTuNV(thuTu), maNganh(mn), trangThai(tt) {}

    bool operator==(const NguyenVong& other) const {
        return (id != 0 && id == other.id) || 
               (soBaoDanh == other.soBaoDanh && thuTuNV == other.thuTuNV);
    }
};

/**
 * @brief Thực thể CẤU HÌNH TOÀN CỤC (Bảng MySQL: cau_hinh)
 * Quy chuẩn GEMINI.md - 100% lowerCamelCase
 */
struct CauHinh {
    string khoa;        // PK: namTuyenSinhHienHanh, siSoLopChuan, diemLiet, tenTruong...
    string giaTri;      // Gia tri tham so
    string moTa;        // Ghi chu nghiep vu

    CauHinh() : khoa(""), giaTri(""), moTa("") {}
    CauHinh(const string& k, const string& v, const string& desc = "")
        : khoa(k), giaTri(v), moTa(desc) {}

    bool operator==(const CauHinh& other) const {
        return khoa == other.khoa;
    }
};

#endif // DUT_MODELS_H
