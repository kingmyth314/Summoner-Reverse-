// parse.cpp - Sum.exe v1.40, code 0x515960-0x5172eb (vsdk\parse\parse.cpp)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Built without string pooling: three copies of the path (0x5a7974, 0x5a7a0c, 0x5a7b84), "," twice per
// vector / matrix, "true" / "false" three times, "a string" / "a string terminator" twice. The ""
// literals are zero bytes in this unit's .bss range (0x26b63d4 SkipLine, 0x26b63dc / 0x26b63e0
// file_blast_into_buffer).
//
// .data 0x5a7950-0x5a7dcb: file_blast_size (-1), then the literals in function order, ending with
// file_blast_into_buffer's message (0x5a7d78). The movie code starts at 0x5172f0 / 0x5a7dcc.
// file_blast_into_buffer (sum.symbols module vsdk\vfile) is in this unit: its code follows
// TextFile::AtEnd, its literals and file_blast_size are in parse.cpp's .data run, its .bss follows
// SkipLine's "" (decomp22 D206).
//
// Error() never returns (DebugError loops), but the compiler does not know it: every Parse* function
// continues after the call as if the input had been right.

#include <stdio.h>
#include <string.h>
#include "vsdk/parse/parse.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/vstring.h"
#include "vsdk/os/crc32.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/vfile/file.h"
#include "vsdk/vfile/packfile/file_packfile.h"

// Original __FILE__ of this unit (literals 0x5a7974, 0x5a7a0c, 0x5a7b84: not pooled).
#define PARSE_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\parse\\parse.cpp"

// ---- .data ----------------------------------------------------------------------------------
int file_blast_size = -1;               // 0x5a7950 (name ours; size passed to packfile_read_whole_file)

// ---- .bss, in address order ----------------------------------------------------------------
char *file_blast_buffer;                // 0x26b63a0
// 0x30 bytes: SkipLine's "" literal follows at 0x26b63d4 (sum.symbols says 0x40; decomp22 D206).
char file_blast_filename[0x30];         // 0x26b63a4

// 0x515960  Confidence: exact.
TextFile::TextFile()
{
    buffer = NULL;
    cursor = NULL;
    filename[0] = '\0';
}

// 0x515970  Confidence: exact. TextFile__TextFile_open; error line 191 (0xbf). The members are not
// cleared first.
TextFile::TextFile(const char *name, int path_id)
{
    char error[256];

    if (Open(name, path_id) < 0) {
        sprintf(error, "Unable to open parse file %s", name);
        VSDK_ERROR(PARSE_CPP, 191, error);
    }
}

// 0x5159d0  Confidence: exact (jmp Close).
TextFile::~TextFile()
{
    Close();
}

// 0x5159e0  Confidence: exact. Called with the cursor after "/*"; leaves it after "*/".
void TextFile::SkipBlockComment()
{
    while (*cursor != '\0') {
        if (cursor[0] == '*' && cursor[1] == '/')
            break;
        if (*cursor == '\r')
            line++;
        cursor++;
    }
    if (*cursor != '\0')
        cursor += 2;
}

// 0x515a20  Confidence: likely (the second '/' test is repeated in the code: two separate else-ifs).
// Space and 0x09-0x0c; a CR stops it, so a // comment ends on its CR.
void TextFile::SkipSpaces()
{
    while (*cursor != '\0') {
        if (*cursor == ' ' || (*cursor >= '\t' && *cursor < '\r')) {
            cursor++;
        } else if (cursor[0] == '/' && cursor[1] == '/') {
            while (*cursor != '\0' && *cursor != '\r')
                cursor++;
        } else if (cursor[0] == '/' && cursor[1] == '*') {
            cursor += 2;
            SkipBlockComment();
        } else {
            break;
        }
    }
}

