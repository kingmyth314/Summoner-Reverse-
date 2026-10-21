/* saveedit.c: see saveedit.h. A port of tools/lib/savedata.mjs (tables, save layout) and of the edit
 * engine of tools/savedit.mjs. Comments name the Node function each part follows. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "saveedit.h"

const char *const se_level_names[SE_LEVEL_COUNT] = {
    "Wolong", "Catacombs", "test", "masad", "worldmap1", "lenele1b", "lenele1c", "lenele1d", "lenele1e",
    "sewer", "rand-hills01", "IkaemosBottomInt", "IkaemosTopInt", "IkaemosExt", "KhosaniLab", "IonaExt", "WolongCaverns",
    "TempleInt", "rand-forest01", "rand-forestnite1", "Liangshan", "Rand-Desert", "IonaExt02", "KhosaniStrng", "LPalaceInt",
    "tancredhouse", "LPalaceInt02", "KhosaniLab2", "Wolong2", "eleh", "lenele2aa", "lenele2ab", "lenele3a", "lenele3d",
    "jadetemple", "lenele1aa", "lenele1ab", "TempleInt2", "IkaemosExt2", "IkaemosBottomInt2", "Rand-HillsNite01",
    "Rand-Iceland01", "Rand-Grassland01", "Rand-Orenia01", "Rand-OreniaNite01", "Rand-DesertNite01", "Rand-GrasslandNite01",
    "Rand-IcelandNite01", "endgame", "lenele2d", "sewerboss"};
const char *const se_section_names[SE_SECTION_COUNT] = {
    "thumb", "level data", "conversation", "items", "inv", "le", "camera", "item_object", "quest_flag",
    "level_item", "two_stage", "automap_labels", "hot_buttons"};
static const char *const party_names[4] = {"Joseph", "Flece", "Jekhar", "Rosalind"};

/* ------------------------------------------------------------------------------------------------------ */
/* small helpers */

static void seterr(char *err, size_t n, const char *fmt, ...)
{
    va_list ap;
    if (!err || !n) return;
    va_start(ap, fmt);
    vsnprintf(err, n, fmt, ap);
    va_end(ap);
}

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) { fputs("saveedit: out of memory\n", stderr); exit(3); }
    return p;
}
static void *xrealloc(void *p, size_t n)
{
    p = realloc(p, n ? n : 1);
    if (!p) { fputs("saveedit: out of memory\n", stderr); exit(3); }
    return p;
}
static char *xstrndup(const char *s, size_t n)
{
    char *p = xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

/* String.prototype.toLowerCase on a Latin-1 character */
static unsigned char lc1(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c + 32);
    if (c >= 0xc0 && c <= 0xde && c != 0xd7) return (unsigned char)(c + 32);
    return c;
}
static int ieq(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (lc1((unsigned char)*a) != lc1((unsigned char)*b)) return 0;
    return *a == *b;
}
/* JavaScript white space and line terminators (\s, String.prototype.trim) in the Latin-1 range */
static int jsws(unsigned char c) { return (c >= 9 && c <= 13) || c == ' ' || c == 0xa0; }
/* "." of a JavaScript regular expression does not match these */
static int jsnl(unsigned char c) { return c == '\n' || c == '\r'; }

static void trim_span(const char *s, size_t n, size_t *b, size_t *e)
{
    size_t i = 0, j = n;
    while (i < j && jsws((unsigned char)s[i])) i++;
    while (j > i && jsws((unsigned char)s[j - 1])) j--;
    *b = i; *e = j;
}
static char *trim_dup(const char *s, size_t n)
{
    size_t b, e;
    trim_span(s, n, &b, &e);
    return xstrndup(s + b, e - b);
}
static int all_digits(const char *s, size_t n)
{
    size_t i;
    if (!n) return 0;
    for (i = 0; i < n; i++) if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}
/* Number() of a decimal digit string, saturated (enough for every range check here) */
static int64_t digits_value(const char *s, size_t n)
{
    int64_t v = 0;
    size_t i;
    for (i = 0; i < n; i++) {
        v = v * 10 + (s[i] - '0');
        if (v > ((int64_t)1 << 53)) return (int64_t)1 << 53;
    }
    return v;
}

/* name_hash_nocase */
static uint32_t crc_table[256];
static int crc_ready;
static void crc_init(void)
{
    uint32_t i, k, v;
    for (i = 0; i < 256; i++) {
        v = i;
        for (k = 0; k < 8; k++) v = (v & 1) ? (v >> 1) ^ 0xedb88320u : v >> 1;
        crc_table[i] = v;
    }
    crc_ready = 1;
}
uint32_t se_name_hash_n(const char *s, size_t n)
{
    uint32_t v = 0;
    size_t i;
    if (!crc_ready) crc_init();
    for (i = 0; i < n; i++) v = crc_table[(v ^ lc1((unsigned char)s[i])) & 0xff] ^ (v >> 8);
    return v;
}
uint32_t se_name_hash(const char *s) { return se_name_hash_n(s, strlen(s)); }

/* Bounds-checked little-endian access (Buffer.readInt32LE and friends throw outside the buffer) */
static int inb(size_t size, int64_t off, int64_t len) { return off >= 0 && len >= 0 && off + len <= (int64_t)size; }
static uint32_t ru32(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static int32_t ri32(const unsigned char *p) { return (int32_t)ru32(p); }
static int16_t ri16(const unsigned char *p) { return (int16_t)(uint16_t)(p[0] | p[1] << 8); }
static float rf32(const unsigned char *p) { uint32_t u = ru32(p); float f; memcpy(&f, &u, 4); return f; }
static void wu32(unsigned char *p, uint32_t v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24); }
static void wi16(unsigned char *p, int v) { p[0] = (unsigned char)v; p[1] = (unsigned char)((unsigned)v >> 8); }
static void wf32(unsigned char *p, float f) { uint32_t u; memcpy(&u, &f, 4); wu32(p, u); }

/* ------------------------------------------------------------------------------------------------------ */
/* Tables: tokens (TextFile style: // comments removed, quoted strings kept whole) */

typedef struct { int str; char *s; } tok;
typedef struct { tok *t; int n, cap; } toks;

static void toks_push(toks *v, int str, const char *s, size_t n)
{
    if (v->n == v->cap) { v->cap = v->cap ? v->cap * 2 : 256; v->t = xrealloc(v->t, (size_t)v->cap * sizeof(tok)); }
    v->t[v->n].str = str;
    v->t[v->n].s = xstrndup(s, n);
    v->n++;
}
static void toks_free(toks *v)
{
    int i;
    for (i = 0; i < v->n; i++) free(v->t[i].s);
    free(v->t);
    v->t = NULL; v->n = v->cap = 0;
}
/* tokens(): split on /\r?\n/, then /"([^"]*)"|(\S+)/g per line; a word starting with // ends the line */
static void tokenize(const char *text, size_t len, toks *out)
{
    size_t ls = 0;
    memset(out, 0, sizeof *out);
    while (ls <= len) {
        size_t le = ls, p;
        while (le < len && text[le] != '\n') le++;
        {
            size_t end = le;
            if (le < len && end > ls && text[end - 1] == '\r') end--; /* "\r\n" separator */
            p = ls;
            while (p < end) {
                unsigned char c = (unsigned char)text[p];
                if (jsws(c)) { p++; continue; }
                if (c == '"') {
                    size_t q = p + 1;
                    while (q < end && text[q] != '"') q++;
                    if (q < end) { toks_push(out, 1, text + p + 1, q - p - 1); p = q + 1; continue; }
                }
                {
                    size_t q = p;
                    while (q < end && !jsws((unsigned char)text[q])) q++;
                    if (q - p >= 2 && text[p] == '/' && text[p + 1] == '/') break;
                    toks_push(out, 0, text + p, q - p);
                    p = q;
                }
            }
        }
        if (le >= len) break;
        ls = le + 1;
    }
}
static int isw(const toks *v, int i, const char *w) { return i >= 0 && i < v->n && !v->t[i].str && strcmp(v->t[i].s, w) == 0; }
static const char *strtok_at(const toks *v, int i) { return (i >= 0 && i < v->n && v->t[i].str) ? v->t[i].s : NULL; }
static const char *wordtok_at(const toks *v, int i) { return (i >= 0 && i < v->n && !v->t[i].str) ? v->t[i].s : NULL; }

