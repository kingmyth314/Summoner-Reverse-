// gr.cpp - Sum.exe v1.40, code 0x505550-0x5081bf (vsdk\gr\gr.cpp): the renderer-independent graphics
// layer. Every drawing call checks gr_screen.mode and goes to the Direct3D (0x66, the retail default),
// Glide (0x67) or OpenGL (0x68, what SummonerFix selects) back end. Also: the 30 render-mode globals,
// gr_screen, four console commands, gr_init's window setup, clipping, fades, gamma, screenshots.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Like the other vsdk objects this unit was built without inline expansion: the mode initialisers call
// the header inline gr::Mode(6 fields) out of line, and its COMDAT copy, the implicit gr__Screen
// constructor and gr_vertex's constructor sit at the end of the unit (0x508160-0x5081bf), after the
// last function of the source, in the order the unit first uses them (D2001, D2002).
//
// Data of this unit: .data 0x5a6058-0x5a6397 (five variables, then the string literals in function
// order; the two copies of the path show that string pooling is off), .bss 0x25d88a0-0x25d9237 (the 30
// gr::Mode objects, gr_color_gamma_lut, gr_screen, four console_command objects, gr_gamma_ramp, the
// flags). The .bss addresses do not follow the source order (the mode objects come in initialiser order
// 0x5887ec-0x588860 below, the plain .bss in address order); the dwords 0x25d8cec, 0x25d8d2c,
// 0x25d8ddc, 0x25d8dfc and 0x25d920c have no reference and are the alignment in front of the 16-byte
// aligned objects that follow them.
//
// SummonerFix: patches LoadSettings, not this unit (renderer 0x67 -> 0x68, window mode 201 -> 200, so
// gr_init takes the windowed branch), and hooks GetSystemMetrics for the calls from gr_init: the
// windowed (and desktop) aspect comes from the desktop size, not from the game's resolution.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/frametime.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/bmpman/targautils.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/gr/glide/gr_glide.h"
#define GL_MIN_NO_DLLIMPORT                           // glFinish through the linker thunk (D2004)
#include "vsdk/gr/opengl/gr_opengl.h"

// Original __FILE__ of this unit (0x5a62d8 and 0x5a6364; string pooling is off).
#define GR_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\gr\\gr.cpp"

// ---- .data (0x5a6058-0x5a606b) -----------------------------------------------------------------------
float Gr_alpha_fog_ratio = 0.8f;                      // 0x5a6058 console "alpha_fog"
float Gr_level_fog_multiplier = 1.0f;                 // 0x5a605c console "level_fog"
float gr_fade_level = 1.0f;                           // 0x5a6060
float gr_gamma = 1.0f;                                // 0x5a6064
int gr_gamma_percent = 180;                           // 0x5a6068 (name ours; written by gr_set_gamma only)

// ---- the render modes (dynamic initialisers, table 0x5887ec-0x588860) --------------------------------
// Each initialiser is two compiler-generated functions: the table entry (`jmp` to the next one) and the
// body (`mov ecx, &global; call gr::Mode::Mode(6 fields)`, 0x508160). One sum.symbols entry covers both.
// gr::Mode(texture source, colour source, alpha source, alpha blend, zbuffer type, fog type).

// 0x505550 init_gr_mode_25d8df4 (body 0x505560)  Confidence: exact.
gr::Mode gr_string_mode(3, 2, 3, 3, 0, 3);            // 0x25d8df4
// 0x505580 init_gr_mode_25d8cdc (body 0x505590)  Confidence: exact.
gr::Mode gr_mode_25d8cdc(3, 2, 3, 3, 0, 3);           // 0x25d8cdc no reader found
// 0x5055b0 init_gr_mode_25d8d08 (body 0x5055c0)  Confidence: exact.
gr::Mode gr_mode_ui_bitmap(1, 1, 0, 3, 0, 3);         // 0x25d8d08
// 0x5055e0 init_gr_mode_25d88c4 (body 0x5055f0)  Confidence: exact.
gr::Mode gr_bitmap_wrap_mode(1, 2, 3, 3, 0, 3);       // 0x25d88c4
// 0x505610 init_gr_mode_25d8d10 (body 0x505620)  Confidence: exact.
gr::Mode gr_mode_automap_additive(1, 1, 0, 2, 0, 3);  // 0x25d8d10
// 0x505640 init_gr_mode_25d8ce4 (body 0x505650)  Confidence: exact.
gr::Mode gr_line_mode(0, 0, 0, 3, 0, 3);              // 0x25d8ce4
// 0x505670 init_gr_mode_25d88ac (body 0x505680)  Confidence: exact.
gr::Mode gr_rect_mode(0, 0, 0, 3, 0, 3);              // 0x25d88ac
// 0x5056a0 init_gr_mode_25d88a4 (body 0x5056b0)  Confidence: exact.
gr::Mode gr_mode_25d88a4(0, 0, 0, 3, 0, 3);           // 0x25d88a4
// 0x5056d0 init_gr_mode_25d8d28 (body 0x5056e0)  Confidence: exact.
gr::Mode gr_mode_flat_alpha(0, 0, 0, 3, 4, 0);        // 0x25d8d28
// 0x505700 init_gr_mode_25d88c0 (body 0x505710)  Confidence: exact.
gr::Mode gr_line_3d_mode(0, 0, 0, 3, 4, 0);           // 0x25d88c0
// 0x505730 init_gr_mode_25d8ce0 (body 0x505740)  Confidence: exact.
gr::Mode gr_mode_line_quad(1, 1, 2, 3, 0, 0);         // 0x25d8ce0
// 0x505760 init_gr_mode_25d88cc (body 0x505770)  Confidence: exact.
gr::Mode gr_mode_25d88cc(1, 2, 3, 3, 0, 0);           // 0x25d88cc
// 0x505790 init_gr_mode_25d8df0 (body 0x5057a0)  Confidence: exact.
gr::Mode gr_mode_particle_alpha(2, 2, 3, 3, 1, 0);    // 0x25d8df0
// 0x5057c0 init_gr_mode_25d8d24 (body 0x5057d0)  Confidence: exact.
gr::Mode gr_mode_particle_additive(2, 2, 3, 2, 1, 3); // 0x25d8d24
// 0x5057f0 init_gr_mode_25d88a8 (body 0x505800)  Confidence: exact.
gr::Mode gr_mode_level_alpha_add(1, 2, 3, 2, 1, 0);   // 0x25d88a8
// 0x505820 init_gr_mode_25d8cfc (body 0x505830)  Confidence: exact.
gr::Mode gr_mode_lamp_glow(1, 2, 3, 2, 0, 0);         // 0x25d8cfc
// 0x505850 init_gr_mode_25d88a0 (body 0x505860)  Confidence: exact.
gr::Mode gr_mode_25d88a0(1, 2, 0, 3, 4, 0);           // 0x25d88a0
// 0x505880 init_gr_mode_25d8cd4 (body 0x505890)  Confidence: exact.
gr::Mode gr_mode_vfx_mesh_opaque(1, 2, 0, 0, 4, 0);   // 0x25d8cd4
// 0x5058b0 init_gr_mode_25d8df8 (body 0x5058c0)  Confidence: exact.
gr::Mode gr_mode_mesh_alpha(1, 2, 3, 3, 4, 0);        // 0x25d8df8
// 0x5058e0 init_gr_mode_25d8d00 (body 0x5058f0)  Confidence: exact.
gr::Mode gr_mode_25d8d00(1, 2, 3, 3, 1, 0);           // 0x25d8d00
// 0x505910 init_gr_mode_25d8d0c (body 0x505920)  Confidence: exact.
gr::Mode gr_mode_shadow(1, 2, 3, 3, 1, 0);            // 0x25d8d0c
// 0x505940 init_gr_mode_25d8cd8 (body 0x505950)  Confidence: exact.
gr::Mode gr_mode_25d8cd8(2, 2, 0, 0, 4, 1);           // 0x25d8cd8
// 0x505970 init_gr_mode_25d88c8 (body 0x505980)  Confidence: exact.
gr::Mode gr_mode_25d88c8(2, 2, 3, 2, 1, 1);           // 0x25d88c8
// 0x5059a0 init_gr_mode_25d8dd8 (body 0x5059b0)  Confidence: exact.
gr::Mode gr_mode_25d8dd8(1, 1, 2, 5, 4, 2);           // 0x25d8dd8
// 0x5059d0 init_gr_mode_25d8d04 (body 0x5059e0)  Confidence: exact.
gr::Mode gr_mode_vfx_flat_additive(0, 0, 0, 2, 1, 0); // 0x25d8d04
// 0x505a00 init_gr_mode_25d8d1c (body 0x505a10)  Confidence: exact.
gr::Mode gr_mode_lightmap_multitexture(4, 1, 3, 3, 4, 0);   // 0x25d8d1c
// 0x505a30 init_gr_mode_25d8d18 (body 0x505a40)  Confidence: exact.
gr::Mode gr_mode_25d8d18(6, 2, 3, 3, 4, 0);           // 0x25d8d18
// 0x505a60 init_gr_mode_25d8cd0 (body 0x505a70)  Confidence: exact.
gr::Mode gr_mode_25d8cd0(4, 2, 3, 2, 1, 0);           // 0x25d8cd0
// 0x505a90 init_gr_mode_25d8ce8 (body 0x505aa0)  Confidence: exact.
gr::Mode gr_mode_25d8ce8(4, 2, 3, 2, 4, 0);           // 0x25d8ce8
// 0x505ac0 init_gr_mode_25d8d14 (body 0x505ad0)  Confidence: exact.
gr::Mode gr_mode_25d8d14(1, 1, 2, 3, 4, 0);           // 0x25d8d14

