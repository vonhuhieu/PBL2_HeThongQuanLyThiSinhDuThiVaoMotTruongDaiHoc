#include "../include/AppGUI.h"
#include "../include/Utils.h"
#include "../include/imgui/imgui.h"
#include "../include/imgui/imgui_impl_win32.h"
#include "../include/imgui/imgui_impl_dx9.h"

#include <d3d9.h>
#include <tchar.h>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <ctime>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Direct3D Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void ResetDevice();
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Cấu trúc danh mục tỉnh thành chuẩn hóa theo mã số hành chính quốc gia
struct TinhThanhEntry {
    const char* ma;
    const char* ten;
    const char* loai;
};

// Danh mục 34 tỉnh, thành phố trực thuộc Trung ương với mã số chính thức (Nghị quyết 202/2025/QH15)
static const TinhThanhEntry kDanhSach34TinhThanhMoi[] = {
    { "01", "TP. Ha Noi", "Thanh pho truc thuoc Trung uong" },
    { "04", "Cao Bang", "Tinh truc thuoc" },
    { "08", "Tuyen Quang", "Tinh truc thuoc" },
    { "10", "Lao Cai", "Tinh truc thuoc" },
    { "11", "Dien Bien", "Tinh truc thuoc" },
    { "12", "Lai Chau", "Tinh truc thuoc" },
    { "14", "Son La", "Tinh truc thuoc" },
    { "19", "Thai Nguyen", "Tinh truc thuoc" },
    { "20", "Lang Son", "Tinh truc thuoc" },
    { "22", "Quang Ninh", "Tinh truc thuoc" },
    { "25", "Phu Tho", "Tinh truc thuoc" },
    { "27", "Bac Ninh", "Tinh truc thuoc" },
    { "31", "TP. Hai Phong", "Thanh pho truc thuoc Trung uong" },
    { "33", "Hung Yen", "Tinh truc thuoc" },
    { "37", "Ninh Binh", "Tinh truc thuoc" },
    { "38", "Thanh Hoa", "Tinh truc thuoc" },
    { "40", "Nghe An", "Tinh truc thuoc" },
    { "42", "Ha Tinh", "Tinh truc thuoc" },
    { "45", "Quang Tri", "Tinh truc thuoc" },
    { "46", "TP. Hue", "Thanh pho truc thuoc Trung uong" },
    { "48", "TP. Da Nang", "Thanh pho truc thuoc Trung uong" },
    { "51", "Quang Ngai", "Tinh truc thuoc" },
    { "56", "Khanh Hoa", "Tinh truc thuoc" },
    { "64", "Gia Lai", "Tinh truc thuoc" },
    { "66", "Dak Lak", "Tinh truc thuoc" },
    { "68", "Lam Dong", "Tinh truc thuoc" },
    { "72", "Tay Ninh", "Tinh truc thuoc" },
    { "75", "Dong Nai", "Tinh truc thuoc" },
    { "79", "TP. Ho Chi Minh", "Thanh pho truc thuoc Trung uong" },
    { "86", "Vinh Long", "Tinh truc thuoc" },
    { "87", "Dong Thap", "Tinh truc thuoc" },
    { "89", "An Giang", "Tinh truc thuoc" },
    { "92", "TP. Can Tho", "Thanh pho truc thuoc Trung uong" },
    { "96", "Ca Mau", "Tinh truc thuoc" }
};
static const int kSoLuong34TinhThanh = sizeof(kDanhSach34TinhThanhMoi) / sizeof(kDanhSach34TinhThanhMoi[0]);

// Tìm vị trí tỉnh thành trong danh mục 34 tỉnh thành
static int timViTriTinhThanh(const string& tenTinh) {
    for (int i = 0; i < kSoLuong34TinhThanh; ++i) {
        if (Utils::containsIgnoreCase(kDanhSach34TinhThanhMoi[i].ten, tenTinh) || 
            Utils::containsIgnoreCase(tenTinh, kDanhSach34TinhThanhMoi[i].ten)) {
            return i;
        }
    }
    return 20; // Mặc định là TP. Da Nang (index 20: Ma 48)
}

