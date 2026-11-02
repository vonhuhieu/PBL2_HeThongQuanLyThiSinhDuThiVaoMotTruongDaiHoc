#include "../include/QuanLyThiSinh.h"
#include "../include/ConsoleUI.h"
#include "../include/Utils.h"
#include "../include/AppGUI.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

// In báº£ng danh sÃ¡ch thÃ­ sinh
static void inBangThiSinh(const Vector<ThiSinh>& ds, const QuanLyThiSinh& ql, bool hienThiKetQua = false) {
    if (ds.empty()) {
        ConsoleUI::printWarning("Danh sach hien tai dang trong!");
        return;
    }

    ConsoleUI::setColor(ConsoleColor::COLOR_CYAN);
    cout << "\n  " << left << setw(5) << "STT"
         << setw(9) << "SBD"
         << setw(14) << "So CCCD"
         << setw(22) << "Ho va Ten"
         << setw(12) << "Ngay Sinh"
         << setw(16) << "Que Quan"
         << setw(6)  << "Phai"
         << right
         << setw(6) << "Toan"
         << setw(6) << "Ly"
         << setw(6) << "Hoa"
         << setw(7) << "Tong";
    if (hienThiKetQua) {
        cout << "   " << left << setw(14) << "Ket Qua";
    }
    cout << "\n  ";
    ConsoleUI::printLine('-', hienThiKetQua ? 116 : 98);
    ConsoleUI::resetColor();

    for (size_t i = 0; i < ds.size(); ++i) {
        const ThiSinh& ts = ds[i];
        string tenQue = ql.layTenTinhThanh(ts.getQueQuan());
        string tenGT  = ql.layTenGioiTinh(ts.getGioiTinh());
        cout << "  " << right << setw(3) << (i + 1) << "  "
             << left  << setw(9) << ts.getSoBaoDanh()
             << setw(14) << ts.getSoCCCD()
             << setw(22) << ts.getHoTen().substr(0, 20)
             << setw(12) << ts.getNgaySinh()
             << setw(16) << tenQue.substr(0, 14)
             << setw(6)  << tenGT
             << right << fixed << setprecision(2)
             << setw(6) << ts.getDiemToan()
             << setw(6) << ts.getDiemLy()
             << setw(6) << ts.getDiemHoa()
             << setw(7) << ts.tinhTongDiem();

        if (hienThiKetQua) {
            bool pass = ts.xetTrungTuyen(ql.getDiemChuan());
            if (pass) {
                ConsoleUI::setColor(ConsoleColor::COLOR_GREEN);
                cout << "   " << left << setw(14) << AdmissionConst::KET_QUA_TRUNG_TUYEN;
            } else {
                ConsoleUI::setColor(ConsoleColor::COLOR_RED);
                cout << "   " << left << setw(14) << (ts.kiemTraDiemLiet() ? "Diem liet" : AdmissionConst::KET_QUA_KHONG_DAT);
            }
            ConsoleUI::resetColor();
        }
        cout << "\n";
    }
    cout << "  ";
    ConsoleUI::printLine('=', hienThiKetQua ? 116 : 98);
    cout << "  Tong cong: " << ds.size() << " thi sinh.\n";
}

// Chá»©c nÄƒng 1: Hiá»ƒn thá»‹ toÃ n bá»™ danh sÃ¡ch
static void chucNangHienThi(QuanLyThiSinh& ql) {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TOAN BO DANH SACH THI SINH DU THI");
    inBangThiSinh(ql.getDanhSach(), ql, false);
    ConsoleUI::pauseScreen();
}

