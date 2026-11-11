// display.cpp - the in-source host's output window (include/modern/modern.h; rung L3b). Not original source.
//
// SummonerFix's display layer (src/summonerfix/display.c) written into the source: the game keeps rendering in its
// own pixel coordinates (gr_screen.max_w x max_h), and
//   - its window is borderless full screen on its monitor ([Display] Mode=Fullscreen, the default) or a window of
//     [Display] WindowWidth x WindowHeight (Mode=Window); Mode=Exclusive (2026-11-10) is the borderless window with
//     the display mode set to [Display] ExclusiveWidth x ExclusiveHeight at ExclusiveRefresh Hz (0: the desktop's),
//     given back while the window is not in front (it is minimised then), and under Vulkan full-screen exclusive
//     (VK_EXT_full_screen_exclusive, modern/vk_device.cpp) where the driver offers it,
//   - [Display] Antialiasing (2, 4, 8) asks OpenGL for a multisampled pixel format (modern_choose_pixel_format);
//     reading a multisampled window back (thumbnails, captures, the brightness pass) resolves it,
//   - every glViewport is scaled into a centred, aspect-correct area of the window,
//   - the bars outside it are cleared before each swap,
//   - the mouse is mapped between window pixels and game pixels,
//   - the save thumbnail's read-back reads the scaled area and box-filters it down,
//   - agent mode's captures are taken from the back buffer (half size, 24-bit BMP).
//   - on a screen wider than 1024 the menus' 1024-wide layout is centred ([Display] CenterMenus, default 1), and in
//     the experimental build so are the full-screen panels in play (inventory, character ...) when its interface is
//     centred ([Display] CenterHud), as SummonerFix's GetMap does.
// The brightness and the fast-mode badge are drawn from modern_swap_buffers (modern/gamma.cpp).
#include <windows.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "modern/modern.h"
#include "summoner/summoner.h"
#ifdef FIXED_BUILD
#include "experimental/experimental.h"
#endif
#ifdef _WIN64
#include "modern/vk.h"
#endif

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

static HWND display_wnd;
static bool display_borderless = true;
static int display_window_w = 1600, display_window_h = 1200;

// ---- [Display] Mode=Exclusive ----
static bool display_exclusive;                 // Mode=Exclusive
static bool display_mode_wanted;               // a display mode other than the desktop's was asked for and is valid
static bool display_mode_set;                  // that mode is on now (given back while the window is not in front)
static DEVMODEA display_dm;

bool modern_display_exclusive()
{
    return display_exclusive;
}

static void exclusive_leave()
{
    if (!display_mode_set)
        return;
    ChangeDisplaySettingsA(NULL, 0);   // the desktop's mode from the registry
    display_mode_set = false;
}

static void exclusive_enter()
{
    if (!display_mode_wanted || display_mode_set)
        return;
    LONG r = ChangeDisplaySettingsA(&display_dm, CDS_FULLSCREEN);
    if (r != DISP_CHANGE_SUCCESSFUL) {
        modern_log("modern display: exclusive mode %lux%lu at %lu Hz failed (%ld); the desktop's mode stays",
                   display_dm.dmPelsWidth, display_dm.dmPelsHeight, display_dm.dmDisplayFrequency, r);
        display_mode_wanted = false;
        return;
    }
    display_mode_set = true;
    static bool at_exit;
    if (!at_exit) {
        at_exit = true;
        atexit(exclusive_leave);   // CDS_FULLSCREEN is temporary anyway; this gives it back before the process ends
    }
}

