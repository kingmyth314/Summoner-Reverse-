// unnamed_ui_manager_0x52f570.cpp - Sum.exe v1.40, code 0x52f570-0x530b86 (vsdk\ui; the unit's file name
// is not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /Ox /Ow (D7502). ui_container::get_mask_buffer and
// ui_window::get_mask_data are defined at the end of this file, not inline (ui_draw calls them, D8403); the
// colour set constructors call the COMDAT Color() 0x505530. Confidence per function: exact = expect identical code, likely = same
// semantics, small layout or scheduling risk, unsure = the source shape is a guess.
//
// The UI manager: the window list (ui_window_sentinel), the per-frame input pass (ui_process: mouse
// buttons into the ui_* globals, the window under a press, tooltips, the end of a button drag) and the
// draw pass (ui_draw: windows, tooltip, the dragged button), the colour sets of every gadget type and
// their defaults (ui_init), the UI sounds and the window group depth.
//
// Unit bounds (NOTES.md): starts with its initialiser run 0x52f570-0x52f700 (table 0x588974-0x5889a0,
// twelve entries, the next table entry 0x531a80 is the os unit's); ends with its COMDATs (the colour set
// constructors 0x5309c0-0x530b50 and the two inline getters 0x530b70 / 0x530b80) before the os unit
// (0x530b90). .bss 0x2cbf628-0x2cbfcd9 (after the list box's 0x2cbf584-0x2cbf623); no .data. No EH.
//
// The globals are defined in their initialiser order (the objects with constructors) and then in address
// order; VC6's .bss placement is not reproduced by source order. Not referenced in this unit, so not
// defined here (unknown, maybe padding): 0x2cbf62b-0x2cbf62f, 0x2cbf6a4, 0x2cbf704, 0x2cbf714,
// 0x2cbfc1c, 0x2cbfc6c, 0x2cbfcda-0x2cbfcdf.

#include <string.h>
#include <stdlib.h>
#include "vsdk/ui/window.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/mouse.h"
#include "vsdk/snd/snd.h"

// ---- objects with constructors, in initialiser order ------------------------------------------------
// 0x52f570 / 0x52f580  init_ui_tooltip_timer: the dynamic initialiser (a jmp to its body, which tail-calls
//                      RealtimeTimer::RealtimeTimer). Compiler-generated.
RealtimeTimer ui_tooltip_timer;                 // 0x2cbf680
// 0x52f590  init_ui_window_sentinel: calls 0x52f5a0 and 0x52f5b0 /
// 0x52f5a0  ui_window_sentinel_ctor: mov ecx, 0x2cbf718; jmp ui_window::ui_window /
// 0x52f5b0  ui_window_sentinel_register_dtor: atexit(0x52f5c0) /
// 0x52f5c0  ui_window_sentinel_dtor: mov ecx, 0x2cbf718; jmp ui_window::~ui_window.
//           Compiler-generated. Confidence: exact.
ui_window ui_window_sentinel;                   // 0x2cbf718 (next 0x2cbfbfc, prev 0x2cbfc00)
// 0x52f5d0 / 0x52f5e0  init_ui_tooltip_state (ui_colors3). Compiler-generated.
ui_colors3 ui_tooltip_colors;                   // 0x2cbf708
// 0x52f5f0 / 0x52f600  init_ui_container_state (ui_colors4). Compiler-generated.
ui_colors4 ui_container_colors;                 // 0x2cbfc20
// 0x52f610 / 0x52f620  init_ui_window_defaults (ui_window_color_set). Compiler-generated.
ui_window_color_set ui_window_colors;           // 0x2cbf670
// 0x52f630 / 0x52f640  init_ui_global_2cbf6a8 (ui_colors5_x3). Compiler-generated.
ui_colors5_x3 ui_button_colors;                 // 0x2cbf6a8
// 0x52f650 / 0x52f660  init_ui_global_2cbf6e8 (ui_colors3_x2). Compiler-generated.
ui_colors3_x2 ui_static_colors;                 // 0x2cbf6e8
// 0x52f670 / 0x52f680  init_ui_global_2cbf630 (ui_colors4_x4). Compiler-generated.
ui_colors4_x4 ui_checkbox_colors;               // 0x2cbf630
// 0x52f690 / 0x52f6a0  init_ui_global_2cbfc70 (ui_colors6_x2). Compiler-generated.
ui_colors6_x2 ui_inputbox_colors;               // 0x2cbfc70
// 0x52f6b0 / 0x52f6c0  init_ui_global_2cbfc08 (ui_colors5). Compiler-generated.
ui_colors5 ui_slider_colors;                    // 0x2cbfc08
// 0x52f6d0 / 0x52f6e0  init_ui_global_2cbfc50 (ui_colors2_x3). Compiler-generated.
ui_colors2_x3 ui_listbox_entry_colors;          // 0x2cbfc50
// 0x52f6f0 / 0x52f700  init_ui_global_2cbfc30 (ui_colors8). Compiler-generated.
ui_colors8 ui_listbox_colors;                   // 0x2cbfc30

