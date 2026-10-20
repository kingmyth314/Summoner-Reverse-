// level_script_jadetemple.cpp - Sum.exe v1.40, code 0x4effb0-0x4f0d70 (levelscripts\Scripts\
// level_script_jadetemple.cpp; strings.tbl ids 475, 490 and 491 are used here). Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function:
// exact (control flow, calls, constants and globals match), likely (one detail of statement order or
// expression form is a guess), unsure (the shape itself is a guess).
//
// The Jade Temple. "jadetemple": the Tiger Rider and the Ghost Rider (Tiger-Ghost-Intro at $tgtrigger),
// then Murod and Murod Oni (Murod-Intro / -Trans / -Death; the Ring of Jade), then the trip to the
// Khosani stronghold; Gongji's dragon scales. "jadetemple_v2": the four riders again, solo, for the Spirit
// of Laharah, then the fade to the world map. Static initialiser table entry for the two timers: the
// thunk 0x4effb0 (the initialiser at 0x4f0d80, sum.symbols' static_init_ls_jadetemple_timer_b, jumps to
// init_ls_lenele3d_timer and belongs to the next unit).
// Data: .bss 0x257e9bc-0x257e9cc.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdio.h>
#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/boss_ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

void lsf_tg_intro();
void ls_jadetemple_tiger_ghost_intro_cs_end();
void lsf_tiger2_dead();
void lsf_murod_intro();
void ls_jadetemple_murod_intro_cs_end();
void ls_jadetemple_murod_trans_cs_end();
void ls_jadetemple_murod_death_cs_end();
void ls_jadetemple_finish();

// ---- .bss (0x257e9bc-0x257e9cc) ------------------------------------------------------------------------
bool ls_jadetemple_murod_intro_done;                 // 0x257e9bc
bool ls_jadetemple_tiger_dead_done;                  // 0x257e9bd
bool ls_jadetemple_murod_dead_done;                  // 0x257e9c4
bool ls_jadetemple_tg_intro_done;                    // 0x257e9c5
bool ls_jadetemple_place_pending;                    // 0x257e9cc
// 0x4effb0  Confidence: likely. static_init_ls_jadetemple_timers: "jmp 0x4effc0".
// 0x4effc0  Confidence: likely (one initialiser constructs both timers, fade timer first: written as one
// declaration with two declarators). init_ls_jadetemple_timers.
GameTimer ls_jadetemple_fade_timer, ls_jadetemple_tiger2_timer;   // 0x257e9c8, 0x257e9c0

// 0x4effe0  Confidence: exact (the two preset pairs are written as if / else with constant stores; the
// code stores 1 or a zero register). level_script_table init.
void ls_jadetemple_init()
{
    level_music = music_find_by_name("jadetemple");
    if (!_stricmp(level_script_name, "jadetemple_v2")) {
        tiger_set_solo(script_object_find_character("Tiger Rider"));
        ghost_set_solo(script_object_find_character("Ghost Rider"));
        serpent_set_solo(script_object_find_character("Serpent Rider"));
        phoenix_set_solo(script_object_find_character("Phoenix Rider"));
        ls_jadetemple_fade_timer.invalidate();
        ls_jadetemple_tiger2_timer.invalidate();
        ls_jadetemple_place_pending = false;
        return;
    }
    game_stage_set(0xe);
    if (savegame_state.get_flag(cur_level_index, "murod_dead")) {
        ls_jadetemple_murod_intro_done = true;
        ls_jadetemple_murod_dead_done = true;
    } else {
        ls_jadetemple_murod_intro_done = false;
        ls_jadetemple_murod_dead_done = false;
    }
    if (savegame_state.get_flag(cur_level_index, "ghost_dead")
        && savegame_state.get_flag(cur_level_index, "tiger_dead")) {
        ls_jadetemple_place_pending = false;
        ls_jadetemple_tg_intro_done = true;
        ls_jadetemple_tiger_dead_done = true;
    } else {
        ls_jadetemple_tg_intro_done = false;
        ls_jadetemple_tiger_dead_done = false;
        ls_jadetemple_place_pending = false;
    }
}

// 0x4f0110  Confidence: exact. level_script_table frame. EndCutscene (ls_jadetemple_action) moves the party
// to the stronghold once no conversation runs.
void ls_jadetemple_frame()
{
    if (!_stricmp(level_script_name, "jadetemple_v2")) {
        lsf_tiger2_dead();
    } else {
        lsf_tg_intro();
        lsf_murod_intro();
    }
    if (ls_jadetemple_place_pending && !(game_flags & GF_CONVERSATION)) {
        ls_jadetemple_finish();
        ls_jadetemple_place_pending = false;
    }
}

