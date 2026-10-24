// Widescreen in-game UI: with GameWidth > 1024 the 3D world uses the full width, but the HUD and the
// in-game screens are a fixed 1024-wide layout drawn at the left. During gameplay (game state 4) we
// shift that layout to the centre by X = (GameWidth - 1024) / 2 game pixels:
//
//  - Calls from UI code (game UI 0x401000-0x430000, vsdk ui, conversation text widgets 0x4d6600-0x4d7ab0) to the 2D primitives are redirected to
//    wrappers that add X to the x coordinate.
//  - Every 2D draw adds the clip origin gr_screen.offset_x (set by gr::set_clip). UI code that sets its
//    own clip (panels, list boxes) gets X added to the clip instead, and draws inside a shifted clip are
//    not shifted again. All set_clip call sites are wrapped so we know whether the current clip is ours.
//  - UI code sees gr_screen_width() = 1024 and a clip origin without X (gr_get_clip), so its own layout
//    arithmetic stays in 1024 space.
//  - The mouse: GameTick (0x4010ef) copies the cursor position into input_x/input_y (0x5fbfdc/0x5fbfe0),
//    which UI code reads ~375 times. We hand it x - X there, and point the 9 reads by world code
//    (camera edge scroll, clicks into the world) at the raw position. The cursor itself is drawn by
//    FrameEnd from the raw position, so it is unaffected.
//  - interface_set_view_clip (0x412220), which sizes the 3D view, is excluded.
//
// Three cases during gameplay, by interface screen (interface_cur_screen, 0x5bd814):
//  - Full-screen panels (inventory, character, journal, options, save/load, ...): the world and HUD are
//    hidden, but item and character models are rendered in screen coordinates by the 3D renderer, which
//    call shifting cannot reach. Instead the whole output is shifted, exactly like the menus (display.c
//    GetMap via WidescreenFullPanel), and no call shifting is applied.
//  - Side panels (map, spells, skills, item trade): the panel stays flush left as in the original, the 3D
//    view uses the rest of the width; only the HUD is centred. Their draw (interface_draw, 0x411cd0) and
//    frame (interface_frame, 0x4119a0) calls run unshifted with the raw mouse position.
//  - Everything else (HUD only, pickup, play menu, assess, ...): call shifting as described above.
// See docs/sum-exe/interface.md and the roadmap's widescreen section.
#include "summonerfix.h"
#include <string.h>

#define VA_GAMESEQ_STATE_CUR 0x5b27e0u
#define VA_GR_BITMAP 0x506f90u
#define VA_GR_BITMAP_EX 0x506cb0u
#define VA_GR_BITMAP_STRETCHED 0x506ec0u
#define VA_GR_BITMAP_FLOAT 0x507100u
#define VA_GR_STRING 0x508c60u
#define VA_GR_RECT 0x5077d0u
#define VA_GR_LINE 0x507310u
#define VA_GR_SET_CLIP 0x506840u
#define VA_GR_GET_CLIP 0x506970u
#define VA_GR_SCREEN_WIDTH 0x506170u
#define VA_MOUSE_GET_POS 0x5011e0u
#define VA_GAMETICK_MOUSE_CALL 0x4010efu
#define VA_INPUT_X 0x5fbfdcu
#define VA_INPUT_Y 0x5fbfe0u
#define LAYOUT_WIDTH 1024

typedef int(__cdecl *F10)(int, int, int, int, int, int, int, int, int, int);

static F10 o_bitmap, o_bitmapEx, o_bitmapStretched, o_bitmapFloat, o_string, o_rect, o_line, o_setClip;
static void(__cdecl *o_getClip)(int *, int *, int *, int *);
static int(__cdecl *o_screenWidth)(void);
static void(__cdecl *o_mouseGetPos)(int *, int *, int *);
static int g_shift;          // X in game pixels
static int g_clipShifted;    // the current clip was set by UI code and already includes X
static int g_rawX, g_rawY;   // cursor position in game pixels, unshifted (read by world code)

