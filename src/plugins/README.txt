CaptureForge plug-ins
=====================

This folder is now a live drop-in plugin folder.

HOW TO UPDATE/ADD A PLUGIN LATER
1. Close CaptureForge (recommended for replacing an existing DLL).
2. Put the compatible 64-bit Qt 6/MSVC plugin DLL in this folder beside the EXE.
3. Start CaptureForge, or use tray menu: Plugins -> Reload Plugins.
4. Use Plugins -> Plugin Status to see what loaded and any DLL errors.

PLUGIN ABI
- General plugins implement ICaptureForgePlugin from src/plugins/PluginInterface.h.
- OCR plugins implement ICaptureForgeOcrPlugin.
- A DLL may implement both interfaces.
- Build plugins with the same Qt major/minor toolchain and MSVC architecture as CaptureForge.

OCR FALLBACK
If no OCR plugin succeeds, CaptureForge automatically uses its built-in Windows OCR
provider (ocr.ps1). This means a future Tesseract, PaddleOCR, cloud OCR, or custom
OCR DLL can be added without changing Controller.cpp.

Suggested future plugins:
- Tesseract OCR
- scrolling capture
- screen/video recorder
- cloud upload
- color picker
- image filters