// 0x505af0 init_gr_screen (body 0x505b00: mov ecx, &gr_screen; jmp gr__Screen::gr__Screen)  Confidence: exact.
// The implicit constructor exists because of the two Color members (D2000).
gr__Screen gr_screen;                                 // 0x25d8d30

// ---- plain .bss, in address order --------------------------------------------------------------------
// The mode objects and the console commands are defined with their initialisers.
RECT gr_fullscreen_rect;                              // 0x25d88b0 (name ours; written by gr_init only)
float gr_color_gamma_lut[256];                        // 0x25d88d0 (D1302)
float gr_fade_duration;                               // 0x25d8d20
float gr_fade_time;                                   // 0x25d8dec
int gr_gamma_ramp[256];                               // 0x25d8e0c
bool gr_antialias_enabled;                            // 0x25d921c
bool gr_byte_25d921d;                                 // 0x25d921d set by the Direct3D code
bool gr_initialized;                                  // 0x25d921e
int Gr_show_stats;                                    // 0x25d9220
bool cmd_antialiasing_on;                             // 0x25d9224
int gr_fade_state;                                    // 0x25d9228 0 none, 1 fading in, 2 fading out
bool gr_zbuffer_enabled;                              // 0x25d922c (name ours; written by gr_zbuffer_clear only)
// The help text of the antialiasing command is a zeroed .bss object, not a literal (push 0x25d9230 at
// 0x505d95): an empty string. Name and size (at most 8: gr_fonts follows at 0x25d9238) ours.
char ccmd_antialiasing_help[8];                       // 0x25d9230

// ---- console commands --------------------------------------------------------------------------------
void cmd_level_fog();
void cmd_alpha_fog();
void cmd_gr_show_stats();
void cmd_antialiasing();

// 0x505b10 init_ccmd_level_fog_thunk  Confidence: exact. Generated: the initialiser table entry, jmp to the next.
// 0x505b20 init_ccmd_level_fog  Confidence: exact.
console_command ccmd_level_fog("level_fog", "Sets Gr_level_fog_multiplier", cmd_level_fog);   // 0x25d8cf0

// 0x505b40  Confidence: exact. The console's float command macro.
void cmd_level_fog()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_level_fog_multiplier = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "level_fog",
                "Gr_level_fog_multiplier");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "level_fog", Gr_level_fog_multiplier);
        print_to_console(string_buffer, 0);
    }
}

// 0x505bd0 init_ccmd_alpha_fog_thunk  Confidence: exact. Generated: the initialiser table entry, jmp to the next.
// 0x505be0 init_ccmd_alpha_fog  Confidence: exact.
console_command ccmd_alpha_fog("alpha_fog", "Sets Gr_alpha_fog_ratio", cmd_alpha_fog);   // 0x25d9210

// 0x505c00  Confidence: exact.
void cmd_alpha_fog()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Gr_alpha_fog_ratio = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "alpha_fog",
                "Gr_alpha_fog_ratio");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "alpha_fog", Gr_alpha_fog_ratio);
        print_to_console(string_buffer, 0);
    }
}

// 0x505c90 init_ccmd_gr_show_stats_thunk  Confidence: exact. Generated: the initialiser table entry, jmp to the next.
// 0x505ca0 init_ccmd_gr_show_stats  Confidence: exact.
console_command ccmd_gr_show_stats("gr_show_stats", "Toggles Gr_show_stats", cmd_gr_show_stats);   // 0x25d8de0

// 0x505cc0  Confidence: exact. The console's bool command macro on a dword.
void cmd_gr_show_stats()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Gr_show_stats = 1;
        else if (arg_type & ARG_FALSE)
            Gr_show_stats = 0;
        else if (arg_type & ARG_NONE)
            Gr_show_stats ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "gr_show_stats", "Gr_show_stats");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "gr_show_stats", Gr_show_stats ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x505d80 init_ccmd_antialiasing_thunk  Confidence: exact. Generated: the initialiser table entry.
// 0x505d90 init_ccmd_antialiasing  Confidence: likely (the help object). Name of the object ours
// (0x25d8e00 has no sum.symbols name).
console_command ccmd_antialiasing("antialiasing", ccmd_antialiasing_help, cmd_antialiasing);   // 0x25d8e00

// 0x505db0  Confidence: exact. Hand-written: no help or status branch.
void cmd_antialiasing()
{
    if (gr_antialias_enabled) {
        gr_set_antialias(!cmd_antialiasing_on);
        cmd_antialiasing_on = !cmd_antialiasing_on;
        print_to_console("antialiasing toggled!", 0);
    } else {
        print_to_console("antialiasing not supported on this card.", 0);
    }
}

// ---- init ------------------------------------------------------------------------------------------------