#define VA_INTERFACE_CUR_SCREEN 0x5bd814u
#define VA_INTERFACE_DRAW 0x411cd0u
#define VA_INTERFACE_DRAW_END 0x411ea0u
#define VA_INTERFACE_FRAME 0x4119a0u
#define VA_INTERFACE_FRAME_END 0x411be0u
static volatile int g_noShift; // inside a side panel's draw/frame call

static int InGame(void) { return *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) == 4; }
static int Screen(void) { return *(volatile int *)(UINT_PTR)SumVa(VA_INTERFACE_CUR_SCREEN); }
static int IsCallShiftScreen(int s) { return s == 0 || s == 4 || s == 9 || s == 13 || s == 20; }
static int IsSidePanel(int s) { return s == 3 || s == 5 || s == 6 || s == 19; }

int WidescreenFullPanel(void)
{
    if (g_shift <= 0 || !InGame())
        return 0;
    int s = Screen();
    return !IsCallShiftScreen(s) && !IsSidePanel(s);
}

static int Active(void)
{
    return g_shift > 0 && !g_noShift && InGame() && !WidescreenFullPanel();
}

static int ShiftDraw(void) { return Active() && !g_clipShifted; }

static int AddF(int bits, int add)
{
    float f;
    memcpy(&f, &bits, 4);
    f += (float)add;
    memcpy(&bits, &f, 4);
    return bits;
}

#define ARGS int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10
#define PASS a1, a2, a3, a4, a5, a6, a7, a8, a9, a10

static int __cdecl W_bitmap(ARGS) { if (ShiftDraw()) a2 += g_shift; return o_bitmap(PASS); }
static int __cdecl W_bitmapEx(ARGS) { if (ShiftDraw()) a2 += g_shift; return o_bitmapEx(PASS); }
static int __cdecl W_bitmapStretched(ARGS) { if (ShiftDraw()) a2 += g_shift; return o_bitmapStretched(PASS); }
static int __cdecl W_bitmapFloat(ARGS) { if (ShiftDraw()) a2 = AddF(a2, g_shift); return o_bitmapFloat(PASS); }
static int __cdecl W_string(ARGS) { if (ShiftDraw() && a1 != 0x8000) a1 += g_shift; return o_string(PASS); }
static int __cdecl W_rect(ARGS) { if (ShiftDraw()) a1 += g_shift; return o_rect(PASS); }
static int __cdecl W_line(ARGS)
{
    if (ShiftDraw()) {
        a1 = AddF(a1, g_shift);
        a3 = AddF(a3, g_shift);
    }
    return o_line(PASS);
}

// set_clip from UI code: shift the clip origin and remember that the clip includes X.
static int __cdecl W_setClipUi(ARGS)
{
    if (Active()) {
        a1 += g_shift;
        g_clipShifted = 1;
    } else {
        g_clipShifted = 0;
    }
    return o_setClip(PASS);
}

// set_clip from anywhere else (3D view, reset_clip, gr_init): an unshifted clip.
static int __cdecl W_setClipOther(ARGS)
{
    g_clipShifted = 0;
    return o_setClip(PASS);
}

static void __cdecl W_getClipUi(int *x, int *y, int *w, int *h)
{
    o_getClip(x, y, w, h);
    if (Active() && g_clipShifted && x)
        *x -= g_shift;
}

static int __cdecl W_screenWidthUi(void)
{
    return Active() ? LAYOUT_WIDTH : o_screenWidth();
}

// UI code asking for the cursor position: UI space.
static void __cdecl W_mouseGetPosUi(int *x, int *y, int *z)
{
    o_mouseGetPos(x, y, z);
    if (x && Active())
        *x -= g_shift;
}

// GameTick's per-frame copy into input_x/input_y: keep the raw position for world code, UI space for UI.
static void __cdecl W_mouseGetPosTick(int *x, int *y, int *z)
{
    o_mouseGetPos(x, y, z);
    if (x)
        g_rawX = *x;
    if (y)
        g_rawY = *y;
    if (x && Active())
        *x -= g_shift;
}