// ---- .bss (0x2cbf628-0x2cbfcd9), address order --------------------------------------------------------
bool ui_left_pressed;                           // 0x2cbf628
bool ui_right_released;                         // 0x2cbf629
bool ui_right_pressed;                          // 0x2cbf62a
int ui_sounds[8];                               // 0x2cbf684
bool ui_tooltip_ready;                          // 0x2cbf6e4
bool ui_left_double_clicked;                    // 0x2cbf700
bool ui_right_double_clicked;                   // 0x2cbfc04
int ui_key;                                     // 0x2cbfc68
int ui_mouse_wheel;                             // 0x2cbfca0
bool ui_left_released;                          // 0x2cbfca4
int ui_mouse_x;                                 // 0x2cbfca8
int ui_mouse_y;                                 // 0x2cbfcac
static int ui_last_mouse_x;                     // 0x2cbfcb0 the position before this frame's (name ours)
static int ui_last_mouse_y;                     // 0x2cbfcb4 (name ours)
ui_button *ui_drag_gadget;                      // 0x2cbfcb8
ui_gadget *ui_drop_target;                      // 0x2cbfcbc
bool ui_drag_dropped;                           // 0x2cbfcc0
ui_gadget *ui_current_gadget;                   // 0x2cbfcc4
int ui_group_depth;                             // 0x2cbfcc8
ui_element *ui_tooltip_gadget;                  // 0x2cbfccc
static int ui_inited;                           // 0x2cbfcd0 ui_init's guard (name ours)
ui_gadget *ui_last_gadget;                      // 0x2cbfcd4
// 0x2cbfcd8 / 0x2cbfcd9: a press may count as a double click (cleared after a double click so the third
// click starts over). Names ours.
static bool ui_left_double_click_ok;
static bool ui_right_double_click_ok;

// 0x52f710  Confidence: exact. The empty function ui_init registers with atexit.
void atexit_nop_52f710()
{
}

// 0x52f720  Confidence: likely (the local for the handle). Missing sounds above index 1 fall back to
// sound 1.
void ui_play_sound(int index)
{
    int handle = ui_sounds[index];
    if (handle < 0) {
        if (index <= 1)
            return;
        handle = ui_sounds[1];
        if (handle < 0)
            return;
    }
    snd_play(handle, 1.0f, 0.0f, 0, 0);
}

