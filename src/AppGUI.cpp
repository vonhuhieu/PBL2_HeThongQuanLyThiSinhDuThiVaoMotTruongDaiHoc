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

// ------------------------------------------------------------------------------
// CÁC HÀM RENDER ICON BUTTONS VECTOR CHO CỘT THAO TÁC (XEM, SỬA, XÓA)
// Độc lập hoàn toàn với font chữ, vẽ trực tiếp bằng ImDrawList sắc nét ở mọi độ phân giải
// ------------------------------------------------------------------------------
static bool RenderIconButtonView(const char* id, ImVec2 size = ImVec2(26, 22), const char* tooltip = "Xem chi tiet ho so thi sinh") {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.50f, 0.44f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.65f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.40f, 0.35f, 1.0f));
    bool clicked = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    // Vẽ icon con mắt vector cách điệu lên button
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 iconColor = ImGui::GetColorU32(ImVec4(0.95f, 0.98f, 1.0f, 1.0f));

    float rX = 6.5f;
    float rY = 4.0f;
    // Đường cong mí mắt trên
    dl->AddBezierQuadratic(ImVec2(center.x - rX, center.y), ImVec2(center.x, center.y - rY * 1.4f), ImVec2(center.x + rX, center.y), iconColor, 1.5f);
    // Đường cong mí mắt dưới
    dl->AddBezierQuadratic(ImVec2(center.x - rX, center.y), ImVec2(center.x, center.y + rY * 1.4f), ImVec2(center.x + rX, center.y), iconColor, 1.5f);
    // Tròng mắt ở giữa
    dl->AddCircleFilled(center, 2.0f, iconColor);

    if (ImGui::IsItemHovered() && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

static bool RenderIconButtonEdit(const char* id, ImVec2 size = ImVec2(26, 22), const char* tooltip = "Sua thong tin ho so") {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.80f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.55f, 0.92f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.35f, 0.70f, 1.0f));
    bool clicked = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    // Vẽ icon chiếc bút chì vector nghiêng 45 độ
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 iconColor = ImGui::GetColorU32(ImVec4(0.95f, 0.98f, 1.0f, 1.0f));

    // Thân bút chì
    dl->AddLine(ImVec2(center.x - 3.5f, center.y + 3.5f), ImVec2(center.x + 4.0f, center.y - 4.0f), iconColor, 2.2f);
    // Ngòi bút chì
    dl->AddTriangleFilled(ImVec2(center.x - 3.5f, center.y + 3.5f),
                          ImVec2(center.x - 6.5f, center.y + 5.0f),
                          ImVec2(center.x - 4.5f, center.y + 6.5f), iconColor);
    // Đuôi tẩy
    dl->AddCircleFilled(ImVec2(center.x + 4.2f, center.y - 4.2f), 1.6f, iconColor);

    if (ImGui::IsItemHovered() && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

static bool RenderIconButtonDelete(const char* id, ImVec2 size = ImVec2(26, 22), const char* tooltip = "Xoa ho so thi sinh") {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.32f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.18f, 0.18f, 1.0f));
    bool clicked = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    // Vẽ icon thùng rác vector
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 iconColor = ImGui::GetColorU32(ImVec4(0.95f, 0.98f, 1.0f, 1.0f));

    // Nắp thùng rác
    dl->AddLine(ImVec2(center.x - 5.0f, center.y - 3.5f), ImVec2(center.x + 5.0f, center.y - 3.5f), iconColor, 1.4f);
    // Quai nắp
    dl->AddLine(ImVec2(center.x - 2.0f, center.y - 5.5f), ImVec2(center.x + 2.0f, center.y - 5.5f), iconColor, 1.4f);
    // Thân thùng rác (hình thang nhỏ dần xuống đáy)
    dl->AddLine(ImVec2(center.x - 4.0f, center.y - 2.0f), ImVec2(center.x - 3.0f, center.y + 5.5f), iconColor, 1.4f);
    dl->AddLine(ImVec2(center.x + 4.0f, center.y - 2.0f), ImVec2(center.x + 3.0f, center.y + 5.5f), iconColor, 1.4f);
    dl->AddLine(ImVec2(center.x - 3.0f, center.y + 5.5f), ImVec2(center.x + 3.0f, center.y + 5.5f), iconColor, 1.4f);
    // Nan thùng
    dl->AddLine(ImVec2(center.x - 1.2f, center.y - 1.0f), ImVec2(center.x - 1.0f, center.y + 4.0f), iconColor, 1.0f);
    dl->AddLine(ImVec2(center.x + 1.2f, center.y - 1.0f), ImVec2(center.x + 1.0f, center.y + 4.0f), iconColor, 1.0f);

    if (ImGui::IsItemHovered() && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

// ------------------------------------------------------------------------------
// CÁC HÀM CĂN CHỈNH NỘI DUNG CELL: CĂN GIỮA DỌC, CANH TRÁI CHO TEXT, CANH GIỮA CHO CÒN LẠI
// ------------------------------------------------------------------------------
static void AlignAndRenderCenter(int colIdx, const string& text, const ImVec4& color = ImVec4(0.88f, 0.90f, 0.94f, 1.0f), float minH = 26.0f) {
    ImGui::TableSetColumnIndex(colIdx);
    float availW = ImGui::GetContentRegionAvail().x;
    float textW = ImGui::CalcTextSize(text.c_str()).x;
    float textH = ImGui::GetTextLineHeight();

    float padY = (minH - textH) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }
    if (availW > textW) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - textW) * 0.5f);
    }
    ImGui::TextColored(color, "%s", text.c_str());
}

static void AlignAndRenderCenter(const string& text, const ImVec4& color = ImVec4(0.88f, 0.90f, 0.94f, 1.0f), float minH = 26.0f) {
    float availW = ImGui::GetContentRegionAvail().x;
    float textW = ImGui::CalcTextSize(text.c_str()).x;
    float textH = ImGui::GetTextLineHeight();

    float padY = (minH - textH) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }
    if (availW > textW) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - textW) * 0.5f);
    }
    ImGui::TextColored(color, "%s", text.c_str());
}

static void AlignAndRenderTextLeft(int colIdx, const string& text, const ImVec4& color = ImVec4(0.88f, 0.90f, 0.94f, 1.0f), float minH = 26.0f) {
    ImGui::TableSetColumnIndex(colIdx);
    float availW = ImGui::GetContentRegionAvail().x;
    float wrapW = (availW > 20.0f) ? availW : 120.0f;
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrapW);

    float padY = (minH - textSize.y) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }
    if (textSize.y <= ImGui::GetTextLineHeight() * 1.35f) {
        ImGui::TextColored(color, "%s", text.c_str());
    } else {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapW);
        ImGui::TextColored(color, "%s", text.c_str());
        ImGui::PopTextWrapPos();
    }
}

static void AlignAndRenderTextLeft(const string& text, const ImVec4& color = ImVec4(0.88f, 0.90f, 0.94f, 1.0f), float minH = 26.0f) {
    float availW = ImGui::GetContentRegionAvail().x;
    float wrapW = (availW > 20.0f) ? availW : 120.0f;
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrapW);

    float padY = (minH - textSize.y) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }
    if (textSize.y <= ImGui::GetTextLineHeight() * 1.35f) {
        ImGui::TextColored(color, "%s", text.c_str());
    } else {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapW);
        ImGui::TextColored(color, "%s", text.c_str());
        ImGui::PopTextWrapPos();
    }
}

static void AlignAndRenderTextRight(int colIdx, const string& text, const ImVec4& color = ImVec4(0.88f, 0.90f, 0.94f, 1.0f), float minH = 26.0f) {
    ImGui::TableSetColumnIndex(colIdx);
    float availW = ImGui::GetContentRegionAvail().x;
    float wrapW = (availW > 20.0f) ? availW : 120.0f;
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrapW);

    float padY = (minH - textSize.y) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }
    if (textSize.y <= ImGui::GetTextLineHeight() * 1.35f) {
        if (availW > textSize.x) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - textSize.x));
        }
        ImGui::TextColored(color, "%s", text.c_str());
    } else {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapW);
        ImGui::TextColored(color, "%s", text.c_str());
        ImGui::PopTextWrapPos();
    }
}

// ------------------------------------------------------------------------------
// KIẾN TRÚC BẢNG DÙNG CHUNG (COMMON TABLE ARCHITECTURE)
// Phù hợp chuẩn thiết kế Frontend chuyên nghiệp (tham khảo sdt-baocaodieuhanh)
// ------------------------------------------------------------------------------
struct PaginationState {
    int currentPage = 1;
    int pageSizeIndex = 0; // 0: 10, 1: 25, 2: 50, 3: 100
    int totalRecords = 0;
    int totalPages = 1;
    int targetJumpPage = 1;
    int sortCol = 0;
    bool sortAsc = true;

    int getPageSize() const {
        static const int SIZES[] = { 10, 25, 50, 100 };
        if (pageSizeIndex < 0 || pageSizeIndex > 3) return 10;
        return SIZES[pageSizeIndex];
    }
    int getOffset() const {
        return (currentPage - 1) * getPageSize();
    }
    void updateTotal(int total) {
        totalRecords = total;
        int pSize = getPageSize();
        totalPages = (totalRecords + pSize - 1) / pSize;
        if (totalPages < 1) totalPages = 1;
        if (currentPage > totalPages) currentPage = totalPages;
        if (currentPage < 1) currentPage = 1;
    }
    void update(int total) { updateTotal(total); }
    string getRangeDisplay() const {
        if (totalRecords <= 0) return "Hien thi 0-0 tren tong 0 ban ghi";
        int start = getOffset() + 1;
        int end = getOffset() + getPageSize();
        if (end > totalRecords) end = totalRecords;
        return "Hien thi " + to_string(start) + "-" + to_string(end) + " tren tong " + to_string(totalRecords) + " ban ghi";
    }
};

struct ActionToolbarConfig {
    bool showAdd = true;
    bool showExport = true;
    bool showImport = true;
    string addLabel = "[+] Them moi";
    string exportLabel = "[ Xuat Excel ]";
    string importLabel = "[+] Import du lieu mau";
    string addTooltip = "Them moi ban ghi vao he thong";
    string exportTooltip = "Xuat toan bo du lieu ra file Excel (.xls) chuan dinh dang";
    string importTooltip = "Nap du lieu mau chuan DUT vao he thong";
};

static void RenderCommonActionToolbar(const string& screenId,
                                      const ActionToolbarConfig& cfg,
                                      bool& outAddClicked,
                                      bool& outExportClicked,
                                      bool& outImportClicked) {
    float wAdd = 125.0f;
    float wExport = 135.0f;
    float wImport = 185.0f;
    float spacing = 8.0f;
    float totalW = 0.0f;

    if (cfg.showAdd) totalW += wAdd + spacing;
    if (cfg.showExport) totalW += wExport + spacing;
    if (cfg.showImport) totalW += wImport + spacing;
    if (totalW > 0.0f) totalW -= spacing;

    float availW = ImGui::GetContentRegionAvail().x;
    if (availW > totalW) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - totalW));
    }

    bool isFirst = true;
    if (cfg.showAdd) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.68f, 0.42f, 1.0f));
        if (ImGui::Button((cfg.addLabel + "##" + screenId).c_str(), ImVec2(wAdd, 28))) {
            outAddClicked = true;
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", cfg.addTooltip.c_str());
        isFirst = false;
    }
    if (cfg.showExport) {
        if (!isFirst) ImGui::SameLine(0, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.62f, 0.88f, 1.0f));
        if (ImGui::Button((cfg.exportLabel + "##" + screenId).c_str(), ImVec2(wExport, 28))) {
            outExportClicked = true;
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", cfg.exportTooltip.c_str());
        isFirst = false;
    }
    if (cfg.showImport) {
        if (!isFirst) ImGui::SameLine(0, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.35f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.45f, 0.18f, 1.0f));
        if (ImGui::Button((cfg.importLabel + "##" + screenId).c_str(), ImVec2(wImport, 28))) {
            outImportClicked = true;
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", cfg.importTooltip.c_str());
    }
}

static void RenderCommonActionCell(const string& rowKey,
                                   bool& outViewClicked,
                                   bool& outEditClicked,
                                   bool& outDeleteClicked,
                                   bool enableView = true,
                                   bool enableEdit = true,
                                   bool enableDelete = true,
                                   const char* viewTip = nullptr,
                                   const char* editTip = nullptr,
                                   const char* delTip = nullptr,
                                   float minH = 26.0f) {
    float availW = ImGui::GetContentRegionAvail().x;
    float contentW = (enableView ? 26.0f : 0.0f) + (enableEdit ? 26.0f : 0.0f) + (enableDelete ? 26.0f : 0.0f);
    int count = (enableView ? 1 : 0) + (enableEdit ? 1 : 0) + (enableDelete ? 1 : 0);
    if (count > 1) contentW += (count - 1) * 4.0f;

    if (availW > contentW) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - contentW) * 0.5f);
    }
    float padY = (minH - 22.0f) * 0.5f;
    if (padY > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
    }

    bool isFirst = true;
    if (enableView) {
        if (RenderIconButtonView(("##btnV_" + rowKey).c_str(), ImVec2(26, 22), viewTip)) {
            outViewClicked = true;
        }
        isFirst = false;
    }
    if (enableEdit) {
        if (!isFirst) ImGui::SameLine(0, 4);
        if (RenderIconButtonEdit(("##btnE_" + rowKey).c_str(), ImVec2(26, 22), editTip)) {
            outEditClicked = true;
        }
        isFirst = false;
    }
    if (enableDelete) {
        if (!isFirst) ImGui::SameLine(0, 4);
        if (RenderIconButtonDelete(("##btnD_" + rowKey).c_str(), ImVec2(26, 22), delTip)) {
            outDeleteClicked = true;
        }
    }
}

static void RenderCommonEmptyRow(int colSpan, const char* msg = "Khong tim thay du lieu phu hop!") {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    float availW = ImGui::GetWindowWidth() - 50.0f;
    float textW = ImGui::CalcTextSize(msg).x;
    if (availW > textW) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - textW) * 0.5f);
    }
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", msg);
}

