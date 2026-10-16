// new_tutorial.cpp - Sum.exe v1.40, code 0x42e630-0x42f1df (the tutorial hint boxes of the first levels:
// basic controls and basic combat on masad, Flece joining in the sewers, the first summoning ring in the
// palace interior; the Ignore_tutorial console command).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x592a20-0x592c6b (box layout tables, the entry table, the literals), .bss 0x5f9610-0x5f966f.
//
// Each entry names a level, a level script slot and an interface screen; tutorial_frame (called from
// hud_frame) shows the first entry whose condition callback returns true. The condition sets its savegame
// flag itself, so every hint is shown once. A shown hint pauses the game; Space or a click on the box
// turns the page, Esc closes it.

#include <string.h>
#include <stdio.h>
#include "vsdk/gr/gr.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/console.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/rendering/levelrender.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/interface/new_tutorial.h"

#define TUTORIAL_NUM_ENTRIES 5
#define TUTORIAL_MAX_LINES   16      // gr_split_str max_lines; the line arrays have 20 slots (0x50 bytes)

// A reference to a localize_strings slot: binding the reference is what makes each page table a dynamic
// initialiser (init_strptrs_5f96xx), as in new_help.cpp / new_status.cpp.
struct tutorial_str_ref {
    char *&s;
#ifdef _MSC_VER
    tutorial_str_ref(char *&r) : s(r) {}   // VC6 rejects { XSTR(..) } for a reference member otherwise (D4902)
#endif
};

// One hint (0x24 bytes; type and field names ours).
struct tutorial_entry {
    bool (*cond)();               // +0x00 true once: shows the hint (and sets its savegame flag)
    tutorial_str_ref *pages;      // +0x04 page texts
    int num_pages;                // +0x08
    int level;                    // +0x0c cur_level_index
    int script;                   // +0x10 level_script_find_index(level, level_script_name)
    int screen;                   // +0x14 interface_get_screen (0 = the HUD)
    int x;                        // +0x18 box position and width (tutorial_box_pos / tutorial_box_width)
    int y;                        // +0x1c
    int w;                        // +0x20
};

bool tutorial_cond_basic_controls();
bool tutorial_cond_combat_part1();
bool tutorial_cond_combat_part2();
bool tutorial_cond_sewer_flece();
bool tutorial_cond_ring();
void cmd_Ignore_tutorial();

// ---- .bss (0x5f9610-0x5f966f) ----------------------------------------------------------------------
// The definition order below is the static-initialiser order (entries 83..89); .bss puts the console
// command first.
console_command_t ccmd_Ignore_tutorial("Ignore_tutorial", "Toggles Ignore_tutorial", cmd_Ignore_tutorial); // 0x5f9610
bool tutorial_was_paused;                                     // 0x5f961c
int  tutorial_page;                                           // 0x5f9620
bool tutorial_drawn;                                          // 0x5f9624 (*new*) set by tutorial_draw, never read
bool Ignore_tutorial;                                         // 0x5f9625

// 0x42e630 init_strptrs_5f9628_thunk (jmp, static_initializers entry 83). Confidence: exact.
// 0x42e640 init_strptrs_5f9628: the slot stores of the table below (0x5f9628). Confidence: likely (as new_help.cpp).
tutorial_str_ref tutorial_controls_pages[4] = {                                     // (*new*)
    { XSTR("Basic Controls\n\nWelcome to Summoner! This tutorial explain...", 913) },
    { XSTR("There are two ways to move in the world of Summoner. You can...", 915) },
    { XSTR("The second mode of travel is called direct travel and you ca...", 916) },
    { XSTR("Camera Control\n\nBy moving your mouse pointer to the right ...", 917) },
};
// 0x42e670 init_strptrs_5f9638_thunk (jmp, entry 84). Confidence: exact.
// 0x42e680 init_strptrs_5f9638 (0x5f9638). Confidence: likely.
tutorial_str_ref tutorial_combat1_pages[4] = {
    { XSTR("Basic Combat Tutorial\n\nOne of the Orenians has killed a vi...", 918) },
    { XSTR("You can attack an enemy by highlighting him with your mouse ...", 920) },
    { XSTR("Once a target has been selected Joseph will attack and defen...", 921) },
    { XSTR("During combat numbers will appear over the participants' hea...", 922) },
};
// 0x42e6b0 init_strptrs_5f9648_thunk (jmp, entry 85). Confidence: exact.
// 0x42e6c0 init_strptrs_5f9648 (0x5f9648). Confidence: likely.
tutorial_str_ref tutorial_combat2_pages[2] = {
    { XSTR("Joseph has defeated the Scout!\n\n Sometimes when enemies ar...", 923) },
    { XSTR("Joseph may have been injured in battle. However, he can be h...", 924) },
};
// 0x42e6e0 init_strptrs_5f9650_thunk (jmp, entry 86). Confidence: exact.
// 0x42e6f0 init_strptrs_5f9650 (0x5f9650). Confidence: likely.
tutorial_str_ref tutorial_sewer_flece_pages[3] = {
    { XSTR("Flece has now joined your party!\n\n Now you have two charac...", 925) },
    { XSTR("Party members have their own unique set of skills and abilit...", 926) },
    { XSTR("You can also go into solo mode, by clicking the Solo button.", 927) },
};
// 0x42e710 init_strptrs_5f965c_thunk (jmp, entry 87). Confidence: exact.
// 0x42e720 init_strptrs_5f965c (0x5f965c). Confidence: likely.
tutorial_str_ref tutorial_ring_pages[4] = {
    { XSTR("Joseph has regained his ability to Summon creatures!\n\n To ...", 928) },
    { XSTR("As Joseph gains more summoning rings he'll gain access to mo...", 929) },
    { XSTR("You can have your summon banished in one of three ways. It c...", 930) },
    { XSTR("Be careful! If Joseph is ever defeated in battle the summon ...", 931) },
};
tutorial_entry *tutorial_current;                             // 0x5f966c 0 = no hint shown

