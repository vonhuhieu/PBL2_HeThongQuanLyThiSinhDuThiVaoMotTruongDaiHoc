#include "../include/AppGUI.h"
#include "../include/Utils.h"
#include "../include/Constants.h"
#include "../include/DanhMuc.h"
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

// Searchable Combobox chọn Tỉnh / Thành phố từ danh mục hệ thống
static bool RenderSearchableComboTinhThanh(const char* comboId, const Vector<TinhThanh>& ds, string& selectedMa) {
    static char filterBuf[64] = "";
    bool changed = false;

    string currentLabel = "Chon tinh / thanh pho...";
    for (size_t i = 0; i < ds.size(); ++i) {
        if (ds[i].ma == selectedMa) {
            currentLabel = ds[i].ten + " (" + ds[i].ma + ")";
            break;
        }
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo(comboId, currentLabel.c_str())) {
        if (ImGui::IsWindowAppearing()) {
            filterBuf[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filterTinh", "Tim kiem (VD: Da Nang, Ha Noi, 48...)", filterBuf, sizeof(filterBuf));
        ImGui::Separator();

        string query = Utils::toLower(Utils::trim(filterBuf));
        int matchCount = 0;

        for (size_t i = 0; i < ds.size(); ++i) {
            string itemName = Utils::toLower(ds[i].ten);
            string itemMa   = Utils::toLower(ds[i].ma);
            if (query.empty() || itemName.find(query) != string::npos || itemMa.find(query) != string::npos) {
                matchCount++;
                bool isSelected = (selectedMa == ds[i].ma);
                string displayLabel = "[" + ds[i].ma + "] " + ds[i].ten;
                if (ImGui::Selectable(displayLabel.c_str(), isSelected)) {
                    selectedMa = ds[i].ma;
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

// Combobox chọn Giới tính từ danh mục hệ thống
static bool RenderComboGioiTinh(const char* comboId, const Vector<GioiTinh>& ds, string& selectedMa) {
    bool changed = false;
    string currentLabel = "Chon gioi tinh...";
    for (size_t i = 0; i < ds.size(); ++i) {
        if (Utils::toUpper(ds[i].ma) == Utils::toUpper(selectedMa)) {
            currentLabel = ds[i].ten;
            break;
        }
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo(comboId, currentLabel.c_str())) {
        for (size_t i = 0; i < ds.size(); ++i) {
            bool isSelected = (Utils::toUpper(selectedMa) == Utils::toUpper(ds[i].ma));
            string displayLabel = ds[i].ten + " (" + ds[i].moTa + ")";
            if (ImGui::Selectable(displayLabel.c_str(), isSelected)) {
                selectedMa = ds[i].ma;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
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
    struct tm* t = localtime(&now);
    if (t) {
        d = t->tm_mday;
        m = t->tm_mon + 1;
        y = t->tm_year + 1900;
    } else {
        d = 1; m = 1; y = 2005;
    }
}

// DatePicker Calendar Modal
static bool RenderDatePickerCalendar(const char* btnLabel, const char* popupId, int& d, int& m, int& y) {
    bool selected = false;
    if (ImGui::Button(btnLabel, ImVec2(90, 24))) {
        ImGui::OpenPopup(popupId);
    }
    if (ImGui::BeginPopup(popupId)) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CHON NGAY SINH:");
        ImGui::Separator();

        if (ImGui::Button("<##prevY")) { y--; }
        ImGui::SameLine();
        ImGui::Text("%d", y);
        ImGui::SameLine();
        if (ImGui::Button(">##nextY")) { y++; }

        ImGui::SameLine(130);
        if (ImGui::Button("<##prevM")) { m--; if (m < 1) { m = 12; y--; } }
        ImGui::SameLine();
        ImGui::Text("Thang %02d", m);
        ImGui::SameLine();
        if (ImGui::Button(">##nextM")) { m++; if (m > 12) { m = 1; y++; } }

        ImGui::Separator();

        const char* daysHeader[] = { "T2", "T3", "T4", "T5", "T6", "T7", "CN" };
        for (int i = 0; i < 7; ++i) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", daysHeader[i]);
            if (i < 6) ImGui::SameLine(0.0f, 14.0f);
        }
        ImGui::Separator();

        int firstDay = getFirstDayOfWeek(m, y);
        int totalDays = getDaysInMonth(m, y);

        for (int i = 0; i < firstDay; ++i) {
            ImGui::Text("  ");
            ImGui::SameLine(0.0f, 18.0f);
        }

        for (int day = 1; day <= totalDays; ++day) {
            char dayStr[8];
            sprintf(dayStr, "%02d", day);
            bool isCurrent = (day == d);

            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::SmallButton(dayStr)) {
                d = day;
                selected = true;
                ImGui::CloseCurrentPopup();
            }

            if (isCurrent) {
                ImGui::PopStyleColor();
            }

            int curCol = (firstDay + day - 1) % 7;
            if (curCol < 6 && day < totalDays) {
                ImGui::SameLine(0.0f, 10.0f);
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Hom nay", ImVec2(80, 22))) {
            getCurrentSystemDate(d, m, y);
            selected = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Dong", ImVec2(60, 22))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return selected;
}

// Thiết lập giao diện Dark Theme hiện đại
static void SetupModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.09f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.11f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.15f, 0.20f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.20f, 0.25f, 0.32f, 0.60f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.15f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.22f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.26f, 0.34f, 0.44f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.10f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.18f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.24f, 0.45f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.28f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.26f, 0.35f, 0.48f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.30f, 0.42f, 0.58f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.13f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.25f, 0.38f, 0.55f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.32f, 0.48f, 1.00f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.11f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(0.13f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_Text]               = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
}
int AppGUI::run(QuanLyThiSinh& ql) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"PBL2_ImGui_DX9", nullptr };
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

    // Khởi tạo Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupModernDarkTheme();

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

    // Form buffers
    static char bufSBD[32] = "";
    static char bufCCCD[32] = "";
    static char bufHoTen[64] = "";
    static int  formNgay = 15;
    static int  formThang = 5;
    static int  formNam = 2005;
    static string formQueQuanMa = "48";
    static string formGioiTinhMa = AdmissionConst::GT_NAM;
    static float formToan = 8.0f;
    static float formLy = 8.0f;
    static float formHoa = 8.0f;
    static string formErrorMsg = "";

    // State quản lý danh mục (Tab 4)
    static char bufNewMaTinh[16] = "";
    static char bufNewTenTinh[64] = "";
    static char bufNewLoaiTinh[64] = "Tinh truc thuoc";
    static string msgTinhThanh = "";

    static char bufNewMaGT[16] = "";
    static char bufNewTenGT[64] = "";
    static char bufNewDescGT[64] = "";
    static string msgGioiTinh = "";

    // Form tham số hệ thống
    static char bufTenTruong[128] = "";
    static char bufNamTuyenSinh[32] = "";
    static float formDiemChuan = (float)ql.getDiemChuan();
    static float formDiemLiet = (float)ql.getDiemLiet();
    strcpy(bufTenTruong, ql.getTenTruong().c_str());
    strcpy(bufNamTuyenSinh, ql.getNamTuyenSinh().c_str());

    // Preview báo cáo file
    static int selectedReportFile = 0;
    static string reportContent = "";

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

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("MainPanel", nullptr, winFlags);

        // HEADER BAR
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.16f, 0.24f, 1.0f));
        ImGui::BeginChild("HeaderBar", ImVec2(0, 72), true);
        {
            ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", ql.getTenTruong().c_str());
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "PBL2 (DE TAI 24): HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC");
            if (ql.isMySQLConnected()) {
                ImGui::SameLine(ImGui::GetWindowWidth() - 460);
                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "[ * MySQL 8.4: Connected ]");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.90f, 1.0f), "[ 3NF 5 Bang ]");
            } else {
                ImGui::SameLine(ImGui::GetWindowWidth() - 480);
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "[ o MySQL: Offline ]");
                ImGui::SameLine();
                if (ImGui::SmallButton("Ket noi lai##Hdr")) {
                    if (ql.ketNoiMySQL()) {
                        ql.dongBoTuMySQL();
                    }
                }
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.90f, 1.0f), "[ Cache File ]");
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // TAB BAR (6 TABS CHUYÊN NGHIỆP)
        if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {

            // ==================== TAB 1: QUẢN LÝ HỒ SƠ THÍ SINH ====================
            if (ImGui::BeginTabItem("  1. Quan ly Ho so Thi sinh  ")) {
                ImGui::Spacing();

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
                    string autoSBD = ql.sinhSoBaoDanhMoi();
                    strcpy(bufSBD, autoSBD.c_str());
                    strcpy(bufCCCD, "");
                    strcpy(bufHoTen, "");
                    getCurrentSystemDate(formNgay, formThang, formNam);
                    formQueQuanMa = "48";
                    formGioiTinhMa = AdmissionConst::GT_NAM;
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
                    ImGui::TableSetupColumn("Gioi Tinh", ImGuiTableColumnFlags_WidthFixed, 70);
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
                        string tenQue = ql.layTenTinhThanh(ts.getQueQuan());
                        string tenGT  = ql.layTenGioiTinh(ts.getGioiTinh());

                        if (!filterStr.empty()) {
                            bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), filterStr) ||
                                         Utils::containsIgnoreCase(ts.getSoCCCD(), filterStr) ||
                                         Utils::containsIgnoreCase(ts.getHoTen(), filterStr) ||
                                         Utils::containsIgnoreCase(ts.getQueQuan(), filterStr) ||
                                         Utils::containsIgnoreCase(tenQue, filterStr);
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
                        ImGui::Text("%s", tenQue.c_str());

                        ImGui::TableSetColumnIndex(6);
                        ImGui::Text("%s", tenGT.c_str());

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
                            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s", AdmissionConst::KET_QUA_TRUNG_TUYEN.c_str());
                        } else {
                            if (ts.kiemTraDiemLiet()) {
                                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Diem liet");
                            } else {
                                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", AdmissionConst::KET_QUA_KHONG_DAT.c_str());
                            }
                        }

                        // CỘT 12: THAO TÁC (SỬA & XÓA)
                        ImGui::TableSetColumnIndex(12);
                        ImGui::PushID((int)i);

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.80f, 1.0f));
                        if (ImGui::Button("Sua", ImVec2(45, 22))) {
                            selectedRow = (int)i;
                            const ThiSinh& sel = ql.getDanhSach()[i];
                            strcpy(bufSBD, sel.getSoBaoDanh().c_str());
                            strcpy(bufCCCD, sel.getSoCCCD().c_str());
                            strcpy(bufHoTen, sel.getHoTen().c_str());

                            string ns = sel.getNgaySinh();
                            if (ns.length() == 10) {
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

                            formQueQuanMa = sel.getQueQuan();
                            formGioiTinhMa = sel.getGioiTinh();
                            formToan = (float)sel.getDiemToan();
                            formLy = (float)sel.getDiemLy();
                            formHoa = (float)sel.getDiemHoa();
                            formErrorMsg = "";
                            showEditModal = true;
                        }
                        ImGui::PopStyleColor();

                        ImGui::SameLine();

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
                        if (ImGui::Button("Xoa", ImVec2(45, 22))) {
                            selectedRow = (int)i;
                            showDeleteModal = true;
                        }
                        ImGui::PopStyleColor();

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 2: CƠ SỞ DỮ LIỆU QUAN HỆ (CLASS TABLE 3NF) ====================
            if (ImGui::BeginTabItem("  2. Co so Du lieu Quan he (Class Table)  ")) {
                ImGui::Spacing();

                if (ImGui::BeginTabBar("SubTabsCSDL", ImGuiTabBarFlags_None)) {

                    // SUBTAB 2.1: BẢNG THỰC THỂ CHA NGƯỜI
                    if (ImGui::BeginTabItem("  Bang 1: NGUOI (data/nguoi.txt)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THUC THE CHA: NGUOI (Khoa chinh: soCCCD | Khoa ngoai: queQuan, gioiTinh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblNguoiRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("soCCCD (PK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("hoTen", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("ngaySinh", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("queQuan (FK)", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("gioiTinh (FK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ts.getSoCCCD().c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", ts.getHoTen().c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", ts.getNgaySinh().c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::Text("%s [%s]", ql.layTenTinhThanh(ts.getQueQuan()).c_str(), ts.getQueQuan().c_str());
                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%s [%s]", ql.layTenGioiTinh(ts.getGioiTinh()).c_str(), ts.getGioiTinh().c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.2: BẢNG THỰC THỂ CON THÍ SINH
                    if (ImGui::BeginTabItem("  Bang 2: THI SINH (data/thisinh.txt)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THUC THE CON: THI SINH (Khoa chinh: soBaoDanh | Khoa ngoai: soCCCD)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblThiSinhRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("soBaoDanh (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("soCCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("diemToan", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableSetupColumn("diemLy", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableSetupColumn("diemHoa", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoBaoDanh().c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ts.getSoCCCD().c_str());
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

                    // SUBTAB 2.3: BẢNG DANH MỤC TỈNH THÀNH
                    if (ImGui::BeginTabItem("  Bang 3: TINH THANH (data/tinhthanh.txt)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC: 34 TINH / THANH PHO (Khoa chinh: ma)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblTinhThanhRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("ma (PK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("ten", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("loai", ImGuiTableColumnFlags_WidthFixed, 260);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tt.ma.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", tt.ten.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", tt.loai.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.4: BẢNG DANH MỤC GIỚI TÍNH
                    if (ImGui::BeginTabItem("  Bang 4: GIOI TINH (data/gioitinh.txt)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC: GIOI TINH (Khoa chinh: ma)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblGioiTinhRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("ma (PK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("ten", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("moTa", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", gt.ma.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gt.ten.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", gt.moTa.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.5: BẢNG CẤU HÌNH HỆ THỐNG
                    if (ImGui::BeginTabItem("  Bang 5: HE THONG (data/system.txt)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THAM SO HE THONG & TUYEN SINH (data/system.txt)");
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("SystemCfgView", ImVec2(0, 180), true);
                        {
                            ImGui::BulletText("Truong Dao tao    : %s", ql.getTenTruong().c_str());
                            ImGui::BulletText("Nam Tuyen Sinh    : %s", ql.getNamTuyenSinh().c_str());
                            ImGui::BulletText("Diem Chuan San    : %.2f diem", ql.getDiemChuan());
                            ImGui::BulletText("Nguong Diem Liet  : %.2f diem", ql.getDiemLiet());
                            ImGui::BulletText("Tong So Thi Sinh  : %d thi sinh", (int)ql.getDanhSach().size());
                            ImGui::BulletText("Tong So Tinh Thanh: %d tinh/thanh", (int)ql.getDanhSachTinhThanh().size());
                            ImGui::BulletText("Tong So Gioi Tinh : %d gioi tinh", (int)ql.getDanhSachGioiTinh().size());
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.6: SƠ ĐỒ LIÊN KẾT ERD (5 BẢNG 3NF)
                    if (ImGui::BeginTabItem("  So do Lien ket ERD (PK - FK)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "SO DO CHI TIET LIEN KET KHOA CHINH (PK) VA KHOA NGOAI (FK) CHUAN 3NF:");
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));
                        ImGui::BeginChild("ERDView", ImVec2(0, ImGui::GetWindowHeight() - 250), true);
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), 
                                "+---------------------------------------------------------------------------------+\n"
                                "|                BANG 3: TINH THANH               BANG 4: GIOI TINH               |\n"
                                "|                   (tinhthanh.txt)                  (gioitinh.txt)               |\n"
                                "+------------------------------------+  +-----------------------------------------+\n"
                                "| * ma (PK)   : 01, 04, 48...        |  | * ma (PK)   : NAM, NU, KHAC...          |\n"
                                "|   ten       : TP. Da Nang...       |  |   ten       : Nam, Nu, Khac             |\n"
                                "|   loai      : Tinh / Thanh pho     |  |   moTa      : Mo ta chi tiet            |\n"
                                "+------------------------------------+  +-----------------------------------------+\n"
                                "                  ^                                          ^\n"
                                "                  | 1                                        | 1\n"
                                "                  |                                          |\n"
                                "                  | N (FK: queQuan)                          | N (FK: gioiTinh)\n"
                                "+---------------------------------------------------------------------------------+\n"
                                "|                   BANG 1: NGUOI (Tep luu tru: data/nguoi.txt)                   |\n"
                                "+---------------------------------------------------------------------------------+\n"
                                "|  * soCCCD (PRIMARY KEY - PK)  : So Can cuoc Cong dan (12 chu so)                |\n"
                                "|    hoTen                      : Ho va ten                                       |\n"
                                "|    ngaySinh                   : dd/mm/yyyy                                      |\n"
                                "|  * queQuan (FOREIGN KEY - FK) : Tham chieu den TINH THANH(ma)                   |\n"
                                "|  * gioiTinh (FOREIGN KEY - FK): Tham chieu den GIOI TINH(ma)                    |\n"
                                "+---------------------------------------------------------------------------------+\n"
                                "                                          ^\n"
                                "                                          | 1 (Quan he 1 - 1 qua Ke thua)\n"
                                "                                          |\n"
                                "                                          | 1 (FK: soCCCD)\n"
                                "+---------------------------------------------------------------------------------+\n"
                                "|                 BANG 2: THI SINH (Tep luu tru: data/thisinh.txt)                |\n"
                                "+---------------------------------------------------------------------------------+\n"
                                "|  * soBaoDanh (PRIMARY KEY - PK): So bao danh du thi (DUTxxx)                    |\n"
                                "|  * soCCCD (FOREIGN KEY - FK)   : Khoa ngoai tham chieu sang bang NGUOI          |\n"
                                "|    diemToan                   : Diem thi mon Toan (0.00 - 10.00)                |\n"
                                "|    diemLy                     : Diem thi mon Vat ly (0.00 - 10.00)              |\n"
                                "|    diemHoa                    : Diem thi mon Hoa hoc (0.00 - 10.00)             |\n"
                                "+---------------------------------------------------------------------------------+\n\n"
                                "[*] BANG 5: data/system.txt (Truong, Nam tuyen sinh, Diem chuan, Diem liet)\n"
                                "[*] BAO CAO: data/ketqua_xettuyen.txt, data/thukhoa.txt, data/thongke_phodiem.txt\n");
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 3: XÉT TUYỂN & THỐNG KÊ PHỔ ĐIỂM ====================
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

                // 2 CỘT THỐNG KÊ
                ImGui::Columns(2, "AdmissionCols", true);

                // CỘT 1: THỦ KHOA
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "DANH SACH THU KHOA KY THI TUYEN SINH:");
                ImGui::Separator();

                Vector<ThiSinh> dsTK = ql.getDanhSachThuKhoa();
                if (dsTK.empty()) {
                    ImGui::Text("(Chua co thi sinh nao trong CSDL)");
                } else {
                    if (ImGui::BeginTable("ThuKhoaTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 270))) {
                        ImGui::TableSetupColumn("SBD", ImGuiTableColumnFlags_WidthFixed, 80);
                        ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 85);
                        ImGui::TableSetupColumn("Danh Hieu", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableHeadersRow();

                        for (size_t i = 0; i < dsTK.size(); ++i) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", dsTK[i].getSoBaoDanh().c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", dsTK[i].getHoTen().c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", ql.layTenTinhThanh(dsTK[i].getQueQuan()).c_str());
                            ImGui::TableSetColumnIndex(3);
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%.2f", dsTK[i].tinhTongDiem());
                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "THU KHOA");
                        }
                        ImGui::EndTable();
                    }
                }

                ImGui::NextColumn();

                // CỘT 2: PHỔ ĐIỂM
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "PHAN TICH PHO DIEM THI (5 MUC PHAN LOAI):");
                ImGui::Separator();

                int thap, tb, kha, cao, ratCao;
                ql.tinhThongKePhoDiem(thap, tb, kha, cao, ratCao);
                int totalTS = (int)ql.getDanhSach().size();

                auto veMucDiem = [&](const char* label, const char* range, int count, ImVec4 color) {
                    float pct = (totalTS > 0) ? ((float)count / totalTS) : 0.0f;
                    ImGui::Text("%s (%s):", label, range);
                    ImGui::SameLine(ImGui::GetColumnWidth() - 110);
                    ImGui::TextColored(color, "%d thi sinh (%.1f%%)", count, pct * 100.0f);
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
                    ImGui::ProgressBar(pct, ImVec2(-1, 18), "");
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                };

                ImGui::BeginChild("HistogramView", ImVec2(0, ImGui::GetWindowHeight() - 270), true);
                {
                    veMucDiem(AdmissionConst::TEN_MUC_XUAT_SAC.c_str(), ">= 27.00", ratCao, ImVec4(0.2f, 0.85f, 0.4f, 1.0f));
                    veMucDiem(AdmissionConst::TEN_MUC_GIOI.c_str(), "24.00 - 26.99", cao, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
                    veMucDiem(AdmissionConst::TEN_MUC_KHA.c_str(), "21.00 - 23.99", kha, ImVec4(0.9f, 0.8f, 0.2f, 1.0f));
                    veMucDiem(AdmissionConst::TEN_MUC_TRUNG_BINH.c_str(), "15.00 - 20.99", tb, ImVec4(0.95f, 0.55f, 0.2f, 1.0f));
                    veMucDiem(AdmissionConst::TEN_MUC_THAP.c_str(), "< 15.00", thap, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                }
                ImGui::EndChild();

                ImGui::Columns(1);
                ImGui::EndTabItem();
            }
            // ==================== TAB 4: QUẢN LÝ DANH MỤC HỆ THỐNG (MỚI ✨) ====================
            if (ImGui::BeginTabItem("  4. Quan ly Danh muc He thong  ")) {
                ImGui::Spacing();

                if (ImGui::BeginTabBar("SubTabsDanhMuc", ImGuiTabBarFlags_None)) {

                    // SUBTAB 4.1: QUẢN LÝ TỈNH / THÀNH PHỐ
                    if (ImGui::BeginTabItem("  Danh muc Tinh / Thanh pho  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "QUAN LY DANH MUC 34 TINH THANH (THEM / SUA / XOA):");
                        ImGui::Separator();

                        // Form thêm tỉnh thành mới
                        ImGui::BeginChild("AddTinhThanhForm", ImVec2(0, 75), true);
                        {
                            ImGui::SetNextItemWidth(80);
                            ImGui::InputTextWithHint("##newMaTinh", "Ma tinh", bufNewMaTinh, sizeof(bufNewMaTinh));
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputTextWithHint("##newTenTinh", "Ten Tinh/TP", bufNewTenTinh, sizeof(bufNewTenTinh));
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(260);
                            ImGui::InputTextWithHint("##newLoaiTinh", "Loai hinh (TP/Tinh)", bufNewLoaiTinh, sizeof(bufNewLoaiTinh));
                            ImGui::SameLine();

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                            if (ImGui::Button("[+] Them Tinh Thanh", ImVec2(160, 26))) {
                                string m = Utils::trim(bufNewMaTinh);
                                string t = Utils::trim(bufNewTenTinh);
                                string l = Utils::trim(bufNewLoaiTinh);
                                if (m.empty() || t.empty()) {
                                    msgTinhThanh = "Ma va Ten tinh thanh khong duoc de trong!";
                                } else {
                                    TinhThanh ttMoi(m, t, l);
                                    if (ql.themTinhThanh(ttMoi)) {
                                        msgTinhThanh = "Da them tinh thanh moi thanh cong!";
                                        bufNewMaTinh[0] = '\0';
                                        bufNewTenTinh[0] = '\0';
                                        showToast = true;
                                        toastMsg = msgTinhThanh;
                                    } else {
                                        msgTinhThanh = "Trung ma tinh thanh voi ban ghi da co!";
                                    }
                                }
                            }
                            ImGui::PopStyleColor();

                            if (!msgTinhThanh.empty()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", msgTinhThanh.c_str());
                            }
                        }
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // Bảng danh mục Tỉnh thành
                        if (ImGui::BeginTable("TblManageTinhThanh", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 320))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Tinh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Don Vi Hanh Chinh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Loai Hinh", ImGuiTableColumnFlags_WidthFixed, 260);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tt.ma.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", tt.ten.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", tt.loai.c_str());

                                ImGui::TableSetColumnIndex(4);
                                ImGui::PushID((int)(1000 + i));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                                if (ImGui::SmallButton("Xoa")) {
                                    if (ql.xoaTinhThanh(tt.ma)) {
                                        msgTinhThanh = "Da xoa tinh thanh thanh cong!";
                                        showToast = true;
                                        toastMsg = msgTinhThanh;
                                    } else {
                                        msgTinhThanh = "Khong the xoa vi co thi sinh dang tham chieu den tinh nay!";
                                    }
                                }
                                ImGui::PopStyleColor();
                                ImGui::PopID();
                            }
                            ImGui::EndTable();
                        }

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.2: QUẢN LÝ GIỚI TÍNH
                    if (ImGui::BeginTabItem("  Danh muc Gioi tinh  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "QUAN LY DANH MUC GIOI TINH (TRUYEN THONG & MO RONG):");
                        ImGui::Separator();

                        // Form thêm giới tính mới
                        ImGui::BeginChild("AddGioiTinhForm", ImVec2(0, 75), true);
                        {
                            ImGui::SetNextItemWidth(100);
                            ImGui::InputTextWithHint("##newMaGT", "Ma (NAM/NU/..)", bufNewMaGT, sizeof(bufNewMaGT));
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(180);
                            ImGui::InputTextWithHint("##newTenGT", "Ten hien thi", bufNewTenGT, sizeof(bufNewTenGT));
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(260);
                            ImGui::InputTextWithHint("##newDescGT", "Ghi chu mo ta", bufNewDescGT, sizeof(bufNewDescGT));
                            ImGui::SameLine();

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                            if (ImGui::Button("[+] Them Gioi Tinh", ImVec2(160, 26))) {
                                string m = Utils::toUpper(Utils::trim(bufNewMaGT));
                                string t = Utils::trim(bufNewTenGT);
                                string d = Utils::trim(bufNewDescGT);
                                if (m.empty() || t.empty()) {
                                    msgGioiTinh = "Ma va Ten gioi tinh khong duoc de trong!";
                                } else {
                                    GioiTinh gtMoi(m, t, d);
                                    if (ql.themGioiTinh(gtMoi)) {
                                        msgGioiTinh = "Da them gioi tinh moi thanh cong!";
                                        bufNewMaGT[0] = '\0';
                                        bufNewTenGT[0] = '\0';
                                        bufNewDescGT[0] = '\0';
                                        showToast = true;
                                        toastMsg = msgGioiTinh;
                                    } else {
                                        msgGioiTinh = "Trung ma gioi tinh voi danh muc da co!";
                                    }
                                }
                            }
                            ImGui::PopStyleColor();

                            if (!msgGioiTinh.empty()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", msgGioiTinh.c_str());
                            }
                        }
                        ImGui::EndChild();

                        ImGui::Spacing();

                        // Bảng danh mục Giới tính
                        if (ImGui::BeginTable("TblManageGioiTinh", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 320))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Hien Thi", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("Mo Ta Chi Tiet", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", gt.ma.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gt.ten.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", gt.moTa.c_str());

                                ImGui::TableSetColumnIndex(4);
                                ImGui::PushID((int)(2000 + i));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                                if (ImGui::SmallButton("Xoa")) {
                                    if (ql.xoaGioiTinh(gt.ma)) {
                                        msgGioiTinh = "Da xoa gioi tinh thanh cong!";
                                        showToast = true;
                                        toastMsg = msgGioiTinh;
                                    } else {
                                        msgGioiTinh = "Khong the xoa vi co thi sinh dang tham chieu den gioi tinh nay!";
                                    }
                                }
                                ImGui::PopStyleColor();
                                ImGui::PopID();
                            }
                            ImGui::EndTable();
                        }

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.3: THAM SỐ TUYỂN SINH & CẤU HÌNH HỆ THỐNG
                    if (ImGui::BeginTabItem("  Tham so Tuyen sinh & Cau hinh  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "THIET LAP THAM SO KY THI (data/system.txt):");
                        ImGui::Separator();

                        ImGui::BeginChild("SystemParamsChild", ImVec2(0, 300), true);
                        {
                            ImGui::Text("Ten Truong Dai Hoc:");
                            ImGui::SetNextItemWidth(450);
                            ImGui::InputText("##truongSys", bufTenTruong, sizeof(bufTenTruong));

                            ImGui::Spacing();
                            ImGui::Text("Nam Tuyen Sinh:");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputText("##namSys", bufNamTuyenSinh, sizeof(bufNamTuyenSinh));

                            ImGui::Spacing();
                            ImGui::Text("Diem Chuan Xet Tuyen (thang 30):");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputFloat("##dcSys", &formDiemChuan, 0.25f, 1.0f, "%.2f");

                            ImGui::Spacing();
                            ImGui::Text("Nguong Diem Liet Mon Thi (mac dinh 1.00):");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputFloat("##dlSys", &formDiemLiet, 0.25f, 1.0f, "%.2f");

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.75f, 1.0f));
                            if (ImGui::Button("[ Luu Cau Hinh He Thong ]", ImVec2(220, 32))) {
                                ql.setTenTruong(Utils::trim(bufTenTruong));
                                ql.setNamTuyenSinh(Utils::trim(bufNamTuyenSinh));
                                ql.setDiemChuan((double)formDiemChuan);
                                ql.setDiemLiet((double)formDiemLiet);
                                showToast = true;
                                toastMsg = "Da luu cau hinh he thong va dong bo bao cao thanh cong!";
                            }
                            ImGui::PopStyleColor();
                        }
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 5: LƯU TRỮ & XUẤT BÁO CÁO ====================
            if (ImGui::BeginTabItem("  5. Luu tru & Xuat Bao cao  ")) {
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Quan tri He quan tri Co so Du lieu MySQL (Laragon):");
                ImGui::Spacing();
                if (ql.isMySQLConnected()) {
                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "Trang thai: Da ket noi CSDL `pbl2_tuyensinh` (Host: 127.0.0.1:3306 - MySQL 8.4)");
                    if (ImGui::Button("[ Dong bo toan bo du lieu len MySQL ]", ImVec2(310, 34))) {
                        if (ql.dongBoLenMySQL()) {
                            showToast = true;
                            toastMsg = "Da dong bo toan bo du lieu len MySQL thanh cong!";
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("[ Tai du lieu tu MySQL ve Bo nho / Cache ]", ImVec2(330, 34))) {
                        if (ql.dongBoTuMySQL()) {
                            showToast = true;
                            toastMsg = "Da tai du lieu tu MySQL ve bo nho va cache thanh cong!";
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Trang thai: Chua ket noi MySQL Server (Vui long khoi dong Laragon / MySQL)");
                    if (ImGui::Button("[ Ket noi lai MySQL Server ]", ImVec2(240, 34))) {
                        if (ql.ketNoiMySQL()) {
                            ql.dongBoTuMySQL();
                            showToast = true;
                            toastMsg = "Ket noi MySQL thanh cong!";
                        } else {
                            showToast = true;
                            toastMsg = "Ket noi MySQL that bai! Kiem tra Laragon da chay chua.";
                        }
                    }
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Chuc nang quan tri sao luu he thong CSDL da tep (Key - Value):");
                ImGui::Spacing();

                if (ImGui::Button("[ Luu Toan Bo CSDL (nguoi.txt, thisinh.txt, tinhthanh.txt, gioitinh.txt, system.txt) ]", ImVec2(480, 36))) {
                    ql.luuDuLieu();
                    ql.luuDanhMucTinhThanh("data/tinhthanh.txt");
                    ql.luuDanhMucGioiTinh("data/gioitinh.txt");
                    ql.luuCauHinhHeThong("data/system.txt");
                    showToast = true;
                    toastMsg = "Da luu toan bo CSDL va danh muc he thong thanh cong!";
                }
                ImGui::SameLine();
                if (ImGui::Button("[ Xuat Tat Ca Bao Cao (Ket qua, Thu khoa, Pho diem) ]", ImVec2(410, 36))) {
                    ql.xuatTatCaBaoCao();
                    showToast = true;
                    toastMsg = "Da xuat ca 3 tep bao cao thanh cong!";
                    reportContent.clear();
                }

                ImGui::Spacing();
                ImGui::Separator();

                ImGui::Text("Chon tep bao cao xem truoc:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(260);
                const char* reportFiles[] = {
                    "ketqua_xettuyen.txt (Ket qua xet tuyen)",
                    "thukhoa.txt (Vinh danh thu khoa)",
                    "thongke_phodiem.txt (Phan tich pho diem)"
                };
                if (ImGui::Combo("##selReport", &selectedReportFile, reportFiles, 3)) {
                    reportContent.clear();
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

            // ==================== TAB 6: HƯỚNG DẪN & KIẾN TRÚC HỆ THỐNG ====================
            if (ImGui::BeginTabItem("  6. Huong dan & Kien truc  ")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "DO AN CO SO LAP TRINH (PBL2) - DE TAI 24");
                ImGui::BulletText("Truong : Dai hoc Bach khoa - Dai hoc Da Nang (DUT)");
                ImGui::BulletText("Tac gia: Vo Nhu Hieu");
                ImGui::BulletText("Hoc phan: PBL2 - Do an co so lap trinh");
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "KIEN TRUC DU LIEU DA TEP THEO CHUAN CSDL 3NF:");
                ImGui::BulletText("1. data/tinhthanh.txt: Bang danh muc TINH THANH (34 tinh thanh theo NQ 202/2025/QH15).");
                ImGui::BulletText("2. data/gioitinh.txt: Bang danh muc GIOI TINH (NAM, NU, KHAC).");
                ImGui::BulletText("3. data/nguoi.txt: Bang thuc the cha NGUOI (PK: soCCCD, FK: queQuan, FK: gioiTinh).");
                ImGui::BulletText("4. data/thisinh.txt: Bang thuc the con THI SINH (PK: soBaoDanh, FK: soCCCD).");
                ImGui::BulletText("5. data/system.txt: Cau hinh he thong (Truong, Nam tuyen sinh, Diem chuan, Diem liet).");
                ImGui::BulletText("6. data/ketqua_xettuyen.txt, data/thukhoa.txt, data/thongke_phodiem.txt: Bao cao dong.");
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

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("So Bao Danh (Tu dong tang):");
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##sbd", bufSBD, IM_ARRAYSIZE(bufSBD));
                ImGui::EndDisabled();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("So CCCD (12 so)*:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##cccd", bufCCCD, IM_ARRAYSIZE(bufCCCD));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Ho va Ten*:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##hoTen", bufHoTen, IM_ARRAYSIZE(bufHoTen));

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

                // Row 5: Quê quán (Searchable Combobox)
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Que Quan (Tinh/TP)*:");
                ImGui::TableSetColumnIndex(1);
                RenderSearchableComboTinhThanh("##comboQueQuanAdd", ql.getDanhSachTinhThanh(), formQueQuanMa);

                // Row 6: Giới tính
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Gioi Tinh*:");
                ImGui::TableSetColumnIndex(1);
                RenderComboGioiTinh("##comboGioiTinhAdd", ql.getDanhSachGioiTinh(), formGioiTinhMa);

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

            double tongDiem = formToan + formLy + formHoa;
            bool liet = (formToan <= ql.getDiemLiet() || formLy <= ql.getDiemLiet() || formHoa <= ql.getDiemLiet());
            bool pass = (!liet && tongDiem >= ql.getDiemChuan());

            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Tong Diem: %.2f  |  Ket Qua: %s", 
                tongDiem, (pass ? AdmissionConst::KET_QUA_TRUNG_TUYEN.c_str() : (liet ? "BI DIEM LIET" : AdmissionConst::KET_QUA_KHONG_DAT.c_str())));

            if (!formErrorMsg.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", formErrorMsg.c_str());
            }

            ImGui::Separator();
            if (ImGui::Button("Xac Nhan Them", ImVec2(120, 30))) {
                string sbd = Utils::toUpper(Utils::trim(bufSBD));
                string cccd = Utils::trim(bufCCCD);
                string hoten = Utils::chuanHoaTen(bufHoTen);
                
                char dateBuf[16];
                sprintf(dateBuf, "%02d/%02d/%04d", formNgay, formThang, formNam);
                string ns = dateBuf;

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
                    ThiSinh moi(cccd, hoten, ns, formQueQuanMa, formGioiTinhMa, sbd, (double)formToan, (double)formLy, (double)formHoa);

                    if (ql.themThiSinh(moi)) {
                        reportContent.clear();
                        showAddModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them thi sinh moi thanh cong!";
                    } else {
                        formErrorMsg = "Loi rang buoc tham chieu Que quan hoac Gioi tinh!";
                    }
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

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("So Bao Danh (SBD):");
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##sbdEd", bufSBD, IM_ARRAYSIZE(bufSBD));
                ImGui::EndDisabled();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("So CCCD (12 so)*:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##cccdEd", bufCCCD, IM_ARRAYSIZE(bufCCCD));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Ho va Ten*:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputText("##hoTenEd", bufHoTen, IM_ARRAYSIZE(bufHoTen));

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

                // Row 5: Quê quán (Searchable Combobox)
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Que Quan (Tinh/TP)*:");
                ImGui::TableSetColumnIndex(1);
                RenderSearchableComboTinhThanh("##comboQueQuanEdit", ql.getDanhSachTinhThanh(), formQueQuanMa);

                // Row 6: Giới tính
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Gioi Tinh*:");
                ImGui::TableSetColumnIndex(1);
                RenderComboGioiTinh("##comboGioiTinhEdit", ql.getDanhSachGioiTinh(), formGioiTinhMa);

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

                if (!Utils::kiemTraCCCD(cccd)) {
                    formErrorMsg = "So CCCD phai gom dung 12 chu so!";
                } else if (hoten.length() < 2) {
                    formErrorMsg = "Ho va ten phai co it nhat 2 ky tu!";
                } else if (!Utils::kiemTraNgaySinh(ns)) {
                    formErrorMsg = "Ngay sinh khong hop le!";
                } else {
                    ThiSinh capNhat(cccd, hoten, ns, formQueQuanMa, formGioiTinhMa, bufSBD, (double)formToan, (double)formLy, (double)formHoa);

                    if (ql.capNhatThiSinh(capNhat)) {
                        reportContent.clear();
                        showEditModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat ho so thi sinh thanh cong!";
                    } else {
                        formErrorMsg = "Trung so CCCD voi thi sinh khac hoac loi tham chieu!";
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
                ImGui::Text("Que Quan: %s", ql.layTenTinhThanh(sel.getQueQuan()).c_str());
                ImGui::Spacing();

                if (ImGui::Button("DONG Y XOA", ImVec2(120, 30))) {
                    ql.xoaThiSinh(sel.getSoBaoDanh());
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
            static float toastTimer = 0.0f;
            toastTimer += io.DeltaTime;
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 380, 20));
            ImGui::SetNextWindowSize(ImVec2(360, 50));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.45f, 0.25f, 0.95f));
            ImGui::Begin("Toast", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[+] %s", toastMsg.c_str());
            ImGui::End();
            ImGui::PopStyleColor();
            if (toastTimer > 3.0f) {
                showToast = false;
                toastTimer = 0.0f;
            }
        }

        ImGui::End();

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(24, 28, 36, 255);
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

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Direct3D 9 Helpers
static bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, 
                             D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
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
