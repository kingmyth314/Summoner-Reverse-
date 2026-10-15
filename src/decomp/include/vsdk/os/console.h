// vsdk/os/console.h - developer console commands (console.cpp in sum.symbols; 0x50a180-0x50a5b0).
// Directory guessed (vsdk\os). Not original source.
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
#define ARG_TRUE  0x40
#define ARG_FALSE 0x80

extern char next_arg[256];               // 0x25dacec lowercase copy of the parsed argument (decomp20)
extern int command_argument_int;         // 0x25db2dc (decomp20)
extern int calling_a_command_function;   // 0x25db218
extern int doing_help_for_comand;        // 0x25db2e0 (spelling as in Summoner 2)
extern int checking_status_for_command;  // 0x25dadf0
extern unsigned int arg_type;            // 0x25dae04
void process_command(int arg_types);     // 0x50a5b0 (return value never used; type guessed)
void print_to_console(const char *text, int flags);   // 0x50a5a0 (empty in this build)

// The shared sprintf buffer (0x257ed80, .bss next to frametime.cpp's; owner unit not known).
extern char string_buffer[];             // 0x257ed80

#endif
