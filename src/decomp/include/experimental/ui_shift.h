// The experimental build's widescreen interface (experimental/widescreen.cpp). Included LAST by the interface's
// source files (after every other header): it points their 2D drawing, clip, screen width and mouse calls at the
// ui_* wrappers, which centre the 1024-wide layout on a wider screen during gameplay. Empty in the shipped and fixed
// builds.
#ifdef EXPERIMENTAL_BUILD
#ifndef EXPERIMENTAL_UI_SHIFT_H
#define EXPERIMENTAL_UI_SHIFT_H

#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/mouse.h"

void ui_gr_bitmap(int bm, int x, int y, gr::Mode mode);
void ui_gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr::Mode mode);
void ui_gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode);
void ui_gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr::Mode mode);
void ui_gr_string(int x, int y, const char *s, int font, gr::Mode mode);
void ui_gr_rect(int x, int y, int w, int h, gr::Mode mode, float z);
void ui_gr_line(float x1, float y1, float x2, float y2, gr::Mode mode);
namespace gr {
void ui_set_clip(int x, int y, int w, int h);
}
void ui_gr_get_clip(int *x, int *y, int *w, int *h);
int ui_gr_screen_width();
int ui_mouse_get_pos(int *x, int *y, int *z);

#endif

#define gr_bitmap ui_gr_bitmap
#define gr_bitmap_ex ui_gr_bitmap_ex
#define gr_bitmap_stretched ui_gr_bitmap_stretched
#define gr_bitmap_float ui_gr_bitmap_float
#define gr_string ui_gr_string
#define gr_rect ui_gr_rect
#define gr_line ui_gr_line
#undef gr__set_clip
#define gr__set_clip gr::ui_set_clip
#define set_clip ui_set_clip
#define gr_get_clip ui_gr_get_clip
#define gr_screen_width ui_gr_screen_width
#define mouse_get_pos ui_mouse_get_pos

#endif // EXPERIMENTAL_BUILD
