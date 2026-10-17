// unnamed_snow_0x4946a0.cpp - Sum.exe v1.40, code 0x4946a0-0x495281: falling snow (a 15 x 15 grid of 5 m
// cells around the camera, each with a block of 32 flakes from a 100-block pool) and the snow /
// snow_render console toggles.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: two empty static initialisers (entries 269-270, 0x4946a0-0x4946d0) after the camera
// sequence unit, then the cell pool, the cells, start / stop / update / render and the two console
// commands (entries 271-272) up to cmd_snow_render (0x4951d0-0x495281); the serpent unit's timer
// initialiser (entry 273) follows at 0x495290. By .bss and .data the console commands end this unit
// (the tu-split2 cut at 0x4950b0 put them into the serpent unit; source-map.md).
// Data: .bss 0x2444598-0x2451d78 (grid origin, ccmd_snow_render 0x24445a0, the cell records, the flake
// directions, the cell pointers, the pool, ccmd_snow 0x2451d50, ...), .data 0x59c5e8-0x59c65b (the two
// toggles, snow_flake_bm, "SnowFlake.tga" and the console strings). The literals 2.6666667 (0x581b0c)
// and 0.2 (0x581b10) are this unit's; 5.0, 32.0, 0.375, 0.5, 1.5, -0.5, pi/180 are pooled.

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "engine/rendering/unnamed_snow_0x4946a0.h"
#include "engine/rendering/levelrender.h"
#include "engine/camera/camera.h"
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/console.h"
#include "vsdk/math/vector.h"

#define SNOW_FALL_RATE 2.6666667f     // flakes respawned per cell per second (scroll rows)

// ---- .bss / .data ----------------------------------------------------------------------------------
int snow_grid_x;                                              // 0x2444598
int snow_grid_z;                                              // 0x244459c
// 0x4946a0 init_empty_4946b0_thunk (jmp, static initialiser entry 269). Confidence: exact.
// 0x4946b0 init_empty_4946b0 (ret). Confidence: unsure (which array it constructs).
// 0x4946c0 init_empty_4946d0_thunk (jmp, entry 270). Confidence: exact.
// 0x4946d0 init_empty_4946d0 (ret): with 0x4946b0, the empty constructors of the two arrays below
//          (snow_cell(), vector3()). Confidence: unsure (which is which).
snow_cell snow_cell_records[SNOW_GRID][SNOW_GRID];             // 0x24445b0
vector3 snow_offsets[SNOW_FLAKES];                            // 0x2445040
float snow_camera_y;                                          // 0x24451c0
bool snow_reset_cells;                                        // 0x24451c4
snow_cell *snow_cells[SNOW_GRID][SNOW_GRID];                  // 0x24451c8
snow_flake snow_cell_pool[SNOW_POOL_BLOCKS][SNOW_FLAKES];     // 0x2445550
float snow_base_y;                                            // 0x2451d5c
ubyte snow_cell_used[13];                                     // 0x2451d60
int snow_scroll_row;                                          // 0x2451d70
float snow_scroll_accum;                                      // 0x2451d74
bool snow_active;                                             // 0x2451d78
bool Snow_enabled = true;                                     // 0x59c5e8
bool Snow_render = true;                                      // 0x59c5e9
int snow_flake_bm = -1;                                       // 0x59c5ec

// 0x4946e0  Confidence: exact (VC6 byte match, cl 8966; was likely: the two loop guards on the block index). The
// first free pool block.
snow_flake *snow_cell_alloc()
{
    int idx = 0;
    for (int b = 0; b < 13; b++) {
        if (idx >= SNOW_POOL_BLOCKS)
            break;
        for (int bit = 0; bit < 8; idx++, bit++) {
            if (idx >= SNOW_POOL_BLOCKS)
                break;
            int mask = 1 << bit;
            if (!(snow_cell_used[b] & mask)) {
                snow_cell_used[b] |= mask;
                return snow_cell_pool[idx];
            }
        }
    }
    return NULL;
}