// 0x52f760  Confidence: exact (the set calls in the binary's order, which is not index order within a
// set). Once: the empty window list, no sounds, the default colours of every gadget type.
void ui_init()
{
    if (ui_inited)
        return;

    ui_tooltip_ready = false;
    ui_tooltip_timer.SetTimeout(0);
    list_init(&ui_window_sentinel);
    atexit(atexit_nop_52f710);
    memset(ui_sounds, -1, sizeof(ui_sounds));
    ui_group_depth = 0;

    ui_tooltip_colors.c0.set(0, 0, 0, 96);
    ui_tooltip_colors.c1.set(255, 255, 255, 255);
    ui_tooltip_colors.c2.set(0, 0, 0, 0);

    ui_container_colors.c2.set(144, 144, 144, 255);
    ui_container_colors.c3.set(0, 0, 0, 255);
    ui_container_colors.c0.set(128, 128, 128, 196);
    ui_container_colors.c1.set(48, 48, 48, 196);

    ui_window_colors.c2.set(96, 92, 88, 224);
    ui_window_colors.c0.set(128, 128, 128, 224);
    ui_window_colors.c3.set(96, 92, 88, 224);
    ui_window_colors.c1.set(48, 48, 48, 224);

    ui_button_colors.g0.c2.set(0, 0, 0, 0);
    ui_button_colors.g0.c0.set(128, 128, 128, 196);
    ui_button_colors.g0.c1.set(32, 32, 32, 196);
    ui_button_colors.g0.c3.set(144, 144, 144, 255);
    ui_button_colors.g0.c4.set(0, 0, 0, 255);
    ui_button_colors.g1.c2.set(255, 255, 0, 24);
    ui_button_colors.g1.c0.set(128, 128, 128, 196);
    ui_button_colors.g1.c1.set(32, 32, 32, 196);
    ui_button_colors.g1.c3.set(160, 160, 160, 255);
    ui_button_colors.g1.c4.set(0, 0, 0, 255);
    ui_button_colors.g2.c2.set(0, 0, 0, 0);
    ui_button_colors.g2.c0.set(128, 128, 128, 196);
    ui_button_colors.g2.c1.set(48, 48, 48, 196);
    ui_button_colors.g2.c3.set(48, 48, 48, 255);
    ui_button_colors.g2.c4.set(112, 112, 112, 255);

    ui_static_colors.g0.c0.set(0, 0, 0, 0);
    ui_static_colors.g0.c1.set(144, 144, 144, 255);
    ui_static_colors.g0.c2.set(0, 0, 0, 255);
    ui_static_colors.g1.c0.set(0, 0, 0, 0);
    ui_static_colors.g1.c1.set(48, 48, 48, 255);
    ui_static_colors.g1.c2.set(112, 112, 112, 255);

    ui_checkbox_colors.g0.c0.set(255, 255, 255, 255);
    ui_checkbox_colors.g0.c1.set(255, 0, 0, 255);
    ui_checkbox_colors.g0.c2.set(144, 144, 144, 255);
    ui_checkbox_colors.g0.c3.set(0, 0, 0, 255);
    ui_checkbox_colors.g1.c0.set(255, 255, 255, 255);
    ui_checkbox_colors.g1.c1.set(255, 0, 0, 255);
    ui_checkbox_colors.g1.c2.set(196, 196, 196, 255);
    ui_checkbox_colors.g1.c3.set(0, 0, 0, 255);
    ui_checkbox_colors.g2.c0.set(128, 128, 128, 255);
    ui_checkbox_colors.g2.c1.set(255, 0, 0, 255);
    ui_checkbox_colors.g2.c2.set(196, 196, 196, 255);
    ui_checkbox_colors.g2.c3.set(0, 0, 0, 255);
    ui_checkbox_colors.g3.c0.set(172, 172, 172, 255);
    ui_checkbox_colors.g3.c1.set(255, 0, 0, 255);
    ui_checkbox_colors.g3.c2.set(48, 48, 48, 255);
    ui_checkbox_colors.g3.c3.set(112, 112, 112, 255);

    ui_inputbox_colors.g0.c2.set(48, 48, 48, 224);
    ui_inputbox_colors.g0.c0.set(128, 128, 128, 128);
    ui_inputbox_colors.g0.c1.set(24, 24, 24, 196);
    ui_inputbox_colors.g0.c3.set(188, 188, 188, 255);
    ui_inputbox_colors.g0.c4.set(0, 0, 0, 255);
    ui_inputbox_colors.g0.c5.set(255, 140, 0, 160);
    ui_inputbox_colors.g1.c2.set(0, 0, 0, 0);
    ui_inputbox_colors.g1.c0.set(128, 128, 128, 128);
    ui_inputbox_colors.g1.c1.set(24, 24, 24, 196);
    ui_inputbox_colors.g1.c3.set(144, 144, 144, 255);
    ui_inputbox_colors.g1.c4.set(24, 24, 24, 255);
    ui_inputbox_colors.g1.c5.set(0, 0, 0, 0);

    ui_slider_colors.c2.set(62, 66, 94, 255);
    ui_slider_colors.c0.set(86, 92, 125, 255);
    ui_slider_colors.c1.set(22, 25, 50, 255);
    ui_slider_colors.c3.set(120, 120, 120, 255);
    ui_slider_colors.c4.set(128, 128, 128, 255);

    ui_listbox_entry_colors.g0.c0.set(170, 170, 170, 255);
    ui_listbox_entry_colors.g0.c1.set(0, 0, 0, 255);
    ui_listbox_entry_colors.g1.c0.set(170, 170, 0, 255);
    ui_listbox_entry_colors.g1.c1.set(0, 0, 0, 255);
    ui_listbox_entry_colors.g2.c0.set(140, 140, 0, 255);
    ui_listbox_entry_colors.g2.c1.set(0, 0, 0, 255);

    ui_listbox_colors.g0.c2.set(48, 48, 48, 224);
    ui_listbox_colors.g0.c0.set(128, 128, 128, 128);
    ui_listbox_colors.g0.c1.set(24, 24, 24, 196);
    ui_listbox_colors.g1.c2.set(0, 0, 0, 0);
    ui_listbox_colors.g1.c0.set(128, 128, 128, 128);
    ui_listbox_colors.g1.c1.set(24, 24, 24, 196);
    ui_listbox_colors.g2.c0.set(144, 144, 144, 255);
    ui_listbox_colors.g2.c1.set(24, 24, 24, 255);

    ui_inited = 1;
}

