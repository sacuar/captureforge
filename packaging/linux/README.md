# CaptureForge on Ubuntu 24.04 x64

This native Qt/C++ Linux port targets **X11** first. On modern Ubuntu Wayland sessions, direct full-desktop screenshot access and global shortcut support are restricted. CaptureForge displays an explanation instead of silently capturing a black image. OCR with Tesseract, image editing and text editing use Qt.

## Install the .deb from a successful Ubuntu CI build

Download the `CaptureForge-Ubuntu-24.04-amd64` artifact from [GitHub Actions](https://github.com/sacuar/captureforge/actions/workflows/ubuntu-build.yml), unzip, verify its checksum, then install:

```bash
sha256sum -c CaptureForge-1.0.2-ubuntu24.04-amd64.deb.sha256
sudo apt install ./CaptureForge-1.0.2-ubuntu24.04-amd64.deb
```

Find CaptureForge in the Ubuntu Applications menu. For region capture, log in using an **Ubuntu on Xorg** session. This project does not change desktop shell defaults or install background startup services.

## Build from source

```bash
sudo apt install -y cmake ninja-build g++ git qt6-base-dev qt6-svg-dev libx11-dev libxcb1-dev libxcb-keysyms1-dev tesseract-ocr tesseract-ocr-eng
cmake -S . -B build-ubuntu -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ubuntu -j2
./build-ubuntu/CaptureForge
```

The Ubuntu build requires fetching the pinned QHotkey 1.5.0 source at CMake configure time. Native QHotkey availability and tray integration depend on the desktop environment. The Linux updater opens the official release page and never launches the Windows installer. External Qt add-ons should only be loaded when trusted and compiled for Linux.

Automated CI verifies compilation, OCR and an X11 application smoke test. Manual region capture across multiple monitors, different DPI scaling, Ubuntu desktop versions and Wayland portal functionality remain unverified.