// The mode to set: the desktop's, with ExclusiveWidth / ExclusiveHeight / ExclusiveRefresh where they are above 0.
// A size the display does not offer at the asked refresh is tried at its default refresh, then dropped.
static void exclusive_choose()
{
    DEVMODEA cur;
    memset(&cur, 0, sizeof cur);
    cur.dmSize = sizeof cur;
    display_mode_wanted = false;
    if (!EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &cur))
        return;
    int w = modern_ini_int("Display", "ExclusiveWidth", 0), h = modern_ini_int("Display", "ExclusiveHeight", 0);
    int hz = modern_ini_int("Display", "ExclusiveRefresh", 0);
    memset(&display_dm, 0, sizeof display_dm);
    display_dm.dmSize = sizeof display_dm;
    display_dm.dmPelsWidth = w > 0 && h > 0 ? (DWORD)w : cur.dmPelsWidth;
    display_dm.dmPelsHeight = w > 0 && h > 0 ? (DWORD)h : cur.dmPelsHeight;
    display_dm.dmDisplayFrequency = hz > 0 ? (DWORD)hz : cur.dmDisplayFrequency;
    display_dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    if (display_dm.dmPelsWidth == cur.dmPelsWidth && display_dm.dmPelsHeight == cur.dmPelsHeight &&
        display_dm.dmDisplayFrequency == cur.dmDisplayFrequency) {
        modern_log("modern display: exclusive at the desktop's mode %lux%lu at %lu Hz (no mode change)",
                   cur.dmPelsWidth, cur.dmPelsHeight, cur.dmDisplayFrequency);
        return;
    }
    DWORD tried = display_dm.dmDisplayFrequency;
    if (ChangeDisplaySettingsA(&display_dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL) {
        display_dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;   // the size at the display's default refresh
        if (ChangeDisplaySettingsA(&display_dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL) {
            modern_log("modern display: exclusive mode %lux%lu is not offered by the display; the desktop's mode stays",
                       display_dm.dmPelsWidth, display_dm.dmPelsHeight);
            return;
        }
        modern_log("modern display: %lu Hz is not offered at %lux%lu; the display's default refresh", tried,
                   display_dm.dmPelsWidth, display_dm.dmPelsHeight);
        display_dm.dmDisplayFrequency = 0;
    }
    modern_log("modern display: exclusive mode %lux%lu at %lu Hz", display_dm.dmPelsWidth, display_dm.dmPelsHeight,
               display_dm.dmDisplayFrequency);
    display_mode_wanted = true;
}

// Game pixels -> window pixels for the current client size.
// menu_shift: a centred 1024x768 menu layout (only that part of the game screen is shown).
// off_y is from the bottom (glViewport's origin, and the Vulkan back end's mapping of it); off_y_top is the same
// placement from the top (the mouse, the badge). They differ by the vertical centring of a 768-line layout in a
// taller game ([Display] InterfaceScale, 2026-11-10): the picture moves down, so off_y_top grows and off_y shrinks.
struct display_map { int cw, ch; double scale; int off_x, off_y, off_y_top; bool menu_shift; };
static int display_center_menus = -1;   // [Display] CenterMenus, read at the first frame

static bool get_map(display_map *m)
{
    RECT rc;
    if (!modern_host() || !display_wnd || !GetClientRect(display_wnd, &rc) || gr_screen.max_w <= 0 || gr_screen.max_h <= 0)
        return false;
    m->cw = rc.right - rc.left;
    m->ch = rc.bottom - rc.top;
    if (m->cw <= 0 || m->ch <= 0)
        return false;
    double sx = (double)m->cw / gr_screen.max_w, sy = (double)m->ch / gr_screen.max_h;
    m->scale = sx < sy ? sx : sy;
    m->off_x = (int)((m->cw - gr_screen.max_w * m->scale) / 2 + 0.5);
    m->off_y = (int)((m->ch - gr_screen.max_h * m->scale) / 2 + 0.5);
    m->off_y_top = m->off_y;
    m->menu_shift = false;
    // Outside play (state 4) the interface is a fixed 1024-wide layout at the left of a wider game screen: the output
    // is shifted so that layout is centred, and the empty right part moves off screen. The mouse uses the same map.
    if (display_center_menus < 0)
        display_center_menus = modern_ini_int("Display", "CenterMenus", 1);
    bool full_panel = false;
#ifdef FIXED_BUILD
    full_panel = gameseq_state_cur == 4 && exp_ws_full_panel();
#endif
    if (display_center_menus && gr_screen.max_w > 1024 && (gameseq_state_cur != 4 || full_panel)) {
        m->off_x += (int)((gr_screen.max_w - 1024) / 2.0 * m->scale + 0.5);
        m->menu_shift = gameseq_state_cur != 4;   // menus only, not the panels in play
    }
    // A game taller than 768 (InterfaceScale): the layout's 768 lines at the top are centred the same way in y.
    if (display_center_menus && gr_screen.max_h > 768 && (gameseq_state_cur != 4 || full_panel)) {
        int d = (int)((gr_screen.max_h - 768) / 2.0 * m->scale + 0.5);
        m->off_y -= d;
        m->off_y_top += d;
        m->menu_shift = gameseq_state_cur != 4;
    }
    return true;
}

// The same map for the Vulkan back end (modern/vk_device.cpp), which draws into the window itself.
bool modern_display_map(int *cw, int *ch, double *scale, int *off_x, int *off_y, bool *menu_shift)
{
    display_map m;
    if (!get_map(&m))
        return false;
    *cw = m.cw;
    *ch = m.ch;
    *scale = m.scale;
    *off_x = m.off_x;
    *off_y = m.off_y;
    *menu_shift = m.menu_shift;
    return true;
}

static int round_int(double v) { return (int)(v < 0 ? v - 0.5 : v + 0.5); }

// The window's rectangle: the primary screen (borderless) or the configured client size, centred in the work area.
// (SummonerFix takes the window's monitor; the primary screen and work area are what the game itself uses, and the
// only monitor calls VC6's headers and libraries have.)
static void window_rect(HWND, RECT *out)
{
    if (display_borderless) {
        SetRect(out, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        return;
    }
    RECT wa;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
    // A client size larger than the work area is shrunk, keeping its shape, so the window fits (the default
    // 1600x1200 is taller than a 1080p screen).
    RECT r = { 0, 0, display_window_w, display_window_h };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    int fw = (r.right - r.left) - display_window_w, fh = (r.bottom - r.top) - display_window_h;
    double fit = 1.0;
    if (display_window_w > 0 && display_window_h > 0) {
        double fx = (double)((wa.right - wa.left) - fw) / display_window_w;
        double fy = (double)((wa.bottom - wa.top) - fh) / display_window_h;
        fit = fx < fy ? fx : fy;
    }
    if (fit < 1.0 && fit > 0.0) {
        SetRect(&r, 0, 0, round_int(display_window_w * fit), round_int(display_window_h * fit));
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    }
    int w = r.right - r.left, h = r.bottom - r.top;
    int x = wa.left + ((wa.right - wa.left) - w) / 2, y = wa.top + ((wa.bottom - wa.top) - h) / 2;
    if (x < wa.left) x = wa.left;
    if (y < wa.top) y = wa.top;
    SetRect(out, x, y, x + w, y + h);
}

void modern_window_setup(HWND hwnd)
{
    char mode[32];
    modern_ini_str("Display", "Mode", "Fullscreen", mode, sizeof mode);
    display_borderless = lstrcmpiA(mode, "Window") != 0;
    display_window_w = modern_ini_int("Display", "WindowWidth", 1600);
    display_window_h = modern_ini_int("Display", "WindowHeight", 1200);
    display_wnd = hwnd;
    if (!lstrcmpiA(mode, "Exclusive")) {
        // Before the window's rectangle: the screen's size is the new mode's.
        if (!display_exclusive)
            exclusive_choose();
        display_exclusive = true;
        exclusive_enter();
    }
    RECT r;
    window_rect(hwnd, &r);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetWindowLongA(hwnd, GWL_STYLE, display_borderless ? (LONG)(WS_POPUP | WS_VISIBLE) : (LONG)(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
    SetWindowLongA(hwnd, GWL_EXSTYLE, 0);
    SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    SetActiveWindow(hwnd);
    SetForegroundWindow(hwnd);
    modern_log("modern display: %s %ldx%ld at %ld,%ld, game %dx%d",
               display_exclusive ? "exclusive" : display_borderless ? "borderless" : "window",
               r.right - r.left, r.bottom - r.top, r.left, r.top, gr_screen.max_w, gr_screen.max_h);
}

// The game makes its window topmost and screen-sized on activation and pulls it back to the front when it loses
// the focus, so Alt+Tab to another program failed (SummonerFix's SetWindowPos hook). Losing the focus sends the
// window to the back; gaining it keeps the output geometry.
bool modern_window_activate(HWND hwnd, bool active)
{
    if (!modern_host() || hwnd != display_wnd || !display_wnd)
        return false;
#ifdef _WIN64
    if (display_exclusive && modern_vk_active)
        vk_window_active(active);   // full-screen exclusive released now, acquired again at the next frame
#endif
    if (!active && display_exclusive) {
        // Exclusive: the desktop's mode back and the window minimised (not activating anything: the program being
        // switched to is), as full-screen games do; activation (the taskbar, Alt+Tab) restores both.
        exclusive_leave();
        ShowWindow(hwnd, SW_SHOWMINNOACTIVE);
        modern_log("modern display: exclusive: focus lost, minimised");
    } else if (!active) {
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        if (display_exclusive) {
            exclusive_enter();
            if (IsIconic(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
            modern_log("modern display: exclusive: focus back");
        }
        RECT r;
        window_rect(hwnd, &r);
        SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE);
    }
    return true;
}

// ---- [Display] Antialiasing under OpenGL ----
// WGL_ARB_pixel_format / WGL_ARB_multisample (the extension specifications' values).
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
typedef BOOL (WINAPI *wgl_choose_pixel_format_fn)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);

static int gl_aa_format;                       // the multisampled format found, 0 = none
static bool gl_aa_tried;

// wglChoosePixelFormatARB needs a current context, and a window's pixel format can be set only once: a hidden
// window gets a plain format and a throwaway context to ask through, and the format found is for the game's DC.
static int find_multisample_format(HDC dc, int want)
{
    HINSTANCE inst = GetModuleHandleA(NULL);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = inst;
    wc.lpszClassName = "SummonerPixelFormatProbe";
    RegisterClassA(&wc);
    HWND probe = CreateWindowA(wc.lpszClassName, "", WS_POPUP, 0, 0, 1, 1, NULL, NULL, inst, NULL);
    HDC pdc = probe ? GetDC(probe) : NULL;
    int found = 0, samples = 0;
    if (pdc) {
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof pfd);
        pfd.nSize = sizeof pfd;
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 16;
        HGLRC rc = NULL;
        int pf = ChoosePixelFormat(pdc, &pfd);
        if (pf && SetPixelFormat(pdc, pf, &pfd))
            rc = wglCreateContext(pdc);
        HDC old_dc = wglGetCurrentDC();
        HGLRC old_rc = wglGetCurrentContext();
        if (rc && wglMakeCurrent(pdc, rc)) {
            wgl_choose_pixel_format_fn choose =
                (wgl_choose_pixel_format_fn)(void *)wglGetProcAddress("wglChoosePixelFormatARB");
            // The original's request (double-buffered RGBA, 16-bit depth at least), accelerated, multisampled; fewer
            // samples when the driver has no format with as many.
            for (int n = want; choose && n >= 2 && !found; n /= 2) {
                const int attrs[] = { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1,
                                      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, WGL_ACCELERATION_ARB,
                                      WGL_FULL_ACCELERATION_ARB, WGL_COLOR_BITS_ARB, 16, WGL_DEPTH_BITS_ARB, 16,
                                      WGL_SAMPLE_BUFFERS_ARB, 1, WGL_SAMPLES_ARB, n, 0 };
                int fmt = 0;
                UINT count = 0;
                if (choose(dc, attrs, NULL, 1, &fmt, &count) && count > 0 && fmt > 0) {
                    found = fmt;
                    samples = n;
                }
            }
            if (!choose)
                modern_log("modern display: antialiasing: the driver has no wglChoosePixelFormatARB");
        }
        wglMakeCurrent(old_dc, old_rc);
        if (rc)
            wglDeleteContext(rc);
        ReleaseDC(probe, pdc);
    }
    if (probe)
        DestroyWindow(probe);
    UnregisterClassA(wc.lpszClassName, inst);
    if (found) {
        PIXELFORMATDESCRIPTOR got;
        DescribePixelFormat(dc, found, sizeof got, &got);
        modern_log("modern display: antialiasing: pixel format %d, %d samples (%d asked for), colour %d, depth %d bits",
                   found, samples, want, got.cColorBits, got.cDepthBits);
    } else {
        modern_log("modern display: antialiasing: no multisampled pixel format; drawing without");
    }
    return found;
}

int WINAPI modern_choose_pixel_format(HDC dc, const PIXELFORMATDESCRIPTOR *pfd)
{
    int want = modern_host() ? modern_ini_int("Display", "Antialiasing", 0) : 0;
    if (want > 1) {
        // The same format each time: gr_opengl_init_context sets it again when the context is made again.
        if (!gl_aa_tried) {
            gl_aa_tried = true;
            gl_aa_format = find_multisample_format(dc, want >= 8 ? 8 : want >= 4 ? 4 : 2);
        }
        if (gl_aa_format)
            return gl_aa_format;
    }
    return ChoosePixelFormat(dc, pfd);
}

void modern_gl_viewport(int x, int y, int w, int h)
{
    display_map m;
    if (!get_map(&m)) {
        glViewport(x, y, w, h);
        return;
    }
    // Edges, not sizes, are mapped, so adjacent viewports stay seamless.
    int x0 = m.off_x + round_int(x * m.scale), x1 = m.off_x + round_int((x + w) * m.scale);
    int y0 = m.off_y + round_int(y * m.scale), y1 = m.off_y + round_int((y + h) * m.scale);
    glViewport(x0, y0, x1 - x0, y1 - y0);
}

// The save thumbnail: the scene drawn into a 201x151 clip in game pixels and read back. The framebuffer holds the
// scaled picture, so the mapped rectangle is read and box-filtered down to the requested size.
void modern_gl_read_pixels(int x, int y, int w, int h, unsigned format, unsigned type, void *data)
{
    display_map m;
    if (!get_map(&m) || format != GL_RGBA || type != GL_UNSIGNED_BYTE || w <= 0 || h <= 0) {
        glReadPixels(x, y, w, h, format, type, data);
        return;
    }
    int x0 = m.off_x + round_int(x * m.scale), x1 = m.off_x + round_int((x + w) * m.scale);
    int y0 = m.off_y + round_int(y * m.scale), y1 = m.off_y + round_int((y + h) * m.scale);
    int fw = x1 - x0, fh = y1 - y0;
    unsigned char *src = fw > 0 && fh > 0 ? (unsigned char *)malloc((size_t)fw * fh * 4) : NULL;
    if (!src) {
        glReadPixels(x, y, w, h, format, type, data);
        return;
    }
    glReadPixels(x0, y0, fw, fh, format, type, src);
    unsigned char *dst = (unsigned char *)data;
    for (int dy = 0; dy < h; dy++) {
        int sy0 = dy * fh / h, sy1 = (dy + 1) * fh / h;
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int dx = 0; dx < w; dx++) {
            int sx0 = dx * fw / w, sx1 = (dx + 1) * fw / w;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            unsigned sum[4] = { 0, 0, 0, 0 }, n = 0;
            for (int sy = sy0; sy < sy1 && sy < fh; sy++)
                for (int sx = sx0; sx < sx1 && sx < fw; sx++, n++)
                    for (int c = 0; c < 4; c++)
                        sum[c] += src[((size_t)sy * fw + sx) * 4 + c];
            for (int c = 0; c < 4; c++)
                dst[((size_t)dy * w + dx) * 4 + c] = (unsigned char)(n ? sum[c] / n : 0);
        }
    }
    free(src);
}

// ---- agent mode: captures and the agent's cursor ----
static char capture_path[MAX_PATH];
static volatile LONG capture_wanted;
static int capture_step = 2;                           // 2: half size (agent coordinates); 1: full size
static volatile LONG agent_cursor, agent_x, agent_y;   // client pixels

void modern_request_capture(const char *path, bool full)
{
    lstrcpynA(capture_path, path, MAX_PATH);
    capture_step = full ? 1 : 2;
    InterlockedExchange((LONG *)&capture_wanted, 1);
}

bool modern_capture_pending() { return capture_wanted != 0; }

void modern_set_agent_cursor(int client_x, int client_y)
{
    InterlockedExchange((LONG *)&agent_x, client_x);
    InterlockedExchange((LONG *)&agent_y, client_y);
    InterlockedExchange((LONG *)&agent_cursor, 1);
}

void modern_get_agent_cursor(int *client_x, int *client_y)
{
    *client_x = agent_x;
    *client_y = agent_y;
}

// The back buffer at half size as a bottom-up 24-bit BMP (the size agent coordinates are in), or at full size.
static void save_frame(const display_map *m, const char *path)
{
    int w = m->cw, h = m->ch, stride = (w * 3 + 3) & ~3;
    unsigned char *full = (unsigned char *)malloc((size_t)stride * h);
    if (!full)
        return;
    GLint old_align, old_read;
    glGetIntegerv(GL_PACK_ALIGNMENT, &old_align);
    glGetIntegerv(GL_READ_BUFFER, &old_read);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, full);
    glReadBuffer((GLenum)old_read);
    glPixelStorei(GL_PACK_ALIGNMENT, old_align);

    int s = capture_step, ow = w / s, oh = h / s, ostride = (ow * 3 + 3) & ~3;
    unsigned char *half = (unsigned char *)calloc((size_t)ostride * oh, 1);
    if (half) {
        for (int y = 0; y < oh; y++)
            for (int x = 0; x < ow; x++)
                memcpy(half + y * ostride + x * 3, full + (y * s) * stride + (x * s) * 3, 3);
        BITMAPFILEHEADER fh = { 0x4D42, 0, 0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) };
        BITMAPINFOHEADER ih = { sizeof ih, ow, oh, 1, 24, BI_RGB, (DWORD)(ostride * oh), 0, 0, 0, 0 };
        fh.bfSize = fh.bfOffBits + ih.biSizeImage;
        HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD wr;
            WriteFile(f, &fh, sizeof fh, &wr, NULL);
            WriteFile(f, &ih, sizeof ih, &wr, NULL);
            WriteFile(f, half, ih.biSizeImage, &wr, NULL);
            CloseHandle(f);
        }
        free(half);
    }
    free(full);
}

