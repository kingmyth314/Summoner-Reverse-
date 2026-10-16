// vsdk/gr/gr.h - the renderer front end (vsdk\gr\gr.cpp, path references 0x505edf-0x506274; the 3D setup
// 0x50dec0 and gr_light_desc 0x50f3e0 are vsdk\gr in sum.symbols, file not known). Includes gr_font.h,
// so one include gives the whole gr interface. Not original source. gr__Screen: symbols/types.h.
#ifndef VSDK_GR_GR_H
#define VSDK_GR_GR_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include <windows.h>                                  // HWND (gr_init)

// gr::Mode (gr__Mode__Mode packs six 5-bit fields) is a 4-byte class passed by value. Until it is
// declared, it is a dword typedef (DECISIONS.md D7); gr_font.cpp's own definitions take int.
typedef unsigned int gr_mode;

// gr::Mode itself (decomp33, DECISIONS.md D1300): the packed render state as the back ends see it.
// Six 5-bit fields, texture source in bits 0-4 up to the fog type in bits 25-29 (gr__Mode__Mode).
// The members are header inlines: their only out-of-line copies are COMDATs in other objects (the
// accessors 0x5368a0-0x536930 among the Direct3D code, the constructors in gr.cpp), and the vsdk
// objects, built without inline expansion, call them (gr_opengl_set_mode, gr_opengl_clear). A class with
// constructors and no copy constructor or destructor is still passed by value as one dword.
// decomp40 (gr.cpp): the 30 mode globals of gr.cpp are gr::Mode objects (each has a dynamic initialiser
// `mov ecx, &global; call 0x508160`) and the front end passes gr::Mode (DECISIONS.md D2001).
// Mode(gr_mode) is not inline: its only copy 0x506810 lies between gr_close and gr::set_clip, among
// gr.cpp's own functions, and nothing calls it, so it is an out-of-line definition, not a COMDAT
// (D2002). The rest of the tree still stores and passes the dword in places (gr_string's int mode,
// ui_bitmap_scaled); operator gr_mode() (ours, inline) converts back.
namespace gr {
class Mode {
public:
    Mode() { value = (gr_mode)-1; }                   // 0x5368a0 gr__Mode__Mode_invalid
    Mode(gr_mode v);                                  // 0x506810 gr__Mode__Mode_value (gr.cpp, D2002)
    Mode(int texture_source, int color_source, int alpha_source, int alpha_blend, int zbuffer_type,
         int fog_type)                                // 0x508160 gr__Mode__Mode
    {
        value = ((((((((((gr_mode)fog_type << 5) | zbuffer_type) << 5) | alpha_blend) << 5) | alpha_source)
                  << 5) | color_source) << 5) | texture_source;
    }
    bool operator==(const Mode &m) const { return value == m.value; }   // 0x5368b0 gr__Mode__operator_eq
    int get_texture_source() const { return (value >> 0) & 0x1f; }      // 0x5368c0
    int get_color_source() const { return (value >> 5) & 0x1f; }        // 0x5368d0
    int get_alpha_source() const { return (value >> 10) & 0x1f; }       // 0x5368e0
    int get_alpha_blend() const { return (value >> 15) & 0x1f; }        // 0x5368f0
    int get_zbuffer_type() const { return (value >> 20) & 0x1f; }       // 0x536900
    int get_fog_type() const { return (value >> 25) & 0x1f; }           // 0x536910
    void invalidate() { value = (gr_mode)-1; }                          // 0x536920 gr__Mode__invalidate
    bool is_multitexture() const { return get_texture_source() >= 4; }  // 0x536930
    operator gr_mode() const { return value; }        // ours (D2001): no copy in the binary
    gr_mode value;
};
}
LAYOUT_CHECK(gr_Mode, sizeof(gr::Mode) == 4);

// A colour (4 bytes r, g, b, a). It has a constructor (DECISIONS.md D41): the static colour objects
// are built by Color::Color(r, g, b, a) (0x5067f0, thiscall, ret 0x10), and hud_portrait_get_color
// (0x40f880) returns its Color through a hidden pointer, which VC6 does only for a non-POD class
// (a 4-byte POD struct comes back in EAX). The default constructor is inline and empty (no call is
// emitted for the uninitialised locals and the colour arrays).
struct Color {
    Color() {}
    Color(int r, int g, int b, int a);                // 0x5067f0 Color__Color
    ubyte r, g, b, a;
    void set(int r, int g, int b, int a);             // 0x506820 Color__set
};
typedef Color Color_ctor;                             // the pilots' wrapper name (D41)
LAYOUT_CHECK(Color, sizeof(Color) == 4);

