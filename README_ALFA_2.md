# MSX Forge Studio 0.6.0 — Alfa 2

Esta entrega avanza sobre el proyecto Qt 6 de la 0.6.0 anterior **sin sustituir todavía** el programa estable 0.5.9.4.

## Probar la nueva interfaz ahora

Abrir `preview/VistaPrevia_060_Alpha2.html` en Chrome o Edge. No necesita instalar Qt ni conectarse a Internet.

En esta vista previa interactiva se puede:
- Seleccionar escenas, NPC y puertas desde el explorador o el mapa.
- Editar nombre, apariencia y diálogos de NPC; pulsar **Aplicar**.
- Duplicar personajes: **Duplicar**, después marcar una casilla libre.
- Elegir una puerta y su llegada: **Elegir llegada en mapa**, marcar una casilla válida del destino.
- Pintar terrenos, colocar NPC y crear puertas; usar Deshacer y Rehacer.
- Arrastrar paneles y usar Restaurar paneles.
- Guardar los datos del prototipo como **JSON de prueba** y volver a importarlos con **Abrir prueba**.
- Cancelar la restauración cuando hay modificaciones pendientes y evitar sobrescribir por error los cambios de prueba.

**Importante:** el JSON generado es solo para esta vista previa. No se puede abrir con MSX Forge Studio 0.5.9.4 y NO reemplaza el formato `.msxf`. En esta vista previa tampoco se genera ROM. El botón de Probar ROM indica esta limitación.

## Cambios en el código de la aplicación Qt

El inspector de puerta incorpora **Elegir llegada en el mapa** y **Retorno automático (interiores)**. La herramienta selecciona el escenario destino, rechaza paredes/objetos y registra exactamente las coordenadas elegidas, regresando luego al escenario original. Clic derecho cancela. El retorno automático usa las coordenadas especiales `255,255` ya interpretadas por el motor para permitir volver junto a la entrada de un interior.

La selección de llegadas impide operaciones inconsistentes de Guardar/Abrir/Exportar hasta confirmar o cancelar. No se han modificado los archivos originales del motor MSX ni el formato binario `.msxf`.

## Compilar el programa de escritorio

Se requiere **Qt 6 Widgets**, CMake y un kit C++ (para Windows se recomienda MSVC 2022 x64). Abrir `CMakeLists.txt` en Qt Creator, compilar Release y distribuir mediante `windeployqt`. El script `build_windows_qt6.bat` está incluido.

**Estado:** Qt 6 no está instalado en el entorno donde se preparó esta entrega. El código C++ de la alfa 2 no pudo compilarse ni ejecutarse aquí; NO se entrega un EXE Qt falso ni se afirma que el editor nativo ya está validado.

## Pruebas disponibles

```
bash tests/run_core_tests.sh
python3 tests/test_0602_source.py
```

Ver `BUILD_REPORT.md` para más información.
