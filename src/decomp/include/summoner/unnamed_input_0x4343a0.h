// summoner/unnamed_input_0x4343a0.h - the player input unit (code 0x4343a0-0x4348f6, .data 0x593948-0x593999,
// .bss 0x5fcca8-0x5fccb8; source game/unnamed_input_0x4343a0.cpp, decomp52): the slew console command, the
// input poll of the game state, the mouse input record and the camera keys. THE FILE NAME IS A
// PLACEHOLDER (no path string, no strings.tbl block, sum.symbols module "-"); the directory is ours too.
// Not original source. Included by summoner/summoner.h.
#ifndef SUMMONER_UNNAMED_INPUT_0X4343A0_H
#define SUMMONER_UNNAMED_INPUT_0X4343A0_H

#include "vsdk/vsdk.h"
#include "vsdk/os/console.h"

// player_input[0] bits (player_read_mouse_input; names ours).
#define PI_RIGHT_PRESSED   0x01
#define PI_RIGHT_RELEASED  0x02
#define PI_LEFT_CLICK      0x04
#define PI_CTRL_CLICK      0x08
#define PI_LEFT_HELD       0x10       // the left button held 400 ms or more
#define PI_SHIFT           0x20
#define PI_ALT             0x40
#define PI_GRAVE_SHIFT     0x40000000 // left click with ` and Shift held
#define PI_GRAVE           0x80000000 // left click with ` held

extern console_command ccmd_slew;     // 0x5fcca8
// combat_mode (0x5fccb4) is defined here too; it is declared in summoner/player/player.h.
extern bool player_mouse_held;        // 0x5fccb6
extern bool camera_edge_mouse_held;   // 0x5fccb7 (name ours) PI_LEFT_HELD of this frame
extern bool camera_edge_turn_blocked; // 0x5fccb8 (name ours) never set: see camera_process_input

void cmd_slew();                      // 0x4343d0
void input_drain_events();            // 0x434440
void player_input_frame();            // 0x434470 (state_game_frame)
void player_read_mouse_input(unsigned int *in);   // 0x4344b0
void camera_process_input(unsigned int *in);      // 0x434660
int  camera_key_yaw();                // 0x434840
int  camera_key_zoom();               // 0x434870 (formerly camera_key_pitch)
int  camera_key_extra_axis();         // 0x4348a0 returns 0
void slew_set_enabled(int on);        // 0x4348b0

// ---- other units ------------------------------------------------------------------------------------
// (max velocity, max rotation velocity, a3, a4): the free-movement (slew) limits 0x2cbff58.. (vsdk, unit
// not known).
void movement_init(float max_vel, float max_rotvel, float a3, float a4);   // 0x531ed0

#endif
