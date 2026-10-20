// unnamed_worldmap_automap_0x42f1e0.cpp - Sum.exe v1.40, code 0x42f1e0-0x430baf (file name not known:
// no path literal, no strings.tbl block; the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs) by decomp76. Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression
// form is a guess), unsure (the shape itself is a guess).
//
// One unit: its .rdata 0x580d98-0x580df3, .data 0x592c8c-0x5930b7 and .bss 0x5f9670-0x5fa273 are
// contiguous, and the static initialiser entries 90-92 (the two map corners, ccmd_save_map_labels) lie in
// its code. It holds the PXO login reader (pxo_*), the world map location markers of level 4
// (worldmap_*, drawn by the HUD through interface_frame / interface_draw), the map labels, corners and
// reveal grid that the map screen (new_automap.cpp) uses (automap_*), the console command
// save_map_labels, and the handlers of game states 7 (a debug help text, never entered) and 9 (credits).
//
// .data: the registry and table literals, the marker bitmap name, the eight location tags and names,
// automap_label_type_names, the messages. The literals "" (0x5bd0d0) and "#End" (0x58bd08) are pooled
// copies of earlier units (/Gf).

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/registry.h"
#include "vsdk/os/console.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "levelscripts/savegame.h"
#include "summoner/automap.h"
#include "summoner/unnamed_worldmap_automap_0x42f1e0.h"

// ---- .rdata (0x580d98; names ours) ------------------------------------------------------------------
// The screen border the off-screen markers sit on, per resolution_index: {left, right} and {top, bottom}.
const int worldmap_screen_edge_x[3][2] = { { 10, 629 }, { 13, 786 }, { 16, 1006 } };   // 0x580d98
const int worldmap_screen_edge_y[3][2] = { { 10, 377 }, { 13, 471 }, { 16, 603 } };    // 0x580db0
// The label box margins (the same four values as new_automap.cpp's automap_label_margin, 0x57e958).
struct worldmap_label_margins {
    int left, right, top, bottom;
};
const worldmap_label_margins worldmap_label_margin = { 1, 0, 1, 1 };                   // 0x580dc8
// How far an off-screen marker's label is pushed back from the border, per resolution_index.
const int worldmap_marker_label_offset[3] = { 20, 25, 32 };                            // 0x580dd8
// 0.025f, 50.0f, 0.0625f and 16.0f (0x580de4-0x580df0) are this unit's __real constants.

// ---- .data (0x592c8c) -------------------------------------------------------------------------------
const char *worldmap_marker_bitmap = "LandmarkArrow";   // 0x592cb0 (name ours)
// The level-file tags of the eight locations and their display names (names ours).
const char *worldmap_location_tags[WORLDMAP_NUM_LOCATIONS] = {          // 0x592cb4
    "$masad", "$lenele", "$iona", "$ikaemos", "$khosani", "$eleh", "$wolong", "$liangshan"
};
const char *worldmap_location_names[WORLDMAP_NUM_LOCATIONS] = {         // 0x592cd4
    "Masad", "Lenele", "Iona", "Ikaemos", "Khosani", "Eleh", "Wolong", "Liangshan"
};
const char *automap_label_type_names[2] = { "Permanent", "Custom" };   // 0x592dbc

// ---- .bss (0x5f9670-0x5fa273, address order) ----------------------------------------------------------
utrack_login_request pxo_login_info;          // 0x5f9670 pxo_login_name / pxo_login_password
char pxo_login_reply[12];                     // 0x5f96ac
int pxo_login_state;                          // 0x5f96b8
int worldmap_overlay_w;                       // 0x5f96c0 (name ours; written, never read)
int worldmap_overlay_h;                       // 0x5f96c4 (name ours)
worldmap_location worldmap_locations[WORLDMAP_NUM_LOCATIONS];   // 0x5f96c8
float worldmap_center_x;                      // 0x5f9828
float worldmap_center_y;                      // 0x5f982c
int worldmap_overlay_bm;                      // 0x5f9830
bool worldmap_show_locations;                 // 0x5f9834
automap_label automap_labels[AUTOMAP_MAX_LABELS];   // 0x5f9838
// ccmd_save_map_labels 0x5f9e38, automap_lower_right 0x5f9e48 and automap_upper_left 0x5f9e58 are defined
// with their initialisers below.
int automap_num_labels;                       // 0x5f9e64
int automap_cells_x;                          // 0x5f9e68
int automap_cells_z;                          // 0x5f9e6c
int automap_corners_found;                    // 0x5f9e70
char state7_message[1024];                    // 0x5f9e74

