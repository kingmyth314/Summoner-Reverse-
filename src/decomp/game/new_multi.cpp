// new_multi.cpp - Sum.exe v1.40, code 0x41dde0-0x41e3cf (the multiplayer elevators and ui_screen_stop_player)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 821-827 of the `// new_multi.cpp` block). It follows
// new_mainmenu.cpp (mainmenu_draw 0x41dd90) and ends before new_multichat.cpp's first static initialiser
// (0x41e3d0). Data: .data 0x58e0a0-0x58e0c7 (the five navpoint names; "Yes" / "No" / "Cancel" are string
// table slots of other units); no .bss.
//
// In a network game the party cannot use the level exits of the two elevator levels: interface_frame calls
// elevator_popup_frame every frame, which offers a message box when cur_character stands within 2 units of
// one of the elevator navpoints. The callbacks move the local player's character (and its summon) to the
// player start group (level_entry_point) of the chosen floor and restore the entry point afterwards.

#include "vsdk/localize/localize.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_multi.h"

#define ELEVATOR_RANGE 2.0f           // script_player_near_navpoint distance (push 0x40000000)

// 0x41dde0  Confidence: exact (VC6 byte match, cl 8966; was likely: if / else if against a two-case switch; both
// give the cmp / je chain, the
// two calls are tail jumps).
void elevator_popup_frame()
{
    if (!(game_flags & GF_MULTIPLAYER))
        return;
    switch (cur_level_index) {
    case ELEVATOR_LEVEL_IONAEXT:
        elevator_offer_ride();
        break;
    case ELEVATOR_LEVEL_ELEH:
        elevator_choose_floor();
        break;
    }
}

// 0x41de10  Confidence: exact.
void elevator_offer_ride()
{
    char *buttons[2];

    buttons[0] = XSTR("Yes", 654);
    buttons[1] = XSTR("No", 655);
    if (ui_message_box_is_active())
        return;
    if (script_player_near_navpoint("$npc117", ELEVATOR_RANGE))
        ui_message_box_show(XSTR("Elevator", 821), XSTR("Go up?", 822), -1, -1, 2, buttons,
                            elevator_go_up_cb, 0);
    else if (script_player_near_navpoint("$npc118", ELEVATOR_RANGE))
        ui_message_box_show(XSTR("Elevator", 821), XSTR("Go down?", 823), -1, -1, 2, buttons,
                            elevator_go_down_cb, 0);
}

// 0x41dec0  Confidence: exact (the ternary: `neg / sbb / add 4`; here the start group is looked up before
// the party_cycle test, in the other callbacks after it). Yes = start group 4, No = 3.
void elevator_go_up_cb(int button)
{
    int saved_entry = level_entry_point;
    int cycled;

    level_entry_point = (button == 0) ? 4 : 3;
    level_find_player_starts();
    cycled = 0;
    if (cur_character == cur_summon) {
        party_cycle(1);
        cycled = 1;
    }
    cur_character->place_at_party_start(mp_local_player->id);
    camera_reset_to_player(false);
    mp_send_obj_update(cur_character, 1);
    if (summon_player && summon_player->mp_owner == mp_local_player)
        elevator_move_party(mp_local_player->id);
    level_entry_point = saved_entry;
    level_find_player_starts();
    if (cycled)
        party_cycle(1);
}

// 0x41df70  Confidence: exact. The summon goes to the start after its owner's ((id + 1) % 4, signed
// remainder) and loses its start navpoint.
void elevator_move_party(int player_id)
{
    cur_summon->place_at_party_start((player_id + 1) % 4);
    mp_send_obj_update(cur_summon, 1);
    cur_summon->start_navpoint = NULL;
}

// 0x41dfb0  Confidence: exact (`setne / add 3`). Yes = start group 3, No = 4.
void elevator_go_down_cb(int button)
{
    int saved_entry = level_entry_point;
    int cycled;

    level_entry_point = (button == 0) ? 3 : 4;
    cycled = 0;
    if (cur_character == cur_summon) {
        party_cycle(1);
        cycled = 1;
    }
    level_find_player_starts();
    cur_character->place_at_party_start(mp_local_player->id);
    camera_reset_to_player(false);
    mp_send_obj_update(cur_character, 1);
    if (summon_player && summon_player->mp_owner == mp_local_player)
        elevator_move_party(mp_local_player->id);
    level_entry_point = saved_entry;
    level_find_player_starts();
    if (cycled)
        party_cycle(1);
}

