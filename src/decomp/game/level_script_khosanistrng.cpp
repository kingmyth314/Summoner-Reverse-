// level_script_khosanistrng.cpp - Sum.exe v1.40, code 0x4f3a80-0x4f454c. Written from the Sum.exe disassembly
// (tools/asm.mjs; the floats 4.0 / 7.0 / 2.0 / 5.0 / 3.0 are push immediates). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (one detail of statement order or expression form is a guess), unsure (the shape itself is
// a guess).
//
// One level_script_table record: "KhosaniStrng" (index 0x17; the Khosani stronghold: the Guardian of the
// Stone Portal, the two flip-labyrinth doors, Ulsadana, the act 4 errands). FILE NAME NOT PROVEN (D5300): no
// strings.tbl block, no path string, no .bss; its literals (0x5a2bd0-0x5a2fd8) lie between
// level_script_lpalaceint.cpp's and level_script_iona_ext2.cpp's (source-map.md). The name follows the
// level_script_<record>.cpp convention. "khosanistrng_v3", "got_ring_of_fire", "found_khosani", "Ulsadana"
// and "KhosaniStrng" are pooled copies.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdio.h>
#include <string.h>
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "summoner/gameplay.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_TEST           2                       // "extra_6" .. "extra_8" (0x4f3e69): spare flags
#define LEVEL_WORLDMAP1      4                       // "found_khosani" (0x4f3dff)
#define LEVEL_KHOSANILAB     0xe                     // "got_ring_of_fire" (0x4f3cd5)
#define LEVEL_RAND_FOREST01  0x12                    // "crone_mq_gave_rose" (0x4f3e10)

void lsf_khosani_strng_talked_to_portal();

// 0x4f3a80  Confidence: likely (one stack cleanup before the v4 printf is scheduled differently by VC6, D5612;
// the v2 branch ends in a jump into the v5 branch's identical tail, 0x4f3d10
// jmp 0x4f3b44: tail merging; each branch's own calls are written out). level_script_table init. The script
// version sets the stage (v6 20, v5 18, v4 17, v3 15, v2 6, else 5), the two flip-labyrinth doors, the
// portal / Ulsadana flags and which lab exit is a script use; then the stronghold is marked found and the
// act 4 extras that were already dealt with are hidden. "enabled kl1/kl2" are debug printfs.
void ls_khosanistrng_init()
{
    level_music = music_find_by_name("KhosaniStrng");
    if (!_stricmp(level_script_name, "khosanistrng_v6")) {
        game_stage_set(0x14);
        door_apply_saved_state("door-fliplaby02", true);
        door_apply_saved_state("door-fliplaby01", false);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", false);
        level_trigger_set_script_use("khosanilab", 0);
        level_trigger_set_script_use("khosanilab2", 0);
    } else if (!_stricmp(level_script_name, "khosanistrng_v5")) {
        game_stage_set(0x12);
        savegame_state.set_flag(cur_level_index, "khosani_strng_talked_to_portal", true);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", true);
        door_apply_saved_state("door-fliplaby02", false);
        door_apply_saved_state("door-fliplaby01", false);
        level_trigger_set_script_use("khosanilab", 0);
        level_trigger_set_script_use("khosanilab2", 0);
    } else if (!_stricmp(level_script_name, "khosanistrng_v4")) {
        game_stage_set(0x11);
        savegame_state.set_flag(cur_level_index, "khosani_strng_talked_to_portal", true);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", true);
        door_apply_saved_state("door-fliplaby02", false);
        door_apply_saved_state("door-fliplaby01", true);
        printf("enabled kl2\n");
        level_trigger_set_script_use("khosanilab", 0);
        level_trigger_set_exit("khosanilab2", 0);
    } else if (!_stricmp(level_script_name, "khosanistrng_v3")) {
        if (!savegame_state.get_flag(cur_level_index, "khosanistrng_st_Ulsadana_must_speak"))
            savegame_state.set_flag(cur_level_index, "khosani_strng_talked_to_portal", false);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", false);
        game_stage_set(0xf);
        door_apply_saved_state("door-fliplaby02", true);
        door_apply_saved_state("door-fliplaby01", false);
        level_trigger_set_script_use("khosanilab", 0);
        level_trigger_set_script_use("khosanilab2", 0);
    } else if (!_stricmp(level_script_name, "khosanistrng_v2")) {
        savegame_state.set_flag(LEVEL_KHOSANILAB, "got_ring_of_fire", true);
        game_stage_set(6);
        savegame_state.set_flag(cur_level_index, "khosani_strng_talked_to_portal", true);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", true);
        door_apply_saved_state("door-fliplaby02", false);
        door_apply_saved_state("door-fliplaby01", false);
        level_trigger_set_script_use("khosanilab", 0);
        level_trigger_set_script_use("khosanilab2", 0);
    } else {
        game_stage_set(5);
        if (!savegame_state.get_flag(cur_level_index, "khosani_strng_gate1")
            && savegame_state.get_flag(cur_level_index, "khosani_strng_guardian1")) {
            door_apply_saved_state("door-fliplaby02", true);
            savegame_state.set_flag(cur_level_index, "khosani_strng_gate1", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "khosani_strng_gate2")
            && savegame_state.get_flag(cur_level_index, "khosani_strng_guardian2")) {
            door_apply_saved_state("door-fliplaby01", true);
            savegame_state.set_flag(cur_level_index, "khosani_strng_gate2", true);
        }
        printf("enabled kl1\n");
        level_trigger_set_exit("khosanilab", 0);
        level_trigger_set_script_use("khosanilab2", 0);
    }

    savegame_state.set_flag(LEVEL_WORLDMAP1, "found_khosani", true);
    if (savegame_state.get_flag(LEVEL_RAND_FOREST01, "crone_mq_gave_rose")) {
        script_npc_set_disabled("Salamanka#1", true);
        script_npc_set_disabled("Salamanka#2", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "buy_seeds"))
        script_npc_set_disabled("Elarh the Merchant#act4", true);
    if (savegame_state.get_flag(LEVEL_TEST, "extra_6"))
        script_npc_set_disabled("Saato the Priestess", true);
    if (savegame_state.get_flag(LEVEL_TEST, "extra_7"))
        script_npc_set_disabled("Athodi the Priestess", true);
    if (savegame_state.get_flag(LEVEL_TEST, "extra_8"))
        script_npc_set_disabled("Elarh the Merchant#act4", true);
}

