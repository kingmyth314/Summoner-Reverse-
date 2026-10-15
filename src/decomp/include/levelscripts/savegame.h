// levelscripts/savegame.h - the savegame object (savegame_* 0x4cfca0-0x4d4a50, between
// level_scripts_common.cpp and level_scripts.cpp; unit name not known, file name ours).
// Not original source. One merged class of the three stub copies (DECISIONS.md D14).
#ifndef LEVELSCRIPTS_SAVEGAME_H
#define LEVELSCRIPTS_SAVEGAME_H

#include "vsdk/vsdk.h"

class savegame {                     // savegame_state at 0x2493a38 (thiscall members; layout not needed yet)
public:
    int  get_flag(int level, const char *name);                      // 0x4cfca0 savegame_get_flag
    int  get_quest_bit(int quest);                                   // 0x4d0c00 0 or 1 in EAX (+0x103c bit n)
    int  write(const char *filename, int unk, const char *description); // 0x4d1c40, nonzero = written
    // Header only. version may be 0. Nonzero = the header is valid.
    int  read_info(const char *filename, char *level_name, char *script_name, int *unk_a8,
                   float *level_time, char *description, int *version); // 0x4d2800
    void read_thumbnail_file(const char *filename, int unk);         // 0x4d2910 (return unused)
    void set_pending_load(const char *filename, int slot);           // 0x4d3870
};
extern savegame savegame_state;                                      // 0x2493a38
extern int savegame_thumbnail_bm;                                    // 0x59e794 (201x151, 32-bit)
int savegame_location_allowed();     // 0x4d4a50 (0 allowed, 1 combat, 2 level, 3 summon)

#endif
