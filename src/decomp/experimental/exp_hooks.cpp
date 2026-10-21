// The calls from the game's code into the experimental features (include/experimental/experimental.h). Each
// feature's init runs once, at the first frame (the game's window, renderer and input exist by then).
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

static bool exp_started;

static void exp_start()
{
    exp_started = true;
    exp_log("experimental build: started");
    exp_controller_init();
    exp_autosave_init();
    exp_display_init();
}

void exp_frame_begin()
{
    if (!exp_started)
        exp_start();
    exp_controller_frame();
}

void exp_frame_end()
{
    exp_display_frame();
    exp_autosave_frame();
}

void exp_level_loaded(bool ok)
{
    exp_log("level loaded: %d", ok ? 1 : 0);
    exp_autosave_level_loaded(ok);
}

#endif // EXPERIMENTAL_BUILD
