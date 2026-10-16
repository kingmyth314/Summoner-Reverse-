// unnamed_string_0x5566e0.cpp - Sum.exe v1.40, code 0x5566e0-0x558337: the vsdk String class and
// TextBuffer. The file name is a placeholder: no path string, no strings.tbl block, and Summoner 2 has
// no unit with these functions.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md). One object: TextBuffer's two initialised globals (0x5adb94 / 0x5adb98)
// precede the String literals in .data (0x5adbac "%d" .. 0x5adbe0 "0x%x"), so they are defined at the top
// of the same file. LineFile (0x558340) starts the next object: its line-end pointer (0x5adbe8) follows
// the last String literal. .bss 0x2f53af0-0x2f53aff: operator[]'s spare char and three "" literals
// (all-zero literals are .bss with VC6, as in unnamed_vfile_rw_0x525720.cpp); no string pooling (three
// separate "" and two "0x%x" / ", " copies). Exception handling is on (/GX): every function that returns
// a String by value has a frame with a cleanup state.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "vsdk/os/vstring.h"
#include "vsdk/os/memory.h"
#include "vsdk/math/vector.h"

// ---- .data ------------------------------------------------------------------------------------------
static char *textbuffer_overrun_text = "Buffer overrun!";   // 0x5adb94 (labels ours)
static int textbuffer_min_size = -1;                       // 0x5adb98 set on first use: strlen + 2

// 0x5566e0  Confidence: exact. The first n chars of s (all of it for n < 0 or n past its end).
void String::assign_n(const char *s, int n)
{
    free();
    if (s != NULL)
        max_len = strlen(s);
    else
        max_len = 0;
    if (n >= 0 && n < max_len)
        max_len = n;
    if (max_len != 0) {
        buf = new char[max_len + 1];
        strncpy(buf, s, max_len);
        buf[max_len] = '\0';
    } else {
        max_len = 0;
        buf = NULL;
    }
}

// 0x556760  Confidence: exact.
void String::assign_char(char c)
{
    free();
    max_len = 1;
    char *p = new char[2];
    buf = p;
    p[0] = c;
    p[1] = '\0';
}

// 0x556790  Confidence: exact.
void String::alloc(int len)
{
    free();
    max_len = len;
    if (len != 0) {
        buf = new char[len + 1];
        buf[0] = '\0';
    } else {
        buf = NULL;
    }
}

// 0x5567d0  Confidence: exact. max_len is left as it was.
void String::free()
{
    if (buf != NULL) {
        delete buf;
        buf = NULL;
    }
}

// 0x5567f0  Confidence: exact.
String::String()
{
    buf = NULL;
    assign_n("", -1);
}

// 0x556810  Confidence: exact.
String::String(const char *s)
{
    buf = NULL;
    assign_n(s, -1);
}

// 0x556830  Confidence: exact.
String::String(char c)
{
    buf = NULL;
    assign_char(c);
}

// 0x556850  Confidence: exact.
String::String(const String &other)
{
    buf = NULL;
    assign_n(other.buf, -1);
}

// 0x556880  Confidence: exact (two alloc calls).
String::String(int len)
{
    buf = NULL;
    if (len < 0)
        alloc(0);
    else
        alloc(len);
}

// 0x5568b0  Confidence: exact (tail jump).
String::~String()
{
    free();
}

// 0x5568c0  Confidence: exact.
const char *String::c_str() const
{
    if (buf == NULL)
        return "";
    return buf;
}

// 0x5568d0  Confidence: exact.
int String::size() const
{
    if (buf != NULL)
        return strlen(buf);
    return 0;
}

// 0x5568f0  Confidence: exact. With max_len > 0 and buf NULL (after trim_left / trim_right freed an
// all-blank string) it reads through NULL.
bool String::empty() const
{
    if (max_len <= 0)
        return true;
    return buf[0] == '\0';
}

// 0x556910  Confidence: exact.
bool String::is_blank() const
{
    int i;

    if (empty())
        return true;
    for (i = 0; i < max_len; i++) {
        if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n')
            return false;
    }
    return true;
}

// 0x556950  Confidence: likely (an unused copy of buf in the frame).
void String::to_lower()
{
    int i;

    for (i = 0; i < max_len; i++)
        buf[i] = tolower(buf[i]);
}

