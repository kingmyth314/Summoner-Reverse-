// new_credits.cpp - screen 21, the scrolling credits (Sum.exe v1.40, 0x408590-0x408d72).
// Written from the Sum.exe disassembly only (tools/asm.mjs). Target: VC++ 6.0 SP (cl 12.00.8966), /O2.
//
// Confidence per function: exact = every instruction accounted for and the source shape is the
// obvious one; likely = same behaviour and probably the same code, but register/stack choices or
// an equivalent source spelling could differ; unsure = behaviour right, shape guessed.

#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/frametime.h"
#include "vsdk/parse/parse.h"
#include "vsdk/os/movie.h"
#include "summoner/interface/new_credits.h"
#include "experimental/ui_shift.h"   // EXPERIMENTAL_BUILD only (widescreen); keep it the last include

// ---------------------------------------------------------------- .rdata (0x57eb34-0x57eb7b)
// Per resolution_index (0 = 640, 1 = 800, 2 = 1024) layout of the credits column.
const int credits_text_width[3]        = { 464, 580, 742 };  // 0x57eb34 wrap width and left-aligned block width
const int credits_column_x[3]          = { 380, 475, 608 };  // 0x57eb40 centre of the text column
const int credits_fade_top_start[3]    = {  32,  40,  51 };  // 0x57eb4c alpha 0 at this y
const int credits_fade_top_end[3]      = {  96, 120, 154 };  // 0x57eb58 alpha 1 from this y
const int credits_fade_bottom_start[3] = { 384, 480, 614 };  // 0x57eb64 alpha 1 up to this y
const int credits_fade_bottom_end[3]   = { 448, 560, 717 };  // 0x57eb70 alpha 0 at this y
// The float literals 10.0f (0x57eb7c), 255.0f (0x57eb80) and 0.0f (0x57eb84) follow: they are
// the compiler's __real@ constants, first emitted by this object.

// ---------------------------------------------------------------- .data (0x58a0e8-0x58a157)
// The two strings are stored in reverse order (1024 at 0x58a0f4, 640 at 0x58a10c), as VC6 does
// for the literals of one initialiser.
char *credits_bkgrnd_names[2] = { "Credits-Bkgrnd640.tga", "Credits-Bkgrnd1024.tga" };  // 0x58a0e8
float credits_scroll_speed = 20.0f;                                                       // 0x58a0f0 pixels per second at 640

// ---------------------------------------------------------------- .bss (0x5bc3f8-0x5bc413)
char *credits_lines;                       // 0x5bc3f8 0x2000-byte buffer of NUL-separated lines
int credits_left_align;                    // 0x5bc3fc set by a '@' line, cleared by a '-' line
unsigned int credits_saved_mempool_offset; // 0x5bc400 level_mempool.offset before credits_lines was allocated
int credits_line_count;                    // 0x5bc404
RealtimeTimer credits_timer;               // 0x5bc408 fade-out timer; constructed by init_realtimetimer_5bc408
int credits_bkgrnd_bm;                     // 0x5bc40c
float credits_scroll_y;                    // 0x5bc410 y of the first line

// The compiler-generated dynamic initialiser pair for credits_timer above (no source of their own):
// 0x408590 init_realtimetimer_5bc408_thunk (jmp, static_initializers entry 11). Confidence: exact.
// 0x4085a0 init_realtimetimer_5bc408 (mov ecx, 0x5bc408 / jmp RealtimeTimer::RealtimeTimer). Confidence: exact.

// 0x4085b0, screen 21 init. Confidence: exact
void credits_init()
{
    credits_bkgrnd_bm = bm_load(credits_bkgrnd_names[ui_hires], -1, -1);
    credits_saved_mempool_offset = level_mempool.offset;
    credits_parse_table();
    credits_scroll_y = (float)gr_screen_height() + 10.0f;
    music_play(music_find_by_name("Credits"), 1);
    credits_timer.invalidate();
}

// 0x408620, reads the "#<language>" section of credits.tbl into credits_lines, wrapping lines
// wider than the column. Confidence: exact (VC6 byte match, D24502: the piece length written out as
// `end - start` three times instead of a len local, so start is not coalesced with the zero register;
// see the '&' test below).
void credits_parse_table()
{
    TextFile tf;
    TextFilePosition pos;
    char section[64];
    int starts[16];
    char line[512];
    int lengths[16];
    int w, h;
    int out;

    if (tf.Open("credits.tbl", 9999999)) {
        credits_exit();
        return;
    }

    strcpy(section, "#");
    strcat(section, localize_get_language_name());
    if (section[0]) {
        char *p = section;
        do {
            *p = tolower(*p);
            p++;
        } while (*p);
    }

    tf.Rewind();
    tf.SkipTo(section, 0);
    tf.Expect(section);

    set_current_mempool(&level_mempool);
    credits_lines = new char[0x2000];
    set_current_mempool(0);

    credits_line_count = 0;
    out = 0;
    while (tf.ReadLine(line, 0x200)) {
        localize_remap_charset(line, 1);
        credits_get_line_size(&w, &h, line);
        if (w > credits_text_width[resolution_index]) {
            int start = 0;
            int count = gr_split_str(lengths, starts, line, credits_text_width[resolution_index], 16, 0, -1);
            for (int i = 0; i < count; i++) {
                int end;
                if (i == count - 1)
                    end = strlen(line);
                else
                    end = starts[i + 1];
                strncpy(credits_lines + out, line + start, end - start);
                credits_lines[out + end - start] = 0;
                out += end - start + 1;
                // The binary tests credits_lines[start], i.e. the output buffer indexed by the
                // input offset (probably meant line[start]). A '&' piece shares the previous row.
                if (credits_lines[start] != '&')
                    credits_line_count++;
                start = end;
            }
        } else {
            int len = strlen(line);
            strncpy(credits_lines + out, line, len);
            credits_lines[out + len] = 0;
            out += len + 1;
            credits_line_count++;
        }
        tf.GetPosition(&pos);
        if (tf.Accept("#end"))
            break;
        tf.SetPosition(&pos);
    }
    credits_lines[out] = 0;
    tf.Close();
}