// 0x494730  Confidence: exact.
void snow_cell_free(snow_flake *block)
{
    if (block == NULL)
        return;
    int idx = (block - &snow_cell_pool[0][0]) / SNOW_FLAKES;
    ubyte mask = 1 << (idx & 7);
    int i = idx >> 3;
    snow_cell_used[i] = snow_cell_used[i] & ~mask;
}

// Folded: identical-code folding merged this body with conversation_reset_state (0x4dac10); snow_start
// calls that copy. Confidence: likely.
void snow_cell::reset()
{
    flakes = NULL;
}

// 0x494770  Confidence: exact (VC6 byte match, cl 8966; was likely: the origin as a vector3 local). The 32 flakes
// from the top of the
// visible range downwards along their fall direction (12 s to 0.375 s of fall), jittered +-5 m.
void snow_cell::fill()
{
    if (flakes == NULL)
        return;
    vector3 origin;
    float t;
    origin.x = (float)x * SNOW_CELL_SIZE;
    origin.y = snow_base_y;
    t = 12.0f;
    origin.z = (float)z * SNOW_CELL_SIZE;
    for (int i = 0; i < SNOW_FLAKES; i++) {
        int k = (snow_scroll_row + i) % SNOW_FLAKES;
        flakes[k].dir = (ubyte)(int)(frand() * 32.0f);
        flakes[k].pos = snow_offsets[flakes[k].dir] * t + origin;
        flakes[k].pos.x += frand_range(-5.0f, 5.0f);
        flakes[k].pos.z += frand_range(-5.0f, 5.0f);
        t -= 0.375f;
    }
}

// 0x4948a0  Confidence: likely. (n): n new flakes at the top, from the scroll row on (the step per
// flake works out at 0.375 s, as in fill).
void snow_cell::fill_count(int n)
{
    if (flakes == NULL)
        return;
    vector3 origin;
    origin.x = (float)x * SNOW_CELL_SIZE;
    origin.y = snow_base_y;
    origin.z = (float)z * SNOW_CELL_SIZE;
    float t = n / SNOW_FALL_RATE;
    float step = t / n;
    for (int i = 0; i < n; i++) {
        int k = (snow_scroll_row + i) % SNOW_FLAKES;
        flakes[k].dir = (ubyte)(int)(frand() * 32.0f);
        flakes[k].pos = snow_offsets[flakes[k].dir] * t + origin;
        flakes[k].pos.x += frand_range(-5.0f, 5.0f);
        flakes[k].pos.z += frand_range(-5.0f, 5.0f);
        t -= step;
    }
}

// 0x4949f0  Confidence: exact (VC6 byte match, cl 8966; was likely: the box corners as two vector3 locals). A cell
// outside the view gives
// its block back; a visible one gets a block. True when the cell has no block afterwards.
bool snow_cell::update()
{
    vector3 mins, maxs;
    mins.x = ((float)x - 0.5f) * SNOW_CELL_SIZE;
    mins.y = snow_camera_y;
    mins.z = ((float)z - 0.5f) * SNOW_CELL_SIZE;
    maxs.x = ((float)x + 0.5f) * SNOW_CELL_SIZE;
    maxs.y = snow_base_y;
    maxs.z = ((float)z + 0.5f) * SNOW_CELL_SIZE;
    if (gr_cull_bounding_box(&mins, &maxs)) {
        if (flakes) {
            snow_cell_free(flakes);
            flakes = NULL;
        }
        return true;
    }
    if (flakes == NULL)
        flakes = snow_cell_alloc();
    return flakes == NULL;
}

// 0x494ab0  Confidence: likely (differs under cl 12.00.8966, match23; was exact; the 13-byte clear is memset expanded: three dwords and a byte).
// ls_lenele3d_init, ls_lenele3a_init, ls_ionaext02_init.
void snow_start()
{
    int i, j;

    snow_flake_bm = bm_load("SnowFlake.tga", -1, -1);
    for (i = 0; i < SNOW_GRID; i++) {
        snow_cell *rec = snow_cell_records[i];
        for (j = 0; j < SNOW_GRID; j++) {
            snow_cells[i][j] = rec++;
            snow_cells[i][j]->reset();
        }
    }
    memset(snow_cell_used, 0, sizeof(snow_cell_used));
    for (i = 0; i < SNOW_FLAKES; i++) {
        snow_offsets[i].random_unit_vector();
        snow_offsets[i] *= 0.5f;
        snow_offsets[i].y -= 1.5f;
    }
    snow_scroll_row = 0;
    snow_scroll_accum = 0;
    snow_active = true;
    snow_reset_cells = true;
}

