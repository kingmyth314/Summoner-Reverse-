// ui_container.cpp - Sum.exe v1.40, code 0x5534f0-0x553b9b (vsdk\ui\ui_container.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// The unit has no .data or .bss of its own (ui_container_colors 0x2cbfc20 and its initialiser
// init_ui_container_state 0x52f5f0 are in the vsdk\ui window/gadget unit; the static ui_container objects
// and their initialisers 0x4de810.. belong to the script editor). Its .rdata/.data strings are
// 0x5ad9e0-0x5ada4f. Float constants: 1.0f 0x57e990, 2.0f 0x580200, 6.0f 0x581a38, 7.0f 0x581bf8,
// 10.0f 0x57eb7c, 11.0f 0x581f64, 12.0f 0x581a2c (all float).

#include "stubs_credits_help.h"     // Color, gr_set_color_ptr, gr_string, gr_get_string_size,
                                    // gr_string_mode, bm_load, sprintf, strcpy, strlen
#include "stubs_container_targa.h"
#include "ui_container.h"

// Original __FILE__ of this unit (pushed by load_mask_bitmap, 0x5ad9f8).
#define UI_CONTAINER_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\ui\\ui_container.cpp"

// 0x5534f0  vtable slot 5. Confidence: exact. The children's result is ignored (EAX = key at the end).
int ui_container::process(int key)
{
    if (is_hidden() || is_disabled())
        return key;

    ui_gadget::process(key);
    return key;
}

// 0x553530  vtable slot 0. Confidence: exact. mask_data is left as it is.
void ui_container::dtor()
{
    if (mask_bm >= 0) {
        bm_unlock(mask_bm);
        mask_bm = -1;
    }
    ui_gadget::dtor();
}

// 0x553560  Confidence: likely (the three stores follow the vtable store in this order: +0x37c, +0x274,
// +0x378; an initialiser list would give the same code if the members were declared in that order,
// which they are not).
ui_container::ui_container()
{
    mask_data = 0;
    label[0] = '\0';
    mask_bm = -1;
}

// 0x553590  Confidence: likely. The eighth argument (1 from ui_parse_container, 0 from
// script_editor_create_ui) is never read: ui_gadget::init gets the member `owned` (byte +0x248) instead,
// presumably a name clash in the original (see NOTES.md). flags is stored at +0x374, the font at +0x380
// and the colour set at +0x384; mask_bm is not touched (the constructor set it).
void ui_container::create(ui_element *window, int x, int y, int w, int h, int id, int flags,
                          bool allocated)
{
    init(window, 1, x, y, w, h, id, owned);
    label[0] = '\0';
    this->flags = flags;
    font = -1;
    colors = ui_container_colors;
}

// 0x5535f0  Confidence: likely (store order of the node fields). Appends a node to the tooltip list;
// unlike ui_button__set_tooltip it does not remove an existing one first.
void ui_container::set_tooltip(const char *text, int font, int x, int y, int delay)
{
    ui_tooltip *t = new ui_tooltip;

    t->owner = this;
    strcpy(t->text, text);
    t->x = x;
    t->delay = delay;
    t->font = font;
    t->y = y;
    t->colors = ui_tooltip_colors;

    t->next = (ui_tooltip *)&tooltips;
    t->prev = tooltips.prev;
    tooltips.prev->next = t;
    tooltips.prev = t;
}

// 0x553680  Confidence: exact.
void ui_container::set_label(const char *label)
{
    strcpy(this->label, label);
}

// 0x5536b0  Confidence: exact. Error line 266 (0x10a).
void ui_container::load_mask_bitmap(const char *name)
{
    char error[256];
    int bm = bm_load(name, -1, -1);

    if (bm < 0) {
        sprintf(error, "Could not load in \"%s\"!", name);
        for (;;)
            DebugError(UI_CONTAINER_CPP, 266, error);
    }
    set_mask_bitmap(bm, name);
}

