// unnamed_dialogue_box_0x4d6660.cpp - Sum.exe v1.40, code 0x4d6660-0x4d7b2f: the conversation screen's
// text box and topic list (file name not known: a placeholder, see src/decomp/README.md).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the vtables and .rdata floats read from
// Summoner/Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: the initialiser run 0x4d6660-0x4d68ba (table entries 359-368, 0x5885a0-0x5885c4) after
// level_scripts.cpp's cmd_door_toggle (0x4d6600), to dialogue_show_line's RET (0x4d7b2b);
// conversation.cpp starts at 0x4d7b30 (entry 369, conversation_db 0x25461a0). No path string and no
// strings.tbl header: ids 949 / 950 ("more", "end") sit in the untitled block after new_mainmenu.cpp's 948
// (docs/sum-exe/source-map.md row 0x4d6770-0x4d7900), so the file name is not known.
// Data: no .data of its own; .rdata 0x581c4c-0x581c6f (the five vtables) and the scroll speed 1/7 px per
// ms (0x581c74; 0.0f at 0x57eb84 is pooled). .bss 0x25443b0-0x2546197: the colours, the two boxes, the
// two text slots and dialogue_selected_topic. 0x25443bc-0x25443fb (0x40 bytes) is inside the block but no
// instruction of Sum.exe reads or writes it (not defined here; NOTES).
//
// The definitions are in initialiser order (entries 359-368: text box, topic box, blue, green, red, grey,
// yellow, highlight, "more", "end"), which is the order VC6 emits the dynamic initialisers in.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/os/mouse.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/localize/localize.h"
#include "summoner/interface/interface.h"
#include "levelscripts/conversation.h"

#define DIALOGUE_RING      64            // records per box (the % 64 of every index)
#define DIALOGUE_LABELS    32
#define DIALOGUE_MAX_WRAP  16            // gr_split_str's line limit
#define DIALOGUE_SCROLL_SPEED 0.142857149f   // 0x581c74: pixels per ms of the scroll animation

// ---- .bss ----------------------------------------------------------------------------------------
// 0x4d6660 static_init_dialogue_box_2544408 (call 0x4d6670; jmp 0x4d6680) /
// 0x4d6670 init_dialogue_box_2544408 (mov ecx, 0x2544408; jmp dialogue_box__ctor) /
// 0x4d6680 register_atexit_dialogue_box_2544408 (atexit(0x4d6690)) /
// 0x4d6690 atexit_dtor_dialogue_box_2544408
// (mov ecx, 0x2544408; jmp 0x4d6720): the compiler's dynamic initialiser and atexit helpers of this
// definition. Confidence: exact.
// 0x4d6720 dialogue_box__dtor (jmp dialogue_box_base__dtor): the implicit destructor of both boxes (one
// body for the two classes, identical-code folding; the atexit helpers of both jump to it). Confidence: likely.
dialogue_box dialogue_text_box;                      // 0x2544408
// 0x4d66e0 static_init_dialogue_topic_box_2545b68 /
// 0x4d66f0 init_dialogue_topic_box_2545b68 /
// 0x4d6700 register_atexit_dialogue_topic_box_2545b68 /
// 0x4d6710 atexit_dtor_dialogue_topic_box_2545b68 (the same
// helpers for the topic list). Confidence: exact.
dialogue_topic_list dialogue_topic_box;              // 0x2545b68
// 0x4d6760 static_init_color_2544400 (jmp) /
// 0x4d6770 init_color_2544400. Confidence: exact.
Color dialogue_color_blue(107, 154, 184, 255);       // 0x2544400
// 0x4d6790 static_init_color_2545b64 /
// 0x4d67a0 init_color_2545b64. Confidence: exact.
Color dialogue_color_green(95, 197, 88, 255);        // 0x2545b64
// 0x4d67c0 static_init_color_25443b0 /
// 0x4d67d0 init_color_25443b0. Confidence: exact.
Color dialogue_color_red(150, 80, 80, 255);          // 0x25443b0
// 0x4d67f0 static_init_color_2545b5c /
// 0x4d6800 init_color_2545b5c. Confidence: exact.
Color dialogue_color_gray(180, 180, 180, 255);       // 0x2545b5c
// 0x4d6820 static_init_color_25443b8 /
// 0x4d6830 init_color_25443b8. Confidence: exact.
Color dialogue_color_yellow(255, 255, 158, 255);     // 0x25443b8
// 0x4d6850 static_init_color_2546194 /
// 0x4d6860 init_color_2546194. Confidence: exact.
Color dialogue_color_highlight(255, 255, 158, 255);  // 0x2546194
// 0x4d6880 static_init_dialogue_more_text (jmp) /
// 0x4d6890 init_dialogue_more_text (a dynamic store of
// the slot address, as ai.cpp's and new_status.cpp's XSTR_SLOT tables). Confidence: likely.
char **dialogue_more_text = XSTR_SLOT("more", 949);  // 0x2545b58
// 0x4d68a0 static_init_dialogue_end_text (jmp) /
// 0x4d68b0 init_dialogue_end_text. Confidence: likely.
char **dialogue_end_text = XSTR_SLOT("end", 950);    // 0x25443fc
int dialogue_selected_topic;                         // 0x25443b4 (no initialiser)