// ---- .data (0x592a20-) -----------------------------------------------------------------------------
int tutorial_box_width[3] = { 480, 600, 768 };                                       // 0x592a20 (*new*)
// [resolution_index][entry] x, y. Only the basic combat hint (entry 1) sits lower.
int tutorial_box_pos[3][TUTORIAL_NUM_ENTRIES][2] = {                                 // 0x592a2c (*new*)
    { { 60, 60 }, { 60, 200 }, { 60, 60 }, { 60, 60 }, { 60, 60 } },
    { { 75, 75 }, { 75, 250 }, { 75, 75 }, { 75, 75 }, { 75, 75 } },
    { { 96, 96 }, { 96, 320 }, { 96, 96 }, { 96, 96 }, { 96, 96 } },
};
int tutorial_unused_592aa4 = 0;                               // 0x592aa4 (*new*) never referenced (see NOTES)
// 0x42e750 init_tutorial_entries_thunk (jmp, entry 88). Confidence: exact.
// 0x42e760 init_tutorial_entries: the run-time part of the table below. Confidence: likely.
// 0x592aa8. The x / y / w members read resolution_index, so the table is initialised at run time from the
// first of them on (init_tutorial_entries 0x42e760, thunk 0x42e750, entry 88): VC6 keeps the constant
// prefix (entry 0 +0x00..+0x14) in .data and stores everything after it, constants included. Confidence:
// likely (the store order of the initialiser is the compiler's).
tutorial_entry tutorial_entries[TUTORIAL_NUM_ENTRIES] = {
    { tutorial_cond_basic_controls, tutorial_controls_pages, 4, 3, 0, 0,
      tutorial_box_pos[resolution_index][0][0], tutorial_box_pos[resolution_index][0][1],
      tutorial_box_width[resolution_index] },
    { tutorial_cond_combat_part1, tutorial_combat1_pages, 4, 3, 0, 0,
      tutorial_box_pos[resolution_index][1][0], tutorial_box_pos[resolution_index][1][1],
      tutorial_box_width[resolution_index] },
    { tutorial_cond_combat_part2, tutorial_combat2_pages, 2, 3, 0, 0,
      tutorial_box_pos[resolution_index][2][0], tutorial_box_pos[resolution_index][2][1],
      tutorial_box_width[resolution_index] },
    { tutorial_cond_sewer_flece, tutorial_sewer_flece_pages, 3, 9, 0, 0,
      tutorial_box_pos[resolution_index][3][0], tutorial_box_pos[resolution_index][3][1],
      tutorial_box_width[resolution_index] },
    { tutorial_cond_ring, tutorial_ring_pages, 4, 0x18, 0, 0,
      tutorial_box_pos[resolution_index][4][0], tutorial_box_pos[resolution_index][4][1],
      tutorial_box_width[resolution_index] },
};
// 0x42e8e0 init_ccmd_Ignore_tutorial_thunk (jmp, entry 89). Confidence: exact.
// 0x42e8f0 init_ccmd_Ignore_tutorial: the constructor call of ccmd_Ignore_tutorial. Confidence: exact
//          (compiler-generated).

