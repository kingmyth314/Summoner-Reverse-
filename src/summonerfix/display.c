// Resolution-independent output for the OpenGL renderer.
//
// The game lays out its UI, clip rectangles and menus in fixed pixel coordinates for its internal
// resolution (normally 1024x768; see docs/summonerfix). Instead of patching every one of those, the
// game keeps rendering in its own coordinates and we
//   - make its window borderless-fullscreen (or a resizable window of the configured size),
//   - scale every glViewport call into a centred, aspect-correct area of the real window, so
//     OpenGL rasterises all geometry at the monitor's native resolution,
//   - clear the unused bars each frame (before SwapBuffers),
//   - map mouse coordinates between window pixels and game pixels.
#include "summonerfix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_CLEAR_VALUE 0x0C22

typedef void(WINAPI *PFN_glViewport)(int, int, int, int);
typedef void(WINAPI *PFN_glScissor)(int, int, int, int);
typedef void(WINAPI *PFN_glClear)(unsigned int);
typedef void(WINAPI *PFN_glClearColor)(float, float, float, float);
typedef void(WINAPI *PFN_glGetFloatv)(unsigned int, float *);
typedef void(WINAPI *PFN_glCap)(unsigned int);
typedef unsigned char(WINAPI *PFN_glIsEnabled)(unsigned int);
typedef BOOL(WINAPI *PFN_SwapBuffers)(HDC);
typedef LONG(WINAPI *PFN_SetWindowLongA)(HWND, int, LONG);
typedef BOOL(WINAPI *PFN_SetWindowPos)(HWND, HWND, int, int, int, int, UINT);
typedef BOOL(WINAPI *PFN_PointFn)(HWND, LPPOINT);
typedef int(WINAPI *PFN_GetSystemMetrics)(int);

static PFN_glViewport real_glViewport;
static PFN_SwapBuffers real_SwapBuffers;
static PFN_SetWindowLongA real_SetWindowLongA;
static PFN_SetWindowPos real_SetWindowPos;
static PFN_PointFn real_ScreenToClient, real_ClientToScreen;
static PFN_GetSystemMetrics real_GetSystemMetrics;
static PFN_glScissor p_glScissor;
static PFN_glClear p_glClear;
static PFN_glClearColor p_glClearColor;
static PFN_glGetFloatv p_glGetFloatv;
static PFN_glCap p_glEnable, p_glDisable;
static PFN_glIsEnabled p_glIsEnabled;

static HWND g_wnd;

#define VA_GAMESEQ_STATE_CUR 0x5b27e0u // current game state; 4 = in game

// Game-pixel -> window-pixel mapping for the current client size.
typedef struct { int cw, ch; double scale; int offX, offY; int menuShift; } Map; // menuShift: a centred 1024-wide menu

static BOOL GetMap(Map *m)
{
    RECT rc;
    if (!g_wnd || !GetClientRect(g_wnd, &rc))
        return FALSE;
    m->cw = rc.right - rc.left;
    m->ch = rc.bottom - rc.top;
    if (m->cw <= 0 || m->ch <= 0)
        return FALSE;
    double sx = (double)m->cw / g_cfg.width, sy = (double)m->ch / g_cfg.height;
    m->scale = sx < sy ? sx : sy;
    m->offX = (int)((m->cw - g_cfg.width * m->scale) / 2 + 0.5);
    m->offY = (int)((m->ch - g_cfg.height * m->scale) / 2 + 0.5);
    m->menuShift = 0;
    // Widescreen menus: outside gameplay (game state 4) the UI is a fixed 1024-wide layout at the left of
    // the wider game screen. Shift the output so that layout is centred; the empty right part moves off
    // screen. The mouse uses this same mapping, so clicks stay aligned.
    // In game, full-screen panels (inventory, character, ...) are shifted the same way (WidescreenFullPanel).
    if (g_cfg.centerMenus && g_cfg.width > 1024 &&
        (*(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) != 4 || WidescreenFullPanel()))
    {
        m->offX += (int)((g_cfg.width - 1024) / 2.0 * m->scale + 0.5);
        m->menuShift = *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) != 4; // menus only, not in-game panels
    }
    return TRUE;
}

static int Round(double v) { return (int)(v < 0 ? v - 0.5 : v + 0.5); }

static void WINAPI Hook_glViewport(int x, int y, int w, int h)
{
    Map m;
    if (!GetMap(&m)) {
        real_glViewport(x, y, w, h);
        return;
    }
    // Map edges (not sizes) so adjacent viewports stay seamless.
    int x0 = m.offX + Round(x * m.scale), x1 = m.offX + Round((x + w) * m.scale);
    int y0 = m.offY + Round(y * m.scale), y1 = m.offY + Round((y + h) * m.scale);
    real_glViewport(x0, y0, x1 - x0, y1 - y0);
}

// Savegame thumbnails: the game renders the scene into a 201x151 clip and reads it back with
// gr_opengl_read_backbuffer (0x5502c0), the only caller of glReadPixels, in game-pixel coordinates. Our
// framebuffer is the scaled output, so read the mapped rectangle and box-filter it down to the requested size
// (unmapped, the read hit an unrelated region and thumbnails came out black).
typedef void(WINAPI *PFN_glReadPixels)(int, int, int, int, unsigned, unsigned, void *);
static PFN_glReadPixels real_glReadPixels;