// ---- PXO login --------------------------------------------------------------------------------------

// 0x42f1e0  Confidence: exact. The stored login (HKLM\...\THQMultiplay Login / Password); without both
// the state is 1, else the request goes out (state 3, 2 when pxo_login_request refuses at once). The
// reply buffer is cleared up to 10 of its 12 bytes.
void pxo_read_login()
{
    os_registry_read_string("THQMultiplay", "Login", pxo_login_name, "");
    os_registry_read_string("THQMultiplay", "Password", pxo_login_password, "");
    if (strlen(pxo_login_name) == 0 || strlen(pxo_login_password) == 0) {
        pxo_login_state = 1;
        return;
    }
    memset(pxo_login_reply, 0, 10);
    pxo_login_state = pxo_login_request(&pxo_login_info, pxo_login_reply) ? 3 : 2;
}

// 0x42f280  Confidence: exact.
void pxo_login_reset()
{
    pxo_login_state = 0;
}

// 0x42f290  Confidence: exact. 0 logged in, 1 no account, 2 cannot contact the server, 3 busy, 4 invalid
// user name (multi.h); every final answer resets the state.
int pxo_login_step()
{
    int r;

    switch (pxo_login_state) {
    case 0:
        pxo_read_login();
        return 3;
    case 1:
        pxo_login_state = 0;
        return 1;
    case 2:
    case 3:
        r = pxo_login_request(NULL, NULL);
        if (r == -2) {
            pxo_login_state = 0;
            return 2;
        }
        if (r == 1) {
            pxo_login_state = 0;
            return 0;
        }
        if (r == -1) {
            pxo_login_state = 0;
            return 4;
        }
        return 3;
    }
    return 0;
}

// ---- world map location markers -----------------------------------------------------------------

// 0x42f320  Confidence: exact. Only on the world map (level 4): the marker bitmap and the locations.
void worldmap_load_overlay()
{
    char name[64];

    if (cur_level_index == 4) {
        sprintf(name, "%s%s", worldmap_marker_bitmap, ui_bitmap_suffix_tga[ui_hires]);
        worldmap_overlay_bm = bm_load(name, path_interface, -1);
        if (worldmap_overlay_bm >= 0)
            bm_get_dimensions(worldmap_overlay_bm, &worldmap_overlay_w, &worldmap_overlay_h);
        else {
            worldmap_overlay_w = 0;
            worldmap_overlay_h = 0;
        }
        worldmap_init_locations();
        worldmap_show_locations = true;
    }
}

// 0x42f3b0  Confidence: exact (VC6 byte match). Which locations the map shows by the
// game stage: Masad before stage 9; Lenele, Iona (stage 2 on), Ikaemos (with the "got_ikaemos_map" flag of
// level 5) and Khosani (with this level's "found_khosani" flag) outside stages 9-14; Eleh (14 on), Wolong
// and Liangshan (13 on) inside them. The label size is the text plus the margins plus 2.
void worldmap_init_locations()
{
    int stage;
    bool early;
    bool middle;
    int i;
    int w, h;
    worldmap_location *loc;

    stage = game_stage_get();
    early = stage < 9;
    middle = stage >= 9 && stage < 15;
    for (i = 0; i < WORLDMAP_NUM_LOCATIONS; i++) {
        loc = &worldmap_locations[i];
        loc->flags = 0;
        loc->nav = navpoint_find(worldmap_location_tags[i]);
        loc->name = worldmap_location_names[i];
        gr_get_string_size(&w, &h, loc->name, -1, -1);
        loc->label_w = worldmap_label_margin.left + worldmap_label_margin.right + w + 2;
        loc->label_h = worldmap_label_margin.top + worldmap_label_margin.bottom + h + 2;
        switch (i) {
        case 0:
            if (early)
                loc->flags |= 1;
            break;
        case 1:
            if (!middle)
                loc->flags |= 1;
            break;
        case 2:
            if (stage >= 2 && !middle)
                loc->flags |= 1;
            break;
        case 3:
            if (savegame_state.get_flag(5, "got_ikaemos_map") && !middle)
                loc->flags |= 1;
            break;
        case 4:
            if (savegame_state.get_flag(cur_level_index, "found_khosani") && !middle)
                loc->flags |= 1;
            break;
        case 5:
            if (stage >= 14 && middle)
                loc->flags |= 1;
            break;
        case 6:
            if (middle)
                loc->flags |= 1;
            break;
        case 7:
            if (stage >= 13 && middle)
                loc->flags |= 1;
            break;
        }
    }
}

