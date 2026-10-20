// new_dialogue.cpp - Sum.exe v1.40, code 0x408d80-0x4099ac (screen 13, the conversation panel)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact / likely / unsure (see new_playmenu.cpp header).
// Data: .rdata 0x57eb88-0x57ec20 (position tables, arrow glyphs), .data 0x58a158-0x58a1dd (button table,
// "more"), .bss 0x5bc418-0x5bc490. The text and topic boxes are defined elsewhere (dialogue_box.h).
// Untagged neighbours dialogue_init, dialogue_leave, dialogue_frame and dialogue_draw sit between the
// tagged dialogue_load_bitmaps and dialogue_mouse_in_panel and use only this file's globals, so they are here.

#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "levelscripts/conversation.h"
#include "summoner/interface/new_dialogue.h"

#define DIALOGUE_NUM_BUTTONS 2

// ---- .rdata (names ours) -------------------------------------------------------------------
// Offsets from the panel origin (dialogue_buttons[0].x/y) per resolution_index.
static const int dialogue_title_pos[3][2]   = { { 18, 9 }, { 23, 11 }, { 29, 11 } };     // 0x57eb88
static const int dialogue_text_pos[3][2]    = { { 18, 33 }, { 23, 41 }, { 29, 53 } };    // 0x57eba0
static const int dialogue_text_lines[3]     = { 5, 5, 5 };                               // 0x57ebb8
static const int dialogue_topic_pos[3][2]   = { { 26, 132 }, { 33, 165 }, { 42, 211 } }; // 0x57ebc4
static const int dialogue_topic_lines[3]    = { 4, 4, 4 };                               // 0x57ebdc
static const char dialogue_up_arrow[]       = "\xa9";                                    // 0x57ebe8 font glyph
static const char dialogue_down_arrow[]     = "\xaa";                                    // 0x57ebec font glyph
static const int dialogue_up_arrow_pos[3][2]   = { { 7, 126 }, { 9, 158 }, { 11, 202 } };  // 0x57ebf0
static const int dialogue_down_arrow_pos[3][2] = { { 7, 186 }, { 9, 233 }, { 11, 298 } };  // 0x57ec08

// ---- .data ---------------------------------------------------------------------------------
// 0x58a158  2 x 0x40: the panel and the close button.
ui_button_def dialogue_button_table[DIALOGUE_NUM_BUTTONS] = {
    { "hud-dialoguebox",  { { 11, 4 },   { 14, 5 },   { 18, 6 } },    0 },
    { "crossbox-hilite2", { { 512, 12 }, { 640, 15 }, { 826, 21 } }, 2 },
};

// ---- .bss ----------------------------------------------------------------------------------
char dialogue_title[64];                                     // 0x5bc418  speaker name from dialogue_init
ui_button_state dialogue_buttons[DIALOGUE_NUM_BUTTONS];      // 0x5bc458
bool dialogue_topic_box_built;                               // 0x5bc488
int  dialogue_mode;                                          // 0x5bc48c  0 closed, 1 text, 2 text scrolling,
                                                             //           3 topics, 4 topics scrolling

// 0x408d80  Confidence: exact (VC6 byte match, cl 8966; was likely: the loop is exact; the set_rect width is
// written as b1.x - 2*dx + b1.w,
// the order the binary computes it in).
void dialogue_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < DIALOGUE_NUM_BUTTONS; i++) {
        ui_button_state *b = &dialogue_buttons[i];
        ui_button_def *d = &dialogue_button_table[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);

        if (strlen(d->name)) {
            char filename[128];
            bool scaled;

            strcpy(filename, d->name);
            if (!strchr(filename, '.')) {
                scaled = true;
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            } else {
                scaled = false;
            }
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            if (scaled) {
                b->w = (int)(b->w * scale + 0.5f);
                b->h = (int)(b->h * scale + 0.5f);
            }
            if (i == 0)
                b->flags |= 1;
        }
    }

    dialogue_text_box.set_rect(dialogue_text_pos[resolution_index][0] + dialogue_buttons[0].x,
                               dialogue_text_pos[resolution_index][1] + dialogue_buttons[0].y,
                               dialogue_buttons[1].w - 2 * dialogue_text_pos[resolution_index][0] + dialogue_buttons[1].x,
                               dialogue_text_lines[resolution_index]);
    dialogue_topic_box.set_rect(dialogue_topic_pos[resolution_index][0] + dialogue_buttons[0].x,
                                dialogue_topic_pos[resolution_index][1] + dialogue_buttons[0].y,
                                dialogue_buttons[1].x - 2 * dialogue_topic_pos[resolution_index][0] + dialogue_buttons[1].w,
                                dialogue_topic_lines[resolution_index]);
}

// 0x408f90  Confidence: exact.
void dialogue_init(const char *name)
{
    strcpy(dialogue_title, name);
    dialogue_text_box.start();
    dialogue_mode = 1;
    dialogue_topic_box_built = false;
}

// 0x408fe0  Confidence: exact.
void dialogue_leave()
{
    if (conversation_db.journal_updated)
        hud_show_message(localize_strings[476]);     // "Quest Journal Updated"
}

