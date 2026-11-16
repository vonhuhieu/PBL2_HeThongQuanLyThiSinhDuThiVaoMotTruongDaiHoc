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
#include <shellapi.h>
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
            currentLabel = ds[i].ten;
            break;
        }
    }

    ImGui::SetNextItemWidth(-1);
    ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 100.0f), ImVec2(650.0f, 350.0f));
    if (ImGui::BeginCombo(comboId, currentLabel.c_str())) {
        if (ImGui::IsWindowAppearing()) {
            filterBuf[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filterTinh", "Tim kiem tinh / thanh pho...", filterBuf, sizeof(filterBuf));
        ImGui::Separator();

        string query = Utils::toLower(Utils::trim(filterBuf));
        int matchCount = 0;

        for (size_t i = 0; i < ds.size(); ++i) {
            string itemName = Utils::toLower(ds[i].ten);
            string itemMa   = Utils::toLower(ds[i].ma);
            if (query.empty() || itemName.find(query) != string::npos || itemMa.find(query) != string::npos) {
                matchCount++;
                bool isSelected = (selectedMa == ds[i].ma);
                string displayLabel = ds[i].ten;
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 100.0f), ImVec2(650.0f, 350.0f));
    if (ImGui::BeginCombo(comboId, currentLabel.c_str())) {
        for (size_t i = 0; i < ds.size(); ++i) {
            bool isSelected = (Utils::toUpper(selectedMa) == Utils::toUpper(ds[i].ma));
            string displayLabel = ds[i].ten;
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

// Combobox chọn Ngành tuyển sinh DUT từ Danh mục ngành (hỗ trợ loại trừ/vô hiệu hóa các ngành đã chọn ở NV khác)
static bool RenderComboNganh(const char* comboId, const Vector<DanhMucNganh>& dsNganh, string& selectedMaNganh,
                             const string& exclude1 = "", const string& exclude2 = "") {
    bool changed = false;
    string currentLabel = "(Chua chon / Bo trong)";
    for (size_t i = 0; i < dsNganh.size(); ++i) {
        if (dsNganh[i].maNganh == selectedMaNganh) {
            currentLabel = dsNganh[i].tenNganh + " [" + dsNganh[i].maNganh + "]";
            break;
        }
    }

    ImGui::SetNextItemWidth(-1);
    ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 100.0f), ImVec2(650.0f, 350.0f));
    if (ImGui::BeginCombo(comboId, currentLabel.c_str())) {
        if (ImGui::Selectable("(Bo trong / Khong dang ky)", selectedMaNganh.empty())) {
            selectedMaNganh = "";
            changed = true;
        }
        for (size_t i = 0; i < dsNganh.size(); ++i) {
            const string& ma = dsNganh[i].maNganh;
            bool isSelected = (!selectedMaNganh.empty() && selectedMaNganh == ma);
            bool isExcluded = (!exclude1.empty() && ma == exclude1) || (!exclude2.empty() && ma == exclude2);

            if (isExcluded) {
                string lblDisabled = dsNganh[i].tenNganh + " [" + ma + "] (Da chon o NV khac)";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
                ImGui::Selectable(lblDisabled.c_str(), false, ImGuiSelectableFlags_Disabled);
                ImGui::PopStyleColor();
            } else {
                string lbl = dsNganh[i].tenNganh + " [" + ma + "]";
                if (ImGui::Selectable(lbl.c_str(), isSelected)) {
                    selectedMaNganh = ma;
                    changed = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

// Bổ sung hiệu ứng Hover nổi bật vào từng hàng cho toàn bộ các bảng trong ứng dụng
static void RenderRowHover(const char* rowId, bool isSelected = false) {
    ImGui::Selectable(rowId, isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 0));
    ImGui::SameLine();
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
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.35f, 0.55f, 0.50f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.26f, 0.44f, 0.70f, 0.65f); // Hieu ung hover hang sang ro, de theo doi du lieu
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.30f, 0.52f, 0.80f, 0.80f);
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
    static int filterNam = 2026; // Mac dinh lay nam hien hanh (2026)
    static string filterKhoa = "";  // "" (Tat ca), "CNTT", "CKHI"...
    static string filterNganh = ""; // "" (Tat ca), "7480201B"...
    static string filterLop = "";   // "" (Tat ca), "26TCLC_KHDL1"...

    static int selectedRow = -1;
    static string selectedSBD = "";

    // State phân trang Server-side & Sorting Thead
    static int currentPage = 1;
    static int pageSizeIndex = 0; // 0: 10, 1: 25, 2: 50, 3: 100
    static int sortCol = 0;       // 0: ngaySua DESC (moi tao/sua len dau), 1=SBD, 2=CCCD, 3=Ten...
    static bool sortAsc = false;  // false: DESC (moi nhat truoc), true: ASC
    static int targetJumpPage = 1;

    // Track filter changes to reset to page 1
    static char lastFilterBuf[128] = "";
    static int lastFilterStatus = 0;
    static int lastFilterNam = 2026;
    static string lastFilterKhoa = "";
    static string lastFilterNganh = "";
    static string lastFilterLop = "";

    // State modals
    static bool showDetailModal = false;
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
    static string formNV1 = "7480201B";
    static string formNV2 = "7480201";
    static string formNV3 = "7520216";

    // State quản lý danh mục (Tab 4)
    static char bufNewMaTinh[16] = "";
    static char bufNewTenTinh[64] = "";
    static char bufNewLoaiTinh[64] = "Tinh truc thuoc";
    static string msgTinhThanh = "";

    static char bufNewMaGT[16] = "";
    static char bufNewTenGT[64] = "";
    static char bufNewDescGT[64] = "";
    static string msgGioiTinh = "";

    // State Modal Sửa Danh mục Tỉnh thành & Giới tính
    static bool showEditTinhThanhModal = false;
    static char editTinhThanhMa[16] = "";
    static char editTinhThanhTen[128] = "";
    static char editTinhThanhLoai[64] = "";
    static string editTinhThanhError = "";

    static bool showEditGioiTinhModal = false;
    static char editGioiTinhMa[16] = "";
    static char editGioiTinhTen[64] = "";
    static char editGioiTinhDesc[128] = "";
    static string editGioiTinhError = "";

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

                // HÀNG BỘ LỌC 1: Tìm kiếm từ khóa, Kết quả, Năm tuyển sinh, Khoa đào tạo & Nút chức năng
                ImGui::Text("Tim kiem:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(210);
                ImGui::InputTextWithHint("##filter", "SBD, CCCD, Ho ten, Que...", filterBuf, IM_ARRAYSIZE(filterBuf));

                // Năm tuyển sinh (được đưa lên trước Kết quả theo chuẩn phân cấp)
                ImGui::SameLine();
                ImGui::Text("Nam:");
                ImGui::SameLine();
                char currentNamLabel[32];
                if (filterNam == 0) strcpy(currentNamLabel, "Tat ca nam");
                else sprintf(currentNamLabel, "%d", filterNam);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.20f, 0.28f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.32f, 0.45f, 1.0f));
                if (ImGui::Button(currentNamLabel, ImVec2(95, 26))) {
                    ImGui::OpenPopup("PopupYearPicker");
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click de mo hop thoai chon nam tuyen sinh");
                }

                if (ImGui::BeginPopup("PopupYearPicker")) {
                    static int yearBase = 2024;
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "CHON NAM TUYEN SINH");
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::ArrowButton("##yPrev", ImGuiDir_Left)) {
                        yearBase -= 6;
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "  %d - %d  ", yearBase, yearBase + 5);
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##yNext", ImGuiDir_Right)) {
                        yearBase += 6;
                    }

                    ImGui::Spacing();
                    bool isAllYears = (filterNam == 0);
                    if (isAllYears) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.35f, 1.0f));
                    if (ImGui::Button("Tat Ca Cac Nam", ImVec2(170, 26))) {
                        filterNam = 0;
                        currentPage = 1;
                        ImGui::CloseCurrentPopup();
                    }
                    if (isAllYears) ImGui::PopStyleColor();

                    ImGui::Spacing();

                    for (int y = yearBase; y < yearBase + 6; ++y) {
                        if ((y - yearBase) % 2 != 0) ImGui::SameLine();

                        bool isSelected = (filterNam == y);
                        bool isCurrentSystemYear = (y == ql.getNamTuyenSinhHienHanh());

                        if (isSelected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.80f, 1.0f));
                        } else if (isCurrentSystemYear) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.36f, 0.48f, 1.0f));
                        }

                        char yBtnId[32];
                        sprintf(yBtnId, "%d##yp_%d", y, y);
                        if (ImGui::Button(yBtnId, ImVec2(80, 28))) {
                            filterNam = y;
                            currentPage = 1;
                            ImGui::CloseCurrentPopup();
                        }

                        if (isSelected || isCurrentSystemYear) {
                            ImGui::PopStyleColor();
                        }
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    if (ImGui::Button("Dong", ImVec2(170, 24))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // Kết quả tuyển sinh (0: Tất cả, 1: Trúng tuyển, 2: Không đạt)
                ImGui::SameLine();
                ImGui::Text("Ket qua:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                const char* statusNames[] = { "Tat ca", "Trung tuyen", "Khong dat" };
                if (ImGui::Combo("##filterStatus", &filterStatus, statusNames, 3)) {
                    if (filterStatus == 2) {
                        filterKhoa = "";
                        filterNganh = "";
                        filterLop = "";
                    }
                    currentPage = 1;
                }

                ImGui::SameLine();
                ImGui::Text("Khoa:");
                ImGui::SameLine();

                bool disableKhoa = (filterStatus == 2);
                if (disableKhoa) {
                    filterKhoa = "";
                    ImGui::BeginDisabled();
                }

                string currentKhoaLabel = disableKhoa ? "(Khong ap dung)" : "Tat ca khoa";
                if (!disableKhoa) {
                    for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                        if (ql.getDanhSachKhoa()[k].maKhoa == filterKhoa) {
                            currentKhoaLabel = ql.getDanhSachKhoa()[k].tenKhoa;
                            break;
                        }
                    }
                }
                ImGui::SetNextItemWidth(140);
                if (ImGui::BeginCombo("##filterKhoa", currentKhoaLabel.c_str())) {
                    if (ImGui::Selectable("Tat ca khoa", filterKhoa.empty())) {
                        filterKhoa = "";
                        filterNganh = "";
                        filterLop = "";
                        currentPage = 1;
                    }
                    for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                        const Khoa& kh = ql.getDanhSachKhoa()[k];
                        string label = kh.maKhoa + " - " + kh.tenKhoa;
                        if (ImGui::Selectable(label.c_str(), filterKhoa == kh.maKhoa)) {
                            filterKhoa = kh.maKhoa;
                            filterNganh = "";
                            filterLop = "";
                            currentPage = 1;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (disableKhoa) {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine(ImGui::GetWindowWidth() - 340);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                if (ImGui::Button("[+] Them Thi Sinh", ImVec2(145, 28))) {
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
                    formNV1 = "7480201B";
                    formNV2 = "7480201";
                    formNV3 = "7520216";
                    formErrorMsg = "";
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 1.0f));
                if (ImGui::Button("[ Xuat Bao Cao Excel ]", ImVec2(165, 28))) {
                    // 1. Tạo timestamp thời gian thực YYYYMMDD_HHMMSS
                    time_t now = time(nullptr);
                    tm* ltm = localtime(&now);
                    char tsBuf[32];
                    strftime(tsBuf, sizeof(tsBuf), "%Y%m%d_%H%M%S", ltm);
                    string ts = tsBuf;

                    // 2. Sinh tên file Excel động theo Phương án 1 (ngữ cảnh bộ lọc)
                    string fileName;
                    if (!filterLop.empty()) {
                        fileName = "BaoCao_ThiSinh_DUT_" + filterLop + "_" + ts + ".xls";
                    } else if (!filterNganh.empty()) {
                        fileName = "BaoCao_ThiSinh_DUT_" + filterNganh + "_" + ts + ".xls";
                    } else if (!filterKhoa.empty()) {
                        fileName = "BaoCao_ThiSinh_DUT_Khoa" + filterKhoa + "_" + ts + ".xls";
                    } else {
                        int namKhoa = (filterNam > 0) ? (filterNam % 100) : (ql.getNamTuyenSinhHienHanh() % 100);
                        fileName = "BaoCao_DanhSach_ThiSinh_DUT_K" + to_string(namKhoa) + "_" + ts + ".xls";
                    }
                    string fullPath = "data/" + fileName;

                    // 3. Chuẩn bị tên Khoa và Ngành đầy đủ
                    string tenKhoaFilter = "";
                    if (!filterKhoa.empty()) tenKhoaFilter = ql.layTenKhoa(filterKhoa);

                    string tenNganhFilter = "";
                    if (!filterNganh.empty()) tenNganhFilter = ql.layTenNganh(filterNganh);

                    // 4. Lấy đầy đủ tham số bộ lọc hiện hành
                    FilterThiSinhParams curFilterParams;
                    curFilterParams.keyword = filterBuf;
                    curFilterParams.filterStatus = filterStatus;
                    curFilterParams.filterNam = filterNam;
                    curFilterParams.filterKhoa = filterKhoa;
                    curFilterParams.filterNganh = filterNganh;
                    curFilterParams.filterLop = filterLop;
                    curFilterParams.sortCol = sortCol;
                    curFilterParams.sortAsc = sortAsc;

                    // 5. Xuất báo cáo Excel
                    bool ok = ql.xuatBaoCaoExcel(fullPath, curFilterParams, tenKhoaFilter, tenNganhFilter);
                    showToast = true;
                    if (ok) {
                        toastMsg = "Da xuat Excel: " + fileName;
                        // Mở file bằng Excel hoặc phần mềm mặc định của Windows
                        ShellExecuteA(NULL, "open", fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    } else {
                        toastMsg = "Loi: Khong the xuat bao cao Excel ra " + fullPath;
                    }
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Xuat toan bo danh sach thoa bo loc hien tai ra file Excel (.xls) voi day du tieu de, thong tin loc va thead");
                }

                // HÀNG BỘ LỌC 2: Ngành trúng tuyển, Lớp sinh hoạt, Nút xóa lọc & Thông tin kết quả
                ImGui::Spacing();
                ImGui::Text("Nganh:");
                ImGui::SameLine();

                bool disableNganh = (filterStatus == 2) || filterKhoa.empty();
                if (disableNganh) {
                    filterNganh = "";
                    ImGui::BeginDisabled();
                }

                // Tự động kiểm tra tính hợp lệ của filterNganh khi filterKhoa thay đổi
                if (!filterKhoa.empty() && !filterNganh.empty()) {
                    string mk = ql.layMaKhoaCuaNganh(filterNganh);
                    if (mk != filterKhoa) {
                        filterNganh = "";
                        filterLop = "";
                    }
                }

                string currentNganhLabel;
                if (filterStatus == 2) {
                    currentNganhLabel = "(Khong ap dung)";
                } else if (filterKhoa.empty()) {
                    currentNganhLabel = "(Chon khoa truoc)";
                } else if (filterNganh.empty()) {
                    currentNganhLabel = "Tat ca nganh";
                } else {
                    currentNganhLabel = "";
                    for (size_t n = 0; n < ql.getDanhSachDanhMucNganh().size(); ++n) {
                        if (ql.getDanhSachDanhMucNganh()[n].maNganh == filterNganh) {
                            currentNganhLabel = ql.getDanhSachDanhMucNganh()[n].tenNganh;
                            break;
                        }
                    }
                }
                ImGui::SetNextItemWidth(250);
                if (ImGui::BeginCombo("##filterNganh", currentNganhLabel.c_str())) {
                    if (ImGui::Selectable("Tat ca nganh", filterNganh.empty())) {
                        filterNganh = "";
                        filterLop = "";
                        currentPage = 1;
                    }
                    for (size_t n = 0; n < ql.getDanhSachDanhMucNganh().size(); ++n) {
                        const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[n];
                        if (!filterKhoa.empty() && dmn.maKhoa != filterKhoa) continue;
                        string label = dmn.maNganh + " - " + dmn.tenNganh;
                        if (ImGui::Selectable(label.c_str(), filterNganh == dmn.maNganh)) {
                            filterNganh = dmn.maNganh;
                            filterKhoa = dmn.maKhoa; // Đồng bộ khoa trực thuộc
                            filterLop = "";          // Reset lớp để nạp danh sách lớp của ngành mới
                            currentPage = 1;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (disableNganh) {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();
                ImGui::Text("Lop:");
                ImGui::SameLine();

                bool disableLop = (filterStatus == 2) || filterKhoa.empty() || filterNganh.empty();
                if (disableLop) {
                    filterLop = "";
                    ImGui::BeginDisabled();
                }

                // Tải danh sách lớp sinh hoạt phân cấp theo Khoa và Ngành
                Vector<string> dsLop;
                if (!disableLop) {
                    dsLop = ql.layDanhSachLopSinhHoat(filterKhoa, filterNganh);
                    // Nếu lớp đang chọn không còn nằm trong dsLop của ngành/khoa mới thì tự động reset
                    if (!filterLop.empty()) {
                        bool lopHopLe = false;
                        for (size_t l = 0; l < dsLop.size(); ++l) {
                            if (dsLop[l] == filterLop) { lopHopLe = true; break; }
                        }
                        if (!lopHopLe) filterLop = "";
                    }
                }

                string currentLopLabel;
                if (filterStatus == 2) {
                    currentLopLabel = "(Khong ap dung)";
                } else if (filterKhoa.empty() || filterNganh.empty()) {
                    currentLopLabel = "(Chon nganh truoc)";
                } else if (filterLop.empty()) {
                    currentLopLabel = "Tat ca lop";
                } else {
                    currentLopLabel = filterLop;
                }
                ImGui::SetNextItemWidth(140);
                if (ImGui::BeginCombo("##filterLop", currentLopLabel.c_str())) {
                    if (ImGui::Selectable("Tat ca lop", filterLop.empty())) {
                        filterLop = "";
                        currentPage = 1;
                    }
                    for (size_t l = 0; l < dsLop.size(); ++l) {
                        if (ImGui::Selectable(dsLop[l].c_str(), filterLop == dsLop[l])) {
                            filterLop = dsLop[l];
                            currentPage = 1;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (disableLop) {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.38f, 0.44f, 0.85f));
                if (ImGui::Button("Xoa Bo Loc", ImVec2(90, 24))) {
                    filterBuf[0] = '\0';
                    filterStatus = 0;
                    filterNam = ql.getNamTuyenSinhHienHanh();
                    filterKhoa = "";
                    filterNganh = "";
                    filterLop = "";
                    currentPage = 1;
                    selectedRow = -1;
                }
                ImGui::PopStyleColor();

                // Kiểm tra thay đổi bất kỳ bộ lọc nào để tự động reset về trang 1
                if (filterStatus == 2) {
                    filterKhoa = "";
                    filterNganh = "";
                    filterLop = "";
                }
                if (strcmp(filterBuf, lastFilterBuf) != 0 || filterStatus != lastFilterStatus ||
                    filterNam != lastFilterNam || filterKhoa != lastFilterKhoa ||
                    filterNganh != lastFilterNganh || filterLop != lastFilterLop) {
                    strcpy(lastFilterBuf, filterBuf);
                    lastFilterStatus = filterStatus;
                    lastFilterNam = filterNam;
                    lastFilterKhoa = filterKhoa;
                    lastFilterNganh = filterNganh;
                    lastFilterLop = filterLop;
                    currentPage = 1;
                }

                int pageSize = AdmissionConst::PAGE_SIZE_OPTIONS[pageSizeIndex];
                int totalRecords = 0;

                FilterThiSinhParams filterParams;
                filterParams.pageIndex = currentPage;
                filterParams.pageSize = pageSize;
                filterParams.keyword = filterBuf;
                filterParams.filterStatus = filterStatus;
                filterParams.filterNam = filterNam;
                filterParams.filterKhoa = filterKhoa;
                filterParams.filterNganh = filterNganh;
                filterParams.filterLop = filterLop;
                filterParams.sortCol = sortCol;
                filterParams.sortAsc = sortAsc;

                Vector<ThiSinh> pageData = ql.layDanhSachThiSinhPhanTrang(filterParams, totalRecords);

                int totalPages = (totalRecords + pageSize - 1) / pageSize;
                if (totalPages < 1) totalPages = 1;
                if (currentPage > totalPages) currentPage = totalPages;

                ImGui::SameLine(ImGui::GetWindowWidth() - 410);
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Ket qua loc: %d / %d thi sinh | Diem san: %.2f", 
                    totalRecords, (int)ql.getDanhSach().size(), ql.getDiemSan());

                ImGui::Separator();

                float maxTableHeight = ImGui::GetWindowHeight() - 305.0f;
                if (maxTableHeight < 200.0f) maxTableHeight = 200.0f;

                // Tự động tính toán và đo lường chiều cao bảng để ôm sát số lượng dòng thực tế
                // Tránh tình trạng thanh phân trang bị đẩy dính tịt xuống đáy tạo khoảng trống mênh mông
                float estimatedHeight = 27.0f + (float)(pageData.empty() ? 1 : pageData.size()) * 27.0f;
                if (ImGui::GetContentRegionAvail().x < 1550.0f) {
                    estimatedHeight += ImGui::GetStyle().ScrollbarSize;
                }

                static float measuredTableHeight = 0.0f;
                static int lastPageDataCount = -1;
                static int lastCurrentPage = -1;
                static int lastPageSize = -1;

                if (lastPageDataCount != (int)pageData.size() || lastCurrentPage != currentPage || lastPageSize != pageSize) {
                    measuredTableHeight = estimatedHeight;
                    lastPageDataCount = (int)pageData.size();
                    lastCurrentPage = currentPage;
                    lastPageSize = pageSize;
                }

                bool needScrollY = (measuredTableHeight >= maxTableHeight);
                float appliedTableHeight = needScrollY ? maxTableHeight : measuredTableHeight;

                ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                                             ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | 
                                             ImGuiTableFlags_Sortable;
                if (needScrollY) tableFlags |= ImGuiTableFlags_ScrollY;

                if (ImGui::BeginTable("CandidatesTable", 17, tableFlags, ImVec2(0, appliedTableHeight))) {
                    ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 38);
                    ImGui::TableSetupColumn("SBD (PK)", ImGuiTableColumnFlags_WidthFixed, 85);
                    ImGui::TableSetupColumn("So CCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                    ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 95);
                    ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 115);
                    ImGui::TableSetupColumn("Gioi Tinh", ImGuiTableColumnFlags_WidthFixed, 75);
                    ImGui::TableSetupColumn("Toan", ImGuiTableColumnFlags_WidthFixed, 60);
                    ImGui::TableSetupColumn("Ly", ImGuiTableColumnFlags_WidthFixed, 60);
                    ImGui::TableSetupColumn("Hoa", ImGuiTableColumnFlags_WidthFixed, 60);
                    ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 90);
                    ImGui::TableSetupColumn("Ket Qua", ImGuiTableColumnFlags_WidthFixed, 135);
                    ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 140);
                    ImGui::TableSetupColumn("Nganh Trung Tuyen", ImGuiTableColumnFlags_WidthFixed, 180);
                    ImGui::TableSetupColumn("Lop Sinh Hoat", ImGuiTableColumnFlags_WidthFixed, 120);
                    ImGui::TableSetupColumn("Ma SV DUT", ImGuiTableColumnFlags_WidthFixed, 100);
                    ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 155);

                    // Render Custom Thead: Luôn hiển thị biểu tượng vector mũi tên sắc nét ở mọi cột sortable
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    for (int col = 0; col < 17; col++) {
                        if (!ImGui::TableSetColumnIndex(col)) continue;
                        const char* colName = ImGui::TableGetColumnName(col);
                        if (!colName || colName[0] == '\0') continue;
                        ImGui::TableHeader(colName);

                        if (col >= 1 && col <= 15) {
                            ImVec2 rMin = ImGui::GetItemRectMin();
                            ImVec2 rMax = ImGui::GetItemRectMax();
                            ImDrawList* drawList = ImGui::GetWindowDrawList();
                            float arrowX = rMax.x - 14.0f;
                            float centerY = (rMin.y + rMax.y) * 0.5f;

                            if (sortCol == col) {
                                // Cột đang active sort: vẽ mũi tên màu vàng sáng rõ nét
                                ImU32 activeColor = ImGui::GetColorU32(ImVec4(1.0f, 0.85f, 0.35f, 1.0f));
                                if (sortAsc) {
                                    // Mũi tên lên ▲ (ASC)
                                    drawList->AddTriangleFilled(ImVec2(arrowX, centerY - 4.0f),
                                                                ImVec2(arrowX - 4.0f, centerY + 2.0f),
                                                                ImVec2(arrowX + 4.0f, centerY + 2.0f), activeColor);
                                } else {
                                    // Mũi tên xuống ▼ (DESC)
                                    drawList->AddTriangleFilled(ImVec2(arrowX, centerY + 4.0f),
                                                                ImVec2(arrowX - 4.0f, centerY - 2.0f),
                                                                ImVec2(arrowX + 4.0f, centerY - 2.0f), activeColor);
                                }
                            } else {
                                // Cột chưa sort: LUÔN HIỂN THỊ mũi tên kép 2 chiều vector mờ tinh tế ▲/▼
                                ImU32 neutralColor = ImGui::GetColorU32(ImVec4(0.50f, 0.60f, 0.72f, 0.60f));
                                drawList->AddTriangleFilled(ImVec2(arrowX, centerY - 4.0f),
                                                            ImVec2(arrowX - 3.5f, centerY - 0.5f),
                                                            ImVec2(arrowX + 3.5f, centerY - 0.5f), neutralColor);
                                drawList->AddTriangleFilled(ImVec2(arrowX, centerY + 4.0f),
                                                            ImVec2(arrowX - 3.5f, centerY + 0.5f),
                                                            ImVec2(arrowX + 3.5f, centerY + 0.5f), neutralColor);
                            }
                        }
                    }

                    // Xử lý sự kiện click Thead để sắp xếp
                    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                    if (sortSpecs && sortSpecs->SpecsDirty) {
                        if (sortSpecs->SpecsCount > 0) {
                            int newCol = sortSpecs->Specs[0].ColumnIndex;
                            bool newAsc = (sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
                            if (newCol != sortCol || newAsc != sortAsc) {
                                sortCol = newCol;
                                sortAsc = newAsc;
                                currentPage = 1;
                            }
                        }
                        sortSpecs->SpecsDirty = false;
                    }

                    for (size_t i = 0; i < pageData.size(); ++i) {
                        const ThiSinh& ts = pageData[i];
                        string tenQue = ql.layTenTinhThanh(ts.getQueQuan());
                        string tenGT  = ql.layTenGioiTinh(ts.getGioiTinh());

                        ImGui::TableNextRow();

                        int stt = (currentPage - 1) * pageSize + (int)i + 1;

                        ImGui::TableSetColumnIndex(0);
                        char rId[32];
                        sprintf(rId, "##row_ts_%d", (int)i);
                        bool isRowSelected = (!selectedSBD.empty() && selectedSBD == ts.getSoBaoDanh()) || (selectedRow == (int)i);
                        RenderRowHover(rId, isRowSelected);
                        ImGui::Text("%d", stt);

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
                        if (ts.daTrungTuyen()) {
                            ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "TRUNG TUYEN (NV%d)", ts.getNguyenVongTrungTuyen());
                        } else if (ts.kiemTraDiemLiet(ql.getDiemLiet())) {
                            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "%s", AdmissionConst::KET_QUA_DIEM_LIET.c_str());
                        } else if (ts.tinhTongDiem() < ql.getDiemSan()) {
                            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "%s", AdmissionConst::KET_QUA_KHONG_DAT_SAN.c_str());
                        } else {
                            bool coNVDangXet = false;
                            const Vector<NguyenVong>& nvList = ts.getDsNguyenVong();
                            for (size_t k = 0; k < nvList.size(); ++k) {
                                if (nvList[k].trangThai == AdmissionConst::TT_NV_DANG_XET) {
                                    coNVDangXet = true;
                                    break;
                                }
                            }
                            if (coNVDangXet || nvList.empty()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", AdmissionConst::KET_QUA_DANG_XET.c_str());
                            } else {
                                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", AdmissionConst::KET_QUA_KHONG_DAT.c_str());
                            }
                        }

                        // CỘT 12: KHOA TRÚNG TUYỂN (BỔ SUNG MỚI)
                        ImGui::TableSetColumnIndex(12);
                        if (ts.daTrungTuyen() && !ts.getNganhTrungTuyen().empty()) {
                            string mk = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
                            string tenKhoa = ql.layTenKhoa(mk);
                            ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "%s", tenKhoa.c_str());
                        } else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                        }

                        // CỘT 13: NGÀNH TRÚNG TUYỂN
                        ImGui::TableSetColumnIndex(13);
                        if (!ts.getNganhTrungTuyen().empty()) {
                            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", ql.layTenNganh(ts.getNganhTrungTuyen()).c_str());
                        } else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                        }

                        // CỘT 14: LỚP SINH HOẠT
                        ImGui::TableSetColumnIndex(14);
                        if (!ts.getLopSinhHoat().empty()) {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ts.getLopSinhHoat().c_str());
                        } else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                        }

                        // CỘT 15: MÃ SINH VIÊN DUT
                        ImGui::TableSetColumnIndex(15);
                        if (!ts.getMaSinhVien().empty()) {
                            ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.75f, 1.0f), "%s", ts.getMaSinhVien().c_str());
                        } else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                        }

                        // CỘT 16: THAO TÁC (XEM, SỬA & XÓA)
                        ImGui::TableSetColumnIndex(16);
                        ImGui::PushID((int)i);

                        // 1. Nút Xem chi tiết hồ sơ thí sinh
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.50f, 0.44f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.65f, 0.55f, 1.0f));
                        if (ImGui::Button("Xem", ImVec2(42, 22))) {
                            selectedSBD = ts.getSoBaoDanh();
                            showDetailModal = true;
                        }
                        ImGui::PopStyleColor(2);

                        ImGui::SameLine();

                        // 2. Nút Sửa hồ sơ
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.80f, 1.0f));
                        if (ImGui::Button("Sua", ImVec2(42, 22))) {
                            selectedRow = (int)i;
                            selectedSBD = ts.getSoBaoDanh();
                            strcpy(bufSBD, ts.getSoBaoDanh().c_str());
                            strcpy(bufCCCD, ts.getSoCCCD().c_str());
                            strcpy(bufHoTen, ts.getHoTen().c_str());

                            string ns = ts.getNgaySinh();
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

                            formQueQuanMa = ts.getQueQuan();
                            formGioiTinhMa = ts.getGioiTinh();
                            formToan = (float)ts.getDiemToan();
                            formLy = (float)ts.getDiemLy();
                            formHoa = (float)ts.getDiemHoa();
                            formErrorMsg = "";

                            formNV1 = ""; formNV2 = ""; formNV3 = "";
                            const Vector<NguyenVong>& dsNV = ts.getDsNguyenVong();
                            for (size_t nvIdx = 0; nvIdx < dsNV.size(); ++nvIdx) {
                                if (dsNV[nvIdx].thuTuNV == 1) formNV1 = dsNV[nvIdx].maNganh;
                                else if (dsNV[nvIdx].thuTuNV == 2) formNV2 = dsNV[nvIdx].maNganh;
                                else if (dsNV[nvIdx].thuTuNV == 3) formNV3 = dsNV[nvIdx].maNganh;
                            }

                            showEditModal = true;
                        }
                        ImGui::PopStyleColor();

                        ImGui::SameLine();

                        // 3. Nút Xóa hồ sơ
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
                        if (ImGui::Button("Xoa", ImVec2(42, 22))) {
                            selectedRow = (int)i;
                            selectedSBD = ts.getSoBaoDanh();
                            showDeleteModal = true;
                        }
                        ImGui::PopStyleColor();

                        ImGui::PopID();
                    }

                    if (pageData.empty()) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "Khong tim thay thi sinh nao phu hop!");
                    }

                    // Đo lường chính xác chiều cao thực tế của các hàng bên trong bảng
                    float innerY = ImGui::GetCursorPosY();
                    bool hasScrollX = (ImGui::GetScrollMaxX() > 0.0f);
                    float scrollXHeight = hasScrollX ? ImGui::GetStyle().ScrollbarSize : 0.0f;
                    float realTableHeight = innerY + scrollXHeight + 1.0f;

                    if (!needScrollY && realTableHeight > 40.0f) {
                        measuredTableHeight = realTableHeight;
                    }

                    ImGui::EndTable();
                }

                // ==================== THANH ĐIỀU HƯỚNG PHÂN TRANG ====================
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 1. Phân trang - Số bản ghi / trang
                ImGui::Text("So ban ghi / trang:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(110);
                const char* pageSizeLabels[] = { "10 / trang", "25 / trang", "50 / trang", "100 / trang" };
                int oldPageSizeIdx = pageSizeIndex;
                if (ImGui::Combo("##comboPageSize", &pageSizeIndex, pageSizeLabels, AdmissionConst::SO_LUONG_PAGE_SIZE_OPTIONS)) {
                    if (pageSizeIndex != oldPageSizeIdx) {
                        currentPage = 1;
                    }
                }

                ImGui::SameLine();
                int recordStart = (totalRecords == 0) ? 0 : ((currentPage - 1) * pageSize + 1);
                int recordEnd = (currentPage * pageSize < totalRecords) ? (currentPage * pageSize) : totalRecords;
                ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), " | Hien thi tu %d den %d tren tong so %d thi sinh",
                    recordStart, recordEnd, totalRecords);

                // 2. Nút điều hướng phân trang
                ImGui::SameLine(ImGui::GetWindowWidth() - 440);

                bool atFirstPage = (currentPage <= 1);
                if (atFirstPage) ImGui::BeginDisabled();
                if (ImGui::Button(" |<< Dau ")) {
                    currentPage = 1;
                    selectedRow = -1;
                }
                if (atFirstPage) ImGui::EndDisabled();

                ImGui::SameLine();

                if (atFirstPage) ImGui::BeginDisabled();
                if (ImGui::Button(" < Truoc ")) {
                    if (currentPage > 1) currentPage--;
                    selectedRow = -1;
                }
                if (atFirstPage) ImGui::EndDisabled();

                ImGui::SameLine();

                // NÚT TRANG HIỆN TẠI (CLICK VÀO BẬT POPUP CHỌN TRANG ĐẾN)
                char pageLabel[64];
                sprintf(pageLabel, " Trang %d / %d ", currentPage, totalPages);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.28f, 0.42f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.48f, 0.72f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.35f, 1.0f));
                if (ImGui::Button(pageLabel)) {
                    targetJumpPage = currentPage;
                    ImGui::OpenPopup("PopupChuyenTrang");
                }
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click de chuyen nhanh den trang bat ky");
                }

                // POPUP NHẬP SỐ TRANG ĐỂ CHUYỂN
                if (ImGui::BeginPopup("PopupChuyenTrang")) {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Chuyen Den Trang");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Nhap so trang (1 - %d):", totalPages);
                    ImGui::SetNextItemWidth(130);
                    if (ImGui::InputInt("##jumpPageInput", &targetJumpPage, 1, 5)) {
                        if (targetJumpPage < 1) targetJumpPage = 1;
                        if (targetJumpPage > totalPages) targetJumpPage = totalPages;
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Chuyen ngay", ImVec2(90, 26)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        if (targetJumpPage >= 1 && targetJumpPage <= totalPages) {
                            currentPage = targetJumpPage;
                            selectedRow = -1;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Dong", ImVec2(60, 26))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::SameLine();

                bool atLastPage = (currentPage >= totalPages);
                if (atLastPage) ImGui::BeginDisabled();
                if (ImGui::Button(" Sau > ")) {
                    if (currentPage < totalPages) currentPage++;
                    selectedRow = -1;
                }
                if (atLastPage) ImGui::EndDisabled();

                ImGui::SameLine();

                if (atLastPage) ImGui::BeginDisabled();
                if (ImGui::Button(" Cuoi >>| ")) {
                    currentPage = totalPages;
                    selectedRow = -1;
                }
                if (atLastPage) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }

            // ==================== TAB 2: CƠ SỞ DỮ LIỆU QUAN HỆ (CLASS TABLE 3NF) ====================
            if (ImGui::BeginTabItem("  2. Co so Du lieu Quan he (Class Table)  ")) {
                ImGui::Spacing();

                if (ImGui::BeginTabBar("SubTabsCSDL", ImGuiTabBarFlags_None)) {

                    // SUBTAB 2.1: BẢNG THỰC THỂ CHA NGƯỜI
                    if (ImGui::BeginTabItem("  Bang 1: NGUOI (MySQL: nguoi)  ")) {
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
                                char rId[32]; sprintf(rId, "##row_ng_%d", (int)i); RenderRowHover(rId);
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
                    if (ImGui::BeginTabItem("  Bang 2: THI SINH (MySQL: thisinh)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THUC THE CON: THI SINH (Khoa chinh: soBaoDanh | Khoa ngoai: soCCCD, namTuyenSinh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblThiSinhRaw", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("soBaoDanh (PK)", ImGuiTableColumnFlags_WidthFixed, 105);
                            ImGui::TableSetupColumn("soCCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("namTuyenSinh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("diemToan", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("diemLy", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("diemHoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("maSinhVien", ImGuiTableColumnFlags_WidthFixed, 115);
                            ImGui::TableSetupColumn("lopSinhHoat", ImGuiTableColumnFlags_WidthFixed, 125);
                            ImGui::TableSetupColumn("nganhTrungTuyen", ImGuiTableColumnFlags_WidthFixed, 115);
                            ImGui::TableSetupColumn("nguyenVongTrungTuyen", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_tsr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoBaoDanh().c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ts.getSoCCCD().c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%d", ts.getNamTuyenSinh());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::Text("%.2f", ts.getDiemToan());
                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%.2f", ts.getDiemLy());
                                ImGui::TableSetColumnIndex(6);
                                ImGui::Text("%.2f", ts.getDiemHoa());
                                ImGui::TableSetColumnIndex(7);
                                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%s", ts.getMaSinhVien().empty() ? "NULL" : ts.getMaSinhVien().c_str());
                                ImGui::TableSetColumnIndex(8);
                                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "%s", ts.getLopSinhHoat().empty() ? "NULL" : ts.getLopSinhHoat().c_str());
                                ImGui::TableSetColumnIndex(9);
                                ImGui::Text("%s", ts.getNganhTrungTuyen().empty() ? "NULL" : ts.getNganhTrungTuyen().c_str());
                                ImGui::TableSetColumnIndex(10);
                                ImGui::Text("%d", ts.getNguyenVongTrungTuyen());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.3: BẢNG NGUYỆN VỌNG ĐĂNG KÝ
                    if (ImGui::BeginTabItem("  Bang 3: NGUYEN VONG (MySQL: nguyen_vong)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG NGUYEN VONG: QUAN HE NHIEU - NHIEU GIUA THI SINH & NGANH (Khoa ngoai: soBaoDanh, maNganh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblNguyenVongRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("soBaoDanh (FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("thuTuNV", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("maNganh (FK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Nganh Dang Ky", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("trangThai", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableHeadersRow();

                            int sttNV = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                const Vector<NguyenVong>& dsNV = ts.getDsNguyenVong();
                                for (size_t j = 0; j < dsNV.size(); ++j) {
                                    sttNV++;
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_nvr_%d", sttNV); RenderRowHover(rId);
                                    ImGui::Text("%d", sttNV);
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ts.getSoBaoDanh().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::Text("NV%d", dsNV[j].thuTuNV);
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", dsNV[j].maNganh.c_str());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%s", ql.layTenNganh(dsNV[j].maNganh).c_str());
                                    ImGui::TableSetColumnIndex(5);
                                    if (dsNV[j].trangThai == AdmissionConst::TT_NV_TRUNG_TUYEN) {
                                        ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%s", dsNV[j].trangThai.c_str());
                                    } else if (dsNV[j].trangThai == AdmissionConst::TT_NV_KHONG_TRUNG) {
                                        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "%s", dsNV[j].trangThai.c_str());
                                    } else {
                                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", dsNV[j].trangThai.c_str());
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.4: BẢNG KHOA ĐÀO TẠO
                    if (ImGui::BeginTabItem("  Bang 4: KHOA (MySQL: khoa)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC KHOA CHUYEN MON DUT (Khoa chinh: maKhoa)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblKhoaRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maKhoa (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenKhoa", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("maKhoaTheoChuanDUT", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                                const Khoa& k = ql.getDanhSachKhoa()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_kr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", k.maKhoa.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", k.tenKhoa.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", k.maKhoaTheoChuanDUT.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.5: BẢNG DANH MỤC NGÀNH TĨNH
                    if (ImGui::BeginTabItem("  Bang 5: DANH MUC NGANH (MySQL: danh_muc_nganh)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC NGANH DAO TAO CHUAN BO GD&DT (Khoa chinh: maNganh | Khoa ngoai: maKhoa)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblDanhMucNganhRaw", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maNganh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenNganh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("maKhoa (FK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("tienToLop", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                                const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_dmnr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", dmn.maNganh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", dmn.tenNganh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", dmn.maKhoa.c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%s", dmn.tienToLop.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.6: BẢNG NGÀNH THEO KHÓA HỌC (BIẾN ĐỘNG)
                    if (ImGui::BeginTabItem("  Bang 6: NGANH THEO KHOA HOC (MySQL: nganh_theo_khoa_hoc)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG CHI TIEU VA DIEM CHUAN BIEN DONG THEO KHOA (Khoa chinh kep: namTuyenSinh, maNganh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblNganhTheoKhoaHocRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("namTuyenSinh (PK, FK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("maNganh (PK, FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("chiTieu", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableSetupColumn("diemSanToiThieu", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("diemChuanChot", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_ntkr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%d", ntk.namTuyenSinh);
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ntk.maNganh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%d", ntk.chiTieu);
                                ImGui::TableSetColumnIndex(4);
                                ImGui::Text("%.2f", ntk.diemSanToiThieu);
                                ImGui::TableSetColumnIndex(5);
                                if (ntk.diemChuanChot > 0.0) {
                                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%.2f", ntk.diemChuanChot);
                                } else {
                                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Chua chot");
                                }
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.7: BẢNG KHÓA HỌC
                    if (ImGui::BeginTabItem("  Bang 7: KHOA HOC (MySQL: khoa_hoc)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC KHOA HOC / NAM TUYEN SINH (Khoa chinh: namTuyenSinh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblKhoaHocRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("namTuyenSinh (PK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("tenKhoaHoc", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("trangThai", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachKhoaHoc().size(); ++i) {
                                const KhoaHoc& kh = ql.getDanhSachKhoaHoc()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_khr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%d", kh.namTuyenSinh);
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", kh.tenKhoaHoc.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%s", kh.trangThai.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.8: BẢNG DANH MỤC TỈNH THÀNH
                    if (ImGui::BeginTabItem("  Bang 8: TINH THANH (MySQL: tinh_thanh)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC: 34 TINH / THANH PHO (Khoa chinh: maTinh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblTinhThanhRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maTinh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenTinh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("loai", ImGuiTableColumnFlags_WidthFixed, 260);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_ttr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tt.maTinh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", tt.tenTinh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", tt.loai.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.9: BẢNG DANH MỤC GIỚI TÍNH
                    if (ImGui::BeginTabItem("  Bang 9: GIOI TINH (MySQL: gioi_tinh)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG DANH MUC: GIOI TINH (Khoa chinh: maGioiTinh)");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblGioiTinhRaw", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maGioiTinh (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("tenGioiTinh", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("moTa", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_gtr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", gt.maGioiTinh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gt.tenGioiTinh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", gt.moTa.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.10: BẢNG CẤU HÌNH HỆ THỐNG
                    if (ImGui::BeginTabItem("  Bang 10: CAU HINH (MySQL: cau_hinh)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG THAM SO HE THONG & TUYEN SINH (MySQL: cau_hinh)");
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("SystemCfgView", ImVec2(0, 220), true);
                        {
                            ImGui::BulletText("Truong Dao tao         : %s", ql.getTenTruong().c_str());
                            ImGui::BulletText("Nam Tuyen Sinh Hien Hanh: %d (Khoa K%d)", ql.getNamTuyenSinhHienHanh(), ql.getNamTuyenSinhHienHanh() % 100);
                            ImGui::BulletText("Si So Lop Chuan Dong   : %d sinh vien / lop", ql.getSiSoLopChuan());
                            ImGui::BulletText("Diem San Toi Thieu     : %.2f diem", ql.getDiemSanMacDinh());
                            ImGui::BulletText("Nguong Diem Liet       : %.2f diem", ql.getDiemLiet());
                            ImGui::BulletText("Tong So Khoa Dao Tao   : %d khoa DUT", (int)ql.getDanhSachKhoa().size());
                            ImGui::BulletText("Tong So Nganh Tuyen Sinh: %d nganh", (int)ql.getDanhSachDanhMucNganh().size());
                            ImGui::BulletText("Tong So Thi Sinh Dang Ky: %d thi sinh", (int)ql.getDanhSach().size());
                            ImGui::BulletText("Tong So Tinh Thanh     : %d tinh/thanh", (int)ql.getDanhSachTinhThanh().size());
                            ImGui::BulletText("Tong So Gioi Tinh      : %d gioi tinh", (int)ql.getDanhSachGioiTinh().size());
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.11: SƠ ĐỒ LIÊN KẾT ERD (10 BẢNG 3NF DUT)
                    if (ImGui::BeginTabItem("  So do Lien ket ERD (10 Bang 3NF DUT)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "SO DO CHI TIET LIEN KET KHOA CHINH (PK) VA KHOA NGOAI (FK) 10 BANG CHUAN 3NF:");
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.13f, 0.18f, 1.0f));
                        ImGui::BeginChild("ERDView", ImVec2(0, ImGui::GetWindowHeight() - 250), true);
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), 
                                "========================================================================================================\n"
                                "                          SO DO THUC THE - LIEN KET ERD 10 BANG CHUAN 3NF (DUT ADMISSION)\n"
                                "========================================================================================================\n\n"
                                "    [khoa_hoc]                               [khoa]\n"
                                "    * namTuyenSinh (PK)                      * maKhoa (PK)\n"
                                "      tenKhoaHoc                               tenKhoa\n"
                                "      namBatDau, namKetThuc                    maKhoaTheoChuanDUT (102, 105, 101...)\n"
                                "         |                                        |\n"
                                "         | 1                                      | 1\n"
                                "         |                                        v N (FK: maKhoa)\n"
                                "         |                                   [danh_muc_nganh]\n"
                                "         |                                   * maNganh (PK)\n"
                                "         |                                     tenNganh\n"
                                "         |                                   * maKhoa (FK -> khoa.maKhoa)\n"
                                "         |                                     tienToLop (TCLC_KHDL, T_DT, TDH...)\n"
                                "         |                                        |\n"
                                "         v 1                                      v 1\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    |                        nganh_theo_khoa_hoc (Chi tieu & Diem chuan)              |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    | * namTuyenSinh (PK, FK -> khoa_hoc.namTuyenSinh)                                |\n"
                                "    | * maNganh      (PK, FK -> danh_muc_nganh.maNganh)                               |\n"
                                "    |   chiTieu                                                                       |\n"
                                "    |   diemSanToiThieu                                                               |\n"
                                "    |   diemChuanChot                                                                 |\n"
                                "    +---------------------------------------------------------------------------------+\n\n"
                                "    [tinh_thanh]                              [gioi_tinh]\n"
                                "    * maTinh (PK)                             * maGioiTinh (PK)\n"
                                "      tenTinh                                   tenGioiTinh\n"
                                "      loai                                      moTa\n"
                                "         ^                                         ^\n"
                                "         | 1                                       | 1\n"
                                "         | N (FK: queQuan)                         | N (FK: gioiTinh)\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    |                                     nguoi                                       |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    | * soCCCD (PRIMARY KEY - PK)   : So Can cuoc Cong dan (12 chu so)                |\n"
                                "    |   hoTen                       : Ho va ten                                       |\n"
                                "    |   ngaySinh                    : dd/mm/yyyy                                      |\n"
                                "    | * queQuan (FOREIGN KEY - FK)  : Tham chieu den TINH THANH(maTinh)               |\n"
                                "    | * gioiTinh (FOREIGN KEY - FK) : Tham chieu den GIOI TINH(maGioiTinh)            |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "                                          ^\n"
                                "                                          | 1 (Quan he 1 - 1 qua Ke thua)\n"
                                "                                          |\n"
                                "                                          | 1 (FK: soCCCD)\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    |                                    thisinh                                      |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    | * soBaoDanh (PRIMARY KEY - PK): So bao danh du thi (DUTxxx)                     |\n"
                                "    | * soCCCD (FOREIGN KEY - FK)   : Khoa ngoai tham chieu sang bang NGUOI           |\n"
                                "    | * namTuyenSinh (FK)           : Tham chieu sang bang KHOA HOC                   |\n"
                                "    |   diemToan, diemLy, diemHoa   : Diem 3 mon Toan - Ly - Hoa (0.00 - 10.00)       |\n"
                                "    |   maSinhVien                  : Ma SV DUT 9 chu so (VD: 102260001)              |\n"
                                "    |   lopSinhHoat                 : Lop sinh hoat DUT (VD: 26TCLC_KHDL1)            |\n"
                                "    |   nganhTrungTuyen             : Ma nganh trung tuyen                            |\n"
                                "    |   nguyenVongTrungTuyen        : Thu tu NV trung tuyen (1, 2, 3)                 |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "                                          |\n"
                                "                                          | 1 (Quan he 1 - Nhieu)\n"
                                "                                          | N (FK: soBaoDanh)\n"
                                "                                          v\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    |                                  nguyen_vong                                    |\n"
                                "    +---------------------------------------------------------------------------------+\n"
                                "    | * id (PRIMARY KEY - AI)       : Ma dinh danh tu dong tang                       |\n"
                                "    | * soBaoDanh (FK)              : Tham chieu sang thisinh.soBaoDanh               |\n"
                                "    |   thuTuNV                     : Thu tu nguyen vong uu tien (1, 2, 3)            |\n"
                                "    | * maNganh (FK)                : Tham chieu sang danh_muc_nganh.maNganh          |\n"
                                "    |   trangThai                   : TRUNG TUYEN / KHONG TRUNG TUYEN / DANG XET      |\n"
                                "    +---------------------------------------------------------------------------------+\n\n"
                                "    [cau_hinh] (Tham so toan cuc: namTuyenSinhHienHanh, tenTruong, siSoLopChuan, diemLiet, diemSan)\n");
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 3: XÉT TUYỂN & ĐIỂM CHUẨN ĐỘNG DUT ====================
            if (ImGui::BeginTabItem("  3. Xet tuyen & Thong ke DUT  ")) {
                ImGui::Spacing();

                // Thanh điều khiển Xét tuyển Tuyển sinh DUT
                ImGui::BeginChild("AdmissionControlHeader", ImVec2(0, 85), true);
                {
                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "QUY TRINH XET TUYEN DA NGUYEN VONG & CHOT DIEM CHUAN DUT (KHOA %d):", ql.getNamTuyenSinhHienHanh());
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.60f, 0.35f, 1.0f));
                    if (ImGui::Button("[ >> CHAY XET TUYEN & CHOT DIEM CHUAN DUT << ]", ImVec2(380, 32))) {
                        ql.chayThuatToanXetTuyenVaChotDiemChuan(ql.getNamTuyenSinhHienHanh());
                        sortCol = 10;   // Tự động sắp xếp theo cột Tổng điểm
                        sortAsc = false; // Điểm cao nhất từ trên xuống dưới
                        currentPage = 1;
                        showToast = true;
                        toastMsg = "Da xet tuyen, chot diem chuan va cap MSSV DUT thanh cong!";
                    }
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(110);
                    static int curSiSoInput = ql.getSiSoLopChuan();
                    if (ImGui::InputInt("Si so lop tieu chuan", &curSiSoInput)) {
                        if (curSiSoInput < 10) curSiSoInput = 10;
                        if (curSiSoInput > 100) curSiSoInput = 100;
                        ql.setSiSoLopChuan(curSiSoInput);
                    }

                    ImGui::SameLine(ImGui::GetWindowWidth() - 250);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.75f, 1.0f));
                    if (ImGui::Button("[ Xuat DS Trung Tuyen DUT ]", ImVec2(230, 32))) {
                        ql.xuatBaoCaoTrungTuyenDUT("data/danhsach_trungtuyen_dut.txt", ql.getNamTuyenSinhHienHanh());
                        showToast = true;
                        toastMsg = "Da xuat data/danhsach_trungtuyen_dut.txt!";
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::EndChild();

                ImGui::Spacing();

                if (ImGui::BeginTabBar("SubTabsAdmissionDUT", ImGuiTabBarFlags_None)) {

                    // SUBTAB 3.1: BẢNG CHỈ TIÊU & ĐIỂM CHUẨN CHỐT THEO NGÀNH
                    if (ImGui::BeginTabItem("  1. Bang Diem Chuan Chot & Chi Tieu Nganh DUT  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "DANH MUC NGANH TUYEN SINH, CHI TIEU VA DIEM CHUAN CHOT K%d:", ql.getNamTuyenSinhHienHanh() % 100);
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblDiemChuanNganh", 10, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 290))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Nganh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Tien To Lop", ImGuiTableColumnFlags_WidthFixed, 105);
                            ImGui::TableSetupColumn("Chi Tieu", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Diem San", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Diem Chuan Chot", ImGuiTableColumnFlags_WidthFixed, 125);
                            ImGui::TableSetupColumn("Da Trung", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Du Kien Lop", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableHeadersRow();

                            int stt = 0;
                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                                stt++;

                                string tenNganh = ql.layTenNganh(ntk.maNganh);
                                string maKhoa = ql.layMaKhoaCuaNganh(ntk.maNganh);
                                string tienTo = ql.layTienToLop(ntk.maNganh);

                                // Đếm số lượng đã trúng tuyển ngành này
                                int countAdmitted = 0;
                                for (size_t k = 0; k < ql.getDanhSach().size(); ++k) {
                                    if (ql.getDanhSach()[k].getNamTuyenSinh() == ql.getNamTuyenSinhHienHanh() &&
                                        ql.getDanhSach()[k].getNganhTrungTuyen() == ntk.maNganh &&
                                        ql.getDanhSach()[k].daTrungTuyen()) {
                                        countAdmitted++;
                                    }
                                }

                                int siSo = ql.getSiSoLopChuan() > 0 ? ql.getSiSoLopChuan() : 30;
                                int soLop = countAdmitted > 0 ? ((countAdmitted - 1) / siSo + 1) : 0;

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_dcn_%d", stt); RenderRowHover(rId);
                                ImGui::Text("%d", stt);
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ntk.maNganh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", tenNganh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", maKhoa.c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tienTo.c_str());
                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%d", ntk.chiTieu);
                                ImGui::TableSetColumnIndex(6);
                                ImGui::Text("%.2f", ntk.diemSanToiThieu);
                                ImGui::TableSetColumnIndex(7);
                                if (ntk.diemChuanChot > 0.0) {
                                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%.2f diem", ntk.diemChuanChot);
                                } else {
                                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Chua chot");
                                }
                                ImGui::TableSetColumnIndex(8);
                                ImGui::TextColored(countAdmitted > 0 ? ImVec4(0.2f, 0.95f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%d / %d", countAdmitted, ntk.chiTieu);
                                ImGui::TableSetColumnIndex(9);
                                ImGui::Text("%d lop", soLop);
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 3.2: DANH SÁCH TÂN SINH VIÊN TRÚNG TUYỂN & PHÂN LỚP
                    if (ImGui::BeginTabItem("  2. Danh Sach Tan Sinh Vien & Lop Sinh Hoat DUT  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.75f, 1.0f), "DANH SACH TAN SINH VIEN DA CAP MA SINH VIEN VA PHAN LOP SINH HOAT K%d:", ql.getNamTuyenSinhHienHanh() % 100);
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblTanSinhVienDUT", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 290))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Sinh Vien DUT", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Lop Sinh Hoat", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("Nganh Trung Tuyen", ImGuiTableColumnFlags_WidthFixed, 220);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("NV", ImGuiTableColumnFlags_WidthFixed, 45);
                            ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 85);
                            ImGui::TableHeadersRow();

                            int sttAdmitted = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                if (ts.getNamTuyenSinh() == ql.getNamTuyenSinhHienHanh() && ts.daTrungTuyen()) {
                                    sttAdmitted++;
                                    string tenNganh = ql.layTenNganh(ts.getNganhTrungTuyen());
                                    string maKhoa = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_tsv_%d", sttAdmitted); RenderRowHover(rId);
                                    ImGui::Text("%d", sttAdmitted);
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", ts.getMaSinhVien().c_str());
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::Text("%s", ts.getHoTen().c_str());
                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "%s", ts.getLopSinhHoat().c_str());
                                    ImGui::TableSetColumnIndex(4);
                                    ImGui::Text("%s", tenNganh.c_str());
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::Text("%s", maKhoa.c_str());
                                    ImGui::TableSetColumnIndex(6);
                                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "NV%d", ts.getNguyenVongTrungTuyen());
                                    ImGui::TableSetColumnIndex(7);
                                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%.2f", ts.tinhTongDiem());
                                }
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 3.3: VINH DANH THỦ KHOA & PHỔ ĐIỂM
                    if (ImGui::BeginTabItem("  3. Thu Khoa & Pho Diem Ky Thi  ")) {
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
                            if (ImGui::BeginTable("ThuKhoaTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 320))) {
                                ImGui::TableSetupColumn("SBD", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthFixed, 110);
                                ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 85);
                                ImGui::TableSetupColumn("Danh Hieu", ImGuiTableColumnFlags_WidthFixed, 110);
                                ImGui::TableHeadersRow();

                                for (size_t i = 0; i < dsTK.size(); ++i) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_tk_%d", (int)i); RenderRowHover(rId);
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

                        ImGui::BeginChild("HistogramView", ImVec2(0, ImGui::GetWindowHeight() - 320), true);
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

                    ImGui::EndTabBar();
                }

                ImGui::EndTabItem();
            }
            // ==================== TAB 4: QUẢN LÝ DANH MỤC HỆ THỐNG (MỚI ✨) ====================
            if (ImGui::BeginTabItem("  4. Quan ly Danh muc He thong  ")) {
                ImGui::Spacing();

                if (ImGui::BeginTabBar("SubTabsDanhMuc", ImGuiTabBarFlags_None)) {

                    // SUBTAB 4.1: DANH MỤC KHOA ĐÀO TẠO DUT
                    if (ImGui::BeginTabItem("  1. Khoa Dao tao DUT  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "DANH MUC KHOA CHUYEN MON TRUONG DAI HOC BACH KHOA - DH DA NANG (DUT):");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblManageKhoa", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Khoa (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Ten Khoa Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Ma Dinh Danh DUT", ImGuiTableColumnFlags_WidthFixed, 150);
                            ImGui::TableSetupColumn("Quy Uoc Ma SV", ImGuiTableColumnFlags_WidthFixed, 220);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                                const Khoa& k = ql.getDanhSachKhoa()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_kr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", k.maKhoa.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", k.tenKhoa.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", k.maKhoaTheoChuanDUT.c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%s26xxxx (VD: %s260001)", k.maKhoaTheoChuanDUT.c_str(), k.maKhoaTheoChuanDUT.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.2: DANH MỤC NGÀNH ĐÀO TẠO DUT
                    if (ImGui::BeginTabItem("  2. Danh muc Nganh Tuyen sinh DUT  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "DANH MUC NGANH TUYEN SINH CHUAN BO GD&DT & TIEN TO LOP SINH HOAT DUT:");
                        ImGui::Separator();

                        if (ImGui::BeginTable("TblManageDanhMucNganh", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Nganh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa Truc Thuoc", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableSetupColumn("Tien To Lop DUT", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableSetupColumn("Mau Lop K26", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                                const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_dmnr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", dmn.maNganh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", dmn.tenNganh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", dmn.maKhoa.c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", dmn.tienToLop.c_str());
                                ImGui::TableSetColumnIndex(5);
                                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "26%s1, 26%s2", dmn.tienToLop.c_str(), dmn.tienToLop.c_str());
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.3: CHỈ TIÊU & ĐIỂM SÀN THEO KHÓA (ĐIỀU CHỈNH ĐỘNG)
                    if (ImGui::BeginTabItem("  3. Chi tieu Tuyen sinh Dong (K26)  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "QUAN LY VA DIEU CHINH CHI TIEU TUYEN SINH DONG THEO KHOA (K%d):", ql.getNamTuyenSinhHienHanh() % 100);
                        ImGui::Separator();

                        // Form điều chỉnh nhanh chỉ tiêu
                        static string curSelNganh = "7480201B";
                        static int curInputChiTieu = 50;

                        ImGui::BeginChild("EditQuotaHeader", ImVec2(0, 75), true);
                        {
                            ImGui::Text("Chon Nganh:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(340);
                            RenderComboNganh("##selNganhQuota", ql.getDanhSachDanhMucNganh(), curSelNganh);

                            ImGui::SameLine();
                            ImGui::Text("Chi tieu moi:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(100);
                            ImGui::InputInt("##valNewQuota", &curInputChiTieu);
                            if (curInputChiTieu < 1) curInputChiTieu = 1;
                            if (curInputChiTieu > 1000) curInputChiTieu = 1000;

                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                            if (ImGui::Button("[ Luu Chi Tieu Nganh ]", ImVec2(170, 26))) {
                                if (ql.capNhatChiTieuNganh(ql.getNamTuyenSinhHienHanh(), curSelNganh, curInputChiTieu)) {
                                    showToast = true;
                                    toastMsg = "Da cap nhat chi tieu nganh " + curSelNganh + " thanh " + to_string(curInputChiTieu) + "!";
                                }
                            }
                            ImGui::PopStyleColor();
                        }
                        ImGui::EndChild();

                        ImGui::Spacing();

                        if (ImGui::BeginTable("TblManageNganhTheoKhoaHoc", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 320))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Nganh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Chi Tieu Hien Tai", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Diem San", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Diem Chuan Chot", ImGuiTableColumnFlags_WidthFixed, 115);
                            ImGui::TableSetupColumn("Dieu Chinh Nhanh", ImGuiTableColumnFlags_WidthFixed, 150);
                            ImGui::TableHeadersRow();

                            int sttQ = 0;
                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                                sttQ++;

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_mntk_%d", sttQ); RenderRowHover(rId);
                                ImGui::Text("%d", sttQ);
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", ntk.maNganh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", ql.layTenNganh(ntk.maNganh).c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", ql.layMaKhoaCuaNganh(ntk.maNganh).c_str());
                                ImGui::TableSetColumnIndex(4);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%d chi tieu", ntk.chiTieu);
                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%.2f", ntk.diemSanToiThieu);
                                ImGui::TableSetColumnIndex(6);
                                if (ntk.diemChuanChot > 0.0) {
                                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "%.2f", ntk.diemChuanChot);
                                } else {
                                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Chua chot");
                                }
                                ImGui::TableSetColumnIndex(7);
                                ImGui::PushID((int)(5000 + i));
                                if (ImGui::SmallButton("- 5")) {
                                    int m = (ntk.chiTieu > 5) ? (ntk.chiTieu - 5) : 1;
                                    ql.capNhatChiTieuNganh(ntk.namTuyenSinh, ntk.maNganh, m);
                                }
                                ImGui::SameLine();
                                if (ImGui::SmallButton("+ 5")) {
                                    ql.capNhatChiTieuNganh(ntk.namTuyenSinh, ntk.maNganh, ntk.chiTieu + 5);
                                }
                                ImGui::SameLine();
                                if (ImGui::SmallButton("Chon")) {
                                    curSelNganh = ntk.maNganh;
                                    curInputChiTieu = ntk.chiTieu;
                                }
                                ImGui::PopID();
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.4: QUẢN LÝ TỈNH / THÀNH PHỐ
                    if (ImGui::BeginTabItem("  4. Danh muc Tinh / Thanh pho  ")) {
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
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_ttr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", tt.maTinh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", tt.tenTinh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", tt.loai.c_str());

                                ImGui::TableSetColumnIndex(4);
                                ImGui::PushID((int)(1000 + i));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.7f, 1.0f));
                                if (ImGui::SmallButton("Sua")) {
                                    strcpy(editTinhThanhMa, tt.maTinh.c_str());
                                    strcpy(editTinhThanhTen, tt.tenTinh.c_str());
                                    strcpy(editTinhThanhLoai, tt.loai.c_str());
                                    editTinhThanhError = "";
                                    showEditTinhThanhModal = true;
                                }
                                ImGui::PopStyleColor();
                                ImGui::SameLine();
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                                if (ImGui::SmallButton("Xoa")) {
                                    if (ql.xoaTinhThanh(tt.maTinh)) {
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

                    // SUBTAB 4.5: QUẢN LÝ GIỚI TÍNH
                    if (ImGui::BeginTabItem("  5. Danh muc Gioi tinh  ")) {
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
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                char rId[32]; sprintf(rId, "##row_gtr_%d", (int)i); RenderRowHover(rId);
                                ImGui::Text("%d", (int)(i + 1));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", gt.maGioiTinh.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gt.tenGioiTinh.c_str());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%s", gt.moTa.c_str());

                                ImGui::TableSetColumnIndex(4);
                                ImGui::PushID((int)(2000 + i));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.7f, 1.0f));
                                if (ImGui::SmallButton("Sua")) {
                                    strcpy(editGioiTinhMa, gt.maGioiTinh.c_str());
                                    strcpy(editGioiTinhTen, gt.tenGioiTinh.c_str());
                                    strcpy(editGioiTinhDesc, gt.moTa.c_str());
                                    editGioiTinhError = "";
                                    showEditGioiTinhModal = true;
                                }
                                ImGui::PopStyleColor();
                                ImGui::SameLine();
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                                if (ImGui::SmallButton("Xoa")) {
                                    if (ql.xoaGioiTinh(gt.maGioiTinh)) {
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

                    // SUBTAB 4.6: THAM SỐ TUYỂN SINH & CẤU HÌNH HỆ THỐNG
                    if (ImGui::BeginTabItem("  6. Tham so Tuyen sinh & Cau hinh  ")) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "THIET LAP THAM SO KY THI (MySQL: cau_hinh):");
                        ImGui::Separator();

                        ImGui::BeginChild("SystemParamsChild", ImVec2(0, 360), true);
                        {
                            ImGui::Text("Ten Truong Dai Hoc:");
                            ImGui::SetNextItemWidth(450);
                            ImGui::InputText("##truongSys", bufTenTruong, sizeof(bufTenTruong));

                            ImGui::Spacing();
                            ImGui::Text("Nam Tuyen Sinh Hien Hanh (VD: 2026):");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputText("##namSys", bufNamTuyenSinh, sizeof(bufNamTuyenSinh));

                            ImGui::Spacing();
                            ImGui::Text("Si So Lop Sinh Hoat Tieu Chuan (Dong):");
                            ImGui::SetNextItemWidth(200);
                            static int formSiSo = ql.getSiSoLopChuan();
                            if (ImGui::InputInt("##sisoSys", &formSiSo)) {
                                if (formSiSo < 10) formSiSo = 10;
                                if (formSiSo > 100) formSiSo = 100;
                                ql.setSiSoLopChuan(formSiSo);
                            }

                            ImGui::Spacing();
                            ImGui::Text("Diem Chuan Xet Tuyen Khoi Tao (thang 30):");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputFloat("##dcSys", &formDiemChuan, 0.0f, 0.0f, "%.2f");

                            ImGui::Spacing();
                            ImGui::Text("Nguong Diem Liet Mon Thi (mac dinh 1.00):");
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputFloat("##dlSys", &formDiemLiet, 0.0f, 0.0f, "%.2f");

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.75f, 1.0f));
                            if (ImGui::Button("[ Luu Cau Hinh He Thong ]", ImVec2(220, 32))) {
                                ql.setTenTruong(Utils::trim(bufTenTruong));
                                ql.setNamTuyenSinh(Utils::trim(bufNamTuyenSinh));
                                ql.setSiSoLopChuan(formSiSo);
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
                if (ImGui::Button("[ Luu va Dong bo Toan Bo Du Lieu Vao MySQL (Commit) ]", ImVec2(480, 36))) {
                    if (ql.luuDuLieuVaoMySQL()) {
                        showToast = true;
                        toastMsg = "Da luu toan bo du lieu vao MySQL Database thanh cong!";
                    } else {
                        showToast = true;
                        toastMsg = "Loi luu du lieu vao MySQL! Kiem tra ket noi.";
                    }
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
                ImGui::BulletText("1. tinh_thanh: Bang danh muc TINH THANH (34 tinh thanh theo NQ 202/2025/QH15 trong MySQL).");
                ImGui::BulletText("2. gioi_tinh: Bang danh muc GIOI TINH (NAM, NU, KHAC trong MySQL).");
                ImGui::BulletText("3. nguoi: Bang thuc the cha NGUOI (PK: so_cccd, FK: que_quan, FK: gioi_tinh trong MySQL).");
                ImGui::BulletText("4. thisinh: Bang thuc the con THI SINH (PK: sbd, FK: so_cccd CASCADE DELETE trong MySQL).");
                ImGui::BulletText("5. cau_hinh: Bang tham so he thong (Truong, Nam tuyen sinh, Diem chuan, Diem liet trong MySQL).");
                ImGui::BulletText("6. data/ketqua_xettuyen.txt, data/thukhoa.txt, data/thongke_phodiem.txt: Bao cao dong.");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // ==================== MODAL: THÊM THÍ SINH ====================
        if (showAddModal) {
            ImGui::OpenPopup("Them Thi Sinh Moi");
        }
        ImGui::SetNextWindowSize(ImVec2(720.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Them Thi Sinh Moi", &showAddModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "NHAP HO SO VA DIEM THI THI SINH");
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTable("FormAddTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("ColLabel", ImGuiTableColumnFlags_WidthFixed, 195.0f);
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

                // Row 10: Nguyện vọng 1
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Nguyen Vong 1 (NV1)*:");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV1Add", ql.getDanhSachDanhMucNganh(), formNV1, formNV2, formNV3)) {
                    if (!formNV1.empty()) {
                        if (formNV2 == formNV1) formNV2.clear();
                        if (formNV3 == formNV1) formNV3.clear();
                    }
                }

                // Row 11: Nguyện vọng 2
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Nguyen Vong 2 (NV2):");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV2Add", ql.getDanhSachDanhMucNganh(), formNV2, formNV1, formNV3)) {
                    if (!formNV2.empty()) {
                        if (formNV1 == formNV2) formNV1.clear();
                        if (formNV3 == formNV2) formNV3.clear();
                    }
                }

                // Row 12: Nguyện vọng 3
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Nguyen Vong 3 (NV3):");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV3Add", ql.getDanhSachDanhMucNganh(), formNV3, formNV1, formNV2)) {
                    if (!formNV3.empty()) {
                        if (formNV1 == formNV3) formNV1.clear();
                        if (formNV2 == formNV3) formNV2.clear();
                    }
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::Separator();

            double tongDiem = formToan + formLy + formHoa;
            bool liet = (formToan <= ql.getDiemLiet() || formLy <= ql.getDiemLiet() || formHoa <= ql.getDiemLiet());
            bool duSan = (!liet && tongDiem >= ql.getDiemSan());

            if (liet) {
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [!] BI DIEM LIET (Co mon <= %.2f - Khong du dieu kien)",
                    tongDiem, ql.getDiemLiet());
            } else if (!duSan) {
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [!] KHONG DAT DIEM SAN (Toi thieu %.2f - Khong du dieu kien)",
                    tongDiem, ql.getDiemSan());
            } else {
                ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [OK] DU DIEU KIEN XET TUYEN (San >= %.2f - Dang cho quet)",
                    tongDiem, ql.getDiemSan());
            }

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
                } else if (formNV1.empty()) {
                    formErrorMsg = "Nguyen Vong 1 (NV1) la bat buoc, vui long chon nganh!";
                } else if (!formNV1.empty() && !formNV2.empty() && formNV1 == formNV2) {
                    formErrorMsg = "Nguyen Vong 2 khong duoc trung nganh voi Nguyen Vong 1!";
                } else if (!formNV1.empty() && !formNV3.empty() && formNV1 == formNV3) {
                    formErrorMsg = "Nguyen Vong 3 khong duoc trung nganh voi Nguyen Vong 1!";
                } else if (!formNV2.empty() && !formNV3.empty() && formNV2 == formNV3) {
                    formErrorMsg = "Nguyen Vong 3 khong duoc trung nganh voi Nguyen Vong 2!";
                } else {
                    ThiSinh moi(cccd, hoten, ns, formQueQuanMa, formGioiTinhMa, sbd, (double)formToan, (double)formLy, (double)formHoa, ql.getNamTuyenSinhHienHanh());
                    string nvTrangThai = duSan ? AdmissionConst::TT_NV_DANG_XET : AdmissionConst::TT_NV_KHONG_TRUNG;
                    Vector<NguyenVong> dsNV;
                    if (!formNV1.empty()) dsNV.push_back(NguyenVong(0, sbd, 1, formNV1, nvTrangThai));
                    if (!formNV2.empty()) dsNV.push_back(NguyenVong(0, sbd, 2, formNV2, nvTrangThai));
                    if (!formNV3.empty()) dsNV.push_back(NguyenVong(0, sbd, 3, formNV3, nvTrangThai));
                    moi.setDsNguyenVong(dsNV);

                    if (ql.themThiSinh(moi)) {
                        reportContent.clear();
                        sortCol = 0; // Sắp xếp theo ngày sửa/tạo mới nhất
                        sortAsc = false;
                        currentPage = 1;
                        selectedSBD = sbd;
                        showAddModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them thi sinh moi thanh cong! Ho so da duoc dua len dau bang.";
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
        ImGui::SetNextWindowSize(ImVec2(720.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Cap Nhat Thong Tin Thi Sinh", &showEditModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT THONG TIN THI SINH (SBD: %s)", bufSBD);
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTable("FormEditTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("ColLabelEd", ImGuiTableColumnFlags_WidthFixed, 195.0f);
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

                // Row 10: Nguyện vọng 1
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Nguyen Vong 1 (NV1)*:");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV1Edit", ql.getDanhSachDanhMucNganh(), formNV1, formNV2, formNV3)) {
                    if (!formNV1.empty()) {
                        if (formNV2 == formNV1) formNV2.clear();
                        if (formNV3 == formNV1) formNV3.clear();
                    }
                }

                // Row 11: Nguyện vọng 2
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Nguyen Vong 2 (NV2):");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV2Edit", ql.getDanhSachDanhMucNganh(), formNV2, formNV1, formNV3)) {
                    if (!formNV2.empty()) {
                        if (formNV1 == formNV2) formNV1.clear();
                        if (formNV3 == formNV2) formNV3.clear();
                    }
                }

                // Row 12: Nguyện vọng 3
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Nguyen Vong 3 (NV3):");
                ImGui::TableSetColumnIndex(1);
                if (RenderComboNganh("##comboNV3Edit", ql.getDanhSachDanhMucNganh(), formNV3, formNV1, formNV2)) {
                    if (!formNV3.empty()) {
                        if (formNV1 == formNV3) formNV1.clear();
                        if (formNV2 == formNV3) formNV2.clear();
                    }
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::Separator();

            double tongDiemEd = formToan + formLy + formHoa;
            bool lietEd = (formToan <= ql.getDiemLiet() || formLy <= ql.getDiemLiet() || formHoa <= ql.getDiemLiet());
            bool duSanEd = (!lietEd && tongDiemEd >= ql.getDiemSan());

            if (lietEd) {
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [!] BI DIEM LIET (Co mon <= %.2f - Khong du dieu kien)",
                    tongDiemEd, ql.getDiemLiet());
            } else if (!duSanEd) {
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [!] KHONG DAT DIEM SAN (Toi thieu %.2f - Khong du dieu kien)",
                    tongDiemEd, ql.getDiemSan());
            } else {
                ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "Tong Diem: %.2f  |  Trang Thai: [OK] DU DIEU KIEN XET TUYEN (San >= %.2f - Dang cho quet)",
                    tongDiemEd, ql.getDiemSan());
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
                } else if (formNV1.empty()) {
                    formErrorMsg = "Nguyen Vong 1 (NV1) la bat buoc, vui long chon nganh!";
                } else if (!formNV1.empty() && !formNV2.empty() && formNV1 == formNV2) {
                    formErrorMsg = "Nguyen Vong 2 khong duoc trung nganh voi Nguyen Vong 1!";
                } else if (!formNV1.empty() && !formNV3.empty() && formNV1 == formNV3) {
                    formErrorMsg = "Nguyen Vong 3 khong duoc trung nganh voi Nguyen Vong 1!";
                } else if (!formNV2.empty() && !formNV3.empty() && formNV2 == formNV3) {
                    formErrorMsg = "Nguyen Vong 3 khong duoc trung nganh voi Nguyen Vong 2!";
                } else {
                    int namTS = (selectedRow >= 0 && selectedRow < (int)ql.getDanhSach().size()) ? ql.getDanhSach()[selectedRow].getNamTuyenSinh() : ql.getNamTuyenSinhHienHanh();
                    ThiSinh capNhat(cccd, hoten, ns, formQueQuanMa, formGioiTinhMa, bufSBD, (double)formToan, (double)formLy, (double)formHoa, namTS);
                    capNhat.setMaSinhVien("");
                    capNhat.setLopSinhHoat("");
                    capNhat.setNganhTrungTuyen("");
                    capNhat.setNguyenVongTrungTuyen(0);

                    string nvTrangThaiEd = duSanEd ? AdmissionConst::TT_NV_DANG_XET : AdmissionConst::TT_NV_KHONG_TRUNG;
                    Vector<NguyenVong> dsNV;
                    if (!formNV1.empty()) dsNV.push_back(NguyenVong(0, bufSBD, 1, formNV1, nvTrangThaiEd));
                    if (!formNV2.empty()) dsNV.push_back(NguyenVong(0, bufSBD, 2, formNV2, nvTrangThaiEd));
                    if (!formNV3.empty()) dsNV.push_back(NguyenVong(0, bufSBD, 3, formNV3, nvTrangThaiEd));
                    capNhat.setDsNguyenVong(dsNV);

                    if (ql.capNhatThiSinh(capNhat)) {
                        reportContent.clear();
                        sortCol = 0; // Sắp xếp theo ngày cập nhật mới nhất
                        sortAsc = false;
                        currentPage = 1;
                        selectedSBD = bufSBD;
                        showEditModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat ho so thi sinh thanh cong! Ho so da duoc dua len dau bang.";
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
            int delIdx = ql.timViTriTheoSBD(selectedSBD);
            if (delIdx != -1) {
                const ThiSinh& sel = ql.getDanhSach()[delIdx];
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
                    selectedSBD = "";
                    showDeleteModal = false;
                    ImGui::CloseCurrentPopup();
                    showToast = true;
                    toastMsg = "Da xoa thi sinh va dong bo CSDL thanh cong!";
                }
                ImGui::SameLine();
                if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                    selectedSBD = "";
                    showDeleteModal = false;
                    ImGui::CloseCurrentPopup();
                }
            } else {
                selectedSBD = "";
                showDeleteModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM CHI TIẾT HỒ SƠ THÍ SINH DUT ====================
        if (showDetailModal) {
            ImGui::OpenPopup("Chi Tiet Ho So Thi Sinh DUT");
        }
        ImGui::SetNextWindowSize(ImVec2(720.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Chi Tiet Ho So Thi Sinh DUT", &showDetailModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            int detIdx = ql.timViTriTheoSBD(selectedSBD);
            if (detIdx != -1) {
                const ThiSinh& sel = ql.getDanhSach()[detIdx];
                string sbdDet = sel.getSoBaoDanh();
                string cccdDet = sel.getSoCCCD();
                string hoTenDet = sel.getHoTen();
                string ngaySinhDet = sel.getNgaySinh();
                string queQuanDet = sel.getQueQuan();
                string gioiTinhDet = sel.getGioiTinh();
                float toanDet = (float)sel.getDiemToan();
                float lyDet = (float)sel.getDiemLy();
                float hoaDet = (float)sel.getDiemHoa();

                string nv1Det = "", nv2Det = "", nv3Det = "";
                const Vector<NguyenVong>& dsNV = sel.getDsNguyenVong();
                for (size_t nvIdx = 0; nvIdx < dsNV.size(); ++nvIdx) {
                    if (dsNV[nvIdx].thuTuNV == 1) nv1Det = dsNV[nvIdx].maNganh;
                    else if (dsNV[nvIdx].thuTuNV == 2) nv2Det = dsNV[nvIdx].maNganh;
                    else if (dsNV[nvIdx].thuTuNV == 3) nv3Det = dsNV[nvIdx].maNganh;
                }

                int dDet = 1, mDet = 1, yDet = 2008;
                if (ngaySinhDet.length() == 10) {
                    try {
                        dDet = stoi(ngaySinhDet.substr(0, 2));
                        mDet = stoi(ngaySinhDet.substr(3, 2));
                        yDet = stoi(ngaySinhDet.substr(6, 4));
                    } catch (...) {}
                }

                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CHI TIET HO SO VA DIEM THI THI SINH (SBD: %s)", sbdDet.c_str());
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::BeginDisabled();
                if (ImGui::BeginTable("FormDetailTable", 2, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("ColLabelDt", ImGuiTableColumnFlags_WidthFixed, 195.0f);
                    ImGui::TableSetupColumn("ColInputDt", ImGuiTableColumnFlags_WidthStretch);

                    // Row 1: SBD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So Bao Danh:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    char bufSBDDt[32]; strcpy(bufSBDDt, sbdDet.c_str());
                    ImGui::InputText("##sbdDt", bufSBDDt, sizeof(bufSBDDt));

                    // Row 2: CCCD
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("So CCCD:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    char bufCCCDDt[32]; strcpy(bufCCCDDt, cccdDet.c_str());
                    ImGui::InputText("##cccdDt", bufCCCDDt, sizeof(bufCCCDDt));

                    // Row 3: Họ và Tên
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ho va Ten:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(260.0f);
                    char bufHoTenDt[64]; strcpy(bufHoTenDt, hoTenDet.c_str());
                    ImGui::InputText("##hoTenDt", bufHoTenDt, sizeof(bufHoTenDt));

                    // Row 4: Ngày sinh
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ngay Sinh (dd/mm/yyyy):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##dayDt", &dDet, 0, 0);
                    ImGui::SameLine(); ImGui::Text("/"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(50.0f);
                    ImGui::InputInt("##monthDt", &mDet, 0, 0);
                    ImGui::SameLine(); ImGui::Text("/"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputInt("##yearDt", &yDet, 0, 0);

                    // Row 5: Quê quán
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Que Quan (Tinh/TP):");
                    ImGui::TableSetColumnIndex(1);
                    RenderSearchableComboTinhThanh("##comboQueQuanDt", ql.getDanhSachTinhThanh(), queQuanDet);

                    // Row 6: Giới tính
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Gioi Tinh:");
                    ImGui::TableSetColumnIndex(1);
                    RenderComboGioiTinh("##comboGioiTinhDt", ql.getDanhSachGioiTinh(), gioiTinhDet);

                    // Row 7: Điểm Toán
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Toan (0.00 - 10.00):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##toanDt", &toanDet, 0.0f, 0.0f, "%.2f");

                    // Row 8: Điểm Lý
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Ly (0.00 - 10.00):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##lyDt", &lyDet, 0.0f, 0.0f, "%.2f");

                    // Row 9: Điểm Hóa
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Diem Hoa (0.00 - 10.00):");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(140.0f);
                    ImGui::InputFloat("##hoaDt", &hoaDet, 0.0f, 0.0f, "%.2f");

                    // Row 10: Nguyện vọng 1
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Nguyen Vong 1 (NV1):");
                    ImGui::TableSetColumnIndex(1);
                    RenderComboNganh("##comboNV1Dt", ql.getDanhSachDanhMucNganh(), nv1Det, nv2Det, nv3Det);

                    // Row 11: Nguyện vọng 2
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Nguyen Vong 2 (NV2):");
                    ImGui::TableSetColumnIndex(1);
                    RenderComboNganh("##comboNV2Dt", ql.getDanhSachDanhMucNganh(), nv2Det, nv1Det, nv3Det);

                    // Row 12: Nguyện vọng 3
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Nguyen Vong 3 (NV3):");
                    ImGui::TableSetColumnIndex(1);
                    RenderComboNganh("##comboNV3Dt", ql.getDanhSachDanhMucNganh(), nv3Det, nv1Det, nv2Det);

                    // Row 13: Kết quả DUT (nếu trúng tuyển)
                    if (sel.daTrungTuyen()) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "Trung Tuyen Nganh:");
                        ImGui::TableSetColumnIndex(1);
                        char bufTT[128];
                        sprintf(bufTT, "%s (%s)", ql.layTenNganh(sel.getNganhTrungTuyen()).c_str(), sel.getNganhTrungTuyen().c_str());
                        ImGui::InputText("##ttNganhDt", bufTT, sizeof(bufTT));

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Lop Sinh Hoat DUT:");
                        ImGui::TableSetColumnIndex(1);
                        char bufLSH[64]; strcpy(bufLSH, sel.getLopSinhHoat().c_str());
                        ImGui::InputText("##lshDt", bufLSH, sizeof(bufLSH));

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.75f, 1.0f), "Ma Sinh Vien DUT:");
                        ImGui::TableSetColumnIndex(1);
                        char bufMSV[32]; strcpy(bufMSV, sel.getMaSinhVien().c_str());
                        ImGui::InputText("##msvDt", bufMSV, sizeof(bufMSV));
                    }

                    ImGui::EndTable();
                }
                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::Separator();

                double tongDiemDt = sel.tinhTongDiem();
                bool lietDt = sel.kiemTraDiemLiet(ql.getDiemLiet());
                bool duSanDt = (!lietDt && tongDiemDt >= ql.getDiemSan());

                string kqStr;
                ImVec4 kqColor;
                if (sel.daTrungTuyen()) {
                    kqStr = "TRUNG TUYEN (Nguyen Vong " + to_string(sel.getNguyenVongTrungTuyen()) + ")";
                    kqColor = ImVec4(0.2f, 0.95f, 0.4f, 1.0f);
                } else if (lietDt) {
                    kqStr = "BI DIEM LIET";
                    kqColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
                } else if (!duSanDt) {
                    kqStr = "KHONG DAT DIEM SAN";
                    kqColor = ImVec4(1.0f, 0.65f, 0.25f, 1.0f);
                } else {
                    bool coNVDangXet = false;
                    const Vector<NguyenVong>& nvList = sel.getDsNguyenVong();
                    for (size_t k = 0; k < nvList.size(); ++k) {
                        if (nvList[k].trangThai == AdmissionConst::TT_NV_DANG_XET) {
                            coNVDangXet = true;
                            break;
                        }
                    }
                    if (coNVDangXet || nvList.empty()) {
                        kqStr = "DANG XET TUYEN (Dang cho quet)";
                        kqColor = ImVec4(0.35f, 0.95f, 0.55f, 1.0f);
                    } else {
                        kqStr = "KHONG TRUNG TUYEN";
                        kqColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                    }
                }

                ImGui::TextColored(kqColor, "Tong Diem: %.2f  |  Ket Qua: %s", 
                    tongDiemDt, kqStr.c_str());

                ImGui::Separator();
                if (ImGui::Button("Dong", ImVec2(100, 30))) {
                    showDetailModal = false;
                    ImGui::CloseCurrentPopup();
                }
            } else {
                showDetailModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: SỬA TỈNH THÀNH ====================
        if (showEditTinhThanhModal) {
            ImGui::OpenPopup("Cap Nhat Tinh / Thanh Pho");
        }
        if (ImGui::BeginPopupModal("Cap Nhat Tinh / Thanh Pho", &showEditTinhThanhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT THONG TIN TINH / THANH PHO (MA: %s)", editTinhThanhMa);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Tinh (PK):");
            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editMaTT", editTinhThanhMa, sizeof(editTinhThanhMa));
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Text("Ten Tinh / Thanh Pho*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editTenTT", editTinhThanhTen, sizeof(editTinhThanhTen));

            ImGui::Spacing();
            ImGui::Text("Loai Hinh (Tinh / TP truc thuoc TW):");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editLoaiTT", editTinhThanhLoai, sizeof(editTinhThanhLoai));

            if (!editTinhThanhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", editTinhThanhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Luu Thay Doi", ImVec2(130, 30))) {
                string ten = Utils::trim(editTinhThanhTen);
                string loai = Utils::trim(editTinhThanhLoai);
                if (ten.empty()) {
                    editTinhThanhError = "Ten tinh thanh khong duoc de trong!";
                } else {
                    TinhThanh ttCapNhat(editTinhThanhMa, ten, loai);
                    if (ql.capNhatTinhThanh(ttCapNhat)) {
                        showEditTinhThanhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat tinh / thanh pho thanh cong!";
                    } else {
                        editTinhThanhError = "Khong the cap nhat tinh thanh!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                showEditTinhThanhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: SỬA GIỚI TÍNH ====================
        if (showEditGioiTinhModal) {
            ImGui::OpenPopup("Cap Nhat Gioi Tinh");
        }
        if (ImGui::BeginPopupModal("Cap Nhat Gioi Tinh", &showEditGioiTinhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT THONG TIN GIOI TINH (MA: %s)", editGioiTinhMa);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Gioi Tinh (PK):");
            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editMaGT", editGioiTinhMa, sizeof(editGioiTinhMa));
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Text("Ten Hien Thi*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editTenGT", editGioiTinhTen, sizeof(editGioiTinhTen));

            ImGui::Spacing();
            ImGui::Text("Ghi Chu Mo Ta:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editDescGT", editGioiTinhDesc, sizeof(editGioiTinhDesc));

            if (!editGioiTinhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", editGioiTinhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Luu Thay Doi", ImVec2(130, 30))) {
                string ten = Utils::trim(editGioiTinhTen);
                string desc = Utils::trim(editGioiTinhDesc);
                if (ten.empty()) {
                    editGioiTinhError = "Ten gioi tinh khong duoc de trong!";
                } else {
                    GioiTinh gtCapNhat(editGioiTinhMa, ten, desc);
                    if (ql.capNhatGioiTinh(gtCapNhat)) {
                        showEditGioiTinhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat gioi tinh thanh cong!";
                    } else {
                        editGioiTinhError = "Khong the cap nhat gioi tinh!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo", ImVec2(100, 30))) {
                showEditGioiTinhModal = false;
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
