# 1.0.3 (candidate — validation pending)

- Launch directly into the tray and optional compact camera sidebar; no automatic home window.
- A dedicated sidebar close button hides only the sidebar, with a tray toggle to restore it.
- Draw distinct built-in icons even when external Material SVG assets are absent.
- Opt-in, memory-only text clipboard history panel, including copy selected and clear.
- Image editor sizes to its content; four-corner adjustable projective document straighten with undo.
- Windows WIA scanner acquisition to the editor; network scanners require a Windows-compatible WIA driver.
- Settings for clipboard history privacy and editor toolbar label visibility.
- Release workflow fixes and pull-request Windows build checks.

Known limitations: Windows scanner detection is WIA-dependent; no network protocol auto-discovery, no image clipboard history.
See CI logs and Windows real-device verification before calling this a stable release.

# 1.0.2

- Compact image and text editor toolbars: 18 px icons, tighter spacing, hover tooltips and optional text labels via right-click.
- Compact launcher: Capture, OCR and Text editor grouped in one row; Settings and More below.
- Retain all existing capture, editing, OCR and add-on functionality.

# 1.0.1

- Optional installation maintenance wizard and Start menu tool.
- Search old builds, installers, folders, running copies and app registrations.
- Explicit Recycle Bin removal of selected old compiled folders, with installation, source project and link protection.
- Backup before removal of eligible missing-target startup entries.
- Single-instance lock and activation of existing window.
- Retain bundled Tesseract recognition test.

# CaptureForge 1.0.0 development candidate
- Added a normal desktop launcher with capture, OCR, settings, add-ons and updates.
- Added bundled Tesseract OCR with explicit language-data path and Windows OCR fallback.
- OCR runs asynchronously; bounded process timeout and temporary file cleanup.
- Installer performs genuine English text recognition test.
- Refined Google Material toolbar icons, compact controls and rounded scrollbars.
- Settings OCR diagnostics, reset, startup update toggle and trusted DLL loading preference.
- Add-on status manager and user-writable add-on directory.
- GitHub release notes, SHA-256 validated installer updates and Windows installer workflow.
- Retained original screenshot and annotation editors, tray and hotkeys.
