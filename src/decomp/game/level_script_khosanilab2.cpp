// level_script_khosanilab2.cpp - Sum.exe v1.40, code 0x4f2250-0x4f244b. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "KhosaniLab2" (index 0x1b; Pyrul's lair behind the Khosani laboratory: the
// intro at $salstart, the fight, the death cutscene). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no
// path string; the name follows the level_script_<record>.cpp convention. A unit of its own (D5301): .bss
// 0x257e9f8-0x257e9f9 between wolong2's 0x257e9f4 and lpalaceint02's 0x257e9fc; literals 0x5a27f8-0x5a2838.
// "pyrulstart" and "trapped_pyrul" (0x59ee54, 0x59edf8), "Pyrul" and "KhosaniLab" are pooled copies.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void lsf_khosanilab2_flag_pyrulstart();
void ls_khosanilab2_pyrul_intro_cs_end();
void ls_khosanilab2_pyrul_death_cs_end();

// ---- .bss (0x257e9f8-0x257e9f9) ------------------------------------------------------------------------
bool ls_khosanilab2_pyrulstart_done;                 // 0x257e9f8 the Pyrul music was started
bool ls_khosanilab2_trapped_pyrul_done;              // 0x257e9f9 the lab music is back

// 0x4f2250  Confidence: exact. level_script_table init (stage 17): gate A open, gate B shut, Pyrul out of
// combat until his intro ends.
void ls_khosanilab2_init()
{
    level_music = music_find_by_name("KhosaniLab");
    game_stage_set(0x11);
    door_apply_saved_state("BOSS-GateA-Door01", true);
    door_apply_saved_state("BOSS-GateB-Door01", false);
    pyrul_combat_enabled = false;
    ls_khosanilab2_pyrulstart_done = false;
    ls_khosanilab2_trapped_pyrul_done = false;
}

// 0x4f22a0  Confidence: exact. level_script_table frame: "jmp 0x4f22b0" (as ls_lenele1c_frame, D2704).
void ls_khosanilab2_frame()
{
    lsf_khosanilab2_flag_pyrulstart();
}

// 0x4f22b0  Confidence: exact (not in sum.symbols; name ours, after its first flag, D5603). Within 4 of
// $salstart the gate behind the player shuts and Pyrul-Intro plays; after it the Pyrul music. Pyrul dead:
// his loot, Pyrul-Death, gate B opens; after it the lab music.
void lsf_khosanilab2_flag_pyrulstart()
{
    int music;

    if (!savegame_state.get_flag(cur_level_index, "pyrulstart") && script_player_near_navpoint("$salstart", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "pyrulstart", true);
        door_apply_saved_state("BOSS-GateA-Door01", false);
        script_play_cutscene("Pyrul-Intro", ls_khosanilab2_pyrul_intro_cs_end);
    }
    if (!ls_khosanilab2_pyrulstart_done && savegame_state.get_flag(cur_level_index, "pyrulstart")
        && !(game_flags & GF_CUTSCENE)) {
        ls_khosanilab2_pyrulstart_done = true;
        music = music_find_by_name("Pyrul");
        music_play(music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "trapped_pyrul") && script_npc_is_dead("Pyrul")) {
        savegame_state.set_flag(cur_level_index, "trapped_pyrul", true);
        script_npc_drop_loot("Pyrul");
        script_play_cutscene("Pyrul-Death", ls_khosanilab2_pyrul_death_cs_end);
        door_apply_saved_state("BOSS-GateB-Door01", true);
    }
    if (!ls_khosanilab2_trapped_pyrul_done && savegame_state.get_flag(cur_level_index, "trapped_pyrul")
        && !(game_flags & GF_CUTSCENE)) {
        ls_khosanilab2_trapped_pyrul_done = true;
        music_play(music_find_by_name("KhosaniLab"), true);
    }
}

// 0x4f2420  Confidence: exact (the two stores from AL, D5302). End of Pyrul-Intro: Pyrul fights.
void ls_khosanilab2_pyrul_intro_cs_end()
{
    pyrul_combat_enabled = true;
    level_script_event_active = true;
}

// 0x4f2430  Confidence: exact. End of Pyrul-Death.
void ls_khosanilab2_pyrul_death_cs_end()
{
    script_npc_set_disabled("Pyrul", true);
    pyrul_combat_enabled = false;
    level_script_event_active = false;
}
