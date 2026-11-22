#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
VERSION="$(sed -nE 's/.*project\(CaptureForge VERSION ([0-9.]+).*/\1/p' CMakeLists.txt | head -n1)"
test -n "$VERSION" || { echo "Missing CMake project version" >&2; exit 1; }
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/opt/captureforge" "$STAGE/DEBIAN" "$STAGE/usr/share/applications" "$STAGE/usr/share/icons/hicolor/64x64/apps" installer/linux
install -m 755 build-ubuntu/CaptureForge "$STAGE/opt/captureforge/CaptureForge"
install -m 644 resources/camera.png "$STAGE/opt/captureforge/camera.png"
install -m 644 resources/camera.png "$STAGE/usr/share/icons/hicolor/64x64/apps/captureforge.png"
cat > "$STAGE/usr/share/applications/captureforge.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=CaptureForge
Comment=Capture, annotate and recognise text
Exec=/opt/captureforge/CaptureForge
Icon=captureforge
Terminal=false
Categories=Graphics;Utility;
StartupNotify=true
EOF
cat > "$STAGE/DEBIAN/control" <<EOF
Package: captureforge
Version: $VERSION
Section: graphics
Priority: optional
Architecture: amd64
Maintainer: SACUAR Tools <noreply@github.com>
Depends: libqt6widgets6, libqt6network6, libqt6svg6, libqt6concurrent6, tesseract-ocr, tesseract-ocr-eng
Description: Screen capture, annotations and OCR for Ubuntu X11
 Native Qt 6 capture and image tools. Wayland region capture is restricted.
EOF
OUTPUT="installer/linux/CaptureForge-${VERSION}-ubuntu24.04-amd64.deb"
dpkg-deb --build --root-owner-group "$STAGE" "$OUTPUT"
sha256sum "$OUTPUT" > "$OUTPUT.sha256"
dpkg-deb --info "$OUTPUT"
