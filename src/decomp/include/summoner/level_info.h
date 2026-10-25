// summoner/level_info.h - the current level's names, party starts and layer maps (gameplay.cpp's .bss
// 0x60a2c8-0x60a40b). Not original source. One object (D7201): build 8966 keeps level_entry_point's store
// after the inline strcpy calls into level_filename / level_script_name / level_prev_filename (every level
// change: ls_jadetemple_finish 0x4f0a1f, ls_rand_hills01_goto_khosanistrng, ...), which it does only for
// members of one object; level_find_player_starts reads level_player_starts[4] as level_entry_point
// (0x4cafcc). The struct and field names are ours; the old global names are macros. level_is_town
// (0x60a40c) and the camera globals before 0x60a2c8 stay separate.
#ifndef SUMMONER_LEVEL_INFO_H
#define SUMMONER_LEVEL_INFO_H

#include "vsdk/vsdk.h"

struct navpoint;

struct level_info_block {
    char filename[32];                 // +0x00 0x60a2c8 current level name (game_cleanup clears the three names)
    char script_name[32];              // +0x20 0x60a2e8
    char prev_filename[32];            // +0x40 0x60a308
    float session_time;                // +0x60 0x60a328 seconds in the level, not counting pauses
    navpoint *player_starts[4];        // +0x64 0x60a32c filled by level_find_player_starts
    int entry_point;                   // +0x74 0x60a33c start point index
    int num_layers;                    // +0x78 0x60a340
    int layer_maps[10];                // +0x7c 0x60a344 automap bitmap of each layer
    int layer_automap_index[40];       // +0xa4 0x60a36c
};
LAYOUT_CHECK_32(level_info_block, sizeof(level_info_block) == 0x144);

extern level_info_block level_info;    // 0x60a2c8

#define level_filename            level_info.filename
#define level_script_name         level_info.script_name
#define level_prev_filename       level_info.prev_filename
#define level_session_time        level_info.session_time
#define level_player_starts       level_info.player_starts
#define level_entry_point         level_info.entry_point
#define level_num_layers          level_info.num_layers
#define level_layer_maps          level_info.layer_maps
#define level_layer_automap_index level_info.layer_automap_index

#endif
