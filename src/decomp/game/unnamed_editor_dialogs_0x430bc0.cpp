// unnamed_editor_dialogs_0x430bc0.cpp - Sum.exe v1.40, code 0x430bc0-0x43186e: the editor dialog boxes (the
// script editor's modal "Error!" / "Warning!" message windows with up to three buttons).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: from init_empty_430bd0_thunk (static_initializers entry 93) to the ui_button destructor
// COMDAT 0x4317e0; the boot / load-game state unit starts with entry 94 at 0x431870. The previous unit ends
// with state_credits_frame (0x430b90).
// Data: no initialised data. The literal pool 0x59309c-0x5930b7 ("No", "Yes", "Retry", "Cancel", "OK", in
// reverse order of use) is followed by the next unit's first initialised dword (build_stamp_pending,
// 0x5930b8), which ends this unit's .data. "\n" (0x588ba4) is pooled with an earlier unit (/Gf).
// .bss 0x5fa278-0x5fa2c7: editor_dialog_count, the six colours, the list head (a whole 0x30-byte dialog).
// 0x5fa27c (4 bytes) has no reference.

#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/window.h"
#include "summoner/summoner.h"

#ifdef __GNUC__
// editor_dialog_destroy deletes the window and gadgets through their non-virtual destructors, as the binary
// does (call the destructor, then operator delete); mingw warns, VC6 does not (as objects.cpp, D61).
#pragma GCC diagnostic ignored "-Wdelete-non-abstract-non-virtual-dtor"
#endif

#define EDITOR_DIALOG_MAX_LINES 10

// ---- .bss ----------------------------------------------------------------------------------------------
// 0x430bc0  init_empty_430bd0_thunk (static_initializers entry 93, jmp). Confidence: exact (compiler-generated).
// 0x430bd0  init_empty_430bd0: a bare ret, the dynamic initialiser of the Color array (the inline empty
// Color() constructor, its loop optimised away). One initialiser for six colours means one array, not six
// objects. Confidence: likely (which object it belongs to).
Color editor_dialog_colors[6];        // 0x5fa280
int editor_dialog_count;              // 0x5fa278
editor_dialog editor_dialog_list;     // 0x5fa298 (no constructor: no initialiser)

// ---- code ----------------------------------------------------------------------------------------------

// 0x430be0  Confidence: exact. GameInit.
void editor_dialog_init()
{
    list_init(&editor_dialog_list);
    editor_dialog_count = 0;
    editor_dialog_colors[0].set(0, 0, 0, 0);
    editor_dialog_colors[1].set(0x90, 0x90, 0x90, 0xff);
    editor_dialog_colors[2].set(0, 0, 0, 0xff);
    editor_dialog_colors[3].set(0, 0, 0, 0);
    editor_dialog_colors[4].set(0x30, 0x30, 0x30, 0xff);
    editor_dialog_colors[5].set(0x70, 0x70, 0x70, 0xff);
}

// One 50 x 25 button with the next id; the six calls are written out in full in the binary, so this is a
// macro, not a function (/Ob1 inlines only what is marked inline; a helper would be a call).
#define EDITOR_DIALOG_BUTTON(b, window, x, y, label)                       \
    do {                                                                   \
        (b) = new ui_button;                                               \
        (b)->create((window), (x), (y), 50, 25, editor_dialog_count++, 0, false); \
        (b)->set_text(label);                                              \
        (b)->font = -1;                                                    \
        (b)->callback = editor_dialog_handle_event;                        \
    } while (0)

