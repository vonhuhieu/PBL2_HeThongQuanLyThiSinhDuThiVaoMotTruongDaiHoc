@echo off
chcp 65001 > nul
cls
echo ==============================================================================
echo          CHUONG TRINH DICH VA CHAY - DO AN PBL2 (DE TAI 24)
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
if exist "tools\DevCpp_Setup.exe" (
    echo [*] Phat hien bo cai dat Dev-C++ da duoc tich hop san trong thu muc tools/
    set /p INSTALL_CHOICE="Ban co muon khoi dong trinh cai dat Dev-C++ ngay bay gio khong? (Y/N): "
    if /i "%INSTALL_CHOICE%"=="Y" (
        echo [*] Dang khoi dong trinh cai dat...
        start "" "tools\DevCpp_Setup.exe"
        echo Cua so cai dat da duoc mo. Sau khi cai dat xong, vui long chay lai build.bat!
        pause
        exit /b 0
    )
)
echo Vui long cai dat Dev-C++ hoac MinGW truoc khi su dung!
pause
exit /b 1

:COMPILE
:: Trich xuat thu muc bin va them vao PATH de trinh bien dich tim thay cac DLL nhu libiconv-2.dll
for %%F in ("%CXX%") do set "COMPILER_BIN=%%~dpF"
set "PATH=%COMPILER_BIN%;%PATH%"

echo [*] Dang su dung trinh bien dich: %CXX%
echo [*] Dang bien dich cac file nguon...

"%CXX%" -std=c++11 -Iinclude src\Nguoi.cpp src\ThiSinh.cpp src\QuanLyThiSinh.cpp src\ConsoleUI.cpp src\Utils.cpp src\main.cpp -o QuanLyThiSinh.exe

if %ERRORLEVEL% equ 0 (
    echo.
    echo ==============================================================================
    echo [THANH CONG] Da bien dich xong file chuong trinh: QuanLyThiSinh.exe
    echo ==============================================================================
    echo.
    echo [*] Dang khoi chay chuong trinh...
    pause
    cls
    QuanLyThiSinh.exe
) else (
    echo.
    echo [LOI] Co loi xay ra trong qua trinh bien dich!
    pause
)