// ---- the line records ------------------------------------------------------------------------------
// 0x4d68c0  Confidence: exact (VC6 byte match) (the in-quote flag starts as a branch `cont || *text == '"'`; arg 5 unread).
bool dialogue_line::split_quotes(int *count, int *lengths, int *starts, const char *text, int max, bool in_quote)
{
    bool quoted = in_quote || *text == '"';
    const char *p = text;
    const char *q;

    *count = 0;
    if (*p != 0) {
        do {
            starts[*count] = p - text;
            if (quoted) {
                // the opening quote is part of the segment unless the line continues one
                if (*count == 0 && in_quote)
                    q = strchr(p, '"');
                else
                    q = strchr(p + 1, '"');
                if (q) {
                    lengths[*count] = q - p + 1;
                    quoted = false;
                } else {
                    lengths[*count] = strlen(p);
                }
            } else {
                q = strchr(p, '"');
                if (q) {
                    lengths[*count] = q - p;
                    quoted = true;
                } else {
                    lengths[*count] = strlen(p);
                }
            }
            p += lengths[*count];
            (*count)++;
        } while (*p);
    }
    return quoted;
}

// 0x4d69d0  Confidence: exact (vtable 0x581c4c slot 2).
void dialogue_box::layout()
{
    int i;

    text_height = 0;
    for (i = 0; i < count; i++) {
        dialogue_line *l = &line[(first + i) % DIALOGUE_RING];
        l->top = text_height;
        text_height += l->height;
    }
}

// 0x4d6a20  Confidence: exact.
dialogue_label *dialogue_box::alloc_label()
{
    int i;

    for (i = 0; i < DIALOGUE_LABELS; i++)
        if (!label[i].used)
            return &label[i];
    return NULL;
}

// 0x4d6a50  Confidence: exact (the loop compares the text addresses, +1 into each label).
void dialogue_box::free_label(char *text)
{
    int i;

    for (i = 0; i < DIALOGUE_LABELS; i++) {
        if (label[i].text == text) {
            label[i].clear();
            return;
        }
    }
}

// 0x4d6a90  Confidence: exact (vtable 0x581c5c slot 2).
void dialogue_topic_list::layout()
{
    int i;

    text_height = 0;
    for (i = 0; i < count; i++) {
        dialogue_topic_entry *e = &entry[(first + i) % DIALOGUE_RING];
        e->top = text_height;
        text_height += e->height;
    }
}

// 0x4d6ad0  Confidence: exact.
void dialogue_line::init(const char *text, int width, int kind, conversation_slot *talker)
{
    int lengths[DIALOGUE_MAX_WRAP];
    int offsets[DIALOGUE_MAX_WRAP];

    this->width = width;
    this->text = text;
    this->kind = kind;
    if (kind == 0 || kind == 2)
        this->talker = talker;
    else
        this->talker = NULL;
    if (text && kind == 2)
        this->text = localize_topic_name(text);
    if (this->text) {
        int n = gr_split_str(lengths, offsets, this->text, this->width, DIALOGUE_MAX_WRAP, 0, -1);
        height = gr_get_font_height(-1) * n;
    }
    else
        height = gr_get_font_height(-1);
}