// 0x515a80  Confidence: likely (as SkipSpaces). A CR is skipped and counted.
void TextFile::SkipWhitespace()
{
    while (*cursor != '\0') {
        if (*cursor == ' ' || (*cursor >= '\t' && *cursor < '\r')) {
            cursor++;
        } else if (*cursor == '\r') {
            cursor++;
            line++;
        } else if (cursor[0] == '/' && cursor[1] == '/') {
            while (*cursor != '\0' && *cursor != '\r')
                cursor++;
        } else if (cursor[0] == '/' && cursor[1] == '*') {
            cursor += 2;
            SkipBlockComment();
        } else {
            break;
        }
    }
}

// 0x515af0  Confidence: exact. Defect: after the CR loop the LF test reads the CR itself, so the
// cursor stays on the CR (the line is counted here and again by the next SkipWhitespace).
void TextFile::SkipToNextLine()
{
    while (*cursor != '\r') {
        if (*cursor++ == '\0')
            return;
    }
    line++;
    if (*cursor == '\n')
        cursor++;
}

// 0x515b30  Confidence: exact.
bool TextFile::SkipToWrap(const char *str)
{
    TextFilePosition pos;

    if (SkipTo(str, NULL))
        return true;
    GetPosition(&pos);
    Rewind();
    if (SkipTo(str, NULL))
        return true;
    SetPosition(&pos);
    return false;
}

// 0x515ba0  Confidence: exact. Fails when stop comes before str. Counts the CRs it passes.
bool TextFile::SkipTo(const char *str, const char *stop)
{
    if (cursor - buffer > size)
        return false;

    char *p = strstr(cursor, str);
    if (p == NULL)
        return false;
    if (stop != NULL) {
        char *q = strstr(cursor, stop);
        if (q != NULL && q < p)
            return false;
    }
    while (cursor != p) {
        if (*cursor++ == '\r')
            line++;
    }
    return true;
}

// 0x515c20  Confidence: exact.
bool TextFile::SkipPastWrap(const char *str)
{
    if (SkipToWrap(str)) {
        Expect(str);
        return true;
    }
    return false;
}

// 0x515c50  Confidence: exact.
bool TextFile::SkipPast(const char *str, const char *stop)
{
    if (SkipTo(str, stop)) {
        Expect(str);
        return true;
    }
    return false;
}

// 0x515c80  Confidence: exact. Without a match the rest of the line is copied instead.
int TextFile::CopyUntil(char *dest, int size, const char *str, const char *stop)
{
    char *start = cursor;

    if (!SkipTo(str, stop)) {
        ReadLine(dest, size);
        return strlen(dest);
    }
    int len = cursor - start;
    strncpy(dest, start, len);
    dest[len] = '\0';
    return len;
}

// 0x515ce0  Confidence: exact.
void TextFile::GetPosition(TextFilePosition *pos)
{
    pos->line = line;
    pos->cursor = cursor;
}

// 0x515d00  Confidence: exact.
void TextFile::SetPosition(TextFilePosition *pos)
{
    line = pos->line;
    cursor = pos->cursor;
}

// 0x515d20  Confidence: likely (EH frame for the File; return paths). With a blasted packfile the
// table must be in it (1 = not found, no disk fallback); else the file is read into the common block
// (one byte more for the terminator).
int TextFile::Open(const char *name, int path_id)
{
    File f;

    strcpy(filename, name);
    buffer = NULL;
    if (file_blast_buffer != NULL) {
        int offset;
        if (!packfile_find_in_packfile(file_blast_filename, filename, &size, &offset))
            return 1;
        buffer = file_blast_buffer + offset;
    }

    if (buffer == NULL) {
        int rval = f.open(name, 1, path_id);
        if (rval < 0)
            return rval;
        size = f.size(NULL, PATH_ANY);
        buffer = (char *)allocate_in_commonblock(size + 1);
        buffer[size] = '\0';
        f.read(buffer, size, 0, 0);
        f.close();
    }

    Rewind();
    return 0;
}

// 0x515e90  Confidence: exact. A blasted buffer is not freed.
void TextFile::Close()
{
    if (buffer != NULL && file_blast_buffer == NULL)
        deallocate_from_commonblock(buffer);
    buffer = NULL;
}

