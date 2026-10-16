// summoner/automap.h - the map labels, the map corners and the reveal grid (automap_* 0x42fc00-0x4302b0,
// module "-" in sum.symbols; .bss 0x5f9838-0x5f9e6c). decomp76 wrote them in the unit 0x42f1e0-0x430baf
// (game/unnamed_worldmap_automap_0x42f1e0.cpp, a placeholder name; its own header
// summoner/unnamed_worldmap_automap_0x42f1e0.h declares the rest). The screen itself is new_automap.cpp
// (summoner/interface/new_automap.h). Directory and file name ours. Not original source.
#ifndef SUMMONER_AUTOMAP_H
#define SUMMONER_AUTOMAP_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

// One map label (0x30 bytes). The layout is read from new_automap.cpp: +4 / +8 are the world x / z passed
// to automap_world_to_map, +0xc the layer compared with automap_layer, +0x10 the text (31 characters,
// automap_label_text_input stops at strlen 0x1f). sum.symbols' "level +8, position +0xc" is off.
// Type name ours.
struct automap_label {
    int   type;                                   // +0x00 0 permanent, 1 custom (automap_label_type_colors)
    float x;                                      // +0x04 world x
    float z;                                      // +0x08 world z
    int   layer;                                  // +0x0c
    char  text[0x20];                             // +0x10
};
LAYOUT_CHECK(automap_label, sizeof(automap_label) == 0x30);
#define AUTOMAP_LABEL_CUSTOM 1
#define AUTOMAP_MAX_LABELS   32

extern automap_label automap_labels[AUTOMAP_MAX_LABELS];   // 0x5f9838 (sum.symbols module new_automap.cpp)
extern int     automap_num_labels;                         // 0x5f9e64
extern vector3 automap_lower_right;                        // 0x5f9e48 ($mapLowerRight)
extern vector3 automap_upper_left;                         // 0x5f9e58 ($mapUpperLeft)
extern int     automap_cells_x;                            // 0x5f9e68 reveal grid width
extern int     automap_cells_z;                            // 0x5f9e6c reveal grid height

void automap_world_to_map(float *out_x, float *out_y, float x, float z);   // 0x42ffe0 0..1 map fractions
void automap_get_world_scale(float *out_x, float *out_z);  // 0x430030
int  automap_is_cell_revealed(int level, int layer, int x, int y);   // 0x4300c0 0 / 1
int  automap_layer_map_index(int layer);                   // 0x430220
int  automap_label_alloc();                                // 0x430230 index, 0 when full (sic)
void automap_label_delete(int index);                      // 0x430250
int  automap_labels_reset_default();                       // 0x4302b0 nonzero = failure (type guessed)
void automap_reveal_all();                                 // 0x42fc00 (GameInit)

// decomp42 (state_game_enter / state_game_frame).
void automap_find_map_corners();      // 0x42fc60
void automap_reveal_around_party();   // 0x42ff50

#endif