// 0x42f500  Confidence: likely (VC6: the distance goes through a stack temporary; same size). From interface_frame
// on the world
// map. Key 0x26 (L) toggles the markers (sound 0x27) and is eaten. With the party leader on screen, each
// known location is placed: when its tag is outside the view (gr_cull_sphere), a marker on the screen
// border in the direction of a point 1 unit from the leader towards it (flags 2 and 4, D5805), else its
// label centred on its projected tag (flag 4).
int worldmap_frame(int key)
{
    gr_vertex v;
    gr_vertex p;
    worldmap_location *loc;
    int i;

    if (!cur_character)
        return key;
    if (key == 0x26) {
        worldmap_show_locations = !worldmap_show_locations;
        key = 0;
        game_play_sound(0x27, 1.0f, 0, 0, 0);
    }
    gr_rotate_vertex(&v, &cur_character->pos);
    gr_project_vertex(&v);
    if (v.flags != 1)
        return key;
    worldmap_center_x = v.sx;
    worldmap_center_y = v.sy;
    for (i = 0; i < WORLDMAP_NUM_LOCATIONS; i++) {
        loc = &worldmap_locations[i];
        loc->flags &= ~6;
        if (!(loc->flags & 1))
            continue;
        if (gr_cull_sphere(&loc->nav->pos, 1.0f, false)) {
            vector3 d;
            d = loc->nav->pos - cur_character->pos;
            loc->scale = worldmap_marker_scale_for_distance(d.len());
            d.normalize_safe();
            d += cur_character->pos;
            gr_rotate_vertex(&p, &d);
            gr_project_vertex(&p);
            if (p.flags != 1)
                continue;
            loc->angle = worldmap_marker_screen_angle(&p);
            worldmap_marker_place_on_edge(loc);
            worldmap_marker_clamp_to_edge(loc);
            loc->flags |= 6;
        } else {
            gr_rotate_vertex(&p, &loc->nav->pos);
            gr_project_vertex(&p);
            if (p.flags != 1)
                continue;
            worldmap_center_label(loc, &p);
            loc->flags |= 4;
        }
    }
    return key;
}

// 0x42f6f0  Confidence: exact (the halving is its own statement: VC6 folds f * 0.5 + 0.5 into (f + 1) * 0.5). Full
// size within 0 units, half size from 50 on.
float worldmap_marker_scale_for_distance(float dist)
{
    float f = (50.0f - dist) * 0.025f;

    if (f <= 0.0f)
        f = 0.0f;
    else if (f >= 1.0f)
        f = 1.0f;
    f *= 0.5f;
    return f + 0.5f;
}

// 0x42f750  Confidence: exact. The angle of the point around the screen centre, measured from +x about +z.
float worldmap_marker_screen_angle(const gr_vertex *v)
{
    vector3 x_axis(1.0f, 0.0f, 0.0f);
    vector3 d;

    d.x = v->sx - worldmap_center_x;
    d.y = v->sy - worldmap_center_y;
    d.z = 0.0f;
    vector3 z_axis(0.0f, 0.0f, 1.0f);
    return vector3_angle_and_axis(&x_axis, &d, &z_axis);
}

