// pathfinding.cpp - Sum.exe v1.40, code 0x49c840-0x4a04af (Engine\ai\pathfinding.cpp: the path
// literal 0x59cc04 is pushed by pathfinding_load_graph at 0x49ce7d, 0x49d8fa and 0x49d91c).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; .rdata floats and the EH tables read from
// Summoner/Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame
// pointer, 0x90 padding, arguments' stack slots reused for locals).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: init_ccmd_precalc_vis_method_thunk (0x49c840, static initialiser entry 288; the tiger
// rider unit ends before it) to pf_node_index (0x4a04a0-0x4a04af); boss_ai.cpp starts at 0x4a04b0.
// The geometry functions after boss_ai.cpp (pf_select_graph 0x4a4f30 .. pf_layer_from_edges 0x4a6600)
// and their .bss (0x245f3a0-0x2460cbb, the graph tables and the pools' remaining counts) belong to a
// later unit (docs/sum-exe/source-map.md) and are extern here (engine/ai/pathfinding.h).
//
// Profiling: five functions open an EH frame whose only unwind action is the empty
// destructor 0x434650 (profile_label, vsdk/os/profile.h): the inline constructor leaves only the state store, as in
// objects.cpp. The label texts are not in the binary; the function names are used (unsure). The
// discarded timer_get(1000) calls around the timed parts are the other remains of the profiling code.
//
// Data: .data 0x59cb2c-0x59ccb7, only this unit's literals (command name and help, the error texts,
// "PFG memory size: %d\n", the path, ".pfg", the path-node message). The toggle texts "TRUE" / "FALSE",
// "%s is %s\n" and the usage format are pooled (0x588c34-0x588c50).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "engine/ai/pathfinding.h"
#include "engine/ai/pathfinding_visibility.h"
#include "engine/ai/ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/profile.h"
#include "vsdk/math/vector.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "summoner/summoner.h"

#define PATHFINDING_CPP "D:\\projects\\Summoner\\pccode\\Engine\\ai\\pathfinding.cpp"

// ---- .bss (0x2452cb8-0x245c9df) ------------------------------------------------------------------
// In address order. The two objects with initialisers come in initialiser-table order (entry 288 the
// command, 289 the node pool).
uint pf_layer_links[PF_MAX_LAYERS];           // 0x2452cb8
pf_vis_node *pf_search_start_nodes[10];       // 0x2452d08
ai_path *ai_path_free_list;                   // 0x2452d30
int ai_path_peak_used;                        // 0x2452d34
int pf_search_iterations;                     // 0x2452d38

void cmd_precalc_vis_method();
// 0x49c840 init_ccmd_precalc_vis_method_thunk (jmp, static initialiser entry 288). Confidence: exact.
// 0x49c850 init_ccmd_precalc_vis_method (console_command constructor on 0x2452d40). Confidence: exact.
console_command ccmd_precalc_vis_method("precalc_vis_method", "Toggles Precalc_vis_method",
                                        cmd_precalc_vis_method);   // 0x2452d40

int ai_path_used;                             // 0x2452d4c
int pf_mem_size;                              // 0x2452d50 "PFG memory size" (name ours)
// 0x49c930 init_empty_49c940_thunk (jmp, static initialiser entry 289). Confidence: exact.
// 0x49c940 init_empty_49c940 (bare ret). Confidence: likely (that this array is the object).
// The dynamic initialiser of the array below: its element constructor (ai_path's two vector3 members,
// inline and empty, D500) leaves no code. The only object of this unit with such a type.
ai_path ai_path_nodes[AI_PATH_MAX_NODES];     // 0x2452d58 (name ours)
rect2d pf_bounds;                             // 0x245c998 (D1200)
int pf_search_insertions;                     // 0x245c9a8 open-list insertions (name ours)
int pf_search_start_count;                    // 0x245c9ac
// The .pfg pools (operator new from level_mempool; carved from the top by the *_remaining counts).
short *pf_word_pool;                          // 0x245c9b0 (names ours)
pf_cell *pf_cell_pool;                        // 0x245c9b4
pf_vis_node *pf_vis_node_pool;                // 0x245c9b8
pf_vis_node **pf_link_pool;                   // 0x245c9bc
void *pf_pool_9c0;                            // 0x245c9c0 cleared only, never allocated
pf_bsp *pf_bsp_pool;                          // 0x245c9c4
pf_grid_row *pf_grid_row_pool;                // 0x245c9c8
pf_grid_cell *pf_grid_cell_pool;              // 0x245c9cc
pf_edge **pf_edge_ref_pool;                   // 0x245c9d0
int *pf_dword_pool;                           // 0x245c9d4
bool Precalc_vis_method;                      // 0x245c9d8
int pf_search_id;                             // 0x245c9dc (name ours)

// File-local helpers (no header callers).
void pathfinding_atexit_free();
pf_edge *pf_read_edges(int *count, File *f, int max_edges);
void pf_read_typed_refs(File *f, int count, pf_bsp *nodes);
void pf_compute_graph_bounds();
void pf_read_edge_list(File *f, pf_grid_cell *cell);
void pf_read_region(File *f, pf_grid *grid);
void pf_skip_node_floats(File *f);
bool pf_sweep_test_wall(pf_sweep *s);
void pf_wall_normal(pf_edge *wall, vector2 *out);
void pf_sweep_record_hit(pf_sweep *s, float *t, pf_edge *wall, vector2 *pt, int *kind, vector2 **corner);
bool pf_sweep_segment(pf_sweep *s);

// 0x49c870  Confidence: exact. The console's toggle macro.
void cmd_precalc_vis_method()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Precalc_vis_method = true;
        else if (arg_type & ARG_FALSE)
            Precalc_vis_method = false;
        else if (arg_type & ARG_NONE)
            Precalc_vis_method ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "precalc_vis_method", "Precalc_vis_method");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "precalc_vis_method", Precalc_vis_method ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x49c950  Confidence: exact. Registered by pathfinding_init; the second call is a tail jump.
void pathfinding_atexit_free()
{
    ai_path_free(pf_debug_path);
    pathfinding_free_graph();
}

// 0x49c970  Confidence: likely (the loop shape: a down-counter without an entry test, the count being
// known non-zero). Appends the file's edge records to pf_edges; returns the first. max_edges is unused.
pf_edge *pf_read_edges(int *count, File *f, int max_edges)
{
    pf_edge *first;
    pf_edge *e;
    int n;

    *count = f->read_int(0, 0);
    if (*count == 0)
        return NULL;
    first = &pf_edges[pf_edge_count];
    n = *count;
    do {
        e = &pf_edges[pf_edge_count++];
        e->a = &pf_nodes[f->read_int(0, 0)];
        e->b = &pf_nodes[f->read_int(0, 0)];
        e->flags = f->read_uint16(0, 0);
        e->door = (ubyte)f->read_uint16(0, 0);
        e->_pad0b = 0;
        e->search_id = 0;
    } while (--n);
    return first;
}

// 0x49ca20  Confidence: exact. The BSP nodes of a graph; children as indices into the same block.
void pf_read_typed_refs(File *f, int count, pf_bsp *nodes)
{
    pf_bsp *b;
    int i, idx;

    for (i = 0; i < count; i++) {
        b = &nodes[i];
        b->type = f->read_int(0, 0);
        switch (b->type) {
        case 1:
            b->data = &pf_nodes[f->read_int(0, 0)];
            break;
        case 2:
            b->data = &pf_edges[f->read_int(0, 0)];
            break;
        case 3:
            f->read_int(0, 0);
            break;
        }
        idx = f->read_int(0, 0);
        if (idx != -1)
            b->front = &nodes[idx];
        else
            b->front = NULL;
        idx = f->read_int(0, 0);
        if (idx != -1)
            b->back = &nodes[idx];
        else
            b->back = NULL;
    }
}

