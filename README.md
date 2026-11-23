<div align="center">

![CaptureForge banner](assets/brand-cover.svg)

# CaptureForge

**A Windows capture workspace for screenshots, annotations and local text recognition.**

[![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-2563eb)](https://github.com/sacuar/captureforge#install--try) [![Latest release](https://img.shields.io/github/v/release/sacuar/captureforge?label=latest%20release)](https://github.com/sacuar/captureforge/releases/latest) [![Issues](https://img.shields.io/github/issues/sacuar/captureforge)](https://github.com/sacuar/captureforge/issues)

**[DOWNLOAD / RELEASES](https://github.com/sacuar/captureforge/releases)** · **[VIEW SOURCE](https://github.com/sacuar/captureforge)** · [More SACUAR apps](https://github.com/sacuar)

</div>

<div align="center">

[![GitHub stars](https://img.shields.io/github/stars/sacuar/captureforge?style=flat&label=Stars)](https://github.com/sacuar/captureforge/stargazers) [![Windows installer downloads](https://img.shields.io/github/downloads/sacuar/captureforge/v1.0.2/CaptureForge-1.0.2-Setup-x64.exe?style=flat&label=Windows%20downloads)](https://github.com/sacuar/captureforge/releases/tag/v1.0.2) [![Ubuntu package downloads](https://img.shields.io/github/downloads/sacuar/captureforge/v1.0.2/CaptureForge-1.0.2-ubuntu24.04-amd64.deb?style=flat&label=Ubuntu%20downloads)](https://github.com/sacuar/captureforge/releases/tag/v1.0.2)

[View visitors and referral sources](https://github.com/sacuar/captureforge/graphs/traffic) · [All releases](https://github.com/sacuar/captureforge/releases)

</div>

## Why use CaptureForge?

- Capture a region or monitor, then crop, rotate, annotate and save.
- Run local OCR with Tesseract or an available Windows OCR fallback.
- Work with desktop hotkeys, a tray launcher and compact editor controls.

## Ubuntu build

**Ubuntu 24.04 x64 (X11 preview):** Native Qt/C++ `.deb` with Tesseract OCR. Screen capture targets X11; Wayland desktop capture is not supported yet and requires the appropriate portal integration. [Ubuntu install guide](packaging/linux/README.md) · [Ubuntu workflow](https://github.com/sacuar/captureforge/actions/workflows/ubuntu-build.yml).

## Install / try

Open **[GitHub Releases](https://github.com/sacuar/captureforge/releases)**, choose the latest suitable Windows asset, review its release notes and known limitations, and follow the installer instructions. If no verified installer is available, use the documented source-build instructions below. Download only from the official project release page.

**Development candidate:** Native C++/Qt Windows compilation, installer and OCR regression tests are not yet independently verified here. Review build results before installing; don't treat this page as a certified stable release.

**Licence:** No licence file was found in this audit. Public source visibility does not itself grant permission to redistribute or modify the code.

## Screenshots and first look

### Capture → annotate → recognise text

**CaptureForge brings region capture, annotation and local Tesseract OCR into one workflow.** Select a region, draw or crop in the image editor, then extract text without uploading your screenshot to a cloud OCR provider.

For an authentic view of the current application, use the [CaptureForge Windows release](https://github.com/sacuar/captureforge/releases/tag/v1.0.2) and run the editor. The GitHub Actions builds confirm application startup and OCR self-testing; they do not provide a full, verified editor screenshot yet.

> The promotional CaptureForge concept image is illustrative, not a capture of the existing interface. A genuine in-app screenshot will be added when a verified capture is available.

## Get involved

[Development notes](CHANGELOG.md) · [Report a bug](https://github.com/sacuar/captureforge/issues/new) · [Source code](https://github.com/sacuar/captureforge). Please report your Windows version, application version and steps to reproduce. Screenshots of reproducible issues help.

---

## Detailed documentation

## Features
Desktop launcher, region capture, multi-monitor selection, global hotkeys, floating camera button, system tray, image crop/pen/rotation/flip/undo, text editor, local OCR, configurable themes and trusted native add-ons. Tesseract is preferred; Windows OCR remains a fallback. Built-in OCR is processed off the UI thread. English is bundled; other language selection is not implemented.

## Install and updates
When a Windows build passes, the setup EXE is published at https://github.com/sacuar/captureforge/releases . Installer packages Qt and Tesseract, runs recognition on a generated test phrase, and reports failure. No Python or developer tools needed by end users. Updates are checked on startup (optional) or via tray menu, with release notes and SHA-256 verification before launching the installer with consent. Unsigned development binaries may trigger SmartScreen.

## Build
Install Visual Studio 2022 C++ workload, CMake, Qt 6.8.3 MSVC x64, vcpkg and Inno Setup 6. Set `QT_ROOT_DIR` to the Qt installation and `VCPKG_ROOT` to vcpkg. Run `BUILD_INSTALLER.bat`. The GitHub Actions workflow automates this and tests the installed executable. For local source development the older BUILD.bat remains available (without bundled OCR runtime). The dependency script downloads official Google Material Icons; if an individual icon is unavailable Qt provides a fallback.

## Add-ons
Tray > Add-ons > Manage add-ons. External DLLs are stored in the user app-data add-ons directory. Only load trusted DLLs compatible with the existing PluginInterface ABI; automatic loading is opt-in. Built-in Tesseract and Windows OCR work without third-party add-ons. Settings > OCR diagnostics tests the saved configuration. Add-on independent binary update catalogue is not yet implemented.

## Status
Source package prepared, Windows C++ compilation/installer/recognition tests not verified in this environment. Do not present as a certified or signed release. See THIRD_PARTY_NOTICES.md before distribution.

## Installation maintenance (1.0.1)
During setup, choose **Review previous builds → Scan and review old builds**. The same tool is installed in the Start menu as **CaptureForge maintenance**. Scan common locations, fixed drives or a specific folder; inspect registry entries and running copies. Recycle selected old files or compiled folders containing CaptureForge.exe, subject to safety checks. Installed locations, source projects and linked folders are protected; review contents first. Use Windows Installed apps to uninstall registered older versions. Stale per-user startup entries are backed up before removal. New CaptureForge launches reuse the existing session; legacy binaries may still run separately until removed. Tesseract retains its installation-time recognition test. No automatic deletion or registry sweeping.

## Compact interface (1.0.2)
Capture, OCR and Text editor are grouped in the launcher. Settings is alongside More (add-ons and updates). Editor toolbars use compact icons with tooltips by default. Right-click an editor toolbar and toggle **Show toolbar labels** to restore icon and text mode; the preference is remembered for new editor windows.
