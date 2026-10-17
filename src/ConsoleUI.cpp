#include "../include/ConsoleUI.h"
#include <windows.h>
#include <conio.h>
#include <iomanip>

namespace ConsoleUI {

    void setupConsole() {
        // Đặt bảng mã UTF-8 cho Console trên Windows
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // Đặt tiêu đề cho cửa sổ console
        SetConsoleTitleA("HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2");

        // Đặt kích thước buffer và font chữ thích hợp
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }

    void setColor(int textColor, int bgColor) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, (WORD)((bgColor << 4) | textColor));
    }

    void resetColor() {
        setColor(COLOR_WHITE, COLOR_BLACK);
    }

    void gotoxy(int x, int y) {
        COORD coord;
        coord.X = (SHORT)x;
        coord.Y = (SHORT)y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    }

    void setCursorVisibility(bool visible) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = visible ? TRUE : FALSE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }

    void clearScreen() {
        system("cls");
    }

    void pauseScreen(const string& message) {
        setColor(COLOR_DARK_YELLOW);
        cout << "\n  >> " << message;
        resetColor();
        _getch();
        cout << endl;
    }

    void printLine(char c, int length) {
        for (int i = 0; i < length; ++i) {
            cout << c;
        }
        cout << endl;
    }

    void printTitle(const string& title) {
        clearScreen();
        int width = 95;
        setColor(COLOR_CYAN);
        cout << "\n  ";
        printLine('=', width);
        cout << "  ||";
        
        int pad = (width - 4 - (int)title.length()) / 2;
        if (pad < 0) pad = 0;
        for (int i = 0; i < pad; ++i) cout << " ";
        setColor(COLOR_YELLOW);
        cout << title;
        setColor(COLOR_CYAN);
        for (int i = 0; i < width - 4 - pad - (int)title.length(); ++i) cout << " ";
        cout << "||\n  ";
        printLine('=', width);
        resetColor();
        cout << endl;
    }

    void printSuccess(const string& message) {
        setColor(COLOR_GREEN);
        cout << "\n  [THANH CONG] " << message << "\n";
        resetColor();
    }

    void printError(const string& message) {
        setColor(COLOR_RED);
        cout << "\n  [LOI] " << message << "\n";
        resetColor();
    }

    void printWarning(const string& message) {
        setColor(COLOR_YELLOW);
        cout << "\n  [CHU Y] " << message << "\n";
        resetColor();
    }

    int showInteractiveMenu(const string& title, const string options[], int numOptions) {
        int selected = 0;
        setCursorVisibility(false);

        while (true) {
            printTitle(title);
            setColor(COLOR_GRAY);
            cout << "  (Su dung cac phim mui ten LEN/XUONG de di chuyen, ENTER de chon, ESC de thoat)\n\n";

            for (int i = 0; i < numOptions; ++i) {
                if (i == selected) {
                    setColor(COLOR_BLACK, COLOR_CYAN); // Highlight mục được chọn
                    cout << "   -> [ " << (i + 1) << ". " << options[i] << " ]";
                    resetColor();
                    cout << "\n";
                } else {
                    setColor(COLOR_WHITE);
                    cout << "       " << (i + 1) << ". " << options[i] << "\n";
                    resetColor();
                }
            }

            cout << "\n  ";
            setColor(COLOR_DARK_GRAY);
            printLine('-', 65);
            resetColor();

            int key = _getch();
            if (key == 224 || key == 0) { // Phím mũi tên / Function key
                key = _getch();
                if (key == 72) { // Mũi tên LÊN
                    selected--;
                    if (selected < 0) selected = numOptions - 1;
                } else if (key == 80) { // Mũi tên XUỐNG
                    selected++;
                    if (selected >= numOptions) selected = 0;
                }
            } else if (key == 13) { // Phím ENTER
                setCursorVisibility(true);
                return selected;
            } else if (key == 27) { // Phím ESC
                setCursorVisibility(true);
                return numOptions - 1; // Thường tùy chọn cuối là Thoát / Quay lại
            } else if (key >= '1' && key < '1' + numOptions) {
                // Nhập số trực tiếp
                setCursorVisibility(true);
                return key - '1';
            }
        }
    }
}