// The Vulkan back end's frame (modern/vk_device.cpp): the window's pixels top row first as B, G, R, A, written as
// save_frame writes the OpenGL back buffer.
bool modern_capture_wanted() { return capture_wanted != 0; }

void modern_capture_bgra(const unsigned char *bgra, int w, int h)
{
    int s = capture_step, ow = w / s, oh = h / s, ostride = (ow * 3 + 3) & ~3;
    unsigned char *half = (unsigned char *)calloc((size_t)ostride * oh, 1);
    if (half) {
        for (int y = 0; y < oh; y++)   // bottom-up rows
            for (int x = 0; x < ow; x++)
                memcpy(half + y * ostride + x * 3, bgra + ((size_t)(h - 1 - y * s) * w + x * s) * 4, 3);
        BITMAPFILEHEADER fh = { 0x4D42, 0, 0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) };
        BITMAPINFOHEADER ih = { sizeof ih, ow, oh, 1, 24, BI_RGB, (DWORD)(ostride * oh), 0, 0, 0, 0 };
        fh.bfSize = fh.bfOffBits + ih.biSizeImage;
        HANDLE f = CreateFileA(capture_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD wr;
            WriteFile(f, &fh, sizeof fh, &wr, NULL);
            WriteFile(f, &ih, sizeof ih, &wr, NULL);
            WriteFile(f, half, ih.biSizeImage, &wr, NULL);
            CloseHandle(f);
        }
        free(half);
    }
    InterlockedExchange((LONG *)&capture_wanted, 0);
}

