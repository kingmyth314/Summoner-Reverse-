// Widescreen interface (docs/experimental-build.md), SummonerFix's widescreen.c in the source. Not part of Sum.exe:
// compiled only for link-build --config experimental.
//
// With a game width above 1024 (SummonerFix's [Display] GameWidth, through the resolution table, CustomResolution)
// the 3D world uses the full width, but the HUD and the in-game screens are a 1024-wide layout drawn at the left.
// During gameplay (game state 4) that layout is moved to the centre by (width - 1024) / 2 pixels:
//  - The interface's source files include experimental/ui_shift.h last. It points their calls to the 2D primitives
//    (gr_bitmap, gr_bitmap_ex, gr_bitmap_stretched, gr_bitmap_float, gr_string, gr_rect, gr_line), the clip
//    (gr::set_clip, gr_get_clip), gr_screen_width and mouse_get_pos at the ui_* wrappers below, which add the shift
//    to x. (SummonerFix picks the same calls by their address range: the game UI 0x401000-0x430000, the vsdk ui units
//    and the conversation widgets 0x4d6600-0x4d7ab0, which are these files.)
//  - A clip set by interface code gets the shift instead, and draws inside it are not shifted again
//    (exp_ws_clip_shifted; gr::set_clip clears it for every other caller, gr.cpp).
//  - Interface code sees a 1024-wide screen and a clip origin without the shift, so its layout stays in 1024 space.
//  - The mouse: GameTick copies the cursor into game_mouse_x / y, which interface code reads; it gets x - shift
//    (exp_ws_mouse_tick). The world's reads (camera edge turn, clicks into the world, the script editor's ray) use
//    the raw position, exp_mouse_raw_x / y.
//  - interface_set_view_clip, which sizes the 3D view, keeps the real width (exp_ws_view_width).
// By interface screen during gameplay:
//  - Full-screen panels (inventory, character, journal, options, save / load, ...): models are drawn in screen
//    coordinates by the 3D renderer, which call shifting cannot reach, so SummonerFix's display layer shifts the
//    whole picture, as for the menus (display.c, WidescreenFullPanel). It knows to through the "CenterHud" key in
//    Sum-exp.fixes (the marker below); no call shifting then.
//  - Side panels (map, spells, skills, item trade) stay flush left; their draw and frame run unshifted with the raw
//    cursor (exp_ws_side_panel_begin / end), and the 3D view keeps its full width behind them
//    ([Display] SidePanelsOverWorld, default 1).
//  - Everything else (the HUD alone, pickup, play menu, conversation, assess): call shifting.
// [Display] CenterHud (default 1) switches it off, as in SummonerFix.
// FIXED_BUILD: Widescreen CenterHud
//
// A game height above 768 (2026-11-10, [Display] InterfaceScale, docs/future/interface-scale.md): the interface is
// still the 1024x768 layout, so the same machinery places it in y as well. During gameplay the call-shifted screens
// and the side panels move down by (height - 768): the HUD bar sits at the bottom of the screen and the 3D view above
// it keeps the full height (exp_ws_view_height); the side panels keep x unshifted and take only the y shift. Full
// panels and the menus are centred as a whole picture by the display layer (modern/display.cpp, SummonerFix's
// display.c), and interface code then sees a 768-high screen (ui_gr_screen_height), except interface_init_global's
// field-of-view measurement, which needs the real shape (exp_ws_real_screen_height). The cursor's y gets the same
// treatment as x (exp_ws_mouse_tick, ui_mouse_get_pos); world code reads exp_mouse_raw_y.
// FIXED_BUILD: Widescreen InterfaceScale
#include <string.h>
#include "experimental/experimental.h"

#ifdef FIXED_BUILD

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/mouse.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"

#define WS_LAYOUT_WIDTH 1024
#define WS_LAYOUT_HEIGHT 768

namespace gr {
void ui_set_clip(int x, int y, int w, int h);   // as in ui_shift.h, which this file does not include (its macros)
}