// 0x494b80  Confidence: exact. The blocks are not given back (snow_start clears the bit map).
void snow_stop()
{
    if (snow_active)
        snow_active = false;
}

// 0x494ba0  Confidence: likely (the grid rounding as a conditional per axis; the scroll loops). From
// fx_frame with the frame time. The visible height range (the view's vertical fov around its pitch, out
// to the fog distance), the camera cell, then the grid: re-centred by one row / column when the camera
// moved one cell, rebuilt otherwise; the flakes fall and the scroll row is refilled.
// GAME BUG (NOTES.md): the row scroll for a camera moving to +x refills the cell when snow_cell::update
// returns true (no block), the other three directions when it returns false, so the cells that scroll
// in on the +x side keep their old flakes.
void snow_update(float frametime)
{
    int i, j;
    snow_cell *c;

    if (!snow_active || !Snow_enabled)
        return;

    float fog_far = fog_get_far_current();
    vector3 &fvec = camera_view_orient.fvec;
    float horiz = (float)sqrt(fvec.z * fvec.z + fvec.x * fvec.x);
    if (horiz == 0.0f) {
        if (fvec.y >= 0.0f) {
            snow_base_y = camera_view_pos.y + fog_far;
            snow_camera_y = camera_view_pos.y;
        } else {
            snow_base_y = camera_view_pos.y;
            snow_camera_y = camera_view_pos.y - fog_far;
        }
    } else {
        float h_fov, v_fov;
        float pitch = (float)atan(fvec.y / horiz);
        gr_get_view_fov(&h_fov, &v_fov);
        float half = (float)(v_fov * 0.017453292f) * 0.5f;
        float top = (float)sin(half + pitch) * fog_far + camera_view_pos.y;
        if (!(top > camera_view_pos.y))
            top = camera_view_pos.y;
        snow_base_y = top;
        float bottom = (float)sin(pitch - half) * fog_far + camera_view_pos.y;
        if (!(bottom < camera_view_pos.y))
            bottom = camera_view_pos.y;
        snow_camera_y = bottom;
    }

    if (camera_view_pos.x > -0.5f)
        snow_grid_x = (int)(camera_view_pos.x * 0.2f + 0.5f);
    else
        snow_grid_x = (int)(camera_view_pos.x * 0.2f + 0.5f) - 1;
    if (camera_view_pos.z > -0.5f)
        snow_grid_z = (int)(camera_view_pos.z * 0.2f + 0.5f);
    else
        snow_grid_z = (int)(camera_view_pos.z * 0.2f + 0.5f) - 1;

    snow_cell *centre = snow_cells[7][7];
    if (!snow_reset_cells && abs(snow_grid_x - centre->x) <= 1 && abs(snow_grid_z - centre->z) <= 1) {
        if (snow_grid_x > centre->x) {
            for (j = 0; j < SNOW_GRID; j++) {
                c = snow_cells[0][j];
                for (i = 0; i < SNOW_GRID - 1; i++)
                    snow_cells[i][j] = snow_cells[i + 1][j];
                c->x += SNOW_GRID;
                snow_cells[SNOW_GRID - 1][j] = c;
                if (c->update())
                    c->fill();
            }
        } else if (snow_grid_x < centre->x) {
            for (j = 0; j < SNOW_GRID; j++) {
                c = snow_cells[SNOW_GRID - 1][j];
                for (i = SNOW_GRID - 1; i > 0; i--)
                    snow_cells[i][j] = snow_cells[i - 1][j];
                c->x += -SNOW_GRID;
                snow_cells[0][j] = c;
                if (!c->update())
                    c->fill();
            }
        }
        if (snow_grid_z > centre->z) {
            for (i = 0; i < SNOW_GRID; i++) {
                c = snow_cells[i][0];
                for (j = 0; j < SNOW_GRID - 1; j++)
                    snow_cells[i][j] = snow_cells[i][j + 1];
                c->z += SNOW_GRID;
                snow_cells[i][SNOW_GRID - 1] = c;
                if (!c->update())
                    c->fill();
            }
        } else if (snow_grid_z < centre->z) {
            for (i = 0; i < SNOW_GRID; i++) {
                c = snow_cells[i][SNOW_GRID - 1];
                for (j = SNOW_GRID - 1; j > 0; j--)
                    snow_cells[i][j] = snow_cells[i][j - 1];
                c->z += -SNOW_GRID;
                snow_cells[i][0] = c;
                if (!c->update())
                    c->fill();
            }
        }
    } else {
        for (i = 0; i < SNOW_GRID; i++) {
            for (j = 0; j < SNOW_GRID; j++) {
                snow_cells[i][j]->x = snow_grid_x + i - 7;
                snow_cells[i][j]->z = snow_grid_z + j - 7;
                if (!snow_cells[i][j]->update())
                    snow_cells[i][j]->fill();
            }
        }
        snow_reset_cells = false;
    }

    snow_scroll_accum += SNOW_FALL_RATE * frametime;
    int n = (int)snow_scroll_accum;
    for (i = 0; i < SNOW_GRID; i++) {
        for (j = 0; j < SNOW_GRID; j++) {
            bool was_empty = snow_cells[i][j]->flakes == NULL;
            if (snow_cells[i][j]->update())
                continue;
            if (was_empty) {
                snow_cells[i][j]->fill();
            } else {
                for (int k = 0; k < SNOW_FLAKES; k++) {
                    snow_flake *f = &snow_cells[i][j]->flakes[k];
                    f->pos += snow_offsets[f->dir] * frametime;
                }
                if (n > 0)
                    snow_cells[i][j]->fill_count(n);
            }
        }
    }
    snow_scroll_accum -= (float)n;
    snow_scroll_row = (snow_scroll_row + n) % SNOW_FLAKES;
}

