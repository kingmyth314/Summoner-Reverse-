// gr_font.cpp - Sum.exe v1.40, code 0x5081d0-0x50939a (vsdk\gr\gr_font.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1. Confidence per function: exact = expect identical code, likely = same
// semantics, small layout risk, unsure = the source shape is a guess.
// Dash Faction (rf::gr font functions) was used for names and argument lists only.

#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/vstring.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/localize/localize.h"

// One kern pair in Font.kern_data (3 bytes). The bytes are read with movsx, so they are plain
// (signed) char; types.h types the pointer as uint8_t*.
struct FontKernEntry {
    char ch0;       // first character, as a glyph index
    char ch1;       // second character, as a glyph index
    char offset;    // added to the first character's spacing
};

// ---- .bss, in address order (0x25d9238-0x25d94db) ------------------------------------------
Font gr_fonts[MAX_FONTS];           // 0x25d9238  0x6c bytes each
int gr_num_fonts;                   // 0x25d94c0
Font *gr_current_font;              // 0x25d94c4
Font *gr_default_font;              // 0x25d94c8
int gr_text_pos_x;                  // 0x25d94cc
int gr_text_pos_y;                  // 0x25d94d0
// 0x25d94d4 / 0x25d94d8: the two "" passed by gr_fit_string_String. They are .bss addresses, so
// not string literals; zero-filled char arrays read as "". Labels and sizes are ours.
static char gr_font_empty_line[4];  // 0x25d94d4
static char gr_font_empty_rest[4];  // 0x25d94d8

// 0x5081d0  Confidence: exact.
// Frees the seven tables of every used font. The bitmap (bm_handle) is not released and
// gr_num_fonts is not reset.
void gr_close_fonts()
{
    Font *f = gr_fonts;
    for (int i = 0; i < gr_num_fonts; f++, i++) {
        if (f->signature) {
            f->filename[0] = 0;
            if (f->kern_data) {
                delete f->kern_data;
                f->kern_data = NULL;
            }
            if (f->chars) {
                delete f->chars;
                f->chars = NULL;
            }
            if (f->pixel_data) {
                delete f->pixel_data;
                f->pixel_data = NULL;
            }
            if (f->palette) {
                delete f->palette;
                f->palette = NULL;
            }
            if (f->char_x_coords) {
                delete f->char_x_coords;
                f->char_x_coords = NULL;
            }
            if (f->char_y_coords) {
                delete f->char_y_coords;
                f->char_y_coords = NULL;
            }
            if (f->bm_bits) {
                delete f->bm_bits;
                f->bm_bits = NULL;
            }
            f->signature = 0;
        }
    }
}

// 0x508290  Confidence: exact (int result: callers test EAX).
int gr_font_is_whitespace(char c)
{
    if (c == ' ' || c == '\t' || c == '\n')
        return 1;
    return 0;
}

// 0x5082b0  Confidence: exact.
int gr_font_is_space(char c)
{
    if (c == ' ' || c == '\t')
        return 1;
    return 0;
}

// 0x5082d0  Confidence: exact.
// French typography puts a space before : ? !, so with the French table a space followed by one of
// these is not a break point (gr_split_str).
int gr_font_french_punct(char c)
{
    if (localize_french_punct && (c == ':' || c == '?' || c == '!'))
        return 1;
    return 0;
}

// 0x508300  Confidence: exact.
// One store to gr_current_font for both cases (EAX holds the pointer), so a conditional
// expression, not an if/else with two stores as in gr_string.
int gr_get_font_height(int font)
{
    gr_current_font = (font < 0) ? gr_default_font : &gr_fonts[font];
    if (gr_current_font)
        return gr_current_font->height;
    return 16;
}

// 0x508330  Confidence: exact.
int gr_get_font_height2(int font)
{
    gr_current_font = (font < 0) ? gr_default_font : &gr_fonts[font];
    if (gr_current_font)
        return gr_current_font->height2;
    return 16;
}

// 0x508360  Confidence: exact (VC6 byte match, match23; was likely; loop rotation; the compiler keeps c and the spacing in the
// dead argument slots).
// Width of the widest line and the height of all lines; len < 0 means the whole string.
void gr_get_string_size(int *out_w, int *out_h, const char *s, int len, int font)
{
    if (!s) {
        *out_w = 0;
        *out_h = 0;
        return;
    }

    gr_current_font = (font < 0) ? gr_default_font : &gr_fonts[font];
    if (len < 0)
        len = strlen(s);

    int h = gr_current_font->height;
    int x = 0;
    int max_w = 0;
    while (*s && len > 0) {
        char c = *s++;
        len--;
        if (c == '\n') {
            x = 0;
            if (*s)
                h += gr_current_font->height;
        } else {
            int width, spacing;
            gr_font_get_char(c, *s, &width, &spacing);
            x += spacing;
            if (x > max_w)
                max_w = x;
        }
    }
    *out_h = h;
    *out_w = max_w;
}

