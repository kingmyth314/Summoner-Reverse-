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
// [Debug] TestSaves=1 (testing only): setup_data_paths (0x43dee1) registers the .sav folder from the string
// "savegame" (0x594b00); it becomes "savetest", so saves and loads use Summoner/savetest and the player's own
// slots are never touched. The folder is created if missing.
#include "summonerfix.h"
#include <stdio.h>
#include <stdlib.h>

#define VA_CAMERA_ZOOM_MAX 0x59b800u
#define VA_PLAY_INTRO_MOVIES 0x5930bdu
#define VA_INTERACT_OPERAND 0x40e696u  // FCOM dword ptr [0x57ef18] in hud_interact_nearest
#define VA_CONTAINER_OPERAND 0x43ad38u // FCOMP dword ptr [0x580e08] in world_mouse_frame
#define VA_SAVEGAME_DIR_NAME 0x594b00u // "savegame", the .sav path registered by setup_data_paths

static float g_interactRange, g_containerRangeSq;

static float IniFloat(const char *key)
{
    char buf[32];
    GetPrivateProfileStringA("Game", key, "", buf, sizeof buf, g_cfg.iniPath);
    return buf[0] ? (float)atof(buf) : 0.0f;
}

void GameOptionsInstall(void)
{
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

    if (GetPrivateProfileIntA("Debug", "TestSaves", 0, g_cfg.iniPath)) {
        char dir[MAX_PATH];
        _snprintf(dir, sizeof dir, "%s%csavetest", g_cfg.installPath, 92);
        dir[sizeof dir - 1] = 0;
        CreateDirectoryA(dir, NULL);
        PatchBytes(VA_SAVEGAME_DIR_NAME, "savegame", "savetest", 8, "save folder -> savetest (TestSaves)");
    }
}