// 0x49caf0  Confidence: exact. The first cell's bounds are copied as one rect2d (the 16-byte copy
// through `lea edx, [ecx + 0xc]`, D1200), the others widen it.
void pf_compute_graph_bounds()
{
    bool have = false;
    pf_cell *c;
    int size, layer;

    for (size = 0; size < PF_NUM_SIZES; size++) {
        for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
            pf_select_graph(size, layer);
            for (c = pf_cur_graph->cells; c; c = c->next) {
                if (have) {
                    if (c->bounds.min_x < pf_bounds.min_x)
                        pf_bounds.min_x = c->bounds.min_x;
                    if (c->bounds.max_x > pf_bounds.max_x)
                        pf_bounds.max_x = c->bounds.max_x;
                    if (c->bounds.max_y > pf_bounds.max_y)
                        pf_bounds.max_y = c->bounds.max_y;
                    if (c->bounds.min_y < pf_bounds.min_y)
                        pf_bounds.min_y = c->bounds.min_y;
                } else {
                    have = true;
                    pf_bounds = c->bounds;
                }
            }
        }
    }
}

// 0x49cbd0  Confidence: exact. One wall-grid cell: an int, the wall count and the wall indices.
void pf_read_edge_list(File *f, pf_grid_cell *cell)
{
    int n, i;

    cell->_unk00 = f->read_int(0, 0);
    cell->num_walls = f->read_uint16(0, 0);
    n = cell->num_walls;
    if (n > 0) {
        pf_edge_ref_pool_remaining -= n;
        cell->walls = &pf_edge_ref_pool[pf_edge_ref_pool_remaining];
    }
    for (i = 0; i < n; i++)
        cell->walls[i] = &pf_edges[f->read_int(0, 0)];
}

// 0x49cc50  Confidence: exact. The wall grid of a graph.
void pf_read_region(File *f, pf_grid *grid)
{
    pf_grid_row *row;
    int i, k, n;

    grid->bounds[0] = f->read_float(0, 0);
    grid->bounds[1] = f->read_float(0, 0);
    grid->bounds[2] = f->read_float(0, 0);
    grid->bounds[3] = f->read_float(0, 0);
    grid->bounds[4] = f->read_float(0, 0);
    grid->bounds[5] = f->read_float(0, 0);
    grid->_unk18 = f->read_int(0, 0);
    grid->_unk1c = f->read_int(0, 0);
    grid->num_rows = f->read_int(0, 0);
    if (grid->num_rows > 0) {
        pf_row_pool_remaining -= grid->num_rows;
        grid->rows = &pf_grid_row_pool[pf_row_pool_remaining];
    }
    grid->_unk28 = f->read_int(0, 0);
    for (i = 0; i < grid->num_rows; i++) {
        grid->rows[i].num_cells = f->read_uint16(0, 0);
        row = &grid->rows[i];
        row->_pad02 = 0;
        n = row->num_cells;
        if (n > 0) {
            pf_region_pool_remaining -= n;
            grid->rows[i].cells = &pf_grid_cell_pool[pf_region_pool_remaining];
            for (k = 0; k < n; k++)
                pf_read_edge_list(f, &grid->rows[i].cells[k]);
        }
    }
}

// 0x49cd90  Confidence: exact. A second copy of the node coordinates, dropped.
void pf_skip_node_floats(File *f)
{
    int i;

    for (i = 0; i < pf_node_count; i++) {
        f->read_float(0, 0);
        f->read_float(0, 0);
    }
}