// 0x4f0160  Confidence: exact (VC6 byte match, match14; was exact). The Tiger Rider and the Ghost Rider: boss music after the intro, the level
// music and FrontDoor once both are dead; Tiger-Ghost-Intro at $tgtrigger (24).
void lsf_tg_intro()
{
    if (!ls_jadetemple_tg_intro_done && savegame_state.get_flag(cur_level_index, "tg_intro")
        && !(game_flags & GF_CUTSCENE)) {
        ls_jadetemple_tg_intro_done = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!ls_jadetemple_tiger_dead_done && savegame_state.get_flag(cur_level_index, "tiger_dead")
        && savegame_state.get_flag(cur_level_index, "ghost_dead") && !(game_flags & GF_CUTSCENE)) {
        door_apply_saved_state("FrontDoor", true);
        ls_jadetemple_tiger_dead_done = true;
        music_play(level_music, true);
        level_script_event_active = false;
    }
    if (!savegame_state.get_flag(cur_level_index, "tg_intro")) {
        if (script_player_near_navpoint("$tgtrigger", 24.0f)) {
            script_play_cutscene("Tiger-Ghost-Intro", ls_jadetemple_tiger_ghost_intro_cs_end);
            savegame_state.set_flag(cur_level_index, "tg_intro", true);
        }
        return;
    }
    if (!savegame_state.get_flag(cur_level_index, "tiger_dead") && script_npc_is_dead("Tiger Rider")) {
        script_npc_goto_label("Tiger Rider", 2);
        savegame_state.set_flag(cur_level_index, "tiger_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ghost_dead") && script_npc_is_dead("Ghost Rider")) {
        script_npc_goto_label("Ghost Rider", 2);
        savegame_state.set_flag(cur_level_index, "ghost_dead", true);
    }
}

// 0x4f0320  Confidence: exact. End of Tiger-Ghost-Intro.
void ls_jadetemple_tiger_ghost_intro_cs_end()
{
    script_npc_send_go("Tiger Rider");
    script_npc_send_go("Ghost Rider");
    tiger_set_solo(script_object_find_character("Tiger Rider"));
    ghost_set_solo(script_object_find_character("Ghost Rider"));
    level_script_event_active = true;
}

