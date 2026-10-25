// engine/rendering/unnamed_snow_0x4946a0.h - falling snow (code 0x4946a0-0x495281, the snow and
// snow_render console toggles included; source game/unnamed_snow_0x4946a0.cpp, decomp54). The unit's
// file name is not known (no assert path, no strings.tbl section); this header's name and directory are
// placeholders. Not original source. Names from sum.symbols; the ones marked "ours" are new.
// The particles live in a 15 x 15 grid of 5 m cells around the camera (snow_cells, re-centred by
// scrolling whole rows / columns); a visible cell borrows a block of 32 flakes from the pool.
#ifndef ENGINE_RENDERING_UNNAMED_SNOW_0X4946A0_H
#define ENGINE_RENDERING_UNNAMED_SNOW_0X4946A0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/console.h"

#define SNOW_GRID          15         // cells per side
#define SNOW_CELL_SIZE     5.0f       // metres
#define SNOW_FLAKES        32         // per cell (0x200-byte block)
#define SNOW_POOL_BLOCKS   100

// One flake (0x10 bytes). Names ours.
struct snow_flake {
    vector3 pos;                      // +0x0
    ubyte dir;                        // +0xC snow_offsets index (0..31)
    ubyte _pad0d[3];
};
LAYOUT_CHECK(snow_flake, sizeof(snow_flake) == 0x10);

// One grid cell (0xc bytes, 225 at 0x24445b0). Type and field names ours; the methods are thiscall in
// the binary (ECX = the cell).
class snow_cell {
public:
    snow_cell() {}                    // empty (static initialiser 269 or 270)
    void reset();                     // flakes = NULL: folded into 0x4dac10 (conversation_reset_state)
    void fill();                      // 0x494770 snow_cell_fill (all 32 flakes)
    void fill_count(int n);           // 0x4948a0 snow_cell_fill_count (n flakes at the scroll row, ret 4)
    bool update();                    // 0x4949f0 snow_cell_update: true = culled (block freed) or no block
    int x;                            // +0x0 grid coordinates (world / 5)
    int z;                            // +0x4
    snow_flake *flakes;               // +0x8 block of snow_cell_pool, NULL when none
};
LAYOUT_CHECK_32(snow_cell, sizeof(snow_cell) == 0xc);

extern int snow_grid_x;                               // 0x2444598 camera cell (name ours)
extern int snow_grid_z;                               // 0x244459c (name ours)
extern console_command ccmd_snow_render;              // 0x24445a0
extern snow_cell snow_cell_records[SNOW_GRID][SNOW_GRID];   // 0x24445b0 (name ours)
extern vector3 snow_offsets[SNOW_FLAKES];             // 0x2445040 fall direction per flake kind (name ours)
extern float snow_camera_y;                           // 0x24451c0 lowest visible height (sum.symbols name)
extern bool snow_reset_cells;                         // 0x24451c4
extern snow_cell *snow_cells[SNOW_GRID][SNOW_GRID];   // 0x24451c8
extern snow_flake snow_cell_pool[SNOW_POOL_BLOCKS][SNOW_FLAKES];   // 0x2445550
extern console_command ccmd_snow;                     // 0x2451d50
extern float snow_base_y;                             // 0x2451d5c highest visible height
extern ubyte snow_cell_used[13];                      // 0x2451d60 bit per pool block
extern int snow_scroll_row;                           // 0x2451d70
extern float snow_scroll_accum;                       // 0x2451d74
extern bool snow_active;                              // 0x2451d78
extern bool Snow_enabled;                             // 0x59c5e8 console "snow" (init 1; the cutscene file reads it)
extern bool Snow_render;                              // 0x59c5e9 console "snow_render" (init 1)
extern int snow_flake_bm;                             // 0x59c5ec (init -1)

snow_flake *snow_cell_alloc();                        // 0x4946e0 NULL when the pool is used up
void snow_cell_free(snow_flake *block);               // 0x494730
void snow_start();                                    // 0x494ab0 (moved from levelrender.h, decomp54)
void snow_stop();                                     // 0x494b80 (moved from levelrender.h)
void snow_update(float frametime);                    // 0x494ba0 (fx_frame)
void snow_render();                                   // 0x495010 (levelrender_render_scene)
void cmd_snow();                                      // 0x4950e0
void cmd_snow_render();                               // 0x4951d0

#endif