/* languageSection(): index after the "#<lang>" word, or -1 */
static int language_section(const toks *v, const char *lang)
{
    char want[80];
    int i;
    snprintf(want, sizeof want, "#%s", lang);
    for (i = 0; i < v->n; i++)
        if (!v->t[i].str && ieq(v->t[i].s, want)) return i + 1;
    return -1;
}

/* Number(string) as JavaScript converts it (used for the automap layer numbers) */
static double js_number(const char *s)
{
    size_t b, e, i, n;
    const char *p;
    int neg = 0;
    if (!s) return NAN;
    trim_span(s, strlen(s), &b, &e);
    p = s + b; n = e - b;
    if (n == 0) return 0;
    if (n > 2 && p[0] == '0' && ((p[1] | 32) == 'x' || (p[1] | 32) == 'o' || (p[1] | 32) == 'b')) {
        int base = (p[1] | 32) == 'x' ? 16 : (p[1] | 32) == 'o' ? 8 : 2;
        double v = 0;
        for (i = 2; i < n; i++) {
            int c = (unsigned char)p[i], d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if ((c | 32) >= 'a' && (c | 32) <= 'f') d = (c | 32) - 'a' + 10;
            else return NAN;
            if (d >= base) return NAN;
            v = v * base + d;
        }
        return v;
    }
    i = 0;
    if (p[0] == '+' || p[0] == '-') { neg = p[0] == '-'; i = 1; }
    if (n - i == 8 && memcmp(p + i, "Infinity", 8) == 0) return neg ? -INFINITY : INFINITY;
    {
        size_t j = i, intd = 0, frac = 0;
        char buf[400];
        while (j < n && p[j] >= '0' && p[j] <= '9') { j++; intd++; }
        if (j < n && p[j] == '.') { j++; while (j < n && p[j] >= '0' && p[j] <= '9') { j++; frac++; } }
        if (!intd && !frac) return NAN;
        if (j < n && (p[j] | 32) == 'e') {
            size_t ed = 0;
            j++;
            if (j < n && (p[j] == '+' || p[j] == '-')) j++;
            while (j < n && p[j] >= '0' && p[j] <= '9') { j++; ed++; }
            if (!ed) return NAN;
        }
        if (j != n) return NAN;
        if (n >= sizeof buf) return NAN; /* not reached by any real table */
        memcpy(buf, p, n);
        buf[n] = 0;
        return strtod(buf, NULL);
    }
}
/* ToUint32 */
static uint32_t to_uint32(double x)
{
    double m;
    if (!isfinite(x)) return 0;
    m = fmod(trunc(x), 4294967296.0);
    if (m < 0) m += 4294967296.0;
    return (uint32_t)m;
}

/* ------------------------------------------------------------------------------------------------------ */
/* Table sources (tableSource) */

typedef struct { char *name; uint32_t size; int64_t offset; } vpp_file;
typedef struct {
    const se_table_opts *o;
    int ndir;
    wchar_t **dir;      /* file names in tables_dir */
    int have_vpp;
    int nvpp;
    vpp_file *vpp;
} tsrc;

static int file_exists(const wchar_t *p)
{
    return p && *p && GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES;
}

/* Reads len bytes at off; bytes past the end of the file are 0 (Buffer.alloc + readSync). */
static unsigned char *read_at(FILE *f, int64_t off, size_t len)
{
    unsigned char *b = calloc(len + 1, 1);
    if (!b) return NULL;
    if (_fseeki64(f, off, SEEK_SET) == 0) (void)fread(b, 1, len, f);
    return b;
}

/* vpp.mjs readDirectory */
static int vpp_read_dir(const wchar_t *path, tsrc *ts)
{
    FILE *f = _wfopen(path, L"rb");
    unsigned char *hdr, *dir;
    uint32_t magic, version, count, i;
    int64_t sector, dir_sectors;
    if (!f) return -1;
    hdr = read_at(f, 0, 16);
    if (!hdr) { fclose(f); return -1; }
    magic = ru32(hdr); version = ru32(hdr + 4); count = ru32(hdr + 8);
    free(hdr);
    if (magic != 0x51890aceu || version == 0 || count > 1000000) { fclose(f); return -1; }
    dir_sectors = ((int64_t)count + 31) / 32;
    dir = read_at(f, 0x800, (size_t)(dir_sectors * 0x800));
    fclose(f);
    if (!dir) return -1;
    ts->vpp = xmalloc((size_t)count * sizeof(vpp_file));
    ts->nvpp = (int)count;
    sector = 1 + dir_sectors;
    for (i = 0; i < count; i++) {
        const unsigned char *e = dir + (size_t)i * 0x40;
        size_t end = 0;
        while (end < 0x40 && e[end]) end++;
        if (end > 0x3c) end = 0x3c;
        ts->vpp[i].name = xstrndup((const char *)e, end);
        ts->vpp[i].size = ru32(e + 0x3c);
        ts->vpp[i].offset = sector * 0x800;
        sector += (uint32_t)(ts->vpp[i].size + 0x7ffu) >> 11;
    }
    free(dir);
    return 0;
}

static wchar_t *widen(const char *s)
{
    size_t n = strlen(s), i;
    wchar_t *w = xmalloc((n + 1) * sizeof(wchar_t));
    for (i = 0; i <= n; i++) w[i] = (unsigned char)s[i];
    return w;
}
static wchar_t *path_join(const wchar_t *dir, const wchar_t *name)
{
    size_t a = wcslen(dir), b = wcslen(name);
    wchar_t *p = xmalloc((a + b + 2) * sizeof(wchar_t));
    memcpy(p, dir, a * sizeof(wchar_t));
    if (a && dir[a - 1] != L'\\' && dir[a - 1] != L'/') p[a++] = L'\\';
    memcpy(p + a, name, (b + 1) * sizeof(wchar_t));
    return p;
}
static char *read_whole(const wchar_t *path, size_t *len)
{
    FILE *f = _wfopen(path, L"rb");
    char *b;
    int64_t n;
    if (!f) return NULL;
    if (_fseeki64(f, 0, SEEK_END) != 0 || (n = _ftelli64(f)) < 0 || n > 0x7fffffff) { fclose(f); return NULL; }
    rewind(f);
    b = xmalloc((size_t)n + 1);
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
    fclose(f);
    b[n] = 0;
    *len = (size_t)n;
    return b;
}