// 0x505e00  Confidence: likely (store order of the gr_screen fields, the work-area arithmetic).
// (w, h, renderer, window_mode, hwnd, far_z, vsync). Window modes: 200 windowed (a captioned window
// centred horizontally in the work area, at its top), 201 fullscreen (a topmost popup over the whole
// desktop), 202 desktop size (window left alone). The aspect is (width / height) * 0.75; windowed and
// desktop take the width and height from GetSystemMetrics, i.e. the desktop, not the game's resolution
// (what SummonerFix's GetSystemMetrics hook corrects).
int gr_init(int w, int h, int mode, int window_mode, HWND hwnd, float far_z, bool vsync)
{
    float aspect;

    memset(&gr_screen, 0, sizeof(gr_screen));
    gr_screen.max_w = w;
    gr_screen.max_h = h;
    gr_screen.mode = mode;
    gr_screen.window_mode = window_mode;
    gr_screen.aspect = 1.0f;
    gr_screen.offset_x = 0;
    gr_screen.offset_y = 0;
    gr_screen.clip_width = w;
    gr_screen.clip_height = h;
    gr_screen.clip_left = 0;
    gr_screen.clip_right = w - 1;
    gr_screen.clip_top = 0;
    gr_screen.clip_bottom = h - 1;
    gr_screen.far_zvalue = far_z;
    gr_screen.current_bitmap = -1;
    gr_screen.current_bitmap_2 = -1;
    gr_screen.fog_enabled = false;
    gr_screen.fog_near = 0.0f;
    gr_screen.fog_far = 0.0f;
    gr_screen.vsync = vsync;
    gr_screen.poly_arg_a4 = 0;
    gr_check_dual_monitors();

    switch (gr_screen.window_mode) {
    case 200: {
        RECT rect;
        RECT work;
        rect.left = 0;
        rect.top = 0;
        rect.right = gr_screen.max_w;
        rect.bottom = gr_screen.max_h;
        AdjustWindowRect(&rect, WS_CAPTION, FALSE);
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
        int x = (work.right - work.left - rect.right + rect.left) / 2 + work.left;
        if (x < work.left)
            x = work.left;
        int y = work.top;
        int ww = rect.right - rect.left;
        int wh = rect.bottom - rect.top;
        ShowWindow(hwnd, SW_SHOWNORMAL);
        SetWindowLongA(hwnd, GWL_STYLE, WS_CAPTION);
        SetWindowLongA(hwnd, GWL_EXSTYLE, 0);
        SetWindowPos(hwnd, HWND_NOTOPMOST, x, y, ww, wh, SWP_SHOWWINDOW);
        SetActiveWindow(hwnd);
        SetForegroundWindow(hwnd);
        aspect = (float)GetSystemMetrics(SM_CXSCREEN) / (float)GetSystemMetrics(SM_CYSCREEN);
        break;
    }
    case 201:
        SetWindowLongA(hwnd, GWL_EXSTYLE, mode == GR_GLIDE ? WS_EX_TRANSPARENT : 0);
        SetWindowLongA(hwnd, GWL_STYLE, WS_POPUP);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0);
        SetActiveWindow(hwnd);
        SetForegroundWindow(hwnd);
        gr_fullscreen_rect.left = 0;
        gr_fullscreen_rect.top = 0;
        gr_fullscreen_rect.right = gr_screen.max_w - 1;
        gr_fullscreen_rect.bottom = gr_screen.max_h - 1;
        aspect = (float)gr_screen.max_w / (float)gr_screen.max_h;
        break;
    case 202:
        aspect = (float)GetSystemMetrics(SM_CXSCREEN) / (float)GetSystemMetrics(SM_CYSCREEN);
        break;
    default:
        VSDK_ERROR(GR_CPP, 0x183, "Invalid window_mode passed to gr_init\n");
    }
    gr_screen.aspect = aspect * 0.75f;

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_init(hwnd);
        break;
    case GR_GLIDE:
        gr_glide_init(hwnd);
        break;
    case GR_OPENGL:
        gr_opengl_init(hwnd);
        break;
    }
    gr_set_fog(false, 0, 0, 0, -1.0f, -1.0f);
    gr_set_gamma(gr_gamma);
    gr_initialized = true;
    gr::reset_clip();
    gr_clear();
    if (hwnd)
        gr_flip();
    return 0;
}

// 0x506110  Confidence: exact (the store after the call). Fullscreen only: the 3dfx SST_DUALHEAD
// variable (a number other than 0) turns on the dual-monitor handling of the Glide back end.
void gr_check_dual_monitors()
{
    if (gr_screen.window_mode == 201) {
        char value[32];
        DWORD len = GetEnvironmentVariableA("SST_DUALHEAD", value, 31);
        gr_screen.dual_monitors = false;
        if (len >= 1 && atoi(value) != 0)
            gr_screen.dual_monitors = true;
    }
}

// 0x506160  Confidence: exact.
void gr_set_vsync(bool on)
{
    gr_screen.vsync = on;
}

// 0x506170  Confidence: exact.
int gr_screen_width()
{
    return gr_screen.max_w;
}

// 0x506180  Confidence: exact.
int gr_screen_height()
{
    return gr_screen.max_h;
}

// 0x506190  Confidence: exact. Reached from DebugError (through 0x531ca0).
void gr_deactivate()
{
    if (gr_initialized && gr_screen.mode == GR_GLIDE)
        gr_glide_deactivate();
}

// 0x5061b0  Confidence: exact. Glide has nothing to do.
void gr_init_context(HWND hwnd)
{
    gr_poly_buffer_flush();
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_window(hwnd);
        break;
    case GR_OPENGL:
        gr_opengl_init_context(hwnd);
        break;
    }
    gr::reset_clip();
}

