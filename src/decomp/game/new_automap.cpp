// new_automap.cpp - Sum.exe v1.40, code 0x401e80-0x404871 (screen 3, the map screen)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x57e6b8-0x57e99b (button table, layout tables, two file names; 0.375 / 1.0 / 1.25 / 1.6 are
// this unit's __real constants, 0.5f at 0x57e6b0 belongs to an earlier unit), .data 0x588bc0-0x588e03,
// .bss 0x5b29d8-0x5b2db7. The labels themselves, the map corners and the reveal grid size live in
// automap.cpp's .bss (0x5f9838..; module "-" in sum.symbols), so they are extern here although
// sum.symbols tags automap_labels / automap_num_labels new_automap.cpp.
//
// The map of the current layer (level_layer_maps[layer]) is drawn at automap_scroll inside the view
// rectangle automap_view_pos / automap_view_size, scaled by the resolution zoom (1.0, 1.25, 1.6). With
// Map_mask on, only the revealed cells (savegame bits, automap_is_cell_revealed) are visible: the mask
// bitmap is stretched additively over each revealed cell, then the map is drawn through it. Right drag
// scrolls (automap_drag_scroll); Ctrl + left click places a custom label (automap_add_label_at_mouse, typed
// in by automap_label_text_input); left click on a marker selects its label, Delete removes a custom one.
// automap_mouse_mode: 0 idle, 1 button held, 2 dragging, 3 placing a label, 4 typing, 5 over the selected
// label box, 6 over a marker.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/console.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/camera/camera.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/automap.h"
#include "summoner/interface/new_automap.h"

#define AUTOMAP_NUM_BUTTONS 9

// Resolution zoom of the map bitmap. Expanded at every use (the switch on resolution_index with the three
// constants appears in nine functions), so an inline function. Name *new*.
inline float automap_get_zoom()
{
    switch (resolution_index) {
    case 0:
        return 1.0f;
    case 1:
        return 1.25f;
    case 2:
        return 1.6f;
    }
    return 1.0f;
}

