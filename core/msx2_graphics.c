#include "msx2_graphics.h"

// Standard MSX palette approximation for the editor's hardware preview.
const uint32_t forge_msx2_palette[16] = {
  0x000000,0x000000,0x3eb849,0x74d07d,0x5955e0,0x8076f1,
  0xb95e51,0x65dbef,0xdb6559,0xff897d,0xccc35e,0xded087,
  0x3aa241,0xb766b5,0xcccccc,0xffffff
};
static const uint32_t editor_sprite_palette[16]={
  0x101725,0xf6f2dc,0xe98070,0x302a46,0x7195db,0x6fb473,
  0xffd367,0xa97cd0,0xe9b3be,0xa87153,0x7cdad0,0x627287,
  0xa4414f,0x366a6c,0xd5d5dc,0x18121f
};
static int dist(uint32_t x,uint32_t y){
 int r=(int)(x>>16&255)-(int)(y>>16&255);
 int g=(int)(x>>8&255)-(int)(y>>8&255);
 int b=(int)(x&255)-(int)(y&255);
 return 2*r*r+4*g*g+2*b*b;
}
static uint8_t palette_index(uint32_t rgb){
 int k,best=1,d=0x7fffffff;
 for(k=1;k<16;k++){int v=dist(rgb,forge_msx2_palette[k]);if(v<d){d=v;best=k;}}
 return (uint8_t)best;
}
// For each 8-pixel scanline find the palette pair that best represents
// the original RGB pixels, then create its foreground bit pattern.
// SCREEN 4 permits two palette entries per 8-pixel horizontal line.
static void encode_row(const uint32_t pixels[8],uint8_t *pattern,uint8_t *color){
 int fg,bg,best_fg=1,best_bg=1,best_cost=0x7fffffff,x;
 for(bg=1;bg<16;bg++)for(fg=1;fg<16;fg++){
   int cost=0;
   for(x=0;x<8;x++){
     int a=dist(pixels[x],forge_msx2_palette[bg]);
     int b=dist(pixels[x],forge_msx2_palette[fg]);
     cost+=a<b?a:b;
   }
   if(cost<best_cost){best_cost=cost;best_fg=fg;best_bg=bg;}
 }
 *pattern=0;
 for(x=0;x<8;x++){
   int a=dist(pixels[x],forge_msx2_palette[best_bg]);
   int b=dist(pixels[x],forge_msx2_palette[best_fg]);
   if(b<a)*pattern|=(uint8_t)(0x80>>x);
 }
 *color=(uint8_t)(best_fg<<4|best_bg);
}
void forge_screen4_graphics(const ForgeProject *p,uint8_t patterns[MSX2_PATTERN_BYTES],
                            uint8_t colors[MSX2_PATTERN_BYTES]){
 int slot,quad,row,col;uint32_t rgb[8];
 for(slot=0;slot<MSX2_TILE_COUNT*4;slot++)for(row=0;row<8;row++){
   int off=slot*8+row;
   int t=slot/4,q=slot%4;
   for(col=0;col<8;col++){
      int yy=(q/2)*8+row,xx=(q%2)*8+col;
      rgb[col]=editor_sprite_palette[forge_tile_pixels_const(p,t)[yy*16+xx]&15];
   }
   encode_row(rgb,&patterns[off],&colors[off]);
 }
}

// TMS9918/V9938 sprite-2 16x16: 16 rows left, then 16 rows right.
// Bit zero is TRANSPARENT: it reveals the unchanged background tile.
// Each scanline receives its own foreground color in sprite mode 2.
void forge_screen4_hw_sprite(const uint8_t sprite[256],uint8_t pattern[32],
                             uint8_t line_colors[16]){
 int y,x,c,counts[16],best,bestCount;
 for(y=0;y<16;y++){
   pattern[y]=pattern[y+16]=0;
   for(c=0;c<16;c++)counts[c]=0;
   for(x=0;x<16;x++){
     uint8_t pix=sprite[y*16+x];
     if(pix==255)continue;
     if(x<8)pattern[y]|=(uint8_t)(0x80>>x);
     else pattern[y+16]|=(uint8_t)(0x80>>(x-8));
     counts[palette_index(editor_sprite_palette[pix&15])]++;
   }
   best=0;bestCount=0;
   for(c=1;c<16;c++)if(counts[c]>bestCount){bestCount=counts[c];best=c;}
   line_colors[y]=(uint8_t)best;
 }
}
