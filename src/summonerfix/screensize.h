// The game size for [Display] GameWidth=Auto, shared by SummonerFix (main.c) and SummonerFixConfig.exe (config.c):
// the interface's 1024x768 layout at the screen's shape. 768 high (the height the interface is laid out for) and as
// wide as the primary display's aspect gives, rounded to an even width; never narrower than 1024 (a 4:3 or taller
// screen gets 1024x768). The 3D world is drawn at the screen's own resolution whatever this size is.
#ifndef SUMMONERFIX_SCREENSIZE_H
#define SUMMONERFIX_SCREENSIZE_H
#include <windows.h>

// The primary display's current mode in real pixels (EnumDisplaySettings is not scaled by DPI virtualization, which
// GetSystemMetrics is in a process that is not DPI aware). 0 x 0 if it cannot be read.
static __inline void DisplayDesktopSize(int *w, int *h)
{
    DEVMODEA dm;
    ZeroMemory(&dm, sizeof dm);
    dm.dmSize = sizeof dm;
    if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) && dm.dmPelsWidth && dm.dmPelsHeight) {
        *w = (int)dm.dmPelsWidth;
        *h = (int)dm.dmPelsHeight;
    } else {
        *w = GetSystemMetrics(SM_CXSCREEN);
        *h = GetSystemMetrics(SM_CYSCREEN);
    }
}

// The 768-high game size for a screen of sw x sh.
static __inline void DisplaySizeForAspect(int sw, int sh, int *w, int *h)
{
    int gw = 1024;
    if (sw > 0 && sh > 0)
        gw = (int)((768.0 * sw / sh) / 2.0 + 0.5) * 2;
    if (gw < 1024)
        gw = 1024;
    *w = gw;
    *h = 768;
}

static __inline void DisplaySizeForScreen(int *w, int *h)
{
    int sw, sh;
    DisplayDesktopSize(&sw, &sh);
    DisplaySizeForAspect(sw, sh, w, h);
}

#endif