// 0x5061f0  Confidence: likely (loop shapes, the per-case pitch). (name, format, path_id): grabs the clip
// rectangle; format 0 writes name.tga (24-bit RLE, rows bottom-up), anything else name.jpg (565 is
// expanded into a new buffer, 888 has red and blue swapped in place, 8888 is packed to 3 bytes in place).
void gr_screenshot(const char *name, int format, int path_id)
{
    int w = gr_screen.clip_width;
    int h = gr_screen.clip_height;
    int bpp;
    int pitch;
    char filename[512];
    ubyte *buf = new ubyte[w * h * 4];
    if (buf == NULL)
        return;
    switch (gr_read_backbuffer(gr_screen.clip_left, gr_screen.clip_top, w, h, buf)) {
    case BM_FORMAT_565:
        bpp = 2;
        pitch = w * 2;
        break;
    case BM_FORMAT_888:
        bpp = 3;
        pitch = w * 3;
        break;
    case BM_FORMAT_8888:
        bpp = 4;
        pitch = w * 4;
        break;
    default:
        VSDK_ERROR(GR_CPP, 0x22f, "The current screen mode doesn't support print screen... see your graphics guy.\n");
    }
    strcpy(filename, name);
    if (format == 0) {
        strcat(filename, ".tga");
        ubyte *rle = new ubyte[w * 8];
        if (rle == NULL) {
            delete[] buf;
            return;
        }
        File f;
        f.open(filename, 2, path_id);
        f.write_uint8(0);                             // id length
        f.write_uint8(0);                             // colour map type
        f.write_uint8(10);                            // RLE true colour
        f.write_uint16(0);                            // colour map start
        f.write_uint16(0);                            // colour map length
        f.write_uint8(0);                             // colour map depth
        f.write_uint16(0);                            // x origin
        f.write_uint16(0);                            // y origin
        f.write_uint16((ushort)w);
        f.write_uint16((ushort)h);
        f.write_uint8(24);
        f.write_uint8(0);
        for (int y = h - 1; y >= 0; y--) {
            int len = tga_rle_encode_row(rle, buf + y * pitch, 3, bpp, pitch);
            f.write(rle, len);
        }
        f.close();
        delete[] rle;
    } else {
        int x, y;
        strcat(filename, ".jpg");
        switch (bpp) {
        case 2: {
            ubyte *rgb = new ubyte[w * h * 3];
            ushort *row = (ushort *)buf;
            ubyte *d = rgb;
            for (y = 0; y < h; y++) {
                ushort *s = row;
                for (x = 0; x < w; x++) {
                    ushort p = *s++;
                    *d++ = (ubyte)((p >> 8) & 0xf8);
                    *d++ = (ubyte)((p >> 3) & 0xfc);
                    *d++ = (ubyte)(p << 3);
                }
                row += pitch / 2;
            }
            jpeg_write_file(filename, rgb, w, h);
            delete[] rgb;
            break;
        }
        case 3: {
            ubyte *p = buf;
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    ubyte t = p[0];
                    p[0] = p[2];
                    p[2] = t;
                    p += 3;
                }
            }
            jpeg_write_file(filename, buf, w, h);
            break;
        }
        case 4: {
            ubyte *s = buf;
            ubyte *d = buf;
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    s += 4;
                    d += 3;
                }
            }
            jpeg_write_file(filename, buf, w, h);
            break;
        }
        }
    }
    delete[] buf;
}

// ---- fades -------------------------------------------------------------------------------------------

// 0x5065e0  Confidence: exact (`ms <= 0`: test / jg, D6800). Black overlay 0 -> 1 over ms milliseconds;
// ms < 1: black at once.
void gr_fade_out(int ms)
{
    if (ms <= 0) {
        gr_fade_state = 0;
        gr_fade_level = 1.0f;
        return;
    }
    gr_fade_state = 2;
    gr_fade_time = 0.0f;
    gr_fade_duration = ms * 0.001f;
}

// 0x506630  Confidence: exact (`ms <= 0`, D6800). Overlay 1 -> 0, then off (-1); ms < 1: off at once.
void gr_fade_in(int ms)
{
    if (ms <= 0) {
        gr_fade_state = 0;
        gr_fade_level = -1.0f;
        return;
    }
    gr_fade_state = 1;
    gr_fade_time = 0.0f;
    gr_fade_duration = ms * 0.001f;
}

// 0x506680  Confidence: exact.
float gr_get_fade_level()
{
    if (gr_fade_level < 0.0f)
        return 0.0f;
    return gr_fade_level;
}

// 0x5066b0  Confidence: exact. From gr_flip: advances the fade by reported_frametime and draws the black
// rectangle over the whole screen (it sets the colour twice).
void gr_draw_fade()
{
    if (gr_fade_state) {
        gr_fade_time += reported_frametime;
        if (gr_fade_time <= gr_fade_duration) {
            gr_fade_level = gr_fade_time / gr_fade_duration;
            if (gr_fade_state == 1)
                gr_fade_level = 1.0f - gr_fade_level;
        } else {
            if (gr_fade_state == 1)
                gr_fade_level = -1.0f;
            else
                gr_fade_level = 1.0f;
            gr_fade_state = 0;
        }
    }
    if (gr_fade_level >= 0.0f) {
        gr_set_color(0, 0, 0, (int)(gr_fade_level * 255.0f));
        gr_set_color(0, 0, 0, (int)(gr_fade_level * 255.0f));
        gr::set_clip(0, 0, gr_screen_width(), gr_screen_height());
        gr_rect(0, 0, gr_screen_width(), gr_screen_height(), gr_rect_mode, 1.0f);
    }
}

// 0x5067c0  Confidence: exact.
void gr_close()
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_close_thunk();
        break;
    case GR_GLIDE:
        gr_glide_close();
        break;
    case GR_OPENGL:
        gr_opengl_close();
        break;
    }
    gr_close_fonts();
}

// ---- Color and gr::Mode members (out of line in this unit) -----------------------------------------

// 0x5067f0  Confidence: exact.
Color::Color(int r, int g, int b, int a)
{
    this->r = (ubyte)r;
    this->g = (ubyte)g;
    this->b = (ubyte)b;
    this->a = (ubyte)a;
}

// 0x506810  Confidence: exact. No caller (D2002).
gr::Mode::Mode(gr_mode v)
{
    value = v;
}

// 0x506820  Confidence: exact.
void Color::set(int r, int g, int b, int a)
{
    this->r = (ubyte)r;
    this->g = (ubyte)g;
    this->b = (ubyte)b;
    this->a = (ubyte)a;
}

// ---- clipping --------------------------------------------------------------------------------------------

// 0x506840  Confidence: exact. The clip rectangle is (x, y, w, h) on the screen; offset_x / offset_y
// are its origin and clip_left .. clip_bottom the part on the screen, relative to that origin. The right
// edge is tested with >= and the bottom edge with >, as the binary does.
void gr::set_clip(int x, int y, int w, int h)
{
    gr_screen.offset_x = x;
    gr_screen.offset_y = y;
    gr_screen.clip_left = 0;
    gr_screen.clip_right = w - 1;
    gr_screen.clip_top = 0;
    gr_screen.clip_bottom = h - 1;

    if (gr_screen.offset_x < 0)
        gr_screen.clip_left = -gr_screen.offset_x;
    else if (gr_screen.offset_x > gr_screen.max_w - 1)
        gr_screen.clip_left = gr_screen.max_w - gr_screen.offset_x - 1;
    if (gr_screen.offset_x + gr_screen.clip_right < 0)
        gr_screen.clip_right = -gr_screen.offset_x;
    else if (gr_screen.offset_x + gr_screen.clip_right >= gr_screen.max_w - 1)
        gr_screen.clip_right = gr_screen.max_w - gr_screen.offset_x - 1;

    if (gr_screen.offset_y < 0)
        gr_screen.clip_top = -gr_screen.offset_y;
    else if (gr_screen.offset_y > gr_screen.max_h - 1)
        gr_screen.clip_top = gr_screen.max_h - gr_screen.offset_y - 1;
    if (gr_screen.offset_y + gr_screen.clip_bottom < 0)
        gr_screen.clip_bottom = -gr_screen.offset_y;
    else if (gr_screen.offset_y + gr_screen.clip_bottom > gr_screen.max_h - 1)
        gr_screen.clip_bottom = gr_screen.max_h - gr_screen.offset_y - 1;

    gr_screen.clip_width = gr_screen.clip_right - gr_screen.clip_left + 1;
    gr_screen.clip_height = gr_screen.clip_bottom - gr_screen.clip_top + 1;

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_viewport();
        break;
    case GR_GLIDE:
        gr_glide_set_viewport();
        break;
    case GR_OPENGL:
        gr_opengl_set_viewport();
        break;
    }
}

// 0x506950  Confidence: exact.
float gr_get_aspect()
{
    return gr_screen.aspect;
}

