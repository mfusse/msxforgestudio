// Qt-free integration test of the exact C engine linked to the new GUI.
// Run from the repository root: bash tests/run_core_tests.sh
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int condition, const char *message) {
    if(!condition){fprintf(stderr,"FAILED: %s\n",message);exit(1);}
}
int main(int argc,char **argv) {
    static ForgeProject project, loaded;
    static uint8_t rom[32768];
    char error[160] = {0};
    forge_demo(&project);
    check(project.magic==KEY_MAGIC,"magic preserved");
    check(project.version==FORGE_VERSION,"version preserved");
    check(forge_validate(&project,error,sizeof(error)),"demo validates");
    check(forge_export_rom(&project,rom,sizeof(rom),error,sizeof(error))==32768,"demo ROM exports");
    check(rom[0]!=0 || rom[1]!=0,"ROM contains data");

    int scene=0, selected=0;
    ForgeScene *s=&project.scenes[scene];
    int x=2,y=2;
    // Find a valid free tile for a full NPC duplicate.
    for(y=1;y<FH-1;y++) {
        int good=0;
        for(x=1;x<FW-1;x++) {
            int occupied=(s->spawn_x==x&&s->spawn_y==y);
            for(int i=0;i<s->npc_count;i++)occupied |= s->npcs[i].x==x&&s->npcs[i].y==y;
            for(int i=0;i<s->portal_count;i++)occupied |= s->portals[i].x==x&&s->portals[i].y==y;
            if(!occupied&&!forge_tile_blocking(s->map[y*FW+x])){good=1;break;}
        }
        if(good)break;
    }
    check(y<FH-1,"free tile for duplicate");
    ForgeNPC first=s->npcs[selected];
    int prev=s->npc_count;
    int duplicate=forge_duplicate_npc(&project,scene,selected,x,y);
    check(duplicate==prev,"NPC copy allocated");
    check(s->npc_count==prev+1,"NPC count updated");
    check(s->npcs[duplicate].kind==first.kind,"sprite copied");
    check(!strcmp(s->npcs[duplicate].first,first.first),"dialogue copied");
    strcpy(s->npcs[duplicate].name,"COPIA NPC");
    check(strcmp(s->npcs[duplicate].name,s->npcs[selected].name),"copies are independent");
    check(forge_validate(&project,error,sizeof(error)),"duplication remains valid");

    if(argc>1) {
        FILE *f=fopen(argv[1],"rb");check(f!=NULL,"demo file exists");
        check(fread(&loaded,1,sizeof(loaded),f)==sizeof(loaded),"v12 size identical");
        check(fgetc(f)==EOF,"no additional payload");fclose(f);
        check(forge_validate(&loaded,error,sizeof(error)),"0.5.9.4 existing project validates in 0.6.0 engine");
        check(forge_export_rom(&loaded,rom,sizeof(rom),error,sizeof(error))==32768,"existing project exports ROM");
    }
    puts("PASS: exact v12 engine, NPC duplication, legacy 0.5.9.4 project, 32KB ROM");
    return 0;
}
