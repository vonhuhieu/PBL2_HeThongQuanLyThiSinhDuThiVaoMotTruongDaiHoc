#ifndef UTILS_H
#define UTILS_H

#include <string>

using namespace std;

namespace Utils {
    // Xóa khoảng trắng ở đầu và cuối chuỗi
    string trim(const string& str);

    // Chuẩn hóa họ tên: viết hoa chữ cái đầu mỗi từ, loại bỏ khoảng trắng thừa
    string chuanHoaTen(const string& name);

    // Chuyển chuỗi về chữ thường (phục vụ tìm kiếm không phân biệt hoa/thường)
    string toLower(const string& str);

    // Chuyển chuỗi về chữ hoa
    string toUpper(const string& str);

    // Kiểm tra chuỗi chứa chuỗi con (Case-insensitive)
    bool containsIgnoreCase(const string& source, const string& target);

    // Kiểm tra tính hợp lệ của định dạng ngày sinh (dd/mm/yyyy)
    bool kiemTraNgaySinh(const string& dateStr);

    // Kiểm tra tính hợp lệ của số CMND/CCCD (chỉ gồm số, độ dài 9 hoặc 12)
    bool kiemTraCMND(const string& cmnd);

    // Kiểm tra tính hợp lệ của điểm thi môn (0.0 đến 10.0)
    bool kiemTraDiem(double diem);

    // Kiểm tra tính hợp lệ của điểm ưu tiên (0.0 đến 2.5)
    bool kiemTraDiemUuTien(double diem);

    // Kiểm tra tính hợp lệ của Số báo danh (không chứa dấu cách, độ dài từ 3 đến 15)
    bool kiemTraSBD(const string& sbd);
}

#endif // UTILS_H