// 0x4d6b80  Confidence: likely (kinds 1 and 2 share the drawing loop after their colour, tail-merged here
// from two cases; the line loop runs on a down-counter).
void dialogue_line::draw(int scroll)
{
    char wrapped[DIALOGUE_MAX_WRAP][100];
    char segment[100];
    int lengths[DIALOGUE_MAX_WRAP];
    int offsets[DIALOGUE_MAX_WRAP];
    int seg_lengths[32];
    int seg_starts[32];
    int font_h = gr_get_font_height(-1);
    int n, i, j;

    if (!text)
        return;
    n = gr_split_str(lengths, offsets, text, width, DIALOGUE_MAX_WRAP, 0, -1);
    for (i = 0; i < n; i++) {
        strncpy(wrapped[i], text + offsets[i], lengths[i]);
        wrapped[i][lengths[i]] = 0;
    }
    switch (kind) {
    case 0: {
        // spoken text: the parts in quotes blue, the rest green; a quote can go on over the wrap
        bool in_quote = false;
        int y = 0;
        for (i = 0; i < n; i++) {
            bool quoted = in_quote || wrapped[i][0] == '"';
            int segs;
            int x;
            in_quote = split_quotes(&segs, seg_lengths, seg_starts, wrapped[i], 32, in_quote);
            x = 0;
            for (j = 0; j < segs; j++) {
                int w, h;
                strncpy(segment, wrapped[i] + seg_starts[j], seg_lengths[j]);
                segment[seg_lengths[j]] = 0;
                gr_set_color_ptr(quoted ? &dialogue_color_blue : &dialogue_color_green);
                gr_string(x, y + scroll + top, segment, -1, gr_bitmap_wrap_mode);
                gr_get_string_size(&w, &h, segment, -1, -1);
                x += w;
                quoted = !quoted;
            }
            y += font_h;
        }
        break;
    }
    case 1:
        gr_set_color_ptr(&dialogue_color_red);
        for (i = 0; i < n; i++)
            gr_string(0, i * font_h + scroll + top, wrapped[i], -1, gr_bitmap_wrap_mode);
        break;
    case 2:
        gr_set_color_ptr(&dialogue_color_gray);
        for (i = 0; i < n; i++)
            gr_string(0, i * font_h + scroll + top, wrapped[i], -1, gr_bitmap_wrap_mode);
        break;
    }
}

// 0x4d6de0  Confidence: exact (tests the topic's name, wraps its text).
void dialogue_topic_entry::init(conversation_topic *topic, int width, int state)
{
    int lengths[DIALOGUE_MAX_WRAP];
    int offsets[DIALOGUE_MAX_WRAP];

    this->topic = topic;
    this->width = width;
    this->state = state;
    if (topic && topic->name) {
        char *text = localize_topic_name(topic->text);
        int n = gr_split_str(lengths, offsets, text, this->width, DIALOGUE_MAX_WRAP, 0, -1);
        height = gr_get_font_height(-1) * n;
    }
    else
        height = gr_get_font_height(-1);
}

