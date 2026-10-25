# MSX Forge Studio 0.6.0 — Windows Portable Beta 1 (preparación de release)

**Este ZIP contiene fuentes y automatización del build. NO contiene un ejecutable Qt6 compilado.**

## Objetivo del lanzamiento
El usuario final descarga **MSXForgeStudio_0_6_0_Windows_Portable_Beta1.zip**, lo extrae y abre `MSXForgeStudio.exe` sin cuentas ni herramientas de desarrollo.

## Generación del ejecutable (solo quien publica)
1. Subir el contenido de esta carpeta a un repositorio con GitHub Actions habilitado.
2. Abrir **Actions > Windows Portable Beta 1 > Run workflow**.
3. Al terminar correctamente, descargar el artifact `MSXForgeStudio_0_6_0_Windows_Portable_Beta1`.
4. Extraerlo, comprobar que incluya el ZIP de usuario y probarlo en un Windows limpio antes de distribuir.

Alternativa desde un Windows con Qt 6.8.3 MSVC 2022 x64, CMake y Visual Studio Build Tools:
```
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build build-win --config Release
powershell -ExecutionPolicy Bypass -File packaging/make_portable.ps1 -BuildDir build-win -OutputDir release -QtBin "C:/Qt/6.8.3/msvc2022_64/bin"
```
**Las tres herramientas anteriores son exclusivamente para compilar la distribución, no para sus usuarios.** El flujo automatizado las instala en su propio entorno temporal.

## Verificaciones hechas en el entorno actual
- Contratos del motor C, formato `.msxf`, exportación ROM 32 KB y revisión estática del código Qt6: aprobados.
- **No se pudo ejecutar ni compilar el proyecto Qt6 en Windows aquí.** La primera ejecución de Actions y la prueba Windows siguen pendientes.
- El editor nativo de sprites todavía no implementa las cuatro direcciones/dos cuadros del prototipo HTML; no se debe anunciar esa característica como integrada.

## Licencia y distribución
Antes de una publicación pública, comprobar el régimen de licencias aplicable a Qt y a todas las dependencias, y distribuir los avisos/licencias correspondientes; también probar en Windows sin instalaciones de Qt. No hay firma digital de código incluida.
