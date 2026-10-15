// engine/ai/pathfinding_visibility.h - point visibility over the nav graph (engine\ai\pathfinding_visibility.cpp,
// assert path 0x4b7d68; 0x4b77f0-0x4b8790). Not original source. Only what pathfinding.cpp calls (decomp32).
#ifndef ENGINE_AI_PATHFINDING_VISIBILITY_H
#define ENGINE_AI_PATHFINDING_VISIBILITY_H

#include "vsdk/vsdk.h"

class vector2;

// Marks the graph nodes of the current size class that the point sees (PFN_VISIBLE), with the
// edge-list sweep; prints "determine_point_visibility() took %d ms.".
void determine_point_visibility(const vector2 *pt, int layer);   // 0x4b8350

#endif
