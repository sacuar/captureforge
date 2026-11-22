CAPTUREFORGE WORKING v2

This rebuild fixes the two source errors from the previous package:
1. Controller.cpp now includes QClipboard.
2. The undeclared/unimplemented ImageEditor::drawMode slot was removed.
It also uses a robust Windows .rc path and no longer references a nonexistent Qt resource URL.

FIRST BUILD
Run CLEAN_BUILD.bat once.
Later use BUILD.bat for faster incremental builds.

MAIN FILES TO EDIT

src/Controller.cpp
  App coordination, global shortcuts, tray, floating capture button, OCR handoff.

src/CaptureOverlay.cpp
  Drag-region screen capture and overlay geometry.

src/editors/ImageEditor.cpp
  Image editing, save/export, pen, crop, rotate, flip, undo, OCR.

src/editors/TextEditor.cpp
  OCR text editor / Notepad-like functionality.

src/settings/SettingsDialog.cpp
  Keyboard shortcut settings, theme and floating-button setting.

src/themes/ThemeManager.cpp
  Dark, Graphite and Light appearance.

CMakeLists.txt
  Add new .cpp/.h files and Qt modules here.

windows.rc + resources/captureforge.ico
  Windows EXE icon.

ocr.ps1
  Windows OCR helper. BUILD.bat copies it beside the EXE.

FUTURE MODULES
  src/video/VideoRecorder.*
  src/video/VideoPlayer.*
  src/editors/tools/CropTool.*
  src/editors/tools/TextTool.*
  src/editors/tools/ArrowTool.*
  src/editors/tools/BlurTool.*
  src/plugins/PluginManager.*

Do not reinstall Qt/CMake/MSVC for ordinary C++ compiler/linker errors.
