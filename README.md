# MSX Forge Studio 0.6.0 — Segunda alfa Qt6

**Estado real de esta entrega:** primera implementación del editor visual en **Qt 6 Widgets/C++**, reutilizando directamente el motor de la versión estable **0.5.9.4**. No es todavía una sustitución completa de la aplicación anterior.

## Avances de la segunda alfa

- **Selector gráfico de llegada** de puertas en el inspector: el botón abre el mapa destino, valida casilla libre y guarda las coordenadas exactas; clic derecho cancela sin tocar la puerta.
- Valida que la llegada no coincida con NPC, otras puertas ni posición inicial y que el terreno sea caminable.
- Guardar, exportar, abrir y cambiar de proyecto se bloquean mientras hay una llegada en selección.
- Segunda vista previa web con edición temporal, historial, navegación y exportación/importación JSON **exclusiva del prototipo**. No equivale al formato .msxf.

## Funcionalidad ya implementada en el código

- Ventana oscura Qt 6 con paneles **acoplables, flotantes, tabulados, redimensionables** y diseño persistente; recupera ventanas flotantes cuando se desconecta una pantalla y ofrece **Ver → Restaurar paneles**.
- Explorador de los escenarios, NPC y puertas reales del proyecto.
- Mapa central de **16 × 12**, dibujado con los tiles MSX del `.msxf`, sprites, puertas y personaje de inicio; zoom.
- Inspector por selección (escenario, NPC, puerta) con formulario desplazable y barra de acciones fija.
- Crear mapas (máximo 16), pintar tiles, colocar NPC con apariencia previa y colocar puertas. Editar sus datos; duplicar NPC en otra casilla libre; eliminar objetos.
- Deshacer/Rehacer (historial del proyecto), guardado seguro `.msxf` (QSaveFile), confirmación al reemplazar/cerrar, importación del formato v12 y migración de versiones anteriores vía motor C.
- Exportación real de ROM de 32 KB y lanzamiento opcional de openMSX con `F5`.
- Motor de misiones, puerta con llave, condiciones, recompensas y sprites conservado **sin cambios** en archivos `core/`.

## No migrado aún (deliberadamente)

- Compositor visual completo de eventos (los campos de eventos del NPC están en el inspector, pero falta el lienzo de nodos Qt).
- Editor de sprites de píxeles y compositor PSG de la interfaz 0.5.9.4.
- Herramientas avanzadas de puertas (selector gráfico de casilla de llegada y retornos interiores) y creación de interiores mediante asistentes; la coordenada de llegada sí puede editarse manualmente.
- Empaquetado y pruebas de la interfaz Qt en un Windows real.

**No se alteran los proyectos existentes automáticamente.** Hacé una copia de cualquier `.msxf` antes de probar esta alfa.

## Cómo compilar en Windows

1. Instalá Qt 6.4 o superior con el kit **MSVC 2022 x64**, Qt Creator, CMake y Ninja. Usá el compilador MSVC de ese kit.
2. Abrí `CMakeLists.txt` con Qt Creator, elegí el kit Windows Desktop Qt 6 MSVC x64 y compilá en Release.
3. Para compartir una versión portable, copiá el EXE generado y ejecutá `windeployqt MSXForgeStudio.exe` desde el entorno Qt correspondiente. Esto incorpora las DLL y plugins de Qt que correspondan; no copies solamente el EXE.
4. Podés usar `build_windows_qt6.bat` desde la consola del entorno Qt + MSVC con CMake/Ninja en PATH.

El proyecto también puede compilarse en Linux con Qt 6 Widgets, pero la aplicación está diseñada principalmente para Windows.

## Cómo ejecutar verificaciones sin Qt

```
bash tests/run_core_tests.sh
```

Estas pruebas ejecutan **el motor C real** sobre una demo creada con 0.5.9.4, duplican un NPC y compilan ROM de 32 KB. Los tests estáticos adicionales revisan puntos clave de la integración Qt, pero **no equivalen a compilar ni probar visualmente Qt**.

## Notas técnicas

- La app Qt usa `QMainWindow`, `QDockWidget`, `QTreeWidget`, `QScrollArea`, `QStackedWidget`, formularios y `QSettings`.
- Los paneles comparten geometría automática y los botones del inspector permanecen fuera del área desplazable.
- Los proyectos usan el binario exacto de `ForgeProject` (version 12), importado desde `core/engine.h`; se validan antes de guardar.
- El guardado es atómico (`QSaveFile`) y el usuario puede cancelar un reemplazo.
- El motor es **el original de 0.5.9.4**, no una reinterpretación.

**Importante:** el entorno de compilación de esta entrega no incluye Qt6 ni acceso al servidor de paquetes. Por eso no se incluye un `MSXForgeStudio.exe` rotulado como 0.6.0. No sería honesto afirmar que el código Qt se probó en ejecución. La referencia ejecutable estable sigue siendo **0.5.9.4**.

## Vista previa inmediata (sin Qt)

Abrí `preview/VistaPrevia_060.html` con Chrome, Edge o Firefox. Usa datos reales del ejemplo `Demo_0594_NPC_Duplicado.msxf`. Permite recorrer escenas, seleccionar NPC y puertas, cambiar visualmente tiles y arrastrar paneles entre sectores del boceto. **No guarda en `.msxf` ni ejecuta ROM**; sirve para revisar las decisiones de diseño antes del ensayo en Qt.

`assets/msx_logo.png` está rasterizado a partir del logotipo MSX real que ya veníamos usando; se integra en el proyecto Qt mediante `qt/resources.qrc`.

Consultá `BUILD_REPORT.md` para conocer qué pruebas pasaron y qué no fue posible verificar.
