// level_script_khosani_lab.cpp - Sum.exe v1.40, code 0x4f77c0-0x4f9e4b (levelscripts\Scripts\
// level_script_khosani_lab.cpp; strings.tbl ids 492-499 are this unit's, docs/sum-exe/source-map.md).
// Written from the Sum.exe disassembly (tools/asm.mjs; tables and floats read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (same, but one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// The Khosani laboratory ("the Ordeal"): Ulsadana's room intros, the golem rooms (stepping on the symbol,
// the pylon doors, the prisoners, the rune sequence), the flipper puzzle with its lava platforms and the
// beam path drawn as avoidance splines, and the Giant Salamanka fight. Static initialiser table entry 491
// (0x5887b0) is this unit's.
// Data: .data 0x5a3c78-0x5a4538 (the two 10 x 7 tables, then the literals); .bss 0x257eb10-0x257ecf9.
// Pooled literals of other units: "KhosaniLab" 0x594790, "BOSS-GateA-Door01" 0x5a280c, "BOSS-GateB-Door01" 0x5a27f8,
// "$salstart" 0x5a2838, "Giant Salamanka" 0x5967f8, "Ring of Fire" 0x593e78, "Ulsadana" 0x59bfc0,
// "room8" 0x59ee60, "got_ring_of_fire" 0x59ed9c.
//
// Header (D1400): the level script units have no header of their own. The table hooks are declared in
// levelscripts/level_scripts.h (the gathering header); the helpers below are declared at the top of this
// file. Nothing outside the unit calls them or reads the unit's globals (tools/asm.mjs grep).

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/script_internal.h"

// A puzzle grid cell or a grid direction (+-1 unit vector). Type name ours.
struct ls_khosanilab_cell {
    int x, y;
};

// Puzzle grid cell bits (names ours): bit 0 open (the flipper is up), 2 / 4 / 8 / 0x10 the side the beam
// passed (ls_khosanilab_puzzle_dir_to_side_bit), 0x20 on the flood fill's path, 0x40 visited by it.
#define KL_CELL_OPEN     0x1
#define KL_CELL_SIDES    0x1e
#define KL_CELL_FLOOD    0x20
#define KL_CELL_VISITED  0x40

void ls_khosanilab_puzzle_update();
void ls_khosanilab_puzzle_flush_path();
void ls_khosanilab_puzzle_clear_paths();
bool ls_khosanilab_puzzle_flood_connected(ls_khosanilab_cell c);
bool ls_khosanilab_puzzle_cell_open(const ls_khosanilab_cell *c, bool state);
bool ls_khosanilab_puzzle_cell_valid(const ls_khosanilab_cell *c);
void ls_khosanilab_puzzle_trace_beam(const ls_khosanilab_cell *start, int dir, bool state);
void ls_khosanilab_puzzle_turn_dirs(ls_khosanilab_cell *a, ls_khosanilab_cell *b, ls_khosanilab_cell *c,
                                    vector2 *fb, vector2 *fc, int dir);
void ls_khosanilab_puzzle_reflect_dir(int *out, const ls_khosanilab_cell *d);
void ls_khosanilab_puzzle_corner_nn(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner);
void ls_khosanilab_puzzle_corner_np(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner);
void ls_khosanilab_puzzle_corner_pp(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner);
void ls_khosanilab_puzzle_choose_turn(const ls_khosanilab_cell *cell, int dir, int kind, bool state);
int  ls_khosanilab_puzzle_dir_to_side_bit(const ls_khosanilab_cell *d);
void ls_khosanilab_puzzle_emit_segment(const vector2 *p0, const vector2 *p1, bool state);
void ls_khosanilab_puzzle_cell_to_world(const vector2 *cell, vector2 *out);
void ls_khosanilab_puzzle_add_point(const vector2 *a, const vector2 *b);
void ls_khosanilab_puzzle_idle_path();
void ls_khosanilab_apply_lava_platforms();
void ls_khosanilab_room1_frame();
void lsf_room1_killed();
void ls_khosanilab_room4_frame();
void ls_khosanilab_room4_check_done();
void ls_khosanilab_reset_runes();
void ls_khosanilab_room1_intro();
void ls_khosanilab_room2_intro();
void ls_khosanilab_room4_intro();
void ls_khosanilab_room5_intro();
void ls_khosanilab_room7_intro();
void lsf_room8();
void ls_khosanilab_salamanka_fight();
void ls_khosanilab_salamanka_intro_cs_end();
void ls_khosanilab_salamanka_death_cs_end();
void ls_khosanilab_salamanka_trans1_cs_end();
void ls_khosanilab_salamanka_trans2_cs_end();
void ls_khosanilab_pylon_use(const char *name);
void ls_khosanilab_room2_doors();
void ls_khosanilab_puzzle_use(int flipper);
void ls_khosanilab_puzzle_build_grid();
void ls_khosanilab_set_flippers(int row, int col);
void ls_khosanilab_room7_doors();

// ---- .data (0x5a3c78-) -------------------------------------------------------------------------------
// The flipper door number ("puzzle-flippers%02d") of each cell of the 10 x 7 board; the zero border makes
// ls_khosanilab_set_flippers' neighbours of an edge flipper no-ops. Rows 1..8, columns 1..5 are the beam
// grid (ls_khosanilab_puzzle_build_grid). Name ours.
int ls_khosanilab_flipper_ids[10][7] = {                     // 0x5a3c78
    { 0,  0,  0,  0,  0,  0, 0 },
    { 0,  0,  0,  1,  0,  0, 0 },
    { 0,  6,  5,  4,  3,  2, 0 },
    { 0,  7,  8,  9, 10, 11, 0 },
    { 0, 32, 15, 14, 13, 12, 0 },
    { 0, 31, 16, 17, 18, 19, 0 },
    { 0, 30, 25, 24, 23, 20, 0 },
    { 0, 29, 26, 27, 22, 21, 0 },
    { 0,  0,  0, 28,  0,  0, 0 },
    { 0,  0,  0,  0,  0,  0, 0 },
};
// The lava platform number ("lavaplatform%02d") shown while the flipper of the same cell is up. Every zero
// cell names "lavaplatform00" (see NOTES). Name ours.
int ls_khosanilab_lava_platform_ids[10][7] = {               // 0x5a3d90
    { 0,  0,  0,  0,  0,  0, 0 },
    { 0,  0,  0, 32,  0,  0, 0 },
    { 0, 30, 24, 18, 12,  6, 0 },
    { 0, 29, 23,  1, 11,  5, 0 },
    { 0, 28, 22, 16, 10,  4, 0 },
    { 0, 27, 21, 15,  9,  3, 0 },
    { 0, 26, 20, 14,  8,  2, 0 },
    { 0, 25, 19, 13,  7, 17, 0 },
    { 0,  0,  0, 31,  0,  0, 0 },
    { 0,  0,  0,  0,  0,  0, 0 },
};