static void clear_rect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
        return;
    glScissor(x, y, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
}

BOOL modern_swap_buffers(HDC dc)
{
    display_map m;
    if (get_map(&m)) {
        // A menu shows only its 1024x768 layout: the layout's top 768 lines, bottom-up from off_y + (max_h - 768).
        int lay_w = m.menu_shift && gr_screen.max_w > 1024 ? 1024 : gr_screen.max_w;
        int lay_h = m.menu_shift && gr_screen.max_h > 768 ? 768 : gr_screen.max_h;
        int used_w = round_int(lay_w * m.scale), used_h = round_int(lay_h * m.scale);
        int used_y = m.off_y + round_int((gr_screen.max_h - lay_h) * m.scale);
        if (m.off_x > 0 || used_y > 0 || m.menu_shift || m.off_x + used_w < m.cw) {
            GLfloat old[4];
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, old);
            glClearColor(0, 0, 0, 1);
            glEnable(GL_SCISSOR_TEST);
            clear_rect(0, 0, m.off_x, m.ch);                                 // left bar
            clear_rect(m.off_x + used_w, 0, m.cw - m.off_x - used_w, m.ch);  // right bar
            clear_rect(0, 0, m.cw, used_y);                                  // bottom bar
            clear_rect(0, used_y + used_h, m.cw, m.ch - used_y - used_h);    // top bar
            if (!scissor)
                glDisable(GL_SCISSOR_TEST);
            glClearColor(old[0], old[1], old[2], old[3]);
        }
        modern_brightness(display_wnd, m.cw, m.ch);   // before the badge and a capture, as in SummonerFix
        modern_speed_badge(dc, m.cw, m.ch, m.off_x, m.off_y_top, m.scale);
        if (capture_wanted) {
            save_frame(&m, capture_path);
            InterlockedExchange((LONG *)&capture_wanted, 0);
        }
    }
    return SwapBuffers(dc);
}