// Chá»©c nÄƒng 2: ThÃªm thÃ­ sinh má»›i
static void chucNangThem(QuanLyThiSinh& ql) {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("TIEP NHAN HO SO THI SINH MOI");

    string sbd, cccd, hoten, ns, que, gt;
    double toan = 0.0, ly = 0.0, hoa = 0.0;

    cout << "  [1/8] Nhap So Bao Danh (SBD, vd: DUT015): ";
    getline(cin, sbd);
    sbd = Utils::toUpper(Utils::trim(sbd));
    if (!Utils::kiemTraSBD(sbd)) {
        ConsoleUI::printError("So bao danh khong hop le (tu 3-15 ky tu, khong co dau cach)!");
        ConsoleUI::pauseScreen();
        return;
    }
    if (ql.timViTriTheoSBD(sbd) != -1) {
        ConsoleUI::printError("So bao danh nay da ton tai trong he thong!");
        ConsoleUI::pauseScreen();
        return;
    }

    cout << "  [2/8] Nhap So CCCD (dung 12 chu so)     : ";
    getline(cin, cccd);
    cccd = Utils::trim(cccd);
    if (!Utils::kiemTraCCCD(cccd)) {
        ConsoleUI::printError("So CCCD bat buoc phai gom dung 12 chu so!");
        ConsoleUI::pauseScreen();
        return;
    }
    if (ql.timViTriTheoCCCD(cccd) != -1) {
        ConsoleUI::printError("So CCCD nay da duoc dang ky boi mot thi sinh khac!");
        ConsoleUI::pauseScreen();
        return;
    }

    cout << "  [3/8] Nhap Ho va Ten thi sinh           : ";
    getline(cin, hoten);
    hoten = Utils::chuanHoaTen(hoten);
    if (hoten.length() < 2) {
        ConsoleUI::printError("Ho va ten khong duoc de trong!");
        ConsoleUI::pauseScreen();
        return;
    }

    cout << "  [4/8] Nhap Ngay sinh (dd/mm/yyyy)       : ";
    getline(cin, ns);
    ns = Utils::trim(ns);
    if (!Utils::kiemTraNgaySinh(ns)) {
        ConsoleUI::printError("Ngay sinh khong hop le hoac sai dinh dang dd/mm/yyyy!");
        ConsoleUI::pauseScreen();
        return;
    }

    cout << "  [5/8] Nhap Que quan (Ma hoac Ten tinh)  : ";
    getline(cin, que);
    que = ql.layMaTinhThanh(que);

    cout << "  [6/8] Nhap Gioi tinh (NAM / NU / KHAC)  : ";
    getline(cin, gt);
    gt = ql.layMaGioiTinh(gt);

    cout << "  [7/8] Nhap diem thi mon Toan (0 - 10)   : ";
    cin >> toan;
    cout << "  [8/8] Nhap diem thi mon Ly   (0 - 10)   : ";
    cin >> ly;
    cout << "        Nhap diem thi mon Hoa  (0 - 10)   : ";
    cin >> hoa;
    cin.ignore();

    if (!Utils::kiemTraDiem(toan) || !Utils::kiemTraDiem(ly) || !Utils::kiemTraDiem(hoa)) {
        ConsoleUI::printError("Diem cac mon thi phai nam trong doan [0.00, 10.00]!");
        ConsoleUI::pauseScreen();
        return;
    }

    ThiSinh tsMoi(cccd, hoten, ns, que, gt, sbd, toan, ly, hoa);
    if (ql.themThiSinh(tsMoi)) {
        ConsoleUI::printSuccess("Da tiep nhan va luu ho so thi sinh vao CSDL thanh cong!");
    } else {
        ConsoleUI::printError("Khong the them thi sinh do loi trung lap du lieu!");
    }
    ConsoleUI::pauseScreen();
}

