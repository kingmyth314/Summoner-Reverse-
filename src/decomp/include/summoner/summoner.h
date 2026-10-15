// summoner/summoner.h - the game's top level: state sequencing, game flags, pause, level changes, the
// summoner.cfg options and the file path ids. Units: summoner.cpp (Summoner_options, summoner_cfg_save,
// the path ids), the state sequencer (gameseq_set_state 0x401020), the frame-end / pause unit
// (0x432660-0x432e60), the player input unit (0x4343a0) and the cutscene file (0x4349b0-0x437650);
// gameplay.cpp has summoner/gameplay.h and gametarget.cpp summoner/gametarget.h (both included below).
// One header for now; file name ours. Not original source.
#ifndef SUMMONER_SUMMONER_H
#define SUMMONER_SUMMONER_H

#include "vsdk/vsdk.h"

// ---- game flags (0x5fbfd8) --------------------------------------------------------------------------
extern int game_flags;                // 0x5fbfd8
#define GF_SINGLE_PLAYER 0x1
#define GF_MULTIPLAYER   0x2
#define GF_ESCAPE_CUTSCENE 0x10       // state_game_enter plays the Tiger / Serpent escape (cutscene_play_escape; name ours, decomp42)
#define GF_CONVERSATION  0x100        // state_game_frame runs conversation_frame until it ends (name ours, decomp42)
// Set at the end of state_game_enter and cleared by state_game_leave (decomp42): the level is entered,
// so leader changes during the load make no sound.
#define GF_LEADER_SOUND  0x200        // party_set_leader plays sound 0x28 when set
#define GF_NO_RENDER     0x400        // state_game_frame skips game_render_frame (name ours, decomp42)
#define GF_SOLO_MODE     0x1000
#define GF_NO_CLEAR      0x4000       // state_game_frame skips gr_clear (name ours, decomp42)
#define GF_PARTY_DEAD    0x40000      // party_check_all_dead: nobody alive (single player); cleared on enter / leave (name ours)
#define GF_CUTSCENE      0x20         // a cutscene is running (sum.symbols game_flags; name ours, decomp34)
#define GF_NO_INPUT      0x10000      // script_disable_player_input / script_enable_player_input (name ours, decomp43)
// Level-script input lock (unit not known, before player.cpp; decomp43): player_input = 0 and GF_NO_INPUT
// set / cleared.
void script_disable_player_input();   // 0x432f50
void script_enable_player_input();    // 0x432f70
// A byte next to the console variables 0x5fc008.. that no code writes (.bss, zero): ls_catacombs_frame
// plays cryptintro only while it is set (0x4fe1e6). Name ours (decomp43).
extern bool game_debug_5fc014;        // 0x5fc014

extern int game_mouse_x;              // 0x5fbfdc
extern int game_mouse_y;              // 0x5fbfe0

// ---- state sequencing, pause, levels ---------------------------------------------------------------
void gameseq_init();                  // 0x401000 cur = prev = next = 0
void gameseq_set_state(int state);    // 0x401020
int  gameseq_get_state();             // 0x401040 (4 = game play)
int  gameseq_get_prev_state();        // 0x401050 (decomp42)
int  gameseq_get_next_state();        // 0x401060 (decomp42)
int  GameTick();                      // 0x401070 one frame; returns the current state (3 = quit)
void game_pause(int a, int b);        // 0x432660 (both 0 in the pilots)
void game_pause_count_inc();          // 0x432d90 (GameInit)
void game_pause_count_dec();          // 0x432d80 (state_game_enter, state_loading_leave; decomp42)
void game_unpause();                  // 0x4326c0
bool game_is_paused();                // 0x432da0
void game_change_level(const char *filename, void *arg);   // 0x431a40 (second argument type guessed)
extern bool play_intro_movies;        // 0x5930bd
// The input poll of the menu states: calls key_handler(key) for every key until none is left (decomp42).
int  ui_poll_and_process(void (*key_handler)(int key), bool process_ui);   // 0x432860
void input_flush();                   // 0x4328a0 (decomp42)
extern bool cursor_visible;           // 0x5fc01c FrameEnd draws the mouse cursor while set (decomp42)
extern bool frame_debug_flag_5fc01e;  // 0x5fc01e FrameEnd calls debug_noop_frame_end while set; state_game_leave clears it (name ours)
void hud_set_timed_highlight(int handle);   // 0x43cd50 gametarget.cpp (decomp37: the dropped loot)
extern int world_mouse_target_handle;  // 0x593f50 gametarget.cpp: the object under the mouse (cmd_health; decomp37)

