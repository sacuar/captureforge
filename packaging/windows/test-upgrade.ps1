$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '../..')
$version = [regex]::Match((Get-Content CMakeLists.txt -Raw), 'project\(CaptureForge VERSION ([0-9.]+)').Groups[1].Value
$current = (Resolve-Path "installer/CaptureForge-$version-Setup-x64.exe").Path
$installDir = Join-Path $env:RUNNER_TEMP 'CaptureForgeUpgradeTest'
$previous = Join-Path $env:RUNNER_TEMP 'CaptureForge-1.0.2-Setup-x64.exe'
$release = Invoke-RestMethod 'https://api.github.com/repos/sacuar/captureforge/releases/tags/v1.0.2' -Headers @{'User-Agent'='CaptureForge-upgrade-test'}
$asset = $release.assets | Where-Object { $_.name -eq 'CaptureForge-1.0.2-Setup-x64.exe' } | Select-Object -First 1
if (!$asset) { throw 'Previous release asset unavailable' }
$verifiedDigest = [regex]::Match([string]$asset.digest, '^sha256:([a-fA-F0-9]{64})$')
if (!$verifiedDigest.Success) { throw 'Previous release checksum unavailable' }
$expected = $verifiedDigest.Groups[1].Value
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $previous
if ((Get-FileHash $previous -Algorithm SHA256).Hash -ine $expected) { throw 'Previous release checksum mismatch' }
function Install-App([string]$path) {
 $args = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/DIR="' + $installDir + '"'))
 $p = Start-Process -FilePath $path -ArgumentList $args -Wait -PassThru
 if ($p.ExitCode -ne 0) { throw "Setup exit code $($p.ExitCode)" }
}
Install-App $previous
$settings = 'HKCU:\Software\CaptureForge\CaptureForge'
New-Item -Path $settings -Force | Out-Null
New-ItemProperty -Path $settings -Name 'UpgradeSentinel' -Value 'keep-me' -PropertyType String -Force | Out-Null
Install-App $current
if (!(Test-Path "$installDir/update-helper.ps1")) { throw 'Update helper missing from installation' }
if ((Get-FileHash "$installDir/CaptureForge.exe" -Algorithm SHA256).Hash -ne
   (Get-FileHash 'dist/CaptureForge/CaptureForge.exe' -Algorithm SHA256).Hash) {
  throw 'New executable did not replace original'
}
if ((Get-ItemProperty -Path $settings -Name UpgradeSentinel).UpgradeSentinel -ne 'keep-me') {
 throw 'Settings were lost'
}
$entries = @(Get-ChildItem 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall' |
  ForEach-Object {Get-ItemProperty $_.PSPath} |
  Where-Object {$_.DisplayName -match '^CaptureForge(?:\s|$)'})
if ($entries.Count -ne 1) {throw "Expected one uninstall entry, got $($entries.Count)"}
if ($entries[0].InstallLocation.TrimEnd('\') -ine $installDir.TrimEnd('\')) {
 throw 'Installation directory changed'
}
$p=Start-Process "$installDir/CaptureForge.exe" -ArgumentList '--ocr-self-test' -Wait -PassThru
if ($p.ExitCode -ne 0) {throw 'Installed OCR check failed'}
$p=Start-Process "$installDir/CaptureForge.exe" -ArgumentList '--smoke-test' -PassThru
if (!$p.WaitForExit(30000)) {$p.Kill();throw 'Startup timeout'}
if ($p.ExitCode -ne 0) {throw 'Startup smoke test failed'}
Write-Host 'PASS: upgrade keeps directory, settings and one uninstall entry; OCR and startup work.'
