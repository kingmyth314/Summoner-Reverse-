// unnamed_inputbox_0x5521a0.cpp - Sum.exe v1.40, code 0x5521a0-0x5534e1: the ui_inputbox gadget
// (vsdk\ui in sum.symbols; vtable 0x581f1c). The file name is a placeholder: no path string, no
// strings.tbl block; ui_container.cpp (0x5534f0) follows.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Unit range: after the CRC-32 object (0x552192) and before ui_container.cpp. No .data or .bss of its
// own (the colours are the UI manager's ui_inputbox_colors 0x2cbfc70); the character set string
// 0x581ebc is in .rdata. Only the script editor uses input boxes (multiplayer chat has its own code).

#include <string.h>
#include <ctype.h>
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/key.h"

#define KEY_HOME 0xc7

// The characters a valid / invalid set can hold, in the order the sets are rebuilt in (0x581ebc; '&'
// is missing between '%' and '^', so it can never be in a set).
static const char ui_inputbox_charset[] =
    " !\"#$%^'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

// 0x5521a0  Confidence: exact. Keypad digits (scan codes 0x47..0x52) as the digit keys; Num Lock
// state is not looked at.
int ui_inputbox::numpad_to_ascii(int key)
{
    switch (key) {
    case 0x52: return key_to_ascii(0x0b);   // keypad 0 -> 0
    case 0x4f: return key_to_ascii(0x02);   // 1
    case 0x50: return key_to_ascii(0x03);   // 2
    case 0x51: return key_to_ascii(0x04);   // 3
    case 0x4b: return key_to_ascii(0x05);   // 4
    case 0x4c: return key_to_ascii(0x06);   // 5
    case 0x4d: return key_to_ascii(0x07);   // 6
    case 0x47: return key_to_ascii(0x08);   // 7
    case 0x48: return key_to_ascii(0x09);   // 8
    case 0x49: return key_to_ascii(0x0a);   // 9
    default:   return key_to_ascii(key);
    }
}

// 0x552280  Confidence: exact.
bool ui_inputbox::is_char_allowed(char c)
{
    if (strlen(invalid_chars) != 0 && strchr(invalid_chars, c))
        return false;
    if (strlen(valid_chars) != 0 && !strchr(valid_chars, c))
        return false;
    if (char_filter)
        return char_filter(text, c);
    return true;
}

// 0x552310  Confidence: likely (the case order of the switch). A click on a hidden box only plays sound
// 4; a click on a shown one focuses it; only the focused box takes keys. Backspace, Left and Delete all
// remove the last character; Esc (unless UI_INPUTBOX_NO_ESCAPE) and Home clear the text.
int ui_inputbox::process(int key)
{
    char buf[0x100];
    int tw, th, c;

    if (is_disabled())
        return key;
    if (is_hidden()) {
        if (ui_left_pressed && is_mouse_over())
            ui_play_sound(4);
        return key;
    }
    if (ui_left_pressed && is_mouse_over())
        set_focus();
    if (ui_current_gadget != this)
        return key;
    if (key == 0)
        return 0;

    send_event(0x14, UI_USE_ID);
    switch (key) {
    case KEY_BACKSPACE:
    case KEY_LEFT:
    case KEY_DELETE:
        if (strlen(text) == 0) {
            ui_play_sound(5);
            return 0;
        }
        text[strlen(text) - 1] = '\0';
        send_event(0x15, UI_USE_ID);
        return 0;
    case KEY_ENTER:
        return 0;
    case KEY_ESC:
        if (flags & UI_INPUTBOX_NO_ESCAPE)
            return 0;
        // fall through
    case KEY_HOME:
        text[0] = '\0';
        send_event(0x15, UI_USE_ID);
        return 0;
    }

    if (key & (KEY_ALTED | KEY_CTRLED))
        return 0;
    c = numpad_to_ascii(key);
    if (c == 0 || c == 0xff)
        return 0;
    if (!is_char_allowed((char)c))
        return key;
    if ((int)strlen(text) >= max_length) {
        send_event(0x16, UI_USE_ID);
        return 0;
    }
    strcpy(buf, text);
    buf[strlen(text)] = (char)c;
    buf[strlen(text) + 1] = '\0';
    gr_get_string_size(&tw, &th, buf, -1, font);
    if ((flags & UI_INPUTBOX_FIT_WIDTH) && tw > w - margin * 2 - cursor_w) {
        send_event(0x16, UI_USE_ID);
        return 0;
    }
    strcpy(text, buf);
    send_event(0x15, UI_USE_ID);
    return 0;
}