// 0x42e910  Confidence: exact (the automap toggles' template).
void cmd_Ignore_tutorial()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Ignore_tutorial = true;
        else if (arg_type & ARG_FALSE)
            Ignore_tutorial = false;
        else if (arg_type & ARG_NONE)
            Ignore_tutorial ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "Ignore_tutorial", "Ignore_tutorial");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Ignore_tutorial", Ignore_tutorial ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x42e9d0  Confidence: exact. Half a second into the level after the intro.
bool tutorial_cond_basic_controls()
{
    if (!savegame_state.get_flag(cur_level_index, "masad_basic_controls_tutorial")
        && savegame_state.get_flag(cur_level_index, "masad_intro")
        && !(level_session_time < 0.5f)) {
        savegame_state.set_flag(cur_level_index, "masad_basic_controls_tutorial", 1);
        return true;
    }
    return false;
}

// 0x42ea40  Confidence: exact. The scout must be alive (type 5) and within 5 units.
bool tutorial_cond_combat_part1()
{
    if (!savegame_state.get_flag(cur_level_index, "masad_basic_combat_tutorial_part1")
        && savegame_state.get_flag(cur_level_index, "masad_basic_controls_tutorial")) {
        int obj = script_object_find("Orenian Scout#$npc002");

        if (obj >= 0 && script_object_is_type5(obj) && script_npc_near_player("Orenian Scout#$npc002", 5.0f)) {
            savegame_state.set_flag(cur_level_index, "masad_basic_combat_tutorial_part1", 1);
            return true;
        }
    }
    return false;
}

// 0x42ead0  Confidence: likely (the found test after the loop is duplicated as in status_init; the two
// out-arguments of two_stage_get are bools by their stack slots +3 / +4). When the dead scout has finished
// his death move, the door "invis-door01" is opened for good (two-stage state open, the lobj moved to the
// door's open pose) and the fire effect in front of it is switched off.
bool tutorial_cond_combat_part2()
{
    Character *c;

    if (savegame_state.get_flag(cur_level_index, "masad_basic_combat_tutorial_part2")
        || !savegame_state.get_flag(cur_level_index, "masad_basic_combat_tutorial_part1"))
        return false;

    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
        if (!_stricmp(c->get_name(), "Orenian Scout#$npc002"))
            break;
    }
    if (c == &living_list_inactive || c->get_current_move())
        return false;

    {
        lobj *l;
        door_info *door;
        bool open, flag2;

        savegame_state.set_flag(cur_level_index, "masad_basic_combat_tutorial_part2", 1);
        l = &lobjs[lobj_find_by_name("invis-door01")];
        door = door_from_lobj(l);
        savegame_state.two_stage_get(&open, &flag2, l);
        savegame_state.two_stage_set(true, flag2, l);
        l->pos = door->open_pos;
        door->open_orient.to_matrix_b(&l->orient);
        fx_disable_by_effect_name("masad-firedoor2.vfx");
    }
    return true;
}

// 0x42ec00  Confidence: likely (loop exit duplicated as above).
bool tutorial_cond_sewer_flece()
{
    Player *p;

    if (savegame_state.get_flag(cur_level_index, "sewer_flece_tutorial"))
        return false;
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (living_from_handle(p->handle)->info->species == SPECIES_FLECE)
            break;
    }
    if (p == &party_list)
        return false;
    if (level_session_time < 0.5f)
        return false;
    savegame_state.set_flag(cur_level_index, "sewer_flece_tutorial", 1);
    return true;
}

// 0x42ec90  Confidence: exact.
bool tutorial_cond_ring()
{
    if (!savegame_state.get_flag(cur_level_index, "lpalaceint_ring_tutorial_part1")
        && savegame_state.get_flag(cur_level_index, "got_ring_of_darkness")) {
        savegame_state.set_flag(cur_level_index, "lpalaceint_ring_tutorial_part1", 1);
        return true;
    }
    return false;
}

// 0x42ecf0  Confidence: exact. From interface_load_all.
void tutorial_reset()
{
    tutorial_current = 0;
}

// 0x42ed00  Confidence: exact.
void tutorial_show(int entry)
{
    if (entry < 0 || entry >= TUTORIAL_NUM_ENTRIES)
        return;
    tutorial_page = 0;
    tutorial_current = &tutorial_entries[entry];
    tutorial_current->x = tutorial_box_pos[resolution_index][entry][0];
    tutorial_current->y = tutorial_box_pos[resolution_index][entry][1];
    tutorial_current->w = tutorial_box_width[resolution_index];
    tutorial_was_paused = game_is_paused();
    if (!tutorial_was_paused)
        game_pause(0, 0);
}