extern bool level_entered_by_load;             // 0x25443a8 (level_scripts.cpp; tutorial_frame / tutorial_draw run only while set; D101)

// ---- gameplay.cpp (0x437660-0x43a32f), in its unit header (decomp42: the party control, the level
// globals and camera_reset_to_player moved there) ------------------------------------------------------
#include "summoner/gameplay.h"

// ---- gametarget.cpp (0x43a330-0x43cd6d): world picking, in its unit header (decomp35) ---------------
#include "summoner/gametarget.h"
// The cutscene player (cutscene.tbl entries; unit not known) and the loaded cutscene script's name
// (cutscene_script_name 0x257e974, "" when none).
void cutscene_play(const char *name);              // 0x4361d0 (return unused)
char *cutscene_script_table();                     // 0x4ed760
extern bool US_open_demo;                          // 0x5fbf1a console "us_open_demo" (no level exits)
// ---- player input and cutscene playback (0x4343a0-0x437650; module "-"; decomp42) ------------------
void player_input_frame();            // 0x434470 (state_game_frame)
void cutscene_sequence_update();      // 0x436a90 (state_game_frame while GF_CUTSCENE)
void cutscene_events_frame();         // 0x436c00
void cutscene_play_escape();          // 0x437130 (state_game_enter with GF_ESCAPE_CUTSCENE)
extern bool cheat_gold_5fbf19;        // 0x5fbf19 9999900 gold on a new game (state_game_enter), read by lsf_catacombs_boss_dead (name ours)

// Empty functions the screens call (identical-code folding shares bare RETs between units).
void empty_frame_hook_40acc0();       // 0x40acc0 (bare ret, own copy)
void empty_function();                // 0x4f13e0 (bare ret shared by identical-code folding)

// ---- mouse cursor (gameplay_load_cursors / cursor_set_type 0x432e50; unit not known) ---------------------
void gameplay_load_cursors();         // 0x432db0 (GameInit)
void cursor_set_type(int bm);         // 0x432e50 cursor_type (0x5931b4) = bm
extern int cursor_url_bm;             // 0x5fbf2c cursor-url.vbm
extern int cursor_walk_bm;            // 0x5fbfd4 cursor-walk.vbm (the default)
extern int cursor_spot_target_bm;     // 0x5fbf40 cursorspot-target.vbm (world_pick_ground_point's decal; decomp35)
extern int cursor_spot_invalid_bm;    // 0x5fbf4c cursorspot-invalid.vbm
extern int cursor_spot_walkto_bm;     // 0x5fbf3c cursorspot-walkto.vbm (move_target_set_bitmap; decomp42)
extern int cursor_target_bm;          // 0x5fbf50 cursor-target.vbm (decomp42)
extern int cursor_hover_bm;           // 0x5fc018 cursor-hover.vbm (decomp42)

