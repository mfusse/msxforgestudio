#include "engine.h"
#include "minicrt.h"
#include "art.h"


static const uint32_t forge_editor_palette[16]={
  0x101725,0xf6f2dc,0xe98070,0x302a46,0x7195db,0x6fb473,
  0xffd367,0xa97cd0,0xe9b3be,0xa87153,0x7cdad0,0x627287,
  0xa4414f,0x366a6c,0xd5d5dc,0x18121f};
static int rgbdiff(uint32_t x,uint32_t y){
 int r=(int)((x>>16)&255)-(int)((y>>16)&255);
 int g=(int)((x>>8)&255)-(int)((y>>8)&255);
 int b=(int)(x&255)-(int)(y&255);
 return 2*r*r+4*g*g+2*b*b;
}
void forge_default_tile_art(ForgeProject *p){
 int t,i,k;for(t=0;t<LEGACY_TILE_COUNT;t++)for(i=0;i<256;i++){
  int best=0,cost=0x7fffffff;uint32_t rgb=tile_art[t][i];
  for(k=0;k<16;k++){int d=rgbdiff(rgb,forge_editor_palette[k]);if(d<cost){cost=d;best=k;}}
  p->tile_sprites[t][i]=(uint8_t)best;
 }
}
uint8_t *forge_tile_pixels(ForgeProject *p,int id){
 if(id<0||id>=T_COUNT)id=0;
 return id<LEGACY_TILE_COUNT?p->tile_sprites[id]:id<LEGACY_TILE_COUNT+NEW_TILE_COUNT?p->extra_tile_sprites[id-LEGACY_TILE_COUNT]:id<LEGACY_TILE_COUNT+NEW_TILE_COUNT+INTERIOR_TILE_COUNT?p->interior_tile_sprites[id-LEGACY_TILE_COUNT-NEW_TILE_COUNT]:p->custom_tile_pixels[id-LEGACY_TILE_COUNT-NEW_TILE_COUNT-INTERIOR_TILE_COUNT];
}
const uint8_t *forge_tile_pixels_const(const ForgeProject *p,int id){
 if(id<0||id>=T_COUNT)id=0;
 return id<LEGACY_TILE_COUNT?p->tile_sprites[id]:id<LEGACY_TILE_COUNT+NEW_TILE_COUNT?p->extra_tile_sprites[id-LEGACY_TILE_COUNT]:id<LEGACY_TILE_COUNT+NEW_TILE_COUNT+INTERIOR_TILE_COUNT?p->interior_tile_sprites[id-LEGACY_TILE_COUNT-NEW_TILE_COUNT]:p->custom_tile_pixels[id-LEGACY_TILE_COUNT-NEW_TILE_COUNT-INTERIOR_TILE_COUNT];
}
int forge_tile_blocking(int id){
 return id==T_TREE||id==T_WATER||id==T_WALL||id==T_ROOF||id==T_BRICK||
        id==T_WINDOW||id==T_FENCE||id==T_TABLE||id==T_BED||id==T_SHELF||
        id==T_COUNTER||id==T_CHAIR||id==T_LAMP||id==T_CABINET||id==T_SINK;
}
int forge_tile_msx1_fallback(int id){
 static const uint8_t fallback[NEW_TILE_COUNT]={T_WALL,T_WALL,T_WALL,T_WALL,T_FLOWER,T_PATH,T_DIRT,T_WALL};
 return id<0?T_GRASS:id<LEGACY_TILE_COUNT?id:id<LEGACY_TILE_COUNT+NEW_TILE_COUNT?fallback[id-LEGACY_TILE_COUNT]:id<T_USER_1?(id==T_RUG?T_DIRT:T_WALL):T_GRASS;
}
static void tile_rect(uint8_t *img,int x0,int y0,int x1,int y1,int col){
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)
  if(x>=0&&x<16&&y>=0&&y<16)img[y*16+x]=(uint8_t)col;
}
void forge_default_extra_tile_art(ForgeProject *p){
 int x,y;
 for(int t=LEGACY_TILE_COUNT;t<LEGACY_TILE_COUNT+NEW_TILE_COUNT;t++){
  uint8_t *q=forge_tile_pixels(p,t);
  int base=t==T_ROOF?3:t==T_BRICK?9:t==T_WINDOW?9:t==T_FENCE?5:
           t==T_GARDEN?5:t==T_PAVEMENT?14:t==T_FLOOR?11:7;
  for(y=0;y<16;y++)for(x=0;x<16;x++)q[y*16+x]=(uint8_t)base;
  if(t==T_ROOF){
   for(y=0;y<16;y++)for(x=0;x<16;x++)if(y%6==0||((x+(y/6)*4)%8==0))q[y*16+x]=12;
   tile_rect(q,0,14,15,15,9);
  }else if(t==T_BRICK){
   for(y=0;y<16;y++)for(x=0;x<16;x++)if(y%5==0||((x+((y/5)%2)*5)%10==0))q[y*16+x]=3;
  }else if(t==T_WINDOW){
   tile_rect(q,1,1,14,14,3);tile_rect(q,3,3,12,12,10);
   tile_rect(q,7,2,8,13,1);tile_rect(q,2,7,13,8,1);
   tile_rect(q,0,14,15,15,9);
  }else if(t==T_FENCE){
   tile_rect(q,2,1,5,15,9);tile_rect(q,10,1,13,15,9);
   tile_rect(q,0,5,15,7,3);tile_rect(q,0,11,15,13,3);
   tile_rect(q,3,2,4,3,11);tile_rect(q,11,2,12,3,11);
  }else if(t==T_GARDEN){
   for(y=0;y<16;y++)for(x=0;x<16;x++)if(((x*7+y*5)%17)<3)q[y*16+x]=4;
   tile_rect(q,3,5,6,7,8);tile_rect(q,4,4,5,8,1);
   tile_rect(q,10,10,13,12,6);tile_rect(q,11,9,12,13,1);
  }else if(t==T_PAVEMENT){
   tile_rect(q,0,0,15,0,11);tile_rect(q,0,8,15,8,11);
   for(y=0;y<16;y++)for(x=0;x<16;x++)if((y<8&&x==7)||(y>=8&&(x==3||x==12)))q[y*16+x]=11;
  }else if(t==T_FLOOR){
   for(y=0;y<16;y++)for(x=0;x<16;x++)if(y==0||x==0)q[y*16+x]=9;
   tile_rect(q,5,5,6,6,1);tile_rect(q,12,12,13,13,1);
  }else if(t==T_TABLE){
   tile_rect(q,1,2,14,12,9);tile_rect(q,2,3,13,10,6);
   tile_rect(q,2,13,4,15,3);tile_rect(q,11,13,13,15,3);
   tile_rect(q,4,4,5,5,11);tile_rect(q,10,8,11,9,11);
  }
 }
}
// Eight additional editable 16x16 indoor tiles, appended in project v7.
void forge_default_interior_tile_art(ForgeProject *p){
 int x,y;
 for(int id=T_BED;id<T_USER_1;id++){
   uint8_t *q=forge_tile_pixels(p,id);
   int base=(id==T_RUG)?4:(id==T_LAMP)?11:(id==T_SINK)?14:9;
   for(y=0;y<16;y++)for(x=0;x<16;x++)q[y*16+x]=(uint8_t)base;
   if(id==T_BED){tile_rect(q,1,0,14,15,3);tile_rect(q,2,2,13,14,8);tile_rect(q,3,2,12,5,1);tile_rect(q,2,13,13,14,4);}
   else if(id==T_SHELF){tile_rect(q,1,1,14,14,3);for(y=3;y<14;y+=5){tile_rect(q,2,y,13,y+1,11);for(x=3;x<12;x+=3)tile_rect(q,x,y-2,x+1,y-1,6+(x%3));}}
   else if(id==T_COUNTER){tile_rect(q,0,2,15,12,9);tile_rect(q,1,3,14,7,11);tile_rect(q,2,9,13,12,3);tile_rect(q,3,10,4,11,6);}
   else if(id==T_RUG){tile_rect(q,1,1,14,14,8);tile_rect(q,3,3,12,12,4);tile_rect(q,6,5,9,10,6);tile_rect(q,2,2,13,2,1);tile_rect(q,2,13,13,13,1);}
   else if(id==T_CHAIR){tile_rect(q,3,2,12,5,3);tile_rect(q,4,6,11,11,9);tile_rect(q,4,12,5,15,3);tile_rect(q,10,12,11,15,3);}
   else if(id==T_LAMP){tile_rect(q,6,1,9,3,6);tile_rect(q,4,4,11,6,6);tile_rect(q,7,7,8,13,3);tile_rect(q,5,14,10,15,9);}
   else if(id==T_CABINET){tile_rect(q,1,1,14,15,3);tile_rect(q,2,2,13,14,9);tile_rect(q,7,2,8,14,3);tile_rect(q,5,8,6,9,6);tile_rect(q,10,8,11,9,6);}
   else if(id==T_SINK){tile_rect(q,1,2,14,14,9);tile_rect(q,2,3,13,12,7);tile_rect(q,4,5,11,10,14);tile_rect(q,7,1,8,5,3);tile_rect(q,7,9,8,10,4);}
 }
}
static void init_legacy_controls(ForgeProject *p){
 p->key_talk=' ';p->key_inventory='I';p->key_yes='1';p->key_no='2';
 memset(p->npc_reward,0,sizeof(p->npc_reward));
}
void forge_upgrade_v9(ForgeProject *p){init_legacy_controls(p);
  for(int i=0;i<CUSTOM_TILE_COUNT;i++){
    char name[]="Propio 1";name[7]=(char)('1'+i);
    forge_copy_string(p->custom_tile_names[i],24,name);
    for(int y=0;y<256;y++)p->custom_tile_pixels[i][y]=(uint8_t)((y/16+y%16)&1?5:4);
  }
  memset(p->npc_flag_index,0,sizeof(p->npc_flag_index));
  memset(p->npc_choice_set_flag,0,sizeof(p->npc_choice_set_flag));
  p->custom_tile_count=0;
  p->version=FORGE_VERSION;
}
void forge_upgrade_v8(ForgeProject *p){init_legacy_controls(p);
  memset(p->npc_choice_question,0,sizeof(p->npc_choice_question));
  for(int i=0;i<p->scene_count&&i<MAX_SCENES;i++)for(int j=0;j<p->scenes[i].npc_count&&j<MAX_NPCS;j++)
    if(p->npc_radius[i][j]==0)p->npc_radius[i][j]=2;
  p->version=FORGE_VERSION;
}
void forge_upgrade_v7(ForgeProject *p){init_legacy_controls(p);
  memset(p->npc_event_mode,0,sizeof(p->npc_event_mode));p->version=FORGE_VERSION;
}
void forge_upgrade_v6(ForgeProject *p){init_legacy_controls(p);forge_default_interior_tile_art(p);p->version=FORGE_VERSION;}
void forge_default_npc_behavior(ForgeProject *p){
 for(int sc=0;sc<MAX_SCENES;sc++)for(int j=0;j<MAX_NPCS;j++){
   p->npc_behavior[sc][j]=NPC_FREE;
   p->npc_speed[sc][j]=NPC_NORMAL;
   p->npc_radius[sc][j]=2;
 }
}
void forge_upgrade_v5(ForgeProject *p){init_legacy_controls(p);
 forge_default_npc_behavior(p);
 p->version=FORGE_VERSION;forge_default_interior_tile_art(p);
}
void forge_upgrade_v4(ForgeProject *p){init_legacy_controls(p);
 forge_default_extra_tile_art(p);
 forge_default_npc_behavior(p);forge_default_interior_tile_art(p);
 p->version=FORGE_VERSION;
}
void forge_upgrade_v2(ForgeProject *p){init_legacy_controls(p);
 forge_default_tile_art(p);forge_upgrade_v3(p);forge_default_extra_tile_art(p);forge_default_npc_behavior(p);forge_default_interior_tile_art(p);
}