// 0x52ff30  Confidence: exact (the mask getters are out-of-line calls, D8403; the tooltip selection written with t = NULL for the "none" cases; the
// code jumps past the final test there). Windows from the first to the last, the caller's overlay, the
// hovered element's tooltip (containers and windows pick the one whose value matches the mask pixel under
// the mouse; statics and input boxes show none), then the dragged button under the mouse.
void ui_draw(void (*callback)())
{
    ui_window *win;
    char text[256];

    ui_init();
    gr::reset_clip();
    win = ui_window_sentinel.next;
    while (win != &ui_window_sentinel) {
        win->draw();
        win = win->next;
        gr::reset_clip();
    }

    if (callback)
        callback();

    if (ui_tooltip_ready && ui_tooltip_gadget) {
        ui_tooltip *t = NULL;
        int pixel;

        int kind = ui_tooltip_gadget->get_kind();
        if (kind == 0) {
            switch (((ui_gadget *)ui_tooltip_gadget)->get_type()) {
            case UI_TYPE_BUTTON:
            case UI_TYPE_CHECKBOX:
            case UI_TYPE_RADIO:
            case UI_TYPE_SLIDER:
            case UI_TYPE_LISTBOX:
                t = ui_tooltip_gadget->tooltips.next;
                break;
            case UI_TYPE_CONTAINER: {
                ui_container *c = (ui_container *)ui_tooltip_gadget;
                if (c->get_mask_buffer()) {
                    pixel = c->get_mask_pixel(ui_mouse_x, ui_mouse_y);
                    for (t = c->tooltips.next; t != (ui_tooltip *)&c->tooltips; t = t->next) {
                        if (t->font == pixel)
                            break;
                    }
                }
                break;
            }
            }
        } else if (kind == 1) {
            ui_window *w = (ui_window *)ui_tooltip_gadget;
            if (w->get_mask_data()) {
                pixel = w->get_mask_pixel(ui_mouse_x, ui_mouse_y);
                for (t = w->tooltips.next; t != (ui_tooltip *)&w->tooltips; t = t->next) {
                    if (t->font == pixel)
                        break;
                }
            }
        }
        if (t && t != (ui_tooltip *)&ui_tooltip_gadget->tooltips)
            t->draw();
    }

    if (ui_drag_gadget) {
        int flags = ui_drag_gadget->get_flags();
        Color *colors = ui_drag_gadget->colors;
        int w, h;
        int ox, oy;

        ui_drag_gadget->get_size(&w, &h);
        ui_drag_gadget->get_drag_origin(&ox, &oy);
        if (!ui_drag_gadget->state_drag.draw(ui_mouse_x + ox, ui_mouse_y + oy, w, h, -1, -1)) {
            if (!(flags & (UI_BUTTON_FLAT | UI_BUTTON_NO_BEVEL))) {
                gr_set_color_ptr(&colors[2]);
                gr_rect(ui_mouse_x + ox + 2, ui_mouse_y + oy + 2, w - 4, h - 4, gr_rect_mode, 1.0f);
                ui_draw_bevel(ui_mouse_x + ox, ui_mouse_y + oy, w, h, &colors[0], &colors[1], 2, 0);
            } else {
                gr_set_color_ptr(&colors[2]);
                gr_rect(ui_mouse_x + ox, ui_mouse_y + oy, w, h, gr_rect_mode, 1.0f);
            }
        }

        strcpy(text, ui_drag_gadget->get_text());
        if (strlen(text)) {
            int tw, th;
            gr_get_string_size(&tw, &th, text, -1, ui_drag_gadget->font);
            th = th - gr_get_font_height(ui_drag_gadget->font) + gr_get_font_height2(ui_drag_gadget->font) + 1;
            int x = w / 2 - tw / 2 + ui_mouse_x + ox;
            int y = h / 2 - th / 2 + ui_mouse_y + oy;
            if (colors[4].a) {
                gr_set_color_ptr(&colors[4]);
                gr_string(x + 1, y + 1, text, ui_drag_gadget->font, gr_string_mode);
            }
            gr_set_color_ptr(&colors[3]);
            gr_string(x, y, text, ui_drag_gadget->font, gr_string_mode);
        }
    }
}