// 0x506960  Confidence: exact. No caller.
void gr_set_aspect(float aspect)
{
    gr_screen.aspect = aspect;
}

// 0x506970  Confidence: exact.
void gr_get_clip(int *x, int *y, int *w, int *h)
{
    *x = gr_screen.offset_x;
    *y = gr_screen.offset_y;
    *w = gr_screen.clip_width;
    *h = gr_screen.clip_height;
}

// 0x5069a0  Confidence: exact.
void gr::reset_clip()
{
    gr::set_clip(0, 0, gr_screen.max_w, gr_screen.max_h);
}

// ---- frame -------------------------------------------------------------------------------------------

// 0x5069c0  Confidence: exact.
void gr_clear()
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_clear();
        break;
    case GR_GLIDE:
        gr_glide_clear();
        break;
    case GR_OPENGL:
        gr_opengl_clear();
        break;
    }
}

// 0x5069e0  Confidence: exact. Only Glide gets the second argument.
void gr_page_in(int bm, int flags)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_page_in(bm);
        break;
    case GR_GLIDE:
        gr_glide_page_in(bm, flags);
        break;
    case GR_OPENGL:
        gr_opengl_page_in(bm, 0);
        break;
    }
}

// 0x506a20  Confidence: exact.
void gr_flip()
{
    gr_poly_buffer_flush();
    gr::reset_clip();
    gr_draw_fade();
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_flip_thunk();
        break;
    case GR_GLIDE:
        gr_glide_flip();
        break;
    case GR_OPENGL:
        gr_opengl_flip();
        break;
    }
}

// 0x506a50  Confidence: exact. Clears the z buffer only when enable is set; the flag is not read.
void gr_zbuffer_clear(bool enable)
{
    if (enable) {
        gr_zbuffer_enabled = true;
        switch (gr_screen.mode) {
        case GR_DIRECT3D:
            gr_d3d_zbuffer_clear();
            break;
        case GR_GLIDE:
            gr_glide_zbuffer_clear();
            break;
        case GR_OPENGL:
            gr_opengl_zbuffer_clear();
            break;
        }
    } else {
        gr_zbuffer_enabled = false;
    }
}

// 0x506a90  Confidence: exact (the exponent overwrites gamma, the binary's argument slot; (float)pow, the inline
// powf written out because the unit is /Ob0 (D12103); for the
// float 255 multiplier, D6800). The reciprocal constants are written as multiplications (see NOTES).
// gr_gamma_ramp[i] = 255 * (i / 255) ^ (1 / gamma), clamped; gr_color_gamma_lut[i] = i, clamped to
// 0..255 (an identity table: no gamma is applied to it). Only Direct3D and Glide upload a ramp.
void gr_set_gamma(float gamma)
{
    int i;
    gr_gamma = gamma;
    gr_gamma_percent = (int)(gamma * 100.0f);
    gamma = 1.0f / gamma;
    for (i = 0; i < 256; i++) {
        int v = (int)((float)pow(i * (1.0f / 255.0f), gamma) * 255.0f);
        if (v > 255)
            v = 255;
        else if (v < 0)
            v = 0;
        gr_gamma_ramp[i] = v;
    }
    for (i = 0; i < 256; i++) {
        float f = (float)i;
        if (f > 255.0f)
            f = 255.0f;
        else if (f < 0.0f)
            f = 0.0f;
        gr_color_gamma_lut[i] = f;
    }
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_gamma();
        break;
    case GR_GLIDE:
        gr_glide_set_gamma();
        break;
    }
}

// ---- state -------------------------------------------------------------------------------------------

// 0x506b90  Confidence: exact. The back end hears of it only when the colour changes (and Glide never).
void gr_set_color(int r, int g, int b, int a)
{
    if (r == gr_screen.current_color.r && g == gr_screen.current_color.g && b == gr_screen.current_color.b &&
        a == gr_screen.current_color.a)
        return;
    gr_screen.current_color.r = (ubyte)r;
    gr_screen.current_color.g = (ubyte)g;
    gr_screen.current_color.b = (ubyte)b;
    gr_screen.current_color.a = (ubyte)a;
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_color();
        break;
    case GR_OPENGL:
        gr_opengl_set_color();
        break;
    }
}

// 0x506c10  Confidence: exact.
void gr_set_color_ptr(Color *c)
{
    gr_set_color(c->r, c->g, c->b, c->a);
}

// 0x506c40  Confidence: exact. Changes the alpha without telling the back end.
void gr_set_alpha(int a)
{
    gr_screen.current_color.a = (ubyte)a;
}

// 0x506c50  Confidence: exact.
void gr_set_texture(int bm, int bm2)
{
    gr_screen.current_bitmap = bm;
    gr_screen.current_bitmap_2 = bm2;
}

// 0x506c70  Confidence: exact.
void gr_mark_texture_dirty(int bm)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_mark_texture_dirty(bm);
        break;
    case GR_GLIDE:
        gr_glide_mark_texture_dirty(bm);
        break;
    case GR_OPENGL:
        gr_opengl_mark_texture_dirty(bm);
        break;
    }
}

// ---- 2D drawing ------------------------------------------------------------------------------------------

// 0x506cb0  Confidence: exact (variable homes: bw lives in h's argument slot, dy1 in w's).
// (bm, x, y, w, h, sx, sy, mode): the w x h part of the bitmap at (sx, sy) drawn at (x, y), clipped;
// a negative sx / sy moves the destination and clips again.
void gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr::Mode mode)
{
    int reclip;
    int dx1 = x, dx2 = x + w - 1;
    int dy1 = y, dy2 = y + h - 1;
    int bw, bh;
    bm_get_dimensions(bm, &bw, &bh);

    do {
        reclip = 0;
        if (dx1 > gr_screen.clip_right || dx2 < gr_screen.clip_left)
            return;
        if (dy1 > gr_screen.clip_bottom || dy2 < gr_screen.clip_top)
            return;
        if (dx1 < gr_screen.clip_left) {
            sx += gr_screen.clip_left - dx1;
            dx1 = gr_screen.clip_left;
        }
        if (dy1 < gr_screen.clip_top) {
            sy += gr_screen.clip_top - dy1;
            dy1 = gr_screen.clip_top;
        }
        if (dx2 > gr_screen.clip_right)
            dx2 = gr_screen.clip_right;
        if (dy2 > gr_screen.clip_bottom)
            dy2 = gr_screen.clip_bottom;
        if (sx < 0) {
            dx1 -= sx;
            sx = 0;
            reclip = 1;
        }
        if (sy < 0) {
            dy1 -= sy;
            sy = 0;
            reclip = 1;
        }
        w = dx2 - dx1 + 1;
        h = dy2 - dy1 + 1;
        if (sx + w > bw) {
            w = bw - sx;
            dx2 = dx1 + w - 1;
        }
        if (sy + h > bh) {
            h = bh - sy;
            dy2 = dy1 + h - 1;
        }
        if (w < 1)
            return;
        if (h < 1)
            return;
    } while (reclip);

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    case GR_GLIDE:
        gr_glide_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    case GR_OPENGL:
        gr_opengl_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    }
}