// Side panels: their draw and frame functions run unshifted, with the raw cursor in input_x.
typedef int(__cdecl *PFN_Frame)(int key);
static const DWORD k_panelDraw[] = {0x403690, 0x42ab60, 0x428c30, 0x41a230};  // map, spells, skills, item trade
static const DWORD k_panelFrame[] = {0x402680, 0x42a190, 0x427e10, 0x419560};
static void(__cdecl *o_panelDraw[4])(void);
static PFN_Frame o_panelFrame[4];

#define PANEL_DRAW(i) static void __cdecl W_panelDraw##i(void) { g_noShift++; o_panelDraw[i](); g_noShift--; }
#define PANEL_FRAME(i) static int __cdecl W_panelFrame##i(int key) {         volatile int *ix = (volatile int *)(UINT_PTR)SumVa(VA_INPUT_X); int saved = *ix;         if (g_shift > 0 && InGame()) *ix = g_rawX;         g_noShift++; int r = o_panelFrame[i](key); g_noShift--; *ix = saved; return r; }
PANEL_DRAW(0) PANEL_DRAW(1) PANEL_DRAW(2) PANEL_DRAW(3)
PANEL_FRAME(0) PANEL_FRAME(1) PANEL_FRAME(2) PANEL_FRAME(3)
static void *const k_panelDrawW[4] = {W_panelDraw0, W_panelDraw1, W_panelDraw2, W_panelDraw3};
static void *const k_panelFrameW[4] = {W_panelFrame0, W_panelFrame1, W_panelFrame2, W_panelFrame3};

// interface_set_view_clip (0x412220-0x41236x) sizes the 3D view per screen:
//  - its gr_screen_width() calls: report 1024 while a full-screen panel is open, so models drawn inside
//    the panel (inventory character, character portrait) are projected around the layout centre (512)
//    rather than the wide screen's centre;
//  - side panels: the per-panel width it starts the 3D view at is replaced by 0 ([Display]
//    SidePanelsOverWorld=1), so the world keeps its full width behind the panel and does not move.
static const DWORD k_viewWidthCalls[] = {0x412233, 0x412284, 0x4122ab, 0x4122da, 0x412300, 0x412326, 0x41234d};
static const DWORD k_panelWidthCalls[] = {0x4122a2, 0x4122d1, 0x4122f7, 0x41231d};
static const DWORD k_panelWidthFns[] = {0x404460, 0x42ada0, 0x429060, 0x41a8b0};
static int(__cdecl *o_panelWidth[4])(void);

// The 3D projection scale is the view width divided by gr_screen.aspect ((w / h) * 0.75, 1.33 at 1366x768).
// With a 1024-wide view for a full panel that made the panel's models 25% too small, so the 4:3 aspect (1.0)
// is used while a full panel is open and the wide one restored afterwards.
#define VA_GR_SCREEN_ASPECT 0x25d8d48u
static float g_wideAspect;

static int __cdecl W_viewWidth(void)
{
    volatile float *aspect = (volatile float *)(UINT_PTR)SumVa(VA_GR_SCREEN_ASPECT);
    int full = WidescreenFullPanel();
    if (!g_wideAspect && !full)
        g_wideAspect = *aspect;
    if (g_wideAspect)
        *aspect = full ? 1.0f : g_wideAspect;
    return full ? LAYOUT_WIDTH : o_screenWidth();
}

#define PANEL_WIDTH(i) static int __cdecl W_panelWidth##i(void) {         int r = o_panelWidth[i](); return (g_cfg.sidePanelsOverWorld && g_shift > 0 && InGame()) ? 0 : r; }
PANEL_WIDTH(0) PANEL_WIDTH(1) PANEL_WIDTH(2) PANEL_WIDTH(3)
static void *const k_panelWidthW[4] = {W_panelWidth0, W_panelWidth1, W_panelWidth2, W_panelWidth3};

static int InUi(DWORD va)
{
    if (va >= 0x412220 && va < 0x412620) // interface_set_view_clip: the 3D view, not UI
        return 0;
    return (va >= 0x401000 && va < 0x430000)    // game UI (new_*.cpp)
           || (va >= 0x527000 && va < 0x530c00) // vsdk ui (window.cpp)
           || (va >= 0x552000 && va < 0x554b60) // vsdk ui (ui_container.cpp)
           || (va >= 0x55e000 && va < 0x560000)  // vsdk ui gadgets
           || (va >= 0x4d6600 && va < 0x4d7ab0); // conversation text widgets (dialogue text and choices)
}

