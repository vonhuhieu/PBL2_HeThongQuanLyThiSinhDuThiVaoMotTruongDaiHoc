#include "../include/Utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace Utils {
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    string chuanHoaTen(const string& name) {
        string trimmed = trim(name);
        if (trimmed.empty()) return "";

        stringstream ss(trimmed);
        string word;
        string result = "";

        while (ss >> word) {
            for (size_t i = 0; i < word.length(); ++i) {
                word[i] = tolower(static_cast<unsigned char>(word[i]));
            }
            if (!word.empty()) {
                word[0] = toupper(static_cast<unsigned char>(word[0]));
            }
            if (!result.empty()) result += " ";
            result += word;
        }

        return result;
    }

    string toLower(const string& str) {
        string res = str;
        transform(res.begin(), res.end(), res.begin(),
                  [](unsigned char c) { return tolower(c); });
        return res;
    }

    string toUpper(const string& str) {
        string res = str;
        transform(res.begin(), res.end(), res.begin(),
                  [](unsigned char c) { return toupper(c); });
        return res;
    }

    bool containsIgnoreCase(const string& source, const string& target) {
        string srcLower = toLower(source);
        string tgtLower = toLower(target);
        return srcLower.find(tgtLower) != string::npos;
    }

    bool kiemTraCCCD(const string& cccd) {
        string trimmed = trim(cccd);
        if (trimmed.length() != 12) return false;
        for (char c : trimmed) {
            if (!isdigit(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    static bool laNamNhuan(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    bool kiemTraNgaySinh(const string& dateStr) {
        string s = trim(dateStr);
        if (s.length() != 10) return false;
        if (s[2] != '/' || s[5] != '/') return false;

        for (int i = 0; i < 10; ++i) {
            if (i == 2 || i == 5) continue;
            if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
        }

        int d = stoi(s.substr(0, 2));
        int m = stoi(s.substr(3, 2));
        int y = stoi(s.substr(6, 4));

        if (y < 1900 || y > 2026) return false;
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
        return (s == "nam" || s == "nu" || s == "khac");
    }

    bool tachKeyValue(const string& line, string& key, string& value) {
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) return false;

        key = trim(line.substr(0, colonPos));
        value = trim(line.substr(colonPos + 1));
        return (!key.empty());
    }

    double toDouble(const string& str, double defVal) {
        string s = trim(str);
        if (s.empty()) return defVal;
        try {
            return stod(s);
        } catch (...) {
            return defVal;
        }
    }

    int toInt(const string& str, int defVal) {
        string s = trim(str);
        if (s.empty()) return defVal;
        try {
            return stoi(s);
        } catch (...) {
            return defVal;
        }
    }
}
