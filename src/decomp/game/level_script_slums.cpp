// level_script_slums.cpp - Sum.exe v1.40, code 0x4fad90-0x4fb4c5 (levelscripts\Scripts\
// level_script_slums.cpp; strings.tbl id 557 is this unit's, 472 is shared). Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function:
// exact (control flow, calls, constants and globals match), likely (one detail of statement order or
// expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele1b" (the Lenele slums and docks; lenele1b_v2 stage 4, lenele1b_v3
// stage 6): the sewer grate, the slum quests' conversation checks (the beggar's eyes, the apothecary's
// tongue, Gorbus and the forged release order, Dagis the forger, Varyssa's shards, Pedrog's salt, the
// weeping butcher's doll, the bacite tails). No static initialiser, no .bss, no .data of its own.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdio.h>
#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_SEWER        9                         // "sewer_flag_found_crate" (0x4fb326)

void lsf_lenele_slums_flag_prost1_speaks();
void lsf_lenele_slums_flag_prost2_speaks();
void lsf_lenele_slums_flag_ken_speaks();

// 0x4fad90  Confidence: exact. level_script_table init. Coming up from the sewer opens the grate for good;
// the exits v11-v33 by stage as in the other Lenele quarters; from stage 1 Starvus the Skulk offers
// Tancred's key while the party does not hold it.
void ls_lenele1b_init()
{
    level_music = music_find_by_name("lenele slumdock");
    if (!_stricmp(level_script_name, "lenele1b_v3"))
        game_stage_set(6);
    else if (!_stricmp(level_script_name, "lenele1b_v2"))
        game_stage_set(4);
    else
        game_stage_set(0);
    if (!_stricmp(level_prev_filename, "sewer"))
        savegame_state.set_flag(cur_level_index, "sewer_grate_open", true);
    if (savegame_state.get_flag(cur_level_index, "sewer_grate_open")) {
        door_apply_saved_state("SewerGrateBarHor17", true);
        level_trigger_set_exit_by_refname("sewer");
    } else {
        level_trigger_set_script_use_by_refname("sewer");
    }
    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_exit_by_refname("v31");
        level_trigger_set_exit_by_refname("v32");
        level_trigger_set_exit_by_refname("v33");
    } else {
        if (game_stage_get() >= 4) {
            level_trigger_set_script_use_by_refname("v11");
            level_trigger_set_script_use_by_refname("v12");
            level_trigger_set_script_use_by_refname("v13");
            level_trigger_set_exit_by_refname("v21");
            level_trigger_set_exit_by_refname("v22");
            level_trigger_set_exit_by_refname("v23");
        } else {
            level_trigger_set_exit_by_refname("v11");
            level_trigger_set_exit_by_refname("v12");
            level_trigger_set_exit_by_refname("v13");
            level_trigger_set_script_use_by_refname("v21");
            level_trigger_set_script_use_by_refname("v22");
            level_trigger_set_script_use_by_refname("v23");
        }
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
    }
    if (game_stage_get() >= 1 && !party_count_item_by_name("Tancred's Key"))
        conversation_db.set_topic_available("Starvus the Skulk", "\"Give me the key to Tancred's house.\"", true);
}

// 0x4faf90  Confidence: exact (ends in a tail jump to lsf_lenele_slums_flag_ken_speaks).
void ls_lenele1b_frame()
{
    lsf_lenele_slums_flag_prost1_speaks();
    lsf_lenele_slums_flag_prost2_speaks();
    lsf_lenele_slums_flag_ken_speaks();
}

// 0x4fafa0  Confidence: exact. Within 3 of the player: label 2 (her greeting).
void lsf_lenele_slums_flag_prost1_speaks()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele_slums_flag_prost1_speaks")
        && script_npc_near_player("Prostitute#1", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele_slums_flag_prost1_speaks", true);
        script_npc_goto_label("Prostitute#1", 2);
    }
}

// 0x4fb000  Confidence: exact.
void lsf_lenele_slums_flag_prost2_speaks()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele_slums_flag_prost2_speaks")
        && script_npc_near_player("Prostitute#2", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele_slums_flag_prost2_speaks", true);
        script_npc_goto_label("Prostitute#2", 2);
    }
}

// 0x4fb060  Confidence: exact.
void lsf_lenele_slums_flag_ken_speaks()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele_slums_flag_ken_speaks")
        && script_npc_near_player("Ken", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele_slums_flag_ken_speaks", true);
        script_npc_goto_label("Ken", 2);
    }
}