// 0x508440  Confidence: exact (VC6 byte match, m-1019 w1, D10000; the kern pair count in a local).
// Glyph index of c in gr_current_font, or -1 (width 0, default spacing). The kern search starts at
// the character's first pair and stops at the first pair that is not (i, < j); only ch1 is
// compared after the loop.
int gr_font_get_char(char c, char next, int *width, int *spacing)
{
    Font *f = gr_current_font;
    int i = (unsigned char)c - f->first_ascii;
    if (i < 0 || i >= f->num_chars) {
        *width = 0;
        *spacing = f->default_spacing;
        return -1;
    }

    FontCharInfo *ci = &f->chars[i];
    *width = ci->width;
    *spacing = ci->spacing;

    int k = ci->kerning_entry;
    if (k > -1 && next && next != '\n') {
        int j = (unsigned char)next - f->first_ascii;
        if (j >= 0 && j < f->num_chars) {
            FontKernEntry *kp = &((FontKernEntry *)f->kern_data)[k];
            int n = f->num_kern_pairs;
            while (kp->ch0 == i && kp->ch1 < j && k < n) {
                kp++;
                k++;
            }
            if (kp->ch1 == j)
                *spacing = ci->spacing + kp->offset;
        }
    }
    return i;
}

// 0x508510  Confidence: exact.
// The first font loaded becomes the default; height2 > -1 overrides the file's second height.
int gr_load_font(const char *name, int height2)
{
    int i = gr_font_create(name);
    if (i < 0)
        return -1;
    if (!gr_default_font)
        gr_default_font = &gr_fonts[i];
    if (height2 > -1)
        gr_fonts[i].height2 = height2;
    return i;
}

