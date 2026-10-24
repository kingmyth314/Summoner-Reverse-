// SummonerFix - dinput.dll proxy that updates Summoner (Volition, 2000) v1.40 for modern Windows.
// Shared declarations.
#pragma once
#include <windows.h>

#define SUM_IMAGE_BASE 0x400000u

typedef enum { RENDERER_DIRECT3D = 0x66, RENDERER_GLIDE = 0x67, RENDERER_OPENGL = 0x68 } Renderer;

typedef struct {
    int width;          // game's internal resolution (UI is laid out for 1024x768)
    int height;
    Renderer renderer;
    int windowed;       // Direct3D/Glide: game window mode 200 instead of 201
    int borderless;     // OpenGL output: 1 = borderless fullscreen, 0 = window
    int windowWidth;    // OpenGL output window client size when borderless == 0
    int windowHeight;
    int maxFps;         // 0 = game default (120)
    int captureEvery;   // debug: seconds between frame captures to summonerfix-capture-N.bmp, 0 = off
    int maxTextureSize;
    int centerMenus;         // GameWidth > 1024: centre the 1024-wide menu layout outside gameplay
    int centerHud;           // GameWidth > 1024: centre the 1024-wide HUD and in-game screens during gameplay
    int sidePanelsOverWorld; // wide screens: keep the 3D view full width behind side panels (map, spells, skills)
    int autoSize;            // GameWidth=Auto: width and height were worked out from the screen (screensize.h)
    char soundcard[64];
    char installPath[MAX_PATH];
    char iniPath[MAX_PATH];
} Config;

extern Config g_cfg;

// util.c
void LogInit(const char *path);
void Log(const char *fmt, ...);
DWORD SumVa(DWORD va); // Sum.exe virtual address -> address in this process
BOOL RebuiltInit(void);  // rebuilt.c: in the standalone rebuilt exe, load its address table
BOOL RebuiltFixInSource(const char *key); // rebuilt.c: the fixed build already fixes this [Fixes] key in its source
DWORD RebuiltVa(DWORD va);
DWORD RebuiltOrigVa(DWORD now);
DWORD OrigVa(DWORD addr); // an address in this process (a return address) -> its Sum.exe address
extern BOOL g_rebuilt;
BOOL PatchBytes(DWORD va, const void *expect, const void *repl, SIZE_T n, const char *what);
DWORD PatchedWithin(DWORD va, DWORD n); // the first address PatchBytes changed in [va, va + n), or 0
void KeepOriginal(DWORD va, DWORD n);   // the hybrid loader must not replace the function(s) here (a hook checks its caller)
BOOL WriteDword(DWORD va, DWORD expect, DWORD value, const char *what);
void *HookImport(HMODULE module, const char *dll, const char *func, void *replacement);

// reghook.c
void RegHookInstall(HMODULE exe);

// diag.c
void DiagInstall(HMODULE exe);

// glfix.c
void GlFixInstall(HMODULE exe);

// display.c
void DisplayInstall(HMODULE exe);

// diag.c
void FpsLoggerStart(void);

// timing.c
void TimingFixInstall(void);

// gameopts.c
void GameOptionsInstall(void);
void TestSavesInstall(void);

// bugfix.c
void BugFixesInstall(void);

// tentacle.c (experimental, [Experimental] TentacleGrab)
void TentacleGrabInstall(void);

// hybrid.c (the link experiment, [Hybrid] Dll)
void HybridInstall(void);
int HybridWriteCoverage(const char *path); // [Hybrid] Coverage: the call counts of the replaced functions

// dualcall.c ([Hybrid] Dual: a decompiled function checked against the original on the game's own calls)
BYTE *DualTarget(DWORD va, BYTE *dll, const char *list);
void DualSummary(void);

// keys.c
void KeyBindInstall(void);

// keys.c (agent mode)
void KeysInjectKey(int code, int down);
void KeysInjectMouse(int button, int down);
void KeysAgentSpeed(int on);  // agent mode: switch fast mode on/off
BOOL KeysAgentRun(void (*fn)(void *), void *arg); // agent mode: run fn(arg) on the game thread (posted message)
float KeysSpeedActive(void);   // current speed factor while fast mode is on, else 0

// display.c (agent mode)
void DisplayRequestCapture(const char *path);
BOOL DisplayCapturePending(void);
void DisplaySetAgentCursor(int clientX, int clientY);
void DisplayGetAgentCursor(int *clientX, int *clientY);
void DisplayMoveCursor(double dxGame, double dyGame); // controller: move the cursor by game pixels

// controller.c ([Controller], XInput gamepad, tier 1: drives the existing mouse and key input)
void ControllerInstall(void);
void ControllerFrame(void); // once per frame, on the game thread (display.c SwapBuffers hook)
void ControllerSetSynthetic(int lx, int ly, int rx, int ry, unsigned buttons, DWORD ms); // agent mode "pad"
BOOL ControllerSyntheticDone(void); // the synthetic state has ended and its inputs are released

// agent.c
void AgentInstall(void);

// trace.c (testing: [Debug] TraceArmourSpells)
void TraceInstall(void);
void TraceGlLightInstall(void);                            // [Debug] TraceGlLights (K310)
void TraceGlLightStats(long *faces, long *coloured, float rgb[3]);
void TraceGlDrawStats(long *calls, long *lit, long *maxLights);

// gamma.c
void GammaConfigure(const char *mode); // [Display] Brightness: Auto, Shader, Ramp or Off
void GammaApply(HWND wnd, int cw, int ch);
void GammaShutdown(void);

// widescreen.c
void WidescreenInstall(void);
int WidescreenFullPanel(void); // in game with a full-screen panel open: shift the whole output
