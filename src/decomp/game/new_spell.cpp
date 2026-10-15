// new_spell.cpp - Sum.exe v1.40, code 0x429850-0x42b1b7 (screen 5, the Spells roll-out: the six spell
// classes as tabs, the castable spells of the current class as rows, drag a row icon to a hot button, click a
// row to cast; at the end the casting entry points the HUD, the skills panel and world_mouse_frame share).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x58075c-0x58079b (layout tables, the 0.04 literal), .data 0x591860-0x5927df (button table,
// icon names, their literals), .bss 0x5f5378-0x5f5aa7.
//
// The screen is new_skill.cpp's twin (same button table layout, same mouse modes, the same code shape in
// almost every function); new_skill.cpp was written first in the source, as its addresses say.

#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/living_entity.h"
#include "engine/spells/spells.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "summoner/interface/new_hud.h"
#include "summoner/interface/new_spell.h"

#define SPELLS_NUM_BUTTONS 49
#define SPELLS_NUM_ICONS   38      // the player spells (spells 0..37)
#define SPELLS_MAX_ROWS    7

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024); names ours.
const int spells_list_pos[3][2]    = { { 84, 171 }, { 105, 214 }, { 134, 274 } };   // 0x58075c first row icon
const int spells_text_offset[3][2] = { { 45, 8 }, { 56, 10 }, { 72, 4 } };         // 0x580774 name from icon
const int spells_row_height[3]     = { 34, 40, 40 };                               // 0x58078c
// 0x580798 is the 0.04f literal of character_use_skill_on.

