// summoner/gametarget.h - gametarget.cpp (0x43a330-0x43cd6d: the world mouse: picking, hover tints, clicks
// on characters, items, containers and doors, the click move, chain attack selection; strings.tbl ids
// 623-638 and 213-223). File name from the module column of sum.symbols; the directory follows
// summoner.h, which includes this header (decomp35; the block was in summoner.h before). Not original
// source.
#ifndef SUMMONER_GAMETARGET_H
#define SUMMONER_GAMETARGET_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"

class Character;
class obj;
struct lobj;

// world_mouse_hover_target's result: what a click does (the switch of world_mouse_frame). Names ours.
#define WORLD_CLICK_NONE        0       // the click goes on to the move
#define WORLD_CLICK_SELECT      1       // a party member: party_select_member
#define WORLD_CLICK_TALK        2       // player_talk_to (single player)
#define WORLD_CLICK_ATTACK      3       // party_order_attack
#define WORLD_CLICK_AT_POINT    4       // the pending skill / item on the ground point
#define WORLD_CLICK_AT_TARGET   5       // the pending skill / item on the character
#define WORLD_CLICK_GOLD        6       // pick up a gold pile (item object, type 3)
#define WORLD_CLICK_CONTAINER   7       // open a container object (type 11)
#define WORLD_CLICK_DOOR        8       // object_try_unlock_or_open on the level object
#define WORLD_CLICK_EXAMINE     9       // container_examine on the level object
#define WORLD_CLICK_PICK_LOCK   10      // character_use_skill(SKILL_PICK_LOCK)
#define WORLD_CLICK_TRADE       11      // another player's character: mp_send_it_rqst
#define WORLD_CLICK_SHOP        12      // an NPC in a network game: the shop screen (7)

// ---- .bss 0x60ab60-0x60abab ------------------------------------------------------------------------
extern GameTimer gold_highlight_timer;         // 0x60ab60 5 s (hud_set_timed_highlight)
extern bool  world_pick_portrait_clicked;      // 0x60ab68
extern Character *world_hover_portrait_character;   // 0x60ab6c (formerly world_pick_target_mode; D47)
extern GameTimer chain_attack_timer;           // 0x60ab70 1 s between chain attack picks
extern int   world_interact_kind;              // 0x60ab88 1..4 (hud_interact_nearest, object_use_container)
extern void *world_hover_object;               // 0x60ab8c
extern void *world_interact_object;            // 0x60ab90
extern int   gold_highlight_handle;            // 0x60ab9c
extern bool  world_click_handled;              // 0x60aba0
extern const char *world_pick_filter;          // 0x60aba4 surface filter of world_pick_ground_point, NULL = default
extern int   world_pick_lobj_counter;          // 0x60aba8 world_pick_lobj runs every third frame
// ---- .data 0x5943f0-0x594437 ------------------------------------------------------------------------
extern const char world_pick_default_filter[8];// 0x5943f0 by surface: 1 = a valid ground point
extern const char world_pick_skill26_filter[5];// 0x5943f8 the filter for skill 0x26 (name ours)
extern bool  world_mouse_auto_turn;            // 0x5943fd 1: a held click turns the camera (name ours)
extern float world_tint_enemy[3];              // 0x594400 {1, 0.5, 0.5} (names of the four ours)
extern float world_tint_party[3];              // 0x59440c {0.75, 1, 0.75}
extern float world_hover_tint[3];              // 0x594418 {0.75, 1, 1} (sum.symbols world_hover_tint_r/_g/_b)
extern float world_tint_gold[3];               // 0x594424 {0.98, 1, 0.51}
// Outside the unit (module "-"): the character under the mouse for a held right click (.data, -1), and
// the last ground point clicked (.bss before game_camera). Names: sum.symbols / ours.
extern int   world_mouse_target_handle;        // 0x593f50
extern vector3 world_last_click_pos;           // 0x609ff8 (name ours)

// ---- functions --------------------------------------------------------------------------------------
void world_pick_set_skill_filter(int skill);   // 0x43a370
void world_pick_clear_filter();                // 0x43a390
void party_order_attack(Character *target);    // 0x43a3a0
int  character_can_talk_to(Character *npc);    // 0x43a460 1 / 0
void world_tint_gold_object();                 // 0x43a600
void world_hover_tint_object(obj *o, float r, float g, float b);   // 0x43a650 (o may be NULL)
void object_open_pickup(obj *o);               // 0x43a6e0 the pickup screen on a container (return unused)
// (ray origin, direction, out hit point, out face index): the level object hit first, -1 none.
int  world_pick_lobj(vector3 *origin, vector3 *dir, vector3 *hit, int *face);   // 0x43a770
void world_pick_reset();                       // 0x43a940 (level_load)
// Empty; world_mouse_frame calls it with (level object, face). Folded into the bare ret 0x40acc0 (name ours).
void world_pick_debug_hook(lobj *l, int face);
// (player_input {flags, x, y}, camera_view_pos), every frame of state_game_frame.
void world_mouse_frame(unsigned int *input, vector3 *eye);   // 0x43a960
// The level object variant of world_hover_tint_object (hover_tint_set_character tints a level object).
void world_hover_tint_character(lobj *l, float r, float g, float b);   // 0x43b1d0
void world_pick_under_mouse(vector3 *eye, vector3 *dir, vector3 *out, unsigned int *input);   // 0x43b260
int  world_mouse_hover_target(unsigned int *input, lobj *l, lobj *door, int face, vector3 *hit,
                              obj *picked, Character *target, bool *valid);   // 0x43b5e0 WORLD_CLICK_*
// (cursor bitmap when valid, when not, level object, face, point, check range / navigability).
bool world_pick_ground_point(int ok_cursor, int bad_cursor, lobj *l, int face, vector3 *pos,
                             bool check);      // 0x43bba0
void world_click_move(unsigned int *input, vector3 *pos, lobj *l);   // 0x43be80
void object_try_unlock_or_open(lobj *l);       // 0x43c060
void container_examine(lobj *l);               // 0x43c420
void player_talk_to(Character *npc);           // 0x43c5a0
obj *pick_test_character(vector3 *hit);        // 0x43c620 the living, item or container object on the ray
void chain_attack_select(unsigned int *input); // 0x43c830
void object_use_container();                   // 0x43cb60 world_interact_kind's action
void hud_set_timed_highlight(int handle);      // 0x43cd50 (monster_drop_loot)

#endif
