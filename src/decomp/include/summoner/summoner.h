// summoner/summoner.h - the game's top level: state sequencing, game flags, pause, level changes, the
// summoner.cfg options and the file path ids. Units: summoner.cpp (Summoner_options, summoner_cfg_save,
// the path ids), gameplay.cpp (0x4340d0-0x439ba0 by string ids; game_pause 0x432660), gametarget.cpp
// (0x43a390-0x43cb60) and the state sequencer (gameseq_set_state 0x401020). One header for now; file name
// ours. Not original source.
#ifndef SUMMONER_SUMMONER_H
#define SUMMONER_SUMMONER_H

#include "vsdk/vsdk.h"

// ---- game flags (0x5fbfd8) --------------------------------------------------------------------------
extern int game_flags;                // 0x5fbfd8
#define GF_SINGLE_PLAYER 0x1
#define GF_MULTIPLAYER   0x2
#define GF_LEADER_SOUND  0x200        // party_set_leader plays sound 0x28 when set (meaning unsure)
#define GF_SOLO_MODE     0x1000

extern int game_mouse_x;              // 0x5fbfdc
extern int game_mouse_y;              // 0x5fbfe0

// ---- state sequencing, pause, levels ---------------------------------------------------------------
void gameseq_set_state(int state);    // 0x401020
int  gameseq_get_state();             // 0x401040 (4 = game play)
void game_pause(int a, int b);        // 0x432660 (both 0 in the pilots)
void game_unpause();                  // 0x4326c0
bool game_is_paused();                // 0x432da0
void game_change_level(const char *filename, void *arg);   // 0x431a40 (second argument type guessed)
extern bool play_intro_movies;        // 0x5930bd
void combat_mode_set(int on);         // 0x439580 (gameplay.cpp range; decomp20: conversation_frame passes 0)

// The current level's names and layer maps (set by game_change_level / level_parse_table; module "-").
extern char level_script_name[32];             // 0x60a2e8
extern int  level_num_layers;                  // 0x60a340
extern int  level_layer_maps[10];              // 0x60a344 automap bitmap of each layer
extern int  level_layer_automap_index[40];     // 0x60a36c

// ---- gameplay.cpp: party control (0x439580-0x4396d0) -------------------------------------------------
void combat_mode_set(bool on);                 // 0x439580
void party_toggle_hold();                      // 0x439620
void party_select_member(class Player *p);     // 0x4396d0

// ---- gametarget.cpp: world picking (0x43a370..; .bss 0x60ab68-0x60ab90) -------------------------------
class Character;
void world_pick_set_skill_filter(int skill);   // 0x43a370
void world_pick_clear_filter();                // 0x43a390
extern bool  world_pick_portrait_clicked;      // 0x60ab68
extern Character *world_hover_portrait_character;   // 0x60ab6c (formerly world_pick_target_mode; D47)
extern int   world_interact_kind;              // 0x60ab88 1..4
extern void *world_hover_object;               // 0x60ab8c
extern void *world_interact_object;            // 0x60ab90

// Empty functions the screens call (identical-code folding shares bare RETs between units).
void empty_frame_hook_40acc0();       // 0x40acc0 (bare ret, own copy)
void empty_function();                // 0x4f13e0 (bare ret shared by identical-code folding)

// ---- mouse cursor (gameplay_load_cursors / cursor_set_type 0x432e50; unit not known) ---------------------
void cursor_set_type(int bm);         // 0x432e50 cursor_type (0x5931b4) = bm
extern int cursor_url_bm;             // 0x5fbf2c cursor-url.vbm
extern int cursor_walk_bm;            // 0x5fbfd4 cursor-walk.vbm (the default)

// ---- file path ids (file_add_path results, summoner.cpp .bss) -----------------------------------------
extern int path_interface;            // 0x60ac10 data\interface\pc
extern int path_tables;               // 0x60ac5c data\tables
extern int path_players;              // 0x60ac80
extern int path_savegame;             // 0x60ac88
extern int path_multicache;           // 0x60ad48 multicache (downloaded .cfg / .pcx)
extern int resolution_index;          // 0x60ad68 0 = 640, 1 = 800, 2 = 1024

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

    SummonerOptions()
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
// A field new_hud.cpp reads under its sum.symbols name (as camera_fov in camera.h, DECISIONS.md D19).
extern bool config_solo_confirm;                                     // 0x60acd5 (= Summoner_options.solo_confirm)
void summoner_cfg_save();                                            // 0x43e410

#endif
