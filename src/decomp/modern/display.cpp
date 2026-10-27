// display.cpp - the in-source host's output window (include/modern/modern.h; rung L3b). Not original source.
//
// SummonerFix's display layer (src/summonerfix/display.c) written into the source: the game keeps rendering in its
// own pixel coordinates (gr_screen.max_w x max_h), and
//   - its window is borderless full screen on its monitor ([Display] Mode=Fullscreen, the default) or a window of
//     [Display] WindowWidth x WindowHeight (Mode=Window),
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
#ifdef EXPERIMENTAL_BUILD
#include "experimental/experimental.h"
#endif

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

static HWND display_wnd;
static bool display_borderless = true;
static int display_window_w = 1600, display_window_h = 1200;

// Game pixels -> window pixels for the current client size.
// menu_shift: a centred 1024-wide menu layout (only that part of the game screen is shown).
struct display_map { int cw, ch; double scale; int off_x, off_y; bool menu_shift; };
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
    m->menu_shift = false;
    // Outside play (state 4) the interface is a fixed 1024-wide layout at the left of a wider game screen: the output
    // is shifted so that layout is centred, and the empty right part moves off screen. The mouse uses the same map.
    if (display_center_menus < 0)
        display_center_menus = modern_ini_int("Display", "CenterMenus", 1);
    bool full_panel = false;
#ifdef EXPERIMENTAL_BUILD
    full_panel = gameseq_state_cur == 4 && exp_ws_full_panel();
#endif
    if (display_center_menus && gr_screen.max_w > 1024 && (gameseq_state_cur != 4 || full_panel)) {
        m->off_x += (int)((gr_screen.max_w - 1024) / 2.0 * m->scale + 0.5);
        m->menu_shift = gameseq_state_cur != 4;   // menus only, not the panels in play
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
    RECT r = { 0, 0, display_window_w, display_window_h };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    int w = r.right - r.left, h = r.bottom - r.top;
    RECT wa;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
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
    RECT r;
    window_rect(hwnd, &r);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetWindowLongA(hwnd, GWL_STYLE, display_borderless ? (LONG)(WS_POPUP | WS_VISIBLE) : (LONG)(WS_OVERLAPPEDWINDOW | WS_VISIBLE));
    SetWindowLongA(hwnd, GWL_EXSTYLE, 0);
    SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    SetActiveWindow(hwnd);
    SetForegroundWindow(hwnd);
    modern_log("modern display: %s %ldx%ld at %ld,%ld, game %dx%d", display_borderless ? "borderless" : "window",
               r.right - r.left, r.bottom - r.top, r.left, r.top, gr_screen.max_w, gr_screen.max_h);
}

// The game makes its window topmost and screen-sized on activation and pulls it back to the front when it loses
// the focus, so Alt+Tab to another program failed (SummonerFix's SetWindowPos hook). Losing the focus sends the
// window to the back; gaining it keeps the output geometry.
bool modern_window_activate(HWND hwnd, bool active)
{
    if (!modern_host() || hwnd != display_wnd || !display_wnd)
        return false;
    if (!active) {
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        RECT r;
        window_rect(hwnd, &r);
        SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE);
    }
    return true;
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
static volatile LONG agent_cursor, agent_x, agent_y;   // client pixels

void modern_request_capture(const char *path)
{
    lstrcpynA(capture_path, path, MAX_PATH);
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

// The back buffer at half size as a bottom-up 24-bit BMP (the size agent coordinates are in).
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

    int ow = w / 2, oh = h / 2, ostride = (ow * 3 + 3) & ~3;
    unsigned char *half = (unsigned char *)calloc((size_t)ostride * oh, 1);
    if (half) {
        for (int y = 0; y < oh; y++)
            for (int x = 0; x < ow; x++)
                memcpy(half + y * ostride + x * 3, full + (y * 2) * stride + (x * 2) * 3, 3);
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
        int used_w = round_int((m.menu_shift ? 1024 : gr_screen.max_w) * m.scale), used_h = round_int(gr_screen.max_h * m.scale);
        if (m.off_x > 0 || m.off_y > 0 || m.menu_shift || m.off_x + used_w < m.cw) {
            GLfloat old[4];
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, old);
            glClearColor(0, 0, 0, 1);
            glEnable(GL_SCISSOR_TEST);
            clear_rect(0, 0, m.off_x, m.ch);                                 // left bar
            clear_rect(m.off_x + used_w, 0, m.cw - m.off_x - used_w, m.ch);  // right bar
            clear_rect(0, 0, m.cw, m.off_y);                                 // bottom bar
            clear_rect(0, m.off_y + used_h, m.cw, m.ch - m.off_y - used_h);  // top bar
            if (!scissor)
                glDisable(GL_SCISSOR_TEST);
            glClearColor(old[0], old[1], old[2], old[3]);
        }
        modern_brightness(display_wnd, m.cw, m.ch);   // before the badge and a capture, as in SummonerFix
        modern_speed_badge(dc, m.cw, m.ch, m.off_x, m.off_y, m.scale);
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
        int gx = (int)((p->x - m.off_x) / m.scale), gy = (int)((p->y - m.off_y) / m.scale);
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
        p->y = m.off_y + round_int(p->y * m.scale);
        if (agent_cursor) {
            InterlockedExchange((LONG *)&agent_x, p->x);
            InterlockedExchange((LONG *)&agent_y, p->y);
        }
    }
    return ClientToScreen(hwnd, p);
}
