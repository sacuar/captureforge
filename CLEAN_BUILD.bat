@echo off
cd /d "%~dp0"
if exist build rmdir /s /q build
call BUILD.bat