// Chá»©c nÄƒng 3: Sá»­a thÃ´ng tin thÃ­ sinh
static void chucNangSua(QuanLyThiSinh& ql) {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("CAP NHAT THONG TIN THI SINH");

    cout << "  Nhap SBD hoac So CCCD cua thi sinh can sua: ";
    string ma;
    getline(cin, ma);
    ma = Utils::trim(ma);

    int idx = ql.timViTriTheoSBD(ma);
    if (idx == -1) idx = ql.timViTriTheoCCCD(ma);

    if (idx == -1) {
        ConsoleUI::printError("Khong tim thay thi sinh phu hop trong he thong!");
        ConsoleUI::pauseScreen();
        return;
    }

    ThiSinh& ts = ql.getDanhSach()[idx];
    cout << "\n  --- THONG TIN HIEN TAI ---\n";
    cout << "  SBD       : " << ts.getSoBaoDanh() << "\n"
         << "  So CCCD   : " << ts.getSoCCCD() << "\n"
         << "  Ho va Ten : " << ts.getHoTen() << "\n"
         << "  Ngay Sinh : " << ts.getNgaySinh() << "\n"
         << "  Que Quan  : " << ts.getQueQuan() << "\n"
         << "  Gioi Tinh : " << ts.getGioiTinh() << "\n"
         << fixed << setprecision(2)
         << "  Diem Toan : " << ts.getDiemToan() << " | Ly: " << ts.getDiemLy() << " | Hoa: " << ts.getDiemHoa() << "\n";
    cout << "\n  (Nhan Enter neu muon GIU NGUYEN gia tri cu)\n";

    string buf;
    cout << "  Ho va Ten moi  : ";
    getline(cin, buf);
    if (!Utils::trim(buf).empty()) ts.setHoTen(Utils::chuanHoaTen(buf));

    cout << "  Ngay sinh moi  : ";
    getline(cin, buf);
    if (!Utils::trim(buf).empty() && Utils::kiemTraNgaySinh(buf)) ts.setNgaySinh(Utils::trim(buf));

    cout << "  Que quan moi   : ";
    getline(cin, buf);
    if (!Utils::trim(buf).empty()) ts.setQueQuan(Utils::chuanHoaTen(buf));

    cout << "  Gioi tinh moi  : ";
    getline(cin, buf);
    if (!Utils::trim(buf).empty() && Utils::kiemTraGioiTinh(buf)) ts.setGioiTinh(Utils::chuanHoaTen(buf));

    cout << "  Ban co muon sua diem thi khong? (y/n): ";
    getline(cin, buf);
    if (Utils::toLower(Utils::trim(buf)) == "y") {
        double t, l, h;
        cout << "  Diem Toan moi: "; cin >> t;
        cout << "  Diem Ly moi  : "; cin >> l;
        cout << "  Diem Hoa moi : "; cin >> h;
        cin.ignore();
        if (Utils::kiemTraDiem(t) && Utils::kiemTraDiem(l) && Utils::kiemTraDiem(h)) {
            ts.setDiemToan(t);
            ts.setDiemLy(l);
            ts.setDiemHoa(h);
        } else {
            ConsoleUI::printWarning("Diem khong hop le, giu nguyen diem cu!");
        }
    }

    ql.luuDuLieuVaoMySQL();
    ConsoleUI::printSuccess("Cap nhat ho so thi sinh thanh cong!");
    ConsoleUI::pauseScreen();
}

// Chá»©c nÄƒng 4: XÃ³a thÃ­ sinh
static void chucNangXoa(QuanLyThiSinh& ql) {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("XOA HO SO THI SINH (CASCADE DELETE)");

    cout << "  Nhap SBD hoac So CCCD cua thi sinh can xoa: ";
    string ma;
    getline(cin, ma);
    ma = Utils::trim(ma);

    int idx = ql.timViTriTheoSBD(ma);
    if (idx == -1) idx = ql.timViTriTheoCCCD(ma);

    if (idx == -1) {
        ConsoleUI::printError("Khong tim thay thi sinh phu hop!");
        ConsoleUI::pauseScreen();
        return;
    }

    const ThiSinh& ts = ql.getDanhSach()[idx];
    ConsoleUI::setColor(ConsoleColor::COLOR_RED);
    cout << "\n  [CANH BAO] Ban co chac chan muon xoa thi sinh sau khoi CSDL?\n";
    ConsoleUI::resetColor();
    cout << "  SBD    : " << ts.getSoBaoDanh() << "\n"
         << "  Ho ten : " << ts.getHoTen() << "\n"
         << "  CCCD   : " << ts.getSoCCCD() << "\n"
         << "  Nhap 'Y' de XAC NHAN xoa vinh vien, phim bat ky de HUY: ";

    string xacNhan;
    getline(cin, xacNhan);
    if (Utils::toUpper(Utils::trim(xacNhan)) == "Y") {
        ql.xoaThiSinh(ts.getSoBaoDanh());
        ConsoleUI::printSuccess("Da xoa thi sinh va dong bo CSDL thanh cong!");
    } else {
        ConsoleUI::printWarning("Da huy thao tac xoa. Du lieu duoc giu nguyen.");
    }
    ConsoleUI::pauseScreen();
}