static void tsrc_open(tsrc *ts, const se_table_opts *o)
{
    memset(ts, 0, sizeof *ts);
    ts->o = o;
    if (o->tables_dir && *o->tables_dir && file_exists(o->tables_dir)) {
        wchar_t *pat = path_join(o->tables_dir, L"*");
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pat, &fd);
        free(pat);
        if (h != INVALID_HANDLE_VALUE) {
            int cap = 0;
            do {
                if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
                if (ts->ndir == cap) { cap = cap ? cap * 2 : 64; ts->dir = xrealloc(ts->dir, (size_t)cap * sizeof(wchar_t *)); }
                ts->dir[ts->ndir++] = _wcsdup(fd.cFileName);
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }
    if (o->tables_vpp && *o->tables_vpp && file_exists(o->tables_vpp))
        ts->have_vpp = vpp_read_dir(o->tables_vpp, ts) == 0;
}
static void tsrc_close(tsrc *ts)
{
    int i;
    for (i = 0; i < ts->ndir; i++) free(ts->dir[i]);
    free(ts->dir);
    for (i = 0; i < ts->nvpp; i++) free(ts->vpp[i].name);
    free(ts->vpp);
    memset(ts, 0, sizeof *ts);
}
/* get(name): the table text, or NULL */
static char *tsrc_get(tsrc *ts, const char *name, size_t *len)
{
    const wchar_t *ex = NULL;
    int i;
    if (ieq(name, "items.tbl")) ex = ts->o->items_tbl;
    else if (ieq(name, "events.tbl")) ex = ts->o->events_tbl;
    if (ex && *ex) return file_exists(ex) ? read_whole(ex, len) : NULL;
    if (ts->ndir) {
        wchar_t *wn = widen(name);
        for (i = ts->ndir - 1; i >= 0; i--) /* Map: a later name replaces an earlier one */
            if (CompareStringOrdinal(ts->dir[i], -1, wn, -1, TRUE) == CSTR_EQUAL) break;
        free(wn);
        if (i >= 0) {
            wchar_t *p = path_join(ts->o->tables_dir, ts->dir[i]);
            char *t = read_whole(p, len);
            free(p);
            return t;
        }
    }
    if (ts->have_vpp) {
        for (i = ts->nvpp - 1; i >= 0; i--)
            if (ieq(ts->vpp[i].name, name)) break;
        if (i >= 0) {
            FILE *f = _wfopen(ts->o->tables_vpp, L"rb");
            char *t;
            if (!f) return NULL;
            t = (char *)read_at(f, ts->vpp[i].offset, ts->vpp[i].size);
            fclose(f);
            if (!t) return NULL;
            *len = ts->vpp[i].size;
            return t;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------------------ */
/* Table parsers */

static void warn(se_tables *t, const char *fmt, ...)
{
    va_list ap;
    if (t->nwarnings >= SE_MAX_WARNINGS) return;
    va_start(ap, fmt);
    vsnprintf(t->warnings[t->nwarnings++], sizeof t->warnings[0], fmt, ap);
    va_end(ap);
}

typedef struct { char **v; int n, cap; } svec;
static void svec_push(svec *s, const char *str)
{
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 16; s->v = xrealloc(s->v, (size_t)s->cap * sizeof(char *)); }
    s->v[s->n++] = xstrndup(str, strlen(str));
}
typedef struct { uint32_t *v; int n, cap; } hset;
static int hset_has(const hset *h, uint32_t x)
{
    int i;
    for (i = 0; i < h->n; i++) if (h->v[i] == x) return 1;
    return 0;
}
static void hset_add(hset *h, uint32_t x)
{
    if (h->n == h->cap) { h->cap = h->cap ? h->cap * 2 : 256; h->v = xrealloc(h->v, (size_t)h->cap * sizeof(uint32_t)); }
    h->v[h->n++] = x;
}

/* parseItems */
static int parse_items(se_tables *t, const char *text, size_t len, const char *lang)
{
    toks v;
    int i, s, cap = 0;
    tokenize(text, len, &v);
    s = language_section(&v, lang);
    if (s < 0) { toks_free(&v); return -1; }
    for (i = s; i < v.n; i++) {
        const char *nm;
        if (isw(&v, i, "#End")) break;
        if (isw(&v, i, "$Name:") && (nm = strtok_at(&v, i + 1)) != NULL) {
            char *name = trim_dup(nm, strlen(nm));
            uint32_t h = se_name_hash(name);
            int k;
            for (k = 0; k < t->nitems; k++) if (t->items[k].hash == h) break;
            if (k < t->nitems) { free(t->items[k].name); t->items[k].name = name; continue; }
            if (t->nitems == cap) { cap = cap ? cap * 2 : 256; t->items = xrealloc(t->items, (size_t)cap * sizeof(se_item_name)); }
            t->items[t->nitems].hash = h;
            t->items[t->nitems].name = name;
            t->nitems++;
        }
    }
    toks_free(&v);
    return 0;
}

/* parseEvents (events_parse_table) */
static int parse_events(se_tables *t, const char *text, size_t len, const char *lang)
{
    toks v;
    svec fl[SE_LEVEL_COUNT], in[SE_LEVEL_COUNT], fo[SE_LEVEL_COUNT];
    hset flag_known = {0}, int_known = {0};
    int i, li, qcap = 0;
    tokenize(text, len, &v);
    i = language_section(&v, lang);
    if (i < 0) { toks_free(&v); return -1; }
    memset(fl, 0, sizeof fl); memset(in, 0, sizeof in); memset(fo, 0, sizeof fo);
    while (i < v.n && !isw(&v, i, "#End")) {
        const char *lname;
        if (!isw(&v, i, "$Level:")) { i++; continue; }
        lname = strtok_at(&v, i + 1);
        if (!lname) lname = "";
        for (li = 0; li < SE_LEVEL_COUNT; li++) if (ieq(se_level_names[li], lname)) break;
        i += 2;
        if (li == SE_LEVEL_COUNT) { t->stopped = xstrndup(lname, strlen(lname)); break; }
        while (i < v.n && !isw(&v, i, "$Level:") && !isw(&v, i, "#End")) {
            const char *kind = wordtok_at(&v, i), *name = strtok_at(&v, i + 1);
            if (kind && name && (!strcmp(kind, "+Flag:") || !strcmp(kind, "+Int:") || !strcmp(kind, "+Float:"))) {
                uint32_t h = se_name_hash(name);
                if (!strcmp(kind, "+Flag:")) {
                    if (!hset_has(&flag_known, h)) { hset_add(&flag_known, h); svec_push(&fl[li], name); }
                } else if (!hset_has(&int_known, h)) {
                    if (!strcmp(kind, "+Int:")) { hset_add(&int_known, h); svec_push(&in[li], name); }
                    else svec_push(&fo[li], name);
                }
                i += 2;
            } else i++;
        }
    }
    if (!t->stopped && isw(&v, i, "#End")) {
        i++;
        while (i < v.n && !isw(&v, i, "#End")) {
            se_quest q;
            const char *nm;
            if (!isw(&v, i, "$Quest:")) { i++; continue; }
            q.index = t->nquests;
            nm = strtok_at(&v, i + 1);
            q.name = xstrndup(nm ? nm : "", nm ? strlen(nm) : 0);
            q.main = 0;
            q.invalidate = NULL;
            i += 2;
            if (isw(&v, i, "+Main")) { q.main = 1; i += 2; }
            while (i < v.n && !isw(&v, i, "$Quest:") && !isw(&v, i, "#End")) {
                const char *iv;
                if (isw(&v, i, "+Invalidate:") && (iv = strtok_at(&v, i + 1)) != NULL) {
                    free(q.invalidate);
                    q.invalidate = xstrndup(iv, strlen(iv));
                    i += 2;
                } else i++;
            }
            if (t->nquests < SE_MAX_QUESTS) { /* quest_count stops at 0xc8 */
                if (t->nquests == qcap) { qcap = qcap ? qcap * 2 : 64; t->quests = xrealloc(t->quests, (size_t)qcap * sizeof(se_quest)); }
                t->quests[t->nquests++] = q;
            } else { free(q.name); free(q.invalidate); }
        }
    }
    for (li = 0; li < SE_LEVEL_COUNT; li++) {
        se_level_def *d = &t->levels[li];
        d->name = se_level_names[li];
        d->flags = fl[li].v; d->nflags = fl[li].n;
        d->ints = in[li].v; d->nints = in[li].n;
        d->floats = fo[li].v; d->nfloats = fo[li].n;
    }
    free(flag_known.v); free(int_known.v);
    toks_free(&v);
    return 0;
}

/* parseAutomapLayers (level_parse_automap_table) */
static double parse_automap_layers(const char *text, size_t len)
{
    toks v;
    int i;
    double max = 0;
    tokenize(text, len, &v);
    for (i = 0; i < v.n; i++) if (isw(&v, i, "#Automap")) break;
    if (i == v.n) { toks_free(&v); return 0; }
    for (i++; i < v.n;) {
        if (isw(&v, i, "$AutomapPC:") || isw(&v, i, "$AutomapPSX:")) {
            double n = js_number(wordtok_at(&v, i + 2));
            if (isw(&v, i, "$AutomapPC:") && n > max) max = n;
            i += 4;
        } else break;
    }
    toks_free(&v);
    return max;
}

void se_tables_load(se_tables *t, const se_table_opts *opts)
{
    se_table_opts none = {0};
    tsrc ts;
    char *text;
    size_t len = 0;
    int li;
    if (!opts) opts = &none;
    memset(t, 0, sizeof *t);
    snprintf(t->lang, sizeof t->lang, "%s", opts->lang ? opts->lang : "english");
    for (li = 0; li < SE_LEVEL_COUNT; li++) t->levels[li].name = se_level_names[li];
    tsrc_open(&ts, opts);
    text = tsrc_get(&ts, "items.tbl", &len);
    if (text) {
        if (parse_items(t, text, len, t->lang) == 0) t->have_items = 1;
        else warn(t, "items.tbl has no #%s section", t->lang);
        free(text);
    } else warn(t, "items.tbl not found: items are shown by hash");
    text = tsrc_get(&ts, "events.tbl", &len);
    if (text) {
        if (parse_events(t, text, len, t->lang) != 0) warn(t, "events.tbl has no #%s section", t->lang);
        else {
            if (t->stopped) warn(t, "events.tbl: unknown level \"%s\" ends the table (the game stops there too)", t->stopped);
            for (li = 0; li < SE_LEVEL_COUNT; li++) {
                char name[64];
                char *lt;
                size_t ll = 0;
                snprintf(name, sizeof name, "%s.tbl", se_level_names[li]);
                lt = tsrc_get(&ts, name, &ll);
                t->levels[li].layers = lt ? parse_automap_layers(lt, ll) : 0;
                if (!lt) t->nmissing++;
                free(lt);
            }
            t->have_events = 1;
        }
        free(text);
    } else warn(t, "events.tbl not found: story flags are not available");
    tsrc_close(&ts);
}

void se_tables_free(se_tables *t)
{
    int i, k;
    for (i = 0; i < t->nitems; i++) free(t->items[i].name);
    free(t->items);
    for (i = 0; i < SE_LEVEL_COUNT; i++) {
        se_level_def *d = &t->levels[i];
        for (k = 0; k < d->nflags; k++) free(d->flags[k]);
        for (k = 0; k < d->nints; k++) free(d->ints[k]);
        for (k = 0; k < d->nfloats; k++) free(d->floats[k]);
        free(d->flags); free(d->ints); free(d->floats);
    }
    for (i = 0; i < t->nquests; i++) { free(t->quests[i].name); free(t->quests[i].invalidate); }
    free(t->quests);
    free(t->stopped);
    memset(t, 0, sizeof *t);
}

const char *se_item_name_of(const se_tables *t, uint32_t hash)
{
    int i;
    if (!t || !t->have_items) return NULL;
    for (i = 0; i < t->nitems; i++) if (t->items[i].hash == hash) return t->items[i].name;
    return NULL;
}

/* ------------------------------------------------------------------------------------------------------ */
/* Save layout */

static int32_t size_of(const unsigned char *d, int i) { return ri32(d + 0x1b0 + i * 4); }

static void offsets_raw(const unsigned char *d, se_sections *out)
{
    static const int order[SE_SECTION_COUNT] = {0, 1, 4, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12};
    int64_t o = 0x200;
    int k;
    for (k = 0; k < SE_SECTION_COUNT; k++) { out->at[order[k]] = o; o += size_of(d, order[k]); }
    out->end = o;
}
void se_section_offsets(const se_save *s, se_sections *out)
{
    if (s->size < 0x200) { memset(out, 0, sizeof *out); return; }
    offsets_raw(s->data, out);
}

static int64_t chunk_at_raw(const unsigned char *d, size_t size, const char *tag)
{
    se_sections off;
    int idx;
    int64_t o;
    if (!strcmp(tag, "INVT")) idx = 4;
    else if (!strcmp(tag, "ITEM")) idx = 3;
    else if (!strcmp(tag, "LENT")) idx = 5;
    else if (!strcmp(tag, "CMRA")) idx = 6;
    else if (!strcmp(tag, "CONT")) idx = 7;
    else return -1;
    if (size < 0x200) return -1;
    offsets_raw(d, &off);
    o = off.at[idx];
    return inb(size, o, 4) && memcmp(d + o, tag, 4) == 0 ? o : -1;
}
int64_t se_chunk_at(const se_save *s, const char *tag) { return chunk_at_raw(s->data, s->size, tag); }

const char *se_check(const unsigned char *d, size_t size, char *msg, size_t msglen)
{
    static char fallback[160];
    se_sections off;
    if (!msg || !msglen) { msg = fallback; msglen = sizeof fallback; }
    if (size < 0x200 || memcmp(d, "SAVg", 4) != 0) { snprintf(msg, msglen, "not a Summoner save (no SAVg tag)"); return msg; }
    if (ri32(d + 4) != 0x27) { snprintf(msg, msglen, "version %ld, Summoner v1.40 expects 39", (long)ri32(d + 4)); return msg; }
    offsets_raw(d, &off);
    if (off.end != (int64_t)size) {
        snprintf(msg, msglen, "the size table adds up to %lld bytes, the file has %lu", (long long)off.end, (unsigned long)size);
        return msg;
    }
    return NULL;
}

int se_load_mem(se_save *s, const void *data, size_t size, char *err, size_t errlen)
{
    char msg[160];
    memset(s, 0, sizeof *s);
    if (se_check(data, size, msg, sizeof msg)) { seterr(err, errlen, "%s", msg); return -1; }
    s->data = xmalloc(size);
    memcpy(s->data, data, size);
    s->size = size;
    return 0;
}

int se_load_file(se_save *s, const wchar_t *path, char *err, size_t errlen)
{
    HANDLE h;
    LARGE_INTEGER sz;
    unsigned char *b;
    DWORD got;
    size_t done = 0;
    char msg[160];
    memset(s, 0, sizeof *s);
    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { seterr(err, errlen, "cannot read the save (error %lu)", GetLastError()); return -1; }
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart > 0x10000000) { CloseHandle(h); seterr(err, errlen, "cannot read the save (size)"); return -1; }
    b = xmalloc((size_t)sz.QuadPart);
    while (done < (size_t)sz.QuadPart) {
        if (!ReadFile(h, b + done, (DWORD)((size_t)sz.QuadPart - done), &got, NULL) || !got) break;
        done += got;
    }
    CloseHandle(h);
    if (done != (size_t)sz.QuadPart) { free(b); seterr(err, errlen, "cannot read the save"); return -1; }
    if (se_check(b, done, msg, sizeof msg)) { free(b); seterr(err, errlen, "%s", msg); return -1; }
    s->data = b;
    s->size = done;
    return 0;
}

void se_free(se_save *s)
{
    free(s->data);
    s->data = NULL;
    s->size = 0;
}

int se_write_new(const se_save *s, const wchar_t *path, char *err, size_t errlen)
{
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    size_t done = 0;
    DWORD put;
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_EXISTS || e == ERROR_ALREADY_EXISTS) seterr(err, errlen, "the output already exists (never overwritten)");
        else seterr(err, errlen, "cannot create the output (error %lu)", e);
        return -1;
    }
    while (done < s->size) {
        if (!WriteFile(h, s->data + done, (DWORD)(s->size - done), &put, NULL) || !put) break;
        done += put;
    }
    CloseHandle(h);
    if (done != s->size) {
        DeleteFileW(path); /* the partial file this call created */
        seterr(err, errlen, "cannot write the output");
        return -1;
    }
    return 0;
}