// 0x42f7d0  Confidence: exact. The marker goes along the angle from the centre, scaled by the distance
// to the border on that side (an ellipse-like path through the four border distances).
void worldmap_marker_place_on_edge(worldmap_location *loc)
{
    float c, s;

    c = (float)cos(loc->angle);
    loc->x = (c >= 0.0f ? worldmap_screen_edge_x[resolution_index][1] - worldmap_center_x
                        : worldmap_center_x - worldmap_screen_edge_x[resolution_index][0]) * c + worldmap_center_x;
    s = (float)sin(loc->angle);
    loc->y = (s >= 0.0f ? worldmap_screen_edge_y[resolution_index][1] - worldmap_center_y
                        : worldmap_center_y - worldmap_screen_edge_y[resolution_index][0]) * s + worldmap_center_y;
}

// 0x42f870  Confidence: exact (D16001: the cos / sin factors of the last two lines parenthesised). Places
// the label next to the marker:
// the direction to the border is normalised, both half-diagonals of the label box are projected onto it,
// and the box corner is offset by the projection with the smaller dot product (subtracted when it is
// positive); then the label is pulled back towards the centre by the per-resolution offset along the
// angle and by half its size.
void worldmap_marker_clamp_to_edge(worldmap_location *loc)
{
    vector3 dir;
    vector3 pa, pb;
    float c, s;
    float da, db;

    c = (float)cos(loc->angle);
    dir.x = (c >= 0.0f ? worldmap_screen_edge_x[resolution_index][1] - worldmap_center_x
                       : worldmap_center_x - worldmap_screen_edge_x[resolution_index][0]) * c;
    s = (float)sin(loc->angle);
    dir.y = (s >= 0.0f ? worldmap_screen_edge_y[resolution_index][1] - worldmap_center_y
                       : worldmap_center_y - worldmap_screen_edge_y[resolution_index][0]) * s;
    dir.z = 0.0f;
    dir.normalize_safe();
    vector3 a(loc->label_w * 0.5f, loc->label_h * 0.5f, 0.0f);
    vector3 b(loc->label_w * 0.5f, -loc->label_h * 0.5f, 0.0f);
    da = vector3_project_onto_normal(&pa, &a, &dir);
    db = vector3_project_onto_normal(&pb, &b, &dir);
    if (da <= db) {
        if (da <= 0.0f) {
            loc->label_x = pa.x + loc->x;
            loc->label_y = pa.y + loc->y;
        } else {
            loc->label_x = loc->x - pa.x;
            loc->label_y = loc->y - pa.y;
        }
    } else {
        if (db <= 0.0f) {
            loc->label_x = pb.x + loc->x;
            loc->label_y = pb.y + loc->y;
        } else {
            loc->label_x = loc->x - pb.x;
            loc->label_y = loc->y - pb.y;
        }
    }
    loc->label_x -= worldmap_marker_label_offset[resolution_index] * ((float)cos(loc->angle)) + loc->label_w * 0.5f;
    loc->label_y -= worldmap_marker_label_offset[resolution_index] * ((float)sin(loc->angle)) + loc->label_h * 0.5f;
}

// 0x42fa50  Confidence: exact.
void worldmap_center_label(worldmap_location *loc, const gr_vertex *v)
{
    loc->label_x = v->sx - loc->label_w * 0.5f;
    loc->label_y = v->sy - loc->label_h * 0.5f;
}

// 0x42fa80  Confidence: exact. From interface_draw on the world map.
void worldmap_draw_locations()
{
    int i;

    if (worldmap_show_locations && worldmap_overlay_bm >= 0) {
        for (i = 0; i < WORLDMAP_NUM_LOCATIONS; i++) {
            if (worldmap_locations[i].flags & 6)
                worldmap_draw_location(&worldmap_locations[i]);
        }
    }
}

