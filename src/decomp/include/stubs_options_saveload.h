// stubs_options_saveload.h - declarations that new_options.cpp and new_saveload.cpp need beyond
// stubs_menus_dialogue.h, ui_screen.h and vsdk/timestamp.h. Names and addresses from symbols/sum.symbols;
// argument lists read from the call sites in Sum.exe v1.40 (build/sum.asm). "(type guessed)" marks return
// or parameter types that the call sites do not show. (*new*) marks names that sum.symbols does not have.
//
// Not includable together with stubs_gamecreate_journal.h (both define class savegame; the one there has
// only get_quest_bit) or with any header that defines class File (stubs_sound_items_registry.h,
// stubs_container_targa.h). The plain function declarations repeated from stubs_gamecreate_journal.h
// (key_to_ascii, gr_split_str, ...) are identical, so they do not clash.
#ifndef STUBS_OPTIONS_SAVELOAD_H
#define STUBS_OPTIONS_SAVELOAD_H

#include <time.h>
#include "stubs_menus_dialogue.h"   // gr_*, bm_*, mouse_*, ui_*, interface_*, game_*, localize_strings
#include "ui_screen.h"              // ui_button_def, ui_button_state
#include "timestamp.h"              // vsdk/timestamp.h: RealtimeTimer

// ---- DirectInput scan codes (the ones stubs_menus_dialogue.h does not define) --------------------
#ifndef KEY_BACKSPACE
#define KEY_BACKSPACE 0x0e
#endif
#ifndef KEY_ENTER
#define KEY_ENTER     0x1c
#endif

#ifndef XSTR
#define XSTR(text, id) (localize_strings[id])
#endif

// ---- summoner.cfg block (summoner.cpp) -------------------------------------------------------------
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
extern SummonerOptions Summoner_options;                             // 0x60acc8 (config_sfx_volume ..)
void summoner_cfg_save();                                            // 0x43e410
extern int path_savegame;                                            // 0x60ac88

// ---- detail / gr settings ---------------------------------------------------------------------------
void options_apply_detail();                                         // 0x4872d0
void options_draw_version();                                         // 0x4146c0 (interface unit, not options)
extern unsigned int pxo_cfg_flags;                                   // 0xa59a88
extern bool gr_antialias_enabled;                                    // 0x25d921c
extern unsigned int gr_mode_lightmap_multitexture;                   // 0x25d8d1c gr__Mode
bool gr_is_mode_supported(const unsigned int *mode);                 // 0x5079b0 (by address; type guessed)
void gr_set_gamma(float gamma);                                      // 0x506a90
void gr_rect(int x, int y, int w, int h, unsigned int mode, float a6); // 0x5077d0 (a6 always 1.0 here)
extern unsigned int gr_rect_mode;                                    // 0x25d88ac
extern Color ui_color_dark_green;                                    // 0x5bd8f0 (8, 100, 0, 255)

// ---- sound ------------------------------------------------------------------------------------------
void snd_set_group_volume(int group, float volume);                  // 0x5218e0 (0 = effects)
void music_set_volume(float volume);                                 // 0x4691b0
void eax_restore_reverb();                                           // 0x522090
void eax_mute_reverb();                                              // 0x522020

// ---- ui choice popup (new_hud.cpp area) -------------------------------------------------------------
bool ui_choice_popup_is_active();                                    // 0x413e20
void ui_choice_popup_draw();                                         // 0x413c70

// ---- vsdk\os ----------------------------------------------------------------------------------------
int  key_to_ascii(int key);                                          // 0x509ca0, 0xff = no character
bool mouse_was_double_clicked(int button);                           // 0x5012e0
void mouse_get_delta(int *dx, int *dy, int *dz);                     // 0x5013c0

// ---- vsdk\gr ----------------------------------------------------------------------------------------
void gr_get_clip(int *x, int *y, int *w, int *h);                    // 0x506970
void gr__set_clip(int x, int y, int w, int h);                       // 0x506840
int  gr_split_str(int *lengths, int *starts, const char *s, int max_w,
                  int max_lines, char ignore_char, int font);       // 0x509130
