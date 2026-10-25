#ifndef MSX_FORGE_ENGINE_H
#define MSX_FORGE_ENGINE_H
#include <stdint.h>
#include <stddef.h>
#define FW 16
#define FH 12
#define MAX_SCENES 16
#define MAX_NPCS 8
#define NPC_ART_SLOTS 3
#define FORGE_VERSION 12
#define CUSTOM_TILE_COUNT 8
#define FORGE_FLAG_COUNT 4
#define MAX_PORTALS 8
#define MAX_MESSAGE 128
#define KEY_MAGIC 0x3147534Du
#define MAX_PROJECT_BYTES 100000

enum {T_GRASS,T_TREE,T_PATH,T_WATER,T_WALL,T_DOOR,T_FLOWER,T_DIRT,
      T_ROOF,T_BRICK,T_WINDOW,T_FENCE,T_GARDEN,T_PAVEMENT,T_FLOOR,T_TABLE,
      T_BED,T_SHELF,T_COUNTER,T_RUG,T_CHAIR,T_LAMP,T_CABINET,T_SINK,
      T_USER_1,T_USER_2,T_USER_3,T_USER_4,T_USER_5,T_USER_6,T_USER_7,T_USER_8,T_COUNT};
#define LEGACY_TILE_COUNT 8
#define NEW_TILE_COUNT 8
#define INTERIOR_TILE_COUNT 8
enum {PROFILE_MSX=0,PROFILE_MSX2=1,PROFILE_MSX2PLUS=2};
enum {NPC_STILL=0,NPC_FREE=1,NPC_LOCAL=2};
enum {NPC_SLOW=0,NPC_NORMAL=1,NPC_FAST=2};
enum {EVENT_CLASSIC=0,EVENT_KEY_BRANCH=1,EVENT_CHOICE=2,EVENT_FLAG_BRANCH=3};
enum {MOVE_UP,MOVE_DOWN,MOVE_LEFT,MOVE_RIGHT,TALK,INVENTORY,CHOOSE_YES,CHOOSE_NO};

typedef struct {
    uint8_t x,y,gives_key,kind;
    char name[28],first[78],again[78];
} ForgeNPC;
typedef struct {
    uint8_t x,y,to,tx,ty,requires_key;
} ForgePortal;
typedef struct {
    char name[32];
    uint8_t map[FW*FH];
    uint8_t spawn_x,spawn_y;
    uint8_t npc_count,portal_count;
    ForgeNPC npcs[MAX_NPCS];
    ForgePortal portals[MAX_PORTALS];
} ForgeScene;
typedef struct {
    uint32_t magic,version;
    uint8_t profile,scene_count;
    uint8_t reserved[2];
    char name[48];
    ForgeScene scenes[6];
    uint8_t sprite[256]; // 0...15 indices; 255=transparent
    uint8_t music[3][16]; // 0=silence, 1...8 note
    // Added after the old v1 binary payload: v1 files can be upgraded safely.
    uint8_t npc_sprites[NPC_ART_SLOTS][256];
    char npc_sprite_names[NPC_ART_SLOTS][24];
    uint8_t tile_sprites[LEGACY_TILE_COUNT][256]; // Editable terrain atlas for MSX2 SCREEN 4.
    // Animation: direction 0=down, 1=up, 2=left, 3=right, two frames.
    // Down frame 0 stays in sprite[] for legacy compatibility.
    uint8_t player_extra_frames[7][256];
    // v5 extension; appended to preserve the complete legacy v4 prefix.
    uint8_t extra_tile_sprites[NEW_TILE_COUNT][256];
    // v6 extension: per-NPC AI settings. Keep all previous file bytes intact.
    uint8_t npc_behavior[6][MAX_NPCS];
    uint8_t npc_speed[6][MAX_NPCS];
    uint8_t npc_radius[6][MAX_NPCS];
    // v7: appended after the entire 0.4.5 payload; old projects upgrade safely.
    uint8_t interior_tile_sprites[INTERIOR_TILE_COUNT][256];
    // v8: visual NPC event condition, appended after v7 (0.4.6.2) payload.
    uint8_t npc_event_mode[6][MAX_NPCS];
    // v9: freeform prompt for a two-option interactive NPC conversation.
    char npc_choice_question[6][MAX_NPCS][64];
    // v10: 8 new editable tiles and 4 global mission flags.
    uint8_t custom_tile_pixels[CUSTOM_TILE_COUNT][256];
    char custom_tile_names[CUSTOM_TILE_COUNT][24];
    uint8_t npc_flag_index[6][MAX_NPCS];
    uint8_t npc_choice_set_flag[6][MAX_NPCS];
    uint8_t custom_tile_count;
} ForgeProjectV10;