// gr::Screen at 0x25d8d30, 0xa8 bytes (gr_init zeroes 0x2a dwords). decomp40 (D2000): the colour,
// texture, fog and tint state that sum.symbols names as separate globals (gr_color 0x25d8d84 ..
// gr_tint_b 0x25d8dcc, 0x25d8dd0) lies inside it: gr_init's `rep stosd` clears it, and the implicit
// constructor gr__Screen__Screen (0x5081a0) constructs the two Color members at +0x54 and +0x61 (the
// call goes to 0x505530, the empty constructor that vector3() / Color() share by identical-code
// folding). The old global names are macros on the fields below.
struct gr__Screen {
    int signature;           // +0x00
    int max_w;               // +0x04 screen width
    int max_h;               // +0x08 screen height
    int mode;                // +0x0C renderer: 0x66 Direct3D, 0x67 Glide, 0x68 OpenGL
    int window_mode;         // +0x10 200 windowed, 201 fullscreen, 202 desktop size
    bool dual_monitors;      // +0x14 SST_DUALHEAD (gr_check_dual_monitors writes the byte)
    ubyte _pad15[3];
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
    Color current_color;     // +0x54 gr_color (gr_set_color)
    int current_bitmap;      // +0x58 gr_texture (gr_set_texture)
    int current_bitmap_2;    // +0x5C gr_texture_2
    bool fog_enabled;        // +0x60 gr_fog_enabled (gr_set_fog)
    Color fog_color;         // +0x61 gr_fog_color
    ubyte _pad65[3];
    float fog_near;          // +0x68 gr_fog_near
    float fog_far;           // +0x6C gr_fog_far (times Gr_level_fog_multiplier)
    float fog_z_near;        // +0x70 gr_fog_z_near (d3d_fog_mode 3)
    float fog_z_far;         // +0x74 gr_fog_z_far
    float fog_z_inv_range;   // +0x78 gr_fog_z_inv_range
    float fog_alpha_near;    // +0x7C gr_fog_alpha_near
    float fog_alpha_far;     // +0x80 gr_fog_alpha_far
    float fog_alpha_inv_range; // +0x84 gr_fog_alpha_inv_range
    float fog_vertex_near;   // +0x88 gr_fog_vertex_near
    float fog_vertex_far;    // +0x8C gr_fog_vertex_far
    bool tint_enabled;       // +0x90 gr_tint_enabled (gr_set_tint)
    ubyte _pad91[3];
    float tint_r;            // +0x94 gr_tint_r
    float tint_g;            // +0x98 gr_tint_g
    float tint_b;            // +0x9C gr_tint_b
    bool vsync;              // +0xA0 gr_vsync (gr_set_vsync; gr_init's last argument)
    ubyte _padA1[3];
    int poly_arg_a4;         // +0xA4 0 from gr_init, passed to gr_poly_begin by gr_poly (name ours)
};
LAYOUT_CHECK(gr__Screen, sizeof(gr__Screen) == 0xa8);
extern gr__Screen gr_screen;                          // 0x25d8d30
#define gr_color (gr_screen.current_color)            // 0x25d8d84
#define gr_texture (gr_screen.current_bitmap)         // 0x25d8d88
#define gr_texture_2 (gr_screen.current_bitmap_2)     // 0x25d8d8c
#define gr_fog_enabled (gr_screen.fog_enabled)        // 0x25d8d90
#define gr_fog_color (gr_screen.fog_color)            // 0x25d8d91
#define gr_fog_near (gr_screen.fog_near)              // 0x25d8d98
#define gr_fog_far (gr_screen.fog_far)                // 0x25d8d9c
#define gr_fog_z_near (gr_screen.fog_z_near)          // 0x25d8da0
#define gr_fog_z_far (gr_screen.fog_z_far)            // 0x25d8da4
#define gr_fog_z_inv_range (gr_screen.fog_z_inv_range)  // 0x25d8da8
#define gr_fog_alpha_near (gr_screen.fog_alpha_near)  // 0x25d8dac
#define gr_fog_alpha_far (gr_screen.fog_alpha_far)    // 0x25d8db0
#define gr_fog_alpha_inv_range (gr_screen.fog_alpha_inv_range)  // 0x25d8db4
#define gr_fog_vertex_near (gr_screen.fog_vertex_near)  // 0x25d8db8
#define gr_fog_vertex_far (gr_screen.fog_vertex_far)  // 0x25d8dbc
#define gr_tint_enabled (gr_screen.tint_enabled)      // 0x25d8dc0
#define gr_tint_r (gr_screen.tint_r)                  // 0x25d8dc4
#define gr_tint_g (gr_screen.tint_g)                  // 0x25d8dc8
#define gr_tint_b (gr_screen.tint_b)                  // 0x25d8dcc
// gr_vsync (0x25d8dd0, +0xa0) is a macro on gr_screen, after the gr_direct3d.cpp group below.

