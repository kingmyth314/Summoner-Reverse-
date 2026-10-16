// unnamed_input_0x4343a0.cpp - Sum.exe v1.40, code 0x4343a0-0x4348f6: the "slew" console command, the game
// state's input poll, the mouse input record (player_input) and the camera keys / mouse turn.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp52, NOTES.md): player.cpp ends with its last RET at 0x43439f; this unit starts with
// the static initialiser thunk of ccmd_slew (static_initializers entry 110) and ends with slew_set_enabled
// (0x4348b0-0x4348f6). The optimised initialisers 0x434900-0x4349a5 that follow (entries 111-113) construct
// the cutscene object, the 85 cutscene entries and the cutscene pool: those objects sit inside the cutscene
// file's .bss (0x5fccc0-0x609f3b, interleaved with variables only the unoptimised cutscene code uses), so
// they are that file's (game/unnamed_cutscene_0x434900.cpp, D3201), not this one's. Whether this unit and
// the cutscene file are one translation unit is open (both are adjacent in .text, .data and .bss).
// Data: .data 0x593948-0x593999 (the five literals), .bss 0x5fcca8-0x5fccb8 (player_input 0x5fcc98 is
// player.cpp's, as player.cpp has it).

#include "vsdk/os/console.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/profile.h"
#include "vsdk/gr/gr.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/interface.h"
#include "engine/camera/camera.h"
#include "levelscripts/level_scripts.h"

// DirectInput scan codes this unit reads (vsdk/os/key.h has no names for them).
#define DIK_LCONTROL 0x1d
#define DIK_GRAVE    0x29
#define DIK_LSHIFT   0x2a
#define DIK_RSHIFT   0x36
#define DIK_LMENU    0x38
#define DIK_RCONTROL 0x9d
#define DIK_RMENU    0xb8
#define DIK_UP       0xc8
#define DIK_LEFT     0xcb
#define DIK_RIGHT    0xcd
#define DIK_DOWN     0xd0

// Slew mode is on while the camera's follow bit (camera +0xc bit 0) is clear (slew_set_enabled(1) sets the
// follow flags (false, true)). An inline of the original's (name ours): both tests are `not al; test al, 1`.
static inline bool slew_mode_on() { return !(game_camera.flags & 1); }

// ---- .bss (0x5fcca8-0x5fccb8) ----------------------------------------------------------------------
// 0x4343a0 init_ccmd_slew_thunk (jmp; static_initializers entry 110). Confidence: exact.
// 0x4343b0 init_ccmd_slew (console_command::console_command(0x5fcca8, "slew", "Toggles Slew Mode", cmd_slew)).
//          Confidence: exact.
console_command ccmd_slew("slew", "Toggles Slew Mode", cmd_slew);   // 0x5fcca8
bool combat_mode;                     // 0x5fccb4 (declared in player.h; the hud and gameplay.cpp use it)
bool player_mouse_held;               // 0x5fccb6
bool camera_edge_mouse_held;          // 0x5fccb7 (name ours)
bool camera_edge_turn_blocked;        // 0x5fccb8 (name ours)

// 0x4343d0  Confidence: likely (the two `not al; test al, 1` tests are written as slew_mode_on()).
void cmd_slew()
{
    if (calling_a_command_function)
        slew_set_enabled(!slew_mode_on());
    if (checking_status_for_command)
        print_to_console(slew_mode_on() ? "Slew mode is on\n" : "Slew mode is off\n", 0);
    if (doing_help_for_comand)
        print_to_console("Usage: slew\n", 0);
}

// 0x434440  Confidence: exact. Polls until no key is left; the interface sees the keys in single player,
// in a conversation (game_flags 0x101) and in multiplayer once the local player's state is 5 or more.
void input_drain_events()
{
    int key;

    do {
        key = PollInputEvents();
        if ((game_flags & (GF_SINGLE_PLAYER | GF_CONVERSATION))
            || (mp_local_player != NULL && mp_local_player->type >= 5))
            key = interface_frame(key);
    } while (key != 0);
}

// 0x434470  Confidence: exact (camera_frame is a tail jump).
void player_input_frame()
{
    if (game_flags & GF_NO_INPUT)
        return;
    input_drain_events();
    if (interface_screen_is_fullscreen())
        return;
    player_read_mouse_input(player_input);
    camera_process_input(player_input);
    game_camera.frame();
}