void forge_default_npc_art(ForgeProject *p){
    static const char *patterns[NPC_ART_SLOTS][16]={
        {"................",".....111111.....","....11111111....","....15555551....","....15555551....","....15511551....",".....555555.....","......5555......",".....222222.....","....22222222....","....22222222....",".....222222.....",".....22..22.....",".....22..22.....","....222..222....","................"},
        {"................","......3333......",".....333333.....","....33333333....","....36666663....","....36166163....",".....666666.....","......6666......",".....444444.....","....44444444....","....44444444....",".....444444.....","......4..4......",".....44..44.....",".....44..44.....","................"},
        {"................","......8888......",".....888888.....","....88888888....","....87777778....","....87711778....",".....777777.....","......7777......",".....999999.....","....99999999....","....99999999....",".....999999.....",".....99..99.....",".....99..99.....","....999..999....","................"}
    };
    static const char *names[NPC_ART_SLOTS]={"Entrenador", "Exploradora", "Cientifica"};
    int i,x,y;
    for(i=0;i<NPC_ART_SLOTS;i++){
      forge_copy_string(p->npc_sprite_names[i],24,names[i]);
      for(y=0;y<16;y++)for(x=0;x<16;x++){
        int ch=patterns[i][y][x];
        p->npc_sprites[i][y*16+x]=(uint8_t)(ch=='.'?255:ch-'0');
      }
    }
}
void forge_upgrade_v1(ForgeProject *p){init_legacy_controls(p);
    // The first FORGE_V1_FILE_BYTES bytes preserve the 0.2 layout exactly.
    forge_default_npc_art(p);
    forge_default_tile_art(p);
    forge_upgrade_v3(p);
    forge_default_extra_tile_art(p);
    forge_default_npc_behavior(p);forge_default_interior_tile_art(p);
    for(int i=0;i<p->scene_count&&i<MAX_SCENES;i++)
      for(int j=0;j<p->scenes[i].npc_count&&j<MAX_NPCS;j++)p->scenes[i].npcs[j].kind=0;
}
// Explicit 4-direction, two-step protagonist animation.  The original sprite[]
// is always down/step 0; older .msxf projects retain their original artwork.
uint8_t *forge_player_frame(ForgeProject *p,int direction,int frame){
 int index=direction*2+frame;
 if(direction<0||direction>3||frame<0||frame>1)return p->sprite;
 return index==0?p->sprite:p->player_extra_frames[index-1];
}
const uint8_t *forge_player_frame_const(const ForgeProject *p,int direction,int frame){
 int index=direction*2+frame;
 if(direction<0||direction>3||frame<0||frame>1)return p->sprite;
 return index==0?p->sprite:p->player_extra_frames[index-1];
}
// Generate editable starting poses from existing artwork. The user can repaint
// every one of these seven extra animation frames independently in Windows.
void forge_upgrade_v3(ForgeProject *p){init_legacy_controls(p);
 int direction,frame,x,y;
 for(direction=0;direction<4;direction++)for(frame=0;frame<2;frame++){
  uint8_t *out=forge_player_frame(p,direction,frame);
  if(direction==0 && frame==0)continue;
  for(y=0;y<16;y++)for(x=0;x<16;x++){
   int sx=x,sy=y;
   if(direction==3)sx=15-x;  // different right-facing profile
   if(direction==1 && y>=3&&y<=7){ // back of head instead of face
     uint8_t base=p->sprite[y*16+sx];
     out[y*16+x]=(base==5)?3:base;
     continue;
   }
   if((direction==2 || direction==3) && y>=4 && y<=7){
     // A side-facing head with one visible eye; do not stretch face pixels.
     uint8_t face=p->sprite[y*16+sx];
     if(face==0)face=5;
     if(face==5 && ((direction==2&&x==5)||(direction==3&&x==10)))face=3;
     out[y*16+x]=face;
     continue;
   }
   if(frame && y>=12){
     if(x<8)sy=(y==15)?15:y+1;
     else sy=(y==12)?12:y-1;
   }
   out[y*16+x]=p->sprite[sy*16+sx];
  }
 }
 forge_default_npc_behavior(p);forge_default_interior_tile_art(p);
 p->version=FORGE_VERSION;
}
void forge_copy_string(char *d,int cap,const char *s) {int i=0; if(!d||cap<1)return; while(s&&s[i]&&i<cap-1){d[i]=s[i];i++;} d[i]=0;}
static void scene_init(ForgeScene *s,const char *name){int x,y;memset(s,0,sizeof(*s));forge_copy_string(s->name,32,name);for(y=0;y<FH;y++)for(x=0;x<FW;x++)s->map[y*FW+x]=(!x||!y||x==FW-1||y==FH-1)?T_TREE:T_GRASS;s->spawn_x=2;s->spawn_y=2;}
void forge_blank(ForgeProject *p){memset(p,0,sizeof(*p));p->magic=KEY_MAGIC;p->version=FORGE_VERSION;p->profile=PROFILE_MSX2;p->key_talk=' ';p->key_inventory='I';p->key_yes='1';p->key_no='2';p->scene_count=1;forge_copy_string(p->name,48,"Mi aventura MSX");scene_init(&p->scenes[0],"Pueblo");memset(p->sprite,255,256);forge_default_npc_art(p);forge_default_tile_art(p);forge_default_extra_tile_art(p);forge_default_npc_behavior(p);forge_default_interior_tile_art(p);
// Default 16x16 avatar is visible even in a blank project.
{static const char *avatar[16]={
"................","......3333......",".....333333.....","....35555553....","....35555553....","....33033033....",".....335533.....","......3333......",".....227722.....","....22277222....","....22277222....",".....227722.....","......2..2......","......2..2......",".....22..22.....",".....22..22....."};
int xx,yy;for(yy=0;yy<16;yy++)for(xx=0;xx<16;xx++)p->sprite[yy*16+xx]=avatar[yy][xx]=='.'?255:(uint8_t)(avatar[yy][xx]-'0');}
forge_upgrade_v3(p);forge_upgrade_v9(p);
}
static void rect(ForgeScene *s,int x1,int y1,int x2,int y2,int t){int x,y;for(y=y1;y<=y2;y++)for(x=x1;x<=x2;x++)if(x>=0&&x<FW&&y>=0&&y<FH)s->map[y*FW+x]=(uint8_t)t;}
static ForgeNPC* npc(ForgeScene *s,int x,int y,const char*name,const char*first,const char*again,int gives){ForgeNPC*n=&s->npcs[s->npc_count++];n->x=x;n->y=y;n->kind=(uint8_t)((s->npc_count-1)%NPC_ART_SLOTS);n->gives_key=gives;forge_copy_string(n->name,28,name);forge_copy_string(n->first,78,first);forge_copy_string(n->again,78,again);return n;}
static void portal(ForgeScene *s,int x,int y,int to,int tx,int ty,int key){ForgePortal*q=&s->portals[s->portal_count++];q->x=x;q->y=y;q->to=to;q->tx=tx;q->ty=ty;q->requires_key=key; s->map[y*FW+x]=T_DOOR;}
// Generated room with a dynamic exit: it leads next to its matching exterior door.
// The exterior entrance is placed using the existing Conectar tool.
int forge_add_interior(ForgeProject *p,int style,int origin_scene){
 if(!p||p->scene_count>=MAX_SCENES||origin_scene<0||origin_scene>=p->scene_count)return -1;
 if(style<0||style>2)return -1;
 int idx=p->scene_count++;
 ForgeScene *s=&p->scenes[idx];
 scene_init(s,style==0?"Casa nueva":style==1?"Tienda nueva":"Laboratorio nuevo");
 for(int y=1;y<FH-1;y++)for(int x=1;x<FW-1;x++)s->map[y*FW+x]=T_FLOOR;
 rect(s,0,0,15,0,T_BRICK);rect(s,0,11,15,11,T_BRICK);
 rect(s,0,0,0,11,T_BRICK);rect(s,15,0,15,11,T_BRICK);
 s->map[0*FW+4]=T_WINDOW;s->map[0*FW+11]=T_WINDOW;
 rect(s,4,4,11,7,T_RUG);
 if(style==0){
   rect(s,2,2,4,3,T_BED);s->map[2*FW+12]=T_CABINET;
   s->map[7*FW+3]=T_CHAIR;s->map[7*FW+12]=T_LAMP;
 }else if(style==1){
   rect(s,3,2,5,3,T_SHELF);rect(s,10,2,12,3,T_SHELF);
   rect(s,5,6,10,6,T_COUNTER);s->map[8*FW+12]=T_CABINET;
   npc(s,8,5,"Comerciante","Bienvenido a mi tienda!","Volve cuando quieras.",0);
   p->npc_behavior[idx][0]=NPC_STILL;
 }else{
   rect(s,2,2,4,3,T_CABINET);rect(s,11,2,13,3,T_SINK);
   rect(s,5,6,10,6,T_TABLE);s->map[8*FW+12]=T_LAMP;
   npc(s,8,5,"Investigador","Estamos haciendo pruebas.","Pronto tendremos novedades.",0);
   p->npc_behavior[idx][0]=NPC_STILL;
 }
 s->spawn_x=7;s->spawn_y=9;
 portal(s,7,10,origin_scene,255,255,0); // AUTO RETURN to exterior entrance
 return idx;
}
void forge_demo(ForgeProject*p){int x,y;forge_blank(p);p->profile=PROFILE_MSX2;p->scene_count=3;forge_copy_string(p->name,48,"Aventura del Bosque");
ForgeScene *a=&p->scenes[0],*b=&p->scenes[1],*c=&p->scenes[2];scene_init(a,"Pueblo Brisa");scene_init(b,"Ruta del Lago");scene_init(c,"Laboratorio");
rect(a,1,8,14,8,T_PATH);rect(a,3,1,3,8,T_PATH);rect(a,6,1,6,7,T_PATH);rect(a,9,1,13,3,T_WALL);rect(a,10,4,12,4,T_WALL);rect(a,1,2,2,5,T_WATER);rect(a,7,4,8,5,T_FLOWER);a->spawn_x=2;a->spawn_y=8;
npc(a,5,7,"Profesor","Toma esta llave. Abre el porton del bosque.","Ya tenes la llave. Adelante!",1);
p->npc_behavior[0][0]=NPC_STILL;
// A roaming NPC is visible from the starting village, making it easy to verify
// autonomous movement without first completing the key/portal sequence.
npc(a,6,3,"Vecina","Buen dia! Me encanta pasear por el pueblo.","Nos vemos por las calles.",0);
p->npc_behavior[0][1]=NPC_LOCAL;p->npc_speed[0][1]=NPC_NORMAL;p->npc_radius[0][1]=2;
// v0.5.0: live visual example of a key-based conditional conversation.
p->npc_event_mode[0][1]=EVENT_KEY_BRANCH;
forge_copy_string(a->npcs[1].first,sizeof(a->npcs[1].first),"Todavia no tenes la llave. Habla con el profesor.");
forge_copy_string(a->npcs[1].again,sizeof(a->npcs[1].again),"Ya tenes la llave! El bosque te espera.");
portal(a,14,8,1,2,8,1);
rect(b,1,8,14,8,T_PATH);rect(b,2,1,2,8,T_PATH);rect(b,7,1,7,8,T_PATH);rect(b,3,3,5,5,T_FLOWER);rect(b,10,2,13,5,T_WATER);rect(b,11,9,13,10,T_TREE);b->spawn_x=2;b->spawn_y=8;portal(b,1,8,0,13,8,0);portal(b,14,8,2,2,8,0);
npc(b,7,7,"Exploradora","Que lindos bosques! Segui el sendero.","Cuidado con el agua.",0);b->npcs[0].kind=1;
rect(c,1,1,14,10,T_DIRT);rect(c,1,1,14,1,T_WALL);rect(c,1,10,14,10,T_WALL);rect(c,1,1,1,10,T_WALL);rect(c,14,1,14,10,T_WALL);rect(c,3,3,6,4,T_WALL);rect(c,9,3,12,4,T_WALL);rect(c,2,8,13,8,T_PATH);c->spawn_x=2;c->spawn_y=8;portal(c,2,9,1,13,8,0);npc(c,8,6,"Cientifica","La mision todavia esta pendiente.","Mision completada! Recibis monedas.",0);c->npcs[0].kind=2;
p->npc_event_mode[2][0]=EVENT_FLAG_BRANCH;p->npc_flag_index[2][0]=0;p->npc_reward[2][0]=2;
// v0.4.4 demonstration: original village architecture, gardens and an
// interior laboratory.  All decor is made from exportable 16x16 tiles.
rect(a,9,1,13,1,T_ROOF);rect(a,9,2,13,3,T_BRICK);
a->map[2*FW+10]=T_WINDOW;a->map[2*FW+12]=T_WINDOW;
a->map[3*FW+11]=T_WINDOW;
rect(a,10,4,12,4,T_FENCE);
rect(a,4,9,7,10,T_GARDEN);rect(a,3,8,10,8,T_PAVEMENT);
a->map[8*FW+14]=T_DOOR; // preserve exit / its portal
rect(b,7,1,7,7,T_PAVEMENT);
rect(b,5,9,9,9,T_FENCE);rect(b,4,2,5,2,T_GARDEN);
rect(c,2,2,13,9,T_FLOOR);
rect(c,1,1,14,1,T_BRICK);rect(c,1,10,14,10,T_BRICK);
rect(c,1,1,1,10,T_BRICK);rect(c,14,1,14,10,T_BRICK);
rect(c,3,3,6,4,T_TABLE);rect(c,9,3,12,4,T_TABLE);
c->map[1*FW+6]=T_WINDOW;c->map[1*FW+10]=T_WINDOW;
rect(c,2,8,13,8,T_PAVEMENT);
// Restore portal after the new floor and furniture decorations.
c->map[9*FW+2]=T_DOOR;
// 0.4.6: fully connected new interior rooms: house and shop.
// Return doors lead to the walkable square beside their matching entrance.
int house=forge_add_interior(p,0,0);
int shop=forge_add_interior(p,1,0);
if(house>=0){portal(a,11,4,house,7,9,0);}
if(shop>=0){portal(a,13,4,shop,7,9,0);}
// Keep entrance paths walkable and visibly connected to the village.
rect(a,11,5,11,8,T_PAVEMENT);rect(a,13,5,13,8,T_PAVEMENT);
if(house>=0)a->map[4*FW+11]=T_DOOR;
if(shop>=0)a->map[4*FW+13]=T_DOOR;
// simple original player sprite indexed palette (255 transparent)
static const char *pix[16]={
"................","......3333......",".....333333.....","....35555553....","....35555553....","....33033033....",".....335533.....","......3333......",".....227722.....","....22277222....","....22277222....",".....227722.....","......2..2......","......2..2......",".....22..22.....",".....22..22....."};
for(y=0;y<16;y++)for(x=0;x<16;x++)p->sprite[y*16+x]=pix[y][x]=='.'?255:(uint8_t)(pix[y][x]-'0');
forge_upgrade_v3(p);
// Demo behaviors; the professor must not wander from the quest dialogue.
p->npc_behavior[0][0]=NPC_STILL;
p->npc_behavior[0][1]=NPC_LOCAL;p->npc_speed[0][1]=NPC_NORMAL;p->npc_radius[0][1]=2;
// v0.5.0: live visual example of a key-based conditional conversation.
p->npc_event_mode[0][1]=EVENT_KEY_BRANCH;
forge_copy_string(a->npcs[1].first,sizeof(a->npcs[1].first),"Todavia no tenes la llave. Habla con el profesor.");
forge_copy_string(a->npcs[1].again,sizeof(a->npcs[1].again),"Ya tenes la llave! El bosque te espera.");
if(shop>=0){
 p->npc_behavior[shop][0]=NPC_STILL;
 p->npc_event_mode[shop][0]=EVENT_CHOICE;
 forge_copy_string(p->npc_choice_question[shop][0],64,"Queres conocer la tienda?");
 forge_copy_string(p->scenes[shop].npcs[0].first,78,"Si! Tenemos muchos objetos para tu viaje.");
 forge_copy_string(p->scenes[shop].npcs[0].again,78,"Perfecto. Podes volver cuando quieras.");
 // 0.5.3 demonstration: answer SI to the merchant, then speak to the
 // second shop NPC to see the shared mission-state consequence.
 p->npc_flag_index[shop][0]=0;
 p->npc_choice_set_flag[shop][0]=1;
 if(p->scenes[shop].npc_count<MAX_NPCS){
    ForgeNPC *guide=npc(&p->scenes[shop],13,7,"Ayudante",
        "La mision sigue pendiente.","Gracias por ayudar en la tienda!",0);
    guide->kind=1;
    p->npc_behavior[shop][1]=NPC_STILL;
    p->npc_event_mode[shop][1]=EVENT_FLAG_BRANCH;
    p->npc_flag_index[shop][1]=0;
 }
}
for(x=0;x<16;x++){p->music[0][x]=(x%4==0)?5:0;p->music[1][x]=(x%8==0)?3:0;p->music[2][x]=(x%2==0)?1:0;}
}
int forge_tile_walkable(const ForgeScene*sc,int x,int y){int i;if(x<0||x>=FW||y<0||y>=FH)return 0;uint8_t t=sc->map[y*FW+x];if(forge_tile_blocking(t))return 0;for(i=0;i<sc->npc_count;i++)if(sc->npcs[i].x==x&&sc->npcs[i].y==y)return 0;return 1;}
/* These routines copy the NPC and every per-NPC array, including dialogue
   choices and rewards. Keeping them together prevents character data from
   accidentally being assigned to a different NPC after a delete. */