static void cstr_copy(char *dst, const unsigned char *src, size_t n)
{
    size_t i = 0;
    while (i < n && src[i]) { dst[i] = (char)src[i]; i++; }
    dst[i] = 0;
}
int se_read_header(const se_save *s, se_header *h)
{
    int i;
    memset(h, 0, sizeof *h);
    if (s->size < 0x200 || memcmp(s->data, "SAVg", 4) != 0) return -1;
    h->version = ri32(s->data + 4);
    cstr_copy(h->level, s->data + 0x08, 0x80);
    cstr_copy(h->script, s->data + 0x88, 0x20);
    h->level_time = rf32(s->data + 0xac);
    cstr_copy(h->description, s->data + 0xb0, 0x100);
    for (i = 0; i < SE_SECTION_COUNT; i++) h->sizes[i] = size_of(s->data, i);
    return 0;
}

/* readInventory */
static int read_inv_raw(const unsigned char *d, size_t size, se_inventory *inv, char *err, size_t errlen)
{
    int64_t o = chunk_at_raw(d, size, "INVT");
    int32_t n;
    int i;
    memset(inv, 0, sizeof *inv);
    if (o < 0) { seterr(err, errlen, "no INVT chunk"); return -1; }
    if (!inb(size, o + 4, 8)) { seterr(err, errlen, "INVT chunk cut off"); return -1; }
    n = ri32(d + o + 8);
    if (!inb(size, o + 12 + 12 * (int64_t)n, 4) || (n > 0 && !inb(size, o + 12, 12 * (int64_t)n))) {
        seterr(err, errlen, "INVT entry count %ld runs past the file", (long)n);
        return -1;
    }
    inv->gold = ri32(d + o + 4);
    inv->count = n > 0 ? n : 0;
    inv->entries = xmalloc((size_t)inv->count * sizeof(se_inv_entry));
    for (i = 0; i < inv->count; i++) {
        const unsigned char *e = d + o + 12 + 12 * i;
        inv->entries[i].hash = ru32(e);
        inv->entries[i].count = ri32(e + 4);
        inv->entries[i].field = ri32(e + 8);
    }
    inv->length_ok = ri32(d + o + 12 + 12 * (int64_t)n) == 12 + 12 * n + 4;
    return 0;
}
int se_read_inventory(const se_save *s, se_inventory *inv, char *err, size_t errlen) { return read_inv_raw(s->data, s->size, inv, err, errlen); }
void se_inventory_free(se_inventory *inv) { free(inv->entries); memset(inv, 0, sizeof *inv); }

