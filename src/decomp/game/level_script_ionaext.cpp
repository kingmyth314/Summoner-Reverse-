// level_script_ionaext.cpp - Sum.exe v1.40, code 0x4f7220-0x4f77be. Written from the Sum.exe disassembly
// (tools/asm.mjs; the float 0x57e990 = 1.0 read from the image). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1
// /GX. Confidence per function: exact (control flow, calls, constants and globals match), likely (one detail
// of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IonaExt" (index 0xf; the abbey of Iona outside, stage 2: Rosalind joins,
// the aspirants' little camera scenes, the lift between the abbey floors). FILE NAME NOT PROVEN (D5300): no
// strings.tbl block, no path string; the name follows the level_script_<level>.cpp convention. Its .bss
// 0x257eb00-0x257eb0e lies between wolong_caverns' and khosani_lab's (source-map.md). No static initialiser
// (the one GameTimer is a function-local static, D2405, as in level_script_eleh.cpp).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/objects/living_entity.h"
#include "engine/camera/camera.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_LENELE1D   7                           // "lenele1d_mq_talked_to_captain" (0x4f724d)

void lsf_iona_ext_flag_chick_cam();
bool lsf_iona_ext_played_intro();
void ls_ionaext_nar_iona_cs_end();
void lsf_iona_ext_flag_sweep();
void lsf_iona_ext_flag_wood_cam();
bool lsf_iona_ext_found_rosalind();
void ls_ionaext_rosl_intro_cs_end();
void ls_ionaext_fade_place_party_frame();

// ---- .bss (0x257eb00-0x257eb0e) ------------------------------------------------------------------------
// (0x257eb00 / 0x257eb04: the static GameTimer of ls_ionaext_fade_place_party_frame and its guard.)
int ls_ionaext_cutscene_state;                       // 0x257eb08 the lift: 0 idle, 1 waiting, 2 faded out
bool ls_ionaext_go_up;                               // 0x257eb0c "GoUp" (entry point 4)
bool ls_ionaext_go_down;                             // 0x257eb0d "GoDown" (entry point 3)
bool ls_ionaext_go_home;                             // 0x257eb0e "GoHome" (set and cleared, nothing else)

// 0x4f7220  Confidence: exact. level_script_table init (stage 2). Sister Qelah is gone once Joseph talked to
// the captain in lenele1d.
void ls_ionaext_init()
{
    ls_ionaext_cutscene_state = 0;
    level_music = music_find_by_name("IonaExt");
    game_stage_set(2);
    if (savegame_state.get_flag(LEVEL_LENELE1D, "lenele1d_mq_talked_to_captain"))
        script_npc_set_disabled("Sister Qelah", true);
}

// 0x4f7270  Confidence: exact. Rosalind is looked for only on a frame on which the intro did not start.
// "GoHome" is cleared outside conversations and has no other effect (see NOTES).
void ls_ionaext_frame()
{
    lsf_iona_ext_flag_sweep();
    lsf_iona_ext_flag_chick_cam();
    lsf_iona_ext_flag_wood_cam();
    ls_ionaext_fade_place_party_frame();
    if (!lsf_iona_ext_played_intro())
        lsf_iona_ext_found_rosalind();
    if (!(game_flags & GF_CONVERSATION) && ls_ionaext_go_home)
        ls_ionaext_go_home = false;
}

// 0x4f72c0  Confidence: exact. Within 3 m of $npc058: the camera cuts to $camera007 and the two chicken
// aspirants go to label 2.
void lsf_iona_ext_flag_chick_cam()
{
    vector3 pos;
    matrix orient;

    if (!savegame_state.get_flag(cur_level_index, "iona_ext_flag_chick_cam")
        && script_player_near_navpoint("$npc058", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "iona_ext_flag_chick_cam", true);
        navpoint_get_position("$camera007", &pos, &orient);
        game_camera.cut_to_view_stub(&pos, &orient, 17.0f, 4.0f, 4.0f);
        script_npc_goto_label("Aspirant#chick1", 2);
        script_npc_goto_label("Aspirant#chick2", 2);
    }
}

// 0x4f7370  Confidence: exact. Once: the narrator's Nar-Iona, then IonaIntro. True on the frame it starts.
bool lsf_iona_ext_played_intro()
{
    if (savegame_state.get_flag(cur_level_index, "iona_ext_played_intro"))
        return false;
    savegame_state.set_flag(cur_level_index, "iona_ext_played_intro", true);
    script_play_cutscene("Nar-Iona", ls_ionaext_nar_iona_cs_end);
    return true;
}

// 0x4f73c0  Confidence: exact. End of Nar-Iona.
void ls_ionaext_nar_iona_cs_end()
{
    script_play_cutscene("IonaIntro", NULL);
}

