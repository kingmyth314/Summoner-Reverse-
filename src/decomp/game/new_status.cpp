// new_status.cpp - Sum.exe v1.40, code 0x42b1c0-0x42e62e (screen 2, the Character screen: level and stats,
// skills with +/- buttons and chain keys, resistances, status effects, the 3D model view, the AI combat-style
// popup and the chain-attack key popup).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x5807a0-0x580d93 (button table, layout tables), .data 0x5927e0-0x592a1f (layout tables,
// bitmap name tables, strings), .bss 0x5f5aa8-0x5f960b. status_character (0x5bd810) belongs to the
// interface unit, so it is extern here.
//
// Skill rows (status_skill_entries) are the Player's skill progression entries reached at the character's
// level, then the chain attacks the species has unlocked (is_chain). Spent points stay pending (+0x10) until
// status_apply_skill_points; leaving the screen applies them when status_skills_dirty is set, else returns
// them to Player.skill_points.

#include <string.h>
#include <stdio.h>
#include <float.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/living_entity.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_buysell.h"
#include "summoner/interface/new_status.h"
#include "experimental/ui_shift.h"   // EXPERIMENTAL_BUILD only (widescreen); keep it the last include

#define STATUS_NUM_BUTTONS 21
#define STATUS_MAX_SKILLS  35

// One row of the skill list (0x14 bytes).
struct status_skill_entry {
    bool is_chain;        // +0x00 a chain attack (no level, no +/- buttons; right click picks its key)
    int  skill;           // +0x04 skill id (skill_table index)
    int  base_level;      // +0x08 Character.skill byte * 0.1, rounded (without equipment)
    int  level;           // +0x0c get_skill * 10, rounded (with equipment); the difference is the bonus colour
    int  pending;         // +0x10 points spent on this screen, not yet applied
};

// A reference to a localize_strings slot (see new_help.cpp: binding the reference is what makes the table a
// dynamic initialiser).
struct status_str_ref {
    char *&s;
#ifdef _MSC_VER
    status_str_ref(char *&r) : s(r) {}   // VC6 rejects { XSTR(..) } for a reference member otherwise (D4902)
#endif
};

