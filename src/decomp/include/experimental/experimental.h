// The experimental build's additions (docs/experimental-build.md). Everything here exists only when
// EXPERIMENTAL_BUILD is defined (tools/link-build.mjs --config experimental); the shipped and fixed builds never see
// it. The code lives in src/decomp/experimental/, one file per feature, which link-build compiles only for that
// configuration; the byte matching (vc6-compare, vc6-sweep) never reads that folder.
//
// Switches: every feature has an ini key in summonerfix.ini next to the exe (the file SummonerFixConfig.exe edits),
// off by default, so a problem can be ruled out by turning it off. A feature that SummonerFix also provides marks its
// code with `// EXPERIMENTAL_BUILD: <feature> <Key>`; link-build writes those keys into Sum-exp.fixes, and SummonerFix
// (RebuiltFixInSource) then leaves its own version out, so the two never drive the same thing twice.
#ifndef EXPERIMENTAL_EXPERIMENTAL_H
#define EXPERIMENTAL_EXPERIMENTAL_H

#ifdef EXPERIMENTAL_BUILD

// ---- exp_config.cpp: switches and log ---------------------------------------------------------------------------
// summonerfix.ini next to the exe; def when the file or the key is missing.
int  exp_ini_int(const char *section, const char *key, int def);
// The value as text into out (size n); def when missing. Returns the length.
int  exp_ini_string(const char *section, const char *key, const char *def, char *out, int n);
// One line into experimental.log next to the exe (printf format; a newline is added). Cheap when [Debug]
// ExperimentalLog=0 (the default): nothing is written.
void exp_log(const char *fmt, ...);

// ---- exp_hooks.cpp: the calls from the game's code (each under #ifdef EXPERIMENTAL_BUILD at its call site) ------
// GameTick (0x401070), after mouse_get_pos and before the multiplayer frame and the state's frame handler: the
// place to feed input for this frame.
void exp_frame_begin();
// GameTick, just before FrameEnd: per-frame work after the game state ran (autosave timing, overlays).
void exp_frame_end();
// level_load (0x438570), after level_load_files; ok is its result.
void exp_level_loaded(bool ok);

// ---- one entry point set per feature, each in its own file (the stubs do nothing) ------------------------------
// controller.cpp: [Controller] (tier 1, the SummonerFix controller in the source)
void exp_controller_init();
void exp_controller_frame();
// autosave.cpp: [Experimental] Autosave
void exp_autosave_init();
void exp_autosave_frame();
void exp_autosave_level_loaded(bool ok);
bool exp_autosave_enabled();      // [Experimental] Autosave is on (read at the first frame, before any screen opens)
// display.cpp: [Experimental] display options (frame-rate cap, vsync, field of view)
void exp_display_init();
void exp_display_frame();
// hires.cpp: [Experimental] HiResInterface (upscaled interface images and fonts on OpenGL)
void exp_bm_entries_allocated(int index, int count);     // bm_alloc_entry / bm_alloc_entries: forget the old entry
// bm_load_internal: count entries from index (an animation's header and frames) loaded from filename at w x h.
void exp_bm_loaded(int index, int count, const char *filename, int w, int h, bool reduced);
bool exp_bm_nominal_size(int index, int *w, int *h);     // bm_get_dimensions: the size the game was made for
// A pixel reader of bitmap bm that indexes its data with the nominal width: scales (x, y) to the real image and
// returns the real width (the row stride); unchanged without an override.
int  exp_bm_real_point(int bm, int *x, int *y, int nominal_w);
// gameopts.cpp: SummonerFix's [Game] CameraZoomMax, SkipIntroMovies, InteractRange, ContainerRange
void exp_game_options_init();                            // GameInit, before the main menu
extern float exp_interact_range;                         // hud_interact_nearest (5.0)
extern float exp_container_range_sq;                     // world_mouse_frame (25.0)
// widescreen.cpp: the interface centred on a wide screen ([Display] CenterHud); the wrappers are in ui_shift.h
extern int exp_ws_clip_shifted;                          // gr::set_clip clears it (gr.cpp)
extern int exp_mouse_raw_x, exp_mouse_raw_y;             // the unshifted cursor, for world code
int  exp_ws_mouse_tick(int *x, int *y, int *z);          // GameTick's mouse_get_pos
int  exp_ws_side_panel_begin(bool frame);                // around interface_draw / interface_frame's switch
void exp_ws_side_panel_end(int undo);
int  exp_ws_view_width();                                // interface_set_view_clip's gr_screen_width
int  exp_ws_panel_width(int width);                      // interface_set_view_clip's side-panel widths
// The Tentacle Beast grab is written inside game/unnamed_tentacle_beast_0x4c6480.cpp under EXPERIMENTAL_BUILD
// ([Experimental] TentacleGrab); it needs no hook here.

#endif // EXPERIMENTAL_BUILD
#endif
