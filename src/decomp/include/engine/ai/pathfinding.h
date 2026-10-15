// engine/ai/pathfinding.h - navigation mesh queries and AI paths (Engine\ai\pathfinding.cpp, the path
// reference that gives engine/ai/ its directory; pf_* / ai_path_* in sum.symbols). Not original source.
// Only what conversationctl.cpp calls; argument lists from those call sites.
#ifndef ENGINE_AI_PATHFINDING_H
#define ENGINE_AI_PATHFINDING_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

struct ai_path;                       // path node list (layout not needed)

// Moves *pos to the closest navigable point for a character of `size` (char_info +0x90) on `layer`;
// true when one was found.
bool pf_find_nearest_navigable_point(vector3 *pos, int size, int layer);   // 0x49e8c0
// cdecl, 6 arguments (add esp, 0x18 at 0x4da703): NULL when no path.
ai_path *ai_path_find(const vector3 *from, int from_layer, const vector3 *to, int to_layer, int size,
                      int flags);                                          // 0x4a01d0
void ai_path_free(ai_path *path);                                          // 0x4a0480
// Follows a move from *pos to dest across the layer-transition edges; returns the layer at dest.
// cdecl, (pos, layer, dest by value): add esp, 0x14 at 0x4da348 (unit not tagged in sum.symbols).
int  pf_trace_edge_crossings(const vector3 *pos, int layer, vector3 dest);  // 0x4a54b0

// Per character size (char_info +0x90) and layer: a list of graph points (boss_mount_pick_flee_point,
// boss_ai.cpp, decomp29: 0x60-byte records from 0x245f460, index size * 20 + layer; 3 sizes fit before
// pf_dword_pool_remaining 0x2460ae0). Type, field and table names ours; only +0x00 and +0x10 are read.
struct pf_point_ref {
    vector2 *pos;                     // +0x00 (x, z)
    ubyte _unk04[0x24];
};
LAYOUT_CHECK(pf_point_ref, sizeof(pf_point_ref) == 0x28);
struct pf_point_list {
    int count;                        // +0x00
    ubyte _unk04[0x0c];
    pf_point_ref *points;             // +0x10
    ubyte _unk14[0x4c];
};
LAYOUT_CHECK(pf_point_list, sizeof(pf_point_list) == 0x60);
#define PF_MAX_LAYERS 20
extern pf_point_list pf_point_lists[3][PF_MAX_LAYERS];   // 0x245f460 (pathfinding.cpp .bss)

#endif