// ---- .bss (0x257eb10-0x257ecf9) ----------------------------------------------------------------------
bool ls_khosanilab_ring_of_fire_done;                        // 0x257eb10
// sum.symbols: int[40]; the code indexes it [x * 5 + y] (x 0..7, y 0..4).
int ls_khosanilab_puzzle_grid[8][5];                         // 0x257eb14

// 0x4f77c0  Confidence: exact. static_init_ls_khosanilab_timer: "jmp 0x4f77d0" (initialiser entry 491).
// 0x4f77d0  Confidence: exact. init_ls_khosanilab_timer: "mov ecx, 0x257ebb4 / jmp GameTimer::GameTimer".
GameTimer ls_khosanilab_timer;                               // 0x257ebb4

int ls_khosanilab_path_len;                                  // 0x257ebb8 (0x257ebbc is never referenced)
// sum.symbols: uint8[0xc4]; avoid_spline_create's description (D1402).
avoid_spline_def ls_khosanilab_path;                         // 0x257ebc0
int ls_khosanilab_path_splines[10];                          // 0x257ec84
int ls_khosanilab_path_spline_count;                         // 0x257ecac
bool ls_khosanilab_salstart_done;                            // 0x257ecb0
// sum.symbols: uint8[0x46]. bool: the state is pushed to door_apply_saved_state as is (0x4f9ce7) and
// toggled with test / sete.
bool ls_khosanilab_flippers[10][7];                          // 0x257ecb4

// ---- the beam path through the flipper grid -----------------------------------------------------------

// 0x4f77e0  Confidence: exact (VC6 byte match, match23; was likely; the loop variables: x runs 2..7 and the left neighbour is computed).
// 0x4f77fa  Confidence: likely. ls_khosanilab_puzzle_update__trace: the tail of the same function (Ghidra
// starts a function after its first ret). The beam enters at cell (0, 2) going in direction 1; every closed cell whose left
// neighbour is open, not yet passed on its right side and connected to the lit path starts a second beam.
void ls_khosanilab_puzzle_update()
{
    ls_khosanilab_cell c;
    int x, y;

    ls_khosanilab_puzzle_clear_paths();
    if (!(ls_khosanilab_puzzle_grid[0][2] & KL_CELL_OPEN)) {
        ls_khosanilab_puzzle_idle_path();
        return;
    }
    c.x = 0;
    c.y = 2;
    ls_khosanilab_puzzle_trace_beam(&c, 1, true);
    for (x = 2; x < 8; x++) {
        for (y = 0; y < 5; y++) {
            if (ls_khosanilab_puzzle_grid[x][y] & KL_CELL_OPEN)
                continue;
            c.x = x - 1;
            c.y = y;
            if ((ls_khosanilab_puzzle_grid[x - 1][y] & KL_CELL_OPEN) && !(ls_khosanilab_puzzle_grid[x - 1][y] & 4)
                && ls_khosanilab_puzzle_flood_connected(c)) {
                c.x = x;
                c.y = y;
                ls_khosanilab_puzzle_trace_beam(&c, 2, false);
            }
        }
    }
    ls_khosanilab_puzzle_flush_path();
}

// 0x4f78a0  Confidence: exact. The pending point list becomes one avoidance spline (the count is cleared
// before the call).
void ls_khosanilab_puzzle_flush_path()
{
    int spline;

    if (ls_khosanilab_path_len) {
        ls_khosanilab_path_len = 0;
        spline = avoid_spline_create(&ls_khosanilab_path);
        if (spline != -1)
            ls_khosanilab_path_splines[ls_khosanilab_path_spline_count++] = spline;
    }
}

// 0x4f78e0  Confidence: exact.
void ls_khosanilab_puzzle_clear_paths()
{
    int i;

    for (i = 0; i < ls_khosanilab_path_spline_count; i++)
        avoid_spline_free(ls_khosanilab_path_splines[i]);
    ls_khosanilab_path_spline_count = 0;
    ls_khosanilab_path_len = 0;
}

// 0x4f7930  Confidence: exact (VC6 byte match, match23; was likely; the cell is taken by value: the neighbours are built in the argument slots,
// `n = c` then one coordinate changed, 0x4f7964-0x4f7973). True when the open region around the cell
// touches a cell the beam has passed. sum.symbols: (x, y), the same two dwords.
bool ls_khosanilab_puzzle_flood_connected(ls_khosanilab_cell c)
{
    int *cell = &ls_khosanilab_puzzle_grid[c.x][c.y];
    ls_khosanilab_cell n;

    if (*cell & KL_CELL_SIDES)
        return true;
    *cell |= KL_CELL_FLOOD | KL_CELL_VISITED;
    n = c;
    n.x--;
    if (ls_khosanilab_puzzle_cell_open(&n, true) && !(ls_khosanilab_puzzle_grid[n.x][n.y] & KL_CELL_FLOOD)
        && ls_khosanilab_puzzle_flood_connected(n) == true) {
        *cell &= ~KL_CELL_FLOOD;
        return true;
    }
    n = c;
    n.x++;
    if (ls_khosanilab_puzzle_cell_open(&n, true) && !(ls_khosanilab_puzzle_grid[n.x][n.y] & KL_CELL_FLOOD)
        && ls_khosanilab_puzzle_flood_connected(n) == true) {
        *cell &= ~KL_CELL_FLOOD;
        return true;
    }
    n = c;
    n.y++;
    if (ls_khosanilab_puzzle_cell_open(&n, true) && !(ls_khosanilab_puzzle_grid[n.x][n.y] & KL_CELL_FLOOD)
        && ls_khosanilab_puzzle_flood_connected(n) == true) {
        *cell &= ~KL_CELL_FLOOD;
        return true;
    }
    n = c;
    n.y--;
    if (ls_khosanilab_puzzle_cell_open(&n, true) && !(ls_khosanilab_puzzle_grid[n.x][n.y] & KL_CELL_FLOOD)
        && ls_khosanilab_puzzle_flood_connected(n) == true) {
        *cell &= ~KL_CELL_FLOOD;
        return true;
    }
    *cell &= ~KL_CELL_FLOOD;
    return false;
}

// 0x4f7ad0  Confidence: exact (VC6 byte match, match9; was likely: the return expressions: `and eax, 1` / `not eax;
// and eax, 1` without a
// bool conversion). state true: the cell is open; false: closed. Off the grid: false.
bool ls_khosanilab_puzzle_cell_open(const ls_khosanilab_cell *c, bool state)
{
    if (ls_khosanilab_puzzle_cell_valid(c)) {
        if (state)
            return ls_khosanilab_puzzle_grid[c->x][c->y] & KL_CELL_OPEN;
        return ~ls_khosanilab_puzzle_grid[c->x][c->y] & KL_CELL_OPEN;
    }
    return false;
}