// 0x506ec0  Confidence: exact. Not clipped.
void gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_bitmap(bm, x, y, w, h, sx, sy, sw, sh, mode);
        break;
    case GR_GLIDE:
        gr_glide_bitmap(bm, x, y, w, h, sx, sy, sw, sh, mode);
        break;
    case GR_OPENGL:
        gr_opengl_bitmap(bm, x, y, w, h, sx, sy, sw, sh, mode);
        break;
    }
}

// 0x506f90  Confidence: exact (the w == 0 test's spelling). The whole bitmap at (x, y), clipped.
void gr_bitmap(int bm, int x, int y, gr::Mode mode)
{
    int w, h;
    bm_get_dimensions(bm, &w, &h);
    if (w == 0)
        return;

    int dx1 = x, dx2 = x + w - 1;
    int dy1 = y, dy2 = y + h - 1;
    int sx = 0, sy = 0;
    if (dx1 > gr_screen.clip_right || dx2 < gr_screen.clip_left)
        return;
    if (dy1 > gr_screen.clip_bottom || dy2 < gr_screen.clip_top)
        return;
    if (dx1 < gr_screen.clip_left) {
        sx = gr_screen.clip_left - dx1;
        dx1 = gr_screen.clip_left;
    }
    if (dy1 < gr_screen.clip_top) {
        sy = gr_screen.clip_top - dy1;
        dy1 = gr_screen.clip_top;
    }
    if (dx2 > gr_screen.clip_right)
        dx2 = gr_screen.clip_right;
    if (dy2 > gr_screen.clip_bottom)
        dy2 = gr_screen.clip_bottom;
    if (sx < 0)
        return;
    if (sy < 0)
        return;
    if (sx >= w)
        return;
    if (sy >= h)
        return;

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    case GR_GLIDE:
        gr_glide_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    case GR_OPENGL:
        gr_opengl_bitmap(bm, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1, sx, sy, dx2 - dx1 + 1, dy2 - dy1 + 1, mode);
        break;
    }
}

// 0x507100  Confidence: likely (D16003: the corner sums grouped `(a - b) + x`; the vertex array is built by the vector constructor iterator after
// gr_set_texture, D12103; only hs / wc are locals, the w * sin / h * cos products written in the sums, which
// gives the binary's order hs, wc, corner 0's x, then the two products kept on the FPU; corner 2's constants
// after its sx / sy (D18705); the gr_tmapper argument pushes are still scheduled differently).
// The bitmap as a quad of its size times scale, centred on (x, y) and rotated by angle, through
// gr_tmapper (no clipping here). Only sx, sy, sw = 1, uv and the "uv given" flag are set. The polygon
// is passed in the order 3, 2, 1, 0.
void gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr::Mode mode)
{
    gr_vertex *vlist[4];
    int w, h;

    gr_set_texture(bm, -1);
    gr_vertex verts[4];
    vlist[0] = &verts[3];
    vlist[1] = &verts[2];
    vlist[2] = &verts[1];
    vlist[3] = &verts[0];
    float sa = (float)sin(angle);
    float ca = (float)cos(angle);
    bm_get_dimensions(bm, &w, &h);

    float hw = w * scale * 0.5f;
    float hh = h * scale * 0.5f;
    float hs = hh * sa;
    float wc = hw * ca;

    verts[0].sw = 1.0f;
    verts[0].u = 0.0f;
    verts[0].v = 0.0f;
    verts[0]._unk1a = 1;
    verts[0].sx = (hs - wc) + x;
    verts[0].sy = (-(hw * sa) - hh * ca) + y;

    verts[1].sw = 1.0f;
    verts[1].u = 1.0f;
    verts[1].v = 0.0f;
    verts[1]._unk1a = 1;
    verts[1].sx = (wc + hs) + x;
    verts[1].sy = (hw * sa - hh * ca) + y;

    verts[2].sx = (wc - hs) + x;
    verts[2].sy = (hh * ca + hw * sa) + y;
    verts[2].sw = 1.0f;
    verts[2].u = 1.0f;
    verts[2].v = 1.0f;
    verts[2]._unk1a = 1;

    verts[3].sw = 1.0f;
    verts[3].u = 0.0f;
    verts[3].v = 1.0f;
    verts[3]._unk1a = 1;
    verts[3].sx = (-wc - hs) + x;
    verts[3].sy = (hh * ca - hw * sa) + y;

    gr_tmapper(4, vlist, mode);
}

// 0x507310  Confidence: likely (the swap's spelling). (x1, y1, x2, y2, mode): the line clipped to the
// clip rectangle in floats, sorted top to bottom first; then the back end.
void gr_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    float clip_left = (float)gr_screen.clip_left;
    float clip_top = (float)gr_screen.clip_top;
    float clip_right = (float)gr_screen.clip_right;
    float clip_bottom = (float)gr_screen.clip_bottom;

    if (y1 > y2) {
        float tmp;
        tmp = y1;
        y1 = y2;
        y2 = tmp;
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y2 < clip_top)
        return;
    if (y1 > clip_bottom)
        return;
    if (x1 < x2) {
        if (x2 < clip_left)
            return;
        if (x1 > clip_right)
            return;
        if (x1 < clip_left) {
            y1 = (clip_left - x1) * (y2 - y1) / (x2 - x1) + y1;
            if (y1 > clip_bottom)
                return;
            x1 = clip_left;
        }
        if (x2 > clip_right) {
            y2 = y2 - (x2 - clip_right) * (y2 - y1) / (x2 - x1);
            if (y2 < clip_top)
                return;
            x2 = clip_right;
        }
        if (y1 < clip_top) {
            x1 = (clip_top - y1) * (x2 - x1) / (y2 - y1) + x1;
            y1 = clip_top;
        }
        if (y2 > clip_bottom) {
            x2 = x2 - (y2 - clip_bottom) * (x2 - x1) / (y2 - y1);
            y2 = clip_bottom;
        }
    } else {
        if (x1 < clip_left)
            return;
        if (x2 > clip_right)
            return;
        if (x1 > clip_right) {
            y1 = (x1 - clip_right) * (y2 - y1) / (x1 - x2) + y1;
            if (y1 > clip_bottom)
                return;
            x1 = clip_right;
        }
        if (x2 < clip_left) {
            y2 = y2 - (clip_left - x2) * (y2 - y1) / (x1 - x2);
            if (y2 < clip_top)
                return;
            x2 = clip_left;
        }
        if (y1 < clip_top) {
            x1 = x1 - (clip_top - y1) * (x1 - x2) / (y2 - y1);
            y1 = clip_top;
        }
        if (y2 > clip_bottom) {
            x2 = (y2 - clip_bottom) * (x1 - x2) / (y2 - y1) + x2;
            y2 = clip_bottom;
        }
    }

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_line(x1, y1, x2, y2, mode);
        break;
    case GR_GLIDE:
        gr_glide_line(x1, y1, x2, y2, mode);
        break;
    case GR_OPENGL:
        gr_opengl_line(x1, y1, x2, y2, mode);
        break;
    }
}

