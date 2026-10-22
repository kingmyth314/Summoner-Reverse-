// [Experimental] display options (docs/experimental-build.md): a frame-rate cap and vertical sync for the OpenGL
// renderer. Not part of Sum.exe: compiled only for link-build --config experimental.
//
// FrameRateCap (frames per second, 0 = the game's own 120): the frame timer's floor min_reported_frametime
// (0x5a574c, vsdk/frametime.cpp) becomes 1 / cap. The fixed build's frame limiter (FIXED_BUILD G1 in FrameDone)
// waits each frame until that floor has passed and reports exactly the real time, so the game's speed stays the
// same at any cap; only the number of frames changes. The 0.25 s ceiling (max_reported_frametime) is not touched.
// The floor is set again whenever something else wrote it: frametime_initialize sets 1/120 at the first FrameDone
// and frametime_restore_limits (cutscene_cleanup) puts back the value saved by the CRT at startup (1/120; the
// matching frametime_unlimit has no caller), so a one-time write would be lost after the first cutscene.
// With a cap, FrameDone also carries the fraction of a millisecond the game drops when it turns the frame time into
// the millisecond tickers (exp_display_exact_ms; GameTimer, RealtimeTimer and the bitmap animations), so those
// clocks do not run slower than the frame time at caps such as 144 (6.94 ms frames would count as 6 ms).
//
// VSync (0 = as the game, 1 = on): on the OpenGL renderer, wglSwapIntervalEXT(1) once a GL context exists, and
// again when the game's context or device context changes. The game's own vsync flag (gr_screen.vsync, the options
// and the cutscenes) is read only by the Direct3D and Glide back ends; for those the switch only logs that it is
// not supported here.
//
// FieldOfView is not implemented: see NOTES.md (the draw lists it would fill have no bounds check).
#include <windows.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

#include "vsdk/vsdk.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/gr/gr.h"

// From engine/rendering/levelrender.h and unnamed_render_scene_0x481680.h (not included: their other headers bring
// in warnings this file would not otherwise have). The draw-list counts of the last levelrender_render_scene.
extern int levelrender_attach_count;  // 0x22fb6c4
extern int levelrender_opaque_count;  // 0x22fb6c8
extern int levelrender_sort_count;    // 0x22fb6cc
#define DISPLAY_LEVELRENDER_MAX_ATTACH 2000   // LEVELRENDER_MAX_ATTACH
#define DISPLAY_LEVELRENDER_MAX_SORTED 500    // LEVELRENDER_MAX_SORTED (sort and opaque lists, not bounds-checked)

#define DISPLAY_CAP_MIN 30
#define DISPLAY_CAP_MAX 240
#define DISPLAY_REPORT_MS 5000   // the speed check line in experimental.log

// Read by FrameDone (vsdk/frametime.cpp, under EXPERIMENTAL_BUILD): nonzero = carry the millisecond fraction.
int exp_display_exact_ms;

typedef BOOL(WINAPI *display_swap_interval_proc)(int interval);
typedef int(WINAPI *display_get_swap_interval_proc)(void);

static float display_cap_dt;           // 0 = no cap set (the game's floor stays)
static int display_vsync;              // the switch
static bool display_vsync_looked_up;
static display_swap_interval_proc display_swap_interval;
static display_get_swap_interval_proc display_get_swap_interval;
static HGLRC display_vsync_rc;         // the context and device context the interval was set for
static HDC display_vsync_dc;

// The speed check: wall time against the game's real-time ticker, over DISPLAY_REPORT_MS.
static DWORD display_report_tick;
static int display_report_ticker;
static int display_report_frames;
static int display_max_sorted, display_max_opaque, display_max_attach;

static const char *display_renderer_name(int mode)
{
    switch (mode) {
    case GR_DIRECT3D:
        return "Direct3D";
    case GR_GLIDE:
        return "Glide";
    case GR_OPENGL:
        return "OpenGL";
    }
    return "unknown";
}

// OpenGL: set the swap interval for the current context (looked up once, the first time a context exists).
static void display_vsync_apply()
{
    HGLRC rc = wglGetCurrentContext();
    HDC dc = wglGetCurrentDC();
    if (!rc || !dc)
        return;
    if (rc == display_vsync_rc && dc == display_vsync_dc)
        return;
    display_vsync_rc = rc;
    display_vsync_dc = dc;
    if (!display_vsync_looked_up) {
        display_vsync_looked_up = true;
        display_swap_interval = (display_swap_interval_proc)wglGetProcAddress("wglSwapIntervalEXT");
        display_get_swap_interval = (display_get_swap_interval_proc)wglGetProcAddress("wglGetSwapIntervalEXT");
        if (!display_swap_interval)
            exp_log("display: VSync: the driver has no wglSwapIntervalEXT");
    }
    if (!display_swap_interval)
        return;
    BOOL ok = display_swap_interval(1);
    exp_log("display: VSync: wglSwapIntervalEXT(1) = %d, interval now %d", ok ? 1 : 0,
            display_get_swap_interval ? display_get_swap_interval() : -1);
}

