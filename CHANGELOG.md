# 1.0.3 — In-place update maintenance

- Existing installation receives a verified update in the same directory with one Update and restart action.
- In-app updater uses the published GitHub SHA-256 digest and checks it again immediately before launching setup.
- Update helper waits for the running application to exit and starts Inno Setup silently with the existing installation path.
- After successful installation, CaptureForge relaunches without opening a startup launcher.
- Same Inno Setup AppId and per-user installation location are retained to avoid duplicate application registrations.
- User settings and shortcuts remain in the existing application profile.
- Preserve original camera and tray architecture; add a small camera × dismiss control and tray reopen command.
- Normal startup uses tray plus optional camera without showing a redundant home dialog.
- Failure is written to the local update-error log rather than silently creating another installation.
- This updater becomes available after upgrading from v1.0.2; v1.0.2 itself still has its original update behaviour.

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
