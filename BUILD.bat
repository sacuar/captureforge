@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title CaptureForge Working v4 Build

echo ============================================================
echo  CaptureForge Working v4 - Qt 6 / MSVC 2022
echo ============================================================

set "QTROOT=C:\Qt"
set "QTV="
for /f "delims=" %%D in ('dir /b /ad "%QTROOT%\6.*" 2^>nul ^| sort /r') do if not defined QTV set "QTV=%%D"

if not defined QTV (
 echo [ERROR] No Qt 6 installation found under C:\Qt
 goto fail
)

set "QTDIR=%QTROOT%\%QTV%\msvc2022_64"
if not exist "%QTDIR%\bin\qmake.exe" (
 echo [ERROR] Missing: %QTDIR%\bin\qmake.exe
 goto fail
)

echo Qt: %QTDIR%
if not exist build mkdir build

echo.
echo [1/3] Configure...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QTDIR%" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if errorlevel 1 goto fail

echo.
echo [2/3] Compile...
cmake --build build --config Release --parallel
if errorlevel 1 goto fail

echo.
echo [3/3] Deploy Qt...
"%QTDIR%\bin\windeployqt.exe" --release --no-translations "build\Release\CaptureForge.exe"
if errorlevel 1 goto fail
copy /y "ocr.ps1" "build\Release\ocr.ps1" >nul
copy /y "resources\camera.png" "build\Release\camera.png" >nul
if not exist "build\Release\plugins" mkdir "build\Release\plugins"
copy /y "src\plugins\README.txt" "build\Release\plugins\README.txt" >nul

echo.
echo ============================================================
echo  SUCCESS
echo  EXE: %CD%\build\Release\CaptureForge.exe
echo ============================================================
start "" "%CD%\build\Release\CaptureForge.exe"
pause
exit /b 0

:fail
echo.
echo ============================================================
echo  BUILD FAILED - copy the FIRST error line into ChatGPT.
echo ============================================================
pause
exit /b 1
