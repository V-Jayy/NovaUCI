@echo off
setlocal

echo ============================================
echo   Nova Chess Engine - Build Script
echo ============================================
echo.

:: Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

:: Ensure build directory exists
if not exist "build" mkdir build

:: Source files
set SOURCES=src\main.cpp src\attacks.cpp src\board.cpp src\movegen.cpp src\eval.cpp src\search.cpp src\book.cpp src\see.cpp

echo [*] Compiling Nova with MSVC (C++20, /O2)...
echo.

cl /std:c++20 /O2 /EHsc /W4 /Fe:build\nova.exe /I src %SOURCES%

if %ERRORLEVEL% neq 0 (
    echo.
    echo [!] BUILD FAILED
    exit /b 1
)

echo.
echo [+] Build successful: build\nova.exe
echo.

:: Cleanup obj files from root
del *.obj >nul 2>&1

endlocal