int exp_ws_clip_shifted;              // the current clip was set by interface code and includes the shift
int exp_mouse_raw_x, exp_mouse_raw_y; // the cursor in game pixels, unshifted (read by world code)
static int ws_shift = -1;             // x; -1 = not known yet
static int ws_shift_y;                // y: height - 768 above 768 (the layout at the bottom), else 0
static int ws_center_menus;           // [Display] CenterMenus: the display layer centres menus and full panels
static int ws_no_shift;               // inside a side panel's draw or frame (no x shift)
static int ws_clip_dx, ws_clip_dy;    // the shift the current interface clip got (with exp_ws_clip_shifted)
static int ws_panels_over_world = -1;
static float ws_wide_aspect;

static int ws_get_shift()
{
    if (ws_shift < 0) {
        int w = gr_screen_width();   // the real size: this file does not include ui_shift.h
        int h = gr_screen_height();
        if (w <= 0 || h <= 0)
            return 0;                // before gr_init
        bool on = exp_ini_int("Display", "CenterHud", 1) != 0;
        ws_shift = (on && w > WS_LAYOUT_WIDTH) ? (w - WS_LAYOUT_WIDTH) / 2 : 0;
        ws_shift_y = (on && h > WS_LAYOUT_HEIGHT) ? h - WS_LAYOUT_HEIGHT : 0;
        ws_center_menus = exp_ini_int("Display", "CenterMenus", 1);
        ws_panels_over_world = exp_ini_int("Display", "SidePanelsOverWorld", 1);
        exp_log("widescreen: %dx%d, interface shift %d, %d", w, h, ws_shift, ws_shift_y);
    }
    return ws_shift;
}

// Any placement at all (a game wider than 1024 or taller than 768, with CenterHud).
static bool ws_any() { return ws_get_shift() > 0 || ws_shift_y > 0; }

static bool ws_in_game() { return gameseq_get_state() == 4; }
static bool ws_call_shift_screen(int s) { return s == 0 || s == 4 || s == 9 || s == 13 || s == 20; }
static bool ws_side_panel(int s) { return s == 3 || s == 5 || s == 6 || s == 19; }

static bool ws_full_panel()
{
    if (!ws_any() || !ws_in_game())
        return false;
    int s = interface_cur_screen;
    return !ws_call_shift_screen(s) && !ws_side_panel(s);
}

// The host's display map (modern/display.cpp) centres these panels as SummonerFix's GetMap does.
bool exp_ws_full_panel() { return ws_any() && ws_full_panel(); }

// Call shifting in play: the HUD-type screens (x and y) and the side panels (y only).
static bool ws_placed() { return ws_any() && ws_in_game() && !ws_full_panel(); }
static int ws_dx() { return (ws_placed() && !ws_no_shift) ? ws_shift : 0; }
static int ws_dy() { return ws_placed() ? ws_shift_y : 0; }
static bool ws_active() { return ws_placed() && !ws_no_shift && ws_shift > 0; }   // x placement (as before)
static int ws_draw_dx() { return exp_ws_clip_shifted ? 0 : ws_dx(); }
static int ws_draw_dy() { return exp_ws_clip_shifted ? 0 : ws_dy(); }

// ---- the wrappers (ui_shift.h) ----
void ui_gr_bitmap(int bm, int x, int y, gr::Mode mode)
{
    gr_bitmap(bm, x + ws_draw_dx(), y + ws_draw_dy(), mode);
}

void ui_gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr::Mode mode)
{
    gr_bitmap_ex(bm, x + ws_draw_dx(), y + ws_draw_dy(), w, h, sx, sy, mode);
}

void ui_gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    gr_bitmap_stretched(bm, x + ws_draw_dx(), y + ws_draw_dy(), w, h, sx, sy, sw, sh, mode);
}

void ui_gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr::Mode mode)
{
    gr_bitmap_float(bm, x + (float)ws_draw_dx(), y + (float)ws_draw_dy(), scale, angle, mode);
}

void ui_gr_string(int x, int y, const char *s, int font, gr::Mode mode)
{
    gr_string(x != GR_CENTER_X ? x + ws_draw_dx() : x, y + ws_draw_dy(), s, font, mode);
}

void ui_gr_rect(int x, int y, int w, int h, gr::Mode mode, float z)
{
    gr_rect(x + ws_draw_dx(), y + ws_draw_dy(), w, h, mode, z);
}