// 0x4d6e70  Confidence: likely (jump table 0x4d70d8 for states 3-6; the per-case locals). The "more" and
// "end" buttons test the mouse against the topic box's top left and the text size, and the bottom test
// compares the mouse x with y + h (game bug: `mx < box.y + h`, NOTES).
void dialogue_topic_entry::draw(int scroll)
{
    char wrapped[DIALOGUE_MAX_WRAP][100];
    int lengths[DIALOGUE_MAX_WRAP];
    int offsets[DIALOGUE_MAX_WRAP];
    int font_h = gr_get_font_height(-1);
    int i, n, y;

    switch (state) {
    case 3:
    case 4: {
        char *s;
        if (!topic)
            return;
        s = localize_topic_name(topic->text);
        n = gr_split_str(lengths, offsets, s, width, DIALOGUE_MAX_WRAP, 0, -1);
        for (i = 0; i < n; i++) {
            strncpy(wrapped[i], s + offsets[i], lengths[i]);
            wrapped[i][lengths[i]] = 0;
        }
        gr_set_color_ptr(state == 3 ? &dialogue_color_gray : &dialogue_color_yellow);
        for (i = 0, y = 0; i < n; i++, y += font_h)
            gr_string(0, y + top + scroll, wrapped[i], -1, gr_bitmap_wrap_mode);
        break;
    }
    case 5: {
        int mx, my, w, h;
        mouse_get_pos(&mx, &my, &game_mouse_z);
        gr_get_string_size(&w, &h, localize_string_or_invalid(dialogue_more_text), -1, -1);
        if (mx >= dialogue_topic_box.x && mx < dialogue_topic_box.x + w
            && my >= dialogue_topic_box.y && mx < dialogue_topic_box.y + h)
            gr_set_color_ptr(&dialogue_color_highlight);
        else
            gr_set_color_ptr(&dialogue_color_gray);
        gr_string(0, top + scroll, localize_string_or_invalid(dialogue_more_text), -1, gr_bitmap_wrap_mode);
        break;
    }
    case 6: {
        int mx, my, w, h;
        mouse_get_pos(&mx, &my, &game_mouse_z);
        gr_get_string_size(&w, &h, localize_string_or_invalid(dialogue_end_text), -1, -1);
        if (mx >= dialogue_topic_box.x && mx < dialogue_topic_box.x + w
            && my >= dialogue_topic_box.y && mx < dialogue_topic_box.y + h)
            gr_set_color_ptr(&dialogue_color_highlight);
        else
            gr_set_color_ptr(&dialogue_color_gray);
        gr_string(0, top + scroll, localize_string_or_invalid(dialogue_end_text), -1, gr_bitmap_wrap_mode);
        break;
    }
    }
}

// ---- the boxes ------------------------------------------------------------------------------------
// 0x4d70f0  Confidence: exact (VC6 byte match) (+0x20 / +0x24 are not cleared here; the stores follow the timer's
// constructor and the vtable store, as the compiler orders them).
dialogue_box_base::dialogue_box_base()
{
    x = 0;
    y = 0;
    scroll = 0;
    w = 0;
    text_height = 0;
    lines = 0;
    timer.invalidate();
}

// 0x4d7130  Confidence: exact (the destructor body is empty; VC6 restores the vtable).
dialogue_box_base::~dialogue_box_base()
{
}

// 0x4d7140 dialogue_box_base__reset  Confidence: exact (slot 0 of the base vtable).
void dialogue_box_base::reset()
{
    scroll = 0;
    scroll_target = 0;
    text_height = 0;
    first = 0;
    count = 0;
}

// 0x4d66a0  Confidence: exact (the line records' implicit constructors store only their vtable; the labels'
// clear the two bytes).
dialogue_box::dialogue_box()
{
}

// 0x4d6730  Confidence: exact.
dialogue_topic_list::dialogue_topic_list()
{
}

// 0x4d7160  Confidence: exact (virtual reset through the vtable).
void dialogue_box_base::set_rect(int x, int y, int w, int lines)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->lines = lines;
    reset();
}

// 0x4d7190  Confidence: likely (the two ring-full paths; the talker is not passed when the ring is full
// - game bug, NOTES).
dialogue_line *dialogue_box::add_text(const char *text, int kind, bool spacer, conversation_slot *talker)
{
    dialogue_line *l;
    dialogue_label *lb;

    // an empty line between two texts
    if (spacer && count > 0) {
        if (count == DIALOGUE_RING) {
            scroll += line[first].height;
            scroll_target += line[first].height;
            if (line[first].kind == 1)
                free_label((char *)line[first].text);
            line[first].init(NULL, w, kind, NULL);
            first = (first + 1) % DIALOGUE_RING;
        } else {
            line[(first + count) % DIALOGUE_RING].init(NULL, w, kind, NULL);
            count++;
        }
    }
    if (count == DIALOGUE_RING) {
        // the oldest line goes
        scroll += line[first].height;
        scroll_target += line[first].height;
        if (line[first].kind == 1)
            free_label((char *)line[first].text);
        if (kind == 1) {
            lb = alloc_label();
            if (!lb)
                return NULL;
            strcpy(lb->text, text);
            lb->used = 1;
            line[first].init(lb->text, w, 1, NULL);
        } else {
            line[first].init(text, w, kind, NULL);
        }
        l = &line[first];
        first = (first + 1) % DIALOGUE_RING;
        layout();
        return l;
    }
    if (kind == 1) {
        lb = alloc_label();
        if (!lb)
            return NULL;
        strcpy(lb->text, text);
        lb->used = 1;
        l = &line[(first + count) % DIALOGUE_RING];
        l->init(lb->text, w, 1, talker);
    } else {
        l = &line[(first + count) % DIALOGUE_RING];
        l->init(text, w, kind, talker);
    }
    count++;
    layout();
    return l;
}