// ---- other units GameInit / game_cleanup call (module "-" in sum.symbols; units not known) ------------
void editor_dialog_init();            // 0x430be0 empty dialog list and its six colours
void cutscene_parse_table();          // 0x4355e0 cutscene.tbl
void cutscene_shutdown_stub();        // 0x437120 empty (push ebp / pop ebp / ret)
void alt_textures_parse_table();      // 0x4aac10 alt_textures.tbl
void alt_textures_free();             // 0x4aafb0
// decomp38 (characterinfo_parse_table): the texture list of a base name and an entry of it.
void *alt_textures_find(const char *base, int *count);               // 0x4aaff0
void *alt_textures_find_entry(const char *base, const char *name);   // 0x4ab050 (0x2d4-byte entry)
void debug_noop_cleanup();            // 0x531c80 bare ret
void vif_records_reset();             // 0x532e80 clears +0x68 of the 100 records at 0x2d064e8
void vif_reset_all();                 // 0x532ea0 (level_load; decomp42)
void script_editor_frame();           // 0x4e01f0 (game_render_frame; decomp42)

// ---- summoner_cd.cpp (0x43e660-0x43eaff) ----------------------------------------------------------
void cd_select_drive();               // 0x43e8a0 (WinMain, before MainLoop)
void CdCheck();                       // 0x43e9b0 (LoadSettings; return unused)
int  cd_has_level_data();             // 0x43e940 nonzero: the level packfiles need no adding (level_load; decomp42)
void cd_prompt_disc2();               // 0x43ea20 (level_load, when levelm.vpp is not found; decomp42)

// ---- summoner.cpp (0x43cd70-0x43e654): init, main loop, summoner.cfg (decomp26) ---------------------------
int  MainLoop();                      // 0x43d7b0 0 when the state machine reaches 3
void LoadSettings();                  // 0x43d7f0 registry, command line, subsystems, gr_init
void cmdline_read_params();           // 0x43dc30
void setup_data_paths();              // 0x43dca0
void game_shutdown_systems();         // 0x43df20
void GameInit();                      // 0x43df40
void game_cleanup();                  // 0x43e1f0
void summoner_cfg_load();             // 0x43e250
void launch_updater();                // 0x43e4a0 Summoner.exe -straight_to_update

// The screen resolutions, {w, h} per resolution_index (.rdata 0x580e38).
extern const uint resolution_table[3][2];      // unsigned (SHR at 0x43db6f; D606)
extern int resolution_index;          // 0x60ad68 0 = 640, 1 = 800, 2 = 1024

// Fonts loaded by GameInit.
extern int font_small;                // 0x60abe0 font-small.vf (also the console font)
extern int font_main;                 // 0x60ad0c sumbones640/800/1024.vf by resolution_index
extern int font_bones_small;          // 0x60ad10 SumBones-small.vf

// Command-line flags (cmdline_read_params). 0x60ad6d / 0x60ad6e are never written (labels ours).
extern bool cmdline_directsound3d;    // 0x60ad6c
extern bool cmdline_flag_60ad6d;      // 0x60ad6d
extern bool cmdline_flag_60ad6e;      // 0x60ad6e
extern bool cmdline_eax;              // 0x60ad6f
extern bool cmdline_pack;             // 0x60ad70 never read (label ours)

// ---- file path ids (file_add_path results, summoner.cpp .bss; setup_data_paths) ---------------------
extern int path_root;                 // 0x60ac54 "" (.cfg .vpp ...), summoner.cfg
extern int path_data;                 // 0x60ac60 data
extern int path_multicache;           // 0x60ad48 multicache (downloaded .cfg / .pcx)
extern int path_interface;            // 0x60ac10 data\interface\pc
extern int path_maps_characters;      // 0x60ac2c
extern int path_maps_effects;         // 0x60abb0
extern int path_maps_items;           // 0x60ac58
extern int path_maps_levels;          // 0x60ac14
extern int path_tables;               // 0x60ac5c data\tables
extern int path_models_levels_copy;   // 0x60ac18 (also stored in path_models_levels 0x59aa54)
extern int path_scripts;              // 0x60ac84
extern int path_tables_characterinfo; // 0x60acc0
extern int path_internal_copy;        // 0x60ac30 (also stored in path_internal 0x59bd74)
extern int path_players;              // 0x60ac80
extern int path_savegame;             // 0x60ac88
extern int path_tables_levels;        // 0x60abe4
extern int path_movies;               // 0x60ad08
extern int path_internal;             // 0x59bd74 (.data of another unit; label from sum.symbols)
extern int path_models_levels;        // 0x59aa54 (.data of another unit; label from sum.symbols)