// 0x553710  Confidence: likely (mask_bm is stored between the pushes for bm_get_dimensions, which
// gets the argument; the width and height are not used). The message is formatted and never shown.
void ui_container::set_mask_bitmap(int bm, const char *name)
{
    int w, h;
    char error[256];

    if (bm == mask_bm)
        return;

    if (mask_bm >= 0)
        bm_unlock(mask_bm);

    mask_bm = bm;
    bm_get_dimensions(bm, &w, &h);
    if (!bm_lock(mask_bm, &mask_data, &bm_lock_format_unused)) {
        sprintf(error, "bm_lock() failed on bitmap \"%s\"\n", name);
        mask_bm = -1;
    }
}

// 0x5537a0  vtable slot 6. Confidence: exact. Mask byte under the screen point, -1 outside the gadget
// or without a mask.
int ui_container::get_mask_pixel(int x, int y)
{
    if (mask_data != 0) {
        x -= get_screen_x(0);
        y -= get_screen_y(0);
        if (x >= 0 && y >= 0 && x < w && y < h)
            return mask_data[y * w + x];
    }
    return -1;
}

// 0x553800  vtable slot 7. Confidence: likely (float expression shapes; the two one-point lines are in
// the binary as written). With a label: an etched frame (colour 0 one pixel in, colour 1 on the edge)
// that leaves a gap for the label 10 pixels from the left; without one, flag 1 draws a double bevel.
void ui_container::draw()
{
    gr::reset_clip();

    if (strlen(label) != 0) {
        int text_w, text_h;
        float sx = (float)get_screen_x(0);
        float sy = (float)get_screen_y(0);

        gr_get_string_size(&text_w, &text_h, label, -1, font);

        float text_x = sx + 10.0f;
        float text_y = sy - (float)(text_h / 2) + 1.0f;

        if (colors[3].a) {
            gr_set_color_ptr(&colors[3]);
            gr_string((int)(text_x + 1.0f), (int)(text_y + 1.0f), label, font, gr_string_mode);
        }
        gr_set_color_ptr(&colors[2]);
        gr_string((int)text_x, (int)text_y, label, font, gr_string_mode);

        float right = sx + w - 1.0f;
        float bottom = sy + h - 1.0f;

        gr_set_color_ptr(&colors[0]);
        gr_line(sx + 1.0f, sy + 1.0f, sx + 7.0f, sy + 1.0f, gr_line_mode);
        gr_line(sx + 1.0f, sy + 2.0f, sx + 1.0f, bottom - 1.0f, gr_line_mode);
        gr_line(sx + 1.0f, bottom, right, bottom, gr_line_mode);
        gr_line(right, sy + 1.0f, right, bottom - 1.0f, gr_line_mode);
        gr_line(sx + 7.0f, sy, sx + 7.0f, sy, gr_line_mode);
        if (sx + text_w + 12.0f <= right - 1.0f)
            gr_line(sx + text_w + 12.0f, sy + 1.0f, right - 1.0f, sy + 1.0f, gr_line_mode);

        gr_set_color_ptr(&colors[1]);
        gr_line(sx, sy, sx + 6.0f, sy, gr_line_mode);
        gr_line(sx, sy + 1.0f, sx, bottom, gr_line_mode);
        gr_line(sx + 2.0f, bottom - 1.0f, right - 1.0f, bottom - 1.0f, gr_line_mode);
        gr_line(right - 1.0f, sy + 2.0f, right - 1.0f, bottom - 2.0f, gr_line_mode);
        if (sx + text_w + 11.0f <= right) {
            gr_line(sx + text_w + 11.0f, sy, right, sy, gr_line_mode);
            gr_line(sx + text_w + 11.0f, sy + 1.0f, sx + text_w + 11.0f, sy + 1.0f, gr_line_mode);
        }
    } else if (flags & 1) {
        draw_bevel(&colors[1], &colors[0], 1, 0);
        draw_bevel(&colors[0], &colors[1], 1, 1);
    }

    ui_gadget::draw();
}