static void WINAPI Hook_glReadPixels(int x, int y, int w, int h, unsigned format, unsigned type, void *data)
{
    Map m;
    if (!GetMap(&m) || format != 0x1908 /* GL_RGBA */ || type != 0x1401 /* GL_UNSIGNED_BYTE */ || w <= 0 || h <= 0) {
        real_glReadPixels(x, y, w, h, format, type, data);
        return;
    }
    int x0 = m.offX + Round(x * m.scale), x1 = m.offX + Round((x + w) * m.scale);
    int y0 = m.offY + Round(y * m.scale), y1 = m.offY + Round((y + h) * m.scale);
    int fw = x1 - x0, fh = y1 - y0;
    unsigned char *src = fw > 0 && fh > 0 ? (unsigned char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)fw * fh * 4) : NULL;
    if (!src) {
        real_glReadPixels(x, y, w, h, format, type, data);
        return;
    }
    real_glReadPixels(x0, y0, fw, fh, format, type, src);
    unsigned char *dst = (unsigned char *)data;
    for (int dy = 0; dy < h; dy++) {
        int sy0 = dy * fh / h, sy1 = (dy + 1) * fh / h;
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int dx = 0; dx < w; dx++) {
            int sx0 = dx * fw / w, sx1 = (dx + 1) * fw / w;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            unsigned sum[4] = {0, 0, 0, 0}, n = 0;
            for (int sy = sy0; sy < sy1 && sy < fh; sy++)
                for (int sx = sx0; sx < sx1 && sx < fw; sx++, n++)
                    for (int c = 0; c < 4; c++)
                        sum[c] += src[((size_t)sy * fw + sx) * 4 + c];
            for (int c = 0; c < 4; c++)
                dst[((size_t)dy * w + dx) * 4 + c] = (unsigned char)(n ? sum[c] / n : 0);
        }
    }
    HeapFree(GetProcessHeap(), 0, src);
}

static void ClearRect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
        return;
    p_glScissor(x, y, w, h);
    p_glClear(GL_COLOR_BUFFER_BIT);
}

// Debug: save the back buffer (half size, 24-bit BMP) every CaptureEvery seconds.
#define GL_BACK 0x0405
#define GL_READ_BUFFER 0x0C02
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_BGR_EXT 0x80E0
#define GL_UNSIGNED_BYTE 0x1401
typedef void(WINAPI *PFN_glReadPixels)(int, int, int, int, unsigned int, unsigned int, void *);
typedef void(WINAPI *PFN_glGetIntegerv)(unsigned int, int *);
typedef void(WINAPI *PFN_glPixelStorei)(unsigned int, int);
typedef void(WINAPI *PFN_glReadBuffer)(unsigned int);
static PFN_glReadPixels p_glReadPixels;
static PFN_glGetIntegerv p_glGetIntegerv;
static PFN_glPixelStorei p_glPixelStorei;
static PFN_glReadBuffer p_glReadBuffer;

// Save the back buffer at half size as a 24-bit BMP.
static void SaveFrame(const Map *m, const char *path)
{
    if (!p_glReadPixels)
        return;
    int w = m->cw, h = m->ch, stride = (w * 3 + 3) & ~3;
    BYTE *full = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)stride * h);
    if (!full)
        return;
    int oldAlign, oldRead;
    p_glGetIntegerv(GL_PACK_ALIGNMENT, &oldAlign);
    p_glGetIntegerv(GL_READ_BUFFER, &oldRead);
    p_glPixelStorei(GL_PACK_ALIGNMENT, 4);
    p_glReadBuffer(GL_BACK);
    p_glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, full);
    p_glReadBuffer((unsigned int)oldRead);
    p_glPixelStorei(GL_PACK_ALIGNMENT, oldAlign);

    int ow = w / 2, oh = h / 2, ostride = (ow * 3 + 3) & ~3; // bottom-up rows, as BMP expects
    BYTE *half = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)ostride * oh);
    if (half) {
        for (int y = 0; y < oh; y++)
            for (int x = 0; x < ow; x++)
                memcpy(half + y * ostride + x * 3, full + (y * 2) * stride + (x * 2) * 3, 3);
        BITMAPFILEHEADER fh = {0x4D42, 0, 0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)};
        BITMAPINFOHEADER ih = {sizeof ih, ow, oh, 1, 24, BI_RGB, (DWORD)(ostride * oh), 0, 0, 0, 0};
        fh.bfSize = fh.bfOffBits + ih.biSizeImage;
        HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD wr;
            WriteFile(f, &fh, sizeof fh, &wr, NULL);
            WriteFile(f, &ih, sizeof ih, &wr, NULL);
            WriteFile(f, half, ih.biSizeImage, &wr, NULL);
            CloseHandle(f);
            Log("captured frame %dx%d (client %dx%d) -> %s", ow, oh, w, h, path);
        }
        HeapFree(GetProcessHeap(), 0, half);
    }
    HeapFree(GetProcessHeap(), 0, full);
}

static void CaptureFrame(const Map *m)
{
    static DWORD next;
    static int index;
    DWORD now = GetTickCount();
    if ((int)(now - next) < 0)
        return;
    next = now + (DWORD)g_cfg.captureEvery * 1000;
    char path[MAX_PATH];
    lstrcpynA(path, g_cfg.iniPath, MAX_PATH);
    char *slash = strrchr(path, '\\');
    wsprintfA(slash ? slash + 1 : path, "summonerfix-capture-%d.bmp", ++index);
    SaveFrame(m, path);
}

// Agent mode (agent.c): one-shot capture requests, and a cursor position that replaces the real one.
// g_agentCursor: 0 = the real cursor; 1 = agent mode set the position (it keeps it); 2 = the controller moved it
// (controller.c): the real cursor is moved along, and the override ends as soon as the real mouse moves.
static char g_capturePath[MAX_PATH];
static volatile LONG g_captureWanted;
static volatile LONG g_agentCursor, g_agentX, g_agentY; // client pixels
static volatile LONG g_padRealX, g_padRealY;            // mode 2: where the real cursor was put (screen pixels)

void DisplayRequestCapture(const char *path)
{
    lstrcpynA(g_capturePath, path, MAX_PATH);
    InterlockedExchange(&g_captureWanted, 1);
}

