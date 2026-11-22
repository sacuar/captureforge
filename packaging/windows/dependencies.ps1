param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference='Stop'
$dest=[IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Force "$dest/tesseract/tessdata","$dest/icons","$dest/licenses" | Out-Null
$root=$env:VCPKG_INSTALLATION_ROOT
if(!$root){$root=$env:VCPKG_ROOT}
if(!$root -or !(Test-Path "$root/vcpkg.exe")){throw 'Install vcpkg and set VCPKG_ROOT, or use GitHub Actions.'}
& "$root/vcpkg.exe" install tesseract:x64-windows --classic
if($LASTEXITCODE -ne 0){throw 'Tesseract dependency build failed'}
$installed="$root/installed/x64-windows"
Copy-Item "$installed/tools/tesseract/*" "$dest/tesseract" -Recurse -Force
Copy-Item "$installed/bin/*.dll" "$dest/tesseract" -Force
if(!(Test-Path "$dest/tesseract/tesseract.exe")){throw 'Tesseract executable not found'}
Invoke-WebRequest 'https://raw.githubusercontent.com/tesseract-ocr/tessdata_fast/4.1.0/eng.traineddata' -OutFile "$dest/tesseract/tessdata/eng.traineddata"
foreach($p in (Get-ChildItem "$installed/share" -Directory)){
 if(Test-Path "$($p.FullName)/copyright"){Copy-Item "$($p.FullName)/copyright" "$dest/licenses/$($p.Name).txt"}
}
& "$dest/tesseract/tesseract.exe" --version
if($LASTEXITCODE -ne 0){throw 'Bundled Tesseract cannot start'}
& "$dest/tesseract/tesseract.exe" --list-langs --tessdata-dir "$dest/tesseract/tessdata"
if($LASTEXITCODE -ne 0){throw 'Tesseract language data test failed'}
$icons=@{save='content/save';content_copy='content/content_copy';crop='image/crop';rotate_left='image/rotate_left';flip='image/flip';edit='editor/edit';undo='content/undo';document_scanner='image/document_scanner';settings='action/settings';update='action/update';extension='action/extension';search='action/search'}
foreach($name in $icons.Keys){
 $path=$icons[$name]; $url="https://raw.githubusercontent.com/google/material-design-icons/master/src/$path/materialicons/24px.svg"
 try{Invoke-WebRequest $url -OutFile "$dest/icons/$name.svg"}catch{Write-Warning "Icon $name unavailable; native fallback will be used."}
}
Invoke-WebRequest 'https://raw.githubusercontent.com/google/material-design-icons/master/LICENSE' -OutFile "$dest/licenses/Google-Material-Icons.txt"
Invoke-WebRequest 'https://raw.githubusercontent.com/tesseract-ocr/tessdata_fast/4.1.0/LICENSE' -OutFile "$dest/licenses/tessdata-fast.txt"