// 0x515ec0  Confidence: exact.
void TextFile::Rewind()
{
    if (buffer != NULL) {
        line = 1;
        cursor = buffer;
    }
}

// 0x515ee0  Confidence: likely (the order of the three buffers in the frame). Error line 582
// (0x246). Control chars show as 0x16. The "found" copy always takes 40 bytes, past the terminator
// too (it reads beyond the end of the buffer near the end of the file).
void TextFile::Error(const char *expected)
{
    char found[44];
    char prior[44];
    char msg[256];
    int i;

    char *p = cursor;
    for (i = 0; i < 40; i++) {
        if (p[i] != '\0' && p[i] < ' ')
            found[i] = 0x16;
        else
            found[i] = p[i];
    }
    found[i] = '\0';

    char *start = cursor - 40;
    if (start < buffer)
        start = buffer;
    for (i = 0; start != cursor && i < 40; i++, start++) {
        if (*start < ' ')
            prior[i] = 0x16;
        else
            prior[i] = *start;
    }
    prior[i] = '\0';

    sprintf(msg, "Parsing file \"%s\", at line #%d\nExpected %s but found this text:\n\n[%s]\n\n"
                 "Prior text was this:\n\n[%s]\n",
            filename, line, expected, found, prior);
    VSDK_ERROR(PARSE_CPP, 582, msg);
}

// 0x515f90  Confidence: exact. TextFile__SkipLine: the rest of the line into a String (the String
// version of ReadLine), then past CR LF. At the end of the text the String is emptied ("" 0x26b63d4).
bool TextFile::SkipLine(String *out)
{
    if (*cursor == '\0') {
        *out = "";
        return false;
    }

    int len;
    for (len = 0; cursor[len] != '\0' && cursor[len] != '\r'; len++)
        ;
    out->assign_n(cursor, len);
    cursor += len;
    if (*cursor == '\r') {
        line++;
        cursor++;
        if (*cursor == '\n')
            cursor++;
    }
    return true;
}

// 0x516010  Confidence: likely (store order of the terminator and the cursor). Fails without moving
// when the line does not fit (len >= dest_size).
bool TextFile::ReadLine(char *dest, int dest_size)
{
    if (*cursor == '\0')
        return false;

    int len;
    for (len = 0; cursor[len] != '\0' && cursor[len] != '\r'; len++)
        ;
    if (len >= dest_size)
        return false;

    strncpy(dest, cursor, len);
    dest[len] = '\0';
    cursor += len;
    if (*cursor == '\r') {
        line++;
        cursor++;
        if (*cursor == '\n')
            cursor++;
    }
    return true;
}

// 0x516090  Confidence: exact.
bool TextFile::SkipWhitespaceAndLine(String *out)
{
    SkipWhitespace();
    return SkipLine(out);
}

// 0x5160b0  Confidence: exact.
bool TextFile::ReadNextLine(char *dest, int dest_size)
{
    SkipWhitespace();
    return ReadLine(dest, dest_size);
}

// 0x5160d0  Confidence: exact. Case-insensitive prefix test (strlen before the whitespace skip).
bool TextFile::Peek(const char *str)
{
    int len = strlen(str);

    SkipWhitespace();
    if (_strnicmp(cursor, str, len) == 0)
        return true;
    return false;
}

// 0x516110  Confidence: exact (sete then test: the result is a bool local).
bool TextFile::Accept(const char *str)
{
    int len = strlen(str);

    SkipWhitespace();
    bool match = _strnicmp(cursor, str, len) == 0;
    if (match)
        cursor += len;
    return match;
}

// 0x516150  Confidence: exact (`cmp bl, 1`: a comparison with true). Accept skips whitespace after
// the opening quote, but the final advance is strlen + 2, so `" name"` is accepted and misplaces the
// cursor by the skipped spaces.
bool TextFile::AcceptQuoted(const char *str)
{
    TextFilePosition pos;

    SkipWhitespace();
    if (*cursor != '"')
        return false;

    GetPosition(&pos);
    cursor++;
    bool match = Accept(str);
    if (match == true && *cursor != '"')
        match = false;
    SetPosition(&pos);
    if (!match)
        return false;

    cursor += strlen(str) + 2;
    return true;
}