// 0x5526a0  vtable slot 0 (sum.symbols ui_inputbox__vfunc0). Confidence: exact (jmp).
void ui_inputbox::dtor()
{
    ui_gadget::dtor();
}

// 0x5526b0  Confidence: exact.
ui_inputbox::ui_inputbox()
{
    text[0] = '\0';
    valid_chars[0] = '\0';
    invalid_chars[0] = '\0';
    char_filter = NULL;
    bm_cursor = -1;
    bm_normal = -1;
    bm_disabled = -1;
}

// 0x552700  Confidence: likely (the store order of the defaults).
void ui_inputbox::create(ui_element *window, int x, int y, int w, int h, int id, int flags, bool owned)
{
    init(window, UI_TYPE_INPUTBOX, x, y, w, h, id, owned);
    this->flags = flags;
    cursor_h = h - 2;
    text[0] = '\0';
    font = -1;
    valid_chars[0] = '\0';
    invalid_chars[0] = '\0';
    max_length = 0x50;
    password_char = '*';
    v_justify = 1;
    margin = 2;
    text_y = 0;
    cursor_y = 1;
    cursor_w = 2;
    colors = &ui_inputbox_colors.g0.c0;
}

// 0x5527b0  Confidence: likely (the unlink is written with locals: the NULL stores come before the
// neighbours' updates). Replaces the first tooltip, as ui_button::set_tooltip.
void ui_inputbox::set_tooltip(const char *text, int x, int y, int delay)
{
    ui_tooltip *t = tooltips.next;

    if (t != &tooltips) {
        ui_tooltip *prev = t->prev;
        ui_tooltip *next = t->next;
        t->next = NULL;
        t->prev = NULL;
        prev->next = next;
        next->prev = prev;
    }
    t = new ui_tooltip;
    t->owner = this;
    strcpy(t->text, text);
    t->x = x;
    t->delay = delay;
    t->next = &tooltips;
    t->colors = &ui_tooltip_colors.c0;
    t->y = y;
    t->font = -1;
    t->prev = tooltips.prev;
    tooltips.prev = t;
    t->prev->next = t;
}

// The range a-b of a set (either order).
#define INPUTBOX_MARK_RANGE(a, b)                                       \
    {                                                                   \
        char lo = (a) < (b) ? (a) : (b);                                \
        char hi = (a) < (b) ? (b) : (a);                                \
        if (lo <= hi)                                                   \
            memset(&allowed[(unsigned char)lo], 1, hi - lo + 1);        \
    }

