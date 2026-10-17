// level_script_rand_forest01.cpp - Sum.exe v1.40, code 0x4f5910-0x4f5e8f. Written from the Sum.exe
// disassembly (tools/asm.mjs; the floats 0x57eb84 = 0.0, 0x57e6b0 = 0.5, 0x580200 = 2.0 read from the image).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls,
// constants and globals match), likely (one detail of statement order or expression form is a guess), unsure
// (the shape itself is a guess).
//
// One level_script_table record: "rand-forest01" (index 0x12; the forest of the world map: the _v2 script
// holds the Serpent Rider, the _v6 script the Tome of the King of Talas and Gesualdo). FILE NAME NOT PROVEN
// (D5300): no strings.tbl block, no path string; the name follows the level_script_<level>.cpp convention.
// A unit of its own (D5301): .bss 0x257ea68-0x257ea6a, after rand-forestnite1's 0x257ea64-0x257ea65 with a
// 4-byte step and before temple_inside's 0x257ea6c. No static initialiser.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/math/vector.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/unnamed_serpent_0x495290.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_IKAEMOSTOPINT  0xc                     // "got_ring_of_stone" (0x4f5b3e)
#define LEVEL_KHOSANISTRNG   0x17                    // "crone_active" (0x4f5aa0)

void ls_rand_forest01_serpent_intro_cs_end();
void ls_rand_forest01_serpent_dismount_cs_end();
void ls_rand_forest01_serpent_escape_cs_end();

// ---- .bss (0x257ea68-0x257ea6a) ------------------------------------------------------------------------
bool ls_rand_forest01_serpent_intro_done;            // 0x257ea68
bool ls_rand_forest01_serpent_killed_done;           // 0x257ea69
bool ls_rand_forest01_exit_pending;                  // 0x257ea6a "EnterTome": to jadetemple_v2

// 0x4f5910  Confidence: exact (VC6 byte match, match11; was likely) (the five-way choice of the _v6 script: the shared tails of the disable lists
// are the compiler's tail merging of this if / else-if chain; the order of the tests is from the code).
// level_script_table init. _v6: with page 8 of the Tome the King, the Crone, the traders and Gesualdo's first
// self go; with pages 1-6 but not 7 his second self instead; else, at a 50 % chance unless the party talked to
// the king, Gesualdo is gone and the king stays; else while the crone of Khosani is active and has not had
// her rose, the king goes and the crone stays; else only the traders stay. _v2: the Ring of Stone counts as
// found (stage 6), the serpent fight.
void ls_rand_forest01_init()
{
    float chance;

    ls_rand_forest01_exit_pending = false;
    level_music = music_find_by_name("rand-forest01");
    chance = frand();
    if (savegame_state.get_flag(cur_level_index, "kingtalas_mq_talked_to_king"))
        chance += 2.0f;
    if (!_stricmp(level_script_name, "rand-forest01_v6")) {
        if (party_count_item_by_name("Parchment#Tome Page 8")) {
            script_npc_set_disabled("Gesualdo#part1", true);
            script_npc_set_disabled("King of Talas", true);
            script_npc_set_disabled("Crone of Savheo", true);
            script_npc_set_disabled("Trader#1", true);
            script_npc_set_disabled("Trader#2", true);
            script_npc_set_disabled("Trader#3", true);
        } else if (party_count_item_by_name("Parchment#Tome Page 1")
                   && party_count_item_by_name("Parchment#Tome Page 2")
                   && party_count_item_by_name("Parchment#Tome Page 3")
                   && party_count_item_by_name("Parchment#Tome Page 4")
                   && party_count_item_by_name("Parchment#Tome Page 5")
                   && party_count_item_by_name("Parchment#Tome Page 6")
                   && !party_count_item_by_name("Parchment#Tome Page 7")) {
            script_npc_set_disabled("Gesualdo#part2", true);
            script_npc_set_disabled("King of Talas", true);
            script_npc_set_disabled("Crone of Savheo", true);
            script_npc_set_disabled("Trader#1", true);
            script_npc_set_disabled("Trader#2", true);
            script_npc_set_disabled("Trader#3", true);
        } else if (chance < 0.5f && !savegame_state.get_flag(cur_level_index, "kingtalas_mq_talked_to_king")) {
            script_npc_set_disabled("Gesualdo#part1", true);
            script_npc_set_disabled("Gesualdo#part2", true);
            script_npc_set_disabled("Crone of Savheo", true);
            script_npc_set_disabled("Trader#1", true);
            script_npc_set_disabled("Trader#2", true);
            script_npc_set_disabled("Trader#3", true);
        } else if (savegame_state.get_flag(LEVEL_KHOSANISTRNG, "crone_active")
                   && !savegame_state.get_flag(cur_level_index, "crone_mq_gave_rose")) {
            script_npc_set_disabled("Gesualdo#part1", true);
            script_npc_set_disabled("Gesualdo#part2", true);
            script_npc_set_disabled("King of Talas", true);
            script_npc_set_disabled("Trader#1", true);
            script_npc_set_disabled("Trader#2", true);
            script_npc_set_disabled("Trader#3", true);
        } else {
            script_npc_set_disabled("Gesualdo#part1", true);
            script_npc_set_disabled("Gesualdo#part2", true);
            script_npc_set_disabled("King of Talas", true);
            script_npc_set_disabled("Crone of Savheo", true);
        }
    }
    if (!_stricmp(level_script_name, "rand-forest01_v2")) {
        savegame_state.set_flag(LEVEL_IKAEMOSTOPINT, "got_ring_of_stone", true);
        game_stage_set(6);
    }
    if (savegame_state.get_flag(cur_level_index, "serpent_killed")) {
        ls_rand_forest01_serpent_intro_done = true;
        ls_rand_forest01_serpent_killed_done = true;
    } else {
        ls_rand_forest01_serpent_intro_done = false;
        ls_rand_forest01_serpent_killed_done = false;
    }
}

