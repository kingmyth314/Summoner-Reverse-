// engine_stubs.h - declarations of the engine (vsdk and game) functions, classes and globals
// that new_credits.cpp and new_help.cpp use. Names from symbols/sum.symbols; signatures are
// inferred from the call sites in Sum.exe v1.40 (argument counts and pushes), not from any
// reference source. Only what these two units need is declared.
//
// This header deliberately does not include symbols/types.h: types.h describes TextFile as a C
// struct, while the game calls it as a C++ class with thiscall methods.
#ifndef STUBS_ASSESS_PICKUP_H
#define STUBS_ASSESS_PICKUP_H

// ---------------------------------------------------------------- C runtime
extern "C" {
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
char *strncpy(char *dst, const char *src, unsigned int n);   // 0x56c810
unsigned int strlen(const char *s);
int tolower(int c);                                          // 0x56cebe
int sprintf(char *buf, const char *fmt, ...);                // 0x56c740
}

// ---------------------------------------------------------------- vsdk\os\timestamp.cpp
// RealtimeTimer: one dword, -1 = invalid. Constructor 0x5019e0 (no destructor, so no atexit).
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

// ---------------------------------------------------------------- vsdk\parse\parse.cpp
struct TextFilePosition {             // 8 bytes on the stack of credits_parse_table: {line, cursor}
    int line;
    char *cursor;
};

class TextFile {                      // 0x50 bytes (stack frame of credits_parse_table)
public:
    TextFile();                                         // 0x515960
    ~TextFile();                                        // 0x5159d0 (thunk to Close)
    int  Open(const char *name, int path_id);           // 0x515d20, nonzero = failure
    void Close();                                       // 0x515e90
    void Rewind();                                      // 0x515ec0
    void SkipTo(const char *str, const char *stop);     // 0x515ba0 (return value unused here)
    void Expect(const char *str);                       // 0x5161e0
    bool Accept(const char *str);                       // 0x516110
    bool ReadLine(char *dest, int dest_size);           // 0x516010
    void GetPosition(TextFilePosition *pos);            // 0x515ce0
    void SetPosition(TextFilePosition *pos);            // 0x515d00
private:
    char *buffer;                     // +0x00
    char *cursor;                     // +0x04
    unsigned char flag08;             // +0x08
    unsigned char unmapped[0x3f];     // +0x09
    int line;                         // +0x48
    int size;                         // +0x4c
};

// ---------------------------------------------------------------- vsdk\os\memory.cpp
struct Mempool {
    unsigned char *destination;       // +0x00
    unsigned int size;                // +0x04
    unsigned int offset;              // +0x08 (credits saves and restores it)
};
extern Mempool level_mempool;                            // 0x22e43c0
void set_current_mempool(Mempool *pool);                 // 0x5127f0
void *operator_new(unsigned int size);                   // 0x512550 (the game's operator new)

// ---------------------------------------------------------------- vsdk\gr
typedef unsigned int gr_mode;                            // gr__Mode, passed by value as a dword
struct Color {
    unsigned char r, g, b, a;
};
extern gr_mode gr_bitmap_wrap_mode;                      // 0x25d88c4
extern gr_mode gr_string_mode;                           // 0x25d8df4
int  gr_screen_height();                                 // 0x506180
void gr_set_color(int r, int g, int b, int a);           // 0x506b90
void gr_set_color_ptr(Color *c);                         // 0x506c10
void gr_set_alpha(int a);                                // 0x506c40
void gr_fade_out(int ms);                                // 0x5065e0
void gr_fade_in(int ms);                                 // 0x506630
int  gr_get_font_height(int font);                       // 0x508300
void gr_get_string_size(int *w, int *h, const char *s, int len, int font);   // 0x508360
void gr_string(int x, int y, const char *s, int font, gr_mode mode);         // 0x508c60, x 0x8000 = centred
int  gr_split_str(int *lengths, int *starts, const char *s, int max_w,
                  int max_lines, char ignore_char, int font);                // 0x509130
int  bm_load(const char *name, int a, int b);            // 0x5026e0 (called with -1, -1)

// ---------------------------------------------------------------- localize
extern char *localize_strings[1000];                     // 0x25cd4dc
#define XSTR(text, id) (localize_strings[id])
char *localize_get_language_name();                      // 0x505510
void  localize_remap_charset(char *text, int to_font);   // 0x5051a0
char *localize_string_or_invalid(char **p);              // 0x5054f0: *p, or "INVALID"

// ---------------------------------------------------------------- game / ui
extern int   ui_hires;                                   // 0x5bd318, 0 = 640 layout, 1 = 1024
extern int   resolution_index;                           // 0x60ad68, 0..2
extern float reported_frametime;                         // 0x5a5740
extern int   game_flags;                                 // 0x5fbfd8, bit 0 single player
extern Color ui_color_text;                              // 0x5bd2ec
extern Color ui_color_white;                             // 0x5bd324
extern Color ui_color_highlight;                         // 0x5bd808
extern Color ui_color_green;                             // 0x5bd8f8