void ui_gr_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    float dx = (float)ws_draw_dx(), dy = (float)ws_draw_dy();
    gr_line(x1 + dx, y1 + dy, x2 + dx, y2 + dy, mode);
}

void gr::ui_set_clip(int x, int y, int w, int h)
{
    int dx = ws_dx(), dy = ws_dy();
    gr::set_clip(x + dx, y + dy, w, h);   // clears exp_ws_clip_shifted
    exp_ws_clip_shifted = dx != 0 || dy != 0;
    ws_clip_dx = dx;
    ws_clip_dy = dy;
}

void ui_gr_get_clip(int *x, int *y, int *w, int *h)
{
    gr_get_clip(x, y, w, h);
    if (exp_ws_clip_shifted && ws_placed()) {
        if (x)
            *x -= ws_clip_dx;
        if (y)
            *y -= ws_clip_dy;
    }
}

int ui_gr_screen_width()
{
    return ws_active() ? WS_LAYOUT_WIDTH : gr_screen_width();
}

// 768 wherever the layout is placed in a taller screen: in play (call shifting), and where the display layer centres
// the whole picture (menus outside play, full panels in play).
int ui_gr_screen_height()
{
    int h = gr_screen_height();
    if (h <= WS_LAYOUT_HEIGHT)
        return h;
    ws_get_shift();
    if (ws_shift_y > 0 && ws_placed())
        return WS_LAYOUT_HEIGHT;
    if (ws_center_menus && (!ws_in_game() || ws_full_panel()))
        return WS_LAYOUT_HEIGHT;
    return h;
}

int exp_ws_real_screen_height()
{
    return gr_screen_height();
}

int ui_mouse_get_pos(int *x, int *y, int *z)
{
    int r = mouse_get_pos(x, y, z);
    if (x)
        *x -= ws_dx();
    if (y)
        *y -= ws_dy();
    return r;
}

// ---- the special cases ----
int exp_ws_mouse_tick(int *x, int *y, int *z)
{
    int r = mouse_get_pos(x, y, z);
    if (x)
        exp_mouse_raw_x = *x;
    if (y)
        exp_mouse_raw_y = *y;
    if (x)
        *x -= ws_dx();
    if (y)
        *y -= ws_dy();
    return r;
}

// Around interface_draw's / interface_frame's switch: for a side panel, no shifting, and for its frame the raw cursor
// in game_mouse_x. Returns what exp_ws_side_panel_end needs to undo (0 = nothing, 1 = shifting only, 2 = also x).
static int ws_saved_mouse_x;

int exp_ws_side_panel_begin(bool frame)
{
    if (!ws_side_panel(interface_cur_screen))
        return 0;
    ws_no_shift++;
    if (frame && ws_get_shift() > 0 && ws_in_game()) {
        ws_saved_mouse_x = game_mouse_x;
        game_mouse_x = exp_mouse_raw_x;
        return 2;
    }
    return 1;
}

void exp_ws_side_panel_end(int undo)
{
    if (undo == 0)
        return;
    ws_no_shift--;
    if (undo == 2)
        game_mouse_x = ws_saved_mouse_x;
}

int exp_ws_view_width()
{
    bool full = ws_full_panel();
    if (ws_any()) {
        // The projection scale is the view width over gr_screen.aspect; a 1024-wide view for a full panel uses the
        // 4:3 aspect (1.0), so the panel's models keep their size.
        if (!ws_wide_aspect && !full)
            ws_wide_aspect = gr_screen.aspect;
        if (ws_wide_aspect)
            gr_screen.aspect = full ? 1.0f : ws_wide_aspect;
    }
    return full ? WS_LAYOUT_WIDTH : gr_screen_width();
}

// interface_set_view_clip's gr_screen_height: the real height (the HUD bar sits at the bottom of a taller screen, so
// height - bar is the view above it), 768 while a full-screen panel is open (its picture is centred like a menu).
int exp_ws_view_height()
{
    return ws_full_panel() ? WS_LAYOUT_HEIGHT : gr_screen_height();
}

int exp_ws_panel_width(int width)
{
    return (ws_panels_over_world && ws_get_shift() > 0 && ws_in_game()) ? 0 : width;
}

#endif // FIXED_BUILD