// 0x42fac0  Confidence: exact. The marker bitmap points along the angle (+pi)
// at the resolution's scale times the distance scale; the label is a text box with the name.
void worldmap_draw_location(worldmap_location *loc)
{
    float scale;
    int x, y;

    switch (resolution_index) {
    case 0:
        scale = 1.0f;
        break;
    case 1:
        scale = 1.25f;
        break;
    case 2:
        scale = 1.6f;
        break;
    default:
        scale = 1.0f;
        break;
    }
    gr_set_color(0xff, 0xff, 0xff, 0xff);
    if (loc->flags & 2)
        gr_bitmap_float(worldmap_overlay_bm, loc->x, loc->y, scale * loc->scale, loc->angle + 3.14159265f,
                        gr_bitmap_wrap_mode);
    if (loc->flags & 4) {
        x = (int)(loc->label_x + 0.5f);
        y = (int)(loc->label_y + 0.5f);
        ui_draw_text_box(x, y, loc->label_w, loc->label_h);
        gr_set_color_ptr(&ui_color_text);
        gr_string(worldmap_label_margin.left + x + 1, worldmap_label_margin.top + y + 1, loc->name, -1,
                  gr_string_mode);
    }
}

// ---- map corners, labels and the reveal grid -------------------------------------------------------

// The two map corners (vector3's empty inline constructor; D500).
// 0x42fbc0 init_empty_42fbd0_thunk (jmp, static initialiser entry 90). Confidence: exact.
// 0x42fbd0 init_empty_42fbd0 (a bare ret). Confidence: likely (which of the two corners).
vector3 automap_lower_right;                  // 0x5f9e48 $mapLowerRight
// 0x42fbe0 init_empty_42fbf0_thunk (jmp, entry 91). Confidence: exact.
// 0x42fbf0 init_empty_42fbf0 (a bare ret). Confidence: likely.
vector3 automap_upper_left;                   // 0x5f9e58 $mapUpperLeft

// 0x42fc00  Confidence: exact. From GameInit: every cell of every map layer of every level gets the value
// 0, so the grid starts hidden (the name says reveal; D5805).
void automap_clear_all()
{
    int level;
    int layer;
    int x, y;

    for (level = 0; level < NUM_LEVEL_SCRIPTS; level++) {
        for (layer = 0; layer < level_script_table[level].automap_layers; layer++)
            for (x = 0; x < 64; x++)
                for (y = 0; y < 64; y++)
                    automap_reveal_cell(level, layer, x, y, false);
    }
}

// 0x42fc60  Confidence: exact. The corners come from the level-file markers;
// every layer must have a map. The reveal grid has one cell per 16 world units.
void automap_find_map_corners()
{
    matrix orient;
    int i;

    automap_corners_found = 1;
    if (!level_marker_find("$mapUpperLeft", &automap_upper_left, &orient)) {
        printf("AUTOMAP ERROR : Could not find $mapUpperLeft tag...\n");
        automap_corners_found = 0;
        return;
    }
    if (!level_marker_find("$mapLowerRight", &automap_lower_right, &orient)) {
        printf("AUTOMAP ERROR : Could not find $mapLowerRight tag...\n");
        automap_corners_found = 0;
        return;
    }
    if (level_num_layers <= 0) {
        automap_corners_found = 0;
        return;
    }
    for (i = 0; i < level_num_layers; i++) {
        if (level_layer_maps[i] < 0) {
            automap_corners_found = 0;
            return;
        }
    }
    automap_cells_x = (int)((float)fabs(automap_upper_left.x - automap_lower_right.x) * 0.0625f);
    automap_cells_z = (int)((float)fabs(automap_upper_left.z - automap_lower_right.z) * 0.0625f);
    automap_labels_parse_table();
}

