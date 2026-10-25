#ifndef MSX2_GRAPHICS_H
#define MSX2_GRAPHICS_H
#include <stdint.h>
#include "engine.h"
#define MSX2_PATTERN_BYTES 1024
#define MSX2_TILE_COUNT T_COUNT
#define MSX2_GLYPHS_PER_TILE 4
#define MSX2_GLYPH_BYTES 8
#define MSX2_TILE_BYTES (MSX2_GLYPHS_PER_TILE * MSX2_GLYPH_BYTES)
// Common tile-atlas offset for both Windows preview and Z80 ROM graphics.
// All 32*32=1024 bytes are editable terrain patterns (glyphs 128..255).
static inline int forge_screen4_tile_pixel_offset(int tile,int x,int y){
 return tile*MSX2_TILE_BYTES+((y/8)*2+x/8)*MSX2_GLYPH_BYTES+(y%8);
}
extern const uint32_t forge_msx2_palette[16];
// SCREEN 4: glyphs 128..239; 8 bytes of pattern and 8 bytes of color per glyph.
// Four consecutive glyphs represent each 16x16 object.
void forge_screen4_hw_sprite(const uint8_t sprite[256],uint8_t pattern[32],
                             uint8_t line_colors[16]);
void forge_screen4_graphics(const ForgeProject *p, uint8_t patterns[MSX2_PATTERN_BYTES],
                            uint8_t colors[MSX2_PATTERN_BYTES]);
#endif
