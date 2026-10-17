// level_script_wolong_caverns.cpp - Sum.exe v1.40, code 0x4f6620-0x4f7210 (levelscripts\Scripts\
// level_script_wolong_caverns.cpp; strings.tbl ids 564-567 are this unit's). Written from the Sum.exe
// disassembly (tools/asm.mjs; the float constants read from the image). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals match), likely
// (one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The Wolong caverns: the two spear traps ($spike1a-c / $spike2a-c) and the poison mist trap ($poisontrap),
// Balis the Demilich's maze (MazeStart / MazeEnd fade the party to another start), Nali's healing point
// and the Ring of Water (Hand-Restoration), the plants, the store room switches. Static initialiser table
// entries for the three timers and the two vector3 objects (0x4f6620-0x4f66b0).
// Data: .bss 0x257ea78-0x257eaff.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdlib.h>
#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "vsdk/snd/snd.h"
#include "vsdk/gr/gr.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

// Character +0x320 bit the trap triggers skip (`test ah, 4` at 0x4f692c / 0x4f6abb). Name ours.
#define WC_TRAP_IGNORE_FLAG 0x400

void ls_wolongcaverns_update_triggers();
void ls_wolongcaverns_trap1_frame();
void ls_wolongcaverns_trap1_fire();
void ls_wolongcaverns_ptrap_frame();
void ls_wolongcaverns_ptrap_fire();
void ls_wolongcaverns_demilich_sequence();
void lsf_wc_nali_used();
void ls_wolongcaverns_plants_remark();
void ls_wolongcaverns_trap2_fire();

// ---- .bss (0x257ea78-0x257eaff) ------------------------------------------------------------------------
// The dynamically initialised objects, in initialiser-table order.
// 0x4f6620  Confidence: exact. static_init_ls_wolongcaverns_trap1_timer: "jmp 0x4f6630".
// 0x4f6630  Confidence: exact. init_ls_wolongcaverns_trap1_timer: "mov ecx, 0x257ea84 / jmp GameTimer::GameTimer".
GameTimer ls_wolongcaverns_trap1_timer;              // 0x257ea84
// 0x4f6640  Confidence: exact. static_init_ls_wolongcaverns_trap2_timer: "jmp 0x4f6650".
// 0x4f6650  Confidence: exact. init_ls_wolongcaverns_trap2_timer.
GameTimer ls_wolongcaverns_trap2_timer;              // 0x257eaa8
// 0x4f6660  Confidence: exact. static_init_ls_wolongcaverns_ptrap_timer: "jmp 0x4f6670".
// 0x4f6670  Confidence: exact. init_ls_wolongcaverns_ptrap_timer.
GameTimer ls_wolongcaverns_ptrap_timer;              // 0x257ea78
// The two vector3 objects (empty inline constructor, a bare ret each); which initialiser is which is not
// visible.
// 0x4f6680  Confidence: unsure (the assignment). static_init_empty_4f6690: "jmp 0x4f6690".
// 0x4f6690  Confidence: unsure (the assignment). init_empty_4f6690: a bare ret.
vector3 ls_wolongcaverns_ptrap_pos;                  // 0x257ea88
// 0x4f66a0  Confidence: unsure (the assignment). static_init_empty_4f66b0: "jmp 0x4f66b0".
// 0x4f66b0  Confidence: unsure (the assignment). init_empty_4f66b0: a bare ret.
// $spike1a-c (trap 1, 0x257eab0) and $spike2a-c (trap 2, 0x257ead4). Not in sum.symbols; name ours.
vector3 ls_wolongcaverns_spike_pos[6];               // 0x257eab0
int ls_wolongcaverns_poisonmist_sound;               // 0x257ea80
int ls_wolongcaverns_speartrap_sound;                // 0x257ea94
int ls_wolongcaverns_ptrap_fx;                       // 0x257ea98
int ls_wolongcaverns_trap2_fx;                       // 0x257eaa0
int ls_wolongcaverns_trap1_fx;                       // 0x257eaa4
int ls_wolongcaverns_demilich_step;                  // 0x257eaf8 1 MazeEnd, 2 MazeStart, 0 idle
int ls_wolongcaverns_demilich_state;                 // 0x257eafc

