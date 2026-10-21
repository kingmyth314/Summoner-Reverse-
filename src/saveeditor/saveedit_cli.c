/* saveedit_cli.c: the command line of tools/savedit.mjs on top of the C library (saveedit.h).
 *
 * usage: saveedit <in.sav> <out.sav> [edits...] [--tables FILE.vpp] [--tables-dir DIR] [--items-tbl FILE]
 *                 [--events-tbl FILE] [--lang NAME]
 *        saveedit --roundtrip <in.sav> <out.sav>      (load, check and write the save with no edit)
 * The edits are those of savedit.mjs (--desc, --gold, --add-item, --remove-item, --set-item-count, --flag,
 * --member, --level, --xp, --hp, --maxhp, --ap, --maxap, --skillpoints, --skill). The output must not exist
 * and must be a different file than the input; one bad edit writes nothing. Exit code 0 = written, 1 = refused.
 * Without --tables the packfile is tables.vpp next to saveedit.exe, else Summoner\tables.vpp under the current
 * folder (the repository's default). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saveedit.h"

static const char USAGE[] =
    "usage: saveedit <in.sav> <out.sav> [--desc text] [--gold N] [--add-item Name[=N]] [--remove-item Name[=N]] "
    "[--set-item-count Name=N] [--flag LEVEL:NAME=0|1] [--member I|NAME] [--level N] [--xp N] [--hp N] [--maxhp N] "
    "[--ap N] [--maxap N] [--skillpoints N] [--skill ID=VALUE] [--tables FILE.vpp] [--tables-dir DIR] "
    "[--items-tbl FILE] [--events-tbl FILE] [--lang NAME]\n"
    "       saveedit --roundtrip <in.sav> <out.sav>";

static int fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    return 1;
}

/* A command-line string as Node's Buffer.write(..., "latin1") stores it: the low byte of each UTF-16 unit. */
static char *latin1(const wchar_t *w)
{
    size_t n = wcslen(w), i;
    char *s = malloc(n + 1);
    if (!s) { fputs("out of memory\n", stderr); exit(3); }
    for (i = 0; i < n; i++) s[i] = (char)(unsigned char)(w[i] & 0xff);
    s[n] = 0;
    return s;
}

static int starts_dashdash(const wchar_t *w) { return w[0] == L'-' && w[1] == L'-'; }

static int same_file(const wchar_t *a, const wchar_t *b)
{
    wchar_t fa[MAX_PATH * 4], fb[MAX_PATH * 4];
    DWORD na = GetFullPathNameW(a, MAX_PATH * 4, fa, NULL), nb = GetFullPathNameW(b, MAX_PATH * 4, fb, NULL);
    if (!na || !nb || na >= MAX_PATH * 4 || nb >= MAX_PATH * 4) return wcscmp(a, b) == 0;
    return CompareStringOrdinal(fa, -1, fb, -1, TRUE) == CSTR_EQUAL;
}
static int exists(const wchar_t *p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }

static void print_latin1(FILE *f, const char *s)
{
    /* the console shows Latin-1 bytes through the ANSI code page; good enough for a log */
    fputs(s, f);
}

