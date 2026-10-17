// console.cpp - Sum.exe v1.40, code 0x50a000-0x50af59 (the developer console: command registry, the
// command-line tokenizer and parser, .vcs console scripts)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit file name from sum.symbols (Summoner 2's console.cpp gives the names); directory guessed (vsdk\os,
// the header is vsdk/os/console.h).
// Data: .data 0x5a6f20-0x5a727f (command_line_cursor, token_write_ptr, console_font, then the literals in
// order of use; the two "?" and the two "Print out ..." strings are separate copies, so this vsdk object
// was built without string pooling), .bss 0x25dace8-0x25db92b (the parser state, the 300 command
// pointers, the two command objects, and two "" literals at 0x25db924 / 0x25db928 that VC6 placed in
// .bss: the initial command_line_cursor and console_printf's buffer initialiser).
//
// The PC build has no console window: print_to_console is empty (0x50a5a0 is a bare ret), so every
// message below is formatted and dropped. Commands still run from .vcs scripts (cmd_script) and from
// other callers of execute_command_line (load_script only, in v1.40).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include "vsdk/os/console.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/vfile/file.h"
#include "vsdk/vfile/linefile.h"

// ---- .data (0x5a6f20-0x5a6f2b) ------------------------------------------------------------------------
static char *command_line_cursor = "";       // 0x5a6f20 -> the "" literal 0x25db924
static char token[128];                      // 0x25db894 (defined here so the initialiser below can use it)
static char *token_write_ptr = token;        // 0x5a6f24
int console_font = -1;                       // 0x5a6f28 (name ours) console_set_font; never read in v1.40

// ---- .bss (0x25dace8-0x25db923; listed in address order, VC6 does not keep the definition order) ----
float command_argument_float;                // 0x25dace8
char next_arg[256];                          // 0x25dacec lower case
static int token_type;                       // 0x25dadec TOKEN_*
int checking_status_for_command;             // 0x25dadf0
// 0x25dadf4: 4 bytes with no reference.
unsigned int arg_type;                       // 0x25dae04 ARG_* of the last parsed argument
static int char_types[256];                  // 0x25dae14 CHAR_* (dwords: sum.symbols' uint8[256] is wrong)
static char current_char;                    // 0x25db214
int calling_a_command_function;              // 0x25db218
static char lowercase_token[128];            // 0x25db21c
static jmp_buf console_error_jmp;            // 0x25db29c (name ours) process_command's longjmp target
int command_argument_int;                    // 0x25db2dc
int doing_help_for_comand;                   // 0x25db2e0
char original_arg[256];                      // 0x25db2e4
console_command *Console_commands[MAX_CONSOLE_COMMANDS];   // 0x25db3e4
char *rest_of_command_line;                  // 0x25db914
int current_command_count;                   // 0x25db918
static bool char_types_initialized;          // 0x25db91c (name ours) tested by init_char_types, never set
int debug_command_prefixed;                  // 0x25db920

static void set_command_line(char *line);
static void get_next_char();
static void get_token();
static void skip_whitespace();
static void make_lowercase_token();
static void read_word();
static void read_quoted_string();

// 0x50a000 init_ccmd_print_to_file_thunk (jmp; not in sum.symbols: the bytes 0x50a000-0x50a004 are
// `jmp 0x50a010`). Confidence: exact.
// 0x50a010 init_ccmd_print_to_file (console_command constructor). Confidence: exact.
console_command ccmd_print_to_file("print_to_file", "Print out console commands to the text file console.txt",
                                   cmd_print_to_file);   // 0x25dadf8

// 0x50a030  Confidence: exact. Writes "name - description" (or the name alone) per command to
// console_commands.txt, not console.txt as the description says. A failed open returns before the help
// text.
void cmd_print_to_file()
{
    if (calling_a_command_function) {
        File f;
        char line[256];
        int i;

        if (f.open("console_commands.txt", 0x80000002, 9999999) != 0)
            return;

        for (i = 0; i < current_command_count; i++) {
            console_command *c = Console_commands[i];
            if (c != NULL) {
                if (c->description != NULL)
                    sprintf(line, "%s - %s", c->name, c->description);
                else
                    strcpy(line, c->name);
                f.write_line(line, -1);
            }
        }
        f.close();
    }
    if (doing_help_for_comand)
        print_to_console("Print out console commands to the text file console.txt", 0);
}