// 0x42ed90  Confidence: exact (tail jump to game_unpause).
void tutorial_close()
{
    tutorial_current = 0;
    if (!tutorial_was_paused)
        game_unpause();
}

// 0x42edb0  Confidence: exact. The box rect of the current page; its height is the wrapped line count.
void tutorial_layout(int *x, int *y, int *w, int *h, int *offsets, int *lengths)
{
    int font_h;
    tutorial_str_ref *pages;

    *h = 0;
    *w = 0;
    *y = 0;
    *x = 0;
    font_h = gr_get_font_height(-1);
    if (tutorial_current && (pages = tutorial_current->pages) != 0) {
        int lines = gr_split_str(lengths, offsets, localize_string_or_invalid(&pages[tutorial_page].s),
                                 tutorial_current->w - 2 * ui_frame_border[resolution_index]
                                     - ui_frame_inset_x[resolution_index],
                                 TUTORIAL_MAX_LINES, 0, -1);

        *x = tutorial_current->x;
        *y = tutorial_current->y;
        *w = tutorial_current->w;
        *h = lines * font_h + 2 * (ui_frame_inset_y[resolution_index] + ui_frame_border[resolution_index]);
    }
}

// 0x42eea0  Confidence: exact. The whole box is the OK area.
bool tutorial_ok_hit_test()
{
    int y, x, w, h;
    int lengths[20];
    int offsets[20];

    tutorial_layout(&x, &y, &w, &h, offsets, lengths);
    return point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h);
}

// 0x42ef00  Confidence: likely (the Space test goes through a bool: sete / test).
bool tutorial_frame(int key)
{
    bool used = false;
    int screen = interface_get_screen();
    int i;

    if (!level_entered_by_load || Ignore_tutorial)
        return false;

    for (i = 0; i < TUTORIAL_NUM_ENTRIES; i++) {
        tutorial_entry *t = &tutorial_entries[i];

        if (t->screen == screen && t->level == cur_level_index
            && t->script == level_script_find_index(cur_level_index, level_script_name) && t->cond()) {
            tutorial_show(i);
            break;
        }
    }

    if (tutorial_current && tutorial_current->pages) {
        bool next = key == KEY_SPACE;

        used = true;
        if (next || (mouse_was_pressed(0) && tutorial_ok_hit_test())) {
            sound_play_2d(0x23, 1.0f);
            if (++tutorial_page >= tutorial_current->num_pages) {
                tutorial_close();
                mouse_flush();
            }
        } else if (key == KEY_ESC) {
            sound_play_2d(0x24, 1.0f);
            tutorial_close();
            mouse_flush();
        }
    }
    return used;
}

// 0x42f020  Confidence: likely (tutorial_layout's code inlined by hand, not a call; line buffer size from
// the frame).
void tutorial_draw()
{
    int font_h = gr_get_font_height(-1);
    int lengths[TUTORIAL_MAX_LINES];
    int offsets[TUTORIAL_MAX_LINES];
    char line[256];

    if (level_entered_by_load && !Ignore_tutorial && tutorial_current && tutorial_current->pages) {
        int lines = gr_split_str(lengths, offsets,
                                 localize_string_or_invalid(&tutorial_current->pages[tutorial_page].s),
                                 tutorial_current->w - 2 * ui_frame_border[resolution_index]
                                     - ui_frame_inset_x[resolution_index],
                                 TUTORIAL_MAX_LINES, 0, -1);
        int i, dy;

        ui_draw_text_box(tutorial_current->x, tutorial_current->y, tutorial_current->w,
                         lines * font_h + 2 * (ui_frame_inset_y[resolution_index] + ui_frame_border[resolution_index]));
        gr_set_color(220, 220, 220, 220);
        for (i = 0, dy = 0; i < lines; i++, dy += font_h) {
            strncpy(line, localize_string_or_invalid(&tutorial_current->pages[tutorial_page].s) + offsets[i],
                    lengths[i]);
            line[lengths[i]] = 0;
            gr_string(tutorial_current->x + ui_frame_inset_x[resolution_index] + ui_frame_border[resolution_index],
                      tutorial_current->y + ui_frame_inset_y[resolution_index] + dy + ui_frame_border[resolution_index],
                      line, -1, gr_string_mode);
        }
        tutorial_drawn = true;
    }
}

// 0x42f1c0  Confidence: exact.
bool tutorial_is_active()
{
    return tutorial_current != 0;
}

// 0x42f1d0  Confidence: exact (the same code as tutorial_is_active, not folded: the arguments are ignored).
bool tutorial_mouse_in_panel(int x, int y)
{
    return tutorial_current != 0;
}
