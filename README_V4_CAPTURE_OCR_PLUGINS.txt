CaptureForge v4 - Capture, Crop, OCR and Plugin Update
======================================================

BUILD ON WINDOWS
1. Close any running CaptureForge.exe.
2. Run CLEAN_BUILD.bat once (important: new C++ source files were added).
3. The app starts from build\Release\CaptureForge.exe.

WHAT TO TEST FIRST
1. Click the floating camera or press Ctrl+Shift+S.
2. Drag over a visible area of the screen and release.
3. The editor should open with exactly that selected area.
4. Click Crop, drag a smaller rectangle, release.
5. Click OCR -> Text Editor.
6. OCR text should open in the text editor and also be copied to the clipboard.

PLUGIN SYSTEM
Right-click tray icon -> Plugins:
- Reload Plugins
- Open Plugins Folder
- Plugin Status

Put compatible future plugin DLLs into build\Release\plugins.
OCR plugins implement ICaptureForgeOcrPlugin in src\plugins\PluginInterface.h.
If no OCR plugin succeeds, built-in Windows OCR is used automatically.

IF OCR STILL FAILS
The message box now shows the real provider/Windows error. Typical Windows-side issue:
a Windows display/input language exists but its OCR-capable language feature is not installed.
The screenshot/crop features do not depend on OCR and should still work.