void exp_display_init()
{
    int cap = exp_ini_int("Experimental", "FrameRateCap", 0);
    display_vsync = exp_ini_int("Experimental", "VSync", 0) != 0;

    // FrameRateCap. No stand-down marker: SummonerFix's [Game] MaxFPS is a byte patch it applies only to the
    // original Sum.exe (ApplyDisplayPatches is not run for a rebuilt exe), so nothing of SummonerFix overlaps.
    if (cap > 0) {
        if (cap < DISPLAY_CAP_MIN) {
            exp_log("display: FrameRateCap %d is below %d; using %d", cap, DISPLAY_CAP_MIN, DISPLAY_CAP_MIN);
            cap = DISPLAY_CAP_MIN;
        } else if (cap > DISPLAY_CAP_MAX) {
            exp_log("display: FrameRateCap %d is above %d; using %d", cap, DISPLAY_CAP_MAX, DISPLAY_CAP_MAX);
            cap = DISPLAY_CAP_MAX;
        }
        display_cap_dt = 1.0f / (float)cap;
        exp_display_exact_ms = 1;
        exp_log("display: FrameRateCap %d (min frame time %.6f s, was %.6f s; max %.3f s kept)", cap,
                display_cap_dt, min_reported_frametime, max_reported_frametime);
        min_reported_frametime = display_cap_dt;
    } else {
        exp_log("display: FrameRateCap off (the game's min frame time %.6f s)", min_reported_frametime);
    }

    // VSync (SummonerFix sets no swap interval; its SwapBuffers hook only draws).
    if (display_vsync) {
        if (gr_screen.mode == GR_OPENGL) {
            exp_log("display: VSync on (OpenGL)");
            display_vsync_apply();
        } else {
            exp_log("display: VSync is not supported on the %s renderer (only OpenGL); the game's own vsync setting "
                    "stays in charge", display_renderer_name(gr_screen.mode));
            display_vsync = 0;
        }
    }

    display_report_tick = GetTickCount();
    display_report_ticker = Timestamp_realtime_ticker;
    display_report_frames = 0;
}

void exp_display_frame()
{
    if (display_cap_dt != 0.0f && min_reported_frametime != display_cap_dt) {
        exp_log("display: min frame time was set to %.6f s; FrameRateCap puts back %.6f s", min_reported_frametime,
                display_cap_dt);
        min_reported_frametime = display_cap_dt;
    }
    if (display_vsync)
        display_vsync_apply();

    // The speed check and the draw-list peaks (logging only; nothing here changes the game).
    display_report_frames++;
    if (levelrender_sort_count > display_max_sorted)
        display_max_sorted = levelrender_sort_count;
    if (levelrender_opaque_count > display_max_opaque)
        display_max_opaque = levelrender_opaque_count;
    if (levelrender_attach_count > display_max_attach)
        display_max_attach = levelrender_attach_count;
    DWORD now = GetTickCount();
    DWORD wall = now - display_report_tick;
    if (wall >= DISPLAY_REPORT_MS) {
        int ticker = Timestamp_realtime_ticker - display_report_ticker;
        if (ticker < 0)
            ticker += MAX_TIME;
        exp_log("display: speed: wall %lu ms, game real-time ticker %d ms (%.1f%%), %d frames, fps %.1f, min dt %.6f; "
                "draw lists peak sorted %d opaque %d (of %d) attach %d (of %d)",
                (unsigned long)wall, ticker, ticker * 100.0 / (double)wall, display_report_frames, (double)fps,
                (double)min_reported_frametime, display_max_sorted, display_max_opaque, DISPLAY_LEVELRENDER_MAX_SORTED,
                display_max_attach, DISPLAY_LEVELRENDER_MAX_ATTACH);
        display_report_tick = now;
        display_report_ticker = Timestamp_realtime_ticker;
        display_report_frames = 0;
        display_max_sorted = display_max_opaque = display_max_attach = 0;
    }
}

#endif // EXPERIMENTAL_BUILD
