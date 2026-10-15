// vsdk/parse/parse.h - the table parser (vsdk\parse\parse.cpp, path references 0x5159ad-0x51673f).
// Not original source. Layout: symbols/types.h TextFile. One merged declaration of the four stub TextFile
// classes (DECISIONS.md D4).
#ifndef VSDK_PARSE_PARSE_H
#define VSDK_PARSE_PARSE_H

#include "vsdk/vsdk.h"

struct TextFilePosition {             // 8 bytes: {line, cursor} (GetPosition / SetPosition)
    int line;
    char *cursor;
};

class TextFile {                      // 0x50 bytes (stack frames of credits_parse_table, ui_load_table)
public:
    TextFile();                                           // 0x515960
    ~TextFile();                                          // 0x5159d0 (thunk to Close)
    void SkipWhitespace();                                // 0x515a80
    bool SkipTo(const char *str, const char *stop);       // 0x515ba0 (AL tested by ui_parse_slider)
    void CopyUntil(char *dest, int size, const char *str, const char *stop);  // 0x515c80
    void GetPosition(TextFilePosition *pos);              // 0x515ce0
    void SetPosition(TextFilePosition *pos);              // 0x515d00
    int  Open(const char *name, int path_id);             // 0x515d20, nonzero = failure
    void Close();                                         // 0x515e90
    void Rewind();                                        // 0x515ec0
    bool ReadLine(char *dest, int dest_size);             // 0x516010
    bool Peek(const char *str);                           // 0x5160d0
    bool Accept(const char *str);                         // 0x516110
    void Expect(const char *str);                         // 0x5161e0
    int  ParseInt();                                      // 0x516290
    void ParseString(char *dest, int size, char open_quote, char close_quote);  // 0x5166b0
    int  ParseTypeString(const char **types, int count);  // 0x516bd0
    float ParseFloat();                                   // 0x516460
    int  ParseIntList(int *list, int max_count, int unk); // 0x516940, the count read (callers pass 0)
    unsigned int Checksum();                              // 0x516c70 Crc32 of the buffer

    char *buffer;                     // +0x00 file contents (freed by Close unless a static buffer is used)
    char *cursor;                     // +0x04 current parse position
    ubyte flag08;                     // +0x08 zeroed by the constructor
    ubyte _unmapped[0x3f];            // +0x09
    int line;                         // +0x48 current line number (Rewind sets 1)
    int size;                         // +0x4c buffer length
};
LAYOUT_CHECK(TextFile, sizeof(TextFile) == 0x50);

#endif