// 0x4f66c0  Confidence: exact. level_script_table init (stage 11; Jekhar leaves the party before stage 13).
// The trap positions from their navpoints, the trap effects off, the two trap sounds loaded.
void ls_wolongcaverns_init()
{
    matrix orient;

    level_music = music_find_by_name("WolongCaverns");
    game_stage_set(0xb);
    if (game_stage_get() < 0xd)
        party_remove_member("Jekhar");
    ls_wolongcaverns_update_triggers();
    ls_wolongcaverns_trap1_fx = -1;
    ls_wolongcaverns_trap2_fx = -1;
    ls_wolongcaverns_ptrap_fx = -1;
    navpoint_get_position("$spike1a", &ls_wolongcaverns_spike_pos[0], &orient);
    navpoint_get_position("$spike1b", &ls_wolongcaverns_spike_pos[1], &orient);
    navpoint_get_position("$spike1c", &ls_wolongcaverns_spike_pos[2], &orient);
    navpoint_get_position("$spike2a", &ls_wolongcaverns_spike_pos[3], &orient);
    navpoint_get_position("$spike2b", &ls_wolongcaverns_spike_pos[4], &orient);
    navpoint_get_position("$spike2c", &ls_wolongcaverns_spike_pos[5], &orient);
    navpoint_get_position("$poisontrap", &ls_wolongcaverns_ptrap_pos, &orient);
    ls_wolongcaverns_trap1_timer.invalidate();
    ls_wolongcaverns_trap2_timer.invalidate();
    ls_wolongcaverns_ptrap_timer.invalidate();
    ls_wolongcaverns_demilich_step = 0;
    script_npc_set_disabled("Balis the Demilich#2", true);
    fx_disable_by_effect_name("wc-trap1.vfx");
    fx_disable_by_effect_name("wc-trap2.vfx");
    fx_disable_by_effect_name("wc-ptrap.vfx");
    ls_wolongcaverns_speartrap_sound = snd_get_handle("wolong_caverns_speartrap_up.wav", 1.0f, 1.0f, 1.0f, 0, false);
    snd_load(ls_wolongcaverns_speartrap_sound, 0);
    ls_wolongcaverns_poisonmist_sound = snd_get_handle("wolong_caverns_poisonmist.wav", 1.0f, 1.0f, 1.0f, 0, false);
    snd_load(ls_wolongcaverns_poisonmist_sound, 0);
}

// 0x4f6850  Confidence: exact. The Wolong exits by stage: from 13 wolong and wolong2 are script-use
// triggers and wolong2_v2 the live exit; at 12 wolong2 is the exit; before, wolong.
void ls_wolongcaverns_update_triggers()
{
    if (game_stage_get() >= 0xd) {
        level_trigger_set_script_use("wolong", "wolong");
        level_trigger_set_script_use("wolong2", "wolong2");
        level_trigger_set_exit("wolong2", "wolong2_v2");
        return;
    }
    if (game_stage_get() >= 0xc) {
        level_trigger_set_script_use("wolong", "wolong");
        level_trigger_set_exit("wolong2", "wolong2");
        level_trigger_set_script_use("wolong2", "wolong2_v2");
        return;
    }
    level_trigger_set_exit("wolong", "wolong");
    level_trigger_set_script_use("wolong2", "wolong2");
    level_trigger_set_script_use("wolong2", "wolong2_v2");
}

// 0x4f68f0  Confidence: exact (ends in a tail jump to lsf_wc_nali_used). level_script_table frame. Trap 2
// fires only from Switch-03 (ls_wolongcaverns_use).
void ls_wolongcaverns_frame()
{
    ls_wolongcaverns_plants_remark();
    empty_frame_hook_40acc0();
    ls_wolongcaverns_trap1_frame();
    ls_wolongcaverns_ptrap_frame();
    ls_wolongcaverns_demilich_sequence();
    lsf_wc_nali_used();
}

// 0x4f6910  Confidence: exact (the squared distance as vector3 - and len_sq, expanded). A party character
// (type 7) within 3 of a trap 1 spike fires the trap.
void ls_wolongcaverns_trap1_frame()
{
    Character *c;
    int i;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7 && !(c->flags & WC_TRAP_IGNORE_FLAG)) {
            for (i = 0; i < 3; i++) {
                vector3 d = c->pos - ls_wolongcaverns_spike_pos[i];
                if (d.len_sq() < 9.0f)
                    ls_wolongcaverns_trap1_fire();
            }
        }
    }
}

// 0x4f6990  Confidence: exact (the inner loop's break). Spear trap 1 (6 s rearm): the effect, the sound,
// and every living thing within 3 of a spike takes 140, flinches and is slowed (40 min).
void ls_wolongcaverns_trap1_fire()
{
    Character *c;
    int i;

    if (ls_wolongcaverns_trap1_timer.elapsed())
        ls_wolongcaverns_trap1_timer.invalidate();
    if (ls_wolongcaverns_trap1_timer.valid())
        return;
    ls_wolongcaverns_trap1_timer.SetTimeout(6000);
    ls_wolongcaverns_trap1_fx = fx_create_world("wc-trap1.vfx", true);
    snd_play(ls_wolongcaverns_speartrap_sound, 1.0f, 0, 0, 0);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        for (i = 0; i < 3; i++) {
            vector3 d = c->pos - ls_wolongcaverns_spike_pos[i];
            if (d.len_sq() < 9.0f) {
                c->add_hp(-140.0f, -1, 1, false);
                c->play_flinch(0);
                c->status_effect_add(STATUS_SLOW, 2400000, -1, 0, -1, true);
                break;
            }
        }
    }
}