// 0x4f3ee0  Confidence: exact (VC6 byte match; the two set_flag calls of gate2 and act4_intro share one
// call, 0x4f40f4
// jmp 0x4f4146: tail merging). level_script_table frame. Before the Ring of Fire: the narration once, the
// Guardian's two talks (the second after Ulsadana, within 7), and each flip door opens when the Guardian
// stands at its navpoint ($npc059 / $npc063, within 2) with the player near (5). After it: the portal talk
// and Act4-Intro (v3 only). Ulsadana's second talk at $npc064 (3) in both cases.
void ls_khosanistrng_frame()
{
    if (!savegame_state.get_flag(LEVEL_KHOSANILAB, "got_ring_of_fire")) {
        if (!savegame_state.get_flag(cur_level_index, "khosani_intro")) {
            savegame_state.set_flag(cur_level_index, "khosani_intro", true);
            script_play_cutscene("Nar-KhosaniStronghold", 0);
        }
        if (conversation_active.is_idle() && !savegame_state.get_flag(cur_level_index, "khosani_strng_guardian1")) {
            script_npc_talk("Guardian of the Stone Portal");
            savegame_state.set_flag(cur_level_index, "khosani_strng_guardian1", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "khosani_strng_guardian2") && conversation_active.is_idle()
            && savegame_state.get_flag(cur_level_index, "khosani_strng_ulsadana")
            && script_npc_near_player("Guardian of the Stone Portal", 7.0f)) {
            script_npc_talk("Guardian of the Stone Portal");
            savegame_state.set_flag(cur_level_index, "khosani_strng_guardian2", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "khosani_strng_gate1")
            && script_check_navpoint(script_object_find("Guardian of the Stone Portal"), "$npc059", 2.0f)
            && script_player_near_navpoint("$npc059", 5.0f)) {
            door_apply_saved_state("door-fliplaby02", true);
            savegame_state.set_flag(cur_level_index, "khosani_strng_gate1", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "khosani_strng_gate2")
            && script_check_navpoint(script_object_find("Guardian of the Stone Portal"), "$npc063", 2.0f)
            && script_player_near_navpoint("$npc063", 5.0f)) {
            door_apply_saved_state("door-fliplaby01", true);
            savegame_state.set_flag(cur_level_index, "khosani_strng_gate2", true);
        }
    } else {
        lsf_khosani_strng_talked_to_portal();
        if (!savegame_state.get_flag(cur_level_index, "act4_intro") && !_stricmp(level_script_name, "khosanistrng_v3")) {
            script_play_cutscene("Act4-Intro", 0);
            savegame_state.set_flag(cur_level_index, "act4_intro", true);
        }
    }
    if (!savegame_state.get_flag(cur_level_index, "khosani_strng_ulsadana2") && conversation_active.is_idle()
        && script_player_near_navpoint("$npc064", 3.0f)) {
        script_npc_talk("Ulsadana");
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana2", true);
    }
}

