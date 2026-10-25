#include "engine.h"
#include "minicrt.h"

#include "rom_patterns.h"
#include "msx2_graphics.h"
#include "font8x8.h"
#define RBASE 0x4000
#define RM_X 0xc000
#define RM_Y 0xc001
#define RM_CX 0xc002
#define RM_CY 0xc003
#define RM_SCENE 0xc008
#define RM_KEY 0xc100
#define RM_FACING 0xc009
#define RM_FRAME 0xc00a
#define RM_NPC_X 0xc180
#define RM_NPC_Y 0xc188
#define RM_NPC_CLOCK 0xc190
#define RM_NPC_CYCLE 0xc191
#define RM_NPC_SEED 0xc192
#define RM_NPC_STEP 0xc1a0
#define RM_NPC_AGE 0xc1a8
#define RM_SPOKEN 0xc1c0 // 16 scenes * 8 NPCs, initialized on boot
#define RM_FLAGS 0xc270 // four persistent-in-run mission flags
// Physical movement inputs can repeat, but one tile is accepted at most every
// 8 VBlank ticks in the same direction (about 133ms at 60Hz).
// Keeps quick single taps responsive and prevents BIOS CHGET repeats from
// suddenly moving many tiles.  Runtime-only; .msxf format unchanged.
#define RM_MOVE_TICK 0xc274
#define RM_MOVE_DIRECTION 0xc275
#define RM_MOVE_REQUEST 0xc276
#define RM_COINS 0xc277
#define RM_REWARD_CLAIMED 0xc278
#define RMAXLABELS 1800
#define RMAXFIX 4000

typedef struct {char name[40];int addr;} RL;
typedef struct {char name[40];int offset;} RF;
typedef struct {uint8_t *rom;int pc,failed;RL labels[RMAXLABELS];RF fix[RMAXFIX];int lc,fc;} RA;
static void emit(RA*a,int b){if(a->pc>=32768){a->failed=1;return;}a->rom[a->pc++]=(uint8_t)b;}
static void ew(RA*a,int w){emit(a,w);emit(a,w>>8);}
static void name2(char*dest,const char*prefix,int a,int b){int i=0;while(*prefix&&i<28)dest[i++]=*prefix++;dest[i++]='_';dest[i++]=(char)('0'+(a/10)%10);dest[i++]=(char)('0'+a%10);if(b>=0){dest[i++]='_';dest[i++]=(char)('0'+(b/10)%10);dest[i++]=(char)('0'+b%10);}dest[i]=0;}
static void mark(RA*a,const char*n){int i;if(a->lc>=RMAXLABELS){a->failed=1;return;}for(i=0;i<a->lc;i++)if(strcmp(a->labels[i].name,n)==0){a->failed=1;return;}forge_copy_string(a->labels[a->lc].name,40,n);a->labels[a->lc++].addr=a->pc+RBASE;}
static void ref(RA*a,const char*n){if(a->fc>=RMAXFIX){a->failed=1;return;}forge_copy_string(a->fix[a->fc].name,40,n);a->fix[a->fc++].offset=a->pc;ew(a,0);}
static void jp(RA*a,const char*n){emit(a,0xc3);ref(a,n);}static void jz(RA*a,const char*n){emit(a,0xca);ref(a,n);}static void jnz(RA*a,const char*n){emit(a,0xc2);ref(a,n);}static void jnc(RA*a,const char*n){emit(a,0xd2);ref(a,n);}static void jc(RA*a,const char*n){emit(a,0xda);ref(a,n);}
static void call(RA*a,const char*n){emit(a,0xcd);ref(a,n);}static void bios(RA*a,int addr){emit(a,0xcd);ew(a,addr);}
static void ldh(RA*a,const char*n){emit(a,0x21);ref(a,n);}static void get(RA*a,int addr){emit(a,0x3a);ew(a,addr);}
static void put(RA*a,int addr){emit(a,0x32);ew(a,addr);}static void lda(RA*a,int x){emit(a,0x3e);emit(a,x);}static void cpn(RA*a,int x){emit(a,0xfe);emit(a,x);}static void andn(RA*a,int x){emit(a,0xe6);emit(a,x);}static void print(RA*a,const char*n){ldh(a,n);call(a,"printStr");}
static void string_rom(RA*a,const char*s,int limit){int i;for(i=0;s&&s[i]&&i<limit;i++){int v=(unsigned char)s[i];emit(a,v<32||v>126?'?':v);}emit(a,0);}
// A SCREEN 1 / SCREEN 4 dialogue window contains two rows of 32 characters.
// Reserve its last 2 columns for a page indicator, and wrap at spaces.
// Encoded text uses CR (13) for a row and FF (12) for a page transition.
// No source dialogue is silently truncated at 56 characters.
#define DIALOG_COLUMNS 30
static void dialog_rom(RA*a,const char*s){
  int pos=0,row=0,col=0;
  if(!s){emit(a,0);return;}
  while(s[pos]){
    int start,len,k,next;
    // Explicit line breaks are supported by the project text editor.
    if(s[pos]=='\r'||s[pos]=='\n'){
       if(s[pos]=='\r'&&s[pos+1]=='\n')pos++;
       pos++;
       if(row==0){emit(a,13);row=1;col=0;}
       else {emit(a,12);row=0;col=0;}
       continue;
    }
    if(s[pos]==' '||s[pos]=='\t'){pos++;continue;}
    start=pos;
    while(s[pos]&&s[pos]!=' '&&s[pos]!='\t'&&s[pos]!='\r'&&s[pos]!='\n')pos++;
    len=pos-start;
    // Fit a word on the current line. A long uninterrupted word can
    // still be split across rows without overrunning SCREEN 4 VRAM.
    while(len>0){
      if(col && (col+1+len)>DIALOG_COLUMNS){
         if(row==0){emit(a,13);row=1;}else {emit(a,12);row=0;}
         col=0;
      }
      if(col){emit(a,' ');col++;}
      next=DIALOG_COLUMNS-col;
      if(next>len)next=len;
      for(k=0;k<next;k++){
        int v=(unsigned char)s[start+k];
        emit(a,v>=32&&v<=126?v:'?');
      }
      col+=next;start+=next;len-=next;
      if(len){
        if(row==0){emit(a,13);row=1;}else {emit(a,12);row=0;}
        col=0;
      }
    }
  }
  emit(a,0);
}
// Convert the editor's 16x16 indexed sprite into four SCREEN 1 8x8 patterns.
// The current cross-generation ROM profile has one foreground color per 8 glyphs.
// 255 is the only transparent index; all 0..15 are foreground/opaque.
// Pattern order matches drawPlayer: top-left, top-right, bottom-left, bottom-right.
void forge_sprite_1bit(const uint8_t sprite[256],uint8_t out[32]){
    int qx,qy,row,col;
    if(!sprite||!out)return;
    for(qy=0;qy<2;qy++)for(qx=0;qx<2;qx++)for(row=0;row<8;row++){
        uint8_t bits=0;
        for(col=0;col<8;col++)if(sprite[(qy*8+row)*16+qx*8+col]!=255)
            bits|=(uint8_t)(0x80>>col);
        out[(qy*2+qx)*8+row]=bits;
    }
}