// 0x556990  Confidence: likely (as to_lower).
void String::to_upper()
{
    int i;

    for (i = 0; i < max_len; i++)
        buf[i] = toupper(buf[i]);
}

// 0x5569d0  Confidence: exact.
String String::substr(int begin, int end) const
{
    int len;

    if (end == -1)
        end = max_len - 1;
    len = end - begin + 1;
    String r(len);
    strncpy(r.buf, buf + begin, len);
    r.buf[len] = '\0';
    return r;
}

// 0x556a80  Confidence: exact.
String String::left(int n) const
{
    String r;

    if (n >= max_len)
        r.assign_n(buf, -1);
    else if (n > 0)
        r = substr(0, n - 1);
    return r;
}

// 0x556b40  Confidence: exact. The copy takes the terminator too, and the result's max_len is n + 1.
String String::right(int n) const
{
    String r;

    if (n >= max_len) {
        r.assign_n(buf, -1);
    } else if (n > 0) {
        r.alloc(n + 1);
        strncpy(r.buf, buf - n + max_len, n + 1);
    }
    return r;
}

// 0x556c00  Confidence: exact.
String String::mid(int start, int len) const
{
    String r(len);

    if (start + len > max_len)
        len = max_len - start;
    if (len > 0) {
        strncpy(r.buf, buf + start, len);
        r.buf[len] = '\0';
    }
    return r;
}

// 0x556cb0  Confidence: exact (the loop forms).
int String::find_nocase(const char *s, bool from_end) const
{
    int len;
    int i;

    if (s == NULL)
        return 0;
    len = strlen(s);
    if (len == 0)
        return 0;
    if (from_end) {
        i = max_len;
        while (i-- != 0) {
            if (_strnicmp(buf + i, s, len) == 0)
                return i;
        }
        return -1;
    }
    for (i = 0; i < max_len; i++) {
        if (_strnicmp(buf + i, s, len) == 0)
            return i;
    }
    return -1;
}

// 0x556d50  Confidence: exact.
void String::trim_left()
{
    int i;

    if (buf == NULL)
        return;
    for (i = 0; i < max_len; i++) {
        if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n')
            break;
    }
    if (i == max_len) {
        free();
        return;
    }
    if (i > 0)
        strcpy(buf, buf + i);
}

// 0x556db0  Confidence: exact.
void String::trim_right()
{
    int i;

    if (buf == NULL)
        return;
    for (i = max_len - 1; i >= 0; i--) {
        if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n')
            break;
    }
    if (i >= 0)
        buf[i + 1] = '\0';
    else
        free();
}

// 0x556df0  Confidence: exact.
float String::to_float() const
{
    if (buf == NULL)
        return 0.0f;
    return (float)atof(buf);
}

// 0x556e10  Confidence: exact.
void String::trim()
{
    trim_right();
    trim_left();
}

// 0x556e30  Confidence: exact (tail jump to empty).
bool String::clear_thunk() const
{
    return empty();
}

// 0x556e40  Confidence: exact.
String &String::operator=(const String &other)
{
    if (&other != this) {
        if (other.max_len == max_len) {
            if (max_len != 0)
                strcpy(buf, other.buf);
        } else {
            free();
            assign_n(other.buf, -1);
        }
    }
    return *this;
}

// 0x556ea0  Confidence: exact.
String &String::operator=(const char *s)
{
    if (s == NULL) {
        free();
        assign_n("", -1);
        return *this;
    }
    if (s != buf) {
        if ((int)strlen(s) == max_len) {
            if (buf != NULL)
                strcpy(buf, s);
        } else {
            free();
            assign_n(s, -1);
        }
    }
    return *this;
}

// 0x556f30  Confidence: exact.
String &String::operator=(char c)
{
    if (max_len == 1) {
        char *p = buf;
        p[0] = c;
        p[1] = '\0';
    } else {
        free();
        assign_char(c);
    }
    return *this;
}

// 0x556f70  Confidence: exact.
String operator+(const String &a, const String &b)
{
    String r(a.max_len + b.max_len);

    if (r.buf != NULL) {
        if (a.max_len > 0) {
            strcpy(r.buf, a.buf);
            if (b.max_len > 0)
                strcat(r.buf, b.buf);
        } else {
            strcpy(r.buf, b.buf);
        }
    }
    return r;
}

