@echo off
setlocal
cd /d "%~dp0"
where gh >nul 2>nul || (echo Install GitHub CLI first: https://cli.github.com/ & pause & exit /b 1)
where git >nul 2>nul || (echo Install Git first. & pause & exit /b 1)
gh auth status || gh auth login
if errorlevel 1 goto fail
for /f "delims=" %%U in ('gh api user --jq .login') do set "GHUSER=%%U"
if /i not "%GHUSER%"=="sacuar" (echo Sign in as sacuar before continuing. & goto fail)
gh repo view sacuar/captureforge >nul 2>nul
if not errorlevel 1 (echo Repository already exists. Clone it using GitHub Desktop and copy these source files to avoid overwriting history. & pause & exit /b 1)
if not exist .git git init -b main
git add . || goto fail
git commit -m "Initial CaptureForge Windows source" || goto fail
gh repo create sacuar/captureforge --public --source=. --remote=origin --push || goto fail
echo Open https://github.com/sacuar/captureforge/actions to run Windows installer workflow with publish enabled.
pause
exit /b 0
:fail
echo Publishing failed. Check output.
pause
exit /b 1
