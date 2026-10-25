// level_script_lpalaceint.cpp - Sum.exe v1.40, code 0x4f29d0-0x4f3a71 (levelscripts\Scripts\
// level_script_lpalaceint.cpp; strings.tbl ids 518-547 are this unit's). Written from the Sum.exe
// disassembly (tools/asm.mjs; the jump tables, the two guard tables and the float constant read from the
// image). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow,
// calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// The palace of Lenele. "lpalaceint" (act 1): Flece sneaks through the palace to find Yago (the guards'
// view cones, the servant's disguise, being caught: fade, back to the start, the attempt counter and Joseph's
// and Flece's remarks), King-Meeting-Aoqi, Meeting-With-Yago, the Ring of Darkness. "lpalaceint_v2"
// (act 2, stage 4+): Yago and King Bellias talk to the party, Jekhar joins (King-Audience-B). Static
// initialiser table entries for the timer and the three line tables (0x4f29d0-0x4f2ab0).
// Data: .data 0x5a28f4-0x5a291f (the guard name tables, then the literals); .bss 0x257ea00-0x257ea55.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/camera/camera.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_LENELE1B       5                       // "ivane_unhide_evien" (0x4f2b0c)
#define LEVEL_IKAEMOSTOPINT  0xc                     // "got_ring_of_stone" (0x4f2b50)
#define LEVEL_KHOSANILAB     0xe                     // "got_ring_of_fire" (0x4f2b91)
// The palace guards: object type 4, species 0x22 (ls_lpalaceint_guard_in_group 0x4f34a8 / 0x4f34b4).
#define LP_GUARD_TYPE        4
#define LP_GUARD_SPECIES     0x22

void ls_lpalaceint_sneak_frame();
bool ls_lpalaceint_guard_in_group(Character *c, bool upper);
void ls_lpalaceint_sneak_remark();
void lsf_lpalaceint_talked_to_joe();
void ls_lpalaceint_king_meeting_aoqi_cs_end();
void ls_lpalaceint_meeting_with_yago_cs_end();
void ls_lpalaceint_king_audience_b_cs_end();
void ls_lpalaceint_talk_to_king();

// ---- .data (0x5a28f4-) ----------------------------------------------------------------------------------
// The guards of the upper floors, who stop the disguised Flece (and get an avoidance spline around her),
// and the others, who only send her back (ls_lpalaceint_guard_in_group's two lists, counts 4 / 7).
// Names ours.
const char *ls_lpalaceint_upper_guards[4] = {            // 0x5a28f4
    "Palace Guard#3rdlvldoor2", "Palace Guard#3b", "Palace Guard#3stairs", "Palace Guard#2stairs",
};
const char *ls_lpalaceint_lower_guards[7] = {            // 0x5a2904
    "Palace Guard#2ndlvldoor2", "Palace Guard#3a", "Palace Guard#3rdlvldoor1", "Palace Guard#2ndlvldoor1",
    "Palace Guard#1a", "Palace Guard#2a", "Palace Guard#2b",
};