// 0x4f7b20  Confidence: exact.
bool ls_khosanilab_puzzle_cell_valid(const ls_khosanilab_cell *c)
{
    return c->x >= 0 && c->x < 8 && c->y >= 0 && c->y < 5;
}

// 0x4f7b50  Confidence: likely (the loop form; `dir` is the argument slot, the start direction is kept in
// EBP, `back_dir` lives in the start pointer's argument slot). Follows the beam from `start` until it comes
// back to the start cell in the start direction: a turns (corner_nn), b goes straight, c turns the other way
// (corner_np), and a dead end turns round (corner_np, corner_pp). Each corner adds a segment.
void ls_khosanilab_puzzle_trace_beam(const ls_khosanilab_cell *start, int dir, bool state)
{
    ls_khosanilab_cell cur, next, a, b, c;
    vector2 fb, fc, from, to;
    int start_dir = dir;
    int back_dir;
    bool first = true;

    ls_khosanilab_puzzle_turn_dirs(&a, &b, &c, &fb, &fc, dir);
    cur = *start;
    ls_khosanilab_puzzle_corner_nn(&from, &cur, &fb, &fc, state);
    while (cur.x != start->x || cur.y != start->y || dir != start_dir || first) {
        first = false;
        next.x = cur.x + a.x;
        next.y = cur.y + a.y;
        if (ls_khosanilab_puzzle_cell_open(&next, state)) {
            ls_khosanilab_puzzle_choose_turn(&cur, dir, 0, state);
            ls_khosanilab_puzzle_corner_nn(&to, &cur, &fb, &fc, state);
            ls_khosanilab_puzzle_emit_segment(&from, &to, state);
            ls_khosanilab_puzzle_reflect_dir(&dir, &a);
            if (cur.x == start->x && cur.y == start->y && dir == start_dir)
                return;
            ls_khosanilab_puzzle_turn_dirs(&a, &b, &c, &fb, &fc, dir);
            from = to;
            cur = next;
            continue;
        }
        next.x = cur.x + b.x;
        next.y = cur.y + b.y;
        if (ls_khosanilab_puzzle_cell_open(&next, state)) {
            ls_khosanilab_puzzle_choose_turn(&cur, dir, 1, state);
            cur = next;
            continue;
        }
        next.x = cur.x + c.x;
        next.y = cur.y + c.y;
        if (ls_khosanilab_puzzle_cell_open(&next, state)) {
            ls_khosanilab_puzzle_choose_turn(&cur, dir, 2, state);
            ls_khosanilab_puzzle_corner_np(&to, &cur, &fb, &fc, state);
            ls_khosanilab_puzzle_emit_segment(&from, &to, state);
            ls_khosanilab_puzzle_reflect_dir(&dir, &c);
            if (cur.x == start->x && cur.y == start->y && dir == start_dir)
                return;
            ls_khosanilab_puzzle_turn_dirs(&a, &b, &c, &fb, &fc, dir);
            from = to;
            cur = next;
            continue;
        }
        // Dead end: two corners in this cell, then back the way the beam came.
        ls_khosanilab_puzzle_choose_turn(&cur, dir, 3, state);
        ls_khosanilab_puzzle_corner_np(&to, &cur, &fb, &fc, state);
        ls_khosanilab_puzzle_emit_segment(&from, &to, state);
        ls_khosanilab_puzzle_reflect_dir(&back_dir, &c);
        if (cur.x == start->x && cur.y == start->y && back_dir == start_dir)
            return;
        from = to;
        ls_khosanilab_puzzle_corner_pp(&to, &cur, &fb, &fc, state);
        ls_khosanilab_puzzle_emit_segment(&from, &to, state);
        b.x = -b.x;
        b.y = -b.y;
        ls_khosanilab_puzzle_reflect_dir(&dir, &b);
        ls_khosanilab_puzzle_turn_dirs(&a, &b, &c, &fb, &fc, dir);
        from = to;
    }
}

// 0x4f7ed0  Confidence: exact (VC6 byte match, match9; was likely: case order = the code order 2, 0, 3, 1; jump table
// 0x4f7f98; the float copies
// load y before x). Direction codes: 0 (-1, 0), 1 (1, 0), 2 (0, 1), 3 (0, -1) = b, the straight-on
// direction; a and c are the two turns. An unknown code leaves a, b and c as they are.
void ls_khosanilab_puzzle_turn_dirs(ls_khosanilab_cell *a, ls_khosanilab_cell *b, ls_khosanilab_cell *c,
                                    vector2 *fb, vector2 *fc, int dir)
{
    switch (dir) {
    case 2:
        a->x = -1; a->y = 0;
        b->x = 0;  b->y = 1;
        c->x = 1;  c->y = 0;
        break;
    case 0:
        a->x = 0;  a->y = -1;
        b->x = -1; b->y = 0;
        c->x = 0;  c->y = 1;
        break;
    case 3:
        a->x = 1;  a->y = 0;
        b->x = 0;  b->y = -1;
        c->x = -1; c->y = 0;
        break;
    case 1:
        a->x = 0;  a->y = 1;
        b->x = 1;  b->y = 0;
        c->x = 0;  c->y = -1;
        break;
    }
    float y = (float)b->y;
    float x = (float)b->x;
    fb->x = x;
    fb->y = y;
    y = (float)c->y;
    x = (float)c->x;
    fc->x = x;
    fc->y = y;
}

// 0x4f7fb0  Confidence: exact (VC6 byte match, match23; was likely; setne / sete from the conditional expressions). The direction code of a
// unit vector (inverse of turn_dirs' b).
void ls_khosanilab_puzzle_reflect_dir(int *out, const ls_khosanilab_cell *d)
{
    if (d->x == 0)
        *out = d->y == 1 ? 2 : 3;
    else
        *out = d->x == 1 ? 1 : 0;
}

// 0x4f7fe0  Confidence: likely (the products of d2 are formed before the subtraction). A corner of the
// cell: centre - s * d2 - s * d1, s = 0.3 (inner) or 0.7 (0x581970 / 0x58020c). sum.symbols' former name
// was ls_khosanilab_puzzle_edge_point_a.
void ls_khosanilab_puzzle_corner_nn(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner)
{
    float s;

    out->x = cell->x + 0.5f;
    out->y = cell->y + 0.5f;
    s = inner ? 0.3f : 0.7f;
    out->x -= s * d2->x;
    out->y -= s * d2->y;
    out->x -= s * d1->x;
    out->y -= s * d1->y;
}

// 0x4f8070  Confidence: likely (as corner_nn). centre - s * d2 + s * d1.
void ls_khosanilab_puzzle_corner_np(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner)
{
    float s;

    out->x = cell->x + 0.5f;
    out->y = cell->y + 0.5f;
    s = inner ? 0.3f : 0.7f;
    out->x -= s * d2->x;
    out->y -= s * d2->y;
    out->x += s * d1->x;
    out->y += s * d1->y;
}