// 0x42fd40  Confidence: exact (D15001: the two level tests as separate early returns put the shared
// early-exit epilogue after the Open test, as the binary). The section "#Script: "<level script>"" of
// automap_labels.tbl: $Label: "text" $Type: Permanent / Custom $Pos: x, z $Layer: n per label. The labels
// are cleared first. automap_label_alloc returns 0 when the 32 labels are used, so the < 0 test never ends
// the loop and later labels overwrite label 0 (latent).
void automap_labels_parse_table()
{
    TextFile tf;
    char buf[0x100];
    int index;
    automap_label *lab;

    if (cur_level_index == -1)
        return;
    if (!automap_corners_found)
        return;
    automap_labels_clear();
    if (tf.Open("automap_labels.tbl", 0x98967f))
        return;
    sprintf(buf, "#Script: \"%s\"", level_script_name);
    if (tf.SkipPast(buf, "#End")) {
        while (tf.Accept("$Label:")) {
            index = automap_label_alloc();
            if (index < 0)
                break;
            lab = &automap_labels[index];
            tf.ParseString(buf, 0x100, '"', '"');
            if ((int)strlen(buf) <= 0x1f)
                strcpy(lab->text, buf);
            else {
                strncpy(lab->text, buf, 0x1f);
                lab->text[0x1f] = '\0';
            }
            tf.Expect("$Type:");
            lab->type = tf.ParseTypeString(automap_label_type_names, 2);
            tf.Expect("$Pos:");
            lab->x = tf.ParseFloat();
            tf.Expect(",");
            lab->z = tf.ParseFloat();
            tf.Expect("$Layer:");
            lab->layer = tf.ParseInt();
        }
    }
    tf.Close();
}

// 0x42ff50  Confidence: exact. From state_game_frame: the cell under each party member that is not hidden
// or dying, on the map layer of the member's layer.
void automap_reveal_around_party()
{
    Player *p;
    Character *c;
    int x, y;
    int layer;

    if (!automap_has_map_corners())
        return;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (!c || (c->flags & CF_HIDDEN))
            continue;
        automap_world_to_reveal_cell(&x, &y, c->pos.x, c->pos.z);
        layer = level_layer_automap_index[c->layer];
        if (layer < 0)
            continue;
        automap_reveal_cell(cur_level_index, layer, x, y, true);
    }
}

// 0x42ffe0  Confidence: exact. 0..1 fractions of the map from the upper left corner (x grows to the west,
// z to the south).
void automap_world_to_map(float *out_x, float *out_y, float x, float z)
{
    float dz = z - automap_upper_left.z;
    float dx = automap_upper_left.x - x;

    *out_x = dx / (float)fabs(automap_upper_left.x - automap_lower_right.x);
    *out_y = dz / (float)fabs(automap_upper_left.z - automap_lower_right.z);
}

// 0x430030  Confidence: exact. One reveal cell (16 world units) as a fraction of the map.
void automap_get_world_scale(float *out_x, float *out_z)
{
    *out_x = 16.0f / (float)fabs(automap_upper_left.x - automap_lower_right.x);
    *out_z = 16.0f / (float)fabs(automap_upper_left.z - automap_lower_right.z);
}

// 0x430070  Confidence: exact.
void automap_world_to_reveal_cell(int *out_x, int *out_y, float x, float z)
{
    float dz = z - automap_upper_left.z;
    float dx = automap_upper_left.x - x;

    *out_x = (int)(dx * 0.0625f);
    *out_y = (int)(dz * 0.0625f);
}

// 0x4300b0  Confidence: exact.
int automap_has_map_corners()
{
    return automap_corners_found;
}

// 0x4300c0  Confidence: exact. The savegame bit (layer * 64 + y) * 64 + x of the level; 0 out of range.
int automap_is_cell_revealed(int level, int layer, int x, int y)
{
    if (level < 0 || level >= NUM_LEVEL_SCRIPTS || layer >= level_script_table[level].automap_layers)
        return 0;
    return savegame_state.get_bit(level, ((layer << 6) + y << 6) + x);
}

// 0x430110  Confidence: exact (value is a bool passed on: set_bit tests its byte). The twin levels share their map:
// the same bit is set in
// the other one when both have as many map layers.
void automap_reveal_cell(int level, int layer, int x, int y, bool value)
{
    int bit;
    int twin;

    if (level < 0 || level >= NUM_LEVEL_SCRIPTS || layer >= level_script_table[level].automap_layers)
        return;
    bit = ((layer << 6) + y << 6) + x;
    savegame_state.set_bit(level, bit, value);
    switch (level) {
    case 7:
        twin = 49;
        break;
    case 49:
        twin = 7;
        break;
    case 35:
        twin = 30;
        break;
    case 30:
        twin = 35;
        break;
    case 36:
        twin = 31;
        break;
    case 31:
        twin = 36;
        break;
    default:
        return;
    }
    if (level_script_table[twin].automap_layers == level_script_table[level].automap_layers)
        savegame_state.set_bit(twin, bit, value);
}

