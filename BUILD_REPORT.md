# MSX Forge Studio 0.6.0 alfa 2 — Informe de verificación

Fecha: 25 de octubre de 2026.

## Verificaciones completadas en este entorno

- Core C original: se compila y vincula con GCC sin alterar el motor Z80.
- `tests/run_core_tests.sh`: pasa lectura y validación de un proyecto v12 existente de la 0.5.9.4, duplicación de NPC e integración que exporta una ROM de 32768 bytes.
- 18 comprobaciones estáticas de la interfaz Qt de la alfa original: pasan.
- 12 comprobaciones adicionales sobre llegada de puertas, retorno automático, exclusión de objetos y vista previa: pasan.
- Los archivos `core/engine.c`, `core/rom.c` y `examples/Demo_0594_NPC_Duplicado.msxf` son idénticos byte por byte a la alfa anterior (comparados mediante SHA-256).
- La vista previa web usa el mapa de la demo real y **32 tiles** obtenidos a través de `forge_tile_pixels_const()` del motor C, con sprites reales.
- JavaScript verificado con `node --check`.
- Pruebas de navegador Chromium: editar NPC, aplicar, deshacer/rehacer, elegir casilla de llegada en mapa destino, guardar JSON y volver a importar JSON, proteger restablecimiento ante cambios: **pasaron**.
- Maquetación en Chromium: 1366×768 a 100% y 150% de escala de pantalla, 1920×1080 y 1280×720: acciones y botones del inspector permanecen dentro del panel.

## No verificado

- **La aplicación Qt 6 nativa no se pudo compilar aquí:** no están instaladas las cabeceras ni bibliotecas de Qt 6. El código C++ está actualizado, pero requiere una compilación con un kit Qt 6 MSVC x64 en Windows.
- No se pudo validar el editor Qt en ejecución en un Windows real. Las pruebas web pertenecen exclusivamente a la vista previa HTML.
- La vista previa HTML **NO lee ni escribe `.msxf`**, y no ejecuta openMSX. Los archivos JSON de esta vista previa no son proyectos ni ROM.
- Los editores Qt de Eventos, Sprites y Música todavía están pendientes de migración.

## Ensayo recomendado en Windows

1. Abrir `CMakeLists.txt` en Qt Creator con kit Qt 6 MSVC 2022 x64.
2. Compilar Release y corregir cualquier error específico del compilador/SDK que aparezca.
3. Probar el Inspector de puerta, el botón **Elegir llegada en el mapa**, el regreso a la escena original y **Retorno automático (interiores)**.
4. Verificar que Guardar, Abrir y Exportar bloqueen el flujo mientras se está eligiendo la llegada.
5. Ejecutar pruebas reales con proyectos `.msxf` de la 0.5.9.4 y ROMs en openMSX.