typedef struct {
    uint32_t magic,version;
    uint8_t profile,scene_count;
    uint8_t reserved[2];
    char name[48];
    ForgeScene scenes[MAX_SCENES];
    uint8_t sprite[256]; // 0...15 indices; 255=transparent
    uint8_t music[3][16]; // 0=silence, 1...8 note
    // Added after the old v1 binary payload: v1 files can be upgraded safely.
    uint8_t npc_sprites[NPC_ART_SLOTS][256];
    char npc_sprite_names[NPC_ART_SLOTS][24];
    uint8_t tile_sprites[LEGACY_TILE_COUNT][256]; // Editable terrain atlas for MSX2 SCREEN 4.
    // Animation: direction 0=down, 1=up, 2=left, 3=right, two frames.
    // Down frame 0 stays in sprite[] for legacy compatibility.
    uint8_t player_extra_frames[7][256];
    // v5 extension; appended to preserve the complete legacy v4 prefix.
    uint8_t extra_tile_sprites[NEW_TILE_COUNT][256];
    // v6 extension: per-NPC AI settings. Keep all previous file bytes intact.
    uint8_t npc_behavior[MAX_SCENES][MAX_NPCS];
    uint8_t npc_speed[MAX_SCENES][MAX_NPCS];
    uint8_t npc_radius[MAX_SCENES][MAX_NPCS];
    // v7: appended after the entire 0.4.5 payload; old projects upgrade safely.
    uint8_t interior_tile_sprites[INTERIOR_TILE_COUNT][256];
    // v8: visual NPC event condition, appended after v7 (0.4.6.2) payload.
    uint8_t npc_event_mode[MAX_SCENES][MAX_NPCS];
    // v9: freeform prompt for a two-option interactive NPC conversation.
    char npc_choice_question[MAX_SCENES][MAX_NPCS][64];
    // v10: 8 new editable tiles and 4 global mission flags.
    uint8_t custom_tile_pixels[CUSTOM_TILE_COUNT][256];
    char custom_tile_names[CUSTOM_TILE_COUNT][24];
    uint8_t npc_flag_index[MAX_SCENES][MAX_NPCS];
    uint8_t npc_choice_set_flag[MAX_SCENES][MAX_NPCS];
    uint8_t custom_tile_count;
} ForgeProjectV11;
typedef struct {
    uint32_t magic,version;
    uint8_t profile,scene_count;
    uint8_t reserved[2];
    char name[48];
    ForgeScene scenes[MAX_SCENES];
    uint8_t sprite[256]; // 0...15 indices; 255=transparent
    uint8_t music[3][16]; // 0=silence, 1...8 note
    // Added after the old v1 binary payload: v1 files can be upgraded safely.
    uint8_t npc_sprites[NPC_ART_SLOTS][256];
    char npc_sprite_names[NPC_ART_SLOTS][24];
    uint8_t tile_sprites[LEGACY_TILE_COUNT][256]; // Editable terrain atlas for MSX2 SCREEN 4.
    // Animation: direction 0=down, 1=up, 2=left, 3=right, two frames.
    // Down frame 0 stays in sprite[] for legacy compatibility.
    uint8_t player_extra_frames[7][256];
    // v5 extension; appended to preserve the complete legacy v4 prefix.
    uint8_t extra_tile_sprites[NEW_TILE_COUNT][256];
    // v6 extension: per-NPC AI settings. Keep all previous file bytes intact.
    uint8_t npc_behavior[MAX_SCENES][MAX_NPCS];
    uint8_t npc_speed[MAX_SCENES][MAX_NPCS];
    uint8_t npc_radius[MAX_SCENES][MAX_NPCS];
    // v7: appended after the entire 0.4.5 payload; old projects upgrade safely.
    uint8_t interior_tile_sprites[INTERIOR_TILE_COUNT][256];
    // v8: visual NPC event condition, appended after v7 (0.4.6.2) payload.
    uint8_t npc_event_mode[MAX_SCENES][MAX_NPCS];
    // v9: freeform prompt for a two-option interactive NPC conversation.
    char npc_choice_question[MAX_SCENES][MAX_NPCS][64];
    // v10: 8 new editable tiles and 4 global mission flags.
    uint8_t custom_tile_pixels[CUSTOM_TILE_COUNT][256];
    char custom_tile_names[CUSTOM_TILE_COUNT][24];
    uint8_t npc_flag_index[MAX_SCENES][MAX_NPCS];
    uint8_t npc_choice_set_flag[MAX_SCENES][MAX_NPCS];
    uint8_t custom_tile_count;
    // v12: project-specific keys and small objective rewards.
    // talk: SPACE, Z, X, RETURN; inventory: I, B, V, C.
    // yes: 1, J, Q; no: 2, K, E.
    uint8_t key_talk, key_inventory, key_yes, key_no;
    uint8_t npc_reward[MAX_SCENES][MAX_NPCS]; // 0,5,10,20 coins when a flag is already active
} ForgeProject;
#define FORGE_V1_FILE_BYTES ((int)offsetof(ForgeProjectV10, npc_sprites))
#define FORGE_V2_FILE_BYTES ((int)offsetof(ForgeProjectV10, tile_sprites))
#define FORGE_V3_FILE_BYTES ((int)offsetof(ForgeProjectV10, player_extra_frames))
#define FORGE_V4_FILE_BYTES ((int)offsetof(ForgeProjectV10, extra_tile_sprites))
#define FORGE_V5_FILE_BYTES ((int)offsetof(ForgeProjectV10, npc_behavior))
#define FORGE_V6_FILE_BYTES ((int)offsetof(ForgeProjectV10, interior_tile_sprites))
#define FORGE_V7_FILE_BYTES ((int)offsetof(ForgeProjectV10, npc_event_mode))
#define FORGE_V8_FILE_BYTES ((int)offsetof(ForgeProjectV10, npc_choice_question))
#define FORGE_V9_FILE_BYTES ((int)offsetof(ForgeProjectV10, custom_tile_pixels))
#define FORGE_V10_FILE_BYTES ((int)sizeof(ForgeProjectV10))
#define FORGE_V11_FILE_BYTES ((int)sizeof(ForgeProjectV11))
typedef struct {
    uint8_t scene,x,y,has_key;
    uint8_t facing,walk_frame;
    uint8_t npc_x[MAX_NPCS],npc_y[MAX_NPCS],npc_tick,bubble_npc;
    uint8_t spoken[MAX_SCENES][MAX_NPCS];
    uint8_t choice_pending,choice_scene,choice_npc;
    uint8_t flags[FORGE_FLAG_COUNT];
    uint8_t reward_claimed[FORGE_FLAG_COUNT],coins;
    char message[MAX_MESSAGE];
} ForgeState;