// 0x557070  Confidence: exact.
String operator+(const String &a, const char *s)
{
    if (s == NULL)
        return a;
    String r((int)strlen(s) + a.max_len);
    if (r.buf != NULL) {
        if (a.buf != NULL)
            strcpy(r.buf, a.buf);
        strcat(r.buf, s);
    }
    return r;
}

// 0x557180  Confidence: exact.
String operator+(const char *s, const String &b)
{
    if (s == NULL)
        return b;
    String r((int)strlen(s) + b.max_len);
    if (r.buf != NULL) {
        strcpy(r.buf, s);
        if (b.buf != NULL)
            strcat(r.buf, b.buf);
    }
    return r;
}

// 0x557290  Confidence: exact.
String operator+(const String &a, char c)
{
    int len = a.buf != NULL ? strlen(a.buf) : 0;
    String r(len + 1);

    if (len != 0)
        strcpy(r.buf, a.buf);
    r.buf[len] = c;
    r.buf[len + 1] = '\0';
    return r;
}

// 0x557360  Confidence: exact. b's buffer is copied without a NULL test (c + an empty String crashes).
String operator+(char c, const String &b)
{
    int len = b.buf != NULL ? strlen(b.buf) : 0;
    String r(len + 1);

    r[0] = c;
    strcpy(r.buf + 1, b.buf);
    return r;
}

// 0x557420  Confidence: exact.
String &String::operator+=(const String &s)
{
    *this = *this + s;
    return *this;
}

// 0x557490  Confidence: exact.
String &String::operator+=(const char *s)
{
    *this = *this + s;
    return *this;
}

// 0x557500  Confidence: exact.
String &String::append_char(char c)
{
    *this = *this + c;
    return *this;
}

// 0x557570  Confidence: exact.
bool operator==(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return true;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) == 0;
    return false;
}

// 0x5575b0  Confidence: exact.
bool operator==(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return true;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) == 0;
    return false;
}

// 0x5575f0  Confidence: exact.
bool operator==(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return true;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) == 0;
    return false;
}

// 0x557630  Confidence: exact.
bool operator!=(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return false;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) != 0;
    return true;
}

// 0x557670  Confidence: exact.
bool operator!=(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return false;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) != 0;
    return true;
}

// 0x5576b0  Confidence: exact.
bool operator!=(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return false;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) != 0;
    return true;
}

// 0x5576f0  Confidence: exact.
bool operator<(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return false;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) < 0;
    return a.max_len == 0;
}

// 0x557740  Confidence: exact.
bool operator<(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return false;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) < 0;
    return b.max_len > 0;
}

// 0x557790  Confidence: exact.
bool operator<(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return false;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) < 0;
    return a.max_len == 0;
}

// 0x5577e0  Confidence: exact.
bool operator>(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return false;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) > 0;
    return b.max_len == 0;
}

// 0x557830  Confidence: exact.
bool operator>(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return false;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) > 0;
    return b.max_len == 0;
}

// 0x557880  Confidence: exact.
bool operator>(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return false;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) > 0;
    return a.max_len > 0;
}

// 0x5578d0  Confidence: exact.
bool operator<=(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return true;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) <= 0;
    return b.max_len > 0;
}

// 0x557920  Confidence: exact.
bool operator<=(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return true;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) <= 0;
    return b.max_len > 0;
}

// 0x557970  Confidence: exact. With one side empty the answer is the opposite of the other operators'
// (an empty String is not <= "x", "x" is <= NULL); reproduced as written (NOTES.md).
bool operator<=(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return true;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) <= 0;
    return a.max_len != 0;
}

// 0x5579c0  Confidence: exact.
bool operator>=(const String &a, const String &b)
{
    if (a.max_len == 0 && b.max_len == 0)
        return true;
    if (a.max_len != 0 && b.max_len != 0)
        return _stricmp(a.buf, b.buf) >= 0;
    return a.max_len != 0;
}

// 0x557a10  Confidence: exact.
bool operator>=(const char *s, const String &b)
{
    if ((s == NULL || *s == '\0') && b.max_len == 0)
        return true;
    if (s != NULL && b.max_len != 0)
        return _stricmp(s, b.buf) >= 0;
    return b.max_len == 0;
}

// 0x557a60  Confidence: exact.
bool operator>=(const String &a, const char *s)
{
    if (a.max_len == 0 && (s == NULL || *s == '\0'))
        return true;
    if (a.max_len != 0 && s != NULL)
        return _stricmp(a.buf, s) >= 0;
    return a.max_len != 0;
}

