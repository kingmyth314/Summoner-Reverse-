// level_script_rand_forestnite1.cpp - Sum.exe v1.40, code 0x4f5670-0x4f5903. Written from the Sum.exe
// disassembly (tools/asm.mjs; the float 0x57eb84 = 0.0 read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (one detail of statement order or expression form is a guess), unsure (the shape itself is a
// guess).
//
// One level_script_table record: "rand-forestnite1" (index 0x13; the night forest, the _v2 script at stage 4
// holds the Ghost Rider). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path string; the name follows
// the level_script_<level>.cpp convention. A unit of its own and not one with rand-forest01 (D5301): its
// .bss is 0x257ea64-0x257ea65 and rand-forest01's starts at 0x257ea68, a 4-byte step that a single object's
// packed bools would not leave. No static initialiser.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "engine/objects/living_entity.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_rand_forestnite1_ghost_intro_cs_end();
void ls_rand_forestnite1_ghost_dismount_cs_end();
void ls_rand_forestnite1_ghost_escape_cs_end();

// ---- .bss (0x257ea64-0x257ea65) ------------------------------------------------------------------------
bool ls_rand_forestnite1_ghost_intro_done;           // 0x257ea64
bool ls_rand_forestnite1_ghost_killed_done;          // 0x257ea65

// 0x4f5670  Confidence: likely (the two flags are stored from AL on both paths, ls_rand_forest01_init's
// same statements as immediates; which source form gives which is not known, D5302). level_script_table
// init (stage 4 on the _v2 script).
void ls_rand_forestnite1_init()
{
    level_music = music_find_by_name("rand-forestnite1");
    if (!_stricmp(level_script_name, "rand-forestnite1_v2"))
        game_stage_set(4);
    if (savegame_state.get_flag(cur_level_index, "ghost_killed")) {
        ls_rand_forestnite1_ghost_intro_done = true;
        ls_rand_forestnite1_ghost_killed_done = true;
    } else {
        ls_rand_forestnite1_ghost_intro_done = false;
        ls_rand_forestnite1_ghost_killed_done = false;
    }
}

// 0x4f56e0  Confidence: exact. The boss music after the intro, the level music after the kill; on the _v2
// script Ghost-Intro once (the world map exit becomes a script use), Ghost-Dismount when the boss asks for
// it, and Ghost-Escape when the rider's HP reach 0 (the exit opens again, the minions stop). The rider is not
// checked for NULL (ls_rand_forest01_frame checks the serpent rider; see NOTES).
void ls_rand_forestnite1_frame()
{
    if (!ls_rand_forestnite1_ghost_intro_done && savegame_state.get_flag(cur_level_index, "ghost_intro_played")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_forestnite1_ghost_intro_done = true;
        music_play(music_find_by_name("Boss"), true);
    }
    if (!ls_rand_forestnite1_ghost_killed_done && savegame_state.get_flag(cur_level_index, "ghost_killed")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_forestnite1_ghost_killed_done = true;
        music_play(level_music, true);
    }
    if (_stricmp(level_script_name, "rand-forestnite1_v2"))
        return;
    if (!savegame_state.get_flag(cur_level_index, "ghost_intro_played")) {
        level_trigger_set_script_use("worldmap1", NULL);
        script_play_cutscene("Ghost-Intro", ls_rand_forestnite1_ghost_intro_cs_end);
        savegame_state.set_flag(cur_level_index, "ghost_intro_played", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ghost_dismount_played") && ghost_dismount_requested == 1) {
        script_play_cutscene("Ghost-Dismount", ls_rand_forestnite1_ghost_dismount_cs_end);
        savegame_state.set_flag(cur_level_index, "ghost_dismount_played", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ghost_killed") && ghost_find_rider()->hp <= 0.0f) {
        level_trigger_set_exit("worldmap1", NULL);
        ghost_minions_deactivate();
        script_play_cutscene("Ghost-Escape", ls_rand_forestnite1_ghost_escape_cs_end);
        savegame_state.set_flag(cur_level_index, "ghost_killed", true);
    }
}

// 0x4f5890  Confidence: exact (one AL for both stores, as in ls_templeint_tiger_intro_cs_end). End of
// Ghost-Intro: the fight starts.
void ls_rand_forestnite1_ghost_intro_cs_end()
{
    ghost_combat_enabled = true;
    level_script_event_active = true;
}

// 0x4f58a0  Confidence: exact. End of Ghost-Dismount: the boss may go on.
void ls_rand_forestnite1_ghost_dismount_cs_end()
{
    ghost_dismount_done = true;
}

// 0x4f58b0  Confidence: exact. End of Ghost-Escape: once, the rider and his horse go.
void ls_rand_forestnite1_ghost_escape_cs_end()
{
    if (!savegame_state.get_flag(cur_level_index, "ghost_killed2")) {
        script_npc_set_disabled("Ghost Rider", true);
        script_npc_set_disabled("WarHorse1#$WHRS055", true);
        savegame_state.set_flag(cur_level_index, "ghost_killed2", true);
        level_script_event_active = false;
    }
}