// Chá»©c nÄƒng 5: Menu tÃ¬m kiáº¿m
static void chucNangTimKiem(QuanLyThiSinh& ql) {
    const string options[] = {
        "Tim kiem chinh xac theo So Bao Danh (SBD)",
        "Tim kiem theo So Can cuoc Cong dan (CCCD)",
        "Tim kiem gan dung theo Ho va Ten",
        "Tim kiem theo Que quan / Tinh thanh",
        "Quay lai Menu chinh"
    };

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("TRA CUU & TIM KIEM THI SINH", options, 5);
        if (choice == 0) {
            ConsoleUI::clearScreen();
            cout << "  Nhap SBD can tim: ";
            string sbd; getline(cin, sbd);
            int idx = ql.timViTriTheoSBD(sbd);
            if (idx != -1) {
                Vector<ThiSinh> motTS;
                motTS.push_back(ql.getDanhSach()[idx]);
                inBangThiSinh(motTS, ql, true);
            } else {
                ConsoleUI::printError("Khong tim thay thi sinh co SBD nay!");
            }
            ConsoleUI::pauseScreen();
        } else if (choice == 1) {
            ConsoleUI::clearScreen();
            cout << "  Nhap So CCCD can tim: ";
            string cccd; getline(cin, cccd);
            int idx = ql.timViTriTheoCCCD(cccd);
            if (idx != -1) {
                Vector<ThiSinh> motTS;
                motTS.push_back(ql.getDanhSach()[idx]);
                inBangThiSinh(motTS, ql, true);
            } else {
                ConsoleUI::printError("Khong tim thay thi sinh co so CCCD nay!");
            }
            ConsoleUI::pauseScreen();
        } else if (choice == 2) {
            ConsoleUI::clearScreen();
            cout << "  Nhap Ho ten (hoac tu khoa can tim): ";
            string ten; getline(cin, ten);
            Vector<ThiSinh> kq = ql.timTheoTen(ten);
            inBangThiSinh(kq, ql, true);
            ConsoleUI::pauseScreen();
        } else if (choice == 3) {
            ConsoleUI::clearScreen();
            cout << "  Nhap QuÃª quÃ¡n cáº§n tÃ¬m: ";
            string que; getline(cin, que);
            Vector<ThiSinh> kq = ql.timTheoQueQuan(que);
            inBangThiSinh(kq, ql, true);
            ConsoleUI::pauseScreen();
        } else {
            return;
        }
    }
}

// Chá»©c nÄƒng 6: Menu sáº¯p xáº¿p
static void chucNangSapXep(QuanLyThiSinh& ql) {
    const string options[] = {
        "Sap xep theo Tong diem 3 mon (Giam dan)",
        "Sap xep theo Ten thi sinh (A -> Z theo quy uoc tieng Viet)",
        "Sap xep theo So Bao Danh (Tang dan)",
        "Quay lai Menu chinh"
    };

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("SAP XEP DANH SACH (QUICKSORT)", options, 4);
        if (choice == 0) {
            ql.sapXepTheoTongDiemGiamDan();
            ConsoleUI::printSuccess("Da sap xep danh sach theo Tong diem giam dan!");
            inBangThiSinh(ql.getDanhSach(), ql, true);
            ConsoleUI::pauseScreen();
        } else if (choice == 1) {
            ql.sapXepTheoTenAZ();
            ConsoleUI::printSuccess("Da sap xep danh sach theo Ten thi sinh A -> Z!");
            inBangThiSinh(ql.getDanhSach(), ql, false);
            ConsoleUI::pauseScreen();
        } else if (choice == 2) {
            ql.sapXepTheoSBDTangDan();
            ConsoleUI::printSuccess("Da sap xep danh sach theo So Bao Danh tang dan!");
            inBangThiSinh(ql.getDanhSach(), ql, false);
            ConsoleUI::pauseScreen();
        } else {
            return;
        }
    }
}