void forge_blank(ForgeProject *p);
int forge_import_legacy(ForgeProject *out,const ForgeProjectV10 *old,unsigned int bytes);
int forge_import_v11(ForgeProject *out,const ForgeProjectV11 *old,unsigned int bytes);
int forge_controls_valid(const ForgeProject *p);
int forge_delete_scene(ForgeProject *p,int index);
int forge_duplicate_scene(ForgeProject *p,int index);
/* Copy a complete NPC and its event metadata to a free map position. Returns new index or -1. */
int forge_duplicate_npc(ForgeProject *p,int scene,int npc,int x,int y);
/* Delete an NPC, preserving per-character event settings of the survivors. */
int forge_delete_npc(ForgeProject *p,int scene,int npc);
void forge_upgrade_v1(ForgeProject *p);
void forge_upgrade_v2(ForgeProject *p);
void forge_upgrade_v3(ForgeProject *p);
uint8_t *forge_player_frame(ForgeProject *p,int direction,int frame);
const uint8_t *forge_player_frame_const(const ForgeProject *p,int direction,int frame);
void forge_default_tile_art(ForgeProject *p);
void forge_upgrade_v4(ForgeProject *p);
void forge_upgrade_v5(ForgeProject *p);
void forge_default_npc_behavior(ForgeProject *p);
void forge_default_extra_tile_art(ForgeProject *p);
void forge_default_interior_tile_art(ForgeProject *p);
void forge_upgrade_v6(ForgeProject *p);
void forge_upgrade_v7(ForgeProject *p);
void forge_upgrade_v8(ForgeProject *p);
void forge_upgrade_v9(ForgeProject *p);
int forge_add_interior(ForgeProject *p,int style,int origin_scene);
uint8_t *forge_tile_pixels(ForgeProject *p,int id);
const uint8_t *forge_tile_pixels_const(const ForgeProject *p,int id);
int forge_tile_blocking(int id);
int forge_tile_msx1_fallback(int id);
void forge_default_npc_art(ForgeProject *p);
void forge_demo(ForgeProject *p);
void forge_start(const ForgeProject *p,ForgeState *s);
int forge_move(const ForgeProject *p,ForgeState *s,int command);
int forge_tick(const ForgeProject *p,ForgeState *s);
int forge_land(const ForgeScene *sc,int x,int y,int *out_x,int *out_y);
// Destino efectivo: la salida de un interior regresa junto a su puerta exterior.
int forge_portal_arrival(const ForgeProject *p,int source_scene,const ForgePortal *portal,int *out_x,int *out_y);
int forge_tile_walkable(const ForgeScene *sc,int x,int y);
int forge_reachable(const ForgeScene *sc,int sx,int sy,int tx,int ty);
int forge_validate(const ForgeProject *p,char *error,int cap);
int forge_export_rom(const ForgeProject *p,uint8_t *rom,int cap,char *error,int errcap);
void forge_sprite_1bit(const uint8_t sprite[256],uint8_t out[32]);
void forge_copy_string(char *dest,int cap,const char *src);
#endif