// 0x4f0360  Confidence: exact (VC6 byte match, match14; was likely) (the else-if chain after got_laharah_spirit; each branch ends in a ret).
// jadetemple_v2: the four riders; with all four dead the Spirit of Laharah, then 8 s, a 5 s fade, and the
// world map (entry 5, game sequence state 4). The inventory screen (4) is closed during the fade.
void lsf_tiger2_dead()
{
    if (!savegame_state.get_flag(cur_level_index, "tiger2_dead") && script_npc_is_dead("Tiger Rider")) {
        script_npc_goto_label("Tiger Rider", 2);
        savegame_state.set_flag(cur_level_index, "tiger2_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ghost2_dead") && script_npc_is_dead("Ghost Rider")) {
        ghost_minions_deactivate();
        script_npc_goto_label("Ghost Rider", 2);
        savegame_state.set_flag(cur_level_index, "ghost2_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "serpent2_dead") && script_npc_is_dead("Serpent Rider")) {
        script_npc_goto_label("Serpent Rider", 2);
        savegame_state.set_flag(cur_level_index, "serpent2_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "phoenix2_dead") && script_npc_is_dead("Phoenix Rider")) {
        script_npc_goto_label("Phoenix Rider", 2);
        savegame_state.set_flag(cur_level_index, "phoenix2_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "got_laharah_spirit")) {
        if (savegame_state.get_flag(cur_level_index, "ghost2_dead")
            && savegame_state.get_flag(cur_level_index, "tiger2_dead")
            && savegame_state.get_flag(cur_level_index, "phoenix2_dead")
            && savegame_state.get_flag(cur_level_index, "serpent2_dead")) {
            savegame_state.set_flag(cur_level_index, "got_laharah_spirit", true);
            hud_show_message(XSTR("Joseph acquired Summon Spirit of Laharah!", 490));
            ls_jadetemple_tiger2_timer.SetTimeout(8000);
            level_script_event_active = false;
        }
    } else if (ls_jadetemple_tiger2_timer.elapsed()) {
        ls_jadetemple_tiger2_timer.invalidate();
        ls_jadetemple_fade_timer.SetTimeout(5000);
        gr_fade_out(5000);
    } else if (ls_jadetemple_fade_timer.valid()) {
        if (interface_get_screen() == 4)
            interface_set_screen(0, 0);
        if (ls_jadetemple_fade_timer.elapsed()) {
            gr_fade_in(2000);
            strcpy(level_prev_filename, level_filename);
            strcpy(level_filename, "worldmap1");
            strcpy(level_script_name, "worldmap1");
            level_entry_point = 5;
            gameseq_set_state(4);
        }
    }
}

// 0x4f0680  Confidence: exact (VC6 byte match, match14; was exact). Murod: boss music after Murod-Intro, the level music after the death; the
// three cutscenes ($murodtrigger within 8, Murod dead -> Murod-Trans, Murod Oni dead -> Murod-Death).
void lsf_murod_intro()
{
    if (!ls_jadetemple_murod_intro_done && savegame_state.get_flag(cur_level_index, "murod_intro")
        && !(game_flags & GF_CUTSCENE)) {
        ls_jadetemple_murod_intro_done = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!ls_jadetemple_murod_dead_done && savegame_state.get_flag(cur_level_index, "murod_dead")
        && !(game_flags & GF_CUTSCENE)) {
        ls_jadetemple_murod_dead_done = true;
        music_play(level_music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "murod_intro")) {
        if (script_player_near_navpoint("$murodtrigger", 8.0f)) {
            savegame_state.set_flag(cur_level_index, "murod_intro", true);
            script_play_cutscene("Murod-Intro", ls_jadetemple_murod_intro_cs_end);
        }
        return;
    }
    if (!savegame_state.get_flag(cur_level_index, "murod_xform")) {
        if (script_npc_is_dead("Murod")) {
            script_npc_drop_loot("Murod");
            savegame_state.set_flag(cur_level_index, "murod_xform", true);
            script_play_cutscene("Murod-Trans", ls_jadetemple_murod_trans_cs_end);
        }
        return;
    }
    if (!savegame_state.get_flag(cur_level_index, "murod_dead") && script_npc_is_dead("Murod Oni")) {
        script_npc_drop_loot("Murod Oni");
        savegame_state.set_flag(cur_level_index, "murod_dead", true);
        script_play_cutscene("Murod-Death", ls_jadetemple_murod_death_cs_end);
    }
}

// 0x4f0840  Confidence: exact. End of Murod-Intro.
void ls_jadetemple_murod_intro_cs_end()
{
    script_npc_send_go("Murod");
    level_script_event_active = true;
}

// 0x4f0860  Confidence: exact. End of Murod-Trans: Murod becomes Murod Oni.
void ls_jadetemple_murod_trans_cs_end()
{
    script_npc_set_disabled("Murod", true);
    script_npc_goto_label("Murod Oni", 1);
}

// 0x4f0880  Confidence: exact. End of Murod-Death: the Ring of Jade; the companions leave the party
// (ls_jadetemple_finish adds them back) and every NPC of the temple takes label 1.
void ls_jadetemple_murod_death_cs_end()
{
    script_npc_set_disabled("Murod Oni", true);
    script_party_acquire_item("Ring of Jade", 1);
    savegame_state.set_flag(cur_level_index, "got_ring_of_jade", true);
    level_script_event_active = false;
    party_remove_member("Jekhar");
    party_remove_member("Flece");
    party_remove_member("Rosalind");
    hud_show_message(XSTR("Joseph acquired Summon Jade Golem!", 491));
    script_npc_goto_label("Qifeng", 1);
    script_npc_goto_label("Jekhar#npc", 1);
    script_npc_goto_label("Flece#npc", 1);
    script_npc_goto_label("Rosalind#npc", 1);
    script_npc_goto_label("Jade Temple Priest#1", 1);
    script_npc_goto_label("Gongji", 1);
    script_npc_goto_label("Jade Temple Priest#3", 1);
    script_npc_goto_label("Jade Temple Soldier#1", 1);
    script_npc_goto_label("Jade Temple Soldier#2", 1);
    script_npc_goto_label("Jade Temple Soldier#3", 1);
    script_npc_goto_label("Jade Temple Soldier#4", 1);
    script_npc_goto_label("Jade Temple Soldier#5", 1);
    script_npc_goto_label("Jade Temple Soldier#6", 1);
}

// 0x4f0990  Confidence: exact (VC6 byte match, match14; was exact). After the EndCutscene action: the companions rejoin, and the level changes
// to khosanistrng (script khosanistrng_v3, entry 1; game sequence state 4).
void ls_jadetemple_finish()
{
    party_add_member("Jekhar");
    party_add_member("Rosalind");
    party_add_member("Flece");
    strcpy(level_prev_filename, level_filename);
    strcpy(level_filename, "khosanistrng");
    strcpy(level_script_name, "khosanistrng_v3");
    level_entry_point = 1;
    gameseq_set_state(4);
}

// 0x4f0a40  Confidence: likely (the four "lost item" blocks are written out: they are expanded in place,
// with the item pointer formed before the index is checked). level_script_table action. CheckScales counts
// the four dragon scales the party holds and opens Gongji's matching topics; LoseScales takes one of each.
void ls_jadetemple_action(dialogue_action *action)
{
    char line[256];
    char item_name[256];
    int scales, type, slot;
    ItemType *t;

    if (!_stricmp(action->name, "EndCutscene"))
        ls_jadetemple_place_pending = true;
    if (!_stricmp(action->name, "CheckScales")) {
        scales = 0;
        if (party_count_item_by_name("Green Dragon Scale"))
            scales = 1;
        if (party_count_item_by_name("Red Dragon Scale"))
            scales++;
        if (party_count_item_by_name("Gray Dragon Scale"))
            scales++;
        if (party_count_item_by_name("Blue Dragon Scale"))
            scales++;
        if (scales > 0) {
            conversation_db.set_topic_available("Gongji", "\"I found what you are seeking.\"", true);
            conversation_db.set_topic_available("Gongji", "Keep the dragon scales", true);
        }
        if (scales == 1)
            conversation_db.set_topic_available("Gongji", "Give him the dragon scale.#1", true);
        else if (scales == 2)
            conversation_db.set_topic_available("Gongji", "Give him both dragon scales.#2", true);
        else if (scales == 3)
            conversation_db.set_topic_available("Gongji", "Give him the three dragon scales.#3", true);
        else if (scales == 4)
            conversation_db.set_topic_available("Gongji", "Give him the four dragon scales.#4", true);
    }
    if (!_stricmp(action->name, "LoseScales")) {
        type = item_type_find("Red Dragon Scale");
        slot = party_inventory_find_type(&item_types[type]);
        if (slot >= 0 && type >= 0) {
            if (party_inventory[slot].count > 1)
                party_inventory[slot].count--;
            else
                party_inventory_remove(slot, 1, 1);
            item_get_display_name(item_name, &item_types[type]);
            sprintf(line, XSTR("<lost item: \"%s\">", 475), item_name);
            dialogue_show_line(line, 1, NULL);
        }
        type = item_type_find("Green Dragon Scale");
        slot = party_inventory_find_type(&item_types[type]);
        if (slot >= 0 && type >= 0) {
            if (party_inventory[slot].count > 1)
                party_inventory[slot].count--;
            else
                party_inventory_remove(slot, 1, 1);
            item_get_display_name(item_name, &item_types[type]);
            sprintf(line, XSTR("<lost item: \"%s\">", 475), item_name);
            dialogue_show_line(line, 1, NULL);
        }
        type = item_type_find("Blue Dragon Scale");
        slot = party_inventory_find_type(&item_types[type]);
        if (slot >= 0 && type >= 0) {
            if (party_inventory[slot].count > 1)
                party_inventory[slot].count--;
            else
                party_inventory_remove(slot, 1, 1);
            item_get_display_name(item_name, &item_types[type]);
            sprintf(line, XSTR("<lost item: \"%s\">", 475), item_name);
            dialogue_show_line(line, 1, NULL);
        }
        type = item_type_find("Gray Dragon Scale");
        slot = party_inventory_find_type(&item_types[type]);
        if (slot >= 0 && type >= 0) {
            if (party_inventory[slot].count > 1)
                party_inventory[slot].count--;
            else
                party_inventory_remove(slot, 1, 1);
            item_get_display_name(item_name, &item_types[type]);
            sprintf(line, XSTR("<lost item: \"%s\">", 475), item_name);
            dialogue_show_line(line, 1, NULL);
        }
    }
}