// 0x49cdd0  Confidence: likely (statement order of the pool reads is the code's; where the memory-size
// subtraction sits and the loop forms are guesses). Loads <level>.pfg from the internal path into
// level_mempool. Version 0x6d only (the >= 0x6c branches are always taken); -4 from File::open (no
// file) is silent, other errors stop in DebugError.
int pathfinding_load_graph(const char *level_name)
{
    File f;
    char filename[256];
    char msg[256];
    int version, num_edges, dummy, err;
    int size, layer, size20, i, k, n, found;
    unsigned int mem_start, mem_end;
    pf_wall *w;
    pf_cell *c;
    pf_vis_node *node;

    avoid_reset();
    ai_path_pool_init();
    timer_get(1000);
    file_change_ext(filename, level_name, ".pfg");
    err = f.open(filename, 1, path_internal_copy);
    if (err != 0) {
        if (err != -4) {
            sprintf(msg, "Couldn't open pathfinding graph data file %s", filename);
            VSDK_ERROR(PATHFINDING_CPP, 0x284, msg);
        }
        return 0;
    }
    set_current_mempool(&level_mempool);
    mem_start = level_mempool.offset;
    version = f.read_int(0, 0);
    if (version > PFG_VERSION || version < PFG_VERSION) {
        f.close();
        VSDK_ERROR(PATHFINDING_CPP, 0x292, "Pathfinding graph data is obsolete.  Continuing without pathfinding data");
    }
    f.set_version(version);
    f.read_uint(0, 0);

    pf_word_pool_remaining = f.read_int(0, 0);
    if (pf_word_pool_remaining > 0)
        pf_word_pool = new short[pf_word_pool_remaining];
    pf_cell_pool_remaining = f.read_int(0, 0);
    if (pf_cell_pool_remaining > 0)
        pf_cell_pool = new pf_cell[pf_cell_pool_remaining];
    pf_vis_node_pool_remaining = f.read_int(0, 0);
    if (pf_vis_node_pool_remaining > 0) {
        pf_vis_node_pool = new pf_vis_node[pf_vis_node_pool_remaining];
        memset(pf_vis_node_pool, 0, pf_vis_node_pool_remaining * sizeof(pf_vis_node));
    }
    pf_link_pool_remaining = f.read_int(0, 0);
    if (pf_link_pool_remaining > 0)
        pf_link_pool = new pf_vis_node *[pf_link_pool_remaining];
    pf_unused_count_c90 = f.read_int(0, 0);
    pf_bsp_pool_remaining = f.read_int(0, 0);
    if (pf_bsp_pool_remaining > 0)
        pf_bsp_pool = new pf_bsp[pf_bsp_pool_remaining];
    pf_row_pool_remaining = f.read_int(0, 0);
    if (pf_row_pool_remaining > 0)
        pf_grid_row_pool = new pf_grid_row[pf_row_pool_remaining];
    pf_region_pool_remaining = f.read_int(0, 0);
    if (pf_region_pool_remaining > 0)
        pf_grid_cell_pool = new pf_grid_cell[pf_region_pool_remaining];
    pf_edge_ref_pool_remaining = f.read_int(0, 0);
    if (pf_edge_ref_pool_remaining > 0)
        pf_edge_ref_pool = new pf_edge *[pf_edge_ref_pool_remaining];

    pf_node_count = f.read_int(0, 0);
    if (pf_node_count > 0) {
        pf_nodes = new vector2[pf_node_count];
        for (i = 0; i < pf_node_count; i++) {
            pf_nodes[i].x = f.read_float(0, 0);
            pf_nodes[i].y = f.read_float(0, 0);
        }
    }
    num_edges = f.read_int(0, 0);
    if (num_edges > 0)
        pf_edges = new pf_edge[num_edges];
    pf_edge_count = 0;
    pf_border_edges = pf_read_edges(&pf_border_edge_count, &f, num_edges);

    pf_wall_count = f.read_int(0, 0);
    if (pf_wall_count > 0) {
        pf_walls = new pf_wall[pf_wall_count];
        for (i = 0; i < pf_wall_count; i++) {
            w = &pf_walls[i];
            if (i + 1 < pf_wall_count)
                w->next = &pf_walls[i + 1];
            else
                w->next = NULL;
            w->a = &pf_nodes[f.read_int(0, 0)];
            w->b = &pf_nodes[f.read_int(0, 0)];
            w->layer_a = f.read_int(0, 0);
            w->layer_b = f.read_int(0, 0);
            pf_layer_links[w->layer_a] |= 1 << w->layer_b;
            pf_layer_links[w->layer_b] |= 1 << w->layer_a;
        }
    }

    pf_dword_pool_remaining = f.read_int(0, 0);
    if (pf_dword_pool_remaining > 0)
        pf_dword_pool = new int[pf_dword_pool_remaining];
    for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
        n = f.read_int(0, 0);
        if (n != 0) {
            pf_dword_pool_remaining -= n;
            pf_layer_lists[layer].count = n;
            pf_layer_lists[layer].list = &pf_dword_pool[pf_dword_pool_remaining];
            for (k = 0; k < n; k++)
                pf_layer_lists[layer].list[k] = f.read_int(0, 0);
        } else {
            pf_layer_lists[layer].count = 0;
            pf_layer_lists[layer].list = NULL;
        }
    }

    for (size = 0; size < PF_NUM_SIZES; size++) {
        for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
            pf_select_graph(size, layer);
            n = f.read_int(0, 0);
            if (n != 0) {
                pf_cell_pool_remaining -= n;
                pf_cur_graph->cells = &pf_cell_pool[pf_cell_pool_remaining];
                for (i = 0; i < n; i++) {
                    c = &pf_cur_graph->cells[i];
                    if (i + 1 < n)
                        c->next = &pf_cur_graph->cells[i + 1];
                    else
                        c->next = NULL;
                    c->edges = pf_read_edges(&c->num_edges, &f, num_edges);
                    c->bounds.min_x = f.read_float(0, 0);
                    c->bounds.max_x = f.read_float(0, 0);
                    c->bounds.max_y = f.read_float(0, 0);
                    c->bounds.min_y = f.read_float(0, 0);
                }
            }
            progress_update(0);

            pf_cur_graph->count = f.read_int(0, 0);
            if (pf_cur_graph->count > 0) {
                pf_vis_node_pool_remaining -= pf_cur_graph->count;
                pf_cur_graph->points = &pf_vis_node_pool[pf_vis_node_pool_remaining];
            }
            for (i = 0; i < pf_cur_graph->count; i++) {
                node = &pf_cur_graph->points[i];
                node->layer = (short)layer;
                node->pos = &pf_nodes[f.read_int(0, 0)];
                node->num_links = (short)f.read_int(0, 0);
                if (node->num_links) {
                    pf_link_pool_remaining -= node->num_links;
                    node->links = &pf_link_pool[pf_link_pool_remaining];
                }
                if (version >= 0x6c) {
                    if (f.read_bool(0, true)) {
                        node->flags |= PFN_NO_EDGES;
                    } else {
                        node->edges[0] = &pf_edges[f.read_int(0, 0)];
                        node->edges[1] = &pf_edges[f.read_int(0, 0)];
                    }
                }
            }

            pf_cur_graph->count_b = f.read_int(0, 0);
            if (pf_cur_graph->count_b > 0) {
                pf_vis_node_pool_remaining -= pf_cur_graph->count_b;
                pf_cur_graph->points_b = &pf_vis_node_pool[pf_vis_node_pool_remaining];
            }
            for (i = 0; i < pf_cur_graph->count_b; i++) {
                node = &pf_cur_graph->points_b[i];
                node->pos = &pf_nodes[f.read_int(0, 0)];
                node->num_links = (short)f.read_int(0, 0);
                if (node->num_links) {
                    pf_link_pool_remaining -= node->num_links;
                    node->links = &pf_link_pool[pf_link_pool_remaining];
                }
                for (k = 0; k < node->num_links; k++)
                    node->links[k] = &pf_cur_graph->points_b[f.read_int(0, 0)];
            }
            progress_update(0);

            // A block of records that is read and dropped (two floats, an int list, an int16 list
            // whose size is still taken from the word pool).
            n = f.read_int(0, 0);
            for (i = 0; i < n; i++) {
                int m, j;

                f.read_float(0, 0);
                f.read_float(0, 0);
                m = f.read_int(0, 0);
                for (j = 0; j < m; j++)
                    f.read_int(0, 0);
                m = f.read_int(0, 0);
                if (m != 0) {
                    pf_word_pool_remaining -= m;
                    for (j = 0; j < m; j++)
                        f.read_int16(0, 0);
                }
            }
            progress_update(0);

            pf_cur_graph->num_bsp = f.read_int(0, 0);
            pf_bsp_pool_remaining -= pf_cur_graph->num_bsp;
            pf_cur_graph->bsp = &pf_bsp_pool[pf_bsp_pool_remaining];
            pf_read_typed_refs(&f, pf_cur_graph->num_bsp, pf_cur_graph->bsp);
        }
    }

    pf_read_edges(&dummy, &f, num_edges);
    for (size = 0; size < PF_NUM_SIZES; size++) {
        for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
            pf_select_graph(size, layer);
            pf_read_region(&f, &pf_cur_graph->grid);
        }
    }

    // The visibility links between the layers of one size class; before version 0x6c the wall edges
    // meeting at each point were searched for here.
    for (size = 0, size20 = 0; size < PF_NUM_SIZES; size++, size20 += PF_MAX_LAYERS) {
        for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
            pf_select_graph(size, layer);
            for (i = 0; i < pf_cur_graph->count; i++) {
                node = &pf_cur_graph->points[i];
                for (k = 0; k < node->num_links; k++) {
                    int l = f.read_uint16(0, 0);
                    int idx = f.read_int(0, 0);

                    node->links[k] = &pf_graphs[0][size20 + l].points[idx];
                }
                if (version < 0x6c && !(node->flags & PFN_NO_EDGES)) {
                    found = 0;
                    for (c = pf_cur_graph->cells; c; c = c->next) {
                        int j;
                        pf_edge *e;

                        for (j = 0, e = c->edges; j < c->num_edges; j++, e++) {
                            if (vector2::nearly_equal(e->a, node->pos) || vector2::nearly_equal(e->b, node->pos)) {
                                if (found >= 2)
                                    VSDK_ERROR(PATHFINDING_CPP, 0x411, "More than 2 segs connected to a single point");
                                node->edges[found++] = e;
                            }
                        }
                    }
                }
            }
        }
    }

    set_current_mempool(NULL);
    mem_end = level_mempool.offset;
    timer_get(1000);
    pf_skip_node_floats(&f);
    pf_compute_graph_bounds();
    pf_mem_size = mem_end - mem_start;
    sprintf(string_buffer, "PFG memory size: %d\n", pf_mem_size);
    return f.close();
}

// 0x49d930  Confidence: exact. Threads the pool backwards (the last node heads the free list). The
// peak counter starts at 500, half the pool (as in the binary).
void ai_path_pool_init()
{
    ai_path *prev = NULL;
    ai_path *n;

    for (n = ai_path_nodes; n < &ai_path_nodes[AI_PATH_MAX_NODES]; n++) {
        n->next = prev;
        prev = n;
    }
    ai_path_free_list = prev;
    ai_path_used = 0;
    ai_path_peak_used = 500;
}

// 0x49d970  Confidence: exact.
int pf_next_search_id()
{
    return ++pf_search_id;
}

// 0x49d980  Confidence: exact (the store order of the pointers is the code's; the atexit argument is
// pushed before them).
void pathfinding_init()
{
    memset(pf_graphs, 0, sizeof(pf_graphs));
    memset(pf_layer_links, 0, sizeof(pf_layer_links));
    pf_nodes = NULL;
    pf_unk_cac = NULL;
    pf_border_edges = NULL;
    pf_walls = NULL;
    pf_word_pool = NULL;
    pf_cell_pool = NULL;
    pf_vis_node_pool = NULL;
    pf_link_pool = NULL;
    pf_pool_9c0 = NULL;
    pf_bsp_pool = NULL;
    pf_dword_pool = NULL;
    pf_edge_ref_pool = NULL;
    pf_grid_cell_pool = NULL;
    pf_grid_row_pool = NULL;
    atexit(pathfinding_atexit_free);
}

// 0x49d9f0  Confidence: exact (VC6 byte match) (whether the `if (p) p = NULL` pairs were a free macro that expands to
// nothing for mempool memory). Nothing is freed: the arrays live in level_mempool.
void pathfinding_free_graph()
{
    if (pf_node_count && pf_nodes) {
        pf_nodes = NULL;
        pf_node_count = 0;
    }
    if (pf_edge_count && pf_edges) {
        pf_edges = NULL;
        pf_edge_count = 0;
    }
    pf_border_edges = NULL;
    pf_border_edge_count = 0;
    if (pf_wall_count) {
        pf_walls = NULL;
        pf_wall_count = 0;
    }
    if (pf_word_pool)
        pf_word_pool = NULL;
    if (pf_cell_pool)
        pf_cell_pool = NULL;
    if (pf_vis_node_pool)
        pf_vis_node_pool = NULL;
    if (pf_link_pool)
        pf_link_pool = NULL;
    if (pf_pool_9c0)
        pf_pool_9c0 = NULL;
    if (pf_bsp_pool)
        pf_bsp_pool = NULL;
    if (pf_dword_pool)
        pf_dword_pool = NULL;
    if (pf_edge_ref_pool)
        pf_edge_ref_pool = NULL;
    if (pf_grid_cell_pool)
        pf_grid_cell_pool = NULL;
    if (pf_grid_row_pool)
        pf_grid_row_pool = NULL;
    pf_word_pool_remaining = 0;
    pf_cell_pool_remaining = 0;
    pf_vis_node_pool_remaining = 0;
    pf_link_pool_remaining = 0;
    pf_unused_count_c90 = 0;
    pf_bsp_pool_remaining = 0;
    pf_edge_ref_pool_remaining = 0;
    pf_region_pool_remaining = 0;
    pf_row_pool_remaining = 0;
    memset(pf_graphs, 0, sizeof(pf_graphs));
    memset(pf_layer_links, 0, sizeof(pf_layer_links));
    avoid_seglists_clear_refs();
}