void gameseq_set_state(int state);                       // 0x401020
void ui_bitmap_scaled(int bm, int x, int y, gr_mode mode);   // 0x4144f0
void interface_set_screen(int id, int arg);              // 0x412620
void ui_screen_stop_player();                            // 0x41e380
void game_pause(int a, int b);                           // 0x432660
void game_unpause();                                     // 0x4326c0
bool game_is_paused();                                   // 0x432da0
int  game_play_sound(int handle, float volume, int a, int b, int c);   // 0x468d90
void music_play(int index, int loop);                    // 0x469080
void music_stop(int fade_ms);                            // 0x469180
int  music_find_by_name(const char *name);               // 0x4696d0
void movie_play(const char *file);                       // 0x5172f0
void mouse_get_delta(int *dx, int *dy, int *dz);         // 0x5013c0

// DirectInput scan codes used by the screen frame functions
#define KEY_ESC       0x01
#define KEY_ENTER     0x1c
#define KEY_SPACE     0x39
#define KEY_PAD8      0x48
#define KEY_PAD4      0x4b
#define KEY_PAD6      0x4d
#define KEY_PAD2      0x50
#define KEY_UP        0xc8
#define KEY_PAGEUP    0xc9
#define KEY_LEFT      0xcb
#define KEY_RIGHT     0xcd
#define KEY_DOWN      0xd0
#define KEY_PAGEDOWN  0xd1

// ---------------------------------------------------------------- this pilot's two units
// (would be new_credits.h / new_help.h; prototypes needed because the binary's function order
// has callers before callees)
void credits_init();
void credits_parse_table();
void credits_exit();
void credits_get_line_size(int *w, int *h, char *s);
void credits_leave();
int  credits_frame(int key);
void credits_finish();
void credits_draw();
int  credits_draw_lines(int w, int y, char *s);

void help_reset_page();
void help_init(bool in_game);
void help_leave();
int  help_frame(int key);
void help_prev_page();
void help_next_page();
void help_draw();

// ================================================================ added for new_assess.cpp / new_pickup.cpp
// Everything below was added by the second batch (assess and pickup screens). Signatures inferred
// from the call sites in Sum.exe v1.40 like the ones above.

// ---------------------------------------------------------------- C runtime (more)
extern "C" {
unsigned int strcspn(const char *s, const char *reject);    // 0x56c910
int _stricmp(const char *a, const char *b);                  // 0x57ae80
void *memset(void *dst, int c, unsigned int n);              // intrinsic under /O2 (four dword stores)
}

// windef.h style min / max (the game's code has the setle / dec / and pattern of `a > b ? a : b`)
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// ---------------------------------------------------------------- vsdk (more)
namespace gr {
void set_clip(int x, int y, int w, int h);                   // 0x506840 gr__set_clip
}
void gr_get_clip(int *x, int *y, int *w, int *h);            // 0x506970
void bm_get_dimensions(int bm, int *w, int *h);              // 0x503500
int  progress_update(int check_keys);                        // 0x503e80 (called with 0)
bool mouse_is_down(int button);                              // 0x5012c0
bool mouse_was_pressed(int button);                          // 0x501360

// ---------------------------------------------------------------- ui helpers (more)
extern int   path_interface;                                 // 0x60ac10 path id of data\interface\pc
extern char *ui_bitmap_suffix_tga[2];                        // 0x58bda4 {"640.tga", "1024.tga"}
extern int   game_mouse_x;                                   // 0x5fbfdc
extern int   game_mouse_y;                                   // 0x5fbfe0
extern unsigned int player_input[3];                         // 0x5fcc98 {flags, x, y}; flags bit 0x10 blocks screen input
float ui_get_scale();                                        // 0x4134a0
bool  point_in_rect(int px, int py, int x, int y, int w, int h);  // 0x413460
void  interface_close_screen();                              // 0x416a10 interface_set_screen(0, 0)
void  hud_show_message(char *text);                          // 0x414a20
int   sound_play_2d(int sound, float volume);                // 0x468dc0

// A screen button definition in .rdata (0x40 bytes; assess 0x57e4c0, pickup 0x580220): the bitmap
// base name (suffix "640.tga" / "1024.tga" appended), the position per resolution_index and flags
// (1 = fires on release, 4 = fires on press and auto-repeats; the "...Press" entries are 0).
struct screen_button_def {
    char name[0x24];                  // +0x00
    struct { int x, y; } pos[3];      // +0x24
    int  flags;                       // +0x3c
};

// The run-time record of one button (0x18 bytes). flags: 1 always drawn (background, index 0),
// 2 highlighted this frame (cleared by the frame function, drawn when set), 4 disabled.
struct screen_button {
    int flags;                        // +0x00
    int bitmap;                       // +0x04
    int x, y;                         // +0x08, +0x0c
    int w, h;                         // +0x10, +0x14 bitmap size times ui_get_scale()
};