// ---- modes (gr.cpp .bss; D2001) ------------------------------------------------------------------
// gr::Mode(texture, colour, alpha, blend, zbuffer, fog), as built by each global's dynamic initialiser.
// The names are sum.symbols' where it has one (its module column puts some of them in other units;
// all 30 initialisers are gr.cpp's, table 0x5887ec-0x588860); gr_mode_<address> are ours.
extern gr::Mode gr_string_mode;                       // 0x25d8df4 (3, 2, 3, 3, 0, 3)
extern gr::Mode gr_mode_25d8cdc;                      // 0x25d8cdc (3, 2, 3, 3, 0, 3)
extern gr::Mode gr_mode_ui_bitmap;                    // 0x25d8d08 (1, 1, 0, 3, 0, 3)
extern gr::Mode gr_bitmap_wrap_mode;                  // 0x25d88c4 (1, 2, 3, 3, 0, 3)
extern gr::Mode gr_mode_automap_additive;             // 0x25d8d10 (1, 1, 0, 2, 0, 3)
extern gr::Mode gr_line_mode;                         // 0x25d8ce4 (0, 0, 0, 3, 0, 3)
extern gr::Mode gr_rect_mode;                         // 0x25d88ac (0, 0, 0, 3, 0, 3)
extern gr::Mode gr_mode_25d88a4;                      // 0x25d88a4 (0, 0, 0, 3, 0, 3)
extern gr::Mode gr_mode_flat_alpha;                   // 0x25d8d28 (0, 0, 0, 3, 4, 0)
extern gr::Mode gr_line_3d_mode;                      // 0x25d88c0 (0, 0, 0, 3, 4, 0)
extern gr::Mode gr_mode_line_quad;                    // 0x25d8ce0 (1, 1, 2, 3, 0, 0)
extern gr::Mode gr_mode_25d88cc;                      // 0x25d88cc (1, 2, 3, 3, 0, 0)
extern gr::Mode gr_mode_particle_alpha;               // 0x25d8df0 (2, 2, 3, 3, 1, 0)
extern gr::Mode gr_mode_particle_additive;            // 0x25d8d24 (2, 2, 3, 2, 1, 3)
extern gr::Mode gr_mode_level_alpha_add;              // 0x25d88a8 (1, 2, 3, 2, 1, 0)
extern gr::Mode gr_mode_lamp_glow;                    // 0x25d8cfc (1, 2, 3, 2, 0, 0)
extern gr::Mode gr_mode_25d88a0;                      // 0x25d88a0 (1, 2, 0, 3, 4, 0)
extern gr::Mode gr_mode_vfx_mesh_opaque;              // 0x25d8cd4 (1, 2, 0, 0, 4, 0)
extern gr::Mode gr_mode_mesh_alpha;                   // 0x25d8df8 (1, 2, 3, 3, 4, 0)
extern gr::Mode gr_mode_25d8d00;                      // 0x25d8d00 (1, 2, 3, 3, 1, 0)
extern gr::Mode gr_mode_shadow;                       // 0x25d8d0c (1, 2, 3, 3, 1, 0)
extern gr::Mode gr_mode_25d8cd8;                      // 0x25d8cd8 (2, 2, 0, 0, 4, 1)
extern gr::Mode gr_mode_25d88c8;                      // 0x25d88c8 (2, 2, 3, 2, 1, 1)
extern gr::Mode gr_mode_25d8dd8;                      // 0x25d8dd8 (1, 1, 2, 5, 4, 2)
extern gr::Mode gr_mode_vfx_flat_additive;            // 0x25d8d04 (0, 0, 0, 2, 1, 0)
extern gr::Mode gr_mode_vfx_flat_alpha;               // 0x2f91a5c (0, 0, 0, 3, 1, 0) decomp66: the emitter unit's .bss, own initialiser
extern gr::Mode gr_mode_lightmap_multitexture;        // 0x25d8d1c (4, 1, 3, 3, 4, 0)
extern gr::Mode gr_mode_25d8d18;                      // 0x25d8d18 (6, 2, 3, 3, 4, 0)
extern gr::Mode gr_mode_25d8cd0;                      // 0x25d8cd0 (4, 2, 3, 2, 1, 0)
extern gr::Mode gr_mode_25d8ce8;                      // 0x25d8ce8 (4, 2, 3, 2, 4, 0)
extern gr::Mode gr_mode_25d8d14;                      // 0x25d8d14 (1, 1, 2, 3, 4, 0)

// ---- other state --------------------------------------------------------------------------------
extern int gr_gamma_ramp[256];                        // 0x25d8e0c
extern bool gr_antialias_enabled;                     // 0x25d921c
extern bool gr_byte_25d921d;                          // 0x25d921d unnamed; set to 1 by the Direct3D code (0x539efd)
// The masked automap bitmap is drawn with an immediate mode: 0x4028821 = gr::Mode(1, 1, 2, 5, 0, 2),
// the inline six-field constructor, which the optimised game unit folds to the constant.
#define GR_MODE_AUTOMAP_MASKED gr::Mode(1, 1, 2, 5, 0, 2)

