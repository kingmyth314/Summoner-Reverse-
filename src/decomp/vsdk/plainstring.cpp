// plainstring.cpp - Sum.exe v1.40, code 0x514ec0-0x515955 (plainstring.cpp; directory not known, put
// next to the other vsdk\os units: vsdk/os/plainstring.cpp in the original tree is a guess)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// The unit starts after vfile's Crc32 COMDAT (0x514eb0) and ends before parse.cpp (0x515960). Built
// without string pooling: "%d" (0x5a78f8, 0x5a7900), "0x%x" (0x5a7904, 0x5a7948) and the four " "
// (0x5a78e8-0x5a78f4) are separate literals. The four "" are zero bytes in this unit's .bss range
// (0x26b6390-0x26b639c), referenced once each.
//
// .data 0x5a783c-0x5a794b: wide_charmap_table, then the literals in function order.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "vsdk/os/plainstring.h"
#include "vsdk/math/vector.h"
#include "vsdk/gr/gr_font.h"

// ---- .data ----------------------------------------------------------------------------------
// Shift-JIS full-width forms of the printable ASCII chars 0x20..0x75 (0x8140 = ideographic space,
// 0x824f.. = full-width digits, 0x8260.. = A-Z, 0x8281.. = a-u).
ushort wide_charmap_table[86] = {                // 0x5a783c
    0x8140, 0x8196, 0x817b, 0x8143, 0x817c, 0x8144, 0x815e, 0x824f, 0x8250, 0x8251, 0x8252, 0x8253,
    0x8254, 0x8255, 0x8256, 0x8257, 0x8258, 0x8146, 0x8147, 0x8171, 0x8181, 0x8172, 0x8148, 0x8197,
    0x8260, 0x8261, 0x8262, 0x8263, 0x8264, 0x8265, 0x8266, 0x8267, 0x8268, 0x8269, 0x826a, 0x826b,
    0x826c, 0x826d, 0x826e, 0x826f, 0x8270, 0x8271, 0x8272, 0x8273, 0x8274, 0x8275, 0x8276, 0x8277,
    0x8278, 0x8279, 0x816d, 0x818f, 0x816e, 0x814f, 0x8151, 0x8165, 0x8281, 0x8282, 0x8283, 0x8284,
    0x8285, 0x8286, 0x8287, 0x8288, 0x8289, 0x828a, 0x828b, 0x828c, 0x828d, 0x828e, 0x828f, 0x8290,
    0x8291, 0x8292, 0x8293, 0x8294, 0x8295, 0x8296, 0x8297, 0x8298, 0x8299, 0x829a, 0x816f, 0x8162,
    0x8170, 0x8150,
};

// 0x514ec0  Confidence: likely (the length is tested twice: an explicit empty test before the loop).
bool is_blank(const char *s)
{
    int len = strlen(s);
    int i;

    if (len <= 0)
        return true;
    for (i = 0; i < len; i++) {
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
            return false;
    }
    return true;
}

// 0x514f00  Confidence: exact.
void trim_leading_whitespace(char *dest, const char *src)
{
    int len = strlen(src);
    int i;

    for (i = 0; i < len; i++) {
        if (src[i] != ' ' && src[i] != '\t' && src[i] != '\n')
            break;
    }
    strcpy(dest, src + i);
}

// 0x514f60  Confidence: exact.
void trim_trailing_whitespace(char *dest, const char *src)
{
    int i;

    for (i = strlen(src) - 1; i >= 0; i--) {
        if (src[i] != ' ' && src[i] != '\t' && src[i] != '\n')
            break;
    }
    strncpy(dest, src, i + 1);
    dest[i + 1] = '\0';
}

