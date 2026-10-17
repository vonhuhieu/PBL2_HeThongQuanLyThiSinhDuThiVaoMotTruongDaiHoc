@echo off
chcp 65001 > nul
cls
echo ==============================================================================
echo        CAI DAT TRINH BIEN DICH DEV-C++ (CHO DO AN PBL2)
echo ==============================================================================
echo.
echo [*] Thao tac nay se khoi dong trinh cai dat Dev-C++ 5.9.2 (TDM-GCC 4.8.1).
echo [*] Huong dan cai dat:
echo     1. Chon ngon ngu cai dat (Mac dinh la English) -> Bam OK.
echo     2. Bam "I Agree" de dong y dieu khoan.
echo     3. De nguyen cac tuy chon mac dinh -> Bam "Next".
echo     4. Bam "Install" va cho khoang 1-2 phut de cai dat hoan tat.
echo.
echo ==============================================================================
pause

if exist "tools\DevCpp_Setup.exe" (
    echo [*] Dang khoi dong trinh cai dat tu tools\DevCpp_Setup.exe...
    start "" "tools\DevCpp_Setup.exe"
    echo.
    echo [THANH CONG] Cua so cai dat da duoc bat len tren man hinh!
    echo Vui long hoan tat cac buoc tren trinh cai dat.
) else (
    echo [LOI] Khong tim thay file tools\DevCpp_Setup.exe!
)
echo.
pause