/* itemTypes */
static int item_types_raw(const unsigned char *d, size_t size, uint32_t **hashes, int *count, char *err, size_t errlen)
{
    int64_t o = chunk_at_raw(d, size, "ITEM");
    int32_t n;
    int i;
    *hashes = NULL; *count = 0;
    if (o < 0) { seterr(err, errlen, "no ITEM chunk"); return -1; }
    if (!inb(size, o + 4, 4)) { seterr(err, errlen, "ITEM chunk cut off"); return -1; }
    n = ri32(d + o + 4);
    if (n > 0 && !inb(size, o + 8, 4 * (int64_t)n)) { seterr(err, errlen, "ITEM count %ld runs past the file", (long)n); return -1; }
    if (n < 0) n = 0;
    *hashes = xmalloc((size_t)n * sizeof(uint32_t));
    for (i = 0; i < n; i++) (*hashes)[i] = ru32(d + o + 8 + 4 * i);
    *count = n;
    return 0;
}
int se_item_types(const se_save *s, uint32_t **hashes, int *count, char *err, size_t errlen) { return item_types_raw(s->data, s->size, hashes, count, err, errlen); }

/* entities */
static int entities_raw(const unsigned char *d, size_t size, se_entities *e, char *err, size_t errlen)
{
    int64_t lent = chunk_at_raw(d, size, "LENT"), o;
    int32_t count;
    int i, cap = 0;
    memset(e, 0, sizeof *e);
    if (lent < 0) { seterr(err, errlen, "no LENT chunk"); return -1; }
    if (!inb(size, lent + 0x0c, 4) || !inb(size, lent + 0x30, 4)) { seterr(err, errlen, "LENT chunk cut off"); return -1; }
    count = ri32(d + lent + 0x30);
    e->at = lent;
    e->party_state = ru32(d + lent + 0x0c);
    o = lent + 0x34;
    for (i = 0; i < count && o + 0x180 <= (int64_t)size; i++) {
        int32_t type = ri32(d + o + 4);
        if (e->count == cap) { cap = cap ? cap * 2 : 64; e->list = xrealloc(e->list, (size_t)cap * sizeof(se_entity)); }
        e->list[e->count].index = i;
        e->list[e->count].at = o;
        e->list[e->count].hash = ru32(d + o);
        e->list[e->count].type = type;
        e->count++;
        o += 4 + 0x17c + (type == 7 ? 8 : 0);
    }
    return 0;
}
int se_read_entities(const se_save *s, se_entities *e, char *err, size_t errlen) { return entities_raw(s->data, s->size, e, err, errlen); }
void se_entities_free(se_entities *e) { free(e->list); memset(e, 0, sizeof *e); }

int se_party(const se_save *s, se_member *out, int max, char *err, size_t errlen)
{
    se_entities e;
    int i, k, n = 0;
    if (entities_raw(s->data, s->size, &e, err, errlen)) return -1;
    for (i = 0; i < e.count; i++) {
        const unsigned char *r;
        se_member *m;
        if (e.list[i].type != 7) continue;
        if (n < max && out) {
            m = &out[n];
            memset(m, 0, sizeof *m);
            r = s->data + e.list[i].at + 4;
            m->record = e.list[i].index;
            m->at = e.list[i].at;
            m->hash = e.list[i].hash;
            for (k = 0; k < 4; k++) if (se_name_hash(party_names[k]) == m->hash) m->name = party_names[k];
            for (k = 0; k < 9; k++) m->equip[k] = ri32(r + 0x6c + k * 4);
            memcpy(m->skills, r + 0x120, SE_SKILL_COUNT);
            m->level = ri16(r + 0x144);
            m->xp = ri32(r + 0x146);
            m->hp = rf32(r + 0x14a);
            m->maxhp = rf32(r + 0x14e);
            m->ap = ri16(r + 0x152);
            m->maxap = ri16(r + 0x154);
            m->skillpoints_ok = inb(s->size, e.list[i].at + 4 + 0x180, 4);
            m->skillpoints = m->skillpoints_ok ? ri32(r + 0x180) : 0;
            m->dead = (ru32(r + 0x1c) & 0x80) != 0;
        }
        n++;
    }
    se_entities_free(&e);
    return n;
}

/* levelBlocks (savegame_layout_level_data) */
static int level_blocks_raw(const unsigned char *d, size_t size, const se_tables *t, se_level_block out[SE_LEVEL_COUNT], char *err, size_t errlen)
{
    se_sections off;
    int64_t base, o;
    int i;
    if (!t || !t->have_events) { seterr(err, errlen, "events.tbl not loaded"); return -1; }
    offsets_raw(d, &off);
    base = off.at[1];
    o = base + 0xcc;
    for (i = 0; i < SE_LEVEL_COUNT; i++) {
        const se_level_def *lv = &t->levels[i];
        uint32_t bytes = to_uint32((double)lv->nflags + lv->layers * 4096 + 7) >> 3;
        int64_t sz = (int64_t)bytes + 4 * ((int64_t)lv->nints + lv->nfloats);
        out[i].index = i;
        out[i].name = se_level_names[i];
        out[i].offset = o;
        out[i].size = sz;
        out[i].ints_at = o + bytes;
        out[i].visits = inb(size, base + 4 * i, 4) ? ri32(d + base + 4 * i) : 0;
        out[i].level = lv;
        o += sz;
    }
    if (o - base != size_of(d, 1)) {
        char why[96];
        if (t->nmissing > 1) snprintf(why, sizeof why, "; %d level tables (<level>.tbl) were not found", t->nmissing);
        else snprintf(why, sizeof why, " (tables of another game version, or a modded table?)");
        seterr(err, errlen, "the tables give %lld bytes of level data, the save has %ld%s", (long long)(o - base), (long)size_of(d, 1), why);
        return -1;
    }
    if (!inb(size, base, 0xcc) || !inb(size, base + 0xcc, o - base - 0xcc)) { seterr(err, errlen, "level data runs past the file"); return -1; }
    return 0;
}
int se_level_blocks(const se_save *s, const se_tables *t, se_level_block out[SE_LEVEL_COUNT], char *err, size_t errlen)
{
    return level_blocks_raw(s->data, s->size, t, out, err, errlen);
}
static int get_flag_raw(const unsigned char *d, const se_level_block *b, int i) { return (d[b->offset + (i >> 3)] >> (i & 7)) & 1; }
static void set_flag_raw(unsigned char *d, const se_level_block *b, int i, int v)
{
    unsigned char *p = d + b->offset + (i >> 3);
    if (v) *p |= (unsigned char)(1 << (i & 7)); else *p &= (unsigned char)~(1 << (i & 7));
}
int se_get_flag(const se_save *s, const se_level_block *b, int i) { return get_flag_raw(s->data, b, i); }
int32_t se_level_int(const se_save *s, const se_level_block *b, int i) { return ri32(s->data + b->ints_at + 4 * i); }
float se_level_float(const se_save *s, const se_level_block *b, int i) { return rf32(s->data + b->ints_at + 4 * (b->level->nints + i)); }

