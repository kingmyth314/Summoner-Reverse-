// new_multichat.cpp - Sum.exe v1.40, code 0x41e3d0-0x41ee8f (the in-game multiplayer chat overlay)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit name: strings.tbl section new_multichat.cpp (ids 829-833, the /msg formats).
// Data: .rdata 0x5800f8-0x580133 (layout tables), .data 0x58e0c8-0x58e0cd ("**", " "), .bss
// 0x5c30d8-0x5c32d4. Pooled copies in other units: "" (0x5bd0d0), "|" (0x588c10).
// The overlay shows up to six received lines for 5 s each above the bottom-left corner of the screen, and
// the input line (Enter opens it) under them. Typing past 63 characters sends the text up to the last
// word and keeps that word in the input line; a /msg line keeps its "/msg <name>" prefix.

#include <stdio.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multichat.h"
#include "summoner/interface/new_multichat.h"

// ---- .rdata (0x5800f8-0x580133) ---------------------------------------------------------------------
const int chat_width[3] = { 310, 388, 496 };                                  // 0x5800f8
const int chat_origin[3][2] = { { 5, 370 }, { 6, 463 }, { 8, 592 } };         // 0x580104
const int chat_origin_fullscreen[3][2] = { { 5, 460 }, { 6, 575 }, { 8, 736 } };   // 0x58011c

// ---- .bss (0x5c30d8-0x5c32d4) ------------------------------------------------------------------------
// Compiler-generated dynamic initialiser pairs (thunk "jmp", initialiser), static_initializers entries
// 59 and 60:
// 0x41e3d0 init_realtimetimer_array_5c311c_thunk. Confidence: exact.
// 0x41e3e0 init_realtimetimer_array_5c311c (the 6 line timers at +0x44: chat_line's implicit constructor,
// expanded). Confidence: exact.
chat_line chat_lines[CHAT_MAX_LINES];         // 0x5c30d8
// 0x41e400 init_realtimetimer_5c3288_thunk. Confidence: exact.
// 0x41e410 init_realtimetimer_5c3288 (tail jump to RealtimeTimer::RealtimeTimer). Confidence: exact.
RealtimeTimer chat_cursor_timer;              // 0x5c3288
int chat_input_mode;                          // 0x5c328c
char chat_input_text[CHAT_INPUT_SIZE];        // 0x5c3290
int chat_num_lines;                           // 0x5c32d0
bool chat_cursor_visible;                     // 0x5c32d4

// 0x41e420  Confidence: exact.
void chat_reset()
{
    chat_num_lines = 0;
    chat_input_mode = 0;
}

// 0x41e430  Confidence: exact (switch over the input mode, `sub eax, 0` / `dec eax`). Returns the key
// unused, or what the input handlers return. Nothing is read while a message box is open.
int chat_input_frame(int key)
{
    chat_update_lines();
    if (chat_cursor_timer.elapsed()) {
        chat_cursor_timer.SetTimeout(500);
        chat_cursor_visible = !chat_cursor_visible;
    }
    if (!ui_message_box_is_active()) {
        switch (chat_input_mode) {
        case 0:
            return chat_input_key_enter(key);
        case 1:
            return chat_input_handle_key(key);
        }
    }
    return key;
}

// 0x41e4a0  Confidence: exact (the scan keeps the index of the last elapsed line). The oldest lines whose
// timers have elapsed are dropped (the scan stops at the first live one), then every queued entry of
// multichat.cpp's ring is formatted and added: "** text **" as a system line, "<name> text" as a
// whisper, the rest as a normal line.
void chat_update_lines()
{
    char buf[512];
    chat_entry *e;
    int last = -1;
    int i;

    for (i = 0; i < chat_num_lines; i++) {
        if (!chat_lines[i].timer.elapsed())
            break;
        last = i;
    }
    if (last >= 0) {
        chat_num_lines = chat_num_lines - last - 1;
        if (chat_num_lines > 0)
            memmove(chat_lines, &chat_lines[last + 1], chat_num_lines * sizeof(chat_line));
    }

    while ((e = chat_history_pop()) != NULL) {
        chat_format_entry(buf, sizeof(buf), e);
        if (strlen(buf) != 0) {
            if (strncmp(buf, "**", 2) == 0)
                chat_add_line(buf, CHAT_LINE_SYSTEM);
            else if (buf[0] == '<')
                chat_add_line(buf, CHAT_LINE_WHISPER);
            else
                chat_add_line(buf, CHAT_LINE_NORMAL);
        }
    }
}