// The console test sound (cmd_play_sound / cmd_stop_sound).
extern int console_sound_handle;      // 0x60ad74 snd_play_streamed result
extern int console_sound_buffer;      // 0x60ad78 snd_get_handle result (sum.symbols: void*; D600)

// ---- summoner.cfg ---------------------------------------------------------------------------------
// The 0x40-byte options block. Summoner_options (0x60acc8; sum.symbols names its fields config_*, the
// variable name is the one in cmd_gamma's help text "Sets Summoner_options.gamma_correction") is written
// to / read from summoner.cfg as a whole. new_options.cpp keeps a working copy (options_edit, 0x5c32e0,
// fields options_edit_* in sum.symbols). The constructor is inline: init_game_config_defaults (0x43d190)
// and init_options_config_defaults (0x41eea0) are the same code on the two objects. Type name (*new*).
struct SummonerOptions {
    float sfx_volume;          // +0x00 1.0
    float music_volume;        // +0x04 1.0
    bool  eax;                 // +0x08 1
    bool  invert_camera;       // +0x09 0
    bool  unk_0a;              // +0x0a 0 (*new*, never read here)
    bool  unk_0b;              // +0x0b 0 (*new*)
    bool  combat_flag;         // +0x0c 0
    bool  solo_confirm;        // +0x0d 0
    int   dialogue_camera;     // +0x10 0..2
    int   detail_14;           // +0x14 2, party model detail
    int   detail_18;           // +0x18 2, model detail
    int   texture_quality;     // +0x1c 2, 0..2
    bool  lightmaps_off;       // +0x20 0
    bool  cfg_21;              // +0x21 1
    int   detail_24;           // +0x24 2, 0..2
    int   fog_quality;         // +0x28 0, 0..2
    bool  antialias;           // +0x2c 0
    float gamma_correction;    // +0x30 1.0, 0.7..2.2
    bool  cfg_34;              // +0x34 1
    float camera_speed;        // +0x38 1.0, 1.0..2.0
    bool  cfg_3c;              // +0x3c 1

    SummonerOptions() { set_defaults(); }

    // summoner_cfg_load (0x43e295-0x43e320) writes the same values in the same order (D605).
    void set_defaults()
    {
        sfx_volume = 1.0f;
        music_volume = 1.0f;
        eax = true;
        invert_camera = false;
        unk_0a = false;
        unk_0b = false;
        combat_flag = false;
        solo_confirm = false;
        dialogue_camera = 0;
        detail_14 = 2;
        detail_18 = 2;
        texture_quality = 2;
        lightmaps_off = false;
        cfg_21 = true;
        detail_24 = 2;
        fog_quality = 0;
        antialias = false;
        gamma_correction = 1.0f;
        cfg_34 = true;
        camera_speed = 1.0f;
        cfg_3c = true;
    }
};
LAYOUT_CHECK(SummonerOptions, sizeof(SummonerOptions) == 0x40);
extern SummonerOptions Summoner_options;                             // 0x60acc8 (config_sfx_volume ..)
// A field new_hud.cpp reads under its sum.symbols name. A macro now that summoner.cpp defines
// Summoner_options: an extern bool at 0x60acd5 would have no definition (D604).
#define config_solo_confirm (Summoner_options.solo_confirm)   // 0x60acd5
void summoner_cfg_save();                                            // 0x43e410

// The combat option under its sum.symbols name (decomp23: living_combat_speed_share, combat_resolve_exchange).
extern bool config_combat_flag;                                      // 0x60acd4 (= Summoner_options.combat_flag)

#endif
