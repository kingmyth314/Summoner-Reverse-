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
    int centerMenus;
    int centerHud;      // GameWidth > 1024: centre the 1024-wide HUD and in-game screens during gameplay    // GameWidth > 1024: centre the 1024-wide menu layout outside gameplay
    char soundcard[64];
    char installPath[MAX_PATH];
    char iniPath[MAX_PATH];
} Config;

extern Config g_cfg;

// util.c
void LogInit(const char *path);
void Log(const char *fmt, ...);
DWORD SumVa(DWORD va); // Sum.exe virtual address -> address in this process
BOOL PatchBytes(DWORD va, const void *expect, const void *repl, SIZE_T n, const char *what);
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

// keys.c
void KeyBindInstall(void);

// widescreen.c
void WidescreenInstall(void);
int WidescreenFullPanel(void); // in game with a full-screen panel open: shift the whole output