// 0x508560  Confidence: likely overall; the two format switches are unsure in shape (see below).
// Returns the index of an already loaded font with the same name, or loads it into the slot after
// the last used one. The first free slot is found and kept in free_slot, but never used, so a
// slot freed by gr_close_fonts is not reused (MAX_FONTS counts every load).
// Packs the glyphs left to right, row by row, into a bm_width x bm_height 4444 texture.
//
// Switches: both test 0xF, then 0x0F0F0F0F, then -0x10 (ascending as unsigned). The first one
// places the default (return -1) first and the shared 0xF / -0x10 case last; the second places
// the bodies as -0x10, 0x0F0F0F0F, 0xF. Written as switches in body order; if VC6 orders the
// compares differently, an if chain is the alternative.
// The 0x0F0F0F0F size is halved with SHR, so the division is unsigned; the compares that follow
// are signed, so the result is an int.
int gr_font_create(const char *name)
{
    Font *f = gr_fonts;
    int free_slot = -1;
    int i;

    for (i = 0; i < gr_num_fonts; f++, i++) {
        if (f->signature) {
            if (!strcmp(f->filename, name))
                return i;
        } else if (free_slot < 0) {
            free_slot = i;
        }
    }
    if (i == MAX_FONTS)
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\gr\\gr_font.cpp", 676,
                   "Too many fonts!\nSee John, or change MAX_FONTS in Graphics\\Font.h\n");
    if (i == gr_num_fonts)
        gr_num_fonts++;

    if (!gr_font_read_file(name, f))
        return -1;

    int num_pixels;
    switch (f->format) {
    default:
        return -1;
    case FONT_FORMAT_RGB16:
        num_pixels = (unsigned int)f->pixel_data_size / 2;
        break;
    case FONT_FORMAT_ALPHA4:
    case FONT_FORMAT_PAL8:
        num_pixels = f->pixel_data_size;
        break;
    }

    // Each branch loads both values (mov eax, N; mov ecx, eax), hence two variables.
    int bm_w, bm_h;
    if (num_pixels * 2 < 0x1000)
        bm_w = bm_h = 64;
    else if (num_pixels * 2 < 0x4000)
        bm_w = bm_h = 128;
    else
        bm_w = bm_h = 256;
    f->bm_width = bm_w;
    f->bm_height = bm_h;

    f->bm_bits = new uint16_t[f->bm_width * f->bm_height];
    f->char_x_coords = new int[f->num_chars];
    f->char_y_coords = new int[f->num_chars];

    int x = 0;
    int y = 0;
    for (int c = 0; c < f->num_chars; c++) {
        if (f->chars[c].width + x >= f->bm_width) {
            y += f->height;
            x = 0;
            if (f->height + y > f->bm_height)
                VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\gr\\gr_font.cpp", 731, "Font too big!\n");
        }
        f->char_x_coords[c] = x;
        f->char_y_coords[c] = y;

        switch (f->format) {
        case FONT_FORMAT_PAL8: {
            // Palette entry bytes 0..3 -> nibbles 8, 4, 0, 12 (low nibble of each byte), built as
            // ((a << 4 | b) << 4 | c) << 4 | d with 16-bit pieces (MOVZX r16 in the binary).
            int *pal = f->palette;
            unsigned char *src = f->pixel_data + f->chars[c].pixel_data_offset;
            for (int row = 0; row < f->height; row++) {
                for (int col = 0; col < f->chars[c].width; col++) {
                    unsigned int color = pal[*src++];
                    unsigned short pixel = (unsigned short)((color >> 24) & 0xf);
                    pixel = (pixel << 4) | (unsigned short)(color & 0xf);
                    pixel = (pixel << 4) | (unsigned short)((color >> 8) & 0xf);
                    pixel = (pixel << 4) | (unsigned short)((color >> 16) & 0xf);
                    f->bm_bits[(row + y) * f->bm_width + col + x] = pixel;
                }
            }
            break;
        }
        case FONT_FORMAT_RGB16: {
            uint16_t *src = (uint16_t *)(f->pixel_data + f->chars[c].pixel_data_offset);
            for (int row = 0; row < f->height; row++) {
                for (int col = 0; col < f->chars[c].width; col++)
                    f->bm_bits[(row + y) * f->bm_width + col + x] = *src++;
            }
            break;
        }
        case FONT_FORMAT_ALPHA4: {
            // Alpha 0..14 scaled to 0..255 (unsigned divide by 14), through the gamma ramp, into the
            // top nibble; colour bits all set.
            unsigned char *src = f->pixel_data + f->chars[c].pixel_data_offset;
            for (int row = 0; row < f->height; row++) {
                for (int col = 0; col < f->chars[c].width; col++) {
                    unsigned int alpha = *src++;
                    if (alpha > 14)
                        alpha = 14;
                    f->bm_bits[(row + y) * f->bm_width + col + x] =
                        (uint16_t)(((gr_gamma_ramp[alpha * 255 / 14] >> 4) << 12) | 0xfff);
                }
            }
            break;
        }
        }

        x += f->chars[c].width;
    }

    f->bm_handle = bm_create(4, f->bm_width, f->bm_height, f->bm_bits, NULL, -1);
    return i;
}

// 0x508970  Confidence: exact (VC6 byte match, match23; was likely; kern pair addressing, see the loop).
// Version 0 files have no format field (4-bit alpha) and two extra ints after the kern count;
// versions above 1 are fatal. Each kern pair is two read_int8 and one raw 1-byte File::read.
// In the kern loop the binary loads kern_data after the first read and keeps &pair[1] across the
// second read, so the address of the second and third bytes is formed before the call.
bool gr_font_read_file(const char *name, Font *f)
{
    File file;
    if (file.open(name, 1, 9999999) < 0)
        return false;

    strcpy(f->filename, name);
    f->signature = file.read_int(0, 0);
    int version = file.read_int(0, 0);
    if (version > 1)
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\gr\\gr_font.cpp", 532,
                   "Trying to read a newer font version than supported!!\n");
    if (version < 1)
        f->format = FONT_FORMAT_ALPHA4;
    else
        f->format = file.read_uint(0, 0);

    f->num_chars = file.read_int(0, 0);
    f->first_ascii = file.read_int(0, 0);
    f->default_spacing = file.read_int(0, 0);
    f->height2 = f->height = file.read_int(0, 0);
    f->num_kern_pairs = file.read_int(0, 0);
    if (version < 1) {
        file.read_int(0, 0);
        file.read_int(0, 0);
    }
    f->pixel_data_size = file.read_int(0, 0);

    if (f->num_kern_pairs) {
        f->kern_data = new uint8_t[f->num_kern_pairs * 3];
        for (int i = 0; i < f->num_kern_pairs; i++) {
            f->kern_data[i * 3] = file.read_int8(0, 0);
            f->kern_data[i * 3 + 1] = file.read_int8(0, 0);
            file.read(&f->kern_data[i * 3 + 2], 1, 0, 0);
        }
    } else {
        f->kern_data = NULL;
    }

    if (f->num_chars) {
        f->chars = new FontCharInfo[f->num_chars];
        for (int i = 0; i < f->num_chars; i++) {
            f->chars[i].spacing = file.read_int(0, 0);
            f->chars[i].width = file.read_int(0, 0);
            f->chars[i].pixel_data_offset = file.read_int(0, 0);
            f->chars[i].kerning_entry = file.read_int16(0, 0);
            f->chars[i].user_data = file.read_int16(0, 0);
        }
    } else {
        f->chars = NULL;
    }

    if (f->pixel_data_size) {
        f->pixel_data = new uint8_t[f->pixel_data_size];
        file.read(f->pixel_data, f->pixel_data_size, 0, 0);
    } else {
        f->pixel_data = NULL;
    }

    if (f->format == FONT_FORMAT_PAL8) {
        f->palette = new int32_t[256];
        file.read(f->palette, 0x400, 0, 0);
    }

    file.close();
    return true;
}