// 0x507690  Confidence: exact (float conversions inline, as the loop hoists them and the last segment
// converts again; angle declared first, D6800). No caller. A circle of radius * 3 segments (5..128)
// drawn with gr_line; x uses the sine and y the cosine, and the closing segment ends at sin(0.0) /
// cos(0.0) (the start).
void gr_circle(int x, int y, int radius, gr::Mode mode)
{
    float angle = 0.0f;
    int segments = radius * 3;
    clamp_int(&segments, 5, 128);
    float step = 6.2831855f / segments;
    for (int i = 0; i < segments - 1; i++) {
        float next = angle + step;
        gr_line((float)x + (float)sin(angle) * (float)radius, (float)y + (float)cos(angle) * (float)radius,
                (float)x + (float)sin(next) * (float)radius, (float)y + (float)cos(next) * (float)radius, mode);
        angle = next;
    }
    gr_line((float)x + (float)sin(angle) * (float)radius, (float)y + (float)cos(angle) * (float)radius,
            (float)x + (float)sin(0.0) * (float)radius, (float)y + (float)cos(0.0) * (float)radius, mode);
}

// 0x5077d0  Confidence: exact (D12103: the unit is /Ob0, so the vertex array is built by the vector
// constructor iterator; the array is declared after the clipping; each corner is vlist, sx, sy, sw, flag). (x, y, w,
// h, mode, z): a negative w or h extends left / up; clipped to whole pixels, then one quad through
// gr_tmapper with sw = z on every corner and no uv / colour given (the back end uses gr_color).
void gr_rect(int x, int y, int w, int h, gr::Mode mode, float z)
{
    gr_vertex *vlist[4];
    int x1 = x, y1 = y;
    int x2, y2;
    if (w > 0)
        x2 = x + w - 1;
    else
        x2 = x + w + 1;
    if (h > 0)
        y2 = y + h - 1;
    else
        y2 = y + h + 1;
    if (x2 < x1) {
        x1 = x2;
        x2 = x;
    }
    if (y2 < y1) {
        y1 = y2;
        y2 = y;
    }
    if (x1 > gr_screen.clip_right || x2 < gr_screen.clip_left)
        return;
    if (y1 > gr_screen.clip_bottom || y2 < gr_screen.clip_top)
        return;
    if (x1 < gr_screen.clip_left)
        x1 = gr_screen.clip_left;
    if (y1 < gr_screen.clip_top)
        y1 = gr_screen.clip_top;
    if (x2 > gr_screen.clip_right)
        x2 = gr_screen.clip_right;
    if (y2 > gr_screen.clip_bottom)
        y2 = gr_screen.clip_bottom;

    gr_vertex verts[4];
    memset(verts, 0, sizeof(verts));
    vlist[0] = &verts[0];
    verts[0].sx = (float)(gr_screen.offset_x + x1);
    verts[0].sy = (float)(gr_screen.offset_y + y1);
    verts[0].sw = z;
    verts[0]._unk1a = 0;

    vlist[1] = &verts[1];
    verts[1].sx = (float)(gr_screen.offset_x + x2 + 1);
    verts[1].sy = (float)(gr_screen.offset_y + y1);
    verts[1].sw = z;
    verts[1]._unk1a = 0;

    vlist[2] = &verts[2];
    verts[2].sx = (float)(gr_screen.offset_x + x2 + 1);
    verts[2].sy = (float)(gr_screen.offset_y + y2 + 1);
    verts[2].sw = z;
    verts[2]._unk1a = 0;

    vlist[3] = &verts[3];
    verts[3].sx = (float)(gr_screen.offset_x + x1);
    verts[3].sy = (float)(gr_screen.offset_y + y2 + 1);
    verts[3].sw = z;
    verts[3]._unk1a = 0;

    gr_tmapper(4, vlist, mode);
}

// ---- capabilities ----------------------------------------------------------------------------------------

// 0x5079b0  Confidence: exact. OpenGL claims every mode; an unknown renderer none.
bool gr_is_mode_supported(gr::Mode *mode)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        return gr_d3d_is_mode_supported(mode);
    case GR_GLIDE:
        return gr_glide_is_mode_supported(mode);
    case GR_OPENGL:
        return true;
    }
    return false;
}

// 0x5079f0  Confidence: exact. OpenGL reports a constant 8 (GameInit picks the reduced textures at
// <= 8; the SummonerFix bug fix patches this constant to 64).
int gr_get_video_memory_mb()
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        return gr_d3d_get_video_memory_mb();
    case GR_GLIDE:
        return gr_glide_get_video_memory_mb();
    case GR_OPENGL:
#ifdef FIXED_BUILD
        // FIXED_BUILD: K64 OpenGLTextures. Report 64 MB, so a start without summoner.cfg (a fresh install) keeps
        // the full-detail textures instead of the reduced ones that summoner_cfg_load picks at <= 8.
        return 64;
#else
        return 8;
#endif
    }
    return 0;
}

// ---- polygons --------------------------------------------------------------------------------------------

// 0x507a20  Confidence: exact.
void gr_tmapper(int nv, gr_vertex **verts, gr::Mode mode)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_tmapper(nv, verts, mode);
        break;
    case GR_GLIDE:
        gr_glide_tmapper(nv, verts, mode);
        break;
    case GR_OPENGL:
        gr_opengl_tmapper(nv, verts, mode);
        break;
    }
}

// 0x507a80  Confidence: likely (loop shapes). Direct3D and Glide draw all passes themselves; otherwise
// (OpenGL) each pass is one gr_tmapper call: the pass's uv / colour / alpha are copied into the vertices
// (and flagged in +0x1a) when the pass takes them per vertex, else the pass colour is set. The inner
// dispatch repeats all three back ends.
void gr_tmapper_multipass(int nv, gr_multipass_vertex **verts, int npasses, gr_tmapper_pass *passes)
{
    gr_vertex *vlist[32];
    int i, p;

    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_tmapper_multipass(nv, verts, npasses, passes);
        break;
    case GR_GLIDE:
        gr_glide_tmapper_multitexture(nv, verts, npasses, passes);
        break;
    default:
        for (i = 0; i < nv; i++)
            vlist[i] = verts[i]->v;
        for (p = 0; p < npasses; p++) {
            gr_tmapper_pass *pass = &passes[p];
            ubyte alpha;
            for (i = 0; i < nv; i++)
                vlist[i]->_unk1a = 0;
            if (pass->enabled) {
                for (i = 0; i < nv; i++) {
                    vlist[i]->u = verts[i]->pass[p].u;
                    vlist[i]->v = verts[i]->pass[p].v;
                    vlist[i]->_unk1a |= 1;
                }
            }
            if (pass->vertex_alpha) {
                for (i = 0; i < nv; i++) {
                    vlist[i]->a = verts[i]->pass[p].a;
                    vlist[i]->_unk1a |= 8;
                }
                alpha = 0xff;
            } else {
                alpha = pass->a;
            }
            if (pass->vertex_color) {
                for (i = 0; i < nv; i++) {
                    vlist[i]->r = verts[i]->pass[p].r;
                    vlist[i]->g = verts[i]->pass[p].g;
                    vlist[i]->b = verts[i]->pass[p].b;
                    vlist[i]->_unk1a |= 4;
                }
                gr_set_color(0xff, 0xff, 0xff, alpha);
            } else {
                gr_set_color(pass->r, pass->g, pass->b, alpha);
            }
            gr_set_texture(pass->bitmap, -1);
            switch (gr_screen.mode) {
            case GR_DIRECT3D:
                gr_d3d_tmapper(nv, vlist, pass->mode);
                break;
            case GR_GLIDE:
                gr_glide_tmapper(nv, vlist, pass->mode);
                break;
            case GR_OPENGL:
                gr_opengl_tmapper(nv, vlist, pass->mode);
                break;
            }
        }
        break;
    }
}