// 0x5161e0  Confidence: exact.
void TextFile::Expect(const char *str)
{
    char buf[256];

    if (!Accept(str)) {
        sprintf(buf, "\"%s\"", str);
        Error(buf);
    }
}

// 0x516230  Confidence: exact.
bool TextFile::ParseBool()
{
    SkipWhitespace();
    if (Accept("true") || Accept("yes"))
        return true;
    if (Accept("false") || Accept("no"))
        return false;
    Error("a boolean value");
    return false;
}

// 0x516290  Confidence: likely (the digit expressions; VC6 folds value * 16 + c - '0' to
// (value - 3) << 4 + c). Optional '-', 0x / 0X hex or decimal.
// Defect: strchr also finds the terminator, so a hex number at the very end of the text takes the NUL
// as a digit ('A'-'F' branch) and the cursor runs past the end.
int TextFile::ParseInt()
{
    bool neg = false;
    int value = 0;
    int digits = 0;

    SkipWhitespace();
    if (*cursor == '-') {
        cursor++;
        neg = true;
    }

    if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        cursor += 2;
        while (strchr("0123456789abcdefABCDEF", *cursor) != NULL) {
            digits++;
            char c = *cursor;
            if (c >= '0' && c <= '9')
                value = value * 16 + c - '0';
            else if (c >= 'a' && c <= 'f')
                value = value * 16 + c - 'a' + 10;
            else
                value = value * 16 + c - 'A' + 10;
            cursor++;
        }
    } else {
        while (*cursor >= '0' && *cursor <= '9') {
            digits++;
            value = value * 10 + *cursor - '0';
            cursor++;
        }
    }

    if (neg)
        value = -value;
    if (digits == 0)
        Error("an integer");
    return value;
}

// 0x516380  Confidence: likely (as ParseInt, no sign). Same strchr defect.
unsigned int TextFile::ParseUInt()
{
    int digits = 0;
    unsigned int value = 0;

    SkipWhitespace();
    if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        cursor += 2;
        while (strchr("0123456789abcdefABCDEF", *cursor) != NULL) {
            digits++;
            char c = *cursor;
            if (c >= '0' && c <= '9')
                value = value * 16 + c - '0';
            else if (c >= 'a' && c <= 'f')
                value = value * 16 + c - 'a' + 10;
            else
                value = value * 16 + c - 'A' + 10;
            cursor++;
        }
    } else {
        while (*cursor >= '0' && *cursor <= '9') {
            digits++;
            value = value * 10 + *cursor - '0';
            cursor++;
        }
    }

    if (digits == 0)
        Error("a uint");
    return value;
}

// 0x516460  Confidence: exact (the two '.' tests share the fraction code). The integer part goes
// through ParseInt (which would accept a second '-'); the fraction sums digit * 0.1^n in x87
// precision (0.0f 0x57eb84, 0.1f 0x580214).
float TextFile::ParseFloat()
{
    bool neg;
    int whole;

    SkipWhitespace();
    neg = false;
    if (*cursor == '-') {
        cursor++;
        neg = true;
    }

    if (*cursor == '.') {
        whole = 0;
    } else {
        whole = ParseInt();
        if (*cursor != '.') {
            if (neg)
                whole = -whole;
            return (float)whole;
        }
    }

    float frac = 0.0f;
    float scale = 0.1f;
    cursor++;
    while (*cursor >= '0' && *cursor <= '9') {
        frac += (*cursor - '0') * scale;
        scale *= 0.1f;
        cursor++;
    }
    if (neg) {
        whole = -whole;
        frac = -frac;
    }
    return frac + whole;
}

