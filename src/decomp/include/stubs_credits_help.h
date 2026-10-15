// engine_stubs.h - declarations of the engine (vsdk and game) functions, classes and globals
// that new_credits.cpp and new_help.cpp use. Names from symbols/sum.symbols; signatures are
// inferred from the call sites in Sum.exe v1.40 (argument counts and pushes), not from any
// reference source. Only what these two units need is declared.
//
// This header deliberately does not include symbols/types.h: types.h describes TextFile as a C
// struct, while the game calls it as a C++ class with thiscall methods.
#ifndef STUBS_CREDITS_HELP_H
#define STUBS_CREDITS_HELP_H

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

#endif
