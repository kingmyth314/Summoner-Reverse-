// SummonerFix's optional [Game] settings (gameopts.c) in the source. Not part of Sum.exe: compiled only for
// link-build --config experimental. SummonerFix cannot apply them to a rebuilt exe (two are operands in code that
// moved), so the experimental build reads the same keys itself, once, from GameInit (before the main menu):
//   CameraZoomMax     camera zoom-out limit (camera_zoom_max, 1.0; the zoom_limit console command sets 1000)
//   SkipIntroMovies   no vlogo.bik / eaxlogo.bik at start (play_intro_movies, read once by mainmenu_init)
//   InteractRange     door interaction distance in hud_interact_nearest (5.0)
//   ContainerRange    container / item click distance in world_mouse_frame (5.0, compared squared)
// and, as SummonerFix does above 1024 lines (game-bugs C290), Print Screen writes a Targa: the JPEG writer keeps one
// row pointer per line in a 1024-entry table.
#include <stdlib.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

#include "vsdk/vsdk.h"
#include "engine/camera/camera.h"
#include "summoner/summoner.h"

extern bool play_intro_movies;      // 0x5930bd (unnamed_states_0x431870.cpp)
extern bool Print_screen_as_targa;  // 0x5fc022 (unnamed_frame_0x4323e0.cpp)

float exp_interact_range = 5.0f;        // read by hud_interact_nearest (new_hud.cpp)
float exp_container_range_sq = 25.0f;   // read by world_mouse_frame (gametarget.cpp)

static float gameopts_float(const char *key)
{
    char buf[32];
    exp_ini_string("Game", key, "", buf, sizeof buf);
    return buf[0] ? (float)atof(buf) : 0.0f;
}

void exp_game_options_init()
{
    float v = gameopts_float("CameraZoomMax");
    if (v > 0.3f) {
        camera_zoom_max = v;
        exp_log("game options: camera zoom-out limit %.2f", v);
    }
    if (exp_ini_int("Game", "SkipIntroMovies", 0)) {
        play_intro_movies = false;
        exp_log("game options: opening movies skipped");
    }
    v = gameopts_float("InteractRange");
    if (v > 0.0f) {
        exp_interact_range = v;
        exp_log("game options: interaction range %.2f", v);
    }
    v = gameopts_float("ContainerRange");
    if (v > 0.0f) {
        exp_container_range_sq = v * v;
        exp_log("game options: container range %.2f", v);
    }
    if (resolution_table[resolution_index][1] > 1024) {
        Print_screen_as_targa = true;
        exp_log("game options: Print Screen as Targa (height above 1024, C290)");
    }
}

#endif // EXPERIMENTAL_BUILD
