@echo off
chcp 65001 > nul
cls
echo ==============================================================================
echo          CHUONG TRINH BIEN DICH VA CHAY - DO AN PBL2 (DE TAI 24)
echo     [ Kien truc CSDL Quan he 2 Bang (Class Table) + Giao dien Dear ImGui / Console ]
echo ==============================================================================
echo.

:: Kiem tra trinh bien dich g++ trong PATH
where g++ >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set CXX=g++
    goto COMPILE
)

:: Kiem tra cac duong dan Dev-Cpp va MinGW pho bien
if exist "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" (
    set "CXX=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
    goto COMPILE
)
if exist "C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe" (
    set "CXX=C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe"
    goto COMPILE
)
if exist "C:\Dev-Cpp\MinGW64\bin\g++.exe" (
    set "CXX=C:\Dev-Cpp\MinGW64\bin\g++.exe"
    goto COMPILE
)
if exist "C:\MinGW\bin\g++.exe" (
    set "CXX=C:\MinGW\bin\g++.exe"
    goto COMPILE
)
if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "CXX=C:\msys64\mingw64\bin\g++.exe"
    goto COMPILE
)
if exist "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" (
    set "CXX=C:\Program Files\CodeBlocks\MinGW\bin\g++.exe"
    goto COMPILE
)

echo [LOI] Khong tim thay trinh bien dich g++ tren may tinh!
echo.
pause
exit /b 1

:COMPILE
:: Trich xuat thu muc bin va them vao PATH de trinh bien dich tim thay cac DLL
for %%F in ("%CXX%") do set "COMPILER_BIN=%%~dpF"
set "PATH=%COMPILER_BIN%;%PATH%"

echo [*] Trinh bien dich : %CXX%
echo [*] Dang bien dich toan bo ma nguon (OOP, CSDL 2 Bang, GUI ImGui DirectX 9 / Console)...
echo.

"%CXX%" -std=c++11 -O2 -Iinclude -Iinclude/imgui ^
    src\Nguoi.cpp src\ThiSinh.cpp ^
    src\QuanLyThiSinh.cpp src\ConsoleUI.cpp src\Utils.cpp src\AppGUI.cpp ^
    src\imgui\imgui.cpp src\imgui\imgui_draw.cpp src\imgui\imgui_tables.cpp ^
    src\imgui\imgui_widgets.cpp src\imgui\imgui_impl_win32.cpp src\imgui\imgui_impl_dx9.cpp ^
    src\main.cpp ^
    -ld3d9 -lgdi32 -ldwmapi -o QuanLyThiSinh_PBL2.exe

if %ERRORLEVEL% equ 0 (
    echo.
    echo ==============================================================================
    echo [THANH CONG] Da bien dich xong file chuong trinh: QuanLyThiSinh_PBL2.exe
    echo ==============================================================================
    echo.
    echo [1] Chay che do Giao dien Do hoa (Dear ImGui DirectX 9)
    echo [2] Chay che do Console UTF-8 dieu huong bang phim mui ten
    echo.
    set /p choice="Nhap lua chon cua ban (1 hoac 2, mac dinh 1): "
    if "%choice%"=="2" (
        cls
        QuanLyThiSinh_PBL2.exe --console
    ) else (
        start "" QuanLyThiSinh_PBL2.exe
    )
) else (
    echo.
    echo [LOI] Co loi xay ra trong qua trinh bien dich!
    pause
)