// 0x4f8100  Confidence: likely (as corner_nn). centre + s * d2 + s * d1.
void ls_khosanilab_puzzle_corner_pp(vector2 *out, const ls_khosanilab_cell *cell, const vector2 *d1,
                                    const vector2 *d2, bool inner)
{
    float s;

    out->x = cell->x + 0.5f;
    out->y = cell->y + 0.5f;
    s = inner ? 0.3f : 0.7f;
    out->x += s * d2->x;
    out->y += s * d2->y;
    out->x += s * d1->x;
    out->y += s * d1->y;
}

// 0x4f8190  Confidence: likely (two jump tables, 0x4f83b0 for state and 0x4f83c0 without; only the
// state-0 kind-0 case keeps the cell address across both calls, so it is written with a pointer). Marks the
// sides the beam passes in or next to the cell. kind: 0 turn a, 1 straight, 2 turn c, 3 dead end.
void ls_khosanilab_puzzle_choose_turn(const ls_khosanilab_cell *cell, int dir, int kind, bool state)
{
    ls_khosanilab_cell a, b, c, back;
    vector2 fb, fc;
    int *p;

    ls_khosanilab_puzzle_turn_dirs(&a, &b, &c, &fb, &fc, dir);
    if (state) {
        switch (kind) {
        case 0:
            ls_khosanilab_puzzle_grid[cell->x - b.x][cell->y - b.y] |= ls_khosanilab_puzzle_dir_to_side_bit(&a);
            back.x = -b.x;
            back.y = -b.y;
            ls_khosanilab_puzzle_grid[cell->x + a.x][cell->y + a.y] |= ls_khosanilab_puzzle_dir_to_side_bit(&back);
            break;
        case 1:
            ls_khosanilab_puzzle_grid[cell->x][cell->y] |= ls_khosanilab_puzzle_dir_to_side_bit(&a);
            break;
        case 2:
        case 3:
            ls_khosanilab_puzzle_grid[cell->x][cell->y] |= ls_khosanilab_puzzle_dir_to_side_bit(&a);
            ls_khosanilab_puzzle_grid[cell->x][cell->y] |= ls_khosanilab_puzzle_dir_to_side_bit(&b);
            break;
        }
    } else {
        switch (kind) {
        case 0:
            p = &ls_khosanilab_puzzle_grid[cell->x - b.x + a.x][cell->y - b.y + a.y];
            *p |= ls_khosanilab_puzzle_dir_to_side_bit(&c);
            *p |= ls_khosanilab_puzzle_dir_to_side_bit(&b);
            break;
        case 1:
            ls_khosanilab_puzzle_grid[cell->x + a.x][cell->y + a.y] |= ls_khosanilab_puzzle_dir_to_side_bit(&c);
            break;
        case 2:
        case 3:
            ls_khosanilab_puzzle_grid[cell->x + a.x][cell->y + a.y] |= ls_khosanilab_puzzle_dir_to_side_bit(&c);
            back.x = -b.x;
            back.y = -b.y;
            ls_khosanilab_puzzle_grid[cell->x + b.x][cell->y + b.y] |= ls_khosanilab_puzzle_dir_to_side_bit(&back);
            break;
        }
    }
}

// 0x4f83d0  Confidence: exact. 2 / 4 for x <= 0 / x > 0, 0x10 / 8 for y <= 0 / y > 0.
int ls_khosanilab_puzzle_dir_to_side_bit(const ls_khosanilab_cell *d)
{
    if (d->x == 0)
        return d->y > 0 ? 8 : 0x10;
    return d->x > 0 ? 4 : 2;
}

// 0x4f8400  Confidence: likely (which of the four locals each value lands in). A beam segment in grid
// coordinates to world point pairs. Four segments along the grid border are bent round fixed world points
// (the left edge x = 0.2, the right edge x = 7.8, the bottom edge y = 4.8 between x 2.5, the top edge
// y = 0.2 at x 5.5); any other segment is one pair, in beam order when `state` is set, reversed when not.
void ls_khosanilab_puzzle_emit_segment(const vector2 *p0, const vector2 *p1, bool state)
{
    vector2 b, a, g, g2;

    if (fabsf(p0->x - 0.2f) < 0.001f && fabsf(p1->x - 0.2f) < 0.001f) {
        ls_khosanilab_puzzle_cell_to_world(p0, &a);
        b.x = -136.2f;
        b.y = -38.1f;
        ls_khosanilab_puzzle_add_point(&a, &b);
        ls_khosanilab_puzzle_cell_to_world(p1, &b);
        a.x = -136.2f;
        a.y = -40.8f;
        ls_khosanilab_puzzle_add_point(&a, &b);
        return;
    }
    if (fabsf(p0->x - 7.8f) < 0.001f && fabsf(p1->x - 7.8f) < 0.001f) {
        b.x = -155.2f;
        b.y = -38.1f;
        ls_khosanilab_puzzle_cell_to_world(p1, &a);
        ls_khosanilab_puzzle_add_point(&b, &a);
        ls_khosanilab_puzzle_cell_to_world(p0, &b);
        a.x = -155.2f;
        a.y = -40.8f;
        ls_khosanilab_puzzle_add_point(&b, &a);
        return;
    }
    if (fabsf(p0->y - 4.8f) < 0.001f && fabsf(p1->y - 4.8f) < 0.001f && p0->x <= 2.5f && p1->x >= 2.5f) {
        if (p0->x < 2.0f) {
            ls_khosanilab_puzzle_cell_to_world(p0, &b);
            g.x = 2.2f;
            g.y = p0->y;
            ls_khosanilab_puzzle_cell_to_world(&g, &a);
            ls_khosanilab_puzzle_add_point(&b, &a);
        }
        if (p1->x > 3.0f) {
            g2.x = 2.8f;
            g2.y = p0->y;
            ls_khosanilab_puzzle_cell_to_world(&g2, &b);
            ls_khosanilab_puzzle_cell_to_world(p1, &a);
            ls_khosanilab_puzzle_add_point(&b, &a);
        }
        g.x = 2.2f;
        g.y = p0->y;
        ls_khosanilab_puzzle_cell_to_world(&g, &b);
        a.x = -140.9f;
        a.y = -45.9f;
        ls_khosanilab_puzzle_add_point(&b, &a);
        g2.x = 2.8f;
        g2.y = p0->y;
        b.x = -143.3f;
        b.y = -45.7f;
        ls_khosanilab_puzzle_cell_to_world(&g2, &a);
        ls_khosanilab_puzzle_add_point(&b, &a);
        return;
    }
    // Only p0's y is tested here (both are in the y = 4.8 case above).
    if (fabsf(p0->y - 0.2f) < 0.001f && p0->x >= 5.5f && p1->x <= 5.5f) {
        if (p0->x > 6.0f) {
            ls_khosanilab_puzzle_cell_to_world(p0, &b);
            g.x = 5.8f;
            g.y = p0->y;
            ls_khosanilab_puzzle_cell_to_world(&g, &a);
            ls_khosanilab_puzzle_add_point(&b, &a);
        }
        if (p1->x < 5.0f) {
            g2.x = 5.2f;
            g2.y = p0->y;
            ls_khosanilab_puzzle_cell_to_world(&g2, &b);
            ls_khosanilab_puzzle_cell_to_world(p1, &a);
            ls_khosanilab_puzzle_add_point(&b, &a);
        }
        g.x = 5.8f;
        g.y = p0->y;
        ls_khosanilab_puzzle_cell_to_world(&g, &b);
        a.x = -150.6f;
        a.y = -32.1f;
        ls_khosanilab_puzzle_add_point(&b, &a);
        g2.x = 5.2f;
        g2.y = p0->y;
        b.x = -148.0f;
        b.y = -32.1f;
        ls_khosanilab_puzzle_cell_to_world(&g2, &a);
        ls_khosanilab_puzzle_add_point(&b, &a);
        return;
    }
    ls_khosanilab_puzzle_cell_to_world(p0, &b);
    ls_khosanilab_puzzle_cell_to_world(p1, &a);
    if (state)
        ls_khosanilab_puzzle_add_point(&b, &a);
    else
        ls_khosanilab_puzzle_add_point(&a, &b);
}

