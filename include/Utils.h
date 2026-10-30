#ifndef UTILS_H
#define UTILS_H

#include <string>

using namespace std;

namespace Utils {
    // Xóa khoảng trắng thừa ở đầu và cuối chuỗi
    string trim(const string& str);

    // Chuẩn hóa họ tên: viết hoa chữ cái đầu mỗi từ, loại bỏ khoảng trắng thừa
    string chuanHoaTen(const string& name);

    // Chuyển chuỗi về chữ thường (phục vụ tìm kiếm không phân biệt hoa/thường)
    string toLower(const string& str);

    // Chuyển chuỗi về chữ hoa
    string toUpper(const string& str);

    // Kiểm tra chuỗi nguồn có chứa chuỗi con không (không phân biệt hoa/thường)
    bool containsIgnoreCase(const string& source, const string& target);

    // Kiểm tra tính hợp lệ của Số Căn cước Công dân (CCCD: đúng 12 chữ số)
    bool kiemTraCCCD(const string& cccd);

    // Kiểm tra tính hợp lệ của định dạng ngày sinh (dd/mm/yyyy, kiểm tra năm nhuận)
    bool kiemTraNgaySinh(const string& dateStr);

    // Kiểm tra tính hợp lệ của điểm thi từng môn (0.00 đến 10.00)
    bool kiemTraDiem(double diem);

    // Kiểm tra tính hợp lệ của Số báo danh (không rỗng, không chứa khoảng trắng, độ dài 3-15)
    bool kiemTraSBD(const string& sbd);

    // Kiểm tra tính hợp lệ của giới tính (Nam hoặc Nu)
    bool kiemTraGioiTinh(const string& gt);

    // Tách dòng văn bản dạng "Key: Value" thành 2 phần key và value (phục vụ đọc CSDL)
    bool tachKeyValue(const string& line, string& key, string& value);
}

#endif // UTILS_H
