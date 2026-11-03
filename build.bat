@echo off
rem Compile WaveSketch en Release (VST3 + Standalone) et lance les tests.
rem Ne copie rien dans C:\Program Files : pas besoin des droits administrateur.
setlocal
cd /d "%~dp0"

set "CMAKE=cmake"
where cmake >nul 2>nul || set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
set "CTEST=ctest"
where ctest >nul 2>nul || set "CTEST=%ProgramFiles%\CMake\bin\ctest.exe"

"%CMAKE%" -S . -B build -G "Visual Studio 17 2022" -A x64 || goto :error
"%CMAKE%" --build build --config Release --parallel || goto :error

if exist build\CTestTestfile.cmake (
    "%CTEST%" --test-dir build -C Release --output-on-failure || goto :error
)

echo.
echo Build termine.
echo VST3       : %~dp0build\WaveSketch_artefacts\Release\VST3\WaveSketch.vst3
echo Standalone : %~dp0build\WaveSketch_artefacts\Release\Standalone\WaveSketch.exe
exit /b 0

:error
echo.
echo *** Echec du build (code %errorlevel%) ***
exit /b 1
