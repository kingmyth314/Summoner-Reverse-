// vsdk/gr/directx/gr_direct3d.h - the Direct3D 7 back end (vsdk\gr\directx\gr_direct3d.cpp, decomp36):
// renderer 0x66, the retail default. gr.cpp dispatches to it on gr_screen.mode == GR_DIRECT3D; the level
// renderer (levelrender_draw_list_d3d), the character mesh renderer (vsdk\ca) and the particle code
// (vfx_emitter_instance__render_d3d) use the state functions, the vertex buffer and the texture cache
// directly. Not original source.
//
// sum.symbols tags two code blocks with this file: 0x534a20-0x536944 (state, drawing) and 0x538650-0x53ac18
// (init, frame, fog, cmdline). Between them lies the vfx object (0x536950-0x538630, vsdk\geom), and the
// first block ends with its own out-of-line header inlines (gr::Mode, d3d_f2dw), so the first block is a
// separate object, file name unknown (DECISIONS.md D1601). Both are declared here.
//
// Includes d3d_min.h: do not include this header in a unit that includes the system DirectX headers.
#ifndef VSDK_GR_DIRECTX_GR_DIRECT3D_H
#define VSDK_GR_DIRECTX_GR_DIRECT3D_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/cmdline.h"
#include "vsdk/gr/directx/d3d_min.h"
#include "vsdk/os/mouse.h"     // clamp_int (moved there by decomp49, D2902)

// ---- records -------------------------------------------------------------------------------------------
// The vertex every draw call sends (0x28 bytes, FVF 0x2c4: XYZRHW, diffuse, specular, two texture
// coordinate sets). The alpha byte of the specular colour carries the fog (gr_get_fog_alpha). Name ours.
struct d3d_tlvertex {
    float sx, sy, sz, rhw;            // +0x00
    union {                           // +0x10 diffuse ARGB
        D3DCOLOR color;
        ubyte color_bytes[4];         // b, g, r, a (levelrender's draw list writes the bytes; decomp76)
    };
    union {                           // +0x14
        D3DCOLOR specular;
        ubyte specular_bytes[4];      // [3] = fog alpha
    };
    float tu, tv;                     // +0x18 stage 0
    float tu2, tv2;                   // +0x20 stage 1
};
LAYOUT_CHECK(d3d_tlvertex, sizeof(d3d_tlvertex) == 0x28);

// One enumerated device (0x428 bytes): the DirectDraw driver (d3d_ddraw_devices, filled by
// d3d_ddraw_enum_callback) or a Direct3D device of one (d3d_3d_devices, d3d_enum_devices_callback). A
// NULL guid pointer means the primary display driver. Names ours.
struct d3d_device_record {
    GUID ddraw_guid;                  // +0x000
    GUID *ddraw_guid_ptr;             // +0x010 &ddraw_guid or NULL
    GUID d3d_guid;                    // +0x014 D3DDEVICEDESC7.deviceGUID
    GUID *d3d_guid_ptr;               // +0x024 &d3d_guid
    char name[0x400];                 // +0x028 description ("Direct 3D - " + driver for a 3D device)
};
LAYOUT_CHECK(d3d_device_record, sizeof(d3d_device_record) == 0x428);
#define D3D_MAX_DDRAW_DEVICES 8       // 0x2dc07c8..0x2dc2908
#define D3D_MAX_3D_DEVICES    16      // 0x2dc2990..0x2dc6c10

// Bit layout of one colour channel of the render target (d3d_compute_pixel_format_shifts). Names ours.
struct d3d_color_channel {
    int bits;                         // +0x00 width of the mask
    int shift;                        // +0x04 position of its lowest bit
    int scale;                        // +0x08 255 / (mask >> shift)
    uint mask;                        // +0x0C
};

// ---- the state / drawing object (0x534a20-0x536944, file name unknown, D1601) ----------------------
extern bool gr_d3d_vertex_color;                      // 0x5a9cf8 (1) colour source uses vertex colours
extern bool gr_d3d_vertex_alpha;                      // 0x5a9cf9 (1) alpha source uses vertex alpha
// Texture coordinate scales of the bitmap bound on stage 0 / 1. sum.symbols had the pair swapped
// (D1602): gr_d3d_tcache_set gets &gr_d3d_texture_scale_u[s] as its u scale and the code multiplies tu
// by it.
extern float gr_d3d_texture_scale_v[2];               // 0x2d90ed4
extern float gr_d3d_texture_scale_u[2];               // 0x2d90edc
extern gr::Mode gr_d3d_current_mode;                  // 0x2d90ee4

