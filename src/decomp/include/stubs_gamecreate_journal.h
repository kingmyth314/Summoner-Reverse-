// stubs_gamecreate_journal.h - declarations that new_gamecreate.cpp and new_journal.cpp need beyond
// stubs_menus_dialogue.h and ui_screen.h. Names and addresses from symbols/sum.symbols; argument lists
// read from the call sites in Sum.exe v1.40 (build/sum.asm). "(type guessed)" marks return or parameter
// types that the call sites do not show.
#ifndef STUBS_GAMECREATE_JOURNAL_H
#define STUBS_GAMECREATE_JOURNAL_H

#include "stubs_menus_dialogue.h"   // gr_*, bm_*, mouse_*, ui_*, interface_*, game_*, mp_local_player
#include "types.h"                  // quest_info

// ---- DirectInput scan codes (the ones stubs_menus_dialogue.h does not define) --------------------
#define KEY_BACKSPACE 0x0e
#define KEY_R         0x13
#define KEY_I         0x17
#define KEY_B         0x30
#ifndef KEY_ENTER
#define KEY_ENTER     0x1c
#endif

// ---- vsdk\os\timestamp.cpp ------------------------------------------------------------------------
// Same class as in stubs_credits_help.h (which cannot be included together with stubs_menus_dialogue.h:
// both define Color). One dword, -1 = invalid; no destructor, so the static initialiser needs no atexit.
#ifndef STUBS_CREDITS_HELP_H
class RealtimeTimer {
public:
    RealtimeTimer();                  // 0x5019e0
    void SetTimeout(int ms);          // 0x501a00
    void invalidate();                // 0x501a80
    bool elapsed();                   // 0x501a90
    bool valid();                     // 0x501b10
private:
    int value;
};

int  gr_split_str(int *lengths, int *starts, const char *s, int max_w,
                  int max_lines, char ignore_char, int font);       // 0x509130
void mouse_get_delta(int *dx, int *dy, int *dz);                     // 0x5013c0
#endif

// ---- vsdk\gr ----------------------------------------------------------------------------------------
void gr_get_clip(int *x, int *y, int *w, int *h);                    // 0x506970
void gr__set_clip(int x, int y, int w, int h);                       // 0x506840
void gr_string_truncate_ellipsis(char *dest, const char *s, int max_w, int font); // 0x5153b0 (return unused)

// ---- vsdk\os\key ------------------------------------------------------------------------------------
int  key_to_ascii(int key);                                          // 0x509ca0, 0xff = no character

// ---- localize ---------------------------------------------------------------------------------------
char *localize_quest_name(const char *name);                         // 0x505070

// ---- new_charselect.cpp: the character profile records (0x154 bytes at 0x5b6e78) ----------------------
struct mp_char_profile {
    unsigned char _unk00[0x4c];
    unsigned int flags;               // +0x4c 0x4000000 / 0x8000000 enable difficulty 1 / 2 on screen 17
    unsigned char _unk50[0x104];
};
mp_char_profile *mp_charselect_get_selected_profile(int validate);  // 0x408540

// mp_player in stubs_menus_dialogue.h stops at +0x2c; the name the game list shows is at +0x34
// (mp_local_player_create copies it from the profile). Read through this macro instead.
#define MP_PLAYER_NAME(p) ((char *)(p) + 0x34)

// ---- multiplayer ------------------------------------------------------------------------------------
void mp_host_game(const char *name, const char *password, int max_players, int difficulty,
                  int levels_below, int levels_above);               // 0x43edc0 (return type guessed)

// ---- new_buysell.cpp / statscreen -------------------------------------------------------------------
void shop_set_shopkeeper_from_char(void *character);                 // 0x406360 (always 0 here; type guessed)
bool ui_has_owner_character();                                       // 0x406390
void statscreen_refresh_anim(Character *c);                          // 0x412e50 (always 0 here; type guessed)

// ---- savegame / quests ------------------------------------------------------------------------------
class savegame {                      // savegame_state at 0x2493a38 (layout not needed here)
public:
    int get_quest_bit(int quest);     // 0x4d0c00 savegame_get_quest_bit, 0 or 1 in EAX (+0x103c bit n)
};
extern savegame savegame_state;                                      // 0x2493a38
int quest_get_current_step(int quest);                               // 0x4d6080, -1 = not active
extern quest_info quest_table[200];                                  // 0x253ef40 (events_parse_table's .bss)
extern int quest_count;                                              // 0x25443a4

#endif
