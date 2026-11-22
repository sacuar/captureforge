#ifndef AppVersion
#define AppVersion "1.0.0"
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
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "startup"; Description: "Start CaptureForge when I sign in"; Flags: unchecked
[Files]
Source: "..\..\dist\CaptureForge\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\CaptureForge"; Filename: "{app}\CaptureForge.exe"
Name: "{autodesktop}\CaptureForge"; Filename: "{app}\CaptureForge.exe"; Tasks: desktopicon
Name: "{userstartup}\CaptureForge"; Filename: "{app}\CaptureForge.exe"; Parameters: "--background"; Tasks: startup
[Run]
Filename: "{app}\CaptureForge.exe"; Description: "Launch CaptureForge"; Flags: nowait postinstall skipifsilent; Check: OcrPassed
[Code]
var Passed: Boolean;
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