// 0x514fb0  Confidence: exact. max_length is never read.
void trim_whitespace(char *dest, const char *src, int max_length)
{
    int first = 0;
    int len = strlen(src);
    int last;

    for (; first < len; first++) {
        if (src[first] != ' ' && src[first] != '\t' && src[first] != '\n')
            break;
    }
    for (last = len - 1; last >= 0; last--) {
        if (src[last] != ' ' && src[last] != '\t' && src[last] != '\n')
            break;
    }

    int count = last - first + 1;
    if (count <= 0) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src + first, count);
    dest[count] = '\0';
}

// 0x515030  Confidence: likely (local names ours). "" literals 0x26b6390 / 0x26b6394, " " 0x5a78e8 /
// 0x5a78ec. At least one space separates the label from the text.
void format_indented_column(char *dest, int indent, int width, const char *label, const char *fmt, ...)
{
    char text[2048] = "";
    va_list args;
    int i;

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);
    int text_len = strlen(text);

    strcpy(dest, "");
    for (i = 0; i < indent; i++)
        strcat(dest, " ");
    strcat(dest, label);

    int len = strlen(dest);
    int pad;
    if (width > len + text_len)
        pad = width - len - text_len;
    else
        pad = 1;
    for (i = 0; i < pad; i++)
        strcat(dest, " ");

    strcat(dest, text);
}

// 0x515170  Confidence: likely. As format_indented_column, but the text length does not count:
// the label alone is padded to width. "" 0x26b6398 / 0x26b639c, " " 0x5a78f0 / 0x5a78f4.
void format_indented_column_b(char *dest, int indent, int width, const char *label, const char *fmt, ...)
{
    char text[2048] = "";
    va_list args;
    int i;

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);

    strcpy(dest, "");
    for (i = 0; i < indent; i++)
        strcat(dest, " ");
    strcat(dest, label);

    int len = strlen(dest);
    int pad;
    if (width > len)
        pad = width - len;
    else
        pad = 1;
    for (i = 0; i < pad; i++)
        strcat(dest, " ");

    strcat(dest, text);
}

// 0x5152a0  Confidence: likely (the second sign test is a setne of neg: `neg != 0`).
// Defect: a negative number whose digit count is a multiple of 3 gets a comma after the sign
// (-123456 -> "-,123,456"), and the size check counts one comma too few for it.
char *int_to_string_with_commas(char *dest, int size, int value)
{
    char buf[32];

    sprintf(buf, "%d", value);
    int len = strlen(buf);
    int neg = value < 0;
    int commas = (len - neg - 1) / 3;
    if (len + commas >= size)
        return NULL;
    if (commas == 0) {
        strcpy(dest, buf);
        return dest;
    }

    int first = (len - neg) % 3 + (neg != 0);
    if (first == 0)
        first = 3;
    strncpy(dest, buf, first);

    int pos = first;
    for (int i = first; i < len; i += 3) {
        dest[pos++] = ',';
        strncpy(dest + pos, buf + i, 3);
        pos += 3;
    }
    dest[pos] = '\0';
    return dest;
}

// 0x5153b0  Confidence: likely. The first two measurements pass `font` in gr_get_string_size's length
// slot and -1 as the font; the loop passes -1 twice (it reuses EBX, so it cannot be the argument).
// Every caller passes -1, so all three measure the whole string in the current font (decomp22 D204).
char *gr_string_truncate_ellipsis(char *dest, const char *s, int max_w, int font)
{
    int w, h;
    char buf[256];

    strcpy(buf, s);
    gr_get_string_size(&w, &h, buf, font, -1);
    if (w > max_w) {
        strcat(buf, "...");
        gr_get_string_size(&w, &h, buf, font, -1);
        while (w > max_w) {
            buf[strlen(buf) - 4] = '.';
            buf[strlen(buf) - 1] = '\0';
            gr_get_string_size(&w, &h, buf, -1, -1);
        }
    }
    return strcpy(dest, buf);
}

// 0x5154d0  Confidence: exact.
char *strcat_char(char *string, char c)
{
    int len = strlen(string);

    string[len] = c;
    string[len + 1] = '\0';
    return string;
}

