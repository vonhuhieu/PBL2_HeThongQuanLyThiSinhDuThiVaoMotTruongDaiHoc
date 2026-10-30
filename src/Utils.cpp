#include "../include/Utils.h"
#include <sstream>
#include <cctype>
#include <algorithm>

namespace Utils {

    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }

    string chuanHoaTen(const string& name) {
        string trimmed = trim(name);
        if (trimmed.empty()) return "";

        stringstream ss(trimmed);
        string word;
        string result = "";

        while (ss >> word) {
            // Chuyển toàn bộ ký tự sang chữ thường
            for (size_t i = 0; i < word.length(); ++i) {
                word[i] = static_cast<char>(tolower(static_cast<unsigned char>(word[i])));
            }
            // Viết hoa chữ cái đầu tiên
            if (!word.empty()) {
                word[0] = static_cast<char>(toupper(static_cast<unsigned char>(word[0])));
            }
            if (!result.empty()) {
                result += " ";
            }
            result += word;
        }
        return result;
    }

    string toLower(const string& str) {
        string res = str;
        for (char& c : res) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        return res;
    }

    string toUpper(const string& str) {
        string res = str;
        for (char& c : res) {
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        }
        return res;
    }

    bool containsIgnoreCase(const string& source, const string& target) {
        string sLower = toLower(source);
        string tLower = toLower(target);
        return (sLower.find(tLower) != string::npos);
    }

    static bool laNamNhuan(int nam) {
        return (nam % 400 == 0) || (nam % 4 == 0 && nam % 100 != 0);
    }

    bool kiemTraCCCD(const string& cccd) {
        string trimmed = trim(cccd);
        // CCCD chuẩn quốc gia bắt buộc đúng 12 chữ số
        if (trimmed.length() != 12) return false;
        for (char c : trimmed) {
            if (!isdigit(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    bool kiemTraNgaySinh(const string& dateStr) {
        string s = trim(dateStr);
        // Định dạng hợp lệ: dd/mm/yyyy (độ dài đúng 10 ký tự)
        if (s.length() != 10) return false;
        if (s[2] != '/' || s[5] != '/') return false;

        for (int i = 0; i < 10; ++i) {
            if (i == 2 || i == 5) continue;
            if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
        }

        int d = (s[0] - '0') * 10 + (s[1] - '0');
        int m = (s[3] - '0') * 10 + (s[4] - '0');
        int y = (s[6] - '0') * 1000 + (s[7] - '0') * 100 + 
                (s[8] - '0') * 10 + (s[9] - '0');

        if (y < 1970 || y > 2020) return false;
        if (m < 1 || m > 12) return false;

        int ngayTrongThang[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (laNamNhuan(y)) {
            ngayTrongThang[2] = 29;
        }

        if (d < 1 || d > ngayTrongThang[m]) return false;

        return true;
    }

    bool kiemTraDiem(double diem) {
        return (diem >= 0.0 && diem <= 10.0);
    }

    bool kiemTraSBD(const string& sbd) {
        string trimmed = trim(sbd);
        if (trimmed.length() < 3 || trimmed.length() > 15) return false;
        for (char c : trimmed) {
            if (isspace(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    bool kiemTraGioiTinh(const string& gt) {
        string s = toLower(trim(gt));
        return (s == "nam" || s == "nu");
    }

    bool tachKeyValue(const string& line, string& key, string& value) {
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) return false;

        key = trim(line.substr(0, colonPos));
        value = trim(line.substr(colonPos + 1));
        return (!key.empty());
    }
}
