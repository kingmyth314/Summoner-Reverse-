// level_script_eleh.cpp - Sum.exe v1.40, code 0x4f1490-0x4f1941 (levelscripts\Scripts\
// level_script_eleh.cpp; strings.tbl id 482 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs; the float 0x57e990 = 1.0 read from the image). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1
// /GX. Confidence per function: exact (control flow, calls, constants and globals match), likely (one detail
// of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "eleh" (the Orenian slave camp, stage 9: Flece alone and stripped of her
// gear, the search for Joseph, the lift between the floors, the portcullis). No static initialiser (the
// one GameTimer is a function-local static, D2405).
// Data: .bss 0x257e9e4-0x257e9f0 (0x257e9e4 the static timer's guard). No .data of its own.
//
// The four choice bytes: sum.symbols names them go_ground / go_bottom / go_other / go_top, but
// ls_eleh_action sets 0x257e9ec for "TopFloor", 0x257e9ed for "GroundFloor", 0x257e9ee for "BottomFloor"
// and 0x257e9ef for "JoinJoseph" (0x4f18a6-0x4f18fa). The symbol names are kept; see NOTES (renames).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_TEST         2                         // "extra_4" (0x4f15f8): a spare flag of the "test" record

void ls_eleh_fade_place_party_frame();

// ---- .bss (0x257e9e4-0x257e9f0) ------------------------------------------------------------------------
// (0x257e9e4 / 0x257e9e8: the guard and the static GameTimer of ls_eleh_fade_place_party_frame.)
bool ls_eleh_go_ground;                              // 0x257e9ec "TopFloor" (entry point 5)
bool ls_eleh_go_bottom;                              // 0x257e9ed "GroundFloor" (entry point 3)
bool ls_eleh_go_other;                               // 0x257e9ee "BottomFloor" (entry point 4)
bool ls_eleh_go_top;                                 // 0x257e9ef "JoinJoseph" (ls_eleh_frame)
int ls_eleh_cutscene_state;                          // 0x257e9f0

// 0x4f1490  Confidence: exact. level_script_table init (stage 9). A fresh entry strips Flece's gear and
// takes Joseph, Jekhar and Rosalind out of the party (the US open demo sets the party to level 15 and
// Flece's dark skill to 80). Until Joseph is found the quest is set, the world map exit is closed and the
// portcullis open.
void ls_eleh_init()
{
    level_music = music_find_by_name("eleh");
    game_stage_set(9);
    if (US_open_demo)
        party_set_level(15);
    if (!level_entered_by_load) {
        script_eleh_strip_party_gear(true);
        party_remove_member("Joseph");
        party_remove_member("Jekhar");
        party_remove_member("Rosalind");
    }
    if (US_open_demo)
        cur_character->skill[SKILL_DARK] = 80;
    if (!savegame_state.get_flag(cur_level_index, "found_joseph")) {
        quest_journal_set_entry(quest_find_by_name("Find Joseph"), 1);
        level_trigger_set_script_use("worldmap1", NULL);
        door_apply_saved_state("Portcullis", true);
        return;
    }
    level_trigger_set_exit("worldmap1", NULL);
}

// 0x4f1560  Confidence: exact. level_script_table frame. Once: Flece gets Springsteel Leather (what it
// replaces goes to the party inventory) and Nar-Eleh-Intro plays. With Joseph found, the portcullis
// drops at $porttrig (20). JoinJoseph: Joseph joins (gear stripped as well), the world map exit opens, the
// slave driver goes and Joseph is placed at $npc094.
void ls_eleh_frame()
{
    item_info *old1;
    item_info *old2;
    Player *p;
    Character *c;

    if (!savegame_state.get_flag(cur_level_index, "eleh_intro")) {
        character_equip_item(&old1, &old2, cur_character, 7, item_create_by_name("Springsteel Leather"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        if (old2)
            party_inventory_add_item(old2, 1, 1);
        script_play_cutscene("Nar-Eleh-Intro", NULL);
        savegame_state.set_flag(cur_level_index, "eleh_intro", true);
    }
    if (!savegame_state.get_flag(LEVEL_TEST, "extra_4") && savegame_state.get_flag(cur_level_index, "found_joseph")
        && script_player_near_navpoint("$porttrig", 20.0f)) {
        savegame_state.set_flag(LEVEL_TEST, "extra_4", true);
        door_apply_saved_state("Portcullis", false);
    }
    ls_eleh_fade_place_party_frame();
    if (ls_eleh_go_top && !(game_flags & GF_CONVERSATION)) {
        script_npc_set_disabled("Joseph#$npc", true);
        p = party_add_member("Joseph");
        c = living_from_handle(p->handle);
        script_eleh_strip_party_gear(true);
        level_trigger_set_exit("worldmap1", NULL);
        script_npc_send_go("Slave Driver#$npc057");
        level_entry_point = 2;
        c->place_at_marker(navpoint_find("$npc094"));
        ls_eleh_go_top = false;
    }
}

// 0x4f16e0  Confidence: exact. The lift: after a floor choice (and no conversation) input off, 0.5 s, a
// 1 s fade out, then at full fade the party is placed at entry point 5 (TopFloor), 3 (GroundFloor) or 4
// (BottomFloor), the camera snaps, 0.5 s, input on and a 1 s fade in.
void ls_eleh_fade_place_party_frame()
{
    if (game_flags & GF_CONVERSATION)
        return;
    static GameTimer timer;                          // 0x257e9e8 ls_eleh_cutscene_timer (guard 0x257e9e4)
    switch (ls_eleh_cutscene_state) {
    case 0:
        if (ls_eleh_go_ground || ls_eleh_go_bottom || ls_eleh_go_other) {
            script_disable_player_input();
            timer.invalidate();
            ls_eleh_cutscene_state = 1;
        }
        break;
    case 1:
        if (!timer.valid())
            timer.SetTimeout(500);
        if (timer.elapsed()) {
            gr_fade_out(1000);
            timer.invalidate();
            ls_eleh_cutscene_state = 2;
        }
        break;
    case 2:
        if (gr_get_fade_level() != 1.0f)
            break;
        if (!timer.valid()) {
            timer.SetTimeout(500);
            if (ls_eleh_go_ground)
                level_entry_point = 5;
            else
                level_entry_point = ls_eleh_go_bottom ? 3 : 4;
            script_place_party_at_starts();
            game_camera.set_target(cur_character);
            game_camera.snap_to_target(1, 1);
        }
        if (timer.elapsed()) {
            script_enable_player_input();
            gr_fade_in(1000);
            ls_eleh_go_ground = false;
            ls_eleh_go_bottom = false;
            ls_eleh_go_other = false;
            ls_eleh_cutscene_state = 0;
        }
        break;
    }
}
// 0x4f1880  Confidence: exact. atexit_nop_4f1880: the compiler's empty atexit handler of the static timer.

// 0x4f1890  Confidence: exact. level_script_table action: the conversation choices.
void ls_eleh_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "JoinJoseph"))
        ls_eleh_go_top = true;
    if (!_stricmp(action->name, "TopFloor"))
        ls_eleh_go_ground = true;
    if (!_stricmp(action->name, "GroundFloor"))
        ls_eleh_go_bottom = true;
    if (!_stricmp(action->name, "BottomFloor"))
        ls_eleh_go_other = true;
}

// 0x4f1910  Confidence: exact. level_script_table use.
char ls_eleh_use(const char *name)
{
    if (!_stricmp(name, "worldmap1"))
        script_party_member_say("Flece", XSTR("I must find Joseph. I won't leave Eleh without him.", 482), 5.0f);
    return 0;
}
