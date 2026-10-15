// level_script_courtyard.cpp - Sum.exe v1.40, code 0x4fa7c0-0x4fad8b (levelscripts\Scripts\
// level_script_courtyard.cpp; strings.tbl id 481 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// Three level_script_table records of Lenele's palace quarter: "lenele1e" (the palace courtyard),
// "lenele1d" (the dock, untagged in sum.symbols: its three hooks 0x4fa890-0x4faa57 lie between lenele1e's
// and lenele1c's, and its literals 0x5a46ec-0x5a4760 between theirs, so they are this unit's) and
// "lenele1c" (the marketplace gate, where Flece joins). The hidden function 0x4fab90 (lenele1c's frame
// body, no sum.symbols entry) is lsf_lenele1c_flag_found_flece (name ours). No static initialiser, no .bss,
// no .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void lsf_lenele1c_flag_found_flece();
void ls_lenele1c_flece_intro_cs_end();
void ls_lenele1c_flece_intro_p2_cs_end();

// ---- lenele1e ------------------------------------------------------------------------------------------

// 0x4fa7c0  Confidence: exact. level_script_table init. The two palace exits (the lpalaceint and
// lpalaceint_v2 scripts of the same trigger): stage 4 and later lpalaceint_v2, stage 2-3 lpalaceint, before
// neither.
void ls_lenele1e_init()
{
    level_music = music_find_by_name("lenele marketplace");
    game_stage_set(0);
    if (game_stage_get() >= 4) {
        level_trigger_set_exit("lpalaceint", "lpalaceint_v2");
        level_trigger_set_script_use("lpalaceint", "lpalaceint");
        return;
    }
    if (game_stage_get() >= 2) {
        level_trigger_set_exit("lpalaceint", "lpalaceint");
        level_trigger_set_script_use("lpalaceint", "lpalaceint_v2");
        return;
    }
    level_trigger_set_script_use("lpalaceint", "lpalaceint");
    level_trigger_set_script_use("lpalaceint", "lpalaceint_v2");
}

// 0x4fa850  Confidence: exact. level_script_table use.
char ls_lenele1e_use(const char *name)
{
    if (!_stricmp(name, "lpalaceint") && game_stage_get() < 2)
        script_party_member_say("Joseph", XSTR("These guards won't let me pass.", 481), 5.0f);
    return 0;
}

// ---- lenele1d (untagged in sum.symbols) ---------------------------------------------------------------

// 0x4fa890  Confidence: exact. level_script_table init (lenele1d_v3 stage 6, lenele1d_v2 stage 4, else 0).
// The three exits v11 / v21 / v31 by stage; the Muthavi captain is gone once the main quest talked to him.
void ls_lenele1d_init()
{
    level_music = music_find_by_name("lenele slumdock");
    if (!_stricmp(level_script_name, "lenele1d_v3"))
        game_stage_set(6);
    else if (!_stricmp(level_script_name, "lenele1d_v2"))
        game_stage_set(4);
    else
        game_stage_set(0);
    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_exit_by_refname("v31");
    } else {
        if (game_stage_get() >= 4) {
            level_trigger_set_script_use_by_refname("v11");
            level_trigger_set_exit_by_refname("v21");
        } else {
            level_trigger_set_exit_by_refname("v11");
            level_trigger_set_script_use_by_refname("v21");
        }
        level_trigger_set_script_use_by_refname("v31");
    }
    if (savegame_state.get_flag(cur_level_index, "lenele1d_mq_talked_to_captain"))
        script_npc_set_disabled("Muthavi Captain", true);
}

// 0x4fa970  Confidence: exact. level_script_table frame: Nar-Act2 once, on the lenele1d_v2 visit.
void ls_lenele1d_frame()
{
    if (!savegame_state.get_flag(cur_level_index, "act2_intro") && !_stricmp(level_script_name, "lenele1d_v2")) {
        savegame_state.set_flag(cur_level_index, "act2_intro", true);
        script_play_cutscene("Nar-Act2", NULL);
    }
}

// 0x4fa9d0  Confidence: exact. level_script_table use: Crazy Iven's trout gives the Fiery Falchion.
char ls_lenele1d_use(const char *name)
{
    if (!_stricmp(name, "FISH-trout02") && savegame_state.get_flag(cur_level_index, "iven_quest")
        && !savegame_state.get_flag(cur_level_index, "got_iven")) {
        savegame_state.set_flag(cur_level_index, "got_iven", true);
        quest_journal_set_entry(quest_find_by_name("Crazy Iven"), 0);
        script_party_acquire_item("Fiery Falchion", 1);
    }
    return 0;
}