// 0x515500  Confidence: exact. No length check: strings over 511 chars overflow the buffers.
char *stristr(const char *str, const char *substr)
{
    char lstr[512];
    char lsub[512];
    int len = strlen(str);
    int sublen = strlen(substr);
    int i;

    for (i = 0; i < len; i++)
        lstr[i] = tolower(str[i]);
    lstr[i] = '\0';
    for (i = 0; i < sublen; i++)
        lsub[i] = tolower(substr[i]);
    lsub[i] = '\0';

    char *p = strstr(lstr, lsub);
    if (p == NULL)
        return NULL;
    return (char *)str + (p - lstr);
}

// 0x5155e0  Confidence: exact (caller file_add_path).
char *strlwr(char *s)
{
    int len = strlen(s);

    for (int i = 0; i < len; i++)
        s[i] = tolower(s[i]);
    return s;
}

// 0x515620  Confidence: exact.
char *strupr(char *s)
{
    int len = strlen(s);

    for (int i = 0; i < len; i++)
        s[i] = toupper(s[i]);
    return s;
}

// 0x515660  Confidence: exact. size unused.
char *int_to_string(char *dest, int size, int value)
{
    char buf[32];

    sprintf(buf, "%d", value);
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x5156b0  Confidence: exact. size unused.
char *int_to_hex_string(char *dest, int size, int value)
{
    char buf[32];

    sprintf(buf, "0x%x", value);
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x515700  Confidence: exact. size unused.
char *float_to_string(char *dest, int size, float value, int precision)
{
    char buf[32];

    sprintf(buf, "%.*f", precision, value);
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x515760  Confidence: likely (VC6 turns the divisions by 1024 into the multiplications by 1/1024
// at 0x581d54; the KB limit is 999.99f at 0x581d50). size unused.
char *byte_count_to_string(char *dest, int size, int bytes)
{
    char buf[64];

    if (bytes < 999) {
        sprintf(buf, "%d bytes", bytes);
    } else {
        float kb = bytes / 1024.0f;
        if (kb < 999.99f)
            sprintf(buf, "%.2f KB", kb);
        else
            sprintf(buf, "%.2f MB", kb / 1024.0f);
    }
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x515800  Confidence: exact. size unused. parse.cpp has a copy (vector3_to_cstr 0x517060, 64-byte
// buffer, no return value).
char *vector3_to_string_buf(char *dest, int size, const vector3 *v, int precision)
{
    char buf[256];

    sprintf(buf, "< %.*f, %.*f, %.*f >", precision, v->x, precision, v->y, precision, v->z);
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x515880  Confidence: exact. size unused; the cast only satisfies the format check.
char *ptr_to_hex_string(char *dest, int size, const void *value)
{
    char buf[32];

    sprintf(buf, "0x%x", (unsigned int)value);
    strcpy(dest, buf);   // not return strcpy(..): dest is loaded before the copy (vc6-compare, D4905)
    return dest;
}

// 0x5158d0  Confidence: likely (loop shape). Stops at the first char outside 0x20..0x75.
void string_to_wide_charmap(ushort *dest, const char *src)
{
    int i;

    for (i = 0; src[i] != '\0'; i++) {
        int c = src[i] - 0x20;
        if (c < 0 || c >= 86)
            break;
        dest[i] = wide_charmap_table[c];
    }
    dest[i] = 0;
}

// 0x515910  Confidence: likely (the inner loop is a pointer walk to &wide_charmap_table[86]).
// Defect: a wide char that is not in the table leaves its byte of dest unwritten.
void wide_charmap_to_string(char *dest, const ushort *src)
{
    int i;

    for (i = 0; *src != 0; i++, src++) {
        for (int j = 0; j < 86; j++) {
            if (*src == wide_charmap_table[j]) {
                dest[i] = (char)(j + 0x20);
                break;
            }
        }
    }
    dest[i] = '\0';
}
