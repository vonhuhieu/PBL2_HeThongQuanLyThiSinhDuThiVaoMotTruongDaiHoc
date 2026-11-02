#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace AdmissionConst {
    // 1. Quy định điểm thi và ngưỡng xét tuyển
    const double DIEM_LIET = 1.00;              // Môn thi <= 1.00 là điểm liệt
    const double DIEM_CHUAN_MAC_DINH = 22.00;   // Điểm chuẩn mặc định thang 30
    const double DIEM_TOI_DA_MON = 10.00;       // Điểm tối đa mỗi môn thi
    const double DIEM_TOI_THIEU_MON = 0.00;     // Điểm tối thiểu mỗi môn thi

    // 2. Kết quả xét tuyển
    const std::string KET_QUA_TRUNG_TUYEN = "TRUNG TUYEN";
    const std::string KET_QUA_KHONG_DAT   = "Khong dat";

    // 3. Phân khúc thang điểm phổ điểm
    const double MOC_XUAT_SAC   = 27.00;
    const double MOC_GIOI       = 24.00;
    const double MOC_KHA        = 21.00;
    const double MOC_TRUNG_BINH = 15.00;

    const std::string TEN_MUC_XUAT_SAC   = "Muc diem Rat Cao (Xuat sac)";
    const std::string TEN_MUC_GIOI       = "Muc diem Cao (Gioi)";
    const std::string TEN_MUC_KHA        = "Muc diem Kha";
    const std::string TEN_MUC_TRUNG_BINH = "Muc diem Trung binh";
    const std::string TEN_MUC_THAP       = "Muc diem Thap";

    // 4. Mã danh mục giới tính trực quan (Mnemonic Natural Key)
    const std::string GT_NAM  = "NAM";  // Nam
    const std::string GT_NU   = "NU";   // Nu
    const std::string GT_KHAC = "KHAC"; // Khac
}

#endif // CONSTANTS_H