// 0x4d7410  Confidence: exact (VC6 8966: the ring index as (first + i) % 64 and line[idx] fields, D7700).
// Returns the position in the ring order, not the ring index.
int dialogue_box::first_visible_line()
{
    int top_y = -(int)scroll;
    int i;

    for (i = 0; i < count; i++) {
        int idx = (first + i) % DIALOGUE_RING;
        if (line[idx].text && line[idx].height + line[idx].top > top_y)
            return i;
    }
    return -1;
}

// 0x4d7480  Confidence: likely (under 8966 EBX / EBP swap, D7702). Returns the ring index of the last line
// that starts above the bottom.
int dialogue_box::last_visible_line()
{
    int bottom = gr_get_font_height(-1) * lines - (int)scroll;
    int last = -1;
    int i;

    for (i = 0; i < count; i++) {
        int idx = (first + i) % DIALOGUE_RING;
        if (line[idx].top >= bottom)
            return last;
        last = idx;
    }
    return last;
}

// 0x4d74f0  Confidence: likely (min / max as the vsdk.h macros: inline compares, no imin / imax calls).
void dialogue_box::scroll_next_page()
{
    int i;

    scroll_target = (int)scroll - gr_get_font_height(-1) * lines;
    scroll_target = max(scroll_target, gr_get_font_height(-1) * lines - text_height);
    scroll_target = min(scroll_target, 0);
    for (i = 0; i < count; i++) {
        int idx = (first + i) % DIALOGUE_RING;
        if (-scroll_target >= line[idx].top && -scroll_target < line[idx].top + line[idx].height) {
            // a page that starts on a spacer starts on the line after it
            if (!line[idx].text)
                scroll_target = -line[(idx + 1) % DIALOGUE_RING].top;
            return;
        }
    }
}

// 0x4d7600  Confidence: exact (VC6 byte match) (operand order of the sum).
bool dialogue_box::line_fits(int i)
{
    dialogue_line *l = &line[i];
    int visible = gr_get_font_height(-1) * lines;
    return (int)scroll + l->height + l->top <= visible;
}

// 0x4d7650 dialogue_box__reset  Confidence: exact (slot 0 of the text box vtable).
void dialogue_box::reset()
{
    int i;

    dialogue_box_base::reset();
    for (i = 0; i < DIALOGUE_LABELS; i++)
        label[i].clear();
}

// 0x4d7680  Confidence: likely (the float expression forms). The visibility test compares the box height
// with the scroll, not with the line's top (always true while scroll <= 0); the clip hides the rest.
void dialogue_box::draw()
{
    int cx, cy, cw, ch;
    int i;

    if (count <= 0)
        return;
    if (timer.elapsed() && scroll_target != scroll) {
        float step = timer.GetElapsedTime() * DIALOGUE_SCROLL_SPEED;
        float target = (float)scroll_target;
        if (scroll < target) {
            scroll += step;
            if (scroll > target)
                scroll = target;
        } else {
            scroll -= step;
            if (scroll < target)
                scroll = target;
        }
    }
    if (scroll_target != scroll)
        timer.SetTimeout(0);
    else
        timer.invalidate();
    gr_get_clip(&cx, &cy, &cw, &ch);
    gr::set_clip(x, y, w, gr_get_font_height(-1) * lines);
    for (i = 0; i < count; i++) {
        dialogue_line *l = &line[(i + first) % DIALOGUE_RING];
        int visible = gr_get_font_height(-1) * lines;
        if (visible > scroll && (float)l->height + l->top + scroll >= 0.0f)
            l->draw((int)scroll);
    }
    gr::set_clip(cx, cy, cw, ch);
}