// ---- gr.cpp ---------------------------------------------------------------------------------------
class vector3;
// 0x50ecd0 (vsdkgr): true when the sphere lies fully outside one frustum plane; the objects.cpp
// is_in_view functions return its result unchanged (bool: no conversion after the call).
// decomp65 (D4501): ignore_far is tested as a byte (0x50ece5, 0x50ed2f): bool, was int.
bool gr_cull_sphere(const vector3 *pos, float radius, bool ignore_far);

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
void gr_tcache_free(int bm);                          // 0x508020 per-mode texture cache release (bm_release, decomp31)
// (bm, x, y, w, h, sx, sy, mode), clipped.
void gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr::Mode mode);   // 0x506cb0
void gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                         gr::Mode mode);              // 0x506ec0
void gr_bitmap(int bm, int x, int y, gr::Mode mode);  // 0x506f90
// Rotated bitmap through gr_tmapper: (bm, x, y, scale, angle, mode): a quad of the bitmap's size times
// scale, centred on (x, y) and rotated by angle (radians).
void gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr::Mode mode);   // 0x507100
void gr_line(float x1, float y1, float x2, float y2, gr::Mode mode);   // 0x507310
void gr_rect(int x, int y, int w, int h, gr::Mode mode, float z);      // 0x5077d0 (z is each corner's sw: 1.0f or FLT_MAX in the pilots)
bool gr_is_mode_supported(gr::Mode *mode);            // 0x5079b0 (the pointer goes on to the back end, D1606)
int  gr_read_backbuffer(int x, int y, int w, int h, void *buf);   // 0x507d30 -> pixel format (3 565, 6 888, 7 8888; 0 none)
void gr_set_alpha_test(bool enable);                  // 0x508110 (forwarded as a dword to gr_d3d_set_alpha_test(bool); decomp40)

// ---- init / close (gr.cpp) ---------------------------------------------------------------------------
#define GR_DIRECT3D 0x66                              // gr__Screen.mode values (names ours)
#define GR_GLIDE    0x67
#define GR_OPENGL   0x68
// (w, h, mode, window_mode, hwnd, far_z, vsync): far_z goes to gr_screen +0x28, vsync (byte) to +0xa0
// (gr_screen.vsync, which gr_set_vsync writes; decomp40 D2003, was "dual"). Returns 0 (unused).
// LoadSettings passes 201 (fullscreen), Summoner_options.cfg_34 as the float and false.
int  gr_init(int w, int h, int mode, int window_mode, HWND hwnd, float far_z, bool vsync);   // 0x505e00
void gr_close();                                      // 0x5067c0
int  gr_get_video_memory_mb();                        // 0x5079f0
void gr_load_reflection_map();                        // 0x50b130 refmap01.tga (vsdk\gr)
void gr_set_use_buffered_alpha_polys(bool on);        // 0x53e760 (vsdk\gr)

// ---- 3D -------------------------------------------------------------------------------------------
// decomp65 (D4501): zbuffer goes unchanged (a pushed dword, no conversion) to gr_zbuffer_clear(bool), so
// it is bool (was int); fov below 2.0 is taken as the tangent of the half angle, else as degrees; a5 is
// not read.
void gr_setup_3d(matrix *orient, vector3 *pos, float fov, bool zbuffer, int a5);   // 0x50dec0 (1, 1 in the pilots)
bool gr_cull_bounding_box(vector3 *mins, vector3 *maxs);   // 0x50edc0 (true = outside the view)
// decomp54 (snow_render): (position, a, size, mode) a camera-facing sprite of the current texture.
// decomp66: the second argument is a float (the emitter passes the particle angle; the others 0).
// decomp69 (D5004): returns bool (false when the point does not project; else gr_poly's result).
bool gr_bitmap_3d(vector3 *pos, float angle, float size, gr::Mode mode);   // 0x53f3b0 (vsdk\gr)

