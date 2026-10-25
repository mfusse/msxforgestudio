@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: falta CMake en PATH. Instalalo junto con Qt Creator.
  pause
  exit /b 1
)
echo ============================================================
echo  MSX Forge Studio 0.6.0 ALFA - Qt 6 Widgets / C++
echo ============================================================
echo Asegurate de abrir Qt 6.4+ para MSVC 2022 x64 desde Qt Creator
cmake -S . -B build-qt -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
  echo ERROR al configurar: revisa instalacion Qt y compilador MSVC.
  pause
  exit /b 1
)
cmake --build build-qt --config Release
if errorlevel 1 (
  echo ERROR al compilar.
  pause
  exit /b 1
)
echo.
echo Ejecutable esperado: build-qt\MSXForgeStudio.exe
echo Para distribuir, copia el exe y ejecuta windeployqt sobre esa copia.
pause
