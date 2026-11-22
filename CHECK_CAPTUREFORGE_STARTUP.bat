@echo off
setlocal
echo.
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "CaptureForge" >nul 2>&1
if errorlevel 1 (
    echo CaptureForge startup: DISABLED
) else (
    echo CaptureForge startup: ENABLED
    echo.
    reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "CaptureForge"
)
echo.
pause
