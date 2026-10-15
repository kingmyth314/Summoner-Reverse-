// unnamed_linefile_0x558340.cpp - Sum.exe v1.40, code 0x558340-0x558a23: LineFile, a buffered stdio text
// file read and written by lines (console.cpp's load_script reads scripts with it). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): .data 0x5adbe8-0x5adbfb (the line-end pointer and "\r\n", then "rb",
// "ab", "wb"), after the String file's last literal; no .bss. The bezier code
// (0x558a30) that follows shares nothing with it. The scalar deleting destructor LineFile_open /
// LineFile_create call is console.obj's COMDAT (0x50af60).

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "vsdk/vfile/linefile.h"
#include "vsdk/os/memory.h"

// ---- .data ------------------------------------------------------------------------------------------
char *LineFile_eol = "\r\n";                       // 0x5adbe8

// 0x558340  Confidence: exact.
int LineFile::open_read(const char *name, int size)
{
    int err;

    if (fp != NULL)
        close();
    flags &= ~LINEFILE_EOF;
    fp = fopen(name, "rb");
    if (fp == NULL)
        return 1;
    setvbuf(fp, NULL, _IONBF, 0);
    buffer_size = size;
    buffer = (char *)allocate_in_commonblock(size);
    count = 0;
    err = detect_eol();
    if (err != 0)
        close();
    flags |= LINEFILE_READ;
    return err;
}

// 0x5583d0  Confidence: exact.
int LineFile::open_write(const char *name, bool append, int size)
{
    if (fp != NULL)
        close();
    fp = fopen(name, append ? "ab" : "wb");
    if (fp == NULL)
        return 1;
    setvbuf(fp, NULL, _IOFBF, size);
    flags &= ~LINEFILE_READ;
    return 0;
}

// 0x558430  Confidence: likely (the branch order). The line ending of the first buffer: CRLF when the
// first '\r' is followed by '\n' (and no lone '\n' comes first), LF or CR when only one kind is there,
// ANY otherwise (also when the buffer holds no line end at all).
int LineFile::detect_eol()
{
    char *lf;
    char *cr;

    if (fill_buffer() != 0)
        return 1;
    lf = (char *)memchr(buffer, '\n', count);
    cr = (char *)memchr(buffer, '\r', count);
    if (cr != NULL && cr - buffer < count - 1 && cr[1] == '\n') {
        if (lf != NULL && (lf > cr + 1 || lf < cr)) {
            flags |= LINEFILE_EOL_ANY;
            return 0;
        }
        flags &= ~LINEFILE_EOL_MASK;
        return 0;
    }
    if (lf != NULL) {
        if (cr != NULL) {
            flags |= LINEFILE_EOL_ANY;
            return 0;
        }
        flags = (flags & ~LINEFILE_EOL_CR) | LINEFILE_EOL_LF;
        return 0;
    }
    if (cr != NULL) {
        flags = (flags & ~LINEFILE_EOL_LF) | LINEFILE_EOL_CR;
        return 0;
    }
    flags |= LINEFILE_EOL_ANY;
    return 0;
}

// 0x5584f0  Confidence: exact (the inline memcpy of the unread bytes to the buffer start).
int LineFile::fill_buffer()
{
    int want;
    int got;

    if (count != 0 && cur > buffer) {
        memcpy(buffer, cur, count);
        end += buffer - cur;
        cur = buffer;
    } else if (count == 0) {
        cur = buffer;
        end = buffer;
    }
    if (!(flags & LINEFILE_EOF)) {
        want = buffer + buffer_size - end;
        got = fread(end, 1, want, fp);
        end += got;
        count += got;
        if ((unsigned)got < (unsigned)want)
            flags |= LINEFILE_EOF;
    }
    return 0;
}

// 0x558580  Confidence: exact.
LineFile::LineFile()
{
    fp = NULL;
    buffer = NULL;
}

// 0x558590  Confidence: exact.
LineFile::~LineFile()
{
    if (fp != NULL)
        fclose(fp);
    if (buffer != NULL)
        deallocate_from_commonblock(buffer);
}

// 0x5585c0  Confidence: exact.
void LineFile::close()
{
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    if (buffer != NULL) {
        deallocate_from_commonblock(buffer);
        buffer = NULL;
    }
}