// 0x41e060  Confidence: exact (the three tables are one 9-entry frame at esp, +0xc, +0x18; written as
// three arrays in that order; the stores interleave, the shared ui_message_box_show call is tail-merged).
// $npc106 is the ground floor, $npclg2 the lower, $npclg3 the upper one.
void elevator_choose_floor()
{
    char *from_ground[3];
    char *from_lower[3];
    char *from_upper[3];

    from_ground[0] = XSTR("Upper", 824);
    from_ground[1] = XSTR("Lower", 825);
    from_ground[2] = XSTR("Cancel", 696);
    from_lower[0] = XSTR("Upper", 824);
    from_lower[1] = XSTR("Ground", 826);
    from_lower[2] = XSTR("Cancel", 696);
    from_upper[0] = XSTR("Ground", 826);
    from_upper[1] = XSTR("Lower", 825);
    from_upper[2] = XSTR("Cancel", 696);
    if (ui_message_box_is_active())
        return;
    if (script_player_near_navpoint("$npc106", ELEVATOR_RANGE))
        ui_message_box_show(XSTR("Elevator", 821), XSTR("Which floor?", 827), -1, -1, 3, from_ground,
                            elevator_floor_cb_a, 0);
    else if (script_player_near_navpoint("$npclg2", ELEVATOR_RANGE))
        ui_message_box_show(XSTR("Elevator", 821), XSTR("Which floor?", 827), -1, -1, 3, from_lower,
                            elevator_floor_cb_b, 0);
    else if (script_player_near_navpoint("$npclg3", ELEVATOR_RANGE))
        ui_message_box_show(XSTR("Elevator", 821), XSTR("Which floor?", 827), -1, -1, 3, from_upper,
                            elevator_floor_cb_c, 0);
}

// 0x41e140  Confidence: exact (the ternary: `dec / neg / sbb / add 4`). Upper 5, Lower 4, Cancel 3 (the
// ground floor's own start: Cancel also moves the party).
void elevator_floor_cb_a(int button)
{
    int saved_entry = level_entry_point;
    int cycled;

    if (button == 0)
        level_entry_point = 5;
    else
        level_entry_point = (button == 1) ? 4 : 3;
    cycled = 0;
    if (cur_character == cur_summon) {
        party_cycle(1);
        cycled = 1;
    }
    level_find_player_starts();
    cur_character->place_at_party_start(mp_local_player->id);
    camera_reset_to_player(false);
    mp_send_obj_update(cur_character, 1);
    if (summon_player && summon_player->mp_owner == mp_local_player)
        elevator_move_party(mp_local_player->id);
    level_entry_point = saved_entry;
    level_find_player_starts();
    if (cycled)
        party_cycle(1);
}

// 0x41e200  Confidence: exact (`cmp 1 / setne / add 3`). Upper 5, Ground 3, Cancel 4.
void elevator_floor_cb_b(int button)
{
    int saved_entry = level_entry_point;
    int cycled;

    if (button == 0)
        level_entry_point = 5;
    else
        level_entry_point = (button == 1) ? 3 : 4;
    cycled = 0;
    if (cur_character == cur_summon) {
        party_cycle(1);
        cycled = 1;
    }
    level_find_player_starts();
    cur_character->place_at_party_start(mp_local_player->id);
    camera_reset_to_player(false);
    mp_send_obj_update(cur_character, 1);
    if (summon_player && summon_player->mp_owner == mp_local_player)
        elevator_move_party(mp_local_player->id);
    level_entry_point = saved_entry;
    level_find_player_starts();
    if (cycled)
        party_cycle(1);
}

// 0x41e2c0  Confidence: exact (`cmp 1 / setne / add 4`). Ground 3, Lower 4, Cancel 5.
void elevator_floor_cb_c(int button)
{
    int saved_entry = level_entry_point;
    int cycled;

    if (button == 0)
        level_entry_point = 3;
    else
        level_entry_point = (button == 1) ? 4 : 5;
    cycled = 0;
    if (cur_character == cur_summon) {
        party_cycle(1);
        cycled = 1;
    }
    level_find_player_starts();
    cur_character->place_at_party_start(mp_local_player->id);
    camera_reset_to_player(false);
    mp_send_obj_update(cur_character, 1);
    if (summon_player && summon_player->mp_owner == mp_local_player)
        elevator_move_party(mp_local_player->id);
    level_entry_point = saved_entry;
    level_find_player_starts();
    if (cycled)
        party_cycle(1);
}

// 0x41e380  Confidence: exact. Only in a network game, and not while leaving it: a screen opening there does
// not pause the game, so the character is stopped (and SWI!DIR sent) instead.
void ui_screen_stop_player()
{
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && !(mp_local_player->flags & MP_PF_LEAVING)) {
        player_input[0] = 0;
        cur_character->ai.goals_clear();
        cur_character->ai.go_idle();
        mp_send_swi_ndir();
    }
}