// Chá»©c nÄƒng 7: Menu xÃ©t tuyá»ƒn & Thá»‘ng kÃª
static void chucNangXetTuyen(QuanLyThiSinh& ql) {
    const string options[] = {
        "Thiet lap Diem chuan xet tuyen cua truong",
        "Xem danh sach thi sinh TRUNG TUYEN",
        "Vinh danh THU KHOA ky thi",
        "Thong ke Pho diem thi & Ty le trung tuyen",
        "Quay lai Menu chinh"
    };

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("XET TUYEN & THONG KE KY THI", options, 5);
        if (choice == 0) {
            ConsoleUI::clearScreen();
            cout << "  Diem chuan hien tai: " << fixed << setprecision(2) << ql.getDiemChuan() << " diem\n";
            cout << "  Nhap Diem chuan moi : ";
            double dc; cin >> dc; cin.ignore();
            if (dc >= 0.0 && dc <= 30.0) {
                ql.setDiemChuan(dc);
                ConsoleUI::printSuccess("Da cap nhat Diem chuan xet tuyen moi!");
            } else {
                ConsoleUI::printError("Diem chuan phai tu 0.00 den 30.00!");
            }
            ConsoleUI::pauseScreen();
        } else if (choice == 1) {
            ConsoleUI::clearScreen();
            ConsoleUI::printTitle("DANH SACH THI SINH TRUNG TUYEN");
            cout << "  (Diem chuan: " << fixed << setprecision(2) << ql.getDiemChuan() << " diem)\n";
            Vector<ThiSinh> dsTT = ql.getDanhSachTrungTuyen(ql.getDiemChuan());
            inBangThiSinh(dsTT, ql, true);
            ConsoleUI::pauseScreen();
        } else if (choice == 2) {
            ConsoleUI::clearScreen();
            ConsoleUI::printTitle("VINH DANH THU KHOA KY THI TUYEN SINH");
            Vector<ThiSinh> dsTK = ql.getDanhSachThuKhoa();
            if (dsTK.empty()) {
                ConsoleUI::printWarning("Chua co thi sinh nao!");
            } else {
                ConsoleUI::setColor(ConsoleColor::COLOR_YELLOW);
                cout << "\n  â˜… â˜… â˜… CHUC MUNG THU KHOA â˜… â˜… â˜…\n\n";
                ConsoleUI::resetColor();
                for (size_t i = 0; i < dsTK.size(); ++i) {
                    cout << "  - Ho va Ten : " << dsTK[i].getHoTen() << "\n"
                         << "    SBD       : " << dsTK[i].getSoBaoDanh() << "\n"
                         << "    So CCCD   : " << dsTK[i].getSoCCCD() << "\n"
                         << "    Que quan  : " << dsTK[i].getQueQuan() << "\n"
                         << fixed << setprecision(2)
                         << "    Diem 3 mon: Toan " << dsTK[i].getDiemToan() 
                         << " | Ly " << dsTK[i].getDiemLy() 
                         << " | Hoa " << dsTK[i].getDiemHoa() << "\n"
                         << "    TONG DIEM : " << dsTK[i].tinhTongDiem() << " diem\n\n";
                }
            }
            ConsoleUI::pauseScreen();
        } else if (choice == 3) {
            ConsoleUI::clearScreen();
            ConsoleUI::printTitle("THONG KE PHO DIEM & TY LE TRUNG TUYEN");
            int thap, tb, kha, cao, ratCao;
            ql.tinhThongKePhoDiem(thap, tb, kha, cao, ratCao);
            size_t total = ql.getDanhSach().size();

            cout << "  Phan bo diem thi 3 mon (Toan + Ly + Hoa):\n\n";
            auto inMuc = [&](const string& label, int count, ConsoleColor col) {
                ConsoleUI::setColor(col);
                cout << "  " << left << setw(26) << label << ": " << setw(4) << count;
                double pct = total > 0 ? (count * 100.0 / total) : 0.0;
                cout << " (" << fixed << setprecision(1) << setw(5) << pct << "%)  ";
                int bars = (int)(pct / 5);
                for (int b = 0; b < bars; ++b) cout << (char)219;
                cout << "\n";
                ConsoleUI::resetColor();
            };

            inMuc("Rat cao  (>= 27.0 diem) ", ratCao, ConsoleColor::COLOR_YELLOW);
            inMuc("Cao      (24.0 - <27.0) ", cao, ConsoleColor::COLOR_GREEN);
            inMuc("Kha      (21.0 - <24.0) ", kha, ConsoleColor::COLOR_CYAN);
            inMuc("Trung TB (15.0 - <21.0) ", tb, ConsoleColor::COLOR_WHITE);
            inMuc("Thap     (< 15.0 diem)  ", thap, ConsoleColor::COLOR_RED);

            cout << "\n  ";
            ConsoleUI::printLine('-', 65);
            Vector<ThiSinh> dsTT = ql.getDanhSachTrungTuyen(ql.getDiemChuan());
            cout << "  Diem chuan hien tai  : " << fixed << setprecision(2) << ql.getDiemChuan() << " diem\n";
            cout << "  So luong trung tuyen : " << dsTT.size() << " / " << total << " ("
                 << fixed << setprecision(1) << (total > 0 ? dsTT.size() * 100.0 / total : 0.0) << "%)\n";

            ConsoleUI::pauseScreen();
        } else {
            return;
        }
    }
}

