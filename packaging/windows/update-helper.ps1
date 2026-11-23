# Staged by CaptureForge after SHA-256 verification of an official release.
param(
 [Parameter(Mandatory=$true)][string]$InstallerPath,
 [Parameter(Mandatory=$true)][string]$InstallDir,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{64}$')][string]$ExpectedSha256,
 [Parameter(Mandatory=$true)][int]$ProcessIdToWait,
 [Parameter(Mandatory=$true)][string]$RestartExe
)
$ErrorActionPreference = 'Stop'
$stateDir = Join-Path $env:LOCALAPPDATA 'CaptureForge'
$stageDir = Split-Path -Parent $PSCommandPath
$logFile = Join-Path $stateDir 'update-helper.log'
try {
 New-Item -Path $stateDir -ItemType Directory -Force | Out-Null
 "[$(Get-Date -Format o)] Update requested" | Add-Content -LiteralPath $logFile
 $target = [IO.Path]::GetFullPath($InstallDir).TrimEnd('\')
 $runningDir = [IO.Path]::GetFullPath((Split-Path -Parent $RestartExe)).TrimEnd('\')
 if ($target -ine $runningDir -or
     !(Test-Path -LiteralPath $RestartExe -PathType Leaf) -or
     !(Test-Path -LiteralPath $InstallerPath -PathType Leaf)) {
   throw 'Current installation or downloaded installer could not be verified.'
 }
 if ((Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash -ine $ExpectedSha256) {
   throw 'Downloaded installer checksum mismatch. Installation cancelled.'
 }
 $running = Get-Process -Id $ProcessIdToWait -ErrorAction SilentlyContinue
 if ($null -ne $running -and !$running.WaitForExit(120000)) {
   throw 'CaptureForge did not exit within 120 seconds.'
 }
 $setupLog = Join-Path $stateDir 'update-installer.log'
 $args = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CLOSEAPPLICATIONS',
   ('/DIR="' + $target + '"'),('/LOG="' + $setupLog + '"'))
 $setup = Start-Process -FilePath $InstallerPath -ArgumentList $args -Wait -PassThru
 if ($setup.ExitCode -ne 0) { throw "Installer failed with code $($setup.ExitCode). See $setupLog" }
 if (!(Test-Path -LiteralPath $RestartExe -PathType Leaf)) {
   throw 'CaptureForge.exe was not found after the upgrade.'
 }
 Start-Process -FilePath $RestartExe -ArgumentList '--background' -WorkingDirectory $target | Out-Null
 "[$(Get-Date -Format o)] Update installed and restarted" | Add-Content -LiteralPath $logFile
} catch {
 $message = $_.Exception.Message
 try { "[$(Get-Date -Format o)] FAILED: $message" | Add-Content -LiteralPath $logFile } catch {}
 try {
   Add-Type -AssemblyName System.Windows.Forms
   [void][System.Windows.Forms.MessageBox]::Show(
     "CaptureForge could not update: $message`n`nLog: $logFile",
     'CaptureForge update failed', 'OK', 'Error')
 } catch {}
 exit 1
} finally {
 if ((Split-Path -Leaf $stageDir) -like 'CaptureForgeUpdate-*') {
   try { Remove-Item -LiteralPath $stageDir -Recurse -Force } catch {}
 }
}
