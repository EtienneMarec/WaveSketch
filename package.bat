@echo off
rem Compile WaveSketch en Release puis prépare une archive à distribuer dans dist\.
rem Ne copie rien dans C:\Program Files : pas besoin des droits administrateur.
setlocal
cd /d "%~dp0"

call "%~dp0build.bat" || goto :error

rem Version lue dans CMakeLists.txt : project(WaveSketch VERSION x.y.z)
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "(Select-String -Path CMakeLists.txt -Pattern 'project\(WaveSketch VERSION ([0-9.]+)').Matches[0].Groups[1].Value"`) do set "VERSION=%%v"
if "%VERSION%"=="" goto :error

set "NAME=WaveSketch-%VERSION%-win64"
set "OUT=dist\%NAME%"
set "ART=build\WaveSketch_artefacts\Release"

if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%OUT%.zip" del /q "%OUT%.zip"
mkdir "%OUT%" || goto :error

xcopy /e /i /q "%ART%\VST3\WaveSketch.vst3" "%OUT%\WaveSketch.vst3" >nul || goto :error
copy /y "%ART%\Standalone\WaveSketch.exe" "%OUT%\" >nul || goto :error
copy /y "packaging\INSTALLATION.txt" "%OUT%\" >nul || goto :error

powershell -NoProfile -Command "Compress-Archive -Path '%OUT%\*' -DestinationPath '%OUT%.zip' -Force" || goto :error

echo.
echo Archive prete : %~dp0%OUT%.zip
exit /b 0

:error
echo.
echo *** Echec de la creation de l'archive ***
exit /b 1