// 0x41e590  Confidence: exact.
int chat_input_key_enter(int key)
{
    if (key == KEY_ENTER) {
        chat_input_mode = 1;
        chat_input_text[0] = '\0';
        chat_cursor_timer.SetTimeout(500);
        chat_cursor_visible = true;
        return 0;
    }
    return key;
}

// 0x41e5d0  Confidence: likely (the switch: dec / sub 0xd / sub 0xe). Enter sends the line without its
// trailing blanks and leaves the input line open and empty; Enter on an empty line and Esc close it.
int chat_input_handle_key(int key)
{
    char line[CHAT_INPUT_SIZE];
    int c;
    int len;

    if (key == 0)
        return key;
    {
        c = key_to_ascii(key);
        if (c != 0xff) {
            chat_input_add_char_wrap((char)c);
            chat_cursor_timer.SetTimeout(500);
            chat_cursor_visible = true;
            return 0;
        }
        switch (key) {
        case KEY_ENTER:
            if (strlen(chat_input_text) == 0) {
                chat_input_cancel();
                return 0;
            }
            trim_trailing_whitespace(line, chat_input_text);
            if (strlen(line) != 0)
                chat_send_line(line);
            chat_input_text[0] = '\0';
            return 0;
        case KEY_ESC:
            chat_input_cancel();
            return 0;
        case KEY_BACKSPACE:
            len = strlen(chat_input_text);
            if (len > 0) {
                chat_input_text[len - 1] = '\0';
                chat_cursor_timer.SetTimeout(500);
                chat_cursor_visible = true;
                return 0;
            }
            break;
        }
    }
    return key;
}

// 0x41e6b0  Confidence: likely (the order of the /msg branches). Appends c; when the line already holds
// 63 characters it is sent first, up to its last word, and the last word plus c stay in the input. For
// "/msg <name> ..." the kept part gets the "/msg <name> " prefix back (L830 / L831 / L833) and the sent
// part is rebuilt with L832. A NUL c is appended as it is.
void chat_input_add_char_wrap(char c)
{
    char trimmed[CHAT_INPUT_SIZE];
    char copy[CHAT_INPUT_SIZE];
    int len = strlen(chat_input_text);
    char *sp;

    if (c == '\0' || len < CHAT_INPUT_SIZE - 1) {
        chat_input_text[len] = c;
        chat_input_text[len + 1] = '\0';
        return;
    }

    trim_whitespace(trimmed, chat_input_text, -1);
    if (strlen(trimmed) == 0) {
        chat_input_text[0] = c;
        chat_input_text[1] = '\0';
        return;
    }

    strcpy(copy, trimmed);
    if (_stricmp(strtok(copy, " "), XSTR("/msg", 829)) == 0) {
        char *name = strtok(NULL, " ");
        char *rest;

        if (name == NULL) {
            sprintf(chat_input_text, XSTR("/msg %c", 833), c);
            return;
        }
        rest = strtok(NULL, "");
        if (rest == NULL) {
            sprintf(chat_input_text, XSTR("/msg %s %c", 831), name, c);
            return;
        }
        sp = strrchr(rest, ' ');
        if (sp != NULL) {
            *sp = '\0';
            sprintf(chat_input_text, XSTR("/msg %s %s%c", 830), name, sp + 1, c);
        } else {
            sprintf(chat_input_text, XSTR("/msg %s %c", 831), name, c);
        }
        sprintf(trimmed, XSTR("/msg %s %s", 832), name, rest);
        chat_send_line(trimmed);
        return;
    }

    sp = strrchr(trimmed, ' ');
    if (sp != NULL) {
        strcpy(chat_input_text, sp + 1);
        len = strlen(chat_input_text);
        *sp = '\0';
    } else {
        len = 0;
    }
    chat_send_line(trimmed);
    chat_input_text[len] = c;
    chat_input_text[len + 1] = '\0';
}

// 0x41e8e0  Confidence: exact (tail jump to chat_draw_input).
void chat_draw()
{
    chat_draw_lines();
    if (chat_input_mode == 1)
        chat_draw_input();
}