static BOOL PatchCall(DWORD siteVa, DWORD targetVa, void *wrapper)
{
    DWORD site = SumVa(siteVa);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD rel = SumVa(targetVa) - (site + 5);
    memcpy(orig + 1, &rel, 4);
    rel = (DWORD)(UINT_PTR)wrapper - (site + 5);
    memcpy(repl + 1, &rel, 4);
    return PatchBytes(siteVa, orig, repl, 5, NULL);
}

void WidescreenInstall(void)
{
    if (!g_cfg.centerHud || g_cfg.width <= LAYOUT_WIDTH)
        return;
    if (g_rebuilt) {
        // The experimental build centres its interface in its own source (experimental/widescreen.cpp, the
        // "CenterHud" key in its .fixes file); only the full-screen panels are shifted here, by display.c through
        // WidescreenFullPanel, which needs g_shift. No call is patched. Other rebuilt exes: nothing.
        if (RebuiltFixInSource("CenterHud")) {
            g_shift = (g_cfg.width - LAYOUT_WIDTH) / 2;
            Log("widescreen UI: in this exe's source (EXPERIMENTAL_BUILD); full-screen panels shifted here (%d px)", g_shift);
        } else {
            Log("widescreen UI: not in this rebuilt exe; the interface stays at the left");
        }
        return;
    }
    g_shift = (g_cfg.width - LAYOUT_WIDTH) / 2;
    o_bitmap = (F10)(UINT_PTR)SumVa(VA_GR_BITMAP);
    o_bitmapEx = (F10)(UINT_PTR)SumVa(VA_GR_BITMAP_EX);
    o_bitmapStretched = (F10)(UINT_PTR)SumVa(VA_GR_BITMAP_STRETCHED);
    o_bitmapFloat = (F10)(UINT_PTR)SumVa(VA_GR_BITMAP_FLOAT);
    o_string = (F10)(UINT_PTR)SumVa(VA_GR_STRING);
    o_rect = (F10)(UINT_PTR)SumVa(VA_GR_RECT);
    o_line = (F10)(UINT_PTR)SumVa(VA_GR_LINE);
    o_setClip = (F10)(UINT_PTR)SumVa(VA_GR_SET_CLIP);
    o_getClip = (void(__cdecl *)(int *, int *, int *, int *))(UINT_PTR)SumVa(VA_GR_GET_CLIP);
    o_screenWidth = (int(__cdecl *)(void))(UINT_PTR)SumVa(VA_GR_SCREEN_WIDTH);
    o_mouseGetPos = (void(__cdecl *)(int *, int *, int *))(UINT_PTR)SumVa(VA_MOUSE_GET_POS);

    static const struct { DWORD target; void *ui; void *other; } k[] = {
        {VA_GR_BITMAP, W_bitmap, NULL},
        {VA_GR_BITMAP_EX, W_bitmapEx, NULL},
        {VA_GR_BITMAP_STRETCHED, W_bitmapStretched, NULL},
        {VA_GR_BITMAP_FLOAT, W_bitmapFloat, NULL},
        {VA_GR_STRING, W_string, NULL},
        {VA_GR_RECT, W_rect, NULL},
        {VA_GR_LINE, W_line, NULL},
        {VA_GR_SET_CLIP, W_setClipUi, W_setClipOther},
        {VA_GR_GET_CLIP, W_getClipUi, NULL},
        {VA_GR_SCREEN_WIDTH, W_screenWidthUi, NULL},
        {VA_MOUSE_GET_POS, W_mouseGetPosUi, NULL},
    };
    int counts[sizeof k / sizeof k[0]] = {0}, other = 0;
    const BYTE *base = (const BYTE *)(UINT_PTR)SumVa(SUM_IMAGE_BASE);
    for (DWORD va = 0x401000; va < 0x56c740 - 5; va++) {
        const BYTE *p = base + (va - SUM_IMAGE_BASE);
        if (*p != 0xE8)
            continue;
        DWORD target;
        memcpy(&target, p + 1, 4);
        target += va + 5;
        if (va == VA_GAMETICK_MOUSE_CALL)
            continue; // handled separately below
        for (size_t i = 0; i < sizeof k / sizeof k[0]; i++) {
            if (target != k[i].target)
                continue;
            void *w = InUi(va) ? k[i].ui : k[i].other;
            if (w && PatchCall(va, target, w)) {
                if (w == k[i].ui)
                    counts[i]++;
                else
                    other++;
            }
            break;
        }
    }
    PatchCall(VA_GAMETICK_MOUSE_CALL, VA_MOUSE_GET_POS, W_mouseGetPosTick);

    // interface_set_view_clip: 3D view width for full panels, world behind side panels.
    int view = 0;
    for (size_t i = 0; i < sizeof k_viewWidthCalls / sizeof k_viewWidthCalls[0]; i++)
        view += PatchCall(k_viewWidthCalls[i], VA_GR_SCREEN_WIDTH, W_viewWidth);
    for (int i = 0; i < 4; i++) {
        o_panelWidth[i] = (int(__cdecl *)(void))(UINT_PTR)SumVa(k_panelWidthFns[i]);
        view += PatchCall(k_panelWidthCalls[i], k_panelWidthFns[i], k_panelWidthW[i]);
    }

    // Side panels: the per-screen calls in interface_draw / interface_frame.
    int panels = 0;
    for (int i = 0; i < 4; i++) {
        o_panelDraw[i] = (void(__cdecl *)(void))(UINT_PTR)SumVa(k_panelDraw[i]);
        o_panelFrame[i] = (PFN_Frame)(UINT_PTR)SumVa(k_panelFrame[i]);
    }
    for (DWORD va = VA_INTERFACE_FRAME; va < VA_INTERFACE_DRAW_END; va++) {
        int inDraw = va >= VA_INTERFACE_DRAW, inFrame = va < VA_INTERFACE_FRAME_END;
        if (!inDraw && !inFrame)
            continue;
        const BYTE *p = base + (va - SUM_IMAGE_BASE);
        if (*p != 0xE8)
            continue;
        DWORD target;
        memcpy(&target, p + 1, 4);
        target += va + 5;
        for (int i = 0; i < 4; i++) {
            if (inDraw && target == k_panelDraw[i])
                panels += PatchCall(va, target, k_panelDrawW[i]);
            if (inFrame && target == k_panelFrame[i])
                panels += PatchCall(va, target, k_panelFrameW[i]);
        }
    }

    // World code reads of input_x/input_y -> the raw position (operand addresses, 4 bytes each).
    static const DWORD kRawX[] = {0x434500, 0x4346de, 0x4346ff, 0x434753, 0x4347cd, 0x4347db, 0x4e0220};
    static const DWORD kRawY[] = {0x434508, 0x4e0215};
    int raw = 0;
    for (size_t i = 0; i < sizeof kRawX / sizeof kRawX[0]; i++) {
        DWORD from = VA_INPUT_X, to = (DWORD)(UINT_PTR)&g_rawX;
        raw += PatchBytes(kRawX[i], &from, &to, 4, NULL);
    }
    for (size_t i = 0; i < sizeof kRawY / sizeof kRawY[0]; i++) {
        DWORD from = VA_INPUT_Y, to = (DWORD)(UINT_PTR)&g_rawY;
        raw += PatchBytes(kRawY[i], &from, &to, 4, NULL);
    }
    Log("widescreen UI: shift %d px; UI calls patched: bitmap %d, bitmap_ex %d, stretched %d, float %d, "
        "string %d, rect %d, line %d, set_clip %d (+%d other), get_clip %d, screen_width %d, mouse %d; "
        "world mouse reads %d/9; side-panel calls %d/8; view-clip calls %d/11",
        g_shift, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7], other,
        counts[8], counts[9], counts[10], raw, panels, view);
}