// 0x50a180  Confidence: exact (the append and the insert share the tail; the insert is a guarded
// down-shift). Inserts the command into Console_commands sorted by _stricmp. A full table or a name that
// is already registered leaves the object unlinked and its fields unset (still zero).
console_command::console_command(const char *name, const char *help, void (*func)())
{
    int i, j;

    if (current_command_count >= MAX_CONSOLE_COMMANDS)
        return;

    for (i = 0; i < current_command_count; i++) {
        int r = _stricmp(Console_commands[i]->name, name);
        if (r == 0)
            return;
        if (r > 0)
            break;
    }
    if (i < current_command_count) {
        for (j = current_command_count; j > i; j--)
            Console_commands[j] = Console_commands[j - 1];
        Console_commands[i] = this;
    } else {
        Console_commands[current_command_count] = this;
    }
    current_command_count++;

    this->name = name;
    this->description = help;
    this->callback = func;
}

// 0x50a210  Confidence: exact (mode in EBX: 0 call, 1 "?" status, 2 help; the "?" test reuses BL for
// ARG_NONE). Frame pointer because of setjmp. An argument error inside the command (process_command's
// longjmp) returns -1 from here.
int execute_command_line(const char *line)
{
    char buf[256];
    int mode;
    int i;

    if (strlen(line) < 1 || strlen(line) >= 255)
        return -1;

    strcpy(buf, line);
    debug_command_prefixed = 0;
    rest_of_command_line = buf;
    set_command_line(buf);
    if (setjmp(console_error_jmp) != 0)
        return -1;

    mode = 0;
    process_command(-1);
    if (_stricmp(next_arg, "debug") == 0) {
        debug_command_prefixed = 1;
        print_to_console("Command line:", 0);
        print_to_console(rest_of_command_line, 0);
        process_command(-1);
    }
    if (_stricmp(next_arg, "?") == 0) {
        mode = 1;
        process_command(-1);
        if (arg_type & ARG_NONE)
            return 0;
    }
    if (_stricmp(next_arg, "help") == 0 || _stricmp(next_arg, "man") == 0) {
        mode = 2;
        process_command(-1);
        if (arg_type & ARG_NONE)
            return 0;
    }
    if (strstr(rest_of_command_line, "?") != NULL)
        mode = 2;

    if (!(arg_type & ARG_STRING)) {
        sprintf(string_buffer, "Invalid keyword '%s'\n", next_arg);
        print_to_console(string_buffer, 0);
        return -1;
    }
    if (debug_command_prefixed) {
        sprintf(string_buffer, "Searching for command '%s'\n", next_arg);
        print_to_console(string_buffer, 0);
    }

    for (i = 0; i < current_command_count; i++) {
        if (_stricmp(next_arg, Console_commands[i]->name) == 0) {
            if (mode == 0) {
                if (debug_command_prefixed) {
                    sprintf(string_buffer, "Calling function '%s'\n", next_arg);
                    print_to_console(string_buffer, 0);
                }
                doing_help_for_comand = 0;
                calling_a_command_function = 1;
                checking_status_for_command = 1;
            } else if (mode == 1) {
                if (debug_command_prefixed) {
                    sprintf(string_buffer, "Checking status for '%s'\n", next_arg);
                    print_to_console(string_buffer, 0);
                }
                calling_a_command_function = 0;
                doing_help_for_comand = 0;
                checking_status_for_command = 1;
            } else {
                if (debug_command_prefixed) {
                    sprintf(string_buffer, "Doing help for '%s'\n", next_arg);
                    print_to_console(string_buffer, 0);
                }
                calling_a_command_function = 0;
                doing_help_for_comand = 1;
                checking_status_for_command = 0;
            }

            Console_commands[i]->callback();

            if (mode == 0) {
                process_command(-1);
                if (!(arg_type & ARG_NONE)) {
                    sprintf(string_buffer, "Ignoring the unused command line tail '%s %s'\n", original_arg,
                            rest_of_command_line);
                    print_to_console(string_buffer, 0);
                }
            }
            return 0;
        }
    }

    sprintf(string_buffer, "Unknown command '%s'\n", next_arg);
    print_to_console(string_buffer, 0);
    return -1;
}

