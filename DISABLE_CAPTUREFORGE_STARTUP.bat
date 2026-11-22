@echo off
setlocal
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "CaptureForge" /f >nul 2>&1
echo.
echo CaptureForge startup entry has been removed.
echo.
pause
