// Optional gameplay settings from [Game] in summonerfix.ini. Each is off (the original behaviour)
// unless set. They cover the remaining options of Timeslip's patch, reimplemented from our own analysis.
//
//   CameraZoomMax=3     camera zoom-out limit; the game clamps camera +0x184 to [0.3, camera_zoom_max]
//                       with camera_zoom_max = 1.0 (0x59b800). The zoom_limit console command sets 1000.
//   SkipIntroMovies=1   clears play_intro_movies (0x5930bd), which mainmenu_init checks once before playing
//                       vlogo.bik and eaxlogo.bik. The credits movie still plays.
//   InteractRange=8     hud_interact_nearest (0x40e5..) compares the distance with 5.0 (`FCOM [0x57ef18]`
//                       at 0x40e694); the operand is pointed at our own float.
//   ContainerRange=8    world_mouse_frame compares the squared distance with 25.0 (`FCOMP [0x580e08]` at
//                       0x43ad36); the operand is pointed at our own float (range squared).
// 5.0 and 25.0 are shared constants used by many other sites, so only these operands are changed.
//
// Print Screen above 1024 rows (game-bugs C290): jpeg_write_file (0x5565e0) keeps one row pointer per line in a
// 1024-entry stack table, so a JPEG screenshot taller than 1024 lines overwrites its return address. The retail game
// never runs above 768, but GameHeight can. With GameHeight > 1024 Print_screen_as_targa (0x5fc022) is set at start-up:
// gr_screenshot then writes a Targa row by row, which has no such table. (The "targa" console command toggles it back.)
//
// [Debug] TestSaves=1 (testing only): setup_data_paths (0x43dee1) registers the .sav folder from the string
// "savegame" (0x594b00); it becomes "savetest", so saves and loads use Summoner/savetest and the player's own
// slots are never touched. The folder is created if missing.
//
// [Display] FieldOfView=90 (2026-11-10; empty or 0 = the game's): the horizontal field of view, in degrees, of the
// 4:3 in-game view (the game's is 73.74). interface_init_global (GameInit) measures the vertical fov of that view
// once into interface_view_fov (0x5bd818, about 52.4 degrees); every frame outside cutscenes interface_set_view_clip
// turns it back into the horizontal fov for the current clip (side panels, a wide GameWidth), and gr_setup_3d builds
// the projection, the frustum and the mouse-picking ray from that. So one write of interface_view_fov after GameInit
// changes the in-game view everywhere and keeps the game's widescreen behaviour (the vertical angle is what stays;
// wider screens see more at the sides). Cutscenes keep their shots' fov; the item views keep their fixed 60.
// A wider view puts more objects in the draw lists, which the game fills with no bounds check (sort and opaque 500
// entries, attach 2000; only the shared entry pool, 0xc800 bytes = at most 3200 entries, is checked). So the three
// lists are first moved to arrays of 3200 entries here (23 operands, each compared before it is written); if any
// site does not match, neither the lists nor the fov are changed. The rebuilt exes do this in their source
// (experimental/display.cpp, game/unnamed_render_scene_0x481680.cpp) and SummonerFix does not install it there.
#include "summonerfix.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VA_CAMERA_ZOOM_MAX 0x59b800u
#define VA_PLAY_INTRO_MOVIES 0x5930bdu
#define VA_INTERACT_OPERAND 0x40e696u  // FCOM dword ptr [0x57ef18] in hud_interact_nearest
#define VA_CONTAINER_OPERAND 0x43ad38u // FCOMP dword ptr [0x580e08] in world_mouse_frame
#define VA_PRINT_SCREEN_AS_TARGA 0x5fc022u // bool, console command "targa"
#define VA_SAVEGAME_DIR_NAME 0x594b00u // "savegame", the .sav path registered by setup_data_paths

static float g_interactRange, g_containerRangeSq;

static float IniFloat(const char *key)
{
    char buf[32];
    GetPrivateProfileStringA("Game", key, "", buf, sizeof buf, g_cfg.iniPath);
    return buf[0] ? (float)atof(buf) : 0.0f;
}

