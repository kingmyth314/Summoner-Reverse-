// unnamed_gameseq_0x401000.cpp - Sum.exe v1.40, code 0x401000-0x4011ad: the game state sequencer
// (gameseq_init .. GameStateSwap) and GameTick, the main loop's one frame.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"). It is the first unit of the executable; new_assess.cpp's
// static initialiser follows at 0x4011c0. Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Data: .data 0x588a70-0x588b47: gameseq_state_table (15 x 0xc), then the one literal ("Packet type %d
// created %d time(s)\n", 0x588b24); new_assess.cpp's .data follows (assess_scrollbar_track 0x588b50).
// .bss 0x5b27e0-0x5b27ec: the four state variables.

#include <stdio.h>
#include "vsdk/os/console.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/frametime.h"
#include "engine/effects/effect_stream.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"
#include "summoner/multi/unnamed_mpmenus_0x43eb00.h"
#include "summoner/multi/unnamed_lobby_0x43ef30.h"
#include "summoner/unnamed_mlayout_0x43f9d0.h"

// ---- .data (0x588a70) ----------------------------------------------------------------------------------
// {enter, frame, leave} per state (main-loop.md). States 0 and 3 (quit) are empty; the title, credits and
// three multiplayer menus leave through the shared bare ret (empty_function).
GameStateHandlers gameseq_state_table[15] = {   // 0x588a70
    { 0, 0, 0 },
    { state_boot_enter, state_boot_frame, state_boot_leave },
    { state_title_enter, state_title_frame, empty_function },
    { 0, 0, 0 },
    { state_game_enter, state_game_frame, state_game_leave },
    { state_help_enter, state_help_frame, state_help_leave },
    { state6_enter, state6_frame, state6_leave },
    { state_debug_help_enter, state7_frame, state7_leave },
    { state_loadgame_enter, state_loadgame_frame, state_loadgame_leave },
    { state_credits_enter, state_credits_frame, empty_function },
    { state_mp_charselect_enter, state_mp_menu_frame, empty_function },
    { state_mp_lobby_enter, state_mp_lobby_frame, state_mp_lobby_leave },
    { state_mp_gamelist_enter, state_mp_gamelist_frame, state_mp_gamelist_leave },
    { state_mp_gamecreate_enter, state_mp_menu_frame, empty_function },
    { state14_enter, state14_frame, state14_leave },
};

// ---- .bss (0x5b27e0) -----------------------------------------------------------------------------------
int  gameseq_state_cur;               // 0x5b27e0
int  gameseq_state_prev;              // 0x5b27e4
int  gameseq_state_next;              // 0x5b27e8
bool gameseq_state_pending;           // 0x5b27ec

// ---- code ----------------------------------------------------------------------------------------------

// 0x401000  Confidence: exact.
void gameseq_init()
{
    gameseq_state_cur = 0;
    gameseq_state_prev = 0;
    gameseq_state_next = 0;
}

// 0x401020  Confidence: exact. The switch happens at the start of the next GameTick.
void gameseq_set_state(int state)
{
    gameseq_state_pending = true;
    gameseq_state_next = state;
}

// 0x401040  Confidence: exact.
int gameseq_get_state()
{
    return gameseq_state_cur;
}

// 0x401050  Confidence: exact.
int gameseq_get_prev_state()
{
    return gameseq_state_prev;
}

// 0x401060  Confidence: exact.
int gameseq_get_next_state()
{
    return gameseq_state_next;
}

// 0x401070  Confidence: exact (VC6 bytes match; the `== true` test of game_frozen: `cmp byte ptr [0x2491a5c], 1`). One
// frame: pending state switches (leave, swap, enter; an enter may queue another switch), the frame time,
// the mouse, the multiplayer frame, the effect stream, the state's frame handler, the editor dialogs and
// FrameEnd. In multiplayer the packet send counters are zeroed before multi_do_frame and formatted into
// string_buffer afterwards: a debug print with its output call compiled out (D5501).
int GameTick()
{
    int i;

    while (gameseq_state_pending) {
        if (gameseq_state_table[gameseq_state_cur].leave)
            gameseq_state_table[gameseq_state_cur].leave();
        GameStateSwap();
        ClearFrameFlag5bd77c();
        ClearFrameFlag5bd504();
        if (gameseq_state_table[gameseq_state_cur].enter)
            gameseq_state_table[gameseq_state_cur].enter();
    }
    if (game_frozen == true)
        reported_frametime = 0.0f;
    else
        FrameDone();
    mouse_update();
    mouse_get_pos(&game_mouse_x, &game_mouse_y, &game_mouse_z);
    if (game_flags & GF_MULTIPLAYER) {
        for (i = 0; i < MP_NUM_PACKET_TYPES; i++)
            mp_packet_table[i].sent = 0;
        multi_do_frame();
    }
    if (effect_stream_state == 1)
        effect_stream_update();
    if (gameseq_state_table[gameseq_state_cur].frame)
        gameseq_state_table[gameseq_state_cur].frame();
    if (game_flags & GF_MULTIPLAYER) {
        for (i = 0; i < MP_NUM_PACKET_TYPES; i++)
            if (mp_packet_table[i].sent > 0)
                sprintf(string_buffer, "Packet type %d created %d time(s)\n", i, mp_packet_table[i].sent);
    }
    editor_dialogs_free_closed();
    FrameEnd();
    return gameseq_state_cur;
}

// 0x401190  Confidence: exact.
void GameStateSwap()
{
    gameseq_state_prev = gameseq_state_cur;
    gameseq_state_cur = gameseq_state_next;
    gameseq_state_pending = false;
}
