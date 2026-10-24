@echo off
chcp 65001 > nul
cls
echo ==============================================================================
echo          CHUONG TRINH BIEN DICH VA CHAY - DO AN PBL2 (DE TAI 24)
echo     [ Kien truc CSDL Quan he 3NF + Giao dien Do hoa Dear ImGui DirectX 9 ]
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
echo [*] Dang bien dich toan bo ma nguon (OOP, CSDL 3NF, GUI ImGui DirectX 9)...
echo.

"%CXX%" -std=c++11 -O2 -Iinclude -Iinclude/imgui ^
    src\Nguoi.cpp src\ThiSinh.cpp src\KhuVuc.cpp src\NganhXetTuyen.cpp ^
    src\QuanLyThiSinh.cpp src\ConsoleUI.cpp src\Utils.cpp src\AppGUI.cpp ^
    src\imgui\imgui.cpp src\imgui\imgui_draw.cpp src\imgui\imgui_tables.cpp ^
    src\imgui\imgui_widgets.cpp src\imgui\imgui_impl_win32.cpp src\imgui\imgui_impl_dx9.cpp ^
    src\main.cpp ^
    -ld3d9 -lgdi32 -ldwmapi -o QuanLyThiSinh.exe

if %ERRORLEVEL% equ 0 (
    echo.
    echo ==============================================================================
    echo [THANH CONG] Da bien dich xong file chuong trinh: QuanLyThiSinh.exe
    echo ==============================================================================
    echo.
    echo [*] Dang khoi chay chuong trinh Giao dien Do hoa GUI...
    pause
    start "" QuanLyThiSinh.exe
) else (
    echo.
    echo [LOI] Co loi xay ra trong qua trinh bien dich!
    pause
)