// 0x50a560  Confidence: exact (tail jump to get_next_char).
static void set_command_line(char *line)
{
    command_line_cursor = line;
    get_next_char();
}

// 0x50a570  Confidence: exact. The cursor stops on the terminating NUL.
static void get_next_char()
{
    if (*command_line_cursor == '\0')
        current_char = *command_line_cursor;
    else
        current_char = *command_line_cursor++;
}

// 0x50a5a0  Confidence: exact. Empty in the PC build.
void print_to_console(const char *text, int flags)
{
}

// 0x50a5b0  Confidence: exact (the token-type mapping: the last case is a ternary, sub / neg / sbb).
// Parses the next argument into original_arg / next_arg and sets arg_type; when none of arg_types'
// bits is set it reports the error and longjmps back into execute_command_line.
void process_command(int arg_types)
{
    char *end;

    get_token();
    rest_of_command_line = command_line_cursor;
    strcpy(original_arg, token);
    strcpy(next_arg, lowercase_token);
    if (debug_command_prefixed) {
        sprintf(string_buffer, "next arg is '%s', was originally '%s'\n", next_arg, original_arg);
        print_to_console(string_buffer, 0);
        print_to_console("Rest of the command line is ", 0);
        print_to_console(rest_of_command_line, 0);
        print_to_console("\n", 0);
    }

    if (token_type == TOKEN_END)
        arg_type = ARG_NONE;
    else if (token_type == TOKEN_WORD)
        arg_type = ARG_STRING;
    else
        arg_type = (token_type == TOKEN_QUOTED) ? ARG_QUOTED : ARG_STRING;

    if (arg_type & ARG_QUOTED) {
        int len = strlen(next_arg);
        memmove(next_arg, next_arg + 1, len - 2);
        next_arg[len - 2] = '\0';
        arg_type = ARG_STRING;
    }

    if (arg_type & ARG_STRING) {
        int len = strlen(next_arg);
        int digits = 0;
        int i;

        for (i = 0; i < len; i++) {
            if (char_types[(unsigned char)next_arg[i]] == CHAR_DIGIT)
                digits++;
        }
        if (digits == len) {
            arg_type |= ARG_FLOAT;
            command_argument_float = (float)atof(next_arg);
            if (strchr(next_arg, '.') == NULL) {
                arg_type |= ARG_INT;
                command_argument_int = atoi(next_arg);
            }
        } else if (next_arg[0] == '0' && next_arg[1] == 'x') {
            int value = strtol(next_arg, &end, 0);
            if (*end == '\0') {
                arg_type |= ARG_HEX | ARG_INT;
                command_argument_int = value;
            }
        }

        if (debug_command_prefixed) {
            if (arg_type & ARG_FLOAT) {
                sprintf(string_buffer, "Found float number! %f\n", command_argument_float);
                print_to_console(string_buffer, 0);
            }
            if (arg_type & ARG_INT) {
                // "%f" with an int argument (debug trace only).
                sprintf(string_buffer, "Found int number! %f\n", command_argument_int);
                print_to_console(string_buffer, 0);
            }
            if (arg_type & ARG_HEX) {
                sprintf(string_buffer, "Found hex number! %x\n", command_argument_int);
                print_to_console(string_buffer, 0);
            }
        }

        if (_stricmp(next_arg, "on") == 0)
            arg_type |= ARG_TRUE;
        if (_stricmp(next_arg, "true") == 0)
            arg_type |= ARG_TRUE;
        if (_stricmp(next_arg, "off") == 0)
            arg_type |= ARG_FALSE;
        if (_stricmp(next_arg, "false") == 0)
            arg_type |= ARG_FALSE;
        if (_stricmp(next_arg, "+") == 0)
            arg_type |= ARG_PLUS;
        if (_stricmp(next_arg, "-") == 0)
            arg_type |= ARG_MINUS;
        if (_stricmp(next_arg, ",") == 0)
            arg_type |= ARG_COMMA;
    }

    if (arg_type & ARG_INT) {
        if (command_argument_int)
            arg_type |= ARG_TRUE;
        else
            arg_type |= ARG_FALSE;
    }

    if (!(arg_types & arg_type)) {
        if ((arg_type & ARG_NONE) && !(arg_types & ARG_NONE)) {
            print_to_console("Error: Not enough parameters.\n", 0);
        } else {
            sprintf(string_buffer, "Error: '%s' invalid type\n", next_arg);
            print_to_console(string_buffer, 0);
        }
        longjmp(console_error_jmp, 1);
    }
}