// 0x4fb0c0  Confidence: exact. level_script_table use: the grate bar toggles the sewer exit (returns 1).
char ls_lenele1b_use(const char *name)
{
    if (!_stricmp(name, "SewerGrateBarHor17")) {
        if (savegame_state.get_flag(cur_level_index, "sewer_grate_open")) {
            savegame_state.set_flag(cur_level_index, "sewer_grate_open", false);
            level_trigger_set_script_use_by_refname("sewer");
            door_apply_saved_state("SewerGrateBarHor17", false);
            return 1;
        }
        savegame_state.set_flag(cur_level_index, "sewer_grate_open", true);
        level_trigger_set_exit_by_refname("sewer");
        door_apply_saved_state("SewerGrateBarHor17", true);
        return 1;
    }
    return 0;
}

// 0x4fb160  Confidence: exact. level_script_table action: the conversation checks. Note the else of
// CheckReleaseOrder: every other action re-toggles Gorbus's release order topic with the plain (not _inv)
// test, the opposite polarity (0x4fb227, see NOTES). TradeBacite: the whole stack of Bacite Tails for 50
// gold each.
void ls_lenele1b_action(dialogue_action *action)
{
    char line[0x50];
    int type, slot, gold;

    if (!_stricmp(action->name, "CheckEyes")) {
        script_topic_toggle_by_item("Bottle with Eyes", "A Beggar With No Eyes", "\"I'm working on it.\"");
        script_topic_toggle_by_item_inv("Bottle with Eyes", "A Beggar With No Eyes", "Give him the eyes.");
    }
    if (!_stricmp(action->name, "CheckTongue")) {
        script_topic_toggle_by_item("Salamanka Tongue", "Apothecary", "\"No. Not yet.\"");
        script_topic_toggle_by_item_inv("Salamanka Tongue", "Apothecary", "Give him the salamanka's tongue.");
    }
    if (!_stricmp(action->name, "CheckReleaseOrder")) {
        script_topic_toggle_by_item("Forged Document", "Gorbus the Guard", "\"The Keep?\"");
        script_topic_toggle_by_item_inv("Forged Document", "Gorbus the Guard", "Give him the release order.");
    } else {
        script_topic_toggle_by_item("Forged Document", "Gorbus the Guard", "Give him the release order.");
    }
    if (!_stricmp(action->name, "CheckGold"))
        conversation_db.set_topic_available("Gorbus the Guard", "Give him 20 gold.", party_gold >= 20);
    if (!_stricmp(action->name, "CheckGold2"))
        conversation_db.set_topic_available("Dagis the Forger", "Pay 25 gold.", party_gold >= 25);
    if (!_stricmp(action->name, "CheckShards"))
        conversation_db.set_topic_available("Varyssa", "Give her the shards.",
                                            party_count_item_by_name("Blue Glass Shard")
                                            && party_count_item_by_name("Green Glass Shard")
                                            && party_count_item_by_name("Red Glass Shard"));
    if (!_stricmp(action->name, "CheckCrate")
        && (savegame_state.get_flag(LEVEL_SEWER, "sewer_flag_found_crate") || party_count_item_by_name("Chunk of Salt"))) {
        conversation_db.set_topic_available("Pedrog", "Tell him what you found in the sewers.", true);
        conversation_db.set_topic_available("Pedrog", "\"Not yet.\"", false);
    }
    if (!_stricmp(action->name, "CheckSalt")) {
        script_topic_toggle_by_item("Chunk of Salt", "Pedrog", "\"No.\"");
        script_topic_toggle_by_item_inv("Chunk of Salt", "Pedrog", "Give him the salt.");
    }
    if (!_stricmp(action->name, "CheckDoll"))
        script_topic_toggle_by_item_inv("Rag Doll", "Weeping Butcher", "Give him the doll.");
    if (!_stricmp(action->name, "SalamankaActive"))
        savegame_state.set_flag(cur_level_index, "lenele_slums_got_salamanka_quest", true);
    if (!_stricmp(action->name, "TradeBacite")) {
        type = item_type_find("Bacite Tail");
        if (type >= 0) {
            slot = party_inventory_find_type(&item_types[type]);
            if (slot >= 0) {
                gold = party_inventory[slot].count * 50;
                party_inventory[slot].count = 0;
                party_inventory_remove(slot, 1, 1);
                sprintf(line, XSTR("<lost items: \"Bacite Tails\">", 557));
                dialogue_show_line(line, 1, NULL);
                sprintf(line, XSTR("<party received %d gold>", 472), gold);
                dialogue_show_line(line, 1, NULL);
                party_gold += gold;
            }
        }
    }
}