// The lighting record a model is rendered with (0x20 bytes on the callers' stacks; thiscall constructor,
// no destructor). The constructor builds a vector3 at +0xc and resets +0..+0x1c. status_draw_model
// stores the multiplayer appearance at +0x10 and the hero model prefix at +0x1c; buysell sets +0x14.
class gr_light_desc {
public:
    gr_light_desc();                  // 0x50f3e0 gr_light_desc__gr_light_desc
    // decomp63 (levelrender_draw_list): +0 = 0, +4 = -1, +8 = 0xff, +0x10 = 0, +0x14 = -1.0f, +0x1c = 0.
    // levelrender_draw_list sets +0 bit 2 (CIF_EDGED), +4 the model LOD (-1 / 0 / 2), +8 the alpha.
    void reset();                     // 0x50f400 gr_light_desc__reset
    int _unk00;                       // +0x00 bit 0: character_instance::render skips the mesh (decomp60)
    int _unk04;                       // +0x04 -1; not 0 / -1: render the shadow mesh when there is one (decomp60)
    int _unk08;                       // +0x08 0xff; 0: character_instance::render draws nothing (decomp60)
    float _unk0c;                     // +0x0c (vector3 x; y, z overlap +0x10 / +0x14 as written by the ctor)
    void *appearance;                 // +0x10 charinfo_get_multi_appearance
    float f14;                        // +0x14 -1.0f from the ctor; buysell sets 0.15 before ca_render_mesh
    ubyte tint[3];                    // +0x18 the instance's tint (character_instance::render; decomp60, was int _unk18)
    ubyte _pad1b;
    const char *model_prefix;         // +0x1c character_hero_model_prefix
};
LAYOUT_CHECK(gr_light_desc, sizeof(gr_light_desc) == 0x20);

#include "vsdk/gr/gr_font.h"

float rgb_to_lightened_grey(float r, float g, float b);  // 0x508090 (avg + (255 - avg) * 0.6) / 255 (decomp23)

// ---- decomp28 (levelrender.cpp) -------------------------------------------------------------------
// The colour tint the back ends apply when enabled: gr_tint_enabled / _r / _g / _b are gr_screen
// fields (macros above, D2000).

// A 3D pipeline vertex (0x30 bytes; symbols/types.h gr_Vertex, +0x1a.. from levelrender_face_vertex_colors).
// +0x1a: what the vertex supplies besides the position (gr_tmapper_multipass: 1 uv, 4 rgb, 8 alpha;
// gr_bitmap_float sets 1, gr_rect 0, levelrender 0xf). The original has a default constructor
// (gr__Vertex__Vertex 0x5081c0, a COMDAT at the end of gr.cpp, constructs a vector3 at +0), which gr_rect
// and gr_bitmap_float run through `vector constructor iterator`; not declared here (D1605, still open).
struct gr_vertex {
    float x, y, z;                    // +0x00 view space
    float sx, sy, sw;                 // +0x0c screen x / y and 1/z (gr_project_vertex)
    ubyte codes;                      // +0x18 clip codes
    ubyte flags;                      // +0x19 1 projected, 2 behind the viewer, 4 temporary
    ubyte _unk1a;                     // +0x1a levelrender stores 0xf
    ubyte index;                      // +0x1b levelrender stores the corner number
    float u, v;                       // +0x1c
    float u2, v2;                     // +0x24 lightmap
    ubyte r, g, b, a;                 // +0x2c
};
LAYOUT_CHECK(gr_vertex, sizeof(gr_vertex) == 0x30);
// (n, verts): clips the polygon, projects the result; returns the vertex count (vsdk\gr).
int gr_clip_and_project_poly(int n, gr_vertex **verts);   // 0x53e9d0
// decomp56 (decals): (out, in, count) view-space vertices from world points; (v, d) pushes a vertex d
// along its view ray (depth bias); (n, verts, flags, a, b) draws a polygon (the decals pass 0x118c21, 0, 0).
void gr_rotate_vertices(gr_vertex *out, const vector3 *in, int count);   // 0x50e990
void gr_offset_point_depth(gr_vertex *v, float d);    // 0x50ea00
// decomp69 (D5003): (n, verts, mode, fixed_depth, depth); the mode is a gr::Mode (get_alpha_source is
// called on the argument slot), the last two go to gr_poly_begin; returns gr_poly_end's result.
bool gr_poly(int n, gr_vertex **verts, gr::Mode mode, bool fixed_depth, float depth);   // 0x53e770

// A light record as levelrender_sum_point_lights reads it (decomp28). decomp60 (D4002): these are the
// attached-light fields (vfx.h attached_light, 0x98 bytes: world position +0x20, colour +0x2c,
// intensity +0x38, range +0x48..+0x50), not the 0x94-byte light table of the light unit (class light
// below, colour at +0x34). Kept as its own declaration so levelrender.cpp compiles unchanged.
struct gr_light {
    ubyte _unk00[0x20];
    vector3 pos;                      // +0x20
    float r, g, b;                    // +0x2c
    float intensity;                  // +0x38
    ubyte _unk3c[0xc];
    bool has_range;                   // +0x48
    ubyte _pad49[3];
    float range_near;                 // +0x4c
    float range_far;                  // +0x50
    ubyte _unk54[0x44];
};
LAYOUT_CHECK(gr_light, sizeof(gr_light) == 0x98);
// The dynamic light table and its functions (decomp28 / decomp48 / decomp56 declared the first ones
// here; decomp60 moved them into the light unit's header).
#include "vsdk/gr/unnamed_lights_0x51e070.h"

