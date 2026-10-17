#include <iostream>
#include <string>
#include "../include/ConsoleUI.h"
#include "../include/QuanLyThiSinh.h"
#include "../include/Utils.h"

using namespace std;

// Menu tìm kiếm
void menuTimKiem(QuanLyThiSinh& ql) {
    const string options[] = {
        "Tim kiem theo So Bao Danh (Chinh xac)",
        "Tim kiem theo Ho va Ten thi sinh (Gan dung)",
        "Tim kiem theo Que quan / Tinh thanh",
        "Tim kiem theo Khoang Tong Diem",
        "Quay lai Menu chinh"
    };
    const int numOptions = sizeof(options) / sizeof(options[0]);

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("MENU TIM KIEM THI SINH", options, numOptions);
        switch (choice) {
            case 0: ql.timKiemTheoSBD(); break;
            case 1: ql.timKiemTheoTen(); break;
            case 2: ql.timKiemTheoQueQuan(); break;
            case 3: ql.timKiemTheoKhoangDiem(); break;
            case 4: return;
        }
    }
}

// Menu sắp xếp
void menuSapXep(QuanLyThiSinh& ql) {
    const string options[] = {
        "Sap xep theo Tong Diem giam dan (Bang xep hang)",
        "Sap xep theo Thu tu Ten A - Z",
        "Sap xep theo So Bao Danh tang dan",
        "Quay lai Menu chinh"
    };
    const int numOptions = sizeof(options) / sizeof(options[0]);

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("MENU SAP XEP DANH SACH", options, numOptions);
        switch (choice) {
            case 0: ql.sapXepTheoTongDiemGiamDan(); break;
            case 1: ql.sapXepTheoTenAZ(); break;
            case 2: ql.sapXepTheoSBDTangDan(); break;
            case 3: return;
        }
    }
}

// Menu thống kê và xét tuyển
void menuThongKe(QuanLyThiSinh& ql) {
    const string options[] = {
        "Vinh danh Thu Khoa ky thi (Diem cao nhat)",
        "Danh sach thi sinh Trung Tuyen (Theo diem chuan)",
        "Thong ke Pho Diem xet tuyen (Ty le % hoc luc)",
        "Thong ke So luong thi sinh theo Que quan",
        "Thiet lap / Thay doi Diem chuan xet tuyen",
        "Quay lai Menu chinh"
    };
    const int numOptions = sizeof(options) / sizeof(options[0]);

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("MENU THONG KE & XET TUYEN", options, numOptions);
        switch (choice) {
            case 0: ql.vinhDanhThuKhoa(); break;
            case 1: ql.danhSachTrungTuyen(); break;
            case 2: ql.thongKePhoDiem(); break;
            case 3: ql.thongKeTheoQueQuan(); break;
            case 4: ql.thietLapDiemChuan(); break;
            case 5: return;
        }
    }
}

int main() {
    // Khởi tạo cài đặt Console Windows
    ConsoleUI::setupConsole();

    // Khởi tạo đối tượng quản lý
    QuanLyThiSinh ql("data/thisinh.txt");

    const string mainOptions[] = {
        "Xem toan bo danh sach thi sinh",
        "Them thi sinh moi vao he thong",
        "Tim kiem thi sinh (SBD, Ten, Que quan, Diem)",
        "Cap nhat / Chinh sua thong tin thi sinh",
        "Xoa thi sinh khoi he thong (Theo SBD)",
        "Sap xep danh sach thi sinh",
        "Thong ke, Vinh danh Thu Khoa & Ket qua xet tuyen",
        "Xuat bao cao ket qua xet tuyen ra tap tin",
        "Thoat chuong trinh & Luu toan bo du lieu"
    };
    const int numMainOptions = sizeof(mainOptions) / sizeof(mainOptions[0]);

    bool running = true;
    while (running) {
        int choice = ConsoleUI::showInteractiveMenu("HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC", 
                                                    mainOptions, numMainOptions);

        switch (choice) {
            case 0: // Xem toàn bộ
                ql.hienThiTatCa();
                break;

            case 1: // Thêm mới
                ql.themMoiThiSinh();
                break;

            case 2: // Tìm kiếm
                menuTimKiem(ql);
                break;

            case 3: { // Sửa
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("CAP NHAT THONG TIN THI SINH");
                cout << "  Nhap So Bao Danh cua thi sinh can sua: ";
                string sbd;
                getline(cin, sbd);
                ql.suaThongTinThiSinh(sbd);
                break;
            }

            case 4: { // Xóa
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("XOA THI SINH");
                cout << "  Nhap So Bao Danh cua thi sinh can xoa: ";
                string sbd;
                getline(cin, sbd);
                ql.xoaThiSinh(sbd);
                ConsoleUI::pauseScreen();
                break;
            }

            case 5: // Sắp xếp
                menuSapXep(ql);
                break;

            case 6: // Thống kê
                menuThongKe(ql);
                break;

            case 7: { // Xuất file báo cáo
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("XUAT BAO CAO KET QUA XET TUYEN");
                string path = "data/ketqua_xettuyen.txt";
                if (ql.xuatBaoCaoXetTuyen(path)) {
                    ConsoleUI::printSuccess("Xuat bao cao thanh cong ra tep: " + path);
                } else {
                    ConsoleUI::printError("Khong the tao file bao cao!");
                }
                ConsoleUI::pauseScreen();
                break;
            }

            case 8: // Thoát
                ql.luuDuLieuVaoFile();
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("KET THUC CHUONG TRINH");
                ConsoleUI::setColor(COLOR_GREEN);
                cout << "\n  [THONG BAO] Du lieu da duoc luu an toan vao 'data/thisinh.txt'.\n";
                cout << "  Cam on ban da su dung chuong trinh! Tam biet!\n\n";
                ConsoleUI::resetColor();
                running = false;
                break;

            default:
                break;
        }
    }

    return 0;
}
