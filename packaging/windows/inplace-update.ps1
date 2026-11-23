# CaptureForge in-place updater. Invoked only after the app verifies the GitHub release asset.
param(
    [Parameter(Mandatory=$true)][string]$Installer,
    [Parameter(Mandatory=$true)][string]$TargetDir,
    [Parameter(Mandatory=$true)][string]$ExpectedSha256,
    [Parameter(Mandatory=$true)][int]$ParentPid,
    [switch]$NoRelaunch
)
$ErrorActionPreference = 'Stop'
$exitCode = 1
try {
    # Wait until CaptureForge exits and releases the single-instance lock.
    if ($ParentPid -gt 0) {
        Wait-Process -Id $ParentPid -ErrorAction SilentlyContinue
    }
    Start-Sleep -Milliseconds 400

    $installedExe = Join-Path $TargetDir 'CaptureForge.exe'
    $uninstaller = Get-ChildItem -LiteralPath $TargetDir -Filter 'unins*.exe' -File -ErrorAction SilentlyContinue
    if (!(Test-Path -LiteralPath $installedExe -PathType Leaf) -or !$uninstaller) {
        throw 'Existing CaptureForge installation was not found. Refusing to create another installation.'
    }
    if (!(Test-Path -LiteralPath $Installer -PathType Leaf)) {
        throw 'Verified installer was not found.'
    }
    $actualHash = (Get-FileHash -LiteralPath $Installer -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $ExpectedSha256.ToLowerInvariant()) {
        throw 'Installer SHA-256 changed after download. Installation was cancelled.'
    }

    # Same AppId and directory as the existing installation; never a new location.
    $arguments = '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /SP- /DIR="' + $TargetDir + '"'
    $setup = Start-Process -FilePath $Installer -ArgumentList $arguments -Wait -PassThru
    if ($setup.ExitCode -ne 0) {
        throw "Update setup failed with exit code $($setup.ExitCode)."
    }
    if (!(Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        throw 'Installation finished but CaptureForge.exe is missing.'
    }
    if (-not $NoRelaunch) {
        Start-Process -FilePath $installedExe -ArgumentList '--background' | Out-Null
    }
    $exitCode = 0
}
catch {
    $message = $_.Exception.Message
    try {
        $logDir = Join-Path $env:LOCALAPPDATA 'CaptureForge'
        New-Item -ItemType Directory -Force -Path $logDir | Out-Null
        Set-Content -LiteralPath (Join-Path $logDir 'update-error.log') -Value $message -Encoding UTF8
        Add-Type -AssemblyName System.Windows.Forms
        [System.Windows.Forms.MessageBox]::Show($message, 'CaptureForge update failed', 'OK', 'Error') | Out-Null
    } catch {}
}
finally {
    $workingDir = Split-Path -Parent $Installer
    # Installer and copied helper live in their own dedicated update directory.
    Start-Sleep -Milliseconds 500
    Remove-Item -LiteralPath $workingDir -Recurse -Force -ErrorAction SilentlyContinue
}
exit $exitCode
