// gr_font.h - Sum.exe v1.40 bitmap fonts and text drawing (vsdk\gr\gr_font.cpp, 0x5081d0-0x50939a)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols; Font / FontCharInfo layouts
// from symbols/types.h.
#ifndef GR_FONT_H
#define GR_FONT_H

#include "types.h"

class String;

#define MAX_FONTS 6
#define GR_CENTER_X 0x8000          // gr_string x value that centres each line in the clip width

// Font.format values
#define FONT_FORMAT_ALPHA4  0xF         // 4-bit alpha per byte (version 0 files are always this)
#define FONT_FORMAT_RGB16   0x0F0F0F0F  // 16-bit pixels, copied as they are
#define FONT_FORMAT_PAL8    -0x10       // 8-bit indices into a 256-entry palette

extern Font gr_fonts[MAX_FONTS];    // 0x25d9238
extern int gr_num_fonts;            // 0x25d94c0
extern Font *gr_current_font;       // 0x25d94c4
extern Font *gr_default_font;       // 0x25d94c8  first font loaded
extern int gr_text_pos_x;           // 0x25d94cc  where the last gr_string stopped
extern int gr_text_pos_y;           // 0x25d94d0

void gr_close_fonts();
int gr_font_is_whitespace(char c);
int gr_font_is_space(char c);
int gr_font_french_punct(char c);
int gr_get_font_height(int font);
int gr_get_font_height2(int font);
void gr_get_string_size(int *out_w, int *out_h, const char *s, int len, int font);
int gr_font_get_char(char c, char next, int *width, int *spacing);
int gr_load_font(const char *name, int height2);
int gr_font_create(const char *name);
bool gr_font_read_file(const char *name, Font *f);
void gr_string(int x, int y, const char *s, int font, int mode);
int gr_font_center_x(const char *s);
void gr_string_at_text_pos(const char *s, int font, int mode);
String gr_fit_string_String(String &str, int max_w, int font);
char *gr_font_fit_string(int *split_at, char *s, int max_w, int font);
int gr_split_str(int *len_array, int *offset_array, char *s, int max_w, int max_lines, char ignore_char, int font);

#endif