// ---- decomp33 (gr_opengl.cpp) ---------------------------------------------------------------------
// gr.cpp state the back ends read: gr_color, gr_texture, gr_texture_2, gr_fog_enabled, gr_fog_color,
// gr_fog_near, gr_fog_far are gr_screen fields (macros above, D2000).
// Vertex colour byte -> gamma-corrected value 0..255, as float (gr_set_gamma clamps and fstp's each
// entry; the OpenGL tmapper fld's it): DECISIONS.md D1302 (sum.symbols: uint32[256]).
extern float gr_color_gamma_lut[256];                 // 0x25d88d0
float gr_get_aspect();                                // 0x506950
// 3D view state (vsdk\gr, file not known).
extern vector3 gr_eye_pos;                            // 0x2682a80
extern matrix gr_eye_matrix;                          // 0x26927c0 (rvec, uvec 0x26927cc, fvec 0x26927d8)
extern float gr_far_clip_dist;                        // 0x2692e88
// gr_vertex_depth (0x50d8b0), gr_calc_depth and gr_get_fog_alpha are header inlines since decomp65
// (D4500): defined in vsdk/gr/unnamed_gr_3d_0x50d9b0.h, included at the end of this header.
void gr_get_view_fov(float *h_fov, float *v_fov);     // 0x50e5a0 degrees
// (fov degrees) -> the fov for the current clip rectangle and aspect (interface_set_view_clip; decomp48).
float gr_fov_to_vertical(float fov);                  // 0x50e5e0
// decomp35 (gametarget.cpp): the mouse ray {eye position, direction} through screen point (sx, sy).
struct ix_ray;
void gr_screen_to_world_ray(ix_ray *out, float sx, float sy);   // 0x50ebe0
void gr_get_eye(vector3 *pos, matrix *orient);         // 0x50ef30
float gr_scale_by_far_clip(float f);                  // 0x50ecb0 f * the far clip scale
// decomp55 (the script editor's markers). (out, world point): into view space with the clip codes;
// (v): the screen point, sets flags bit 0 (bit 1 behind the viewer). The return values are not used by
// the editor (types unsure).
ubyte gr_rotate_vertex(gr_vertex *out, const vector3 *p);   // 0x50e910
ubyte gr_project_vertex(gr_vertex *v);                // 0x50ea50 returns v->flags
// Wire shapes in the current colour (vsdk\gr, file not known): two rotated vertices (false when both are
// clipped by one plane), two world points, a box arrow (pos, orient, size) and a sphere.
bool gr_line_3d(gr_vertex *a, gr_vertex *b, gr::Mode mode);                     // 0x53e520
void gr_line_3d_world(const vector3 *a, const vector3 *b, gr::Mode mode);      // 0x53e610
// decomp69 (D5004): both return true (`mov al, 1`). The "arrow" is a wire box: the 8 corners pos +-
// half of size along the orient axes, 12 edges.
bool gr_draw_sphere(const vector3 *pos, float radius, gr::Mode mode);           // 0x53f720
bool gr_draw_arrow_3d(const vector3 *pos, const matrix *orient, const vector3 *size, gr::Mode mode);   // 0x53faf0

// Direct3D back end (vsdk\gr\directx\gr_direct3d.cpp).
extern bool d3d_use_vertex_buffer;                    // 0x2dc6c31
extern void *d3d_direct3d;                            // 0x2dc6c3c IDirect3D7*

// ---- gr_direct3d.cpp (decomp36) ---------------------------------------------------------------------
// The rest of the Direct3D interface is in vsdk/gr/directx/gr_direct3d.h (it needs d3d_min.h).
extern float Gr_far_min;                              // 0x5a7294 console "far_min"
extern float Gr_far_max;                              // 0x5a7298 console "far_max"
// gr_calc_depth (0x50d880) and gr_get_fog_alpha (0x50d830): inline, vsdk/gr/unnamed_gr_3d_0x50d9b0.h (D4500).
// One pass of gr_tmapper_multipass (0x14 bytes; layout from gr_d3d_tmapper_multipass, names ours).
struct gr_tmapper_pass {
    gr::Mode mode;                    // +0x00
    int bitmap;                       // +0x04 -1 = untextured
    bool enabled;                     // +0x08 texture this pass
    bool vertex_color;                // +0x09 per-vertex colours (else r, g, b below)
    bool vertex_alpha;                // +0x0A per-vertex alpha (else a)
    ubyte stage;                      // +0x0B texture stage of the bitmap
    bool sectioned;                   // +0x0C the bitmap is split in sections (gr_poly_get_section)
    ubyte r, g, b;                    // +0x0D
    ubyte a;                          // +0x10
    ubyte _pad11[3];
};
LAYOUT_CHECK(gr_tmapper_pass, sizeof(gr_tmapper_pass) == 0x14);
// A gr_tmapper_multipass vertex: the projected vertex and, per pass, uv and colour (12 bytes each).
struct gr_pass_vertex_data {
    float u, v;                       // +0x00
    ubyte r, g, b, a;                 // +0x08
};
struct gr_multipass_vertex {
    gr_vertex *v;                     // +0x00
    gr_pass_vertex_data pass[1];      // +0x04, one per pass (as many as the caller has)
};
// (nv, verts, pass, bm, &u_offset, &v_offset, &u_scale, &v_scale): the section of bm the polygon's uvs
// fall in and the uv transform into it (vsdk\gr; sum.symbols' "(bitmap, u, v, ...)" is the inner part).
int gr_poly_get_section(int nv, gr_multipass_vertex **verts, int pass, int bm, float *u_offset,
                        float *v_offset, float *u_scale, float *v_scale);   // 0x55fb00