// 0x530330  Confidence: likely (the order of the stores that restore the click flags; the two
// "dropped" paths of the drag end). The input pass:
//  - the mouse buttons into ui_left_pressed .. ui_right_double_clicked (a right press only without a
//    left one; a double click only when the previous press was not one), the mouse position;
//  - on a press, the gadget under it in the topmost window that has one (a double click on another
//    gadget than the last press is dropped) becomes the tooltip element;
//  - else the hovered element: every enabled window from the top down is tested, the last hit wins
//    (F6211: a window under the topmost one under the mouse gets the tooltip);
//  - the focus is dropped from a disabled or hidden gadget; every window's process, top down, sees the
//    same click flags (a window that changed them gets them restored, and with clear_clicks the clicks
//    are then cleared in the mouse module);
//  - the tooltip delay (1 s without movement or buttons);
//  - a left release ends a button drag: back onto nothing outside the button gives the count back
//    (event 0x10); onto another button that has room moves drag_amount (event 0x11 to the dragged
//    button, then 0x10).
int ui_process(int key, bool clear_clicks)
{
    bool any_click = false;
    bool left_pressed, left_released, right_pressed, right_released;
    bool left_double, right_double;
    bool restored = false;
    bool hovered = false;
    ui_gadget *g = NULL;
    ui_window *win;
    int dx, dy;

    ui_init();
    ui_key = key;

    left_pressed = ui_left_pressed = mouse_was_pressed(0);
    left_released = ui_left_released = mouse_was_released(0);
    right_pressed = ui_right_pressed = (!ui_left_pressed && mouse_was_pressed(1));
    right_released = ui_right_released = mouse_was_released(1);
    left_double = ui_left_double_clicked = (ui_left_double_click_ok && mouse_was_double_clicked(0));
    right_double = ui_right_double_clicked =
        (ui_right_double_click_ok && ui_right_pressed && mouse_was_double_clicked(1));
    if (ui_left_pressed || ui_left_released || ui_right_pressed || ui_right_released ||
        ui_left_double_clicked || right_double)
        any_click = true;

    ui_last_mouse_x = ui_mouse_x;
    ui_last_mouse_y = ui_mouse_y;
    mouse_get_pos(&ui_mouse_x, &ui_mouse_y, &game_mouse_z);
    mouse_get_delta(&dx, &dy, &ui_mouse_wheel);

    if (ui_left_pressed || ui_right_pressed) {
        for (win = ui_window_sentinel.prev; win != &ui_window_sentinel; win = win->prev) {
            g = win->gadget_at(ui_mouse_x, ui_mouse_y);
            if (g) {
                if (ui_left_pressed) {
                    if (ui_left_double_clicked && ui_last_gadget != g)
                        ui_left_double_clicked = false;
                } else if (ui_right_pressed) {
                    if (ui_right_double_clicked && ui_last_gadget != g)
                        ui_right_double_clicked = false;
                }
                ui_last_gadget = g;
                break;
            }
        }
        if (!g)
            ui_last_gadget = NULL;

        if (ui_left_pressed)
            ui_left_double_click_ok = !ui_left_double_clicked;
        if (ui_right_pressed)
            ui_right_double_click_ok = !ui_right_double_clicked;

        if (g) {
            if (ui_tooltip_gadget != g) {
                ui_tooltip_ready = false;
                ui_tooltip_gadget = g;
            }
            goto focus;
        }
    }

    for (win = ui_window_sentinel.prev; win != &ui_window_sentinel; win = win->prev) {
        if (win->is_disabled())
            continue;
        ui_gadget *over = win->gadget_at(ui_mouse_x, ui_mouse_y);
        if (over == NULL) {
            int ww, wh, wx, wy;
            win->get_size(&ww, &wh);
            win->get_position(&wx, &wy);
            wx = ui_mouse_x - wx;
            wy = ui_mouse_y - wy;
            if (wx >= 0 && wy >= 0 && wx < ww && wy < wh && win->get_mask_pixel(ui_mouse_x, ui_mouse_y)) {
                hovered = true;
                if (ui_tooltip_gadget != win) {
                    ui_tooltip_ready = false;
                    ui_tooltip_gadget = win;
                }
            }
        } else {
            hovered = true;
            if (ui_tooltip_gadget != over) {
                ui_tooltip_ready = false;
                ui_tooltip_gadget = over;
            }
        }
    }
    if (!hovered && ui_tooltip_gadget) {
        ui_tooltip_ready = false;
        ui_tooltip_gadget = NULL;
    }

focus:
    if (ui_current_gadget && (ui_current_gadget->is_disabled() || ui_current_gadget->is_hidden()))
        ui_current_gadget = NULL;

    win = ui_window_sentinel.prev;
    while (win != &ui_window_sentinel) {
        ui_window *prev = win->prev;
        key = win->process(key);
        if (any_click) {
            if (ui_left_pressed != left_pressed || ui_left_released != left_released ||
                ui_right_pressed != right_pressed || ui_right_released != right_released ||
                ui_left_double_clicked != left_double || ui_right_double_clicked != right_double) {
                ui_left_pressed = left_pressed;
                ui_left_released = left_released;
                ui_right_pressed = right_pressed;
                ui_right_released = right_released;
                ui_left_double_clicked = left_double;
                ui_right_double_clicked = right_double;
                restored = true;
            }
        }
        win = prev;
    }

    if (ui_tooltip_gadget) {
        if (mouse_is_down(0)) {
            ui_tooltip_ready = false;
            ui_tooltip_timer.SetTimeout(1000);
        } else if (!ui_tooltip_ready) {
            if (ui_last_mouse_x != ui_mouse_x || ui_last_mouse_y != ui_mouse_y) {
                ui_tooltip_ready = false;
                ui_tooltip_timer.SetTimeout(1000);
            } else if (ui_tooltip_timer.elapsed()) {
                ui_tooltip_ready = true;
            }
        }
    } else {
        ui_tooltip_ready = false;
    }

    if (ui_left_released && ui_drag_gadget) {
        if (!ui_drag_dropped) {
            ui_button *target = ui_drag_gadget->drag_data;
            if (target == NULL) {
                if (!ui_drag_gadget->is_mouse_over()) {
                    ui_drag_gadget->count -= ui_drag_gadget->drag_amount;
                    ui_drag_gadget->send_event(0x10, UI_USE_ID);
                }
            } else if (target != ui_drag_gadget) {
                int n = target->count + ui_drag_gadget->drag_amount;
                if (n <= target->capacity) {
                    ui_drop_target = target;
                    if (target->capacity >= 0)
                        target->count = n;
                    ui_drag_gadget->send_event(0x11, UI_USE_ID);
                    if (ui_drag_gadget->capacity >= 0)
                        ui_drag_gadget->count -= ui_drag_gadget->drag_amount;
                    ui_drag_gadget->send_event(0x10, UI_USE_ID);
                }
            }
        }
        ui_drag_dropped = false;
        ui_drag_gadget = NULL;
        ui_drop_target = NULL;
    }

    if (restored == true && clear_clicks == true)
        mouse_clear_clicks();
    return key;
}

