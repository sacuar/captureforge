@echo off
cd /d "%~dp0"
if exist build\CaptureForge.sln start "" build\CaptureForge.sln
if not exist build\CaptureForge.sln (
 echo Run BUILD.bat once first to create the Visual Studio solution.
 pause
)