// ---- gr.cpp (decomp40) ------------------------------------------------------------------------------
// gr_vsync is gr_screen.vsync (+0xa0, D2000; 0 = flip with DDFLIP_NOVSYNC).
#define gr_vsync (gr_screen.vsync)                    // 0x25d8dd0

// .data (0x5a6058-0x5a606b)
extern float Gr_alpha_fog_ratio;                      // 0x5a6058 0.8, console "alpha_fog"
extern float Gr_level_fog_multiplier;                 // 0x5a605c 1.0, console "level_fog"
extern float gr_fade_level;                           // 0x5a6060 below 0 = no overlay
extern float gr_gamma;                                // 0x5a6064 1.0
extern int gr_gamma_percent;                          // 0x5a6068 180, (int)(gamma * 100) after gr_set_gamma (name ours; no reader)
// .bss
extern RECT gr_fullscreen_rect;                       // 0x25d88b0 {0, 0, w - 1, h - 1} from gr_init fullscreen (name ours; no reader)
extern float gr_fade_duration;                        // 0x25d8d20
extern float gr_fade_time;                            // 0x25d8dec
extern bool gr_initialized;                           // 0x25d921e
extern int Gr_show_stats;                             // 0x25d9220 console "gr_show_stats" (no reader in the game)
extern bool cmd_antialiasing_on;                      // 0x25d9224
extern int gr_fade_state;                             // 0x25d9228 0 none, 1 fading in, 2 fading out
extern bool gr_zbuffer_enabled;                       // 0x25d922c gr_zbuffer_clear's argument (name ours; no reader)

void gr_check_dual_monitors();                        // 0x506110
void gr_set_vsync(bool on);                           // 0x506160
void gr_deactivate();                                 // 0x506190
void gr_init_context(HWND hwnd);                      // 0x5061b0
// (name, format, path_id): format 0 = .tga (RLE, bottom-up), else .jpg; the extension is appended.
void gr_screenshot(const char *name, int format, int path_id);   // 0x5061f0
float gr_get_fade_level();                            // 0x506680
void gr_draw_fade();                                  // 0x5066b0
void gr_set_aspect(float aspect);                     // 0x506960
void gr_flip();                                       // 0x506a20
void gr_zbuffer_clear(bool enable);                   // 0x506a50
void gr_set_texture(int bm, int bm2);                 // 0x506c50
void gr_circle(int x, int y, int radius, gr::Mode mode);   // 0x507690
void gr_tmapper(int nv, gr_vertex **verts, gr::Mode mode); // 0x507a20
void gr_tmapper_multipass(int nv, gr_multipass_vertex **verts, int npasses,
                          gr_tmapper_pass *passes);   // 0x507a80
void gr_noop_507d20(int flags);                       // 0x507d20 empty (gr_bitmap_3d_projected passes 0x10)
void gr_set_fog(bool enable, int r, int g, int b, float near_dist, float far_dist);   // 0x507db0
void gr_set_tint(bool enable, float r, float g, float b);   // 0x507f70
void gr_stub_ret_507fa0();                            // 0x507fa0 empty, no reference
void gr_finish();                                     // 0x507fb0
void gr_tcache_flush();                               // 0x507fd0
bool gr_stub_ret_false_508060();                      // 0x508060 no reference
int  gr_stub_ret0_508070();                           // 0x508070 no reference
void gr_stub_ret_508080();                            // 0x508080 empty, no reference
void gr_set_antialias(bool on);                       // 0x5080c0
void gr_flush_pipeline();                             // 0x508140
// vsdk\gr, file not known: flushes the buffered polygons (gr_init_context, gr_flip).
void gr_poly_buffer_flush();                          // 0x545d70