// ---- [Display] FieldOfView ----
#define VA_INTERFACE_VIEW_FOV 0x5bd818u   // float, vertical fov of the 4:3 in-game view (degrees), set by GameInit
#define GAME_FOV 73.74f                  // the horizontal fov GameInit measures it with (camera constructor)
#define FOV_MIN 40.0f
#define FOV_MAX 110.0f
#define DRAW_LIST_CAP 3200               // levelrender_entry_pool (0xc800 bytes) / 16-byte entries

typedef struct { DWORD list; const DWORD *sites; int count; } DrawListMove;
// Every instruction that holds a list's base address (llvm-objdump of Sum.exe 1.40, 2026-10-14 listing, searched
// for any address inside the three lists): pushes to levelrender_sort_back_to_front / shadows_draw_characters /
// levelrender_draw_list, the `mov [4*ecx + list], eax` pushes, the lamp-glow loops and the shadow pass (0x4a4c..).
static const DWORD k_sortSites[] = {0x4823db, 0x482407, 0x482589, 0x48314a, 0x483aef, 0x4842f5, 0x48461d, 0x4a4db4};
static const DWORD k_opaqueSites[] = {0x4823ec, 0x482572, 0x483160, 0x483b0b, 0x4842df, 0x4a4d27};
static const DWORD k_attachSites[] = {0x48255b, 0x483482, 0x483695, 0x4837f7, 0x483b21, 0x4843fd, 0x484518, 0x48477e, 0x4a4c93};
static DWORD g_sortList[DRAW_LIST_CAP], g_opaqueList[DRAW_LIST_CAP], g_attachList[DRAW_LIST_CAP];
static float g_fovWant;

// The operand's address inside the instruction at va (the base address is in its first 8 bytes), or 0.
static DWORD OperandOf(DWORD va, DWORD list)
{
    const BYTE *p = (const BYTE *)(UINT_PTR)SumVa(va);
    for (int k = 1; k <= 4; k++) {
        DWORD v;
        memcpy(&v, p + k, 4);
        if (v == list)
            return va + k;
    }
    return 0;
}

static BOOL MoveDrawLists(void)
{
    const DrawListMove moves[3] = {
        {0x22f8d84, k_sortSites, (int)(sizeof k_sortSites / sizeof k_sortSites[0])},
        {0x22fa6a8, k_opaqueSites, (int)(sizeof k_opaqueSites / sizeof k_opaqueSites[0])},
        {0x22ea584, k_attachSites, (int)(sizeof k_attachSites / sizeof k_attachSites[0])},
    };
    DWORD *to[3] = {g_sortList, g_opaqueList, g_attachList};
    for (int i = 0; i < 3; i++) // check every site before writing any
        for (int j = 0; j < moves[i].count; j++)
            if (!OperandOf(moves[i].sites[j], moves[i].list)) {
                Log("SKIP FieldOfView: draw-list site 0x%06lX does not hold 0x%08lX", moves[i].sites[j], moves[i].list);
                return FALSE;
            }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < moves[i].count; j++)
            if (!WriteDword(OperandOf(moves[i].sites[j], moves[i].list), moves[i].list, (DWORD)(UINT_PTR)to[i],
                            "draw list -> 3200 entries (FieldOfView)"))
                return FALSE; // cannot happen after the check; the lists already moved stay consistent per list only
    return TRUE;
}

// Waits for GameInit's interface_view_fov, then scales it once.
static DWORD WINAPI ApplyFov(LPVOID unused)
{
    volatile float *v = (volatile float *)(UINT_PTR)SumVa(VA_INTERFACE_VIEW_FOV);
    while (!(*v > 1.0f))
        Sleep(50);
    const double rad = 3.14159265358979 / 180.0;
    float game = *v;
    float want = (float)(2.0 * atan(tan(game * rad / 2) * tan(g_fovWant * rad / 2) / tan(GAME_FOV * rad / 2)) / rad);
    *v = want;
    Log("FieldOfView %.2f: vertical fov %.2f -> %.2f degrees (the game's horizontal 4:3 view is %.2f)", g_fovWant,
        game, want, GAME_FOV);
    return 0;
}