// ---- .rdata --------------------------------------------------------------------------------
// 0x5807a0 (*new* name), 21 x 0x40. flags: 1 HiLite acting on release through the Press entry after it,
// 2 HiLite acting on press with auto-repeat (the two scroll arrows). 1-4 the species name tabs (flag bit 0
// set by status_update_species_tab), 5/6 inventory, 7/8 shop / journal / inventory, 9/10 previous member,
// 11/12 next member, 13-16 scroll arrows, 17/18 combat style popup, 19/20 close.
const ui_button_def status_button_defs[STATUS_NUM_BUTTONS] = {
    { "Skl-Bkgrnd",         { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Inv-Name-Joseph",    { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Inv-Name-Jekhar",    { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Inv-Name-Flece",     { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Inv-Name-Rosalind",  { { 113, 48 },  { 141, 60 },  { 176, 76 } },  0 },
    { "Icon-ArrwLFHilite",  { { 21, 23 },   { 28, 29 },   { 33, 35 } },   1 },
    { "Icon-ArrwLFPress",   { { 21, 23 },   { 28, 29 },   { 33, 35 } },   0 },
    { "Icon-ArrwRTHilite",  { { 295, 23 },  { 369, 29 },  { 470, 35 } },  1 },
    { "Icon-ArrwRTPress",   { { 295, 23 },  { 369, 29 },  { 470, 35 } },  0 },
    { "Icon-ArrwLFHilite",  { { 21, 61 },   { 28, 76 },   { 33, 95 } },   1 },
    { "Icon-ArrwLFPress",   { { 21, 61 },   { 28, 76 },   { 33, 95 } },   0 },
    { "Icon-ArrwRTHilite",  { { 295, 61 },  { 369, 76 },  { 470, 95 } },  1 },
    { "Icon-ArrwRTPress",   { { 295, 61 },  { 369, 76 },  { 470, 95 } },  0 },
    { "Icon-ArrwUPHilite",  { { 599, 95 },  { 750, 119 }, { 957, 147 } }, 2 },
    { "Icon-ArrwUPPress",   { { 599, 95 },  { 750, 119 }, { 957, 147 } }, 0 },
    { "Icon-ArrwDNHilite",  { { 599, 213 }, { 750, 267 }, { 957, 337 } }, 2 },
    { "Icon-ArrwDNPress",   { { 599, 213 }, { 750, 267 }, { 957, 337 } }, 0 },
    { "Icon-ArrwBRTHilite", { { 83, 448 },  { 104, 560 }, { 130, 714 } }, 1 },
    { "Icon-ArrwBRTPress",  { { 83, 448 },  { 104, 560 }, { 130, 714 } }, 0 },
    { "Icon-CloseHilite",   { { 599, 19 },  { 749, 24 },  { 959, 31 } },  1 },
    { "Icon-ClosePress",    { { 599, 19 },  { 749, 24 },  { 959, 31 } },  0 },
};
// Per resolution_index (640, 800, 1024); all names *new*.
const int status_level_pos[3][2]          = { { 57, 257 }, { 71, 321 }, { 91, 411 } };    // 0x580ce0
const int status_skill_list_pos[3][2]     = { { 342, 88 }, { 428, 109 }, { 547, 141 } }; // 0x580cf8 first row
const int status_resist_pos[3][2]         = { { 342, 310 }, { 428, 388 }, { 547, 496 } }; // 0x580d10 fire..magic
const int status_resist2_pos[3][2]        = { { 485, 310 }, { 606, 388 }, { 776, 496 } }; // 0x580d28 slashing..blunt
const int status_resist_value_right[3]    = { 120, 150, 192 };                             // 0x580d40
const int status_effects_pos[3][2]        = { { 342, 410 }, { 428, 513 }, { 547, 656 } }; // 0x580d4c
const int status_effects_width[3]         = { 283, 354, 453 };                             // 0x580d64 wrap width
const int status_combat_style_pos[3][2]   = { { 141, 448 }, { 176, 560 }, { 226, 717 } }; // 0x580d70
const int status_ai_popup_dy[3]           = { -100, -100, -100 };                          // 0x580d88
// 0x580d94 is the 0.02f literal of status_draw_resistances (a __real COMDAT that living_entity.cpp and others
// also use).

// ---- .data ---------------------------------------------------------------------------------
int status_model_view_xy[3][2]      = { { 77, 86 }, { 96, 108 }, { 123, 138 } };          // 0x5927e0
int status_model_view_wh[3][2]      = { { 192, 162 }, { 240, 203 }, { 307, 259 } };       // 0x5927f8
int status_model_offset[3][2]       = { { -65, -91 }, { -81, -114 }, { -104, -147 } };    // 0x592810 (*new*)
int status_model_offset_mp[3][2]    = { { -50, 0 }, { -63, 0 }, { -80, 0 } };             // 0x592828 (*new*) species 1 in MP
int status_level_value_right[3]     = { 237, 296, 379 };                                  // 0x592840
int status_xp_bar_pos[3][2]         = { { 71, 310 }, { 89, 388 }, { 114, 496 } };         // 0x59284c (*new*)
int status_xp_bar_size[3][2]        = { { 209, 11 }, { 261, 13 }, { 334, 18 } };          // 0x592864 (*new*)
int status_stats_pos[3][2]          = { { 57, 330 }, { 71, 417 }, { 91, 528 } };          // 0x59287c (*new*)
int status_stats_value_right[3]     = { 237, 296, 379 };                                  // 0x592894
int status_hp_bar_pos[3][2]         = { { 96, 333 }, { 120, 416 }, { 154, 533 } };        // 0x5928a0 (*new*)
int status_ap_bar_pos[3][2]         = { { 96, 349 }, { 120, 436 }, { 154, 558 } };        // 0x5928b8 (*new*)
int status_bar_size[3][2]           = { { 109, 9 }, { 136, 11 }, { 174, 14 } };           // 0x5928d0 (*new*)
int status_skill_row_right[3]       = { 248, 309, 397 };                                  // 0x5928e8 (a width)
int status_page_size[3]             = { 10, 10, 10 };                                     // 0x5928f4
// [ui_hires]; the literals sit at 0x592974-0x5929fb in reverse order (VC6 emits the last one first).
const char *status_skill_row_names[2]     = { "Icon-SklMod640.tga", "Icon-SklMod1024.tga" };        // 0x592900 (*new*)
const char *status_skill_up_names[2]      = { "Icon-SklMod-UP640.tga", "Icon-SklMod-UP1024.tga" };  // 0x592908 (*new*)
const char *status_skill_dn_names[2]      = { "Icon-SklMod-DN640.tga", "Icon-SklMod-DN1024.tga" };  // 0x592910 (*new*)
// x, y of the +/- button pair relative to the row; sum.symbols names the members status_skill_button_x / _y.
int status_skill_button_pos[3][2]   = { { 195, 0 }, { 244, 1 }, { 312, 1 } };             // 0x592918 (*new*)
// The strings are the pooled copies at 0x588b8c / 0x588b74 (new_assess.cpp's .data).
const char *status_scrollbar_names[2]     = { "ScrollBarMid640.tga", "ScrollBarMid1024.tga" };  // 0x592930 (*new*)
// Track top x, y; sum.symbols names the y member status_scrollbar_track_y (0x59293c).
int status_scrollbar_track[3][2]    = { { 602, 129 }, { 753, 160 }, { 963, 200 } };       // 0x592938 (*new*)
int status_scrollbar_track_height[3] = { 74, 95, 122 };                                   // 0x592950
int status_skill_points_pos[3][2]   = { { 478, 250 }, { 598, 313 }, { 765, 400 } };       // 0x59295c (*new*) centre x, y
// 0x5929fc "status_cameras.tbl", 0x592a10 "%d%%", 0x592a18 "%d/", 0x592a1c ", " are this unit's literals;
// "%d" (0x589820), "%d/%d" (0x58ac40) and "" (0x5bd0d0) are pooled copies in other units.

// ---- .bss ----------------------------------------------------------------------------------
int status_skills_dirty;                                      // 0x5f5aa8
int status_scrollbar_bm;                                      // 0x5f5aac
new_icam_table status_cameras;                                // 0x5f5ab0 (init_empty_42b1d0)
status_skill_entry status_skill_entries[STATUS_MAX_SKILLS];   // 0x5f90d8
int status_thumb_x;                                           // 0x5f9394
int status_thumb_y;                                           // 0x5f9398
int status_num_skills;                                        // 0x5f939c
unsigned int status_flags;                                    // 0x5f93a0 1 unpause, 2 refresh statscreen, 4 clear shopkeeper
int status_pressed_button;                                    // 0x5f93a4
int status_scroll_pos;                                        // 0x5f93a8
bool status_skill_buttons_shown;                              // 0x5f93ac (*new*) latch of status_get_skill_points
RealtimeTimer status_button_timer;                            // 0x5f93b0
int status_mouse_mode;                                        // 0x5f93b4 0 idle, 1 button, 2 drag, 3 AI popup, 4 +/-, 5 chain popup
int status_skill_row_bm;                                      // 0x5f93b8 "Icon-SklMod" (the +/- pair, idle)
int status_drag_start_mouse_y;                                // 0x5f93bc
int status_pending_screen;                                    // 0x5f93c0
int status_unused_5f93c4;                                     // 0x5f93c4 never referenced (*new*, see NOTES)
ui_button_state status_buttons[STATUS_NUM_BUTTONS];           // 0x5f93c8
int status_hover_row;                                         // 0x5f95c0
bool status_skill_button_pressed;                             // 0x5f95c4
int status_thumb_bm_w;                                        // 0x5f95c8
int status_thumb_bm_h;                                        // 0x5f95cc
int status_skill_button_bm;                                   // 0x5f95d0 "Icon-SklMod-UP" (+ pressed)
int status_thumb_w;                                           // 0x5f95d4
int status_thumb_h;                                           // 0x5f95d8
bool status_scrollbar_visible;                                // 0x5f95dc
int status_saved_gr_offset_x;                                 // 0x5f95e0
int status_saved_gr_offset_y;                                 // 0x5f95e4
int status_skill_button2_bm;                                  // 0x5f95e8 "Icon-SklMod-DN" (- pressed)
bool status_skill_button_side;                                // 0x5f95ec true: left half (+), false: right half (-)
int status_drag_start_thumb_y;                                // 0x5f95f0
int status_skill_row_bm_w;                                    // 0x5f95f4
int status_skill_row_bm_h;                                    // 0x5f95f8
// 0x5f95fc, built by init_strptrs_5f95fc (0x42b1f0, thunk 0x42b1e0). Confidence: likely (as new_help.cpp).
status_str_ref status_modifier_key_names[4] = {
    { XSTR("CAPS", 734) }, { XSTR("SHFT", 735) }, { XSTR("CTRL", 736) }, { XSTR("ALT", 737) },
};

// The dynamic initialisers (compiler-generated):
// 0x42b1c0 init_empty_42b1d0_thunk (jmp, static_initializers entry 80). Confidence: exact.
// 0x42b1d0 init_empty_42b1d0 (a bare ret): read here as the dynamic initialiser of status_cameras, whose
//          constructor is inline and empty (see NOTES). Confidence: unsure (the pairing).
// 0x42b1e0 init_strptrs_5f95fc_thunk (jmp, entry 81). Confidence: exact.
// 0x42b1f0 init_strptrs_5f95fc: the four slot stores of status_modifier_key_names. Confidence: likely.
// 0x42b220 init_realtimetimer_5f93b0_thunk (jmp, entry 82). Confidence: exact.
// 0x42b230 init_realtimetimer_5f93b0 (status_button_timer: mov ecx, 0x5f93b0 / jmp RealtimeTimer::RealtimeTimer).
//          Confidence: exact.

// Value colour of a stat that equipment changes: blue above, red below the base. Inlined at every use
// (fild of the int, compares with the 0.0f literal 0x57eb84).
static inline void status_set_bonus_color(float bonus)
{
    if (bonus > 0.0f)
        gr_set_color_ptr(&ui_color_blue);
    else if (bonus < 0.0f)
        gr_set_color_ptr(&ui_color_red);
    else
        gr_set_color_ptr(&ui_color_text);
}

// 0x42b240  Confidence: exact.
void status_cameras_parse_table()
{
    new_icam_load(&status_cameras, "status_cameras.tbl");
}

// 0x42b260  Confidence: exact (journal_load_bitmaps shape; the size of filename is a guess: 0x70 bytes in
// the frame). The +/- bitmap sizes are not scaled, the scrollbar tile is (without + 0.5).
void status_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < STATUS_NUM_BUTTONS; i++) {
        ui_button_state *b = &status_buttons[i];
        const ui_button_def *d = &status_button_defs[i];

        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);

        if (strlen(d->name)) {
            char filename[128];

            strcpy(filename, d->name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            if (i == 0)
                b->flags |= 1;
        }
    }


    status_skill_row_bm = bm_load(status_skill_row_names[ui_hires], path_interface, -1);
    bm_get_dimensions(status_skill_row_bm, &status_skill_row_bm_w, &status_skill_row_bm_h);
    status_skill_button_bm = bm_load(status_skill_up_names[ui_hires], path_interface, -1);
    status_skill_button2_bm = bm_load(status_skill_dn_names[ui_hires], path_interface, -1);
    progress_update(0);
    status_scrollbar_bm = bm_load(status_scrollbar_names[ui_hires], path_interface, -1);
    bm_get_dimensions(status_scrollbar_bm, &status_thumb_bm_w, &status_thumb_bm_h);
    status_thumb_bm_w = (int)(status_thumb_bm_w * scale);
    status_thumb_bm_h = (int)(status_thumb_bm_h * scale);
}

