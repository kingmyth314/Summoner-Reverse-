// vsdk/parse/parse.h - the table parser (vsdk\parse\parse.cpp, path references 0x5159ad-0x51673f; code
// 0x515960-0x5172eb). Not original source. Layout: symbols/types.h TextFile. One merged declaration of
// the four stub TextFile classes (DECISIONS.md D4).
#ifndef VSDK_PARSE_PARSE_H
#define VSDK_PARSE_PARSE_H

#include "vsdk/vsdk.h"

class String;
class vector3;
class matrix;
class File;

struct TextFilePosition {             // 8 bytes: {line, cursor} (GetPosition / SetPosition)
    int line;
    char *cursor;
};

// Lookup for quoted names in an integer list: (element, index, name), false = unknown name.
typedef bool (*TextFileIntLookup)(int *dest, int index, const char *name);

// Whitespace = space, tab, LF, VT, FF and CR (CR counts a line), // line and /* */ block comments.
// The Parse* functions call Error (which never returns) on bad input.
class TextFile {                      // 0x50 bytes (stack frames of credits_parse_table, ui_load_table)
public:
    TextFile();                                           // 0x515960
    TextFile(const char *name, int path_id);              // 0x515970 Open, DebugError (line 191) on < 0
    ~TextFile();                                          // 0x5159d0 (thunk to Close)
    void SkipBlockComment();                              // 0x5159e0 (cursor after "/*")
    void SkipSpaces();                                    // 0x515a20 SkipWhitespace without CR
    void SkipWhitespace();                                // 0x515a80
    void SkipToNextLine();                                // 0x515af0 (stops on the CR: decomp22 notes)
    bool SkipToWrap(const char *str);                     // 0x515b30 from the cursor, then from the start
    bool SkipTo(const char *str, const char *stop);       // 0x515ba0 (AL tested by ui_parse_slider)
    bool SkipPastWrap(const char *str);                   // 0x515c20
    bool SkipPast(const char *str, const char *stop);     // 0x515c50
    int  CopyUntil(char *dest, int size, const char *str, const char *stop);  // 0x515c80 length copied
    void GetPosition(TextFilePosition *pos);              // 0x515ce0
    void SetPosition(TextFilePosition *pos);              // 0x515d00
    // 0 = loaded, 1 = not in the blasted packfile, < 0 = File::open's error.
    int  Open(const char *name, int path_id);             // 0x515d20
    void Close();                                         // 0x515e90
    void Rewind();                                        // 0x515ec0
    void Error(const char *expected);                     // 0x515ee0 DebugError, never returns
    bool SkipLine(String *line);                          // 0x515f90 the rest of the line into a String
    bool ReadLine(char *dest, int dest_size);             // 0x516010
    bool SkipWhitespaceAndLine(String *line);             // 0x516090
    bool ReadNextLine(char *dest, int dest_size);         // 0x5160b0
    bool Peek(const char *str);                           // 0x5160d0
    bool Accept(const char *str);                         // 0x516110
    bool AcceptQuoted(const char *str);                   // 0x516150
    void Expect(const char *str);                         // 0x5161e0
    bool ParseBool();                                     // 0x516230 true/yes, false/no
    int  ParseInt();                                      // 0x516290
    unsigned int ParseUInt();                             // 0x516380
    float ParseFloat();                                   // 0x516460
    void ParseVector(vector3 *v);                         // 0x516510 "< x, y, z >"
    void ParseMatrix(matrix *m);                          // 0x5165a0 "[ v, v, v ]"
    int  ParseDelimitedString(String *out, char open_quote, char close_quote);  // 0x516630 length
    int  ParseString(char *dest, int size, char open_quote, char close_quote);  // 0x5166b0 length
    void ParseMultiline(String *out, const char *terminator);                   // 0x516780
    void ParseMultilineJoined(String *out, const char *terminator);             // 0x516830 CR -> ' ', no LF
    int  ParseStringList(String *list, int max_count);    // 0x5168b0 count (max_count not checked)
    // The count read (max_count is not checked; the callers pass lookup 0).
    int  ParseIntList(int *list, int max_count, TextFileIntLookup lookup);      // 0x516940
    int  ParseFloatList(float *list, int max_count);      // 0x516a30 count (max_count not checked)
    int  ParseFlags(const char **names, int count);       // 0x516ab0 bit i per name i
    int  ParseTypeString(const char **types, int count);  // 0x516bd0
    unsigned int Checksum();                              // 0x516c70 Crc32 of the buffer
    int  AtEnd();                                         // 0x517210

    char *buffer;                     // +0x00 file contents (freed by Close unless a static buffer is used)
    char *cursor;                     // +0x04 current parse position
    char filename[0x40];              // +0x08 name passed to Open (Error prints it; decomp22 D205)
    int line;                         // +0x48 current line number (Rewind sets 1)
    int size;                         // +0x4c buffer length
};
LAYOUT_CHECK(TextFile, sizeof(TextFile) == 0x50);

// To-text helpers at the end of parse.cpp (the String versions return by value).
String bool_to_String(bool b);                                           // 0x516ca0
String vector3_to_string(const vector3 *v, int precision);               // 0x516cd0
String matrix3_to_string(const matrix *m, int precision);                // 0x516e80
void bool_to_cstr(char *dest, int size, bool b);                         // 0x517000 (size unused, D6000)
void vector3_to_cstr(char *dest, int size, const vector3 *v, int precision);   // 0x517060 (size unused)
void matrix3_to_cstr(char *dest, int size, const matrix *m, int precision);    // 0x5170d0 (size unused)
// Lines of at most 70 chars (broken at a space or CR), then "$end text" and an empty line.
void file_write_text_block(const char *text, File *f);                  // 0x517180

// A packfile already in memory: TextFile::Open takes table files from it while it is set (decomp22 D206).
void file_blast_into_buffer(char *buf, int size, const char *name);     // 0x517220
extern int  file_blast_size;              // 0x5a7950 (-1 = none; name ours)
extern char *file_blast_buffer;           // 0x26b63a0
extern char file_blast_filename[0x30];    // 0x26b63a4

#endif