void gr_d3d_set_state(gr::Mode mode);                 // 0x534a40
void gr_d3d_invert_depth_test();                      // 0x535420
void gr_d3d_restore_depth_test();                     // 0x535450
void gr_d3d_apply_tint(int *r, int *g, int *b);       // 0x535480
void gr_d3d_set_zbias(int bias);                      // 0x535510
void gr_d3d_page_in(int bm);                          // 0x535520
void gr_d3d_clear();                                  // 0x535550
void gr_d3d_zbuffer_clear();                          // 0x5355e0
void gr_d3d_set_viewport();                           // 0x535670
void gr_d3d_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                   gr::Mode mode);                    // 0x5356d0
void gr_d3d_set_bitmap(gr::Mode mode, int bm, int stage);   // 0x535ce0 (stage unused)
void gr_d3d_set_color();                              // 0x535d10 empty
void gr_d3d_fog_alpha(float z, D3DCOLOR *out);        // 0x535d20 (no caller)
void gr_d3d_line(float x1, float y1, float x2, float y2, gr::Mode mode);   // 0x535da0
void gr_d3d_make_line_verts(d3d_tlvertex *v0, d3d_tlvertex *v1, int x0, int y0, int x1,
                            int y1);                  // 0x535eb0
void gr_d3d_line_3d(gr_vertex *v0, gr_vertex *v1, gr::Mode mode);           // 0x535fd0
void gr_d3d_tmapper(int nv, gr_vertex **verts, gr::Mode mode);              // 0x5361a0
void gr_d3d_tmapper_multipass(int nv, gr_multipass_vertex **verts, int npasses,
                              gr_tmapper_pass *passes);                     // 0x536360
bool gr_d3d_has_bump_format();                        // 0x536790
bool gr_d3d_set_bump_env(float lscale, float loffset, float m00, float m01, float m10,
                         float m11);                  // 0x5367a0
// The DirectX SDK's F2DW helper: a header inline, out of line at the end of the object (0x536940).
inline DWORD d3d_f2dw(float f) { return *(DWORD *)&f; }

// ---- gr_direct3d.cpp (0x538650-0x53ac18) ---------------------------------------------------------------
extern DWORD gr_d3d_lock_discard;                     // 0x5a9d24 0x2820, 0x2020 with the vertex buffer
extern DWORD gr_d3d_lock_nooverwrite;                 // 0x5a9d28 0x1820, 0x1020 with the vertex buffer
extern bool d3d_windowed;                             // 0x5a9d34 window modes 200 / 202 (name ours)
extern float d3d_last_fog_start;                      // 0x5a9d38 -1.0, last FOGSTART sent (name ours)
extern float d3d_last_fog_end;                        // 0x5a9d3c -1.0, last FOGEND sent (name ours)

