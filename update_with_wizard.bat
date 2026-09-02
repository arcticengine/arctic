@echo off
rem Regenerates the build files of every project in this repository with the
rem console wizard. Build the wizard first (wizard\wizard.sln, Debug x64).
setlocal
cd /d "%~dp0"
set WIZARD=wizard\build\wizard_Debug_x64.exe
for %%p in (antarctica_pyramids benchmark tests wizard) do (
  "%WIZARD%" update %%p || exit /b 1
)
