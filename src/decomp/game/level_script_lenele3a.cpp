// level_script_lenele3a.cpp - Sum.exe v1.40, code 0x4f0f70-0x4f1061 (levelscripts\Scripts\
// level_script_lenele3a.cpp; strings.tbl id 503 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele3a" (the snowy square before the temple; its frame and action
// hooks are empty_function). Which of templeint2 / lpalaceint02 is a live exit depends on the stage.
// No static initialiser, no .bss, no .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"

// 0x4f0f70  Confidence: exact. level_script_table init. Stage 21 and later: the temple (templeint2) is
// open and the palace (lpalaceint02) closed; stage 17-20 the other way round; before that the stage is
// raised to 16, as in 17-20.
void ls_lenele3a_init()
{
    snow_start();
    level_music = music_find_by_name("lenele3");
    if (game_stage_get() >= 0x15) {
        level_trigger_set_exit("templeint2", NULL);
        level_trigger_set_script_use("lpalaceint02", NULL);
        return;
    }
    if (game_stage_get() >= 0x11) {
        level_trigger_set_script_use("templeint2", NULL);
        level_trigger_set_exit("lpalaceint02", NULL);
        return;
    }
    game_stage_set(0x10);
    level_trigger_set_script_use("templeint2", NULL);
    level_trigger_set_exit("lpalaceint02", NULL);
}

// 0x4f1000  Confidence: exact. level_script_table leave: from stage 17 Joseph and Flece rejoin the party
// on the way out (ls_lenele3d_init removed them in stage 16).
void ls_lenele3a_leave()
{
    snow_stop();
    if (game_stage_get() >= 0x11) {
        party_add_member("Joseph");
        party_add_member("Flece");
    }
}

// 0x4f1030  Confidence: exact. level_script_table use. It tests "templeint", but the script-use trigger
// of lenele3a.tbl is "templeint2" (no +RefName:), so the refusal is never said (see NOTES).
char ls_lenele3a_use(const char *name)
{
    if (!_stricmp(name, "templeint"))
        script_party_member_say("Joseph", XSTR("We're not ready to confront Machival yet.", 503), 5.0f);
    return 0;
}