// 0x507d20  Confidence: exact. Empty; gr_bitmap_3d_projected calls it with 0x10.
void gr_noop_507d20(int flags)
{
}

// 0x507d30  Confidence: exact. Returns the BM_FORMAT_* of the pixels (0 = renderer unknown).
int gr_read_backbuffer(int x, int y, int w, int h, void *buf)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        return gr_d3d_read_backbuffer(x, y, w, h, buf);
    case GR_GLIDE:
        return gr_glide_read_backbuffer(x, y, w, h, buf);
    case GR_OPENGL:
        return gr_opengl_read_backbuffer(x, y, w, h, (ubyte *)buf);
    }
    return 0;
}

// ---- fog and tint ------------------------------------------------------------------------------------

// 0x507db0  Confidence: exact. (enable, r, g, b, near, far): the near distance is limited to half the
// far one (a quarter on Glide); with a negative near or far < near only the colour and the flag change.
// The far distance is scaled by Gr_level_fog_multiplier; the alpha (vertex) fog starts at
// Gr_alpha_fog_ratio of the way from near to far. Then the back end, which reads gr_screen.
void gr_set_fog(bool enable, int r, int g, int b, float near_dist, float far_dist)
{
    gr_screen.fog_color.set(r, g, b, 0xff);
    gr_screen.fog_enabled = enable;
    if (enable) {
        float lim = (gr_screen.mode == GR_GLIDE ? 0.25f : 0.5f) * far_dist;
        if (lim < near_dist)
            near_dist = lim;
        if (near_dist >= 0.0f && far_dist >= near_dist) {
            gr_screen.fog_near = near_dist;
            gr_screen.fog_far = Gr_level_fog_multiplier * far_dist;
            float alpha_near = (1.0f - Gr_alpha_fog_ratio) * near_dist + Gr_alpha_fog_ratio * far_dist;
            gr_screen.fog_alpha_near = gr_scale_by_far_clip(alpha_near);
            gr_screen.fog_alpha_far = gr_scale_by_far_clip(far_dist);
            float range = gr_screen.fog_alpha_far - gr_screen.fog_alpha_near;
            gr_screen.fog_alpha_inv_range = 1.0f / range;
            if (d3d_fog_mode == 3) {
                gr_screen.fog_z_near = gr_scale_by_far_clip(near_dist);
                gr_screen.fog_z_far = gr_scale_by_far_clip(gr_screen.fog_far);
                gr_screen.fog_z_inv_range = 1.0f / (gr_screen.fog_z_far - gr_screen.fog_z_near);
            } else {
                float z = gr_scale_by_far_clip(gr_screen.fog_far) * 30.0f;
                gr_screen.fog_z_inv_range = 1.0f;
                gr_screen.fog_z_near = z;
                gr_screen.fog_z_far = z;
            }
            gr_screen.fog_vertex_far = gr_scale_by_far_clip(Gr_level_fog_multiplier * far_dist);
            gr_screen.fog_vertex_near = gr_screen.fog_vertex_far - range;
        }
    }
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_fog(1);
        break;
    case GR_GLIDE:
        gr_glide_set_fog(1);
        break;
    case GR_OPENGL:
        gr_opengl_set_fog(1);
        break;
    }
}

// 0x507f70  Confidence: exact.
void gr_set_tint(bool enable, float r, float g, float b)
{
    gr_screen.tint_enabled = enable;
    if (enable) {
        gr_screen.tint_r = r;
        gr_screen.tint_g = g;
        gr_screen.tint_b = b;
    }
}

// 0x507fa0  Confidence: exact. Empty, no reference.
void gr_stub_ret_507fa0()
{
}

// 0x507fb0  Confidence: exact. Direct3D has nothing to wait for.
void gr_finish()
{
    switch (gr_screen.mode) {
    case GR_GLIDE:
        gr_glide_finish();
        break;
    case GR_OPENGL:
        glFinish();
        break;
    }
}

// ---- texture cache -----------------------------------------------------------------------------------

// 0x507fd0  Confidence: exact.
void gr_tcache_flush()
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_tcache_flush();
        break;
    case GR_GLIDE:
        gr_glide_tcache_flush();
        break;
    case GR_OPENGL:
        gr_opengl_tcache_flush();
        break;
    }
    gr_screen.current_bitmap = -1;
    gr_screen.current_bitmap_2 = -1;
}

// 0x508020  Confidence: exact.
void gr_tcache_free(int bm)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_tcache_free(bm);
        break;
    case GR_GLIDE:
        gr_glide_tcache_free(bm);
        break;
    case GR_OPENGL:
        gr_opengl_tcache_free(bm);
        break;
    }
}

// 0x508060  Confidence: exact. No reference.
bool gr_stub_ret_false_508060()
{
    return false;
}

// 0x508070  Confidence: exact. No reference.
int gr_stub_ret0_508070()
{
    return 0;
}

// 0x508080  Confidence: exact. Empty, no reference.
void gr_stub_ret_508080()
{
}

// 0x508090  Confidence: exact (the reciprocal constants). Only caller combat_spawn_blood.
float rgb_to_lightened_grey(float r, float g, float b)
{
    float avg = (r + g + b) * (1.0f / 3.0f);
    return (avg + (255.0f - avg) * 0.6f) * (1.0f / 255.0f);
}

// 0x5080c0  Confidence: exact. Does nothing unless the card can (gr_antialias_enabled, set by the
// Direct3D back end); the Glide and OpenGL versions are empty. All three take the bool (the OpenGL call
// pushes the dword unmasked; gr_opengl.h said int, D6801).
void gr_set_antialias(bool on)
{
    if (gr_antialias_enabled) {
        switch (gr_screen.mode) {
        case GR_DIRECT3D:
            gr_d3d_set_antialias(on);
            break;
        case GR_GLIDE:
            gr_glide_set_antialias(on);
            break;
        case GR_OPENGL:
            gr_opengl_set_antialias(on);
            break;
        }
    }
}

// 0x508110  Confidence: exact.
void gr_set_alpha_test(bool enable)
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_set_alpha_test(enable);
        break;
    case GR_GLIDE:
        gr_glide_set_alpha_test(enable);
        break;
    }
}

// 0x508140  Confidence: exact.
void gr_flush_pipeline()
{
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_flush_pipeline();
        break;
    case GR_GLIDE:
        gr_glide_flush_pipeline();
        break;
    }
}

// ---- out-of-line inlines at the end of the object (COMDATs, compiler-emitted) ---------------------
// 0x508160 gr__Mode__Mode: gr::Mode(6 fields), the header inline (gr.h), called by the 30 initialisers.
// Confidence: exact.
// 0x5081a0 gr__Screen__Screen: the implicit constructor of gr__Screen (Color() on +0x54 and +0x61, which
// the linker folded into vector3() 0x505530), called by init_gr_screen. Confidence: exact.
// 0x5081c0 gr__Vertex__Vertex (vsdk\gr in sum.symbols): gr_vertex's implicit constructor (vector3() on
// +0); not produced by this source while gr_vertex has no vector3 member (D1605). Not counted.
