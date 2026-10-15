// new_multichat.h - the in-game chat overlay (new_multichat.cpp, 0x41e3d0-0x41ee8f; strings.tbl ids
// 829-833): the six fading chat lines, the input line and its /msg wrapping (decomp44). The received
// lines come from multichat.cpp's ring (summoner/multi/multichat.h). Not original source.
#ifndef NEW_MULTICHAT_H
#define NEW_MULTICHAT_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"

#define CHAT_MAX_LINES  6
#define CHAT_INPUT_SIZE 64

// One shown line (0x48 bytes, chat_lines 0x5c30d8). Type and field names ours.
// kind: 0 "name text" (the name blue), 1 its continuation, 2 "<name> text" whisper (the name in the
// highlight colour), 3 its continuation, 4 a "** text **" system line (magenta).
struct chat_line {
    int  kind;                        // +0x00
    char text[0x40];                  // +0x04
    RealtimeTimer timer;              // +0x44 5 s; the line is dropped when it has elapsed
};
LAYOUT_CHECK(chat_line, sizeof(chat_line) == 0x48);

// chat_add_line types: 0 / 1 a player line, 2 / 3 a whisper, 4 a system line (the first line of a wrapped
// message gets the even kind, the rest the odd one).
#define CHAT_LINE_NORMAL  1
#define CHAT_LINE_WHISPER 3
#define CHAT_LINE_SYSTEM  4

extern chat_line chat_lines[CHAT_MAX_LINES];      // 0x5c30d8
extern RealtimeTimer chat_cursor_timer;           // 0x5c3288 500 ms cursor blink
extern int chat_input_mode;                       // 0x5c328c 1 = the input line is open
extern char chat_input_text[CHAT_INPUT_SIZE];     // 0x5c3290
extern int chat_num_lines;                        // 0x5c32d0
extern bool chat_cursor_visible;                  // 0x5c32d4
// Layout per resolution_index (.rdata 0x5800f8-0x580133; names ours).
extern const int chat_width[3];                   // 0x5800f8 {310, 388, 496} text width (lines and input)
extern const int chat_origin[3][2];               // 0x580104 bottom-left of the chat area, HUD screens
extern const int chat_origin_fullscreen[3][2];    // 0x58011c the same on the full-screen screens

void chat_reset();                                // 0x41e420 (interface_load_all)
int  chat_input_frame(int key);                   // 0x41e430 (interface_frame) the key, 0 when used
void chat_update_lines();                         // 0x41e4a0 drops elapsed lines, adds the received ones
int  chat_input_key_enter(int key);               // 0x41e590 Enter opens the input line
int  chat_input_handle_key(int key);              // 0x41e5d0
void chat_input_add_char_wrap(char c);            // 0x41e6b0
void chat_draw();                                 // 0x41e8e0 (interface_draw)
void chat_draw_lines();                           // 0x41e900
void chat_get_origin(int *x, int *y);             // 0x41eb80
void chat_draw_input();                           // 0x41ebd0
void chat_input_cancel();                         // 0x41ed50 (itemtrade_begin_gold_input; was in interface.h)
void chat_add_line(const char *text, int type);   // 0x41ed60

#endif