// 0x552870  Confidence: likely (the case order of the five switches; the range code may have been a
// macro, as here). A state machine over the set string: 0 start, 1 / 3 / 5 a digit / capital / small
// letter pending, 2 / 4 / 6 the same followed by '-', 8 another character pending, 7 that followed by
// '-'. A range needs both ends of the same class; "x--" adds x and '-'. In state 7 the next character
// is dropped (only the '-' is added: "!-?" gives "!-"). Returns 0, without touching the box, for a
// malformed set ("a-", "a-5", "x-5").
int ui_inputbox::set_valid_chars(const char *chars)
{
    char allowed[128];
    int state = 0;
    unsigned int len, i, j;
    int n;

    if (chars == NULL)
        return 0;
    len = strlen(chars);
    memset(allowed, 0, sizeof(allowed));
    for (i = 0; i <= len; i++) {
        if (chars[i] == '\0') {
            switch (state) {
            case 0:
                break;
            case 2:
            case 4:
            case 6:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                break;
            }
        } else if (isdigit(chars[i])) {
            switch (state) {
            case 0:
                state = 1;
                break;
            case 2:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 4:
            case 6:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 1;
                break;
            }
        } else if (isupper(chars[i])) {
            switch (state) {
            case 0:
                state = 3;
                break;
            case 4:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 2:
            case 6:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 3;
                break;
            }
        } else if (islower(chars[i])) {
            switch (state) {
            case 0:
                state = 5;
                break;
            case 6:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 2:
            case 4:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 5;
                break;
            }
        } else {
            switch (state) {
            case 0:
                state = 8;
                break;
            case 1:
                if (chars[i] == '-') {
                    state = 2;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 2:
            case 4:
            case 6:
                if (chars[i] != '-')
                    return 0;
                allowed['-'] = 1;
                allowed[(unsigned char)chars[i - 2]] = 1;
                state = 0;
                break;
            case 3:
                if (chars[i] == '-') {
                    state = 4;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 5:
                if (chars[i] == '-') {
                    state = 6;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 7:
                allowed['-'] = 1;
                state = 0;
                break;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = chars[i] == '-' ? 7 : 8;
                break;
            }
        }
    }

    n = 0;
    for (j = 0; j < strlen(ui_inputbox_charset); j++) {
        if (allowed[(unsigned char)ui_inputbox_charset[j]])
            valid_chars[n++] = ui_inputbox_charset[j];
    }
    valid_chars[n] = '\0';
    return 1;
}

// 0x552c50  Confidence: likely. set_valid_chars into the invalid set (the same code).
int ui_inputbox::set_invalid_chars(const char *chars)
{
    char allowed[128];
    int state = 0;
    unsigned int len, i, j;
    int n;

    if (chars == NULL)
        return 0;
    len = strlen(chars);
    memset(allowed, 0, sizeof(allowed));
    for (i = 0; i <= len; i++) {
        if (chars[i] == '\0') {
            switch (state) {
            case 0:
                break;
            case 2:
            case 4:
            case 6:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                break;
            }
        } else if (isdigit(chars[i])) {
            switch (state) {
            case 0:
                state = 1;
                break;
            case 2:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 4:
            case 6:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 1;
                break;
            }
        } else if (isupper(chars[i])) {
            switch (state) {
            case 0:
                state = 3;
                break;
            case 4:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 2:
            case 6:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 3;
                break;
            }
        } else if (islower(chars[i])) {
            switch (state) {
            case 0:
                state = 5;
                break;
            case 6:
                INPUTBOX_MARK_RANGE(chars[i - 2], chars[i]);
                state = 0;
                break;
            case 2:
            case 4:
            case 7:
                return 0;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = 5;
                break;
            }
        } else {
            switch (state) {
            case 0:
                state = 8;
                break;
            case 1:
                if (chars[i] == '-') {
                    state = 2;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 2:
            case 4:
            case 6:
                if (chars[i] != '-')
                    return 0;
                allowed['-'] = 1;
                allowed[(unsigned char)chars[i - 2]] = 1;
                state = 0;
                break;
            case 3:
                if (chars[i] == '-') {
                    state = 4;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 5:
                if (chars[i] == '-') {
                    state = 6;
                } else {
                    allowed[(unsigned char)chars[i - 1]] = 1;
                    state = 8;
                }
                break;
            case 7:
                allowed['-'] = 1;
                state = 0;
                break;
            default:
                allowed[(unsigned char)chars[i - 1]] = 1;
                state = chars[i] == '-' ? 7 : 8;
                break;
            }
        }
    }

    n = 0;
    for (j = 0; j < strlen(ui_inputbox_charset); j++) {
        if (allowed[(unsigned char)ui_inputbox_charset[j]])
            invalid_chars[n++] = ui_inputbox_charset[j];
    }
    invalid_chars[n] = '\0';
    return 1;
}

// 0x553030  Confidence: exact.
int ui_inputbox::set_text(const char *text)
{
    if (text == NULL) {
        this->text[0] = '\0';
        return 1;
    }
    if (strlen(text) > (unsigned int)max_length)
        return 0;
    strcpy(this->text, text);
    return 1;
}

// 0x5530a0  Confidence: exact.
char *ui_inputbox::get_text()
{
    return text;
}

// 0x5530b0  Confidence: likely (the tails the three justifications share). Not drawn when disabled.
// Text that does not fit is drawn from the longest tail that does. The widths are measured with the
// default font (-1) although the text is drawn with the box's font (process measures with the box's
// font). With a negative width left the tail pointer starts past the terminator (n stays 1).
void ui_inputbox::draw()
{
    char buf[0x100];
    int sx = get_screen_x(0);
    int sy = get_screen_y(0);
    int clip_w = w - margin * 2;
    int clip_h = h - text_y * 2;
    int avail, len, tw, th, n, y;
    Color *c;
    int bm;
    char *s;

    if (is_disabled())
        return;
    gr::reset_clip();
    if (is_hidden()) {
        c = colors + 6;
        bm = bm_disabled;
    } else {
        c = colors;
        bm = bm_normal;
    }
    avail = clip_w - cursor_w;
    if (bm >= 0) {
        gr_set_color(255, 255, 255, 255);
        gr_bitmap(bm, get_screen_x(0), get_screen_y(0), gr_bitmap_wrap_mode);
    } else {
        draw_sunken(c, 1);
        avail -= 2;
    }

    len = strlen(text);
    if (flags & UI_INPUTBOX_PASSWORD) {
        if (len > 0)
            memset(buf, password_char, len);
        buf[len] = '\0';
    } else {
        strcpy(buf, text);
    }
    gr_get_string_size(&tw, &th, buf, -1, -1);
    if (tw > avail) {
        tw = 0;
        n = 1;
        char *p = &buf[len - 1];
        while (tw <= avail) {
            gr_get_string_size(&tw, &th, p, -1, -1);
            n++;
            p--;
        }
        s = &buf[len - n + 2];
        gr_get_string_size(&tw, &th, s, -1, -1);
    } else {
        s = buf;
    }

    gr::set_clip(sx + margin, sy + text_y, clip_w, clip_h);
    switch (v_justify) {
    case 0:
        if (c[4].a) {
            gr_set_color_ptr(&c[4]);
            gr_string(1, 1, s, font, gr_string_mode);
        }
        gr_set_color_ptr(&c[3]);
        gr_string(0, 0, s, font, gr_string_mode);
        break;
    case 2:
        y = clip_h - gr_get_font_height(-1);
        if (c[4].a)
            y--;
        if (y < 0)
            y = 0;
        if (c[4].a) {
            gr_set_color_ptr(&c[4]);
            gr_string(1, y + 1, s, font, gr_string_mode);
        }
        gr_set_color_ptr(&c[3]);
        gr_string(0, y, s, font, gr_string_mode);
        break;
    default:
        y = clip_h / 2 - (gr_get_font_height2(-1) + 1) / 2;
        if (y < 0)
            y = 0;
        if (c[4].a) {
            gr_set_color_ptr(&c[4]);
            gr_string(1, y + 1, s, font, gr_string_mode);
        }
        gr_set_color_ptr(&c[3]);
        gr_string(0, y, s, font, gr_string_mode);
        break;
    }

    if (is_hidden() || ui_current_gadget != this)
        return;
    gr::set_clip(sx + margin, sy + cursor_y, clip_w, h - cursor_y);
    if (bm_cursor >= 0) {
        gr_set_color(255, 255, 255, 255);
        gr_bitmap(bm_cursor, tw, 0, gr_bitmap_wrap_mode);
    } else {
        gr_set_color_ptr(&colors[5]);
        for (int i = 0; i < cursor_w; i++)
            gr_line((float)(i + tw), 0.0f, (float)(i + tw), (float)(cursor_h - 1), gr_line_mode);
    }
}

// 0x5534c0  vtable slot 8. Confidence: exact.
bool ui_inputbox::is_selectable()
{
    return is_shown() && !is_disabled();
}
