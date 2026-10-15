// engine/ai/unnamed_ai_move_0x4b1410.h - the AI movement unit: path following, steering around walls
// and characters, turning, and the traveling / cast / disabled / hiding / sneaking mode handlers (code
// 0x4b1410-0x4b513f; source game/unnamed_ai_move_0x4b1410.cpp). File and directory names are
// placeholders (no path string, no strings.tbl block). The ai_state members are declared in
// engine/ai/ai.h (which includes this header). Not original source.
#ifndef ENGINE_AI_UNNAMED_AI_MOVE_0X4B1410_H
#define ENGINE_AI_UNNAMED_AI_MOVE_0X4B1410_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/console.h"
#include "engine/ai/pathfinding.h"

class Character;
class obj;

// ---- .data 0x59d6e4-0x59d713 --------------------------------------------------------------------------
extern float turn_scaler;                             // 0x59d6e4 2.0 ("turn_scaler"; multiplies the turn rate
                                                      //          while ai_state flag 0x10000000 is set)
// The speed factor of ai_follow_path by the cosine of the next turn: index (int)(cos * 5 + 5.5), the
// values sqrt(i / 10) (name ours).
extern float ai_turn_speed_factors[11];               // 0x59d6e8

// ---- .bss 0x247e840-0x247e8c3 ---------------------------------------------------------------------------
// ai_mode_frame_counts (0x247e840) and ai_follow_target (0x247e878) are declared in engine/ai/ai.h.
extern float ai_path_remaining;                       // 0x247e874 distance to the next node + the path's
                                                      //           cost from there (ai_follow_path; name ours)
extern console_command ccmd_turn_scaler;              // 0x247e880
extern console_command ccmd_Turn_anims;               // 0x247e890
// The nearest wall crossing of ai_find_nearest_wall_hit (names ours).
extern float ai_wall_hit_dist;                        // 0x247e8a0 squared distance (1e9 = none)
extern pf_edge *ai_wall_hit_edge;                     // 0x247e8a4 the nav wall, NULL for an obstacle edge
extern int ai_wall_hit_layer;                         // 0x247e8a8
extern bool ai_wall_hit_obstacle;                     // 0x247e8ac the hit is an obstacle outline edge
extern vector2 *ai_wall_hit_a;                        // 0x247e8b0 the obstacle edge's points
extern vector2 *ai_wall_hit_b;                        // 0x247e8b4
extern bool Use_turn_anims;                           // 0x247e8c0 ("Turn_anims"; turn moves 0xa5 / 0xa6)

void cmd_Turn_anims();                                // 0x4b1440
void cmd_turn_scaler();                               // 0x4b1530
// (from, to, direction (x / z read), size class, layer, test the prohibit obstacles too): true when the
// move crosses a nav wall facing it or an obstacle outline; the nearest crossing is in ai_wall_hit_*.
bool ai_find_nearest_wall_hit(vector2 *from, vector2 *to, vector3 *dir, int size, int layer,
                              bool obstacles);        // 0x4b15a0
// (position, move, size class, layer, obstacles): splits the move at the layer-transition edges and
// probes every piece with ai_find_nearest_wall_hit.
bool ai_probe_move_for_walls(vector3 *pos, vector3 *move, int size, int layer, bool obstacles);   // 0x4b18e0
// (velocity of a, velocity of b, a, b): the time of the closest approach, 0 when they do not move apart.
float closest_approach_time(vector3 *va, vector3 *vb, obj *a, obj *b);   // 0x4b1a00

#endif