// 0x516510  Confidence: exact. "< x, y, z >"; the commas are optional (Accept).
void TextFile::ParseVector(vector3 *v)
{
    SkipWhitespace();
    if (*cursor != '<')
        Error("a vector");
    cursor++;
    v->x = ParseFloat();
    Accept(",");
    v->y = ParseFloat();
    Accept(",");
    v->z = ParseFloat();
    SkipWhitespace();
    if (*cursor != '>')
        Error("a vector terminator");
    cursor++;
}

// 0x5165a0  Confidence: exact. "[ rvec, uvec, fvec ]".
void TextFile::ParseMatrix(matrix *m)
{
    SkipWhitespace();
    if (*cursor != '[')
        Error("a matrix");
    cursor++;
    ParseVector(&m->rvec);
    Accept(",");
    ParseVector(&m->uvec);
    Accept(",");
    ParseVector(&m->fvec);
    SkipWhitespace();
    if (*cursor != ']')
        Error("a matrix terminator");
    cursor++;
}

// 0x516630  Confidence: exact. Stops at the close char, a CR or the end; returns the length.
int TextFile::ParseDelimitedString(String *out, char open_quote, char close_quote)
{
    SkipWhitespace();
    if (*cursor != open_quote)
        Error("a string");
    cursor++;

    int len;
    for (len = 0; cursor[len] != '\0' && cursor[len] != '\r' && cursor[len] != close_quote; len++)
        ;
    if (cursor[len] != close_quote)
        Error("a string terminator");

    out->assign_n(cursor, len);
    cursor += len + 1;
    return len;
}

// 0x5166b0  Confidence: likely (store order of the cursor and the terminator). Error line 1171
// (0x493); the length is pushed once for both the sprintf and the strncpy.
int TextFile::ParseString(char *dest, int size, char open_quote, char close_quote)
{
    char error[256];

    SkipWhitespace();
    if (*cursor != open_quote)
        Error("a string");
    cursor++;

    int len;
    for (len = 0; cursor[len] != '\0' && cursor[len] != '\r' && cursor[len] != close_quote; len++)
        ;
    if (cursor[len] != close_quote)
        Error("a string terminator");

    if (len >= size) {
        sprintf(error, "Not enough storage provided (%d) for parsed C string (len=%d)", size, len);
        VSDK_ERROR(PARSE_CPP, 1171, error);
    }
    strncpy(dest, cursor, len);
    cursor += len + 1;
    dest[len] = '\0';
    return len;
}

// 0x516780  Confidence: exact. The text from the current position up to the terminator, trailing
// whitespace (space, 0x09-0x0d) dropped, then past the terminator.
// Defect: the trim has no lower bound; with the terminator right after the skipped whitespace it walks
// back before the start and the length goes negative.
void TextFile::ParseMultiline(String *out, const char *terminator)
{
    TextFilePosition pos;
    char error[256];

    SkipWhitespace();
    GetPosition(&pos);
    if (!SkipTo(terminator, NULL)) {
        SetPosition(&pos);
        sprintf(error, "multiline terminator \"%s\"", terminator);
        Error(error);
    }

    char *start = pos.cursor;
    int len = cursor - start;
    char *p = cursor - 1;
    while (*p == ' ' || (*p >= '\t' && *p <= '\r')) {
        p--;
        len--;
    }
    out->assign_n(start, len);
    cursor += strlen(terminator);
}

// 0x516830  Confidence: exact. Joins the lines: CR -> space, LF dropped.
void TextFile::ParseMultilineJoined(String *out, const char *terminator)
{
    ParseMultiline(out, terminator);

    int n = out->size();
    int j = 0;
    for (int i = 0; i < n; i++) {
        char c = (*out)[i];
        if (c == '\r')
            (*out)[j++] = ' ';
        else if (c != '\n')
            (*out)[j++] = c;
    }
    (*out)[j] = '\0';
}