// 0x49db20  Confidence: likely (the perpendicular point's operand order). The line through p
// perpendicular to the segment, intersected with it; clamped to the end points.
void seg2d_closest_point(vector2 *p, const seg2d *s, vector2 *out)
{
    vector2 perp;
    seg2d line;

    if (vector2::nearly_equal(s->a, s->b)) {
        *out = *s->a;
        return;
    }
    line.a = p;
    line.b = &perp;
    perp.x = p->x + (s->b->y - s->a->y);
    perp.y = p->y - (s->b->x - s->a->x);
    seg2d_line_intersect(s, &line, out, 0);
    if (seg2d_param < 0.0f)
        *out = *s->a;
    else if (seg2d_param > 1.0f)
        *out = *s->b;
}

// 0x49dbe0  Confidence: exact.
bool pf_door_index_is_open(int door, float fraction)
{
    if (door >= 0 && door < num_doors) {
        door_info *d = &doors[door];
        return door_is_open(d, fraction);
    }
    return true;
}

// 0x49dc20  Confidence: likely (the clamp expressions and the kind variable, which the compiler also
// uses as the constant 2 of the door-flag test). Tests the sweep against s->wall, once per search id:
// from in front of the wall, to at most 0.03 behind it; the move shifted back by 0.03 along the normal
// must cross the wall (kind 0), or pass within 0.03 of an end point that is not s->last_corner
// (kind 1, corner hit). Fills s->hit, s->t, s->hit_kind and s->hit_corner.
bool pf_sweep_test_wall(pf_sweep *s)
{
    pf_edge *wall = s->wall;
    vector2 a, b, dir, n, rel, off, p, q, cp;
    vector2 *corner;
    float len, eps, t, u, tt, c;
    int kind;
    bool ok;

    if (wall->search_id == s->search_id)
        return false;
    wall->search_id = s->search_id;
    a.x = wall->a->x;
    a.y = wall->a->y;
    b.x = wall->b->x;
    b.y = wall->b->y;
    dir.x = b.x - a.x;
    dir.y = b.y - a.y;
    len = dir.normalize();
    n.x = dir.y;
    n.y = -dir.x;
    if (wall->flags & PF_EDGE_SIDE)
        n.negate();
    kind = 2;
    if ((wall->flags & PF_EDGE_DOOR) && pf_door_index_is_open(wall->door, 0.4f))
        return false;
    if ((s->from.x - a.x) * n.x + (s->from.y - a.y) * n.y <= 0.0f)
        return false;
    rel.x = s->to.x - a.x;
    rel.y = s->to.y - a.y;
    if (rel.y * n.y + rel.x * n.x > 0.03f)
        return false;
    off.x = n.x * 0.03f;
    off.y = n.y * 0.03f;
    p.x = s->from.x - off.x;
    p.y = s->from.y - off.y;
    q.x = s->to.x - off.x;
    q.y = s->to.y - off.y;
    eps = 0.03f / len;
    if (seg2d_intersect_params(&p, &q, &a, &b, &t, &u) && u >= eps * -0.5f && u <= 1.0 + eps * 0.5
        && t >= -0.001f && t <= 1.001f) {
        kind = 0;
    } else {
        corner = NULL;
        if (u >= eps * -2.0f && u <= eps * 2.0f && s->last_corner != wall->a
            && vector2_closest_point_on_segment(&a, &s->from, &s->to, &cp) < 0.0009f) {
            kind = 1;
            corner = &a;
        }
        if (u >= 1.0f - eps * 2.0f && u <= eps * 2.0f + 1.0f && s->last_corner != wall->b
            && vector2_closest_point_on_segment(&b, &s->from, &s->to, &cp) < 0.0009f) {
            kind = 1;
            corner = &b;
        }
    }
    if (kind == 1) {
        ok = vector2_segment_circle_intersect(&s->from, &s->to, corner, 0.03f, &tt);
        if (!ok)
            tt = 0.0f;
        if (ok && tt >= 1.0f)
            c = 1.0f;
        else
            c = (tt > 0.0f) ? tt : 0.0f;
        s->hit_kind = 1;
        s->hit.x = (s->to.x - s->from.x) * c + s->from.x;
        s->hit.y = (s->to.y - s->from.y) * c + s->from.y;
        s->t = c;
        if (u > 0.5f)
            s->hit_corner = wall->b;
        else
            s->hit_corner = wall->a;
        return true;
    }
    if (kind == 0) {
        c = (t < 1.0f) ? ((t > 0.0f) ? t : 0.0f) : 1.0f;
        s->hit_kind = 0;
        s->hit.x = (s->to.x - s->from.x) * c + s->from.x;
        s->hit.y = (s->to.y - s->from.y) * c + s->from.y;
        s->t = t;
        s->hit_corner = NULL;
        return true;
    }
    return false;
}

// 0x49e050  Confidence: likely (the end points are loaded into temporaries first). The unit normal of
// the wall's open side.
void pf_wall_normal(pf_edge *wall, vector2 *out)
{
    vector2 d;

    d = *wall->b - *wall->a;
    d.normalize();
    out->x = d.y;
    out->y = -d.x;
    if (wall->flags & PF_EDGE_SIDE)
        out->negate();
}

// 0x49e0c0  Confidence: exact.
void pf_sweep_record_hit(pf_sweep *s, float *t, pf_edge *wall, vector2 *pt, int *kind, vector2 **corner)
{
    *t = s->t;
    *wall = *s->wall;
    *pt = s->hit;
    *kind = s->hit_kind;
    *corner = s->hit_corner;
}

// 0x49e110  Confidence: likely (the break tests at the top of the obstacle and spline loops). The
// nearest hit of the sweep among the nav walls of the grid cells it crosses, the outlines of the
// obstacles of its size / layer that it touches and the avoidance spline walls. False when nothing is
// hit (best t still 99.9).
bool pf_sweep_segment(pf_sweep *s)
{
    float best_t = 99.9f;
    int best_kind = 2;
    vector2 *best_corner = NULL;
    vector2 best_hit;
    pf_edge best_wall;
    pf_edge w;
    int size, layer, count, i, k, prev, found;
    pf_grid_cell *cell;
    avoid_obstacle *ob;
    avoid_spline *sp;
    avoid_spline_point *pt;

    size = s->size;
    layer = s->layer;
    best_wall.flags = 0;
    best_hit.x = 0.0f;
    best_hit.y = 0.0f;
    count = pf_cells_along_segment(s->layer, s->size, &s->from, &s->to);
    for (i = 0; i < count; i++) {
        cell = pf_grid_cell_results[i];
        for (k = 0; k < cell->num_walls; k++) {
            s->wall = cell->walls[k];
            if (pf_sweep_test_wall(s) && fabsf(s->t) < fabsf(best_t))
                pf_sweep_record_hit(s, &best_t, &best_wall, &best_hit, &best_kind, &best_corner);
        }
    }

    found = 0;
    for (ob = avoid_obstacles; ob < &avoid_obstacles[AVOID_NUM_OBSTACLES]; ob++) {
        if (found == avoid_obstacle_counts[size * PF_MAX_LAYERS + layer])
            break;
        if ((ob->flags & 1) && (ob->flags & 2) && ob->layer == layer && ob->size == size) {
            if (avoid_segment_hits_prim(ob, &s->from, &s->to, -0.1f)) {
                prev = ob->num_points - 1;
                for (k = 0; k < ob->num_points; prev = k++) {
                    w.a = &ob->points[prev];
                    w.b = &ob->points[k];
                    w.search_id = 0;
                    w.flags = 0;
                    s->wall = &w;
                    if (pf_sweep_test_wall(s) && fabsf(s->t) < fabsf(best_t))
                        pf_sweep_record_hit(s, &best_t, &best_wall, &best_hit, &best_kind, &best_corner);
                }
            }
            found++;
        }
    }

    found = 0;
    for (i = 0; i < 20; i++) {
        if (found == avoid_spline_point_counts[size * PF_MAX_LAYERS + layer])
            break;
        sp = avoid_spline_get(i);
        if (!sp)
            continue;
        for (k = 0; k < sp->count; k++) {
            pt = avoid_spline_point_get(sp->points[k]);
            if (pt && pt->size == size && pt->layer == layer) {
                pt->wall.search_id = 0;
                found++;
                s->wall = &pt->wall;
                if (pf_sweep_test_wall(s) && fabsf(s->t) < fabsf(best_t))
                    pf_sweep_record_hit(s, &best_t, &best_wall, &best_hit, &best_kind, &best_corner);
            }
        }
    }

    if (best_t == 99.9f)
        return false;
    s->hit = best_hit;
    s->hit_wall = best_wall;
    s->last_corner = best_corner;
    s->hit_kind = best_kind;
    return true;
}

