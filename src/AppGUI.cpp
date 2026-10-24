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

    // Palette: Slate Blue / Charcoal / Teal & Cyan accents
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
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.30f, 0.45f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.30f, 0.45f, 0.65f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.60f, 0.85f, 0.90f);
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
            L"HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (CSDL QUAN HE 3NF & GUI)", 
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

        // Tải font hệ thống Segoe UI hoặc Arial nếu có
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
        static int filterNganhIdx = 0; // 0: Tat ca
        static int selectedRow = -1;

        // State modals
        static bool showAddModal = false;
        static bool showEditModal = false;
        static bool showDeleteModal = false;
        static bool showToast = false;
        static string toastMsg = "";

        // Form buffers
        static char bufMaTS[32] = "";
        static char bufSBD[32] = "";
        static char bufCMND[32] = "";
        static char bufHoTen[64] = "";
        static char bufNgaySinh[32] = "";
        static char bufQueQuan[64] = "";
        static int  formGioiTinh = 0; // 0: Nam, 1: Nu
        static int  formKVIdx = 0;
        static int  formNganhIdx = 0;
        static float formToan = 8.0f;
        static float formLy = 8.0f;
        static float formHoa = 8.0f;
        static string formErrorMsg = "";

        // Preview báo cáo file
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

            // Set main window to fill screen
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::Begin("MainPanel", nullptr, winFlags);

            // ==================== HEADER BAR ====================
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.16f, 0.24f, 1.0f));
            ImGui::BeginChild("HeaderBar", ImVec2(0, 72), true);
            {
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG | KHOA CONG NGHE THONG TIN");
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "PBL2 (DE TAI 24): HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC");
                ImGui::SameLine(ImGui::GetWindowWidth() - 360);
                ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.90f, 1.0f), "[ CSDL 3NF + Dear ImGui C++ ]");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();

            // ==================== TAB BAR ====================
            if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {

                // ---------------------------------------------------------------
                // TAB 1: QUẢN LÝ THÍ SINH
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  1. Quan ly Ho so Thi sinh  ")) {
                    ImGui::Spacing();

                    // Toolbar
                    ImGui::Text("Tim kiem nhanh:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(200);
                    ImGui::InputTextWithHint("##filter", "SBD, Ten, CMND, Que...", filterBuf, IM_ARRAYSIZE(filterBuf));

                    ImGui::SameLine();
                    ImGui::Text("Loc ket qua:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(130);
                    const char* statusNames[] = { "Tat ca", "Trung tuyen", "Khong dat" };
                    ImGui::Combo("##filterStatus", &filterStatus, statusNames, 3);

                    ImGui::SameLine();
                    ImGui::Text("Nganh:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(170);
                    vector<string> nganhFilterNames;
                    nganhFilterNames.push_back("Tat ca nganh");
                    for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) {
                        nganhFilterNames.push_back(ql.getDanhSachNganh()[i].getTenNganh());
                    }
                    vector<const char*> nganhFilterCStr;
                    for (size_t i = 0; i < nganhFilterNames.size(); ++i) nganhFilterCStr.push_back(nganhFilterNames[i].c_str());
                    ImGui::Combo("##filterNganh", &filterNganhIdx, nganhFilterCStr.data(), (int)nganhFilterCStr.size());

                    ImGui::SameLine(ImGui::GetWindowWidth() - 370);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                    if (ImGui::Button("[+] Them Thi Sinh")) {
                        showAddModal = true;
                        string newId = ql.sinhMaThiSinhMoi();
                        strcpy(bufMaTS, newId.c_str());
                        strcpy(bufSBD, "");
                        strcpy(bufCMND, "");
                        strcpy(bufHoTen, "");
                        strcpy(bufNgaySinh, "15/05/2005");
                        strcpy(bufQueQuan, "Da Nang");
                        formGioiTinh = 0;
                        formKVIdx = 0;
                        formNganhIdx = 0;
                        formToan = 8.0f;
                        formLy = 8.0f;
                        formHoa = 8.0f;
                        formErrorMsg = "";
                    }
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    bool hasSelection = (selectedRow >= 0 && selectedRow < ql.getDanhSachThiSinh().size());
                    if (!hasSelection) ImGui::BeginDisabled();
                    if (ImGui::Button("[Sua] Cap Nhat")) {
                        showEditModal = true;
                        const ThiSinh& sel = ql.getDanhSachThiSinh()[selectedRow];
                        strcpy(bufMaTS, sel.getMaThiSinh().c_str());
                        strcpy(bufSBD, sel.getSoBaoDanh().c_str());
                        strcpy(bufCMND, sel.getSoCMND().c_str());
                        strcpy(bufHoTen, sel.getHoTen().c_str());
                        strcpy(bufNgaySinh, sel.getNgaySinh().c_str());
                        strcpy(bufQueQuan, sel.getQueQuan().c_str());
                        formGioiTinh = (sel.getGioiTinh() == "Nu" ? 1 : 0);
                        formKVIdx = ql.timViTriKhuVuc(sel.getMaKhuVuc());
                        if (formKVIdx < 0) formKVIdx = 0;
                        formNganhIdx = ql.timViTriNganh(sel.getMaNganh());
                        if (formNganhIdx < 0) formNganhIdx = 0;
                        formToan = (float)sel.getDiemToan();
                        formLy = (float)sel.getDiemLy();
                        formHoa = (float)sel.getDiemHoa();
                        formErrorMsg = "";
                    }
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.22f, 0.22f, 1.0f));
                    if (ImGui::Button("[Xoa] Xoa")) {
                        showDeleteModal = true;
                    }
                    ImGui::PopStyleColor();
                    if (!hasSelection) ImGui::EndDisabled();

                    ImGui::Separator();

                    // Quick Sort Buttons
                    ImGui::Text("Sap xep:");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Diem Giam Dan")) {
                        ql.sapXepTheoTongDiemGiamDan();
                        selectedRow = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Ten A-Z")) {
                        ql.sapXepTheoTenAZ();
                        selectedRow = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("SBD Tang Dan")) {
                        ql.sapXepTheoSBDTangDan();
                        selectedRow = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Theo Nganh")) {
                        ql.sapXepTheoNganh();
                        selectedRow = -1;
                    }

                    ImGui::SameLine(ImGui::GetWindowWidth() - 250);
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Tong so: %d thi sinh", ql.getDanhSachThiSinh().size());

                    // TABLE HIỂN THỊ
                    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                                                 ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | 
                                                 ImGuiTableFlags_ScrollX;

                    float tableHeight = ImGui::GetWindowHeight() - 230;
                    if (tableHeight < 250) tableHeight = 250;

                    if (ImGui::BeginTable("CandidatesTable", 17, tableFlags, ImVec2(0, tableHeight))) {
                        ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                        ImGui::TableSetupColumn("Ma TS (PK)", ImGuiTableColumnFlags_WidthFixed, 75);
                        ImGui::TableSetupColumn("SBD", ImGuiTableColumnFlags_WidthFixed, 75);
                        ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthFixed, 150);
                        ImGui::TableSetupColumn("CMND/CCCD", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 90);
                        ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableSetupColumn("Phai", ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableSetupColumn("KV", ImGuiTableColumnFlags_WidthFixed, 45);
                        ImGui::TableSetupColumn("Nganh Dang Ky", ImGuiTableColumnFlags_WidthFixed, 180);
                        ImGui::TableSetupColumn("Toan", ImGuiTableColumnFlags_WidthFixed, 45);
                        ImGui::TableSetupColumn("Ly", ImGuiTableColumnFlags_WidthFixed, 45);
                        ImGui::TableSetupColumn("Hoa", ImGuiTableColumnFlags_WidthFixed, 45);
                        ImGui::TableSetupColumn("UT", ImGuiTableColumnFlags_WidthFixed, 45);
                        ImGui::TableSetupColumn("Tong", ImGuiTableColumnFlags_WidthFixed, 55);
                        ImGui::TableSetupColumn("Xep Loai", ImGuiTableColumnFlags_WidthFixed, 80);
                        ImGui::TableSetupColumn("Ket Qua", ImGuiTableColumnFlags_WidthFixed, 110);
                        ImGui::TableHeadersRow();

                        string filterStr = Utils::toLower(Utils::trim(filterBuf));
                        string selectedNganhMa = (filterNganhIdx > 0 && filterNganhIdx - 1 < ql.getDanhSachNganh().size())
                                                 ? ql.getDanhSachNganh()[filterNganhIdx - 1].getMaNganh() : "";

                        int visibleCount = 0;
                        for (int i = 0; i < ql.getDanhSachThiSinh().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSachThiSinh()[i];

                            // Filter text
                            if (!filterStr.empty()) {
                                bool match = Utils::containsIgnoreCase(ts.getSoBaoDanh(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getHoTen(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getMaThiSinh(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getSoCMND(), filterStr) ||
                                             Utils::containsIgnoreCase(ts.getQueQuan(), filterStr) ||
                                             Utils::containsIgnoreCase(ql.getTenNganh(ts.getMaNganh()), filterStr);
                                if (!match) continue;
                            }

                            // Filter ngành
                            if (!selectedNganhMa.empty() && ts.getMaNganh() != selectedNganhMa) {
                                continue;
                            }

                            double dc = ql.getDiemChuanNganh(ts.getMaNganh());
                            bool pass = ts.xetTrungTuyen(dc);

                            // Filter status
                            if (filterStatus == 1 && !pass) continue;
                            if (filterStatus == 2 && pass) continue;

                            visibleCount++;
                            ImGui::TableNextRow();

                            bool isSelected = (selectedRow == i);
                            ImGui::TableSetColumnIndex(0);
                            char rowLabel[32];
                            sprintf(rowLabel, "%d", visibleCount);
                            if (ImGui::Selectable(rowLabel, isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                                selectedRow = i;
                            }

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getMaThiSinh().c_str());

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", ts.getSoBaoDanh().c_str());

                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%s", ts.getHoTen().c_str());

                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%s", ts.getSoCMND().c_str());

                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%s", ts.getNgaySinh().c_str());

                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%s", ts.getQueQuan().c_str());

                            ImGui::TableSetColumnIndex(7);
                            ImGui::Text("%s", ts.getGioiTinh().c_str());

                            ImGui::TableSetColumnIndex(8);
                            ImGui::Text("%s", ts.getMaKhuVuc().c_str());

                            ImGui::TableSetColumnIndex(9);
                            ImGui::Text("%s", ql.getTenNganh(ts.getMaNganh()).c_str());

                            ImGui::TableSetColumnIndex(10);
                            ImGui::Text("%.2f", ts.getDiemToan());

                            ImGui::TableSetColumnIndex(11);
                            ImGui::Text("%.2f", ts.getDiemLy());

                            ImGui::TableSetColumnIndex(12);
                            ImGui::Text("%.2f", ts.getDiemHoa());

                            ImGui::TableSetColumnIndex(13);
                            ImGui::Text("+%.2f", ts.getDiemUuTien());

                            ImGui::TableSetColumnIndex(14);
                            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%.2f", ts.tinhTongDiem());

                            ImGui::TableSetColumnIndex(15);
                            ImGui::Text("%s", ts.xepLoaiHocLuc().c_str());

                            ImGui::TableSetColumnIndex(16);
                            if (pass) {
                                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "TRUNG TUYEN");
                            } else {
                                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Khong dat");
                            }
                        }
                        ImGui::EndTable();
                    }

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 2: CƠ SỞ DỮ LIỆU QUAN HỆ 3NF
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  2. Co so Du lieu Quan he (3NF)  ")) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.18f, 0.25f, 1.0f));
                    ImGui::BeginChild("DbExpl", ImVec2(0, 95), true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "KIEN TRUC CO SO DU LIEU QUAN HE (RELATIONAL DATABASE) CHUAN HOA 3NF:");
                        ImGui::BulletText("Tach thuc the thanh cac file rieng biet: danhmuc_khuvuc.txt, danhmuc_nganh.txt, hoso_thisinh.txt, dangky_thi.txt");
                        ImGui::BulletText("Loai bo hoan toan du thua du lieu (Data Redundancy) va di thuong cap nhat (Update Anomaly).");
                        ImGui::BulletText("Quan ly qua Khoa Chinh (PK) 'MaTS', dam bao neu nhieu thi sinh trung ho ten thi he thong van kiem soat 100% chinh xac!");
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    // Sub-tabs cho các bảng CSDL
                    if (ImGui::BeginTabBar("SubTabsDb", ImGuiTabBarFlags_None)) {
                        
                        // --------------------------------------------------------
                        // SUBTAB 2.1: DANH MỤC KHU VỰC & NGÀNH
                        // --------------------------------------------------------
                        if (ImGui::BeginTabItem("  Bang 1 & 2: Danh Muc Khu Vuc & Nganh  ")) {
                            ImGui::Spacing();
                            ImVec2 halfSize = ImVec2((ImGui::GetWindowWidth() - 40) * 0.5f, 260);

                            // Bảng 1: Khu Vực
                            ImGui::BeginChild("TableKhuVuc", halfSize, true);
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "BANG 1: DANH MUC KHU VUC (danhmuc_khuvuc.txt)");
                                ImGui::Separator();
                                if (ImGui::BeginTable("TblKV", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                    ImGui::TableSetupColumn("Ma KV (PK)", ImGuiTableColumnFlags_WidthFixed, 100);
                                    ImGui::TableSetupColumn("Ten Khu Vuc", ImGuiTableColumnFlags_WidthStretch);
                                    ImGui::TableSetupColumn("Diem Uu Tien", ImGuiTableColumnFlags_WidthFixed, 110);
                                    ImGui::TableHeadersRow();

                                    for (int i = 0; i < ql.getDanhSachKhuVuc().size(); ++i) {
                                        const KhuVuc& kv = ql.getDanhSachKhuVuc()[i];
                                        ImGui::TableNextRow();
                                        ImGui::TableSetColumnIndex(0);
                                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", kv.getMaKhuVuc().c_str());
                                        ImGui::TableSetColumnIndex(1);
                                        ImGui::Text("%s", kv.getTenKhuVuc().c_str());
                                        ImGui::TableSetColumnIndex(2);
                                        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "+%.2f diem", kv.getDiemUuTien());
                                    }
                                    ImGui::EndTable();
                                }
                            }
                            ImGui::EndChild();

                            ImGui::SameLine();

                            // Bảng 2: Ngành Xét Tuyển
                            ImGui::BeginChild("TableNganh", halfSize, true);
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "BANG 2: DANH MUC NGANH DAO TAO (danhmuc_nganh.txt)");
                                ImGui::Separator();
                                if (ImGui::BeginTable("TblNg", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                    ImGui::TableSetupColumn("Ma Nganh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                                    ImGui::TableSetupColumn("Ten Nganh", ImGuiTableColumnFlags_WidthStretch);
                                    ImGui::TableSetupColumn("To Hop", ImGuiTableColumnFlags_WidthFixed, 60);
                                    ImGui::TableSetupColumn("Chi Tieu", ImGuiTableColumnFlags_WidthFixed, 70);
                                    ImGui::TableSetupColumn("Diem Chuan", ImGuiTableColumnFlags_WidthFixed, 90);
                                    ImGui::TableHeadersRow();

                                    for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) {
                                        const NganhXetTuyen& ng = ql.getDanhSachNganh()[i];
                                        ImGui::TableNextRow();
                                        ImGui::TableSetColumnIndex(0);
                                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ng.getMaNganh().c_str());
                                        ImGui::TableSetColumnIndex(1);
                                        ImGui::Text("%s", ng.getTenNganh().c_str());
                                        ImGui::TableSetColumnIndex(2);
                                        ImGui::Text("%s", ng.getToHopMon().c_str());
                                        ImGui::TableSetColumnIndex(3);
                                        ImGui::Text("%d", ng.getChiTieu());
                                        ImGui::TableSetColumnIndex(4);
                                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%.2f", ng.getDiemChuan());
                                    }
                                    ImGui::EndTable();
                                }
                            }
                            ImGui::EndChild();

                            ImGui::Spacing();

                            // Điều chỉnh Điểm chuẩn & Chỉ tiêu trực tiếp
                            ImGui::BeginChild("AdjustNganh", ImVec2(0, 95), true);
                            {
                                ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "THIET LAP DIEM CHUAN & CHI TIEU THEO TUNG NGANH:");
                                static int selNgAdjust = 0;
                                static float newDC = 25.0f;
                                static int newCT = 100;

                                ImGui::SetNextItemWidth(250);
                                vector<string> ngList;
                                for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) ngList.push_back(ql.getDanhSachNganh()[i].getTenNganh());
                                vector<const char*> ngCList;
                                for (size_t i = 0; i < ngList.size(); ++i) ngCList.push_back(ngList[i].c_str());
                                if (ImGui::Combo("Chon Nganh", &selNgAdjust, ngCList.data(), (int)ngCList.size())) {
                                    newDC = (float)ql.getDanhSachNganh()[selNgAdjust].getDiemChuan();
                                    newCT = ql.getDanhSachNganh()[selNgAdjust].getChiTieu();
                                }

                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100);
                                ImGui::InputFloat("Diem Chuan Moi", &newDC, 0.25f, 1.0f, "%.2f");

                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(90);
                                ImGui::InputInt("Chi Tieu Moi", &newCT);

                                ImGui::SameLine();
                                if (ImGui::Button("Luu Thay Doi")) {
                                    string maNg = ql.getDanhSachNganh()[selNgAdjust].getMaNganh();
                                    ql.capNhatDiemChuanNganh(maNg, (double)newDC);
                                    ql.capNhatChiTieuNganh(maNg, newCT);
                                    showToast = true;
                                    toastMsg = "Cap nhat thong so nganh thanh cong!";
                                }
                            }
                            ImGui::EndChild();

                            ImGui::EndTabItem();
                        }

                        // --------------------------------------------------------
                        // SUBTAB 2.2: BẢNG 3: HỒ SƠ THÍ SINH (hoso_thisinh.txt)
                        // --------------------------------------------------------
                        if (ImGui::BeginTabItem("  Bang 3: Ho So Thi Sinh (hoso_thisinh.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG 3: HO SO NHAN THAN THI SINH (Tep: data/hoso_thisinh.txt)");
                            ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Dac diem: 'MaTS' la Khoa chinh (PK), 'MaKV' la Khoa ngoai (FK). Nhieu thi sinh trung ten van phan biet hoan toan nho MaTS!");
                            ImGui::Separator();

                            float hTable = ImGui::GetWindowHeight() - 250;
                            if (hTable < 200) hTable = 200;

                            if (ImGui::BeginTable("TblHoSoRaw", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, hTable))) {
                                ImGui::TableSetupColumn("Ma TS (PK)", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("So CMND / CCCD", ImGuiTableColumnFlags_WidthFixed, 140);
                                ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 110);
                                ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 140);
                                ImGui::TableSetupColumn("Gioi Tinh", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableSetupColumn("Ma KV (FK)", ImGuiTableColumnFlags_WidthFixed, 90);
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < ql.getDanhSachThiSinh().size(); ++i) {
                                    const ThiSinh& ts = ql.getDanhSachThiSinh()[i];
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getMaThiSinh().c_str());
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%s", ts.getSoCMND().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    // Highlight nếu là thí sinh trùng tên
                                    if (ts.getHoTen() == "Nguyen Van An") {
                                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s (Trung ten)", ts.getHoTen().c_str());
                                    } else {
                                        ImGui::Text("%s", ts.getHoTen().c_str());
                                    }
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::Text("%s", ts.getNgaySinh().c_str());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%s", ts.getQueQuan().c_str());
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::Text("%s", ts.getGioiTinh().c_str());
                                    ImGui::TableSetColumnIndex(6);
                                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", ts.getMaKhuVuc().c_str());
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // --------------------------------------------------------
                        // SUBTAB 2.3: BẢNG 4: ĐĂNG KÝ VÀ ĐIỂM THI (dangky_thi.txt)
                        // --------------------------------------------------------
                        if (ImGui::BeginTabItem("  Bang 4: Dang Ky & Diem Thi (dangky_thi.txt)  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG 4: DANG KY NGANH & DIEM THI CAC MON (Tep: data/dangky_thi.txt)");
                            ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Dac diem: 'SBD' la Khoa chinh (PK), 'MaTS' va 'MaNganh' la cac Khoa ngoai (FK) lien ket CSDL.");
                            ImGui::Separator();

                            float hTable = ImGui::GetWindowHeight() - 250;
                            if (hTable < 200) hTable = 200;

                            if (ImGui::BeginTable("TblDangKyRaw", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, hTable))) {
                                ImGui::TableSetupColumn("So Bao Danh (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                                ImGui::TableSetupColumn("Ma TS (FK)", ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("Ma Nganh (FK)", ImGuiTableColumnFlags_WidthFixed, 110);
                                ImGui::TableSetupColumn("Ten Nganh Dang Ky", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Diem Toan", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableSetupColumn("Diem Ly", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableSetupColumn("Diem Hoa", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < ql.getDanhSachThiSinh().size(); ++i) {
                                    const ThiSinh& ts = ql.getDanhSachThiSinh()[i];
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "%s", ts.getSoBaoDanh().c_str());
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getMaThiSinh().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::Text("%s", ts.getMaNganh().c_str());
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::Text("%s", ql.getTenNganh(ts.getMaNganh()).c_str());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%.2f", ts.getDiemToan());
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::Text("%.2f", ts.getDiemLy());
                                    ImGui::TableSetColumnIndex(6);
                                    ImGui::Text("%.2f", ts.getDiemHoa());
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }

                        // --------------------------------------------------------
                        // SUBTAB 2.4: XEM TRỰC TIẾP CÁC FILE TEXT CSDL
                        // --------------------------------------------------------
                        if (ImGui::BeginTabItem("  So do Lien Ket ERD 4 Bang  ")) {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "SO DO CHI TIET LIEN KET KHOA CHINH (PK) VA KHOA NGOAI (FK):");
                            ImGui::Separator();

                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));
                            ImGui::BeginChild("ERDView", ImVec2(0, ImGui::GetWindowHeight() - 250), true);
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), 
                                    "+--------------------------+           +--------------------------+\n"
                                    "|   KhuVuc (danhmuc_khuvuc)|           | NganhXetTuyen(danhmuc_ng)| \n"
                                    "+--------------------------+           +--------------------------+\n"
                                    "| * maKhuVuc (PK)          |           | * maNganh (PK)           |\n"
                                    "|   tenKhuVuc              |           |   tenNganh               |\n"
                                    "|   diemUuTien             |           |   toHopMon               |\n"
                                    "+--------------------------+           |   chiTieu                |\n"
                                    "            ^                          |   diemChuan              |\n"
                                    "            | 1                        +--------------------------+\n"
                                    "            |                                       ^\n"
                                    "            | N (FK: maKhuVuc)                      | 1\n"
                                    "+--------------------------+                        |\n"
                                    "|HoSoThiSinh(hoso_thisinh) |                        | N (FK: maNganh)\n"
                                    "+--------------------------+                        |\n"
                                    "| * maThiSinh (PK)         |                        |\n"
                                    "|   soCMND (Unique)        |           +--------------------------+\n"
                                    "|   hoTen                  |           | DangKyThi (dangky_thi)   |\n"
                                    "|   ngaySinh               | 1       N +--------------------------+\n"
                                    "|   queQuan                +-----------> * soBaoDanh (PK)         |\n"
                                    "|   gioiTinh               |           | # maThiSinh (FK)         |\n"
                                    "| # maKhuVuc (FK)          |           | # maNganh (FK)           |\n"
                                    "+--------------------------+           |   diemToan, diemLy, Hoa  |\n"
                                    "                                       +--------------------------+\n"
                                );
                                ImGui::Spacing();
                                ImGui::TextColored(ImVec4(0.8f, 0.9f, 0.8f, 1.0f),
                                    "GIAI THICH:\n"
                                    "- Bang 1 (danhmuc_khuvuc.txt): Quan ly diem cong theo khu vuc (0.75, 0.5, 0.25, 0.0).\n"
                                    "- Bang 2 (danhmuc_nganh.txt): Quan ly chi tieu va diem chuan rieng cua tung nganh hoc.\n"
                                    "- Bang 3 (hoso_thisinh.txt): Quan ly ho so ca nhan, moi nguoi co 1 MaTS duy nhat.\n"
                                    "- Bang 4 (dangky_thi.txt): Quan ly ket qua thi va nganh dang ky cua thi sinh do.\n"
                                    "=> Khong du thua du lieu, khong di thuong, giai quyet 100% van de trung ho ten!");
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
                // TAB 3: THỐNG KÊ & PHỔ ĐIỂM & THỦ KHOA
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  3. Thong ke & Pho diem & Thu khoa  ")) {
                    ImGui::Spacing();

                    // BANNER THỦ KHOA TOÀN TRƯỜNG
                    Vector<ThiSinh> dsThuKhoa = ql.getDanhSachThuKhoa();
                    if (!dsThuKhoa.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.20f, 0.18f, 0.10f, 1.0f));
                        ImGui::BeginChild("BannerTK", ImVec2(0, 90), true);
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "★ ★ ★ VINH DANH THU KHOA KY THI TUYEN SINH DAI HOC ★ ★ ★");
                            ImGui::Separator();
                            for (int i = 0; i < dsThuKhoa.size(); ++i) {
                                const ThiSinh& tk = dsThuKhoa[i];
                                ImGui::Text("Thi sinh: %s (SBD: %s)  |  Que quan: %s  |  Nganh: %s  |  TONG DIEM: %.2f DIEM!",
                                    tk.getHoTen().c_str(), tk.getSoBaoDanh().c_str(), tk.getQueQuan().c_str(),
                                    ql.getTenNganh(tk.getMaNganh()).c_str(), tk.tinhTongDiem());
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }

                    ImGui::Spacing();

                    // PHỔ ĐIỂM
                    int kem = 0, tb = 0, kha = 0, gioi = 0, xuatSac = 0;
                    ql.tinhThongKePhoDiem(kem, tb, kha, gioi, xuatSac);
                    int total = ql.getDanhSachThiSinh().size();

                    ImVec2 halfCol = ImVec2((ImGui::GetWindowWidth() - 40) * 0.5f, 320);

                    ImGui::BeginChild("PhoDiemCol", halfCol, true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "THONG KE PHO DIEM HOC LUC (Tong so: %d TS):", total);
                        ImGui::Separator();

                        auto renderBar = [total](const char* label, int count, ImVec4 color) {
                            float ratio = (total > 0) ? (float)count / total : 0.0f;
                            ImGui::Text("%-22s: %3d TS (%5.1f%%)", label, count, ratio * 100.0f);
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
                            char overlay[32];
                            sprintf(overlay, "%d TS", count);
                            ImGui::ProgressBar(ratio, ImVec2(-1, 18), overlay);
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        };

                        renderBar("Xuat sac (>= 27.0)", xuatSac, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                        renderBar("Gioi     (24.0 - <27.0)", gioi, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
                        renderBar("Kha      (21.0 - <24.0)", kha, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                        renderBar("Trung TB (15.0 - <21.0)", tb, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                        renderBar("Yeu      (< 15.0)", kem, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    // THỐNG KÊ THEO TỪNG NGÀNH
                    ImGui::BeginChild("NganhStatsCol", halfCol, true);
                    {
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "TINH HINH XET TUYEN THEO TUNG NGANH:");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblStatNganh", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Ten Nganh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("D.Chuan", ImGuiTableColumnFlags_WidthFixed, 65);
                            ImGui::TableSetupColumn("Chi Tieu", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableSetupColumn("Dang Ky", ImGuiTableColumnFlags_WidthFixed, 65);
                            ImGui::TableSetupColumn("Trung Tuyen", ImGuiTableColumnFlags_WidthFixed, 85);
                            ImGui::TableHeadersRow();

                            for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) {
                                const NganhXetTuyen& ng = ql.getDanhSachNganh()[i];
                                int dk = ql.demSoLuongDangKyTheoNganh(ng.getMaNganh());
                                int tt = ql.demSoLuongTrungTuyenTheoNganh(ng.getMaNganh());

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%s", ng.getTenNganh().c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%.2f", ng.getDiemChuan());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%d", ng.getChiTieu());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%d", dk);
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "%d (%.0f%%)", tt, (dk > 0 ? (float)tt * 100.0f / dk : 0.0f));
                            }
                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }

                // ---------------------------------------------------------------
                // TAB 4: LƯU TRỮ & XUẤT BÁO CÁO
                // ---------------------------------------------------------------
                if (ImGui::BeginTabItem("  4. Luu tru & Xuat Bao cao  ")) {
                    ImGui::Spacing();

                    ImGui::Text("Chuc nang quan ly File Database:");
                    ImGui::Spacing();

                    if (ImGui::Button("💾 Luu Toan Bo CSDL (4 File Text)", ImVec2(280, 36))) {
                        ql.luuTatCaDuLieu();
                        showToast = true;
                        toastMsg = "Da luu toan bo CSDL vao 4 file text thanh cong!";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("📄 Xuat Bao Cao Trung Tuyen (ketqua_xettuyen.txt)", ImVec2(360, 36))) {
                        ql.xuatBaoCaoXetTuyen("data/ketqua_xettuyen.txt");
                        // Đọc lại để preview
                        ifstream fPreview("data/ketqua_xettuyen.txt");
                        if (fPreview.is_open()) {
                            stringstream ss;
                            ss << fPreview.rdbuf();
                            reportContent = ss.str();
                            fPreview.close();
                        }
                        showToast = true;
                        toastMsg = "Xuat bao cao ket qua xet tuyen thanh cong!";
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "XEM TRUOC NOI DUNG TEP BAO CAO (Preview ketqua_xettuyen.txt):");

                    if (reportContent.empty()) {
                        ifstream fPreview("data/ketqua_xettuyen.txt");
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
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "CAC DIEM NANG CAP DOT PHA THEO GOP Y CUA GIANG VIEN:");
                    ImGui::BulletText("1. Kien truc CSDL Quan he chuan 3NF: Tach thanh 4 file lien ket qua Khoa chinh (PK) va Khoa ngoai (FK).");
                    ImGui::BulletText("2. Giai quyet van de thi sinh trung ten: MaTS la duy nhat, ket hop so CMND/CCCD va SBD de quan ly.");
                    ImGui::BulletText("3. Giao dien Do hoa Dear ImGui (DirectX 9): Truc quan, hien dai, phan hoi 60 FPS cuc ky muot ma.");
                    ImGui::BulletText("4. CSDL linh hoat: Cho phep dieu chinh diem chuan, chi tieu theo tung nganh va tu dong cap nhat trang thai trúng tuyen.");
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

                ImGui::Text("Ma Thi Sinh (PK)  : %s", bufMaTS);
                ImGui::InputText("So Bao Danh (SBD) ", bufSBD, IM_ARRAYSIZE(bufSBD));
                ImGui::InputText("So CMND / CCCD    ", bufCMND, IM_ARRAYSIZE(bufCMND));
                ImGui::InputText("Ho va Ten         ", bufHoTen, IM_ARRAYSIZE(bufHoTen));
                ImGui::InputText("Ngay Sinh (d/m/y) ", bufNgaySinh, IM_ARRAYSIZE(bufNgaySinh));
                ImGui::InputText("Que Quan          ", bufQueQuan, IM_ARRAYSIZE(bufQueQuan));

                ImGui::Text("Gioi Tinh         : ");
                ImGui::SameLine();
                ImGui::RadioButton("Nam", &formGioiTinh, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Nu", &formGioiTinh, 1);

                // Combo Khu Vực
                vector<string> kvNames;
                for (int i = 0; i < ql.getDanhSachKhuVuc().size(); ++i) {
                    const KhuVuc& kv = ql.getDanhSachKhuVuc()[i];
                    kvNames.push_back(kv.getMaKhuVuc() + " (+" + to_string(kv.getDiemUuTien()).substr(0, 4) + "d)");
                }
                vector<const char*> kvCStr;
                for (size_t i = 0; i < kvNames.size(); ++i) kvCStr.push_back(kvNames[i].c_str());
                ImGui::Combo("Khu Vuc Uu Tien   ", &formKVIdx, kvCStr.data(), (int)kvCStr.size());

                // Combo Ngành Đăng Ký
                vector<string> ngNames;
                for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) {
                    const NganhXetTuyen& ng = ql.getDanhSachNganh()[i];
                    ngNames.push_back(ng.getTenNganh() + " [Chuan: " + to_string(ng.getDiemChuan()).substr(0, 5) + "d]");
                }
                vector<const char*> ngCStr;
                for (size_t i = 0; i < ngNames.size(); ++i) ngCStr.push_back(ngNames[i].c_str());
                ImGui::Combo("Nganh Dang Ky     ", &formNganhIdx, ngCStr.data(), (int)ngCStr.size());

                ImGui::Separator();
                ImGui::Text("DIEM SO CAC MON THI (0.00 - 10.00):");
                ImGui::SliderFloat("Diem Toan", &formToan, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Diem Ly  ", &formLy, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Diem Hoa ", &formHoa, 0.0f, 10.0f, "%.2f");

                // Preview tính toán
                double diemUT = ql.getDanhSachKhuVuc()[formKVIdx].getDiemUuTien();
                double tongDiem = formToan + formLy + formHoa + diemUT;
                double dcNganh = ql.getDanhSachNganh()[formNganhIdx].getDiemChuan();
                bool liet = (formToan <= 1.0f || formLy <= 1.0f || formHoa <= 1.0f);
                bool pass = (!liet && tongDiem >= dcNganh);

                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Tong Diem: %.2f (UT: +%.2f)  |  Ket Qua: %s", 
                    tongDiem, diemUT, (pass ? "TRUNG TUYEN" : (liet ? "BI DIEM LIET" : "Khong dat")));

                if (!formErrorMsg.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", formErrorMsg.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Xac Nhan Them", ImVec2(140, 30))) {
                    string sbd = Utils::toUpper(Utils::trim(bufSBD));
                    string cmnd = Utils::trim(bufCMND);
                    string hoten = Utils::chuanHoaTen(bufHoTen);
                    string ns = Utils::trim(bufNgaySinh);
                    string que = Utils::chuanHoaTen(bufQueQuan);

                    if (!Utils::kiemTraSBD(sbd)) {
                        formErrorMsg = "SBD khong hop le (tu 3-15 ky tu)!";
                    } else if (ql.timViTriTheoSBD(sbd) != -1) {
                        formErrorMsg = "So Bao Danh da ton tai tren he thong!";
                    } else if (!Utils::kiemTraCMND(cmnd)) {
                        formErrorMsg = "CMND/CCCD phai gom dung 9 hoac 12 chu so!";
                    } else if (ql.timViTriTheoCMND(cmnd) != -1) {
                        formErrorMsg = "So CMND/CCCD da duoc dang ky boi thi sinh khac!";
                    } else if (hoten.length() < 3) {
                        formErrorMsg = "Ho va ten phai co it nhat 3 ky tu!";
                    } else if (!Utils::kiemTraNgaySinh(ns)) {
                        formErrorMsg = "Ngay sinh khong hop le (dd/mm/yyyy)!";
                    } else {
                        string gt = (formGioiTinh == 1) ? "Nu" : "Nam";
                        string maKV = ql.getDanhSachKhuVuc()[formKVIdx].getMaKhuVuc();
                        string maNg = ql.getDanhSachNganh()[formNganhIdx].getMaNganh();

                        ThiSinh moi(bufMaTS, cmnd, hoten, ns, que, gt, sbd, maKV, maNg,
                                    (double)formToan, (double)formLy, (double)formHoa, diemUT);

                        ql.themThiSinh(moi);
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
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT THONG TIN: %s (Ma TS: %s)", bufHoTen, bufMaTS);
                ImGui::Separator();

                ImGui::Text("So Bao Danh (SBD) : %s", bufSBD);
                ImGui::InputText("So CMND / CCCD    ", bufCMND, IM_ARRAYSIZE(bufCMND));
                ImGui::InputText("Ho va Ten         ", bufHoTen, IM_ARRAYSIZE(bufHoTen));
                ImGui::InputText("Ngay Sinh (d/m/y) ", bufNgaySinh, IM_ARRAYSIZE(bufNgaySinh));
                ImGui::InputText("Que Quan          ", bufQueQuan, IM_ARRAYSIZE(bufQueQuan));

                ImGui::Text("Gioi Tinh         : ");
                ImGui::SameLine();
                ImGui::RadioButton("Nam##edit", &formGioiTinh, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Nu##edit", &formGioiTinh, 1);

                // Combo Khu Vực
                vector<string> kvNames;
                for (int i = 0; i < ql.getDanhSachKhuVuc().size(); ++i) {
                    const KhuVuc& kv = ql.getDanhSachKhuVuc()[i];
                    kvNames.push_back(kv.getMaKhuVuc() + " (+" + to_string(kv.getDiemUuTien()).substr(0, 4) + "d)");
                }
                vector<const char*> kvCStr;
                for (size_t i = 0; i < kvNames.size(); ++i) kvCStr.push_back(kvNames[i].c_str());
                ImGui::Combo("Khu Vuc Uu Tien##ed", &formKVIdx, kvCStr.data(), (int)kvCStr.size());

                // Combo Ngành
                vector<string> ngNames;
                for (int i = 0; i < ql.getDanhSachNganh().size(); ++i) {
                    const NganhXetTuyen& ng = ql.getDanhSachNganh()[i];
                    ngNames.push_back(ng.getTenNganh() + " [Chuan: " + to_string(ng.getDiemChuan()).substr(0, 5) + "d]");
                }
                vector<const char*> ngCStr;
                for (size_t i = 0; i < ngNames.size(); ++i) ngCStr.push_back(ngNames[i].c_str());
                ImGui::Combo("Nganh Dang Ky##ed  ", &formNganhIdx, ngCStr.data(), (int)ngCStr.size());

                ImGui::Separator();
                ImGui::Text("DIEM SO CAC MON THI:");
                ImGui::SliderFloat("Diem Toan##ed", &formToan, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Diem Ly##ed  ", &formLy, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("Diem Hoa##ed ", &formHoa, 0.0f, 10.0f, "%.2f");

                if (!formErrorMsg.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", formErrorMsg.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Luu Thay Doi", ImVec2(140, 30))) {
                    string cmnd = Utils::trim(bufCMND);
                    string hoten = Utils::chuanHoaTen(bufHoTen);
                    string ns = Utils::trim(bufNgaySinh);
                    string que = Utils::chuanHoaTen(bufQueQuan);

                    if (!Utils::kiemTraCMND(cmnd)) {
                        formErrorMsg = "CMND/CCCD phai gom dung 9 hoac 12 chu so!";
                    } else if (hoten.length() < 3) {
                        formErrorMsg = "Ho va ten phai co it nhat 3 ky tu!";
                    } else if (!Utils::kiemTraNgaySinh(ns)) {
                        formErrorMsg = "Ngay sinh khong hop le (dd/mm/yyyy)!";
                    } else {
                        string gt = (formGioiTinh == 1) ? "Nu" : "Nam";
                        string maKV = ql.getDanhSachKhuVuc()[formKVIdx].getMaKhuVuc();
                        string maNg = ql.getDanhSachNganh()[formNganhIdx].getMaNganh();
                        double ut = ql.getDanhSachKhuVuc()[formKVIdx].getDiemUuTien();

                        ThiSinh capNhat(bufMaTS, cmnd, hoten, ns, que, gt, bufSBD, maKV, maNg,
                                        (double)formToan, (double)formLy, (double)formHoa, ut);

                        if (ql.capNhatThiSinh(capNhat)) {
                            showEditModal = false;
                            ImGui::CloseCurrentPopup();
                            showToast = true;
                            toastMsg = "Cap nhat ho so thi sinh thanh cong!";
                        } else {
                            formErrorMsg = "Trung so CMND/CCCD voi thi sinh khac!";
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
                if (selectedRow >= 0 && selectedRow < ql.getDanhSachThiSinh().size()) {
                    const ThiSinh& sel = ql.getDanhSachThiSinh()[selectedRow];
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "BAN CO CHAC CHAN MUON XOA THI SINH NAY?");
                    ImGui::Separator();
                    ImGui::Text("Ma TS   : %s", sel.getMaThiSinh().c_str());
                    ImGui::Text("SBD     : %s", sel.getSoBaoDanh().c_str());
                    ImGui::Text("Ho Ten  : %s", sel.getHoTen().c_str());
                    ImGui::Text("Nganh   : %s", ql.getTenNganh(sel.getMaNganh()).c_str());
                    ImGui::Spacing();

                    if (ImGui::Button("DONG Y XOA", ImVec2(120, 30))) {
                        ql.xoaThiSinh(sel.getMaThiSinh());
                        selectedRow = -1;
                        showDeleteModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Da xoa thi sinh thanh cong!";
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
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowWidth() - 340, 20));
                ImGui::SetNextWindowSize(ImVec2(320, 50));
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
