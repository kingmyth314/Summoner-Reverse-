// vsdk/localize/localize.h - the string table (sum.symbols module "localize", 0x505070-0x505510; directory
// and file name guessed). Not original source.
#ifndef VSDK_LOCALIZE_LOCALIZE_H
#define VSDK_LOCALIZE_LOCALIZE_H

#include "vsdk/vsdk.h"

extern char *localize_strings[1000];                     // 0x25cd4dc
// The id indexes localize_strings; the English text is the source comment (Volition's XSTR convention).
#define XSTR(text, id) (localize_strings[id])

char *localize_quest_name(const char *name);             // 0x505070
void  localize_remap_charset(char *text, int to_font);   // 0x5051a0
char *localize_string_or_invalid(char **p);              // 0x5054f0: *p, or "INVALID"
char *localize_get_language_name();                      // 0x505510
extern int localize_french_punct;                        // 0x25d8870 set for the French string table

#endif