// Chá»©c nÄƒng 8: Xuáº¥t bÃ¡o cÃ¡o káº¿t quáº£ xÃ©t tuyá»ƒn
static void chucNangXuatBaoCao(QuanLyThiSinh& ql) {
    ConsoleUI::clearScreen();
    ConsoleUI::printTitle("XUAT BAO CAO KET QUA XET TUYEN RA TEP");

    string path = "data/ketqua_xettuyen.txt";
    if (ql.xuatBaoCaoXetTuyen(path, ql.getDiemChuan())) {
        ConsoleUI::printSuccess("Da xuat bao cao ket qua xet tuyen ra tep: " + path);
        cout << "\n  Ban co the mo tep " << path << " de xem bang bao cao ke khung hoan my!\n";
    } else {
        ConsoleUI::printError("Khong the mo tep de ghi bao cao!");
    }
    ConsoleUI::pauseScreen();
}

// Cháº¿ Ä‘á»™ Console hoÃ n chá»‰nh
static int runConsoleMode(QuanLyThiSinh& ql) {
    ConsoleUI::setupConsole();

    const string mainOptions[] = {
        "Hien thi toan bo danh sach thi sinh",
        "Tiep nhan ho so thi sinh moi",
        "Cap nhat thong tin thi sinh",
        "Xoa ho so thi sinh (Cascade delete)",
        "Tra cuu & Tim kiem thi sinh (SBD, CCCD, Ten, Que)",
        "Sap xep danh sach (QuickSort: Tong diem, Ten A-Z, SBD)",
        "Xet tuyen, Thong ke pho diem & Thu khoa",
        "Xuat bao cao ket qua xet tuyen ra tep (ketqua_xettuyen.txt)",
        "Luu toan bo CSDL (nguoi.txt & thisinh.txt) va Thoat"
    };

    while (true) {
        int choice = ConsoleUI::showInteractiveMenu("HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC (PBL2)", mainOptions, 9);
        switch (choice) {
            case 0: chucNangHienThi(ql); break;
            case 1: chucNangThem(ql); break;
            case 2: chucNangSua(ql); break;
            case 3: chucNangXoa(ql); break;
            case 4: chucNangTimKiem(ql); break;
            case 5: chucNangSapXep(ql); break;
            case 6: chucNangXetTuyen(ql); break;
            case 7: chucNangXuatBaoCao(ql); break;
            case 8:
                ql.luuDuLieuVaoMySQL();
                ConsoleUI::clearScreen();
                ConsoleUI::printSuccess("Du lieu da duoc luu vao CSDL thanh cong. Tam biet!");
                return 0;
        }
    }
}

int main(int argc, char* argv[]) {
    // Khá»Ÿi táº¡o há»‡ quáº£n trá»‹ CSDL quan há»‡ vÃ  nghiá»‡p vá»¥ tuyá»ƒn sinh
    QuanLyThiSinh ql;

    // Cháº¿ Ä‘á»™ kiá»ƒm thá»­ tá»± Ä‘á»™ng khÃ´ng giao diá»‡n
    if (argc > 1 && string(argv[1]) == "--test") {
        cout << "=== PBL2 MySQL Test Mode ===" << endl;
        cout << "MySQL Connected: " << (ql.isMySQLConnected() ? "YES" : "NO (Offline fallback active)") << endl;
        cout << "So luong thi sinh: " << ql.getDanhSach().size() << endl;
        cout << "So luong tinh thanh: " << ql.getDanhSachTinhThanh().size() << endl;
        cout << "So luong gioi tinh: " << ql.getDanhSachGioiTinh().size() << endl;
        cout << "Diem chuan: " << ql.getDiemChuan() << endl;
        cout << "Diem liet: " << ql.getDiemLiet() << endl;
        bool ok = ql.xuatTatCaBaoCao();
        cout << "Xuat bao cao: " << (ok ? "THANH CONG" : "THAT BAI") << endl;
        return 0;
    }

    // Náº¿u ngÆ°á»i dÃ¹ng cháº¡y vá»›i tham sá»‘ --console, chuyá»ƒn sang Console
    if (argc > 1 && string(argv[1]) == "--console") {
        return runConsoleMode(ql);
    }

    // Máº·c Ä‘á»‹nh: Khá»Ÿi cháº¡y Giao diá»‡n Äá»“ há»a Hiá»‡n Ä‘áº¡i (Dear ImGui DirectX 9)
    return AppGUI::run(ql);
}