BOOL DisplayCapturePending(void) { return g_captureWanted != 0; }

void DisplaySetAgentCursor(int clientX, int clientY)
{
    InterlockedExchange(&g_agentX, clientX);
    InterlockedExchange(&g_agentY, clientY);
    InterlockedExchange(&g_agentCursor, 1);
}

void DisplayGetAgentCursor(int *clientX, int *clientY)
{
    *clientX = g_agentX;
    *clientY = g_agentY;
}

// Controller (controller.c, game thread): move the cursor by (dx, dy) game pixels, kept inside the game picture.
// Fractions are carried over, so slow stick movements still move the cursor.
void DisplayMoveCursor(double dxGame, double dyGame)
{
    static double fracX, fracY;
    Map m;
    if (!GetMap(&m))
        return;
    double fx = dxGame * m.scale + fracX, fy = dyGame * m.scale + fracY;
    int ix = (int)fx, iy = (int)fy; // toward zero; the rest is kept
    fracX = fx - ix;
    fracY = fy - iy;
    if (!ix && !iy)
        return;
    LONG mode = g_agentCursor;
    int x, y;
    if (mode) {
        x = g_agentX;
        y = g_agentY;
    } else { // start from the real cursor (this DLL's ScreenToClient import is not hooked)
        POINT p;
        if (!GetCursorPos(&p) || !ScreenToClient(g_wnd, &p))
            return;
        x = p.x;
        y = p.y;
    }
    int usedW = (int)((m.menuShift ? 1024 : g_cfg.width) * m.scale + 0.5), usedH = (int)(g_cfg.height * m.scale + 0.5);
    int left = m.offX, right = m.offX + usedW - 1, top = m.offY, bottom = m.offY + usedH - 1;
    if (right > m.cw - 1) right = m.cw - 1;
    if (bottom > m.ch - 1) bottom = m.ch - 1;
    x += ix;
    y += iy;
    x = x < left ? left : x > right ? right : x;
    y = y < top ? top : y > bottom ? bottom : y;
    InterlockedExchange(&g_agentX, x);
    InterlockedExchange(&g_agentY, y);
    if (mode == 1)
        return; // agent mode owns the cursor: leave the real one alone
    // Move the real cursor along, so the mouse carries on from here; remember where it landed (it may be clipped).
    POINT s = {x, y};
    ClientToScreen(g_wnd, &s);
    SetCursorPos(s.x, s.y);
    GetCursorPos(&s);
    InterlockedExchange(&g_padRealX, s.x);
    InterlockedExchange(&g_padRealY, s.y);
    InterlockedExchange(&g_agentCursor, 2);
}

// Fast-mode indicator: a small "FAST x3" badge at the top-left of the game picture while the speed key's fast
// mode is on during gameplay. Drawn with fixed-function OpenGL and a GDI bitmap font just before the swap; all
// GL state it touches is saved and restored.
typedef void(WINAPI *PFN_gl0)(void);
typedef void(WINAPI *PFN_glu)(unsigned);
typedef void(WINAPI *PFN_glf4)(float, float, float, float);
typedef void(WINAPI *PFN_glf2)(float, float);
typedef void(WINAPI *PFN_gli2)(int, int);
typedef void(WINAPI *PFN_glOrtho)(double, double, double, double, double, double);
typedef void(WINAPI *PFN_glCallLists)(int, unsigned, const void *);
typedef unsigned(WINAPI *PFN_glGenLists)(int);
typedef void(WINAPI *PFN_glViewportRaw)(int, int, int, int);
typedef BOOL(WINAPI *PFN_wglUseFontBitmapsA)(HDC, DWORD, DWORD, DWORD);
typedef void(WINAPI *PFN_glActiveTexture)(unsigned);
static struct {
    PFN_glu PushAttrib, MatrixMode, ListBase, Begin, Disable;
    PFN_gl0 PopAttrib, PushMatrix, PopMatrix, LoadIdentity, End;
    PFN_glf4 Color4f;
    PFN_glf2 Vertex2f;
    PFN_gli2 RasterPos2i;
    PFN_glOrtho Ortho;
    PFN_glCallLists CallLists;
    PFN_glGenLists GenLists;
    PFN_wglUseFontBitmapsA UseFontBitmapsA;
    PFN_glActiveTexture ActiveTexture;
    BOOL ok;
} gi;
static unsigned g_fontBase;
static int g_fontHeight;

static void IndicatorInit(void)
{
    HMODULE gl = GetModuleHandleA("opengl32.dll");
    if (!gl)
        return;
#define GLP(field, name) *(FARPROC *)&gi.field = GetProcAddress(gl, name)
    GLP(PushAttrib, "glPushAttrib"); GLP(PopAttrib, "glPopAttrib"); GLP(MatrixMode, "glMatrixMode");
    GLP(PushMatrix, "glPushMatrix"); GLP(PopMatrix, "glPopMatrix"); GLP(LoadIdentity, "glLoadIdentity");
    GLP(Ortho, "glOrtho"); GLP(Disable, "glDisable"); GLP(Color4f, "glColor4f"); GLP(Begin, "glBegin");
    GLP(End, "glEnd"); GLP(Vertex2f, "glVertex2f"); GLP(RasterPos2i, "glRasterPos2i"); GLP(ListBase, "glListBase");
    GLP(CallLists, "glCallLists"); GLP(GenLists, "glGenLists"); GLP(UseFontBitmapsA, "wglUseFontBitmapsA");
#undef GLP
    gi.ok = gi.PushAttrib && gi.PopAttrib && gi.MatrixMode && gi.PushMatrix && gi.PopMatrix && gi.LoadIdentity &&
            gi.Ortho && gi.Disable && gi.Color4f && gi.Begin && gi.End && gi.Vertex2f && gi.RasterPos2i &&
            gi.ListBase && gi.CallLists && gi.GenLists && gi.UseFontBitmapsA;
}