// Searchable Combobox cho 34 tỉnh thành
static bool RenderSearchableComboTinhThanh(const char* comboId, int& selectedIdx) {
    static char filterBuf[64] = "";
    bool changed = false;

    const char* currentLabel = (selectedIdx >= 0 && selectedIdx < kSoLuong34TinhThanh) 
                               ? kDanhSach34TinhThanhMoi[selectedIdx].ten 
                               : "Chon tinh / thanh pho...";

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo(comboId, currentLabel)) {
        if (ImGui::IsWindowAppearing()) {
            filterBuf[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filterTinh", "Tim kiem (VD: Da Nang, Ha Noi...)", filterBuf, sizeof(filterBuf));
        ImGui::Separator();

        string query = Utils::toLower(Utils::trim(filterBuf));
        int matchCount = 0;

        for (int i = 0; i < kSoLuong34TinhThanh; ++i) {
            string itemName = Utils::toLower(kDanhSach34TinhThanhMoi[i].ten);
            if (query.empty() || itemName.find(query) != string::npos) {
                matchCount++;
                bool isSelected = (selectedIdx == i);
                if (ImGui::Selectable(kDanhSach34TinhThanhMoi[i].ten, isSelected)) {
                    selectedIdx = i;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }

        if (matchCount == 0) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  (Khong tim thay tinh/thanh)");
        }

        ImGui::EndCombo();
    }
    return changed;
}

static int getDaysInMonth(int m, int y) {
    if (m == 2) {
        bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        return leap ? 29 : 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
}

static int getFirstDayOfWeek(int m, int y) {
    struct tm t = {};
    t.tm_mday = 1;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    mktime(&t);
    return (t.tm_wday + 6) % 7; // 0 = Mon, ..., 6 = Sun
}

static void getCurrentSystemDate(int& d, int& m, int& y) {
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    if (lt) {
        d = lt->tm_mday;
        m = lt->tm_mon + 1;
        y = lt->tm_year + 1900;
    } else {
        d = 31; m = 10; y = 2026;
    }
}

// Bộ lịch chọn ngày tháng trực quan (Calendar Date Picker Popup)
static void RenderDatePickerCalendar(const char* buttonLabel, const char* popupId, int& outDay, int& outMonth, int& outYear) {
    static int calMonth = 10;
    static int calYear = 2026;

    if (ImGui::Button(buttonLabel)) {
        calMonth = (outMonth >= 1 && outMonth <= 12) ? outMonth : 10;
        calYear = (outYear >= 1950 && outYear <= 2030) ? outYear : 2026;
        ImGui::OpenPopup(popupId);
    }

    if (ImGui::BeginPopup(popupId)) {
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "LICH CHON NGAY SINH");
        ImGui::Separator();

        // Thanh điều hướng Tháng / Năm
        if (ImGui::Button("<<##py")) { calYear--; }
        ImGui::SameLine();
        if (ImGui::Button("<##pm")) { 
            calMonth--; 
            if (calMonth < 1) { calMonth = 12; calYear--; } 
        }
        ImGui::SameLine();
        char headerText[64];
        sprintf(headerText, "Thang %02d / %04d", calMonth, calYear);
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%s", headerText);
        ImGui::SameLine();
        if (ImGui::Button(">##nm")) { 
            calMonth++; 
            if (calMonth > 12) { calMonth = 1; calYear++; } 
        }
        ImGui::SameLine();
        if (ImGui::Button(">>##ny")) { calYear++; }

        ImGui::Separator();

        // 7 ngày trong tuần
        const char* dow[7] = { "T2", "T3", "T4", "T5", "T6", "T7", "CN" };
        if (ImGui::BeginTable("CalDow", 7, ImGuiTableFlags_SizingFixedFit)) {
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetupColumn(dow[i], ImGuiTableColumnFlags_WidthFixed, 32.0f);
            }
            ImGui::TableNextRow();
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetColumnIndex(i);
                if (i >= 5) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", dow[i]);
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", dow[i]);
                }
            }
            ImGui::EndTable();
        }

        // Lưới các ngày trong tháng
        int startDay = getFirstDayOfWeek(calMonth, calYear);
        int maxDays = getDaysInMonth(calMonth, calYear);

        if (ImGui::BeginTable("CalDays", 7, ImGuiTableFlags_SizingFixedFit)) {
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 32.0f);
            }
            ImGui::TableNextRow();

            int col = 0;
            for (int i = 0; i < startDay; ++i) {
                ImGui::TableSetColumnIndex(col++);
            }

            for (int d = 1; d <= maxDays; ++d) {
                if (col == 7) {
                    ImGui::TableNextRow();
                    col = 0;
                }
                ImGui::TableSetColumnIndex(col++);

                char dayLabel[16];
                sprintf(dayLabel, "%d##cd%d", d, d);

                bool isSelected = (d == outDay && calMonth == outMonth && calYear == outYear);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                }

                if (ImGui::Button(dayLabel, ImVec2(30.0f, 24.0f))) {
                    outDay = d;
                    outMonth = calMonth;
                    outYear = calYear;
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        if (ImGui::Button("Hom Nay", ImVec2(90.0f, 26.0f))) {
            int cd, cm, cy;
            getCurrentSystemDate(cd, cm, cy);
            outDay = cd;
            outMonth = cm;
            outYear = cy;
            calMonth = cm;
            calYear = cy;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Dong", ImVec2(80.0f, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// Thiết lập Dark Theme phong cách hiện đại cho ImGui
static void SetupModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;
    style.WindowPadding     = ImVec2(14, 14);
    style.FramePadding      = ImVec2(8, 6);
    style.ItemSpacing       = ImVec2(10, 8);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.ScrollbarSize     = 14.0f;

    colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.94f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.16f, 0.22f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.26f, 0.35f, 0.70f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.19f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.28f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.34f, 0.46f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.09f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.14f, 0.18f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.09f, 0.11f, 0.15f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.09f, 0.11f, 0.15f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.30f, 0.38f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.36f, 0.46f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.25f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.25f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.35f, 0.58f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.46f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.15f, 0.30f, 0.52f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.26f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.42f, 0.60f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.26f, 0.35f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.30f, 0.45f, 0.65f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.35f, 0.55f, 0.80f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.13f, 0.17f, 0.23f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.24f, 0.38f, 0.58f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.18f, 0.35f, 0.58f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.11f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.15f, 0.24f, 0.36f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.14f, 0.18f, 0.26f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.25f, 0.32f, 0.44f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.11f, 0.13f, 0.18f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.13f, 0.16f, 0.22f, 1.00f);
}

namespace AppGUI {

    int run(QuanLyThiSinh& ql) {
        // 1. Tạo cửa sổ Win32
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"PBL2_AdmissionApp", nullptr };
        ::RegisterClassExW(&wc);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int winW = 1280;
        int winH = 760;
        int posX = (screenWidth - winW) / 2;
        int posY = (screenHeight - winH) / 2;

        HWND hwnd = ::CreateWindowW(wc.lpszClassName, 
            L"HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)", 
            WS_OVERLAPPEDWINDOW, posX, posY, winW, winH, nullptr, nullptr, wc.hInstance, nullptr);