// 0x430c80  Confidence: likely (the six button blocks: the macro and the statement order around the x
// steps are a guess; the per-case code, constants and stores match). Callers script_write_objects and
// script_save (the script editor). The text is split into at most 10 lines of width - 10 and joined with
// "\n" into one static. Game bugs: with zero lines (an empty text) buf is used uninitialised, and a buttons
// value above 4 leaves d->buttons unset (operator new memory) for editor_dialog_destroy to delete.
ui_window *editor_dialog_show(int width, const char *title, const char *text, int buttons,
                              editor_dialog_callback cb0, editor_dialog_callback cb1,
                              editor_dialog_callback cb2, int flags)
{
    int title_h, text_h, title_w, text_w;
    int line_len[EDITOR_DIALOG_MAX_LINES];
    int line_off[EDITOR_DIALOG_MAX_LINES];
    char line[256];
    char buf[1024];
    int num_lines, i, x, y, h, half_w, bx, by;
    ui_window *window;
    ui_static *s;
    ui_button *b;
    editor_dialog *d = new editor_dialog;

    d->closed = false;
    d->callbacks[0] = cb0;
    d->callbacks[1] = cb1;
    d->callbacks[2] = cb2;
    if (title) {
        gr_get_string_size(&title_w, &title_h, title, -1, -1);
    } else {
        title_w = 0;
        title_h = 0;
    }

    num_lines = gr_split_str(line_len, line_off, text, width - 10, EDITOR_DIALOG_MAX_LINES, 0, -1);
    for (i = 0; i < num_lines; i++) {
        strncpy(line, text + line_off[i], line_len[i]);
        line[line_len[i]] = 0;
        if (i == 0) {
            strcpy(buf, line);
        } else {
            strcat(buf, "\n");
            strcat(buf, line);
        }
    }
    gr_get_string_size(&text_w, &text_h, buf, -1, -1);

    if (title)
        h = title_h + text_h + 65;
    else
        h = text_h + 55;
    half_w = width / 2;
    x = gr_screen_width() / 2 - half_w;
    y = gr_screen_height() / 2 - h / 2;

    window = new ui_window;
    window->init(x, y, width, h, UI_NO_ID, flags | UI_WINDOW_MODAL);
    d->window = window;

    s = NULL;
    if (title) {
        bx = half_w - title_w / 2;
        s = new ui_static;
        s->create(window, bx, 5, title_w, title_h, -1, false);
        s->set_text(title);
        s->font = -1;
        s->colors = editor_dialog_colors;
    }
    d->title = s;

    if (num_lines == 1)
        bx = half_w - text_w / 2;
    else
        bx = 5;
    if (title)
        by = title_h + 15;
    else
        by = 5;
    s = new ui_static;
    s->create(window, bx, by, text_w, text_h, -1, false);
    s->set_text(buf);
    s->font = -1;
    s->colors = editor_dialog_colors;
    d->text = s;

    switch (buttons) {
    case EDITOR_DIALOG_OK:
        bx = half_w - 25;
        by = h - 30;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "OK");
        d->buttons[0] = b;
        d->buttons[1] = NULL;
        d->buttons[2] = NULL;
        break;
    case EDITOR_DIALOG_OK_CANCEL:
        bx = half_w - 60;
        by = h - 30;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "OK");
        bx += 70;
        d->buttons[0] = b;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Cancel");
        d->buttons[1] = b;
        d->buttons[2] = NULL;
        break;
    case EDITOR_DIALOG_RETRY_CANCEL:
        bx = half_w - 60;
        by = h - 30;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Retry");
        bx += 70;
        d->buttons[0] = b;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Cancel");
        d->buttons[1] = b;
        d->buttons[2] = NULL;
        break;
    case EDITOR_DIALOG_YES_NO:
        bx = half_w - 60;
        by = h - 30;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Yes");
        bx += 70;
        d->buttons[0] = b;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "No");
        d->buttons[1] = b;
        d->buttons[2] = NULL;
        break;
    case EDITOR_DIALOG_YES_NO_CANCEL:
        bx = half_w - 95;
        by = h - 30;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Yes");
        bx += 70;
        d->buttons[0] = b;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "No");
        bx += 70;
        d->buttons[1] = b;
        EDITOR_DIALOG_BUTTON(b, window, bx, by, "Cancel");
        d->buttons[2] = b;
        break;
    }

    d->window = window;               // stored a second time (0x431563)
    list_append(&editor_dialog_list, d);
    return window;
}

// 0x4315c0  Confidence: exact (VC6 byte match, D19300: each button's block has its own press test and break;
// 8966 cross-jumps the identical tails, so button 1 jumps back into button 0's call). The callback of every dialog button: finds the dialog that owns
// the gadget id and calls the matching callback; a press (event 12) closes the dialog. Game bug: for the
// third button it tests callbacks[2] but calls callbacks[1] (`call [esi + 0x28]` at 0x431628).
bool editor_dialog_handle_event(int event, int value)
{
    bool result = false;
    editor_dialog *d;

    for (d = editor_dialog_list.next; d != &editor_dialog_list; d = d->next) {
        ui_gadget *g = d->window->find_gadget_by_id(value);
        if (g == NULL)
            continue;
        if (g == d->buttons[0]) {
            if (d->callbacks[0])
                result = d->callbacks[0](event, value);
            if (event == UI_EVENT_PRESSED) {
                d->closed = true;
                return true;
            }
            break;
        }
        if (g == d->buttons[1]) {
            if (d->callbacks[1])
                result = d->callbacks[1](event, value);
            if (event == UI_EVENT_PRESSED) {
                d->closed = true;
                return true;
            }
            break;
        }
        if (g == d->buttons[2]) {
            if (d->callbacks[2])
                result = d->callbacks[1](event, value);
            if (event == UI_EVENT_PRESSED) {
                d->closed = true;
                return true;
            }
            break;
        }
        break;
    }
    return result;
}

// 0x431640  Confidence: exact. GameTick, once per frame before FrameEnd.
void editor_dialogs_free_closed()
{
    editor_dialog *d, *cur;

    for (d = editor_dialog_list.next; d != &editor_dialog_list; ) {
        cur = d;
        d = d->next;
        if (cur->closed) {
            editor_dialog_destroy(cur);
            list_remove(cur);
            delete cur;
        }
    }
}

// 0x4316a0  Confidence: exact. Unlinks the window from the UI window list by hand, calls its destroy
// (vtable slot 0, which unlinks it again), then deletes the window and the five gadgets. The destructors
// are non-virtual (ui_window 0x52b0a0; the implicit ui_static / ui_button destructors below).
void editor_dialog_destroy(editor_dialog *d)
{
    list_remove(d->window);
    d->window->dtor();
    delete d->window;
    delete d->title;
    delete d->text;
    delete d->buttons[0];
    delete d->buttons[1];
    delete d->buttons[2];
}

// 0x431780  ui_static::~ui_static, implicit: ~ui_state_bitmap on +0x3a4 and +0x38c, then ~ui_gadget
// (0x52c410). The COMDAT is emitted after its first use above. Confidence: exact (compiler-generated, D3100).
// 0x4317e0  ui_button::~ui_button, implicit: ~ui_state_bitmap on +0x3f4 .. +0x394, then ~ui_gadget.
// Confidence: exact (compiler-generated, D3100).