// 0x4344b0  Confidence: likely (the profile_label text is ours: the inline constructor leaves only the EH
// state store, the unwind funclet 0x57c020 calls ~profile_label at 0x434650, as in pathfinding.cpp).
// in = {flags, mouse x, mouse y}. The left click ASSIGNS 4, so the Shift, Alt and right-button bits set
// above are dropped on the frame of a left click (observed; whether any reader needs them then is open).
void player_read_mouse_input(unsigned int *in)
{
    profile_label label("player_read_mouse_input");
    bool over_panel;

    if ((game_flags & (GF_CONVERSATION | GF_PARTY_DEAD)) || ui_message_box_is_active())
        return;
    in[0] = 0;
    in[1] = 0;
    in[2] = 0;
    in[1] = game_mouse_x;
    in[2] = game_mouse_y;
    if (key_is_down(DIK_LSHIFT) || key_is_down(DIK_RSHIFT))
        in[0] |= PI_SHIFT;
    if (key_is_down(DIK_LMENU) || key_is_down(DIK_RMENU))
        in[0] |= PI_ALT;
    over_panel = interface_mouse_over_panel();
    if (!over_panel) {
        if (mouse_was_pressed(1))
            in[0] |= PI_RIGHT_PRESSED;
        if (mouse_was_released(1))
            in[0] |= PI_RIGHT_RELEASED;
    }
    if (player_mouse_held && mouse_is_down(0)) {
        if (mouse_time_since_press(0) >= 400)
            in[0] |= PI_LEFT_HELD;
    } else {
        move_target_decal_visible = true;
        player_mouse_held = false;
    }
    if (!over_panel && mouse_was_pressed(0)) {
        player_mouse_held = true;
        in[0] = PI_LEFT_CLICK;
        if (key_is_down(DIK_LCONTROL) || key_is_down(DIK_RCONTROL))
            in[0] |= PI_CTRL_CLICK;
        if (key_is_down(DIK_GRAVE)) {
            if (key_is_down(DIK_LSHIFT))
                in[0] |= PI_GRAVE_SHIFT;
            else
                in[0] |= PI_GRAVE;
        }
        move_target_decal_visible = false;
    }
}

// 0x434650 profile_label::~profile_label (vsdk/os/profile.h): the inline destructor's COMDAT, emitted in
// this unit after its first user and kept by the linker for all 91 callers (identical bare RETs folded).
// Compiler-generated from the header; nothing to write. Confidence: exact.

// 0x434660  Confidence: likely (local order; the dead edge test below is reproduced as the code has it).
// Left / Right (and the screen edges) turn, Up / Down and the wheel zoom; on the world map (level 4) only
// the turn. GAME BUG (latent, NOTES.md B1): camera_edge_mouse_held is set from PI_LEFT_HELD just before the test
// `camera_edge_mouse_held && !(in[0] & PI_LEFT_HELD)`, which can never be true, so
// camera_edge_turn_blocked is never set and the edge turn is never suppressed after a held drag.
void camera_process_input(unsigned int *in)
{
    float yaw;
    float zoom;
    int dx, dy, dz;

    if (game_flags & GF_CONVERSATION)
        return;
    if ((game_flags & 0x80) && !script_editor_no_window_open())
        return;
    zoom = 0;
    if (cur_level_index == 4) {
        yaw = camera_key_yaw();
    } else {
        yaw = camera_key_yaw();
        zoom = camera_key_zoom();
    }
    if (config_invert_camera)
        yaw = -yaw;
    camera_edge_mouse_held = (in[0] & PI_LEFT_HELD) != 0;
    if (game_mouse_x < 3 || game_mouse_x > gr_screen_width() - 4) {
        if (camera_edge_mouse_held && !(in[0] & PI_LEFT_HELD)) {
            camera_edge_mouse_held = false;
            camera_edge_turn_blocked = true;
        } else if (!camera_edge_turn_blocked && !mouse_is_down(0) && !mouse_is_down(1)) {
            if (game_mouse_x < 3)
                yaw -= 1.0f;
            else
                yaw += 1.0f;
        }
    }
    int screen = interface_get_screen();
    if (screen != 4 && (screen <= 0x12 || screen > 0x14)) {
        mouse_get_delta(&dx, &dy, &dz);
        if (dz > 0)
            zoom -= 10.0f;
        else if (dz < 0)
            zoom += 10.0f;
    }
    if (camera_edge_turn_blocked == true && game_mouse_x > 2 && game_mouse_x < gr_screen_width() - 3)
        camera_edge_turn_blocked = false;
    int extra = camera_key_extra_axis();
    game_camera.set_input(config_camera_speed * yaw, config_camera_speed * zoom,
                          config_camera_speed * extra);
}

// 0x434840  Confidence: exact. key_down_time(Left) - key_down_time(Right), truncated (_ftol).
int camera_key_yaw()
{
    return (int)(key_down_time(DIK_LEFT) - key_down_time(DIK_RIGHT));
}

// 0x434870  Confidence: exact. key_down_time(Down) - key_down_time(Up): the zoom rate (camera +0x240).
int camera_key_zoom()
{
    return (int)(key_down_time(DIK_DOWN) - key_down_time(DIK_UP));
}

// 0x4348a0  Confidence: exact. The third camera axis (camera +0x244); no key drives it.
int camera_key_extra_axis()
{
    return 0;
}

// 0x4348b0  Confidence: exact (camera_pop_state is a tail jump).
void slew_set_enabled(int on)
{
    if (on) {
        movement_init(10.0f, 1.0f, 0.1f, 0.1f);
        game_camera.push_state();
        game_camera.set_follow_flags(false, true);
    } else {
        game_camera.pop_state();
    }
}
