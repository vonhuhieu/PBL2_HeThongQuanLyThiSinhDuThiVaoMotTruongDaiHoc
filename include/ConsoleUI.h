#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <iostream>
#include <string>

using namespace std;

// Mã màu sắc console trên Windows
enum ConsoleColor {
    COLOR_BLACK = 0,
    COLOR_DARK_BLUE = 1,
    COLOR_DARK_GREEN = 2,
    COLOR_DARK_CYAN = 3,
    COLOR_DARK_RED = 4,
    COLOR_DARK_MAGENTA = 5,
    COLOR_DARK_YELLOW = 6,
    COLOR_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_BLUE = 9,
    COLOR_GREEN = 10,
    COLOR_CYAN = 11,
    COLOR_RED = 12,
    COLOR_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
};

namespace ConsoleUI {
    // Thiết lập font chữ UTF-8 và kích thước cửa sổ
    void setupConsole();

    // Đổi màu chữ và màu nền
    void setColor(int textColor, int bgColor = COLOR_BLACK);

    // Trả lại màu mặc định
    void resetColor();

    // Di chuyển con trỏ tới tọa độ (x, y)
    void gotoxy(int x, int y);

    // Ẩn/Hiện con trỏ soạn thảo
    void setCursorVisibility(bool visible);

    // Xóa toàn bộ màn hình
    void clearScreen();

    // Dừng màn hình chờ nhấn phím
    void pauseScreen(const string& message = "Nhan phim bat ky de tiep tuc...");

    // Vẽ tiêu đề có khung nổi bật
    void printTitle(const string& title);

    // Vẽ dòng kẻ ngang trang trí
    void printLine(char c = '=', int length = 85);

    // Hiển thị thông báo thành công / lỗi
    void printSuccess(const string& message);
    void printError(const string& message);
    void printWarning(const string& message);

    // Menu tương tác điều hướng bằng phím mũi tên LÊN/XUỐNG và ENTER
    int showInteractiveMenu(const string& title, const string options[], int numOptions);
}

#endif // CONSOLE_UI_H