// 0x49e470  Confidence: likely (the slide expression's operand order). Sweeps the x / z move; on a hit
// slides the rest of it along the wall (or round the corner) and sweeps again.
bool pf_slide_move(const vector3 *from, const vector3 *to, vector3 *out, int size, int layer)
{
    pf_sweep s;
    vector2 n, d, push, proj;
    float dot;

    s.from.x = from->x;
    s.from.y = from->z;
    s.to.x = to->x;
    s.to.y = to->z;
    s.layer = layer;
    s.size = size;
    s.last_corner = NULL;
    s.hit_corner = NULL;
    s.search_id = pf_next_search_id();
    if (!pf_sweep_segment(&s))
        return false;
    if (s.hit_kind == 0) {
        pf_wall_normal(&s.hit_wall, &n);
    } else {
        n.x = s.hit.x - s.last_corner->x;
        n.y = s.hit.y - s.last_corner->y;
        n.normalize();
    }
    d.x = s.to.x - s.hit.x;
    d.y = s.to.y - s.hit.y;
    s.from = s.hit;
    push.x = n.x * 0.015f;
    push.y = n.y * 0.015f;
    dot = n.y * d.y + d.x * n.x;
    proj.x = dot * n.x;
    proj.y = n.y * dot;
    s.to.x = d.x + s.hit.x - proj.x + push.x;
    s.to.y = d.y + s.hit.y - proj.y + push.y;
    s.search_id = pf_next_search_id();
    if (!pf_sweep_segment(&s)) {
        out->x = s.to.x;
        out->z = s.to.y;
        out->y = from->y;
        return true;
    }
    out->x = s.hit.x;
    out->z = s.hit.y;
    out->y = from->y;
    return true;
}

// 0x49e620  Confidence: likely (vector3::len's operand order; the clipped end through operator= whose
// returned copy is the out-of-line copy constructor call 0x410cd0). Only door walls (PF_EDGE_DOOR) of
// closed doors count; the move is clipped to *max_len first.
bool pf_check_direct_walk(const vector3 *from, const vector3 *to, int layer, int size, const float *max_len)
{
    vector3 end, d;
    vector2 a, b, dir, n;
    pf_grid_cell *cell;
    pf_edge *w;
    float len, tw, tm;
    int count, id, i, k, nw;

    if (!pf_layer_lists[layer].count)
        return false;
    end = *to;
    d = end - *from;
    len = d.len();
    if (len > *max_len)
        end = *from + d * (*max_len / len);
    a.x = from->x;
    a.y = from->z;
    b.x = end.x;
    b.y = end.z;
    count = pf_cells_along_segment(layer, size, &a, &b);
    if (!count)
        return false;
    id = pf_next_search_id();
    for (i = 0; i < count; i++) {
        cell = pf_grid_cell_results[i];
        nw = cell->num_walls;
        for (k = 0; k < nw; k++) {
            w = cell->walls[k];
            if (w->search_id == id)
                continue;
            w->search_id = id;
            if (!(w->flags & PF_EDGE_DOOR))
                continue;
            dir.x = b.x - a.x;
            dir.y = b.y - a.y;
            n.x = w->b->y - w->a->y;
            n.y = -(w->b->x - w->a->x);
            if (w->flags & PF_EDGE_SIDE)
                n.negate();
            if (n.y * dir.y + n.x * dir.x < 0.0f && !pf_door_index_is_open(w->door, 0.4f)
                && seg2d_intersect_params(w->a, w->b, &a, &b, &tw, &tm)
                && tm >= 0.0f && tm <= 1.0f && tw >= -0.01f && tw <= 1.01)
                return true;
        }
    }
    return false;
}

// 0x49e8c0  Confidence: likely (the squared-distance operand order and the restart loop's form). Pushes
// *pos 0.035 off every cell edge it is within 0.025 of (or, when it starts off the mesh, onto the
// nearest edge), repeating with the two edges just used excluded; when it still ends off the mesh it
// takes the midpoint of the last two positions. The height is kept. True when it moved the point.
bool pf_find_nearest_navigable_point(vector3 *pos, int size, int layer)
{
    profile_label label("pf_find_nearest_navigable_point");
    bool found = false;
    float best = 1e9f;
    vector3 p1, p2, nrm;
    pf_edge *best_edge, *last, *prev, *e;
    pf_cell **cells;
    pf_cell *c;
    vector2 p, cp;
    float y, dx, dy, d2;
    bool again, in_x, in_z;
    int i;

    p1.zero();
    p2.zero();
    best_edge = NULL;
    prev = NULL;
    y = pos->y;
    last = NULL;
    again = !pf_point_is_navigable(pos, size, layer);
    cells = &pf_graphs[size][layer].cells;
    for (;;) {
        p.from_xz(pos);
        for (c = *cells; c; c = c->next) {
            in_x = c->bounds.max_x + 1.0f > pos->x && c->bounds.min_x - 1.0f < pos->x;
            in_z = c->bounds.max_y + 1.0f > pos->z && c->bounds.min_y - 1.0f < pos->z;
            if (!((in_x && in_z) || again))
                continue;
            for (i = 0, e = c->edges; i < c->num_edges; i++, e++) {
                if (e == last || e == prev)
                    continue;
                seg2d_closest_point(&p, e, &cp);
                dx = cp.x - p.x;
                dy = cp.y - p.y;
                d2 = dy * dy + dx * dx;
                if (d2 < 0.000625f)
                    again = true;
                else if (!again)
                    continue;
                if (d2 < best) {
                    best = d2;
                    best_edge = e;
                    nrm.x = e->b->y - e->a->y;
                    nrm.y = 0.0f;
                    nrm.z = e->a->x - e->b->x;
                    nrm.normalize();
                    if (e->flags & PF_EDGE_SIDE)
                        *pos = cp.to_vector3() - nrm * 0.035f;
                    else
                        *pos = cp.to_vector3() + nrm * 0.035f;
                }
            }
        }
        if (!again)
            break;
        prev = last;
        last = best_edge;
        p2 = p1;
        p1 = *pos;
        again = false;
        found = true;
        best = 1e9f;
    }
    if (!pf_point_is_navigable(pos, size, layer) && last && prev)
        *pos = (p1 + p2) * 0.5f;
    pos->y = y;
    return found;
}