// 0x508c60  Confidence: exact (VC6 8966 byte match, w1 / D11003: mode is a gr::Mode passed by value, not an
// int converted by the Mode constructor; if/else for the font: two stores in the binary).
// x == GR_CENTER_X centres every line. Control characters other than '\n' are skipped. Known
// defect: a character outside the font (index -1) still draws with chars[-1] and coords[-1].
// The end position is saved for gr_string_at_text_pos.
void gr_string(int x, int y, const char *s, int font, gr::Mode mode)
{
    if (!s) {
        gr_text_pos_x = x;
        gr_text_pos_y = y;
        return;
    }

    if (font < 0)
        gr_current_font = gr_default_font;
    else
        gr_current_font = &gr_fonts[font];

    int cx = x;
    int cy = y;
    if (x == GR_CENTER_X)
        cx = gr_font_center_x(s);

    int h = gr_current_font->height;
    while (*s) {
        unsigned char c = *s++;     // compared with JAE, so unsigned
        if (c < ' ') {
            if (c == '\n') {
                cx = x;
                cy += h;
                if (x == GR_CENTER_X)
                    cx = gr_font_center_x(s);
            }
        } else {
            int width, spacing;
            int idx = gr_font_get_char(c, *s, &width, &spacing);
            gr_bitmap_ex(gr_current_font->bm_handle, cx, cy, gr_current_font->chars[idx].width, h,
                         gr_current_font->char_x_coords[idx], gr_current_font->char_y_coords[idx], mode);
            cx += spacing;
        }
    }
    gr_text_pos_x = cx;
    gr_text_pos_y = cy;
}

// 0x508d70  Confidence: exact.
// Uses gr_current_font as set by the caller (gr_string).
int gr_font_center_x(const char *s)
{
    int w = 0;
    for (; *s && *s != '\n'; s++) {
        int width, spacing;
        gr_font_get_char(*s, s[1], &width, &spacing);
        w += spacing;
    }
    return (gr_screen.clip_width - w) / 2;
}

// 0x508dc0  Confidence: exact. No direct callers.
void gr_string_at_text_pos(const char *s, int font, gr::Mode mode)
{
    gr_string(gr_text_pos_x, gr_text_pos_y, s, font, mode);
}

// 0x508df0  Confidence: exact (VC6 byte match, match11; was likely) (EH states and the returned-object flag come from the String
// temporaries; the shape of the space-skipping loop: the binary increments split_at once before
// testing, unlike gr_font_fit_string).
// String version of gr_font_fit_string, no direct callers: returns the first line that fits in
// max_w and leaves the rest in str. When everything fits it returns "" and leaves str unchanged.
String gr_fit_string_String(String &str, int max_w, int font)
{
    int w, h;
    bool in_space = false;
    int split_at = -1;
    String line;

    gr_get_string_size(&w, &h, str.c_str(), -1, font);
    if (w <= max_w)
        return String(gr_font_empty_line);

    int len = str.size();
    int i;
    for (i = 0; i < len; i++) {
        gr_get_string_size(&w, &h, str.c_str(), i, font);
        if (w > max_w)
            break;
        if (str[i] == '\n') {
            line = str.left(i);
            str = str.substr(i + 1, -1);
            return line;
        }
        if (gr_font_is_whitespace(str[i])) {
            if (!in_space)
                split_at = i;
            in_space = true;
        } else {
            in_space = false;
        }
    }
    if (split_at < 0)
        split_at = i;

    line = str.left(split_at);
    split_at++;
    while (split_at < len && gr_font_is_space(str[split_at]))
        split_at++;
    if (split_at < len && str[split_at] == '\n')
        split_at++;
    if (split_at == len)
        str = gr_font_empty_rest;
    else
        str = str.substr(split_at, -1);
    return line;
}