// 0x5168b0  Confidence: exact. ( "a" "b" ... ); max_count is not checked.
int TextFile::ParseStringList(String *list, int max_count)
{
    int count = 0;

    SkipWhitespace();
    if (*cursor != '(')
        Error("a string list");
    cursor++;
    SkipWhitespace();
    while (*cursor != '\0' && *cursor != ')') {
        ParseDelimitedString(&list[count], '"', '"');
        count++;
        SkipWhitespace();
    }
    if (*cursor != ')')
        Error("a string list terminator");
    cursor++;
    return count;
}

// 0x516940  Confidence: likely (frame order of pos and name). Integers, or quoted names resolved by
// lookup(&list[i], i, name); max_count is not checked.
int TextFile::ParseIntList(int *list, int max_count, TextFileIntLookup lookup)
{
    TextFilePosition pos;
    char name[256];
    int count = 0;

    SkipWhitespace();
    if (*cursor != '(')
        Error("an integer list");
    cursor++;
    SkipWhitespace();
    while (*cursor != '\0' && *cursor != ')') {
        if (*cursor == '"') {
            GetPosition(&pos);
            ParseString(name, 256, '"', '"');
            if (lookup == NULL || !lookup(&list[count], count, name)) {
                SetPosition(&pos);
                Error("an integer list element");
            }
        } else {
            list[count] = ParseInt();
        }
        count++;
        SkipWhitespace();
    }
    if (*cursor != ')')
        Error("an integer list terminator");
    cursor++;
    return count;
}

// 0x516a30  Confidence: exact. max_count is not checked.
int TextFile::ParseFloatList(float *list, int max_count)
{
    int count = 0;

    SkipWhitespace();
    if (*cursor != '(')
        Error("a float list");
    cursor++;
    SkipWhitespace();
    while (*cursor != '\0' && *cursor != ')') {
        list[count] = ParseFloat();
        count++;
        SkipWhitespace();
    }
    if (*cursor != ')')
        Error("a float list terminator");
    cursor++;
    return count;
}

// 0x516ab0  Confidence: exact. ( "name" ... ) -> bit i for names[i]. A NULL entry ends the search
// without an error, so with a NULL-terminated table an unknown name is ignored silently.
int TextFile::ParseFlags(const char **names, int count)
{
    int flags = 0;
    TextFilePosition pos;
    char name[256];

    SkipWhitespace();
    if (*cursor != '(')
        Error("a flags list");
    cursor++;
    SkipWhitespace();
    while (*cursor != '\0' && *cursor != ')') {
        GetPosition(&pos);
        ParseString(name, 256, '"', '"');
        int i;
        for (i = 0; i < count; i++) {
            if (names[i] == NULL)
                break;
            if (_stricmp(name, names[i]) == 0) {
                flags |= 1 << i;
                break;
            }
        }
        if (i >= count) {
            SetPosition(&pos);
            Error("a valid flag name");
        }
        SkipWhitespace();
    }
    if (*cursor != ')')
        Error("a flags list terminator");
    cursor++;
    return flags;
}

// 0x516bd0  Confidence: exact. The index of the quoted name in types (a NULL entry ends the table).
int TextFile::ParseTypeString(const char **types, int count)
{
    TextFilePosition pos;
    char name[256];

    SkipWhitespace();
    GetPosition(&pos);
    ParseString(name, 256, '"', '"');
    for (int i = 0; i < count; i++) {
        if (types[i] == NULL)
            break;
        if (_stricmp(name, types[i]) == 0)
            return i;
    }
    SetPosition(&pos);
    Error("a valid type string");
    return 0;
}

// 0x516c70  Confidence: exact (the constructor is the out-of-line COMDAT 0x514eb0, then init; see
// crc32.h).
unsigned int TextFile::Checksum()
{
    Crc32 crc;

    crc.init();
    return crc.update(buffer, size);
}

// 0x516ca0  Confidence: exact. bool_to_String (the [esp] = 0 is the return value's construction flag).
String bool_to_String(bool b)
{
    return String(b ? "true" : "false");
}