// ---- .rdata --------------------------------------------------------------------------------
// 0x57e6b8  9 x 0x40, const (unlike the other screens' tables in .data). flags 1: HiLite hot spot acting
// on release through the Press entry after it (automap_update_buttons); action id = HiLite index + 1.
const ui_button_def automap_button_defs[AUTOMAP_NUM_BUTTONS] = {
    { "Map-Bkgrnd",            { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Icon-ArrwLFHilite",     { { 103, 356 }, { 129, 445 }, { 163, 571 } }, 1 },   // 2 layer down
    { "Icon-ArrwLFPress",      { { 103, 356 }, { 129, 445 }, { 163, 571 } }, 0 },
    { "Icon-ArrwRTHilite",     { { 188, 356 }, { 235, 445 }, { 298, 571 } }, 1 },   // 4 layer up
    { "Icon-ArrwRTPress",      { { 188, 356 }, { 235, 445 }, { 298, 571 } }, 0 },
    { "MapRecenterHiLite",     { { 108, 388 }, { 135, 485 }, { 178, 621 } }, 1 },   // 6 player layer
    { "MapRecenterPress",      { { 108, 388 }, { 135, 485 }, { 178, 621 } }, 0 },
    { "Icon-GrnClseHilite",    { { 277, 21 },  { 346, 26 },  { 444, 35 } },  1 },   // 8 close
    { "Icon-GrnClsePress",     { { 277, 21 },  { 346, 26 },  { 444, 35 } },  0 },
};
// Per resolution_index (640, 800, 1024). Names *new*.
const int automap_view_pos[3][2]  = { { 14, 78 }, { 18, 98 }, { 22, 125 } };        // 0x57e8f8 map view origin
const int automap_view_size[3][2] = { { 291, 266 }, { 364, 333 }, { 466, 426 } };   // 0x57e910 map view size
// Pushed as addresses, so arrays in .rdata rather than literals. Names *new*.
const char automap_camera_bm_name[] = "hud-mapcamera.tga";                          // 0x57e928
const char automap_you_are_here_bm_name[] = "You-R-Here.tga";                       // 0x57e93c
const int automap_label_wrap_w[3] = { 145, 181, 232 };                              // 0x57e94c gr_split_str width
// 0x57e958: four ints read one by one, never indexed, so not foldable constants: a struct. The member
// names are a guess (see NOTES: "left" is also used for a y offset, "bottom" for the cursor y).
struct automap_label_margins {
    int left, right, top, bottom;
};
const automap_label_margins automap_label_margin = { 1, 0, 1, 1 };                  // 0x57e958
const int automap_label_box_gap[3] = { 2, 3, 4 };                                   // 0x57e968 box above the marker
const int automap_title_y[3]       = { 55, 69, 88 };                                // 0x57e974 level name
const int automap_layer_text_y[3]  = { 372, 465, 595 };                             // 0x57e980 "%d of %d"

// ---- .data ---------------------------------------------------------------------------------
// (The literals follow the variables in reverse order: 0x588bd4 "Map-Label1024.tga", 0x588be8
// "Map-Label640.tga", 0x588bfc "hud-mapmask01.tga", then "|" 0x588c10, which new_charselect.cpp,
// new_gamecreate.cpp and new_levelselect.cpp reuse.)
char *automap_mask_bm_name = "hud-mapmask01.tga";                                   // 0x588bc0 (*new*)
bool Map_mask = true;                                                               // 0x588bc4
char *automap_label_bm_names[2] = { "Map-Label640.tga", "Map-Label1024.tga" };      // 0x588bc8 (*new*)
bool Show_coordinates = true;                                                       // 0x588bd0

// ---- .bss ----------------------------------------------------------------------------------
// In address order. The four console_command objects sit between the other variables although their
// initialisers are the last functions of the unit (see NOTES).
int automap_label_w;                                          // 0x5b29d8 size of automap_label_text
int automap_label_h;                                          // 0x5b29dc
// init_colors_5b29e0: red for permanent, green for custom labels (indexed by automap_label.type).
// sum.symbols: automap_label_type_colors (formerly ui_color_red_full, element 0 only).
Color_ctor automap_label_type_colors[2] = { Color_ctor(255, 0, 0, 255), Color_ctor(0, 255, 0, 255) }; // 0x5b29e0
int automap_saved_scroll_x;                                   // 0x5b29e8
int automap_saved_scroll_y;                                   // 0x5b29ec
int automap_label_edit_mode;                                  // 0x5b29f0 type of the label being placed
int automap_layer_w;                                          // 0x5b29f4
int automap_layer_h;                                          // 0x5b29f8
// 0x5b29fc: 4 bytes nothing references (alignment of the next object?)
console_command_t ccmd_map_grid("map_grid", "Toggles Map_grid", cmd_map_grid);       // 0x5b2a00
bool automap_centered;                                        // 0x5b2a0c
int automap_mouse_mode;                                       // 0x5b2a10
char automap_label_text[0x120];                               // 0x5b2a14 selected label, wrapped with '\n'
int automap_mask_bm;                                          // 0x5b2b34
RealtimeTimer automap_label_blink_timer;                      // 0x5b2b38
float automap_scale_x;                                        // 0x5b2b3c map pixels per reveal cell
float automap_scale_y;                                        // 0x5b2b40
int automap_drag_start_mouse_x;                               // 0x5b2b44
int automap_drag_start_mouse_y;                               // 0x5b2b48
int automap_scroll_x;                                         // 0x5b2b4c
int automap_scroll_y;                                         // 0x5b2b50
int automap_mask_tile_w;                                      // 0x5b2b54 (*new*) stretched mask size
int automap_mask_tile_h;                                      // 0x5b2b58 (*new*)
int automap_pressed_button;                                   // 0x5b2b5c
console_command_t ccmd_show_coordinates("show_coordinates", "Toggles Show_coordinates", cmd_show_coordinates);   // 0x5b2b60
float automap_mask_scale;                                     // 0x5b2b6c (*new*) camera icon scale
int automap_label_bm_w;                                       // 0x5b2b70
int automap_label_bm_h;                                       // 0x5b2b74
int automap_selected_label;                                   // 0x5b2b78
int automap_hover_label;                                      // 0x5b2b7c
int automap_label_screen_pos[AUTOMAP_MAX_LABELS][2];          // 0x5b2b80
int automap_you_are_here_w;                                   // 0x5b2c80
int automap_you_are_here_h;                                   // 0x5b2c84
bool automap_label_cursor_on;                                 // 0x5b2c88
bool automap_on_player_layer;                                 // 0x5b2c89
int automap_mask_w;                                           // 0x5b2c8c
int automap_mask_h;                                           // 0x5b2c90
int automap_label_box_x;                                      // 0x5b2c94
int automap_label_box_y;                                      // 0x5b2c98
int automap_drag_start_scroll_x;                              // 0x5b2c9c
int automap_drag_start_scroll_y;                              // 0x5b2ca0
int automap_label_bm;                                         // 0x5b2ca4
console_command_t ccmd_reset_map_labels("reset_map_labels", "Reset automap labels for this level/script",
                                        cmd_reset_map_labels);                     // 0x5b2ca8
int automap_you_are_here_bm;                                  // 0x5b2cb4
int automap_layer;                                            // 0x5b2cb8
// 0x5b2cbc: 4 bytes nothing references (alignment of the button array?)
ui_button_state automap_buttons[AUTOMAP_NUM_BUTTONS];         // 0x5b2cc0
bool automap_label_editing;                                   // 0x5b2d98
int automap_camera_bm;                                        // 0x5b2d9c
int automap_camera_bm_w;                                      // 0x5b2da0 (*new*; written, never read)
int automap_camera_bm_h;                                      // 0x5b2da4
console_command_t ccmd_map_mask("map_mask", "Toggles Map_mask", cmd_map_mask);       // 0x5b2da8
bool Map_grid;                                                // 0x5b2db4

// The compiler's dynamic initialisers of automap_label_blink_timer and automap_label_type_colors (no source
// of their own). Entry 1 before entry 2 means the timer is defined before the colours in the original
// source, the opposite of their .bss order.
// 0x401e80 init_realtimetimer_5b2b38_thunk (jmp, static_initializers entry 1). Confidence: exact.
// 0x401e90 init_realtimetimer_5b2b38 (mov ecx, 0x5b2b38 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.
// 0x401ea0 init_colors_5b29e0_thunk (jmp, entry 2). Confidence: exact.
// 0x401eb0 init_colors_5b29e0 (two Color::Color calls). Confidence: exact.

// 0x401ef0  Confidence: likely (the button loop is exact, as in the journal; the order of the two
// max() sums and of the mask-scale quotient is a guess).
void automap_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < AUTOMAP_NUM_BUTTONS; i++) {
        automap_buttons[i].flags = 0;
        automap_buttons[i].bitmap = -1;
        automap_buttons[i].x = 0;
        automap_buttons[i].y = 0;
        automap_buttons[i].w = 0;
        automap_buttons[i].h = 0;
        progress_update(0);

        if (strlen(automap_button_defs[i].name)) {
            char filename[128];

            strcpy(filename, automap_button_defs[i].name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            automap_buttons[i].bitmap = bm_load(filename, path_interface, -1);
            automap_buttons[i].x = automap_button_defs[i].pos[resolution_index][0];
            automap_buttons[i].y = automap_button_defs[i].pos[resolution_index][1];
            bm_get_dimensions(automap_buttons[i].bitmap, &automap_buttons[i].w, &automap_buttons[i].h);
            automap_buttons[i].w = (int)(automap_buttons[i].w * scale + 0.5f);
            automap_buttons[i].h = (int)(automap_buttons[i].h * scale + 0.5f);
            if (i == 0)
                automap_buttons[i].flags |= 1;
        }
    }

    if (level_num_layers > 0) {
        float zoom = automap_get_zoom();
        int w, h;
        float world_x, world_z;

        bm_get_dimensions(level_layer_maps[0], &w, &h);
        automap_get_world_scale(&world_x, &world_z);
        automap_scale_x = w * world_x * zoom;
        automap_scale_y = h * world_z * zoom;
    }

    automap_mask_bm = bm_load(automap_mask_bm_name, path_interface, -1);
    bm_get_dimensions(automap_mask_bm, &automap_mask_w, &automap_mask_h);
    automap_mask_tile_w = (int)(automap_scale_x + max(automap_scale_x, automap_scale_y) + 1.0f);
    automap_mask_tile_h = (int)(automap_scale_y + max(automap_scale_x, automap_scale_y) + 1.0f);
    progress_update(0);

    automap_label_bm = bm_load(automap_label_bm_names[ui_hires], path_interface, -1);
    bm_get_dimensions(automap_label_bm, &automap_label_bm_w, &automap_label_bm_h);
    automap_label_bm_w = (int)(automap_label_bm_w * scale + 0.5f);
    automap_label_bm_h = (int)(automap_label_bm_h * scale + 0.5f);
    progress_update(0);

    automap_camera_bm = bm_load(automap_camera_bm_name, path_interface, -1);
    bm_get_dimensions(automap_camera_bm, &automap_camera_bm_w, &automap_camera_bm_h);
    progress_update(0);

    automap_you_are_here_bm = bm_load(automap_you_are_here_bm_name, path_interface, -1);
    bm_get_dimensions(automap_you_are_here_bm, &automap_you_are_here_w, &automap_you_are_here_h);
    automap_mask_scale = ((float)automap_mask_tile_h / automap_mask_h
                          + (float)automap_mask_tile_w / automap_mask_w) * 0.375f;
}

// 0x402250  Confidence: exact.
void automap_init()
{
    automap_show_player_layer();
    automap_mouse_mode = 0;
    automap_pressed_button = -1;
    automap_center_on_player();
    automap_labels_update_positions();
    automap_select_label(-1);
}

// folded (0x4f13e0, empty_function): the screen's leave function is empty; the linker folded it into
// empty_function (identical-code folding). interface_set_screen calls each by name (D7903); name ours.
void automap_leave()
{
}

// 0x402280  Confidence: exact (VC6 byte match) (the store of the terminating 0 is scheduled between the pushes of the
// gr_get_string_size call; the expression order of the two box coordinates is read from the code).
// Wraps the label text into at most four lines and places the text box above the marker.
void automap_select_label(int index)
{
    int lengths[4];
    int starts[4];

    automap_label_text[0] = 0;
    automap_selected_label = index;
    if (index >= 0) {
        automap_label *label = &automap_labels[index];
        int n = gr_split_str(lengths, starts, label->text, automap_label_wrap_w[resolution_index], 4, 0, -1);
        int pos = 0;
        int i;

        for (i = 0; i < n; i++) {
            if (i > 0)
                automap_label_text[pos++] = '\n';
            strncpy(&automap_label_text[pos], label->text + starts[i], lengths[i]);
            pos += lengths[i];
        }
        automap_label_text[pos] = 0;
        gr_get_string_size(&automap_label_w, &automap_label_h, automap_label_text, -1, -1);
        automap_label_box_x = automap_label_screen_pos[automap_selected_label][0] - automap_label_w / 2
                              - automap_label_margin.left + 1;
        automap_label_box_y = automap_label_screen_pos[automap_selected_label][1]
                              - automap_label_box_gap[resolution_index] - automap_label_bm_h / 2
                              - automap_label_margin.top - automap_label_margin.bottom - automap_label_h - 2;
    }
}

// 0x4023b0  Confidence: likely (differs under cl 12.00.8966, match23; was exact).
void automap_center_on_player()
{
    float zoom = automap_get_zoom();
    float map_x, map_y;
    int x, y;

    automap_world_to_map(&map_x, &map_y, cur_character->pos.x, cur_character->pos.z);
    x = (int)(automap_layer_w * map_x * zoom + 0.5f);
    y = (int)(automap_layer_h * map_y * zoom + 0.5f);
    if (automap_centered) {
        automap_scroll_x = automap_view_size[resolution_index][0] / 2 - x;
        automap_scroll_y = automap_view_size[resolution_index][1] / 2 - y;
    } else {
        automap_scroll_x = automap_saved_scroll_x;
        automap_scroll_y = automap_saved_scroll_y;
    }
}

// 0x402490  Confidence: exact. Marker positions in map pixels (scroll not included); -1 on other layers.
void automap_labels_update_positions()
{
    float zoom = automap_get_zoom();
    float map_x, map_y;
    int i;

    for (i = 0; i < automap_num_labels; i++) {
        if (automap_labels[i].layer != automap_layer) {
            automap_label_screen_pos[i][0] = -1;
            automap_label_screen_pos[i][1] = -1;
        } else {
            automap_world_to_map(&map_x, &map_y, automap_labels[i].x, automap_labels[i].z);
            automap_label_screen_pos[i][0] = (int)(automap_layer_w * map_x * zoom + 0.5f);
            automap_label_screen_pos[i][1] = (int)(automap_layer_h * map_y * zoom + 0.5f);
        }
    }
}

// 0x402560  Confidence: exact (VC6 byte match) (one register holds both bool stores and one the two zero stores; the
// source may chain them).
void automap_show_player_layer()
{
    automap_set_layer(automap_layer_map_index(cur_character->layer));
    automap_on_player_layer = true;
    automap_centered = true;
    automap_saved_scroll_x = 0;
    automap_saved_scroll_y = 0;
}

// 0x4025a0  Confidence: exact (the last call is a tail jump into automap_update_layer_buttons).
void automap_set_layer(int layer)
{
    if (layer >= 0 && layer < level_num_layers) {
        automap_layer = layer;
        bm_get_dimensions(level_layer_maps[layer], &automap_layer_w, &automap_layer_h);
        automap_labels_update_positions();
        automap_select_label(-1);
        automap_update_layer_buttons();
    }
}

// 0x4025f0  Confidence: exact. Bit 4 disables the layer-down pair on layer 0 and the layer-up pair on the
// last layer.
void automap_update_layer_buttons()
{
    if (automap_layer <= 0) {
        automap_buttons[1].flags |= 4;
        automap_buttons[2].flags |= 4;
    } else {
        automap_buttons[1].flags &= ~4;
        automap_buttons[2].flags &= ~4;
    }
    if (automap_layer >= level_num_layers - 1) {
        automap_buttons[3].flags |= 4;
        automap_buttons[4].flags |= 4;
    } else {
        automap_buttons[3].flags &= ~4;
        automap_buttons[4].flags &= ~4;
    }
}

// 0x402680  Confidence: exact (jump table 0x40273c, every case returns on its own).
int automap_frame(int key)
{
    int i;

    for (i = 0; i < AUTOMAP_NUM_BUTTONS; i++)
        automap_buttons[i].flags &= ~2;

    automap_center_on_player();
    if (automap_on_player_layer) {
        int layer = automap_layer_map_index(cur_character->layer);

        if (layer != automap_layer)
            automap_set_layer(layer);
    }

    if (!(player_input[0] & 0x10)) {
        switch (automap_mouse_mode) {
        case 0:
            key = automap_handle_click(key);
            break;
        case 1:
            automap_update_buttons();
            break;
        case 2:
            automap_drag_scroll();
            break;
        case 3:
            automap_add_label_at_mouse();
            break;
        case 4:
            key = automap_label_text_input(key);
            break;
        case 5:
            automap_label_box_hover();
            break;
        case 6:
            automap_labels_hover();
            break;
        }
    }
    return key;
}

// 0x402760  Confidence: likely (VC6 shares the key-switch tails; the second type test in the Delete case is
// in the code and kept as written). Keys (jump tables 0x402c84 / 0x402ca0): ESC and M close the screen,
// R / I / S / C open screens 2 / 1 / 6 / 5, Delete removes the selected custom label.
int automap_handle_click(int key)
{
    int i;

    automap_pressed_button = -1;
    for (i = 0; i < AUTOMAP_NUM_BUTTONS; i++) {
        if (!(automap_buttons[i].flags & 4) && (automap_button_defs[i].flags & 1)
            && point_in_rect(game_mouse_x, game_mouse_y, automap_buttons[i].x, automap_buttons[i].y,
                             automap_buttons[i].w, automap_buttons[i].h)) {
            automap_buttons[i].flags |= 2;
            automap_pressed_button = i;
            if (mouse_was_pressed(0)) {
                automap_mouse_mode = 1;
                return key;
            }
            break;
        }
    }

    if (automap_mask_bm >= 0
        && point_in_rect(game_mouse_x, game_mouse_y,
                         automap_view_pos[resolution_index][0], automap_view_pos[resolution_index][1],
                         automap_view_size[resolution_index][0], automap_view_size[resolution_index][1])) {
        if (mouse_was_pressed(0)) {
            if (key_is_down(KEY_LCONTROL) || key_is_down(KEY_RCONTROL)) {
                automap_label_edit_mode = AUTOMAP_LABEL_CUSTOM;
                automap_mouse_mode = 3;
                return key;
            }
            if (automap_selected_label >= 0
                && automap_map_point_revealed(automap_label_screen_pos[automap_selected_label][0],
                                              automap_label_screen_pos[automap_selected_label][1], true)
                && point_in_rect(game_mouse_x, game_mouse_y,
                                 automap_view_pos[resolution_index][0] + automap_label_box_x + automap_scroll_x,
                                 automap_view_pos[resolution_index][1] + automap_label_box_y + automap_scroll_y,
                                 automap_label_w, automap_label_h)) {
                if (automap_labels[automap_selected_label].type == AUTOMAP_LABEL_CUSTOM) {
                    automap_label_editing = true;
                    automap_mouse_mode = 5;
                }
                return key;
            }
            for (i = 0; i < automap_num_labels; i++) {
                if (automap_labels[i].layer == automap_layer
                    && automap_map_point_revealed(automap_label_screen_pos[i][0], automap_label_screen_pos[i][1], true)
                    && point_in_rect(game_mouse_x, game_mouse_y,
                                     automap_view_pos[resolution_index][0] - automap_label_bm_w / 2
                                         + automap_label_screen_pos[i][0] + automap_scroll_x,
                                     automap_view_pos[resolution_index][1] - automap_label_bm_h / 2
                                         + automap_scroll_y + automap_label_screen_pos[i][1],
                                     automap_label_bm_w, automap_label_bm_h)) {
                    automap_hover_label = i;
                    automap_label_editing = true;
                    automap_mouse_mode = 6;
                    return key;
                }
            }
            automap_select_label(-1);
            return key;
        }
        if (mouse_was_pressed(1)) {
            automap_mouse_mode = 2;
            if (automap_centered) {
                float zoom = automap_get_zoom();
                float map_x, map_y;
                int x, y;

                automap_world_to_map(&map_x, &map_y, cur_character->pos.x, cur_character->pos.z);
                x = (int)(automap_layer_w * map_x * zoom + 0.5f);
                automap_drag_start_scroll_x = automap_view_size[resolution_index][0] / 2 - x;
                y = (int)(automap_layer_h * map_y * zoom + 0.5f);
                automap_drag_start_scroll_y = automap_view_size[resolution_index][1] / 2 - y;
            } else {
                automap_drag_start_scroll_x = automap_saved_scroll_x;
                automap_drag_start_scroll_y = automap_saved_scroll_y;
            }
            automap_drag_start_mouse_x = game_mouse_x;
            automap_drag_start_mouse_y = game_mouse_y;
            return key;
        }
    }

    switch (key) {
    case KEY_ESC:
    case KEY_M:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_R:
        interface_set_screen(2, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_I: {
        bool arg = false;                 // lives in the key argument's stack slot

        interface_set_screen(1, &arg);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    }
    case KEY_S:
        interface_set_screen(6, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_C:
        interface_set_screen(5, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_DELETE:
        if (automap_selected_label >= 0
            && automap_labels[automap_selected_label].type == AUTOMAP_LABEL_CUSTOM) {
            automap_label *label = &automap_labels[automap_selected_label];

            if (label->type == AUTOMAP_LABEL_CUSTOM)
                SAVEGAME_AUTOMAP.automap_label_remove(cur_level_index,
                    level_script_find_index(cur_level_index, level_script_name), label);
            automap_label_delete(automap_selected_label);
            automap_labels_update_positions();
            automap_select_label(-1);
            return 0;
        }
        break;
    }
    return key;
}

// 0x402d80  Confidence: exact (VC6 byte match) (the clamps as if / else-if; the world-scale outputs reuse the dead
// argument slot `scaled`). x, y in map pixels, times the zoom when `scaled`.
bool automap_map_point_revealed(int x, int y, bool scaled)
{
    float cell_w, cell_h;

    if (!Map_mask)
        return true;
    if (scaled) {
        float zoom = automap_get_zoom();

        x = (int)(x / zoom + 0.5f);
        y = (int)(y / zoom + 0.5f);
    }
    if (x < 0)
        x = 0;
    else if (x >= automap_layer_w)
        x = automap_layer_w - 1;
    if (y < 0)
        y = 0;
    else if (y >= automap_layer_h)
        y = automap_layer_h - 1;

    automap_get_world_scale(&cell_w, &cell_h);
    int cx = (int)(x / (automap_layer_w * cell_w));
    int cy = (int)(y / (automap_layer_h * cell_h));
    return automap_is_cell_revealed(cur_level_index, automap_layer, cx, cy) != 0;
}

// 0x402e90  Confidence: exact. The hit test and the action use the Press entry after the HiLite one.
void automap_update_buttons()
{
    int b = automap_pressed_button;
    int press = b + 1;

    if (b >= 0 && b < AUTOMAP_NUM_BUTTONS) {
        ui_button_state *button = &automap_buttons[press];

        if (point_in_rect(game_mouse_x, game_mouse_y, button->x, button->y, button->w, button->h))
            button->flags |= 2;
        if (mouse_is_down(0))
            return;
        if (button->flags & 2)
            automap_button_action(press);
    }
    automap_mouse_mode = 0;
}

// 0x402f10  Confidence: exact (jump table 0x402f90 for ids 2..8).
void automap_button_action(int id)
{
    switch (id) {
    case 2:
        automap_layer_down();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 4:
        automap_layer_up();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 6:
        automap_show_player_layer();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 8:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x402fb0  Confidence: exact.
void automap_layer_down()
{
    if (automap_layer > 0) {
        automap_set_layer(automap_layer - 1);
        automap_on_player_layer = false;
    }
}

// 0x402fd0  Confidence: exact.
void automap_layer_up()
{
    if (automap_layer < level_num_layers - 1) {
        automap_set_layer(automap_layer + 1);
        automap_on_player_layer = false;
    }
}

// 0x403000  Confidence: likely (min / max macros; the half view size goes through memory for fild).
// The scroll may not leave a gap: x stays within [half - map width, half] of the view.
void automap_drag_scroll()
{
    float zoom = automap_get_zoom();
    int x, y, half;

    x = game_mouse_x - automap_drag_start_mouse_x + automap_drag_start_scroll_x;
    half = automap_view_size[resolution_index][0] / 2;
    x = min(x, half);
    automap_saved_scroll_x = max(x, (int)(half - automap_layer_w * zoom + 0.5f));

    y = game_mouse_y - automap_drag_start_mouse_y + automap_drag_start_scroll_y;
    half = automap_view_size[resolution_index][1] / 2;
    y = min(y, half);
    automap_saved_scroll_y = max(y, (int)(half - automap_layer_h * zoom + 0.5f));

    automap_centered = false;
    if (!mouse_is_down(1))
        automap_mouse_mode = 0;
}

// 0x403100  Confidence: exact. Mode 3: places a label on mouse release and starts typing (mode 4).
void automap_add_label_at_mouse()
{
    int x, y;

    if (mouse_is_down(0))
        return;
    if (automap_mouse_to_map_cell(&x, &y, game_mouse_x, game_mouse_y)
        && automap_map_point_revealed(x, y, false)
        && (automap_label_edit_mode != AUTOMAP_LABEL_CUSTOM || SAVEGAME_AUTOMAP.automap_label_count() < 0x80)) {
        int index = automap_label_alloc();

        if (index >= 0) {
            automap_label *label = &automap_labels[index];
            int script = level_script_find_index(cur_level_index, level_script_name);

            label->type = automap_label_edit_mode;
            automap_map_to_world(&label->x, &label->z, x, y);
            label->layer = automap_layer;
            label->text[0] = 0;
            if (automap_label_edit_mode == AUTOMAP_LABEL_CUSTOM)
                SAVEGAME_AUTOMAP.automap_label_add(cur_level_index, script, label);
            automap_labels_update_positions();
            automap_select_label(index);
            automap_label_blink_timer.SetTimeout(500);
            automap_label_cursor_on = true;
            automap_mouse_mode = 4;
            return;
        }
    }
    automap_mouse_mode = 0;
}

// 0x403240  Confidence: exact. Screen position to unzoomed map pixel; true inside the layer bitmap.
bool automap_mouse_to_map_cell(int *out_x, int *out_y, int mx, int my)
{
    float zoom = automap_get_zoom();

    *out_x = (int)((mx - automap_view_pos[resolution_index][0] - automap_scroll_x) / zoom + 0.5f);
    *out_y = (int)((my - automap_view_pos[resolution_index][1] - automap_scroll_y) / zoom + 0.5f);
    if (*out_x >= 0 && *out_x < automap_layer_w && *out_y >= 0 && *out_y < automap_layer_h)
        return true;
    return false;
}

// 0x403300  Confidence: exact (VC6 byte match) (the z step is computed first and kept on the FPU stack; written as two
// locals in that order).
void automap_map_to_world(float *out_x, float *out_z, int mx, int my)
{
    float step_z = (automap_lower_right.z - automap_upper_left.z) / automap_layer_h;
    float step_x = (automap_lower_right.x - automap_upper_left.x) / automap_layer_w;

    *out_x = step_x * mx + automap_upper_left.x;
    *out_z = my * step_z + automap_upper_left.z;
}

// 0x403350  Confidence: likely (the cursor blink as if / else; Backspace shares the re-wrap with a typed
// character). ESC / Enter keep the text, save a custom label again and leave typing.
int automap_label_text_input(int key)
{
    bool changed = false;
    automap_label *label;

    if (automap_selected_label < 0) {
        automap_mouse_mode = 0;
        return 0;
    }
    label = &automap_labels[automap_selected_label];

    if (key != 0) {
        int c = key_to_ascii(key);

        if (c == 0xff) {
            switch (key) {
            case KEY_ESC:
            case KEY_ENTER:
                if (label->type == AUTOMAP_LABEL_CUSTOM) {
                    int script = level_script_find_index(cur_level_index, level_script_name);

                    SAVEGAME_AUTOMAP.automap_label_remove(cur_level_index, script, label);
                    SAVEGAME_AUTOMAP.automap_label_add(cur_level_index, script, label);
                }
                automap_mouse_mode = 0;
                return 0;
            case KEY_BACKSPACE: {
                int len = strlen(label->text);

                if (len > 0) {
                    label->text[len - 1] = 0;
                    changed = true;
                    automap_select_label(automap_selected_label);
                }
                break;
            }
            }
        } else {
            char s[2];                    // lives in the key argument's stack slot

            s[0] = (char)c;
            s[1] = 0;
            if (strlen(label->text) < 0x1f) {
                strcat(label->text, s);
                changed = true;
                automap_select_label(automap_selected_label);
            }
        }
    }

    if (automap_label_blink_timer.elapsed() || changed) {
        automap_label_blink_timer.SetTimeout(500);
        if (automap_label_cursor_on && !changed)
            automap_label_cursor_on = false;
        else
            automap_label_cursor_on = true;
    }

    if (mouse_was_pressed(0)) {
        if (label->type == AUTOMAP_LABEL_CUSTOM) {
            int script = level_script_find_index(cur_level_index, level_script_name);

            SAVEGAME_AUTOMAP.automap_label_remove(cur_level_index, script, label);
            SAVEGAME_AUTOMAP.automap_label_add(cur_level_index, script, label);
        }
        automap_label_editing = false;
        automap_mouse_mode = 5;
    }
    return 0;
}

// 0x403510  Confidence: exact. Mode 5: on release over the box, start typing; elsewhere deselect.
void automap_label_box_hover()
{
    if (automap_selected_label < 0 || mouse_is_down(0))
        return;
    int x = automap_view_pos[resolution_index][0] + automap_scroll_x + automap_label_box_x;
    int y = automap_view_pos[resolution_index][1] + automap_scroll_y + automap_label_box_y;

    if (point_in_rect(game_mouse_x, game_mouse_y, x, y, automap_label_w, automap_label_h)) {
        automap_mouse_mode = 4;
        return;
    }
    if (automap_label_editing)
        automap_select_label(-1);
    automap_mouse_mode = 0;
}

// 0x4035c0  Confidence: exact. Mode 6: on release over the pressed marker, select its label.
void automap_labels_hover()
{
    if (mouse_is_down(0))
        return;
    int x = automap_label_screen_pos[automap_hover_label][0] + automap_view_pos[resolution_index][0]
            - automap_label_bm_w / 2 + automap_scroll_x;
    int y = automap_label_screen_pos[automap_hover_label][1] + automap_view_pos[resolution_index][1]
            - automap_label_bm_h / 2 + automap_scroll_y;
    if (point_in_rect(game_mouse_x, game_mouse_y, x, y, automap_label_bm_w, automap_label_bm_h)) {
        automap_select_label(automap_hover_label);
        automap_mouse_mode = 0;
        return;
    }
    if (automap_label_editing)
        automap_select_label(-1);
    automap_mouse_mode = 0;
}

// 0x403690  Confidence: exact. Show_coordinates calls the empty hook 0x40acc0 (its debug body is not in
// the release build).
void automap_draw()
{
    int clip_h, clip_w, clip_y, clip_x;
    int i;

    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr__set_clip(0, 0, gr_screen_width(), gr_screen_height());
    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < AUTOMAP_NUM_BUTTONS; i++) {
        ui_button_state *b = &automap_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    automap_draw_map();
    automap_draw_title();
    if (Show_coordinates)
        empty_frame_hook_40acc0();
    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x403750  Confidence: likely (the two map draws end in one shared call, cross-jumped by VC6; the i / j
// counters share their stack slots with the map origin; operand order of some sums is a guess).
void automap_draw_map()
{
    float zoom;
    int i, j;
    float map_x, map_y;
    int clip_h, clip_w, clip_y, clip_x;
    int left;
    int player_x, player_y;
    int sx, sy;

    if (level_num_layers <= 0)
        return;

    zoom = automap_get_zoom();
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    automap_world_to_map(&map_x, &map_y, cur_character->pos.x, cur_character->pos.z);
    player_x = (int)(automap_layer_w * map_x * zoom + 0.5f);
    player_y = (int)(automap_layer_h * map_y * zoom + 0.5f);
    gr_set_color(255, 255, 255, 255);

    if (Map_mask) {
        int view_x = automap_view_pos[resolution_index][0];
        int view_y = automap_view_pos[resolution_index][1];
        int origin_x = view_x + automap_scroll_x;
        int origin_y = view_y + automap_scroll_y;
        int top, right, bottom;

        left = max(view_x, origin_x);
        top = max(view_y, origin_y);
        right = min(automap_view_size[resolution_index][0] + view_x,
                    (int)(automap_layer_w * zoom + origin_x + 0.5f));
        bottom = min(automap_view_size[resolution_index][1] + view_y,
                     (int)(automap_layer_h * zoom + origin_y + 0.5f));
        gr__set_clip(left, top, right - left, bottom - top);

        for (i = 0; i < automap_cells_x; i++) {
            for (j = 0; j < automap_cells_z; j++) {
                if (automap_is_cell_revealed(cur_level_index, automap_layer, i, j)) {
                    int x = (int)((i + 0.5f) * automap_scale_x + 0.5f)
                            + automap_view_pos[resolution_index][0] + automap_scroll_x;
                    int y = (int)((j + 0.5f) * automap_scale_y + 0.5f)
                            + automap_view_pos[resolution_index][1] + automap_scroll_y;

                    gr_bitmap_stretched(automap_mask_bm, x - automap_mask_tile_w / 2 - left,
                                        y - automap_mask_tile_h / 2 - top, automap_mask_tile_w,
                                        automap_mask_tile_h, 0, 0, automap_mask_w, automap_mask_h,
                                        gr_mode_automap_additive);
                }
            }
        }

        gr__set_clip(automap_view_pos[resolution_index][0], automap_view_pos[resolution_index][1],
                     automap_view_size[resolution_index][0], automap_view_size[resolution_index][1]);
        gr_bitmap_stretched(level_layer_maps[automap_layer], automap_scroll_x, automap_scroll_y,
                            (int)(automap_layer_w * zoom + 0.5f), (int)(automap_layer_h * zoom + 0.5f),
                            0, 0, automap_layer_w, automap_layer_h, GR_MODE_AUTOMAP_MASKED);
    } else {
        gr__set_clip(automap_view_pos[resolution_index][0], automap_view_pos[resolution_index][1],
                     automap_view_size[resolution_index][0], automap_view_size[resolution_index][1]);
        gr_bitmap_stretched(level_layer_maps[automap_layer], automap_scroll_x, automap_scroll_y,
                            (int)(automap_layer_w * zoom + 0.5f), (int)(automap_layer_h * zoom + 0.5f),
                            0, 0, automap_layer_w, automap_layer_h, gr_mode_ui_bitmap);
    }

    if (Map_grid) {
        int rows = (int)(automap_layer_h / automap_scale_y) + 1;
        int cols = (int)(automap_layer_w / automap_scale_x) + 1;

        gr_set_color(0, 128, 0, 255);
        for (i = 0; i < rows; i++) {
            float y = i * automap_scale_y + automap_scroll_y;

            gr_line((float)automap_scroll_x, y, (float)(automap_scroll_x + automap_layer_w), y, gr_line_mode);
        }
        for (i = 0; i < cols; i++) {
            float x = i * automap_scale_x + automap_scroll_x;

            gr_line(x, (float)automap_scroll_y, x, (float)(automap_scroll_y + automap_layer_h), gr_line_mode);
        }

        gr_set_color(0, 128, 128, 128);
        for (i = 0; i < automap_cells_x; i++) {
            for (j = 0; j < automap_cells_z; j++) {
                if (automap_is_cell_revealed(cur_level_index, automap_layer, i, j))
                    gr_rect((int)(i * automap_scale_x + automap_scroll_x + 0.5f),
                            (int)(j * automap_scale_y + automap_scroll_y + 0.5f),
                            (int)(automap_scale_x + 0.5f), (int)(automap_scale_y + 0.5f),
                            gr_rect_mode, 1.0f);
            }
        }
    }

    gr_set_color(255, 255, 255, 255);
    sx = automap_scroll_x + player_x;
    sy = automap_scroll_y + player_y;

    // The camera icon, rotated by the camera heading, half its size away from the player.
    if (automap_camera_bm >= 0) {
        float heading = game_camera_heading;
        int size = (int)(automap_camera_bm_h * automap_mask_scale * zoom + 0.5f);

        gr_bitmap_float(automap_camera_bm,
                        (float)((int)(sin(heading) * size * 0.5f + 0.5f)
                                + automap_view_pos[resolution_index][0] + sx),
                        (float)(automap_view_pos[resolution_index][1]
                                - (int)(cos(heading) * size * 0.5f + 0.5f) + sy),
                        automap_mask_scale * zoom, heading, gr_bitmap_wrap_mode);
    }

    {
        int w = (int)(automap_you_are_here_w * zoom + 0.5f);
        int h = (int)(automap_you_are_here_h * zoom + 0.5f);

        if (game_flags & 2) {
            // Multiplayer: a marker for every party member.
            Player *p;

            for (p = party_list.next; p != &party_list; p = p->next) {
                Character *c = living_from_handle(p->handle);

                if (c) {
                    automap_world_to_map(&map_x, &map_y, c->pos.x, c->pos.z);
                    gr_bitmap(automap_you_are_here_bm,
                              (int)(automap_layer_w * map_x * zoom + 0.5f) - w / 2 + automap_scroll_x,
                              (int)(automap_layer_h * map_y * zoom + 0.5f) - h / 2 + automap_scroll_y,
                              gr_bitmap_wrap_mode);
                }
            }
        } else {
            gr_bitmap(automap_you_are_here_bm, sx - w / 2, sy - h / 2, gr_bitmap_wrap_mode);
        }
    }

    {
        int map_w = (int)(automap_layer_w * zoom + 0.5f);
        int map_h = (int)(automap_layer_h * zoom + 0.5f);

        for (i = 0; i < automap_num_labels; i++) {
            if (automap_labels[i].layer == automap_layer
                && point_in_rect(automap_label_screen_pos[i][0], automap_label_screen_pos[i][1], 0, 0, map_w, map_h)
                && automap_map_point_revealed(automap_label_screen_pos[i][0], automap_label_screen_pos[i][1], true))
                automap_draw_label(i);
        }
    }

    gr__set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x403ff0  Confidence: exact (VC6 byte match) (the cursor tails return separately; w / h are reused for the box size;
// the margin members used for each offset are copied from the code, see NOTES).
void automap_draw_label(int index)
{
    int w, h;
    char text[0x120];
    int type = automap_labels[index].type;
    int x = automap_label_screen_pos[index][0] - automap_label_bm_w / 2 + automap_scroll_x;
    int y = automap_label_screen_pos[index][1] - automap_label_bm_h / 2 + automap_scroll_y;

    gr_set_color(255, 255, 255, 255);
    ui_bitmap_scaled(automap_label_bm, x, y, gr_bitmap_wrap_mode);
    if (index != automap_selected_label)
        return;

    if (automap_mouse_mode == 4) {
        // Typing: the box fits the text plus the cursor.
        strcpy(text, automap_label_text);
        strcat(text, "|");
        gr_get_string_size(&w, &h, text, -1, -1);
        w = w + (automap_label_margin.left + automap_label_margin.right) + 2;
        h = h + (automap_label_margin.top + automap_label_margin.bottom) + 2;
        x = automap_label_screen_pos[automap_selected_label][0] - w / 2 + automap_scroll_x;
        y = automap_label_box_y + automap_scroll_y;
        ui_draw_text_box(x, y, w, h);
        gr_set_color_ptr(&automap_label_type_colors[type]);
        gr_string(automap_label_margin.left + x + 1, automap_label_margin.top + y + 1, automap_label_text, -1,
                  gr_string_mode);
        if (automap_label_cursor_on) {
            int lines = 0;
            char *p = strchr(automap_label_text, '\n');
            char *last = NULL;

            while (p) {
                lines++;
                last = p;
                p = strchr(p + 1, '\n');
            }
            if (last) {
                gr_get_string_size(&w, &h, last + 1, -1, -1);
                gr_string(automap_label_margin.left + w + x + 1,
                          gr_get_font_height(-1) * lines + automap_label_margin.top + y + 1, "|", -1,
                          gr_string_mode);
                return;
            }
            gr_string(automap_label_w + automap_label_margin.left + x + 1, automap_label_margin.bottom + y + 1,
                      "|", -1, gr_string_mode);
            return;
        }
    } else {
        x = automap_label_box_x + automap_scroll_x;
        y = automap_label_box_y + automap_scroll_y;
        w = automap_label_w + automap_label_margin.left + automap_label_margin.right + 2;
        h = automap_label_h + automap_label_margin.top + automap_label_margin.bottom + 2;
        ui_draw_text_box(x, y, w, h);
        gr_set_color_ptr(&automap_label_type_colors[type]);
        gr_string(automap_label_margin.left + x + 1, automap_label_margin.left + y + 1, automap_label_text, -1,
                  gr_string_mode);
    }
}

// 0x404310  Confidence: exact. Level name centred on the background, then "%d of %d" (XSTR 639) when the
// level has automap layers.
void automap_draw_title()
{
    int w, h;
    char text[128];
    int layers;

    gr_set_color_ptr(&ui_color_text);
    {
        int level;

        if ((game_flags & 2) && (level = level_table_find_id(cur_level_index)) != -1)
            strcpy(text, level_table_display_name(level));
        else
            level_format_display_name(text, cur_level_index);
    }
    gr_get_string_size(&w, &h, text, -1, -1);
    gr_string((automap_buttons[0].w - w) / 2, automap_title_y[resolution_index], text, -1, gr_string_mode);

    layers = level_script_table[cur_level_index].automap_layers;
    if (layers > 0) {
        sprintf(text, XSTR("%d of %d", 639), automap_layer + 1, layers);
        gr_get_string_size(&w, &h, text, -1, -1);
        gr_string((automap_buttons[0].w - w) / 2, automap_layer_text_y[resolution_index], text, -1,
                  gr_string_mode);
    }
}

// 0x404460  Confidence: exact. Used by interface_set_view_clip.
int automap_get_panel_width()
{
    return automap_buttons[0].w;
}

// 0x404470  Confidence: exact. Case 3 of interface_mouse_over_panel.
bool automap_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, automap_buttons[0].x, automap_buttons[0].y, automap_buttons[0].w,
                         automap_buttons[0].h);
}

// ---- console commands ------------------------------------------------------------------------
// Each object's dynamic initialiser (thunk + body, static_initializers entries 3..6) comes right before
// its command function in .text, so the objects are defined here in the original (they are declared
// above in address order only so that this file has one definition each; see NOTES).
// 0x4044a0 init_ccmd_map_mask_thunk (jmp, entry 3). Confidence: exact.
// 0x4044b0 init_ccmd_map_mask: ccmd_map_mask (console_command constructor). Confidence: exact.

// 0x4044d0  Confidence: exact. The console's bool command macro, as cmd_max_skills in player.cpp.
void cmd_map_mask()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Map_mask = true;
        else if (arg_type & ARG_FALSE)
            Map_mask = false;
        else if (arg_type & ARG_NONE)
            Map_mask ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "map_mask", "Map_mask");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "map_mask", Map_mask ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x404590 init_ccmd_map_grid_thunk (jmp, entry 4). Confidence: exact.
// 0x4045a0 init_ccmd_map_grid: ccmd_map_grid (console_command constructor). Confidence: exact.

// 0x4045c0  Confidence: exact.
void cmd_map_grid()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Map_grid = true;
        else if (arg_type & ARG_FALSE)
            Map_grid = false;
        else if (arg_type & ARG_NONE)
            Map_grid ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "map_grid", "Map_grid");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "map_grid", Map_grid ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x404680 init_ccmd_show_coordinates_thunk (jmp, entry 5). Confidence: exact.
// 0x404690 init_ccmd_show_coordinates: ccmd_show_coordinates (console_command constructor). Confidence: exact.

// 0x4046b0  Confidence: exact.
void cmd_show_coordinates()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Show_coordinates = true;
        else if (arg_type & ARG_FALSE)
            Show_coordinates = false;
        else if (arg_type & ARG_NONE)
            Show_coordinates ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "show_coordinates", "Show_coordinates");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "show_coordinates", Show_coordinates ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x404770 init_ccmd_reset_map_labels_thunk (jmp, entry 6). Confidence: exact.
// 0x404780 init_ccmd_reset_map_labels: ccmd_reset_map_labels (console_command constructor). Confidence: exact.

// 0x4047a0  Confidence: exact. Puts back the level's default (permanent) labels and drops the custom
// labels of this level / script from the savegame.
void cmd_reset_map_labels()
{
    if (doing_help_for_comand) {
        print_to_console("Usage: reset_map_labels", 0);
        return;
    }
    if (!calling_a_command_function)
        return;
    if (gameseq_get_state() != 4) {
        print_to_console("Can only reset labels in gameplay mode.", 0);
        return;
    }
    if (automap_mouse_mode != 0) {
        print_to_console("Can only reset labels in automap idle state.", 0);
        return;
    }
    if (automap_labels_reset_default()) {
        print_to_console("Unable to reset automap labels.", 0);
        return;
    }
    SAVEGAME_AUTOMAP.automap_labels_reset_level(cur_level_index,
                                                level_script_find_index(cur_level_index, level_script_name));
    automap_labels_update_positions();
    automap_select_label(-1);
    print_to_console("Automap labels reset.", 0);
}

// A dynamic initialiser with an empty body (static_initializers entry 7), i.e. a static object defined
// after ccmd_reset_map_labels whose constructor is an empty inline. Nothing in the unit's .bss is left for
// it (every byte up to 0x5b2db7 is accounted for), so it is probably an empty object (sizeof 1, maybe in a
// hole). Not reproduced: see NOTES.
// 0x404860 init_empty_404870_thunk (jmp, entry 7). Confidence: exact.
// 0x404870 init_empty_404870 (a bare ret). Confidence: unsure (which object).

// 0x404880 `vector constructor iterator` (??_H; (array, size, count, constructor), thiscall constructor
// per element, ret 0x10), the compiler-generated helper with no source. The linker keeps the first COMDAT
// copy in link order, and this one follows new_automap.cpp's code, so this unit's object defines it (no
// function of the unit calls it in v1.40; the empty initialiser above is the only candidate). Confidence:
// likely (the placement; the code is the compiler's, D5502).