static void DrawSpeedIndicator(HDC dc, const Map *m)
{
    float factor = KeysSpeedActive();
    if (factor <= 0.0f || !KeysSpeedBadge() || *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) != 4)
        return;
    if (!gi.ok) {
        static BOOL tried;
        if (!tried) {
            tried = TRUE;
            IndicatorInit();
            typedef PROC(WINAPI * PFN_wglGetProcAddress)(LPCSTR);
            PFN_wglGetProcAddress gpa = (PFN_wglGetProcAddress)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglGetProcAddress");
            if (gpa)
                *(FARPROC *)&gi.ActiveTexture = (FARPROC)gpa("glActiveTextureARB");
        }
        if (!gi.ok)
            return;
    }
    int fh = (int)(18 * m->scale + 0.5);
    if (fh != g_fontHeight) { // (re)build the bitmap font for this output size
        HFONT font = CreateFontA(-fh, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 ANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial");
        HGDIOBJ old = SelectObject(dc, font);
        if (!g_fontBase)
            g_fontBase = gi.GenLists(96);
        gi.UseFontBitmapsA(dc, 32, 96, g_fontBase);
        SelectObject(dc, old);
        DeleteObject(font);
        g_fontHeight = fh;
    }
    char text[32];
    snprintf(text, sizeof text, "FAST x%g", factor);
    int len = (int)strlen(text);

    gi.PushAttrib(0x000FFFFF); // GL_ALL_ATTRIB_BITS (viewport, enables, colour, lists, ...)
    if (gi.ActiveTexture) {
        gi.ActiveTexture(0x84C1); // GL_TEXTURE1
        gi.Disable(0x0DE1);       // GL_TEXTURE_2D
        gi.ActiveTexture(0x84C0); // GL_TEXTURE0
    }
    gi.Disable(0x0DE1); gi.Disable(0x0B71 /* DEPTH_TEST */); gi.Disable(0x0B50 /* LIGHTING */);
    gi.Disable(0x0B60 /* FOG */); gi.Disable(0x0C11 /* SCISSOR_TEST */); gi.Disable(0x0BC0 /* ALPHA_TEST */);
    gi.Disable(0x0B44 /* CULL_FACE */);
    real_glViewport(0, 0, m->cw, m->ch);
    gi.MatrixMode(0x1701 /* PROJECTION */); gi.PushMatrix(); gi.LoadIdentity();
    gi.Ortho(0, m->cw, 0, m->ch, -1, 1);
    gi.MatrixMode(0x1700 /* MODELVIEW */); gi.PushMatrix(); gi.LoadIdentity();

    // Badge in the top-left of the game picture (GL origin is bottom-left).
    float pad = 6 * (float)m->scale, x0 = (float)m->offX + 12 * (float)m->scale;
    float y1 = (float)(m->ch - m->offY) - 12 * (float)m->scale, w = len * fh * 0.62f + 2 * pad, h = fh + 2 * pad;
    gi.Color4f(0.0f, 0.0f, 0.0f, 1.0f);
    gi.Begin(7 /* QUADS */);
    gi.Vertex2f(x0, y1 - h); gi.Vertex2f(x0 + w, y1 - h); gi.Vertex2f(x0 + w, y1); gi.Vertex2f(x0, y1);
    gi.End();
    gi.Color4f(1.0f, 0.82f, 0.25f, 1.0f); // gold, like the game's highlight text
    gi.RasterPos2i((int)(x0 + pad), (int)(y1 - pad - fh * 0.8f));
    gi.ListBase(g_fontBase - 32);
    gi.CallLists(len, 0x1401 /* UNSIGNED_BYTE */, text);

    gi.MatrixMode(0x1700); gi.PopMatrix();
    gi.MatrixMode(0x1701); gi.PopMatrix();
    gi.PopAttrib(); // restores the viewport, enables and current colour
}

static BOOL WINAPI Hook_SwapBuffers(HDC dc)
{
    ControllerFrame(); // once per frame, on the game thread; does nothing while the pad is idle
    Map m;
    if (g_cfg.captureEvery > 0 && GetMap(&m))
        CaptureFrame(&m);
    // With the menus centred (menuShift), only the 1024-wide layout is shown; the game's area right of it is not part
    // of the picture. Clear it too: after some cutscenes the game no longer clears its whole screen, and that strip
    // then shows the two back buffers' stale contents in turn (a flickering right bar, game-bugs K280).
    if (p_glClear && GetMap(&m) && (m.offX > 0 || m.offY > 0 || m.menuShift || m.offX + (int)(g_cfg.width * m.scale + 0.5) < m.cw)) {
        float old[4];
        unsigned char scissor = p_glIsEnabled(GL_SCISSOR_TEST);
        p_glGetFloatv(GL_COLOR_CLEAR_VALUE, old);
        p_glClearColor(0, 0, 0, 1);
        p_glEnable(GL_SCISSOR_TEST);
        int used_w = (int)((m.menuShift ? 1024 : g_cfg.width) * m.scale + 0.5), used_h = (int)(g_cfg.height * m.scale + 0.5);
        ClearRect(0, 0, m.offX, m.ch);                                   // left bar
        ClearRect(m.offX + used_w, 0, m.cw - m.offX - used_w, m.ch);     // right bar
        ClearRect(0, 0, m.cw, m.offY);                                   // bottom bar
        ClearRect(0, m.offY + used_h, m.cw, m.ch - m.offY - used_h);     // top bar
        if (!scissor)
            p_glDisable(GL_SCISSOR_TEST);
        p_glClearColor(old[0], old[1], old[2], old[3]);
    }
    if (GetMap(&m)) {
        GammaApply(g_wnd, m.cw, m.ch); // the options screen's brightness (K90), before the badge
        DrawSpeedIndicator(dc, &m);
    }
    if (g_captureWanted && GetMap(&m)) {
        SaveFrame(&m, g_capturePath);
        InterlockedExchange(&g_captureWanted, 0);
    }
    return real_SwapBuffers(dc);
}

