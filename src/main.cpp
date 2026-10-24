#include "../include/QuanLyThiSinh.h"
#include "../include/AppGUI.h"
#include "../include/ConsoleUI.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

// Menu tìm kiếm Console (Fallback)
static void menuTimKiemConsole(QuanLyThiSinh& ql) {
    const string options[] = {
        "Tim kiem theo So Bao Danh",
        "Tim kiem theo Ho va Ten thi sinh",
        "Tim kiem theo Que quan / Tinh thanh",
        "Quay lai Menu chinh"
    };
    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("MENU TIM KIEM THI SINH", options, 4);
        if (choice == 0) {
            ConsoleUI::clearScreen();
            cout << "  Nhap SBD can tim: ";
            string sbd;
            getline(cin, sbd);
            int idx = ql.timViTriTheoSBD(sbd);
            if (idx != -1) {
                cout << "  Tim thay: " << ql.getDanhSachThiSinh()[idx].getHoTen() 
                     << " | Nganh: " << ql.getTenNganh(ql.getDanhSachThiSinh()[idx].getMaNganh())
                     << " | Tong: " << ql.getDanhSachThiSinh()[idx].tinhTongDiem() << "\n";
            } else {
                cout << "  Khong tim thay!\n";
            }
            ConsoleUI::pauseScreen();
        } else if (choice == 3) {
            return;
        }
    }
}

// Chế độ Console dự phòng
static int runConsoleMode(QuanLyThiSinh& ql) {
    ConsoleUI::setupConsole();
    const string mainOptions[] = {
        "Xem toan bo danh sach thi sinh (CSDL 3NF)",
        "Tim kiem thi sinh",
        "Sap xep danh sach theo Tong Diem giam dan",
        "Vinh danh Thu Khoa ky thi",
        "Xuat bao cao ket qua xet tuyen ra tep",
        "Thoat chuong trinh"
    };

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("HE THONG QUAN LY THI SINH (CHE DO CONSOLE)", mainOptions, 6);
        switch (choice) {
            case 0: {
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("DANH SACH THI SINH DANG KY");
                cout << left << setw(8) << "Ma TS" << setw(9) << "SBD" << setw(20) << "Ho va Ten" 
                     << setw(14) << "Que Quan" << setw(26) << "Nganh" << setw(8) << "Tong" << "Ket qua\n";
                cout << string(95, '-') << "\n";
                for (int i = 0; i < ql.getDanhSachThiSinh().size(); ++i) {
                    const ThiSinh& ts = ql.getDanhSachThiSinh()[i];
                    double dc = ql.getDiemChuanNganh(ts.getMaNganh());
                    cout << left << setw(8) << ts.getMaThiSinh()
                         << setw(9) << ts.getSoBaoDanh()
                         << setw(20) << ts.getHoTen().substr(0, 18)
                         << setw(14) << ts.getQueQuan().substr(0, 12)
                         << setw(26) << ql.getTenNganh(ts.getMaNganh()).substr(0, 24)
                         << right << fixed << setprecision(2) << setw(6) << ts.tinhTongDiem()
                         << "   " << left << (ts.xetTrungTuyen(dc) ? "TRUNG TUYEN" : "Khong dat") << "\n";
                }
                ConsoleUI::pauseScreen();
                break;
            }
            case 1:
                menuTimKiemConsole(ql);
                break;
            case 2:
                ql.sapXepTheoTongDiemGiamDan();
                ConsoleUI::printSuccess("Da sap xep danh sach theo tong diem giam dan!");
                ConsoleUI::pauseScreen();
                break;
            case 3: {
                ConsoleUI::clearScreen();
                ConsoleUI::printTitle("VINH DANH THU KHOA");
                Vector<ThiSinh> dsTK = ql.getDanhSachThuKhoa();
                for (int i = 0; i < dsTK.size(); ++i) {
                    cout << "  ★ Thu khoa: " << dsTK[i].getHoTen() << " (SBD: " << dsTK[i].getSoBaoDanh() 
                         << ") - Tong diem: " << dsTK[i].tinhTongDiem() << "\n";
                }
                ConsoleUI::pauseScreen();
                break;
            }
            case 4:
                ql.xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt");
                ConsoleUI::printSuccess("Da xuat bao cao ket qua ra data/ketqua_xettuyen.txt!");
                ConsoleUI::pauseScreen();
                break;
            case 5:
                ql.luuTatCaDuLieu();
                return 0;
        }
    }
}

int main(int argc, char* argv[]) {
    // Khởi tạo hệ quản trị cơ sở dữ liệu và nghiệp vụ tuyển sinh
    QuanLyThiSinh ql("data");

    // Nếu người dùng chạy với tham số --console, chuyển sang Console
    if (argc > 1 && string(argv[1]) == "--console") {
        return runConsoleMode(ql);
    }

    // Mặc định: Khởi chạy Giao diện Đồ họa Hiện đại (Dear ImGui DirectX 9)
    return AppGUI::run(ql);
}