// 0x409000  Confidence: likely. The mode update after the scroll tests appears twice with different code
// (once kept in EAX for the switch, once as two stores), written the same way both times; the Esc and
// close-button exits differ in order (sound before / after the mode store) and in the value returned,
// exactly as in the binary.
int dialogue_frame(int key)
{
    int i;
    int w, h;

    if (dialogue_mode == 0)
        return key;

    for (i = 0; i < DIALOGUE_NUM_BUTTONS; i++)
        dialogue_buttons[i].flags &= ~2;

    if (dialogue_text_box.scroll_target != dialogue_text_box.scroll) {
        dialogue_mode = 2;
        return key;
    }
    if (dialogue_topic_box.scroll_target != dialogue_topic_box.scroll) {
        dialogue_mode = 4;
        return key;
    }

    if (key == KEY_ESC && !conversation_db.cur_topic->flag2) {
        conversation_active.end_cleanup(false);
        empty_function();
        sound_play_2d(0x24, 1.0f);
        dialogue_mode = 0;
        return 0;
    }

    if (point_in_rect(game_mouse_x, game_mouse_y, dialogue_buttons[1].x, dialogue_buttons[1].y,
                      dialogue_buttons[1].w, dialogue_buttons[1].h)) {
        dialogue_buttons[1].flags |= 2;
        if (mouse_was_pressed(0) && !conversation_db.cur_topic->flag2) {
            conversation_active.end_cleanup(false);
            empty_function();
            sound_play_2d(0x24, 1.0f);
            dialogue_mode = 0;
            return key;
        }
    }

    if (dialogue_mode == 2 || dialogue_mode == 4) {
        if (!dialogue_topic_box_built && dialogue_topic_box.count != 0)
            dialogue_mode = 3;
        else
            dialogue_mode = 1;
    }

    switch (dialogue_mode) {
    case 1: {
        // Text: "more" (drawn by the topic box at its origin) pages on, or ends the conversation
        // after the last line.
        bool fits = dialogue_text_box.line_fits((dialogue_text_box.first + dialogue_text_box.count - 1) % 64);
        bool end_after = conversation_db.cur_line->flag6;
        if (fits) {
            if (!(bool)conversation_db.cur_line->flag2 && !end_after) {
                conversation_active.request_end();
            } else {
                gr_get_string_size(&w, &h, "more", -1, -1);
                if (mouse_was_pressed(0)
                    && point_in_rect(game_mouse_x, game_mouse_y, dialogue_topic_box.x, dialogue_topic_box.y, w, h)) {
                    conversation_active.end_cleanup(end_after);
                    empty_function();
                    dialogue_mode = 0;
                    sound_play_2d(0x23, 1.0f);
                    return key;
                }
            }
        } else {
            gr_get_string_size(&w, &h, "more", -1, -1);
            if (mouse_was_pressed(0)
                && point_in_rect(game_mouse_x, game_mouse_y, dialogue_topic_box.x, dialogue_topic_box.y, w, h)) {
                sound_play_2d(0x23, 1.0f);
                dialogue_topic_box.reset();
                dialogue_topic_box_built = false;
                dialogue_text_box.scroll_next_page();
            }
        }
        break;
    }
    case 3:
        // Topics: hover / pick, and the two scroll arrows.
        if (dialogue_topic_box.scroll_target == dialogue_topic_box.scroll) {
            int first = -1;
            int prev = dialogue_selected_topic;
            int last = -1;

            dialogue_selected_topic = -1;
            for (i = 0; i < dialogue_topic_box.count; i++) {
                if (dialogue_topic_box.item_visible(i)) {
                    if (first < 0)
                        first = i;
                    last = i;
                    int x = dialogue_topic_box.x;
                    int y = dialogue_topic_box.item_y(i) + dialogue_topic_box.y;
                    w = dialogue_topic_box.w;
                    h = dialogue_topic_box.entry[i].height;
                    if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h)) {
                        dialogue_selected_topic = i;
                        if (i != prev)
                            sound_play_2d(0x25, 1.0f);
                        if (mouse_was_pressed(0)) {
                            sound_play_2d(0x23, 1.0f);
                            conversation_active.choose_topic(dialogue_topic_box.entry[dialogue_selected_topic].topic);
                        }
                    }
                }
            }

            if (dialogue_topic_box.count > 0) {
                if (dialogue_topic_box.scroll < 0.0f) {
                    int ax, ay, aw, ah;

                    ax = dialogue_up_arrow_pos[resolution_index][0] + dialogue_buttons[0].x;
                    ay = dialogue_up_arrow_pos[resolution_index][1] + dialogue_buttons[0].y;
                    gr_get_string_size(&aw, &ah, dialogue_up_arrow, -1, -1);
                    if (mouse_was_pressed(0) && point_in_rect(game_mouse_x, game_mouse_y, ax, ay, aw, ah)
                        && first > 0) {
                        dialogue_topic_box.scroll_target = -dialogue_topic_box.entry[first - 1].top;
                        sound_play_2d(0x25, 1.0f);
                    }
                }

                if (!dialogue_topic_box.item_visible((dialogue_topic_box.first + dialogue_topic_box.count - 1) % 64)) {
                    int ax, ay, aw, ah;

                    ax = dialogue_down_arrow_pos[resolution_index][0] + dialogue_buttons[0].x;
                    ay = dialogue_down_arrow_pos[resolution_index][1] + dialogue_buttons[0].y;
                    gr_get_string_size(&aw, &ah, dialogue_down_arrow, -1, -1);
                    if (mouse_was_pressed(0) && point_in_rect(game_mouse_x, game_mouse_y, ax, ay, aw, ah)
                        && last < dialogue_topic_box.count - 1) {
                        dialogue_topic_entry *e = &dialogue_topic_box.entry[last + 1];
                        dialogue_topic_box.scroll_target = gr_get_font_height(-1) * dialogue_topic_box.lines
                                                         - e->height - e->top;
                        sound_play_2d(0x25, 1.0f);
                    }
                }
            }
        }
        break;
    }

    if (dialogue_mode != 0) {
        if (dialogue_text_box.scroll_target != dialogue_text_box.scroll) {
            dialogue_mode = 2;
            return key;
        }
        if (dialogue_topic_box.scroll_target != dialogue_topic_box.scroll) {
            dialogue_mode = 4;
            return key;
        }
        if (!dialogue_topic_box_built && dialogue_topic_box.count != 0)
            dialogue_mode = 3;
        else
            dialogue_mode = 1;
    }
    return key;
}