// GrInit's windowed branch (mode 200, 0x505e00-0x50610f) sets a captioned window sized to the game
// resolution. Replace that with our output window.
static BOOL CallerIsGrInit(void *ret)
{
    DWORD va = OrigVa((DWORD)(UINT_PTR)ret);
    return va >= 0x505e00 && va < 0x506110;
}

static LONG OutputStyle(void)
{
    return g_cfg.borderless ? (LONG)(WS_POPUP | WS_VISIBLE) : (LONG)(WS_OVERLAPPEDWINDOW | WS_VISIBLE);
}

// [Display] Mode=Exclusive (2026-11-10): the borderless window with the display mode of the window's monitor set to
// ExclusiveWidth x ExclusiveHeight at ExclusiveRefresh Hz (0: the desktop's), given back while the game is not in
// front (it is minimised then), as the 64-bit build's host does (src/decomp/modern/display.cpp).
static int g_exclusiveWanted;  // a mode other than the desktop's was asked for and the display offers it
static int g_exclusiveSet;     // that mode is on now
static DEVMODEA g_exclusiveMode;
static char g_exclusiveDevice[CCHDEVICENAME];

static int ExclusiveOn(void)
{
    return g_cfg.exclusive && g_cfg.renderer == RENDERER_OPENGL;
}

static void ExclusiveLeave(void)
{
    if (!g_exclusiveSet)
        return;
    ChangeDisplaySettingsExA(g_exclusiveDevice, NULL, NULL, 0, NULL);
    g_exclusiveSet = 0;
}

static void ExclusiveEnter(void)
{
    if (!g_exclusiveWanted || g_exclusiveSet)
        return;
    LONG r = ChangeDisplaySettingsExA(g_exclusiveDevice, &g_exclusiveMode, NULL, CDS_FULLSCREEN, NULL);
    if (r != DISP_CHANGE_SUCCESSFUL) {
        Log("exclusive: mode %lux%lu failed (%ld); the desktop's mode stays", g_exclusiveMode.dmPelsWidth,
            g_exclusiveMode.dmPelsHeight, r);
        g_exclusiveWanted = 0;
        return;
    }
    g_exclusiveSet = 1;
    static int atExit;
    if (!atExit) {
        atExit = 1;
        atexit(ExclusiveLeave);
    }
}

static void ExclusiveChoose(HWND w)
{
    MONITORINFOEXA mi;
    mi.cbSize = sizeof mi;
    if (!GetMonitorInfoA(MonitorFromWindow(w, MONITOR_DEFAULTTOPRIMARY), (MONITORINFO *)&mi))
        return;
    lstrcpynA(g_exclusiveDevice, mi.szDevice, sizeof g_exclusiveDevice);
    DEVMODEA cur;
    ZeroMemory(&cur, sizeof cur);
    cur.dmSize = sizeof cur;
    if (!EnumDisplaySettingsA(g_exclusiveDevice, ENUM_CURRENT_SETTINGS, &cur))
        return;
    ZeroMemory(&g_exclusiveMode, sizeof g_exclusiveMode);
    g_exclusiveMode.dmSize = sizeof g_exclusiveMode;
    int sized = g_cfg.exclusiveWidth > 0 && g_cfg.exclusiveHeight > 0;
    g_exclusiveMode.dmPelsWidth = sized ? (DWORD)g_cfg.exclusiveWidth : cur.dmPelsWidth;
    g_exclusiveMode.dmPelsHeight = sized ? (DWORD)g_cfg.exclusiveHeight : cur.dmPelsHeight;
    g_exclusiveMode.dmDisplayFrequency = g_cfg.exclusiveRefresh > 0 ? (DWORD)g_cfg.exclusiveRefresh : cur.dmDisplayFrequency;
    g_exclusiveMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    if (g_exclusiveMode.dmPelsWidth == cur.dmPelsWidth && g_exclusiveMode.dmPelsHeight == cur.dmPelsHeight &&
        g_exclusiveMode.dmDisplayFrequency == cur.dmDisplayFrequency) {
        Log("exclusive: the desktop's mode %lux%lu at %lu Hz (no mode change)", cur.dmPelsWidth, cur.dmPelsHeight,
            cur.dmDisplayFrequency);
        return;
    }
    DWORD tried = g_exclusiveMode.dmDisplayFrequency;
    if (ChangeDisplaySettingsExA(g_exclusiveDevice, &g_exclusiveMode, NULL, CDS_TEST, NULL) != DISP_CHANGE_SUCCESSFUL) {
        g_exclusiveMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT; // the size at the display's default refresh
        if (ChangeDisplaySettingsExA(g_exclusiveDevice, &g_exclusiveMode, NULL, CDS_TEST, NULL) != DISP_CHANGE_SUCCESSFUL) {
            Log("exclusive: mode %lux%lu is not offered by %s; the desktop's mode stays", g_exclusiveMode.dmPelsWidth,
                g_exclusiveMode.dmPelsHeight, g_exclusiveDevice);
            return;
        }
        Log("exclusive: %lu Hz is not offered at %lux%lu; the display's default refresh", tried,
            g_exclusiveMode.dmPelsWidth, g_exclusiveMode.dmPelsHeight);
        g_exclusiveMode.dmDisplayFrequency = 0;
    }
    Log("exclusive: mode %lux%lu at %lu Hz on %s", g_exclusiveMode.dmPelsWidth, g_exclusiveMode.dmPelsHeight,
        g_exclusiveMode.dmDisplayFrequency, g_exclusiveDevice);
    g_exclusiveWanted = 1;
}