extern D3DDEVICEDESC7 d3d_device_caps;                // 0x2dbfd20
extern DDGAMMARAMP d3d_saved_gamma_ramp;              // 0x2dbfe10
extern DDCAPS d3d_hel_caps;                           // 0x2dc0410 (name ours)
extern float d3d_fog_start;                           // 0x2dc058c
extern DDPIXELFORMAT d3d_screen_pixel_format;         // 0x2dc0590 the render target's (name ours)
extern RECT d3d_window_rect;                          // 0x2dc05b0
extern int d3d_zfunc;                                 // 0x2dc05c0 D3DCMP_LESSEQUAL or _GREATEREQUAL
extern d3d_color_channel d3d_green_channel;           // 0x2dc05c8 (name ours)
extern float d3d_fog_end;                             // 0x2dc05d8
extern CmdLineParam cmdline_param_vertexfog;          // 0x2dc05e0
extern d3d_color_channel d3d_blue_channel;            // 0x2dc05f8 (name ours)
extern DDCAPS d3d_driver_caps;                        // 0x2dc0608 (name ours)
extern DDPIXELFORMAT d3d_texture_format_a;            // 0x2dc0798 best 16-bit RGB texture format
extern d3d_color_channel d3d_red_channel;             // 0x2dc07b8 (name ours)
extern d3d_device_record d3d_ddraw_devices[D3D_MAX_DDRAW_DEVICES];   // 0x2dc07c8 (name ours)
extern CmdLineParam cmdline_param_rz;                 // 0x2dc2908
extern DDPIXELFORMAT d3d_texture_format_b;            // 0x2dc2920 best alpha texture format (< 8 bits)
extern DDPIXELFORMAT d3d_texture_format_c;            // 0x2dc2940 16-bit du/dv bump format
extern int d3d_flag_2dc2960;                          // 0x2dc2960 (1 at init, never read; name ours)
extern RECT d3d_screen_rect;                          // 0x2dc2968
extern float d3d_zclear_value;                        // 0x2dc2988
extern d3d_device_record d3d_3d_devices[D3D_MAX_3D_DEVICES];         // 0x2dc2990 (name ours)
extern CmdLineParam cmdline_param_fb2;                // 0x2dc6c10
extern IDirect3DVertexBuffer7 *d3d_vertex_buffer;     // 0x2dc6c28
extern int d3d_vertex_buffer_pos;                     // 0x2dc6c2c
extern bool d3d_s3_depth;                             // 0x2dc6c30 S3 cards: depth test can be inverted (name ours)
// d3d_use_vertex_buffer (0x2dc6c31) is declared in gr.h.
extern bool d3d_ati_5144;                             // 0x2dc6c32 set for ATI device 0x5144, never read (name ours)
extern LPDIRECTDRAWCREATEEX d3d_ddraw_enumerate_ex;   // 0x2dc6c34 DirectDrawCreateEx
extern IDirectDraw7 *d3d_ddraw;                       // 0x2dc6c38
// d3d_direct3d (0x2dc6c3c) is declared in gr.h as void * (levelrender.cpp casts it to the system
// IDirect3D7, D1604); this unit reads it through D3D_DIRECT3D.
#define D3D_DIRECT3D ((IDirect3D7 *)d3d_direct3d)
extern IDirect3DDevice7 *d3d_device;                  // 0x2dc6c40
// Names as corrected in sum.symbols (D1603; the pair used to be swapped): d3d_back_surface is the render
// target (back buffer / offscreen plain surface), d3d_primary_surface the primary (front) surface.
extern IDirectDrawSurface7 *d3d_back_surface;      // 0x2dc6c44
extern IDirectDrawSurface7 *d3d_primary_surface;         // 0x2dc6c48
extern IDirectDrawSurface7 *d3d_zbuffer_surface;      // 0x2dc6c4c
extern IDirectDrawGammaControl *d3d_gamma_control;    // 0x2dc6c50
extern int d3d_texture_format_b_bits;                 // 0x2dc6c54
extern int d3d_texture_format_a_bits;                 // 0x2dc6c58
extern bool d3d_texture_format_c_found;               // 0x2dc6c5c
extern D3DDEVICEDESC7 *d3d_device_desc;               // 0x2dc6c60 = &d3d_device_caps
extern int d3d_scene_depth;                           // 0x2dc6c64
extern int d3d_device_ready;                          // 0x2dc6c68
extern float d3d_bitmap_u_offset;                     // 0x2dc6c6c never written (0.0)
extern float d3d_bitmap_v_offset;                     // 0x2dc6c70 never written (0.0)
extern float d3d_line_pixel_offset;                   // 0x2dc6c74 never written (0.0)
extern int d3d_min_texture_w;                         // 0x2dc6c78 (names ours, the next five)
extern int d3d_max_texture_w;                         // 0x2dc6c7c
extern int d3d_min_texture_h;                         // 0x2dc6c80
extern int d3d_max_texture_h;                         // 0x2dc6c84
extern int d3d_square_textures_only;                  // 0x2dc6c88
extern int d3d_pow2_textures_only;                    // 0x2dc6c8c
extern bool d3d_multitexture_supported;               // 0x2dc6c90
extern HWND d3d_hwnd;                                 // 0x2dc6c94
// Set from GetAvailableVidMem's free texture memory in bytes, despite the name (sum.symbols).
extern int gr_d3d_video_memory_mb;                    // 0x2dc6c98
extern int d3d_fog_mode;                              // 0x2dc6c9c 0 none, 1 table, 2 w table, 3 vertex, 4, 5 (-fb2 / S3)
extern int d3d_init_done;                             // 0x2dc6ca0 (name ours)
extern volatile int d3d_flush_request;                // 0x2dc6ca4 never set (name ours)
extern volatile int d3d_flush_request_b;              // 0x2dc6ca8 never set (name ours)
extern int d3d_num_ddraw_devices;                     // 0x2dc6cac
extern int d3d_num_3d_devices;                        // 0x2dc6cb0

