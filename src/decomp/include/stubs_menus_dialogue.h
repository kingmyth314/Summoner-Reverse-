// engine_stubs.h - declarations of the Sum.exe v1.40 engine, game and UI functions and globals that
// new_playmenu.cpp, new_mainmenu.cpp and new_dialogue.cpp use but do not define.
// Names and addresses from symbols/sum.symbols; argument lists read from the call sites (build/sum.asm).
// Return and parameter types that the call sites do not show are marked "(type guessed)".
#ifndef STUBS_MENUS_DIALOGUE_H
#define STUBS_MENUS_DIALOGUE_H

#include "types.h" // symbols/types.h: Character, level_script_info

// ---- DirectInput scan codes used by the screens ------------------------------------------------
#define KEY_ESC 0x01
#define KEY_Q   0x10
#define KEY_S   0x1f
#define KEY_L   0x26
#define KEY_N   0x31
#define KEY_M   0x32
#define KEY_F1  0x3b
#define KEY_F2  0x3c

// ---- vsdk\gr ----------------------------------------------------------------------------------------
class Color {                                  // 4 bytes (r, g, b, a); constructed by static initialisers elsewhere
public:
    unsigned char r, g, b, a;
};
void gr_set_color(int r, int g, int b, int a);                      // 0x506b90
void gr_set_color_ptr(Color *c);                                    // 0x506c10
void gr_string(int x, int y, const char *s, int font, unsigned int mode);   // 0x508c60
void gr_get_string_size(int *w, int *h, const char *s, int len, int font);  // 0x508360
int  gr_get_font_height(int font);                                  // 0x508300
extern unsigned int gr_string_mode;                                 // 0x25d8df4
extern unsigned int gr_bitmap_wrap_mode;                            // 0x25d88c4

// ---- vsdk\bmpman ------------------------------------------------------------------------------------
int  bm_load(const char *name, int path_id, int unk);               // 0x5026e0 (third argument always -1 here)
void bm_get_dimensions(int bm, int *w, int *h);                     // 0x503500
int  progress_update(int check_keys);                               // 0x503e80 (type guessed)

// ---- vsdk\os\mouse ----------------------------------------------------------------------------------
bool mouse_is_down(int button);                                     // 0x5012c0
bool mouse_was_pressed(int button);                                 // 0x501360

// ---- interface / ui helpers (new_hud.cpp, new_interface.cpp, ...) -----------------------------------
float ui_get_scale();                                               // 0x4134a0
bool  point_in_rect(int px, int py, int x, int y, int w, int h);    // 0x413460
void  ui_bitmap_scaled(int bm, int x, int y, unsigned int mode);    // 0x4144f0
void  ui_message_box_show(const char *title, const char *text, int x, int y, int num_buttons,
                          char **buttons, void (*callback)(int), int flags);  // 0x413e40 (last argument: 0 or 2)
bool  ui_message_box_is_active();                                   // 0x4144d0
void  ui_screen_stop_player();                                      // 0x41e380
void  interface_set_screen(int id, void *arg);                      // 0x412620
void  interface_close_screen();                                     // 0x416a10
void  hud_show_message(const char *text);                           // 0x414a20
void  hud_key_help();                                               // 0x40e260
void  empty_frame_hook_40acc0();                                    // 0x40acc0 (bare ret, own copy)
void  empty_function();                                             // 0x4f13e0 (bare ret shared by identical-code folding)
extern int   ui_hires;                                              // 0x5bd318
extern char *ui_bitmap_suffix_tga[2];                               // 0x58bda4 {"640.tga", "1024.tga"}
extern int   path_interface;                                        // 0x60ac10
extern int   resolution_index;                                      // 0x60ad68
extern Color ui_color_highlight;                                    // 0x5bd808
extern Color ui_color_text;                                         // 0x5bd2ec
extern char *localize_strings[1000];                                // 0x25cd4dc

// ---- game -------------------------------------------------------------------------------------------
int  game_play_sound(int sound, float volume, int a, int b, int c); // 0x468d90 (a, b, c always 0 here)
void sound_play_2d(int sound, float volume);                        // 0x468dc0
void game_pause(int a, int b);                                      // 0x432660 (both 0 here)
void game_unpause();                                                // 0x4326c0
bool game_is_paused();                                              // 0x432da0
void gameseq_set_state(int state);                                  // 0x401020
void game_change_level(const char *filename, void *arg);            // 0x431a40 (second argument type guessed)
void goto_credits();                                                // 0x4ef6a0
int  savegame_location_allowed();                                   // 0x4d4a50 (0 allowed, 1 combat, 2 level, 3 summon)
void level_format_display_name(char *out, int level);               // 0x4d63b0
void movie_play(const char *filename);                              // 0x5172f0
bool snd_is_eax_enabled();                                          // 0x521ba0
int  music_find_by_name(const char *name);                          // 0x4696d0
void music_play(int index, int loop);                               // 0x469080
Character *living_from_handle(int handle);                          // 0x46b1a0
void character_xp_death_penalty(Character *c);                      // 0x441710
void mp_player_disconnected(int id, int reason);                    // 0x4814e0

extern int   game_flags;                                            // 0x5fbfd8 (1 single player, 2 multiplayer, ...)
extern int   game_mouse_x;                                          // 0x5fbfdc
extern int   game_mouse_y;                                          // 0x5fbfe0
extern unsigned int player_input[3];                                // 0x5fcc98 {flags, mouse x, mouse y}
extern Character *cur_character;                                    // 0x5fcc88
extern int   cur_level_index;                                       // 0x59f120
extern level_script_info level_script_table[51];                    // 0x59f128
extern void *conversation_save_table[357 * 4];                      // 0x25388c0 (see NOTES: 0x2538a10)
extern bool  play_intro_movies;                                     // 0x5930bd

// The multiplayer player record (partial; only the fields read here).
struct mp_player_object {
    unsigned char _unk00[0x0c];
    int handle;                                                     // +0x0c
};
struct mp_player {
    unsigned char _unk00[0x10];
    unsigned int flags;                                             // +0x10 bit 0 host
    unsigned char _unk14[0x14];
    mp_player_object *object;                                       // +0x28
};
extern mp_player *mp_local_player;                                  // 0x651598

// ---- vsdk\net ---------------------------------------------------------------------------------------
extern int nw_network_status;                                       // 0x270812c
extern int nw_networking_ok;                                        // 0x2710e9c
extern int nw_winsock_started;                                      // 0x2710ea4

#endif
