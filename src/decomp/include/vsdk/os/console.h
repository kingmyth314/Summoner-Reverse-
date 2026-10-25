// vsdk/os/console.h - developer console commands (console.cpp in sum.symbols; 0x50a000-0x50af59, written
// in decomp44). Directory guessed (vsdk\os). Not original source.
#ifndef VSDK_OS_CONSOLE_H
#define VSDK_OS_CONSOLE_H

#include "vsdk/vsdk.h"

// 0xc bytes as in symbols/types.h (DECISIONS.md D40): the constructor writes only +0, +4, +8, and two
// command objects are followed directly by another variable at +0xc (ccmd_map_grid 0x5b2a00 /
// automap_centered 0x5b2a0c, ccmd_show_ui 0x5bd780 / ui_choice_popup_mouse_mode 0x5bd78c); the 0x10
// spacing elsewhere in .bss is not the size. The constructor links the command; there is no destructor.
class console_command {
public:
    console_command(const char *name, const char *help, void (*func)());   // 0x50a180
    // fields (symbols/types.h console_command)
    const char *name;                    // +0x0
    const char *description;             // +0x4
    void (*callback)();                  // +0x8
};
LAYOUT_CHECK(console_command, sizeof(console_command) == 0xc);
typedef console_command console_command_t;   // the pilots' wrapper names (D40)
typedef console_command console_command_M;

// Argument types (process_command / arg_type)
#define ARG_NONE  0x01
#define ARG_STRING 0x02                  // decomp20: the dialogue_make_topic_* commands pass 2, then read next_arg
#define ARG_INT   0x08                   // decomp20: cmd_dcamera passes 8, then reads command_argument_int
#define ARG_FLOAT 0x10                   // cmd_gamma passes 0x10, then reads command_argument_float
#define ARG_TRUE  0x40
#define ARG_FALSE 0x80
// decomp44 (console.cpp): the other bits process_command sets.
#define ARG_QUOTED 0x04                  // a "..." token; process_command strips the quotes and makes it ARG_STRING
#define ARG_HEX   0x20                   // "0x..." (set with ARG_INT)
#define ARG_PLUS  0x100                  // "+"
#define ARG_MINUS 0x200                  // "-"
#define ARG_COMMA 0x400                  // ","
// Token types of get_token (token_type).
#define TOKEN_END    0                   // end of the line
#define TOKEN_WORD   1                   // a word / number, or one punctuation character
#define TOKEN_QUOTED 3                   // a "..." string (the quotes are kept in token)
// char_types classes (init_char_types).
#define CHAR_LETTER 0                    // A-Z a-z _ : \ $
#define CHAR_QUOTE  1                    // "
#define CHAR_OTHER  2                    // punctuation and spaces: a one-character token
#define CHAR_END    3                    // NUL
#define CHAR_DIGIT  4                    // 0-9 + - .
#define MAX_CONSOLE_COMMANDS 300

extern char next_arg[256];               // 0x25dacec lowercase copy of the parsed argument (decomp20)
extern int command_argument_int;         // 0x25db2dc (decomp20)
extern float command_argument_float;     // 0x25dace8
extern int calling_a_command_function;   // 0x25db218
extern int doing_help_for_comand;        // 0x25db2e0 (spelling as in Summoner 2)
extern int checking_status_for_command;  // 0x25dadf0
extern unsigned int arg_type;            // 0x25dae04
void process_command(int arg_types);     // 0x50a5b0 (return value never used; type guessed)
void print_to_console(const char *text, int flags);   // 0x50a5a0 (empty in this build)
void console_set_font(int font);         // 0x50af50 (GameInit: font-small.vf)
void init_char_types(int arg);           // 0x50ab60 (LoadSettings passes 0)

// The shared sprintf buffer (0x257ed80, .bss next to frametime.cpp's; owner unit not known).
extern char string_buffer[];             // 0x257ed80

void console_printf(const char *fmt, ...);   // 0x50aef0 (vsprintf + print_to_console; decomp23: cmd_summon)

// decomp44 (console.cpp, 0x50a000-0x50af59). The rest of the unit's globals and functions.
extern char original_arg[256];           // 0x25db2e4 the parsed argument as typed
extern const char *rest_of_command_line;       // 0x25db914 the line after the parsed argument
extern int current_command_count;        // 0x25db918
extern console_command *Console_commands[MAX_CONSOLE_COMMANDS];   // 0x25db3e4 sorted by name (_stricmp)
extern int debug_command_prefixed;       // 0x25db920 "debug <command>": trace the parse
extern console_command ccmd_print_to_file;   // 0x25dadf8
extern console_command ccmd_script;      // 0x25dae08
void cmd_print_to_file();                // 0x50a030
// 0 = run, -1 = empty / too long / bad keyword / unknown command / argument error (longjmp).
int  execute_command_line(const char *line);   // 0x50a210
void cmd_script();                       // 0x50ac60
void load_script(const char *name);      // 0x50acd0 runs <install dir>\<name>.vcs line by line

#endif
