@echo off
REM Simple build script for Windows to avoid long path issues
setlocal
set SRC=%~dp0
set SRC_SHORT=%~sdp0
set BUILD=%SRC_SHORT%build
if exist "%BUILD%" rmdir /s /q "%BUILD%"
mkdir "%BUILD%"
pushd "%BUILD%"
cmake -G Ninja "%SRC_SHORT%"
if errorlevel 1 exit /b %errorlevel%
ninja
popd
endlocal