// 0x557ab0  Confidence: exact. Out of range: a zeroed spare char (.bss 0x2f53af0).
char &String::operator[](int n)
{
    static char none;

    if (n >= 0 && n < max_len)
        return buf[n];
    none = '\0';
    return none;
}

// 0x557ae0  Confidence: exact.
String String::from_int(int n)
{
    char tmp[44];                       // the frame: flag + String + 0x2c bytes

    sprintf(tmp, "%d", n);
    String s(tmp);
    return s;
}

// 0x557b70  Confidence: exact.
String String::from_hex(int n)
{
    char tmp[44];                       // the frame: flag + String + 0x2c bytes

    sprintf(tmp, "0x%x", n);
    String s(tmp);
    return s;
}

// 0x557c00  Confidence: exact.
String String::from_float(float f, int precision)
{
    char tmp[44];                       // the frame: flag + String + 0x2c bytes

    sprintf(tmp, "%.*f", precision, f);
    String s(tmp);
    return s;
}

// 0x557c90  Confidence: exact (the order of the two temporaries in each sum, right operand first).
// Below 999 bytes "n bytes", below 999.99 KB "x.xx KB", else "x.xx MB".
String String::format_bytes(int bytes)
{
    float kb;

    if (bytes < 999)
        return from_int(bytes) + String(" bytes");
    kb = bytes * (1.0f / 1024.0f);
    if (kb < 999.99f)
        return from_float(kb, 2) + String(" KB");
    return from_float(kb * (1.0f / 1024.0f), 2) + String(" MB");
}

// 0x557e10  Confidence: exact (temporaries built right to left, as VC6 evaluates the operands).
String String_from_vector3(const vector3 *v, int precision)
{
    return String("< ") + String::from_float(v->x, precision) + String(", ") +
           String::from_float(v->y, precision) + String(", ") + String::from_float(v->z, precision) +
           String(" >");
}

// 0x558080  Confidence: exact (its own "0x%x" literal, 0x5adbe0).
String String::from_hex_b(int n)
{
    char tmp[44];                       // the frame: flag + String + 0x2c bytes

    sprintf(tmp, "0x%x", n);
    String s(tmp);
    return s;
}

// 0x558110  Confidence: exact.
TextBuffer::TextBuffer(char *b, int sz, int lines)
{
    int min_size = textbuffer_min_size;
    if (min_size < 1) {
        min_size = strlen(textbuffer_overrun_text) + 2;
        textbuffer_min_size = min_size;
    }
    if (b != NULL) {
        external = true;
        buf = b;
    } else {
        external = false;
        if (sz < min_size)
            sz = min_size;
        buf = new char[sz];
    }
    size = sz;
    max_lines = lines;
    clear();
}

// 0x558180  Confidence: exact.
TextBuffer::~TextBuffer()
{
    if (buf != NULL && !external)
        delete buf;
}

// 0x5581a0  Confidence: exact.
void TextBuffer::clear()
{
    length = 0;
    num_lines = 1;
    buf[0] = '\0';
}

// 0x5581c0  Confidence: exact.
char *TextBuffer::get_text()
{
    return buf;
}

// 0x5581d0  Confidence: exact.
int TextBuffer::get_length()
{
    return length;
}

// 0x5581e0  Confidence: exact (the drop loop's form).
void TextBuffer::append(const char *s)
{
    int len;
    int i;
    int drop;

    len = strlen(s);
    if (length + len + 1 < size) {
        for (i = 0; i < len; i++) {
            if (s[i] == '\n')
                num_lines++;
        }
        if (max_lines > 0 && num_lines > max_lines) {
            drop = num_lines - max_lines;
            for (i = 0; i < length; i++) {
                if (buf[i] == '\n' && --drop == 0)
                    break;
            }
            strcpy(buf, buf + i + 1);
            length += -1 - i;
            num_lines = max_lines;
        }
        strcat(buf, s);
        length += len;
    } else {
        clear();
        strcpy(buf, textbuffer_overrun_text);
        length = strlen(buf);
    }
}

// 0x558300  Confidence: exact.
void TextBuffer_printf(TextBuffer *tb, const char *fmt, ...)
{
    char tmp[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    tb->append(tmp);
}