// 0x49ecb0  Confidence: likely. Over the cells of the current graph whose bounds the segment's box
// overlaps, the edge crossing nearest to `to`; out = from + (to - from) * (t - 0.02). The y test is
// reversed in the binary (segment min below the cell's min and max above its max; NOTES.md, bug).
bool pf_find_cell_crossing(vector2 *out, vector2 *from, vector2 *to)
{
    profile_label label("pf_find_cell_crossing");
    bool found = false;
    float best = 1e9f;
    seg2d seg;
    vector2 hit;
    pf_cell *c;
    pf_edge *e;
    float min_x, max_x, min_y, max_y, dx, dy, d2;
    int i;

    seg.a = from;
    seg.b = to;
    for (c = pf_cur_graph->cells; c; c = c->next) {
        if (from->x < to->x) {
            min_x = from->x;
            max_x = to->x;
        } else {
            min_x = to->x;
            max_x = from->x;
        }
        if (from->y < to->y) {
            min_y = from->y;
            max_y = to->y;
        } else {
            min_y = to->y;
            max_y = from->y;
        }
        if (min_x < c->bounds.max_x && max_x > c->bounds.min_x && min_y < c->bounds.min_y
            && max_y > c->bounds.max_y) {
            for (i = 0, e = c->edges; i < c->num_edges; i++, e++) {
                if (seg2d_segment_intersect(&seg, e, &hit) > 0) {
                    dx = hit.x - to->x;
                    dy = hit.y - to->y;
                    d2 = dy * dy + dx * dx;
                    if (d2 < best) {
                        seg2d_param = seg2d_param / seg2d_denom - 0.02f;
                        found = true;
                        out->x = (to->x - from->x) * seg2d_param + from->x;
                        out->y = (to->y - from->y) * seg2d_param + from->y;
                        best = d2;
                    }
                }
            }
        }
    }
    return found;
}

// 0x49ee80  Confidence: likely (the reach test `ok &= flags >> 4`). Grows a box round the point (5 units
// per pass, a pass less every 16 iterations) until enough nodes of the reachable layers are inside
// (PFN_IN_REACH; a node on another layer counts only when following the layer edges from the point
// ends on its layer), then walk-tests them (PFN_VISIBLE); repeats until `passes` nodes are visible or
// more than 50 are in reach. The avoidance corners of the layer take part. Returns the visible count.
int pf_mark_nodes_visible_from_point(const vector2 *pt, int layer, int walk_flags)
{
    int passes = 3;
    int iter = 0;
    int considered, in_reach, visible, max_reach, l, i, k, hl, cl;
    float min_x, max_x, min_y, max_y;
    pf_graph *g;
    pf_vis_node *n;
    vector2 *v;
    vector2 cur, hit;
    seg2d seg;
    avoid_seglist *sl;
    avoid_corner *c;
    ubyte ok;

    min_x = pt->x - 5.0f;
    max_x = pt->x + 5.0f;
    max_y = pt->y + 5.0f;
    min_y = pt->y - 5.0f;
    for (l = 0; l < PF_MAX_LAYERS; l++) {
        g = &pf_graphs[pf_cur_size][l];
        for (i = 0; i < g->count; i++)
            g->points[i].flags &= ~(PFN_VISIBLE | PFN_IN_REACH | PFN_LAYER_TESTED | PFN_WALK_TESTED);
    }
    for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
        if ((sl->flags & AVOID_SEGLIST_USED) && !(sl->flags & AVOID_SEGLIST_OFF) && sl->layer == layer
            && sl->size == pf_cur_size) {
            for (k = 0; k < sl->num_corners; k++)
                sl->corners[k].node.flags &= ~(PFN_VISIBLE | PFN_IN_REACH | PFN_LAYER_TESTED | PFN_WALK_TESTED);
        }
    }
    do {
        do {
            iter++;
            if (iter & 0x10)
                passes--;
            min_x -= 5.0f;
            max_x += 5.0f;
            max_y += 5.0f;
            min_y -= 5.0f;
            considered = 0;
            in_reach = 0;
            for (l = 0; l < PF_MAX_LAYERS; l++) {
                if (!(pf_layer_links[layer] & (1 << l)) && l != layer)
                    continue;
                g = &pf_graphs[pf_cur_size][l];
                considered += g->count;
                for (i = 0; i < g->count; i++) {
                    n = &g->points[i];
                    v = n->pos;
                    if (v->x < min_x || v->x > max_x || v->y > max_y || v->y < min_y)
                        continue;
                    ok = 1;
                    if (l != layer) {
                        if (n->flags & PFN_LAYER_TESTED) {
                            ok &= n->flags >> 4;
                        } else {
                            cur = *pt;
                            seg.a = &cur;
                            seg.b = v;
                            cl = layer;
                            while (pf_first_edge_crossing(&seg, cl, &hit, &hl)) {
                                cur = hit;
                                cl = hl;
                            }
                            if (cl != l)
                                ok = 0;
                            n->flags |= PFN_LAYER_TESTED;
                        }
                    }
                    if (ok) {
                        n->flags |= PFN_IN_REACH;
                        in_reach++;
                    }
                }
            }
            for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
                if ((sl->flags & AVOID_SEGLIST_USED) && !(sl->flags & AVOID_SEGLIST_OFF) && sl->layer == layer
                    && sl->size == pf_cur_size) {
                    for (k = 0; k < sl->num_corners; k++) {
                        v = sl->corners[k].node.pos;
                        if (v->x < min_x || v->x > max_x || v->y > max_y || v->y < min_y)
                            continue;
                        sl->corners[k].node.flags |= PFN_IN_REACH;
                        in_reach++;
                    }
                }
            }
            max_reach = considered;
            if (max_reach > 12)
                max_reach = 12;
        } while (passes >= 0 && in_reach < max_reach);

        visible = 0;
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            if (!(pf_layer_links[layer] & (1 << l)) && l != layer)
                continue;
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++) {
                n = &g->points[i];
                if (n->flags & PFN_IN_REACH) {
                    if (!(n->flags & PFN_WALK_TESTED)) {
                        if (pf_straight_walk_clear(pt, layer, n->pos, l, walk_flags))
                            n->flags |= PFN_VISIBLE;
                        n->flags |= PFN_WALK_TESTED;
                    }
                    if (n->flags & PFN_VISIBLE)
                        visible++;
                }
            }
        }
        for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
            if ((sl->flags & AVOID_SEGLIST_USED) && !(sl->flags & AVOID_SEGLIST_OFF) && sl->layer == layer
                && sl->size == pf_cur_size) {
                for (k = 0; k < sl->num_corners; k++) {
                    c = &sl->corners[k];
                    if (c->node.flags & PFN_IN_REACH) {
                        if (!(c->node.flags & PFN_WALK_TESTED)) {
                            if (pf_straight_walk_clear(pt, layer, c->node.pos, sl->layer, walk_flags))
                                c->node.flags |= PFN_VISIBLE;
                            c->node.flags |= PFN_WALK_TESTED;
                        }
                        if (c->node.flags & PFN_VISIBLE)
                            visible++;
                    }
                }
            }
        }
    } while (visible < passes && in_reach <= 50);
    return visible;
}

// 0x49f340  Confidence: likely (statement forms of the flag copies). PFN_VISIBLE of the start goes to
// PFN_FROM_START, of the end to PFN_FROM_GOAL; with Precalc_vis_method by the edge sweep of
// pathfinding_visibility.cpp (graph nodes only), else by pf_mark_nodes_visible_from_point (with the
// avoidance corners of the current size).
void pf_compute_endpoint_visibility(const vector2 *start, int start_layer, const vector2 *end, int end_layer)
{
    profile_label label("pf_compute_endpoint_visibility");
    pf_graph *g;
    avoid_seglist *sl;
    int l, i, k;

    timer_get(1000);
    if (Precalc_vis_method) {
        determine_point_visibility(start, start_layer);
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++) {
                if (g->points[i].flags & PFN_VISIBLE)
                    g->points[i].flags |= PFN_FROM_START;
                else
                    g->points[i].flags &= ~PFN_FROM_START;
            }
        }
        determine_point_visibility(end, end_layer);
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++) {
                if (g->points[i].flags & PFN_VISIBLE)
                    g->points[i].flags |= PFN_FROM_GOAL;
                else
                    g->points[i].flags &= ~PFN_FROM_GOAL;
            }
        }
    } else {
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++)
                g->points[i].flags &= 0xfe80;
        }
        for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
            if ((sl->flags & AVOID_SEGLIST_USED) && sl->size == pf_cur_size) {
                for (k = 0; k < sl->num_corners; k++)
                    sl->corners[k].node.flags &= 0xfe80;
            }
        }
        pf_mark_nodes_visible_from_point(start, start_layer, 3);
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++) {
                if (g->points[i].flags & PFN_VISIBLE)
                    g->points[i].flags |= PFN_FROM_START;
                g->points[i].flags &= ~PFN_VISIBLE;
            }
        }
        for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
            if ((sl->flags & AVOID_SEGLIST_USED) && sl->size == pf_cur_size) {
                for (k = 0; k < sl->num_corners; k++) {
                    if (sl->corners[k].node.flags & PFN_VISIBLE)
                        sl->corners[k].node.flags |= PFN_FROM_START;
                    sl->corners[k].node.flags &= ~PFN_VISIBLE;
                }
            }
        }
        pf_mark_nodes_visible_from_point(end, end_layer, 3);
        for (l = 0; l < PF_MAX_LAYERS; l++) {
            g = &pf_graphs[pf_cur_size][l];
            for (i = 0; i < g->count; i++) {
                if (g->points[i].flags & PFN_VISIBLE)
                    g->points[i].flags |= PFN_FROM_GOAL;
            }
        }
        for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
            if ((sl->flags & AVOID_SEGLIST_USED) && sl->size == pf_cur_size) {
                for (k = 0; k < sl->num_corners; k++) {
                    if (sl->corners[k].node.flags & PFN_VISIBLE)
                        sl->corners[k].node.flags |= PFN_FROM_GOAL;
                }
            }
        }
    }
    timer_get(1000);
}

