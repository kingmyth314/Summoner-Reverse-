// The features of the fixed and experimental builds (docs/experimental-build.md). Since 2026-11-01 most exist with
// FIXED_BUILD (the fixed and experimental configurations): the controller, the widescreen interface, the game options,
// the frame-rate cap and vsync, the upscaled interface, the level reverb and the dgVoodoo 2 renderers. Autosave and
// the Tentacle Beast grab change how the game plays and stay EXPERIMENTAL_BUILD only. The shipped build never sees
// any of it. The code lives in src/decomp/experimental/ (the folder keeps its name), one file per feature, which
// link-build compiles for both configurations; the byte matching (vc6-compare, vc6-sweep) never reads that folder.
//
// Switches: every feature has an ini key in summonerfix.ini next to the exe (the file SummonerFixConfig.exe edits),
// off by default, so a problem can be ruled out by turning it off. A feature that SummonerFix also provides marks its
// code with `// FIXED_BUILD: <feature> <Key>` (`EXPERIMENTAL_BUILD:` for the experimental-only ones); link-build
// writes those keys into the exe's .fixes, and SummonerFix
// (RebuiltFixInSource) then leaves its own version out, so the two never drive the same thing twice.
#ifndef EXPERIMENTAL_EXPERIMENTAL_H
#define EXPERIMENTAL_EXPERIMENTAL_H

#ifdef FIXED_BUILD

// ---- exp_config.cpp: switches and log ---------------------------------------------------------------------------
// summonerfix.ini next to the exe; def when the file or the key is missing.
int  exp_ini_int(const char *section, const char *key, int def);
// The value as text into out (size n); def when missing. Returns the length.
int  exp_ini_string(const char *section, const char *key, const char *def, char *out, int n);
// An opt-in [Fixes] key, a bug fix that is off unless the player turns it on (2026-11-10: G15 DamageDice and
// K272 ShadowSizes): 1 when [Fixes] Enabled (default 1) and the key (default 0) are both set. Reads the ini on
// every call: callers keep the result.
int  exp_fix_opt_in(const char *key);
// One line into experimental.log next to the exe (printf format; a newline is added). Cheap when [Debug]
// ExperimentalLog=0 (the default): nothing is written.
void exp_log(const char *fmt, ...);

// ---- exp_hooks.cpp: the calls from the game's code (each under #ifdef FIXED_BUILD at its call site) ------------
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
#ifdef EXPERIMENTAL_BUILD
// autosave.cpp: [Experimental] Autosave (experimental build only)
void exp_autosave_init();
void exp_autosave_frame();
void exp_autosave_level_loaded(bool ok);
bool exp_autosave_enabled();      // [Experimental] Autosave is on (read at the first frame, before any screen opens)
#endif
// cheats.cpp: [Cheats] (off by default): XpMultiplier, GoldKey, SkillPointKey
void exp_cheats_init();
void exp_cheats_load_keys();      // the in-source host's key reload
int  exp_cheat_scale_xp(int xp);  // Character::add_xp
void exp_cheat_key(int scan);     // the in-source host's key handler (game thread)
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
// gr_font_create, after the game built a font's atlas: with <font>-hires.tga (tools/texture-upscale.mjs; 2x or 4x
// the atlas, the same layout) the font draws from it at the atlas's size. f is a Font * (vsdk/gr/gr_font.h).
void exp_font_hires(struct Font *f);
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
bool exp_ws_full_panel();                                // a full-screen panel in play, centred like a menu
int  exp_ws_view_width();                                // interface_set_view_clip's gr_screen_width
int  exp_ws_panel_width(int width);                      // interface_set_view_clip's side-panel widths
// levelreverb.cpp: [Experimental] LevelReverb, the level's <level>.eax read from the packfiles (game-bugs K261)
void exp_level_reverb(const char *name);                 // level_parse_table, when no loose file was found
// The Tentacle Beast grab is written inside game/unnamed_tentacle_beast_0x4c6480.cpp under EXPERIMENTAL_BUILD
// ([Experimental] TentacleGrab); it needs no hook here.

#endif // FIXED_BUILD
#endif
