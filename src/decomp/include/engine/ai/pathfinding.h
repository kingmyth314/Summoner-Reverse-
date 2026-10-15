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

#endif