// 0x4f8810  Confidence: exact (VC6 byte match, match23; was likely; the origin is stored first, then the computed values use the constants, not
// a reload). Grid (x, y) to world (x, z): (-136.284 - 2.348 x, -34.044 - 2.151 y), 0x581cc0..0x581ccc.
void ls_khosanilab_puzzle_cell_to_world(const vector2 *cell, vector2 *out)
{
    out->x = -136.284f;
    out->y = -34.044f;
    out->x -= cell->x * 2.348f;
    out->y -= cell->y * 2.151f;
}

// 0x4f8850  Confidence: exact (VC6 byte match, match9; was likely: statement order). Appends one point pair (cells 1
// and 2 for every point);
// eight pending pairs are flushed into a spline.
void ls_khosanilab_puzzle_add_point(const vector2 *a, const vector2 *b)
{
    int n = ls_khosanilab_path_len;
    ls_khosanilab_path.p0[n] = *a;
    ls_khosanilab_path.p1[n] = *b;
    ls_khosanilab_path.cell_x[n] = 1;
    ls_khosanilab_path.cell_y[n] = 2;
    n++;
    ls_khosanilab_path_len = n;
    ls_khosanilab_path.count = n;
    if (n == 8) {
        ls_khosanilab_puzzle_flush_path();
        ls_khosanilab_path_len = 0;
    }
}

// 0x4f88c0  Confidence: exact. The short path shown while the start cell is closed: grid (-0.1, 3.1) to
// (-0.1, 1.9).
void ls_khosanilab_puzzle_idle_path()
{
    vector2 g0, g1, w1, w0;

    g1.x = -0.1f;
    g1.y = 1.9f;
    g0.x = -0.1f;
    g0.y = 3.1f;
    ls_khosanilab_puzzle_cell_to_world(&g0, &w0);
    ls_khosanilab_puzzle_cell_to_world(&g1, &w1);
    ls_khosanilab_puzzle_add_point(&w0, &w1);
    ls_khosanilab_puzzle_flush_path();
}

// ---- level script hooks and rooms ----------------------------------------------------------------------

// 0x4f8920  Confidence: exact. level_script_table init. The flippers start down on every entry (they are
// not saved; see NOTES).
void ls_khosanilab_init()
{
    level_music = music_find_by_name("KhosaniLab");
    game_stage_set(5);
    door_apply_saved_state("Gate-Room2", true);
    door_apply_saved_state("BOSS-GateA-Door01", true);
    ls_khosanilab_timer.invalidate();
    ls_khosanilab_salstart_done = false;
    ls_khosanilab_ring_of_fire_done = false;
    ls_khosanilab_path_len = 0;
    ls_khosanilab_path_spline_count = 0;
    memset(ls_khosanilab_flippers, 0, sizeof(ls_khosanilab_flippers));
    ls_khosanilab_apply_lava_platforms();
    ls_khosanilab_puzzle_idle_path();
    if (savegame_state.get_flag(cur_level_index, "sal_killed"))
        script_npc_set_disabled("Giant Salamanka", true);
}

// 0x4f89c0  Confidence: exact. Shows the lava platform of every raised flipper, hides the others. The loop
// end compare against 0x5a3ea8 ("sal_killed") is the end of the platform table (formerly lsf_sal_killed).
void ls_khosanilab_apply_lava_platforms()
{
    char name[16];
    int i, j;

    for (i = 0; i < 10; i++) {
        for (j = 0; j < 7; j++) {
            sprintf(name, "lavaplatform%02d", ls_khosanilab_lava_platform_ids[i][j]);
            if (ls_khosanilab_flippers[i][j])
                lobjs_show_by_prefix(name);
            else
                lobjs_hide_by_prefix(name);
        }
    }
}

// 0x4f8a30  Confidence: exact (ends in a tail jump to ls_khosanilab_salamanka_fight). level_script_table
// frame.
void ls_khosanilab_frame()
{
    ls_khosanilab_room1_intro();
    if (!savegame_state.get_flag(cur_level_index, "room1_stepped"))
        ls_khosanilab_room1_frame();
    else if (!savegame_state.get_flag(cur_level_index, "room1_killed"))
        lsf_room1_killed();
    ls_khosanilab_room2_intro();
    if (!savegame_state.get_flag(cur_level_index, "room2_killed") && script_npc_is_dead("Onyx Golem#1")
        && script_npc_is_dead("Onyx Golem#2") && script_npc_is_dead("Onyx Golem#3")
        && script_npc_is_dead("Onyx Golem#4")) {
        door_apply_saved_state("LAB-Rm3-DoorPylon01", true);
        door_apply_saved_state("LAB-Rm3-DoorPylon02", true);
        door_apply_saved_state("LAB-Rm3-DoorPylon03", true);
        door_apply_saved_state("LAB-Rm3-DoorPylon04", true);
        door_apply_saved_state("LAB-Rm3-DoorPylon05", true);
        door_apply_saved_state("LAB-Rm3-DoorPylon06", true);
        savegame_state.set_flag(cur_level_index, "room2_killed", true);
    }
    ls_khosanilab_room4_intro();
    if (!savegame_state.get_flag(cur_level_index, "room4_started"))
        ls_khosanilab_room4_frame();
    else if (!savegame_state.get_flag(cur_level_index, "room4_finished"))
        ls_khosanilab_room4_check_done();
    ls_khosanilab_room5_intro();
    ls_khosanilab_room7_intro();
    if (ls_khosanilab_timer.elapsed()) {
        ls_khosanilab_reset_runes();
        ls_khosanilab_timer.invalidate();
    }
    if (!savegame_state.get_flag(cur_level_index, "room7_killed") && script_npc_is_dead("Steel Golem#1")
        && script_npc_is_dead("Steel Golem#2") && script_npc_is_dead("Steel Golem#3")
        && script_npc_is_dead("Steel Golem#4")) {
        door_apply_saved_state("LAB-Rm4-Door04", true);
        savegame_state.set_flag(cur_level_index, "room7_killed", true);
        savegame_state.set_flag(cur_level_index, "room7_finished", true);
    }
    lsf_room8();
    ls_khosanilab_salamanka_fight();
}