void gr_string_truncate_ellipsis(char *dest, const char *s, int max_w, int font); // 0x5153b0 (return unused)
void gr_bitmap_stretched(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                         unsigned int mode);                         // 0x506ec0
void gr_setup_3d(matrix3 *orient, vector3 *pos, float fov, int zbuffer, int a5); // 0x50dec0 (1, 1 here)
void gr_clear();                                                     // 0x5069c0
int  gr_read_backbuffer(int x, int y, int w, int h, void *buf);      // 0x507d30 -> pixel format (3 565, 7 32-bit)
void gr_mark_texture_dirty(int bm);                                  // 0x506c70
int  bm_create(int format, int w, int h, void *data, void *palette, int a6); // 0x503cf0

// ---- level rendering / camera -----------------------------------------------------------------------
void levelrender_render_scene();                                     // 0x482200
extern vector3 camera_view_pos;                                      // 0x60a290 (game_camera +0x278)
extern matrix3 camera_view_orient;                                   // 0x60a29c (game_camera +0x284)
// game_camera +0x18c. The object game_camera (0x60a018) cannot be declared under its own name next to
// types.h's typedef of the same name, so the field is reached through this alias. (*new*)
extern float camera_fov;                                             // 0x60a1a4

// ---- localize / level names -------------------------------------------------------------------------
char *localize_string_or_invalid(char **p);                          // 0x5054f0: *p, or "INVALID"
struct level_display_name {          // (*new* type) {level file name, localized display name slot}
    const char *name;
    char **text;
};
extern level_display_name level_display_names[52];                   // 0x594440 (51 used, cmp 0x33)
int level_script_find(const char *level_name);                       // 0x4d6300, -1 = none
int level_script_find_index(int level, const char *script_name);     // 0x4d6340, -1 = none

// ---- vsdk\vfile -------------------------------------------------------------------------------------
class File {                         // 0x114 bytes
public:
    File();                                                          // 0x5140d0
    ~File();                                                         // 0x5140f0
    bool find(const char *name, int path_id);                        // 0x5141c0 (returns AL)
    int  Delete(const char *name, int path_id);                      // 0x514a80 File__delete, 0 = removed
private:
    char _data[0x114];
};

// file_find_files fills a circular list of 0x38-byte nodes (operator_new) that the caller frees.
struct file_find_node {              // (*new* type)
    file_find_node *next;            // +0x00
    file_find_node *prev;            // +0x04
    char name[0x2c];                 // +0x08 file name (with extension here: flags 6)
    time_t time;                     // +0x34 _finddata_t time_write (VC6 time_t is 32-bit)
};
struct file_find_list {              // list head (*new* type)
    file_find_node *next;
    file_find_node *prev;
};
void file_find_files(file_find_list *list, int path_id, const char *pattern, int flags,
                     void *callback);                                // 0x5133b0 (return unused)

// ---- savegame ---------------------------------------------------------------------------------------
class savegame {                     // savegame_state at 0x2493a38 (thiscall members)
public:
    int  get_quest_bit(int quest);                                   // 0x4d0c00 (stubs_gamecreate_journal.h)
    // Header only. version may be 0. Nonzero = the header is valid.
    int  read_info(const char *filename, char *level_name, char *script_name, int *unk_a8,
                   float *level_time, char *description, int *version); // 0x4d2800
    void read_thumbnail_file(const char *filename, int unk);         // 0x4d2910 (return unused)
    int  write(const char *filename, int unk, const char *description); // 0x4d1c40, nonzero = written
    void set_pending_load(const char *filename, int slot);           // 0x4d3870
};
extern savegame savegame_state;                                      // 0x2493a38
extern int savegame_thumbnail_bm;                                    // 0x59e794 (201x151, savegame section)

// conversation storage object (conversation_db, 0x25461a0; sum.symbols tags it new_saveload.cpp, but it is
// not in this unit's .bss). Class name (*new*).
class conversation_storage {
public:
    void save_state();                                               // 0x4d8390 conversation_save_state
};
extern conversation_storage conversation_db;                         // 0x25461a0

#endif
