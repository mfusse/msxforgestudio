from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
main=(root/'qt/main.cpp').read_text()
project=(root/'qt/project_io.cpp').read_text()
cmake=(root/'CMakeLists.txt').read_text()
checks={
    'Qt Widgets selected': 'find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)' in cmake,
    'Native C engine kept': 'core/engine.c core/rom.c core/msx2_graphics.c' in cmake,
    'Qt docks instead of absolute positions': 'QDockWidget' in main and 'AllowTabbedDocks' in main and 'splitDockWidget' in main,
    'Editable scene tree': 'QTreeWidget' in main and 'rebuildTree()' in main,
    'Real engine tile pixels': 'forge_tile_pixels_const' in main,
    'Resource previews refresh after opening': 'refreshArt();map_->update();log("Proyecto compatible abierto.")' in main,
    'Painting protects interactive objects': 'protectedTile' in main and 'tool_==Tool::Brush' in main,
    'Custom arrival cannot target another portal': 'occupied||!forge_tile_walkable' in main,
    'Real engine NPC data': 'ForgeNPC' in main and 'forge_duplicate_npc' in main,
    'Real engine portal data': 'ForgePortal' in main and 'requires_key' in main,
    'Scene selection and inspector': 'updateInspector()' in main and 'applyInspector()' in main,
    'Protect project replacement': 'guardChanges()' in main and 'QMessageBox::Cancel' in main,
    'Persistent dock placement': 'saveState(1)' in main and 'restoreState(' in main,
    'Offscreen dock recovery': 'recoverFloatingDocks()' in main,
    'Atomic save': 'QSaveFile' in project and 'output.commit()' in project,
    'Legacy v11 and older imports': 'forge_import_v11' in project and 'forge_import_legacy' in project,
    'Built in ROM compilation': 'forge_export_rom' in project,
    'No Win32 UI in new application': 'windows_app.c' not in cmake,
}
for name, ok in checks.items():
    assert ok,name
    print('PASS',name)
print(f'PASS: {len(checks)} static integration checks (not a Qt compile)')
