#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace AdmissionConst {
    // 1. Quy định điểm thi và ngưỡng xét tuyển
    const double DIEM_LIET          = 1.00;     // Môn thi <= 1.00 là điểm liệt
    const double DIEM_SAN_MAC_DINH  = 15.00;    // Điểm sàn tối thiểu nhận hồ sơ
    const double DIEM_CHUAN_MAC_DINH= 22.00;    // Điểm chuẩn chung khởi tạo mặc định
    const double DIEM_TOI_DA_MON    = 10.00;    // Điểm tối đa mỗi môn thi
    const double DIEM_TOI_THIEU_MON = 0.00;     // Điểm tối thiểu mỗi môn thi
    const int    SO_MON_THI         = 3;        // Tổ hợp 3 môn thi A00 (Toán - Lý - Hóa)

    // 2. Kết quả xét tuyển & Trạng thái nguyện vọng
    const std::string KET_QUA_TRUNG_TUYEN = "TRUNG TUYEN";
    const std::string KET_QUA_KHONG_DAT   = "Khong dat";
    const std::string KET_QUA_DIEM_LIET   = "Diem liet";
    
    const std::string TT_NV_TRUNG_TUYEN   = "TRUNG TUYEN";
    const std::string TT_NV_KHONG_TRUNG   = "KHONG TRUNG TUYEN";
    const std::string TT_NV_DANG_XET      = "DANG XET";

    // 3. Phân khúc thang điểm phổ điểm (5 mức chuẩn)
    const double MOC_XUAT_SAC   = 27.00;
    const double MOC_GIOI       = 24.00;
    const double MOC_KHA        = 21.00;
    const double MOC_TRUNG_BINH = 15.00;

    const std::string TEN_MUC_XUAT_SAC   = "Muc diem Rat Cao (Xuat sac)";
    const std::string TEN_MUC_GIOI       = "Muc diem Cao (Gioi)";
    const std::string TEN_MUC_KHA        = "Muc diem Kha";
    const std::string TEN_MUC_TRUNG_BINH = "Muc diem Trung binh";
    const std::string TEN_MUC_THAP       = "Muc diem Thap";

    // 4. Quy chuẩn tuyển sinh & Lớp sinh hoạt Bách Khoa Đà Nẵng (DUT)
    const int         NAM_TUYEN_SINH_MAC_DINH = 2026;       // Khóa tuyển sinh mặc định (K26)
    const int         SI_SO_LOP_CHUAN         = 30;         // Sĩ số chuẩn của 1 lớp sinh hoạt DUT
    const int         MAX_NGUYEN_VONG         = 3;          // Tối đa 3 nguyện vọng
    const std::string MA_TRUONG_DUT           = "DDK";       // Mã trường ĐH Bách khoa - ĐH Đà Nẵng
    const std::string TEN_TRUONG_MAC_DINH     = "TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG";

    // 5. Mã danh mục hệ thống (Mnemonic Natural Keys)
    const std::string GT_NAM            = "NAM";  // Nam
    const std::string GT_NU             = "NU";   // Nữ
    const std::string GT_KHAC           = "KHAC"; // Khác
    const std::string MA_TINH_MAC_DINH  = "48";   // TP. Đà Nẵng
}

#endif // CONSTANTS_H