void gr_d3d_set_render_state(DWORD state, DWORD value);   // 0x538740
void gr_d3d_draw_primitive(DWORD type, DWORD fvf, void *verts, DWORD count, DWORD flags);   // 0x538760
d3d_device_record *d3d_enumerate_ddraw_devices();     // 0x538850 the last 3D device found, or NULL
HRESULT CALLBACK d3d_enum_devices_callback(LPSTR description, LPSTR name, D3DDEVICEDESC7 *desc,
                                           LPVOID context);                  // 0x538920
BOOL WINAPI d3d_ddraw_enum_callback(GUID *guid, LPSTR description, LPSTR name, LPVOID context);   // 0x5389f0
HRESULT gr_d3d_set_default_transforms(IDirect3DDevice7 *dev, float zfar);   // 0x538a70
void gr_d3d_set_window(HWND hwnd);                    // 0x538b70
const char *ddraw_error_string(HRESULT hr);           // 0x538c70
void gr_d3d_init(HWND hwnd);                          // 0x5394e0
void gr_d3d_begin_scene();                            // 0x539590
void gr_d3d_init_device();                            // 0x5395c0
void d3d_pixel_format_masks_valid(DDPIXELFORMAT *pf); // 0x53a170 (walks the masks, result unused)
HRESULT CALLBACK d3d_enum_texture_format_callback(DDPIXELFORMAT *pf, LPVOID context);   // 0x53a1d0
void gr_d3d_noop_hook(int arg);                       // 0x53a360
void d3d_compute_pixel_format_shifts(DDSURFACEDESC2 *desc);   // 0x53a370
HRESULT CALLBACK d3d_enum_zbuffer_callback(DDPIXELFORMAT *pf, LPVOID context);   // 0x53a4c0
void gr_d3d_close();                                  // 0x53a4f0
void gr_d3d_flip_surface();                           // 0x53a5f0
void gr_d3d_end_scene();                              // 0x53a610
void gr_d3d_close_thunk();                            // 0x53a640
void gr_d3d_flip_thunk();                             // 0x53a650
void gr_d3d_flip();                                   // 0x53a660
int  gr_d3d_read_backbuffer(int x, int y, int w, int h, void *buf);   // 0x53a7f0 returns 3 (565)
void gr_d3d_set_fog(int set_range);                   // 0x53a8c0
bool gr_d3d_is_mode_supported(gr::Mode *mode);        // 0x53aad0
int  gr_d3d_get_video_memory_mb();                    // 0x53aaf0
void gr_d3d_set_antialias(bool on);                   // 0x53ab00
void gr_d3d_set_gamma();                              // 0x53ab20
void gr_d3d_set_alpha_test(bool on);                  // 0x53ab80
void gr_d3d_flush_pipeline();                         // 0x53abb0
// 0x53ac00 guid_equal is objbase.h's IsEqualGUID inline, out of line at the end of the object.

// ---- the Direct3D texture cache (vsdk\gr\directx, 0x54bb30-0x54d5b5, file not known) -------------------
// decomp69: source src/decomp/vsdk/unnamed_d3d_tcache_0x54bb30.cpp (file name a placeholder).
// One section of a bitmap's texture (0x20 bytes; names ours): a bitmap wider or taller than the largest
// texture is cut into tiles of at most gr_screen.max_bitmap_w x max_bitmap_h.
struct d3d_tcache_section {
    IDirectDrawSurface7 *surface;     // +0x00 the managed texture surface (top mip level), NULL none
    int size;                         // +0x04 bytes counted in d3d_texture_mem_used (w * h * 2)
    float u_scale;                    // +0x08 section width / texture width
    float v_scale;                    // +0x0c
    int x, y;                         // +0x10 position of the tile in the bitmap
    int w, h;                         // +0x18 tile size
};
LAYOUT_CHECK(d3d_tcache_section, sizeof(d3d_tcache_section) == 0x20);
// One entry per bitmap frame (MAX_BITMAPS of them, indexed by bm_get_current_frame; 0x10 bytes, names ours).
struct d3d_tcache_entry {
    int bitmap;                       // +0x00 the handle loaded (as passed to gr_d3d_tcache_set), -1 none
    int num_sections;                 // +0x04
    bool dirty;                       // +0x08 gr_d3d_mark_texture_dirty: reload on the next set
    ubyte _pad09[3];
    d3d_tcache_section *sections;     // +0x0c operator new'd array
};
LAYOUT_CHECK(d3d_tcache_entry, sizeof(d3d_tcache_entry) == 0x10);

extern d3d_tcache_entry *d3d_tcache_entries;          // 0x2f13308
extern int d3d_texture_mem_used;                      // 0x2f13314