// ---- .bss (0x257ea00-0x257ea55) ------------------------------------------------------------------------
// 0x4f29d0  Confidence: exact. static_init_ls_lpalaceint_timer: "jmp 0x4f29e0".
// 0x4f29e0  Confidence: exact. init_ls_lpalaceint_timer: "mov ecx, 0x257ea00 / jmp GameTimer::GameTimer".
GameTimer ls_lpalaceint_sneak_timer;                 // 0x257ea00
// Set when the guard who caught Flece is not "Palace Guard#1a" (0x4f3450): that capture counts as an
// attempt; a capture by #1a instead gives the "Having trouble?" remark. Not in sum.symbols; name ours.
bool ls_lpalaceint_caught_counts;                    // 0x257ea04
bool ls_lpalaceint_got_suit_done;                    // 0x257ea05
int ls_lpalaceint_sneak_state;                       // 0x257ea10
bool ls_lpalaceint_yago_meeting_pending;             // 0x257ea14
int ls_lpalaceint_remark_stage;                      // 0x257ea18
bool ls_lpalaceint_caught_remark_pending;            // 0x257ea1c
// 0x4f29f0  Confidence: likely. static_init_lpalaceint_guard_halt_lines: "jmp 0x4f2a00".
// 0x4f2a00  Confidence: likely (the slot addresses; as ai.cpp's slot tables). init_lpalaceint_guard_halt_lines.
char **ls_lpalaceint_guard_halt_lines[7] = {         // 0x257ea20
    XSTR_SLOT("You there! Halt! You have no business here!", 518), XSTR_SLOT("Stop! Spy! Alarum!", 519),
    XSTR_SLOT("Who goes there?", 520), XSTR_SLOT("Halt! In the name of the king!", 521),
    XSTR_SLOT("Who are you? What are you doing here?", 522), XSTR_SLOT("Where do you think you're going?", 523),
    XSTR_SLOT("Intruder! Stop her!", 524),
};
// 0x4f2a50  Confidence: likely. static_init_lpalaceint_guard_servant_lines: "jmp 0x4f2a60".
// 0x4f2a60  Confidence: likely. init_lpalaceint_guard_servant_lines.
char **ls_lpalaceint_guard_servant_lines[5] = {      // 0x257ea3c
    XSTR_SLOT("Go back. You're not allowed this way, servant.", 525),
    XSTR_SLOT("Back to the kitchen where you belong!", 526),
    XSTR_SLOT("Return to the kitchen until you're sent for!", 527),
    XSTR_SLOT("No servants allowed! Prince's orders!", 528), XSTR_SLOT("You don't have run of the palace!", 529),
};
// 0x4f2aa0  Confidence: likely. static_init_lpalaceint_guard_servant_line: "jmp 0x4f2ab0".
// 0x4f2ab0  Confidence: likely. init_lpalaceint_guard_servant_line. A one-entry table: the reader calls
// rand() and drops the result (index rand() % 1, D2400).
char **ls_lpalaceint_guard_servant_line[1] = {       // 0x257ea50
    XSTR_SLOT("You're not supposed to be here, servant.", 530),
};
bool ls_lpalaceint_king_audience_pending;            // 0x257ea54
bool ls_lpalaceint_talked_to_joe;                    // 0x257ea55

// 0x4f2ac0  Confidence: exact. level_script_table init.
void ls_lpalaceint_init()
{
    level_music = music_find_by_name("lenele palace sneak");
    ls_lpalaceint_got_suit_done = false;
    ls_lpalaceint_sneak_state = 0;
    if (!_stricmp(level_script_name, "lpalaceint_v2")) {
        game_stage_set(4);
        if (savegame_state.get_flag(LEVEL_LENELE1B, "ivane_unhide_evien"))
            script_npc_set_disabled("Evien", true);
        if (game_stage_get() >= 5)
            script_npc_set_disabled("Yago", true);
        else
            script_npc_set_disabled("Yago#Visit2c", true);
        if (savegame_state.get_flag(LEVEL_IKAEMOSTOPINT, "got_ring_of_stone")) {
            conversation_db.set_topic_available("Yago#Visit2c", "\"We have the Ring of Stone.\"", true);
            conversation_db.set_topic_available("Queen Galienne", "\"Yes, in the ruins of Ikaemos.\"", true);
        } else if (savegame_state.get_flag(LEVEL_KHOSANILAB, "got_ring_of_fire")) {
            conversation_db.set_topic_available("Yago#Visit2c", "\"We have the Ring of Fire.\"", true);
        }
    } else {
        game_stage_set(1);
        if (game_stage_get() >= 2) {
            script_npc_set_disabled("Joseph#npc", true);
            if (game_stage_get() >= 4) {
                level_trigger_set_exit("lenele1e", "lenele1e_v2");
                level_trigger_set_script_use("lenele1e", "lenele1e");
            } else {
                level_trigger_set_exit("lenele1e", "lenele1e");
                level_trigger_set_script_use("lenele1e", "lenele1e_v2");
            }
            script_npc_set_disabled("Yago", true);
            script_npc_set_disabled("King Bellias", true);
            script_npc_set_disabled("Aoqi", true);
            script_npc_send_go("Yago#2");
        } else {
            if (party_remove_member("Joseph"))
                game_camera.reset_default();
            level_trigger_set_script_use("lenele1e", "lenele1e");
            level_trigger_set_script_use("lenele1e", "lenele1e_v2");
        }
        if (game_stage_get() >= 3)
            script_npc_set_disabled("Yago#2", true);
    }
    ls_lpalaceint_remark_stage = 0;
    ls_lpalaceint_sneak_timer.invalidate();
    ls_lpalaceint_yago_meeting_pending = false;
    ls_lpalaceint_king_audience_pending = false;
    ls_lpalaceint_talked_to_joe = false;
}