// 0x430220  Confidence: exact.
int automap_layer_map_index(int layer)
{
    return level_layer_automap_index[layer];
}

// 0x430230  Confidence: exact. 0 (not -1) when the 32 labels are used.
int automap_label_alloc()
{
    if (automap_num_labels < AUTOMAP_MAX_LABELS)
        return automap_num_labels++;
    return 0;
}

// 0x430250  Confidence: exact (the size is a pointer difference).
void automap_label_delete(int index)
{
    if (index < 0 || index >= AUTOMAP_MAX_LABELS)
        return;
    memmove(&automap_labels[index], &automap_labels[index + 1],
            (char *)&automap_labels[automap_num_labels] - (char *)&automap_labels[index + 1]);
    automap_num_labels--;
}

// 0x4302a0  Confidence: exact.
void automap_labels_clear()
{
    automap_num_labels = 0;
}

// 0x4302b0  Confidence: exact. From cmd_reset_map_labels: one permanent label at the start navpoint of
// each level character that is a shopkeeper, with its display name. The shopkeeper is found by comparing
// the name pointers (the table's name with character_get_name's result), not the strings (D5805).
int automap_labels_reset_default()
{
    char name[0x100];
    int i;
    int index;
    Character *c;
    automap_label *lab;
    navpoint *nav;

    if (cur_level_index == -1)
        return 1;
    if (!automap_corners_found)
        return 1;
    automap_labels_clear();
    for (i = 0; i < level_shopkeeper_count; i++) {
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (level_shopkeepers[i].name == c->get_name())
                break;
        }
        if (c == &living_list)
            continue;
        index = automap_label_alloc();
        if (index < 0)
            continue;
        lab = &automap_labels[index];
        lab->type = 0;
        nav = (navpoint *)c->start_navpoint;
        lab->x = nav->pos.x;
        lab->z = ((navpoint *)c->start_navpoint)->pos.z;
        lab->layer = level_layer_automap_index[((navpoint *)c->start_navpoint)->layer];
        c->get_display_name(name);
        if ((int)strlen(name) > 0x1f) {
            strncpy(lab->text, name, 0x1f);
            lab->text[0x1f] = '\0';
        } else
            strcpy(lab->text, name);
    }
    return 0;
}

// 0x430400 init_ccmd_save_map_labels_thunk (jmp, static initialiser entry 92). Confidence: exact.
// 0x430410 init_ccmd_save_map_labels (console_command constructor). Confidence: exact.
console_command ccmd_save_map_labels("save_map_labels", "Save automap labels for this level/script",
                                     cmd_save_map_labels);   // 0x5f9e38

// 0x430430  Confidence: exact. Only in game play (state 4).
void cmd_save_map_labels()
{
    char msg[0x100];

    if (doing_help_for_comand) {
        print_to_console("Usage: save_map_labels", 0);
        return;
    }
    if (calling_a_command_function) {
        if (gameseq_get_state() != 4) {
            print_to_console("Can only save labels in gameplay mode.", 0);
            return;
        }
        switch (automap_labels_save()) {
        case 0:
            strcpy(msg, "Automap labels succesfully saved.");
            break;
        case 1:
            strcpy(msg, "No level loaded.");
            break;
        case 2:
            strcpy(msg, "No map for this level.");
            break;
        case 3:
            sprintf(msg, "Error opening %s for reading.", "automap_labels.tbl");
            break;
        case 4:
            sprintf(msg, "Error opening %s for writing.", "automap_labels_temp.tbl");
            break;
        case 5:
            sprintf(msg, "Error deleting %s.", "automap_labels.tbl");
            break;
        case 6:
            sprintf(msg, "Error renaming %s to %s.", "automap_labels_temp.tbl", "automap_labels.tbl");
            break;
        default:
            strcpy(msg, "Unknown error while saving");
            break;
        }
        print_to_console(msg, 0);
    }
}