// 0x4f8c60  Confidence: exact. Stepping on the symbol (the test of respect) closes Gate-Room2, opens the
// four secret doors and sends the iron golems.
void ls_khosanilab_room1_frame()
{
    if (script_player_near_navpoint("$symbol", 5.0f)) {
        savegame_state.set_flag(cur_level_index, "room1_stepped", true);
        door_apply_saved_state("Gate-Room2", false);
        door_apply_saved_state("LAB-Rm1-SDoor01", true);
        door_apply_saved_state("LAB-Rm1-SDoor02", true);
        door_apply_saved_state("LAB-Rm1-SDoor03", true);
        door_apply_saved_state("LAB-Rm1-SDoor04", true);
        script_npc_send_go("Iron Golem#1");
        script_npc_send_go("Iron Golem#2");
        script_npc_send_go("Iron Golem#3");
        script_npc_send_go("Iron Golem#4");
    }
}

// 0x4f8d00  Confidence: exact. The four iron golems dead: Gate-Room2 opens again.
void lsf_room1_killed()
{
    if (script_npc_is_dead("Iron Golem#1") && script_npc_is_dead("Iron Golem#2")
        && script_npc_is_dead("Iron Golem#3") && script_npc_is_dead("Iron Golem#4")) {
        savegame_state.set_flag(cur_level_index, "room1_killed", true);
        door_apply_saved_state("Gate-Room2", true);
    }
}

// 0x4f8d70  Confidence: exact. On the platform of room 4: the eight prison lids open and the four
// prisoners attack; their script objects get end mode 1 (script_object_find is not checked for -1).
void ls_khosanilab_room4_frame()
{
    if (script_player_near_navpoint("$platform", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "room4_started", true);
        door_apply_saved_state("prisonlid01", true);
        door_apply_saved_state("prisonlid02", true);
        door_apply_saved_state("prisonlid03", true);
        door_apply_saved_state("prisonlid04", true);
        door_apply_saved_state("prisonlid05", true);
        door_apply_saved_state("prisonlid06", true);
        door_apply_saved_state("prisonlid07", true);
        door_apply_saved_state("prisonlid08", true);
        script_npc_send_go("Khosani Prisoner#1");
        script_objects[script_object_find("Khosani Prisoner#1")].end_mode = 1;
        script_npc_send_go("Khosani Prisoner#2");
        script_objects[script_object_find("Khosani Prisoner#2")].end_mode = 1;
        script_npc_send_go("Khosani Prisoner#3");
        script_objects[script_object_find("Khosani Prisoner#3")].end_mode = 1;
        script_npc_send_go("Khosani Prisoner#4");
        script_objects[script_object_find("Khosani Prisoner#4")].end_mode = 1;
    }
}

// 0x4f8eb0  Confidence: exact. The test of might won: 1000 XP, LAB-Rm4-door01 opens.
void ls_khosanilab_room4_check_done()
{
    if (script_npc_is_dead("Khosani Prisoner#1") && script_npc_is_dead("Khosani Prisoner#2")
        && script_npc_is_dead("Khosani Prisoner#3") && script_npc_is_dead("Khosani Prisoner#4")) {
        script_party_gain_xp(1000);
        door_apply_saved_state("LAB-Rm4-door01", true);
        savegame_state.set_flag(cur_level_index, "room4_finished", true);
    }
}

// 0x4f8f30  Confidence: exact. The rune doors of room 7 close again (after ls_khosanilab_room7_doors'
// 2 s timer).
void ls_khosanilab_reset_runes()
{
    door_apply_saved_state("Lab-Rm7-Runenumber01", false);
    door_apply_saved_state("Lab-Rm7-Runenumber02", false);
    door_apply_saved_state("Lab-Rm7-Runenumber03", false);
    door_apply_saved_state("Lab-Rm7-Runenumber04", false);
    door_apply_saved_state("Lab-Rm7-Runenumber05", false);
    door_apply_saved_state("Lab-Rm7-Runenumber06", false);
}

// 0x4f8f80  Confidence: exact.
void ls_khosanilab_room1_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "room1") && script_player_near_navpoint("$rm1", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("Pass the test of respect, and tread not upon us.", 492), 5.0f);
        savegame_state.set_flag(cur_level_index, "room1", true);
    }
}

// 0x4f8ff0  Confidence: exact. 1000 XP when the party did not step on the symbol.
void ls_khosanilab_room2_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "room2") && door_is_open_by_name("Gate-Room2", 0.001f)
        && script_player_near_navpoint("$rm2", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("In these two chambers, pass the test of insight. Here wisdom can move stones.", 493),
                          5.0f);
        savegame_state.set_flag(cur_level_index, "room2", true);
        if (!savegame_state.get_flag(cur_level_index, "room1_stepped"))
            script_party_gain_xp(1000);
    }
}

// 0x4f90a0  Confidence: exact. 1200 XP when the fourth onyx golem door stayed shut.
void ls_khosanilab_room4_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "room4") && script_player_near_navpoint("$rm4", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("Now pass the test of might. Fight well, or the oath-breakers will destroy you!", 494),
                          5.0f);
        savegame_state.set_flag(cur_level_index, "room4", true);
        if (!savegame_state.get_flag(cur_level_index, "room2_door4"))
            script_party_gain_xp(1200);
    }
}

// 0x4f9130  Confidence: exact.
void ls_khosanilab_room5_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "room5") && script_player_near_navpoint("$rm5", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("In these two chambers, pass the test of thought. Your wits must be your guide.", 495),
                          5.0f);
        savegame_state.set_flag(cur_level_index, "room5", true);
    }
}

// 0x4f91a0  Confidence: exact.
void ls_khosanilab_room7_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "room7") && script_player_near_navpoint("$rm7", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("Now pass the test of memory. Those whose eyes are open shall see.", 496), 5.0f);
        savegame_state.set_flag(cur_level_index, "room7", true);
    }
}