// 0x4f5b90  Confidence: exact (the three strcpy are inline rep movs). As ls_rand_forestnite1_frame for the
// Serpent Rider (here the rider is checked for NULL; no minion call), then the "EnterTome" exit outside
// conversations: the next level is jadetemple (script jadetemple_v2, entry 1), game sequence state 4.
void ls_rand_forest01_frame()
{
    if (!ls_rand_forest01_serpent_intro_done && savegame_state.get_flag(cur_level_index, "serpent_intro_played")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_forest01_serpent_intro_done = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!ls_rand_forest01_serpent_killed_done && savegame_state.get_flag(cur_level_index, "serpent_killed")
        && !(game_flags & GF_CUTSCENE)) {
        ls_rand_forest01_serpent_killed_done = true;
        music_play(level_music, true);
    }
    if (!_stricmp(level_script_name, "rand-forest01_v2")) {
        if (!savegame_state.get_flag(cur_level_index, "serpent_intro_played")) {
            level_trigger_set_script_use("worldmap1", NULL);
            script_play_cutscene("Serpent-Intro", ls_rand_forest01_serpent_intro_cs_end);
            savegame_state.set_flag(cur_level_index, "serpent_intro_played", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "serpent_dismount_played")
            && serpent_dismount_stage_reached == 1) {
            script_play_cutscene("Serpent-Dismount", ls_rand_forest01_serpent_dismount_cs_end);
            savegame_state.set_flag(cur_level_index, "serpent_dismount_played", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "serpent_killed") && serpent_get_rider()
            && serpent_get_rider()->hp <= 0.0f) {
            level_trigger_set_exit("worldmap1", NULL);
            script_play_cutscene("Serpent-Escape", ls_rand_forest01_serpent_escape_cs_end);
            savegame_state.set_flag(cur_level_index, "serpent_killed", true);
        }
    }
    if (!(game_flags & GF_CONVERSATION) && ls_rand_forest01_exit_pending) {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, "jadetemple");
        strcpy(level_script_name, "jadetemple_v2");
        level_entry_point = 1;
        gameseq_set_state(4);
        ls_rand_forest01_exit_pending = false;
    }
}

// 0x4f5df0  Confidence: exact. End of Serpent-Intro: the fight starts.
void ls_rand_forest01_serpent_intro_cs_end()
{
    serpent_combat = true;
    level_script_event_active = true;
}

// 0x4f5e00  Confidence: exact. End of Serpent-Dismount: serpent_check_stage may go on.
void ls_rand_forest01_serpent_dismount_cs_end()
{
    serpent_stage_trigger = true;
}

// 0x4f5e10  Confidence: exact. End of Serpent-Escape: once, the rider and his horse go.
void ls_rand_forest01_serpent_escape_cs_end()
{
    if (!savegame_state.get_flag(cur_level_index, "serpent_killed2")) {
        script_npc_set_disabled("Serpent Rider", true);
        script_npc_set_disabled("WarHorse1#$WHRS055", true);
        savegame_state.set_flag(cur_level_index, "serpent_killed2", true);
        level_script_event_active = false;
    }
}

// 0x4f5e70  Confidence: exact. level_script_table action.
void ls_rand_forest01_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "EnterTome"))
        ls_rand_forest01_exit_pending = true;
}