// 0x42b480  Confidence: exact (VC6 byte match) (the found test after the loop survives as a dead compare: the refresh call
// sits inside the loop and the shared not-found tail is duplicated). Without a character the first party
// member (type 7, not dead / flag 0x100) is shown; with none the screen closes.
void status_init()
{
    status_flags = 6;
    if (game_flags & 1) {
        if (!game_is_paused()) {
            status_flags |= 1;
            game_pause(0, 0);
        }
    } else {
        ui_screen_stop_player();
    }

    status_skill_buttons_shown = false;
    status_scroll_pos = 0;
    status_mouse_mode = 0;
    status_pressed_button = -1;

    if (status_character) {
        statscreen_refresh_anim(status_character);
    } else {
        Character *c;

        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->type == 7 && !(c->flags & 0x108)) {
                statscreen_refresh_anim(c);
                break;
            }
        }
        if (c == &living_list) {
            interface_set_screen(0, 0);
            return;
        }
    }

    status_update_species_tab();
    status_update_member_buttons();
    status_build_skill_list();
    status_update_scrollbar();
}

// 0x42b570  Confidence: exact (jump table 0x42b600 over the species 0..3).
void status_update_species_tab()
{
    status_buttons[1].flags &= ~1;
    status_buttons[2].flags &= ~1;
    status_buttons[3].flags &= ~1;
    status_buttons[4].flags &= ~1;

    if (status_character) {
        switch (status_character->info->species) {
        case 0:
            status_buttons[1].flags |= 1;
            break;
        case 1:
            status_buttons[2].flags |= 1;
            break;
        case 2:
            status_buttons[3].flags |= 1;
            break;
        case 3:
            status_buttons[4].flags |= 1;
            break;
        }
    }
}

// 0x42b610  Confidence: exact. The summon does not count as a member.
void status_update_member_buttons()
{
    int count = party_count_active();

    if (summon_player)
        count--;
    if (count > 1 && !(game_flags & 2)) {
        status_buttons[9].flags &= ~4;
        status_buttons[11].flags &= ~4;
    } else {
        status_buttons[9].flags |= 4;
        status_buttons[11].flags |= 4;
    }
}

// 0x42b680  Confidence: exact (VC6 byte match) (the byte goes through a stack slot before fild; the unsigned char
// conversion is read from the and 0xff). Multiplayer hides pick lock (0x15) and appraise (0x14). Skills with
// a negative value are left out.
void status_build_skill_list()
{
    Player *player;
    int i, count;

    status_num_skills = 0;
    status_skills_dirty = 0;
    if (!status_character)
        return;
    player = player_from_handle(status_character->handle);
    if (!player)
        return;

    for (i = 0; i < player->num_progression && status_num_skills < STATUS_MAX_SKILLS; i++) {
        if (status_character->level >= player->progression[i].level) {
            int skill = player->progression[i].skill;
            float value;

            if ((game_flags & 2) && (skill == SKILL_PICK_LOCK || skill == SKILL_APPRAISE))
                continue;
            value = CHAR_M(status_character)->get_skill(skill);
            if (value >= 0.0f) {
                unsigned char base = status_character->skill[skill];
                status_skill_entry *e = &status_skill_entries[status_num_skills];

                e->is_chain = false;
                e->skill = skill;
                e->base_level = (int)(base * 0.1f + 0.5f);
                e->level = (int)(value * 10.0f + 0.5f);
                e->pending = 0;
                status_num_skills++;
            }
        }
    }

    count = chain_count_unlocked_skills(player);
    for (i = 0; i < count && status_num_skills < STATUS_MAX_SKILLS; i++) {
        status_skill_entry *e = &status_skill_entries[status_num_skills];

        e->is_chain = true;
        e->skill = chain_get_unlocked_skill(player, i);
        status_num_skills++;
    }
}

// 0x42b800  Confidence: exact (VC6 byte match) (journal_update_list_scrollbar shape; page is a local because fild needs it in
// memory).
void status_update_scrollbar()
{
    int count = status_num_skills;
    int page = status_page_size[resolution_index];

    if (count <= page) {
        status_scrollbar_visible = false;
        status_buttons[13].flags |= 4;
        status_buttons[15].flags |= 4;
        return;
    }

    status_buttons[13].flags &= ~4;
    status_scrollbar_visible = true;
    status_buttons[15].flags &= ~4;

    if (count <= 0) {
        status_thumb_x = status_scrollbar_track[resolution_index][0];
        status_thumb_y = status_scrollbar_track[resolution_index][1];
        status_thumb_w = status_thumb_bm_w;
        status_thumb_h = status_scrollbar_track_height[resolution_index];
    } else {
        status_thumb_x = status_scrollbar_track[resolution_index][0];
        status_thumb_y = (int)((float)status_scroll_pos / status_num_skills
                               * status_scrollbar_track_height[resolution_index] + 0.5f)
                         + status_scrollbar_track[resolution_index][1];
        status_thumb_w = status_thumb_bm_w;
        status_thumb_h = (int)((float)page / status_num_skills * status_scrollbar_track_height[resolution_index]
                               + 0.5f);
    }
}