// 0x50a9a0  Confidence: exact (jump table 0x50aa18 over the five classes; the one-character case
// stores through the pointer the compiler knows). Ends with a tail jump to make_lowercase_token.
static void get_token()
{
    skip_whitespace();
    token_write_ptr = token;
    token[0] = '\0';

    switch (char_types[(unsigned char)current_char]) {
    case CHAR_QUOTE:
        read_quoted_string();
        break;
    case CHAR_END:
        token_type = TOKEN_END;
        break;
    case CHAR_LETTER:
    case CHAR_DIGIT:
        read_word();
        break;
    default:
        *token_write_ptr++ = current_char;
        *token_write_ptr = '\0';
        get_next_char();
        token_type = TOKEN_WORD;
        break;
    }
    make_lowercase_token();
}

// 0x50aa30  Confidence: exact (the loop is a jmp back to the entry).
static void skip_whitespace()
{
    while (current_char == ' ' || current_char == '\t')
        get_next_char();
}

// 0x50aa50  Confidence: exact (a do-while: there is no test before the first character, so an empty
// token also lower-cases the stale bytes after its NUL up to the next NUL).
static void make_lowercase_token()
{
    char *p;

    strcpy(lowercase_token, token);
    p = lowercase_token;
    do {
        *p = (*p >= 'A' && *p <= 'Z') ? *p + ('a' - 'A') : *p;
    } while (*++p);
}

// 0x50aab0  Confidence: exact. Letters and digit-class characters (0-9 + - .) form one word.
static void read_word()
{
    while (char_types[(unsigned char)current_char] == CHAR_LETTER
           || char_types[(unsigned char)current_char] == CHAR_DIGIT) {
        char *p = token_write_ptr;
        *p++ = current_char;
        token_write_ptr = p;
        get_next_char();
    }
    token_type = TOKEN_WORD;
    *token_write_ptr = '\0';
}

// 0x50ab00  Confidence: exact. Keeps both quotes; an unterminated string ends at the end of the line.
static void read_quoted_string()
{
    char *p = token_write_ptr;
    *p++ = '"';
    token_write_ptr = p;
    get_next_char();
    while (current_char != '"' && current_char != '\0') {
        p = token_write_ptr;
        *p++ = current_char;
        token_write_ptr = p;
        get_next_char();
    }
    get_next_char();
    token_type = TOKEN_QUOTED;
    p = token_write_ptr;
    *p++ = '"';
    token_write_ptr = p;
    *p = '\0';
}