// 0x49f650  Confidence: likely (the index arithmetic of the reversed array). Drops leading nodes while
// the start sees the next one, then the first node of every layer change that is not the last, then
// (for paths of three or more) nodes before the goal while the goal is straight-walkable from further
// back (last 32 nodes only). Nodes with flag 2 stop both walks.
ai_path *ai_path_smooth(ai_path *path, const vector3 *start, int start_layer)
{
    profile_label label("ai_path_smooth");
    ai_path *nodes[32];
    vector2 from, to;
    ai_path *n, *prev;
    int layer, count, num, i;

    if (!path)
        return NULL;
    from.from_xz(start);
    while (path->next && !(path->flags & 2)) {
        to.from_xz(&path->next->pos);
        if (!pf_straight_walk_clear(&from, start_layer, &to, path->next->layer, 7))
            break;
        n = path;
        path = path->next;
        ai_path_free_node(n);
    }
    layer = -1;
    prev = NULL;
    for (n = path; n; n = n->next) {
        if (layer == -1 || n->layer == layer) {
            layer = n->layer;
        } else if (n->next) {
            prev->next = n->next;
            layer = n->layer;
            ai_path_free_node(n);
            n = prev->next;
        }
        prev = n;
    }
    count = 0;
    for (n = path; n; n = n->next)
        count++;
    if (count <= 2)
        return path;
    num = count;
    if (num > 32)
        num = 32;
    i = num - count;
    for (n = path; n; n = n->next, i++) {
        if (i >= 0)
            nodes[num - 1 - i] = n;
    }
    from.from_xz(&nodes[0]->pos);
    for (i = 2; i < num; i++) {
        if (nodes[i - 1]->flags & 2)
            break;
        to.from_xz(&nodes[i]->pos);
        if (!pf_straight_walk_clear(&to, nodes[i]->layer, &from, nodes[0]->layer, 7))
            break;
        nodes[i]->next = nodes[0];
        ai_path_free_node(nodes[i - 1]);
    }
    return path;
}

// 0x49f810  Confidence: likely (the FLT_MAX store is the code's, its variable a guess). For a search
// edge on one layer: the avoidance polygons of its layer / size whose bounds meet the edge's either
// block it (door-type lists, skipped when an end has PFN_SKIP_DOOR_LISTS) or add the cost of going
// round their obstacles. True when blocked or when a cost was added.
bool pf_add_avoidance_nodes(pf_search_seg *s)
{
    rect2d r;
    avoid_prim prim;
    avoid_seglist *sl;
    avoid_poly *poly;
    avoid_obstacle *ob;
    bool have_rect;
    int any, door, k;

    s->blocked = false;
    s->cost = 0.0f;
    if (s->a->layer != s->b->layer)
        return false;
    have_rect = false;
    r.min_y = FLT_MAX;
    if ((s->a->flags & PFN_SKIP_DOOR_LISTS) || (s->b->flags & PFN_SKIP_DOOR_LISTS))
        any = 0;
    else
        any = 1;
    for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
        if (!(sl->flags & AVOID_SEGLIST_USED))
            continue;
        door = sl->flags & AVOID_SEGLIST_DOOR;
        if (!any && door)
            continue;
        if (sl->layer != s->a->layer || sl->size != s->size)
            continue;
        if (!have_rect) {
            r.min_y = r.max_y = s->a->pos->y;
            r.min_x = r.max_x = s->a->pos->x;
            r.min_y = min(r.min_y, s->b->pos->y);
            r.max_y = max(r.max_y, s->b->pos->y);
            r.min_x = min(r.min_x, s->b->pos->x);
            r.max_x = max(r.max_x, s->b->pos->x);
            have_rect = true;
        }
        poly = avoid_poly_get(sl);
        if (!rect2d_overlap(r, poly->bounds))
            continue;
        avoid_prim_from_segment(&prim, s->a->pos, s->b->pos);
        for (k = 0; k < poly->count; k++) {
            ob = &avoid_obstacles[(int)poly->obstacles[k]];
            if (door) {
                if (avoid_prim_overlap(ob, &prim, 0.0f)) {
                    s->blocked = true;
                    return true;
                }
            } else {
                s->cost += avoid_poly_add_search_nodes(&prim, ob);
            }
        }
    }
    if (s->cost != 0.0f)
        return true;
    return false;
}