// 0x5308e0  Confidence: exact. No caller. Slot 0 (dtor) of the first window unlinks it.
void ui_destroy_all_windows()
{
    while (ui_window_sentinel.next != &ui_window_sentinel)
        ui_window_sentinel.next->dtor();
}

// 0x530910  Confidence: exact.
void ui_push_group()
{
    ui_group_depth++;
}

// 0x530920  Confidence: exact.
void ui_pop_group()
{
    ui_group_depth--;
}

// 0x530930  Confidence: exact. No caller.
ui_gadget *ui_find_gadget_by_name(const char *name)
{
    for (ui_window *win = ui_window_sentinel.next; win != &ui_window_sentinel; win = win->next) {
        ui_gadget *g = win->find_gadget_by_name(name);
        if (g)
            return g;
    }
    return NULL;
}

// 0x530970  Confidence: exact.
ui_window *ui_get_top_modal_window()
{
    for (ui_window *win = ui_window_sentinel.next; win != &ui_window_sentinel; win = win->next) {
        if ((win->get_flags() & UI_WINDOW_MODAL) && win->is_shown() && !win->is_disabled())
            return win;
    }
    return NULL;
}

// The COMDATs at the end of the object (compiler-generated; each Color is built by the folded empty
// constructor 0x505530):
// 0x5309c0  ui_window_color_set::ui_window_color_set (ui_window_colors__ctor): ui_colors3, then c3.
// 0x5309e0  ui_colors3::ui_colors3 (ui_colors3__ctor).
// 0x530a00  ui_colors5_x3::ui_colors5_x3 (ui_colors5_x3__ctor).
// 0x530a20  ui_colors3_x2::ui_colors3_x2 (ui_colors3_x2__ctor).
// 0x530a40  ui_colors4_x4::ui_colors4_x4 (ui_colors4_x4__ctor).
// 0x530a70  ui_colors4::ui_colors4 (ui_colors4__ctor).
// 0x530aa0  ui_colors6_x2::ui_colors6_x2 (ui_colors6_x2__ctor).
// 0x530ac0  ui_colors6::ui_colors6 (ui_colors6__ctor): ui_colors3, then c3 .. c5.
// 0x530af0  ui_colors5::ui_colors5 (ui_colors5__ctor): ui_colors3, then c3, c4.
// 0x530b10  ui_colors2_x3::ui_colors2_x3 (ui_colors2_x3__ctor).
// 0x530b30  ui_colors2::ui_colors2 (ui_colors2__ctor).
// 0x530b50  ui_colors8::ui_colors8 (ui_colors8__ctor).

// 0x530b70  Confidence: exact. Not inline: ui_draw calls it (D8403).
ubyte *ui_container::get_mask_buffer()
{
    return mask_data;
}

// 0x530b80  Confidence: exact. Not inline: ui_draw calls it (D8403).
ubyte *ui_window::get_mask_data()
{
    return mask_data;
}
