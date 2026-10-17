// level_script_lpalaceint02.cpp - Sum.exe v1.40, code 0x4f2450-0x4f2821 (levelscripts\Scripts\
// level_script_lpalaceint02.cpp; strings.tbl id 548 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "LPalaceInt02" (the Lenele palace after the fall, stage 16: Sornehan and
// Galienne (SG-Intro), the Khosani scrolls; Joseph and Flece are not in the party here). No static
// initialiser.
// Data: .bss 0x257e9fc-0x257e9fd. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_lpalaceint02_sg_intro_cs_end();

// ---- .bss (0x257e9fc-0x257e9fd) ------------------------------------------------------------------------
bool ls_lpalaceint02_sgintro_done;                   // 0x257e9fc
bool ls_lpalaceint02_bothdead_done;                  // 0x257e9fd

// 0x4f2450  Confidence: exact. level_script_table init (stage 16). Joseph and Flece leave the party until
// the scrolls are found; the world map exit is closed while Sornehan or Galienne lives (the dead ones are
// disabled); after bothdead the music flags are preset.
void ls_lpalaceint02_init()
{
    level_music = music_find_by_name("LPalaceInt02");
    game_stage_set(0x10);
    if (!savegame_state.get_flag(cur_level_index, "got_scrolls")) {
        party_remove_member("Joseph");
        party_remove_member("Flece");
    }
    level_trigger_set_exit("worldmap1", NULL);
    if (!savegame_state.get_flag(cur_level_index, "sornehan_dead"))
        level_trigger_set_script_use("worldmap1", NULL);
    else
        script_npc_set_disabled("Sornehan", true);
    if (!savegame_state.get_flag(cur_level_index, "galienne_dead"))
        level_trigger_set_script_use("worldmap1", NULL);
    else
        script_npc_set_disabled("Galienne", true);
    if (savegame_state.get_flag(cur_level_index, "bothdead")) {
        ls_lpalaceint02_sgintro_done = true;
        ls_lpalaceint02_bothdead_done = true;
    } else {
        ls_lpalaceint02_sgintro_done = false;
        ls_lpalaceint02_bothdead_done = false;
    }
}

// 0x4f2550  Confidence: exact. level_script_table frame. The boss music after SG-Intro (Sornehan within
// 10), the level music after bothdead; the two deaths and the scrolls are recorded, and with all three
// bothdead opens the world map exit.
void ls_lpalaceint02_frame()
{
    if (!ls_lpalaceint02_sgintro_done && savegame_state.get_flag(cur_level_index, "sgintro")
        && !(game_flags & GF_CUTSCENE)) {
        ls_lpalaceint02_sgintro_done = true;
        level_script_event_active = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!ls_lpalaceint02_bothdead_done && savegame_state.get_flag(cur_level_index, "bothdead")
        && !(game_flags & GF_CUTSCENE)) {
        level_script_event_active = false;
        ls_lpalaceint02_bothdead_done = true;
        music_play(level_music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "sgintro")) {
        if (script_npc_near_player("Sornehan", 10.0f)) {
            script_play_cutscene("SG-Intro", ls_lpalaceint02_sg_intro_cs_end);
            savegame_state.set_flag(cur_level_index, "sgintro", true);
        }
    } else {
        if (!savegame_state.get_flag(cur_level_index, "sornehan_dead") && script_npc_is_dead("Sornehan"))
            savegame_state.set_flag(cur_level_index, "sornehan_dead", true);
        if (!savegame_state.get_flag(cur_level_index, "galienne_dead") && script_npc_is_dead("Galienne"))
            savegame_state.set_flag(cur_level_index, "galienne_dead", true);
        if (!savegame_state.get_flag(cur_level_index, "got_scrolls") && party_count_item_by_name("Khosani Scrolls"))
            savegame_state.set_flag(cur_level_index, "got_scrolls", true);
        if (!savegame_state.get_flag(cur_level_index, "bothdead")
            && savegame_state.get_flag(cur_level_index, "sornehan_dead")
            && savegame_state.get_flag(cur_level_index, "galienne_dead")
            && savegame_state.get_flag(cur_level_index, "got_scrolls")) {
            savegame_state.set_flag(cur_level_index, "bothdead", true);
            level_trigger_set_exit("worldmap1", NULL);
        }
    }
}

// 0x4f27a0  Confidence: exact. End of SG-Intro.
void ls_lpalaceint02_sg_intro_cs_end()
{
    script_npc_send_go("Sornehan");
    script_npc_send_go("Galienne");
}

// 0x4f27c0  Confidence: exact. level_script_table leave: from stage 17 Joseph and Flece rejoin.
void ls_lpalaceint02_leave()
{
    if (game_stage_get() >= 0x11) {
        party_add_member("Joseph");
        party_add_member("Flece");
    }
}

// 0x4f27f0  Confidence: exact. level_script_table use.
char ls_lpalaceint02_use(const char *name)
{
    if (!_stricmp(name, "worldmap1"))
        script_party_member_say("Jekhar", XSTR("We're not leaving without the Khosani scrolls.", 548), 5.0f);
    return 0;
}