// ---- .data ---------------------------------------------------------------------------------
// 0x591860 (*new* name), 49 x 0x40. flags: 1 HiLite acting on release through the Press entry after it,
// 2 shows a class tooltip (3 = both). 0 roll-out, 1-16 the portrait tabs (one for each hero and summon,
// spells_update_species_tab), 17/18 the skills arrow, 19-22 previous / next member, 23-40 three icons per
// spell class (A idle, B hilite, C press), 41-46 the class names (flag bit 0 set by spells_set_page),
// 47/48 close.
const ui_button_def spells_button_defs[SPELLS_NUM_BUTTONS] = {
    { "Spells-Rollout",      { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Spelllist-Jos",       { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Jek",       { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Flece",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Ros",       { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Golem",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-RedMin",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BldeUrath", { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BlueImp",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BlkElem",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BldElem",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-PoisElem",  { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-MstElem",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Wraith",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-JdeGlm",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-SpirLah",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-CelSam",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Icon-ArrwBRTHilite",  { { 143, 23 },  { 179, 29 },  { 225, 36 } },  1 },
    { "Icon-ArrwBRTPress",   { { 143, 23 },  { 179, 29 },  { 225, 36 } },  0 },
    { "Icon-ArrwLFHilite",   { { 17, 58 },   { 21, 73 },   { 25, 88 } },   1 },
    { "Icon-ArrwLFPress",    { { 17, 58 },   { 21, 73 },   { 25, 88 } },   0 },
    { "Icon-ArrwRTHilite",   { { 281, 58 },  { 351, 73 },  { 446, 88 } },  1 },
    { "Icon-ArrwRTPress",    { { 281, 58 },  { 351, 73 },  { 446, 88 } },  0 },
    { "Spellicon-HealA#",    { { 27, 91 },   { 34, 114 },  { 44, 136 } },  0 },
    { "Spellicon-HealB#",    { { 27, 91 },   { 34, 114 },  { 44, 136 } },  3 },
    { "Spellicon-HealC#",    { { 27, 91 },   { 34, 114 },  { 44, 136 } },  0 },
    { "Spellicon-DarkA#",    { { 73, 91 },   { 91, 114 },  { 117, 136 } }, 0 },
    { "Spellicon-DarkB#",    { { 73, 91 },   { 91, 114 },  { 117, 136 } }, 3 },
    { "Spellicon-DarkC#",    { { 73, 91 },   { 91, 114 },  { 117, 136 } }, 0 },
    { "Spellicon-EnergyA#",  { { 119, 91 },  { 149, 114 }, { 190, 136 } }, 0 },
    { "Spellicon-EnergyB#",  { { 119, 91 },  { 149, 114 }, { 190, 136 } }, 3 },
    { "Spellicon-EnergyC#",  { { 119, 91 },  { 149, 114 }, { 190, 136 } }, 0 },
    { "Spellicon-HolyA#",    { { 165, 91 },  { 206, 114 }, { 263, 136 } }, 0 },
    { "Spellicon-HolyB#",    { { 165, 91 },  { 206, 114 }, { 263, 136 } }, 3 },
    { "Spellicon-HolyC#",    { { 165, 91 },  { 206, 114 }, { 263, 136 } }, 0 },
    { "Spellicon-FireA#",    { { 211, 91 },  { 264, 114 }, { 336, 136 } }, 0 },
    { "Spellicon-FireB#",    { { 211, 91 },  { 264, 114 }, { 336, 136 } }, 3 },
    { "Spellicon-FireC#",    { { 211, 91 },  { 264, 114 }, { 336, 136 } }, 0 },
    { "Spellicon-IceA#",     { { 257, 91 },  { 321, 114 }, { 409, 136 } }, 0 },
    { "Spellicon-IceB#",     { { 257, 91 },  { 321, 114 }, { 409, 136 } }, 3 },
    { "Spellicon-IceC#",     { { 257, 91 },  { 321, 114 }, { 409, 136 } }, 0 },
    { "Spelllist-Heal",      { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Spelllist-Dark",      { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Spelllist-Energy",    { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Spelllist-Holy",      { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Spelllist-Fire",      { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Spelllist-Ice",       { { 103, 138 }, { 129, 173 }, { 116, 212 } }, 0 },
    { "Icon-CloseHilite",    { { 277, 21 },  { 346, 26 },  { 444, 34 } },  1 },
    { "Icon-ClosePress",     { { 277, 21 },  { 346, 26 },  { 444, 34 } },  0 },
};
// 0x5924a0 (*new* name), by spell index; '?' becomes a..d for the four icon states. 0 = no icon (the
// spells 9, 10, 18, 20 and 27 have none). The literals sit at 0x592538-0x5927df in reverse order.
char *spells_icon_names[SPELLS_NUM_ICONS] = {
    "Spell-Heal?.tga", "Spell-Cure?.tga", "Spell-Regen?.tga", "Spell-Resurrect?.tga",
    "Spell-Vitalize?.tga", "Spell-Revive?.tga", "Spell-Poison?.tga", "Spell-Blind?.tga",
    "Spell-Silence?.tga", 0, 0, "Spell-Invis?.tga",
    "Spell-Death?.tga", "Spell-LifeDrain?.tga", "Spell-Light?.tga", "Spell-Lightning?.tga",
    "Spell-Empower?.tga", "Spell-Powerleech?.tga", 0, "Spell-Bless?.tga",
    0, "Spell-Protect?.tga", "Spell-Faith?.tga", "Spell-Curse?.tga",
    "Spell-Jadebeam?.tga", "Spell-FireArrow?.tga", "Spell-FireBall?.tga", 0,
    "Spell-Meteors?.tga", "Spell-Firewall?.tga", "Spell-Inferno?.tga", "Spell-DarkWall?.tga",
    "Spell-Icicle?.tga", "Spell-Icysleep?.tga", "Spell-Freeze?.tga", "Spell-Blizzard?.tga",
    "Spell-Icecoffin?.tga", "Spell-BlueImpSleep?.tga",
};
// ".tga" (0x5917c8) is new_skill.cpp's literal, pooled.

// ---- .bss ----------------------------------------------------------------------------------
ui_button_state spells_buttons[SPELLS_NUM_BUTTONS];           // 0x5f5378
int  spells_list_count;                                       // 0x5f5810
int  spells_mouse_mode;                                       // 0x5f5814 0 idle, 1 button, 2 row pressed, 3 icon dragged
int  spells_page;                                             // 0x5f5818 spell class shown, -1 none
bool spells_popup_pending;                                    // 0x5f581c (sum.symbols name) the row was pressed on its icon
int  spells_icon_bms[SPELLS_NUM_ICONS][4];                    // 0x5f5820
int  spells_list[SPELLS_MAX_ROWS];                            // 0x5f5a80
int  spells_pressed_button;                                   // 0x5f5a9c
int  spells_hover_row;                                        // 0x5f5aa0
// 0x5f5aa4: 4 bytes nothing references.

// 0x429850  Confidence: likely (skills_load_bitmaps' code, but one filename buffer for both loops: the frame
// is 0x8c bytes against 0x10c there; progress_update before the null test in the icon loop).
void spells_load_bitmaps()
{
    float scale = ui_get_scale();
    char filename[0x70];
    int i, j;

    for (i = 0; i < SPELLS_NUM_BUTTONS; i++) {
        spells_buttons[i].flags = 0;
        spells_buttons[i].bitmap = -1;
        spells_buttons[i].x = 0;
        spells_buttons[i].y = 0;
        spells_buttons[i].w = 0;
        spells_buttons[i].h = 0;
        progress_update(0);

        if (strlen(spells_button_defs[i].name)) {
            strcpy(filename, spells_button_defs[i].name);
            if (strchr(filename, '#')) {
                // "Name<X>#": Name + "640" / "1024" + <X> + ".tga"
                int len = strlen(spells_button_defs[i].name);

                filename[len - 2] = 0;
                strcat(filename, ui_bitmap_suffix[ui_hires]);
                strncat(filename, &spells_button_defs[i].name[len - 2], 1);
                strcat(filename, ".tga");
            } else if (!strchr(filename, '.')) {
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            }
            spells_buttons[i].bitmap = bm_load(filename, path_interface, -1);
            spells_buttons[i].x = spells_button_defs[i].pos[resolution_index][0];
            spells_buttons[i].y = spells_button_defs[i].pos[resolution_index][1];
            bm_get_dimensions(spells_buttons[i].bitmap, &spells_buttons[i].w, &spells_buttons[i].h);
            spells_buttons[i].w = (int)(spells_buttons[i].w * scale + 0.5f);
            spells_buttons[i].h = (int)(spells_buttons[i].h * scale + 0.5f);
            if (i == 0)
                spells_buttons[i].flags |= 1;
        }
    }

    for (i = 0; i < SPELLS_NUM_ICONS; i++) {
        for (j = 0; j < 4; j++) {
            progress_update(0);
            if (spells_icon_names[i]) {
                char *q;

                strcpy(filename, spells_icon_names[i]);
                q = strchr(filename, '?');
                if (q) {
                    switch (j) {
                    case 0: *q = 'a'; break;
                    case 1: *q = 'b'; break;
                    case 2: *q = 'c'; break;
                    case 3: *q = 'd'; break;
                    }
                    spells_icon_bms[i][j] = bm_load(filename, path_interface, -1);
                } else {
                    spells_icon_bms[i][j] = -1;
                }
            } else {
                spells_icon_bms[i][j] = -1;
            }
        }
    }
    spells_page = -1;
}

// 0x429b30  Confidence: exact (tail jump to spells_select_first_page). Keeps the last class when it still
// has spells.
void spells_init()
{
    spells_mouse_mode = 0;
    spells_pressed_button = -1;
    spells_hover_row = -1;
    spells_update_species_tab();
    spells_count_page();
    if (spells_count_in_group(spells_page) > 0)
        spells_set_page(spells_page);
    else
        spells_select_first_page();
}

// 0x429b80  Confidence: exact (byte table 0x429d54 over the species 0..0xce into the jump table 0x429d10).
// The tab of cur_character's species; the summon species ids are those of char_infos (names from the
// bitmaps).
void spells_update_species_tab()
{
    int i;

    for (i = 1; i <= 16; i++)
        spells_buttons[i].flags &= ~1;

    if (cur_character) {
        int tab;

        switch (cur_character->info->species) {
        case SPECIES_JOSEPH:   tab = 1; break;
        case SPECIES_JEKHAR:   tab = 2; break;
        case SPECIES_FLECE:    tab = 3; break;
        case SPECIES_ROSALIND: tab = 4; break;
        case 4:                tab = 5; break;     // Golem
        case 0x90:             tab = 6; break;     // Red Minotaur
        case 0xb0:             tab = 7; break;     // Blade of Urath
        case 0xa4:             tab = 8; break;     // Blue Imp
        case 0xc0:             tab = 9; break;     // Blackfire Elemental
        case 0xcb:             tab = 10; break;    // Blood Elemental
        case 0xca:             tab = 11; break;    // Poison Elemental
        case 0xcc:             tab = 12; break;    // Mist Elemental
        case 0xb9:             tab = 13; break;    // Wraith
        case 9:                tab = 14; break;    // Jade Golem
        case 0xcd:             tab = 15; break;    // Spirit of Laharah
        case 0xce:             tab = 16; break;    // Celestial Samurai
        default:
            return;
        }
        spells_buttons[tab].flags |= 1;
    }
}

// 0x429e30  Confidence: likely (a switch over the button index inside the loop: byte table 0x42a01c over
// i - 23, the compiler folded the constant indexes). The B icon of a class shows (bit 0) and its C icon is
// enabled (bit 2 clear) when the class has castable spells.
void spells_count_page()
{
    int i;

    for (i = 0; i < SPELLS_NUM_BUTTONS; i++) {
        int group;

        switch (i) {
        case 23: group = 0; break;
        case 26: group = 1; break;
        case 29: group = 2; break;
        case 32: group = 3; break;
        case 35: group = 4; break;
        case 38: group = 5; break;
        default:
            continue;
        }
        if (spells_count_in_group(group) > 0) {
            spells_buttons[i].flags |= 1;
            spells_buttons[i + 1].flags &= ~4;
        } else {
            spells_buttons[i].flags &= ~1;
            spells_buttons[i + 1].flags |= 4;
        }
    }
}

// 0x42a030  Confidence: exact (jump table 0x42a130 over the classes 0..5). The rows ignore the AP cost
// (spell_is_usable's third argument), so a spell without AP is listed greyed.
void spells_set_page(int page)
{
    int i;

    for (i = 41; i <= 46; i++)
        spells_buttons[i].flags &= ~1;
    spells_page = page;
    switch (page) {
    case 0: spells_buttons[41].flags |= 1; break;
    case 1: spells_buttons[42].flags |= 1; break;
    case 2: spells_buttons[43].flags |= 1; break;
    case 3: spells_buttons[44].flags |= 1; break;
    case 4: spells_buttons[45].flags |= 1; break;
    case 5: spells_buttons[46].flags |= 1; break;
    }

    spells_list_count = 0;
    if (page != -1) {
        for (i = 0; i < SPELLS_NUM_ICONS; i++) {
            if (spells[i].spell_class == spells_page && spell_is_usable(i, 0, true)) {
                if (spells_list_count >= SPELLS_MAX_ROWS)
                    return;
                spells_list[spells_list_count++] = i;
            }
        }
    }
}

// 0x42a150  Confidence: exact.
void spells_select_first_page()
{
    if (cur_character) {
        int i;

        for (i = 0; i < 6; i++) {
            if (spells_count_in_group(i) > 0) {
                spells_set_page(i);
                return;
            }
        }
    }
    spells_set_page(-1);
}

// 0x42a190  Confidence: exact (jump table 0x42a1f8 over the mouse modes 0..3). Nothing reacts while
// player_input bit 0x10 is set.
int spells_frame(int key)
{
    int i;

    spells_update_member_buttons();
    for (i = 0; i < SPELLS_NUM_BUTTONS; i++)
        spells_buttons[i].flags &= ~2;

    if (player_input[0] & 0x10)
        return key;

    switch (spells_mouse_mode) {
    case 0:
        key = spells_handle_click(key);
        break;
    case 1:
        spells_update_buttons();
        break;
    case 2:
        spells_handle_mouse();
        break;
    case 3:
        spells_release_drag();
        break;
    }
    return key;
}

// 0x42a210  Confidence: exact. The summon does not count; multiplayer is not tested (unlike the status screen).
void spells_update_member_buttons()
{
    int count = party_count_active();

    if (summon_player)
        count--;
    if (count <= 1) {
        spells_buttons[19].flags |= 4;
        spells_buttons[21].flags |= 4;
    } else {
        spells_buttons[19].flags &= ~4;
        spells_buttons[21].flags &= ~4;
    }
}

// 0x42a270  Confidence: likely (pressed_button is stored before the flag, as in skills_handle_click; the
// key switch is the byte table 0x42a474 over key - 1, 0..0x2d, into the jump table 0x42a460). The class
// tooltip shows even over a disabled icon.
int spells_handle_click(int key)
{
    int i;

    spells_pressed_button = -1;
    spells_hover_row = -1;

    for (i = 0; i < SPELLS_NUM_BUTTONS; i++) {
        if ((spells_button_defs[i].flags & 3)
            && point_in_rect(game_mouse_x, game_mouse_y, spells_buttons[i].x, spells_buttons[i].y,
                             spells_buttons[i].w, spells_buttons[i].h)) {
            if ((spells_button_defs[i].flags & 2) && interface_input_ready())
                spells_category_tooltip(i);
            if (!(spells_buttons[i].flags & 4)) {
                spells_pressed_button = i;
                spells_buttons[i].flags |= 2;
                if (mouse_was_pressed(0)) {
                    spells_mouse_mode = 1;
                    return key;
                }
                break;
            }
        }
    }

    for (i = 0; i < spells_list_count; i++) {
        bool on_icon = spells_row_hit_icon(i);
        bool on_text = spells_row_hit_text(i);

        if (on_icon || on_text)
            spells_hover_row = i;
        if (mouse_was_pressed(0)) {
            if (spells_hover_row >= 0) {
                spells_popup_pending = on_icon;
                spells_mouse_mode = 2;
                return key;
            }
        } else {
            if (interface_input_ready()) {
                if (on_icon)
                    spells_show_tooltip(i, true);
                else if (on_text)
                    spells_show_tooltip(i, false);
            }
            if (spells_hover_row >= 0)
                break;
        }
    }

    switch (key) {
    case KEY_ESC:
    case KEY_C:
        interface_set_screen(0, 0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_R:
        interface_set_screen(2, 0);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;
    case KEY_I: {
        bool flag = false;

        interface_set_screen(1, &flag);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;
    }
    case KEY_S:
        interface_set_screen(6, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }
    return key;
}

// 0x42a4b0  Confidence: exact.
bool spells_row_hit_icon(int row)
{
    if (row < 0 || row >= spells_list_count)
        return false;
    return point_in_rect(game_mouse_x, game_mouse_y, spells_list_pos[resolution_index][0],
                         spells_row_height[resolution_index] * row + spells_list_pos[resolution_index][1],
                         hud_hot_button_size[resolution_index][0], hud_hot_button_size[resolution_index][1]);
}

// 0x42a510  Confidence: exact.
bool spells_row_hit_text(int row)
{
    int w, h;

    if (row < 0 || row >= spells_list_count)
        return false;
    gr_get_string_size(&w, &h, localize_string_or_invalid(spell_display_names[spells_list[row]]), -1, -1);
    return point_in_rect(game_mouse_x, game_mouse_y,
                         spells_text_offset[resolution_index][0] + spells_list_pos[resolution_index][0],
                         spells_row_height[resolution_index] * row + spells_text_offset[resolution_index][1]
                             + spells_list_pos[resolution_index][1],
                         w, h);
}

// 0x42a5b0  Confidence: exact (byte table 0x42a690 over button - 24 into the jump table 0x42a674; the strcpy
// is shared by the cases). The tooltip of a class icon (the B entries).
void spells_category_tooltip(int button)
{
    char text[256];

    switch (button) {
    case 24: strcpy(text, XSTR("Heal spells", 728)); break;
    case 27: strcpy(text, XSTR("Dark spells", 729)); break;
    case 30: strcpy(text, XSTR("Energy spells", 730)); break;
    case 33: strcpy(text, XSTR("Holy spells", 731)); break;
    case 36: strcpy(text, XSTR("Fire spells", 732)); break;
    case 39: strcpy(text, XSTR("Ice spells", 733)); break;
    default:
        return;
    }
    ui_tooltip_set(spells_buttons[button].x, spells_buttons[button].y, spells_buttons[button].w,
                   spells_buttons[button].h, text);
}

// 0x42a6a0  Confidence: exact. The description of the row's spell over its icon or its name.
void spells_show_tooltip(int row, bool icon)
{
    int x, y, w, h;

    if (icon) {
        x = spells_list_pos[resolution_index][0];
        y = spells_row_height[resolution_index] * row + spells_list_pos[resolution_index][1];
        w = hud_hot_button_size[resolution_index][0];
        h = hud_hot_button_size[resolution_index][1];
    } else {
        x = spells_text_offset[resolution_index][0] + spells_list_pos[resolution_index][0];
        y = spells_row_height[resolution_index] * row + spells_text_offset[resolution_index][1]
            + spells_list_pos[resolution_index][1];
        gr_get_string_size(&w, &h, localize_string_or_invalid(spell_display_names[spells_list[row]]), -1, -1);
    }
    ui_tooltip_set(x, y, w, h, localize_string_or_invalid(spell_descriptions[spells_list[row]]));
}

// 0x42a780  Confidence: exact. Fires the Press entry after the pressed HiLite on release over it.
void spells_update_buttons()
{
    int button = spells_pressed_button + 1;

    if (spells_pressed_button >= 0 && spells_pressed_button < SPELLS_NUM_BUTTONS) {
        ui_button_state *b = &spells_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            spells_button_action(button);
    }
    spells_mouse_mode = 0;
}

// 0x42a800  Confidence: exact (byte table 0x42a930 over button - 18, 0..0x1e, into the jump table 0x42a904;
// the two member buttons are tail jumps without a sound).
void spells_button_action(int button)
{
    switch (button) {
    case 18:
        spells_goto_skills();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 20:
        ui_party_prev();
        break;
    case 22:
        ui_party_next();
        break;
    case 25:
        spells_page0();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 28:
        spells_page1();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 31:
        spells_page2();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 34:
        spells_page3();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 37:
        spells_page4();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 40:
        spells_page5();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 48:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x42a950  Confidence: exact.
void spells_goto_skills()
{
    interface_set_screen(6, 0);
}

// 0x42a960  Confidence: exact. Shared with new_skill.cpp's buttons.
void ui_party_prev()
{
    party_cycle(0);
}

// 0x42a970  Confidence: exact.
void ui_party_next()
{
    party_cycle(1);
}

// 0x42a980 .. 0x42a9d0  Confidence: exact.
void spells_page0() { spells_set_page(0); }
void spells_page1() { spells_set_page(1); }
void spells_page2() { spells_set_page(2); }
void spells_page3() { spells_set_page(3); }
void spells_page4() { spells_set_page(4); }
void spells_page5() { spells_set_page(5); }

// 0x42a9e0  Confidence: exact. Mode 2: released on the row casts the spell and closes the screen; pressed
// on the icon and moved off the row starts dragging the icon (to a hot button, mode 3).
void spells_handle_mouse()
{
    if (spells_row_hit(spells_hover_row)) {
        int spell;

        if (mouse_is_down(0))
            return;
        spell = spells_list[spells_hover_row];
        if (character_can_cast_now(0) && spell_is_usable(spell, 0, false)) {
            character_cast_spell(spell, 0);
            interface_set_screen(0, 0);
            game_play_sound(0x23, 1.0f, 0, 0, 0);
        } else {
            game_play_sound(0x24, 1.0f, 0, 0, 0);
        }
        spells_mouse_mode = 0;
    } else if (spells_popup_pending) {
        hud_drag_icon_begin(spells_list_pos[resolution_index][0],
                            spells_row_height[resolution_index] * spells_hover_row + spells_list_pos[resolution_index][1],
                            hud_hot_button_size[resolution_index][0], hud_hot_button_size[resolution_index][1],
                            0, spells_list[spells_hover_row]);
        spells_mouse_mode = 3;
    } else if (!mouse_is_down(0)) {
        spells_mouse_mode = 0;
    }
}

// 0x42ab00  Confidence: exact.
bool spells_row_hit(int row)
{
    if (row < 0 || row >= spells_list_count)
        return false;
    if (spells_row_hit_icon(row))
        return true;
    return spells_row_hit_text(row) != 0;
}

// 0x42ab40  Confidence: exact.
void spells_release_drag()
{
    if (!mouse_is_down(0))
        spells_mouse_mode = 0;
}

// 0x42ab60  Confidence: exact.
void spells_draw()
{
    int h, w, y, x;
    int i;

    gr_get_clip(&x, &y, &w, &h);
    gr::set_clip(0, 0, gr_screen_width(), gr_screen_height());
    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < SPELLS_NUM_BUTTONS; i++) {
        if (spells_buttons[i].flags & 3)
            ui_bitmap_scaled(spells_buttons[i].bitmap, spells_buttons[i].x, spells_buttons[i].y, gr_bitmap_wrap_mode);
    }
    spells_draw_list();
    gr::set_clip(x, y, w, h);
}

// 0x42ac10  Confidence: likely (the icon state is the index into spells_icon_bms: 0 idle, 1 hover, 2 pressed,
// 3 not castable; the y step is strength-reduced into EDI).
void spells_draw_list()
{
    int x = spells_list_pos[resolution_index][0];
    int y = spells_list_pos[resolution_index][1];
    int i;

    for (i = 0; i < spells_list_count; i++) {
        int state;

        if (character_can_cast_now(0) && spell_is_usable(spells_list[i], 0, false)) {
            if (i == spells_hover_row) {
                if (spells_mouse_mode == 2)
                    state = spells_row_hit(i) ? 2 : 0;
                else
                    state = 1;
            } else {
                state = 0;
            }
        } else {
            state = 3;
        }

        if (spells_icon_bms[spells_list[i]][state] >= 0) {
            gr_set_color(255, 255, 255, 255);
            ui_bitmap_scaled(spells_icon_bms[spells_list[i]][state], x, y, gr_bitmap_wrap_mode);
        }
        switch (state) {
        case 1:
        case 2:
            gr_set_color_ptr(&ui_color_highlight);
            break;
        case 3:
            gr_set_color_ptr(&ui_color_disabled);
            break;
        case 0:
        default:
            gr_set_color_ptr(&ui_color_text);
            break;
        }
        gr_string(spells_text_offset[resolution_index][0] + x, spells_text_offset[resolution_index][1] + y,
                  localize_string_or_invalid(spell_display_names[spells_list[i]]), -1, gr_string_mode);
        y += spells_row_height[resolution_index];
    }
}

// 0x42ad90  Confidence: exact (tail jump). Entry 5 of the per-screen refresh table 0x58bdc8.
void spells_refresh_for_character()
{
    spells_update_species_tab();
    spells_count_page();
    spells_select_first_page();
}

// 0x42ada0  Confidence: exact.
int spells_get_panel_width()
{
    return spells_buttons[0].w;
}

// 0x42adb0  Confidence: exact.
bool spells_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, spells_buttons[0].x, spells_buttons[0].y, spells_buttons[0].w, spells_buttons[0].h);
}

// 0x42ade0  Confidence: exact. Not in towns, not while silenced, not while already casting.
bool character_can_cast_now(Character *c)
{
    if (!c)
        c = cur_character;
    if (level_is_town)
        return false;
    if (c->has_status(STATUS_SILENCE))
        return false;
    return !c->ai.is_casting();
}

// 0x42ae20  Confidence: likely (the last test returns through mov eax, 1 / xor eax, eax: a comparison
// result, not a bool constant). The character knows the spell when it has its cast move (category 7,
// spell_info +0x50) and the skill of the spell's group reaches the spell's +Skill:. ignore_cost skips the AP
// cost and the cooldown (the lists show such spells greyed).
bool spell_is_usable(int spell, Character *c, bool ignore_cost)
{
    int group;

    if (!c)
        c = cur_character;
    group = spell_get_group(spell);
    if (group == -1)
        return false;
    if (!ignore_cost) {
        if (c->ap < spells[spell].cost)
            return false;
        if (c->spell_on_cooldown(spell))
            return false;
    }
    if (!c->find_move(7, spells[spell].variation))
        return false;
    return c->get_skill(spell_group_skill[group]) >= spells[spell].skill;
}

// 0x42aee0  Confidence: exact.
int spells_count_in_group(int group)
{
    int count = 0;
    int i;

    if (!cur_character || group == -1)
        return 0;
    for (i = 0; i < SPELLS_NUM_ICONS; i++) {
        if (spells[i].spell_class == group && spell_is_usable(i, 0, true))
            count++;
    }
    return count;
}

// 0x42af30  Confidence: likely (the radius is stored twice from one load). Spells with a target radius aim
// with a circle of that size (the walls 0x1d / 0x1f from the caster's position); target-character (flag 2)
// and target-location (flag 4) spells wait for a world pick (hud_target_mode 0 / 1), the rest are cast at once.
void character_cast_spell(int spell, Character *c)
{
    if (!c)
        c = cur_character;
    if (!character_can_cast_now(c) || !spell_is_usable(spell, c, false) || c->ap < spells[spell].cost)
        return;

    ui_clear_pick_overrides();
    if (spells[spell].target_radius != 0.0f) {
        ui_pick_screen_override_on = true;
        ui_pick_override_x = spells[spell].target_radius;
        ui_pick_override_y = spells[spell].target_radius;
        if (spell == 0x1d || spell == 0x1f) {
            ui_pick_world_override_on = true;
            ui_pick_world_override_pos = c->pos;
            ui_pick_override_y = 1.0f;
        }
    }
    if (spells[spell].flags & 2) {
        ui_pending_use_skill = spell;
        hud_target_mode = 0;
    } else if (spells[spell].flags & 4) {
        ui_pending_use_skill = spell;
        hud_target_mode = 1;
    } else {
        c->ai.add_cast_spell(spell, 999999);
        world_pick_set_skill_filter(spell);
    }
}

// 0x42b050  Confidence: likely (the assess target goes to interface_set_screen through a stack copy in the
// dead user argument slot). The pending spell picked on a character (world_mouse_frame) or a skill passed
// through character_use_skill: 0x45 (backstab: a from-behind goal, the AP of skill 0x12) and 0x46 (assess:
// the AP of skill 0x13; the assess screen opens when the skill beats target level * 0.04, else the skill
// cools down and the miss feedback plays).
void character_use_skill_on(int spell, Character *target, int priority, Character *user)
{
    if (!user)
        user = cur_character;
    ui_clear_pick_overrides();
    hud_target_mode = -1;
    if (!target || !user)
        return;

    if (spell == 0x45) {
        user->ai.add_skill_from_behind(target, priority);
        user->add_ap((float)-skill_table[SKILL_BACKSTAB].ap_cost);
        world_pick_clear_filter();
        return;
    }
    if (spell == 0x46) {
        user->add_ap((float)-skill_table[SKILL_ASSESS].ap_cost);
        if (user->get_skill(SKILL_ASSESS) > target->info->level * 0.04f) {
            Character *t = target;

            interface_set_screen(20, &t);
        } else {
            player_start_skill_cooldown(user, SKILL_ASSESS);
            feedback_play(user->info->species, 9, &user->pos);
        }
    }
    user->ai.add_cast_at_target(spell, target, priority);
    user->ai.flags |= 2;
    world_pick_clear_filter();
}

// 0x42b170  Confidence: exact. The pending spell picked on a point of the level (layer from lobj_pick_layer).
void character_cast_at_point(int spell, vector3 *pos, int layer, int priority, Character *c)
{
    if (!c)
        c = cur_character;
    ui_clear_pick_overrides();
    if (c) {
        c->ai.add_cast_at_point(spell, pos, layer, priority);
        hud_target_mode = -1;
        world_pick_clear_filter();
    }
}
