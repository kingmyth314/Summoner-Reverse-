// levelscripts/savegame.h - the savegame object (savegame_* 0x4cfca0-0x4d4a50, between
// level_scripts_common.cpp and level_scripts.cpp; unit name not known, file name ours).
// Not original source. One merged class of the three stub copies (DECISIONS.md D14).
#ifndef LEVELSCRIPTS_SAVEGAME_H
#define LEVELSCRIPTS_SAVEGAME_H

#include "vsdk/vsdk.h"

struct automap_label;                // summoner/automap.h

class savegame {                     // savegame_state at 0x2493a38 (thiscall members; layout not needed yet)
public:
    int  get_flag(int level, const char *name);                      // 0x4cfca0 savegame_get_flag
    int  get_int(int level, const char *name);                       // 0x4d0030 savegame_get_int
    int  get_quest_bit(int quest);                                   // 0x4d0c00 0 or 1 in EAX (+0x103c bit n)
    // Map labels (new_automap.cpp). The second argument is a level_script_find_index slot (sum.symbols
    // calls it "layer"); the return values are unused.
    void automap_label_add(int level, int script, automap_label *label);      // 0x4d12b0
    void automap_label_remove(int level, int script, automap_label *label);   // 0x4d1390
    void automap_labels_reset_level(int level, int script);                   // 0x4d14f0
    int  automap_label_count();                                      // 0x4d1620 (max 0x80)
    // Hot buttons (new_hud.cpp): 5 members x 6 buttons of 6 bytes {used, kind, id} at +0x105c; the member
    // is the portrait species (0..3, 4 for the rest). Argument order from the callee (sum.symbols
    // CORRECTION notes): get stores *kind = byte +1, *id = dword +2 and returns `setne al` of byte +0.
    void hot_button_clear(int member, int slot);                     // 0x4d1660 savegame_hot_button_clear
    void hot_button_set(int kind, int id, int member, int slot);     // 0x4d16a0 savegame_hot_button_set
    bool hot_button_get(int *kind, int *id, int member, int slot);   // 0x4d1770 savegame_hot_button_get
    int  write(const char *filename, int unk, const char *description); // 0x4d1c40, nonzero = written
    // Header only. version may be 0. Nonzero = the header is valid.
    int  read_info(const char *filename, char *level_name, char *script_name, int *unk_a8,
                   float *level_time, char *description, int *version); // 0x4d2800
    void read_thumbnail_file(const char *filename, int unk);         // 0x4d2910 (return unused)
    void set_pending_load(const char *filename, int slot);           // 0x4d3870
};
extern savegame savegame_state;                                      // 0x2493a38
// The pilots' wrapper spellings (new_automap.cpp, new_hud.cpp), now the object itself (D46).
#define SAVEGAME_AUTOMAP (savegame_state)
#define SAVEGAME_HM      (&savegame_state)
extern int savegame_thumbnail_bm;                                    // 0x59e794 (201x151, 32-bit)
int savegame_location_allowed();     // 0x4d4a50 (0 allowed, 1 combat, 2 level, 3 summon)

#endif