// 0x4d7800  Confidence: likely (under 8966 the argument loads are scheduled differently, D7702).
dialogue_topic_entry *dialogue_topic_list::add(conversation_topic *topic, int kind)
{
    dialogue_topic_entry *e;

    if (count == DIALOGUE_RING) {
        scroll += entry[first].height;
        scroll_target += entry[first].height;
        entry[first].init(topic, w, kind);
        e = &entry[first];
        first = (first + 1) % DIALOGUE_RING;
        layout();
        return e;
    }
    e = &entry[(first + count) % DIALOGUE_RING];
    e->init(topic, w, kind);
    count++;
    layout();
    return e;
}

// 0x4d78b0  Confidence: exact (VC6 byte match) (returns through EAX = 1 / 0).
bool dialogue_topic_list::item_visible(int item)
{
    dialogue_topic_entry *e = &entry[item];
    int bottom = (int)scroll + e->height + e->top;

    return bottom > 0 && bottom <= gr_get_font_height(-1) * lines;
}

// 0x4d7900  Confidence: exact.
int dialogue_topic_list::item_y(int item)
{
    return (int)scroll + entry[item].top;
}

// 0x4d7920 dialogue_topic_list__reset  Confidence: exact (slot 0 of the topic list vtable: jmp 0x4d7140).
void dialogue_topic_list::reset()
{
    dialogue_box_base::reset();
}

// 0x4d7930  Confidence: likely (as dialogue_box::draw over the topic entries).
void dialogue_topic_list::draw()
{
    int cx, cy, cw, ch;
    int i;

    if (count <= 0)
        return;
    if (timer.elapsed() && scroll_target != scroll) {
        float step = timer.GetElapsedTime() * DIALOGUE_SCROLL_SPEED;
        float target = (float)scroll_target;
        if (scroll < target) {
            scroll += step;
            if (scroll > target)
                scroll = target;
        } else {
            scroll -= step;
            if (scroll < target)
                scroll = target;
        }
    }
    if (scroll_target != scroll)
        timer.SetTimeout(0);
    else
        timer.invalidate();
    gr_get_clip(&cx, &cy, &cw, &ch);
    gr::set_clip(x, y, w, gr_get_font_height(-1) * lines);
    for (i = 0; i < count; i++) {
        dialogue_topic_entry *e = &entry[(first + i) % DIALOGUE_RING];
        int visible = gr_get_font_height(-1) * lines;
        if (visible > scroll && (float)e->height + e->top + scroll >= 0.0f)
            e->draw((int)scroll);
    }
    gr::set_clip(cx, cy, cw, ch);
}

// ---- free helpers ----------------------------------------------------------------------------------
// 0x4d7ab0  Confidence: exact (mov ecx, box; jmp 0x4d7920: a direct call of the topic list's reset).
void dialogue_topic_box_reset()
{
    dialogue_topic_box.reset();
}

// 0x4d7ac0  Confidence: exact.
int dialogue_topic_box_add_kind3(conversation_topic *topic)
{
    dialogue_topic_box.add(topic, 3);
    return 0;
}

// 0x4d7ae0  Confidence: exact (VC6 byte match) (the name goes through a local copy of the argument whose address is passed:
// `mov [esp+0xc], eax` stores it back into its own slot).
void dialogue_screen_open(const char *name)
{
    const char *arg = name;
    interface_set_screen(13, &arg);
}

// 0x4d7b00  Confidence: exact.
void dialogue_screen_close()
{
    interface_set_screen(0, 0);
}

// 0x4d7b10  Confidence: exact.
void dialogue_show_line(const char *text, int kind, conversation_slot *talker)
{
    dialogue_text_box.add_text(text, kind, true, talker);
}