// 0x4f2ce0  Confidence: exact (ends in a tail jump to lsf_lpalaceint_talked_to_joe). level_script_table
// frame. Stage 4+: the audience with the king and Yago's talk. Before: after StartCutscene the meeting
// with Yago, else King-Meeting-Aoqi at $kingtrig and the sneak sequence.
void ls_lpalaceint_frame()
{
    if (game_stage_get() >= 4) {
        if (ls_lpalaceint_king_audience_pending && !(game_flags & GF_CONVERSATION)) {
            party_member_join("Jekhar", 8, 0, 1, -1);
            jekhar_joined = true;
            ls_lpalaceint_king_audience_pending = false;
            script_play_cutscene("King-Audience-B", ls_lpalaceint_king_audience_b_cs_end);
        }
        if (!savegame_state.get_flag(cur_level_index, "found_jekhar")
            && savegame_state.get_flag(cur_level_index, "talked_to_yago") && conversation_active.is_idle()
            && script_npc_near_player("King Bellias", 8.0f)) {
            ls_lpalaceint_talk_to_king();
            savegame_state.set_flag(cur_level_index, "found_jekhar", true);
        }
        if (game_stage_get() < 5 && script_npc_near_player("Yago", 10.0f) && conversation_active.is_idle()
            && !savegame_state.get_flag(cur_level_index, "talked_to_yago")) {
            script_npc_talk("Yago");
            savegame_state.set_flag(cur_level_index, "talked_to_yago", true);
        }
        return;
    }
    if (savegame_state.get_flag(cur_level_index, "lpalaceint_yago_found")) {
        if (game_flags & GF_CONVERSATION)
            return;
        if (ls_lpalaceint_yago_meeting_pending) {
            script_play_cutscene("Meeting-With-Yago", ls_lpalaceint_meeting_with_yago_cs_end);
            ls_lpalaceint_yago_meeting_pending = false;
        }
        if (ls_lpalaceint_talked_to_joe) {
            script_npc_talk("Yago#2");
            script_npc_set_disabled("Joseph#npc", true);
            ls_lpalaceint_talked_to_joe = false;
        }
        return;
    }
    if (!(game_flags & GF_CONVERSATION) && !savegame_state.get_flag(cur_level_index, "found_aoqi")
        && script_player_near_navpoint("$kingtrig", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "found_aoqi", true);
        script_play_cutscene("King-Meeting-Aoqi", ls_lpalaceint_king_meeting_aoqi_cs_end);
    }
    ls_lpalaceint_sneak_frame();
    ls_lpalaceint_sneak_remark();
    lsf_lpalaceint_talked_to_joe();
}