// 0x4f6aa0  Confidence: likely (as trap1_frame). A party character within 2 of $poisontrap.
void ls_wolongcaverns_ptrap_frame()
{
    Character *c;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7 && !(c->flags & WC_TRAP_IGNORE_FLAG)) {
            vector3 d = c->pos - ls_wolongcaverns_ptrap_pos;
            if (d.len_sq() < 4.0f)
                ls_wolongcaverns_ptrap_fire();
        }
    }
}

// 0x4f6b20  Confidence: likely (the three-pass loop over one invariant distance, hoisted into a stack
// temporary: copied from the spear traps, see NOTES). Poison mist (2 s rearm): everything within 2 takes
// 15 and gets status 3 (Curse, value 0.5, 40 min), not Poison.
void ls_wolongcaverns_ptrap_fire()
{
    Character *c;
    int i;

    if (ls_wolongcaverns_ptrap_timer.elapsed())
        ls_wolongcaverns_ptrap_timer.invalidate();
    if (ls_wolongcaverns_ptrap_timer.valid())
        return;
    ls_wolongcaverns_ptrap_timer.SetTimeout(2000);
    ls_wolongcaverns_ptrap_fx = fx_create_world("wc-ptrap.vfx", true);
    snd_play(ls_wolongcaverns_poisonmist_sound, 1.0f, 0, 0, 0);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        for (i = 0; i < 3; i++) {
            vector3 d = c->pos - ls_wolongcaverns_ptrap_pos;
            if (d.len_sq() < 4.0f) {
                c->add_hp(-15.0f, -1, 1, false);
                c->status_effect_add(STATUS_CURSE, 2400000, -1, 0.5f, -1, true);
                break;
            }
        }
    }
}

// 0x4f6c30  Confidence: exact. Balis the Demilich's maze, run by the MazeEnd / MazeStart actions once no
// conversation is open: input off (state 0), 0.5 s then a 1 s fade out (state 1), at full fade the party
// moves to start 3 (MazeEnd: Balis #1 away, #2 there) or 2 (MazeStart: the other way round), 0.5 s, fade in.
// The function-local GameTimer's empty atexit handler is 0x4f6de0.
void ls_wolongcaverns_demilich_sequence()
{
    if (!ls_wolongcaverns_demilich_step || (game_flags & GF_CONVERSATION))
        return;
    // Constructed here, after the test (the guard check follows it at 0x4f6c4b).
    static GameTimer timer;                          // 0x257ea7c ls_wolongcaverns_demilich_timer (guard 0x257ea9c)
    switch (ls_wolongcaverns_demilich_state) {
    case 0:
        script_disable_player_input();
        timer.invalidate();
        ls_wolongcaverns_demilich_state = 1;
        break;
    case 1:
        if (!timer.valid())
            timer.SetTimeout(500);
        if (timer.elapsed()) {
            gr_fade_out(1000);
            timer.invalidate();
            ls_wolongcaverns_demilich_state = 2;
        }
        break;
    case 2:
        if (gr_get_fade_level() != 1.0f)
            break;
        if (!timer.valid()) {
            timer.SetTimeout(500);
            if (ls_wolongcaverns_demilich_step == 1) {
                level_entry_point = 3;
                script_npc_set_disabled("Balis the Demilich#1", true);
                script_npc_set_disabled("Balis the Demilich#2", false);
            } else if (ls_wolongcaverns_demilich_step == 2) {
                level_entry_point = 2;
                script_npc_set_disabled("Balis the Demilich#2", true);
                script_npc_set_disabled("Balis the Demilich#1", false);
            }
            script_place_party_at_starts();
            game_camera.set_target(cur_character);
            game_camera.snap_to_target(1, 1);
        }
        if (timer.elapsed()) {
            script_enable_player_input();
            gr_fade_in(1000);
            ls_wolongcaverns_demilich_state = 0;
            ls_wolongcaverns_demilich_step = 0;
        }
        break;
    }
}
// 0x4f6de0  Confidence: exact. atexit_nop_4f6de0: the compiler's empty atexit handler of the static timer.

