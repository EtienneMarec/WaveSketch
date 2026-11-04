@echo off
rem Valide le VST3 compilé avec pluginval, au niveau de rigueur maximal (10).
rem pluginval doit se trouver dans tools\pluginval\pluginval.exe (voir README.md).
setlocal
cd /d "%~dp0"

set "PLUGINVAL=tools\pluginval\pluginval.exe"
set "PLUGIN=%~dp0build\WaveSketch_artefacts\Release\VST3\WaveSketch.vst3"
set "LOGS=%~dp0build\pluginval-logs"

if not exist "%PLUGINVAL%" (
    echo pluginval introuvable : %PLUGINVAL%
    echo Telecharge pluginval_Windows.zip depuis https://github.com/Tracktion/pluginval/releases
    echo et extrais pluginval.exe dans tools\pluginval\
    exit /b 1
)

if not exist "%PLUGIN%" (
    echo Plugin introuvable : lance d'abord build.bat
    exit /b 1
)

if not exist "%LOGS%" mkdir "%LOGS%"

rem pluginval est une application fenêtrée : "start /wait" attend la fin et récupère son code de sortie.
start "" /wait "%PLUGINVAL%" --strictness-level 10 --timeout-ms 600000 --output-dir "%LOGS%" --validate "%PLUGIN%"
set "RESULT=%errorlevel%"

echo.
if "%RESULT%"=="0" (
    echo pluginval : SUCCES ^(rigueur 10^). Journal dans %LOGS%
) else (
    echo pluginval : ECHEC ^(code %RESULT%^). Voir le journal dans %LOGS%
)
exit /b %RESULT%