/* questBit / setQuestBit: a byte outside the buffer reads 0 and is not written (Buffer indexing) */
static int quest_bit_raw(const unsigned char *d, size_t size, int n)
{
    se_sections off;
    int64_t o;
    offsets_raw(d, &off);
    o = off.at[8] + (n >> 3);
    return inb(size, o, 1) ? (d[o] >> (n & 7)) & 1 : 0;
}
static void set_quest_bit_raw(unsigned char *d, size_t size, int n, int v)
{
    se_sections off;
    int64_t o;
    offsets_raw(d, &off);
    o = off.at[8] + (n >> 3);
    if (!inb(size, o, 1)) return;
    if (v) d[o] |= (unsigned char)(1 << (n & 7)); else d[o] &= (unsigned char)~(1 << (n & 7));
}
int se_quest_bit(const se_save *s, int n) { return quest_bit_raw(s->data, s->size, n); }

int se_find_level(const char *name)
{
    size_t n = strlen(name);
    int i;
    if (all_digits(name, n)) { int64_t v = digits_value(name, n); return v < SE_LEVEL_COUNT ? (int)v : -1; }
    for (i = 0; i < SE_LEVEL_COUNT; i++) if (ieq(se_level_names[i], name)) return i;
    return -1;
}

/* ------------------------------------------------------------------------------------------------------ */
/* Edits (tools/savedit.mjs) */

static const char *const op_names[SE_OP_COUNT] = {
    "desc", "gold", "add-item", "remove-item", "set-item-count", "flag", "member", "level", "xp", "hp", "maxhp",
    "ap", "maxap", "skillpoints", "skill"};
int se_op_kind_from_name(const char *name)
{
    int i;
    for (i = 0; i < SE_OP_COUNT; i++) if (!strcmp(op_names[i], name)) return i;
    return -1;
}
const char *se_op_name(se_op_kind k) { return (unsigned)k < SE_OP_COUNT ? op_names[k] : "?"; }

typedef struct { uint32_t hash; int64_t count; int64_t field; } ientry;

typedef struct {
    unsigned char *b;
    size_t size;
    const se_tables *t;
    char *log;
    size_t loglen, logpos;
    char *err;
    size_t errlen;
    uint32_t *types;
    int ntypes;
    ientry *e;
    int ne, ecap;
    int64_t *party;      /* record offsets */
    uint32_t *phash;
    int nparty;
    int member;
    int64_t *seen;
    int nseen, seencap;
    int have_blocks;
    se_level_block blocks[SE_LEVEL_COUNT];
} ectx;

static void logf_(ectx *c, const char *fmt, ...)
{
    va_list ap;
    int w;
    if (!c->log || c->logpos + 1 >= c->loglen) return;
    va_start(ap, fmt);
    w = vsnprintf(c->log + c->logpos, c->loglen - c->logpos, fmt, ap);
    va_end(ap);
    if (w < 0) return;
    c->logpos += (size_t)w;
    if (c->logpos >= c->loglen) c->logpos = c->loglen - 1;
    if (c->logpos + 1 < c->loglen) { c->log[c->logpos++] = '\n'; c->log[c->logpos] = 0; }
}
#define FAIL(...) do { seterr(c->err, c->errlen, __VA_ARGS__); return -1; } while (0)

/* int(): /^-?\d+$/ and lo <= Number(v) <= hi */
static int parse_int(ectx *c, const char *k, const char *v, int64_t lo, int64_t hi, int64_t *out)
{
    size_t n = strlen(v), i = 0;
    int64_t x;
    if (n && v[0] == '-') i = 1;
    if (!all_digits(v + i, n - i)) FAIL("--%s needs a whole number from %lld to %lld, not \"%s\"", k, (long long)lo, (long long)hi, v);
    x = digits_value(v + i, n - i);
    if (i) x = -x;
    if (x < lo || x > hi) FAIL("--%s needs a whole number from %lld to %lld, not \"%s\"", k, (long long)lo, (long long)hi, v);
    *out = x;
    return 0;
}
/* real(): /^-?\d+(\.\d+)?$/, Number(v) */
static int parse_real(ectx *c, const char *k, const char *v, double *out)
{
    size_t n = strlen(v), i = 0, j;
    if (n && v[0] == '-') i = 1;
    j = i;
    while (j < n && v[j] >= '0' && v[j] <= '9') j++;
    if (j == i) FAIL("--%s needs a number, not \"%s\"", k, v);
    if (j < n) {
        size_t f;
        if (v[j] != '.') FAIL("--%s needs a number, not \"%s\"", k, v);
        f = ++j;
        while (j < n && v[j] >= '0' && v[j] <= '9') j++;
        if (j == f || j != n) FAIL("--%s needs a number, not \"%s\"", k, v);
    }
    *out = strtod(v, NULL);
    return 0;
}

static int has_type(const ectx *c, uint32_t h)
{
    int i;
    for (i = 0; i < c->ntypes; i++) if (c->types[i] == h) return 1;
    return 0;
}

/* itemArg(): "Name" or "Name=N" (/^(.*?)(?:=(-?\d+))?$/), checked against the save's item-type list */
static int item_arg(ectx *c, const char *k, const char *v, int need_count, int64_t min_count,
                    uint32_t *hash, int *has_count, int64_t *count, const char **label)
{
    size_t n = strlen(v), i, split = n, b, e;
    const char *eq = strrchr(v, '=');
    char *name, *cnt = NULL;
    static char labelbuf[512];
    for (i = 0; i < n; i++) if (jsnl((unsigned char)v[i])) FAIL("--%s: \"%s\" is not an item argument", k, v);
    if (eq) {
        size_t p = (size_t)(eq - v) + 1, q = p;
        if (q < n && v[q] == '-') q++;
        if (all_digits(v + q, n - q)) { split = (size_t)(eq - v); cnt = (char *)v + p; }
    }
    trim_span(v, split, &b, &e);
    if (e == b) FAIL("--%s needs an item name", k);
    if (need_count && !cnt) FAIL("--%s needs Name=N", k);
    *has_count = cnt != NULL;
    if (cnt && parse_int(c, k, cnt, min_count, 0x7fffffff, count)) return -1;
    name = xstrndup(v + b, e - b);
    *hash = se_name_hash(name);
    if (!has_type(c, *hash)) {
        char hint[400] = "";
        if (c->t && c->t->have_items) {
            int found = 0, j;
            size_t hl = 0;
            for (j = 0; j < c->t->nitems && found < 6; j++) {
                const char *nm = c->t->items[j].name;
                size_t L = strlen(nm), Q = strlen(name), x, y;
                int hit = Q == 0;
                for (x = 0; !hit && x + Q <= L; x++) {
                    for (y = 0; y < Q && lc1((unsigned char)nm[x + y]) == lc1((unsigned char)name[y]); y++) {}
                    if (y == Q) hit = 1;
                }
                if (!hit) continue;
                hl += (size_t)snprintf(hint + hl, sizeof hint - hl, "%s%s", found ? ", " : "; similar names: ", nm);
                if (hl >= sizeof hint) hl = sizeof hint - 1;
                found++;
            }
        } else snprintf(hint, sizeof hint, " (items.tbl not loaded, so no suggestions)");
        seterr(c->err, c->errlen, "--%s: unknown item \"%s\" (hash 0x%08lx is not an item type of this save)%s", k, name, (unsigned long)*hash, hint);
        free(name);
        return -1;
    }
    {
        const char *tn = se_item_name_of(c->t, *hash);
        snprintf(labelbuf, sizeof labelbuf, "%s", tn ? tn : name);
        *label = labelbuf;
    }
    free(name);
    return 0;
}

