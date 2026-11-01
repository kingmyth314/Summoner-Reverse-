// The calls from the game's code into the fixed and experimental builds' features (include/experimental/experimental.h). Each
// feature's init runs once, at the first frame (the game's window, renderer and input exist by then).
#include "experimental/experimental.h"

#ifdef FIXED_BUILD

static bool exp_started;

static void exp_start()
{
    exp_started = true;
#ifdef EXPERIMENTAL_BUILD
    exp_log("experimental build: started");
#else
    exp_log("fixed build: started");
#endif
    exp_controller_init();
#ifdef EXPERIMENTAL_BUILD
    exp_autosave_init();
#endif
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
#ifdef EXPERIMENTAL_BUILD
    exp_autosave_frame();
#endif
}

void exp_level_loaded(bool ok)
{
    exp_log("level loaded: %d", ok ? 1 : 0);
#ifdef EXPERIMENTAL_BUILD
    exp_autosave_level_loaded(ok);
#endif
}

#endif // FIXED_BUILD
