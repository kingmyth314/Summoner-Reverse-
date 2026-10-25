// summoner/unnamed_worldmap_automap_0x42f1e0.h - the unit 0x42f1e0-0x430baf (file name not known; source
// game/unnamed_worldmap_automap_0x42f1e0.cpp, decomp76): the PXO login reader, the world map location
// markers, the map labels / corners / reveal grid, the save_map_labels command and game states 7 and 9.
// Most of its functions were declared by earlier batches (summoner/automap.h, summoner/interface/
// interface.h, summoner/multi/multi.h, summoner/summoner.h); this header adds the rest. Not original
// source.
#ifndef SUMMONER_UNNAMED_WORLDMAP_AUTOMAP_0X42F1E0_H
#define SUMMONER_UNNAMED_WORLDMAP_AUTOMAP_0X42F1E0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "summoner/automap.h"
#include "summoner/multi/unnamed_utrack_0x48d7c0.h"

struct gr_vertex;
struct navpoint;

// ---- PXO login --------------------------------------------------------------------------------------
// The login name and password (sum.symbols pxo_login_name 0x5f9670 char[33], pxo_login_password 0x5f9691
// char[27]) are the two fields of the request pxo_login_request takes (utrack_login_request); one object
// here (name ours), the sum.symbols names are macros on its fields.
extern utrack_login_request pxo_login_info;           // 0x5f9670
#define pxo_login_name (pxo_login_info.login)         // 0x5f9670
#define pxo_login_password (pxo_login_info.password)  // 0x5f9691
// 0 idle (pxo_login_reset), 1 no login stored, 2 request refused at once, 3 request sent.
extern int pxo_login_state;                           // 0x5f96b8
void pxo_read_login();                                // 0x42f1e0

// ---- world map location markers (level 4) -----------------------------------------------------------
// One location (0x2c bytes; sum.symbols type name). flags: 1 the location is known in this game stage
// (worldmap_init_locations), 2 its marker is drawn on the screen edge (off screen), 4 its label is drawn.
struct worldmap_location {
    unsigned int flags;               // +0x00
    navpoint *nav;                    // +0x04 navpoint_find of the location's tag
    float scale;                      // +0x08 marker scale by distance (worldmap_marker_scale_for_distance)
    float angle;                      // +0x0c direction from the screen centre (worldmap_marker_screen_angle)
    float x, y;                       // +0x10 marker position on the screen border
    float label_x, label_y;           // +0x18 label box origin
    int label_w, label_h;             // +0x20 label box size (text size plus margins)
    const char *name;                 // +0x28 the location's display name
};
LAYOUT_CHECK_32(worldmap_location, sizeof(worldmap_location) == 0x2c);
#define WORLDMAP_NUM_LOCATIONS 8
extern worldmap_location worldmap_locations[WORLDMAP_NUM_LOCATIONS];   // 0x5f96c8
extern float worldmap_center_x;                       // 0x5f9828 the party leader's screen position
extern float worldmap_center_y;                       // 0x5f982c
extern int worldmap_overlay_bm;                       // 0x5f9830 the marker bitmap ("LandmarkArrow")
extern bool worldmap_show_locations;                  // 0x5f9834 toggled by key 0x26 (L)
void worldmap_init_locations();                       // 0x42f3b0
float worldmap_marker_scale_for_distance(float dist); // 0x42f6f0
float worldmap_marker_screen_angle(const gr_vertex *v);   // 0x42f750
void worldmap_marker_place_on_edge(worldmap_location *loc);   // 0x42f7d0
void worldmap_marker_clamp_to_edge(worldmap_location *loc);   // 0x42f870 places the label next to the marker
void worldmap_center_label(worldmap_location *loc, const gr_vertex *v);   // 0x42fa50
void worldmap_draw_location(worldmap_location *loc);  // 0x42fac0

// ---- map labels, corners, reveal grid (the rest is in summoner/automap.h) ----------------------------
extern const char *automap_label_type_names[2];       // 0x592dbc {"Permanent", "Custom"}
extern int automap_corners_found;                     // 0x5f9e70
void automap_labels_parse_table();                    // 0x42fd40 automap_labels.tbl
// (out x, out y, world x, world z): the reveal cell (16 world units per cell).
void automap_world_to_reveal_cell(int *out_x, int *out_y, float x, float z);   // 0x430070
int  automap_has_map_corners();                       // 0x4300b0
// (level, layer, x, y, value): the reveal bit of a cell, and of the same cell of the twin level
// (7 / 49, 30 / 35, 31 / 36) when both have as many map layers.
void automap_reveal_cell(int level, int layer, int x, int y, bool value);   // 0x430110
void automap_labels_clear();                          // 0x4302a0
void cmd_save_map_labels();                           // 0x430430
// 0 saved, 1 no level, 2 no map corners, 3 automap_labels.tbl not opened, 4 the temporary file not
// opened, 5 the table not deleted, 6 the temporary file not renamed.
int  automap_labels_save();                           // 0x430580

// ---- game states 7 (debug help text, never entered) and 9 (credits) ----------------------------------
extern char state7_message[1024];                     // 0x5f9e74
// (state_debug_help_enter .. state_credits_frame are declared in summoner/summoner.h.)

#endif