// The mouse poll (GetCursorPos + ScreenToClient): the agent's cursor replaces the real one, then window pixels
// become game pixels, clamped to the picture.
BOOL modern_screen_to_client(HWND hwnd, POINT *p)
{
    BOOL ok = ScreenToClient(hwnd, p);
    display_map m;
    if (ok && hwnd == display_wnd && agent_cursor) {
        p->x = agent_x;
        p->y = agent_y;
    }
    if (ok && hwnd == display_wnd && get_map(&m)) {
        int gx = (int)((p->x - m.off_x) / m.scale), gy = (int)((p->y - m.off_y_top) / m.scale);
        p->x = gx < 0 ? 0 : gx >= gr_screen.max_w ? gr_screen.max_w - 1 : gx;
        p->y = gy < 0 ? 0 : gy >= gr_screen.max_h ? gr_screen.max_h - 1 : gy;
    }
    return ok;
}

// The mouse warp (ClientToScreen + SetCursorPos, mouse look): game pixels become window pixels; the agent's cursor
// follows.
BOOL modern_client_to_screen(HWND hwnd, POINT *p)
{
    display_map m;
    if (hwnd == display_wnd && get_map(&m)) {
        p->x = m.off_x + round_int(p->x * m.scale);
        p->y = m.off_y_top + round_int(p->y * m.scale);
        if (agent_cursor) {
            InterlockedExchange((LONG *)&agent_x, p->x);
            InterlockedExchange((LONG *)&agent_y, p->y);
        }
    }
    return ClientToScreen(hwnd, p);
}
