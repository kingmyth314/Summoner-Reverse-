// vsdk/os/console.h - developer console commands (console.cpp in sum.symbols; 0x50a180-0x50a5b0).
// Directory guessed (vsdk\os). Not original source.
#ifndef VSDK_OS_CONSOLE_H
#define VSDK_OS_CONSOLE_H

#include "vsdk/vsdk.h"

// The objects are 0x10 apart in .bss (ccmd_show_framerate 0x5fbfe8, ccmd_targa 0x5fbff8, ccmd_max_skills
// 0x5fc1b8 .. player_free_list 0x5fc1c8), so 0x10 bytes; symbols/types.h lists only the first 0xc
// (name, description, callback). The constructor links the command; there is no destructor.
class console_command {
public:
    console_command(const char *name, const char *help, void (*func)());   // 0x50a180
    // fields (symbols/types.h console_command)
    const char *name;                    // +0x0
    const char *description;             // +0x4
    void (*callback)();                  // +0x8
    int _unk0c;                          // +0xc
};
LAYOUT_CHECK(console_command, sizeof(console_command) == 0x10);

// Argument types (process_command / arg_type)
#define ARG_NONE  0x01
#define ARG_TRUE  0x40
#define ARG_FALSE 0x80

extern int calling_a_command_function;   // 0x25db218
extern int doing_help_for_comand;        // 0x25db2e0 (spelling as in Summoner 2)
extern int checking_status_for_command;  // 0x25dadf0
extern unsigned int arg_type;            // 0x25dae04
void process_command(int arg_types);     // 0x50a5b0 (return value never used; type guessed)
void print_to_console(const char *text, int flags);   // 0x50a5a0 (empty in this build)

// The shared sprintf buffer (0x257ed80, .bss next to frametime.cpp's; owner unit not known).
extern char string_buffer[];             // 0x257ed80

#endif
