"""Contract checks for the Qt6 alpha 2 editor. They do not replace Qt compilation."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'qt/main.cpp').read_text(encoding='utf-8')
h=(r/'preview/VistaPrevia_060_Alpha2.html').read_text(encoding='utf-8')
checks={
    'Arrival map picker in Qt': 'void beginArrival()' in s and 'void cancelArrival(bool restore=true)' in s,
    'Visual arrival stores exact position': 'p.tx=uint8_t(x);p.ty=uint8_t(y);' in s,
    'Arrival rejects blocked cells': 'occupied || !forge_tile_walkable(&dest,x,y)' in s,
    'Arrival rejects a second portal': 'for(int i=0;i<dest.portal_count;++i)occupied' in s,
    'Arrival protects saving/export': 'if(choosingArrival_){log("Completá o cancelá la selección de llegada antes de guardar.' in s and 'if(choosingArrival_){log("Elegí una casilla de llegada antes de exportar la ROM.' in s,
    'Interior automatic return preserved': 'portalAuto_->setChecked(automatic)' in s and 'p.tx=automatic?255:uint8_t(x)' in s,
    'Preview supports NPC duplication': 'function duplicate()' in h,
    'Preview supports undo/redo': 'function history(back)' in h,
    'Preview gates navigation on pending inspector changes': 'async function select(type,idx=-1)' in h and 'function confirmAction(' in h,
    'Preview JSON explicitly separated from MSXF': 'msx-forge-preview' in h and 'NO escribe archivos .msxf' in h,
    'Preview uses data from stable demo': 'Pueblo Brisa' in h and 'Ruta del Lago' in h,
    'Preview offers dock reset': 'resetPanels' in h,
}
for name,ok in checks.items():
    assert ok,name
    print('PASS',name)
print('Total:',len(checks),'source-contract checks; Qt6 native compilation not performed')
