// stubs_charselect_levelselect.h - declarations that new_charselect.cpp and new_levelselect.cpp need beyond
// stubs_menus_dialogue.h, ui_screen.h, stubs_gamecreate_journal.h and vsdk/registry.h. Names and addresses
// from symbols/sum.symbols; argument lists read from the call sites in Sum.exe v1.40 (build/sum.asm).
// "(type guessed)" marks return or parameter types that the call sites do not show.
#ifndef STUBS_CHARSELECT_LEVELSELECT_H
#define STUBS_CHARSELECT_LEVELSELECT_H

#include "stubs_gamecreate_journal.h"   // stubs_menus_dialogue.h (and types.h), RealtimeTimer, gr clip, key_to_ascii

#ifndef PATH_ANY
#define PATH_ANY 9999999                // 0x98967f, the "search every path" id
#endif

// ---- localize: XSTR as in stubs_credits_help.h (not includable here, see NOTES) ---------------------
#ifndef XSTR
#define XSTR(text, id) (localize_strings[id])
#endif

// ---- vsdk\os\mouse ----------------------------------------------------------------------------------
bool mouse_was_double_clicked(int button);                           // 0x5012e0

// ---- interface colours (Color from stubs_menus_dialogue.h) -------------------------------------------
extern Color ui_color_blue;                                          // 0x5bd328 (110, 138, 177)
extern Color ui_color_disabled;                                      // 0x5bd794 (90, 90, 90)
extern Color ui_color_magenta;                                       // 0x5bd928 (190, 80, 190)

// ---- vsdk\vfile -------------------------------------------------------------------------------------
// File (0x114 bytes) again: the other File declarations live in headers that cannot be included next to
// stubs_menus_dialogue.h (see NOTES). Only the two members these units call.
class File {
public:
    File();                                                          // 0x5140d0
    ~File();                                                         // 0x5140f0
    bool find(const char *name, int path_id);                        // 0x5141c0
    int  Delete(const char *name, int path_id);                      // 0x514a80 File__delete (0 = deleted;
                                                                     // "delete" is a keyword)
private:
    char _data[0x114];
};

// One entry of the list that file_find_files builds (0x38 bytes from operator new). The list head is a
// whole entry on the caller's stack; the list is circular through next.
struct file_find_entry {
    file_find_entry *next;                                           // +0x00
    file_find_entry *prev;                                           // +0x04
    char name[0x30];                                                 // +0x08 file name
};
void file_find_files(file_find_entry *list, int path_id, const char *pattern, int flags,
                     void *callback);                                // 0x5133b0 (flags 5, callback 0 here; types guessed)
int  file_make_path_dirs(int path_id);                               // 0x5131e0 nonzero = failure
extern int path_players;                                             // 0x60ac80

// ---- characterinfo.cpp ------------------------------------------------------------------------------
extern char_info char_infos[256];                                    // 0x6d7e50 (char_info from types.h)
int charinfo_find_by_species(int species);                           // 0x446bd0 index or -1

// ---- .plr files (item_paging.cpp in sum.symbols) ----------------------------------------------------
// The character record of new_charselect.cpp's list (mp_char_profiles, 0x154 bytes). This is the
// record that stubs_gamecreate_journal.h declares as mp_char_profile with only the flags at +0x4c.
struct mp_char_record {
    char filename[0x28];                                             // +0x000 from file_find_files
    char name[0x20];                                                 // +0x028 mp_player_file_read_summary fills from here
    int  char_info;                                                  // +0x048 char_infos index
    unsigned int flags;                                              // +0x04c difficulty unlocks (screen 17)
    char description[0x100];                                         // +0x050 the text typed on screen 16
    short level;                                                     // +0x150
    short _pad152;
};
// (out, filename): out is the record + 0x28. 0 = read; sum.symbols' "(path, out)" has the order reversed.
int mp_player_file_read_summary(void *out, const char *filename);    // 0x46d450
// 0 created, 1 player exists, 2 no player directory, 3 cannot write (jump table 0x4078a0).
int mp_player_file_create(const char *name, int char_info, const char *description);   // 0x46cc80

// ---- multiplayer session ----------------------------------------------------------------------------
void mp_session_reset();                                             // 0x441490
extern unsigned int pxo_cfg_flags;                                   // 0xa59a88 bit 0: back to state 11, else 12

// The player list is circular through +0x00, head mp_player_list (a whole record). mp_player in
// stubs_menus_dialogue.h has neither next (+0x00) nor the type (+0x14); read them through these macros
// like MP_PLAYER_NAME (+0x34).
#define MP_PLAYER_NEXT(p) (*(mp_player **)(p))
#define MP_PLAYER_TYPE(p) (*(int *)((char *)(p) + 0x14))            // 3 = a player listed on screen 18
extern mp_player mp_player_list;                                     // 0x651418 head
extern int mp_netgame_level;                                         // 0x6513d8
extern int mp_game_difficulty;                                       // 0x6513ec
extern bool All_multi_levels;                                        // 0x60ae64
bool mp_level_available_to_all(int level, int difficulty);           // 0x43ebd0
void mp_levelselect_set_level(int level);                            // 0x43ec20
void mp_levelselect_start_if_ready();                                // 0x43ec40 (return type guessed)
void mp_remove_player(mp_player *p, int reason);                     // 0x471df0 (reason 1 = kicked)
void chat_send_line(const char *text);                               // 0x488070
void trim_whitespace(char *dest, const char *src, int max_length);   // 0x514fb0 (return unused)

// ---- level table ------------------------------------------------------------------------------------
struct level_table_entry {                                           // 0x10 bytes
    int id;                                                          // +0x0
    char **display_name;                                             // +0x4 localize_strings slot
    unsigned int mp_flags;                                           // +0x8 bit 0: listed on screen 18
    int unk0c;                                                       // +0xc
};
extern level_table_entry level_table[26];                            // 0x59fde8
char *level_table_display_name(int level);                           // 0x4d64c0 NULL when unknown

#endif