int forge_duplicate_npc(ForgeProject *p,int scene,int npc,int x,int y){
 if(!p||scene<0||scene>=p->scene_count)return -1;
 ForgeScene *sc=&p->scenes[scene];
 if(npc<0||npc>=sc->npc_count||sc->npc_count>=MAX_NPCS)return -1;
 if(x<0||x>=FW||y<0||y>=FH||sc->map[y*FW+x]==T_DOOR||
    (x==sc->spawn_x&&y==sc->spawn_y)||!forge_tile_walkable(sc,x,y))return -1;
 int n=sc->npc_count;
 sc->npcs[n]=sc->npcs[npc];sc->npcs[n].x=(uint8_t)x;sc->npcs[n].y=(uint8_t)y;
 p->npc_behavior[scene][n]=p->npc_behavior[scene][npc];
 p->npc_speed[scene][n]=p->npc_speed[scene][npc];
 p->npc_radius[scene][n]=p->npc_radius[scene][npc];
 p->npc_event_mode[scene][n]=p->npc_event_mode[scene][npc];
 memcpy(p->npc_choice_question[scene][n],p->npc_choice_question[scene][npc],
        sizeof(p->npc_choice_question[scene][n]));
 p->npc_flag_index[scene][n]=p->npc_flag_index[scene][npc];
 p->npc_choice_set_flag[scene][n]=p->npc_choice_set_flag[scene][npc];
 p->npc_reward[scene][n]=p->npc_reward[scene][npc];
 sc->npc_count=(uint8_t)(n+1);
 return n;
}
int forge_delete_npc(ForgeProject *p,int scene,int npc){
 if(!p||scene<0||scene>=p->scene_count)return 0;
 ForgeScene *sc=&p->scenes[scene];
 if(npc<0||npc>=sc->npc_count)return 0;
 for(int i=npc;i+1<sc->npc_count;i++){
    sc->npcs[i]=sc->npcs[i+1];
    p->npc_behavior[scene][i]=p->npc_behavior[scene][i+1];
    p->npc_speed[scene][i]=p->npc_speed[scene][i+1];
    p->npc_radius[scene][i]=p->npc_radius[scene][i+1];
    p->npc_event_mode[scene][i]=p->npc_event_mode[scene][i+1];
    memcpy(p->npc_choice_question[scene][i],p->npc_choice_question[scene][i+1],
           sizeof(p->npc_choice_question[scene][i]));
    p->npc_flag_index[scene][i]=p->npc_flag_index[scene][i+1];
    p->npc_choice_set_flag[scene][i]=p->npc_choice_set_flag[scene][i+1];
    p->npc_reward[scene][i]=p->npc_reward[scene][i+1];
 }
 int last=--sc->npc_count;
 memset(&sc->npcs[last],0,sizeof(sc->npcs[last]));
 p->npc_behavior[scene][last]=NPC_FREE;
 p->npc_speed[scene][last]=NPC_NORMAL;
 p->npc_radius[scene][last]=2;
 p->npc_event_mode[scene][last]=EVENT_CLASSIC;
 memset(p->npc_choice_question[scene][last],0,sizeof(p->npc_choice_question[scene][last]));
 p->npc_flag_index[scene][last]=0;
 p->npc_choice_set_flag[scene][last]=0;
 p->npc_reward[scene][last]=0;
 return 1;
}
int forge_land(const ForgeScene *sc,int x,int y,int *ox,int *oy){int radius,dy,dx;if(forge_tile_walkable(sc,x,y)){*ox=x;*oy=y;return 1;}for(radius=1;radius<FW+FH;radius++)for(dy=-radius;dy<=radius;dy++)for(dx=-radius;dx<=radius;dx++)if((dx<0?-dx:dx)+(dy<0?-dy:dy)==radius&&forge_tile_walkable(sc,x+dx,y+dy)){*ox=x+dx;*oy=y+dy;return 1;}return 0;}
// Do not teleport to an exterior scene's initial spawn when leaving a building.
// Newly-created interiors mark their exit destination as 255,255 (auto-return).
// Saved 0.4.6/0.4.6.1 projects use the exterior spawn instead, so recognize
// that legacy form without changing the .msxf project layout or version.
static int legacy_interior_return(const ForgeProject *p,int source,const ForgePortal *q){
 const ForgeScene *room=&p->scenes[source],*outside=&p->scenes[q->to];
 return q->tx==outside->spawn_x&&q->ty==outside->spawn_y&&
        q->x==room->spawn_x&&q->y==room->spawn_y+1&&
        q->x<FW&&q->y<FH&&room->spawn_x<FW&&room->spawn_y<FH&&
        room->map[room->spawn_y*FW+room->spawn_x]==T_FLOOR&&
        room->map[FW*(FH-1)+q->x]==T_BRICK;
}
int forge_portal_arrival(const ForgeProject *p,int source_scene,const ForgePortal *q,int *out_x,int *out_y){
 if(!p||!q||!out_x||!out_y||source_scene<0||source_scene>=p->scene_count||q->to>=p->scene_count)return 0;
 const ForgeScene *dest=&p->scenes[q->to];
 if((q->tx==255&&q->ty==255)||legacy_interior_return(p,source_scene,q)){
   // Find the entrance from the outside back into the room we are leaving.
   // Prefer the cell below its door: building entrances usually face down.
   const int dx[4]={0,-1,1,0},dy[4]={1,0,0,-1};
   for(int k=0;k<dest->portal_count;k++){
     const ForgePortal *entrance=&dest->portals[k];
     if(entrance->to!=source_scene)continue;
     for(int d=0;d<4;d++){
       int x=(int)entrance->x+dx[d],y=(int)entrance->y+dy[d];
       if(x>=0&&y>=0&&x<FW&&y<FH&&dest->map[y*FW+x]!=T_DOOR&&forge_tile_walkable(dest,x,y)){
         *out_x=x;*out_y=y;return 1;
       }
     }
   }
   // A room might not have an exterior entrance yet. Remain backward-compatible.
   return forge_land(dest,dest->spawn_x,dest->spawn_y,out_x,out_y);
 }
 return forge_land(dest,q->tx,q->ty,out_x,out_y);
}
static void forge_npcs_reset(const ForgeProject*p,ForgeState*s){
 const ForgeScene*sc=&p->scenes[s->scene];
 for(int j=0;j<MAX_NPCS;j++){
    s->npc_x[j]=j<sc->npc_count?sc->npcs[j].x:255;
    s->npc_y[j]=j<sc->npc_count?sc->npcs[j].y:255;
 }
 s->npc_tick=0;s->bubble_npc=255;
}
int forge_tick(const ForgeProject*p,ForgeState*s){
 if(!p||!s||p->profile==PROFILE_MSX||s->bubble_npc!=255)return 0;
 const ForgeScene*sc=&p->scenes[s->scene];
 if(!sc->npc_count)return 0;
 s->npc_tick++;
 int j=s->npc_tick%sc->npc_count;
 if(sc->npcs[j].gives_key||p->npc_behavior[s->scene][j]==NPC_STILL)return 0;
 int divider=(p->npc_speed[s->scene][j]==NPC_FAST)?1:(p->npc_speed[s->scene][j]==NPC_SLOW?4:2);
 if(((s->npc_tick/sc->npc_count)%divider)!=0)return 0;
 unsigned int seed=(unsigned int)s->npc_tick*73u + s->x*11u+s->y*17u+j*19u;
 // Small intermittent rests make NPC walking feel less mechanical without
 // modifying their slow/normal/fast settings or project format.
 if(((seed>>3)%11u)==0)return 0;
 int dir=(seed ^ (seed>>2))&3,dx=(dir==0?1:dir==1?-1:0),dy=(dir==2?1:dir==3?-1:0);
 int nx=s->npc_x[j]+dx,ny=s->npc_y[j]+dy;
 if(nx<0||ny<0||nx>=FW||ny>=FH)return 0;
 if(p->npc_behavior[s->scene][j]==NPC_LOCAL){
    int radius=p->npc_radius[s->scene][j];
    if(radius<1||radius>4)radius=2;
    int dx0=nx-(int)sc->npcs[j].x,dy0=ny-(int)sc->npcs[j].y;
    if(dx0 < -radius||dx0>radius||dy0< -radius||dy0>radius)return 0;
 }
 int terrain=sc->map[ny*FW+nx];
 if(forge_tile_blocking(terrain)||terrain==T_DOOR)return 0;
 if(nx==s->x&&ny==s->y)return 0;
 for(int k=0;k<sc->npc_count;k++)if(k!=j&&nx==s->npc_x[k]&&ny==s->npc_y[k])return 0;
 s->npc_x[j]=(uint8_t)nx;s->npc_y[j]=(uint8_t)ny;return 1;
}
void forge_start(const ForgeProject*p,ForgeState*s){int x,y;memset(s,0,sizeof(*s));s->scene=0;forge_land(&p->scenes[0],p->scenes[0].spawn_x,p->scenes[0].spawn_y,&x,&y);s->x=x;s->y=y;forge_npcs_reset(p,s);forge_copy_string(s->message,MAX_MESSAGE,"Flechas/WASD: mover | Hablar | Inventario");}
static void msg(ForgeState*s,const char*m){forge_copy_string(s->message,MAX_MESSAGE,m);}
int forge_move(const ForgeProject*p,ForgeState*s,int cmd){int x=s->x,y=s->y,i;const ForgeScene*sc=&p->scenes[s->scene];if(cmd==INVENTORY){s->bubble_npc=255;
  if(s->coins)msg(s,s->has_key?"Inventario: llave y monedas":"Inventario: monedas");
  else msg(s,s->has_key?"Inventario: llave del bosque":"Inventario vacio");return 1;}
if(cmd==CHOOSE_YES||cmd==CHOOSE_NO){
  if(!s->choice_pending)return 0;
  const ForgeNPC *n=&p->scenes[s->choice_scene].npcs[s->choice_npc];
  int yes=cmd==CHOOSE_YES;
  if(yes&&p->npc_choice_set_flag[s->choice_scene][s->choice_npc]){
    int flag=p->npc_flag_index[s->choice_scene][s->choice_npc];
    if(flag<FORGE_FLAG_COUNT)s->flags[flag]=1;
  }
  msg(s,yes?n->first:n->again);
  if(yes&&n->gives_key)s->has_key=1;
  s->spoken[s->choice_scene][s->choice_npc]=1;
  s->choice_pending=0;
  return 1;
}
if(cmd==TALK){for(i=0;i<sc->npc_count;i++){const ForgeNPC*n=&sc->npcs[i];int dx=s->npc_x[i]-x,dy=s->npc_y[i]-y;if((dx<0?-dx:dx)+(dy<0?-dy:dy)==1){
 if(p->npc_event_mode[s->scene][i]==EVENT_CHOICE){
   s->choice_pending=1;s->choice_scene=s->scene;s->choice_npc=(uint8_t)i;
   msg(s,p->npc_choice_question[s->scene][i][0]?p->npc_choice_question[s->scene][i]:"Queres ayudar?");
   return 1;
 }
 if(p->npc_event_mode[s->scene][i]==EVENT_FLAG_BRANCH){
  int flag=p->npc_flag_index[s->scene][i];
  if(flag<FORGE_FLAG_COUNT&&s->flags[flag]){
    int rw=p->npc_reward[s->scene][i];
    if(rw>0&&!s->reward_claimed[flag]){
      int amount=rw==1?5:rw==2?10:20;
      int total=s->coins+amount;s->coins=(uint8_t)(total>255?255:total);
      s->reward_claimed[flag]=1;
    }
    msg(s,n->again[0]?n->again:n->first);
  }else msg(s,n->first);
 }else if(p->npc_event_mode[s->scene][i]==EVENT_KEY_BRANCH){
  if(s->has_key)msg(s,n->again[0]?n->again:n->first);
  else {msg(s,n->first);if(n->gives_key)s->has_key=1;}
 }else if(n->gives_key&&!s->has_key){s->has_key=1;msg(s,n->first);}
 else if(s->spoken[s->scene][i]&&n->again[0])msg(s,n->again);
 else msg(s,n->first);s->spoken[s->scene][i]=1;s->bubble_npc=(uint8_t)i;return 1;}}s->bubble_npc=255;msg(s,"No hay nadie al lado.");return 0;}
s->bubble_npc=255;s->choice_pending=0;
if(cmd==MOVE_UP){y--;s->facing=1;}else if(cmd==MOVE_DOWN){y++;s->facing=0;}else if(cmd==MOVE_LEFT){x--;s->facing=2;}else if(cmd==MOVE_RIGHT){x++;s->facing=3;}else return 0;
if(x<0||x>=FW||y<0||y>=FH||forge_tile_blocking(sc->map[y*FW+x])){msg(s,"Camino bloqueado");return 0;}
for(i=0;i<sc->npc_count;i++)if(s->npc_x[i]==x&&s->npc_y[i]==y){msg(s,"Camino bloqueado");return 0;}
if(sc->map[y*FW+x]==T_DOOR){int required=0;for(i=0;i<sc->portal_count;i++)if(sc->portals[i].x==x&&sc->portals[i].y==y&&sc->portals[i].requires_key)required=1;if(required&&!s->has_key){msg(s,"La puerta necesita una llave");return 0;}}
s->x=x;s->y=y;s->walk_frame^=1;
for(i=0;i<sc->portal_count;i++){const ForgePortal*q=&sc->portals[i];if(q->x==x&&q->y==y&&q->to<p->scene_count){const ForgeScene*t=&p->scenes[q->to];int tx,ty;if(forge_portal_arrival(p,s->scene,q,&tx,&ty)){s->scene=q->to;s->x=tx;s->y=ty;forge_npcs_reset(p,s);msg(s,t->name);return 1;}}}
msg(s,sc->name);return 1;
}
int forge_reachable(const ForgeScene*sc,int sx,int sy,int tx,int ty){uint8_t seen[FW*FH]={0};int qx[FW*FH],qy[FW*FH],h=0,t=0,k;if(!forge_tile_walkable(sc,sx,sy))return 0;if(tx==sx&&ty==sy)return 1;seen[sy*FW+sx]=1;qx[t]=sx;qy[t++]=sy;while(h<t){int x=qx[h],y=qy[h++];for(k=0;k<4;k++){int nx=x+(k==0)-(k==1),ny=y+(k==2)-(k==3);if(nx<0||nx>=FW||ny<0||ny>=FH||seen[ny*FW+nx]||!forge_tile_walkable(sc,nx,ny))continue;if(nx==tx&&ny==ty)return 1;seen[ny*FW+nx]=1;qx[t]=nx;qy[t++]=ny;}}return 0;}
int forge_controls_valid(const ForgeProject *p){
 if(!p)return 0;
 if(!(p->key_talk==' '||p->key_talk=='Z'||p->key_talk=='X'||p->key_talk==13))return 0;
 if(!(p->key_inventory=='I'||p->key_inventory=='B'||p->key_inventory=='V'||p->key_inventory=='C'))return 0;
 if(!(p->key_yes=='1'||p->key_yes=='J'||p->key_yes=='Q'))return 0;
 if(!(p->key_no=='2'||p->key_no=='K'||p->key_no=='E'))return 0;
 return p->key_talk!=p->key_inventory&&p->key_talk!=p->key_yes&&p->key_talk!=p->key_no&&
        p->key_inventory!=p->key_yes&&p->key_inventory!=p->key_no&&p->key_yes!=p->key_no;
}
int forge_validate(const ForgeProject*p,char*error,int cap){int i,j,x,y;
 if(!forge_controls_valid(p)){forge_copy_string(error,cap,"Controles invalidos o teclas duplicadas.");return 0;}if(p->magic!=KEY_MAGIC||p->version!=FORGE_VERSION){forge_copy_string(error,cap,"Formato de proyecto incorrecto.");return 0;}if(p->custom_tile_count>CUSTOM_TILE_COUNT){forge_copy_string(error,cap,"Cantidad de tiles propios invalida.");return 0;}if(p->scene_count<1||p->scene_count>MAX_SCENES){forge_copy_string(error,cap,"Cantidad de escenarios invalida.");return 0;}for(i=0;i<p->scene_count;i++){const ForgeScene*s=&p->scenes[i];for(int q=0;q<FW*FH;q++)if(s->map[q]>=T_COUNT){forge_copy_string(error,cap,"Escenario contiene un terreno inexistente.");return 0;}if(s->npc_count>MAX_NPCS||s->portal_count>MAX_PORTALS){forge_copy_string(error,cap,"Demasiados objetos en un escenario.");return 0;}for(j=0;j<s->npc_count;j++){
    if(p->npc_reward[i][j]>3){forge_copy_string(error,cap,"Recompensa de NPC invalida.");return 0;}
    if(p->npc_event_mode[i][j]>EVENT_FLAG_BRANCH||p->npc_flag_index[i][j]>=FORGE_FLAG_COUNT||p->npc_choice_set_flag[i][j]>1){forge_copy_string(error,cap,"Evento de NPC con condicion invalida.");return 0;}
    if(s->npcs[j].kind>=NPC_ART_SLOTS){forge_copy_string(error,cap,"NPC con sprite inexistente.");return 0;}
    if(p->npc_behavior[i][j]>NPC_LOCAL||p->npc_speed[i][j]>NPC_FAST||p->npc_radius[i][j]<1||p->npc_radius[i][j]>4){forge_copy_string(error,cap,"Opciones de movimiento NPC invalidas.");return 0;}
 }
if(!forge_land(s,s->spawn_x,s->spawn_y,&x,&y)){forge_copy_string(error,cap,"Escenario sin casillas libres.");return 0;}for(j=0;j<s->portal_count;j++){const ForgePortal*q=&s->portals[j];if(q->to>=p->scene_count||q->x>=FW||q->y>=FH||s->map[q->y*FW+q->x]!=T_DOOR){forge_copy_string(error,cap,"Conexion invalida (falta puerta/destino).");return 0;}if(!forge_portal_arrival(p,i,q,&x,&y)){forge_copy_string(error,cap,"Destino de puerta inaccesible.");return 0;}}}forge_copy_string(error,cap,"OK");return 1;}