static int64_t held(const ectx *c, uint32_t h)
{
    int64_t a = 0;
    int i;
    for (i = 0; i < c->ne; i++) if (c->e[i].hash == h) a += c->e[i].count;
    return a;
}
static void e_push(ectx *c, uint32_t h, int64_t count, int64_t field)
{
    if (c->ne == c->ecap) { c->ecap = c->ecap ? c->ecap * 2 : 64; c->e = xrealloc(c->e, (size_t)c->ecap * sizeof(ientry)); }
    c->e[c->ne].hash = h; c->e[c->ne].count = count; c->e[c->ne].field = field;
    c->ne++;
}
static void e_remove(ectx *c, int i)
{
    memmove(c->e + i, c->e + i + 1, (size_t)(c->ne - i - 1) * sizeof(ientry));
    c->ne--;
}

static int member_rec(ectx *c, int64_t *r)
{
    if (c->member >= c->nparty) FAIL("party member %d not found (%d in this save)", c->member, c->nparty);
    *r = c->party[c->member] + 4;
    return 0;
}
static int seen_once(ectx *c, int64_t key)
{
    int i;
    for (i = 0; i < c->nseen; i++) if (c->seen[i] == key) return 0;
    if (c->nseen == c->seencap) { c->seencap = c->seencap ? c->seencap * 2 : 16; c->seen = xrealloc(c->seen, (size_t)c->seencap * sizeof(int64_t)); }
    c->seen[c->nseen++] = key;
    return 1;
}

static int level_data(ectx *c)
{
    char why[200];
    if (c->have_blocks) return 0;
    if (level_blocks_raw(c->b, c->size, c->t, c->blocks, why, sizeof why)) FAIL("--flag: cannot place story flags safely: %s", why);
    c->have_blocks = 1;
    return 0;
}

static int find_flag(const se_level_block *blk, uint32_t h)
{
    int i;
    for (i = 0; i < blk->level->nflags; i++) if (se_name_hash(blk->level->flags[i]) == h) return i;
    return -1;
}

static int op_flag(ectx *c, const char *v)
{
    size_t n = strlen(v), colon, b, e;
    const char *cp = strchr(v, ':');
    char *lname, *name;
    int li, fi, val, old, q;
    uint32_t h;
    se_level_block *blk;
    /* /^([^:]+):(.+)=([01])$/ */
    if (!cp || cp == v || n < 4 || v[n - 2] != '=' || (v[n - 1] != '0' && v[n - 1] != '1') || (size_t)(cp - v) + 1 >= n - 2)
        FAIL("--flag needs LEVEL:NAME=0 or LEVEL:NAME=1, not \"%s\"", v);
    colon = (size_t)(cp - v);
    for (b = colon + 1; b < n - 2; b++) if (jsnl((unsigned char)v[b])) FAIL("--flag needs LEVEL:NAME=0 or LEVEL:NAME=1, not \"%s\"", v);
    lname = trim_dup(v, colon);
    li = se_find_level(lname);
    if (li < 0) {
        seterr(c->err, c->errlen, "--flag: unknown level \"%.*s\"", (int)colon, v);
        free(lname);
        return -1;
    }
    free(lname);
    if (level_data(c)) return -1;
    blk = &c->blocks[li];
    trim_span(v + colon + 1, n - 2 - colon - 1, &b, &e);
    name = xstrndup(v + colon + 1 + b, e - b);
    val = v[n - 1] - '0';
    h = se_name_hash(name);
    fi = find_flag(blk, h);
    if (fi < 0) {
        int o;
        for (o = 0; o < SE_LEVEL_COUNT; o++) if (find_flag(&c->blocks[o], h) >= 0) break;
        if (o < SE_LEVEL_COUNT) seterr(c->err, c->errlen, "--flag: level %s has no flag \"%s\"; it belongs to level %s", blk->name, name, c->blocks[o].name);
        else seterr(c->err, c->errlen, "--flag: level %s has no flag \"%s\" (not in events.tbl)", blk->name, name);
        free(name);
        return -1;
    }
    free(name);
    old = get_flag_raw(c->b, blk, fi);
    set_flag_raw(c->b, blk, fi, val);
    logf_(c, "flag %s:%s %d -> %d", blk->name, blk->level->flags[fi], old, val);
    if (val && c->t && c->t->have_events) {
        for (q = 0; q < c->t->nquests; q++) {
            const se_quest *qq = &c->t->quests[q];
            if (qq->invalidate && qq->invalidate[0] && se_name_hash(qq->invalidate) == h && quest_bit_raw(c->b, c->size, qq->index)) {
                set_quest_bit_raw(c->b, c->size, qq->index, 0);
                logf_(c, "  journal quest \"%s\" removed (its +Invalidate: flag, as quest_evaluate does)", qq->name);
            }
        }
    }
    return 0;
}

static int op_member(ectx *c, const char *v)
{
    size_t n = strlen(v);
    int64_t r;
    if (all_digits(v, n)) {
        int64_t x = digits_value(v, n);
        c->member = x > 0x7fffffff ? 0x7fffffff : (int)x;
    } else {
        int k, i;
        c->member = -1;
        for (k = 0; k < 4; k++) if (ieq(v, party_names[k])) break;
        if (k < 4) {
            uint32_t h = se_name_hash(v);
            for (i = 0; i < c->nparty; i++) if (c->phash[i] == h) { c->member = i; break; }
        }
        if (c->member < 0) FAIL("--member: no party member named \"%s\" in this save", v);
    }
    return member_rec(c, &r);
}

static int op_field(ectx *c, se_op_kind k, const char *v)
{
    int64_t r, x;
    double d;
    const char *kn = op_names[k];
    if (member_rec(c, &r)) return -1;
    if (!seen_once(c, (int64_t)c->member * 64 + k)) FAIL("--%s given twice for member %d", kn, c->member);
    switch (k) {
    case SE_OP_LEVEL: if (parse_int(c, kn, v, -0x8000, 0x7fff, &x)) return -1; wi16(c->b + r + 0x144, (int)x); break;
    case SE_OP_XP: if (parse_int(c, kn, v, -0x80000000LL, 0x7fffffff, &x)) return -1; wu32(c->b + r + 0x146, (uint32_t)x); break;
    case SE_OP_HP: if (parse_real(c, kn, v, &d)) return -1; wf32(c->b + r + 0x14a, (float)d); break;
    case SE_OP_MAXHP: if (parse_real(c, kn, v, &d)) return -1; wf32(c->b + r + 0x14e, (float)d); break;
    case SE_OP_AP: if (parse_int(c, kn, v, -0x8000, 0x7fff, &x)) return -1; wi16(c->b + r + 0x152, (int)x); break;
    case SE_OP_MAXAP: if (parse_int(c, kn, v, -0x8000, 0x7fff, &x)) return -1; wi16(c->b + r + 0x154, (int)x); break;
    case SE_OP_SKILLPOINTS:
        if (parse_int(c, kn, v, -0x80000000LL, 0x7fffffff, &x)) return -1;
        if (!inb(c->size, r + 0x180, 4)) FAIL("--skillpoints: the member record runs past the end of the file");
        wu32(c->b + r + 0x180, (uint32_t)x);
        break;
    default: FAIL("internal: bad field");
    }
    logf_(c, "member %d %s = %s", c->member, kn, v);
    return 0;
}

static int op_skill(ectx *c, const char *v)
{
    int64_t r, id = -1, val = -1;
    const char *eq = strchr(v, '=');
    int old;
    if (member_rec(c, &r)) return -1;
    if (eq && all_digits(v, (size_t)(eq - v)) && all_digits(eq + 1, strlen(eq + 1))) {
        id = digits_value(v, (size_t)(eq - v));
        val = digits_value(eq + 1, strlen(eq + 1));
    }
    if (!(id >= 0 && id < SE_SKILL_COUNT) || !(val >= 0 && val < 256)) FAIL("--skill needs ID=VALUE with ID 0-35 and VALUE 0-255");
    if (!seen_once(c, (int64_t)c->member * 64 + 16 + id)) FAIL("--skill %d given twice for member %d", (int)id, c->member);
    old = c->b[r + 0x120 + id];
    c->b[r + 0x120 + id] = (unsigned char)val;
    logf_(c, "member %d skill 0x%x %d -> %d", c->member, (int)id, old, (int)val);
    return 0;
}

