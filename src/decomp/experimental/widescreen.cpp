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
// EXPERIMENTAL_BUILD: Widescreen CenterHud
#include <string.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/os/mouse.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"

#define WS_LAYOUT_WIDTH 1024

namespace gr {
void ui_set_clip(int x, int y, int w, int h);   // as in ui_shift.h, which this file does not include (its macros)
}

int exp_ws_clip_shifted;              // the current clip was set by interface code and includes the shift
int exp_mouse_raw_x, exp_mouse_raw_y; // the cursor in game pixels, unshifted (read by world code)
static int ws_shift = -1;             // -1 = not known yet
static int ws_no_shift;               // inside a side panel's draw or frame
static int ws_panels_over_world = -1;
static float ws_wide_aspect;

static int ws_get_shift()
{
    if (ws_shift < 0) {
        int w = gr_screen_width();   // the real width: this file does not include ui_shift.h
        if (w <= 0)
            return 0;                // before gr_init
        ws_shift = (exp_ini_int("Display", "CenterHud", 1) && w > WS_LAYOUT_WIDTH) ? (w - WS_LAYOUT_WIDTH) / 2 : 0;
        ws_panels_over_world = exp_ini_int("Display", "SidePanelsOverWorld", 1);
        exp_log("widescreen: width %d, interface shift %d", w, ws_shift);
    }
    return ws_shift;
}

static bool ws_in_game() { return gameseq_get_state() == 4; }
static bool ws_call_shift_screen(int s) { return s == 0 || s == 4 || s == 9 || s == 13 || s == 20; }
static bool ws_side_panel(int s) { return s == 3 || s == 5 || s == 6 || s == 19; }

static bool ws_full_panel()
{
    if (ws_get_shift() <= 0 || !ws_in_game())
        return false;
    int s = interface_cur_screen;
    return !ws_call_shift_screen(s) && !ws_side_panel(s);
}

static bool ws_active() { return ws_get_shift() > 0 && !ws_no_shift && ws_in_game() && !ws_full_panel(); }
static bool ws_shift_draw() { return ws_active() && !exp_ws_clip_shifted; }

// ---- the wrappers (ui_shift.h) ----
void ui_gr_bitmap(int bm, int x, int y, gr::Mode mode)
{
    gr_bitmap(bm, ws_shift_draw() ? x + ws_shift : x, y, mode);
}

void ui_gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, gr::Mode mode)
{
    gr_bitmap_ex(bm, ws_shift_draw() ? x + ws_shift : x, y, w, h, sx, sy, mode);
}

void ui_gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    gr_bitmap_stretched(bm, ws_shift_draw() ? x + ws_shift : x, y, w, h, sx, sy, sw, sh, mode);
}

void ui_gr_bitmap_float(int bm, float x, float y, float scale, float angle, gr::Mode mode)
{
    gr_bitmap_float(bm, ws_shift_draw() ? x + (float)ws_shift : x, y, scale, angle, mode);
}

void ui_gr_string(int x, int y, const char *s, int font, gr::Mode mode)
{
    gr_string((ws_shift_draw() && x != GR_CENTER_X) ? x + ws_shift : x, y, s, font, mode);
}

void ui_gr_rect(int x, int y, int w, int h, gr::Mode mode, float z)
{
    gr_rect(ws_shift_draw() ? x + ws_shift : x, y, w, h, mode, z);
}

void ui_gr_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    if (ws_shift_draw()) {
        x1 += (float)ws_shift;
        x2 += (float)ws_shift;
    }
    gr_line(x1, y1, x2, y2, mode);
}

void gr::ui_set_clip(int x, int y, int w, int h)
{
    bool active = ws_active();
    gr::set_clip(active ? x + ws_shift : x, y, w, h);   // clears exp_ws_clip_shifted
    exp_ws_clip_shifted = active;
}

void ui_gr_get_clip(int *x, int *y, int *w, int *h)
{
    gr_get_clip(x, y, w, h);
    if (ws_active() && exp_ws_clip_shifted && x)
        *x -= ws_shift;
}

int ui_gr_screen_width()
{
    return ws_active() ? WS_LAYOUT_WIDTH : gr_screen_width();
}

int ui_mouse_get_pos(int *x, int *y, int *z)
{
    int r = mouse_get_pos(x, y, z);
    if (x && ws_active())
        *x -= ws_shift;
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
    if (x && ws_active())
        *x -= ws_shift;
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
    if (ws_get_shift() > 0) {
        // The projection scale is the view width over gr_screen.aspect; a 1024-wide view for a full panel uses the
        // 4:3 aspect (1.0), so the panel's models keep their size.
        if (!ws_wide_aspect && !full)
            ws_wide_aspect = gr_screen.aspect;
        if (ws_wide_aspect)
            gr_screen.aspect = full ? 1.0f : ws_wide_aspect;
    }
    return full ? WS_LAYOUT_WIDTH : gr_screen_width();
}

int exp_ws_panel_width(int width)
{
    return (ws_panels_over_world && ws_get_shift() > 0 && ws_in_game()) ? 0 : width;
}

#endif // EXPERIMENTAL_BUILD