// 0x495010  Confidence: exact. levelrender_render_scene.
void snow_render()
{
    if (!snow_active || !Snow_enabled || !Snow_render)
        return;
    gr_set_texture(snow_flake_bm, -1);
    gr_set_color(255, 255, 255, 255);
    for (int i = 0; i < SNOW_GRID; i++) {
        for (int j = 0; j < SNOW_GRID; j++) {
            if (snow_cells[i][j]->flakes) {
                for (int k = 0; k < SNOW_FLAKES; k++)
                    gr_bitmap_3d(&snow_cells[i][j]->flakes[k].pos, 0, 0.07f, gr_mode_particle_additive);
            }
        }
    }
}

// ---- console commands -------------------------------------------------------------------------------
// 0x4950b0 init_ccmd_snow_thunk (jmp, static initialiser entry 271). Confidence: exact.
// 0x4950c0 init_ccmd_snow (console_command constructor). Confidence: exact.
console_command ccmd_snow("snow", "Toggles Snow_enabled", cmd_snow);   // 0x2451d50

// 0x4950e0  Confidence: exact (the console's bool command macro).
void cmd_snow()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Snow_enabled = true;
        else if (arg_type & ARG_FALSE)
            Snow_enabled = false;
        else if (arg_type & ARG_NONE)
            Snow_enabled ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "snow", "Snow_enabled");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "snow", Snow_enabled ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4951a0 init_ccmd_snow_render_thunk (jmp, static initialiser entry 272). Confidence: exact.
// 0x4951b0 init_ccmd_snow_render (console_command constructor). Confidence: exact.
console_command ccmd_snow_render("snow_render", "Toggles Snow_render", cmd_snow_render);   // 0x24445a0

// 0x4951d0  Confidence: exact.
void cmd_snow_render()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Snow_render = true;
        else if (arg_type & ARG_FALSE)
            Snow_render = false;
        else if (arg_type & ARG_NONE)
            Snow_render ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "snow_render", "Snow_render");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "snow_render", Snow_render ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