// 0x4f41c0  Confidence: exact. The Guardian talks once, when no conversation runs.
void lsf_khosani_strng_talked_to_portal()
{
    if (!savegame_state.get_flag(cur_level_index, "khosani_strng_talked_to_portal") && conversation_active.is_idle()) {
        savegame_state.set_flag(cur_level_index, "khosani_strng_talked_to_portal", true);
        script_npc_talk("Guardian of the Stone Portal");
    }
}

// 0x4f4210  Confidence: exact (VC6 byte match; an else-if chain; the shared returns). level_script_table action
// (the kind
// is not checked). Ulsadana: every Khosani master (1-7), merchant (1-4), priestess (1-9) and the Keeper of
// Lore switch from "Hail" to "Hail"#2. CheckItems / CheckMaps offer the hand-in topics only with all three
// items; the Hide* actions set the spare test flags that the init reads.
void ls_khosanistrng_action(dialogue_action *action)
{
    char name[0x30];
    int i;

    if (!_stricmp(action->name, "Ulsadana")) {
        for (i = 1; i <= 7; i++) {
            sprintf(name, "Khosani Master#%d", i);
            conversation_db.set_topic_available(name, "\"Hail\"#2", true);
            conversation_db.set_topic_available(name, "\"Hail\"", false);
        }
        for (i = 1; i <= 4; i++) {
            sprintf(name, "Khosani Merchant#%d", i);
            conversation_db.set_topic_available(name, "\"Hail\"#2", true);
            conversation_db.set_topic_available(name, "\"Hail\"", false);
        }
        for (i = 1; i <= 9; i++) {
            sprintf(name, "Khosani Priestess#%d", i);
            conversation_db.set_topic_available(name, "\"Hail\"#2", true);
            conversation_db.set_topic_available(name, "\"Hail\"", false);
        }
        conversation_db.set_topic_available("Keeper of Lore", "\"Hail\"#2", true);
        conversation_db.set_topic_available("Keeper of Lore", "\"Hail\"", false);
        savegame_state.set_flag(cur_level_index, "khosani_strng_ulsadana", true);
    } else if (!_stricmp(action->name, "LeadPlayer")) {
        script_npc_goto_label("Guardian of the Stone Portal", 1);
    } else if (!_stricmp(action->name, "LeadPlayer2")) {
        script_npc_goto_label("Guardian of the Stone Portal", 2);
    } else if (!_stricmp(action->name, "OpenLab2")) {
        door_apply_saved_state("door-fliplaby01", true);
    } else if (!_stricmp(action->name, "CheckItems")) {
        if (party_count_item_by_name("Huk Kinar's Skull") && party_count_item_by_name("Udakar's Heart")
            && party_count_item_by_name("Bacite's Gallbladder"))
            conversation_db.set_topic_available("Ilaru the Priestess#act4",
                                                "Give her the skull, heart, & gallbladder.", true);
        else
            conversation_db.set_topic_available("Ilaru the Priestess#act4",
                                                "Give her the skull, heart, & gallbladder.", false);
    } else if (!_stricmp(action->name, "CheckMaps")) {
        if (party_count_item_by_name("Umakil Carving") && party_count_item_by_name("Dehilos Carving")
            && party_count_item_by_name("Muhul Carving"))
            conversation_db.set_topic_available("Kiva the Priestess", "Give Kiva the map and carvings.", true);
        else
            conversation_db.set_topic_available("Kiva the Priestess", "Give Kiva the map and carvings.", false);
    } else if (!_stricmp(action->name, "HideSaato")) {
        savegame_state.set_flag(LEVEL_TEST, "extra_6", true);
    } else if (!_stricmp(action->name, "HideAthodi")) {
        savegame_state.set_flag(LEVEL_TEST, "extra_7", true);
    } else if (!_stricmp(action->name, "HideElarh")) {
        savegame_state.set_flag(LEVEL_TEST, "extra_8", true);
    }
}
