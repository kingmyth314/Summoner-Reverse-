// cmdline.cpp - Sum.exe v1.40, code 0x53dec0-0x53e514 (vsdk\os\cmdline.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1. Confidence per function: exact = expect identical code, likely = same
// semantics, small layout risk, unsure = the source shape is a guess.
// Dash Faction (rf::CmdLineParam, rf::CmdArg) was used for names only.

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmdline.h"
#include "stubs_cmdline_grfont.h"

// ---- .bss, in address order (0x2dc6ea8-0x2dc705b) ------------------------------------------
CmdArg cmdline_args[MAX_CMDLINE_ARGS];          // 0x2dc6ea8
// The list head. Its definition is what produces the four compiler-generated functions at the
// start of the unit (VC6 $E pair plus the atexit destructor thunk):
//   0x53dec0 init_cmdline_params          call 0x53ded0; jmp 0x53def0 (initialiser table 0x5889e8)
//   0x53ded0 cmdline_params_ctor          CmdLineParam(&cmdline_params, NULL, NULL, true)
//   0x53def0 cmdline_params_register_dtor atexit(0x53df00)
//   0x53df00 cmdline_params_dtor          mov ecx, &cmdline_params; jmp ~CmdLineParam
// Confidence: exact (the argument order 0, 0, 1 is pushed as 1, 0, 0).
static CmdLineParam cmdline_params(NULL, NULL, true); // 0x2dc7038
// Read before cmdline.cpp's own initialiser may have run (other files' parameters register first),
// so it must be zero-initialised .bss, not a constructor-set member.
static bool cmdline_params_initialized;         // 0x2dc7050
int cmdline_num_args;                           // 0x2dc7054
// 0x2dc7058: the default passed to os_registry_read_string. It is a .bss address, so it cannot be
// a string literal (VC6 puts "" in initialised .data); a zero-filled char array reads as "".
// Label and size are ours (4 bytes up to the next unit's 0x2dc7060 ccmd object, minus padding).
static char cmdline_no_options[4];              // 0x2dc7058

// 0x53df10  Confidence: likely (the two 1 KB buffers: frame order cmd_line at +0, options at +0x400).
// The second argument is pushed by the only caller (cmdline_read_params, push 0 / push 0) but never
// read. strcpy/strcat are the inline intrinsics (rep movsd/movsb, repne scasb; " " is a 2-byte word
// copy from 0x5abf08).
void cmdline_init(bool check_unknown, int unused)
{
    char cmd_line[1024];
    char options[1024];

    strcpy(cmd_line, GetCommandLineA());
    strcat(cmd_line, " ");
    os_registry_read_string(NULL, "CmdLineOptions", options, cmdline_no_options);
    strcat(cmd_line, options);
    cmdline_parse(cmd_line, check_unknown, true);
    atexit(cmdline_free);
}

// 0x53dfd0  Confidence: exact.
// atexit handler registered above; also called by cmdline_reparse.
void cmdline_free()
{
    CmdLineParam *p;
    for (p = cmdline_params.next; p != &cmdline_params; p = p->next) {
        p->free_arg();
        p->name_found = false;
    }

    for (int i = 0; i < cmdline_num_args; i++) {
        if (cmdline_args[i].arg) {
            delete cmdline_args[i].arg;
            cmdline_args[i].arg = NULL;
        }
    }
    cmdline_num_args = 0;
}

// 0x53e030  Confidence: exact.
void cmdline_parse(char *cmd_line, bool check_unknown, bool skip_first)
{
    cmdline_split(cmd_line, skip_first);
    cmdline_match_params();
    if (check_unknown)
        cmdline_check_unknown(cmd_line);
}

// 0x53e060  Confidence: likely.
// strtok on " \t"; a token holding exactly one '"' (strchr == strrchr) toggles the quoted state, and
// tokens are joined with " " until the closing one. Each finished argument goes into cmdline_args.
// There is no check against MAX_CMDLINE_ARGS (more than 50 arguments overflow the array).
// The four delimiter literals are separate copies in .data (0x5abf1c, 0x5abf20, 0x5abf24 " ",
// 0x5abf28): no string pooling.
void cmdline_split(char *cmd_line, bool skip_first)
{
    bool in_quotes = false;
    char buf[512];

    for (int i = 0; i < MAX_CMDLINE_ARGS; i++) {
        cmdline_args[i].arg = NULL;
        cmdline_args[i].is_done = false;
    }

    buf[0] = 0;
    char *tok = strtok(cmd_line, " \t");
    if (skip_first)
        tok = strtok(NULL, " \t");      // the program name

    while (tok) {
        if (!in_quotes) {
            strcpy(buf, tok);
        } else {
            strcat(buf, " ");
            strcat(buf, tok);
        }

        char *quote = strchr(tok, '"');
        if (quote && quote == strrchr(tok, '"'))
            in_quotes = !in_quotes;

        if (!in_quotes) {
            cmdline_args[cmdline_num_args].arg = new char[strlen(buf) + 1];
            cmdline_args[cmdline_num_args].is_done = false;
            strcpy(cmdline_args[cmdline_num_args].arg, buf);
            cmdline_num_args++;
            buf[0] = 0;
        }

        tok = strtok(NULL, " \t");
    }
}