// 0x4f2f00  Confidence: exact (the case order and the loop forms). The sneak sequence. State 0: in the
// servant's disguise the upper guards who see Flece (cone 0.71) block her with an avoidance spline and
// pause their script until they lose sight of her (cone 0), the lower guards only grumble; without it the
// first guard who sees her shouts, walks to where she will be in 0.5 s, input goes off (state 1). State 1:
// 2 s, then a 1 s fade out (state 2). State 2 at full fade: Flece back at start 0, the remarks start in 2 s,
// every guard back at its start navpoint (Palace Guard#1a stays away after 4 counted attempts); 0.5 s, then
// input on, the attempt counted, Palace Guard#1a teleported to $Teleport001 at exactly 4, fade in.
// The function-local GameTimer's empty atexit handler is 0x4f3510.
void ls_lpalaceint_sneak_frame()
{
    static GameTimer timer;                          // 0x257ea08 ls_lpalaceint_sneak_frame_timer (guard 0x257ea0c)
    Character *c;
    navpoint *np;
    vector3 pos;
    int i;
    int r;

    switch (ls_lpalaceint_sneak_state) {
    case 0:
        if (!conversation_active.is_idle())
            break;
        if (character_wearing_servant_disguise(cur_character)) {
            if (!savegame_state.get_flag(cur_level_index, "got_suit"))
                savegame_state.set_flag(cur_level_index, "got_suit", true);
            if (!ls_lpalaceint_got_suit_done) {
                script_npc_goto_label("Palace Guard#3rdlvldoor2", 1);
                script_npc_goto_label("Palace Guard#2stairs", 1);
                script_npc_goto_label("Palace Guard#3a", 2);
                script_npc_goto_label("Palace Guard#3b", 2);
                script_npc_goto_label("Palace Guard#3stairs", 1);
                ls_lpalaceint_got_suit_done = true;
            }
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (ls_lpalaceint_guard_in_group(c, true)) {
                    if (script_avoid_spline_in_use(c->handle)) {
                        if (!script_guard_sees_player(c, 0.0f)) {
                            script_free_avoid_spline(c->handle);
                            if (c->script_object >= 0)
                                script_object_resume_index(c->script_object);
                        }
                    } else if (script_guard_sees_player(c, 0.71f)) {
                        r = rand();
                        c->say(localize_string_or_invalid(ls_lpalaceint_guard_servant_lines[r % 5]), 2.0f);
                        script_alloc_avoid_spline(c->handle, cur_character->handle);
                        if (c->script_object >= 0)
                            script_object_suspend(c->script_object);
                    }
                } else if (ls_lpalaceint_guard_in_group(c, false) && script_guard_sees_player(c, 0.71f)) {
                    r = rand();
                    c->say(localize_string_or_invalid(ls_lpalaceint_guard_servant_line[r % 1]), 2.0f);
                }
            }
            break;
        }
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if ((ls_lpalaceint_guard_in_group(c, true) || ls_lpalaceint_guard_in_group(c, false))
                && script_guard_sees_player(c, 0.71f))
                break;
        }
        if (c == &living_list)
            break;
        r = rand();
        c->say(localize_string_or_invalid(ls_lpalaceint_guard_halt_lines[r % 7]), 2.0f);
        ls_lpalaceint_caught_counts = _stricmp(c->get_name(), "Palace Guard#1a") != 0;
        cur_character->ai.predict_owner_position(&pos, 0.5f);
        c->ai_add_goal9(&pos, cur_character->layer, 999999, false);
        script_disable_player_input();
        timer.invalidate();
        ls_lpalaceint_sneak_state = 1;
        break;
    case 1:
        if (!timer.valid())
            timer.SetTimeout(2000);
        if (timer.elapsed()) {
            gr_fade_out(1000);
            timer.invalidate();
            ls_lpalaceint_sneak_state = 2;
        }
        break;
    case 2:
        if (gr_get_fade_level() != 1.0f)
            break;
        if (!timer.valid()) {
            timer.SetTimeout(500);
            cur_character->place_at_party_start(0);
            game_camera.set_target(cur_character);
            game_camera.snap_to_target(1, 1);
            ls_lpalaceint_sneak_timer.SetTimeout(2000);
            ls_lpalaceint_remark_stage = 1;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if ((ls_lpalaceint_guard_in_group(c, true) || ls_lpalaceint_guard_in_group(c, false))
                    && (savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts") < 4
                        || _stricmp(c->get_name(), "Palace Guard#1a"))) {
                    np = (navpoint *)c->start_navpoint;
                    c->pos = np->pos;
                    c->orient.set_from_angles(0, 0, np->heading);
                    c->layer = np->layer;
                }
            }
        }
        if (timer.elapsed()) {
            script_enable_player_input();
            if (ls_lpalaceint_caught_counts)
                savegame_state.set_int(cur_level_index, "lpalaceint_int_sneak_attempts",
                                       savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts") + 1);
            else
                ls_lpalaceint_caught_remark_pending = true;
            if (savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts") == 4) {
                i = script_object_find("Palace Guard#1a");
                if (i >= 0) {
                    c = living_from_handle(script_objects[i].handle);
                    if (c) {
                        np = navpoint_find("$Teleport001");
                        c->pos = np->pos;
                        c->orient.set_from_angles(0, 0, np->heading);
                        c->layer = np->layer;
                    }
                }
            }
            gr_fade_in(1000);
            ls_lpalaceint_sneak_state = 0;
        }
        break;
    }
}

