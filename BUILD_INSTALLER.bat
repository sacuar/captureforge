@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File packaging\windows\build.ps1
if errorlevel 1 (echo Build failed. & pause & exit /b 1)
echo Installer is in installer directory.
pause
