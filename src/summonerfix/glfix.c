// OpenGL renderer fixes (renderer 0x68, vsdk\gr\opengl\gr_opengl.cpp).
#include "summonerfix.h"

#define GL_MAX_TEXTURE_SIZE 0x0D33
typedef void(WINAPI *PFN_glGetIntegerv)(unsigned int, int *);
static PFN_glGetIntegerv real_glGetIntegerv;

// FUN_0054e690 allocates a scratch buffer of max_texture_size^2 * 4 bytes. With a modern driver
// reporting 16384 that is exactly 1 GB, which fails in a 32-bit process ("Failed to allocate
// 1073741824 bytes", memory.cpp line 223). The game's own textures are far smaller, so report a
// capped maximum instead.
static void WINAPI Hook_glGetIntegerv(unsigned int pname, int *params)
{
    real_glGetIntegerv(pname, params);
    if (pname == GL_MAX_TEXTURE_SIZE && params && *params > g_cfg.maxTextureSize) {
        Log("GL_MAX_TEXTURE_SIZE %d capped to %d", *params, g_cfg.maxTextureSize);
        *params = g_cfg.maxTextureSize;
    }
}

void GlFixInstall(HMODULE exe)
{
    real_glGetIntegerv = HookImport(exe, "OPENGL32.dll", "glGetIntegerv", Hook_glGetIntegerv);
}