// 0x4f34a0  Confidence: exact (the list and count as two conditionals; the code builds the count
// branch-free: `neg al / sbb / and -3 / add 7`). A palace guard (type 4, species 0x22) named in the upper
// list (4) or the lower list (7).
bool ls_lpalaceint_guard_in_group(Character *c, bool upper)
{
    const char **names;
    int count, i;

    if (c->type == LP_GUARD_TYPE && c->info->species == LP_GUARD_SPECIES) {
        names = upper ? ls_lpalaceint_upper_guards : ls_lpalaceint_lower_guards;
        count = upper ? 4 : 7;
        for (i = 0; i < count; i++) {
            if (!_stricmp(c->get_name(), names[i]))
                return true;
        }
    }
    return false;
}
// 0x4f3510  Confidence: exact. atexit_nop_4f3510: the compiler's empty atexit handler of the static timer.

// 0x4f3520  Confidence: exact (the cases are written with their own calls; the code shares one call per
// stage, jump tables 0x4f36a0 / 0x4f36b0). After a capture: Joseph (stage 1) and Flece (stage 2, 5 s later)
// comment, by the attempt count.
void ls_lpalaceint_sneak_remark()
{
    if (!ls_lpalaceint_sneak_timer.elapsed())
        return;
    if (ls_lpalaceint_remark_stage == 1) {
        if (ls_lpalaceint_caught_remark_pending) {
            script_object_say(script_object_find("Joseph#npc"), XSTR("Having trouble?", 531), 5.0f);
        } else {
            switch (savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts")) {
            case 1:
                script_object_say(script_object_find("Joseph#npc"), XSTR("Did you find Yago?", 532), 4.0f);
                break;
            case 2:
                script_object_say(script_object_find("Joseph#npc"), XSTR("So where's Yago?", 533), 3.0f);
                break;
            case 3:
                script_object_say(script_object_find("Joseph#npc"),
                                  XSTR("You've never done this before, have you?", 534), 5.0f);
                break;
            case 4:
                script_object_say(script_object_find("Joseph#npc"), XSTR("Why don't I go with you this time?", 535),
                                  5.0f);
                break;
            default:
                script_object_say(script_object_find("Joseph#npc"), XSTR("There has to be a better way.", 536), 4.0f);
                break;
            }
        }
        ls_lpalaceint_remark_stage = 2;
        ls_lpalaceint_sneak_timer.SetTimeout(5000);
    } else if (ls_lpalaceint_remark_stage == 2) {
        if (ls_lpalaceint_caught_remark_pending) {
            script_party_member_say("Flece", XSTR("I'm working on it.", 537), 5.0f);
            ls_lpalaceint_caught_remark_pending = false;
        } else {
            switch (savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts")) {
            case 1:
                script_party_member_say("Flece", XSTR("I'm working on it. Just wait right here.", 538), 6.0f);
                break;
            case 2:
                script_party_member_say("Flece", XSTR("I just need to get past the guards!", 539), 5.0f);
                break;
            case 3:
                script_party_member_say("Flece", XSTR("Keep your mouth shut, and stay right where you are!", 540),
                                        5.0f);
                break;
            case 4:
                script_party_member_say("Flece", XSTR("Why don't I break your legs?", 541), 5.0f);
                break;
            default:
                script_party_member_say("Flece", XSTR("Shut up!", 542), 3.0f);
                break;
            }
        }
        ls_lpalaceint_remark_stage = 0;
        ls_lpalaceint_sneak_timer.invalidate();
    }
}

// 0x4f36c0  Confidence: exact. Joseph talks to Flece once at the start.
void lsf_lpalaceint_talked_to_joe()
{
    if (!savegame_state.get_flag(cur_level_index, "lpalaceint_talked_to_joe") && conversation_active.is_idle()) {
        savegame_state.set_flag(cur_level_index, "lpalaceint_talked_to_joe", true);
        script_npc_talk("Joseph#npc");
    }
}

