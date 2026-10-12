// SummonerFix - dinput.dll proxy that updates Summoner (Volition, 2000) v1.40 for modern Windows.
// Shared declarations.
#pragma once
#include <windows.h>

#define SUM_IMAGE_BASE 0x400000u

typedef enum { RENDERER_DIRECT3D = 0x66, RENDERER_GLIDE = 0x67, RENDERER_OPENGL = 0x68 } Renderer;

typedef struct {
    int width;
    int height;
    Renderer renderer;
    int windowed;
    int maxTextureSize;
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