static void RenderCommonPaginationBar(const string& barId, PaginationState& state) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.14f, 0.19f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::BeginChild(("PaginationBar_" + barId).c_str(), ImVec2(0, 36), false, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        ImGui::TextColored(ImVec4(0.65f, 0.75f, 0.85f, 1.0f), "So dong/trang:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        static const char* PAGE_SIZE_LABELS[] = { "10 / trang", "25 / trang", "50 / trang", "100 / trang" };
        if (ImGui::Combo(("##pSize_" + barId).c_str(), &state.pageSizeIndex, PAGE_SIZE_LABELS, 4)) {
            state.updateTotal(state.totalRecords);
        }

        ImGui::SameLine(0, 15);
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "|  %s", state.getRangeDisplay().c_str());

        // Phía phải: 5 nút điều hướng phân trang
        float navW = 280.0f;
        float availW = ImGui::GetContentRegionAvail().x;
        if (availW > navW) {
            ImGui::SameLine(ImGui::GetWindowWidth() - navW - 20.0f);
        }

        bool disablePrev = (state.currentPage <= 1);
        bool disableNext = (state.currentPage >= state.totalPages);

        if (disablePrev) ImGui::BeginDisabled();
        if (ImGui::Button(("|<<##first_" + barId).c_str(), ImVec2(32, 24))) {
            state.currentPage = 1;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ve trang dau tien (Trang 1)");
        ImGui::SameLine(0, 4);
        if (ImGui::Button(("<##prev_" + barId).c_str(), ImVec2(32, 24))) {
            state.currentPage--;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Trang truoc");
        if (disablePrev) ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        char pageLabel[32];
        sprintf(pageLabel, "Trang %d/%d##%s", state.currentPage, state.totalPages, barId.c_str());
        if (ImGui::Button(pageLabel, ImVec2(100, 24))) {
            state.targetJumpPage = state.currentPage;
            ImGui::OpenPopup(("JumpPagePopup_" + barId).c_str());
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click de nhap so trang muon chuyen toi");

        if (ImGui::BeginPopup(("JumpPagePopup_" + barId).c_str())) {
            ImGui::Text("Chuyen toi trang (1 - %d):", state.totalPages);
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt(("##jumpVal_" + barId).c_str(), &state.targetJumpPage);
            if (state.targetJumpPage < 1) state.targetJumpPage = 1;
            if (state.targetJumpPage > state.totalPages) state.targetJumpPage = state.totalPages;
            ImGui::SameLine();
            if (ImGui::Button("Den")) {
                state.currentPage = state.targetJumpPage;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine(0, 4);
        if (disableNext) ImGui::BeginDisabled();
        if (ImGui::Button((">##next_" + barId).c_str(), ImVec2(32, 24))) {
            state.currentPage++;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Trang tiep theo");
        ImGui::SameLine(0, 4);
        if (ImGui::Button((">>|##last_" + barId).c_str(), ImVec2(32, 24))) {
            state.currentPage = state.totalPages;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Den trang cuoi cung");
        if (disableNext) ImGui::EndDisabled();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
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

    auto populateEditThiSinh = [&](const ThiSinh& ts) {
        selectedSBD = ts.getSoBaoDanh();
        strncpy(bufSBD, ts.getSoBaoDanh().c_str(), sizeof(bufSBD) - 1);
        strncpy(bufCCCD, ts.getSoCCCD().c_str(), sizeof(bufCCCD) - 1);
        strncpy(bufHoTen, ts.getHoTen().c_str(), sizeof(bufHoTen) - 1);
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
        formQueQuanMa = ts.getQueQuan();
        formGioiTinhMa = ts.getGioiTinh();
        formToan = (float)ts.getDiemToan();
        formLy = (float)ts.getDiemLy();
        formHoa = (float)ts.getDiemHoa();
        formErrorMsg = "";
        formNV1 = ""; formNV2 = ""; formNV3 = "";
        for (size_t nvIdx = 0; nvIdx < ts.getDsNguyenVong().size(); ++nvIdx) {
            if (ts.getDsNguyenVong()[nvIdx].thuTuNV == 1) formNV1 = ts.getDsNguyenVong()[nvIdx].maNganh;
            else if (ts.getDsNguyenVong()[nvIdx].thuTuNV == 2) formNV2 = ts.getDsNguyenVong()[nvIdx].maNganh;
            else if (ts.getDsNguyenVong()[nvIdx].thuTuNV == 3) formNV3 = ts.getDsNguyenVong()[nvIdx].maNganh;
        }
        showEditModal = true;
    };

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

    // ==================== STATE CÁC BẢNG DÙNG CHUNG (COMMON TABLE ARCHITECTURE) ====================
    // Tab 4: Danh mục hệ thống
    static char searchKhoa[64] = "";
    static PaginationState pagKhoa;
    static bool showAddKhoaModal = false;
    static char addKhoaMa[16] = "";
    static char addKhoaTen[128] = "";
    static char addKhoaMaDUT[16] = "";
    static string addKhoaError = "";
    static bool showEditKhoaModal = false;
    static char editKhoaMa[16] = "";
    static char editKhoaTen[128] = "";
    static char editKhoaMaDUT[16] = "";
    static string editKhoaError = "";
    static bool showViewKhoaModal = false;
    static Khoa viewKhoaData;

    static char searchNganh[64] = "";
    static string filterNganhKhoa = "ALL";
    static PaginationState pagNganh;
    static bool showAddNganhModal = false;
    static char addNganhMa[32] = "";
    static char addNganhTen[150] = "";
    static string addNganhKhoa = "CNTT";
    static char addNganhTienTo[32] = "";
    static string addNganhError = "";
    static bool showEditNganhModal = false;
    static char editNganhMa[32] = "";
    static char editNganhTen[150] = "";
    static string editNganhKhoa = "CNTT";
    static char editNganhTienTo[32] = "";
    static string editNganhError = "";
    static bool showViewNganhModal = false;
    static DanhMucNganh viewNganhData;

    static char searchNTK[64] = "";
    static string filterNTKKhoa = "ALL";
    static PaginationState pagNTK;
    static bool showAddNTKModal = false;
    static int addNTKNam = 2026;
    static string addNTKMa = "7480201B";
    static int addNTKChiTieu = 50;
    static float addNTKDiemSan = 15.0f;
    static string addNTKError = "";
    static bool showEditNTKModal = false;
    static int editNTKNam = 2026;
    static string editNTKMa = "";
    static int editNTKChiTieu = 50;
    static float editNTKDiemSan = 15.0f;
    static string editNTKError = "";
    static bool showViewNTKModal = false;
    static NganhTheoKhoaHoc viewNTKData;

    static char searchTinhThanh[64] = "";
    static PaginationState pagTinhThanh;
    static bool showAddTinhThanhModal = false;
    static char addTinhThanhMa[16] = "";
    static char addTinhThanhTen[128] = "";
    static char addTinhThanhLoai[64] = "Tinh";
    static string addTinhThanhError = "";
    static bool showViewTinhThanhModal = false;
    static TinhThanh viewTinhThanhData;

    static char searchGioiTinh[64] = "";
    static PaginationState pagGioiTinh;
    static bool showAddGioiTinhModal = false;
    static char addGioiTinhMa[16] = "";
    static char addGioiTinhTen[64] = "";
    static char addGioiTinhDesc[128] = "";
    static string addGioiTinhError = "";
    static bool showViewGioiTinhModal = false;
    static GioiTinh viewGioiTinhData;

    // Tab 3: Xét tuyển & Thống kê DUT
    static char searchDiemChuan[64] = "";
    static string filterDiemChuanKhoa = "ALL";
    static PaginationState pagDiemChuan;

    static char searchTanSV[64] = "";
    static string filterTanSVKhoa = "ALL";
    static string filterTanSVNganh = "ALL";
    static PaginationState pagTanSV;

    // Tab 2: 10 Bảng CSDL Quan hệ 3NF
    static char searchCSDL[10][64] = { "" };
    static PaginationState pagCSDL[10];

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

                float rightBtnWidth = 465.0f;
                if (ImGui::GetWindowWidth() > 950.0f) {
                    ImGui::SameLine(ImGui::GetWindowWidth() - rightBtnWidth);
                } else {
                    ImGui::SameLine();
                }
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.34f, 1.0f));
                if (ImGui::Button("[+] Them moi##ts", ImVec2(125, 28))) {
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
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Them moi ho so thi sinh vao he thong");
                }

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 1.0f));
                if (ImGui::Button("[ Xuat Excel ]##ts", ImVec2(135, 28))) {
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

                // NÚT IMPORT DỮ LIỆU MẪU (100 THÍ SINH)
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.35f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.45f, 0.18f, 1.0f));
                if (ImGui::Button("[+] Import du lieu mau##ts", ImVec2(185, 28))) {
                    bool ok = ql.importDuLieuMau100("sql/05_seed_sample_data.sql");
                    showToast = true;
                    if (ok) {
                        currentPage = 1;
                        selectedRow = -1;
                        selectedSBD = "";
                        toastMsg = "Da import thanh cong 100 thi sinh mau xet tuyen CNTT Nhat (K26)!";
                    } else {
                        toastMsg = "Loi: Khong the import du lieu mau tu file sql/05_seed_sample_data.sql!";
                    }
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Nap 100 ho so thi sinh mau chuan xet tuyen Nganh 7480201A (CNTT - Ngoai ngu Nhat) vao he thong");
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
                                             ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | 
                                             ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
                if (needScrollY) tableFlags |= ImGuiTableFlags_ScrollY;

                if (ImGui::BeginTable("CandidatesTable_v3", 17, tableFlags, ImVec2(0, appliedTableHeight))) {
                    ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 34.0f);
                    ImGui::TableSetupColumn("SBD (PK)", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableSetupColumn("So CCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 108.0f);
                    ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch, 1.4f);
                    ImGui::TableSetupColumn("Ngay Sinh", ImGuiTableColumnFlags_WidthFixed, 86.0f);
                    ImGui::TableSetupColumn("Que Quan", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Gioi Tinh", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                    ImGui::TableSetupColumn("Toan", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                    ImGui::TableSetupColumn("Ly", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                    ImGui::TableSetupColumn("Hoa", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                    ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 58.0f);
                    ImGui::TableSetupColumn("Ket Qua", ImGuiTableColumnFlags_WidthStretch, 1.1f);
                    ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthStretch, 0.9f);
                    ImGui::TableSetupColumn("Nganh Trung Tuyen", ImGuiTableColumnFlags_WidthStretch, 1.3f);
                    ImGui::TableSetupColumn("Lop Sinh Hoat", ImGuiTableColumnFlags_WidthFixed, 92.0f);
                    ImGui::TableSetupColumn("Ma SV DUT", ImGuiTableColumnFlags_WidthFixed, 88.0f);
                    ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 102.0f);

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

                    float rowMinH = 42.0f;

                    for (size_t i = 0; i < pageData.size(); ++i) {
                        const ThiSinh& ts = pageData[i];
                        string tenQue = ql.layTenTinhThanh(ts.getQueQuan());
                        string tenGT  = ql.layTenGioiTinh(ts.getGioiTinh());

                        // Thiết lập chiều cao tối thiểu cho từng hàng để căn giữa dọc chuẩn mực
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, rowMinH);

                        int stt = (currentPage - 1) * pageSize + (int)i + 1;

                        // CỘT 0: STT (CĂN GIỮA)
                        AlignAndRenderCenter(0, to_string(stt), ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 1: SBD (PK) (CĂN GIỮA)
                        AlignAndRenderCenter(1, ts.getSoBaoDanh(), ImVec4(0.4f, 0.8f, 1.0f, 1.0f), rowMinH);

                        // CỘT 2: SỐ CCCD (FK) (CĂN GIỮA)
                        AlignAndRenderCenter(2, ts.getSoCCCD(), ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 3: HỌ VÀ TÊN (TEXT => CANH TRÁI)
                        AlignAndRenderTextLeft(3, ts.getHoTen(), ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 4: NGÀY SINH (CĂN GIỮA)
                        AlignAndRenderCenter(4, ts.getNgaySinh(), ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 5: QUÊ QUÁN (TEXT => CANH TRÁI)
                        AlignAndRenderTextLeft(5, tenQue, ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 6: GIỚI TÍNH (CĂN GIỮA)
                        AlignAndRenderCenter(6, tenGT, ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        // CỘT 7-10: ĐIỂM SỐ (CĂN GIỮA)
                        char bufScore[16];
                        sprintf(bufScore, "%.2f", ts.getDiemToan());
                        AlignAndRenderCenter(7, bufScore, ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        sprintf(bufScore, "%.2f", ts.getDiemLy());
                        AlignAndRenderCenter(8, bufScore, ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        sprintf(bufScore, "%.2f", ts.getDiemHoa());
                        AlignAndRenderCenter(9, bufScore, ImVec4(0.92f, 0.94f, 0.96f, 1.0f), rowMinH);

                        sprintf(bufScore, "%.2f", ts.tinhTongDiem());
                        AlignAndRenderCenter(10, bufScore, ImVec4(1.0f, 0.9f, 0.3f, 1.0f), rowMinH);

                        // CỘT 11: KẾT QUẢ XÉT TUYỂN (TEXT => CANH TRÁI)
                        string ketQuaStr;
                        ImVec4 ketQuaColor;
                        if (ts.daTrungTuyen()) {
                            ketQuaStr = "TRUNG TUYEN (NV" + to_string(ts.getNguyenVongTrungTuyen()) + ")";
                            ketQuaColor = ImVec4(0.2f, 0.95f, 0.4f, 1.0f);
                        } else if (ts.kiemTraDiemLiet(ql.getDiemLiet())) {
                            ketQuaStr = AdmissionConst::KET_QUA_DIEM_LIET;
                            ketQuaColor = ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
                        } else if (ts.tinhTongDiem() < ql.getDiemSan()) {
                            ketQuaStr = AdmissionConst::KET_QUA_KHONG_DAT_SAN;
                            ketQuaColor = ImVec4(1.0f, 0.65f, 0.25f, 1.0f);
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
                                ketQuaStr = AdmissionConst::KET_QUA_DANG_XET;
                                ketQuaColor = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
                            } else {
                                ketQuaStr = AdmissionConst::KET_QUA_KHONG_DAT;
                                ketQuaColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                            }
                        }
                        AlignAndRenderTextLeft(11, ketQuaStr, ketQuaColor, rowMinH);

                        // CỘT 12: KHOA (TEXT => CANH TRÁI)
                        string khoaStr = "-";
                        ImVec4 khoaColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                        if (ts.daTrungTuyen() && !ts.getNganhTrungTuyen().empty()) {
                            string mk = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
                            khoaStr = ql.layTenKhoa(mk);
                            khoaColor = ImVec4(0.9f, 0.85f, 0.4f, 1.0f);
                        }
                        AlignAndRenderTextLeft(12, khoaStr, khoaColor, rowMinH);

                        // CỘT 13: NGÀNH TRÚNG TUYỂN (TEXT => CANH TRÁI)
                        string nganhStr = "-";
                        ImVec4 nganhColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                        if (!ts.getNganhTrungTuyen().empty()) {
                            nganhStr = ql.layTenNganh(ts.getNganhTrungTuyen());
                            nganhColor = ImVec4(0.4f, 0.85f, 1.0f, 1.0f);
                        }
                        AlignAndRenderTextLeft(13, nganhStr, nganhColor, rowMinH);

                        // CỘT 14: LỚP SINH HOẠT (CĂN GIỮA)
                        string lopStr = ts.getLopSinhHoat().empty() ? "-" : ts.getLopSinhHoat();
                        ImVec4 lopColor = ts.getLopSinhHoat().empty() ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
                        AlignAndRenderCenter(14, lopStr, lopColor, rowMinH);

                        // CỘT 15: MÃ SINH VIÊN DUT (CĂN GIỮA)
                        string msvStr = ts.getMaSinhVien().empty() ? "-" : ts.getMaSinhVien();
                        ImVec4 msvColor = ts.getMaSinhVien().empty() ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) : ImVec4(0.35f, 0.95f, 0.75f, 1.0f);
                        AlignAndRenderCenter(15, msvStr, msvColor, rowMinH);

                        // CỘT 16: THAO TÁC (3 ICON BUTTONS CĂN GIỮA DỌC VÀ NGANG CHUẨN COMMON TABLE)
                        ImGui::TableSetColumnIndex(16);
                        bool vClk = false, eClk = false, dClk = false;
                        RenderCommonActionCell(ts.getSoBaoDanh(), vClk, eClk, dClk, true, true, true, "Xem chi tiet ho so thi sinh", "Sua thong tin ho so", "Xoa ho so thi sinh", rowMinH);
                        if (vClk) {
                            selectedSBD = ts.getSoBaoDanh();
                            showDetailModal = true;
                        }
                        if (eClk) {
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
                        if (dClk) {
                            selectedRow = (int)i;
                            selectedSBD = ts.getSoBaoDanh();
                            showDeleteModal = true;
                        }
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

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL0", "Tim theo CCCD, Ho ten, Que quan...", searchCSDL[0], sizeof(searchCSDL[0]));

                        ImGui::SameLine();
                        bool addNguoi = false, expNguoi = false, impNguoi = false;
                        ActionToolbarConfig cfgNguoi;
                        RenderCommonActionToolbar("csdl_nguoi", cfgNguoi, addNguoi, expNguoi, impNguoi);
                        if (addNguoi) {
                            showAddModal = true;
                        }
                        if (expNguoi) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("soCCCD (PK)"); hd.push_back("hoTen");
                            hd.push_back("ngaySinh"); hd.push_back("queQuan (FK)"); hd.push_back("gioiTinh (FK)");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(140); wd.push_back(200);
                            wd.push_back(110); wd.push_back(160); wd.push_back(100);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                if (searchCSDL[0][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[0]);
                                    if (Utils::toLower(ts.getSoCCCD()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                        Utils::toLower(ql.layTenTinhThanh(ts.getQueQuan())).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(ts.getSoCCCD());
                                r.push_back(ts.getHoTen());
                                r.push_back(ts.getNgaySinh());
                                r.push_back(ql.layTenTinhThanh(ts.getQueQuan()) + " [" + ts.getQueQuan() + "]");
                                r.push_back(ql.layTenGioiTinh(ts.getGioiTinh()) + " [" + ts.getGioiTinh() + "]");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_Nguoi.xls", "BANG THUC THE CHA: NGUOI", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang Nguoi Excel thanh cong!";
                        }
                        if (impNguoi) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang Nguoi tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredNguoi;
                        for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSach()[i];
                            if (searchCSDL[0][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[0]);
                                if (Utils::toLower(ts.getSoCCCD()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                    Utils::toLower(ql.layTenTinhThanh(ts.getQueQuan())).find(kw) == string::npos) continue;
                            }
                            filteredNguoi.push_back(i);
                        }
                        pagCSDL[0].update((int)filteredNguoi.size());
                        int offsetNguoi = pagCSDL[0].getOffset();
                        int endNguoi = (offsetNguoi + pagCSDL[0].getPageSize() < (int)filteredNguoi.size()) ? 
                                       (offsetNguoi + pagCSDL[0].getPageSize()) : (int)filteredNguoi.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblNguoiRaw", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("soCCCD (PK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("hoTen", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("ngaySinh", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("queQuan (FK)", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("gioiTinh (FK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredNguoi.empty()) {
                                RenderCommonEmptyRow(7);
                            } else {
                                for (int idx = offsetNguoi; idx < endNguoi; ++idx) {
                                    size_t realIdx = filteredNguoi[idx];
                                    const ThiSinh& ts = ql.getDanhSach()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_ng_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: soCCCD (center)
                                    AlignAndRenderCenter(1, ts.getSoCCCD(), ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 2: hoTen (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, ts.getHoTen());
                                    // Col 3: ngaySinh (center)
                                    AlignAndRenderCenter(3, ts.getNgaySinh());
                                    // Col 4: queQuan (LEFT ALIGN)
                                    AlignAndRenderTextLeft(4, ql.layTenTinhThanh(ts.getQueQuan()) + " [" + ts.getQueQuan() + "]");
                                    // Col 5: gioiTinh (center)
                                    AlignAndRenderCenter(5, ql.layTenGioiTinh(ts.getGioiTinh()) + " [" + ts.getGioiTinh() + "]");

                                    // Col 6: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(6);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDetailModal = true;
                                    }
                                    if (onEdit) {
                                        populateEditThiSinh(ts);
                                    }
                                    if (onDelete) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDeleteModal = true;
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL0", pagCSDL[0]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.2: BẢNG THỰC THỂ CON THÍ SINH
                    if (ImGui::BeginTabItem("  Bang 2: THI SINH (MySQL: thisinh)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL1", "Tim theo SBD, CCCD, Ho ten, Lop...", searchCSDL[1], sizeof(searchCSDL[1]));

                        ImGui::SameLine();
                        bool addTS = false, expTS = false, impTS = false;
                        ActionToolbarConfig cfgTS;
                        RenderCommonActionToolbar("csdl_thisinh", cfgTS, addTS, expTS, impTS);
                        if (addTS) {
                            showAddModal = true;
                        }
                        if (expTS) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("soBaoDanh (PK)"); hd.push_back("soCCCD (FK)");
                            hd.push_back("namTuyenSinh"); hd.push_back("diemToan"); hd.push_back("diemLy");
                            hd.push_back("diemHoa"); hd.push_back("maSinhVien"); hd.push_back("lopSinhHoat");
                            hd.push_back("nganhTrungTuyen"); hd.push_back("nguyenVongTrungTuyen");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(105); wd.push_back(120);
                            wd.push_back(95); wd.push_back(75); wd.push_back(75);
                            wd.push_back(75); wd.push_back(115); wd.push_back(125);
                            wd.push_back(115); wd.push_back(60);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                if (searchCSDL[1][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[1]);
                                    if (Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getSoCCCD()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getMaSinhVien()).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(ts.getSoBaoDanh());
                                r.push_back(ts.getSoCCCD());
                                r.push_back(to_string(ts.getNamTuyenSinh()));
                                char b1[32], b2[32], b3[32];
                                sprintf(b1, "%.2f", ts.getDiemToan()); r.push_back(b1);
                                sprintf(b2, "%.2f", ts.getDiemLy()); r.push_back(b2);
                                sprintf(b3, "%.2f", ts.getDiemHoa()); r.push_back(b3);
                                r.push_back(ts.getMaSinhVien().empty() ? "NULL" : ts.getMaSinhVien());
                                r.push_back(ts.getLopSinhHoat().empty() ? "NULL" : ts.getLopSinhHoat());
                                r.push_back(ts.getNganhTrungTuyen().empty() ? "NULL" : ts.getNganhTrungTuyen());
                                r.push_back(to_string(ts.getNguyenVongTrungTuyen()));
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_ThiSinh.xls", "BANG THUC THE CON: THI SINH", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang ThiSinh Excel thanh cong!";
                        }
                        if (impTS) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang ThiSinh tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredTS;
                        for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSach()[i];
                            if (searchCSDL[1][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[1]);
                                if (Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getSoCCCD()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getMaSinhVien()).find(kw) == string::npos) continue;
                            }
                            filteredTS.push_back(i);
                        }
                        pagCSDL[1].update((int)filteredTS.size());
                        int offsetTS = pagCSDL[1].getOffset();
                        int endTS = (offsetTS + pagCSDL[1].getPageSize() < (int)filteredTS.size()) ? 
                                    (offsetTS + pagCSDL[1].getPageSize()) : (int)filteredTS.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblThiSinhRaw", 12, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("soBaoDanh (PK)", ImGuiTableColumnFlags_WidthFixed, 105);
                            ImGui::TableSetupColumn("soCCCD (FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("namTuyenSinh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("diemToan", ImGuiTableColumnFlags_WidthFixed, 70);
                            ImGui::TableSetupColumn("diemLy", ImGuiTableColumnFlags_WidthFixed, 70);
                            ImGui::TableSetupColumn("diemHoa", ImGuiTableColumnFlags_WidthFixed, 70);
                            ImGui::TableSetupColumn("maSinhVien", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("lopSinhHoat", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("nganhTrungTuyen", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("nguyenVongTrungTuyen", ImGuiTableColumnFlags_WidthFixed, 50);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredTS.empty()) {
                                RenderCommonEmptyRow(12);
                            } else {
                                for (int idx = offsetTS; idx < endTS; ++idx) {
                                    size_t realIdx = filteredTS[idx];
                                    const ThiSinh& ts = ql.getDanhSach()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_tsr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: soBaoDanh (center)
                                    AlignAndRenderCenter(1, ts.getSoBaoDanh(), ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 2: soCCCD (center)
                                    AlignAndRenderCenter(2, ts.getSoCCCD(), ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 3: namTuyenSinh (center)
                                    AlignAndRenderCenter(3, to_string(ts.getNamTuyenSinh()));
                                    // Col 4-6: Diem (center)
                                    char bToan[32], bLy[32], bHoa[32];
                                    sprintf(bToan, "%.2f", ts.getDiemToan());
                                    sprintf(bLy, "%.2f", ts.getDiemLy());
                                    sprintf(bHoa, "%.2f", ts.getDiemHoa());
                                    AlignAndRenderCenter(4, bToan);
                                    AlignAndRenderCenter(5, bLy);
                                    AlignAndRenderCenter(6, bHoa);
                                    // Col 7: maSinhVien (center)
                                    AlignAndRenderCenter(7, ts.getMaSinhVien().empty() ? "NULL" : ts.getMaSinhVien(), ImVec4(0.2f, 0.95f, 0.4f, 1.0f));
                                    // Col 8: lopSinhHoat (center)
                                    AlignAndRenderCenter(8, ts.getLopSinhHoat().empty() ? "NULL" : ts.getLopSinhHoat(), ImVec4(0.35f, 0.85f, 1.0f, 1.0f));
                                    // Col 9: nganhTrungTuyen (center)
                                    AlignAndRenderCenter(9, ts.getNganhTrungTuyen().empty() ? "NULL" : ts.getNganhTrungTuyen());
                                    // Col 10: nguyenVongTrungTuyen (center)
                                    AlignAndRenderCenter(10, to_string(ts.getNguyenVongTrungTuyen()));

                                    // Col 11: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(11);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDetailModal = true;
                                    }
                                    if (onEdit) {
                                        populateEditThiSinh(ts);
                                    }
                                    if (onDelete) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDeleteModal = true;
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL1", pagCSDL[1]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.3: BẢNG NGUYỆN VỌNG ĐĂNG KÝ
                    if (ImGui::BeginTabItem("  Bang 3: NGUYEN VONG (MySQL: nguyen_vong)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL2", "Tim theo SBD, Ma nganh, Ten nganh...", searchCSDL[2], sizeof(searchCSDL[2]));

                        ImGui::SameLine();
                        bool addNV = false, expNV = false, impNV = false;
                        ActionToolbarConfig cfgNV;
                        RenderCommonActionToolbar("csdl_nv", cfgNV, addNV, expNV, impNV);
                        if (addNV) {
                            showAddModal = true;
                        }
                        if (expNV) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("soBaoDanh (FK)"); hd.push_back("thuTuNV");
                            hd.push_back("maNganh (FK)"); hd.push_back("Ten Nganh Dang Ky"); hd.push_back("trangThai");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(120); wd.push_back(80);
                            wd.push_back(110); wd.push_back(240); wd.push_back(160);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                for (size_t j = 0; j < ts.getDsNguyenVong().size(); ++j) {
                                    const NguyenVong& nv = ts.getDsNguyenVong()[j];
                                    string tenN = ql.layTenNganh(nv.maNganh);
                                    if (searchCSDL[2][0] != '\0') {
                                        string kw = Utils::toLower(searchCSDL[2]);
                                        if (Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                            Utils::toLower(nv.maNganh).find(kw) == string::npos &&
                                            Utils::toLower(tenN).find(kw) == string::npos) continue;
                                    }
                                    sttExp++;
                                    Vector<string> r;
                                    r.push_back(to_string(sttExp));
                                    r.push_back(ts.getSoBaoDanh());
                                    r.push_back("NV" + to_string(nv.thuTuNV));
                                    r.push_back(nv.maNganh);
                                    r.push_back(tenN);
                                    r.push_back(nv.trangThai);
                                    rows.push_back(r);
                                }
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_NguyenVong.xls", "BANG NGUYEN VONG", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang NguyenVong Excel thanh cong!";
                        }
                        if (impNV) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang NguyenVong tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        struct NVRow { string sbd; NguyenVong nv; };
                        Vector<NVRow> flatNV;
                        for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSach()[i];
                            for (size_t j = 0; j < ts.getDsNguyenVong().size(); ++j) {
                                const NguyenVong& nv = ts.getDsNguyenVong()[j];
                                string tenN = ql.layTenNganh(nv.maNganh);
                                if (searchCSDL[2][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[2]);
                                    if (Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                        Utils::toLower(nv.maNganh).find(kw) == string::npos &&
                                        Utils::toLower(tenN).find(kw) == string::npos) continue;
                                }
                                NVRow r; r.sbd = ts.getSoBaoDanh(); r.nv = nv;
                                flatNV.push_back(r);
                            }
                        }
                        pagCSDL[2].update((int)flatNV.size());
                        int offsetNV = pagCSDL[2].getOffset();
                        int endNV = (offsetNV + pagCSDL[2].getPageSize() < (int)flatNV.size()) ? 
                                    (offsetNV + pagCSDL[2].getPageSize()) : (int)flatNV.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblNguyenVongRaw", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("soBaoDanh (FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("thuTuNV", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("maNganh (FK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Nganh Dang Ky", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("trangThai", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (flatNV.empty()) {
                                RenderCommonEmptyRow(7);
                            } else {
                                for (int idx = offsetNV; idx < endNV; ++idx) {
                                    const NVRow& item = flatNV[idx];
                                    string tenN = ql.layTenNganh(item.nv.maNganh);

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_nvr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: soBaoDanh (center)
                                    AlignAndRenderCenter(1, item.sbd, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 2: thuTuNV (center)
                                    AlignAndRenderCenter(2, "NV" + to_string(item.nv.thuTuNV));
                                    // Col 3: maNganh (center)
                                    AlignAndRenderCenter(3, item.nv.maNganh, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 4: Ten Nganh (LEFT ALIGN)
                                    AlignAndRenderTextLeft(4, tenN);
                                    // Col 5: trangThai (center)
                                    ImVec4 colTT = (item.nv.trangThai == AdmissionConst::TT_NV_TRUNG_TUYEN) ? ImVec4(0.2f, 0.95f, 0.4f, 1.0f) :
                                                   ((item.nv.trangThai == AdmissionConst::TT_NV_KHONG_TRUNG) ? ImVec4(0.9f, 0.4f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                                    AlignAndRenderCenter(5, item.nv.trangThai, colTT);

                                    // Col 6: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(6);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        selectedSBD = item.sbd;
                                        showDetailModal = true;
                                    }
                                    if (onEdit) {
                                        selectedSBD = item.sbd;
                                        int p = ql.timViTriTheoSBD(selectedSBD);
                                        if (p >= 0) {
                                            populateEditThiSinh(ql.getDanhSach()[p]);
                                        }
                                    }
                                    if (onDelete) {
                                        selectedSBD = item.sbd;
                                        showDeleteModal = true;
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL2", pagCSDL[2]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.4: BẢNG KHOA ĐÀO TẠO
                    if (ImGui::BeginTabItem("  Bang 4: KHOA (MySQL: khoa)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL3", "Tim theo Ma khoa, Ten khoa...", searchCSDL[3], sizeof(searchCSDL[3]));

                        ImGui::SameLine();
                        bool addK = false, expK = false, impK = false;
                        ActionToolbarConfig cfgK;
                        RenderCommonActionToolbar("csdl_khoa", cfgK, addK, expK, impK);
                        if (addK) {
                            addKhoaMa[0] = '\0'; addKhoaTen[0] = '\0'; addKhoaMaDUT[0] = '\0';
                            addKhoaError = ""; showAddKhoaModal = true;
                        }
                        if (expK) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("maKhoa (PK)"); hd.push_back("tenKhoa"); hd.push_back("maKhoaTheoChuanDUT");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(110); wd.push_back(240); wd.push_back(180);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                                const Khoa& k = ql.getDanhSachKhoa()[i];
                                if (searchCSDL[3][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[3]);
                                    if (Utils::toLower(k.maKhoa).find(kw) == string::npos &&
                                        Utils::toLower(k.tenKhoa).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(k.maKhoa);
                                r.push_back(k.tenKhoa);
                                r.push_back(k.maKhoaTheoChuanDUT);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_Khoa.xls", "BANG DANH MUC KHOA", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang Khoa Excel thanh cong!";
                        }
                        if (impK) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang Khoa tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredKhoa;
                        for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                            const Khoa& k = ql.getDanhSachKhoa()[i];
                            if (searchCSDL[3][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[3]);
                                if (Utils::toLower(k.maKhoa).find(kw) == string::npos &&
                                    Utils::toLower(k.tenKhoa).find(kw) == string::npos) continue;
                            }
                            filteredKhoa.push_back(i);
                        }
                        pagCSDL[3].update((int)filteredKhoa.size());
                        int offsetK = pagCSDL[3].getOffset();
                        int endK = (offsetK + pagCSDL[3].getPageSize() < (int)filteredKhoa.size()) ? 
                                   (offsetK + pagCSDL[3].getPageSize()) : (int)filteredKhoa.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblKhoaRaw", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maKhoa (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenKhoa", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("maKhoaTheoChuanDUT", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredKhoa.empty()) {
                                RenderCommonEmptyRow(5);
                            } else {
                                for (int idx = offsetK; idx < endK; ++idx) {
                                    size_t realIdx = filteredKhoa[idx];
                                    const Khoa& k = ql.getDanhSachKhoa()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_kr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: maKhoa (center)
                                    AlignAndRenderCenter(1, k.maKhoa, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 2: tenKhoa (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, k.tenKhoa);
                                    // Col 3: maKhoaTheoChuanDUT (center)
                                    AlignAndRenderCenter(3, k.maKhoaTheoChuanDUT, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                                    // Col 4: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(4);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewKhoaData = k;
                                        showViewKhoaModal = true;
                                    }
                                    if (onEdit) {
                                        strncpy(editKhoaMa, k.maKhoa.c_str(), sizeof(editKhoaMa) - 1);
                                        strncpy(editKhoaTen, k.tenKhoa.c_str(), sizeof(editKhoaTen) - 1);
                                        strncpy(editKhoaMaDUT, k.maKhoaTheoChuanDUT.c_str(), sizeof(editKhoaMaDUT) - 1);
                                        editKhoaError = "";
                                        showEditKhoaModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaKhoa(k.maKhoa)) {
                                            showToast = true;
                                            toastMsg = "Da xoa khoa " + k.maKhoa + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL3", pagCSDL[3]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.5: BẢNG DANH MỤC NGÀNH TĨNH
                    if (ImGui::BeginTabItem("  Bang 5: DANH MUC NGANH (MySQL: danh_muc_nganh)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL4", "Tim theo Ma nganh, Ten nganh...", searchCSDL[4], sizeof(searchCSDL[4]));

                        ImGui::SameLine();
                        bool addDMN = false, expDMN = false, impDMN = false;
                        ActionToolbarConfig cfgDMN;
                        RenderCommonActionToolbar("csdl_dmn", cfgDMN, addDMN, expDMN, impDMN);
                        if (addDMN) {
                            addNganhMa[0] = '\0'; addNganhTen[0] = '\0';
                            if (!ql.getDanhSachKhoa().empty()) addNganhKhoa = ql.getDanhSachKhoa()[0].maKhoa;
                            addNganhTienTo[0] = '\0'; addNganhError = ""; showAddNganhModal = true;
                        }
                        if (expDMN) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("maNganh (PK)"); hd.push_back("tenNganh");
                            hd.push_back("maKhoa (FK)"); hd.push_back("tienToLop");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(110); wd.push_back(240); wd.push_back(100); wd.push_back(130);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                                const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                                if (searchCSDL[4][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[4]);
                                    if (Utils::toLower(dmn.maNganh).find(kw) == string::npos &&
                                        Utils::toLower(dmn.tenNganh).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(dmn.maNganh);
                                r.push_back(dmn.tenNganh);
                                r.push_back(dmn.maKhoa);
                                r.push_back(dmn.tienToLop);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_DanhMucNganh.xls", "BANG DANH MUC NGANH", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang DanhMucNganh Excel thanh cong!";
                        }
                        if (impDMN) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang DanhMucNganh tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredDMN;
                        for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                            const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                            if (searchCSDL[4][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[4]);
                                if (Utils::toLower(dmn.maNganh).find(kw) == string::npos &&
                                    Utils::toLower(dmn.tenNganh).find(kw) == string::npos) continue;
                            }
                            filteredDMN.push_back(i);
                        }
                        pagCSDL[4].update((int)filteredDMN.size());
                        int offsetDMN = pagCSDL[4].getOffset();
                        int endDMN = (offsetDMN + pagCSDL[4].getPageSize() < (int)filteredDMN.size()) ? 
                                     (offsetDMN + pagCSDL[4].getPageSize()) : (int)filteredDMN.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblDanhMucNganhRaw", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maNganh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenNganh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("maKhoa (FK)", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("tienToLop", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredDMN.empty()) {
                                RenderCommonEmptyRow(6);
                            } else {
                                for (int idx = offsetDMN; idx < endDMN; ++idx) {
                                    size_t realIdx = filteredDMN[idx];
                                    const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_dmnr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: maNganh (center)
                                    AlignAndRenderCenter(1, dmn.maNganh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 2: tenNganh (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, dmn.tenNganh);
                                    // Col 3: maKhoa (center)
                                    AlignAndRenderCenter(3, dmn.maKhoa, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 4: tienToLop (center)
                                    AlignAndRenderCenter(4, dmn.tienToLop, ImVec4(0.2f, 0.95f, 0.4f, 1.0f));

                                    // Col 5: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(5);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewNganhData = dmn;
                                        showViewNganhModal = true;
                                    }
                                    if (onEdit) {
                                        strncpy(editNganhMa, dmn.maNganh.c_str(), sizeof(editNganhMa) - 1);
                                        strncpy(editNganhTen, dmn.tenNganh.c_str(), sizeof(editNganhTen) - 1);
                                        editNganhKhoa = dmn.maKhoa;
                                        strncpy(editNganhTienTo, dmn.tienToLop.c_str(), sizeof(editNganhTienTo) - 1);
                                        editNganhError = "";
                                        showEditNganhModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaDanhMucNganh(dmn.maNganh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa nganh " + dmn.maNganh + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL4", pagCSDL[4]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.6: BẢNG NGÀNH THEO KHÓA HỌC (BIẾN ĐỘNG)
                    if (ImGui::BeginTabItem("  Bang 6: NGANH THEO KHOA HOC (MySQL: nganh_theo_khoa_hoc)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL5", "Tim theo Nam, Ma nganh, Ten nganh...", searchCSDL[5], sizeof(searchCSDL[5]));

                        ImGui::SameLine();
                        bool addNTK = false, expNTK = false, impNTK = false;
                        ActionToolbarConfig cfgNTK;
                        RenderCommonActionToolbar("csdl_ntk", cfgNTK, addNTK, expNTK, impNTK);
                        if (addNTK) {
                            addNTKNam = ql.getNamTuyenSinhHienHanh();
                            addNTKMa = ql.getDanhSachDanhMucNganh().empty() ? "" : ql.getDanhSachDanhMucNganh()[0].maNganh;
                            addNTKChiTieu = 50; addNTKDiemSan = (float)ql.getDiemSanMacDinh();
                            addNTKError = ""; showAddNTKModal = true;
                        }
                        if (expNTK) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("namTuyenSinh (PK, FK)"); hd.push_back("maNganh (PK, FK)");
                            hd.push_back("chiTieu"); hd.push_back("diemSanToiThieu"); hd.push_back("diemChuanChot");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(140); wd.push_back(120); wd.push_back(90); wd.push_back(110); wd.push_back(120);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                if (searchCSDL[5][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[5]);
                                    if (to_string(ntk.namTuyenSinh).find(kw) == string::npos &&
                                        Utils::toLower(ntk.maNganh).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(to_string(ntk.namTuyenSinh));
                                r.push_back(ntk.maNganh);
                                r.push_back(to_string(ntk.chiTieu));
                                char bSan[32], bChuan[32];
                                sprintf(bSan, "%.2f", ntk.diemSanToiThieu); r.push_back(bSan);
                                if (ntk.diemChuanChot > 0.0) { sprintf(bChuan, "%.2f", ntk.diemChuanChot); r.push_back(bChuan); }
                                else r.push_back("Chua chot");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_NganhTheoKhoaHoc.xls", "BANG NGANH THEO KHOA HOC", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang NganhTheoKhoaHoc Excel thanh cong!";
                        }
                        if (impNTK) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang NganhTheoKhoaHoc tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredNTK;
                        for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                            const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                            if (searchCSDL[5][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[5]);
                                if (to_string(ntk.namTuyenSinh).find(kw) == string::npos &&
                                    Utils::toLower(ntk.maNganh).find(kw) == string::npos) continue;
                            }
                            filteredNTK.push_back(i);
                        }
                        pagCSDL[5].update((int)filteredNTK.size());
                        int offsetNTK = pagCSDL[5].getOffset();
                        int endNTK = (offsetNTK + pagCSDL[5].getPageSize() < (int)filteredNTK.size()) ? 
                                     (offsetNTK + pagCSDL[5].getPageSize()) : (int)filteredNTK.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblNganhTheoKhoaHocRaw", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("namTuyenSinh (PK, FK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("maNganh (PK, FK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("chiTieu", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableSetupColumn("diemSanToiThieu", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("diemChuanChot", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredNTK.empty()) {
                                RenderCommonEmptyRow(7);
                            } else {
                                for (int idx = offsetNTK; idx < endNTK; ++idx) {
                                    size_t realIdx = filteredNTK[idx];
                                    const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_ntkr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: namTuyenSinh (center)
                                    AlignAndRenderCenter(1, to_string(ntk.namTuyenSinh));
                                    // Col 2: maNganh (center)
                                    AlignAndRenderCenter(2, ntk.maNganh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 3: chiTieu (center)
                                    AlignAndRenderCenter(3, to_string(ntk.chiTieu));
                                    // Col 4: diemSanToiThieu (center)
                                    char bSan[32]; sprintf(bSan, "%.2f", ntk.diemSanToiThieu);
                                    AlignAndRenderCenter(4, bSan);
                                    // Col 5: diemChuanChot (center)
                                    if (ntk.diemChuanChot > 0.0) {
                                        char bChuan[32]; sprintf(bChuan, "%.2f", ntk.diemChuanChot);
                                        AlignAndRenderCenter(5, bChuan, ImVec4(0.2f, 0.95f, 0.4f, 1.0f));
                                    } else {
                                        AlignAndRenderCenter(5, "Chua chot", ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                                    }

                                    // Col 6: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(6);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewNTKData = ntk;
                                        showViewNTKModal = true;
                                    }
                                    if (onEdit) {
                                        editNTKNam = ntk.namTuyenSinh;
                                        editNTKMa = ntk.maNganh;
                                        editNTKChiTieu = ntk.chiTieu;
                                        editNTKDiemSan = (float)ntk.diemSanToiThieu;
                                        editNTKError = "";
                                        showEditNTKModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaNganhTheoKhoaHoc(ntk.namTuyenSinh, ntk.maNganh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa chi tieu nganh " + ntk.maNganh + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL5", pagCSDL[5]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.7: BẢNG KHÓA HỌC
                    if (ImGui::BeginTabItem("  Bang 7: KHOA HOC (MySQL: khoa_hoc)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL6", "Tim theo Nam, Ten khoa hoc...", searchCSDL[6], sizeof(searchCSDL[6]));

                        ImGui::SameLine();
                        bool addKH = false, expKH = false, impKH = false;
                        ActionToolbarConfig cfgKH;
                        RenderCommonActionToolbar("csdl_kh", cfgKH, addKH, expKH, impKH);
                        if (addKH) {
                            showToast = true;
                            toastMsg = "Chuc nang them Khoa hoc truc tiep dang duoc phat trien!";
                        }
                        if (expKH) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("namTuyenSinh (PK)"); hd.push_back("tenKhoaHoc"); hd.push_back("trangThai");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(140); wd.push_back(240); wd.push_back(160);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachKhoaHoc().size(); ++i) {
                                const KhoaHoc& kh = ql.getDanhSachKhoaHoc()[i];
                                if (searchCSDL[6][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[6]);
                                    if (to_string(kh.namTuyenSinh).find(kw) == string::npos &&
                                        Utils::toLower(kh.tenKhoaHoc).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(to_string(kh.namTuyenSinh));
                                r.push_back(kh.tenKhoaHoc);
                                r.push_back(kh.trangThai);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_KhoaHoc.xls", "BANG KHOA HOC", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang KhoaHoc Excel thanh cong!";
                        }
                        if (impKH) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang KhoaHoc tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredKH;
                        for (size_t i = 0; i < ql.getDanhSachKhoaHoc().size(); ++i) {
                            const KhoaHoc& kh = ql.getDanhSachKhoaHoc()[i];
                            if (searchCSDL[6][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[6]);
                                if (to_string(kh.namTuyenSinh).find(kw) == string::npos &&
                                    Utils::toLower(kh.tenKhoaHoc).find(kw) == string::npos) continue;
                            }
                            filteredKH.push_back(i);
                        }
                        pagCSDL[6].update((int)filteredKH.size());
                        int offsetKH = pagCSDL[6].getOffset();
                        int endKH = (offsetKH + pagCSDL[6].getPageSize() < (int)filteredKH.size()) ? 
                                    (offsetKH + pagCSDL[6].getPageSize()) : (int)filteredKH.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblKhoaHocRaw", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("namTuyenSinh (PK)", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("tenKhoaHoc", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("trangThai", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredKH.empty()) {
                                RenderCommonEmptyRow(5);
                            } else {
                                for (int idx = offsetKH; idx < endKH; ++idx) {
                                    size_t realIdx = filteredKH[idx];
                                    const KhoaHoc& kh = ql.getDanhSachKhoaHoc()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_khr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: namTuyenSinh (center)
                                    AlignAndRenderCenter(1, to_string(kh.namTuyenSinh), ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 2: tenKhoaHoc (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, kh.tenKhoaHoc);
                                    // Col 3: trangThai (center)
                                    AlignAndRenderCenter(3, kh.trangThai, ImVec4(0.2f, 0.95f, 0.4f, 1.0f));

                                    // Col 4: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(4);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView || onEdit) {
                                        showToast = true;
                                        toastMsg = "Khoa hoc: " + kh.tenKhoaHoc;
                                    }
                                    if (onDelete) {
                                        showToast = true;
                                        toastMsg = "Khong the xoa Khoa hoc he thong!";
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL6", pagCSDL[6]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.8: BẢNG DANH MỤC TỈNH THÀNH
                    if (ImGui::BeginTabItem("  Bang 8: TINH THANH (MySQL: tinh_thanh)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL7", "Tim theo Ma tinh, Ten tinh...", searchCSDL[7], sizeof(searchCSDL[7]));

                        ImGui::SameLine();
                        bool addTT = false, expTT = false, impTT = false;
                        ActionToolbarConfig cfgTT;
                        RenderCommonActionToolbar("csdl_tt", cfgTT, addTT, expTT, impTT);
                        if (addTT) {
                            addTinhThanhMa[0] = '\0'; addTinhThanhTen[0] = '\0'; addTinhThanhLoai[0] = '\0';
                            addTinhThanhError = ""; showAddTinhThanhModal = true;
                        }
                        if (expTT) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("maTinh (PK)"); hd.push_back("tenTinh"); hd.push_back("loai");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(110); wd.push_back(240); wd.push_back(260);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                if (searchCSDL[7][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[7]);
                                    if (Utils::toLower(tt.maTinh).find(kw) == string::npos &&
                                        Utils::toLower(tt.tenTinh).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(tt.maTinh);
                                r.push_back(tt.tenTinh);
                                r.push_back(tt.loai);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_TinhThanh.xls", "BANG TINH THANH", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang TinhThanh Excel thanh cong!";
                        }
                        if (impTT) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang TinhThanh tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredTT;
                        for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                            const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                            if (searchCSDL[7][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[7]);
                                if (Utils::toLower(tt.maTinh).find(kw) == string::npos &&
                                    Utils::toLower(tt.tenTinh).find(kw) == string::npos) continue;
                            }
                            filteredTT.push_back(i);
                        }
                        pagCSDL[7].update((int)filteredTT.size());
                        int offsetTT = pagCSDL[7].getOffset();
                        int endTT = (offsetTT + pagCSDL[7].getPageSize() < (int)filteredTT.size()) ? 
                                    (offsetTT + pagCSDL[7].getPageSize()) : (int)filteredTT.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblTinhThanhRaw", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maTinh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("tenTinh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("loai", ImGuiTableColumnFlags_WidthFixed, 260);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredTT.empty()) {
                                RenderCommonEmptyRow(5);
                            } else {
                                for (int idx = offsetTT; idx < endTT; ++idx) {
                                    size_t realIdx = filteredTT[idx];
                                    const TinhThanh& tt = ql.getDanhSachTinhThanh()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_ttr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: maTinh (center)
                                    AlignAndRenderCenter(1, tt.maTinh, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 2: tenTinh (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, tt.tenTinh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 3: loai (LEFT ALIGN)
                                    AlignAndRenderTextLeft(3, tt.loai);

                                    // Col 4: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(4);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewTinhThanhData = tt;
                                        showViewTinhThanhModal = true;
                                    }
                                    if (onEdit) {
                                        strncpy(editTinhThanhMa, tt.maTinh.c_str(), sizeof(editTinhThanhMa) - 1);
                                        strncpy(editTinhThanhTen, tt.tenTinh.c_str(), sizeof(editTinhThanhTen) - 1);
                                        strncpy(editTinhThanhLoai, tt.loai.c_str(), sizeof(editTinhThanhLoai) - 1);
                                        editTinhThanhError = "";
                                        showEditTinhThanhModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaTinhThanh(tt.maTinh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa tinh thanh " + tt.maTinh + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL7", pagCSDL[7]);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 2.9: BẢNG DANH MỤC GIỚI TÍNH
                    if (ImGui::BeginTabItem("  Bang 9: GIOI TINH (MySQL: gioi_tinh)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchCSDL8", "Tim theo Ma, Ten gioi tinh...", searchCSDL[8], sizeof(searchCSDL[8]));

                        ImGui::SameLine();
                        bool addGT = false, expGT = false, impGT = false;
                        ActionToolbarConfig cfgGT;
                        RenderCommonActionToolbar("csdl_gt", cfgGT, addGT, expGT, impGT);
                        if (addGT) {
                            addGioiTinhMa[0] = '\0'; addGioiTinhTen[0] = '\0'; addGioiTinhDesc[0] = '\0';
                            addGioiTinhError = ""; showAddGioiTinhModal = true;
                        }
                        if (expGT) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("maGioiTinh (PK)"); hd.push_back("tenGioiTinh"); hd.push_back("moTa");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(120); wd.push_back(160); wd.push_back(240);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                if (searchCSDL[8][0] != '\0') {
                                    string kw = Utils::toLower(searchCSDL[8]);
                                    if (Utils::toLower(gt.maGioiTinh).find(kw) == string::npos &&
                                        Utils::toLower(gt.tenGioiTinh).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(gt.maGioiTinh);
                                r.push_back(gt.tenGioiTinh);
                                r.push_back(gt.moTa);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_CSDL_GioiTinh.xls", "BANG GIOI TINH", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat bang GioiTinh Excel thanh cong!";
                        }
                        if (impGT) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo bang GioiTinh tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredGT;
                        for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                            const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                            if (searchCSDL[8][0] != '\0') {
                                string kw = Utils::toLower(searchCSDL[8]);
                                if (Utils::toLower(gt.maGioiTinh).find(kw) == string::npos &&
                                    Utils::toLower(gt.tenGioiTinh).find(kw) == string::npos) continue;
                            }
                            filteredGT.push_back(i);
                        }
                        pagCSDL[8].update((int)filteredGT.size());
                        int offsetGT = pagCSDL[8].getOffset();
                        int endGT = (offsetGT + pagCSDL[8].getPageSize() < (int)filteredGT.size()) ? 
                                    (offsetGT + pagCSDL[8].getPageSize()) : (int)filteredGT.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblGioiTinhRaw", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("maGioiTinh (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("tenGioiTinh", ImGuiTableColumnFlags_WidthFixed, 160);
                            ImGui::TableSetupColumn("moTa", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredGT.empty()) {
                                RenderCommonEmptyRow(5);
                            } else {
                                for (int idx = offsetGT; idx < endGT; ++idx) {
                                    size_t realIdx = filteredGT[idx];
                                    const GioiTinh& gt = ql.getDanhSachGioiTinh()[realIdx];

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_gtr_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: maGioiTinh (center)
                                    AlignAndRenderCenter(1, gt.maGioiTinh, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 2: tenGioiTinh (center)
                                    AlignAndRenderCenter(2, gt.tenGioiTinh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 3: moTa (LEFT ALIGN)
                                    AlignAndRenderTextLeft(3, gt.moTa);

                                    // Col 4: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(4);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewGioiTinhData = gt;
                                        showViewGioiTinhModal = true;
                                    }
                                    if (onEdit) {
                                        strncpy(editGioiTinhMa, gt.maGioiTinh.c_str(), sizeof(editGioiTinhMa) - 1);
                                        strncpy(editGioiTinhTen, gt.tenGioiTinh.c_str(), sizeof(editGioiTinhTen) - 1);
                                        strncpy(editGioiTinhDesc, gt.moTa.c_str(), sizeof(editGioiTinhDesc) - 1);
                                        editGioiTinhError = "";
                                        showEditGioiTinhModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaGioiTinh(gt.maGioiTinh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa gioi tinh " + gt.maGioiTinh + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagCSDL8", pagCSDL[8]);

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

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH & FILTER KHOA | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(180);
                        if (ImGui::BeginCombo("##filterDCNKhoa", filterDiemChuanKhoa == "ALL" ? "Tat ca Khoa" : filterDiemChuanKhoa.c_str())) {
                            if (ImGui::Selectable("Tat ca Khoa", filterDiemChuanKhoa == "ALL")) filterDiemChuanKhoa = "ALL";
                            for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                                const Khoa& kh = ql.getDanhSachKhoa()[k];
                                bool isSel = (filterDiemChuanKhoa == kh.maKhoa);
                                if (ImGui::Selectable(kh.maKhoa.c_str(), isSel)) filterDiemChuanKhoa = kh.maKhoa;
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(260);
                        ImGui::InputTextWithHint("##searchDiemChuan", "Tim theo Ma nganh, Ten nganh...", searchDiemChuan, sizeof(searchDiemChuan));

                        ImGui::SameLine();
                        bool addDCN = false, expDCN = false, impDCN = false;
                        ActionToolbarConfig cfgDCN;
                        RenderCommonActionToolbar("dcn", cfgDCN, addDCN, expDCN, impDCN);
                        if (addDCN) {
                            addNTKNam = ql.getNamTuyenSinhHienHanh();
                            addNTKMa = ql.getDanhSachDanhMucNganh().empty() ? "" : ql.getDanhSachDanhMucNganh()[0].maNganh;
                            addNTKChiTieu = 50;
                            addNTKDiemSan = (float)ql.getDiemSanMacDinh();
                            addNTKError = "";
                            showAddNTKModal = true;
                        }
                        if (expDCN) {
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("Ma Nganh"); hd.push_back("Ten Nganh Dao Tao DUT");
                            hd.push_back("Khoa"); hd.push_back("Tien To Lop"); hd.push_back("Chi Tieu");
                            hd.push_back("Diem San"); hd.push_back("Diem Chuan Chot"); hd.push_back("Da Trung"); hd.push_back("Du Kien Lop");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(95); wd.push_back(240);
                            wd.push_back(75); wd.push_back(100); wd.push_back(75);
                            wd.push_back(75); wd.push_back(115); wd.push_back(80); wd.push_back(95);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                                string maK = ql.layMaKhoaCuaNganh(ntk.maNganh);
                                if (filterDiemChuanKhoa != "ALL" && maK != filterDiemChuanKhoa) continue;
                                string tenN = ql.layTenNganh(ntk.maNganh);
                                if (searchDiemChuan[0] != '\0') {
                                    string kw = Utils::toLower(searchDiemChuan);
                                    if (Utils::toLower(ntk.maNganh).find(kw) == string::npos &&
                                        Utils::toLower(tenN).find(kw) == string::npos) continue;
                                }
                                sttExp++;
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
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(ntk.maNganh);
                                r.push_back(tenN);
                                r.push_back(maK);
                                r.push_back(ql.layTienToLop(ntk.maNganh));
                                r.push_back(to_string(ntk.chiTieu));
                                char b1[32], b2[32];
                                sprintf(b1, "%.2f", ntk.diemSanToiThieu); r.push_back(b1);
                                if (ntk.diemChuanChot > 0.0) { sprintf(b2, "%.2f", ntk.diemChuanChot); r.push_back(b2); }
                                else r.push_back("Chua chot");
                                r.push_back(to_string(countAdmitted) + "/" + to_string(ntk.chiTieu));
                                r.push_back(to_string(soLop) + " lop");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_DiemChuan_ChiTieu_K" + to_string(ql.getNamTuyenSinhHienHanh() % 100) + ".xls",
                                                  "BANG CHI TIEU VA DIEM CHUAN CHOT KHOA " + to_string(ql.getNamTuyenSinhHienHanh()),
                                                  hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat danh sach Diem chuan & Chi tieu Excel thanh cong!";
                        }
                        if (impDCN) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo du lieu Chi tieu & Diem chuan tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredIndices;
                        for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                            const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                            if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                            string maK = ql.layMaKhoaCuaNganh(ntk.maNganh);
                            if (filterDiemChuanKhoa != "ALL" && maK != filterDiemChuanKhoa) continue;
                            string tenN = ql.layTenNganh(ntk.maNganh);
                            if (searchDiemChuan[0] != '\0') {
                                string kw = Utils::toLower(searchDiemChuan);
                                if (Utils::toLower(ntk.maNganh).find(kw) == string::npos &&
                                    Utils::toLower(tenN).find(kw) == string::npos) continue;
                            }
                            filteredIndices.push_back(i);
                        }
                        pagDiemChuan.update((int)filteredIndices.size());
                        int offsetDCN = pagDiemChuan.getOffset();
                        int endDCN = (offsetDCN + pagDiemChuan.getPageSize() < (int)filteredIndices.size()) ? 
                                     (offsetDCN + pagDiemChuan.getPageSize()) : (int)filteredIndices.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblDiemChuanNganh_v2", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Nganh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Tien To Lop", ImGuiTableColumnFlags_WidthFixed, 100);
                            ImGui::TableSetupColumn("Chi Tieu", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Diem San", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Diem Chuan Chot", ImGuiTableColumnFlags_WidthFixed, 115);
                            ImGui::TableSetupColumn("Da Trung", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Du Kien Lop", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredIndices.empty()) {
                                RenderCommonEmptyRow(11);
                            } else {
                                for (int idx = offsetDCN; idx < endDCN; ++idx) {
                                    size_t realIdx = filteredIndices[idx];
                                    const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[realIdx];
                                    string tenNganh = ql.layTenNganh(ntk.maNganh);
                                    string maKhoa = ql.layMaKhoaCuaNganh(ntk.maNganh);
                                    string tienTo = ql.layTienToLop(ntk.maNganh);

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
                                    char rId[32]; sprintf(rId, "##row_dcn_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: Ma Nganh (center)
                                    AlignAndRenderCenter(1, ntk.maNganh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                    // Col 2: Ten Nganh (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, tenNganh);
                                    // Col 3: Khoa (center)
                                    AlignAndRenderCenter(3, maKhoa, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
                                    // Col 4: Tien To Lop (center)
                                    AlignAndRenderCenter(4, tienTo, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
                                    // Col 5: Chi Tieu (center)
                                    AlignAndRenderCenter(5, to_string(ntk.chiTieu));
                                    // Col 6: Diem San (center)
                                    char bufSan[32]; sprintf(bufSan, "%.2f", ntk.diemSanToiThieu);
                                    AlignAndRenderCenter(6, bufSan);
                                    // Col 7: Diem Chuan Chot (center)
                                    if (ntk.diemChuanChot > 0.0) {
                                        char bufDiem[32]; sprintf(bufDiem, "%.2f diem", ntk.diemChuanChot);
                                        AlignAndRenderCenter(7, bufDiem, ImVec4(0.2f, 0.95f, 0.4f, 1.0f));
                                    } else {
                                        AlignAndRenderCenter(7, "Chua chot", ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                                    }
                                    // Col 8: Da Trung (center)
                                    string strAdmit = to_string(countAdmitted) + " / " + to_string(ntk.chiTieu);
                                    AlignAndRenderCenter(8, strAdmit, countAdmitted > 0 ? ImVec4(0.2f, 0.95f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                                    // Col 9: Du Kien Lop (center)
                                    AlignAndRenderCenter(9, to_string(soLop) + " lop");

                                    // Col 10: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(10);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        viewNTKData = ntk;
                                        showViewNTKModal = true;
                                    }
                                    if (onEdit) {
                                        editNTKNam = ntk.namTuyenSinh;
                                        editNTKMa = ntk.maNganh;
                                        editNTKChiTieu = ntk.chiTieu;
                                        editNTKDiemSan = (float)ntk.diemSanToiThieu;
                                        editNTKError = "";
                                        showEditNTKModal = true;
                                    }
                                    if (onDelete) {
                                        if (ql.xoaNganhTheoKhoaHoc(ntk.namTuyenSinh, ntk.maNganh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa nganh " + ntk.maNganh + " khoi khoa " + to_string(ntk.namTuyenSinh) + "!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagDCN", pagDiemChuan);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 3.2: DANH SÁCH TÂN SINH VIÊN TRÚNG TUYỂN & PHÂN LỚP
                    if (ImGui::BeginTabItem("  2. Danh Sach Tan Sinh Vien & Lop Sinh Hoat DUT  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH & FILTERS | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(160);
                        if (ImGui::BeginCombo("##filterTanSVKhoa", filterTanSVKhoa == "ALL" ? "Tat ca Khoa" : filterTanSVKhoa.c_str())) {
                            if (ImGui::Selectable("Tat ca Khoa", filterTanSVKhoa == "ALL")) filterTanSVKhoa = "ALL";
                            for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                                const Khoa& kh = ql.getDanhSachKhoa()[k];
                                bool isSel = (filterTanSVKhoa == kh.maKhoa);
                                if (ImGui::Selectable(kh.maKhoa.c_str(), isSel)) filterTanSVKhoa = kh.maKhoa;
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(260);
                        ImGui::InputTextWithHint("##searchTanSV", "Tim theo Ho ten, SBD, Ma SV, Lop...", searchTanSV, sizeof(searchTanSV));

                        ImGui::SameLine();
                        bool addTSV = false, expTSV = false, impTSV = false;
                        ActionToolbarConfig cfgTSV;
                        RenderCommonActionToolbar("tansv", cfgTSV, addTSV, expTSV, impTSV);
                        if (addTSV) {
                            showAddModal = true;
                        }
                        if (expTSV) {
                            ql.xuatBaoCaoTrungTuyenDUT("data/BaoCao_TanSinhVien_DUT_K" + to_string(ql.getNamTuyenSinhHienHanh() % 100) + ".txt", ql.getNamTuyenSinhHienHanh());
                            Vector<string> hd;
                            hd.push_back("STT"); hd.push_back("Ma Sinh Vien DUT"); hd.push_back("Ho va Ten");
                            hd.push_back("Lop Sinh Hoat"); hd.push_back("Nganh Trung Tuyen"); hd.push_back("Khoa");
                            hd.push_back("NV"); hd.push_back("Tong Diem");
                            Vector<int> wd;
                            wd.push_back(40); wd.push_back(120); wd.push_back(200);
                            wd.push_back(130); wd.push_back(240); wd.push_back(75);
                            wd.push_back(50); wd.push_back(80);
                            Vector<Vector<string>> rows;
                            int sttExp = 0;
                            for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                                const ThiSinh& ts = ql.getDanhSach()[i];
                                if (ts.getNamTuyenSinh() != ql.getNamTuyenSinhHienHanh() || !ts.daTrungTuyen()) continue;
                                string maK = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
                                if (filterTanSVKhoa != "ALL" && maK != filterTanSVKhoa) continue;
                                if (searchTanSV[0] != '\0') {
                                    string kw = Utils::toLower(searchTanSV);
                                    if (Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getMaSinhVien()).find(kw) == string::npos &&
                                        Utils::toLower(ts.getLopSinhHoat()).find(kw) == string::npos) continue;
                                }
                                sttExp++;
                                Vector<string> r;
                                r.push_back(to_string(sttExp));
                                r.push_back(ts.getMaSinhVien());
                                r.push_back(ts.getHoTen());
                                r.push_back(ts.getLopSinhHoat());
                                r.push_back(ql.layTenNganh(ts.getNganhTrungTuyen()));
                                r.push_back(maK);
                                r.push_back("NV" + to_string(ts.getNguyenVongTrungTuyen()));
                                char bDiem[32]; sprintf(bDiem, "%.2f", ts.tinhTongDiem());
                                r.push_back(bDiem);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_TanSinhVien_DUT_K" + to_string(ql.getNamTuyenSinhHienHanh() % 100) + ".xls",
                                                  "DANH SACH TAN SINH VIEN DUT KHOA " + to_string(ql.getNamTuyenSinhHienHanh()),
                                                  hd, wd, rows);
                            showToast = true;
                            toastMsg = "Xuat danh sach Tan sinh vien DUT Excel thanh cong!";
                        }
                        if (impTSV) {
                            ql.taiTatCaDuLieuTuDatabase();
                            showToast = true;
                            toastMsg = "Dong bo du lieu Tan sinh vien tu MySQL thanh cong!";
                        }

                        ImGui::Spacing();

                        // 2. DATA FILTERING & SERVER-SIDE PAGINATION
                        Vector<size_t> filteredTSV;
                        for (size_t i = 0; i < ql.getDanhSach().size(); ++i) {
                            const ThiSinh& ts = ql.getDanhSach()[i];
                            if (ts.getNamTuyenSinh() != ql.getNamTuyenSinhHienHanh() || !ts.daTrungTuyen()) continue;
                            string maK = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());
                            if (filterTanSVKhoa != "ALL" && maK != filterTanSVKhoa) continue;
                            if (searchTanSV[0] != '\0') {
                                string kw = Utils::toLower(searchTanSV);
                                if (Utils::toLower(ts.getHoTen()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getSoBaoDanh()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getMaSinhVien()).find(kw) == string::npos &&
                                    Utils::toLower(ts.getLopSinhHoat()).find(kw) == string::npos) continue;
                            }
                            filteredTSV.push_back(i);
                        }
                        pagTanSV.update((int)filteredTSV.size());
                        int offsetTSV = pagTanSV.getOffset();
                        int endTSV = (offsetTSV + pagTanSV.getPageSize() < (int)filteredTSV.size()) ? 
                                     (offsetTSV + pagTanSV.getPageSize()) : (int)filteredTSV.size();

                        // 3. TABLE
                        if (ImGui::BeginTable("TblTanSinhVienDUT_v2", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 335))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Sinh Vien DUT", ImGuiTableColumnFlags_WidthFixed, 130);
                            ImGui::TableSetupColumn("Ho va Ten", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Lop Sinh Hoat", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("Nganh Trung Tuyen", ImGuiTableColumnFlags_WidthFixed, 220);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("NV", ImGuiTableColumnFlags_WidthFixed, 50);
                            ImGui::TableSetupColumn("Tong Diem", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredTSV.empty()) {
                                RenderCommonEmptyRow(9);
                            } else {
                                for (int idx = offsetTSV; idx < endTSV; ++idx) {
                                    size_t realIdx = filteredTSV[idx];
                                    const ThiSinh& ts = ql.getDanhSach()[realIdx];
                                    string tenNganh = ql.layTenNganh(ts.getNganhTrungTuyen());
                                    string maKhoa = ql.layMaKhoaCuaNganh(ts.getNganhTrungTuyen());

                                    ImGui::TableNextRow();
                                    char rId[32]; sprintf(rId, "##row_tsv_%d", idx + 1); RenderRowHover(rId);

                                    // Col 0: STT (center)
                                    AlignAndRenderCenter(0, to_string(idx + 1));
                                    // Col 1: Ma Sinh Vien (center)
                                    AlignAndRenderCenter(1, ts.getMaSinhVien(), ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
                                    // Col 2: Ho va Ten (LEFT ALIGN)
                                    AlignAndRenderTextLeft(2, ts.getHoTen());
                                    // Col 3: Lop Sinh Hoat (center)
                                    AlignAndRenderCenter(3, ts.getLopSinhHoat(), ImVec4(0.35f, 0.85f, 1.0f, 1.0f));
                                    // Col 4: Nganh Trung Tuyen (LEFT ALIGN)
                                    AlignAndRenderTextLeft(4, tenNganh);
                                    // Col 5: Khoa (center)
                                    AlignAndRenderCenter(5, maKhoa);
                                    // Col 6: NV (center)
                                    AlignAndRenderCenter(6, "NV" + to_string(ts.getNguyenVongTrungTuyen()), ImVec4(0.2f, 0.95f, 0.4f, 1.0f));
                                    // Col 7: Tong Diem (center)
                                    char bDiem[32]; sprintf(bDiem, "%.2f", ts.tinhTongDiem());
                                    AlignAndRenderCenter(7, bDiem, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));

                                    // Col 8: Thao Tac (102px fixed, center)
                                    ImGui::TableSetColumnIndex(8);
                                    bool onView = false, onEdit = false, onDelete = false;
                                    RenderCommonActionCell(rId, onView, onEdit, onDelete);
                                    if (onView) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDetailModal = true;
                                    }
                                    if (onEdit) {
                                        populateEditThiSinh(ts);
                                    }
                                    if (onDelete) {
                                        selectedSBD = ts.getSoBaoDanh();
                                        showDeleteModal = true;
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM DUAL-WING PAGINATION BAR
                        RenderCommonPaginationBar("pagTanSV", pagTanSV);

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

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchKhoa", "Tim theo Ma khoa, Ten khoa...", searchKhoa, sizeof(searchKhoa));

                        ImGui::SameLine();
                        bool addKhoa = false, expKhoa = false, impKhoa = false;
                        ActionToolbarConfig cfgKhoa;
                        RenderCommonActionToolbar("khoa", cfgKhoa, addKhoa, expKhoa, impKhoa);
                        if (addKhoa) {
                            addKhoaMa[0] = '\0';
                            addKhoaTen[0] = '\0';
                            addKhoaMaDUT[0] = '\0';
                            addKhoaError = "";
                            showAddKhoaModal = true;
                        }
                        if (expKhoa) {
                            Vector<string> hd; hd.push_back("STT"); hd.push_back("Ma Khoa (PK)"); hd.push_back("Ten Khoa Dao Tao DUT"); hd.push_back("Ma Dinh Danh DUT"); hd.push_back("Quy Uoc Ma SV");
                            Vector<int> wd; wd.push_back(50); wd.push_back(120); wd.push_back(280); wd.push_back(140); wd.push_back(180);
                            Vector<Vector<string>> rows;
                            for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                                const Khoa& k = ql.getDanhSachKhoa()[i];
                                Vector<string> r;
                                r.push_back(to_string(i + 1));
                                r.push_back(k.maKhoa);
                                r.push_back(k.tenKhoa);
                                r.push_back(k.maKhoaTheoChuanDUT);
                                r.push_back(k.maKhoaTheoChuanDUT + "26xxxx");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_DanhMuc_Khoa_DUT.xls", "DANH MUC KHOA DAO TAO DAI HOC BACH KHOA", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Da xuat Excel: data/BaoCao_DanhMuc_Khoa_DUT.xls";
                        }
                        if (impKhoa) {
                            showToast = true;
                            toastMsg = "Du lieu danh muc Khoa DUT da duoc dong bo chuan!";
                        }

                        ImGui::Spacing();

                        // 2. FILTERING & PAGINATION SLICING
                        Vector<Khoa> filteredKhoa;
                        string sK = Utils::toLower(Utils::trim(searchKhoa));
                        for (size_t i = 0; i < ql.getDanhSachKhoa().size(); ++i) {
                            const Khoa& k = ql.getDanhSachKhoa()[i];
                            if (sK.empty() ||
                                Utils::toLower(k.maKhoa).find(sK) != string::npos ||
                                Utils::toLower(k.tenKhoa).find(sK) != string::npos ||
                                Utils::toLower(k.maKhoaTheoChuanDUT).find(sK) != string::npos) {
                                filteredKhoa.push_back(k);
                            }
                        }
                        pagKhoa.updateTotal((int)filteredKhoa.size());

                        // 3. TABLE
                        if (ImGui::BeginTable("TblManageKhoa_v2", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Khoa (PK)", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Ten Khoa Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Ma Dinh Danh DUT", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("Quy Uoc Ma SV", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredKhoa.empty()) {
                                RenderCommonEmptyRow(6, "Khong tim thay khoa nao phu hop!");
                            } else {
                                int offset = pagKhoa.getOffset();
                                int pSize = pagKhoa.getPageSize();
                                int endIdx = (offset + pSize < (int)filteredKhoa.size()) ? (offset + pSize) : (int)filteredKhoa.size();

                                for (int i = offset; i < endIdx; ++i) {
                                    const Khoa& k = filteredKhoa[i];
                                    ImGui::TableNextRow();

                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_kr_%d", i); RenderRowHover(rId);
                                    AlignAndRenderCenter(to_string(i + 1));

                                    ImGui::TableSetColumnIndex(1);
                                    AlignAndRenderCenter(k.maKhoa, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

                                    ImGui::TableSetColumnIndex(2);
                                    AlignAndRenderTextLeft(k.tenKhoa);

                                    ImGui::TableSetColumnIndex(3);
                                    AlignAndRenderCenter(k.maKhoaTheoChuanDUT, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                                    ImGui::TableSetColumnIndex(4);
                                    AlignAndRenderCenter(k.maKhoaTheoChuanDUT + "26xxxx", ImVec4(0.2f, 0.95f, 0.4f, 1.0f));

                                    ImGui::TableSetColumnIndex(5);
                                    bool vClk = false, eClk = false, dClk = false;
                                    RenderCommonActionCell(k.maKhoa, vClk, eClk, dClk, true, true, true, "Xem chi tiet khoa", "Sua thong tin khoa", "Xoa khoa dao tao");
                                    if (vClk) {
                                        viewKhoaData = k;
                                        showViewKhoaModal = true;
                                    }
                                    if (eClk) {
                                        strcpy(editKhoaMa, k.maKhoa.c_str());
                                        strcpy(editKhoaTen, k.tenKhoa.c_str());
                                        strcpy(editKhoaMaDUT, k.maKhoaTheoChuanDUT.c_str());
                                        editKhoaError = "";
                                        showEditKhoaModal = true;
                                    }
                                    if (dClk) {
                                        if (ql.xoaKhoa(k.maKhoa)) {
                                            showToast = true;
                                            toastMsg = "Da xoa khoa " + k.maKhoa + " thanh cong!";
                                        } else {
                                            showToast = true;
                                            toastMsg = "Khong the xoa vi co nganh hoac thi sinh dang tham chieu den khoa nay!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM PAGINATION BAR
                        RenderCommonPaginationBar("pagKhoa", pagKhoa);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.2: DANH MỤC NGÀNH ĐÀO TẠO DUT
                    if (ImGui::BeginTabItem("  2. Danh muc Nganh Tuyen sinh DUT  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: FILTER KHOA + QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(160);
                        if (ImGui::BeginCombo("##filterNganhKhoa", (filterNganhKhoa == "ALL" ? "[ Tat ca Khoa ]" : filterNganhKhoa.c_str()))) {
                            if (ImGui::Selectable("[ Tat ca Khoa ]", filterNganhKhoa == "ALL")) { filterNganhKhoa = "ALL"; pagNganh.currentPage = 1; }
                            for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                                const Khoa& kh = ql.getDanhSachKhoa()[k];
                                bool isSel = (filterNganhKhoa == kh.maKhoa);
                                string label = kh.maKhoa + " - " + kh.tenKhoa;
                                if (ImGui::Selectable(label.c_str(), isSel)) {
                                    filterNganhKhoa = kh.maKhoa;
                                    pagNganh.currentPage = 1;
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(260);
                        ImGui::InputTextWithHint("##searchNganh", "Tim theo Ma nganh, Ten nganh...", searchNganh, sizeof(searchNganh));

                        ImGui::SameLine();
                        bool addNganh = false, expNganh = false, impNganh = false;
                        ActionToolbarConfig cfgNganh;
                        RenderCommonActionToolbar("nganh", cfgNganh, addNganh, expNganh, impNganh);
                        if (addNganh) {
                            addNganhMa[0] = '\0';
                            addNganhTen[0] = '\0';
                            addNganhKhoa = "CNTT";
                            addNganhTienTo[0] = '\0';
                            addNganhError = "";
                            showAddNganhModal = true;
                        }
                        if (expNganh) {
                            Vector<string> hd; hd.push_back("STT"); hd.push_back("Ma Nganh (PK)"); hd.push_back("Ten Nganh Dao Tao DUT"); hd.push_back("Khoa Truc Thuoc"); hd.push_back("Tien To Lop DUT"); hd.push_back("Mau Lop K26");
                            Vector<int> wd; wd.push_back(50); wd.push_back(110); wd.push_back(300); wd.push_back(120); wd.push_back(120); wd.push_back(150);
                            Vector<Vector<string>> rows;
                            for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                                const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                                Vector<string> r;
                                r.push_back(to_string(i + 1));
                                r.push_back(dmn.maNganh);
                                r.push_back(dmn.tenNganh);
                                r.push_back(dmn.maKhoa);
                                r.push_back(dmn.tienToLop);
                                r.push_back("26" + dmn.tienToLop + "1, 26" + dmn.tienToLop + "2");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_DanhMuc_Nganh_DUT.xls", "DANH MUC NGANH TUYEN SINH CHUAN BO GD&DT", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Da xuat Excel: data/BaoCao_DanhMuc_Nganh_DUT.xls";
                        }
                        if (impNganh) {
                            showToast = true;
                            toastMsg = "Du lieu danh muc Nganh DUT da duoc dong bo chuan!";
                        }

                        ImGui::Spacing();

                        // 2. FILTERING & PAGINATION SLICING
                        Vector<DanhMucNganh> filteredNganh;
                        string sN = Utils::toLower(Utils::trim(searchNganh));
                        for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                            const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                            if (filterNganhKhoa != "ALL" && dmn.maKhoa != filterNganhKhoa) continue;
                            if (sN.empty() ||
                                Utils::toLower(dmn.maNganh).find(sN) != string::npos ||
                                Utils::toLower(dmn.tenNganh).find(sN) != string::npos ||
                                Utils::toLower(dmn.tienToLop).find(sN) != string::npos) {
                                filteredNganh.push_back(dmn);
                            }
                        }
                        pagNganh.updateTotal((int)filteredNganh.size());

                        // 3. TABLE
                        if (ImGui::BeginTable("TblManageDanhMucNganh_v2", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Nganh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa Truc Thuoc", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Tien To Lop DUT", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Mau Lop K26", ImGuiTableColumnFlags_WidthFixed, 140);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredNganh.empty()) {
                                RenderCommonEmptyRow(7, "Khong tim thay nganh nao phu hop!");
                            } else {
                                int offset = pagNganh.getOffset();
                                int pSize = pagNganh.getPageSize();
                                int endIdx = (offset + pSize < (int)filteredNganh.size()) ? (offset + pSize) : (int)filteredNganh.size();

                                for (int i = offset; i < endIdx; ++i) {
                                    const DanhMucNganh& dmn = filteredNganh[i];
                                    ImGui::TableNextRow();

                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_dmnr_%d", i); RenderRowHover(rId);
                                    AlignAndRenderCenter(to_string(i + 1));

                                    ImGui::TableSetColumnIndex(1);
                                    AlignAndRenderCenter(dmn.maNganh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                                    ImGui::TableSetColumnIndex(2);
                                    AlignAndRenderTextLeft(dmn.tenNganh);

                                    ImGui::TableSetColumnIndex(3);
                                    AlignAndRenderCenter(dmn.maKhoa, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));

                                    ImGui::TableSetColumnIndex(4);
                                    AlignAndRenderCenter(dmn.tienToLop, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

                                    ImGui::TableSetColumnIndex(5);
                                    AlignAndRenderCenter("26" + dmn.tienToLop + "1, 26" + dmn.tienToLop + "2", ImVec4(0.2f, 0.95f, 0.4f, 1.0f));

                                    ImGui::TableSetColumnIndex(6);
                                    bool vClk = false, eClk = false, dClk = false;
                                    RenderCommonActionCell(dmn.maNganh, vClk, eClk, dClk, true, true, true, "Xem chi tiet nganh", "Sua thong tin nganh", "Xoa nganh dao tao");
                                    if (vClk) {
                                        viewNganhData = dmn;
                                        showViewNganhModal = true;
                                    }
                                    if (eClk) {
                                        strcpy(editNganhMa, dmn.maNganh.c_str());
                                        strcpy(editNganhTen, dmn.tenNganh.c_str());
                                        editNganhKhoa = dmn.maKhoa;
                                        strcpy(editNganhTienTo, dmn.tienToLop.c_str());
                                        editNganhError = "";
                                        showEditNganhModal = true;
                                    }
                                    if (dClk) {
                                        if (ql.xoaDanhMucNganh(dmn.maNganh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa nganh " + dmn.maNganh + " thanh cong!";
                                        } else {
                                            showToast = true;
                                            toastMsg = "Khong the xoa vi co nguyen vong hoac thi sinh dang dang ky nganh nay!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM PAGINATION BAR
                        RenderCommonPaginationBar("pagNganh", pagNganh);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.3: CHỈ TIÊU & ĐIỂM SÀN THEO KHÓA (ĐIỀU CHỈNH ĐỘNG)
                    if (ImGui::BeginTabItem("  3. Chi tieu Tuyen sinh Dong (K26)  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: FILTER KHOA + QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(160);
                        if (ImGui::BeginCombo("##filterNTKKhoa", (filterNTKKhoa == "ALL" ? "[ Tat ca Khoa ]" : filterNTKKhoa.c_str()))) {
                            if (ImGui::Selectable("[ Tat ca Khoa ]", filterNTKKhoa == "ALL")) { filterNTKKhoa = "ALL"; pagNTK.currentPage = 1; }
                            for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                                const Khoa& kh = ql.getDanhSachKhoa()[k];
                                bool isSel = (filterNTKKhoa == kh.maKhoa);
                                string label = kh.maKhoa + " - " + kh.tenKhoa;
                                if (ImGui::Selectable(label.c_str(), isSel)) {
                                    filterNTKKhoa = kh.maKhoa;
                                    pagNTK.currentPage = 1;
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(260);
                        ImGui::InputTextWithHint("##searchNTK", "Tim theo Ma/Ten nganh...", searchNTK, sizeof(searchNTK));

                        ImGui::SameLine();
                        bool addNTK = false, expNTK = false, impNTK = false;
                        ActionToolbarConfig cfgNTK;
                        RenderCommonActionToolbar("ntk", cfgNTK, addNTK, expNTK, impNTK);
                        if (addNTK) {
                            addNTKMa = ql.getDanhSachDanhMucNganh().empty() ? "" : ql.getDanhSachDanhMucNganh()[0].maNganh;
                            addNTKChiTieu = 50;
                            addNTKDiemSan = 15.0f;
                            addNTKError = "";
                            showAddNTKModal = true;
                        }
                        if (expNTK) {
                            Vector<string> hd; hd.push_back("STT"); hd.push_back("Ma Nganh"); hd.push_back("Ten Nganh Dao Tao DUT"); hd.push_back("Khoa"); hd.push_back("Chi Tieu K26"); hd.push_back("Diem San Toi Thieu"); hd.push_back("Diem Chuan Chot");
                            Vector<int> wd; wd.push_back(50); wd.push_back(100); wd.push_back(300); wd.push_back(100); wd.push_back(110); wd.push_back(130); wd.push_back(130);
                            Vector<Vector<string>> rows;
                            int stt = 0;
                            for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                                const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                                if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                                stt++;
                                Vector<string> r;
                                r.push_back(to_string(stt));
                                r.push_back(ntk.maNganh);
                                r.push_back(ql.layTenNganh(ntk.maNganh));
                                r.push_back(ql.layMaKhoaCuaNganh(ntk.maNganh));
                                r.push_back(to_string(ntk.chiTieu));
                                char bufDS[16], bufDC[16];
                                sprintf(bufDS, "%.2f", ntk.diemSanToiThieu);
                                sprintf(bufDC, "%.2f", ntk.diemChuanChot);
                                r.push_back(bufDS);
                                r.push_back(ntk.diemChuanChot > 0 ? bufDC : "Chua chot");
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_ChiTieu_DiemChuan_K26.xls", "BANG CHI TIEU VA DIEM CHUAN TUYEN SINH K26", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Da xuat Excel: data/BaoCao_ChiTieu_DiemChuan_K26.xls";
                        }
                        if (impNTK) {
                            showToast = true;
                            toastMsg = "Du lieu chi tieu K26 da duoc dong bo!";
                        }

                        ImGui::Spacing();

                        // 2. FILTERING & PAGINATION SLICING
                        Vector<NganhTheoKhoaHoc> filteredNTK;
                        string sNTK = Utils::toLower(Utils::trim(searchNTK));
                        for (size_t i = 0; i < ql.getDanhSachNganhTheoKhoaHoc().size(); ++i) {
                            const NganhTheoKhoaHoc& ntk = ql.getDanhSachNganhTheoKhoaHoc()[i];
                            if (ntk.namTuyenSinh != ql.getNamTuyenSinhHienHanh()) continue;
                            string kKhoa = ql.layMaKhoaCuaNganh(ntk.maNganh);
                            if (filterNTKKhoa != "ALL" && kKhoa != filterNTKKhoa) continue;
                            string tNganh = ql.layTenNganh(ntk.maNganh);
                            if (sNTK.empty() ||
                                Utils::toLower(ntk.maNganh).find(sNTK) != string::npos ||
                                Utils::toLower(tNganh).find(sNTK) != string::npos ||
                                Utils::toLower(kKhoa).find(sNTK) != string::npos) {
                                filteredNTK.push_back(ntk);
                            }
                        }
                        pagNTK.updateTotal((int)filteredNTK.size());

                        // 3. TABLE
                        if (ImGui::BeginTable("TblManageNganhTheoKhoaHoc_v2", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 35);
                            ImGui::TableSetupColumn("Ma Nganh", ImGuiTableColumnFlags_WidthFixed, 95);
                            ImGui::TableSetupColumn("Ten Nganh Dao Tao DUT", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Khoa", ImGuiTableColumnFlags_WidthFixed, 75);
                            ImGui::TableSetupColumn("Chi Tieu", ImGuiTableColumnFlags_WidthFixed, 90);
                            ImGui::TableSetupColumn("Diem San", ImGuiTableColumnFlags_WidthFixed, 80);
                            ImGui::TableSetupColumn("Diem Chuan Chot", ImGuiTableColumnFlags_WidthFixed, 120);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredNTK.empty()) {
                                RenderCommonEmptyRow(8, "Khong tim thay chi tieu nganh nao!");
                            } else {
                                int offset = pagNTK.getOffset();
                                int pSize = pagNTK.getPageSize();
                                int endIdx = (offset + pSize < (int)filteredNTK.size()) ? (offset + pSize) : (int)filteredNTK.size();

                                for (int i = offset; i < endIdx; ++i) {
                                    const NganhTheoKhoaHoc& ntk = filteredNTK[i];
                                    ImGui::TableNextRow();

                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_mntk_%d", i); RenderRowHover(rId);
                                    AlignAndRenderCenter(to_string(i + 1));

                                    ImGui::TableSetColumnIndex(1);
                                    AlignAndRenderCenter(ntk.maNganh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                                    ImGui::TableSetColumnIndex(2);
                                    AlignAndRenderTextLeft(ql.layTenNganh(ntk.maNganh));

                                    ImGui::TableSetColumnIndex(3);
                                    AlignAndRenderCenter(ql.layMaKhoaCuaNganh(ntk.maNganh));

                                    ImGui::TableSetColumnIndex(4);
                                    AlignAndRenderCenter(to_string(ntk.chiTieu), ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

                                    ImGui::TableSetColumnIndex(5);
                                    char bufDS[16]; sprintf(bufDS, "%.2f", ntk.diemSanToiThieu);
                                    AlignAndRenderCenter(bufDS);

                                    ImGui::TableSetColumnIndex(6);
                                    if (ntk.diemChuanChot > 0.0) {
                                        char bufDC[16]; sprintf(bufDC, "%.2f", ntk.diemChuanChot);
                                        AlignAndRenderCenter(bufDC, ImVec4(0.2f, 0.95f, 0.4f, 1.0f));
                                    } else {
                                        AlignAndRenderCenter("Chua chot", ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                                    }

                                    ImGui::TableSetColumnIndex(7);
                                    bool vClk = false, eClk = false, dClk = false;
                                    RenderCommonActionCell(ntk.maNganh, vClk, eClk, dClk, true, true, true, "Xem chi tiet chi tieu", "Dieu chinh chi tieu / diem san", "Xoa chi tieu nganh nay");
                                    if (vClk) {
                                        viewNTKData = ntk;
                                        showViewNTKModal = true;
                                    }
                                    if (eClk) {
                                        editNTKMa = ntk.maNganh;
                                        editNTKChiTieu = ntk.chiTieu;
                                        editNTKDiemSan = (float)ntk.diemSanToiThieu;
                                        editNTKError = "";
                                        showEditNTKModal = true;
                                    }
                                    if (dClk) {
                                        if (ql.xoaNganhTheoKhoaHoc(ntk.namTuyenSinh, ntk.maNganh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa chi tieu nganh " + ntk.maNganh + " khoi khoa " + to_string(ntk.namTuyenSinh) + "!";
                                        } else {
                                            showToast = true;
                                            toastMsg = "Khong the xoa chi tieu nganh nay!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM PAGINATION BAR
                        RenderCommonPaginationBar("pagNTK", pagNTK);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.4: QUẢN LÝ TỈNH / THÀNH PHỐ
                    if (ImGui::BeginTabItem("  4. Danh muc Tinh / Thanh pho  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchTinhThanh", "Tim theo Ma tinh, Ten tinh/thanh pho...", searchTinhThanh, sizeof(searchTinhThanh));

                        ImGui::SameLine();
                        bool addTT = false, expTT = false, impTT = false;
                        ActionToolbarConfig cfgTT;
                        RenderCommonActionToolbar("tinhthanh", cfgTT, addTT, expTT, impTT);
                        if (addTT) {
                            addTinhThanhMa[0] = '\0';
                            addTinhThanhTen[0] = '\0';
                            strcpy(addTinhThanhLoai, "Tinh");
                            addTinhThanhError = "";
                            showAddTinhThanhModal = true;
                        }
                        if (expTT) {
                            Vector<string> hd; hd.push_back("STT"); hd.push_back("Ma Tinh (PK)"); hd.push_back("Ten Don Vi Hanh Chinh"); hd.push_back("Loai Hinh");
                            Vector<int> wd; wd.push_back(50); wd.push_back(110); wd.push_back(260); wd.push_back(200);
                            Vector<Vector<string>> rows;
                            for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                                const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                                Vector<string> r;
                                r.push_back(to_string(i + 1));
                                r.push_back(tt.maTinh);
                                r.push_back(tt.tenTinh);
                                r.push_back(tt.loai);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_DanhMuc_TinhThanh.xls", "DANH MUC DON VI HANH CHINH TINH THANH PHO", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Da xuat Excel: data/BaoCao_DanhMuc_TinhThanh.xls";
                        }
                        if (impTT) {
                            showToast = true;
                            toastMsg = "Du lieu 34 Tinh / Thanh pho da duoc dong bo!";
                        }

                        ImGui::Spacing();

                        // 2. FILTERING & PAGINATION SLICING
                        Vector<TinhThanh> filteredTT;
                        string sTT = Utils::toLower(Utils::trim(searchTinhThanh));
                        for (size_t i = 0; i < ql.getDanhSachTinhThanh().size(); ++i) {
                            const TinhThanh& tt = ql.getDanhSachTinhThanh()[i];
                            if (sTT.empty() ||
                                Utils::toLower(tt.maTinh).find(sTT) != string::npos ||
                                Utils::toLower(tt.tenTinh).find(sTT) != string::npos ||
                                Utils::toLower(tt.loai).find(sTT) != string::npos) {
                                filteredTT.push_back(tt);
                            }
                        }
                        pagTinhThanh.updateTotal((int)filteredTT.size());

                        // 3. TABLE
                        if (ImGui::BeginTable("TblManageTinhThanh_v2", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma Tinh (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Don Vi Hanh Chinh", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Loai Hinh", ImGuiTableColumnFlags_WidthFixed, 220);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredTT.empty()) {
                                RenderCommonEmptyRow(5, "Khong tim thay tinh thanh nao!");
                            } else {
                                int offset = pagTinhThanh.getOffset();
                                int pSize = pagTinhThanh.getPageSize();
                                int endIdx = (offset + pSize < (int)filteredTT.size()) ? (offset + pSize) : (int)filteredTT.size();

                                for (int i = offset; i < endIdx; ++i) {
                                    const TinhThanh& tt = filteredTT[i];
                                    ImGui::TableNextRow();

                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_ttr_%d", i); RenderRowHover(rId);
                                    AlignAndRenderCenter(to_string(i + 1));

                                    ImGui::TableSetColumnIndex(1);
                                    AlignAndRenderCenter(tt.maTinh, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

                                    ImGui::TableSetColumnIndex(2);
                                    AlignAndRenderTextLeft(tt.tenTinh);

                                    ImGui::TableSetColumnIndex(3);
                                    AlignAndRenderTextLeft(tt.loai);

                                    ImGui::TableSetColumnIndex(4);
                                    bool vClk = false, eClk = false, dClk = false;
                                    RenderCommonActionCell(tt.maTinh, vClk, eClk, dClk, true, true, true, "Xem chi tiet tinh thanh", "Sua thong tin tinh thanh", "Xoa tinh thanh");
                                    if (vClk) {
                                        viewTinhThanhData = tt;
                                        showViewTinhThanhModal = true;
                                    }
                                    if (eClk) {
                                        strcpy(editTinhThanhMa, tt.maTinh.c_str());
                                        strcpy(editTinhThanhTen, tt.tenTinh.c_str());
                                        strcpy(editTinhThanhLoai, tt.loai.c_str());
                                        editTinhThanhError = "";
                                        showEditTinhThanhModal = true;
                                    }
                                    if (dClk) {
                                        if (ql.xoaTinhThanh(tt.maTinh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa tinh thanh " + tt.maTinh + " thanh cong!";
                                        } else {
                                            showToast = true;
                                            toastMsg = "Khong the xoa vi co thi sinh dang tham chieu den tinh nay!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM PAGINATION BAR
                        RenderCommonPaginationBar("pagTinhThanh", pagTinhThanh);

                        ImGui::EndTabItem();
                    }

                    // SUBTAB 4.5: QUẢN LÝ GIỚI TÍNH
                    if (ImGui::BeginTabItem("  5. Danh muc Gioi tinh  ")) {
                        ImGui::Spacing();

                        // 1. TOP TOOLBAR (LEFT: QUICK SEARCH | RIGHT: ACTION TOOLBAR)
                        ImGui::SetNextItemWidth(320);
                        ImGui::InputTextWithHint("##searchGioiTinh", "Tim theo Ma, Ten gioi tinh...", searchGioiTinh, sizeof(searchGioiTinh));

                        ImGui::SameLine();
                        bool addGT = false, expGT = false, impGT = false;
                        ActionToolbarConfig cfgGT;
                        RenderCommonActionToolbar("gioitinh", cfgGT, addGT, expGT, impGT);
                        if (addGT) {
                            addGioiTinhMa[0] = '\0';
                            addGioiTinhTen[0] = '\0';
                            addGioiTinhDesc[0] = '\0';
                            addGioiTinhError = "";
                            showAddGioiTinhModal = true;
                        }
                        if (expGT) {
                            Vector<string> hd; hd.push_back("STT"); hd.push_back("Ma Gioi Tinh (PK)"); hd.push_back("Ten Hien Thi"); hd.push_back("Mo Ta Chi Tiet");
                            Vector<int> wd; wd.push_back(50); wd.push_back(120); wd.push_back(160); wd.push_back(280);
                            Vector<Vector<string>> rows;
                            for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                                const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                                Vector<string> r;
                                r.push_back(to_string(i + 1));
                                r.push_back(gt.maGioiTinh);
                                r.push_back(gt.tenGioiTinh);
                                r.push_back(gt.moTa);
                                rows.push_back(r);
                            }
                            ql.xuatBangDuLieuExcel("data/BaoCao_DanhMuc_GioiTinh.xls", "DANH MUC GIOI TINH HE THONG", hd, wd, rows);
                            showToast = true;
                            toastMsg = "Da xuat Excel: data/BaoCao_DanhMuc_GioiTinh.xls";
                        }
                        if (impGT) {
                            showToast = true;
                            toastMsg = "Du lieu danh muc Gioi tinh da duoc dong bo!";
                        }

                        ImGui::Spacing();

                        // 2. FILTERING & PAGINATION SLICING
                        Vector<GioiTinh> filteredGT;
                        string sGT = Utils::toLower(Utils::trim(searchGioiTinh));
                        for (size_t i = 0; i < ql.getDanhSachGioiTinh().size(); ++i) {
                            const GioiTinh& gt = ql.getDanhSachGioiTinh()[i];
                            if (sGT.empty() ||
                                Utils::toLower(gt.maGioiTinh).find(sGT) != string::npos ||
                                Utils::toLower(gt.tenGioiTinh).find(sGT) != string::npos ||
                                Utils::toLower(gt.moTa).find(sGT) != string::npos) {
                                filteredGT.push_back(gt);
                            }
                        }
                        pagGioiTinh.updateTotal((int)filteredGT.size());

                        // 3. TABLE
                        if (ImGui::BeginTable("TblManageGioiTinh_v2", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetWindowHeight() - 250))) {
                            ImGui::TableSetupColumn("STT", ImGuiTableColumnFlags_WidthFixed, 40);
                            ImGui::TableSetupColumn("Ma (PK)", ImGuiTableColumnFlags_WidthFixed, 110);
                            ImGui::TableSetupColumn("Ten Hien Thi", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("Mo Ta Chi Tiet", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Thao Tac", ImGuiTableColumnFlags_WidthFixed, 102);
                            ImGui::TableHeadersRow();

                            if (filteredGT.empty()) {
                                RenderCommonEmptyRow(5, "Khong tim thay gioi tinh nao!");
                            } else {
                                int offset = pagGioiTinh.getOffset();
                                int pSize = pagGioiTinh.getPageSize();
                                int endIdx = (offset + pSize < (int)filteredGT.size()) ? (offset + pSize) : (int)filteredGT.size();

                                for (int i = offset; i < endIdx; ++i) {
                                    const GioiTinh& gt = filteredGT[i];
                                    ImGui::TableNextRow();

                                    ImGui::TableSetColumnIndex(0);
                                    char rId[32]; sprintf(rId, "##row_gtr_%d", i); RenderRowHover(rId);
                                    AlignAndRenderCenter(to_string(i + 1));

                                    ImGui::TableSetColumnIndex(1);
                                    AlignAndRenderCenter(gt.maGioiTinh, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

                                    ImGui::TableSetColumnIndex(2);
                                    AlignAndRenderCenter(gt.tenGioiTinh, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                                    ImGui::TableSetColumnIndex(3);
                                    AlignAndRenderTextLeft(gt.moTa);

                                    ImGui::TableSetColumnIndex(4);
                                    bool vClk = false, eClk = false, dClk = false;
                                    RenderCommonActionCell(gt.maGioiTinh, vClk, eClk, dClk, true, true, true, "Xem chi tiet gioi tinh", "Sua thong tin gioi tinh", "Xoa gioi tinh");
                                    if (vClk) {
                                        viewGioiTinhData = gt;
                                        showViewGioiTinhModal = true;
                                    }
                                    if (eClk) {
                                        strcpy(editGioiTinhMa, gt.maGioiTinh.c_str());
                                        strcpy(editGioiTinhTen, gt.tenGioiTinh.c_str());
                                        strcpy(editGioiTinhDesc, gt.moTa.c_str());
                                        editGioiTinhError = "";
                                        showEditGioiTinhModal = true;
                                    }
                                    if (dClk) {
                                        if (ql.xoaGioiTinh(gt.maGioiTinh)) {
                                            showToast = true;
                                            toastMsg = "Da xoa gioi tinh " + gt.maGioiTinh + " thanh cong!";
                                        } else {
                                            showToast = true;
                                            toastMsg = "Khong the xoa vi co thi sinh dang tham chieu den gioi tinh nay!";
                                        }
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }

                        // 4. BOTTOM PAGINATION BAR
                        RenderCommonPaginationBar("pagGioiTinh", pagGioiTinh);

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

        // ==================== MODAL: THÊM KHOA ====================
        if (showAddKhoaModal) {
            ImGui::OpenPopup("Them Khoa Dao Tao Moi");
        }
        if (ImGui::BeginPopupModal("Them Khoa Dao Tao Moi", &showAddKhoaModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THEM KHOA DAO TAO DUT MOI");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Khoa (PK - VD: CNTT, DIEN, CKHI)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addMaK", addKhoaMa, sizeof(addKhoaMa));

            ImGui::Spacing();
            ImGui::Text("Ten Khoa Dao Tao*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addTenK", addKhoaTen, sizeof(addKhoaTen));

            ImGui::Spacing();
            ImGui::Text("Ma Khoa Chuan DUT (VD: 102, 105, 101)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addMaDUTK", addKhoaMaDUT, sizeof(addKhoaMaDUT));

            if (!addKhoaError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", addKhoaError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Them Khoa", ImVec2(120, 30))) {
                string ma = Utils::trim(addKhoaMa);
                string ten = Utils::trim(addKhoaTen);
                string dut = Utils::trim(addKhoaMaDUT);
                if (ma.empty() || ten.empty()) {
                    addKhoaError = "Ma khoa va ten khoa khong duoc de trong!";
                } else if (ql.timViTriKhoa(ma) >= 0) {
                    addKhoaError = "Ma khoa da ton tai trong he thong!";
                } else {
                    Khoa k(ma, ten, dut);
                    if (ql.themKhoa(k)) {
                        showAddKhoaModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them khoa dao tao thanh cong!";
                    } else {
                        addKhoaError = "Khong the them khoa vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##addK", ImVec2(100, 30))) {
                showAddKhoaModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: SỬA KHOA ====================
        if (showEditKhoaModal) {
            ImGui::OpenPopup("Cap Nhat Khoa Dao Tao");
        }
        if (ImGui::BeginPopupModal("Cap Nhat Khoa Dao Tao", &showEditKhoaModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT KHOA DAO TAO (MA: %s)", editKhoaMa);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Khoa (PK):");
            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editMaK", editKhoaMa, sizeof(editKhoaMa));
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Text("Ten Khoa Dao Tao*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editTenK", editKhoaTen, sizeof(editKhoaTen));

            ImGui::Spacing();
            ImGui::Text("Ma Khoa Chuan DUT (VD: 102, 105...)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editMaDUTK", editKhoaMaDUT, sizeof(editKhoaMaDUT));

            if (!editKhoaError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", editKhoaError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Luu Thay Doi##editK", ImVec2(130, 30))) {
                string ten = Utils::trim(editKhoaTen);
                string dut = Utils::trim(editKhoaMaDUT);
                if (ten.empty()) {
                    editKhoaError = "Ten khoa khong duoc de trong!";
                } else {
                    Khoa k(editKhoaMa, ten, dut);
                    if (ql.capNhatKhoa(k)) {
                        showEditKhoaModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat khoa thanh cong!";
                    } else {
                        editKhoaError = "Khong the cap nhat khoa vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##editK", ImVec2(100, 30))) {
                showEditKhoaModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM KHOA ====================
        if (showViewKhoaModal) {
            ImGui::OpenPopup("Chi Tiet Khoa Dao Tao");
        }
        if (ImGui::BeginPopupModal("Chi Tiet Khoa Dao Tao", &showViewKhoaModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THONG TIN KHOA DAO TAO DUT");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BulletText("Ma Khoa (PK)         : %s", viewKhoaData.maKhoa.c_str());
            ImGui::BulletText("Ten Khoa Dao Tao     : %s", viewKhoaData.tenKhoa.c_str());
            ImGui::BulletText("Ma Dinh Danh DUT (MSV): %s", viewKhoaData.maKhoaTheoChuanDUT.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Dong##viewK", ImVec2(100, 30))) {
                showViewKhoaModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: THÊM NGÀNH ====================
        if (showAddNganhModal) {
            ImGui::OpenPopup("Them Nganh Dao Tao Moi");
        }
        if (ImGui::BeginPopupModal("Them Nganh Dao Tao Moi", &showAddNganhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THEM NGANH DAO TAO DUT MOI");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Nganh (PK - Chuan Bo GD&DT)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addMaN", addNganhMa, sizeof(addNganhMa));

            ImGui::Spacing();
            ImGui::Text("Ten Nganh Dao Tao*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addTenN", addNganhTen, sizeof(addNganhTen));

            ImGui::Spacing();
            ImGui::Text("Khoa Truc Thuoc (FK)*:");
            ImGui::SetNextItemWidth(320);
            if (ImGui::BeginCombo("##addKhoaN", addNganhKhoa.c_str())) {
                for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                    const Khoa& kh = ql.getDanhSachKhoa()[k];
                    bool isSel = (addNganhKhoa == kh.maKhoa);
                    if (ImGui::Selectable(kh.maKhoa.c_str(), isSel)) addNganhKhoa = kh.maKhoa;
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("Tien To Lop (VD: TCLC_KHDL, TDH, T_DT)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addTienToN", addNganhTienTo, sizeof(addNganhTienTo));

            if (!addNganhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", addNganhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Them Nganh", ImVec2(120, 30))) {
                string ma = Utils::trim(addNganhMa);
                string ten = Utils::trim(addNganhTen);
                string tt = Utils::trim(addNganhTienTo);
                if (ma.empty() || ten.empty()) {
                    addNganhError = "Ma nganh va ten nganh khong duoc de trong!";
                } else if (ql.timViTriDanhMucNganh(ma) >= 0) {
                    addNganhError = "Ma nganh da ton tai!";
                } else {
                    DanhMucNganh dmn(ma, ten, addNganhKhoa, tt);
                    if (ql.themDanhMucNganh(dmn)) {
                        showAddNganhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them nganh dao tao thanh cong!";
                    } else {
                        addNganhError = "Khong the them nganh vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##addN", ImVec2(100, 30))) {
                showAddNganhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: SỬA NGÀNH ====================
        if (showEditNganhModal) {
            ImGui::OpenPopup("Cap Nhat Nganh Dao Tao");
        }
        if (ImGui::BeginPopupModal("Cap Nhat Nganh Dao Tao", &showEditNganhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT NGANH DAO TAO (MA: %s)", editNganhMa);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Nganh (PK):");
            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editMaN", editNganhMa, sizeof(editNganhMa));
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Text("Ten Nganh Dao Tao*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editTenN", editNganhTen, sizeof(editNganhTen));

            ImGui::Spacing();
            ImGui::Text("Khoa Truc Thuoc (FK)*:");
            ImGui::SetNextItemWidth(320);
            if (ImGui::BeginCombo("##editKhoaN", editNganhKhoa.c_str())) {
                for (size_t k = 0; k < ql.getDanhSachKhoa().size(); ++k) {
                    const Khoa& kh = ql.getDanhSachKhoa()[k];
                    bool isSel = (editNganhKhoa == kh.maKhoa);
                    if (ImGui::Selectable(kh.maKhoa.c_str(), isSel)) editNganhKhoa = kh.maKhoa;
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("Tien To Lop*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##editTienToN", editNganhTienTo, sizeof(editNganhTienTo));

            if (!editNganhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", editNganhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Luu Thay Doi##editN", ImVec2(130, 30))) {
                string ten = Utils::trim(editNganhTen);
                string tt = Utils::trim(editNganhTienTo);
                if (ten.empty()) {
                    editNganhError = "Ten nganh khong duoc de trong!";
                } else {
                    DanhMucNganh dmn(editNganhMa, ten, editNganhKhoa, tt);
                    if (ql.capNhatDanhMucNganh(dmn)) {
                        showEditNganhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Cap nhat nganh thanh cong!";
                    } else {
                        editNganhError = "Khong the cap nhat nganh vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##editN", ImVec2(100, 30))) {
                showEditNganhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM NGÀNH ====================
        if (showViewNganhModal) {
            ImGui::OpenPopup("Chi Tiet Nganh Dao Tao");
        }
        if (ImGui::BeginPopupModal("Chi Tiet Nganh Dao Tao", &showViewNganhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THONG TIN NGANH DAO TAO DUT");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BulletText("Ma Nganh (PK) : %s", viewNganhData.maNganh.c_str());
            ImGui::BulletText("Ten Nganh     : %s", viewNganhData.tenNganh.c_str());
            ImGui::BulletText("Khoa          : %s (%s)", viewNganhData.maKhoa.c_str(), ql.layTenKhoa(viewNganhData.maKhoa).c_str());
            ImGui::BulletText("Tien To Lop   : %s", viewNganhData.tienToLop.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Dong##viewN", ImVec2(100, 30))) {
                showViewNganhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: THÊM NTK ====================
        if (showAddNTKModal) {
            ImGui::OpenPopup("Them Chi Tieu Nganh Theo Khoa");
        }
        if (ImGui::BeginPopupModal("Them Chi Tieu Nganh Theo Khoa", &showAddNTKModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THEM CHI TIEU NGANH CHO KHOA %d", addNTKNam);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Chon Nganh Dao Tao*:");
            ImGui::SetNextItemWidth(320);
            if (ImGui::BeginCombo("##addNTKMaCombo", (addNTKMa + " - " + ql.layTenNganh(addNTKMa)).c_str())) {
                for (size_t i = 0; i < ql.getDanhSachDanhMucNganh().size(); ++i) {
                    const DanhMucNganh& dmn = ql.getDanhSachDanhMucNganh()[i];
                    string label = dmn.maNganh + " - " + dmn.tenNganh;
                    bool isSel = (addNTKMa == dmn.maNganh);
                    if (ImGui::Selectable(label.c_str(), isSel)) addNTKMa = dmn.maNganh;
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("Chi Tieu Tuyen Sinh (sinh vien)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputInt("##addNTKCT", &addNTKChiTieu);
            if (addNTKChiTieu < 1) addNTKChiTieu = 1;

            ImGui::Spacing();
            ImGui::Text("Diem San Toi Thieu (thang 30)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputFloat("##addNTKDS", &addNTKDiemSan, 0.25f, 1.0f, "%.2f");
            if (addNTKDiemSan < 0.0f) addNTKDiemSan = 0.0f;
            if (addNTKDiemSan > 30.0f) addNTKDiemSan = 30.0f;

            if (!addNTKError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", addNTKError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Them Chi Tieu", ImVec2(130, 30))) {
                if (addNTKMa.empty()) {
                    addNTKError = "Vui long chon nganh dao tao!";
                } else if (ql.timViTriNganhTheoKhoaHoc(addNTKNam, addNTKMa) >= 0) {
                    addNTKError = "Nganh nay da co chi tieu trong khoa " + to_string(addNTKNam) + "!";
                } else {
                    NganhTheoKhoaHoc ntk(addNTKNam, addNTKMa, addNTKChiTieu, 0.0, addNTKDiemSan);
                    if (ql.themNganhTheoKhoaHoc(ntk)) {
                        showAddNTKModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them chi tieu nganh thanh cong!";
                    } else {
                        addNTKError = "Khong the them chi tieu vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##addNTK", ImVec2(100, 30))) {
                showAddNTKModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: SỬA NTK ====================
        if (showEditNTKModal) {
            ImGui::OpenPopup("Cap Nhat Chi Tieu & Diem San");
        }
        if (ImGui::BeginPopupModal("Cap Nhat Chi Tieu & Diem San", &showEditNTKModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAP NHAT NGANH %s - KHOA %d", editNTKMa.c_str(), editNTKNam);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ten Nganh: %s", ql.layTenNganh(editNTKMa).c_str());
            ImGui::Spacing();

            ImGui::Text("Chi Tieu Tuyen Sinh (sinh vien)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputInt("##editNTKCT", &editNTKChiTieu);
            if (editNTKChiTieu < 1) editNTKChiTieu = 1;

            ImGui::Spacing();
            ImGui::Text("Diem San Toi Thieu (thang 30)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputFloat("##editNTKDS", &editNTKDiemSan, 0.25f, 1.0f, "%.2f");
            if (editNTKDiemSan < 0.0f) editNTKDiemSan = 0.0f;
            if (editNTKDiemSan > 30.0f) editNTKDiemSan = 30.0f;

            if (!editNTKError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", editNTKError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Luu Thay Doi##editNTK", ImVec2(130, 30))) {
                if (ql.capNhatChiTieuVaDiemSan(editNTKNam, editNTKMa, editNTKChiTieu, editNTKDiemSan)) {
                    showEditNTKModal = false;
                    ImGui::CloseCurrentPopup();
                    showToast = true;
                    toastMsg = "Cap nhat chi tieu & diem san thanh cong!";
                } else {
                    editNTKError = "Khong the cap nhat vao CSDL!";
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##editNTK", ImVec2(100, 30))) {
                showEditNTKModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM NTK ====================
        if (showViewNTKModal) {
            ImGui::OpenPopup("Chi Tiet Chi Tieu & Diem Chuan");
        }
        if (ImGui::BeginPopupModal("Chi Tiet Chi Tieu & Diem Chuan", &showViewNTKModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THONG TIN CHI TIEU & DIEM CHUAN NGANH");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BulletText("Khoa / Nam Tuyen Sinh: %d (K%d)", viewNTKData.namTuyenSinh, viewNTKData.namTuyenSinh % 100);
            ImGui::BulletText("Ma Nganh             : %s", viewNTKData.maNganh.c_str());
            ImGui::BulletText("Ten Nganh            : %s", ql.layTenNganh(viewNTKData.maNganh).c_str());
            ImGui::BulletText("Khoa Dao Tao         : %s", ql.layTenKhoa(ql.layMaKhoaCuaNganh(viewNTKData.maNganh)).c_str());
            ImGui::BulletText("Chi Tieu Tuyen Sinh  : %d chi tieu", viewNTKData.chiTieu);
            ImGui::BulletText("Diem San Toi Thieu   : %.2f diem", viewNTKData.diemSanToiThieu);
            if (viewNTKData.diemChuanChot > 0.0) {
                ImGui::BulletText("Diem Chuan Chot      : %.2f diem", viewNTKData.diemChuanChot);
            } else {
                ImGui::BulletText("Diem Chuan Chot      : Chua chot");
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Dong##viewNTK", ImVec2(100, 30))) {
                showViewNTKModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: THÊM TỈNH THÀNH ====================
        if (showAddTinhThanhModal) {
            ImGui::OpenPopup("Them Tinh / Thanh Pho Moi");
        }
        if (ImGui::BeginPopupModal("Them Tinh / Thanh Pho Moi", &showAddTinhThanhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THEM TINH / THANH PHO MOI");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Tinh / Thanh (PK - VD: 48, 01)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addMaTT", addTinhThanhMa, sizeof(addTinhThanhMa));

            ImGui::Spacing();
            ImGui::Text("Ten Tinh / Thanh Pho*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addTenTT", addTinhThanhTen, sizeof(addTinhThanhTen));

            ImGui::Spacing();
            ImGui::Text("Phan Loai (VD: Thanh pho Trung uong, Tinh)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addLoaiTT", addTinhThanhLoai, sizeof(addTinhThanhLoai));

            if (!addTinhThanhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", addTinhThanhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Them Tinh Thanh", ImVec2(140, 30))) {
                string ma = Utils::trim(addTinhThanhMa);
                string ten = Utils::trim(addTinhThanhTen);
                string loai = Utils::trim(addTinhThanhLoai);
                if (ma.empty() || ten.empty()) {
                    addTinhThanhError = "Ma tinh va ten tinh khong duoc de trong!";
                } else if (ql.timViTriTinhThanh(ma) >= 0) {
                    addTinhThanhError = "Ma tinh thanh da ton tai!";
                } else {
                    TinhThanh tt(ma, ten, loai);
                    if (ql.themTinhThanh(tt)) {
                        showAddTinhThanhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them tinh thanh thanh cong!";
                    } else {
                        addTinhThanhError = "Khong the them tinh thanh vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##addTT", ImVec2(100, 30))) {
                showAddTinhThanhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM TỈNH THÀNH ====================
        if (showViewTinhThanhModal) {
            ImGui::OpenPopup("Chi Tiet Tinh / Thanh Pho");
        }
        if (ImGui::BeginPopupModal("Chi Tiet Tinh / Thanh Pho", &showViewTinhThanhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THONG TIN TINH / THANH PHO");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BulletText("Ma Tinh (PK)    : %s", viewTinhThanhData.maTinh.c_str());
            ImGui::BulletText("Ten Tinh / TP   : %s", viewTinhThanhData.tenTinh.c_str());
            ImGui::BulletText("Phan Loai       : %s", viewTinhThanhData.loai.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Dong##viewTT", ImVec2(100, 30))) {
                showViewTinhThanhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: THÊM GIỚI TÍNH ====================
        if (showAddGioiTinhModal) {
            ImGui::OpenPopup("Them Gioi Tinh Moi");
        }
        if (ImGui::BeginPopupModal("Them Gioi Tinh Moi", &showAddGioiTinhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THEM GIOI TINH MOI");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Ma Gioi Tinh (PK - VD: NAM, NU, KHAC)*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addMaGT", addGioiTinhMa, sizeof(addGioiTinhMa));

            ImGui::Spacing();
            ImGui::Text("Ten Hien Thi*:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addTenGT", addGioiTinhTen, sizeof(addGioiTinhTen));

            ImGui::Spacing();
            ImGui::Text("Ghi Chu Mo Ta:");
            ImGui::SetNextItemWidth(320);
            ImGui::InputText("##addDescGT", addGioiTinhDesc, sizeof(addGioiTinhDesc));

            if (!addGioiTinhError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[!] %s", addGioiTinhError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Them Gioi Tinh", ImVec2(130, 30))) {
                string ma = Utils::trim(addGioiTinhMa);
                string ten = Utils::trim(addGioiTinhTen);
                string desc = Utils::trim(addGioiTinhDesc);
                if (ma.empty() || ten.empty()) {
                    addGioiTinhError = "Ma va ten gioi tinh khong duoc de trong!";
                } else if (ql.timViTriGioiTinh(ma) >= 0) {
                    addGioiTinhError = "Ma gioi tinh da ton tai!";
                } else {
                    GioiTinh gt(ma, ten, desc);
                    if (ql.themGioiTinh(gt)) {
                        showAddGioiTinhModal = false;
                        ImGui::CloseCurrentPopup();
                        showToast = true;
                        toastMsg = "Them gioi tinh thanh cong!";
                    } else {
                        addGioiTinhError = "Khong the them gioi tinh vao CSDL!";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Huy Bo##addGT", ImVec2(100, 30))) {
                showAddGioiTinhModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ==================== MODAL: XEM GIỚI TÍNH ====================
        if (showViewGioiTinhModal) {
            ImGui::OpenPopup("Chi Tiet Gioi Tinh");
        }
        if (ImGui::BeginPopupModal("Chi Tiet Gioi Tinh", &showViewGioiTinhModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THONG TIN GIOI TINH");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BulletText("Ma Gioi Tinh (PK): %s", viewGioiTinhData.maGioiTinh.c_str());
            ImGui::BulletText("Ten Hien Thi     : %s", viewGioiTinhData.tenGioiTinh.c_str());
            ImGui::BulletText("Mo Ta            : %s", viewGioiTinhData.moTa.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Dong##viewGT", ImVec2(100, 30))) {
                showViewGioiTinhModal = false;
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