// 0x42b910  Confidence: exact.
void status_leave()
{
    if (status_skills_dirty)
        status_apply_skill_points();
    else
        status_cancel_skill_points();
    if (status_character)
        statscreen_unload_anim(status_character);
    if (status_flags & 1)
        game_unpause();
    if (status_flags & 2)
        statscreen_refresh_anim(0);
    if (status_flags & 4)
        shop_set_shopkeeper_from_char(0);
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool status_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x42b970  Confidence: exact (VC6 byte match) (the binary sums the pending points in EDI and never reads the sum, written as
// the dead local `spent`; the pending counts are not cleared). One level is 10 in the skill byte.
void status_apply_skill_points()
{
    int spent = 0;
    int i;

    if (!player_from_handle(status_character->handle))
        return;

    for (i = 0; i < status_num_skills; i++) {
        status_skill_entry *e = &status_skill_entries[i];

        if (!e->is_chain) {
            spent += e->pending;
            status_character->skill[e->skill] += e->pending * 10;
        }
    }
    if (game_flags & 2)
        mp_send_character_skills(status_character);
}

// 0x42b9f0  Confidence: exact.
void status_cancel_skill_points()
{
    Player *player;
    int i;

    if (!status_character)
        return;
    player = player_from_handle(status_character->handle);
    if (!player)
        return;

    for (i = 0; i < status_num_skills; i++) {
        player->skill_points += status_skill_entries[i].pending;
        status_skill_entries[i].pending = 0;
    }
}

// 0x42ba50  Confidence: exact (jump table 0x42badc over the mouse modes 0..5).
int status_frame(int key)
{
    int i;

    for (i = 0; i < STATUS_NUM_BUTTONS; i++)
        status_buttons[i].flags &= ~2;

    if (ui_message_box_is_active())
        return key;

    switch (status_mouse_mode) {
    case 0:
        key = status_handle_click(key);
        break;
    case 1:
        status_update_buttons();
        break;
    case 2:
        status_drag_scrollbar();
        break;
    case 3:
        key = status_ai_popup_frame(key);
        break;
    case 4:
        status_handle_skill_buttons();
        break;
    case 5:
        key = status_chain_popup_frame(key);
        break;
    }
    return key;
}

// 0x42bb00  Confidence: likely (the first font height call's result is dead in the binary; the order of the
// key cases follows the body layout ESC/R, B, I, Q; the row y is i * font_h, strength-reduced in the binary).
// Left click on a row with points: the left half of the +/- pair raises, the right half lowers (mode 4, acted on
// in status_handle_skill_buttons). Right click on a chain row: popup of the four modifier keys (mode 5).
int status_handle_click(int key)
{
    int i, rows;
    int font_h = gr_get_font_height(-1);

    status_hover_row = -1;
    status_pressed_button = -1;

    for (i = 0; i < STATUS_NUM_BUTTONS; i++) {
        ui_button_state *b = &status_buttons[i];
        const ui_button_def *d = &status_button_defs[i];

        if (!(b->flags & 4) && (d->flags & 3)
            && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            status_pressed_button = i;
            b->flags |= 2;
            if (d->flags & 1) {
                if (mouse_was_pressed(0)) {
                    status_mouse_mode = 1;
                    status_button_timer.invalidate();
                    return key;
                }
            } else if (d->flags & 2) {
                if (mouse_was_pressed(0)) {
                    status_mouse_mode = 1;
                    status_button_action(status_pressed_button + 1);
                    status_button_timer.SetTimeout(500);
                    return key;
                }
            }
            break;
        }
    }


    if (point_in_rect(game_mouse_x, game_mouse_y, status_thumb_x, status_thumb_y, status_thumb_w, status_thumb_h)
        && mouse_was_pressed(0)) {
        status_mouse_mode = 2;
        status_drag_start_thumb_y = status_thumb_y;
        status_drag_start_mouse_y = game_mouse_y;
        return key;
    }

    rows = status_num_skills;
    if (rows >= status_page_size[resolution_index])
        rows = status_page_size[resolution_index];
    font_h = gr_get_font_height(-1);

    if (point_in_rect(game_mouse_x, game_mouse_y, status_skill_list_pos[resolution_index][0],
                      status_skill_list_pos[resolution_index][1], status_skill_row_right[resolution_index],
                      rows * font_h)) {
        for (i = 0; i < rows; i++) {
            if (point_in_rect(game_mouse_x, game_mouse_y, status_skill_list_pos[resolution_index][0],
                              status_skill_list_pos[resolution_index][1] + i * font_h,
                              status_skill_row_right[resolution_index], font_h)) {
                status_skill_entry *e;

                status_hover_row = i;
                e = &status_skill_entries[status_scroll_pos + i];
                if (mouse_was_pressed(0) && status_get_skill_points(status_scroll_pos + i)) {
                    int x = status_skill_button_pos[resolution_index][0] + status_skill_list_pos[resolution_index][0];
                    int y = status_skill_button_pos[resolution_index][1] + status_skill_list_pos[resolution_index][1]
                            + i * font_h;
                    int half = (int)((status_skill_row_bm_w + 1.0f) * 0.5f);

                    if (point_in_rect(game_mouse_x, game_mouse_y, x, y, half, status_skill_row_bm_h)) {
                        status_skill_button_side = true;
                        status_mouse_mode = 4;
                        return key;
                    }
                    if (point_in_rect(game_mouse_x, game_mouse_y, x + half, y, half, status_skill_row_bm_h)) {
                        status_skill_button_side = false;
                        status_mouse_mode = 4;
                        return key;
                    }
                } else if (mouse_was_pressed(1) && e->is_chain) {
                    char *names[4];
                    int k;

                    for (k = 0; k < 4; k++)
                        names[k] = localize_string_or_invalid(&status_modifier_key_names[k].s);
                    ui_choice_popup_show(0, game_mouse_x, game_mouse_y, 4, names, 0, 0, 0);
                    status_mouse_mode = 5;
                    return key;
                } else if (interface_input_ready()) {
                    status_show_skill_tooltip(i);
                }
                break;
            }
        }
    }

    // byte table 0x42bf90 over key - 1 (0..0x2f) into the jump table 0x42bf7c
    switch (key) {
    case KEY_ESC:
    case KEY_R:
        status_confirm_skills(0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_B:
        if (!ui_has_owner_character())
            break;
        status_confirm_skills(7);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_I:
        status_confirm_skills(1);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_Q:
        if (!(game_flags & 1))
            break;
        status_confirm_skills(10);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }

    {
        int dx, dy, dz;

        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0)
            status_page_up();
        else if (dz < 0)
            status_page_down();
    }
    return key;
}

// 0x42bfc0  Confidence: exact. Whether a skill row shows the +/- buttons: not for chain rows; once true it stays
// true until the screen opens again or the member changes, so spent points can be taken back after the last one.
bool status_get_skill_points(int row)
{
    Player *player;

    if (row < 0 || row >= status_num_skills || status_skill_entries[row].is_chain)
        return false;
    if (status_skill_buttons_shown)
        return true;
    player = player_from_handle(status_character->handle);
    if (!player)
        return false;
    status_skill_buttons_shown = player->skill_points > 0;
    return status_skill_buttons_shown;
}

// 0x42c020  Confidence: exact. screen: 0 HUD (close), 1 inventory, 7 buy/sell, 10 journal.
void status_confirm_skills(int screen)
{
    char *buttons[2];

    status_pending_screen = screen;
    if (status_skills_dirty) {
        buttons[0] = XSTR("Yes", 654);
        buttons[1] = XSTR("No", 655);
        ui_message_box_show(XSTR("Skills", 677), XSTR("Accept changes to skills?", 738), -1, -1, 2, buttons,
                            status_confirm_skills_cb, 0);
    } else {
        status_goto_pending_screen();
    }
}

// 0x42c080  Confidence: exact. Any other screen keeps the status model and the shopkeeper (bits 2 and 4).
void status_goto_pending_screen()
{
    if (status_pending_screen)
        status_flags &= ~6;
    if (status_pending_screen == 1) {
        char page[4];
        page[0] = 0;
        interface_set_screen(1, page);
    } else {
        interface_set_screen(status_pending_screen, 0);
    }
}

// 0x42c0c0  Confidence: exact. Yes (0) keeps status_skills_dirty, so status_leave applies the points; No (1)
// clears it, so they are returned. -2 = the box was dismissed.
void status_confirm_skills_cb(int button)
{
    if (button == -2)
        return;
    if (button == 1)
        status_skills_dirty = 0;
    status_goto_pending_screen();
}

// 0x42c0e0  Confidence: exact (VC6 byte match, m-1020: the clamp as `&= (x <= 0) - 1`, D10600).
void status_page_up()
{
    if (status_scroll_pos > 0) {
        status_scroll_pos -= status_page_size[resolution_index];
        status_scroll_pos &= (status_scroll_pos <= 0) - 1;
        status_update_scrollbar();
    }
}

// 0x42c110  Confidence: exact.
void status_page_down()
{
    int pos = status_scroll_pos;
    int last = status_num_skills - status_page_size[resolution_index];

    if (pos < last) {
        pos += status_page_size[resolution_index];
        if (pos < last)
            last = pos;
        status_scroll_pos = last;
        status_update_scrollbar();
    }
}

// 0x42c140  Confidence: exact (byte table 0x42c248 over button - 6, 0..14, into the jump table 0x42c224; the
// two member cases share the call and sound 0x28).
void status_button_action(int button)
{
    switch (button) {
    case 6:
        status_confirm_inventory();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 8:
        status_goto_shop_or_inventory();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 10:
        status_confirm_skills_switch_member(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 12:
        status_confirm_skills_switch_member(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 14:
        status_scroll_up();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 16:
        status_scroll_down();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 18:
        status_ai_popup_show();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 20:
        status_confirm_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x42c260  Confidence: exact.
void status_confirm_inventory()
{
    status_confirm_skills(1);
}

// 0x42c270  Confidence: exact (VC6 byte match) (written as nested ifs: ui_has_owner_character is called in both branches).
// Buy/sell (7) while a shopkeeper is set, else the journal (10) in single player, the inventory (1) in
// multiplayer.
void status_goto_shop_or_inventory()
{
    status_flags &= ~6;
    if (game_flags & 1) {
        if (ui_has_owner_character())
            status_confirm_skills(7);
        else
            status_confirm_skills(10);
    } else {
        if (ui_has_owner_character())
            status_confirm_skills(7);
        else
            status_confirm_skills(1);
    }
}

// 0x42c2d0  Confidence: exact.
void status_confirm_skills_switch_member(bool next)
{
    char *buttons[2];

    if (next) {
        if (status_skills_dirty) {
            buttons[0] = XSTR("Yes", 654);
            buttons[1] = XSTR("No", 655);
            ui_message_box_show(XSTR("Skills", 677), XSTR("Accept changes to skills?", 738), -1, -1, 2, buttons,
                                status_confirm_skills_b_cb2, 0);
        } else {
            status_show_next_member();
        }
    } else {
        if (status_skills_dirty) {
            buttons[0] = XSTR("Yes", 654);
            buttons[1] = XSTR("No", 655);
            ui_message_box_show(XSTR("Skills", 677), XSTR("Accept changes to skills?", 738), -1, -1, 2, buttons,
                                status_confirm_skills_b_cb, 0);
        } else {
            status_show_prev_member();
        }
    }
}

// 0x42c380  Confidence: exact.
void status_show_prev_member()
{
    status_scroll_pos = 0;
    status_skill_buttons_shown = false;
    statscreen_show_prev_member(1);
}

// 0x42c3a0  Confidence: exact.
void status_confirm_skills_b_cb(int button)
{
    if (button == -2)
        return;
    if (button == 0)
        status_apply_skill_points();
    else
        status_cancel_skill_points();
    status_show_prev_member();
}

// 0x42c3c0  Confidence: exact.
void status_show_next_member()
{
    status_scroll_pos = 0;
    status_skill_buttons_shown = false;
    statscreen_show_next_member(1);
}

// 0x42c3e0  Confidence: exact.
void status_confirm_skills_b_cb2(int button)
{
    if (button == -2)
        return;
    if (button == 0)
        status_apply_skill_points();
    else
        status_cancel_skill_points();
    status_show_next_member();
}

// 0x42c400  Confidence: exact.
void status_scroll_up()
{
    if (status_scroll_pos > 0) {
        status_scroll_pos--;
        status_update_scrollbar();
    }
}

// 0x42c420  Confidence: exact.
void status_scroll_down()
{
    if (status_scroll_pos < status_num_skills - status_page_size[resolution_index]) {
        status_scroll_pos++;
        status_update_scrollbar();
    }
}

// 0x42c450  Confidence: exact (VC6 byte match) (operand order of the two position sums). Mode 3 is entered by
// status_update_buttons when the popup is up after the button release.
void status_ai_popup_show()
{
    char *names[6];
    int i;

    for (i = 0; i < 6; i++)
        names[i] = localize_string_or_invalid(ai_combat_style_names[i]);
    ui_choice_popup_show(0, status_combat_style_pos[resolution_index][0] - ui_choice_popup_margin[resolution_index],
                         status_ai_popup_dy[resolution_index] + status_combat_style_pos[resolution_index][1],
                         6, names, 0, 0, 0);
}

// 0x42c4c0  Confidence: exact.
void status_confirm_close()
{
    status_confirm_skills(0);
}

// 0x42c4d0  Confidence: exact.
void status_show_skill_tooltip(int row)
{
    int font_h = gr_get_font_height(-1);
    int x = status_skill_list_pos[resolution_index][0];
    int y = status_skill_list_pos[resolution_index][1] + font_h * row;
    int w = status_skill_row_right[resolution_index];
    status_skill_entry *e = &status_skill_entries[status_scroll_pos + row];

    ui_tooltip_set(x, y, w, font_h, localize_string_or_invalid(skill_table[e->skill].description));
}

// 0x42c540  Confidence: exact (VC6 byte match, match22; release / held template, element pointer; was likely) (journal_update_buttons shape, but the release path ends in its own mode store:
// 3 while the choice popup is up, written as a conditional on ui_choice_popup_flags, the inline form of
// ui_choice_popup_is_active).
void status_update_buttons()
{
    int button = status_pressed_button + 1;

    if (status_pressed_button >= 0 && status_pressed_button < STATUS_NUM_BUTTONS) {
        ui_button_state *b = &status_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (!mouse_is_down(0)) {
            if ((b->flags & 2) && !status_button_timer.valid())
                status_button_action(button);
            status_mouse_mode = (ui_choice_popup_flags & 0x80000000) ? 3 : 0;
        } else {
            if (status_button_timer.valid() && status_button_timer.elapsed() && (b->flags & 2)) {
                status_button_action(button);
                status_button_timer.SetTimeout(100);
            }
        }
        return;
    }
    status_mouse_mode = 0;
}

// 0x42c630  Confidence: exact (VC6 byte match, match22; clamp through last; was likely) (order of the three terms of y; the clamp written as two conditionals on a
// local, one store).
void status_drag_scrollbar()
{
    int y = status_drag_start_thumb_y - status_scrollbar_track[resolution_index][1] - status_drag_start_mouse_y + game_mouse_y;
    int pos = (int)((float)y / status_scrollbar_track_height[resolution_index] * status_num_skills + 0.5f);
    int last;

    pos = pos > 0 ? pos : 0;
    last = status_num_skills - status_page_size[resolution_index];
    if (pos < last)
        last = pos;
    status_scroll_pos = last;
    status_update_scrollbar();

    if (!mouse_is_down(0))
        status_mouse_mode = 0;
}

// 0x42c6c0  Confidence: exact. The chosen style goes to ai_state +0x18 (ai_set_18).
int status_ai_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        status_mouse_mode = 0;
        return 0;
    }
    if (choice >= 0)
        AI_M(status_character)->set_18(choice);
    status_mouse_mode = 0;
    return key;
}

// 0x42c710  Confidence: exact (VC6 byte match) (the pressed flag of the miss path is stored from the point_in_rect result
// register, read as a second constant store). Acts on release over the half that was pressed.
void status_handle_skill_buttons()
{
    int font_h = gr_get_font_height(-1);
    int x = status_skill_list_pos[resolution_index][0] + status_skill_button_pos[resolution_index][0];
    int y = status_skill_list_pos[resolution_index][1] + status_skill_button_pos[resolution_index][1]
            + status_hover_row * font_h;
    int half = (int)((status_skill_row_bm_w + 1.0f) * 0.5f);

    if (!status_skill_button_side)
        x += half;

    if (!point_in_rect(game_mouse_x, game_mouse_y, x, y, half, status_skill_row_bm_h)) {
        status_skill_button_pressed = false;
        if (!mouse_is_down(0))
            status_mouse_mode = 0;
    } else {
        status_skill_button_pressed = true;
        if (mouse_is_down(0))
            return;
        status_mouse_mode = 0;
        if (status_skill_button_side)
            status_skill_increment(status_scroll_pos + status_hover_row);
        else
            status_skill_decrement(status_scroll_pos + status_hover_row);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
    }
}

// 0x42c820  Confidence: exact (VC6 byte match) (the (base + pending) * 10 spelling of the cap test). A skill cannot be raised
// past the level's skill cap (xp_level_skill_cap, in levels).
void status_skill_increment(int row)
{
    status_skill_entry *e;
    Player *player;

    if (row < 0 || row >= status_num_skills)
        return;
    e = &status_skill_entries[row];
    if (e->is_chain)
        return;
    player = player_from_handle(status_character->handle);
    if (!player || player->skill_points <= 0)
        return;
    int level = (e->base_level + e->pending) * 10;
    if (level < (int)(xp_level_skill_cap(status_character->level) * 10.0f + 0.5f)) {
        e->pending++;
        player->skill_points--;
        status_skills_dirty = 1;
    }
}

// 0x42c8d0  Confidence: exact.
void status_skill_decrement(int row)
{
    status_skill_entry *e;
    Player *player;

    if (row < 0 || row >= status_num_skills)
        return;
    e = &status_skill_entries[row];
    if (e->is_chain)
        return;
    player = player_from_handle(status_character->handle);
    if (!player)
        return;
    if (e->pending > 0) {
        e->pending--;
        player->skill_points++;
        status_skills_dirty = 1;
    }
}

// 0x42c940  Confidence: exact. The chosen modifier key slot (0 CAPS .. 3 ALT) gets the hovered chain skill.
int status_chain_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        status_mouse_mode = 0;
        return 0;
    }
    if (choice >= 0) {
        Player *player = player_from_handle(status_character->handle);
        if (player)
            character_set_chain_skill(player, choice,
                                      status_skill_entries[status_scroll_pos + status_hover_row].skill);
    }
    status_mouse_mode = 0;
    return key;
}

// 0x42c9c0  Confidence: exact (VC6 byte match) (the mode test is written as a switch: sub 3 / je / sub 2 / jne).
void status_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < STATUS_NUM_BUTTONS; i++) {
        ui_button_state *b = &status_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    status_draw_model();
    status_draw_level();
    status_draw_stats();
    status_draw_skills();
    status_draw_scrollbar();
    status_draw_skill_points();
    status_draw_resistances();
    status_draw_status_effects();
    status_draw_combat_style();

    switch (status_mouse_mode) {
    case 3:
    case 5:
        ui_choice_popup_draw();
        break;
    }
}

// 0x42ca50  Confidence: exact (VC6 byte match) (local order; the operand order of the two border rectangles' sums). The
// model is animated only in single player. When gr_byte_25d921d is set the screen outside the view rectangle
// is filled with black first (four rectangles: left, top, bottom, right).
void status_draw_model()
{
    int clip_w, clip_y, clip_h, clip_x;
    vector3 view_pos;
    vector3 cam_pos;
    gr_light_desc light;
    matrix3 view_orient;
    matrix3 cam_orient;
    int res;

    if (game_flags & 1)
        ((character_instance *)status_character->instance)->process(reported_frametime, 0, 0);
    new_icam_get(&status_cameras, status_character->info->name, &cam_orient, &cam_pos);
    view_pos.x = 0.0f;
    view_pos.y = 0.0f;
    view_pos.z = -3.0f;
    MATRIX3_M(&view_orient)->make_identity();
    gr_setup_3d(&view_orient, &view_pos, 60.0f, 1, 1);
    light.model_prefix = CHAR_M(status_character)->hero_model_prefix();
    if (status_character->info->flags & 0x10000)
        light.appearance = charinfo_get_multi_appearance(status_character->info);

    if (gr_byte_25d921d) {
        gr_set_color(0, 0, 0, 0);
        gr_set_alpha_test(0);
        res = resolution_index;
        gr_rect(0, 0, status_model_view_xy[res][0], gr_screen.max_h, gr_mode_flat_alpha, FLT_MAX);
        res = resolution_index;
        gr_rect(status_model_view_xy[res][0], 0, status_model_view_wh[res][0], status_model_view_xy[res][1],
                gr_mode_flat_alpha, FLT_MAX);
        res = resolution_index;
        gr_rect(status_model_view_xy[res][0], status_model_view_wh[res][1] + status_model_view_xy[res][1],
                status_model_view_wh[res][0],
                gr_screen.max_h - status_model_view_wh[res][1] - status_model_view_xy[res][1],
                gr_mode_flat_alpha, FLT_MAX);
        gr_rect(status_model_view_wh[resolution_index][0] + status_model_view_xy[resolution_index][0], 0,
                gr_screen.max_w - status_model_view_wh[resolution_index][0] - status_model_view_xy[resolution_index][0], gr_screen.max_h,
                gr_mode_flat_alpha, FLT_MAX);
        gr_set_color(255, 255, 255, 255);
        gr_set_alpha_test(1);
    }

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(status_model_view_xy[resolution_index][0], status_model_view_xy[resolution_index][1],
                 status_model_view_wh[resolution_index][0], status_model_view_wh[resolution_index][1]);
    status_model_view_begin();
    ui_3d_begin(&cam_pos);
    ((character_instance *)status_character->instance)->render(&cam_pos, &cam_orient, &light);
    ui_3d_end();
    status_model_view_end();
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x42cce0  Confidence: exact. Jekhar (species 1) gets an extra x offset in multiplayer.
void status_model_view_begin()
{
    status_saved_gr_offset_x = gr_screen.offset_x;
    status_saved_gr_offset_y = gr_screen.offset_y;
    gr_screen.offset_x += status_model_offset[resolution_index][0];
    gr_screen.offset_y += status_model_offset[resolution_index][1];
    if ((game_flags & 2) && status_character->info->species == 1) {
        gr_screen.offset_x += status_model_offset_mp[resolution_index][0];
        gr_screen.offset_y += status_model_offset_mp[resolution_index][1];
    }
}

// 0x42cd60  Confidence: exact.
void status_model_view_end()
{
    gr_screen.offset_x = status_saved_gr_offset_x;
    gr_screen.offset_y = status_saved_gr_offset_y;
}

// 0x42cd80  Confidence: exact (the clamp of the bar fraction as if / else if; the operand order of the
// right-aligned x). Values right-aligned at x + status_level_value_right.
void status_draw_level()
{
    int font_h = gr_get_font_height(-1);
    int x = status_level_pos[resolution_index][0];
    int y = status_level_pos[resolution_index][1];
    int xp_this = xp_for_level(status_character->level);
    int xp_next = xp_for_level(status_character->level + 1);
    float frac;
    int w, h;
    char buf[16];

    if (xp_next <= 0)
        xp_next = 0;
    frac = (float)(status_character->xp - xp_this) / (xp_next - xp_this);
    if (frac <= 0.0f)
        frac = 0.0f;
    else if (frac >= 1.0f)
        frac = 1.0f;

    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Level:", 739), -1, gr_string_mode);
    sprintf(buf, "%d", status_character->level);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_level_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_string(x, y, XSTR("Experience:", 740), -1, gr_string_mode);
    int_to_string_with_commas(buf, 16, status_character->xp);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_level_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_string(x, y, XSTR("To Next Level:", 741), -1, gr_string_mode);
    int_to_string_with_commas(buf, 16, xp_next);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_level_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    gr_set_color_ptr(&ui_color_dark_green);
    gr_rect(status_xp_bar_pos[resolution_index][0], status_xp_bar_pos[resolution_index][1],
            (int)(status_xp_bar_size[resolution_index][0] * frac + 0.5f), status_xp_bar_size[resolution_index][1],
            gr_rect_mode, 1.0f);
}

// 0x42cff0  Confidence: exact (the stack slot of max_hp is reused for the AP fraction; statement order of the
// value computations follows the calls). HP and AP are drawn as "cur/" in the text colour, then the maximum
// coloured by its equipment bonus, the whole "cur/max" right-aligned. Speed is skill 2 * 1000 (%), its bonus
// the difference to the unequipped byte * 10. Damage is the weapon's (slot 5), 0 unarmed.
void status_draw_stats()
{
    int font_h = gr_get_font_height(-1);
    int x = status_stats_pos[resolution_index][0];
    int y = status_stats_pos[resolution_index][1];
    int max_hp = CHAR_M(status_character)->get_max_stat(0);
    int hp_bonus = CHAR_M(status_character)->equip_bonus(0);
    int max_ap = CHAR_M(status_character)->get_max_stat(1);
    int ap_bonus = CHAR_M(status_character)->equip_bonus(1);
    int speed = (int)(CHAR_M(status_character)->get_skill(SKILL_SPEED) * 1000.0f + 0.5f);
    int speed_bonus = speed - status_character->skill[SKILL_SPEED] * 10;
    item_info *weapon = (item_info *)status_character->equipment[5];
    int damage;
    int protection;
    float hp_frac, ap_frac;
    int w, h, w2, h2;
    char buf[16];

    if (weapon)
        damage = (unsigned short)weapon->type->damage;
    else
        damage = 0;
    protection = (int)(COMBAT_M(status_character)->get_protection() * 100.0f + 0.5f);

    hp_frac = status_character->hp / max_hp;
    if (hp_frac <= 0.0f)
        hp_frac = 0.0f;
    else if (hp_frac >= 1.0f)
        hp_frac = 1.0f;
    ap_frac = (float)status_character->ap / max_ap;
    if (ap_frac <= 0.0f)
        ap_frac = 0.0f;
    else if (ap_frac >= 1.0f)
        ap_frac = 1.0f;

    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("HP:", 742), -1, gr_string_mode);
    sprintf(buf, "%d/%d", (int)status_character->hp, max_hp);
    gr_get_string_size(&w, &h, buf, -1, -1);
    sprintf(buf, "%d/", (int)status_character->hp);
    gr_get_string_size(&w2, &h2, buf, -1, -1);
    gr_string(status_stats_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);
    status_set_bonus_color((float)hp_bonus);
    sprintf(buf, "%d", max_hp);
    gr_string(status_stats_value_right[resolution_index] - w + w2 + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("AP:", 743), -1, gr_string_mode);
    sprintf(buf, "%d/%d", status_character->ap, max_ap);
    gr_get_string_size(&w, &h, buf, -1, -1);
    sprintf(buf, "%d/", status_character->ap);
    gr_get_string_size(&w2, &h2, buf, -1, -1);
    gr_string(status_stats_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);
    status_set_bonus_color((float)ap_bonus);
    sprintf(buf, "%d", max_ap);
    gr_string(status_stats_value_right[resolution_index] - w + w2 + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Speed:", 744), -1, gr_string_mode);
    status_set_bonus_color((float)speed_bonus);
    sprintf(buf, "%d%%", speed);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_stats_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Damage:", 745), -1, gr_string_mode);
    sprintf(buf, "%d", damage);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_stats_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Protection:", 746), -1, gr_string_mode);
    sprintf(buf, "%d", protection);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_stats_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    gr_set_color_ptr(&ui_color_dark_red);
    gr_rect(status_hp_bar_pos[resolution_index][0], status_hp_bar_pos[resolution_index][1],
            (int)(status_bar_size[resolution_index][0] * hp_frac + 0.5f), status_bar_size[resolution_index][1],
            gr_rect_mode, 1.0f);
    gr_set_color_ptr(&ui_color_dark_blue);
    gr_rect(status_ap_bar_pos[resolution_index][0], status_ap_bar_pos[resolution_index][1],
            (int)(status_bar_size[resolution_index][0] * ap_frac + 0.5f), status_bar_size[resolution_index][1],
            gr_rect_mode, 1.0f);
}

// 0x42d640  Confidence: exact (VC6 byte match, m-1020: the row clamp as an if / else of two assignments
// from the globals; jump table 0x42d9f4 over the chain slot 0..3; the key names are read from
// localize_strings directly, not through status_modifier_key_names). Chain rows show their modifier key, skill
// rows the level (with pending points) coloured by the equipment bonus and, when points can be spent, the +/-
// pair (pressed halves while mode 4 holds on this row).
void status_draw_skills()
{
    int rows, x, y, font_h, i;
    Player *player;
    int w, h;
    char buf[16];

    if (status_num_skills <= 0)
        return;

    x = status_skill_list_pos[resolution_index][0];
    y = status_skill_list_pos[resolution_index][1];
    if (status_num_skills < status_page_size[resolution_index])
        rows = status_num_skills;
    else
        rows = status_page_size[resolution_index];
    font_h = gr_get_font_height(-1);
    player = player_from_handle(status_character->handle);

    for (i = 0; i < rows; i++) {
        int row = status_scroll_pos + i;
        status_skill_entry *e = &status_skill_entries[row];
        int skill = e->skill;

        if (i == status_hover_row)
            gr_set_color_ptr(&ui_color_highlight);
        else
            gr_set_color_ptr(&ui_color_text);
        gr_string(x, y, localize_string_or_invalid(skill_table[skill].name), -1, gr_string_mode);

        if (e->is_chain) {
            if (player) {
                switch (chain_config_find_skill(player, e->skill)) {
                case 0:
                    strcpy(buf, XSTR("CAPS", 734));
                    break;
                case 1:
                    strcpy(buf, XSTR("SHFT", 735));
                    break;
                case 2:
                    strcpy(buf, XSTR("CTRL", 736));
                    break;
                case 3:
                    strcpy(buf, XSTR("ALT", 737));
                    break;
                default:
                    buf[0] = 0;
                    break;
                }
                if (strlen(buf)) {
                    gr_get_string_size(&w, &h, buf, -1, -1);
                    gr_string(status_skill_row_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);
                }
            }
        } else {
            status_set_bonus_color((float)(e->level - e->base_level));
            sprintf(buf, "%d", e->pending + e->level);
            gr_get_string_size(&w, &h, buf, -1, -1);
            gr_string(status_skill_row_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);
            gr_set_color(255, 255, 255, 255);

            if (status_get_skill_points(row)) {
                if (status_mouse_mode == 4 && i == status_hover_row && status_skill_button_pressed) {
                    if (status_skill_button_side)
                        gr_bitmap(status_skill_button_bm, status_skill_button_pos[resolution_index][0] + x,
                                  status_skill_button_pos[resolution_index][1] + y, gr_bitmap_wrap_mode);
                    else
                        gr_bitmap(status_skill_button2_bm, status_skill_button_pos[resolution_index][0] + x,
                                  status_skill_button_pos[resolution_index][1] + y, gr_bitmap_wrap_mode);
                } else {
                    gr_bitmap(status_skill_row_bm, status_skill_button_pos[resolution_index][0] + x,
                              status_skill_button_pos[resolution_index][1] + y, gr_bitmap_wrap_mode);
                }
            }
        }
        y += font_h;
    }
}

// 0x42da10  Confidence: exact (VC6 byte match) (journal_draw_list_scrollbar shape with the visibility test).
void status_draw_scrollbar()
{
    int tiles, i;

    if (!status_scrollbar_visible)
        return;

    gr_set_color(255, 255, 255, 255);
    tiles = status_thumb_h / status_thumb_bm_h;
    for (i = 0; i < tiles; i++)
        ui_bitmap_scaled(status_scrollbar_bm, status_thumb_x, status_thumb_bm_h * i + status_thumb_y,
                         gr_bitmap_wrap_mode);
    ui_bitmap_scaled(status_scrollbar_bm, status_thumb_x, status_thumb_y - status_thumb_bm_h + status_thumb_h,
                     gr_bitmap_wrap_mode);
}

// 0x42dac0  Confidence: exact (VC6 byte match) (one buffer and one w / h pair reused for the three strings, as the frame shows;
// the colour lerp's operand order). "Skill points: %d" is centred on status_skill_points_pos; the number is
// drawn separately and pulses between the disabled grey and white while points are left.
void status_draw_skill_points()
{
    Player *player = player_from_handle(status_character->handle);
    int x, w, h;
    char buf[32];

    if (!player)
        return;

    sprintf(buf, XSTR("Skill points: %d", 747), player->skill_points);
    gr_get_string_size(&w, &h, buf, -1, -1);
    x = status_skill_points_pos[resolution_index][0] - w / 2;
    strcpy(buf, XSTR("Skill points: ", 748));
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, status_skill_points_pos[resolution_index][1], buf, -1, gr_string_mode);
    x += w;
    sprintf(buf, "%d", player->skill_points);

    if (player->skill_points > 0) {
        float r, g, b;

        r = 1.0f;
        g = 1.0f;
        b = 1.0f;
        ui_apply_pulse_rgb(&r, &g, &b, 0.0f);
        gr_set_color((int)((ui_color_white.r - ui_color_disabled.r) * r + ui_color_disabled.r + 0.5f),
                     (int)((ui_color_white.g - ui_color_disabled.g) * g + ui_color_disabled.g + 0.5f),
                     (int)((ui_color_white.b - ui_color_disabled.b) * b + ui_color_disabled.b + 0.5f), 255);
    }
    gr_string(x, status_skill_points_pos[resolution_index][1], buf, -1, gr_string_mode);
}

// 0x42dcd0  Confidence: exact (VC6 byte match) (statement order of the fourteen value computations follows the calls; the
// magic bonus subtracts the skill and level part that character_get_resistance adds: Magic resist * 0.5 +
// level * 0.02). Fire, ice, energy and magic in the first column, slashing, piercing and blunt in the second.
void status_draw_resistances()
{
    int fire = (int)(CHAR_M(status_character)->get_resistance(0) * 100.0f + 0.5f);
    int fire_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(0) * 100.0f + 0.5f);
    int ice = (int)(CHAR_M(status_character)->get_resistance(1) * 100.0f + 0.5f);
    int ice_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(1) * 100.0f + 0.5f);
    int energy = (int)(CHAR_M(status_character)->get_resistance(2) * 100.0f + 0.5f);
    int energy_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(2) * 100.0f + 0.5f);
    int magic = (int)(CHAR_M(status_character)->get_resistance(3) * 100.0f + 0.5f);
    int magic_bonus = magic - (int)((CHAR_M(status_character)->get_skill(SKILL_MAGIC_RESIST) * 0.5f
                                     + status_character->level * 0.02f) * 100.0f + 0.5f);
    int slashing = (int)(CHAR_M(status_character)->get_resistance(4) * 100.0f + 0.5f);
    int slashing_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(4) * 100.0f + 0.5f);
    int piercing = (int)(CHAR_M(status_character)->get_resistance(5) * 100.0f + 0.5f);
    int piercing_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(5) * 100.0f + 0.5f);
    int blunt = (int)(CHAR_M(status_character)->get_resistance(6) * 100.0f + 0.5f);
    int blunt_bonus = (int)(CHAR_M(status_character)->resist_equip_bonus(6) * 100.0f + 0.5f);
    int font_h = gr_get_font_height(-1);
    int x = status_resist_pos[resolution_index][0];
    int y = status_resist_pos[resolution_index][1];
    int w, h;
    char buf[16];

    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Fire:", 749), -1, gr_string_mode);
    status_set_bonus_color((float)fire_bonus);
    sprintf(buf, "%d%%", fire);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Ice:", 750), -1, gr_string_mode);
    status_set_bonus_color((float)ice_bonus);
    sprintf(buf, "%d%%", ice);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Energy:", 751), -1, gr_string_mode);
    status_set_bonus_color((float)energy_bonus);
    sprintf(buf, "%d%%", energy);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Magic:", 752), -1, gr_string_mode);
    status_set_bonus_color((float)magic_bonus);
    sprintf(buf, "%d%%", magic);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    x = status_resist2_pos[resolution_index][0];
    y = status_resist2_pos[resolution_index][1];
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Slashing:", 753), -1, gr_string_mode);
    status_set_bonus_color((float)slashing_bonus);
    sprintf(buf, "%d%%", slashing);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Piercing:", 754), -1, gr_string_mode);
    status_set_bonus_color((float)piercing_bonus);
    sprintf(buf, "%d%%", piercing);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);

    y += font_h;
    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, XSTR("Blunt:", 755), -1, gr_string_mode);
    status_set_bonus_color((float)blunt_bonus);
    sprintf(buf, "%d%%", blunt);
    gr_get_string_size(&w, &h, buf, -1, -1);
    gr_string(status_resist_value_right[resolution_index] - w + x, y, buf, -1, gr_string_mode);
}

