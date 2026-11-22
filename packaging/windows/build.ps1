param([string]$QtDir=$env:Qt6_DIR)
$ErrorActionPreference='Stop'
Set-Location (Join-Path $PSScriptRoot '../..')
if(!$QtDir){$QtDir=$env:QT_ROOT_DIR}
if(!$QtDir){throw 'Set QT_ROOT_DIR to your Qt 6 MSVC x64 directory.'}
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$QtDir"
if($LASTEXITCODE -ne 0){throw 'CMake failed'}
cmake --build build --config Release --parallel
if($LASTEXITCODE -ne 0){throw 'Build failed'}
$dist='dist/CaptureForge'
Remove-Item $dist -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item build/Release/CaptureForge.exe,ocr.ps1,resources/camera.png,THIRD_PARTY_NOTICES.md $dist
& "$QtDir/bin/windeployqt.exe" --release --no-translations --compiler-runtime "$dist/CaptureForge.exe"
if($LASTEXITCODE -ne 0){throw 'Qt deployment failed'}
& "$PSScriptRoot/dependencies.ps1" -Destination $dist
$p=Start-Process "$dist/CaptureForge.exe" -ArgumentList '--ocr-self-test' -Wait -PassThru
if($p.ExitCode -ne 0){throw 'Bundled OCR recognition test failed'}
$iscc="${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe"
if(!(Test-Path $iscc)){throw 'Install Inno Setup 6'}
$version=[regex]::Match((Get-Content CMakeLists.txt -Raw),'project\(CaptureForge VERSION ([0-9.]+)').Groups[1].Value
if(!$version){throw 'Version missing'}
& $iscc "/DAppVersion=$version" packaging/windows/CaptureForge.iss
if($LASTEXITCODE -ne 0){throw 'Installer compilation failed'}