// 0x49fa70  Confidence: likely (the local nodes' field stores are in a guessed order; `direct` is a
// local that is only ever false). A* over the visibility graph of the size class, from a local start
// node linked to the nodes it sees (PFN_FROM_START, up to 100) to a local goal reached through the
// nodes that see the goal (PFN_FROM_GOAL) and the avoidance corners; edges cost their length plus
// pf_add_avoidance_nodes' cost. The path is built back from the goal node (ai_path_node_create_b of
// *to, cost -cost_bias). On a full node pool it frees NULL and leaks the partial path (NOTES.md).
ai_path *pf_graph_search(const vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                         float cost_bias)
{
    bool direct = false;
    vector2 goal, start, pt;
    pf_vis_node start_node, goal_entry, goal_node;
    pf_vis_node *goal_link;
    pf_vis_node *start_links[100];
    pf_search_seg seg;
    pf_bsp *start_cell, *end_cell;
    pf_vis_node *cur, *nb, *p;
    pf_graph *g;
    avoid_seglist *sl;
    avoid_corner *c;
    ai_path *path;
    float cost, dx, dy;
    int found_layer, l, i, j, k;
    bool walk;

    pf_select_graph(size, from_layer);
    start.x = from->x;
    start.y = from->z;
    goal.x = to->x;
    goal.y = to->z;
    pf_search_insertions = 0;
    pf_search_iterations = 0;
    start_cell = pf_bsp_find_cell(from, from_layer);
    end_cell = pf_bsp_find_cell(to, to_layer);
    if (!end_cell) {
        if (!start_cell)
            return NULL;
        if (!pf_find_cell_crossing(&pt, &start, &goal))
            return NULL;
        goal = pt;
        to->x = pt.x;
        to->z = pt.y;
        end_cell = pf_bsp_find_cell(to, to_layer);
        if (!end_cell)
            return NULL;
    }
    found_layer = -1;
    for (l = 0; l < PF_MAX_LAYERS; l++) {
        if ((pf_layer_links[to_layer] & (1 << l)) && pf_point_is_navigable(to, size, l) == true) {
            found_layer = l;
            break;
        }
    }
    pf_select_graph(size, from_layer);
    if (found_layer != -1)
        to_layer = pf_layer_from_edges(found_layer, to_layer, to);
    path = ai_path_node_create_b(*to, to_layer, NULL);
    if (!path)
        return NULL;
    path->cost = -cost_bias;
    start_node.pos = &start;
    goal_node.pos = &goal;
    if (start_cell == end_cell || direct) {
        walk = pf_straight_walk_clear(&start, from_layer, &goal, to_layer, 2);
        if (walk && pf_straight_walk_clear(&start, from_layer, &goal, to_layer, 4))
            return path;
    } else {
        walk = pf_straight_walk_clear(&start, from_layer, &goal, to_layer, 3);
        if (walk && pf_straight_walk_clear(&start, from_layer, &goal, to_layer, 4))
            return path;
    }
    pf_compute_endpoint_visibility(&start, from_layer, &goal, to_layer);
    for (l = 0; l < PF_MAX_LAYERS; l++) {
        for (i = 0; i < pf_graphs[pf_cur_size][l].count; i++)
            pf_graphs[pf_cur_size][l].points[i].parent = NULL;
    }
    for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
        if ((sl->flags & AVOID_SEGLIST_USED) && sl->size == size) {
            for (k = 0; k < sl->num_corners; k++)
                sl->corners[k].node.parent = NULL;
        }
    }

    start_node.parent = &start_node;
    goal_entry.pos = &goal;
    goal_link = &goal_node;
    goal_entry.links = &goal_link;
    dx = to->x - from->x;
    dy = to->z - from->z;
    start_node.f = (float)sqrt(dx * dx + dy * dy);
    start_node.next_open = NULL;
    start_node.g = 0.0f;
    start_node.flags = 0;
    start_node.layer = (short)from_layer;
    goal_node.parent = NULL;
    goal_node.num_links = 0;
    goal_entry.parent = NULL;
    goal_entry.num_links = 1;
    goal_entry.layer = (short)to_layer;
    start_node.num_links = 0;
    start_node.links = start_links;
    if (walk) {
        start_links[0] = &goal_entry;
        start_node.num_links = 1;
    }
    for (l = 0; l < PF_MAX_LAYERS; l++) {
        g = &pf_graphs[pf_cur_size][l];
        for (i = 0; i < g->count; i++) {
            if ((g->points[i].flags & PFN_FROM_START) && start_node.num_links < 100)
                start_links[start_node.num_links++] = &g->points[i];
        }
    }
    for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
        if ((sl->flags & AVOID_SEGLIST_USED) && sl->layer == from_layer && sl->size == size) {
            for (k = 0; k < sl->num_corners; k++) {
                if ((sl->corners[k].node.flags & PFN_FROM_START) && start_node.num_links < 100)
                    start_links[start_node.num_links++] = &sl->corners[k].node;
            }
        }
    }

    for (cur = &start_node; cur; cur = cur->next_open) {
        pf_search_iterations++;
        if (cur == &goal_node) {
            p = cur->parent;
            if (!p) {
                ai_path_free(path);
                return NULL;
            }
            while (p != &start_node) {
                path = ai_path_node_create(*p->pos, p->layer, path);
                if (!path) {
                    ai_path_free(path);
                    return NULL;
                }
                p = p->parent;
            }
            return path;
        }
        pf_search_start_count = 0;
        if (cur->flags & PFN_FROM_GOAL) {
            pf_search_start_nodes[0] = &goal_entry;
            pf_search_start_count = 1;
        }
        for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
            if ((sl->flags & AVOID_SEGLIST_USED) && sl->layer == cur->layer && sl->size == size) {
                for (k = 0; k < sl->num_corners; k++) {
                    c = &sl->corners[k];
                    for (j = 0; j < c->node.num_links; j++) {
                        if (c->node.links[j] == cur) {
                            if (pf_search_start_count < 10)
                                pf_search_start_nodes[pf_search_start_count++] = &c->node;
                            break;
                        }
                    }
                }
            }
        }
        for (k = 0; k < cur->num_links + pf_search_start_count; k++) {
            if (k < cur->num_links)
                nb = cur->links[k];
            else
                nb = pf_search_start_nodes[k - cur->num_links];
            pt = *nb->pos;
            dx = cur->pos->x - pt.x;
            dy = cur->pos->y - pt.y;
            cost = (float)sqrt(dx * dx + dy * dy) + cur->g;
            if (cur != nb) {
                seg.a = cur;
                seg.b = nb;
                seg.size = size;
                if (pf_add_avoidance_nodes(&seg)) {
                    if (seg.blocked)
                        continue;
                    cost += seg.cost;
                }
            }
            if (nb->parent) {
                if (cost < nb->g) {
                    for (p = cur; p->next_open && p->next_open != nb; p = p->next_open)
                        ;
                    if (p->next_open)
                        p->next_open = p->next_open->next_open;
                    nb->parent = NULL;
                }
                if (nb->parent)
                    continue;
            }
            nb->parent = cur;
            nb->g = cost;
            dx = pt.x - to->x;
            dy = pt.y - to->z;
            cost += (float)sqrt(dx * dx + dy * dy);
            nb->f = cost;
            for (p = cur; p->next_open && p->next_open->f < cost; p = p->next_open)
                ;
            pf_search_insertions++;
            nb->next_open = p->next_open;
            p->next_open = nb;
            if (pf_debug_mode == 7)
                pf_debug_search_step(start.x, start.y, goal.x, goal.y, nb, cur, 0, 1e9f);
        }
    }
    ai_path_free(path);
    return NULL;
}

// 0x4a01d0  Confidence: exact (profile label text). Searches, smooths without Precalc_vis_method and
// stores each node's normalised x / z step from the previous position.
ai_path *ai_path_find(const vector3 *from, int from_layer, vector3 *to, int to_layer, int size, float cost_bias)
{
    profile_label label("ai_path_find");
    ai_path *path, *n;
    vector3 prev;

    timer_get(1000);
    path = pf_graph_search(from, from_layer, to, to_layer, size, cost_bias);
    if (!Precalc_vis_method)
        path = ai_path_smooth(path, from, from_layer);
    prev = *from;
    for (n = path; n; n = n->next) {
        n->dir = n->pos - prev;
        n->dir.y = 0.0f;
        n->dir.normalize_safe();
        prev = n->pos;
    }
    timer_get(1000);
    return path;
}

// 0x4a02d0  Confidence: exact. The message is formatted but not shown (no DebugError in this build).
ai_path *ai_path_alloc_node()
{
    ai_path *n;

    if (!ai_path_free_list) {
        sprintf(string_buffer, "Out of ai path nodes -- current max is %d.  Find DaveA or Allender", AI_PATH_MAX_NODES);
        return NULL;
    }
    n = ai_path_free_list;
    ai_path_free_list = ai_path_free_list->next;
    n->next = NULL;
    ai_path_used++;
    if (ai_path_used > ai_path_peak_used)
        ai_path_peak_used = ai_path_used;
    return n;
}

// 0x4a0330  Confidence: exact (VC6 byte match) (the x / z distance vector, built in the dead pos argument's slot).
ai_path *ai_path_node_create_b(vector3 pos, int layer, ai_path *next)
{
    ai_path *n = ai_path_alloc_node();

    if (!n)
        return NULL;
    n->layer = layer;
    n->pos.x = pos.x;
    n->cost = 0.0f;
    n->pos.y = pos.y;
    n->pos.z = pos.z;
    n->next = next;
    n->flags = 0;
    if (next) {
        vector3 d = vector3(next->pos.x - pos.x, 0.0f, next->pos.z - pos.z);

        n->cost += d.length_quick() + next->cost;
    }
    return n;
}

// 0x4a03c0  Confidence: exact (VC6 byte match). Like ai_path_node_create_b from an x / z point; pos.y is not written
// (it keeps the pool's stale value; NOTES.md).
ai_path *ai_path_node_create(vector2 pos, int layer, ai_path *next)
{
    ai_path *n = ai_path_alloc_node();

    if (!n)
        return NULL;
    n->layer = layer;
    n->next = next;
    n->cost = 0.0f;
    n->flags = 0;
    n->pos.x = pos.x;
    n->pos.z = pos.y;
    if (next) {
        vector3 d = vector3(next->pos.x - pos.x, 0.0f, next->pos.z - pos.y);

        n->cost += d.length_quick() + next->cost;
    }
    return n;
}

// 0x4a0440  Confidence: exact.
void ai_path_free_node(ai_path *node)
{
    node->next = ai_path_free_list;
    ai_path_free_list = node;
    ai_path_used--;
}

// 0x4a0460  Confidence: exact.
ai_path *ai_path_pop_node(ai_path *node)
{
    ai_path *next;

    if (!node)
        return NULL;
    next = node->next;
    ai_path_free_node(node);
    return next;
}

// 0x4a0480  Confidence: exact.
void ai_path_free(ai_path *path)
{
    ai_path *next;

    while (path) {
        next = path->next;
        ai_path_free_node(path);
        path = next;
    }
}

// 0x4a04a0  Confidence: exact.
int pf_node_index(const vector2 *node)
{
    return node - pf_nodes;
}