// 0x41e900  Confidence: likely (the colour choice of kinds 1 / 3 / 4 / others is one switch whose
// targets share the gr_set_color_ptr call). Lines are stacked upwards from the origin, one line height
// higher while the input line is open. In kinds 0 and 2 the text up to and including the first space
// (the "[name]" / "<name>" prefix) is drawn in blue / the highlight colour; a line without a space is
// all blue, also for a whisper.
void chat_draw_lines()
{
    int y, x;
    int w, h;
    int i;
    int line_h;
    char name[64];

    if (chat_num_lines <= 0)
        return;

    line_h = gr_get_font_height(-1);
    chat_get_origin(&x, &y);
    y -= chat_num_lines * line_h;
    if (chat_input_mode == 0)
        y += line_h;

    for (i = 0; i < chat_num_lines; i++) {
        chat_line *line = &chat_lines[i];
        char *sp;
        int len;
        Color *color;

        if (line->kind == 0) {
            sp = strchr(line->text, ' ');
            if (sp != NULL) {
                len = sp - line->text + 1;
                strncpy(name, line->text, len);
                name[len] = '\0';
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + len, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else if (line->kind == 2) {
            sp = strchr(line->text, ' ');
            if (sp != NULL) {
                len = sp - line->text + 1;
                strncpy(name, line->text, len);
                name[len] = '\0';
                gr_get_string_size(&w, &h, name, -1, -1);
                gr_set_color_ptr(&ui_color_highlight);
                gr_string(x, y, name, -1, gr_string_mode);
                gr_set_color_ptr(&ui_color_text);
                gr_string(x + w, y, line->text + len, -1, gr_string_mode);
            } else {
                gr_set_color_ptr(&ui_color_blue);
                gr_string(x, y, line->text, -1, gr_string_mode);
            }
        } else {
            switch (line->kind) {
            case 1:
            case 3:
            default:
                color = &ui_color_text;
                break;
            case 4:
                color = &ui_color_magenta;
                break;
            }
            gr_set_color_ptr(color);
            gr_string(x, y, line->text, -1, gr_string_mode);
        }
        y += line_h;
    }
}

// 0x41eb80  Confidence: exact.
void chat_get_origin(int *x, int *y)
{
    if (interface_screen_is_fullscreen()) {
        *x = chat_origin_fullscreen[resolution_index][0];
        *y = chat_origin_fullscreen[resolution_index][1];
    } else {
        *x = chat_origin[resolution_index][0];
        *y = chat_origin[resolution_index][1];
    }
}

// 0x41ebd0  Confidence: likely (the clamp `offset > 0 ? 0 : offset` is setg / dec / and; the buffer size
// is a guess, 0x44 bytes in the frame). A text box at the origin, the text clipped to it and scrolled
// left once it is wider; "|" is the blinking cursor.
void chat_draw_input()
{
    int x, h, y;
    int clip_y, clip_x, w, clip_h, clip_w;
    int offset;
    char buf[68];

    strcpy(buf, chat_input_text);
    strcat(buf, "|");
    gr_get_string_size(&w, &h, buf, -1, -1);
    chat_get_origin(&x, &y);
    gr_set_color_ptr(&ui_color_text_box_bg);
    gr_rect(x, y, chat_width[resolution_index], h, gr_rect_mode, 1.0f);
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr::set_clip(clip_x + x, clip_y + y, chat_width[resolution_index], h);
    gr_set_color_ptr(&ui_color_text);
    offset = chat_width[resolution_index] - w;
    if (offset > 0)
        offset = 0;
    gr_string(offset, 0, chat_cursor_visible ? buf : chat_input_text, -1, gr_string_mode);
    gr::set_clip(clip_x, clip_y, clip_w, clip_h);
}

// 0x41ed50  Confidence: exact.
void chat_input_cancel()
{
    chat_input_mode = 0;
}

// 0x41ed60  Confidence: exact (switch 0x41ee7c inside the loop; the line pointer is computed before
// it). Wraps the text to the chat width (at most 16 pieces) and drops it whole when a piece is longer
// than 63 characters. Older lines are scrolled out to make room. A message of more than six pieces
// would make the scroll size negative and write past chat_lines (the received text is up to 255
// characters; see the decomp44 NOTES).
void chat_add_line(const char *text, int type)
{
    int lengths[16];
    int offsets[16];
    int n;
    int i;
    int excess;

    n = gr_split_str(lengths, offsets, text, chat_width[resolution_index], 16, 0, -1);
    for (i = 0; i < n; i++) {
        if (lengths[i] > 63)
            return;
    }

    excess = chat_num_lines + n - CHAT_MAX_LINES;
    if (excess > 0) {
        memmove(chat_lines, &chat_lines[excess], (chat_num_lines - excess) * sizeof(chat_line));
        chat_num_lines -= excess;
    }

    for (i = 0; i < n; i++) {
        chat_line *line = &chat_lines[chat_num_lines];
        switch (type) {
        case 0:
        case 1:
            line->kind = i != 0;
            break;
        case 2:
        case 3:
            line->kind = (i != 0) + 2;
            break;
        case 4:
            line->kind = 4;
            break;
        default:
            return;
        }
        strncpy(line->text, text + offsets[i], lengths[i]);
        line->text[lengths[i]] = '\0';
        line->timer.SetTimeout(5000);
        chat_num_lines++;
    }
}