// 0x4f73d0  Confidence: exact. Within 3 m of $npc036: the two sweeping aspirants to label 2.
void lsf_iona_ext_flag_sweep()
{
    if (!savegame_state.get_flag(cur_level_index, "iona_ext_flag_sweep")
        && script_player_near_navpoint("$npc036", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "iona_ext_flag_sweep", true);
        script_npc_goto_label("Aspirant#broom1", 2);
        script_npc_goto_label("Aspirant#broom2", 2);
    }
}

// 0x4f7440  Confidence: exact. Within 3 m of $npc042: the camera cuts to $camera008, the two wood aspirants
// to label 2.
void lsf_iona_ext_flag_wood_cam()
{
    vector3 pos;
    matrix orient;

    if (!savegame_state.get_flag(cur_level_index, "iona_ext_flag_wood_cam")
        && script_player_near_navpoint("$npc042", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "iona_ext_flag_wood_cam", true);
        navpoint_get_position("$camera008", &pos, &orient);
        game_camera.cut_to_view_stub(&pos, &orient, 20.0f, 4.0f, 4.0f);
        script_npc_goto_label("Aspirant#wood1", 2);
        script_npc_goto_label("Aspirant#wood2", 2);
    }
}

// 0x4f74f0  Confidence: exact. Near Abbot Laurent (4 m, no conversation): Rosalind joins at level 6, her
// weapon is paged in, ROSL-Intro plays. True on that frame.
bool lsf_iona_ext_found_rosalind()
{
    Character *rosalind;

    if (savegame_state.get_flag(cur_level_index, "iona_ext_found_rosalind") || (game_flags & GF_CONVERSATION)
        || !script_npc_near_player("Abbot Laurent", 4.0f))
        return false;
    rosalind = party_member_join("Rosalind", 6, false, true, -1);
    item_page_in(rosalind->equipment[5]->type - item_types, 6, 0);
    rosalind_joined = true;
    savegame_state.set_flag(cur_level_index, "iona_ext_found_rosalind", true);
    script_play_cutscene("ROSL-Intro", ls_ionaext_rosl_intro_cs_end);
    return true;
}

// 0x4f75a0  Confidence: exact. End of ROSL-Intro: Brother Ovaard's crypt key topic, the abbot talks.
void ls_ionaext_rosl_intro_cs_end()
{
    conversation_db.set_topic_available("Brother Ovaard", "\"Do you have a key to the crypt?\"", true);
    script_npc_talk("Abbot Laurent");
}

// 0x4f75d0  Confidence: exact. The lift ("GoUp" / "GoDown" of the lift conversation): input off, 0.5 s,
// fade out, the party placed at entry point 4 (up) or 3 (down), 0.5 s, input on and fade in. The same body as
// ls_eleh_fade_place_party_frame with two choices.
void ls_ionaext_fade_place_party_frame()
{
    if (game_flags & GF_CONVERSATION)
        return;
    static GameTimer timer;                          // 0x257eb00 ls_ionaext_elevator_timer (guard 0x257eb04)
    switch (ls_ionaext_cutscene_state) {
    case 0:
        if (ls_ionaext_go_up || ls_ionaext_go_down) {
            script_disable_player_input();
            timer.invalidate();
            ls_ionaext_cutscene_state = 1;
        }
        break;
    case 1:
        if (!timer.valid())
            timer.SetTimeout(500);
        if (timer.elapsed()) {
            gr_fade_out(1000);
            timer.invalidate();
            ls_ionaext_cutscene_state = 2;
        }
        break;
    case 2:
        if (gr_get_fade_level() != 1.0f)
            break;
        if (!timer.valid()) {
            timer.SetTimeout(500);
            level_entry_point = (ls_ionaext_go_up != 0) + 3;
            script_place_party_at_starts();
            game_camera.set_target(cur_character);
            game_camera.snap_to_target(1, 1);
        }
        if (timer.elapsed()) {
            script_enable_player_input();
            gr_fade_in(1000);
            ls_ionaext_go_up = false;
            ls_ionaext_go_down = false;
            ls_ionaext_cutscene_state = 0;
        }
        break;
    }
}
// 0x4f7750  Confidence: exact. atexit_nop_4f7750: the compiler's empty atexit handler of the static timer.

// 0x4f7760  Confidence: exact. level_script_table action (kind 0 actions only: +8 tested first).
void ls_ionaext_action(dialogue_action *action)
{
    if (action->kind == 0) {
        if (!_stricmp(action->name, "GoUp"))
            ls_ionaext_go_up = true;
        if (!_stricmp(action->name, "GoDown"))
            ls_ionaext_go_down = true;
        if (!_stricmp(action->name, "GoHome"))
            ls_ionaext_go_home = true;
    }
}