// 0x558600  Confidence: likely (the CRLF search loop's form; the switch is a four-entry jump table at
// 0x5587a4). The CRLF search reads one byte past the chunk when a '\r' ends it.
char *LineFile::read_line(char *buf, int size)
{
    char *start = buf;
    char *eol;
    char *lf;
    char *cr;
    int left;
    int n;

    if (fp == NULL || buf == NULL)
        return NULL;
    if (count == 0 && (flags & LINEFILE_EOF))
        return NULL;
    eol = NULL;
    left = size - 1;
    while (left != 0) {
        n = count;
        if (n == 0 && (flags & LINEFILE_EOF))
            break;
        if ((unsigned)left <= (unsigned)n)
            n = left;
        switch ((flags >> 2) & 3) {
        case 0:
            eol = cur;
            while (eol != NULL) {
                eol = (char *)memchr(eol, '\r', n - (eol - cur));
                if (eol == NULL)
                    break;
                if (eol[1] == '\n')
                    break;
                eol++;
            }
            break;
        case 1:
            eol = (char *)memchr(cur, '\n', n);
            break;
        case 2:
            eol = (char *)memchr(cur, '\r', n);
            break;
        case 3:
            lf = (char *)memchr(cur, '\n', n);
            cr = (char *)memchr(cur, '\r', n);
            if (lf == NULL)
                eol = cr;
            else if (cr == NULL)
                eol = lf;
            else
                eol = lf < cr ? lf : cr;
            break;
        }
        if (eol != NULL)
            n = eol - cur;
        memcpy(buf, cur, n);
        buf += n;
        left -= n;
        if (eol != NULL)
            n += (flags & LINEFILE_EOL_MASK) ? 1 : 2;
        cur += n;
        count -= n;
        if (count == 0)
            fill_buffer();
        if (eol != NULL)
            break;
    }
    *buf = '\0';
    return start;
}

// 0x5587c0  Confidence: likely (the line-end selection's branch order).
int LineFile::write(const char *s)
{
    const char *cr;
    const char *lf;
    const char *eol;
    bool crlf;

    while (*s != '\0') {
        cr = strchr(s, '\r');
        lf = strchr(s, '\n');
        if (cr == NULL && lf == NULL)
            return fwrite(s, strlen(s), 1, fp) != 1;
        crlf = false;
        if (cr == NULL)
            eol = lf;
        else if (lf == NULL)
            eol = cr;
        else if (lf == cr + 1) {
            crlf = true;
            eol = lf;
        } else if (cr >= lf)
            eol = lf;
        else
            eol = cr;
        if (eol != s) {
            if (crlf) {
                if (cr != s && fwrite(s, eol - s - 1, 1, fp) != 1)
                    return 1;
            } else if (fwrite(s, eol - s, 1, fp) != 1) {
                return 1;
            }
        }
        if (fwrite(LineFile_eol, strlen(LineFile_eol), 1, fp) != 1)
            return 1;
        s = eol + 1;
    }
    return 0;
}

// 0x5588d0  Confidence: exact.
int LineFile_printf(LineFile *f, const char *fmt, ...)
{
    char tmp[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    return f->write(tmp);
}

// 0x558910  Confidence: exact.
LineFile *LineFile_open(const char *name, int size)
{
    LineFile *f = new LineFile;

    if (f == NULL)
        return NULL;
    if (f->open_read(name, size) != 0) {
        delete f;
        return NULL;
    }
    return f;
}

// 0x558960  Confidence: exact.
LineFile *LineFile_create(const char *name, bool append, int size)
{
    LineFile *f = new LineFile;

    if (f == NULL)
        return NULL;
    if (f->open_write(name, append, size) != 0) {
        delete f;
        return NULL;
    }
    return f;
}

// 0x5589b0  Confidence: exact.
int LineFile::putc(char c)
{
    if (c == '\n')
        return fwrite(LineFile_eol, strlen(LineFile_eol), 1, fp) != 1;
    return fputc(c, fp) == EOF;
}

// 0x558a10  Confidence: exact.
void LineFile::flush()
{
    if (fp != NULL && !(flags & LINEFILE_READ))
        fflush(fp);
}