// 0x516cd0  Confidence: exact (one expression; the operands are built right to left: z, y, x, then
// "< ", and the literals are emitted " >", ", ", ", ", "< ").
String vector3_to_string(const vector3 *v, int precision)
{
    return String("< ") + String::from_float(v->x, precision) + ", " + String::from_float(v->y, precision)
           + ", " + String::from_float(v->z, precision) + " >";
}

// 0x516e80  Confidence: exact (as vector3_to_string; rows fvec, uvec, rvec are built first).
String matrix3_to_string(const matrix *m, int precision)
{
    return "\n\t[ " + vector3_to_string(&m->rvec, precision) + "\n\t  " + vector3_to_string(&m->uvec, precision)
           + "\n\t  " + vector3_to_string(&m->fvec, precision) + " ]";
}

// 0x517000  Confidence: exact.
void bool_to_cstr(char *dest, int size, bool b)
{
    if (b)
        strcpy(dest, "true");
    else
        strcpy(dest, "false");
}

// 0x517060  Confidence: exact. size unused (caller script_write_navpoint). Same as plainstring.cpp's
// vector3_to_string_buf with a 64-byte buffer and no return value.
void vector3_to_cstr(char *dest, int size, const vector3 *v, int precision)
{
    char buf[64];

    sprintf(buf, "< %.*f, %.*f, %.*f >", precision, v->x, precision, v->y, precision, v->z);
    strcpy(dest, buf);
}

// 0x5170d0  Confidence: exact (the frame is 8-byte aligned: and esp, -8 with EBP; why is not known).
// size unused (caller script_write_navpoint).
void matrix3_to_cstr(char *dest, int size, const matrix *m, int precision)
{
    char buf[512];

    sprintf(buf, "\n\t[ < %.*f, %.*f, %.*f >\n\t  < %.*f, %.*f, %.*f >\n\t  < %.*f, %.*f, %.*f > ]",
            precision, m->rvec.x, precision, m->rvec.y, precision, m->rvec.z,
            precision, m->uvec.x, precision, m->uvec.y, precision, m->uvec.z,
            precision, m->fvec.x, precision, m->fvec.y, precision, m->fvec.z);
    strcpy(dest, buf);
}

// 0x517180  Confidence: unsure (shape of the break search: the code tests the found case with
// inc / jns). Lines of at most 70 chars: the last space or CR at index <= 69 ends the line (it is
// dropped), else the first one after index 70, else the rest. Then "$end text" and a bare newline.
void file_write_text_block(const char *text, File *f)
{
    while (*text != '\0') {
        int count = strlen(text);
        int advance = count;
        if (count > 70) {
            int i;
            for (i = 69; i >= 0; i--) {
                if (text[i] == '\r' || text[i] == ' ')
                    break;
            }
            if (i >= 0) {
                count = i;
                advance = i + 1;
            } else {
                for (i = 70; i < count; i++) {
                    if (text[i] == '\r' || text[i] == ' ') {
                        count = i;
                        advance = i + 1;
                        break;
                    }
                }
            }
        }
        f->write_line(text, count);
        text += advance;
    }
    f->write_line("$end text", -1);
    f->write_line(NULL, -1);
}

// 0x517210  Confidence: exact (sete into a cleared ECX: an int result).
int TextFile::AtEnd()
{
    return *cursor == '\0';
}

// 0x517220  Confidence: likely (file_blast_size is stored before the call). NULL forgets the
// buffer; else the packfile is read into buf and TextFile::Open takes tables from it.
// "" literals 0x26b63dc, 0x26b63e0.
void file_blast_into_buffer(char *buf, int size, const char *name)
{
    if (buf == NULL) {
        file_blast_size = -1;
        file_blast_buffer = NULL;
        strcpy(file_blast_filename, "");
        return;
    }

    strcpy(file_blast_filename, name);
    file_blast_buffer = buf;
    file_blast_size = size;
    if (!packfile_read_whole_file(buf, size, name)) {
        printf("************************************************ ERROR BLASTING FILE INTO BUFFER!\n");
        file_blast_size = -1;
        file_blast_buffer = NULL;
        strcpy(file_blast_filename, "");
    }
}
