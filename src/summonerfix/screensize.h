// The game size for [Display] GameWidth=Auto, shared by SummonerFix (main.c) and SummonerFixConfig.exe (config.c):
// the interface's 1024x768 layout at the screen's shape. 768 high (the height the interface is laid out for) and as
// wide as the primary display's aspect gives, rounded to an even width; never narrower than 1024 (a 4:3 or taller
// screen gets 1024x768). The 3D world is drawn at the screen's own resolution whatever this size is. [Display]
// InterfaceScale below 100 makes the size taller (768 * 100 / scale) at the same shape, so the 768-line interface
// covers that share of the screen height (the exes that place their interface in such a size: the fixed, experimental
// and 64-bit builds).
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

// [Display] InterfaceScale (2026-11-10, docs/future/interface-scale.md): the interface's height in percent of the
// screen's, 50-100 (100, the default, is the 768-high size). Values outside are clamped; 0 or less means 100.
#define DISPLAY_SCALE_MIN 50
#define DISPLAY_SCALE_MAX 100
static __inline int DisplayClampScale(int percent)
{
    if (percent <= 0 || percent > DISPLAY_SCALE_MAX)
        return DISPLAY_SCALE_MAX;
    return percent < DISPLAY_SCALE_MIN ? DISPLAY_SCALE_MIN : percent;
}

// The game size for a screen of sw x sh with the interface at percent of the screen height: 768 * 100 / percent
// lines (even; 768 at 100), as wide as the screen's aspect gives (even), never narrower than the 1024x768 layout at
// the same scale (1024 at 100).
static __inline void DisplaySizeForAspectScaled(int sw, int sh, int percent, int *w, int *h)
{
    int gh, gw, minw;
    percent = DisplayClampScale(percent);
    gh = (int)((768.0 * 100.0 / percent) / 2.0 + 0.5) * 2;
    minw = (int)((1024.0 * gh / 768.0) / 2.0 + 0.5) * 2;
    gw = minw;
    if (sw > 0 && sh > 0)
        gw = (int)(((double)gh * sw / sh) / 2.0 + 0.5) * 2;
    if (gw < minw)
        gw = minw;
    *w = gw;
    *h = gh;
}

// The 768-high game size for a screen of sw x sh.
static __inline void DisplaySizeForAspect(int sw, int sh, int *w, int *h)
{
    DisplaySizeForAspectScaled(sw, sh, 100, w, h);
}

static __inline void DisplaySizeForScreen(int *w, int *h)
{
    int sw, sh;
    DisplayDesktopSize(&sw, &sh);
    DisplaySizeForAspect(sw, sh, w, h);
}

static __inline void DisplaySizeForScreenScaled(int percent, int *w, int *h)
{
    int sw, sh;
    DisplayDesktopSize(&sw, &sh);
    DisplaySizeForAspectScaled(sw, sh, percent, w, h);
}

#endif
