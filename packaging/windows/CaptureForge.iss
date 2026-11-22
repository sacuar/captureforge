#ifndef AppVersion
#define AppVersion "1.0.2"
#endif
[Setup]
AppId={{C77C873F-687A-43C2-A438-5D72E24B1BB1}
AppName=CaptureForge
AppVersion={#AppVersion}
AppPublisher=CaptureForge
AppPublisherURL=https://github.com/sacuar/captureforge
DefaultDirName={localappdata}\Programs\CaptureForge
DefaultGroupName=CaptureForge
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=..\..\installer
OutputBaseFilename=CaptureForge-{#AppVersion}-Setup-x64
SetupIconFile=..\..\resources\captureforge.ico
UninstallDisplayIcon={app}\CaptureForge.exe
CloseApplications=yes
RestartApplications=no
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableWelcomePage=no
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "startup"; Description: "Start CaptureForge when I sign in"; Flags: unchecked
[Files]
Source: "..\..\maintenance.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\maintenance.ps1"; DestName: "Setup-Maintenance.ps1"; Flags: dontcopy
Source: "..\..\dist\CaptureForge\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\CaptureForge maintenance"; Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\maintenance.ps1"" -App CaptureForge -ProtectPath ""{app}"""; IconFilename: "{app}\CaptureForge.exe"
Name: "{autoprograms}\CaptureForge"; Filename: "{app}\CaptureForge.exe"
Name: "{autodesktop}\CaptureForge"; Filename: "{app}\CaptureForge.exe"; Tasks: desktopicon
Name: "{userstartup}\CaptureForge"; Filename: "{app}\CaptureForge.exe"; Parameters: "--background"; Tasks: startup
[Run]
Filename: "{app}\CaptureForge.exe"; Description: "Launch CaptureForge"; Flags: nowait postinstall skipifsilent; Check: OcrPassed
[Code]
var MaintenanceButton: TNewButton;
    Passed: Boolean;
procedure OpenMaintenance(Sender: TObject);
var ResultCode: Integer;
begin
  ExtractTemporaryFile('Setup-Maintenance.ps1');
  if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\Setup-Maintenance.ps1') + '" -App CaptureForge -ProtectPath "' + WizardDirValue + '"', '', SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode) then
    MsgBox('Maintenance tool could not be opened.', mbError, MB_OK);
end;
procedure InitializeWizard;
var Page: TWizardPage; Info: TNewStaticText;
begin
  Page := CreateCustomPage(wpSelectDir, 'Review previous builds', 'Optional maintenance before installation');
  Info := TNewStaticText.Create(Page); Info.Parent := Page.Surface; Info.Width := Page.SurfaceWidth; Info.WordWrap := True; Info.Caption := 'Find old builds, running copies and startup registrations. Review and recycle selected obsolete build files or compiled folders. Nothing is removed automatically.';
  MaintenanceButton := TNewButton.Create(Page); MaintenanceButton.Parent := Page.Surface; MaintenanceButton.Top := ScaleY(56); MaintenanceButton.Width := ScaleX(235); MaintenanceButton.Caption := 'Scan and review old builds...'; MaintenanceButton.OnClick := @OpenMaintenance;
end;

function OcrPassed: Boolean;
begin Result := Passed; end;
procedure CurStepChanged(CurStep: TSetupStep);
var Code: Integer;
begin
 if CurStep = ssPostInstall then begin
   WizardForm.StatusLabel.Caption := 'Testing bundled Tesseract OCR and English language data...';
   Passed := Exec(ExpandConstant('{app}\CaptureForge.exe'), '--ocr-self-test', ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, Code) and (Code=0);
   if not Passed then begin
     Log('CaptureForge OCR self-test FAILED');
     if WizardSilent then RaiseException('CaptureForge OCR installation self-test failed');
     MsgBox('OCR self-test failed. Open Settings > OCR diagnostics or reinstall. The application has not been launched.', mbError, MB_OK);
   end else Log('CaptureForge OCR self-test PASSED');
 end;
end;