        if (!CreateDeviceD3D(hwnd)) {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // 2. Khởi tạo Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        SetupModernDarkTheme();

        // Tải font Segoe UI hoặc Arial nếu có
        ImFont* font = nullptr;
        if (GetFileAttributesA("C:\\Windows\\Fonts\\segoeui.ttf") != INVALID_FILE_ATTRIBUTES) {
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);
        } else if (GetFileAttributesA("C:\\Windows\\Fonts\\arial.ttf") != INVALID_FILE_ATTRIBUTES) {
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.5f);
        }
        if (!font) {
            io.Fonts->AddFontDefault();
        }

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(g_pd3dDevice);

        // State cho ứng dụng
        static char filterBuf[128] = "";
        static int filterStatus = 0; // 0: Tat ca, 1: Trung tuyen, 2: Khong dat
        static int selectedRow = -1;

        // State modals
        static bool showAddModal = false;
        static bool showEditModal = false;
        static bool showDeleteModal = false;
        static bool showToast = false;
        static string toastMsg = "";

        // Form buffers & Date controls
        static char bufSBD[32] = "";
        static char bufCCCD[32] = "";
        static char bufHoTen[64] = "";
        static int  formNgay = 15;
        static int  formThang = 5;
        static int  formNam = 2005;
        static int  formQueQuanIdx = 14; // Mặc định là Đà Nẵng
        static int  formGioiTinh = 0;    // 0: Nam, 1: Nu
        static float formToan = 8.0f;
        static float formLy = 8.0f;
        static float formHoa = 8.0f;
        static string formErrorMsg = "";

        // Preview báo cáo file
        static int selectedReportFile = 0; // 0: ketqua_xettuyen.txt, 1: thukhoa.txt, 2: thongke_phodiem.txt
        static string reportContent = "";

        // Main Loop
        bool done = false;
        while (!done) {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done) break;

            if (g_DeviceLost) {
                HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
                if (hr == D3DERR_DEVICELOST) {
                    ::Sleep(10);
                    continue;
                }
                if (hr == D3DERR_DEVICENOTRESET)
                    ResetDevice();
                g_DeviceLost = false;
            }

            if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
                g_d3dpp.BackBufferWidth = g_ResizeWidth;
                g_d3dpp.BackBufferHeight = g_ResizeHeight;
                g_ResizeWidth = g_ResizeHeight = 0;
                ResetDevice();
            }

            // Start ImGui frame
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::Begin("MainPanel", nullptr, winFlags);

            // ==================== HEADER BAR ====================
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.16f, 0.24f, 1.0f));
            ImGui::BeginChild("HeaderBar", ImVec2(0, 72), true);
            {
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", ql.getTenTruong().c_str());
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "PBL2 (DE TAI 24): HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC");
                ImGui::SameLine(ImGui::GetWindowWidth() - 360);
                ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.90f, 1.0f), "[ CSDL Class Table 3NF + Dear ImGui ]");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();

            // ==================== TAB BAR ====================
            if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {

                // ---------------------------------------------------------------
                // TAB 1: QUẢN LÝ HỒ SƠ THÍ SINH
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  1. Quan ly Ho so Thi sinh  ")) {
                    ImGui::Spacing();

                    // Toolbar Tìm kiếm & Lọc
                    ImGui::Text("Tim kiem:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(240);
                    ImGui::InputTextWithHint("##filter", "SBD, CCCD, Ho ten, Que quan...", filterBuf, IM_ARRAYSIZE(filterBuf));

                    ImGui::SameLine();
                    ImGui::Text("Ket qua:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(140);
                    const char* statusNames[] = { "Tat ca", "Trung tuyen", "Khong dat" };
                    ImGui::Combo("##filterStatus", &filterStatus, statusNames, 3);

                    ImGui::SameLine(ImGui::GetWindowWidth() - 330);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                    if (ImGui::Button("[+] Them Thi Sinh", ImVec2(150, 30))) {
                        showAddModal = true;
                        // Tự động sinh SBD mới tăng dần (DUTxxx)
                        string autoSBD = ql.sinhSoBaoDanhMoi();
                        strcpy(bufSBD, autoSBD.c_str());
                        strcpy(bufCCCD, "");
                        strcpy(bufHoTen, "");
                        // Mặc định lấy thời điểm hiện tại
                        getCurrentSystemDate(formNgay, formThang, formNam);
                        formQueQuanIdx = 20; // TP. Da Nang
                        formGioiTinh = 0;
                        formToan = 8.0f;
                        formLy = 8.0f;
                        formHoa = 8.0f;
                        formErrorMsg = "";
                    }
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 1.0f));
                    if (ImGui::Button("[ Xuat Bao Cao ]", ImVec2(150, 30))) {
                        string kw = Utils::trim(filterBuf);
                        ql.xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt", ql.getDiemChuan(), kw);
                        showToast = true;
                        if (kw.empty()) {
                            toastMsg = "Da xuat bao cao toan truong ra data/ketqua_xettuyen.txt!";
                        } else {
                            toastMsg = "Da xuat bao cao (tu khoa: " + kw + ") ra data/ketqua_xettuyen.txt!";
                        }
                    }
                    ImGui::PopStyleColor();

                    ImGui::Separator();

                    // Quick Sort Buttons
                    ImGui::Text("Sap xep:");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Tong Diem Giam Dan")) {
                        ql.sapXepTheoTongDiemGiamDan();
                        selectedRow = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Ten Thi Sinh A-Z")) {
                        ql.sapXepTheoTenAZ();
                        selectedRow = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("SBD Tang Dan")) {
                        ql.sapXepTheoSBDTangDan();
                        selectedRow = -1;
                    }

                    ImGui::SameLine(ImGui::GetWindowWidth() - 320);
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Tong so: %d thi sinh | Diem chuan: %.2f", 
                        (int)ql.getDanhSach().size(), ql.getDiemChuan());

                    // BẢNG HIỂN THỊ CHÍNH (THÊM CỘT THAO TÁC SỬA / XÓA)
                    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                                                 ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | 
                                                 ImGuiTableFlags_ScrollX;

                    float tableHeight = ImGui::GetWindowHeight() - 230;
                    if (tableHeight < 250) tableHeight = 250;

                    if (ImGui::BeginTable("CandidatesTable", 13, tableFlags, ImVec2(0, tableHeight))) {
                        ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                        ImGui::TableSetupColumn("SBD (PK)", ImGuiTableColumnFlags_WidthFixed, 90);
                        ImGui::TableSetupColumn("So CCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 130);
                        ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 100);
                        ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 140);
                        ImGui::TableSetupColumn("Phai", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("Toan", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("Ly", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("Hoa", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 90);
                        ImGui::TableSetupColumn("Ket Qua", ImGuiTableColumnFlags_WidthFixed, 120);
                        ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableHeadersRow();

                        string filterStr = Utils::toLower(Utils::trim(filterBuf));

                        int visibleCount = 0;
                        for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSach()[i];

                            if (!filterStr.empty()) {
                                bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getSoCCCD(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getHoTen(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getQueQuan(), filterStr);
                                if (!match) continue;
                            }

                            bool pass = ts.xetTrungTuyen(ql.getDiemChuan());

                            if (filterStatus == 1 && !pass) continue;
                            if (filterStatus == 2 && pass) continue;

                            visibleCount++;
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%d", visibleCount);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoBaoDanh().c_str());

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", ts.getSoCCCD().c_str());

                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%s", ts.getHoTen().c_str());

                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%s", ts.getNgaySinh().c_str());

                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%s", ts.getQueQuan().c_str());

                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%s", ts.getGioiTinh().c_str());

                            ImGui::TableSetColumnIndex(7);
                            ImGui::Text("%.2f", ts.getDiemToan());

                            ImGui::TableSetColumnIndex(8);
                            ImGui::Text("%.2f", ts.getDiemLy());

                            ImGui::TableSetColumnIndex(9);
                            ImGui::Text("%.2f", ts.getDiemHoa());

                            ImGui::TableSetColumnIndex(10);
                            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%.2f", ts.tinhTongDiem());

                            ImGui::TableSetColumnIndex(11);
                            if (pass) {
                                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "TRUNG TUYEN");
                            } else {
                                if (ts.kiemTraDiemLiet()) {
                                    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Diem liet");
                                } else {
                                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Khong dat");
                                }
                            }

                            // CỘT 12: THAO TÁC (SỬA & XÓA CHO TỪNG BẢN GHI)
                            ImGui::TableSetColumnIndex(12);
                            ImGui::PushID((int)i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.75f, 1.0f));
                            if (ImGui::Button("Sua", ImVec2(48, 22))) {
                                selectedRow = (int)i;
                                showEditModal = true;
                                ImGui::OpenPopup("Cap Nhat Thong Tin Thi Sinh");
                                strcpy(bufSBD, ts.getSoBaoDanh().c_str());
                                strcpy(bufCCCD, ts.getSoCCCD().c_str());
                                strcpy(bufHoTen, ts.getHoTen().c_str());

                                string ns = ts.getNgaySinh();
                                if (ns.length() == 10 && ns[2] == '/' && ns[5] == '/') {
                                    try {
                                        formNgay = stoi(ns.substr(0, 2));
                                        formThang = stoi(ns.substr(3, 2));
                                        formNam = stoi(ns.substr(6, 4));
                                    } catch (...) {
                                        getCurrentSystemDate(formNgay, formThang, formNam);
                                    }
                                } else {
                                    getCurrentSystemDate(formNgay, formThang, formNam);
                                }
                                formQueQuanIdx = timViTriTinhThanh(ts.getQueQuan());
                                formGioiTinh = (ts.getGioiTinh() == "Nu" ? 1 : 0);
                                formToan = (float)ts.getDiemToan();
                                formLy = (float)ts.getDiemLy();
                                formHoa = (float)ts.getDiemHoa();
                                formErrorMsg = "";
                            }
                            ImGui::PopStyleColor();

                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.22f, 0.22f, 1.0f));
                            if (ImGui::Button("Xoa", ImVec2(48, 22))) {
                                selectedRow = (int)i;
                                showDeleteModal = true;
                                ImGui::OpenPopup("Xac Nhan Xoa Thi Sinh");
                            }
                            ImGui::PopStyleColor();
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 2: CƠ SỞ DỮ LIỆU QUAN HỆ (CLASS TABLE INHERITANCE)
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  2. Co so Du lieu Quan he (Class Table)  ")) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.18f, 0.25f, 1.0f));
                    ImGui::BeginChild("DbExpl", ImVec2(0, 95), true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "MO HINH ANH XA KE THUA VAO CO SO DU LIEU QUAN HE (CLASS TABLE INHERITANCE):");
                        ImGui::BulletText("Bang 1 (nguoi.txt): Luu tru thuc the NGUOI voi Khoa chinh (PK) la 'So CCCD' (12 chu so duy nhat).");
                        ImGui::BulletText("Bang 2 (thisinh.txt): Luu tru thuc the THI SINH voi Khoa chinh (PK) la 'So Bao Danh' va Khoa ngoai (FK) la 'So CCCD'.");
                        ImGui::BulletText("Ket noi du lieu: He thong thuc hien JOIN in-memory tren khoa ngoai 'So CCCD' de tao doi tuong ThiSinh hoan chinh.");
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    if (ImGui::BeginTabBar("SubTabsDb", ImGuiTabBarFlags_None)) {
                        
                        // SUBTAB 2.1: BẢNG NGƯỜI
                        if (ImGui::BeginTabItem("  Bang 1: NGUOI (data/nguoi.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THUC THE CHA: NGUOI (Khoa chinh PK: So CCCD)");
                            ImGui::Separator();

                            if (ImGui::BeginTable("TblNguoiRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                                ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                                ImGui::TableSetupColumn("So CCCD (PK)", ImGuiTableColumnFlags_WidthFixed, 140);
                                ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 110);
                                ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 160);
                                ImGui::TableSetupColumn("Gioi Tinh", ImGuiTableColumnFlags_WidthFixed, 90);
                                ImGui::TableHeadersRow();

                                for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                    const ThiSinh& ts = ql.getDanhSach()[i];
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("%d", (int)(i + 1));
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoCCCD().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::Text("%s", ts.getHoTen().c_str());
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::Text("%s", ts.getNgaySinh().c_str());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%s", ts.getQueQuan().c_str());
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::Text("%s", ts.getGioiTinh().c_str());
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // SUBTAB 2.2: BẢNG THÍ SINH
                        if (ImGui::BeginTabItem("  Bang 2: THI SINH (data/thisinh.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THUC THE CON: THI SINH (PK: So Bao Danh | FK: So CCCD)");
                            ImGui::Separator();

                            if (ImGui::BeginTable("TblTSRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                                ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                                ImGui::TableSetupColumn("So Bao Danh (PK)", ImGuiTableColumnFlags_WidthFixed, 130);
                                ImGui::TableSetupColumn("So CCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 140);
                                ImGui::TableSetupColumn("Diem Toan", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("Diem Ly", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("Diem Hoa", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableHeadersRow();

                                for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                    const ThiSinh& ts = ql.getDanhSach()[i];
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("%d", (int)(i + 1));
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ts.getSoBaoDanh().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoCCCD().c_str());
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::Text("%.2f", ts.getDiemToan());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%.2f", ts.getDiemLy());
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::Text("%.2f", ts.getDiemHoa());
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // SUBTAB 2.3: BẢNG DANH MỤC TỈNH THÀNH (34 TỈNH THÀNH THEO NQ 202/2025/QH15)
                        if (ImGui::BeginTabItem("  Bang 3: TINH THANH (data/tinhthanh.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC: 34 TINH / THANH PHO (Nghi quyet 202/2025/QH15)");
                            ImGui::Separator();

                            if (ImGui::BeginTable("TblTinhThanhRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                                ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                                ImGui::TableSetupColumn("Ma Tinh (PK)", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("Ten Don Vi Hanh Chinh", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Loai Hinh Don Vi", ImGuiTableColumnFlags_WidthFixed, 260);
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < kSoLuong34TinhThanh; ++i) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("%d", i + 1);
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", kDanhSach34TinhThanhMoi[i].ma);
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", kDanhSach34TinhThanhMoi[i].ten);
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::Text("%s", kDanhSach34TinhThanhMoi[i].loai);
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // SUBTAB 2.4: BẢNG CẤU HÌNH HỆ THỐNG
                        if (ImGui::BeginTabItem("  Bang 4: HE THONG (data/system.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THAM SO HE THONG & TUYEN SINH (data/system.txt)");
                            ImGui::Separator();

                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
                            ImGui::BeginChild("SystemCfgView", ImVec2(0, 160), true);
                            {
                                ImGui::BulletText("Truong Dao tao  : %s", ql.getTenTruong().c_str());
                                ImGui::BulletText("Nam Tuyen Sinh  : %s", ql.getNamTuyenSinh().c_str());
                                ImGui::BulletText("Diem Chuan San  : %.2f diem", ql.getDiemChuan());
                                ImGui::BulletText("Tong So Thi Sinh: %d thi sinh", (int)ql.getDanhSach().size());
                            }
                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                            ImGui::EndTabItem();
                        }

                        // SUBTAB 2.5: SƠ ĐỒ LIÊN KẾT ERD
                        if (ImGui::BeginTabItem("  So do Lien ket ERD (PK - FK)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "SO DO CHI TIET LIEN KET KHOA CHINH (PK) VA KHOA NGOAI (FK):");
                            ImGui::Separator();

                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));
                            ImGui::BeginChild("ERDView", ImVec2(0, ImGui::GetWindowHeight() - 250), true);
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), 
                                    "+-------------------------------------------------------------------------+\n"
                                    "|             BANG 1: NGUOI (Tep luu tru: data/nguoi.txt)                 |\n"
                                    "+-------------------------------------------------------------------------+\n"
                                    "|  * soCCCD (PRIMARY KEY - PK)  : So Can cuoc Cong dan (12 chu so)        |\n"
                                    "|    hoTen                      : Ho va ten                               |\n"
                                    "|    ngaySinh                   : dd/mm/yyyy                              |\n"
                                    "|    queQuan                    : Tinh / Thanh pho                        |\n"
                                    "|    gioiTinh                   : Nam / Nu                                |\n"
                                    "+-------------------------------------------------------------------------+\n"
                                    "                                     ^\n"
                                    "                                     | 1 (Quan he 1 - 1 qua Ke thua)\n"
                                    "                                     |\n"
                                    "                                     | 1 (FK: soCCCD)\n"
                                    "+-------------------------------------------------------------------------+\n"
                                    "|           BANG 2: THI SINH (Tep luu tru: data/thisinh.txt)              |\n"
                                    "+-------------------------------------------------------------------------+\n"
                                    "|  * soBaoDanh (PRIMARY KEY - PK): So bao danh du thi (DUTxxx)            |\n"
                                    "|  * soCCCD (FOREIGN KEY - FK)   : Khoa ngoai tham chieu sang bang NGUOI  |\n"
                                    "|    diemToan                   : Diem thi mon Toan (0.00 - 10.00)        |\n"
                                    "|    diemLy                     : Diem thi mon Vat ly (0.00 - 10.00)      |\n"
                                    "|    diemHoa                    : Diem thi mon Hoa hoc (0.00 - 10.00)     |\n"
                                    "+-------------------------------------------------------------------------+\n\n"
                                    "[*] data/system.txt: Cau hinh he thong (Truong, Nam tuyen sinh, Diem chuan)\n"
                                    "[*] data/ketqua_xettuyen.txt, data/thukhoa.txt, data/thongke_phodiem.txt\n");
                            }
                            ImGui::EndChild();
                            ImGui::PopStyleColor();

                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 3: XÉT TUYỂN & THỐNG KÊ PHỔ ĐIỂM
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  3. Xet tuyen & Thong ke Pho diem  ")) {
                    ImGui::Spacing();

                    ImGui::BeginChild("AdmissionConfig", ImVec2(0, 95), true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "THIET LAP DIEM CHUAN XET TUYEN CUA TRUONG:");
                        static float curDC = (float)ql.getDiemChuan();

                        ImGui::SetNextItemWidth(140);
                        if (ImGui::InputFloat("Diem Chuan (thang 30)", &curDC, 0.25f, 1.0f, "%.2f")) {
                            if (curDC < 0.0f) curDC = 0.0f;
                            if (curDC > 30.0f) curDC = 30.0f;
                            ql.setDiemChuan((double)curDC);
                            ql.luuCauHinhHeThong();
                            ql.xuatTatCaBaoCao();
                        }
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "(Thay doi diem chuan lap tuc cap nhat toan he thong)");

                        ImGui::SameLine(ImGui::GetWindowWidth() - 210);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.75f, 1.0f));
                        if (ImGui::Button("[ Xuat Bao Cao ]", ImVec2(180, 28))) {
                            ql.xuatBaoCaoThuKhoa("data/thukhoa.txt");
                            ql.xuatBaoCaoPhoDiem("data/thongke_phodiem.txt");
                            showToast = true;
                            toastMsg = "Da xuat dong bo data/thukhoa.txt va data/thongke_phodiem.txt!";
                        }
                        ImGui::PopStyleColor();

                        Vector<ThiSinh> dsTT = ql.getDanhSachTrungTuyen(ql.getDiemChuan());
                        size_t total = ql.getDanhSach().size();
                        double pct = total > 0 ? (dsTT.size() * 100.0 / total) : 0.0;
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Ket qua: %d / %d thi sinh TRUNG TUYEN (%.1f%%)  |  %d thi sinh KHONG DAT",
                            (int)dsTT.size(), (int)total, pct, (int)(total - dsTT.size()));
                    }
                    ImGui::EndChild();

                    ImGui::Spacing();

                    float colW = (ImGui::GetWindowWidth() - 40) * 0.5f;

                    // VINH DANH THỦ KHOA
                    ImGui::BeginChild("ValedictorianCol", ImVec2(colW, 280), true);
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "★ VINH DANH THU KHOA KY THI TUYEN SINH ★");
                        ImGui::Separator();

                        Vector<ThiSinh> dsTK = ql.getDanhSachThuKhoa();
                        if (dsTK.empty()) {
                            ImGui::Text("Chua co thi sinh nao trong he thong.");
                        } else {
                            for (size_t i = 0; i < dsTK.size(); ++i) {
                                ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Thu khoa %d: %s", (int)(i + 1), dsTK[i].getHoTen().c_str());
                                ImGui::BulletText("So bao danh : %s", dsTK[i].getSoBaoDanh().c_str());
                                ImGui::BulletText("So CCCD     : %s", dsTK[i].getSoCCCD().c_str());
                                ImGui::BulletText("Que quan    : %s", dsTK[i].getQueQuan().c_str());
                                ImGui::BulletText("Diem Toan   : %.2f", dsTK[i].getDiemToan());
                                ImGui::BulletText("Diem Ly     : %.2f", dsTK[i].getDiemLy());
                                ImGui::BulletText("Diem Hoa    : %.2f", dsTK[i].getDiemHoa());
                                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "  -> TONG DIEM: %.2f diem", dsTK[i].tinhTongDiem());
                                ImGui::Separator();
                            }
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    // THỐNG KÊ PHỔ ĐIỂM
                    ImGui::BeginChild("ScoreStatsCol", ImVec2(colW, 280), true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "PHAN BO PHO DIEM THI 3 MON:");
                        ImGui::Separator();

                        int thap, tb, kha, cao, ratCao;
                        ql.tinhThongKePhoDiem(thap, tb, kha, cao, ratCao);
                        size_t total = ql.getDanhSach().size();

                        auto renderBar = [&](const char* label, int count, ImVec4 col) {
                            float ratio = total > 0 ? (float)count / total : 0.0f;
                            char overlay[64];
                            sprintf(overlay, "%s: %d (%.1f%%)", label, count, ratio * 100.0f);
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
                            ImGui::ProgressBar(ratio, ImVec2(-1, 20), overlay);
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        };

                        renderBar("Rat cao  (>= 27.0)", ratCao, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                        renderBar("Cao      (24.0 - <27.0)", cao, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
                        renderBar("Kha      (21.0 - <24.0)", kha, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                        renderBar("Trung TB (15.0 - <21.0)", tb, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                        renderBar("Thap     (< 15.0)", thap, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 4: LƯU TRỮ & XUẤT BÁO CÁO (ĐA TỆP NHƯ ĐỒ ÁN MẪU)
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  4. Luu tru & Xuat Bao cao  ")) {
                    ImGui::Spacing();

                    ImGui::Text("Chuc nang quan tri he thong CSDL da tep:");
                    ImGui::Spacing();

                    if (ImGui::Button("[ Luu Toan Bo CSDL (nguoi.txt, thisinh.txt, system.txt) ]", ImVec2(390, 36))) {
                        ql.luuDuLieu();
                        ql.luuCauHinhHeThong();
                        showToast = true;
                        toastMsg = "Da luu CSDL va cau hinh he thong thanh cong!";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("[ Xuat Tat Ca Bao Cao (3 Tep: Ket qua, Thu khoa, Pho diem) ]", ImVec2(410, 36))) {
                        ql.xuatTatCaBaoCao();
                        showToast = true;
                        toastMsg = "Da xuat ca 3 tep bao cao thanh cong!";
                        
                        // Đọc lại file được chọn để preview
                        string path = "data/ketqua_xettuyen.txt";
                        if (selectedReportFile == 1) path = "data/thukhoa.txt";
                        else if (selectedReportFile == 2) path = "data/thongke_phodiem.txt";
                        ifstream fPreview(path);
                        if (fPreview.is_open()) {
                            stringstream ss;
                            ss << fPreview.rdbuf();
                            reportContent = ss.str();
                            fPreview.close();
                        }
                    }

                    ImGui::Spacing();
                    ImGui::Separator();

                    // Chọn tệp báo cáo xem trước
                    ImGui::Text("Chon tep bao cao xem truoc:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(260);
                    const char* reportFiles[] = {
                        "ketqua_xettuyen.txt (Ket qua xet tuyen)",
                        "thukhoa.txt (Vinh danh thu khoa)",
                        "thongke_phodiem.txt (Phan tich pho diem)"
                    };
                    if (ImGui::Combo("##selReport", &selectedReportFile, reportFiles, 3)) {
                        string path = "data/ketqua_xettuyen.txt";
                        if (selectedReportFile == 1) path = "data/thukhoa.txt";
                        else if (selectedReportFile == 2) path = "data/thongke_phodiem.txt";
                        ifstream fPreview(path);
                        if (fPreview.is_open()) {
                            stringstream ss;
                            ss << fPreview.rdbuf();
                            reportContent = ss.str();
                            fPreview.close();
                        }
                    }

                    if (reportContent.empty()) {
                        string path = "data/ketqua_xettuyen.txt";
                        if (selectedReportFile == 1) path = "data/thukhoa.txt";
                        else if (selectedReportFile == 2) path = "data/thongke_phodiem.txt";
                        ifstream fPreview(path);
                        if (fPreview.is_open()) {
                            stringstream ss;
                            ss << fPreview.rdbuf();
                            reportContent = ss.str();
                            fPreview.close();
                        }
                    }

                    ImGui::InputTextMultiline("##reportPreview", (char*)reportContent.c_str(), reportContent.size(), 
                        ImVec2(-1, ImGui::GetWindowHeight() - 250), ImGuiInputTextFlags_ReadOnly);

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 5: GIỚI THIỆU & KIẾN TRÚC HỆ THỐNG
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  5. Huong dan & Kien truc  ")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "DO AN CO SO LAP TRINH (PBL2) - DE TAI 24");
                    ImGui::BulletText("Truong: Dai hoc Bach khoa - Dai hoc Da Nang (DUT)");
                    ImGui::BulletText("Tac gia: Vo Nhu Hieu");
                    ImGui::BulletText("Hoc phan: PBL2 - Do an co so lap trinh");
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "KIEN TRUC DU LIEU DA TEP THEO CHUAN DO AN MAU:");
                    ImGui::BulletText("1. data/nguoi.txt: Bang NGUOI theo Key-Value.");
                    ImGui::BulletText("2. data/thisinh.txt: Bang THI SINH theo Key-Value, lien ket qua soCCCD.");
                    ImGui::BulletText("3. data/system.txt: Cau hinh he thong (Diem chuan, Nam hoc, Truong).");
                    ImGui::BulletText("4. data/ketqua_xettuyen.txt: Bang ket qua xet tuyen toan truong.");
                    ImGui::BulletText("5. data/thukhoa.txt: Bang vinh danh thu khoa ky thi.");
                    ImGui::BulletText("6. data/thongke_phodiem.txt: Bang phan tich pho diem thi.");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            // ==================== MODAL: THÊM THÍ SINH ====================
            if (showAddModal) {
                ImGui::OpenPopup("Them Thi Sinh Moi");
            }
            if (ImGui::BeginPopupModal("Them Thi Sinh Moi", &showAddModal, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "NHAP HO SO VA DIEM THI THI SINH");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::BeginTable("FormAddTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("ColLabel", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("ColInput", ImGuiTableColumnFlags_WidthStretch);

                    // Row 1: SBD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So Bao Danh (Tu dong tang):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::BeginDisabled();
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##sbd", bufSBD, IM_ARRAYSIZE(bufSBD));
                    ImGui::EndDisabled();

                    // Row 2: CCCD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So CCCD (12 so)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##cccd", bufCCCD, IM_ARRAYSIZE(bufCCCD));

                    // Row 3: Họ và tên
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ho va Ten*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##hoTen", bufHoTen, IM_ARRAYSIZE(bufHoTen));

                    // Row 4: Ngày sinh (dd/mm/yyyy)*
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ngay Sinh (dd/mm/yyyy)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##day", &formNgay, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        int maxD = getDaysInMonth(formThang, formNam);
                        if (formNgay < 1) formNgay = 1;
                        if (formNgay > maxD) formNgay = maxD;
                    }
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##month", &formThang, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        if (formThang < 1) formThang = 1;
                        if (formThang > 12) formThang = 12;
                    }
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputInt("##year", &formNam, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        if (formNam < 1900) formNam = 1900;
                        if (formNam > 2030) formNam = 2030;
                    }
                    ImGui::SameLine();
                    RenderDatePickerCalendar("[ Chon Lich ]", "##calPopupAdd", formNgay, formThang, formNam);

                    // Row 5: Quê quán (34 tỉnh thành - Searchable Combobox)
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Que Quan (Tinh/TP)*:");
                    ImGui::TableSetColumnIndex(1);
                    RenderSearchableComboTinhThanh("##comboQueQuanAdd", formQueQuanIdx);

                    // Row 6: Giới tính
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Gioi Tinh*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::AlignTextToFramePadding();
                    if (ImGui::RadioButton("Nam", formGioiTinh == 0)) formGioiTinh = 0;
                    ImGui::SameLine(0.0f, 30.0f);
                    if (ImGui::RadioButton("Nu", formGioiTinh == 1)) formGioiTinh = 1;

                    // Row 7: Điểm Toán
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Toan (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##toan", &formToan, 0.0f, 0.0f, "%.2f");
                    if (formToan < 0.0f) formToan = 0.0f; if (formToan > 10.0f) formToan = 10.0f;

                    // Row 8: Điểm Lý
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Ly (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##ly", &formLy, 0.0f, 0.0f, "%.2f");
                    if (formLy < 0.0f) formLy = 0.0f; if (formLy > 10.0f) formLy = 10.0f;

                    // Row 9: Điểm Hóa
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Hoa (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##hoa", &formHoa, 0.0f, 0.0f, "%.2f");
                    if (formHoa < 0.0f) formHoa = 0.0f; if (formHoa > 10.0f) formHoa = 10.0f;

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Preview tính toán
                double tongDiem = formToan + formLy + formHoa;
                bool liet = (formToan <= 1.0f || formLy <= 1.0f || formHoa <= 1.0f);
                bool pass = (!liet && tongDiem >= ql.getDiemChuan());

                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Tong Diem: %.2f  |  Ket Qua: %s", 
                    tongDiem, (pass ? "TRUNG TUYEN" : (liet ? "BI DIEM LIET" : "Khong dat")));

                if (!formErrorMsg.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", formErrorMsg.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Xac Nhan Them", ImVec2(140, 30))) {
                    string sbd = Utils::toUpper(Utils::trim(bufSBD));
                    string cccd = Utils::trim(bufCCCD);
                    string hoten = Utils::chuanHoaTen(bufHoTen);
                    
                    // Ghép ngày sinh chuẩn dd/mm/yyyy
                    char dateBuf[16];
                    sprintf(dateBuf, "%02d/%02d/%04d", formNgay, formThang, formNam);
                    string ns = dateBuf;

                    string que = kDanhSach34TinhThanhMoi[formQueQuanIdx].ten;

                    if (!Utils::kiemTraSBD(sbd)) {
                        formErrorMsg = "SBD khong hop le!";
                    } else if (ql.timViTriTheoSBD(sbd) != -1) {
                        formErrorMsg = "So Bao Danh da ton tai!";
                    } else if (!Utils::kiemTraCCCD(cccd)) {
                        formErrorMsg = "So CCCD phai gom dung 12 chu so!";
                    } else if (ql.timViTriTheoCCCD(cccd) != -1) {
                        formErrorMsg = "So CCCD da duoc dang ky boi thi sinh khac!";
                    } else if (hoten.length() < 2) {
                        formErrorMsg = "Ho va ten phai co it nhat 2 ky tu!";
                    } else if (!Utils::kiemTraNgaySinh(ns)) {
                        formErrorMsg = "Ngay sinh khong hop le!";
                    } else {
                        string gt = (formGioiTinh == 1) ? "Nu" : "Nam";
                        ThiSinh moi(cccd, hoten, ns, que, gt, sbd, (double)formToan, (double)formLy, (double)formHoa);

                        ql.themThiSinh(moi);
                        ql.xuatTatCaBaoCao();
                        reportContent.clear();
                        showAddModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them thi sinh moi thanh cong!";
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                    showAddModal = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // ==================== MODAL: SỬA THÍ SINH ====================
            if (showEditModal) {
                ImGui::OpenPopup("Cap Nhat Thong Tin Thi Sinh");
            }
            if (ImGui::BeginPopupModal("Cap Nhat Thong Tin Thi Sinh", &showEditModal, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT THONG TIN THI SINH (SBD: %s)", bufSBD);
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::BeginTable("FormEditTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("ColLabelEd", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("ColInputEd", ImGuiTableColumnFlags_WidthStretch);

                    // Row 1: SBD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So Bao Danh (SBD):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::BeginDisabled();
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##sbdEd", bufSBD, IM_ARRAYSIZE(bufSBD));
                    ImGui::EndDisabled();

                    // Row 2: CCCD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So CCCD (12 so)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##cccdEd", bufCCCD, IM_ARRAYSIZE(bufCCCD));

                    // Row 3: Họ và tên
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ho va Ten*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##hoTenEd", bufHoTen, IM_ARRAYSIZE(bufHoTen));

                    // Row 4: Ngày sinh (dd/mm/yyyy)*
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ngay Sinh (dd/mm/yyyy)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##dayEd", &formNgay, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        int maxD = getDaysInMonth(formThang, formNam);
                        if (formNgay < 1) formNgay = 1;
                        if (formNgay > maxD) formNgay = maxD;
                    }
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##monthEd", &formThang, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        if (formThang < 1) formThang = 1;
                        if (formThang > 12) formThang = 12;
                    }
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputInt("##yearEd", &formNam, 0, 0);
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        if (formNam < 1900) formNam = 1900;
                        if (formNam > 2030) formNam = 2030;
                    }
                    ImGui::SameLine();
                    RenderDatePickerCalendar("[ Chon Lich ]", "##calPopupEdit", formNgay, formThang, formNam);

                    // Row 5: Quê quán (34 tỉnh thành - Searchable Combobox)
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Que Quan (Tinh/TP)*:");
                    ImGui::TableSetColumnIndex(1);
                    RenderSearchableComboTinhThanh("##comboQueQuanEdit", formQueQuanIdx);

                    // Row 6: Giới tính
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Gioi Tinh*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::AlignTextToFramePadding();
                    if (ImGui::RadioButton("Nam##ed", formGioiTinh == 0)) formGioiTinh = 0;
                    ImGui::SameLine(0.0f, 30.0f);
                    if (ImGui::RadioButton("Nu##ed", formGioiTinh == 1)) formGioiTinh = 1;

                    // Row 7: Điểm Toán
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Toan (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##toanEd", &formToan, 0.0f, 0.0f, "%.2f");
                    if (formToan < 0.0f) formToan = 0.0f; if (formToan > 10.0f) formToan = 10.0f;

                    // Row 8: Điểm Lý
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Ly (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##lyEd", &formLy, 0.0f, 0.0f, "%.2f");
                    if (formLy < 0.0f) formLy = 0.0f; if (formLy > 10.0f) formLy = 10.0f;

                    // Row 9: Điểm Hóa
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Hoa (0.00 - 10.00)*:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##hoaEd", &formHoa, 0.0f, 0.0f, "%.2f");
                    if (formHoa < 0.0f) formHoa = 0.0f; if (formHoa > 10.0f) formHoa = 10.0f;

                    ImGui::EndTable();
                }

                if (!formErrorMsg.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", formErrorMsg.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Luu Thay Doi", ImVec2(140, 30))) {
                    string cccd = Utils::trim(bufCCCD);
                    string hoten = Utils::chuanHoaTen(bufHoTen);
                    
                    char dateBuf[16];
                    sprintf(dateBuf, "%02d/%02d/%04d", formNgay, formThang, formNam);
                    string ns = dateBuf;

                    string que = kDanhSach34TinhThanhMoi[formQueQuanIdx].ten;

                    if (!Utils::kiemTraCCCD(cccd)) {
                        formErrorMsg = "So CCCD phai gom dung 12 chu so!";
                    } else if (hoten.length() < 2) {
                        formErrorMsg = "Ho va ten phai co it nhat 2 ky tu!";
                    } else if (!Utils::kiemTraNgaySinh(ns)) {
                        formErrorMsg = "Ngay sinh khong hop le!";
                    } else {
                        string gt = (formGioiTinh == 1) ? "Nu" : "Nam";
                        ThiSinh capNhat(cccd, hoten, ns, que, gt, bufSBD, (double)formToan, (double)formLy, (double)formHoa);

                        if (ql.capNhatThiSinh(capNhat)) {
                            ql.xuatTatCaBaoCao();
                            reportContent.clear();
                            showEditModal = false;
                            ImGui::CloseCurrentPopup();
                            showToast = true;
                            toastMsg = "Cap nhat ho so thi sinh thanh cong!";
                        } else {
                            formErrorMsg = "Trung so CCCD voi thi sinh khac!";
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                    showEditModal = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // ==================== MODAL: XÓA THÍ SINH ====================
            if (showDeleteModal) {
                ImGui::OpenPopup("Xac Nhan Xoa Thi Sinh");
            }
            if (ImGui::BeginPopupModal("Xac Nhan Xoa Thi Sinh", &showDeleteModal, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (selectedRow >= 0 && selectedRow < (int)ql.getDanhSach().size()) {
                    const ThiSinh& sel = ql.getDanhSach()[selectedRow];
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "BAN CO CHAC CHAN MUON XOA THI SINH NAY KHOI CSDL?");
                    ImGui::Separator();
                    ImGui::Text("SBD     : %s", sel.getSoBaoDanh().c_str());
                    ImGui::Text("So CCCD : %s", sel.getSoCCCD().c_str());
                    ImGui::Text("Ho Ten  : %s", sel.getHoTen().c_str());
                    ImGui::Text("Que Quan: %s", sel.getQueQuan().c_str());
                    ImGui::Spacing();

                    if (ImGui::Button("DONG Y XOA", ImVec2(120, 30))) {
                        ql.xoaThiSinh(sel.getSoBaoDanh());
                        ql.xuatTatCaBaoCao();
                        reportContent.clear();
                        selectedRow = -1;
                        showDeleteModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Da xoa thi sinh va dong bo CSDL thanh cong!";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                        showDeleteModal = false;
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    showDeleteModal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // ==================== TOAST NOTIFICATION ====================
            if (showToast) {
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowWidth() - 380, 20));
                ImGui::SetNextWindowSize(ImVec2(360, 50));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.45f, 0.25f, 0.95f));
                ImGui::Begin("Toast", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::TextColored(ImVec4(1, 1, 1, 1), "[THANH CONG] %s", toastMsg.c_str());
                static float toastTimer = 0;
                toastTimer += io.DeltaTime;
                if (toastTimer > 3.0f) {
                    showToast = false;
                    toastTimer = 0;
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }

            ImGui::End();

            // Render ImGui
            ImGui::EndFrame();
            g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(18, 22, 30, 255);
            g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

            if (g_pd3dDevice->BeginScene() >= 0) {
                ImGui::Render();
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                g_pd3dDevice->EndScene();
            }

            HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
            if (result == D3DERR_DEVICELOST)
                g_DeviceLost = true;
        }

        // Cleanup
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 0;
    }
}

// -------------------------------------------------------------
// Direct3D 9 Helper Functions
// -------------------------------------------------------------
static bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // V-Sync

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

static void CleanupDeviceD3D() {
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

static void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
