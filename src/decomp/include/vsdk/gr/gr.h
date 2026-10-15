// vsdk/gr/gr.h - the renderer front end (vsdk\gr\gr.cpp, path references 0x505edf-0x506274; the 3D setup
// 0x50dec0 and gr_light_desc 0x50f3e0 are vsdk\gr in sum.symbols, file not known). Includes gr_font.h,
// so one include gives the whole gr interface. Not original source. gr__Screen: symbols/types.h.
#ifndef VSDK_GR_GR_H
#define VSDK_GR_GR_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"

// gr::Mode (gr__Mode__Mode packs six 5-bit fields) is a 4-byte class passed by value. Until it is
// declared, it is a dword typedef (DECISIONS.md D7); gr_font.cpp's own definitions take int.
typedef unsigned int gr_mode;

// A colour (4 bytes r, g, b, a). It has a constructor (DECISIONS.md D41): the static colour objects
// are built by Color::Color(r, g, b, a) (0x5067f0, thiscall, ret 0x10), and hud_portrait_get_color
// (0x40f880) returns its Color through a hidden pointer, which VC6 does only for a non-POD class
// (a 4-byte POD struct comes back in EAX). The default constructor is inline and empty (no call is
// emitted for the uninitialised locals and the colour arrays).
struct Color {
    Color() {}
    Color(int r, int g, int b, int a);                // 0x5067f0 Color__Color
    ubyte r, g, b, a;
};
typedef Color Color_ctor;                             // the pilots' wrapper name (D41)
LAYOUT_CHECK(Color, sizeof(Color) == 4);

// gr::Screen at 0x25d8d30 (zeroed by gr__init, 0x2a dwords).
struct gr__Screen {
    int signature;           // +0x00
    int max_w;               // +0x04 screen width
    int max_h;               // +0x08 screen height
    int mode;                // +0x0C renderer: 0x66 Direct3D, 0x67 Glide, 0x68 OpenGL
    int window_mode;         // +0x10 200 windowed, 201 fullscreen, 202 desktop size
    int dual_monitors;       // +0x14
    float aspect;            // +0x18 (width / height) * 0.75
    int rowsize;             // +0x1C
    int bits_per_pixel;      // +0x20
    int bytes_per_pixel;     // +0x24
    float far_zvalue;        // +0x28
    int offset_x;            // +0x2C clip origin x
    int offset_y;            // +0x30 clip origin y
    int clip_width;          // +0x34
    int clip_height;         // +0x38
    int max_bitmap_w;        // +0x3C
    int max_bitmap_h;        // +0x40
    int clip_left;           // +0x44
    int clip_right;          // +0x48
    int clip_top;            // +0x4C
    int clip_bottom;         // +0x50
    // +0x54.. not mapped (0x2a dwords in all)
};
extern gr__Screen gr_screen;                          // 0x25d8d30

// ---- modes and state ----------------------------------------------------------------------------
extern gr_mode gr_rect_mode;                          // 0x25d88ac
extern gr_mode gr_bitmap_wrap_mode;                   // 0x25d88c4
extern gr_mode gr_line_mode;                          // 0x25d8ce4
extern gr_mode gr_mode_ui_bitmap;                     // 0x25d8d08 (wrap, texture, vertex alpha, alpha)
extern gr_mode gr_mode_automap_additive;              // 0x25d8d10 (as above, additive)
extern gr_mode gr_mode_lightmap_multitexture;         // 0x25d8d1c
extern gr_mode gr_mode_flat_alpha;                    // 0x25d8d28
extern gr_mode gr_string_mode;                        // 0x25d8df4
extern int gr_gamma_ramp[256];                        // 0x25d8e0c
extern bool gr_antialias_enabled;                     // 0x25d921c
extern bool gr_byte_25d921d;                          // 0x25d921d unnamed; set to 1 by the Direct3D code (0x539efd)
// The masked automap bitmap is drawn with an immediate gr::Mode dword (no global holds it).
#define GR_MODE_AUTOMAP_MASKED 0x4028821u