static void FieldOfViewInstall(void)
{
    char buf[32];
    GetPrivateProfileStringA("Display", "FieldOfView", "", buf, sizeof buf, g_cfg.iniPath);
    float fov = buf[0] ? (float)atof(buf) : 0.0f;
    if (fov <= 0.0f)
        return;
    if (fov < FOV_MIN || fov > FOV_MAX) {
        float c = fov < FOV_MIN ? FOV_MIN : FOV_MAX;
        Log("FieldOfView %.2f is outside %.0f-%.0f; using %.0f", fov, FOV_MIN, FOV_MAX, c);
        fov = c;
    }
    if (fabsf(fov - GAME_FOV) < 0.01f)
        return;
    if (!MoveDrawLists())
        return;
    g_fovWant = fov;
    CloseHandle(CreateThread(NULL, 0, ApplyFov, NULL, 0, NULL));
}

void GameOptionsInstall(void)
{
    FieldOfViewInstall();
    float zoom = IniFloat("CameraZoomMax");
    if (zoom > 0.3f) {
        float orig = 1.0f;
        PatchBytes(VA_CAMERA_ZOOM_MAX, &orig, &zoom, 4, "camera zoom-out limit (CameraZoomMax)");
        Log("camera zoom-out limit %.2f (game default 1.0)", zoom);
    }

    if (GetPrivateProfileIntA("Game", "SkipIntroMovies", 0, g_cfg.iniPath)) {
        BYTE on = 1, off = 0;
        PatchBytes(VA_PLAY_INTRO_MOVIES, &on, &off, 1, "skip opening movies (SkipIntroMovies)");
    }

    float range = IniFloat("InteractRange");
    if (range > 0.0f) {
        g_interactRange = range;
        WriteDword(VA_INTERACT_OPERAND, 0x57ef18, (DWORD)(UINT_PTR)&g_interactRange, "interaction range (InteractRange)");
        Log("interaction range %.2f (game default 5.0)", range);
    }

    range = IniFloat("ContainerRange");
    if (range > 0.0f) {
        g_containerRangeSq = range * range;
        WriteDword(VA_CONTAINER_OPERAND, 0x580e08, (DWORD)(UINT_PTR)&g_containerRangeSq, "container range (ContainerRange)");
        Log("container range %.2f (game default 5.0)", range);
    }

    if (g_cfg.height > 1024) {
        BYTE off = 0, on = 1;
        PatchBytes(VA_PRINT_SCREEN_AS_TARGA, &off, &on, 1, "Print Screen as Targa (GameHeight above 1024, C290)");
    }
    TestSavesInstall();
}

// Also installed in the standalone rebuilt exe (main.c): the string is data, reached through the address table.
void TestSavesInstall(void)
{
    if (GetPrivateProfileIntA("Debug", "TestSaves", 0, g_cfg.iniPath)) {
        char dir[MAX_PATH];
        _snprintf(dir, sizeof dir, "%s%csavetest", g_cfg.installPath, 92);
        dir[sizeof dir - 1] = 0;
        CreateDirectoryA(dir, NULL);
        if (!g_rebuilt) {
            PatchBytes(VA_SAVEGAME_DIR_NAME, "savegame", "savetest", 8, "save folder -> savetest (TestSaves)");
            return;
        }
        // The rebuilt exe orders its string literals differently, so the address table cannot place this one: find
        // the string itself (it occurs once, as a whole string, in both exes).
        BYTE *base = (BYTE *)GetModuleHandleA(NULL);
        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
        IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
            if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                continue;
            BYTE *p = base + sec->VirtualAddress, *end = p + sec->Misc.VirtualSize - 10;
            for (; p < end; p++) {
                if (p[0] == 0 && memcmp(p + 1, "savegame", 9) == 0) {
                    DWORD prot;
                    if (VirtualProtect(p + 1, 8, PAGE_READWRITE, &prot)) {
                        memcpy(p + 1, "savetest", 8);
                        VirtualProtect(p + 1, 8, prot, &prot);
                        Log("patched save folder -> savetest (TestSaves) at 0x%08lX (rebuilt exe)", (DWORD)(UINT_PTR)(p + 1));
                    }
                    return;
                }
            }
        }
        Log("SKIP save folder -> savetest (TestSaves): \"savegame\" not found in the rebuilt exe");
    }
}