// ---- the texture stage slot (decomp45) ---------------------------------------------------------------
// The common head of the per-stage / per-TMU texture cache records (0x18 bytes): the Glide TMU record
// (glide_record, 0x48, vsdk/gr/glide/gr_glide.h) derives from it and calls is_bound / invalidate_bitmap on
// its glide_tmus (ecx = the record, 0x5495a2, 0x549706). The members lie in the Direct3D texture cache
// object (0x54d470-0x54d5bf, vsdk\gr in sum.symbols, file not known). No constructor is declared: the
// Glide record's constructor 0x549780 sets these fields itself instead of calling one (D2502).
// decomp69 (D5020): the four members are header inlines. Their copies are COMDATs at the end of the
// Direct3D texture cache object, after its last regular function and in the order of their first use
// there (interleaved with the Direct3D slot's own inlines); the Glide object, built without inline
// expansion, calls the same copies (one COMDAT kept by the linker).
class tcache_slot {
public:
    void init_stage(int stage)                        // 0x54d490 tcache_slot__init_stage
    {
        initialized = true;
        this->stage = stage;
        bitmap = -1;
        section = -1;
    }
    void reset()                                      // 0x54d4b0 tcache_slot__reset
    {
        bitmap = -1;
        section = -1;
    }
    // true (and the cached scales) when that bitmap section is the one bound.
    bool is_bound(int bitmap, int section, float *u_scale, float *v_scale)   // 0x54d4f0 tcache_slot__is_bound
    {
        if (this->bitmap == bitmap && this->section == section) {
            *u_scale = this->u_scale;
            *v_scale = this->v_scale;
            return true;
        }
        return false;
    }
    void invalidate_bitmap(int bitmap)                // 0x54d5a0 tcache_slot__invalidate_bitmap
    {
        if (this->bitmap == bitmap)
            this->bitmap = -1;
    }
    bool initialized;                                 // +0x00
    ubyte _pad01[3];
    int stage;                                        // +0x04 texture stage / Glide TMU
    int bitmap;                                       // +0x08 bound bitmap (frame) index, -1 none
    int section;                                      // +0x0c bound section, -1 none
    float u_scale;                                    // +0x10 scales of the bound section
    float v_scale;                                    // +0x14
};
LAYOUT_CHECK(tcache_slot, sizeof(tcache_slot) == 0x18);

// decomp57 (the shadow / obstacle-debug unit): vsdk\gr calls not declared before.
// (p0, p1, mode): both points through gr_rotate_vertex, then gr_line_3d.
// (out, in): (in - view position) rotated into the view; returns the clip codes.
// The buffered / immediate polygon calls (0x545150-0x5457b0).
// decomp69 (D5010, the unit src/decomp/vsdk/unnamed_gr_poly_0x5450c0.cpp): (buffered, z, a2, fixed_depth,
// depth). buffered records the polygon into the sorted command buffer (z is its sort key, gr_poly passes
// the largest view z); fixed_depth makes every vertex z = depth, sw = 1 / depth when drawn; a2 is stored
// and never read. set_texture: (mode, bm, r, g, b, a, stage), the colour bytes are ubyte (the tint code
// masks them with 0xff); gr_poly_end returns false when the polygon is rejected. The rest of the unit:
// vsdk/gr/unnamed_gr_poly_0x5450c0.h.
void gr_poly_begin(bool buffered, float z, int a2, bool fixed_depth, float depth);      // 0x545150
void gr_poly_vertex(gr_vertex *v);                                             // 0x5452c0
void gr_poly_set_texture(gr::Mode mode, int bm, ubyte r, ubyte g, ubyte b, ubyte a, int stage);   // 0x545340
void gr_poly_uv(float u, float v);                                             // 0x5455a0
void gr_poly_alpha(ubyte a);                                                   // 0x545760
// gr_poly_rgb (0x545610) and gr_bitmap_3d_oriented (0x53f410), used by the vfx mesh unit (decomp70), are declared
// in vsdk/gr/unnamed_gr_poly_0x5450c0.h and vsdk/gr/unnamed_gr_line_0x53e520.h with their definitions.
bool gr_poly_end();                                                            // 0x5457b0


// ---- decomp63 (the render-scene unit 0x481680; vsdk\gr in sum.symbols) ------------------------------
// Gr_3d_zmult (0x5a72a0, console "zmult": levelrender_render_scene sets 1.0 during the frame, 0.2 after it,
// unless game_flags & 0x120), gr_start_instance, gr_stop_instance and gr_set_far_clip are declared in the
// 3D view unit's header below.
// (p0, p1, width, mode): a camera-facing quad along p0-p1 (the line trails of the effects).
// decomp69 (D5004): returns true; always buffered (gr_poly_begin(true, centre z, ...)).
bool gr_line_quad_3d(const vector3 *p0, const vector3 *p1, float width, gr::Mode mode);   // 0x53f470
// decomp65: the 3D view state unit (0x50d9b0-0x50f9cf): clip planes, the instance stack, the saved state,
// the fog / depth inlines.
#include "vsdk/gr/unnamed_gr_3d_0x50d9b0.h"

#endif