// 0x4f9210  Confidence: exact. 2000 XP unless all four steel golems were killed (formerly
// lsf_room7_killed).
void lsf_room8()
{
    if (!savegame_state.get_flag(cur_level_index, "room8") && script_player_near_navpoint("$rm8", 3.0f)) {
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("The final test awaits you. Heal yourselves if you are weak.", 497), 5.0f);
        savegame_state.set_flag(cur_level_index, "room8", true);
        if (!savegame_state.get_flag(cur_level_index, "room7_killed"))
            script_party_gain_xp(2000);
    }
}

// 0x4f92a0  Confidence: exact. The Giant Salamanka: intro at $salstart, the two stage cutscenes when the
// boss AI reports a stage, the boss music, the death cutscene, then the reward once the ring (given by the
// intro's end) is in the party and the boss is dead (formerly lsf_sal_killed_4f92a0).
void ls_khosanilab_salamanka_fight()
{
    if (!savegame_state.get_flag(cur_level_index, "salstart")) {
        if (script_player_near_navpoint("$salstart", 4.0f)) {
            savegame_state.set_flag(cur_level_index, "salstart", true);
            door_apply_saved_state("BOSS-GateA-Door01", false);
            script_play_cutscene("GiantSalamanka-Intro", ls_khosanilab_salamanka_intro_cs_end);
        }
    } else {
        if (!savegame_state.get_flag(cur_level_index, "sal_trans1") && giant_salamanka_stage2_reached) {
            savegame_state.set_flag(cur_level_index, "sal_trans1", true);
            script_play_cutscene("GiantSalamanka-Trans1", ls_khosanilab_salamanka_trans1_cs_end);
        }
        if (!savegame_state.get_flag(cur_level_index, "sal_trans2") && giant_salamanka_stage3_reached) {
            savegame_state.set_flag(cur_level_index, "sal_trans2", true);
            script_play_cutscene("GiantSalamanka-Trans2", ls_khosanilab_salamanka_trans2_cs_end);
        }
    }
    if (!ls_khosanilab_salstart_done && savegame_state.get_flag(cur_level_index, "salstart")
        && !(game_flags & GF_CUTSCENE)) {
        ls_khosanilab_salstart_done = true;
        int music = music_find_by_name("Salamanka");
        music_play(music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "sal_killed") && script_npc_is_dead("Giant Salamanka")) {
        savegame_state.set_flag(cur_level_index, "sal_killed", true);
        script_play_cutscene("GiantSalamanka-Death", ls_khosanilab_salamanka_death_cs_end);
    }
    if (!savegame_state.get_flag(cur_level_index, "got_ring_of_fire") && party_count_item_by_name("Ring of Fire")
        && savegame_state.get_flag(cur_level_index, "sal_killed")) {
        savegame_state.set_flag(cur_level_index, "got_ring_of_fire", true);
        hud_show_message(XSTR("Joseph acquired Summon Red Minotaur!", 498));
        door_apply_saved_state("BOSS-GateB-Door01", true);
        script_object_say(script_object_find("Ulsadana"),
                          XSTR("You have passed the Ordeal, Sahugani. The people of Saanavarh welcome you.", 499),
                          5.0f);
    }
    if (!ls_khosanilab_ring_of_fire_done && savegame_state.get_flag(cur_level_index, "got_ring_of_fire")
        && !(game_flags & GF_CUTSCENE)) {
        ls_khosanilab_ring_of_fire_done = true;
        music_play(level_music, true);
    }
}

// 0x4f9520  Confidence: exact. End of GiantSalamanka-Intro.
void ls_khosanilab_salamanka_intro_cs_end()
{
    giant_salamanka_combat_enabled = true;
    level_script_event_active = true;
    script_party_acquire_item("Ring of Fire", 1);
}

// 0x4f9540  Confidence: exact. End of GiantSalamanka-Death.
void ls_khosanilab_salamanka_death_cs_end()
{
    giant_salamanka_combat_enabled = false;
    level_script_event_active = false;
    script_npc_set_disabled("Giant Salamanka", true);
}

// 0x4f9560  Confidence: exact. End of GiantSalamanka-Trans1.
void ls_khosanilab_salamanka_trans1_cs_end()
{
    giant_salamanka_stage2_ack = true;
}

// 0x4f9570  Confidence: exact. End of GiantSalamanka-Trans2.
void ls_khosanilab_salamanka_trans2_cs_end()
{
    giant_salamanka_stage3_ack = true;
}

// 0x4f9580  Confidence: exact. level_script_table leave (clear_paths already zeroes both counts).
void ls_khosanilab_leave()
{
    ls_khosanilab_puzzle_clear_paths();
    ls_khosanilab_path_len = 0;
    ls_khosanilab_path_spline_count = 0;
}

// 0x4f95a0  Confidence: exact. level_script_table use. The flipper prefix compare is 14 characters
// ("puzzle-flipper"). The runes of room 7 must be used in order: a rune whose predecessor is not open
// releases a steel golem; rune 6 after rune 5 opens LAB-Rm4-Door04.
char ls_khosanilab_use(const char *name)
{
    int flipper;

    ls_khosanilab_pylon_use(name);
    if (!_strnicmp(name, "puzzle-flippers", 14)) {
        sscanf(name, "puzzle-flippers%02d", &flipper);
        ls_khosanilab_puzzle_use(flipper);
        ls_khosanilab_apply_lava_platforms();
    }
    if (!_stricmp(name, "Lab-Rm7-Runenumber02") && !door_is_open_by_name("Lab-Rm7-Runenumber01", 0.01f))
        ls_khosanilab_room7_doors();
    if (!_stricmp(name, "Lab-Rm7-Runenumber03") && !door_is_open_by_name("Lab-Rm7-Runenumber02", 0.01f))
        ls_khosanilab_room7_doors();
    if (!_stricmp(name, "Lab-Rm7-Runenumber04") && !door_is_open_by_name("Lab-Rm7-Runenumber03", 0.01f))
        ls_khosanilab_room7_doors();
    if (!_stricmp(name, "Lab-Rm7-Runenumber05") && !door_is_open_by_name("Lab-Rm7-Runenumber04", 0.01f))
        ls_khosanilab_room7_doors();
    if (!_stricmp(name, "Lab-Rm7-Runenumber06")) {
        if (door_is_open_by_name("Lab-Rm7-Runenumber05", 0.01f)) {
            door_apply_saved_state("LAB-Rm4-Door04", true);
            savegame_state.set_flag(cur_level_index, "room7_finished", true);
            return 0;
        }
        ls_khosanilab_room7_doors();
    }
    return 0;
}

