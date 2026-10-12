// SummonerFix entry point: configuration, game-build check, code patches and the DirectInput proxy.
//
// Sum.exe imports DINPUT.dll!DirectInputCreateA, so Windows loads this DLL from the game folder
// before the game's own code runs. DllMain applies the fixes; DirectInputCreateA forwards to the
// real system dinput.dll.
#include "summonerfix.h"
#include <stdio.h>
#include <string.h>

Config g_cfg;

// Bytes at 0x53983f in the v1.40 Sum.exe (same check Timeslip's patch uses).
#define SUM140_SIGNATURE_VA 0x53983fu
#define SUM140_SIGNATURE 0x8B52106Au

static void LoadConfig(void)
{
    char dir[MAX_PATH];
    GetModuleFileNameA(NULL, dir, MAX_PATH);
    char *slash = strrchr(dir, '\\');
    if (slash)
        *slash = 0;

    _snprintf(g_cfg.iniPath, MAX_PATH, "%s\\summonerfix.ini", dir);
    lstrcpynA(g_cfg.installPath, dir, MAX_PATH);

    g_cfg.width = GetPrivateProfileIntA("Display", "Width", 1024, g_cfg.iniPath);
    g_cfg.height = GetPrivateProfileIntA("Display", "Height", 768, g_cfg.iniPath);
    g_cfg.windowed = GetPrivateProfileIntA("Display", "Windowed", 0, g_cfg.iniPath);
    g_cfg.maxTextureSize = GetPrivateProfileIntA("Display", "MaxTextureSize", 2048, g_cfg.iniPath);

    char r[32];
    GetPrivateProfileStringA("Display", "Renderer", "OpenGL", r, sizeof r, g_cfg.iniPath);
    g_cfg.renderer = !lstrcmpiA(r, "Direct3D") ? RENDERER_DIRECT3D
                     : !lstrcmpiA(r, "Glide")  ? RENDERER_GLIDE
                                               : RENDERER_OPENGL;

    GetPrivateProfileStringA("Sound", "Soundcard", "DirectSound", g_cfg.soundcard, sizeof g_cfg.soundcard,
                             g_cfg.iniPath);

    char log[MAX_PATH];
    _snprintf(log, MAX_PATH, "%s\\summonerfix.log", dir);
    LogInit(log);
    Log("SummonerFix loaded. ini: %s", g_cfg.iniPath);
    Log("config: %dx%d, renderer %s, %s, sound '%s'", g_cfg.width, g_cfg.height,
        g_cfg.renderer == RENDERER_DIRECT3D ? "Direct3D" : g_cfg.renderer == RENDERER_GLIDE ? "Glide" : "OpenGL",
        g_cfg.windowed ? "windowed" : "fullscreen", g_cfg.soundcard);
}

static BOOL IsSupportedBuild(void)
{
    // Read the signature without faulting if this DLL was loaded by some other program.
    MEMORY_BASIC_INFORMATION mbi;
    DWORD addr = SumVa(SUM140_SIGNATURE_VA);
    if (!VirtualQuery((void *)(UINT_PTR)addr, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT)
        return FALSE;
    return *(const DWORD *)(UINT_PTR)addr == SUM140_SIGNATURE;
}

static void ApplyDisplayPatches(void)
{
    // Resolution: LoadSettings (FUN_0043d7f0) only accepts sizes found in the 3-entry table at
    // 0x580e38 {640x480, 800x600, 1024x768}. Put a custom size into the 1024x768 slot.
    BOOL inTable = (g_cfg.width == 640 && g_cfg.height == 480) || (g_cfg.width == 800 && g_cfg.height == 600) ||
                   (g_cfg.width == 1024 && g_cfg.height == 768);
    if (!inTable) {
        WriteDword(0x580e48, 1024, (DWORD)g_cfg.width, "resolution table width");
        WriteDword(0x580e4c, 768, (DWORD)g_cfg.height, "resolution table height");
    }

    // Renderer: `MOV EDI, 0x67` at 0x43da64 is the default (Glide) choice. For OpenGL make it 0x68,
    // the game's complete but never-selected gr_opengl renderer.
    if (g_cfg.renderer == RENDERER_OPENGL) {
        static const BYTE orig[] = {0xBF, 0x67, 0x00, 0x00, 0x00};
        static const BYTE ogl[] = {0xBF, 0x68, 0x00, 0x00, 0x00};
        PatchBytes(0x43da64, orig, ogl, sizeof orig, "default renderer -> OpenGL (0x68)");
    }

    // Window mode: `PUSH 0xc9` (201 = fullscreen) at 0x43db45 -> 200 (windowed).
    if (g_cfg.windowed) {
        static const BYTE orig[] = {0x68, 0xC9, 0x00, 0x00, 0x00};
        static const BYTE win[] = {0x68, 0xC8, 0x00, 0x00, 0x00};
        PatchBytes(0x43db45, orig, win, sizeof orig, "window mode -> windowed (200)");
    }
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    DisableThreadLibraryCalls(inst);
    LoadConfig();
    if (!IsSupportedBuild()) {
        Log("Not Sum.exe v1.40 (signature mismatch at 0x%06X); no fixes applied, DirectInput proxy only.",
            SUM140_SIGNATURE_VA);
        return TRUE;
    }
    Log("Sum.exe v1.40 detected.");
    if (GetPrivateProfileIntA("Debug", "Diagnostics", 0, g_cfg.iniPath))
        DiagInstall(GetModuleHandleA(NULL));
    RegHookInstall(GetModuleHandleA(NULL));
    GlFixInstall(GetModuleHandleA(NULL));
    ApplyDisplayPatches();
    Log("startup fixes applied.");
    return TRUE;
}

// ---- DirectInput proxy -------------------------------------------------------------------------

typedef HRESULT(WINAPI *PFN_DirectInputCreateA)(HINSTANCE, DWORD, void **, void *);

__declspec(dllexport) HRESULT WINAPI DirectInputCreateA(HINSTANCE inst, DWORD version, void **out, void *outer)
{
    static PFN_DirectInputCreateA real;
    if (!real) {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH); // SysWOW64 for this 32-bit process
        strcat(path, "\\dinput.dll");
        HMODULE m = LoadLibraryA(path);
        real = m ? (PFN_DirectInputCreateA)GetProcAddress(m, "DirectInputCreateA") : NULL;
        Log("system DirectInput %s (%s)", real ? "loaded" : "NOT FOUND", path);
    }
    if (!real)
        return E_FAIL;
    HRESULT hr = real(inst, version, out, outer);
    Log("DirectInputCreateA(version 0x%04lX) -> 0x%08lX", version, (unsigned long)hr);
    return hr;
}