// The texture stage record of the Direct3D cache (decomp69, D5021): tcache_slot (gr.h) plus a constructor
// and the bind / unbind members, all header inlines (COMDATs 0x54d470, 0x54d4c0, 0x54d530 at the end of
// the cache object). sum.symbols calls the constructor d3d_tcache_slot__ctor; the OpenGL back end does not
// use it (its array is the two d3d_tcache_slots). The constructor leaves the stage unset.
class d3d_tcache_slot : public tcache_slot {
public:
    d3d_tcache_slot()                                 // 0x54d470 d3d_tcache_slot__ctor
    {
        initialized = false;
        bitmap = -1;
        section = -1;
        u_scale = 0.0f;
        v_scale = 0.0f;
    }
    // Unbinds the stage (unless d3d_s3_depth is set), forgets the bitmap but not the section.
    void d3d_unbind()                                 // 0x54d4c0 tcache_slot__d3d_unbind
    {
        if (!d3d_s3_depth)
            IDirect3DDevice7_SetTexture(d3d_device, stage, NULL);
        bitmap = -1;
    }
    // Binds section `section` of the entry and returns its scales.
    void d3d_bind(d3d_tcache_entry *e, int section, float *u_scale, float *v_scale)   // 0x54d530 tcache_slot__d3d_bind
    {
        d3d_tcache_section *s = &e->sections[section];

        e->dirty = false;
        IDirect3DDevice7_SetTexture(d3d_device, stage, s->surface);
        *u_scale = s->u_scale;
        *v_scale = s->v_scale;
        this->section = section;
        this->bitmap = e - d3d_tcache_entries;
        this->u_scale = s->u_scale;
        this->v_scale = s->v_scale;
    }
};
LAYOUT_CHECK(d3d_tcache_slot, sizeof(d3d_tcache_slot) == 0x18);
#define D3D_TCACHE_NUM_SLOTS 2
extern d3d_tcache_slot d3d_tcache_slots[D3D_TCACHE_NUM_SLOTS];   // 0x2f132d8

// (stage, bm, section, &u_scale, &v_scale, bind); bm < 0 unbinds the stage. Returns 0 when the texture
// cannot be created. decomp69 (D5022): the last argument is a bool (tested as a byte): bind the section
// to the stage (false only makes sure the texture is loaded); was int fail_on_full.
int  gr_d3d_tcache_set(int stage, int bm, int section, float *u_scale, float *v_scale, bool bind);   // 0x54d050
void gr_d3d_tcache_init();                            // 0x54bb60
void gr_d3d_tcache_close();                           // 0x54bbb0
void gr_d3d_tcache_flush();                           // 0x54bc60
void gr_d3d_tcache_frame();                           // 0x54bc90 empty
int  gr_d3d_get_section_count(int bm);                // 0x54cfe0
void gr_d3d_get_section_rect(int bm, int section, int *x, int *y, int *w, int *h);   // 0x54d000
// decomp40 (gr.cpp dispatch): (bm): the dirty flag of the bitmap's entry / release of its slots.
void gr_d3d_mark_texture_dirty(int bm);               // 0x54d3f0
void gr_d3d_tcache_free(int bm);                      // 0x54d430
// decomp69: the rest of the cache.
void gr_d3d_tcache_free_entry(d3d_tcache_entry *e);   // 0x54bbf0
// (bm, entry, section, data, palette, bm_w, bm_h, format, num_levels): creates the managed texture of one
// section (mip levels when num_levels > 1); 1, or -1 when the surface cannot be created or locked.
int  gr_d3d_create_texture_section(int bm, d3d_tcache_entry *e, int section, ubyte *data, ubyte *palette,
                                   int bm_w, int bm_h, int format, int num_levels);   // 0x54bca0
// (pf, entry, section, locked desc, format, tex_h, tex_w, bm_h, bm_w, data, palette): converts the bitmap
// into the locked surface's pixel format.
void gr_d3d_copy_bitmap_to_surface(DDPIXELFORMAT *pf, d3d_tcache_entry *e, int section, DDSURFACEDESC2 *ddsd,
                                   int format, int tex_h, int tex_w, int bm_h, int bm_w, ubyte *data,
                                   ubyte *palette);   // 0x54c0a0
int  gr_d3d_create_texture(int bm, d3d_tcache_entry *e);   // 0x54d120 1, 0 when it is not drawn
// (w, h): true when both are already a texture size (4, 8, ..., 128, 256; larger rounds to 256).
bool gr_d3d_is_texture_size(int w, int h);         // 0x54d330

#endif