// 0x4f96f0  Confidence: exact (the last case is a tail jump). Room 3 (the test of insight): each pylon
// trigger moves its door one way; pushed the wrong way (the door already there) an onyx golem comes out.
void ls_khosanilab_pylon_use(const char *name)
{
    if (!_stricmp(name, "LeftPylonTrigger-06")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon01", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon01", true);
    }
    if (!_stricmp(name, "LeftPylonTrigger-05")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon02", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon02", true);
    }
    if (!_stricmp(name, "LeftPylonTrigger-04")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon03", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon03", false);
    }
    if (!_stricmp(name, "LeftPylonTrigger-03")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon04", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon04", false);
    }
    if (!_stricmp(name, "LeftPylonTrigger-02")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon05", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon05", true);
    }
    if (!_stricmp(name, "LeftPylonTrigger-01")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon06", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon06", false);
    }
    if (!_stricmp(name, "RightPylonTrigger-06")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon01", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon01", false);
    }
    if (!_stricmp(name, "RightPylonTrigger-05")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon02", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon02", false);
    }
    if (!_stricmp(name, "RightPylonTrigger-04")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon03", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon03", true);
    }
    if (!_stricmp(name, "RightPylonTrigger-03")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon04", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon04", true);
    }
    if (!_stricmp(name, "RightPylonTrigger-02")) {
        if (!door_is_open_by_name("LAB-Rm3-DoorPylon05", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon05", false);
    }
    if (!_stricmp(name, "RightPylonTrigger-01")) {
        if (door_is_open_by_name("LAB-Rm3-DoorPylon06", 0.01f))
            ls_khosanilab_room2_doors();
        else
            door_apply_saved_state("LAB-Rm3-DoorPylon06", true);
    }
}

// 0x4f99e0  Confidence: exact. The next closed onyx golem door opens and its golem attacks (door 3: the
// golem is sent first).
void ls_khosanilab_room2_doors()
{
    if (!savegame_state.get_flag(cur_level_index, "room2_door1")) {
        door_apply_saved_state("LAB-Rm2-SDoor01", true);
        script_npc_send_go("Onyx Golem#1");
        savegame_state.set_flag(cur_level_index, "room2_door1", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room2_door2")) {
        door_apply_saved_state("LAB-Rm2-SDoor02", true);
        script_npc_send_go("Onyx Golem#2");
        savegame_state.set_flag(cur_level_index, "room2_door2", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room2_door3")) {
        script_npc_send_go("Onyx Golem#3");
        door_apply_saved_state("LAB-Rm2-SDoor03", true);
        savegame_state.set_flag(cur_level_index, "room2_door3", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room2_door4")) {
        door_apply_saved_state("LAB-Rm2-SDoor04", true);
        script_npc_send_go("Onyx Golem#4");
        savegame_state.set_flag(cur_level_index, "room2_door4", true);
    }
}

// 0x4f9b10  Confidence: exact (VC6 byte match, match9; was likely: the search loop's early exit `col > -1` sits after
// the compare). A used
// flipper toggles itself (its door moves on its own) and its four neighbours, the beam grid is rebuilt, and
// every party member within 9 units of $badplace is put back at player start 3.
void ls_khosanilab_puzzle_use(int flipper)
{
    vector3 pos;
    matrix orient;
    Character *c;
    int row = -1, col = -1;
    int i, j, slot;

    for (i = 0; i < 10; i++) {
        for (j = 0; j < 7; j++) {
            if (ls_khosanilab_flipper_ids[i][j] == flipper) {
                row = i;
                col = j;
                break;
            }
            if (col > -1)
                break;
        }
    }
    if (col < 0)
        return;
    ls_khosanilab_flippers[row][col] = !ls_khosanilab_flippers[row][col];
    ls_khosanilab_set_flippers(row - 1, col);
    ls_khosanilab_set_flippers(row + 1, col);
    ls_khosanilab_set_flippers(row, col - 1);
    ls_khosanilab_set_flippers(row, col + 1);
    ls_khosanilab_puzzle_build_grid();
    navpoint_get_position("$badplace", &pos, &orient);
    level_entry_point = 3;
    level_find_player_starts();
    slot = 0;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7) {
            float dx = c->pos.x - pos.x;
            float dy = c->pos.y - pos.y;
            float dz = c->pos.z - pos.z;
            if (dx * dx + dy * dy + dz * dz < 81.0f) {
                c->place_at_party_start(slot);
                slot++;
            }
        }
    }
}

// 0x4f9c40  Confidence: exact. Board rows 1..8, columns 1..5 to grid[8 - row][5 - col].
void ls_khosanilab_puzzle_build_grid()
{
    int i, j;

    memset(ls_khosanilab_puzzle_grid, 0, sizeof(ls_khosanilab_puzzle_grid));
    for (i = 1; i < 9; i++)
        for (j = 1; j < 6; j++)
            if (ls_khosanilab_flippers[i][j] == true)
                ls_khosanilab_puzzle_grid[8 - i][5 - j] |= KL_CELL_OPEN;
    ls_khosanilab_puzzle_update();
}

// 0x4f9ca0  Confidence: exact. Toggles one board cell when it holds a flipper and moves its door. No bounds
// check: the board's zero border stops the neighbours of the edge flippers.
void ls_khosanilab_set_flippers(int row, int col)
{
    char name[24];
    int id = ls_khosanilab_flipper_ids[row][col];

    if (id > 0) {
        ls_khosanilab_flippers[row][col] = !ls_khosanilab_flippers[row][col];
        sprintf(name, "puzzle-flippers%02d", id);
        door_apply_saved_state(name, ls_khosanilab_flippers[row][col]);
    }
}

// 0x4f9d10  Confidence: exact. A wrong rune: the runes reset after 2 s and the next steel golem door opens
// (golems 1, 4, 3, 2 behind LAB-Rm2-SDoor05..08).
void ls_khosanilab_room7_doors()
{
    ls_khosanilab_timer.SetTimeout(2000);
    if (!savegame_state.get_flag(cur_level_index, "room7_door1")) {
        script_npc_send_go("Steel Golem#1");
        door_apply_saved_state("LAB-Rm2-SDoor05", true);
        savegame_state.set_flag(cur_level_index, "room7_door1", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room7_door2")) {
        script_npc_send_go("Steel Golem#4");
        door_apply_saved_state("LAB-Rm2-SDoor06", true);
        savegame_state.set_flag(cur_level_index, "room7_door2", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room7_door3")) {
        script_npc_send_go("Steel Golem#3");
        door_apply_saved_state("LAB-Rm2-SDoor07", true);
        savegame_state.set_flag(cur_level_index, "room7_door3", true);
    } else if (!savegame_state.get_flag(cur_level_index, "room7_door4")) {
        script_npc_send_go("Steel Golem#2");
        door_apply_saved_state("LAB-Rm2-SDoor08", true);
        savegame_state.set_flag(cur_level_index, "room7_door4", true);
    }
}