// 0x42e410  Confidence: exact (VC6 byte match) (the skipped ids 14, 15 and 17 are compared as pointers into status_effect_names
// in the binary, written as a switch on the index; the "" initialiser of text). The active effects as one
// comma-separated text, wrapped into at most 8 lines.
void status_draw_status_effects()
{
    int lengths[8];
    int starts[8];
    char line[256];
    char text[512] = "";
    int count = 0;
    int i;

    for (i = 0; i < 18; i++) {
        if (!CHAR_M(status_character)->has_status(i))
            continue;
        switch (i) {
        case 14:
        case 15:
        case 17:
            break;
        default:
            if (count > 0)
                strcat(text, ", ");
            strcat(text, localize_string_or_invalid(status_effect_names[i]));
            count++;
            break;
        }
    }

    if (count > 0) {
        int font_h = gr_get_font_height(-1);
        int x = status_effects_pos[resolution_index][0];
        int y = status_effects_pos[resolution_index][1];
        int n;

        gr_set_color_ptr(&ui_color_text);
        n = gr_split_str(lengths, starts, text, status_effects_width[resolution_index], 8, 0, -1);
        for (i = 0; i < n; i++, y += font_h) {
            strncpy(line, text + starts[i], lengths[i]);
            line[lengths[i]] = 0;
            gr_string(x, y, line, -1, gr_string_mode);
        }
    }
}

// 0x42e5c0  Confidence: exact.
void status_draw_combat_style()
{
    int x = status_combat_style_pos[resolution_index][0];
    int y = status_combat_style_pos[resolution_index][1];
    int style = AI_M(status_character)->get_combat_style();

    gr_set_color_ptr(&ui_color_text);
    gr_string(x, y, localize_string_or_invalid(ai_combat_style_names[style]), -1, gr_string_mode);
}

// 0x42e620  Confidence: exact (tail call). Entry 2 of the per-screen refresh table 0x58bdc8.
void status_refresh_for_character()
{
    status_update_species_tab();
    status_build_skill_list();
    status_update_scrollbar();
}