static LONG WINAPI Hook_SetWindowLongA(HWND w, int index, LONG value)
{
    if (CallerIsGrInit(__builtin_return_address(0))) {
        if (index == GWL_STYLE) {
            if (ExclusiveOn() && !g_wnd) {
                // Before GrInit's SetWindowPos, which takes the monitor's (new) rectangle.
                ExclusiveChoose(w);
                ExclusiveEnter();
            }
            g_wnd = w;
            Log("game window %p: style 0x%08lX -> 0x%08lX", (void *)w, value, OutputStyle());
            value = OutputStyle();
        } else if (index == GWL_EXSTYLE) {
            value = 0;
        }
    }
    return real_SetWindowLongA(w, index, value);
}

// The game's window procedure (0x530ff0) answers WM_ACTIVATE/WM_ACTIVATEAPP with
// SetWindowPos(main_wnd, HWND_TOPMOST or HWND_NOTOPMOST, 0, 0, screen w, screen h, 0): return addresses
// 0x5310b7 (gaining focus) and 0x5310ef (losing focus). Flags 0 means "activate", so the losing-focus call
// pulled the game straight back to the front and Alt+Tab to another program (e.g. SummonerFixConfig) failed.
#define VA_WNDPROC_SETPOS_ACTIVATE 0x5310b7u
#define VA_WNDPROC_SETPOS_DEACTIVATE 0x5310efu

static DWORD ReturnVa(void *ret)
{
    return OrigVa((DWORD)(UINT_PTR)ret);
}