// 0x409650  Confidence: likely (VC6 8966 byte match, m-1020 w6; was likely). Each arrow's position is kept for the hit test and written out again for
// gr_string (the binary reloads it after gr_set_color_ptr).
void dialogue_draw()
{
    int i;

    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < DIALOGUE_NUM_BUTTONS; i++) {
        ui_button_state *b = &dialogue_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }

    gr_string(dialogue_title_pos[resolution_index][0] + dialogue_buttons[0].x,
              dialogue_title_pos[resolution_index][1] + dialogue_buttons[0].y,
              dialogue_title, -1, gr_string_mode.value);
    dialogue_text_box.draw();

    switch (dialogue_mode) {
    case 1:
        // When the text has stopped scrolling, put a "more" (kind 5) or end (kind 6) entry in the
        // topic box under the last line.
        if (dialogue_text_box.scroll_target == dialogue_text_box.scroll) {
            if (dialogue_text_box.line_fits((dialogue_text_box.count + dialogue_text_box.first - 1) % 64)
                && !(bool)conversation_db.cur_line->flag6) {
                if (!(bool)conversation_db.cur_line->flag2)
                    break;
                if (dialogue_topic_box.count == 0)
                    dialogue_topic_box.add(0, 6);
            } else {
                if (dialogue_topic_box.count == 0)
                    dialogue_topic_box.add(0, 5);
            }
            dialogue_topic_box_built = true;
            dialogue_topic_box.draw();
        }
        break;
    case 3:
    case 4: {
        int x, y;

        for (i = 0; i < dialogue_topic_box.count; i++)
            dialogue_topic_box.entry[i].state = (i == dialogue_selected_topic) ? 4 : 3;
        dialogue_topic_box.draw();

        if (dialogue_topic_box.scroll < 0.0f) {
            int w, h;
            x = dialogue_up_arrow_pos[resolution_index][0] + dialogue_buttons[0].x;
            y = dialogue_up_arrow_pos[resolution_index][1] + dialogue_buttons[0].y;
            gr_get_string_size(&w, &h, dialogue_up_arrow, -1, -1);
            if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h))
                gr_set_color_ptr(&ui_color_highlight);
            else
                gr_set_color_ptr(&ui_color_text);
            gr_string(dialogue_up_arrow_pos[resolution_index][0] + dialogue_buttons[0].x,
                      dialogue_up_arrow_pos[resolution_index][1] + dialogue_buttons[0].y,
                      dialogue_up_arrow, -1, gr_string_mode.value);
        }

        if (!dialogue_topic_box.item_visible((dialogue_topic_box.first + dialogue_topic_box.count - 1) % 64)) {
            int w, h;
            x = dialogue_down_arrow_pos[resolution_index][0] + dialogue_buttons[0].x;
            y = dialogue_down_arrow_pos[resolution_index][1] + dialogue_buttons[0].y;
            gr_get_string_size(&w, &h, dialogue_down_arrow, -1, -1);
            if (point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h))
                gr_set_color_ptr(&ui_color_highlight);
            else
                gr_set_color_ptr(&ui_color_text);
            gr_string(dialogue_down_arrow_pos[resolution_index][0] + dialogue_buttons[0].x,
                      dialogue_down_arrow_pos[resolution_index][1] + dialogue_buttons[0].y,
                      dialogue_down_arrow, -1, gr_string_mode.value);
        }
        break;
    }
    }
}

// 0x409980  Confidence: exact.
bool dialogue_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, dialogue_buttons[0].x, dialogue_buttons[0].y,
                         dialogue_buttons[0].w, dialogue_buttons[0].h);
}
