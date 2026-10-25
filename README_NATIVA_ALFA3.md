# MSX Forge Studio 0.6.0 — transición nativa Alfa 3

Esta entrega está basada en **Qt6 Alfa 2** (no en HTML). Se conserva el motor C, el formato `.msxf` y la exportación ROM.

## Nuevo
- Pestaña **Sprites** habilitada en el programa Qt6.
- Edición del bitmap NPC de 16×16 directamente sobre `ForgeProject::npc_sprites`.
- Selector de apariencias y 16 colores de MSX2.
- Clic izquierdo para pintar, clic derecho para transparencia; botón Limpiar con confirmación.
- Cambios integrados al mecanismo de deshacer, título de cambios pendientes, guardado `.msxf` y vista del NPC en el mapa.

## Pendiente
- Animaciones de cuatro direcciones y dos cuadros de los prototipos web NO están integradas aún: el formato de sprites del motor existente necesita una evolución compatible.
- El editor nativo de eventos sigue pendiente, no se han sustituido funciones del motor.
- Compilación Qt6 en Windows no comprobada en este entorno.

## Compilación en Windows
Instalar Qt 6.4+ Widgets, Qt Creator, CMake y MSVC 2022 x64. Abrir `CMakeLists.txt` con Qt Creator y compilar Release; o ejecutar `build_windows_qt6.bat`.

## Pruebas realizadas
`bash tests/run_core_tests.sh`: pasó contrato del motor y pruebas estáticas del proyecto Qt6. Estas pruebas no sustituyen una compilación Qt real.

Conservar copia de seguridad de los proyectos `.msxf` originales durante las pruebas.