// 0x430580  Confidence: exact (D15000: the open results through a named local, compared with the zero register;
// `while (1)` with the "#End" test's else-part last keeps the binary's unrotated loop). Copies
// automap_labels.tbl into automap_labels_temp.tbl up to this level's "#Script:" line (or up to "#End",
// where the section is appended), writes the permanent labels (type 0) instead of the section's old ones,
// copies the rest (or writes "#End"), then replaces the table with the copy. The first loop does not test
// ReadLine's result: a table without "#End" and without this level's section never ends it (latent).
int automap_labels_save()
{
    TextFile tf;
    File del;
    File out;
    char line[0x200];
    char header[0x40];
    bool at_end;
    int i;
    automap_label *lab;
    int err;

    if (cur_level_index == -1)
        return 1;
    if (!automap_corners_found)
        return 2;
    err = tf.Open("automap_labels.tbl", path_tables);
    if (err != 0)
        return 3;
    err = out.open("automap_labels_temp.tbl", 0x80000002, path_tables);
    if (err != 0)
        return 4;
    sprintf(header, "#Script: \"%s\"", level_script_name);
    while (1) {
        tf.ReadLine(line, 0x200);
        if (strncmp(line, "#End", 4) != 0) {
            out.write_line(line, -1);
            if (strcmp(line, header) == 0) {
                out.write_line(NULL, -1);
                at_end = !tf.SkipTo("#Script:", "#End");
                break;
            }
        } else {
            out.write_line(header, -1);
            out.write_line(NULL, -1);
            at_end = true;
            break;
        }
    }
    for (i = 0; i < automap_num_labels; i++) {
        lab = &automap_labels[i];
        if (lab->type != 0)
            continue;
        sprintf(line, "\t$Label:\t\"%s\"", lab->text);
        out.write_line(line, -1);
        sprintf(line, "\t$Type:\t\"%s\"", automap_label_type_names[lab->type]);
        out.write_line(line, -1);
        sprintf(line, "\t$Pos:\t\t%f, %f", lab->x, lab->z);
        out.write_line(line, -1);
        sprintf(line, "\t$Layer:\t%d", lab->layer);
        out.write_line(line, -1);
        out.write_line(NULL, -1);
    }
    if (at_end)
        out.write_line("#End", -1);
    else {
        while (tf.ReadLine(line, 0x200))
            out.write_line(line, -1);
    }
    tf.Close();
    if (del.Delete("automap_labels.tbl", path_tables) != 0)
        return 5;
    out.close();
    if (out.make_backup("automap_labels.tbl", "automap_labels_temp.tbl", path_tables) != 0)
        return 6;
    return 0;
}

// ---- game states 7 and 9 ----------------------------------------------------------------------------

// 0x430ad0  Confidence: exact. State 7 enter (no code sets state 7 in v1.40).
void state_debug_help_enter()
{
    if (strlen(state7_message) == 0)
        strcpy(state7_message, "You forgot to set the 'Debug_help_str' variable up");
}

// 0x430b10  Confidence: exact.
void state7_leave()
{
    state7_message[0] = '\0';
}

// 0x430b20  Confidence: exact. The text at the top left on a cleared screen; a key returns to the previous
// state.
void state7_frame()
{
    switch (ui_poll_and_process(NULL, true)) {
    case 1:
        gameseq_set_state(gameseq_get_prev_state());
        break;
    }
    gr::reset_clip();
    gr_clear();
    gr_set_color(0x60, 0x60, 0x60, 0xff);
    gr_string(0, 0, state7_message, -1, gr_string_mode);
}

// 0x430b80  Confidence: exact. State 9: the credits screen (21).
void state_credits_enter()
{
    interface_set_screen(21, NULL);
}

// 0x430b90  Confidence: exact.
void state_credits_frame()
{
    cursor_visible = false;
    interface_set_view_clip();
    gr_clear();
    interface_frame(PollInputEvents());
    interface_draw();
}