// ---------------------------------------------------------------- characters (assess screen)
struct ItemType {
    const char *name;                 // +0x00
    unsigned char _unmapped04[0x64];
    unsigned short damage;            // +0x68 $Damage: (zero-extended where read, so unsigned)
};

struct item_info {                    // an item record: the equipped items and item objects point at one
    int _unk00;
    ItemType *type;                   // +0x04
};

struct resistance_set {               // char_info +0xf8, copied whole (rep movsd 7) by assess_format_info
    int level[7];                     // per damage type (fire, ice, energy, magic, slashing, piercing, blunt):
                                      // index into resistance_level_names, 2 = standard
};

struct char_info {
    char *name;                       // +0x000 "onif", "joseph", ...
    unsigned char _unmapped004[0x34];
    unsigned short damage;            // +0x038 $Damage: (unarmed)
    unsigned char _unmapped03a[0xf8 - 0x3a];
    resistance_set resistance;        // +0x0f8
    unsigned char _unmapped114[0x258 - 0x114];
    unsigned int flags;               // +0x258 0x200: ignore the weapon's damage on the assess screen
};

class Character;
class CharacterProtection {           // the sub-object at Character +0x558 ([0] = the character)
public:
    float get_protection();           // 0x464de0 character_get_protection (thiscall on +0x558)
    Character *owner;
};

class Character {
public:
    void  get_display_name(char *buf);    // 0x462070 character_get_display_name
    int   get_max_stat(int which);        // 0x462840 character_get_max_stat (0 = HP, 1 = AP)
    float get_resistance(int type);       // 0x4626b0 character_get_resistance (3 = magic)

    unsigned char _unmapped000[0x558];
    CharacterProtection protection;       // +0x558
    unsigned char _unmapped55c[0x5f8 - 0x55c];
    float hp;                             // +0x5f8
    unsigned char _unmapped5fc[4];
    short level;                          // +0x600
    unsigned char _unmapped602[0x71c - 0x602];
    char_info *info;                      // +0x71c
    unsigned char _unmapped720[0x754 - 0x720];
    item_info *equipment[10];             // +0x754; [5] (+0x768) the weapon
};

extern char **damage_type_loc_names[7];   // 0x8ec9c4 localize_strings slots (init_strptrs_8ec9c4)
extern char **resistance_level_names[6];  // 0x8b5098 localize_strings slots (init_strptrs_8b5098)

// ---------------------------------------------------------------- objects (pickup screen)
class item_obj {                          // an item lying in the level or in a container
public:
    int is_gold();                        // 0x46a790 obj_is_gold (thiscall)
    unsigned char _unmapped00[0x98];
    item_info *item;                      // +0x98
    int count;                            // +0x9c stack size
};

class container {                         // 0xe8 bytes, pool 0x2302fe0 (see symbols/types.h)
public:
    item_obj *get_item(int i);            // 0x46acd0 container_get_item (thiscall)
    void remove_item(item_obj *it);       // 0x46ab60 container_remove_item (thiscall)
    unsigned char _unmapped00[0x80];
    int handle;                           // +0x80 object handle
    unsigned char _unmapped84[0x14];
    item_obj *items[16];                  // +0x98
    int num_items;                        // +0xd8
    int user;                             // +0xdc
    int mesh_item_type;                   // +0xe0
    float lock;                           // +0xe4
};

extern int party_inventory[521][2];       // 0x970820 {count, item}
extern int party_inventory_count;         // 0x97186c
void item_get_display_name(char *buf, ItemType *type);   // 0x44d8a0
void item_obj_pickup(item_obj *it, int flag);            // 0x46b6b0
void party_inventory_sort(int (*inv)[2], int count);     // 0x46b640
void obj_mark_for_delete(void *obj);                     // 0x489920
void mp_send_container_close(int handle, int num_taken, int num_items,
                             item_obj **items, bool *selected);   // 0x488da0

// ---------------------------------------------------------------- new_assess.cpp / new_pickup.cpp
void assess_load_bitmaps();
void assess_init(Character *target);
void assess_format_info(Character *target);
void assess_update_scrollbar();
void assess_leave();
int  assess_frame(int key);
int  assess_handle_click(int key);
void assess_page_up();
void assess_page_down();
void assess_button_action(int id);
void assess_scroll_up();
void assess_scroll_down();
void assess_update_buttons();
void assess_drag_scrollbar();
void assess_draw();
void assess_draw_text();
void assess_draw_scrollbar();
bool assess_mouse_in_panel(int x, int y);

void pickup_load_bitmaps();
void pickup_init(container *c);
void pickup_update_scrollbar();
void pickup_leave();
int  pickup_frame(int key);
int  pickup_handle_click(int key);
void pickup_page_up();
void pickup_page_down();
void pickup_take_selected();
void pickup_close();
void pickup_button_action(int id);
void pickup_scroll_up();
void pickup_scroll_down();
void pickup_update_buttons();
void pickup_list_handle_click();
void pickup_drag_scrollbar();
void pickup_draw();
void pickup_draw_scrollbar();
void pickup_draw_item_list();
bool pickup_mouse_in_panel(int x, int y);

#endif