// 0x50ab60  Confidence: likely (the order of the nine single stores). The argument is not read
// (LoadSettings passes 0). char_types_initialized is never set, so a second call would rebuild the table.
void init_char_types(int arg)
{
    int i;

    if (char_types_initialized == true)
        return;

    for (i = 0; i < 256; i++)
        char_types[(unsigned char)i] = CHAR_OTHER;
    for (i = '0'; i <= '9'; i++)
        char_types[(unsigned char)i] = CHAR_DIGIT;
    for (i = 'A'; i <= 'Z'; i++)
        char_types[(unsigned char)i] = CHAR_LETTER;
    for (i = 'a'; i <= 'z'; i++)
        char_types[(unsigned char)i] = CHAR_LETTER;
    char_types['.'] = CHAR_DIGIT;
    char_types['-'] = CHAR_DIGIT;
    char_types['+'] = CHAR_DIGIT;
    char_types['_'] = CHAR_LETTER;
    char_types['"'] = CHAR_QUOTE;
    char_types[0] = CHAR_END;
    char_types[':'] = CHAR_LETTER;
    char_types['\\'] = CHAR_LETTER;
    char_types['$'] = CHAR_LETTER;
    current_char = '\0';
}

// 0x50ac30 init_ccmd_script_thunk (jmp, initialiser table entry 0x58887c). Confidence: exact.
// 0x50ac40 init_ccmd_script (console_command constructor). Confidence: exact.
console_command ccmd_script("script", "Runs a console script file (.vcs)", cmd_script);   // 0x25dae08

// 0x50ac60  Confidence: exact. With no file name the usage is printed and the help flag set.
void cmd_script()
{
    if (calling_a_command_function) {
        process_command(ARG_STRING);
        if (strlen(next_arg) != 0) {
            load_script(next_arg);
        } else {
            doing_help_for_comand = 1;
            print_to_console("Usage: script <scriptfilename>\n", 0);
            return;
        }
    }
    if (doing_help_for_comand)
        print_to_console("Usage: script <scriptfilename>\n", 0);
}

// 0x50acd0  Confidence: exact (unit /Ob0, D8402; the comment logic; the order of the two buffers). Path: the install
// directory, "\", the name cut at its first '.', ".vcs". The cut uses strchr over the whole path, so a
// '.' anywhere in the install directory truncates the path (strrchr was meant). "//" lines and lines
// inside /* */ are skipped without counting; a "/*" and "*/" on the same line opens a comment that the
// next "*/" line closes. The line number of the error message counts only the empty and executed lines.
void load_script(const char *name)
{
    char line[256];
    char path[1024];
    LineFile *lf;
    char *p;
    int line_no;
    int in_comment;

    file_get_full_dir(0, path);
    strcat(path, "\\");
    strcat(path, name);
    p = strchr(path, '.');
    if (p != NULL)
        *p = '\0';
    strcat(path, ".vcs");

    lf = LineFile_open(path, 0x1000);
    if (lf == NULL) {
        sprintf(string_buffer, "Couldn't find a script file '%s'\n", path);
        print_to_console(string_buffer, 0);
        return;
    }

    sprintf(string_buffer, "Executing script file '%s'...\n", path);
    print_to_console(string_buffer, 0);

    line_no = 1;
    in_comment = 0;
    while (lf->read_line(line, 256)) {
        trim_whitespace(line, line, -1);
        if (line[0] == '/' && line[1] == '/')
            continue;
        if (strstr(line, "/*") != NULL && !in_comment) {
            in_comment = 1;
            continue;
        }
        if (strstr(line, "*/") != NULL && in_comment) {
            in_comment = 0;
            continue;
        }
        if (in_comment)
            continue;
        if (strlen(line) != 0 && execute_command_line(line) != 0) {
            sprintf(string_buffer, "Error processing script line %d: '%s'\n", line_no, line);
            print_to_console(string_buffer, 0);
        }
        line_no++;
    }
    lf->close();
    delete lf;
}

// 0x50aef0  Confidence: exact (the 2 KB buffer is "" initialised: the first byte from the .bss ""
// literal 0x25db928, then 0x7ff zero bytes).
void console_printf(const char *fmt, ...)
{
    char buf[2048] = "";
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    print_to_console(buf, 0);
}

// 0x50af50  Confidence: exact. The font is stored and never used (no console drawing in this build).
void console_set_font(int font)
{
    console_font = font;
}

// 0x50af60 LineFile__scalar_deleting_dtor (compiler-generated for `delete lf` in load_script; COMDAT kept
// from this object, also called by LineFile_open / LineFile_create). Confidence: exact.
