// cmdline.h - Sum.exe v1.40 command-line parameters (vsdk\os\cmdline.cpp, 0x53dec0-0x53e514)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
// The layouts are the ones in symbols/types.h (CmdLineParam, CmdArg); they are repeated here as a
// C++ class because the methods are __thiscall and types.h describes them as C structs.
#ifndef CMDLINE_H
#define CMDLINE_H

#define MAX_CMDLINE_ARGS 50

// Every CmdLineParam with a name links itself into the cmdline_params list when constructed. Most
// of them are static objects in other files, built by their own initialisers (each one an
// initialiser + atexit(~CmdLineParam) pair), so the list head must work before cmdline.cpp's own
// initialiser runs: see cmdline_params_initialized.
class CmdLineParam {
public:
    CmdLineParam(const char *name, const char *help, bool has_argument); // 0x53e3e0
    ~CmdLineParam();                                                     // 0x53e450
    void free_arg();                                                     // 0x53e480
    bool found();                                                        // 0x53e4a0
    int get_arg_int();                                                   // 0x53e4b0
    float get_arg_float();                                               // 0x53e4c0
    char *get_arg();                                                     // 0x53e4d0

    CmdLineParam *next;   // +0x00
    CmdLineParam *prev;   // +0x04
    const char *name;     // +0x08 e.g. "-glide"; NULL for the list head
    const char *help;     // +0x0C
    char *arg;            // +0x10 heap copy of the following argument
    bool name_found;      // +0x14
    bool has_argument;    // +0x15
};                        // 0x18 bytes

struct CmdArg {
    char *arg;            // +0x0 heap copy
    bool is_done;         // +0x4 claimed by a parameter (name or argument)
};

extern CmdArg cmdline_args[MAX_CMDLINE_ARGS]; // 0x2dc6ea8
extern int cmdline_num_args;                  // 0x2dc7054

void cmdline_init(bool check_unknown, int unused);
void cmdline_free();
void cmdline_parse(char *cmd_line, bool check_unknown, bool skip_first);
void cmdline_split(char *cmd_line, bool skip_first);
void cmdline_match_params();
void cmdline_take_argument(CmdLineParam *param, int index);
void cmdline_check_unknown(char *cmd_line);
void cmdline_reparse(char *cmd_line, bool check_unknown);
char *cmdline_get_unclaimed_arg(int n);

#endif
