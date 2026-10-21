// The experimental build's switches and log (include/experimental/experimental.h). Not part of Sum.exe: compiled
// only for link-build --config experimental.
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

// <exe folder>\<name>
static void exp_path(const char *name, char *out, int n)
{
    char *slash;
    if (!GetModuleFileNameA(NULL, out, n))
        out[0] = 0;
    out[n - 1] = 0;
    slash = strrchr(out, '\\');
    if (slash)
        slash[1] = 0;
    else
        out[0] = 0;
    if ((int)(strlen(out) + strlen(name)) < n)
        strcat(out, name);
}

static const char *exp_ini()
{
    static char path[MAX_PATH];
    if (!path[0])
        exp_path("summonerfix.ini", path, MAX_PATH);
    return path;
}

int exp_ini_int(const char *section, const char *key, int def)
{
    return (int)GetPrivateProfileIntA(section, key, def, exp_ini());
}

int exp_ini_string(const char *section, const char *key, const char *def, char *out, int n)
{
    return (int)GetPrivateProfileStringA(section, key, def, out, n, exp_ini());
}

void exp_log(const char *fmt, ...)
{
    static int enabled = -1;
    static char path[MAX_PATH];
    char line[1024];
    va_list ap;
    FILE *f;

    if (enabled < 0) {
        enabled = exp_ini_int("Debug", "ExperimentalLog", 0) != 0;
        exp_path("experimental.log", path, MAX_PATH);
    }
    if (!enabled)
        return;
    va_start(ap, fmt);
    _vsnprintf(line, sizeof line - 1, fmt, ap);
    va_end(ap);
    line[sizeof line - 1] = 0;
    f = fopen(path, "a");
    if (f) {
        fprintf(f, "%lu %s\n", (unsigned long)GetTickCount(), line);
        fclose(f);
    }
}

#endif // EXPERIMENTAL_BUILD