/* writeInventory: the entries, the count and the trailing length; the rest of the capacity zeroed */
static int write_inventory(ectx *c)
{
    int64_t o = chunk_at_raw(c->b, c->size, "INVT"), cap = size_of(c->b, 4);
    int i;
    if (c->ne > SE_INVT_MAX_ENTRIES || 16 + 12 * (int64_t)c->ne + 4 > cap) FAIL("the party inventory is full");
    if (o < 0 || !inb(c->size, o + 8, cap - 8)) FAIL("the INVT section runs past the file");
    memset(c->b + o + 8, 0, (size_t)(cap - 8));
    wu32(c->b + o + 8, (uint32_t)c->ne);
    for (i = 0; i < c->ne; i++) {
        unsigned char *p = c->b + o + 12 + 12 * i;
        if (c->e[i].count < -0x80000000LL || c->e[i].count > 0x7fffffff || c->e[i].field < -0x80000000LL || c->e[i].field > 0x7fffffff)
            FAIL("an inventory count is out of range");
        wu32(p, c->e[i].hash);
        wu32(p + 4, (uint32_t)c->e[i].count);
        wu32(p + 8, (uint32_t)c->e[i].field);
    }
    wu32(c->b + o + 12 + 12 * c->ne, (uint32_t)(12 + 12 * c->ne + 4));
    return 0;
}

static int apply_all(ectx *c, const se_op *ops, int nops)
{
    int i, inv_changed = 0;
    se_inventory inv;
    se_entities ents;
    char why[200];
    for (i = 0; i < nops; i++) if (ops[i].kind != SE_OP_MEMBER) break;
    if (i == nops) FAIL("no edits given");
    {
        int ndesc = 0, ngold = 0;
        for (i = 0; i < nops; i++) {
            if ((unsigned)ops[i].kind >= SE_OP_COUNT || !ops[i].value) FAIL("bad edit %d", i);
            if (ops[i].kind == SE_OP_DESC && ++ndesc > 1) FAIL("--desc given twice");
            if (ops[i].kind == SE_OP_GOLD && ++ngold > 1) FAIL("--gold given twice");
        }
    }
    if (item_types_raw(c->b, c->size, &c->types, &c->ntypes, why, sizeof why)) FAIL("%s", why);
    if (read_inv_raw(c->b, c->size, &inv, why, sizeof why)) FAIL("%s", why);
    for (i = 0; i < inv.count; i++) e_push(c, inv.entries[i].hash, inv.entries[i].count, inv.entries[i].field);
    se_inventory_free(&inv);
    if (entities_raw(c->b, c->size, &ents, NULL, 0) == 0) {
        c->party = xmalloc((size_t)ents.count * sizeof(int64_t));
        c->phash = xmalloc((size_t)ents.count * sizeof(uint32_t));
        for (i = 0; i < ents.count; i++)
            if (ents.list[i].type == 7) { c->party[c->nparty] = ents.list[i].at; c->phash[c->nparty] = ents.list[i].hash; c->nparty++; }
        se_entities_free(&ents);
    }
    c->member = 0;

    for (i = 0; i < nops; i++) {
        const char *v = ops[i].value;
        se_op_kind k = ops[i].kind;
        const char *kn = op_names[k];
        if (k == SE_OP_DESC) {
            if (strlen(v) > 255) FAIL("--desc is longer than 255 characters");
            memset(c->b + 0xb0, 0, 0x100);
            memcpy(c->b + 0xb0, v, strlen(v));
            logf_(c, "description = \"%s\"", v);
        } else if (k == SE_OP_GOLD) {
            int64_t o = chunk_at_raw(c->b, c->size, "INVT"), g;
            int32_t old = ri32(c->b + o + 4);
            if (parse_int(c, kn, v, -0x80000000LL, 0x7fffffff, &g)) return -1;
            wu32(c->b + o + 4, (uint32_t)g);
            logf_(c, "gold %ld -> %lld", (long)old, (long long)g);
        } else if (k == SE_OP_ADD_ITEM) {
            uint32_t h; int hc; int64_t cnt = 1; const char *label;
            if (item_arg(c, kn, v, 0, 1, &h, &hc, &cnt, &label)) return -1;
            if (!hc) cnt = 1;
            e_push(c, h, cnt, 0);
            inv_changed = 1;
            logf_(c, "added %lld x \"%s\" (hash 0x%08lx) to the party inventory", (long long)cnt, label, (unsigned long)h);
        } else if (k == SE_OP_REMOVE_ITEM) {
            uint32_t h; int hc, j; int64_t cnt = 0, have, nleft, removed; const char *label;
            if (item_arg(c, kn, v, 0, 1, &h, &hc, &cnt, &label)) return -1;
            have = held(c, h);
            if (!have) FAIL("--remove-item: the party has no \"%s\"", label);
            nleft = hc ? cnt : have;
            if (nleft > have) FAIL("--remove-item: the party has only %lld x \"%s\"", (long long)have, label);
            removed = nleft;
            for (j = c->ne - 1; j >= 0 && nleft > 0; j--) {
                int64_t take;
                if (c->e[j].hash != h) continue;
                take = nleft < c->e[j].count ? nleft : c->e[j].count;
                c->e[j].count -= take;
                nleft -= take;
                if (c->e[j].count <= 0) e_remove(c, j);
            }
            inv_changed = 1;
            logf_(c, "removed %lld x \"%s\" (%lld -> %lld)", (long long)removed, label, (long long)have, (long long)(have - removed));
        } else if (k == SE_OP_SET_ITEM_COUNT) {
            uint32_t h; int hc, j, first = -1; int64_t cnt = 0, have; const char *label;
            if (item_arg(c, kn, v, 1, 0, &h, &hc, &cnt, &label)) return -1;
            have = held(c, h);
            for (j = 0; j < c->ne; j++) if (c->e[j].hash == h) { first = j; break; }
            if (cnt == 0) {
                for (j = c->ne - 1; j >= 0; j--) if (c->e[j].hash == h) e_remove(c, j);
            } else if (first < 0) e_push(c, h, cnt, 0);
            else {
                c->e[first].count = cnt;
                for (j = c->ne - 1; j > first; j--) if (c->e[j].hash == h) e_remove(c, j);
            }
            inv_changed = 1;
            logf_(c, "\"%s\" count %lld -> %lld", label, (long long)have, (long long)cnt);
        } else if (k == SE_OP_FLAG) {
            if (op_flag(c, v)) return -1;
        } else if (k == SE_OP_MEMBER) {
            if (op_member(c, v)) return -1;
        } else if (k == SE_OP_SKILL) {
            if (op_skill(c, v)) return -1;
        } else {
            if (op_field(c, k, v)) return -1;
        }
    }
    if (inv_changed && write_inventory(c)) return -1;
    return 0;
}

int se_apply(se_save *s, const se_tables *t, const se_op *ops, int nops,
             char *log, size_t loglen, char *err, size_t errlen)
{
    ectx c;
    int rc;
    memset(&c, 0, sizeof c);
    if (!s || !s->data || se_check(s->data, s->size, NULL, 0)) { seterr(err, errlen, "no valid save loaded"); return -1; }
    c.size = s->size;
    c.b = xmalloc(s->size);
    memcpy(c.b, s->data, s->size);
    c.t = t;
    c.log = log; c.loglen = loglen;
    if (log && loglen) log[0] = 0;
    c.err = err; c.errlen = errlen;
    rc = apply_all(&c, ops, nops);
    if (rc == 0) memcpy(s->data, c.b, s->size);
    else if (log && loglen) log[0] = 0;
    free(c.b); free(c.types); free(c.e); free(c.party); free(c.phash); free(c.seen);
    return rc;
}