// 0x509030  Confidence: exact (VC6 byte match, match11; was likely) (loop rotation of the space skip).
// How much of s fits in max_w: *split_at gets the length of the first line, the result points at
// the start of the next one (NULL when s fits or nothing is left). A '\n' inside the fitting part
// returns the text after it without updating *split_at (left at -1 or the last break).
char *gr_font_fit_string(int *split_at, char *s, int max_w, int font)
{
    int w, h;
    bool in_space = false;
    int len = strlen(s);

    *split_at = -1;
    gr_get_string_size(&w, &h, s, -1, font);
    if (w <= max_w) {
        *split_at = len;
        return NULL;
    }

    int i;
    for (i = 0; i < len; i++) {
        gr_get_string_size(&w, &h, s, i, font);
        if (w > max_w)
            break;
        if (s[i] == '\n')
            return &s[i + 1];
        if (gr_font_is_whitespace(s[i])) {
            if (!in_space)
                *split_at = i;
            in_space = true;
        } else {
            in_space = false;
        }
    }
    if (*split_at < 0)
        *split_at = i;

    i = *split_at;
    while (i < len && gr_font_is_space(s[i]))
        i++;
    if (i < len && s[i] == '\n')
        i++;
    if (i == len)
        return NULL;
    return &s[i];
}

// 0x509130  Confidence: unsure (shape of the line break: the '\n' case and the overflow case
// share one block in the binary, END in EAX; written with an end local, the original may have
// duplicated the block or used a goto). Control flow, call order and constants follow the binary.
// Splits s into at most max_lines lines of width < max_w: offset_array[n] / len_array[n] are the
// start and length of line n; returns the line count. Breaks at the last space run (not before
// French : ? ! punctuation) or after a '-'. ignore_char starts a run that is skipped up to the next
// whitespace (not measured). strlen is re-evaluated in the loop test and compared signed.
int gr_split_str(int *len_array, int *offset_array, const char *s, int max_w, int max_lines, char ignore_char, int font)
{
    int w, h;
    char buf[512];
    memset(buf, 0, sizeof(buf));

    int i = 0;
    int last_break = -1;
    int num_lines = 0;
    int in_space = 0;
    int buf_len = 0;
    int skipping = 0;

    while (gr_font_is_whitespace(s[i]))
        i++;

    int new_line = 1;
    offset_array[num_lines] = -1;

    for (; i < (int)strlen(s); i++) {
        if (num_lines >= max_lines)
            return num_lines;

        if (new_line) {
            offset_array[num_lines] = -1;
            if (gr_font_is_space(s[i]))
                continue;
            offset_array[num_lines] = i;
            last_break = -1;
            new_line = 0;
        }

        if (skipping) {
            if (gr_font_is_whitespace(s[i]))
                skipping = 0;
            continue;
        }

        int end;
        if (s[i] == '\n') {
            end = i;
        } else {
            if (s[i] == ignore_char) {
                skipping = 1;
                continue;
            }
            if (i > 0 && s[i - 1] == '-')
                last_break = i;
            if (gr_font_is_space(s[i]) && !gr_font_french_punct(s[i + 1])) {
                if (!in_space)
                    last_break = i;
                in_space = 1;
            } else {
                in_space = 0;
            }

            buf[buf_len++] = s[i];
            buf[buf_len] = 0;
            gr_get_string_size(&w, &h, buf, -1, font);
            if (w < max_w)
                continue;

            if (last_break < 0) {
                end = i;
                i--;                        // this character starts the next line
            } else {
                end = last_break;
                if (last_break > 0 && s[last_break - 1] == '-')
                    i = last_break - 1;     // keep the hyphen; the next line starts at last_break
                else
                    i = last_break;         // drop the space
            }
        }

        len_array[num_lines] = end - offset_array[num_lines];
        num_lines++;
        offset_array[num_lines] = -1;
        new_line = 1;
        memset(buf, 0, sizeof(buf));    // rep stosd after the two stores above
        buf_len = 0;
    }

    if (offset_array[num_lines] >= 0) {
        len_array[num_lines] = i - offset_array[num_lines];
        num_lines++;
    }
    return num_lines;
}