// ---- lenele1c ------------------------------------------------------------------------------------------

// 0x4faa60  Confidence: exact. level_script_table init. The exits v11 / v21 / v31 as in lenele1d; the gate
// to lenele1e by stage (from 4 lenele1e_v2, at 2-3 lenele1e, the four palace guards gone from stage 2;
// before, both closed).
void ls_lenele1c_init()
{
    level_music = music_find_by_name("lenele marketplace");
    game_stage_set(0);
    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_exit_by_refname("v31");
    } else {
        if (game_stage_get() >= 4) {
            level_trigger_set_script_use_by_refname("v11");
            level_trigger_set_exit_by_refname("v21");
        } else {
            level_trigger_set_exit_by_refname("v11");
            level_trigger_set_script_use_by_refname("v21");
        }
        level_trigger_set_script_use_by_refname("v31");
    }
    if (game_stage_get() >= 2) {
        if (game_stage_get() >= 4) {
            level_trigger_set_script_use("lenele1e", "lenele1e");
            level_trigger_set_exit("lenele1e", "lenele1e_v2");
        } else {
            level_trigger_set_exit("lenele1e", "lenele1e");
            level_trigger_set_script_use("lenele1e", "lenele1e_v2");
        }
        script_npc_set_disabled("Palace Guard#1", true);
        script_npc_set_disabled("Palace Guard#2", true);
        script_npc_set_disabled("Palace Guard#3", true);
        script_npc_set_disabled("Palace Guard#4", true);
        return;
    }
    level_trigger_set_script_use("lenele1e", "lenele1e");
    level_trigger_set_script_use("lenele1e", "lenele1e_v2");
}

// 0x4fab80  Confidence: exact. level_script_table frame: "jmp 0x4fab90".
void ls_lenele1c_frame()
{
    lsf_lenele1c_flag_found_flece();
}

// 0x4fab90  Confidence: likely (not in sum.symbols; name ours, after its flag; the order of the
// flece_joined store and the cutscene call is the compiler's). At $fleceintro (6), outside a
// conversation: Flece joins at level 4 (the result is used unchecked), gets a Cloth Shirt (what it replaces
// goes to the party inventory), Flece-Intro plays and the four gate guards go.
void lsf_lenele1c_flag_found_flece()
{
    item_info *old1;
    item_info *old2;
    Character *flece;

    if (!savegame_state.get_flag(cur_level_index, "lenele1c_flag_found_flece") && !(game_flags & GF_CONVERSATION)
        && script_player_near_navpoint("$fleceintro", 6.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele1c_flag_found_flece", true);
        flece = party_member_join("Flece", 4, false, true, -1);
        character_equip_item(&old1, &old2, flece, 7, item_create_by_name("Cloth Shirt"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        if (old2)
            party_inventory_add_item(old2, 1, 1);
        flece_joined = true;
        script_play_cutscene("Flece-Intro", ls_lenele1c_flece_intro_cs_end);
        script_npc_set_disabled("Palace Guard#$npc041", true);
        script_npc_set_disabled("Palace Guard#$npc042", true);
        script_npc_set_disabled("Palace Guard#$npc043", true);
        script_npc_set_disabled("Palace Guard#$npc044", true);
    }
}

// 0x4faca0  Confidence: exact. End of Flece-Intro.
void ls_lenele1c_flece_intro_cs_end()
{
    script_play_cutscene("Flece-Intro-p2", ls_lenele1c_flece_intro_p2_cs_end);
}

// 0x4facc0  Confidence: exact (the three strcpy are inline rep movs). End of Flece-Intro-p2: on to the
// sewer, entry point 1 (game sequence state 4).
void ls_lenele1c_flece_intro_p2_cs_end()
{
    strcpy(level_prev_filename, level_filename);
    strcpy(level_filename, "sewer");
    strcpy(level_script_name, "sewer");
    level_entry_point = 1;
    gameseq_set_state(4);
}

// 0x4fad50  Confidence: exact. level_script_table use.
char ls_lenele1c_use(const char *name)
{
    if (!_stricmp(name, "lenele1e") && game_stage_get() < 4)
        script_party_member_say("Joseph", XSTR("These guards won't let me pass.", 481), 5.0f);
    return 0;
}
