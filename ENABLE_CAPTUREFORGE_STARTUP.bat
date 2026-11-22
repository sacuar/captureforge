@echo off
setlocal
set "APP=%~dp0CaptureForge.exe"

if not exist "%APP%" (
    echo.
    echo ERROR: CaptureForge.exe was not found in:
    echo %~dp0
    echo.
    echo Put this BAT file in the same folder as CaptureForge.exe, then run it again.
    echo.
    pause
    exit /b 1
)

reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "CaptureForge" /t REG_SZ /d "\"%APP%\"" /f >nul

if errorlevel 1 (
    echo.
    echo Could not enable startup.
    echo.
    pause
    exit /b 1
)

echo.
echo CaptureForge is now set to run automatically when you sign in to Windows.
echo No shortcut and no administrator permission are required.
echo.
pause