// Import a project made by v0.5.4.x without losing any settings or scenes.
int forge_import_v11(ForgeProject *out,const ForgeProjectV11 *old,unsigned int bytes){
 if(!out||!old||bytes!=sizeof(*old)||old->magic!=KEY_MAGIC||old->version!=11||old->scene_count<1||old->scene_count>MAX_SCENES)return 0;
 forge_blank(out);
 memcpy(out,old,sizeof(*old));
 out->version=FORGE_VERSION;
 out->key_talk=' ';out->key_inventory='I';out->key_yes='1';out->key_no='2';
 memset(out->npc_reward,0,sizeof(out->npc_reward));
 return 1;
}
// v11 file structure adds ten scene slots and their per-NPC metadata.
// Previous .msxf files used six slots before the graphics atlas. Do not
// memcpy their raw bytes into v11: every subsequent field has moved.
int forge_import_legacy(ForgeProject *out,const ForgeProjectV10 *old,unsigned int bytes){
 if(!out||!old||old->magic!=KEY_MAGIC||old->scene_count<1||old->scene_count>6)return 0;
 if(bytes!=FORGE_V1_FILE_BYTES&&bytes!=FORGE_V2_FILE_BYTES&&bytes!=FORGE_V3_FILE_BYTES&&
    bytes!=FORGE_V4_FILE_BYTES&&bytes!=FORGE_V5_FILE_BYTES&&bytes!=FORGE_V6_FILE_BYTES&&
    bytes!=FORGE_V7_FILE_BYTES&&bytes!=FORGE_V8_FILE_BYTES&&bytes!=FORGE_V9_FILE_BYTES&&
    bytes!=FORGE_V10_FILE_BYTES)return 0;
 forge_blank(out);
 out->magic=KEY_MAGIC;out->version=FORGE_VERSION;out->profile=old->profile;
 out->scene_count=old->scene_count;
 forge_copy_string(out->name,sizeof(out->name),old->name);
 for(int i=0;i<old->scene_count;i++)out->scenes[i]=old->scenes[i];
 memcpy(out->sprite,old->sprite,sizeof(out->sprite));
 memcpy(out->music,old->music,sizeof(out->music));
#define COPY_LEGACY(field) do{if(bytes>=offsetof(ForgeProjectV10,field)+sizeof(old->field))memcpy(out->field,old->field,sizeof(old->field));}while(0)
 COPY_LEGACY(npc_sprites);COPY_LEGACY(npc_sprite_names);
 COPY_LEGACY(tile_sprites);COPY_LEGACY(player_extra_frames);
 COPY_LEGACY(extra_tile_sprites);COPY_LEGACY(interior_tile_sprites);
 COPY_LEGACY(npc_behavior);COPY_LEGACY(npc_speed);COPY_LEGACY(npc_radius);
 COPY_LEGACY(npc_event_mode);COPY_LEGACY(npc_choice_question);
 COPY_LEGACY(custom_tile_pixels);COPY_LEGACY(custom_tile_names);
 COPY_LEGACY(npc_flag_index);COPY_LEGACY(npc_choice_set_flag);
 if(bytes>=offsetof(ForgeProjectV10,custom_tile_count)+1)out->custom_tile_count=old->custom_tile_count;
#undef COPY_LEGACY
 return 1;
}