static int total_npcs(const ForgeProject*p){int i,n=0;for(i=0;i<p->scene_count;i++)n+=p->scenes[i].npc_count;return n;}
static void npc_at(const ForgeProject*p,int flat,int *scene,const ForgeNPC**npc){int i;for(i=0;i<p->scene_count;i++){if(flat<p->scenes[i].npc_count){*npc=&p->scenes[i].npcs[flat];*scene=i;return;}flat-=p->scenes[i].npc_count;}*scene=0;*npc=0;}
// v0.5.2: 1 / 2 choice. The original two NPC messages become the
// outcome text for the YES and NO branches, including normal pagination.
static void emit_choice(RA*a,const ForgeProject*p,int npc_index,int gives_key,int set_flag,int flag_index){
 char lab[40],val[40];
 call(a,"msgPos");name2(lab,"npcQuestion",npc_index,-1);print(a,lab);
 call(a,"waitFreshSpace"); // Wait for a NEW Space press; no auto-repeat.
 call(a,"msgPos");print(a,"chooseOptions");
 name2(lab,"choiceRead",npc_index,-1);mark(a,lab);
 bios(a,0x009f);cpn(a,p->key_yes);name2(val,"choiceYes",npc_index,-1);jz(a,val);
 if(p->key_yes>='A'&&p->key_yes<='Z'){cpn(a,p->key_yes+32);jz(a,val);}
 cpn(a,p->key_no);name2(val,"choiceNo",npc_index,-1);jz(a,val);
 if(p->key_no>='A'&&p->key_no<='Z'){cpn(a,p->key_no+32);jz(a,val);}
 jp(a,lab);
 name2(lab,"choiceYes",npc_index,-1);mark(a,lab);
 if(gives_key){lda(a,1);put(a,RM_KEY);}
 if(set_flag && flag_index<FORGE_FLAG_COUNT){lda(a,1);put(a,RM_FLAGS+flag_index);}
 call(a,"msgPos");name2(val,"npcFirst",npc_index,-1);print(a,val);
 call(a,"waitFreshSpace");call(a,"drawScene");call(a,"drawPlayer");jp(a,"loop");
 name2(lab,"choiceNo",npc_index,-1);mark(a,lab);
 call(a,"msgPos");name2(val,"npcAgainText",npc_index,-1);print(a,val);
 call(a,"waitFreshSpace");call(a,"drawScene");call(a,"drawPlayer");jp(a,"loop");
}
// Mission reward: one collection per global mission flag, on the first
// completed-flag conversation with a reward-giving NPC.
static void emit_mission_reward(RA *a,const ForgeProject *p,int scene,int slot,int flat){
 int flag=p->npc_flag_index[scene][slot],reward=p->npc_reward[scene][slot];
 if(p->npc_event_mode[scene][slot]!=EVENT_FLAG_BRANCH||reward==0||flag>=FORGE_FLAG_COUNT)return;
 char skip[40];name2(skip,"rewardDone",flat,-1);
 get(a,RM_REWARD_CLAIMED+flag);emit(a,0xb7);jnz(a,skip);
 lda(a,1);put(a,RM_REWARD_CLAIMED+flag);
 get(a,RM_COINS);emit(a,0xc6);emit(a,reward==1?5:reward==2?10:20);put(a,RM_COINS);
 mark(a,skip);
}
// ROM export uses SCREEN 1 to guarantee compatibility across MSX, MSX2 and MSX2+.
// 16x12 logical cells map to 2x2 character patterns, exactly like the runtime tile grid.
int forge_export_rom(const ForgeProject*p,uint8_t*buf,int cap,char*error,int errcap){int i,j,x,y,scn,n;char lab[40],lab2[40];static RA a;char val[160];if(!buf||cap<32768){forge_copy_string(error,errcap,"Buffer ROM menor que 32768 bytes.");return 0;}if(!forge_validate(p,error,errcap))return 0;
memset(&a,0,sizeof(a));a.rom=buf;memset(buf,255,32768);buf[0]='A';buf[1]='B';buf[2]=0x10;buf[3]=0x40;for(i=4;i<16;i++)buf[i]=0;a.pc=16;
mark(&a,"init");
if(p->profile==PROFILE_MSX){
  bios(&a,0x006f); // Retained SCREEN 1 export for original MSX.
  ldh(&a,"graphicPatterns");emit(&a,0x11);ew(&a,0x0400);
  emit(&a,0x01);ew(&a,sizeof(rom_patterns));bios(&a,0x005c);
  {static const int color[]={0x34,0x24,0xb4,0x54,0x64,0x94,0xc4,0x84,0xf4,0x84};for(i=0;i<10;i++){
   emit(&a,0x21);ew(&a,0x2010+i);lda(&a,color[i]);bios(&a,0x004d);
  }}
}else{
  // MSX2/MSX2+: SCREEN 4 (GRAPHIC 3), 3 horizontal banks.
  // Each bank has 640 bytes of 8x8 patterns and 640 bytes of row colors.
  lda(&a,4);bios(&a,0x005f); // CHGMOD: SCREEN 4.
  for(i=0;i<3;i++){
    ldh(&a,"graphicPatterns");emit(&a,0x11);ew(&a,0x0400+i*0x800);
    emit(&a,0x01);ew(&a,MSX2_PATTERN_BYTES);bios(&a,0x005c);
    ldh(&a,"graphicColors");emit(&a,0x11);ew(&a,0x2400+i*0x800);
    emit(&a,0x01);ew(&a,MSX2_PATTERN_BYTES);bios(&a,0x005c);
  }
  // Keep the graphic font available across the three SCREEN 4 banks.
  // NPC dialogue is shown only in the lower two-line dialog window.
  for(i=0;i<3;i++){
    ldh(&a,"fontPatterns");emit(&a,0x11);ew(&a,0x0100+i*0x800);
    emit(&a,0x01);ew(&a,sizeof(forge_font_8x8));bios(&a,0x005c);
    emit(&a,0x21);ew(&a,0x2100+i*0x800);emit(&a,0x01);ew(&a,sizeof(forge_font_8x8));
    lda(&a,0x1f);bios(&a,0x0056); // Dark text on light balloon / dialog background.
  }
  // 16x16 hardware sprites (SCREEN 4 sprite mode 2).
  emit(&a,0x01);ew(&a,0xe201);bios(&a,0x0047); // WRTVDP: B=E2 C=1.
  ldh(&a,"hardwareSprites");emit(&a,0x11);ew(&a,0x3800);
  emit(&a,0x01);ew(&a,448);bios(&a,0x005c);
}
// The active scene MUST be set before initNpcs reads it.  Fresh MSX2 RAM is
// not guaranteed to contain zero: if the scene byte was random, NPC positions
// were left uninitialized and sprites appeared at the top-left of the screen.
lda(&a,0);put(&a,RM_SCENE);put(&a,RM_FACING);put(&a,RM_FRAME);
lda(&a,0);put(&a,RM_KEY);lda(&a,p->scenes[0].spawn_x);put(&a,RM_X);lda(&a,p->scenes[0].spawn_y);put(&a,RM_Y);
// Clear per-NPC conversation flags; MSX RAM is undefined after power-on.
emit(&a,0x21);ew(&a,RM_SPOKEN);emit(&a,0x36);emit(&a,0);
emit(&a,0x11);ew(&a,RM_SPOKEN+1);emit(&a,0x01);ew(&a,MAX_SCENES*MAX_NPCS-1);
emit(&a,0xed);emit(&a,0xb0); // LDIR fills all flags with zero.
for(i=0;i<FORGE_FLAG_COUNT;i++){lda(&a,0);put(&a,RM_FLAGS+i);put(&a,RM_REWARD_CLAIMED+i);}
lda(&a,0);put(&a,RM_COINS);
lda(&a,0);put(&a,RM_MOVE_DIRECTION);put(&a,RM_MOVE_TICK);

if(p->profile!=PROFILE_MSX){lda(&a,255);put(&a,RM_NPC_CLOCK);put(&a,RM_NPC_CYCLE);lda(&a,57);put(&a,RM_NPC_SEED);call(&a,"initNpcs");}
call(&a,"drawScene");call(&a,"drawPlayer");
mark(&a,"loop");if(p->profile!=PROFILE_MSX){emit(&a,0x76); /* HALT: wait for VBlank */call(&a,"tickNpcs");bios(&a,0x009c);jz(&a,"loop");}bios(&a,0x009f);emit(&a,0xfe);emit(&a,'w');jz(&a,"up");emit(&a,0xfe);emit(&a,'W');jz(&a,"up");emit(&a,0xfe);emit(&a,'s');jz(&a,"down");emit(&a,0xfe);emit(&a,'S');jz(&a,"down");emit(&a,0xfe);emit(&a,'a');jz(&a,"left");emit(&a,0xfe);emit(&a,'A');jz(&a,"left");emit(&a,0xfe);emit(&a,'d');jz(&a,"right");emit(&a,0xfe);emit(&a,'D');jz(&a,"right");emit(&a,0xfe);emit(&a,p->key_talk);jz(&a,"talkPressed");
 if(p->key_talk>='A'&&p->key_talk<='Z'){emit(&a,0xfe);emit(&a,p->key_talk+32);jz(&a,"talkPressed");}
 emit(&a,0xfe);emit(&a,p->key_inventory);jz(&a,"bag");
 emit(&a,0xfe);emit(&a,p->key_inventory+32);jz(&a,"bag");jp(&a,"loop");
mark(&a,"up");lda(&a,1);put(&a,RM_MOVE_REQUEST);call(&a,"moveReady");emit(&a,0xb7);jz(&a,"loop");lda(&a,1);put(&a,RM_FACING);get(&a,RM_Y);emit(&a,0xb7);jz(&a,"loop");emit(&a,0x3d);put(&a,RM_CY);get(&a,RM_X);put(&a,RM_CX);jp(&a,"tryMove");
mark(&a,"down");lda(&a,2);put(&a,RM_MOVE_REQUEST);call(&a,"moveReady");emit(&a,0xb7);jz(&a,"loop");lda(&a,0);put(&a,RM_FACING);get(&a,RM_Y);emit(&a,0xfe);emit(&a,FH-1);jz(&a,"loop");emit(&a,0x3c);put(&a,RM_CY);get(&a,RM_X);put(&a,RM_CX);jp(&a,"tryMove");
mark(&a,"left");lda(&a,3);put(&a,RM_MOVE_REQUEST);call(&a,"moveReady");emit(&a,0xb7);jz(&a,"loop");lda(&a,2);put(&a,RM_FACING);get(&a,RM_X);emit(&a,0xb7);jz(&a,"loop");emit(&a,0x3d);put(&a,RM_CX);get(&a,RM_Y);put(&a,RM_CY);jp(&a,"tryMove");
mark(&a,"right");lda(&a,4);put(&a,RM_MOVE_REQUEST);call(&a,"moveReady");emit(&a,0xb7);jz(&a,"loop");lda(&a,3);put(&a,RM_FACING);get(&a,RM_X);emit(&a,0xfe);emit(&a,FW-1);jz(&a,"loop");emit(&a,0x3c);put(&a,RM_CX);get(&a,RM_Y);put(&a,RM_CY);jp(&a,"tryMove");
mark(&a,"tryMove");call(&a,"walkable");emit(&a,0xfe);emit(&a,2);jz(&a,"checkDoor");emit(&a,0xb7);jz(&a,"blocked");if(p->profile!=PROFILE_MSX){call(&a,"npcOccupied");emit(&a,0xb7);jnz(&a,"blocked");}mark(&a,"accept");get(&a,RM_FRAME);emit(&a,0xee);emit(&a,1);put(&a,RM_FRAME);get(&a,RM_CX);put(&a,RM_X);get(&a,RM_CY);put(&a,RM_Y);call(&a,"portalTest");call(&a,"drawScene");call(&a,"drawPlayer");jp(&a,"loop");
mark(&a,"checkDoor");get(&a,RM_KEY);emit(&a,0xfe);emit(&a,1);jz(&a,"accept");call(&a,"msgPos");print(&a,"doorText");jp(&a,"loop");
mark(&a,"blocked");call(&a,"msgPos");print(&a,"wallText");jp(&a,"loop");
mark(&a,"bag");get(&a,RM_COINS);emit(&a,0xb7);jz(&a,"bagNoCoins");
call(&a,"msgPos");print(&a,"bagCoins");call(&a,"printCoins");jp(&a,"loop");
mark(&a,"bagNoCoins");get(&a,RM_KEY);emit(&a,0xfe);emit(&a,1);jz(&a,"keyYes");call(&a,"msgPos");print(&a,"bagNo");jp(&a,"loop");
mark(&a,"keyYes");call(&a,"msgPos");print(&a,"bagYes");jp(&a,"loop");
// Print a two-digit reward total (at most 80 coins with four flags).
mark(&a,"printCoins");
get(&a,RM_COINS);emit(&a,0x06);emit(&a,0); // LD B,0 tens
mark(&a,"countTens");cpn(&a,10);jc(&a,"countDone");emit(&a,0xd6);emit(&a,10);emit(&a,0x04);jp(&a,"countTens");
mark(&a,"countDone");emit(&a,0x4f); // LD C,A (ones)
emit(&a,0x78);emit(&a,0xc6);emit(&a,'0');put(&a,0xc27c); // tens
emit(&a,0x79);emit(&a,0xc6);emit(&a,'0');put(&a,0xc27d); // ones
get(&a,0xc27c);
if(p->profile==PROFILE_MSX){bios(&a,0x00a2);}else{emit(&a,0x21);ew(&a,0x1ac9);bios(&a,0x004d);}
get(&a,0xc27d);
if(p->profile==PROFILE_MSX){bios(&a,0x00a2);}else{emit(&a,0x21);ew(&a,0x1aca);bios(&a,0x004d);}
emit(&a,0xc9);
// CHGET has delivered Space: swallow keyboard repeat until that physical key
// is released. The following dialogue pages require NEW presses.
mark(&a,"talkPressed");call(&a,"waitSpaceUp");jp(&a,"talk");
// Movement rate limiting is independent of CHGET's auto-repeat rate.
// A new direction moves immediately. A held direction cannot step again
// until at least 8 BIOS JIFFY frames have elapsed, including 8-bit wrap.
mark(&a,"moveReady");
get(&a,RM_MOVE_DIRECTION);emit(&a,0x21);ew(&a,RM_MOVE_REQUEST);
emit(&a,0xbe);jz(&a,"moveSameDirection");jp(&a,"moveAccept");
mark(&a,"moveSameDirection");
get(&a,RM_MOVE_TICK);emit(&a,0x5f); // E=last accepted JIFFY
get(&a,0xfc9e);emit(&a,0x93); // SUB E: elapsed, modulo 256
cpn(&a,8);jc(&a,"moveReject");
mark(&a,"moveAccept");get(&a,RM_MOVE_REQUEST);put(&a,RM_MOVE_DIRECTION);
get(&a,0xfc9e);put(&a,RM_MOVE_TICK);lda(&a,1);emit(&a,0xc9);
mark(&a,"moveReject");lda(&a,0);emit(&a,0xc9);
// tile index (HL=y*16+x)
mark(&a,"tileIndex");get(&a,RM_CY);emit(&a,0x6f);emit(&a,0x26);emit(&a,0);for(i=0;i<4;i++)emit(&a,0x29);get(&a,RM_CX);emit(&a,0x5f);emit(&a,0x16);emit(&a,0);emit(&a,0x19);emit(&a,0xc9);
mark(&a,"walkable");call(&a,"tileIndex");emit(&a,0xe5);call(&a,"passableBase");emit(&a,0xe1);emit(&a,0x19);emit(&a,0x7e);emit(&a,0xc9);
mark(&a,"passableBase");get(&a,RM_SCENE);for(i=0;i<p->scene_count;i++){emit(&a,0xfe);emit(&a,i);name2(lab,"pb",i,-1);jz(&a,lab);}jp(&a,"pb_00");for(i=0;i<p->scene_count;i++){name2(lab,"pb",i,-1);mark(&a,lab);emit(&a,0x11);name2(lab2,"passable",i,-1);ref(&a,lab2);emit(&a,0xc9);}
mark(&a,"drawScene");get(&a,RM_SCENE);for(i=0;i<p->scene_count;i++){emit(&a,0xfe);emit(&a,i);name2(lab,"sceneDraw",i,-1);jz(&a,lab);}jp(&a,"sceneDraw_00");for(i=0;i<p->scene_count;i++){name2(lab,"sceneDraw",i,-1);mark(&a,lab);name2(lab2,"nameTable",i,-1);ldh(&a,lab2);
 if(p->profile==PROFILE_MSX) {emit(&a,0x11);ew(&a,0x1800);emit(&a,0x01);ew(&a,768);bios(&a,0x005c);}
 else call(&a,"expandScene");
 if(p->profile!=PROFILE_MSX){
  name2(lab2,"spriteAttrs",i,-1);ldh(&a,lab2);emit(&a,0x11);ew(&a,0x1e00);
  emit(&a,0x01);ew(&a,128);bios(&a,0x005c);
  name2(lab2,"spriteColor",i,-1);ldh(&a,lab2);emit(&a,0x11);ew(&a,0x1c00);
  emit(&a,0x01);ew(&a,16*(MAX_NPCS+1));bios(&a,0x005c);
  name2(lab2,"npcSync",i,-1);call(&a,lab2);
 }
 emit(&a,0xc9);}
// MSX2 packed scene expansion. 192 tile IDs replace 768 name-table bytes
// in each ROM scene. Expand into a safe 768-byte RAM scratch buffer at C400,
// then transfer it to SCREEN 4 name table with the existing LDIRVM BIOS.
if(p->profile!=PROFILE_MSX){
 mark(&a,"expandScene");
 emit(&a,0x11);ew(&a,0xc400); // LD DE, scratch
 lda(&a,12);put(&a,0xc280); // 12 tile rows
 mark(&a,"expandRows");
 emit(&a,0x06);emit(&a,16); // LD B,16
 mark(&a,"expandTop");
 emit(&a,0x7e);emit(&a,0x23); // LD A,(HL); INC HL
 emit(&a,0x87);emit(&a,0x87); // tile*4
 emit(&a,0xc6);emit(&a,128); // ADD A,128
 emit(&a,0x12);emit(&a,0x13);emit(&a,0x3c);emit(&a,0x12);emit(&a,0x13); // (DE) x2
 emit(&a,0x10);emit(&a,(uint8_t)(-13)); // DJNZ expandTop
 emit(&a,0xe5); // PUSH HL
 emit(&a,0x01);ew(&a,0xfff0);emit(&a,0x09); // ADD HL,-16
 emit(&a,0x06);emit(&a,16);
 mark(&a,"expandBottom");
 emit(&a,0x7e);emit(&a,0x23);emit(&a,0x87);emit(&a,0x87);
 emit(&a,0xc6);emit(&a,130); // tile*4+130
 emit(&a,0x12);emit(&a,0x13);emit(&a,0x3c);emit(&a,0x12);emit(&a,0x13);
 emit(&a,0x10);emit(&a,(uint8_t)(-13));
 emit(&a,0xe1); // POP HL
 get(&a,0xc280);emit(&a,0x3d);put(&a,0xc280);jnz(&a,"expandRows");
 emit(&a,0x21);ew(&a,0xc400);emit(&a,0x11);ew(&a,0x1800);
 emit(&a,0x01);ew(&a,768);bios(&a,0x005c);
 emit(&a,0xc9);
}
mark(&a,"portalTest");for(i=0;i<p->scene_count;i++){const ForgeScene*s=&p->scenes[i];for(j=0;j<s->portal_count;j++){const ForgePortal*q=&s->portals[j];int lx=0,ly=0;forge_portal_arrival(p,i,q,&lx,&ly);name2(lab,"ptEnd",i,j);get(&a,RM_SCENE);emit(&a,0xfe);emit(&a,i);jnz(&a,lab);get(&a,RM_X);emit(&a,0xfe);emit(&a,q->x);jnz(&a,lab);get(&a,RM_Y);emit(&a,0xfe);emit(&a,q->y);jnz(&a,lab);lda(&a,q->to);put(&a,RM_SCENE);lda(&a,lx);put(&a,RM_X);lda(&a,ly);put(&a,RM_Y);if(p->profile!=PROFILE_MSX)call(&a,"initNpcs");emit(&a,0xc9);mark(&a,lab);}}
emit(&a,0xc9);
mark(&a,"screenAddr");get(&a,RM_Y);emit(&a,0x6f);emit(&a,0x26);emit(&a,0);for(i=0;i<6;i++)emit(&a,0x29);get(&a,RM_X);emit(&a,0x87);emit(&a,0x5f);emit(&a,0x16);emit(&a,0);emit(&a,0x19);emit(&a,0x11);ew(&a,0x1800);emit(&a,0x19);emit(&a,0xc9);
mark(&a,"drawPlayer");
if(p->profile==PROFILE_MSX){
 for(i=0;i<4;i++){call(&a,"screenAddr");if(i&1)emit(&a,0x23);if(i>=2){emit(&a,0x11);ew(&a,32);emit(&a,0x19);}lda(&a,192+i);bios(&a,0x004d);}
}else{
 // A direction has two 16x16 frames, each one 32 bytes / 4 pattern numbers.
 // Frame index = facing*2 + walk_frame, sourced from RAM set by movement.
 get(&a,RM_FACING);emit(&a,0x87);emit(&a,0x5f); // E = direction*2
 get(&a,RM_FRAME);emit(&a,0x83); // ADD A,E = frame id; simulator supports this
 emit(&a,0xf5); // PUSH AF, preserved across BIOS calls
 for(i=0;i<2;i++)emit(&a,0x87); // pattern id = frame*4
 emit(&a,0x21);ew(&a,0x1e02);bios(&a,0x004d);
 emit(&a,0xf1); // POP AF = frame id
 // The pointer to per-line sprite colors = playerLineColors + frame*16.
 emit(&a,0x6f);emit(&a,0x26);emit(&a,0);
 for(i=0;i<4;i++)emit(&a,0x29);
 emit(&a,0x11);ref(&a,"playerLineColors");emit(&a,0x19);
 emit(&a,0x11);ew(&a,0x1c00);emit(&a,0x01);ew(&a,16);bios(&a,0x005c);
 // Sprite #0 remains transparent and follows the player's tile position.
 get(&a,RM_Y);for(i=0;i<4;i++)emit(&a,0x87);emit(&a,0x3d);
 emit(&a,0x21);ew(&a,0x1e00);bios(&a,0x004d);
 get(&a,RM_X);for(i=0;i<4;i++)emit(&a,0x87);
 emit(&a,0x21);ew(&a,0x1e01);bios(&a,0x004d);
}
emit(&a,0xc9);
if(p->profile==PROFILE_MSX){
mark(&a,"talk");n=total_npcs(p);for(i=0;i<n;i++){const ForgeNPC*q;npc_at(p,i,&scn,&q);name2(lab,"skipNpc",i,-1);get(&a,RM_SCENE);emit(&a,0xfe);emit(&a,scn);jnz(&a,lab);for(j=0;j<4;j++){int nx=q->x+(j==0)-(j==1),ny=q->y+(j==2)-(j==3);if(nx<0||ny<0||nx>=FW||ny>=FH)continue;get(&a,RM_X);emit(&a,0xfe);emit(&a,nx);name2(lab2,"skipNeighbor",i,j);jnz(&a,lab2);get(&a,RM_Y);emit(&a,0xfe);emit(&a,ny);name2(val,"npc",i,-1);jz(&a,val);mark(&a,lab2);}mark(&a,lab);}jp(&a,"loop");
for(i=0;i<n;i++){
 const ForgeNPC*q;npc_at(p,i,&scn,&q);name2(lab,"npc",i,-1);mark(&a,lab);
 int slot=i;for(j=0;j<scn;j++)slot-=p->scenes[j].npc_count;
 int mode=p->npc_event_mode[scn][slot];int flag=RM_SPOKEN+scn*MAX_NPCS+slot;
 if(mode==EVENT_CHOICE){emit_choice(&a,p,i,q->gives_key,p->npc_choice_set_flag[scn][slot],p->npc_flag_index[scn][slot]);continue;}
 if(mode==EVENT_FLAG_BRANCH){get(&a,RM_FLAGS+p->npc_flag_index[scn][slot]);cpn(&a,1);}
 else if(mode==EVENT_KEY_BRANCH||q->gives_key){get(&a,RM_KEY);cpn(&a,1);}
 else {get(&a,flag);cpn(&a,1);}
 name2(lab2,"npcAgain",i,-1);jz(&a,lab2);
 if(q->gives_key){lda(&a,1);put(&a,RM_KEY);}
 lda(&a,1);put(&a,flag);
 call(&a,"msgPos");name2(val,"npcFirst",i,-1);print(&a,val);
 call(&a,"waitFreshSpace");call(&a,"drawScene");call(&a,"drawPlayer");jp(&a,"loop");
 mark(&a,lab2);emit_mission_reward(&a,p,scn,slot,i);call(&a,"msgPos");name2(val,"npcAgainText",i,-1);print(&a,val);
 call(&a,"waitFreshSpace");call(&a,"drawScene");call(&a,"drawPlayer");jp(&a,"loop");
}

}else{
mark(&a,"talk");n=total_npcs(p);
for(i=0;i<n;i++){
 const ForgeNPC*q;npc_at(p,i,&scn,&q);
 int slot=i;for(j=0;j<scn;j++)slot-=p->scenes[j].npc_count;
 name2(lab,"skipNpc",i,-1);
 get(&a,RM_SCENE);cpn(&a,scn);jnz(&a,lab);
 for(j=0;j<4;j++){
   name2(lab2,"skipNeighbor",i,j);
   get(&a,RM_X);
   if(j==0)emit(&a,0x3c); // player to the left -> x+1
   else if(j==1)emit(&a,0x3d);
   emit(&a,0x21);ew(&a,RM_NPC_X+slot);emit(&a,0xbe);jnz(&a,lab2);
   get(&a,RM_Y);
   if(j==2)emit(&a,0x3c);
   else if(j==3)emit(&a,0x3d);
   emit(&a,0x21);ew(&a,RM_NPC_Y+slot);emit(&a,0xbe);
   name2(val,"npc",i,-1);jz(&a,val);
   mark(&a,lab2);
 }
 mark(&a,lab);
}
jp(&a,"loop");
for(i=0;i<n;i++){
 const ForgeNPC*q;npc_at(p,i,&scn,&q);
 name2(lab,"npc",i,-1);mark(&a,lab);
 int event_slot=i;for(j=0;j<scn;j++)event_slot-=p->scenes[j].npc_count;
 int key_branch=p->npc_event_mode[scn][event_slot]==EVENT_KEY_BRANCH;
 if(p->npc_event_mode[scn][event_slot]==EVENT_CHOICE){emit_choice(&a,p,i,q->gives_key,p->npc_choice_set_flag[scn][event_slot],p->npc_flag_index[scn][event_slot]);continue;}
 int spoken_flag=RM_SPOKEN+scn*MAX_NPCS+event_slot;
 if(p->npc_event_mode[scn][event_slot]==EVENT_FLAG_BRANCH){get(&a,RM_FLAGS+p->npc_flag_index[scn][event_slot]);cpn(&a,1);}
 else if(q->gives_key||key_branch){get(&a,RM_KEY);cpn(&a,1);}
 else {get(&a,spoken_flag);cpn(&a,1);}
 name2(lab2,"npcAgain",i,-1);jz(&a,lab2);
 if(q->gives_key){lda(&a,1);put(&a,RM_KEY);}
 lda(&a,1);put(&a,spoken_flag);
 // No overhead balloons: use only the lower two-line dialog window.
 call(&a,"msgPos");name2(val,"npcFirst",i,-1);print(&a,val);
 call(&a,"waitFreshSpace");call(&a,"drawScene");call(&a,"drawPlayer");jp(&a,"loop");
 name2(lab2,"npcAgain",i,-1);mark(&a,lab2);
 emit_mission_reward(&a,p,scn,event_slot,i);
 call(&a,"msgPos");name2(val,"npcAgainText",i,-1);print(&a,val);
 call(&a,"waitFreshSpace");call(&a,"drawScene");call(&a,"drawPlayer");jp(&a,"loop");
}

}
mark(&a,"msgPos");
// Clear two name-table rows before writing the next dialogue page.  The
// bottom of SCREEN 1 and SCREEN 4 use the same name-table address.
ldh(&a,"blankMessage");emit(&a,0x11);ew(&a,0x1ac0);
emit(&a,0x01);ew(&a,64);bios(&a,0x005c);
if(p->profile==PROFILE_MSX){
 lda(&a,0);emit(&a,0x67);lda(&a,22);emit(&a,0x6f);bios(&a,0x00c6);
}
emit(&a,0xc9);
if(p->profile==PROFILE_MSX){
 mark(&a,"printStr");
 mark(&a,"printNext");emit(&a,0x7e);emit(&a,0xb7);jz(&a,"endStr");
 emit(&a,0xfe);emit(&a,12);jz(&a,"nextDialogPage");
 emit(&a,0xfe);emit(&a,13);jz(&a,"msx1Line2");
 emit(&a,0xe5);bios(&a,0x00a2);emit(&a,0xe1);
 mark(&a,"skipPrint");emit(&a,0x23);jp(&a,"printNext");
 mark(&a,"msx1Line2");emit(&a,0xe5);lda(&a,13);bios(&a,0x00a2);lda(&a,10);bios(&a,0x00a2);emit(&a,0xe1);jp(&a,"skipPrint");
}else{
 // Write glyph codes directly into SCREEN 4 name-table VRAM.
 mark(&a,"printStr");emit(&a,0x11);ew(&a,0x1ac0);
 mark(&a,"printNext");emit(&a,0x7e);emit(&a,0xb7);jz(&a,"endStr");
 emit(&a,0xfe);emit(&a,13);jz(&a,"printLine2");
 emit(&a,0xfe);emit(&a,10);jz(&a,"skipPrint");
 emit(&a,0xfe);emit(&a,12);jz(&a,"nextDialogPage");
 emit(&a,0xe5);
 emit(&a,0xeb);bios(&a,0x004d);emit(&a,0xeb);
 emit(&a,0xe1);emit(&a,0x13);
 mark(&a,"skipPrint");emit(&a,0x23);jp(&a,"printNext");
 mark(&a,"printLine2");emit(&a,0x11);ew(&a,0x1ae0);jp(&a,"skipPrint");
}
mark(&a,"nextDialogPage");
emit(&a,0xe5); // Preserve text pointer across BIOS calls.
lda(&a,'>');emit(&a,0x21);ew(&a,0x1afe);bios(&a,0x004d);
lda(&a,'>');emit(&a,0x21);ew(&a,0x1aff);bios(&a,0x004d);
call(&a,"waitFreshSpace"); // Only a NEW Space press acknowledges a page.
call(&a,"msgPos");
emit(&a,0xe1);
if(p->profile!=PROFILE_MSX){emit(&a,0x11);ew(&a,0x1ac0);}
emit(&a,0x23);jp(&a,"printNext");
mark(&a,"endStr");emit(&a,0xc9);
// Modal dialogue input is based on the PHYSICAL Space key, not CHGET's
// auto-repeating keyboard buffer. GTTRIG(0) reports FF while Space is down.
// Wait for the release, then clear any queued repeat characters with KILBUF.
// HALT gives the BIOS keyboard scanner time to observe a new key state.
mark(&a,"waitSpaceUp");
mark(&a,"spaceUpPoll");if(p->key_talk==' '){lda(&a,0);bios(&a,0x00d8);emit(&a,0xb7);jz(&a,"spaceIsUp");}
else {lda(&a,p->key_talk==13?7:5);bios(&a,0x0141);
andn(&a,p->key_talk==13?128:p->key_talk=='Z'?128:32);jnz(&a,"spaceIsUp");}emit(&a,0x76);jp(&a,"spaceUpPoll");
mark(&a,"spaceIsUp");bios(&a,0x0156);emit(&a,0xc9);
// SNSMAT follows the PHYSICAL key, so auto-repeat cannot skip pages.
mark(&a,"waitFreshSpace");
mark(&a,"spaceDownPoll");if(p->key_talk==' '){lda(&a,0);bios(&a,0x00d8);emit(&a,0xb7);jnz(&a,"spaceWasPressed");}
else {lda(&a,p->key_talk==13?7:5);bios(&a,0x0141);
andn(&a,p->key_talk==13?128:p->key_talk=='Z'?128:32);jz(&a,"spaceWasPressed");}emit(&a,0x76);jp(&a,"spaceDownPoll");
mark(&a,"spaceWasPressed");call(&a,"waitSpaceUp");emit(&a,0xc9);
// SCREEN 4 runtime NPC positions; MSX1 retains fixed NPC tiles.
if(p->profile!=PROFILE_MSX){
 mark(&a,"initNpcs");
 for(i=0;i<p->scene_count;i++){
   name2(lab,"initSkip",i,-1);get(&a,RM_SCENE);cpn(&a,i);jnz(&a,lab);
   for(j=0;j<p->scenes[i].npc_count;j++){
     const ForgeNPC*q=&p->scenes[i].npcs[j];lda(&a,q->x);put(&a,RM_NPC_X+j);
     lda(&a,q->y);put(&a,RM_NPC_Y+j);lda(&a,0);put(&a,RM_NPC_STEP+j);put(&a,RM_NPC_AGE+j);
   }
   emit(&a,0xc9);mark(&a,lab);
 }
 emit(&a,0xc9);
 // Dynamic occupancy: player and wandering NPCs never share a map cell.
 mark(&a,"npcOccupied");
 for(i=0;i<p->scene_count;i++){
   name2(lab,"occSceneEnd",i,-1);get(&a,RM_SCENE);cpn(&a,i);jnz(&a,lab);
   for(j=0;j<p->scenes[i].npc_count;j++){
      name2(lab2,"occNext",i,j);
      get(&a,RM_CX);emit(&a,0x21);ew(&a,RM_NPC_X+j);emit(&a,0xbe);jnz(&a,lab2);
      get(&a,RM_CY);emit(&a,0x21);ew(&a,RM_NPC_Y+j);emit(&a,0xbe);jnz(&a,lab2);
      lda(&a,1);emit(&a,0xc9);mark(&a,lab2);
   }
   lda(&a,0);emit(&a,0xc9);mark(&a,lab);
 }
 lda(&a,0);emit(&a,0xc9);
 // Bounded updates: one NPC check each 8 VBlank frames.
 // Cycle only through the NPCs present in this scene (not all 8 slots).
 mark(&a,"tickNpcs");
 get(&a,0xfc9e);emit(&a,0x5f); // E = JIFFY low byte
 get(&a,RM_NPC_CLOCK);emit(&a,0xbb);jz(&a,"tickDone"); // CP E
 get(&a,0xfc9e);put(&a,RM_NPC_CLOCK);andn(&a,7);jnz(&a,"tickDone");
 for(i=0;i<p->scene_count;i++){
   int count=p->scenes[i].npc_count;
   name2(lab,"tickSceneSkip",i,-1);get(&a,RM_SCENE);cpn(&a,i);jnz(&a,lab);
   if(count==0)jp(&a,"tickDone");
   get(&a,RM_NPC_CYCLE);emit(&a,0x3c);cpn(&a,count);
   name2(lab2,"tickCycleOK",i,-1);jc(&a,lab2);lda(&a,0);mark(&a,lab2);put(&a,RM_NPC_CYCLE);
   jp(&a,"tickChooseNpc");
   mark(&a,lab);
 }
 jp(&a,"tickDone");
 mark(&a,"tickChooseNpc");
 for(i=0;i<p->scene_count;i++)for(j=0;j<p->scenes[i].npc_count;j++){
   const ForgeNPC*q=&p->scenes[i].npcs[j];
   if(q->gives_key||p->npc_behavior[i][j]==NPC_STILL)continue;
   name2(lab,"tickSkip",i,j);
   get(&a,RM_SCENE);cpn(&a,i);jnz(&a,lab);
   get(&a,RM_NPC_CYCLE);cpn(&a,j);jnz(&a,lab);
   // Per-NPC cooldown, independently of the number of map characters.
   get(&a,RM_NPC_AGE+j);emit(&a,0x3c);
   int threshold=(p->npc_speed[i][j]==NPC_FAST)?1:(p->npc_speed[i][j]==NPC_SLOW?4:2);
   cpn(&a,threshold);name2(lab2,"ageReady",i,j);jnc(&a,lab2);
   put(&a,RM_NPC_AGE+j);emit(&a,0xc9);
   mark(&a,lab2);lda(&a,0);put(&a,RM_NPC_AGE+j);
   // An occasional short rest (roughly one in sixteen active VBlank
   // windows) softens robot-like movement without biasing the PRNG that
   // chooses routes. Clock continues even while an NPC is resting.
   get(&a,0xfc9e);andn(&a,127);cpn(&a,8);jc(&a,"tickDone");
   name2(lab2,"npcWalk",i,j);call(&a,lab2);emit(&a,0xc9);
   mark(&a,lab);
 }
 mark(&a,"tickDone");emit(&a,0xc9);
 for(i=0;i<p->scene_count;i++){
   const ForgeScene*s=&p->scenes[i];
   name2(lab,"npcSync",i,-1);mark(&a,lab);
   for(j=0;j<s->npc_count;j++){
     get(&a,RM_NPC_Y+j);for(x=0;x<4;x++)emit(&a,0x87);emit(&a,0x3d);
     emit(&a,0x21);ew(&a,0x1e00+(j+1)*4);bios(&a,0x004d);
     get(&a,RM_NPC_X+j);for(x=0;x<4;x++)emit(&a,0x87);
     emit(&a,0x21);ew(&a,0x1e01+(j+1)*4);bios(&a,0x004d);
     name2(lab2,"syncPatBase",i,j);name2(val,"syncPatWrite",i,j);
     get(&a,RM_NPC_STEP+j);emit(&a,0xb7);jz(&a,lab2);
     lda(&a,(s->npcs[j].kind+11)*4);jp(&a,val);
     mark(&a,lab2);lda(&a,(s->npcs[j].kind+8)*4);
     mark(&a,val);emit(&a,0x21);ew(&a,0x1e02+(j+1)*4);bios(&a,0x004d);
   }
   emit(&a,0xc9);
 }
 for(i=0;i<p->scene_count;i++)for(j=0;j<p->scenes[i].npc_count;j++){
   const ForgeNPC*q=&p->scenes[i].npcs[j];
   if(q->gives_key||p->npc_behavior[i][j]==NPC_STILL)continue;
   name2(lab,"npcWalk",i,j);mark(&a,lab);
   // Pseudorandom direction from the current JIFFY mixed with NPC index.
   // LCG step; select directions from mixed HIGH seed bits and the
   // 16-frame JIFFY counter. Picking only low bits gave a 2-step loop
   // (up/down then left/right), making NPCs look stuck.
   get(&a,RM_NPC_SEED);emit(&a,0x5f);for(x=0;x<2;x++)emit(&a,0x87);
   emit(&a,0x83);emit(&a,0x3c);put(&a,RM_NPC_SEED);emit(&a,0x5f);
   get(&a,0xfc9e);
   for(x=0;x<4;x++)emit(&a,0x0f); // RRCA x4: one step of time per tick
   emit(&a,0x83); // ADD A,E: blend counter with PRNG seed
   for(x=0;x<2;x++)emit(&a,0x0f); // RRCA x2: mix upper seed bits
   andn(&a,3);
   for(x=0;x<3;x++){cpn(&a,x);name2(lab2,"walkDir",i,j*4+x);jz(&a,lab2);}
   name2(lab2,"walkDir",i,j*4+3);jp(&a,lab2);
   for(x=0;x<4;x++){
     name2(lab2,"walkDir",i,j*4+x);mark(&a,lab2);
     if(x<2){get(&a,RM_NPC_X+j);if(x==0){emit(&a,0xb7);name2(val,"walkDone",i,j);jz(&a,val);emit(&a,0x3d);}else{cpn(&a,FW-1);name2(val,"walkDone",i,j);jz(&a,val);emit(&a,0x3c);}put(&a,RM_CX);get(&a,RM_NPC_Y+j);put(&a,RM_CY);}
     else{get(&a,RM_NPC_Y+j);if(x==2){emit(&a,0xb7);name2(val,"walkDone",i,j);jz(&a,val);emit(&a,0x3d);}else{cpn(&a,FH-1);name2(val,"walkDone",i,j);jz(&a,val);emit(&a,0x3c);}put(&a,RM_CY);get(&a,RM_NPC_X+j);put(&a,RM_CX);}
     name2(val,"walkTry",i,j);jp(&a,val);
   }
   name2(lab2,"walkTry",i,j);mark(&a,lab2);
   call(&a,"walkable");cpn(&a,1);name2(val,"walkDone",i,j);jnz(&a,val);
   if(p->npc_behavior[i][j]==NPC_LOCAL){
      int radius=p->npc_radius[i][j];if(radius<1||radius>4)radius=2;
      int minx=q->x-radius,maxx=q->x+radius,miny=q->y-radius,maxy=q->y+radius;
      // Keep the NPC in a configurable square centered at its starting tile.
      // CP n sets carry if A<n; block on carry below min and on NC above max.
      if(minx>0){get(&a,RM_CX);cpn(&a,minx);jc(&a,val);}
      if(maxx<FW-1){get(&a,RM_CX);cpn(&a,maxx+1);jnc(&a,val);}
      if(miny>0){get(&a,RM_CY);cpn(&a,miny);jc(&a,val);}
      if(maxy<FH-1){get(&a,RM_CY);cpn(&a,maxy+1);jnc(&a,val);}
   }
   // No walking onto the protagonist or another NPC.
   get(&a,RM_CX);emit(&a,0x21);ew(&a,RM_X);emit(&a,0xbe);name2(lab2,"walkNoPlayer",i,j);jnz(&a,lab2);
   get(&a,RM_CY);emit(&a,0x21);ew(&a,RM_Y);emit(&a,0xbe);jz(&a,val);mark(&a,lab2);
   call(&a,"npcOccupied");emit(&a,0xb7);jnz(&a,val);
   get(&a,RM_CX);put(&a,RM_NPC_X+j);get(&a,RM_CY);put(&a,RM_NPC_Y+j);
   get(&a,RM_NPC_Y+j);for(x=0;x<4;x++)emit(&a,0x87);emit(&a,0x3d);
   emit(&a,0x21);ew(&a,0x1e00+(j+1)*4);bios(&a,0x004d);
   get(&a,RM_NPC_X+j);for(x=0;x<4;x++)emit(&a,0x87);
   emit(&a,0x21);ew(&a,0x1e01+(j+1)*4);bios(&a,0x004d);
   // Alternate two simple walk poses by changing the pattern number.
   get(&a,RM_NPC_STEP+j);emit(&a,0xee);emit(&a,1);put(&a,RM_NPC_STEP+j);
   name2(lab2,"walkPatBase",i,j);name2(val,"walkPatWrite",i,j);
   emit(&a,0xb7);jz(&a,lab2);
   lda(&a,(q->kind+11)*4);jp(&a,val);
   mark(&a,lab2);lda(&a,(q->kind+8)*4);
   mark(&a,val);emit(&a,0x21);ew(&a,0x1e02+(j+1)*4);bios(&a,0x004d);
   name2(val,"walkDone",i,j);mark(&a,val);emit(&a,0xc9);
 }

}
mark(&a,"chooseOptions");{
 char optionText[]="1 = SI     2 = NO";
 optionText[0]=(char)p->key_yes;optionText[11]=(char)p->key_no;
 dialog_rom(&a,optionText);
}
mark(&a,"wallText");string_rom(&a,"BLOQUEADO",28);mark(&a,"doorText");string_rom(&a,"PUERTA CERRADA",28);mark(&a,"bagNo");string_rom(&a,"BOLSO VACIO",28);mark(&a,"bagYes");string_rom(&a,"TENES LA LLAVE",28);mark(&a,"bagCoins");string_rom(&a,"MONEDAS: ",28);
for(i=0;i<n;i++){const ForgeNPC*q;npc_at(p,i,&scn,&q);name2(lab,"npcFirst",i,-1);mark(&a,lab);dialog_rom(&a,q->first);name2(lab,"npcAgainText",i,-1);mark(&a,lab);dialog_rom(&a,q->again);name2(lab,"npcQuestion",i,-1);mark(&a,lab);
 {int slot=i;for(j=0;j<scn;j++)slot-=p->scenes[j].npc_count;dialog_rom(&a,p->npc_choice_question[scn][slot][0]?p->npc_choice_question[scn][slot]:"Queres ayudar?");}}
for(i=0;i<p->scene_count;i++){
 const ForgeScene*s=&p->scenes[i];name2(lab,"nameTable",i,-1);mark(&a,lab);
 if(p->profile!=PROFILE_MSX){
    for(y=0;y<FH;y++)for(x=0;x<FW;x++)emit(&a,s->map[y*FW+x]);
 }else{
    uint8_t table[768]={0};
    for(y=0;y<FH;y++)for(x=0;x<FW;x++){
      int t=s->map[y*FW+x],off=y*64+x*2,npc_slot=-1;
      for(j=0;j<s->npc_count;j++)if(s->npcs[j].x==x&&s->npcs[j].y==y)npc_slot=s->npcs[j].kind;
      table[off]=(uint8_t)(npc_slot>=0?196+4*npc_slot:128+8*forge_tile_msx1_fallback(t));
      table[off+1]=table[off]+1;table[off+32]=table[off]+2;table[off+33]=table[off]+3;
    }
    for(j=0;j<768;j++)emit(&a,table[j]);
 }
}
for(i=0;i<p->scene_count;i++){const ForgeScene*s=&p->scenes[i];name2(lab,"passable",i,-1);mark(&a,lab);for(y=0;y<FH;y++)for(x=0;x<FW;x++){int t=s->map[y*FW+x],v=(p->profile==PROFILE_MSX?forge_tile_walkable(s,x,y):(!forge_tile_blocking(t)))?1:0; if(t==T_DOOR){for(j=0;j<s->portal_count;j++)if(s->portals[j].x==x&&s->portals[j].y==y&&s->portals[j].requires_key){v=2;break;}}emit(&a,v);}}
// Hardware sprite data is scene-specific: NPCs retain their real background
// and the player sprite moves independently above the tiles.
if(p->profile!=PROFILE_MSX){
 uint8_t per[14][32],linecol[14][16];
 for(i=0;i<8;i++)forge_screen4_hw_sprite(forge_player_frame_const(p,i/2,i%2),per[i],linecol[i]);
 for(i=0;i<NPC_ART_SLOTS;i++){
   uint8_t pose[256];forge_screen4_hw_sprite(p->npc_sprites[i],per[i+8],linecol[i+8]);
   for(y=0;y<16;y++)for(x=0;x<16;x++)
       pose[y*16+x]=p->npc_sprites[i][y*16+(y>=10?15-x:x)];
   forge_screen4_hw_sprite(pose,per[i+11],linecol[i+11]);
 }
 for(i=0;i<p->scene_count;i++){
   const ForgeScene *s=&p->scenes[i];uint8_t attrs[128],colors[16*(MAX_NPCS+1)];int k;
   memset(attrs,0,sizeof(attrs));memset(colors,0,sizeof(colors));
   for(j=0;j<32;j++)attrs[j*4]=216; // hide unused sprites
   attrs[2]=0;attrs[3]=15;
   for(k=0;k<16;k++)colors[k]=linecol[0][k];
   for(j=0;j<s->npc_count;j++){
      const ForgeNPC *npc=&s->npcs[j];int kind=(npc->kind<NPC_ART_SLOTS?npc->kind:0);
      attrs[(j+1)*4+0]=(uint8_t)(npc->y*16-1);
      attrs[(j+1)*4+1]=(uint8_t)(npc->x*16);
      attrs[(j+1)*4+2]=(uint8_t)((kind+8)*4);
      attrs[(j+1)*4+3]=15;
      for(k=0;k<16;k++)colors[(j+1)*16+k]=linecol[kind+8][k];
   }
   name2(lab,"spriteAttrs",i,-1);mark(&a,lab);
   for(j=0;j<128;j++)emit(&a,attrs[j]);
   name2(lab,"spriteColor",i,-1);mark(&a,lab);
   for(j=0;j<(int)sizeof(colors);j++)emit(&a,colors[j]);
 }
 mark(&a,"hardwareSprites");
 for(i=0;i<14;i++)for(j=0;j<32;j++)emit(&a,per[i][j]);
 mark(&a,"playerLineColors");
 for(i=0;i<8;i++)for(j=0;j<16;j++)emit(&a,linecol[i][j]);
 mark(&a,"fontPatterns");
 for(i=0;i<(int)sizeof(forge_font_8x8);i++)emit(&a,forge_font_8x8[i]);
}
mark(&a,"blankMessage");
for(i=0;i<64;i++)emit(&a,32);
mark(&a,"graphicPatterns");
if(p->profile==PROFILE_MSX){
  // SCREEN 1 legacy: single foreground color per 8 glyphs.
  uint8_t custom[4][32];int slot;
  forge_sprite_1bit(p->sprite,custom[0]);
  for(slot=0;slot<NPC_ART_SLOTS;slot++)forge_sprite_1bit(p->npc_sprites[slot],custom[slot+1]);
  for(i=0;i<(int)sizeof(rom_patterns);i++){
    int rel=i-512,block=rel/32;
    emit(&a,(rel>=0 && rel<128)?custom[block][rel%32]:rom_patterns[i]);
  }
}else{
  uint8_t patterns[MSX2_PATTERN_BYTES],colors[MSX2_PATTERN_BYTES];
  forge_screen4_graphics(p,patterns,colors);
  for(i=0;i<MSX2_PATTERN_BYTES;i++)emit(&a,patterns[i]);
  mark(&a,"graphicColors");
  for(i=0;i<MSX2_PATTERN_BYTES;i++)emit(&a,colors[i]);
}
if(a.failed){forge_copy_string(error,errcap,"ROM de 32 KB excedida o datos de escenario demasiado complejos. Reduce NPC, eventos o mapas.");return 0;}for(i=0;i<a.fc;i++){int addr=-1;for(j=0;j<a.lc;j++)if(!strcmp(a.fix[i].name,a.labels[j].name)){addr=a.labels[j].addr;break;}if(addr<0){forge_copy_string(error,errcap,"Error interno: etiqueta faltante.");return 0;}buf[a.fix[i].offset]=(uint8_t)(addr&255);buf[a.fix[i].offset+1]=(uint8_t)(addr>>8);}
forge_copy_string(error,errcap,p->profile==PROFILE_MSX?"ROM 32KB MSX SCREEN 1":"ROM 32KB MSX2 SCREEN 4 color");return 32768;
}