/* tables.vpp next to the program, else Summoner\tables.vpp under the current folder */
static wchar_t *default_tables(void)
{
    static wchar_t p[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(NULL, p, MAX_PATH * 4);
    if (n && n < MAX_PATH * 4 - 16) {
        wchar_t *slash = wcsrchr(p, L'\\');
        if (slash) {
            wcscpy(slash + 1, L"tables.vpp");
            if (exists(p)) return p;
        }
    }
    wcscpy(p, L"Summoner\\tables.vpp");
    return p;
}

static int roundtrip(int argc, wchar_t **argv)
{
    se_save s;
    char err[512];
    if (argc != 4 || starts_dashdash(argv[2]) || starts_dashdash(argv[3])) return fail(USAGE);
    if (same_file(argv[2], argv[3])) return fail("refusing: output must be a different file than the input");
    if (exists(argv[3])) return fail("refusing: the output already exists (saveedit never overwrites files)");
    if (se_load_file(&s, argv[2], err, sizeof err)) { fprintf(stderr, "input: %s\n", err); return 1; }
    if (se_write_new(&s, argv[3], err, sizeof err)) { se_free(&s); return fail(err); }
    printf("wrote the save unchanged (%lu bytes)\n", (unsigned long)s.size);
    se_free(&s);
    return 0;
}

int wmain(int argc, wchar_t **argv)
{
    static const wchar_t *const table_names[5] = {L"tables", L"tables-dir", L"items-tbl", L"events-tbl", L"lang"};
    const wchar_t *table_vals[5] = {0};
    se_op *ops;
    int nops = 0, i, rc;
    se_save s;
    se_tables t;
    se_table_opts to;
    char err[1024];
    static char log[65536];
    char *lang = NULL;

    if (argc >= 2 && !wcscmp(argv[1], L"--roundtrip")) return roundtrip(argc, argv);
    if (argc < 3 || starts_dashdash(argv[1]) || starts_dashdash(argv[2])) return fail(USAGE);
    if (same_file(argv[1], argv[2])) return fail("refusing: output must be a different file than the input");
    if (exists(argv[2])) return fail("refusing: the output already exists (saveedit never overwrites files)");

    ops = calloc((size_t)argc, sizeof(se_op));
    if (!ops) return fail("out of memory");
    for (i = 3; i < argc; i++) {
        char *k;
        int kind, ti;
        if (!starts_dashdash(argv[i])) { char *a = latin1(argv[i]); fprintf(stderr, "unexpected argument %s\n", a); free(a); return 1; }
        k = latin1(argv[i] + 2);
        kind = se_op_kind_from_name(k);
        for (ti = 0; ti < 5; ti++) if (!wcscmp(argv[i] + 2, table_names[ti])) break;
        if (kind < 0 && ti == 5) { fprintf(stderr, "unknown option --%s\n", k); return 1; }
        if (i + 1 >= argc) { fprintf(stderr, "--%s needs a value\n", k); return 1; }
        i++;
        if (ti < 5) {
            if (table_vals[ti]) { fprintf(stderr, "--%s given twice\n", k); return 1; }
            table_vals[ti] = argv[i];
        } else {
            if ((kind == SE_OP_DESC || kind == SE_OP_GOLD))
                for (rc = 0; rc < nops; rc++)
                    if (ops[rc].kind == (se_op_kind)kind) { fprintf(stderr, "--%s given twice\n", k); return 1; }
            ops[nops].kind = (se_op_kind)kind;
            ops[nops].value = latin1(argv[i]);
            nops++;
        }
        free(k);
    }
    for (i = 0; i < nops; i++) if (ops[i].kind != SE_OP_MEMBER) break;
    if (i == nops) return fail("no edits given");

    if (se_load_file(&s, argv[1], err, sizeof err)) {
        char *a = latin1(argv[1]);
        fprintf(stderr, "%s: %s\n", a, err);
        free(a);
        return 1;
    }
    if (table_vals[4]) lang = latin1(table_vals[4]);
    to.tables_vpp = table_vals[0] ? table_vals[0] : default_tables();
    to.tables_dir = table_vals[1];
    to.items_tbl = table_vals[2];
    to.events_tbl = table_vals[3];
    to.lang = lang;
    se_tables_load(&t, &to);

    rc = se_apply(&s, &t, ops, nops, log, sizeof log, err, sizeof err);
    if (rc == 0) rc = se_write_new(&s, argv[2], err, sizeof err);
    if (rc) {
        print_latin1(stderr, err);
        fputc('\n', stderr);
    } else {
        char *a = latin1(argv[2]), *line = log, *nl;
        printf("wrote %s:\n", a);
        free(a);
        while (*line) {
            nl = strchr(line, '\n');
            if (nl) *nl = 0;
            printf("  ");
            print_latin1(stdout, line);
            putchar('\n');
            if (!nl) break;
            line = nl + 1;
        }
        for (i = 0; i < t.nwarnings; i++) printf("note: %s\n", t.warnings[i]);
    }
    se_tables_free(&t);
    se_free(&s);
    for (i = 0; i < nops; i++) free((char *)ops[i].value);
    free(ops);
    free(lang);
    return rc ? 1 : 0;
}