// 0x408970. Confidence: exact
void credits_exit()
{
    gameseq_set_state(1);
}

// 0x408980, a leading '*' (heading marker) is not measured. Confidence: exact
void credits_get_line_size(int *w, int *h, char *s)
{
    if (*s == '*')
        gr_get_string_size(w, h, s + 1, -1, -1);
    else
        gr_get_string_size(w, h, s, -1, -1);
}

// 0x4089c0, screen 21 leave: frees credits_lines by rewinding the level pool. Confidence: exact
void credits_leave()
{
    gr_fade_in(0);
    level_mempool.offset = credits_saved_mempool_offset;
    music_play(music_find_by_name("Title"), 1);
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool credits_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x4089f0, screen 21 per-frame input. Confidence: exact (the switch might have been written
// with case 0 as its own case rather than merged with default; same code either way)
int credits_frame(int key)
{
    if (credits_timer.valid()) {
        if (credits_timer.elapsed()) {
            gr_fade_in(0);
            music_stop(0);
            movie_play("geeks.bik");
            credits_exit();
        }
        return key;
    }

    float scale;
    switch (resolution_index) {
    case 2:
        scale = 1.6f;
        break;
    case 1:
        scale = 1.25f;
        break;
    case 0:
        scale = 1.0f;
        break;
    default:
        scale = 1.0f;
        break;
    }
    credits_scroll_y -= credits_scroll_speed * reported_frametime * scale;

    switch (key) {
    case KEY_ESC:
        credits_finish();
        return 0;
    }
    return key;
}

// 0x408a90, start the 500 ms fade out; credits_frame plays geeks.bik when the 800 ms timer ends.
// Confidence: exact
void credits_finish()
{
    gr_fade_in(0);
    gr_fade_out(500);
    music_stop(500);
    credits_timer.SetTimeout(800);
}

// 0x408ac0, screen 21 draw. Confidence: exact (VC6 byte match, match24; was likely)
void credits_draw()
{
    gr_set_color(255, 255, 255, 255);
    ui_bitmap_scaled(credits_bkgrnd_bm, 0, 0, gr_bitmap_wrap_mode);

    int screen_h = gr_screen_height();
    int font_h = gr_get_font_height(-1);
    int y = (int)credits_scroll_y;
    char *s = credits_lines;
    int w, h;

    credits_left_align = 0;
    for (int i = 0; i < credits_line_count && y <= screen_h; i++) {
        credits_get_line_size(&w, &h, s);
        y = credits_draw_lines(w, y, s) + font_h;
        s += strlen(s) + 1;
    }

    if (!credits_timer.valid() && y < -20)
        credits_finish();
}

// 0x408ba0, draws one line. Prefixes:
//   '-' ends '@' mode; '!' this line left-aligned; '@' this and the following lines left-aligned
//   then '&' right column on the previous row, '^' right column on this row
//   then '*' heading colour and half a line of extra space after it.
// Lines fade in and out near the top and bottom of the screen. Returns the y used.
// Confidence: exact (D14602: the clamp is float_min(float_max(alpha, 0), 1), vector.h's inline helpers,
// whose by-value argument keeps alpha in the dead y argument slot)
int credits_draw_lines(int w, int y, char *s)
{
    char *p = s;
    int heading = 0;
    int column;
    int x;

    if (*s == '-')
        credits_left_align = 0;

    if (*s != '!' && *s != '@' && credits_left_align == 0) {
        column = credits_column_x[resolution_index];
        x = column - w / 2;
    } else {
        column = credits_column_x[resolution_index];
        x = column - credits_text_width[resolution_index] / 2;
        if (*s == '@')
            credits_left_align = 1;
        if (*s == '!' || *s == '@')
            p = s + 1;
    }

    if (*p == '&' || *p == '^') {
        x = column;
        if (*p == '&')
            y -= gr_get_font_height(-1);
        p++;
    }

    if (*p == '*') {
        gr_set_color_ptr(&ui_color_highlight);
        heading = 1;
        p++;
    } else {
        gr_set_color_ptr(&ui_color_text);
    }

    int mid = gr_get_font_height(-1) / 2 + y;
    float alpha = 1.0f;
    if (mid < credits_fade_top_end[resolution_index]) {
        alpha = (float)((float)(mid - credits_fade_top_start[resolution_index]) /
                (double)(credits_fade_top_end[resolution_index] - credits_fade_top_start[resolution_index]));
    } else if (mid > credits_fade_bottom_start[resolution_index]) {
        alpha = (float)((float)(credits_fade_bottom_end[resolution_index] - mid) /
                (double)(credits_fade_bottom_end[resolution_index] - credits_fade_bottom_start[resolution_index]));
    }
    gr_set_alpha((int)(float_min(float_max(alpha, 0.0f), 1.0f) * 255.0f));

    if (y > -20)
        gr_string(x, y, p, -1, gr_string_mode);

    if (heading)
        y += gr_get_font_height(-1) / 2;

    return y;
}