// 0x4f6df0  Confidence: exact. The Ring of Water's summon when it is picked up; at $healpoint (4) without
// the ring of water: Joseph wants the Tail of the Rokhul (Nali's plant, wc_nali_used) and the ring first,
// then his gear is swapped and the hand restored (Hand-Restoration), and the exits are updated.
void lsf_wc_nali_used()
{
    if (!savegame_state.get_flag(cur_level_index, "wc_picked_up_ring") && party_count_item_by_name("Ring of Water")) {
        savegame_state.set_flag(cur_level_index, "wc_picked_up_ring", true);
        hud_show_message(XSTR("Joseph acquired Summon Blue Imp!", 564));
    }
    if (savegame_state.get_flag(cur_level_index, "got_ring_of_water") || !script_player_near_navpoint("$healpoint", 4.0f))
        return;
    if (!savegame_state.get_flag(cur_level_index, "wc_nali_used")) {
        script_party_member_say("Joseph", XSTR("Not yet! I must find the Tail of the Rokhul.", 565), 5.0f);
        return;
    }
    if (!savegame_state.get_flag(cur_level_index, "wc_picked_up_ring")) {
        script_party_member_say("Joseph", XSTR("The Ring of Water must be around here somewhere.", 566), 5.0f);
        return;
    }
    script_eleh_strip_party_gear(false);
    savegame_state.set_flag(cur_level_index, "got_ring_of_water", true);
    script_play_cutscene("Hand-Restoration", NULL);
    ls_wolongcaverns_update_triggers();
}

// 0x4f6f10  Confidence: exact.
void ls_wolongcaverns_plants_remark()
{
    if (!savegame_state.get_flag(cur_level_index, "wc_near_plants") && script_player_near_navpoint("$nearplants", 6.0f)) {
        script_party_member_say("Rosalind", XSTR("These must be the plants Sizhe told us about.", 567), 5.0f);
        savegame_state.set_flag(cur_level_index, "wc_near_plants", true);
    }
}

// 0x4f6f80  Confidence: exact. level_script_table action.
void ls_wolongcaverns_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "MazeEnd")) {
        ls_wolongcaverns_demilich_step = 1;
        return;
    }
    if (!_stricmp(action->name, "MazeStart")) {
        ls_wolongcaverns_demilich_step = 2;
        return;
    }
    if (!_stricmp(action->name, "LichHostile"))
        script_npc_goto_label("Azha the Archlich", 1);
}

// 0x4f6ff0  Confidence: exact. level_script_table use. A "Plant01-" object is picked (hidden) and cures
// the current character (spell 1, source type 7): wc_nali_used.
char ls_wolongcaverns_use(const char *name)
{
    if (!_strnicmp(name, "Plant01-", strlen("Plant01-"))) {
        lobjs_hide_by_prefix(name);
        cur_character->get_hit_by_spell(1, cur_character->handle, NULL, NULL, 7, NULL, 0, false);
        savegame_state.set_flag(cur_level_index, "wc_nali_used", true);
        return 1;
    }
    if (!_stricmp(name, "SkeletonSummoner")) {
        script_npc_talk("Ngaru's Ghost");
        return 1;
    }
    if (!_stricmp(name, "Switch-01"))
        door_apply_saved_state("StoreRoomDoor", true);
    if (!_stricmp(name, "Switch-02"))
        door_apply_saved_state("StoreRoomDoor", true);
    if (!_stricmp(name, "Switch-04"))
        door_apply_saved_state("Door01", true);
    if (!_stricmp(name, "Switch-03"))
        ls_wolongcaverns_trap2_fire();
    return 0;
}

// 0x4f7110  Confidence: exact (as trap1_fire). Spear trap 2 (6 s): 150 damage within 4 of a $spike2 point.
void ls_wolongcaverns_trap2_fire()
{
    Character *c;
    int i;

    if (ls_wolongcaverns_trap2_timer.elapsed())
        ls_wolongcaverns_trap2_timer.invalidate();
    if (ls_wolongcaverns_trap2_timer.valid())
        return;
    ls_wolongcaverns_trap2_timer.SetTimeout(6000);
    ls_wolongcaverns_trap2_fx = fx_create_world("wc-trap2.vfx", true);
    snd_play(ls_wolongcaverns_speartrap_sound, 1.0f, 0, 0, 0);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        for (i = 3; i < 6; i++) {
            vector3 d = c->pos - ls_wolongcaverns_spike_pos[i];
            if (d.len_sq() < 16.0f) {
                c->add_hp(-150.0f, -1, 1, false);
                c->play_flinch(0);
                c->status_effect_add(STATUS_SLOW, 2400000, -1, 0, -1, true);
                break;
            }
        }
    }
}