static BOOL WINAPI Hook_SetWindowPos(HWND w, HWND after, int x, int y, int cx, int cy, UINT flags)
{
    DWORD from = ReturnVa(__builtin_return_address(0));
    if (from == VA_WNDPROC_SETPOS_DEACTIVATE) {
        if (ExclusiveOn() && w == g_wnd && g_wnd) {
            // Exclusive: the desktop's mode back and the window minimised, without activating anything (the
            // program being switched to is).
            ExclusiveLeave();
            ShowWindow(w, SW_SHOWMINNOACTIVE);
            Log("exclusive: focus lost, minimised");
            return TRUE;
        }
        // Losing focus: don't re-activate. Send the (screen-covering, borderless) window to the back so
        // the program being switched to is actually visible.
        return real_SetWindowPos(w, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (from == VA_WNDPROC_SETPOS_ACTIVATE) {
        flags |= SWP_NOACTIVATE; // already being activated; keep our geometry below
        if (ExclusiveOn() && w == g_wnd && g_wnd) {
            ExclusiveEnter();
            if (IsIconic(w))
                ShowWindow(w, SW_RESTORE);
            Log("exclusive: focus back");
        }
    }
    if (w == g_wnd && g_wnd && !(flags & SWP_NOSIZE)) {
        HMONITOR mon = MonitorFromWindow(w, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi;
        mi.cbSize = sizeof mi;
        GetMonitorInfoA(mon, &mi);
        if (g_cfg.borderless) {
            RECT r = mi.rcMonitor;
            x = r.left; y = r.top; cx = r.right - r.left; cy = r.bottom - r.top;
            after = HWND_TOP;
        } else {
            RECT wa = mi.rcWork;
            // A client size larger than the work area is shrunk, keeping its shape, so the window fits (the
            // default 1600x1200 is taller than a 1080p screen).
            RECT r = {0, 0, g_cfg.windowWidth, g_cfg.windowHeight};
            AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
            int fw = (r.right - r.left) - g_cfg.windowWidth, fh = (r.bottom - r.top) - g_cfg.windowHeight;
            if (g_cfg.windowWidth > 0 && g_cfg.windowHeight > 0) {
                double fx = (double)((wa.right - wa.left) - fw) / g_cfg.windowWidth;
                double fy = (double)((wa.bottom - wa.top) - fh) / g_cfg.windowHeight;
                double fit = fx < fy ? fx : fy;
                if (fit < 1.0 && fit > 0.0) {
                    SetRect(&r, 0, 0, (int)(g_cfg.windowWidth * fit + 0.5), (int)(g_cfg.windowHeight * fit + 0.5));
                    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
                }
            }
            cx = r.right - r.left; cy = r.bottom - r.top;
            x = wa.left + ((wa.right - wa.left) - cx) / 2;
            y = wa.top + ((wa.bottom - wa.top) - cy) / 2;
            if (y < wa.top) y = wa.top;
            if (x < wa.left) x = wa.left;
        }
        flags &= ~SWP_NOMOVE;
        Log("game window -> %s %dx%d at %d,%d", g_cfg.borderless ? "borderless" : "window", cx, cy, x, y);
    }
    return real_SetWindowPos(w, after, x, y, cx, cy, flags);
}

// Mouse: the game polls GetCursorPos + ScreenToClient (0x500b3f/0x500b50) and warps the cursor
// with ClientToScreen + SetCursorPos (0x500ca7/0x500cb7). Convert between window and game pixels.
static BOOL WINAPI Hook_ScreenToClient(HWND w, LPPOINT p)
{
    POINT screen = *p;
    BOOL ok = real_ScreenToClient(w, p);
    Map m;
    // Controller cursor (mode 2): the game's cursor poll sees the real cursor somewhere else, so the real mouse has
    // moved. It takes over again.
    if (ok && w == g_wnd && g_agentCursor == 2) {
        DWORD va = OrigVa((DWORD)(UINT_PTR)__builtin_return_address(0));
        if (va >= 0x500b00 && va < 0x500d00 && (abs(screen.x - g_padRealX) > 1 || abs(screen.y - g_padRealY) > 1))
            InterlockedExchange(&g_agentCursor, 0);
    }
    if (ok && w == g_wnd && g_agentCursor) {
        p->x = g_agentX;
        p->y = g_agentY;
    }
    if (ok && w == g_wnd && GetMap(&m)) {
        int gx = (int)((p->x - m.offX) / m.scale), gy = (int)((p->y - m.offY) / m.scale);
        p->x = gx < 0 ? 0 : gx >= g_cfg.width ? g_cfg.width - 1 : gx;
        p->y = gy < 0 ? 0 : gy >= g_cfg.height ? g_cfg.height - 1 : gy;
    }
    return ok;
}

static BOOL WINAPI Hook_ClientToScreen(HWND w, LPPOINT p)
{
    Map m;
    DWORD va = OrigVa((DWORD)(UINT_PTR)__builtin_return_address(0));
    if (w == g_wnd && va >= 0x500b00 && va < 0x500d00 && GetMap(&m)) {
        p->x = m.offX + Round(p->x * m.scale);
        p->y = m.offY + Round(p->y * m.scale);
        if (g_agentCursor) { // the game warps the cursor (mouse look): the agent cursor follows
            InterlockedExchange(&g_agentX, p->x);
            InterlockedExchange(&g_agentY, p->y);
        }
        BOOL ok = real_ClientToScreen(w, p);
        if (ok && g_agentCursor == 2) { // the game's SetCursorPos puts the real cursor here: not a mouse movement
            InterlockedExchange(&g_padRealX, p->x);
            InterlockedExchange(&g_padRealY, p->y);
        }
        return ok;
    }
    return real_ClientToScreen(w, p);
}

// GrInit's windowed branch derives the 3D aspect factor from the desktop size
// (GetSystemMetrics(SM_CXSCREEN)/(SM_CYSCREEN)). We show the game's own aspect, so report that.
static int WINAPI Hook_GetSystemMetrics(int index)
{
    if (CallerIsGrInit(__builtin_return_address(0))) {
        if (index == SM_CXSCREEN) return g_cfg.width;
        if (index == SM_CYSCREEN) return g_cfg.height;
    }
    return real_GetSystemMetrics(index);
}

// [Display] Antialiasing=2|4|8 (2026-11-10): a multisampled pixel format for the game's window, as the 64-bit build's
// host does it (src/decomp/modern/display.cpp, find_multisample_format). gr_opengl_set_pixel_format (0x54f680) asks
// ChoosePixelFormat for double-buffered RGBA with 16-bit colour and depth and sets the answer with SetPixelFormat; this
// hook answers with a WGL_ARB_multisample format instead. Nothing else changes: the default framebuffer is resolved
// whenever it is read (glReadPixels for the thumbnail and captures, glCopyTexSubImage2D for the brightness pass), and
// the bar clears and the badge draw into it like any other drawing. SummonerFix never binds a framebuffer object.
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
typedef int(WINAPI *PFN_ChoosePixelFormat)(HDC, const PIXELFORMATDESCRIPTOR *);
typedef BOOL(WINAPI *PFN_wglChoosePixelFormatARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef HGLRC(WINAPI *PFN_wglCreateContext)(HDC);
typedef BOOL(WINAPI *PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL(WINAPI *PFN_wglDeleteContext)(HGLRC);
typedef HDC(WINAPI *PFN_wglGetCurrentDC)(void);
typedef HGLRC(WINAPI *PFN_wglGetCurrentContext)(void);
typedef PROC(WINAPI *PFN_wglGetProcAddressA)(LPCSTR);
static PFN_ChoosePixelFormat real_ChoosePixelFormat;
static HMODULE g_opengl;
static int g_aaFormat; // the multisampled format found, 0 = none
static BOOL g_aaTried;

// wglChoosePixelFormatARB needs a current context, and a window's pixel format can be set only once: a hidden window
// gets a plain format (gdi32's own ChoosePixelFormat, not the hooked import of Sum.exe) and a throwaway context to
// ask through, and the format found is for the game's DC.
static int FindMultisampleFormat(HDC dc, int want)
{
    PFN_wglCreateContext pCreate = NULL;
    PFN_wglMakeCurrent pMake = NULL;
    PFN_wglDeleteContext pDelete = NULL;
    PFN_wglGetCurrentDC pCurDc = NULL;
    PFN_wglGetCurrentContext pCurRc = NULL;
    PFN_wglGetProcAddressA pGpa = NULL;
    if (g_opengl) {
        pCreate = (PFN_wglCreateContext)(void *)GetProcAddress(g_opengl, "wglCreateContext");
        pMake = (PFN_wglMakeCurrent)(void *)GetProcAddress(g_opengl, "wglMakeCurrent");
        pDelete = (PFN_wglDeleteContext)(void *)GetProcAddress(g_opengl, "wglDeleteContext");
        pCurDc = (PFN_wglGetCurrentDC)(void *)GetProcAddress(g_opengl, "wglGetCurrentDC");
        pCurRc = (PFN_wglGetCurrentContext)(void *)GetProcAddress(g_opengl, "wglGetCurrentContext");
        pGpa = (PFN_wglGetProcAddressA)(void *)GetProcAddress(g_opengl, "wglGetProcAddress");
    }
    if (!pCreate || !pMake || !pDelete || !pCurDc || !pCurRc || !pGpa) {
        Log("antialiasing: opengl32.dll's wgl functions missing; drawing without");
        return 0;
    }
    HINSTANCE inst = GetModuleHandleA(NULL);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = inst;
    wc.lpszClassName = "SummonerFixPixelFormatProbe";
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
            rc = pCreate(pdc);
        HDC oldDc = pCurDc();
        HGLRC oldRc = pCurRc();
        if (rc && pMake(pdc, rc)) {
            PFN_wglChoosePixelFormatARB choose = (PFN_wglChoosePixelFormatARB)(void *)pGpa("wglChoosePixelFormatARB");
            // The original's request (double-buffered RGBA, 16-bit colour and depth at least), accelerated,
            // multisampled; fewer samples when the driver has no format with as many.
            for (int n = want; choose && n >= 2 && !found; n /= 2) {
                const int attrs[] = {WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1,
                                     WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, WGL_ACCELERATION_ARB,
                                     WGL_FULL_ACCELERATION_ARB, WGL_COLOR_BITS_ARB, 16, WGL_DEPTH_BITS_ARB, 16,
                                     WGL_SAMPLE_BUFFERS_ARB, 1, WGL_SAMPLES_ARB, n, 0};
                int fmt = 0;
                UINT count = 0;
                if (choose(dc, attrs, NULL, 1, &fmt, &count) && count > 0 && fmt > 0) {
                    found = fmt;
                    samples = n;
                }
            }
            if (!choose)
                Log("antialiasing: the driver has no wglChoosePixelFormatARB");
        }
        pMake(oldDc, oldRc);
        if (rc)
            pDelete(rc);
        ReleaseDC(probe, pdc);
    }
    if (probe)
        DestroyWindow(probe);
    UnregisterClassA(wc.lpszClassName, inst);
    if (found) {
        PIXELFORMATDESCRIPTOR got;
        DescribePixelFormat(dc, found, sizeof got, &got);
        Log("antialiasing: pixel format %d, %d samples (%d asked for), colour %d, depth %d bits", found, samples,
            want, got.cColorBits, got.cDepthBits);
    } else {
        Log("antialiasing: no multisampled pixel format; drawing without");
    }
    return found;
}

static int WINAPI Hook_ChoosePixelFormat(HDC dc, const PIXELFORMATDESCRIPTOR *pfd)
{
    if (g_cfg.antialiasing > 1) {
        // The same format each time: gr_opengl_init_context sets it again when the context is made again.
        if (!g_aaTried) {
            g_aaTried = TRUE;
            int want = g_cfg.antialiasing >= 8 ? 8 : g_cfg.antialiasing >= 4 ? 4 : 2;
            g_aaFormat = FindMultisampleFormat(dc, want);
        }
        if (g_aaFormat)
            return g_aaFormat;
    }
    return real_ChoosePixelFormat(dc, pfd);
}

void DisplayInstall(HMODULE exe)
{
    // These hooks recognise their callers by return address: the hybrid game must leave those functions original.
    KeepOriginal(0x505e00, 0x310); // GrInit's windowed branch (CallerIsGrInit)
    KeepOriginal(0x500b00, 0x200); // the mouse poll and warp
    KeepOriginal(VA_WNDPROC_SETPOS_ACTIVATE, 1);
    KeepOriginal(VA_WNDPROC_SETPOS_DEACTIVATE, 1);
    HMODULE gl = LoadLibraryA("opengl32.dll");
    g_opengl = gl;
    p_glScissor = (PFN_glScissor)GetProcAddress(gl, "glScissor");
    p_glClear = (PFN_glClear)GetProcAddress(gl, "glClear");
    p_glClearColor = (PFN_glClearColor)GetProcAddress(gl, "glClearColor");
    p_glGetFloatv = (PFN_glGetFloatv)GetProcAddress(gl, "glGetFloatv");
    p_glEnable = (PFN_glCap)GetProcAddress(gl, "glEnable");
    p_glDisable = (PFN_glCap)GetProcAddress(gl, "glDisable");
    p_glIsEnabled = (PFN_glIsEnabled)GetProcAddress(gl, "glIsEnabled");
    p_glReadPixels = (PFN_glReadPixels)GetProcAddress(gl, "glReadPixels");
    p_glGetIntegerv = (PFN_glGetIntegerv)GetProcAddress(gl, "glGetIntegerv");
    p_glPixelStorei = (PFN_glPixelStorei)GetProcAddress(gl, "glPixelStorei");
    p_glReadBuffer = (PFN_glReadBuffer)GetProcAddress(gl, "glReadBuffer");
    if (!p_glGetIntegerv || !p_glPixelStorei || !p_glReadBuffer)
        p_glReadPixels = NULL;
    if (!p_glScissor || !p_glClear || !p_glClearColor || !p_glGetFloatv || !p_glEnable || !p_glDisable ||
        !p_glIsEnabled) {
        Log("FAIL display: OpenGL entry points missing; bars will not be cleared");
        p_glClear = NULL;
    }
    real_glViewport = HookImport(exe, "OPENGL32.dll", "glViewport", Hook_glViewport);
    real_glReadPixels = (PFN_glReadPixels)HookImport(exe, "OPENGL32.dll", "glReadPixels", (void *)Hook_glReadPixels);
    real_SwapBuffers = HookImport(exe, "GDI32.dll", "SwapBuffers", Hook_SwapBuffers);
    if (g_cfg.antialiasing > 1)
        real_ChoosePixelFormat = (PFN_ChoosePixelFormat)HookImport(exe, "GDI32.dll", "ChoosePixelFormat",
                                                                    (void *)Hook_ChoosePixelFormat);
    real_SetWindowLongA = HookImport(exe, "USER32.dll", "SetWindowLongA", Hook_SetWindowLongA);
    real_SetWindowPos = HookImport(exe, "USER32.dll", "SetWindowPos", Hook_SetWindowPos);
    real_ScreenToClient = HookImport(exe, "USER32.dll", "ScreenToClient", Hook_ScreenToClient);
    real_ClientToScreen = HookImport(exe, "USER32.dll", "ClientToScreen", Hook_ClientToScreen);
    real_GetSystemMetrics = HookImport(exe, "USER32.dll", "GetSystemMetrics", Hook_GetSystemMetrics);
}