int forge_duplicate_scene(ForgeProject *p,int index){
 if(!p||p->scene_count>=MAX_SCENES||index<0||index>=p->scene_count)return -1;
 int n=p->scene_count++;
 p->scenes[n]=p->scenes[index];
 forge_copy_string(p->scenes[n].name,sizeof(p->scenes[n].name),"Copia de escenario");
 // The duplicated map retains its doors, except self-referencing ones: these
 // must reference the copy, not inadvertently teleport to the original.
 for(int j=0;j<p->scenes[n].portal_count;j++)if(p->scenes[n].portals[j].to==index)p->scenes[n].portals[j].to=n;
 memcpy(p->npc_behavior[n],p->npc_behavior[index],MAX_NPCS);
 memcpy(p->npc_speed[n],p->npc_speed[index],MAX_NPCS);
 memcpy(p->npc_radius[n],p->npc_radius[index],MAX_NPCS);
 memcpy(p->npc_event_mode[n],p->npc_event_mode[index],MAX_NPCS);
 memcpy(p->npc_choice_question[n],p->npc_choice_question[index],sizeof(p->npc_choice_question[n]));
 memcpy(p->npc_flag_index[n],p->npc_flag_index[index],MAX_NPCS);
 memcpy(p->npc_choice_set_flag[n],p->npc_choice_set_flag[index],MAX_NPCS);
 memcpy(p->npc_reward[n],p->npc_reward[index],MAX_NPCS);
 return n;
}
int forge_delete_scene(ForgeProject *p,int index){
 if(!p||p->scene_count<=1||index<0||index>=p->scene_count)return 0;
 for(int i=0;i<p->scene_count;i++)if(i!=index){
   ForgeScene *s=&p->scenes[i];int k=0;
   for(int j=0;j<s->portal_count;j++){
      ForgePortal q=s->portals[j];
      if(q.to==index){ // remove invalid link, turn former doorway into traversable path
        if(q.x<FW&&q.y<FH)s->map[q.y*FW+q.x]=T_PATH;
        continue;
      }
      if(q.to>index)q.to--;
      s->portals[k++]=q;
   }
   s->portal_count=k;
 }
 for(int i=index;i+1<p->scene_count;i++){
   p->scenes[i]=p->scenes[i+1];
   memcpy(p->npc_behavior[i],p->npc_behavior[i+1],MAX_NPCS);
   memcpy(p->npc_speed[i],p->npc_speed[i+1],MAX_NPCS);
   memcpy(p->npc_radius[i],p->npc_radius[i+1],MAX_NPCS);
   memcpy(p->npc_event_mode[i],p->npc_event_mode[i+1],MAX_NPCS);
   memcpy(p->npc_choice_question[i],p->npc_choice_question[i+1],sizeof(p->npc_choice_question[i]));
   memcpy(p->npc_flag_index[i],p->npc_flag_index[i+1],MAX_NPCS);
   memcpy(p->npc_choice_set_flag[i],p->npc_choice_set_flag[i+1],MAX_NPCS);
 memcpy(p->npc_reward[i],p->npc_reward[i+1],MAX_NPCS);
 }
 p->scene_count--;return 1;
}