// 0x4f3710  Confidence: exact. End of King-Meeting-Aoqi.
void ls_lpalaceint_king_meeting_aoqi_cs_end()
{
    script_npc_set_disabled("Aoqi", true);
    script_npc_goto_label("King Bellias", 1);
}

// 0x4f3730  Confidence: exact. End of Meeting-With-Yago: Joseph joins and leads, the party moves to start
// 3, Palace Guard#1a goes to $npc109, the lenele1e exit opens, Yago talks next frame.
void ls_lpalaceint_meeting_with_yago_cs_end()
{
    Character *c;
    navpoint *np;
    int i;

    party_set_leader(party_add_member("Joseph"));
    script_npc_set_disabled("Yago", true);
    script_npc_set_disabled("Joseph#npc", true);
    script_npc_send_go("Yago#2");
    level_entry_point = 3;
    script_place_party_at_starts();
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
    i = script_object_find("Palace Guard#1a");
    if (i >= 0) {
        c = living_from_handle(script_objects[i].handle);
        if (c) {
            np = navpoint_find("$npc109");
            c->pos = np->pos;
            c->orient.set_from_angles(0, 0, np->heading);
            c->layer = np->layer;
        }
    }
    level_trigger_set_exit("lenele1e", NULL);
    ls_lpalaceint_talked_to_joe = true;
}

// 0x4f3820  Confidence: exact. End of King-Audience-B.
void ls_lpalaceint_king_audience_b_cs_end()
{
    script_npc_goto_label("King Bellias", 1);
    level_entry_point = 3;
    script_place_party_at_starts();
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
    savegame_state.set_flag(cur_level_index, "found_jekhar", true);
}

// 0x4f3880  Confidence: exact.
void ls_lpalaceint_talk_to_king()
{
    script_npc_talk("King Bellias");
}

// 0x4f3890  Confidence: exact. level_script_table action. StartCutscene (Yago found): Sneak, and
// 500 XP (200 in the disguise) + 50 per unused attempt below 4 for the current character; GotRing: the
// Ring of Darkness; YagoGo; Jekhar: the audience.
void ls_lpalaceint_action(dialogue_action *action)
{
    char line[80];
    int xp;

    if (!_stricmp(action->name, "StartCutscene")) {
        savegame_state.set_flag(cur_level_index, "lpalaceint_yago_found", true);
#ifdef MODERN_BUILD
        sprintf(line, "%s", XSTR("Flece acquired skill: Sneak!", 543));   // translated text, not a format
#else
        sprintf(line, XSTR("Flece acquired skill: Sneak!", 543));
#endif
        dialogue_show_line(line, 1, NULL);
        xp = 500;
        if (savegame_state.get_flag(cur_level_index, "got_suit"))
            xp = 200;
        if (savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts") < 4)
            xp += (4 - savegame_state.get_int(cur_level_index, "lpalaceint_int_sneak_attempts")) * 50;
        sprintf(line, XSTR("<Flece gained xp %d>", 544), xp);
        dialogue_show_line(line, 1, NULL);
        if (cur_character)
            cur_character->add_xp(xp, false, false);
        ls_lpalaceint_yago_meeting_pending = true;
        return;
    }
    if (!_stricmp(action->name, "GotRing")) {
        savegame_state.set_flag(cur_level_index, "got_ring_of_darkness", true);
#ifdef MODERN_BUILD
        sprintf(line, "%s", XSTR("Joseph acquired skill: Summon!", 545));   // translated text, not a format
#else
        sprintf(line, XSTR("Joseph acquired skill: Summon!", 545));
#endif
        dialogue_show_line(line, 1, NULL);
        hud_show_message(XSTR("Joseph acquired Summon Blackfire Elemental!", 546));
        return;
    }
    if (!_stricmp(action->name, "YagoGo")) {
        script_npc_send_go("Yago");
        return;
    }
    if (!_stricmp(action->name, "Jekhar"))
        ls_lpalaceint_king_audience_pending = true;
}

// 0x4f3a40  Confidence: exact. level_script_table use.
char ls_lpalaceint_use(const char *name)
{
    if (!_stricmp(name, "lenele1e"))
        script_party_member_say("Flece", XSTR("I'm not leaving until I find Yago.", 547), 5.0f);
    return 0;
}