// ---- gr.cpp ---------------------------------------------------------------------------------------
class vector3;
// 0x50ecd0 (vsdkgr): true when the sphere lies fully outside one frustum plane; the objects.cpp
// is_in_view functions return its result unchanged (bool: no conversion after the call).
bool gr_cull_sphere(const vector3 *pos, float radius, int ignore_far);

int  gr_screen_width();                               // 0x506170
int  gr_screen_height();                              // 0x506180
void gr_fade_out(int ms);                             // 0x5065e0
void gr_fade_in(int ms);                              // 0x506630
// The clip functions are gr::set_clip / gr::reset_clip (sum.symbols gr__set_clip / gr__reset_clip).
// Some pilots spell the call gr__set_clip; the macro keeps that spelling (DECISIONS.md D8).
namespace gr {
void set_clip(int x, int y, int w, int h);            // 0x506840 gr__set_clip
void reset_clip();                                    // 0x5069a0 gr__reset_clip
}
#define gr__set_clip gr::set_clip
#define gr__reset_clip gr::reset_clip
void gr_get_clip(int *x, int *y, int *w, int *h);     // 0x506970
void gr_clear();                                      // 0x5069c0
void gr_page_in(int bm, int flags);                   // 0x5069e0
void gr_set_gamma(float gamma);                       // 0x506a90
void gr_set_color(int r, int g, int b, int a);        // 0x506b90
void gr_set_color_ptr(Color *c);                      // 0x506c10
void gr_set_alpha(int a);                             // 0x506c40
void gr_mark_texture_dirty(int bm);                   // 0x506c70
// (bm, x, y, w, h, sx, sy, mode), clipped.
void gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr_mode mode);   // 0x506cb0
void gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                         gr_mode mode);               // 0x506ec0
void gr_bitmap(int bm, int x, int y, gr_mode mode);   // 0x506f90
// Rotated bitmap through gr_tmapper: (bm, x, y, scale, angle, mode); the two floats after y are read
// from the automap call only (types guessed: scale = mask scale * zoom, angle = camera heading).
void gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr_mode mode);   // 0x507100
void gr_line(float x1, float y1, float x2, float y2, gr_mode mode);   // 0x507310
void gr_rect(int x, int y, int w, int h, gr_mode mode, float z);       // 0x5077d0 (z 1.0f or FLT_MAX in the pilots)
bool gr_is_mode_supported(const gr_mode *mode);       // 0x5079b0 (by address; type guessed)
int  gr_read_backbuffer(int x, int y, int w, int h, void *buf);   // 0x507d30 -> pixel format (3 565, 7 32-bit)
void gr_set_alpha_test(int enable);                   // 0x508110 (type guessed)

// ---- 3D -------------------------------------------------------------------------------------------
void gr_setup_3d(matrix *orient, vector3 *pos, float fov, int zbuffer, int a5);   // 0x50dec0 (1, 1 in the pilots)
bool gr_cull_bounding_box(vector3 *mins, vector3 *maxs);   // 0x50edc0 (true = outside the view)

// The lighting record a model is rendered with (0x20 bytes on the callers' stacks; thiscall constructor,
// no destructor). The constructor builds a vector3 at +0xc and resets +0..+0x1c. status_draw_model
// stores the multiplayer appearance at +0x10 and the hero model prefix at +0x1c; buysell sets +0x14.
class gr_light_desc {
public:
    gr_light_desc();                  // 0x50f3e0 gr_light_desc__gr_light_desc
    int _unk00;                       // +0x00
    int _unk04;                       // +0x04 -1
    int _unk08;                       // +0x08 0xff
    float _unk0c;                     // +0x0c (vector3 x; y, z overlap +0x10 / +0x14 as written by the ctor)
    void *appearance;                 // +0x10 charinfo_get_multi_appearance
    float f14;                        // +0x14 -1.0f from the ctor; buysell sets 0.15 before ca_render_mesh
    int _unk18;                       // +0x18
    const char *model_prefix;         // +0x1c character_hero_model_prefix
};
LAYOUT_CHECK(gr_light_desc, sizeof(gr_light_desc) == 0x20);

#include "vsdk/gr/gr_font.h"

float rgb_to_lightened_grey(float r, float g, float b);  // 0x508090 (avg + (255 - avg) * 0.6) / 255 (decomp23)

#endif
