// level_script_lenele3d.cpp - Sum.exe v1.40, code 0x4f0d80-0x4f0f6d (levelscripts\Scripts\
// level_script_lenele3d.cpp; strings.tbl id 504 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele3d" (snowy Lenele after the fall of the city, the force field
// over the passage to the sewer; Joseph and Flece leave the party here in stage 16). Static initialiser
// table entry 480 (0x588784) is this unit's.
// The leave hook 0x4f0f00 is shared with IonaExt02 (identical-code folding; this copy is the survivor,
// D2700).
// Data: .bss 0x257e9d0-0x257e9df (0x257e9d4-0x257e9db is not referenced). No .data of its own.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

// ---- .bss (0x257e9d0-0x257e9df) ------------------------------------------------------------------------
// 0x4f0d80  Confidence: exact. static_init_ls_lenele3d_timer: "jmp 0x4f0d90" (initialiser entry 480).
// 0x4f0d90  Confidence: exact. init_ls_lenele3d_timer: "mov ecx, 0x257e9d0 / jmp GameTimer::GameTimer".
GameTimer ls_lenele3d_timer;                         // 0x257e9d0 set to 1 s by ls_lenele3d_init, never read
// Written 0 by ls_lenele3d_init and never read (name ours).
int ls_lenele3d_unused_9dc;                          // 0x257e9dc

// 0x4f0da0  Confidence: exact. level_script_table init (stage 15 at least). From stage 17 Joseph and
// Flece are gone, door01 is open and the force field is off; in stage 16 they leave the party and walk
// off; before that the sewer passage is a script-use trigger (ls_lenele3d_use's refusal).
void ls_lenele3d_init()
{
    snow_start();
    level_music = music_find_by_name("lenele3");
    game_stage_set(0xf);
    if (game_stage_get() >= 0x11) {
        script_npc_set_disabled("Joseph#npc", true);
        script_npc_set_disabled("Flece#npc", true);
        door_apply_saved_state("door01", true);
        fx_disable_by_effect_name("Lenele3d-forcefield.vfx");
        level_trigger_set_exit("sewer", NULL);
    } else if (game_stage_get() >= 0x10) {
        party_remove_member("Joseph");
        party_remove_member("Flece");
        script_npc_send_go("Joseph#npc");
        script_npc_send_go("Flece#npc");
        level_trigger_set_exit("sewer", NULL);
        door_apply_saved_state("door01", true);
    } else {
        script_npc_set_disabled("Joseph#npc", true);
        script_npc_set_disabled("Flece#npc", true);
        door_apply_saved_state("door01", false);
        level_trigger_set_script_use("sewer", NULL);
        game_stage_set(0xf);
    }
    ls_lenele3d_unused_9dc = 0;
    ls_lenele3d_timer.SetTimeout(1000);
}

// 0x4f0eb0  Confidence: exact (the trailing game_stage_get, a tail jump whose result nobody uses: an
// emptied statement of the original). level_script_table frame: Nar-LeneleAct4 once from stage 16.
void ls_lenele3d_frame()
{
    if (game_stage_get() >= 0x10 && !savegame_state.get_flag(cur_level_index, "lenele3_intro")) {
        savegame_state.set_flag(cur_level_index, "lenele3_intro", true);
        script_play_cutscene("Nar-LeneleAct4", NULL);
    }
    game_stage_get();
}

// 0x4f0f00  Confidence: exact. level_script_table leave of lenele3d and IonaExt02 (sum.symbols name: the
// two levels' identical leave hooks were folded into this one copy, which lies in this unit, D2700).
void ls_ionaext02_leave()
{
    snow_stop();
}

// 0x4f0f10  Confidence: exact. level_script_table use: the sewer passage and the way to lenele3a.
char ls_lenele3d_use(const char *name)
{
    if (!_stricmp(name, "sewer"))
        script_party_member_say("Joseph", XSTR("A powerful magic holds me back. I can't go any farther.", 504), 5.0f);
    if (!_stricmp(name, "lenele3a"))
        script_party_member_say("Joseph", XSTR("A powerful magic holds me back. I can't go any farther.", 504), 5.0f);
    return 0;
}