// 0x53e1e0  Confidence: exact.
void cmdline_match_params()
{
    for (CmdLineParam *p = cmdline_params.next; p != &cmdline_params; p = p->next) {
        for (int i = 0; i < cmdline_num_args; i++) {
            if (!_stricmp(p->name, cmdline_args[i].arg)) {
                p->name_found = true;
                cmdline_take_argument(p, i);
            }
        }
    }
}

// 0x53e250  Confidence: likely.
// The message is formatted into a local buffer and then dropped (the debug print is compiled out),
// so a parameter followed by another option just gets no argument.
// After operator new the argument pointer is reloaded from cmdline_args, so the source reads the
// array each time rather than keeping a local. is_done is stored before the inline strcpy.
void cmdline_take_argument(CmdLineParam *param, int index)
{
    cmdline_args[index].is_done = true;

    if (param->has_argument && index + 1 < cmdline_num_args) {
        if (cmdline_args[index + 1].arg[0] == '-') {
            char msg[256];
            sprintf(msg, "Command line parameter %s requires argument, but next argument appears to be option (%s)\n",
                    cmdline_args[index].arg, cmdline_args[index + 1].arg);
            return;
        }
        if (strlen(cmdline_args[index + 1].arg) > 0) {
            param->arg = new char[strlen(cmdline_args[index + 1].arg) + 1];
            cmdline_args[index + 1].is_done = true;
            strcpy(param->arg, cmdline_args[index + 1].arg);
        }
    }
}

// 0x53e310  Confidence: likely (the found-branch re-tests p against the list head; VC6 may emit
// that from this shape or from a found flag). cmd_line is not used.
// A parameter that takes an argument skips the next entry. Anything else starting with '-' is
// fatal: DebugError(cmdline.cpp, 260).
void cmdline_check_unknown(char *cmd_line)
{
    char msg[256];

    for (int i = 0; i < cmdline_num_args; i++) {
        CmdLineParam *p;
        for (p = cmdline_params.next; p != &cmdline_params; p = p->next) {
            if (!_stricmp(p->name, cmdline_args[i].arg)) {
                if (p->has_argument)
                    i++;
                break;
            }
        }
        if (p == &cmdline_params && cmdline_args[i].arg[0] == '-') {
            sprintf(msg, "Unrecognized command line parameter %s", cmdline_args[i].arg);
            VSDK_ERROR("D:\\projects\\Summoner\\pccode\\vsdk\\os\\cmdline.cpp", 260, msg);
        }
    }
}

// 0x53e3c0  Confidence: exact. No reference in the image (dead code).
void cmdline_reparse(char *cmd_line, bool check_unknown)
{
    cmdline_free();
    cmdline_parse(cmd_line, check_unknown, false);
}

// 0x53e3e0  Confidence: likely (store order: the binary writes help and has_argument, reads
// cmdline_params_initialized, then writes name, arg, name_found; VC6 scheduling).
// The first constructor to run (any file's) sets up the circular list head; named parameters
// append themselves at the tail. The head itself has no name and stays unlinked.
CmdLineParam::CmdLineParam(const char *name, const char *help, bool has_argument)
{
    this->name = name;
    this->help = help;
    this->arg = NULL;
    this->name_found = false;
    this->has_argument = has_argument;

    if (!cmdline_params_initialized) {
        cmdline_params.next = &cmdline_params;
        cmdline_params.prev = &cmdline_params;
        cmdline_params_initialized = true;
    }

    if (name) {
        CmdLineParam *tail = cmdline_params.prev;
        next = &cmdline_params;
        tail->next = this;
        prev = tail;
        cmdline_params.prev = this;
    }
}

// 0x53e450  Confidence: exact.
// The binary loads prev and next, clears both, then relinks the neighbours, so the source keeps
// them in locals (the stores to this-> come first).
CmdLineParam::~CmdLineParam()
{
    free_arg();
    if (name) {
        CmdLineParam *p = prev;
        CmdLineParam *n = next;
        next = NULL;
        prev = NULL;
        p->next = n;
        n->prev = p;
    }
}

// 0x53e480  Confidence: exact.
void CmdLineParam::free_arg()
{
    if (arg) {
        delete arg;
        arg = NULL;
    }
}

// 0x53e4a0  Confidence: exact.
bool CmdLineParam::found()
{
    return name_found;
}

// 0x53e4b0  Confidence: exact.
int CmdLineParam::get_arg_int()
{
    return atoi(arg);
}

// 0x53e4c0  Confidence: likely (float or double return: both leave atof's ST0 untouched).
// No reference in the image.
float CmdLineParam::get_arg_float()
{
    return (float)atof(arg);
}

// 0x53e4d0  Confidence: exact. No reference in the image.
char *CmdLineParam::get_arg()
{
    return arg;
}

// 0x53e4e0  Confidence: exact. No reference in the image.
// The n-th argument (from 0) that no parameter claimed, or NULL.
char *cmdline_get_unclaimed_arg(int n)
{
    for (int i = 0; i < cmdline_num_args; i++) {
        if (!cmdline_args[i].is_done) {
            if (n == 0)
                return cmdline_args[i].arg;
            n--;
        }
    }
    return NULL;
}
