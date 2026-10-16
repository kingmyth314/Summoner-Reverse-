// level_script_rand_hills01.cpp - Sum.exe v1.40, code 0x4fa420-0x4fa7b2. Written from the Sum.exe
// disassembly (tools/asm.mjs; the float 0x57eb84 = 0.0 read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (one detail of statement order or expression form is a guess), unsure (the shape itself is a
// guess).
//
// One level_script_table record: "rand-hills01" (index 0xa; the hills of the world map: the _v2 script holds
// the Phoenix Rider and the way to Khosani). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path
// string; the name follows the level_script_<level>.cpp convention. Data: .bss 0x257ed00-0x257ed02, after
// level_script_ikaemosu.cpp's 0x257ecfc-0x257ecfe with a 4-byte step. No static initialiser.
// The hidden function 0x4fa6f0 (no sum.symbols entry, the frame's tail jump) is
// ls_rand_hills01_goto_khosanistrng (name ours).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "engine/objects/living_entity.h"
#include "engine/ai/unnamed_phoenix_0x497ac0.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_WORLDMAP1      4                       // "found_khosani" (0x4fa79f)
#define LEVEL_KHOSANILAB     0xe                     // "got_ring_of_fire" (0x4fa44c)

void ls_rand_hills01_phoenix_intro_cs_end();
void ls_rand_hills01_phoenix_dismount_cs_end();
void ls_rand_hills01_phoenix_escape_cs_end();
void ls_rand_hills01_goto_khosanistrng();

// ---- .bss (0x257ed00-0x257ed02) ------------------------------------------------------------------------
bool ls_rand_hills01_phoenix_intro_done;             // 0x257ed00
bool ls_rand_hills01_phoenix_killed_done;            // 0x257ed01
bool ls_rand_hills01_found_khosani;                  // 0x257ed02 "GotoKhosani"

// 0x4fa420  Confidence: exact. level_script_table init. _v2: the Ring of Fire counts as found (stage 6).
void ls_rand_hills01_init()
{
    level_music = music_find_by_name("rand-hills01");
    if (!_stricmp(level_script_name, "rand-hills01_v2")) {
        savegame_state.set_flag(LEVEL_KHOSANILAB, "got_ring_of_fire", true);
        game_stage_set(6);
    }
    ls_rand_hills01_found_khosani = false;
    if (savegame_state.get_flag(cur_level_index, "phoenix_killed")) {
        ls_rand_hills01_phoenix_intro_done = true;
        ls_rand_hills01_phoenix_killed_done = true;
    } else {
        ls_rand_hills01_phoenix_intro_done = false;
        ls_rand_hills01_phoenix_killed_done = false;
    }
}

// 0x4fa4a0  Confidence: exact (ends in a tail jump to ls_rand_hills01_goto_khosanistrng). As
// ls_rand_forestnite1_frame for the Phoenix Rider; the escape needs the rider's HP at 0 after the rebirth
// (PHOENIX_F_REBORN). The rider is not checked for NULL. Then "GotoKhosani" outside conversations.
void ls_rand_hills01_frame()
{
    if (!ls_rand_hills01_phoenix_intro_done && savegame_state.get_flag(cur_level_index, "phoenix_intro_played")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_hills01_phoenix_intro_done = true;
        music_play(music_find_by_name("Boss"), true);
    }
    if (!ls_rand_hills01_phoenix_killed_done && savegame_state.get_flag(cur_level_index, "phoenix_killed")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_hills01_phoenix_killed_done = true;
        music_play(level_music, true);
    }
    if (!_stricmp(level_script_name, "rand-hills01_v2")) {
        if (!savegame_state.get_flag(cur_level_index, "phoenix_intro_played")) {
            level_trigger_set_script_use("worldmap1", NULL);
            script_play_cutscene("PHNX-Intro", ls_rand_hills01_phoenix_intro_cs_end);
            savegame_state.set_flag(cur_level_index, "phoenix_intro_played", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "phoenix_dismount_played")
            && phoenix_dismount_stage_reached == 1) {
            script_play_cutscene("PHNX-Dismount", ls_rand_hills01_phoenix_dismount_cs_end);
            savegame_state.set_flag(cur_level_index, "phoenix_dismount_played", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "phoenix_killed") && phoenix_find_rider()->hp <= 0.0f
            && (phoenix_flags & PHOENIX_F_REBORN)) {
            level_trigger_set_exit("worldmap1", NULL);
            script_play_cutscene("PHNX-Escape", ls_rand_hills01_phoenix_escape_cs_end);
            savegame_state.set_flag(cur_level_index, "phoenix_killed", true);
        }
    }
    if (!(game_flags & GF_CONVERSATION) && ls_rand_hills01_found_khosani)
        ls_rand_hills01_goto_khosanistrng();
}

// 0x4fa670  Confidence: exact. End of PHNX-Intro: the fight starts.
void ls_rand_hills01_phoenix_intro_cs_end()
{
    phoenix_combat = true;
    level_script_event_active = true;
}

// 0x4fa680  Confidence: exact. End of PHNX-Dismount.
void ls_rand_hills01_phoenix_dismount_cs_end()
{
    phoenix_stage_trigger = true;
}

// 0x4fa690  Confidence: exact. End of PHNX-Escape: once, the rider and his horse go.
void ls_rand_hills01_phoenix_escape_cs_end()
{
    if (!savegame_state.get_flag(cur_level_index, "phoenix_killed2")) {
        script_npc_set_disabled("Phoenix Rider", true);
        script_npc_set_disabled("WarHorse1#$npc023", true);
        savegame_state.set_flag(cur_level_index, "phoenix_killed2", true);
        level_script_event_active = false;
    }
}

// 0x4fa6f0  Confidence: likely (no sum.symbols entry; a function of its own, reached only by the frame's tail
// jump and placed after the cutscene callbacks; name ours, D5304). The level change to khosanistrng (entry
// 1), game sequence state 4. ls_rand_hills01_found_khosani is not cleared here (the next init clears it).
void ls_rand_hills01_goto_khosanistrng()
{
    strcpy(level_prev_filename, level_filename);
    strcpy(level_filename, "khosanistrng");
    strcpy(level_script_name, "khosanistrng");
    level_entry_point = 1;
    gameseq_set_state(4);
}

// 0x4fa780  Confidence: exact. level_script_table action: "GotoKhosani" also sets the world map's
// found_khosani.
void ls_rand_hills01_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "GotoKhosani")) {
        savegame_state.set_flag(LEVEL_WORLDMAP1, "found_khosani", true);
        ls_rand_hills01_found_khosani = true;
    }
}
