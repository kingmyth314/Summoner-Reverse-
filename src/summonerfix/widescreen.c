// Widescreen in-game UI: with GameWidth > 1024 the 3D world uses the full width, but the HUD and the
// in-game screens are a fixed 1024-wide layout drawn at the left. During gameplay (game state 4) we
// shift that layout to the centre by X = (GameWidth - 1024) / 2 game pixels:
//
//  - Calls from UI code (game UI 0x401000-0x430000, vsdk ui) to the 2D primitives are redirected to
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

static int Active(void)
{
    return g_shift > 0 && *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) == 4;
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

static int InUi(DWORD va)
{
    if (va >= 0x412220 && va < 0x412620) // interface_set_view_clip: the 3D view, not UI
        return 0;
    return (va >= 0x401000 && va < 0x430000)    // game UI (new_*.cpp)
           || (va >= 0x527000 && va < 0x530c00) // vsdk ui (window.cpp)
           || (va >= 0x552000 && va < 0x554b60) // vsdk ui (ui_container.cpp)
           || (va >= 0x55e000 && va < 0x560000); // vsdk ui gadgets
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
        "world mouse reads %d/9",
        g_shift, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7], other,
        counts[8], counts[9], counts[10], raw);
}
