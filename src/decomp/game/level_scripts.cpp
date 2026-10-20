// level_scripts.cpp - Sum.exe v1.40, code 0x4ceec0-0x4d6651 (levelscripts\level_scripts.cpp; strings.tbl ids
// 469-476 are this unit's, docs/sum-exe/source-map.md)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The unit holds the savegame object (savegame_state, 0x2493a38: the buffer layout, the flag / bit / int /
// float accessors, the conversation, quest, level item, two-stage, automap label and hot button sections,
// the chunk writers and readers), events.tbl and the automap tables, the game stage console command, the
// savegame / loadgame commands, the level table, the level-script selection and helpers
// (conversation_execute_action, quests, script_filenames.tbl) and the door commands. Static initialiser
// table entries 345-358 (0x588568-0x58859c) are this unit's.
// Data: .data 0x59e794-0x5a024f (savegame_thumbnail_bm, the literals, cur_level_index, level_script_table,
// level_table); .bss 0x2493a18-0x25443af (see the definitions; 0x2493a14-0x2493a17 are left to
// level_scripts_common.cpp). Pooled literals of other units: "_m1" 0x59e51c, "%s.tbl" 0x59821c, the level
// names of level_script_table (0x592d00.., 0x5945d8..), "#End" 0x58bd08, "$Level:" 0x593a58, ".sav"
// 0x58bc98, "Gold" 0x58d4a8, "TRUE" / "FALSE" / "%s is %s\n" and the bool usage text (0x588c34..0x588c50),
// "#01234567890" 0x588ba8, "" 0x5bd0d0.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "vsdk/os/console.h"
#include "vsdk/os/crc32.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/snd/snd.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/automap.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_saveload.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// Chunk tags (little-endian dwords).
#define SAVEGAME_TAG_HEADER 0x67564153      // "SAVg"
#define SAVEGAME_TAG_ITEM   0x4d455449      // "ITEM"
#define SAVEGAME_TAG_INVT   0x54564e49      // "INVT"
#define SAVEGAME_TAG_LENT   0x544e454c      // "LENT"
#define SAVEGAME_TAG_CMRA   0x41524d43      // "CMRA"
#define SAVEGAME_TAG_CONT   0x544e4f43      // "CONT"

// The level item section: a count and up to 0x200 name hashes (type name ours).
struct savegame_level_item_list {
    int count;
    unsigned int hashes[SAVEGAME_MAX_LEVEL_ITEMS];
};
// The section pointers are re-read from the object after every store (VC6 assumes a store may hit the
// member), so the code reaches them through these expressions, not through cached locals. Names ours.
#define LEVEL_ITEM_LIST ((savegame_level_item_list *)level_item_hashes.data)
#define TWO_STAGE       ((savegame_two_stage *)two_stage.data)

// ---- .data (0x59e794-) -----------------------------------------------------------------------------
int savegame_thumbnail_bm = -1;                      // 0x59e794 the bitmap made on the thumbnail section

// ---- .bss (0x2493a18-0x25443af) --------------------------------------------------------------------
// The objects with dynamic initialisers are defined below at their initialisers' place (entries 345-358,
// definition order = initialiser order; their .bss addresses are not in that order, see NOTES):
// ccmd_loadgame 0x2493a18, ccmd_Preparse_scripts 0x2493a28, savegame_state 0x2493a38, ccmd_savegame
// 0x2494b20, ccmd_gamestage 0x24dfb30, ccmd_door_toggle 0x24ed5f0, script_filenames_string_pool 0x24ed600,
// events_string_pool 0x24f6218, script_objects 0x24f6230, ccmd_door_open 0x25388b0,
// level_script_stringpool 0x2539f18, ccmd_door_close 0x253ef30.
ubyte savegame_buffer[SAVEGAME_BUFFER_SIZE];         // 0x2494b2c (a .sav file is its first .used bytes)
int events_int_pool[0x1000];                         // 0x24dfb3c
int savegame_file_version;                           // 0x24e3b3c
bool flece_joined;                                   // 0x24e3b40
bool jekhar_joined;                                  // 0x24e3b41
bool rosalind_joined;                                // 0x24e3b42
bool level_script_event_active;                      // 0x24e3b43
int events_int_pool_offset;                          // 0x24e3b44
bool Preparse_scripts;                               // 0x24e3b48
unsigned int events_tbl_checksum;                    // 0x24e3b4c (name ours)
level_item_rec level_items[MAX_LEVEL_ITEMS];         // 0x24e3b50
navpoint navpoints[MAX_NAVPOINTS];                   // 0x24e4950
char script_filenames_string_buffer[0x800];          // 0x24ed614 (name ours)
char events_string_buffer[0x8400];                   // 0x24ede14 (name ours)
void *conversation_save_table[357 * 4];              // 0x25388c0 (conversation_save_info records)
int level_item_count;                                // 0x2539f10
int navpoint_count;                                  // 0x2539f2c
char level_script_string_buffer[0x5000];             // 0x2539f30 (name ours)
quest_info quest_table[MAX_QUESTS];                  // 0x253ef40
int script_object_count;                             // 0x25443a0
int quest_count;                                     // 0x25443a4
bool level_entered_by_load;                          // 0x25443a8
unsigned int Script_filenames_tbl_checksum;          // 0x25443ac

// The flag / int / float lookups expand this search (the found index is tested for < 0 afterwards, so it
// was a function returning -1; inline, name ours).
static inline int savegame_find_hash(const unsigned int *hashes, int count, unsigned int hash)
{
    int i;

    for (i = 0; i < count; i++)
        if (hashes[i] == hash)
            return i;
    return -1;
}

// A conversation slot whose name ends in "_m1" (a multiplayer script) has no save block. Expanded the same
// way in savegame_layout_conversation, savegame_set_speaker_bit and savegame_set_topic_bit (inline, name
// ours).
static inline bool conversation_slot_is_mp(const char *name)
{
    int len = strlen(name);

    return len >= 3 && _stricmp(name + len - 3, "_m1") == 0;
}

// The two items renamed after the first release; old saves still load (inline, name ours).
static inline unsigned int savegame_remap_item_hash(unsigned int hash)
{
    unsigned int brown = name_hash_nocase("Brown Dragon Scale");
    unsigned int gray = name_hash_nocase("Gray Dragon Scale");
    unsigned int ring = name_hash_nocase("Ring of Preservation");
    unsigned int pres = name_hash_nocase("Preservation Ring");

    if (hash == brown)
        hash = gray;
    if (hash == ring)
        hash = pres;
    return hash;
}

// 0x4ceec0 static_init_savegame_state (call 0x4ceed0; jmp 0x4ceee0) /
// 0x4ceed0 init_savegame_state (mov ecx, 0x2493a38; jmp savegame_state__clear = the constructor) /
// 0x4ceee0 register_atexit_savegame_state (atexit of the handler below) /
// 0x4ceef0 atexit_dtor_savegame_state: the handler, mov ecx, 0x2493a38 and a jmp to the empty destructor,
// which is folded into empty_function 0x4f13e0. Compiler-generated. Confidence: exact.
savegame savegame_state;                             // 0x2493a38

// 0x4cef00 static_init_ccmd_Preparse_scripts (jmp) /
// 0x4cef10 init_ccmd_Preparse_scripts. Compiler-generated. Confidence: exact.
console_command ccmd_Preparse_scripts("Preparse_scripts", "Toggles Preparse_scripts",
                                      cmd_Preparse_scripts);   // 0x2493a28

// 0x4cef30  Confidence: exact. The console's bool command (as cmd_map_mask in new_automap.cpp).
void cmd_Preparse_scripts()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            Preparse_scripts = true;
        else if (arg_type & ARG_FALSE)
            Preparse_scripts = false;
        else if (arg_type & ARG_NONE)
            Preparse_scripts ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "Preparse_scripts", "Preparse_scripts");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Preparse_scripts", Preparse_scripts ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// ---- the header's size table ------------------------------------------------------------------------
// One printf and one stored dword per section, written out as in the binary.
#define SAVEGAME_PUT_SIZE(text, value) \
    printf(text " = %d\n", value); \
    *(int *)(header.data + *off) = value; \
    *off += 4

// 0x4ceff0  Confidence: exact.
void savegame::report_sizes(int *off)
{
    int size, i, j;

    SAVEGAME_PUT_SIZE("thumb_size", thumbnail.size);
    size = SAVEGAME_NUM_LEVELS * sizeof(int);
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++)
        size += levels[i].size;
    SAVEGAME_PUT_SIZE("level data size", size);
    size = 0;
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++)
        for (j = 0; j < SAVEGAME_NUM_SLOTS; j++)
            size += conversations[i][j].size;
    SAVEGAME_PUT_SIZE("conversation data size", size);
    SAVEGAME_PUT_SIZE("items_size", items.size);
    SAVEGAME_PUT_SIZE("inv_size", inventory.size);
    SAVEGAME_PUT_SIZE("le_size", living.size);
    SAVEGAME_PUT_SIZE("camera_size", camera.size);
    SAVEGAME_PUT_SIZE("item_object_size", item_objects.size);
    SAVEGAME_PUT_SIZE("quest_flag_size", quest_flags.size);
    SAVEGAME_PUT_SIZE("level_item_size", level_item_hashes.size);
    SAVEGAME_PUT_SIZE("two_stage_size", two_stage.size);
    SAVEGAME_PUT_SIZE("automap_labels_size", map_labels.size);
    SAVEGAME_PUT_SIZE("hot_buttons_size", hot_buttons.size);
    printf("Script_filenames_tbl_checksum = %u\n", Script_filenames_tbl_checksum);
    *(unsigned int *)(header.data + *off) = Script_filenames_tbl_checksum;
    *off += 4;
}

// The read side: the saved size must equal the current layout's.
#define SAVEGAME_CHECK_SIZE(text, value) \
    saved = *(int *)(header.data + *off); \
    *off += 4; \
    if (saved != (value)) { \
        printf("i = %d, " text " = %d\n", saved, value); \
        return 0; \
    }

// 0x4cf260  Confidence: exact. The script-table checksum after the sizes is skipped, not compared.
int savegame::report_read_sizes(int *off)
{
    int saved, size, i, j;

    SAVEGAME_CHECK_SIZE("thumb_size", thumbnail.size);
    size = SAVEGAME_NUM_LEVELS * sizeof(int);
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++)
        size += levels[i].size;
    SAVEGAME_CHECK_SIZE("level data size", size);
    size = 0;
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++)
        for (j = 0; j < SAVEGAME_NUM_SLOTS; j++)
            size += conversations[i][j].size;
    SAVEGAME_CHECK_SIZE("conversation data size", size);
    SAVEGAME_CHECK_SIZE("items_size", items.size);
    SAVEGAME_CHECK_SIZE("inv_size", inventory.size);
    SAVEGAME_CHECK_SIZE("le_size", living.size);
    SAVEGAME_CHECK_SIZE("camera_size", camera.size);
    SAVEGAME_CHECK_SIZE("item_object_size", item_objects.size);
    SAVEGAME_CHECK_SIZE("quest_flag_size", quest_flags.size);
    SAVEGAME_CHECK_SIZE("level_item_size", level_item_hashes.size);
    SAVEGAME_CHECK_SIZE("two_stage_size", two_stage.size);
    SAVEGAME_CHECK_SIZE("automap_labels_size", map_labels.size);
    SAVEGAME_CHECK_SIZE("hot_buttons_size", hot_buttons.size);
    *off += 4;
    return 1;
}

// ---- LENT helpers -----------------------------------------------------------------------------------
// 0x4cf500  Confidence: likely (the first seven bit-field stores and selected / summon = -1 are merged
// into two or-masks in the binary; combat_mode goes through setne).
void savegame::write_party_state(int *off)
{
    savegame_party_state ps;
    Player *p;
    Character *c;

    ps.members = 0;
    ps.combat_mode = combat_mode != 0;
    ps.solo = (game_flags & GF_SOLO_MODE) >> 12;
    ps.summon_controlled = 0;
    if (cur_character->flags & CF_SUMMON)
        ps.selected = -1;
    else
        ps.selected = cur_character->info->species;
    ps.summon = -1;
    ps.summon_name[0] = 0;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (!(c->flags & CF_SUMMON))
            ps.members |= 1 << c->info->species;
    }
    if (cur_summon) {
        ps.summon = cur_summon->info->species;
        ps.summon_controlled = summon_player != NULL;
        strcpy(ps.summon_name, cur_summon->get_name());
    }
    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        c = living_from_handle(p->handle);
        ps.members |= 1 << c->info->species;
    }
    *(savegame_party_state *)(living.data + *off) = ps;
    *off += sizeof(savegame_party_state);
}

// 0x4cf680  Confidence: exact (copied to a local, then to the hidden result).
savegame_party_state savegame::read_party_state(int *off)
{
    savegame_party_state ps = *(savegame_party_state *)(living.data + *off);

    *off += sizeof(savegame_party_state);
    return ps;
}

// 0x4cf6c0  Confidence: exact (D10402: lent_max_hp copied through a parenthesised float temporary and stored as
// its bits). Party members (type 7) also save their Player's +0x178 and skill points.
int savegame::write_character(Character *c, int *off)
{
    unsigned int hash = name_hash_nocase(c->get_name());
    int size;
    Player *p;

    *(unsigned int *)(living.data + *off) = hash;
    *off += 4;
    printf("saving %s (%d)\n", c->get_name(), hash);
    size = c->save_state(living.data + *off);
    *off += character_save_state_size();
    if (size > character_save_state_size())
        return 0;
    if (c->type == 7) {
        p = player_from_handle(c->handle);
        float hp = (p->lent_max_hp);
        *(int *)(living.data + *off) = *(int *)&hp;
        *off += 4;
        *(int *)(living.data + *off) = p->skill_points;
        *off += 4;
    }
    return 1;
}

// 0x4cf790  Confidence: exact. The record is matched by name hash to an entity the level already made.
int savegame::read_character(int *off)
{
    Player *p = NULL;
    Character *c;
    unsigned int hash;
    int size;

    hash = *(unsigned int *)(living.data + *off);
    *off += 4;
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (hash == (unsigned int)name_hash_nocase(c->get_name()))
            break;
    if (c == &living_list)
        return 0;
    if (c->type == 7) {
        p = player_alloc_slot();
        if (!p)
            return 0;
        p->init_from_character(c);
    }
    size = c->load_state(living.data + *off);
    *off += character_save_state_size();
    if (size > character_save_state_size())
        return 0;
    if (c->type == 7) {
        if (!p)
            return 0;
        p->lent_max_hp = *(float *)(living.data + *off);
        *off += 4;
        p->skill_points = *(int *)(living.data + *off);
        *off += 4;
        p->skill_cap = (int)xp_level_skill_cap(c->level);
        if (c->flags & CF_SUMMON)
            summon_player = p;
    }
    return 1;
}

// 0x4cf8d0  Confidence: exact (VC6 byte match) (the count is stored by moving *off back to the start and then forward).
void savegame::write_groups(int *off)
{
    int start = *off;
    int count = 0;
    int end, i, k;
    ai_group *g;

    *(int *)(living.data + *off) = 0;
    *off += 4;
    for (k = 0; k < 10; k++) {
        g = &ai_groups[k];
        if (g->count > 0) {
            *(unsigned int *)(living.data + *off) = name_hash_nocase(living_from_handle(g->leader)->get_name());
            *off += 4;
            *(int *)(living.data + *off) = g->count - 1;
            *off += 4;
            for (i = 0; i < g->count; i++) {
                if (g->members[i] != g->leader) {
                    *(unsigned int *)(living.data + *off) =
                        name_hash_nocase(living_from_handle(g->members[i])->get_name());
                    *off += 4;
                }
            }
            *(unsigned int *)(living.data + *off) = g->flags;
            *off += 4;
            count++;
        }
    }
    end = *off;
    *off = start;
    *(int *)(living.data + *off) = count;
    *off = end;
}

// 0x4cfa00  Confidence: exact.
int savegame::read_groups(int *off)
{
    int n, i, members, j;
    unsigned int hash;
    Character *c;
    ai_group *g;

    n = *(int *)(living.data + *off);
    *off += 4;
    for (i = 0; i < n; i++) {
        hash = *(unsigned int *)(living.data + *off);
        *off += 4;
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (hash == (unsigned int)name_hash_nocase(c->get_name()))
                break;
        if (c == &living_list)
            return 0;
        g = ai_group_create(c);
        members = *(int *)(living.data + *off);
        *off += 4;
        for (j = 0; j < members; j++) {
            hash = *(unsigned int *)(living.data + *off);
            *off += 4;
            for (c = living_list.live_next; c != &living_list; c = c->live_next)
                if (hash == (unsigned int)name_hash_nocase(c->get_name()))
                    break;
            if (c == &living_list)
                return 0;
            ai_group_add_member(g, c);
        }
        g->flags = *(unsigned int *)(living.data + *off);
        *off += 4;
        g->timer.SetTimeout(0);
    }
    return 1;
}

// ---- the object and the per-level data --------------------------------------------------------------
// 0x4cfb80 savegame_state__clear  Confidence: exact. The constructor (init_savegame_state jumps here); it
// clears the buffer pointer, the used size and four fields of each level block.
savegame::savegame()
{
    int i;

    buffer = NULL;
    used = 0;
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++) {
        levels[i].data = NULL;
        levels[i].num_flags = 0;
        levels[i].num_bits = 0;
        levels[i].num_ints = 0;
    }
}

// 0x4cfbb0  Confidence: exact.
void savegame::init_buffer(ubyte *buf, int size)
{
    buffer = buf;
    buffer_size = size;
    clear_buffer();
}

// 0x4cfbd0  Confidence: exact (VC6 8966 byte match: the second switch assigns and breaks to one return, D12300; two switches with fall-through; the second is the jump table 0x4cfc30).
// The bit offset of entry index of the given type in the level's block: 0 flag, 1 automap bit, 2 int,
// 3 float (the variables start at the next whole byte, 32 bits each).
int savegame::data_offset(int level, int type, int index)
{
    int offset = 0;

    switch (type) {
    case 3:
        offset = levels[level].num_ints * 32;
    case 2:
        offset += levels[level].num_bits;
    case 1:
        offset += levels[level].num_flags;
    }
    switch (type) {
    case 0:
    case 1:
        offset += index;
        break;
    case 2:
    case 3:
        offset = ((offset + 7) & ~7) + index * 32;
        break;
    }
    return offset;
}

// 0x4cfc40  Confidence: exact.
int savegame::get_visit_count(int level)
{
    if (level >= 0 && level < SAVEGAME_NUM_LEVELS && visit_counts)
        return visit_counts[level];
    return 0;
}

// 0x4cfc60  Confidence: exact.
void savegame::set_visit_count(int level, int count)
{
    if (level >= 0 && level < SAVEGAME_NUM_LEVELS && visit_counts)
        visit_counts[level] = count;
}

// 0x4cfc80  Confidence: exact. From state_game_enter.
void savegame::inc_visit_count(int level)
{
    set_visit_count(level, get_visit_count(level) + 1);
}

// 0x4cfca0  Confidence: exact.
int savegame::get_flag(int level, const char *name)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return 0;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].flag_hashes, level_script_table[level].num_flags, hash);
    if (index < 0)
        return 0;
    index = data_offset(level, 0, index);
    return (levels[level].data[index >> 3] & (1 << (index & 7))) != 0;
}

// 0x4cfd30  Confidence: exact (VC6 8966 byte match: byte and mask locals before the test, D12300; the two refresh calls are in both branches in the binary; whether the
// source had them once after the if is not visible). Only setting a flag re-evaluates the quests.
void savegame::set_flag(int level, const char *name, bool value)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].flag_hashes, level_script_table[level].num_flags, hash);
    if (index < 0)
        return;
    index = data_offset(level, 0, index);
    int byte = index >> 3;
    int mask = 1 << (index & 7);
    if (value) {
        levels[level].data[byte] |= mask;
        quest_evaluate(hash);
        savegame_refresh_object_flags();
        savegame_refresh_character_flags();
    } else {
        levels[level].data[byte] &= ~mask;
        savegame_refresh_object_flags();
        savegame_refresh_character_flags();
    }
}

// 0x4cfe00  Confidence: exact. A level item with a +Event: flag is active (obj_flags bit 0) while the flag
// is set; an item in a container switches the container.
void savegame_refresh_object_flags()
{
    obj *o;
    item_obj *it;
    level_item_rec *li;

    for (o = obj_active_list.next; o != &obj_active_list; o = o->next) {
        if (o->type != OBJ_TYPE_ITEM)
            continue;
        it = (item_obj *)o;
        if (it->level_item < 0)
            continue;
        li = &level_items[it->level_item];
        if (li->flag_level == -1)
            continue;
        if (savegame_state.get_flag(li->flag_level, li->flag)) {
            if (it->in_container)
                it->in_container->obj_flags |= 1;
            else
                it->obj_flags |= 1;
        } else {
            if (it->in_container)
                it->in_container->obj_flags &= ~1;
            else
                it->obj_flags &= ~1;
        }
    }
}

// 0x4cfea0  Confidence: exact. A script object with a +Trigger: flag has its character's AI on while the
// flag is set (party members are skipped).
void savegame_refresh_character_flags()
{
    Character *c;
    int so;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7)
            continue;
        so = script_object_find(c->name);
        if (so < 0)
            continue;
        if (script_objects[so].trigger_level == -1)
            continue;
        if (savegame_state.get_flag(script_objects[so].trigger_level, script_objects[so].trigger))
            c->set_ai_active(1);
        else
            c->set_ai_active(0);
    }
}

// 0x4cff20  Confidence: exact. The bound is num_bits * 8, eight times the bits the block has (see NOTES).
int savegame::get_bit(int level, int index)
{
    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return 0;
    if ((unsigned int)index >= levels[level].num_bits << 3)
        return 0;
    index = data_offset(level, 1, index);
    return (levels[level].data[index >> 3] & (1 << (index & 7))) != 0;
}

// 0x4cff80  Confidence: exact (VC6 8966 byte match: byte and mask computed before the size check, D12300).
void savegame::set_bit(int level, int index, bool value)
{
    int offset, byte, mask;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return;
    if ((unsigned int)index >= levels[level].num_bits << 3)
        return;
    offset = data_offset(level, 1, index);
    byte = offset >> 3;
    mask = 1 << (offset & 7);
    if (byte >= levels[level].size)
        printf("bit_flag = %u, byte_number = %d, data_pointers[%d(%s)].size = %d\n", index, byte, level,
               level_script_table[level].name, levels[level].size);
    if (value)
        levels[level].data[byte] |= mask;
    else
        levels[level].data[byte] &= ~mask;
}

// 0x4d0030  Confidence: likely (differs under cl 8966 in match26).
int savegame::get_int(int level, const char *name)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return 0;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].int_hashes, level_script_table[level].num_ints, hash);
    if (index < 0)
        return 0;
    return *(int *)(levels[level].data + (data_offset(level, 2, index) >> 3));
}

// 0x4d00b0  Confidence: exact.
void savegame::set_int(int level, const char *name, int value)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].int_hashes, level_script_table[level].num_ints, hash);
    if (index < 0)
        return;
    *(int *)(levels[level].data + (data_offset(level, 2, index) >> 3)) = value;
}

// 0x4d0130  Confidence: exact (0.0f from 0x57eb84 when not found).
float savegame::get_float(int level, const char *name)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return 0.0f;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].float_hashes, level_script_table[level].num_floats, hash);
    if (index < 0)
        return 0.0f;
    float value;
    *(int *)&value = *(int *)(levels[level].data + (data_offset(level, 3, index) >> 3));
    return value;
}

// 0x4d01c0  Confidence: exact.
void savegame::set_float(int level, const char *name, float value)
{
    unsigned int hash;
    int index;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !levels[level].data)
        return;
    hash = name_hash_nocase(name);
    index = savegame_find_hash(level_script_table[level].float_hashes, level_script_table[level].num_floats, hash);
    if (index < 0)
        return;
    int off = data_offset(level, 3, index) >> 3;
    float v = value;
    *(int *)(levels[level].data + off) = *(int *)&v;
}

// 0x4d0240  Confidence: exact (VC6 8966 byte match: the search loop written out, the index set only when
// found and kept across the levels in EBX, set to -1 once before the loop; the found code inside the level
// loop, D13800). Returns the flag's bit in the first level whose list holds the hash
// (quest_get_current_step); no data pointer check.
int savegame::find_flag_level(unsigned int hash)
{
    int index = -1;
    int level;

    for (level = 0; level < SAVEGAME_NUM_LEVELS; level++) {
        int i;
        for (i = 0; i < level_script_table[level].num_flags; i++) {
            if (level_script_table[level].flag_hashes[i] == hash) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            index = data_offset(level, 0, index);
            return (levels[level].data[index >> 3] & (1 << (index & 7))) != 0;
        }
    }
    return 0;
}

// ---- conversation blocks --------------------------------------------------------------------------------
// A block (savegame_layout_conversation): int "has data", the speaker hashes, the topic count of each
// speaker, the topic hashes, the speaker bits (bit = speaker + kind; (speakers + 7) & ~7 bytes reserved),
// then four bits per topic (bit = 4 * topic + kind).

// 0x4d02d0  Confidence: likely (the speaker / topic induction variables; the speaker-bit loop runs over one
// kind with a one-case switch, written as in the binary). The first use of a slot stores the hashes; the
// persistent speakers (CFLAG_PERSISTENT) then take their bits from the first other slot of the level that
// has data.
void savegame::init_conversation_slot(int level, int slot, conversation_speaker *speakers)
{
    conversation_save_info *info;
    ubyte *data;
    int counts_off, topics_off, num_topics, n, i, kind, bit;
    conversation_speaker *s;
    conversation_topic *t;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return;
    info = CONVERSATION_SAVE_INFO(level, slot);
    if (!info->name)
        return;
    data = conversations[level][slot].data;
    counts_off = info->num_speakers * 4 + 4;
    topics_off = counts_off + info->num_speakers * 4;
    if (*(int *)data != 0)
        return;
    num_topics = 0;
    for (s = speakers, i = 0; s; s = s->next, i++) {
        *(unsigned int *)(data + 4 + i * 4) = name_hash_nocase(s->name);
        n = 0;
        for (t = s->topics; t; t = t->next) {
            *(unsigned int *)(data + topics_off + num_topics * 4) = name_hash_nocase(t->name);
            n++;
            num_topics++;
        }
        *(int *)(data + counts_off + i * 4) = n;
    }
    *(int *)data = 1;
    for (i = 0; i < SAVEGAME_NUM_SLOTS; i++) {
        if (i == slot)
            continue;
        if (!CONVERSATION_SAVE_INFO(level, i)->name)
            return;
        if (has_conversation_data(level, i))
            break;
    }
    if (i >= SAVEGAME_NUM_SLOTS)
        return;
    for (s = speakers; s; s = s->next) {
        if (!(s->flags & CFLAG_PERSISTENT))
            continue;
        for (kind = 0; kind < 1; kind++) {
            bit = get_speaker_bit(level, i, s->name, kind);
            set_speaker_bit(level, slot, s->name, kind, bit != 0, false);
            switch (kind) {
            case 0:
                if (bit)
                    s->flags |= CFLAG_IMPORTANT;
                else
                    s->flags &= ~CFLAG_IMPORTANT;
                break;
            }
        }
        for (t = s->topics; t; t = t->next) {
            for (kind = 0; kind < 4; kind++) {
                bit = get_topic_bit(level, i, s->name, t->name, kind);
                set_topic_bit(level, slot, s->name, t->name, kind, bit != 0, false);
                switch (kind) {
                case 0:
                    if (bit)
                        t->flag0 = 1;
                    else
                        t->flag0 = 0;
                    break;
                case 1:
                    if (bit)
                        t->initial = 1;
                    else
                        t->initial = 0;
                    break;
                case 2:
                    if (bit)
                        t->hidden = 1;
                    else
                        t->hidden = 0;
                    break;
                case 3:
                    if (bit)
                        s->greeting = t;
                    break;
                }
            }
        }
    }
}

// 0x4d0590  Confidence: exact (the count read with a 4-byte memcpy: a load and test, not cmp [mem], 0; D14700).
bool savegame::has_conversation_data(int level, int slot)
{
    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return false;
    if (!CONVERSATION_SAVE_INFO(level, slot)->name)
        return false;
    int v;
    memcpy(&v, conversations[level][slot].data, 4);
    return v != 0;
}

// 0x4d05f0  Confidence: likely (differs under cl 8966 in match26). Nothing is read before the level has been entered once.
int savegame::get_speaker_bit(int level, int slot, const char *speaker, int kind)
{
    conversation_save_info *info;
    ubyte *data;
    int bits_off, n, i;
    unsigned int hash;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return 0;
    if (!CONVERSATION_SAVE_INFO(level, slot)->name)
        return 0;
    if (get_visit_count(level) <= 0)
        return 0;
    data = conversations[level][slot].data;
    bits_off = (CONVERSATION_SAVE_INFO(level, slot)->num_topics + 2 * CONVERSATION_SAVE_INFO(level, slot)->num_speakers) * 4 + 4;
    if (*(int *)data == 0)
        return 0;
    hash = name_hash_nocase(speaker);
    n = CONVERSATION_SAVE_INFO(level, slot)->num_speakers;
    {
        unsigned int *p = (unsigned int *)(data + 4);
        for (i = 0; i < n; i++) {
            unsigned int h = *p++;
            if (hash == h)
                break;
        }
    }
    if (i >= n)
        return 0;
    i += kind;
    return (data[bits_off + (i >> 3)] & (1 << (i & 7))) != 0;
}

// 0x4d06e0  Confidence: likely (differs under cl 8966 in match26). propagate copies the bit to the level's other non-"_m1" slots that have data.
void savegame::set_speaker_bit(int level, int slot, const char *speaker, int kind, bool value, bool propagate)
{
    ubyte *data;
    const char *name;
    int bits_off, n, i, j;
    unsigned int hash;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return;
    if (!CONVERSATION_SAVE_INFO(level, slot)->name)
        return;
    if (get_visit_count(level) <= 0)
        return;
    data = conversations[level][slot].data;
    bits_off = (CONVERSATION_SAVE_INFO(level, slot)->num_topics + 2 * CONVERSATION_SAVE_INFO(level, slot)->num_speakers) * 4 + 4;
    if (*(int *)data == 0)
        return;
    hash = name_hash_nocase(speaker);
    n = CONVERSATION_SAVE_INFO(level, slot)->num_speakers;
    for (i = 0; i < n; i++)
        if (hash == ((unsigned int *)(data + 4))[i])
            break;
    if (i >= n)
        return;
    i += kind;
    if (value)
        data[bits_off + (i >> 3)] |= 1 << (i & 7);
    else
        data[bits_off + (i >> 3)] &= ~(1 << (i & 7));
    if (!propagate)
        return;
    for (j = 0; j < SAVEGAME_NUM_SLOTS; j++) {
        if (j == slot)
            continue;
        name = CONVERSATION_SAVE_INFO(level, j)->name;
        if (!name)
            return;
        if (conversation_slot_is_mp(name))
            continue;
        if (has_conversation_data(level, j))
            set_speaker_bit(level, j, speaker, kind, value, false);
    }
}

// 0x4d0870  Confidence: likely (differs under cl 8966 in match26).
int savegame::get_topic_bit(int level, int slot, const char *speaker, const char *topic, int kind)
{
    ubyte *data;
    int counts_off, topics_off, bits_off, n, i, j, first, count;
    unsigned int hash;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return 0;
    if (!CONVERSATION_SAVE_INFO(level, slot)->name)
        return 0;
    if (get_visit_count(level) <= 0)
        return 0;
    data = conversations[level][slot].data;
    counts_off = CONVERSATION_SAVE_INFO(level, slot)->num_speakers * 4 + 4;
    topics_off = counts_off + CONVERSATION_SAVE_INFO(level, slot)->num_speakers * 4;
    bits_off = topics_off + ((CONVERSATION_SAVE_INFO(level, slot)->num_speakers + 7) & ~7) + CONVERSATION_SAVE_INFO(level, slot)->num_topics * 4;
    if (*(int *)data == 0)
        return 0;
    hash = name_hash_nocase(speaker);
    n = CONVERSATION_SAVE_INFO(level, slot)->num_speakers;
    count = 0;
    first = 0;
    for (i = 0; i < n; i++) {
        count = *(int *)(data + counts_off + i * 4);
        if (hash == *(unsigned int *)(data + 4 + i * 4))
            break;
        first += count;
    }
    if (i >= n)
        return 0;
    hash = name_hash_nocase(topic);
    for (j = 0; j < count; j++)
        if (hash == *(unsigned int *)(data + topics_off + (first + j) * 4))
            break;
    if (j >= count)
        return 0;
    i = kind + (j + first) * 4;
    return (data[bits_off + (i >> 3)] & (1 << (i & 7))) != 0;
}

// 0x4d09e0  Confidence: likely (differs under cl 8966 in match26).
void savegame::set_topic_bit(int level, int slot, const char *speaker, const char *topic, int kind, bool value,
                             bool propagate)
{
    ubyte *data;
    const char *name;
    int counts_off, topics_off, bits_off, n, i, j, first, count;
    unsigned int hash;

    if (level < 0 || level >= SAVEGAME_NUM_LEVELS || !conversation_data || slot < 0 || slot >= SAVEGAME_NUM_SLOTS)
        return;
    if (!CONVERSATION_SAVE_INFO(level, slot)->name)
        return;
    if (get_visit_count(level) <= 0)
        return;
    data = conversations[level][slot].data;
    counts_off = CONVERSATION_SAVE_INFO(level, slot)->num_speakers * 4 + 4;
    topics_off = counts_off + CONVERSATION_SAVE_INFO(level, slot)->num_speakers * 4;
    bits_off = topics_off + ((CONVERSATION_SAVE_INFO(level, slot)->num_speakers + 7) & ~7) + CONVERSATION_SAVE_INFO(level, slot)->num_topics * 4;
    if (*(int *)data == 0)
        return;
    hash = name_hash_nocase(speaker);
    n = CONVERSATION_SAVE_INFO(level, slot)->num_speakers;
    count = 0;
    first = 0;
    for (i = 0; i < n; i++) {
        count = *(int *)(data + counts_off + i * 4);
        if (hash == *(unsigned int *)(data + 4 + i * 4))
            break;
        first += count;
    }
    if (i >= n)
        return;
    hash = name_hash_nocase(topic);
    for (j = 0; j < count; j++)
        if (hash == *(unsigned int *)(data + topics_off + (first + j) * 4))
            break;
    if (j >= count)
        return;
    i = kind + (j + first) * 4;
    if (value)
        data[bits_off + (i >> 3)] |= 1 << (i & 7);
    else
        data[bits_off + (i >> 3)] &= ~(1 << (i & 7));
    if (!propagate)
        return;
    for (j = 0; j < SAVEGAME_NUM_SLOTS; j++) {
        if (j == slot)
            continue;
        name = CONVERSATION_SAVE_INFO(level, j)->name;
        if (!name)
            return;
        if (conversation_slot_is_mp(name))
            continue;
        if (has_conversation_data(level, j))
            set_topic_bit(level, j, speaker, topic, kind, value, false);
    }
}

// ---- quests, level items --------------------------------------------------------------------------------
// 0x4d0c00  Confidence: exact (signed / and %).
int savegame::get_quest_bit(int quest)
{
    return (quest_flags.data[quest / 8] & (1 << (quest % 8))) != 0;
}

// 0x4d0c40  Confidence: exact (`cmp dl, 1`: compared with true; byte shifts).
void savegame::set_quest_bit(int quest, bool value)
{
    int index = quest / 8;
    int bit = quest % 8;

    if (value == true)
        quest_flags.data[index] |= (ubyte)(1 << bit);
    else
        quest_flags.data[index] &= ~(ubyte)(1 << bit);
}

// 0x4d0c90 quest_evaluate  Confidence: exact. Setting a quest's +Invalidate: flag closes the quest.
void savegame::quest_evaluate(unsigned int flag_hash)
{
    int i;

    printf("Evaluating quests!\n");
    for (i = 0; i < quest_count; i++) {
        if (quest_table[i].invalidate_step >= 0 &&
            quest_table[i].steps[quest_table[i].invalidate_step].flag_hash == flag_hash) {
            printf("INVALIDATING QUEST!\n");
            set_quest_bit(i, false);
        }
    }
}

// 0x4d0d00  Confidence: likely (differs under cl 8966 in match26).
int savegame::level_item_add(const char *name)
{
    unsigned int hash = name_hash_nocase(name);
    ubyte *data = level_item_hashes.data;
    int count = *(int *)data;
    int i;
    int off = 4;

    for (i = 0; i < count; i++) {
        unsigned int h = *(unsigned int *)(data + off);
        off += 4;
        if (hash == h)
            return 1;
    }
    if (i < SAVEGAME_MAX_LEVEL_ITEMS) {
        *(unsigned int *)(data + off) = hash;
        *(int *)level_item_hashes.data = count + 1;
        return 1;
    }
    return 0;
}

// 0x4d0d60  Confidence: likely (differs under cl 8966 in match26).
int savegame::level_item_remove(const char *name)
{
    unsigned int hash = name_hash_nocase(name);
    savegame_level_item_list *list = LEVEL_ITEM_LIST;
    int count = list->count;
    int i, n;

    {
        unsigned int *p = list->hashes;
        for (i = 0; i < count; i++) {
            unsigned int h = *p++;
            if (h == hash)
                break;
        }
    }
    if (i != count) {
        n = count - i - 1;
        if (n > 0) {
            unsigned int *dst = &list->hashes[i];
            memmove(dst, dst + 1, n * 4);
        }
        LEVEL_ITEM_LIST->count = count - 1;
    }
    return 1;
}

// 0x4d0dd0  Confidence: likely (differs under cl 8966 in match26).
int savegame::level_item_has(const char *name)
{
    unsigned int hash = name_hash_nocase(name);
    savegame_level_item_list *list = LEVEL_ITEM_LIST;
    int i;
    int count = list->count;
    unsigned int *p;

    for (i = 0, p = list->hashes; i < count; i++)
        if (hash == *p++)
            return 1;
    return 0;
}

// ---- two-stage objects --------------------------------------------------------------------------------
// An entry is name_hash_nocase(level_script_name + object name) in the current level's list; its two bits
// are bit 0 open, bit 1 locked (doors; for level containers bit 0 is "used").

// 0x4d0e20  Confidence: unsure (the container path that finds an existing entry clears both bits: the
// value it stores comes from the `locked` argument slot after a zero is written to it; written as such,
// see NOTES). Doors (lobj flag 0x100) keep open and locked; a new container entry keeps open only.
void savegame::two_stage_set(bool open, bool locked, lobj *obj)
{
    int level = cur_level_index;
    int base = level * SAVEGAME_MAX_TWO_STAGE * 2;
    int count = TWO_STAGE->counts[level];
    char name[0x40];
    unsigned int hash;
    int i, bit, shift;
    ubyte *p;
    ubyte old, v;

    strncpy(name, level_script_name, 0x40);
    strncat(name, obj->name, 0x40);
    hash = name_hash_nocase(name);
    for (i = 0; i < count; i++)
        if (hash == TWO_STAGE->hashes[level][i])
            break;
    if (obj->flags & 0x100) {
        if (i < count) {
            bit = base + i * 2;
            p = &TWO_STAGE->bits[bit / 8];
            shift = bit % 8;
            old = *p & ~(3 << shift);
            v = 0;
            if (open)
                v = 1;
            if (locked)
                v |= 2;
            *p = (v << shift) | old;
            return;
        }
        if (count >= SAVEGAME_MAX_TWO_STAGE)
            return;
        TWO_STAGE->hashes[level][count] = hash;
        bit = base + count * 2;
        p = &TWO_STAGE->bits[bit / 8];
        shift = bit % 8;
        old = *p & ~(3 << shift);
        v = 0;
        if (open)
            v = 1;
        if (locked)
            v |= 2;
        *p = (v << shift) | old;
        TWO_STAGE->counts[level] = count + 1;
        return;
    }
    printf("finding lc\n");
    if (lc_find_by_hash(hash) < 0) {
        printf("no lc\n");
        return;
    }
    if (i < count) {
        printf(open ? "open\n" : "closed\n");
        bit = base + i * 2;
        shift = bit % 8;
        old = TWO_STAGE->bits[bit / 8] & ~(3 << shift);
        printf("stuffing flag\n");
        locked = false;
        TWO_STAGE->bits[bit / 8] = ((locked ? 1 : 0) << shift) | old;
        return;
    }
    printf(open ? "open 2\n" : "closed 2\n");
    if (count >= SAVEGAME_MAX_TWO_STAGE)
        return;
    TWO_STAGE->hashes[level][count] = hash;
    bit = base + count * 2;
    p = &TWO_STAGE->bits[bit / 8];
    shift = bit % 8;
    old = *p & ~(3 << shift);
    v = 0;
    if (open)
        v = 1;
    *p = (v << shift) | old;
    TWO_STAGE->counts[level] = count + 1;
}

// 0x4d1110  Confidence: likely (the bool outputs are stored from `& 1` without setne). A door without an
// entry reports its default locked bit (door_info flag 8).
void savegame::two_stage_get(bool *open, bool *locked, lobj *obj)
{
    int level = cur_level_index;
    int count = TWO_STAGE->counts[level];
    int base = level * SAVEGAME_MAX_TWO_STAGE * 2;
    char name[0x40];
    unsigned int hash;
    int i, bit, d;
    ubyte v;
    door_info *door;

    strncpy(name, level_script_name, 0x40);
    strncat(name, obj->name, 0x40);
    hash = name_hash_nocase(name);
    for (i = 0; i < count; i++)
        if (hash == TWO_STAGE->hashes[level][i])
            break;
    *open = false;
    *locked = false;
    if (i < count) {
        bit = base + i * 2;
        v = TWO_STAGE->bits[bit / 8] >> (bit % 8);
        *open = v & 1;
        *locked = (v >> 1) & 1;
        return;
    }
    if (obj->flags & 0x100) {
        for (d = 0, door = doors; d < num_doors; d++, door++)
            if (strcmp(obj->name, lobjs[door->lobj_index].name) == 0)
                break;
        if (d < num_doors)
            *locked = (door->state >> 3) & 1;
    } else
        *locked = false;
}

// ---- automap labels ------------------------------------------------------------------------------------
// 0x4d12b0  Confidence: exact (D10402: x and z copied through parenthesised float temporaries).
void savegame::automap_label_add(int level, int script, automap_label *label)
{
    int count, off;

    if (level == -1 || script < 0)
        return;
    count = *(int *)map_labels.data;
    if (count >= SAVEGAME_MAX_LABELS)
        return;
    off = count * sizeof(savegame_automap_label) + 4;
    *(int *)(map_labels.data + off) = level;
    off += 4;
    *(int *)(map_labels.data + off) = script;
    off += 4;
    float x = (label->x);
    *(int *)(map_labels.data + off) = *(int *)&x;
    off += 4;
    float z = (label->z);
    *(int *)(map_labels.data + off) = *(int *)&z;
    off += 4;
    *(int *)(map_labels.data + off) = label->layer;
    off += 4;
    memset(map_labels.data + off, 0, 0x20);
    memcpy(map_labels.data + off, label->text, strlen(label->text));
    *(int *)map_labels.data = count + 1;
}

// 0x4d1390  Confidence: likely (each record is copied field by field into locals, the layer load is dead).
// A full list (128 labels) is never searched, so no label can be removed then (see NOTES).
void savegame::automap_label_remove(int level, int script, automap_label *label)
{
    int count, off, start, i, rlevel, rscript;
    float rx, rz;
    char text[0x20];

    if (level == -1 || script < 0)
        return;
    off = 4;
    count = *(int *)map_labels.data;
    if (count >= SAVEGAME_MAX_LABELS)
        return;
    for (i = 0; i < count; i++) {
        start = off;
        rlevel = *(int *)(map_labels.data + off);
        off += 4;
        rscript = *(int *)(map_labels.data + off);
        off += 4;
        *(int *)&rx = *(int *)(map_labels.data + off);
        off += 4;
        *(int *)&rz = *(int *)(map_labels.data + off);
        off += 4;
        off += 4;                    // layer (read into a dead local)
        strcpy(text, (char *)(map_labels.data + off));
        off += 0x20;
        if (rlevel == level && rscript == script && rx == label->x && rz == label->z) {
            memmove(map_labels.data + start, map_labels.data + off,
                    count * sizeof(savegame_automap_label) - (i + 1) * sizeof(savegame_automap_label));
            *(int *)map_labels.data = count - 1;
            return;
        }
    }
}

// 0x4d14f0  Confidence: exact (count * 0x34 and (i + 1) * 0x34 are kept as running values in the binary).
// Only caller cmd_reset_map_labels. Record layout as savegame_automap_label: level, script, x, z, layer
// (0xc bytes), text at +0x14 (D7504: the old source skipped 0x10 there and walked 0x38-byte records).
void savegame::automap_labels_reset_level(int level, int script)
{
    int count, off, start, i, rlevel, rscript;
    char text[0x20];

    if (level == -1 || script < 0)
        return;
    off = 4;
    count = *(int *)map_labels.data;
    if (count >= SAVEGAME_MAX_LABELS)
        return;
    for (i = 0; i < count; ) {
        start = off;
        rlevel = *(int *)(map_labels.data + off);
        off += 4;
        rscript = *(int *)(map_labels.data + off);
        off += 4;
        off += 0xc;                  // x, z, layer
        strcpy(text, (char *)(map_labels.data + off));
        off += 0x20;
        if (rlevel == level && rscript == script) {
            memmove(map_labels.data + start, map_labels.data + off,
                    count * sizeof(savegame_automap_label) - (i + 1) * sizeof(savegame_automap_label));
            count--;
            off = start;
        } else
            i++;
    }
    *(int *)map_labels.data = count;
}

// 0x4d1620  Confidence: exact.
int savegame::automap_label_count()
{
    return *(int *)map_labels.data;
}

// ---- hot buttons: 5 members x 6 buttons x {bool used, bool kind, int id} --------------------------------
// 0x4d1630  Confidence: exact.
void savegame::hot_buttons_clear_member(int member)
{
    if (member >= 0 && member < 5)
        memset(hot_buttons.data + member * 6 * sizeof(savegame_hot_button), 0, 6 * sizeof(savegame_hot_button));
}

// 0x4d1660  Confidence: exact (memset of 6 bytes: a dword and a word store).
void savegame::hot_button_clear(int member, int slot)
{
    if (member >= 0 && member < 5 && slot >= 0 && slot < 6)
        memset(hot_buttons.data + (member * 6 + slot) * sizeof(savegame_hot_button), 0, sizeof(savegame_hot_button));
}

// 0x4d16a0  Confidence: likely (the record is walked with a byte offset, the section pointer re-read for
// every store; kind != 0 is computed once before the loop). The same {kind, id} on the member's other
// buttons is cleared.
void savegame::hot_button_set(int kind, int id, int member, int slot)
{
    int off, i, v;
    ubyte used, k;

    if (member < 0 || member >= 5 || slot < 0 || slot >= 6)
        return;
    off = member * 6 * sizeof(savegame_hot_button);
    bool has_kind = kind != 0;
    for (i = 0; i < 6; i++) {
        if (i == slot) {
            hot_buttons.data[off] = 1;
            off++;
            hot_buttons.data[off] = has_kind;
            off++;
            *(int *)(hot_buttons.data + off) = id;
            off += 4;
        } else {
            used = hot_buttons.data[off];
            off++;
            k = hot_buttons.data[off];
            off++;
            v = *(int *)(hot_buttons.data + off);
            off += 4;
            if (used && k == kind && v == id)
                hot_button_clear(member, i);
        }
    }
}

// 0x4d1770  Confidence: exact (the kind byte copied out with a 1-byte memcpy, so it sits in memory, in the
// dead `member` slot, and is read back as a dword with `and 0xff`, as in the binary; D14700).
bool savegame::hot_button_get(int *kind, int *id, int member, int slot)
{
    int off;
    ubyte used;

    if (member < 0 || member >= 5 || slot < 0 || slot >= 6)
        return false;
    off = (member * 6 + slot) * sizeof(savegame_hot_button);
    used = hot_buttons.data[off];
    off++;
    if (used) {
        ubyte k;
        memcpy(&k, hot_buttons.data + off, 1);
        *kind = k;
        *id = *(int *)(hot_buttons.data + off + 1);
    }
    return used != 0;
}

// 0x4d17e0  Confidence: exact.
void savegame::clear_buffer()
{
    memset(buffer, 0, buffer_size);
}

// ---- the layout (savegame_build_layout's order) ---------------------------------------------------------
// 0x4d1800  Confidence: exact (VC6 byte match) (store order).
void savegame::layout_header()
{
    header.data = buffer;
    used = 0x1f8;
    header.size = 0x1f8;
}

// 0x4d1820  Confidence: exact (VC6 byte match) (the padding is added to used in one expression in the binary).
void savegame::layout_thumbnail()
{
    used += !(used & 0xf) ? 0 : 0x10 - (used & 0xf);
    thumbnail.data = buffer + used;
    used += 0xed1e;                  // 201 x 151 RGB565
    thumbnail.size = 0xed1e;
}

// 0x4d1860  Confidence: exact.
void savegame::layout_level_data()
{
    int i;

    visit_counts = (int *)(buffer + used);
    used += SAVEGAME_NUM_LEVELS * sizeof(int);
    for (i = 0; i < SAVEGAME_NUM_LEVELS; i++) {
        levels[i].data = buffer + used;
        levels[i].num_flags = level_script_table[i].num_flags;
        levels[i].num_bits = level_script_table[i].automap_layers << 12;
        levels[i].num_ints = level_script_table[i].num_ints;
        levels[i].num_floats = level_script_table[i].num_floats;
        levels[i].size = (levels[i].num_bits + levels[i].num_flags + 7) / 8 +
                         (levels[i].num_floats + levels[i].num_ints) * 4;
        used += levels[i].size;
    }
}

// 0x4d18e0  Confidence: likely (the size is built with one += per part, each stored, as in the binary).
// Every slot gets a pointer; the "_m1" slots and the slots after the first unnamed one get no space.
void savegame::layout_conversation()
{
    int level, slot;
    conversation_save_info *info;
    savegame_section *sec;

    conversation_data = buffer + used;
    for (level = 0; level < SAVEGAME_NUM_LEVELS; level++) {
        for (slot = 0; slot < SAVEGAME_NUM_SLOTS; slot++) {
            info = CONVERSATION_SAVE_INFO(level, slot);
            sec = &conversations[level][slot];
            sec->data = buffer + used;
            sec->size = 0;
            if (!info->name)
                break;
            if (conversation_slot_is_mp(info->name))
                continue;
            sec->size += 4;
            sec->size += info->num_speakers * 4;
            sec->size += info->num_speakers * 4;
            sec->size += info->num_topics * 4;
            sec->size += (info->num_speakers + 7) & ~7;
            sec->size += (info->num_topics * 4 + 7) & ~7;
            used += sec->size;
        }
    }
}

// 0x4d19e0  Confidence: exact (VC6 byte match) (the middle size is folded to n * 4 + 8 in the binary).
void savegame::layout_items()
{
    items.data = buffer + used;
    used += 8;
    items.size = 8;
    used += num_item_types * 4;
    items.size += num_item_types * 4;
    used += (num_item_types + 7) >> 3;
    items.size += (num_item_types + 7) >> 3;
}

// 0x4d1a50  Confidence: exact.
void savegame::layout_inventory()
{
    inventory.data = buffer + used;
    used += 0x20a0;
    inventory.size = 0x20a0;
}

// 0x4d1a80  Confidence: exact (VC6 byte match) (one call: the record part is a temporary).
void savegame::layout_living()
{
    int records;

    living.data = buffer + used;
    used += 0x34;
    living.size = 0x34;
    records = character_save_state_size() + 4;
    used += records * SAVEGAME_MAX_LIVING + 0x198;
    living.size += records * SAVEGAME_MAX_LIVING + 0x198;
}

// 0x4d1ae0  Confidence: exact.
void savegame::layout_camera()
{
    camera.data = buffer + used;
    used += 0x28;
    camera.size = 0x28;
}

// 0x4d1b10  Confidence: exact.
void savegame::layout_item_objects()
{
    item_objects.data = buffer + used;
    used += 0x18b4;
    item_objects.size = 0x18b4;
}

// 0x4d1b40  Confidence: exact. Also clears the 200 quest bits through quest_journal_set_entry.
void savegame::layout_quest_flags()
{
    int i;

    quest_flags.data = buffer + used;
    used += 0x19;
    quest_flags.size = 0x19;
    for (i = 0; i < MAX_QUESTS; i++)
        quest_journal_set_entry(i, 0);
}

// 0x4d1b80  Confidence: exact.
void savegame::layout_level_items()
{
    level_item_hashes.data = buffer + used;
    used += sizeof(savegame_level_item_list);
    level_item_hashes.size = sizeof(savegame_level_item_list);
}

// 0x4d1bb0  Confidence: exact. The unpadded byte count 0x673f, not sizeof (0x6740 under VC6; D5400).
void savegame::layout_two_stage()
{
    two_stage.data = buffer + used;
    used += SAVEGAME_TWO_STAGE_BYTES;
    two_stage.size = SAVEGAME_TWO_STAGE_BYTES;
}

// 0x4d1be0  Confidence: exact.
void savegame::layout_automap_labels()
{
    map_labels.data = buffer + used;
    used += 4 + SAVEGAME_MAX_LABELS * sizeof(savegame_automap_label);
    map_labels.size = 4 + SAVEGAME_MAX_LABELS * sizeof(savegame_automap_label);
}

// 0x4d1c10  Confidence: exact.
void savegame::layout_hot_buttons()
{
    hot_buttons.data = buffer + used;
    used += 5 * 6 * sizeof(savegame_hot_button);
    hot_buttons.size = 5 * 6 * sizeof(savegame_hot_button);
}

// ---- writing ---------------------------------------------------------------------------------------------
// 0x4d1c40  Confidence: exact (EH frame for the File; the stubs stand for the untagged sections, which are
// already current in the buffer). unk is not read.
int savegame::write(const char *filename, int unk, const char *description)
{
    if (!write_header(description))
        return 0;
    if (!write_thumbnail())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!write_items())
        return 0;
    if (!write_inventory())
        return 0;
    if (!write_living())
        return 0;
    if (!write_camera())
        return 0;
    if (!write_item_objects())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!chunk_stub())
        return 0;
    if (!chunk_stub())
        return 0;
    if (file_make_path_dirs(path_savegame))
        return 0;
    File f;
    if (f.open(filename, 2, path_savegame))
        return 0;
    f.write(buffer, used);
    if (f.close())
        return 0;
    return 1;
}

// 0x4d1df0  Confidence: exact (m-1020c: the closing `off += 4; return off <= header.size;` gives both
// registers; D14705) (D12302: the three strings are copied as memcpy(dst, src, strlen(src)) into the
// zeroed fields: the binary's `not ecx; dec ecx` count feeds rep movs, so no NUL is copied; a level file
// name of exactly 0x80 or a script name of exactly 0x20 characters is left without one (the description is
// guarded by the > 0xff test). level_time goes through a parenthesised float temporary, the binary's dead
// local.
int savegame::write_header(const char *description)
{
    int off = 0;

    *(int *)(header.data + off) = SAVEGAME_TAG_HEADER;
    off += 4;
    *(int *)(header.data + off) = SAVEGAME_VERSION;
    off += 4;
    memset(header.data + off, 0, 0x80);
    memcpy(header.data + off, level_filename, strlen(level_filename));
    off += 0x80;
    memset(header.data + off, 0, 0x20);
    memcpy(header.data + off, level_script_name, strlen(level_script_name));
    off += 0x20;
    *(int *)(header.data + off) = 0;
    off += 4;
    float t = (level_time);
    *(int *)(header.data + off) = *(int *)&t;
    off += 4;
    if (strlen(description) > 0xff)
        return 0;
    memset(header.data + off, 0, 0x100);
    memcpy(header.data + off, description, strlen(description));
    off += 0x100;
    report_sizes(&off);
    *(int *)(header.data + off) = off + 4;
    off += 4;
    return off <= header.size;
}

// 0x4d1f80  Confidence: exact. The thumbnail saveload_capture_thumbnail rendered.
int savegame::write_thumbnail()
{
    memcpy(thumbnail.data, saveload_thumbnail_pixels, thumbnail.size);
    return 1;
}

// 0x4d1fb0  Confidence: exact. The type count, each type's name hash, then flag bit 0 of each type.
int savegame::write_items()
{
    int off, i;
    ubyte bits;

    off = 8;
    *(int *)items.data = SAVEGAME_TAG_ITEM;
    *(int *)(items.data + 4) = num_item_types;
    for (i = 0; i < num_item_types; i++) {
        *(unsigned int *)(items.data + off) = name_hash_nocase(item_types[i].name);
        off += 4;
    }
    bits = 0;
    for (i = 0; i < num_item_types; i++) {
        if (item_types[i].flags & 1)
            bits |= 1 << (i % 8);
        if (i % 8 == 7 || i == num_item_types - 1) {
            items.data[off++] = bits;
            bits = 0;
        }
    }
    return 1;
}

// 0x4d2090  Confidence: exact.
int savegame::write_inventory()
{
    int off, n, i;

    *(int *)inventory.data = SAVEGAME_TAG_INVT;
    *(int *)(inventory.data + 4) = party_gold;
    n = party_inventory_count;
    if (n < 0)
        n = 0;
    else if (n > MAX_ITEM_TYPES)
        n = MAX_ITEM_TYPES;
    off = 0xc;
    *(int *)(inventory.data + 8) = n;
    for (i = 0; i < n; i++) {
        *(unsigned int *)(inventory.data + off) = name_hash_nocase(party_inventory[i].item->type->name);
        off += 4;
        *(int *)(inventory.data + off) = party_inventory[i].count;
        off += 4;
        *(int *)(inventory.data + off) = party_inventory[i].item->charges;
        off += 4;
    }
    *(int *)(inventory.data + off) = off + 4;
    return off + 4 <= inventory.size;
}

// 0x4d2150  Confidence: exact (m-1020c: `off += 4; return off <= living.size;` at the end; with
// `return off + 4 <= ...` 8966 kept the constant 4 in EDI for every `off += 4`; D14705). The inactive list contributes the entities that are not summons, or that
// have flag 0x40000.
int savegame::write_living()
{
    int off = 0;
    int n;
    Character *c;
    Player *p;

    *(int *)living.data = SAVEGAME_TAG_LENT;
    off += 4;
    *(int *)(living.data + off) = obj_handle_next;
    off += 4;
    *(int *)(living.data + off) = ai_goal_next_id;
    off += 4;
    write_party_state(&off);
    n = 0;
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        n++;
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next)
        if (!(c->flags & CF_SUMMON) || (c->flags & 0x40000))
            n++;
    for (p = player_list_other.next; p != &player_list_other; p = p->next)
        n++;
    if (n > SAVEGAME_MAX_LIVING)
        return 0;
    *(int *)(living.data + off) = n;
    off += 4;
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (!write_character(c, &off))
            return 0;
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next)
        if (!(c->flags & CF_SUMMON) || (c->flags & 0x40000))
            if (!write_character(c, &off))
                return 0;
    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        c = living_from_handle(p->handle);
        if (!write_character(c, &off))
            return 0;
    }
    write_groups(&off);
    *(int *)(living.data + off) = off + 4;
    off += 4;
    return off <= living.size;
}

// 0x4d2320  Confidence: exact.
int savegame::write_camera()
{
    uint state[9];

    *(int *)camera.data = SAVEGAME_TAG_CMRA;
    game_camera.get_state(state);
    memcpy(camera.data + 4, state, sizeof(state));
    return camera.size >= 0x28;
}

// 0x4d2370  Confidence: likely (differs under cl 8966 in match26). Containers in use whose first item is not a level item, then up to 32 loose
// gold piles of the object list.
int savegame::write_item_objects()
{
    int off = 8;
    int count, i, piles, piles_off;
    container *c;
    item_obj *it;
    obj *o;

    *(int *)item_objects.data = SAVEGAME_TAG_CONT;
    count = 0;
    *(int *)(item_objects.data + 4) = 0;
    for (int ci = 0; ci < MAX_CONTAINERS; ci++) {
        if (!containers[ci].type || containers[ci].num_items <= 0)
            continue;
        c = &containers[ci];
        it = c->get_item(0);
        if (!it)
            return 0;
        if (it->level_item >= 0)
            continue;
        float f = (c->pos.x);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        f = (c->pos.y);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        f = (c->pos.z);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        *(int *)(item_objects.data + off) = c->num_items;
        off += 4;
        item_objects.data[off++] = (ubyte)(int)(c->lock * 100.0f);
        for (i = 0; i < c->num_items; i++) {
            it = c->get_item(i);
            *(int *)(item_objects.data + off) = it->item->type - item_types;
            off += 4;
            *(int *)(item_objects.data + off) = it->count;
            off += 4;
        }
        count++;
    }
    *(int *)(item_objects.data + 4) = count;
    piles = 0;
    piles_off = off;
    *(int *)(item_objects.data + off) = 0;
    off += 4;
    for (o = obj_active_list.next; o != &obj_active_list && piles < SAVEGAME_MAX_GOLD_PILES; o = o->next) {
        if (o->type != OBJ_TYPE_ITEM)
            continue;
        it = (item_obj *)o;
        if (it->in_container || !it->is_gold())
            continue;
        float f = (it->pos.x);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        f = (it->pos.y);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        f = (it->pos.z);
        *(int *)(item_objects.data + off) = *(int *)&f;
        off += 4;
        *(int *)(item_objects.data + off) = it->count;
        off += 4;
        piles++;
    }
    *(int *)(item_objects.data + piles_off) = piles;
    return off <= item_objects.size;
}

// 0x4d25a0 savegame_chunk_stub  Confidence: exact.
int savegame::chunk_stub()
{
    return 1;
}

// ---- reading ---------------------------------------------------------------------------------------------
// 0x4d25b0  Confidence: likely (local order). The header, the thumbnail and the rest are read in three
// File::read calls that together read the whole file; a header result <= 0 is returned as it is. The first
// failing check prints its marker.
int savegame::read(const char *filename, int unk)
{
    File f;
    int result, start;
    int unk_a8;
    char script_name[0x20];
    char level_name[0x80];
    char description[0x100];

    if (f.open(filename, 1, path_savegame))
        return 0;
    f.read(buffer, header.size, 0, 0);
    result = read_header(level_name, script_name, &unk_a8, &level_time, description, &savegame_file_version);
    if (result <= 0)
        return result;
    start = header.size;
    f.read(buffer + start, thumbnail.size, 0, 0);
    start += thumbnail.size;
    f.read(buffer + start, used - start, 0, 0);
    if (f.close())
        return 0;
    if (!chunk_stub()) {
        printf("1a\n");
        return 0;
    }
    if (!chunk_stub()) {
        printf("3a\n");
        return 0;
    }
    if (!chunk_stub()) {
        printf("4a\n");
        return 0;
    }
    if (!read_items()) {
        printf("5a\n");
        return 0;
    }
    if (!read_inventory()) {
        printf("6a\n");
        return 0;
    }
    if (!read_living()) {
        printf("7a\n");
        return 0;
    }
    if (!read_camera()) {
        printf("8a\n");
        return 0;
    }
    if (!read_item_objects()) {
        printf("9a\n");
        return 0;
    }
    if (!chunk_stub()) {
        printf("10a\n");
        return 0;
    }
    if (!read_level_items()) {
        printf("11a\n");
        return 0;
    }
    if (!read_two_stage()) {
        printf("12a\n");
        return 0;
    }
    if (!read_automap_labels()) {
        printf("13a\n");
        return 0;
    }
    if (!chunk_stub()) {
        printf("14a\n");
        return 0;
    }
    return 1;
}

// 0x4d2800  Confidence: exact. Reads the header part of the file into the buffer and checks it. The version
// argument is not passed on: read_header gets NULL (0x4d2866 `push 0`, D7504; every caller passes NULL).
int savegame::read_info(const char *filename, char *level_name, char *script_name, int *unk_a8,
                        float *level_time, char *description, int *version)
{
    File f;
    int result;

    if (f.open(filename, 1, path_savegame))
        return 0;
    f.read(buffer, header.size, 0, 0);
    if (f.close())
        return 0;
    result = read_header(level_name, script_name, unk_a8, level_time, description, NULL);
    return result;
}

// 0x4d2910  Confidence: exact. The header and 0xed1e bytes from the header's end: the thumbnail starts at
// 0x200, so its last 8 bytes are not reloaded for the preview.
int savegame::read_thumbnail_file(const char *filename, int unk)
{
    File f;

    if (f.open(filename, 1, path_savegame))
        return 0;
    f.read(buffer, thumbnail.size + header.size, 0, 0);
    gr_mark_texture_dirty(savegame_thumbnail_bm);
    if (f.close())
        return 0;
    return 1;
}

// 0x4d29f0  Confidence: exact. -1: no tag, wrong version or unknown level / script; 0: a section size
// differs; else whether the header length field matches.
int savegame::read_header(char *level_name, char *script_name, int *unk_a8, float *level_time,
                          char *description, int *version)
{
    int off = 0;
    int tag, level;

    tag = *(int *)(header.data + off);
    off += 4;
    if (tag != SAVEGAME_TAG_HEADER)
        return -1;
    savegame_file_version = *(int *)(header.data + off);
    off += 4;
    if (savegame_file_version < SAVEGAME_VERSION || savegame_file_version > SAVEGAME_VERSION)
        return -1;
    if (version)
        *version = savegame_file_version;
    strcpy(level_name, (char *)(header.data + off));
    off += 0x80;
    strcpy(script_name, (char *)(header.data + off));
    off += 0x20;
    level = level_script_find(level_name);
    if (level == -1)
        return -1;
    if (level_script_find_index(level, script_name) < 0)
        return -1;
    *unk_a8 = *(int *)(header.data + off);
    off += 4;
    *level_time = *(float *)(header.data + off);
    off += 4;
    strcpy(description, (char *)(header.data + off));
    off += 0x100;
    if (!report_read_sizes(&off))
        return 0;
    int stored = *(int *)(header.data + off);
    off += 4;
    return off == stored;
}

// 0x4d2b70  Confidence: likely (differs under cl 8966 in match26).
int savegame::read_items()
{
    int bits_off, i, index;
    unsigned int hash;

    if (*(int *)items.data != SAVEGAME_TAG_ITEM)
        return 0;
    if (*(int *)(items.data + 4) != num_item_types)
        return 0;
    bits_off = num_item_types * 4 + 8;
    for (i = 0; i < num_item_types; i++) {
        hash = savegame_remap_item_hash(*(unsigned int *)(items.data + 8 + i * 4));
        index = item_type_find_by_hash(hash);
        if (index < 0)
            return 0;
        if (items.data[bits_off + (i >> 3)] & (1 << (i & 7)))
            item_types[index].flags |= 1;
        else
            item_types[index].flags &= ~1;
    }
    return 1;
}

// 0x4d2c90  Confidence: exact (m-1020c: the item rename written out in a block that changes hash in place,
// not through savegame_remap_item_hash; the closing size read with a 4-byte memcpy; D14700, D14703).
int savegame::read_inventory()
{
    int off, n, i, count, charges, type, size;
    unsigned int hash;

    party_inventory_clear();
    if (*(int *)inventory.data != SAVEGAME_TAG_INVT)
        return 0;
    party_gold = *(int *)(inventory.data + 4);
    off = 0xc;
    n = *(int *)(inventory.data + 8);
    for (i = 0; i < n; i++) {
        hash = *(unsigned int *)(inventory.data + off);
        off += 4;
        count = *(int *)(inventory.data + off);
        off += 4;
        charges = *(int *)(inventory.data + off);
        off += 4;
        {
            unsigned int brown = name_hash_nocase("Brown Dragon Scale");
            unsigned int gray = name_hash_nocase("Gray Dragon Scale");
            unsigned int ring = name_hash_nocase("Ring of Preservation");
            unsigned int pres = name_hash_nocase("Preservation Ring");
            if (hash == brown)
                hash = gray;
            if (hash == ring)
                hash = pres;
        }
        type = item_type_find_by_hash(hash);
        if (type < 0) {
            printf("Couldn't find item type!!!\n");
            return 0;
        }
        party_inventory_add_type(type, count, charges, 1);
    }
    memcpy(&size, inventory.data + off, 4);
    off += 4;
    return off == size;
}

// 0x4d2dc0  Confidence: exact (VC6 byte match) (the two leader loops share the set-leader tail in the binary; written with
// the call inside each loop). Missing party members and the summon are created first; every record must
// match an entity.
int savegame::read_living()
{
    int off = 0;
    savegame_party_state ps;
    int tag, i, ci, n, size;
    Character *c;
    Player *p, *next;

    tag = *(int *)(living.data + off);
    off += 4;
    if (tag != SAVEGAME_TAG_LENT)
        return 0;
    obj_handle_next = *(int *)(living.data + off);
    off += 4;
    ai_goal_next_id = *(int *)(living.data + off);
    off += 4;
    ps = read_party_state(&off);
    combat_mode = ps.combat_mode;
    for (i = 0; i < 4; i++) {
        if (ps.members & (1 << i)) {
            ci = charinfo_find_by_species(i);
            c = living_entity_create(7, ci, &vector3_zero, &identity_matrix, 1, 0, 0, 0, 0);
            if (!c)
                return 0;
            c->set_name(char_infos[ci].name);
            c->ai.reset();
        }
    }
    if (ps.summon >= 0 && ps.summon < 0xe2) {
        ci = charinfo_find_by_species(ps.summon);
        if (ci < 0)
            return 0;
        cur_summon = living_entity_create((ps.summon_controlled << 1) | 5, ci, &vector3_zero, &identity_matrix,
                                          1, 0, 0, 1, 0);
        if (!cur_summon)
            return 0;
        cur_summon->set_name(ps.summon_name);
        summon_player = NULL;
        cur_summon->ai.reset();
    }
    n = *(int *)(living.data + off);
    off += 4;
    if (n > SAVEGAME_MAX_LIVING)
        return 0;
    for (i = 0; i < n; i++)
        if (!read_character(&off))
            return 0;
    ai_groups_reset();
    read_groups(&off);
    for (p = party_list.next, next = p->next; p != &party_list; p = next, next = next->next) {
        c = living_from_handle(p->handle);
        if (c && (c->flags & CF_LEFT_PARTY))
            party_remove_member(c->get_name());
    }
    size = *(int *)(living.data + off);
    off += 4;
    if (size != off) {
        printf("total_size = %d, offset = %d\n", size, off);
        return 0;
    }
    party_sort_by_species();
    game_camera.reset_default();
    int sel = ps.selected;
    if (sel >= 0 && sel <= 3) {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (c && c->info->species == ps.selected) {
                party_set_leader(p);
                break;
            }
        }
    } else {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (c && (c->flags & CF_SUMMON)) {
                party_set_leader(p);
                break;
            }
        }
    }
    if (p == &party_list)
        return 0;
    if (ps.solo)
        party_toggle_hold();
    return 1;
}

// 0x4d3100  Confidence: exact. The follow test reads the low byte of the camera flags (`not cl / test cl, 1`).
int savegame::read_camera()
{
    uint state[9];

    if (!cur_character)
        return 0;
    if (*(int *)camera.data != SAVEGAME_TAG_CMRA)
        return 0;
    memcpy(state, camera.data + 4, sizeof(state));
    game_camera.reset_default();
    game_camera.set_state(state);
    game_camera.set_target(cur_character);
    game_camera.mode = -1;
    game_camera.set_mode(state[7]);
    game_camera.prev_mode = state[8];
    game_camera.snap_to_target(1, 1);
    if ((ubyte)~game_camera.flags & 1) {
        game_camera.set_target(cur_character);
        game_camera.resume_follow();
        game_camera.sync_yaw_to_target();
        game_camera.snap_to_target(1, 1);
    }
    return 1;
}

// 0x4d31e0  Confidence: exact (m-1020c: the three axes are temporaries of the matrix constructor and pos is
// declared in each loop, so pos takes the slot of the last axis; the lock byte read with a 1-byte memcpy;
// D14700, D14704). The gold piles are recreated with a count of 1: the saved amount is skipped (see NOTES).
int savegame::read_item_objects()
{
    matrix orient(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
    int off, n, i, count, j, type, num, piles;
    ubyte lock;
    container *c;
    item_info *item;
    item_obj *it;

    if (*(int *)item_objects.data != SAVEGAME_TAG_CONT)
        return 0;
    n = *(int *)(item_objects.data + 4);
    off = 8;
    for (i = 0; i < n; i++) {
        vector3 pos;

        *(int *)&pos.x = *(int *)(item_objects.data + off);
        off += 4;
        *(int *)&pos.y = *(int *)(item_objects.data + off);
        off += 4;
        *(int *)&pos.z = *(int *)(item_objects.data + off);
        off += 4;
        c = container_obj_create(&pos, 0);
        if (!c)
            return 0;
        count = *(int *)(item_objects.data + off);
        off += 4;
        memcpy(&lock, item_objects.data + off, 1);
        off++;
        c->lock = lock * 0.01f;
        for (j = 0; j < count; j++) {
            type = *(int *)(item_objects.data + off);
            off += 4;
            num = *(int *)(item_objects.data + off);
            off += 4;
            item = item_create(type);
            if (!item)
                return 0;
            it = item_obj_spawn(item, &pos, &orient, num, -1, -1);
            if (!it)
                return 0;
            c->add_item(it);
        }
    }
    piles = *(int *)(item_objects.data + off);
    off += 4;
    for (i = 0; i < piles; i++) {
        vector3 pos;

        *(int *)&pos.x = *(int *)(item_objects.data + off);
        off += 4;
        *(int *)&pos.y = *(int *)(item_objects.data + off);
        off += 4;
        *(int *)&pos.z = *(int *)(item_objects.data + off);
        off += 8;                    // and the amount, not read
        item = item_create_by_name("Gold");
        if (!item)
            return 0;
        item_obj_spawn(item, &pos, &orient, 1, -1, -1);

    }
    return 1;
}

// 0x4d33f0  Confidence: likely (the next object is fetched before the list test). Level items whose name is
// no longer in the level item list (picked up) are deleted, with their container.
int savegame::read_level_items()
{
    obj *o, *next;
    item_obj *it, *inner;
    container *c;
    unsigned int hash;
    int i, count;

    for (o = obj_active_list.next, next = o->next; o != &obj_active_list; o = next, next = next->next) {
        if (o->type != OBJ_TYPE_ITEM)
            continue;
        it = (item_obj *)o;
        if (it->level_item < 0)
            continue;
        hash = name_hash_nocase(level_items[it->level_item].name);
        savegame_level_item_list *list = LEVEL_ITEM_LIST;
        count = list->count;
        {
            unsigned int *p = list->hashes;
            for (i = 0; i < count; i++) {
                unsigned int h = *p++;
                if (hash == h)
                    break;
            }
        }
        if (i != count)
            continue;
        c = it->in_container;
        if (c) {
            while (c->num_items > 0) {
                inner = c->get_item(0);
                c->remove_item(inner);
                inner->deinit();
            }
            c->deinit();
        } else
            it->deinit();
    }
    savegame_refresh_object_flags();
    return 1;
}

// 0x4d34e0  Confidence: likely (the length check is written twice, as in the binary; the counter runs
// down while the hash pointer runs up). Doors go to their saved pose; containers are marked used from bit 0.
int savegame::read_two_stage()
{
    int level = cur_level_index;
    int count = TWO_STAGE->counts[level];
    unsigned int *hashes = TWO_STAGE->hashes[level];
    unsigned int hash;
    char name[0x40];
    int i, d, lc;
    door_info *door;
    lobj *l;
    bool open, locked, used_, flag2;

    for (i = 0; i < count; i++) {
        hash = hashes[i];
        for (d = 0; d < num_doors; d++) {
            door = &doors[d];
            l = &lobjs[door->lobj_index];
            if (strlen(l->name) + strlen(level_script_name) >= 0x40)
                return 0;
            if (strlen(l->name) + strlen(level_script_name) >= 0x40)
                return 0;
            strncpy(name, level_script_name, 0x40);
            strncat(name, l->name, 0x40);
            if (hash == (unsigned int)name_hash_nocase(name))
                break;
        }
        if (d < num_doors) {
            two_stage_get(&open, &locked, l);
            if (open) {
                l->pos = door->open_pos;
                door->open_orient.to_matrix_b(&l->orient);
            } else {
                l->pos = door->closed_pos;
                door->closed_orient.to_matrix_b(&l->orient);
            }
            continue;
        }
        lc = lc_find_by_hash(hash);
        if (lc >= 0) {
            two_stage_get(&used_, &flag2, lc_entries[lc].object);
            printf("checking lc\n");
            if (used_) {
                printf("disabled\n");
                lc_entries[lc].flags |= 1;
            }
        } else
            sprintf(string_buffer, "WARNING: two-stage mesh chksum %d doesn't correspond to any mesh in level \"%s\"\n",
                    hash, level_script_table[cur_level_index].name);
    }
    return 1;
}

// 0x4d3740  Confidence: likely (differs under cl 8966 in match26). The current level's labels become custom map labels.
int savegame::read_automap_labels()
{
    int script = level_script_find_index(cur_level_index, level_script_name);
    int count = *(int *)map_labels.data;
    int off = 4;
    int i, rlevel, rscript, index, layer;
    float x, z;
    char text[0x20];

    for (i = 0; i < count; i++) {
        rlevel = *(int *)(map_labels.data + off);
        off += 4;
        rscript = *(int *)(map_labels.data + off);
        off += 4;
        *(int *)&x = *(int *)(map_labels.data + off);
        off += 4;
        *(int *)&z = *(int *)(map_labels.data + off);

        off += 4;
        layer = *(int *)(map_labels.data + off);
        off += 4;
        strcpy(text, (char *)(map_labels.data + off));
        off += 0x20;
        if (rlevel == cur_level_index && rscript == script) {
            index = automap_label_alloc();
            if (index < 0)
                break;
            automap_label *l = &::automap_labels[index];
            l->x = x;
            l->type = AUTOMAP_LABEL_CUSTOM;
            l->z = z;
            l->layer = layer;
            strcpy(l->text, text);
        }
    }
    return 1;
}

// 0x4d3870  Confidence: exact.
void savegame::set_pending_load(const char *filename, int slot)
{
    strcpy(pending_load, filename);
    pending_slot = slot;
}

// 0x4d38b0  Confidence: exact.
int savegame::pending_load_length()
{
    return strlen(pending_load);
}

// 0x4d38d0  Confidence: exact. From state_game_enter, after game_change_level has loaded the level.
int savegame::load_pending()
{
    int result = read(pending_load, pending_slot);

    strcpy(pending_load, "");
    pending_slot = -1;
    return result;
}

// 0x4d3920  Confidence: exact (an unused TextFile local; EH frame). From GameInit.
void savegame_alloc()
{
    TextFile tf;

    events_parse_table();
    level_parse_automap_table();
    savegame_state.init_buffer(savegame_buffer, SAVEGAME_BUFFER_SIZE);
    savegame_build_layout();
    printf("SAVE GAME ALLOCATED : %d\n", savegame_state.used);
    savegame_thumbnail_bm = bm_create(3, 201, 151, savegame_state.thumbnail.data, NULL, -1);
}

// ---- events.tbl, the automap tables ----------------------------------------------------------------------
// 0x4d39c0  Confidence: exact (m-1020c: the $Level: loop a top-tested `while (!Accept("#End"))`, the
// language name lowered through an index, so 8966 makes the pointer after the first-character test; D14702).
// The language section ("#english", ...)
// holds, per $Level:, the flag / int / float names (counted, then hashed into events_int_pool, each name
// only once over all levels), then the $Quest: records. +Float: names are checked against the int lists
// (see NOTES); a line that is none of the three never advances the file.
void events_parse_table()
{
    TextFile tf;
    TextFilePosition pos;
    char lang[0x40];
    char buf[0x800];
    level_script_info *ls;
    quest_info *q;
    int i, num_flags, num_ints, num_floats;
    unsigned int hash;

    for (i = 0; i < NUM_LEVEL_SCRIPTS; i++) {
        ls = &level_script_table[i];
        ls->num_flags = 0;
        ls->flag_hashes = NULL;
        ls->num_ints = 0;
        ls->int_hashes = NULL;
        ls->num_floats = 0;
        ls->float_hashes = NULL;
    }
    tf.Open("events.tbl", PATH_ANY);
    events_tbl_checksum = tf.Checksum();
    strcpy(lang, "#");
    strcat(lang, localize_get_language_name());
    for (i = 0; lang[i]; i++)
        lang[i] = tolower(lang[i]);
    tf.Rewind();
    tf.SkipTo(lang, NULL);
    tf.Expect(lang);
    events_int_pool_offset = 0;
    while (!tf.Accept("#End")) {
        tf.Expect("$Level:");
        tf.ParseString(buf, 0xff, '"', '"');
        i = level_script_find(buf);
        if (i == -1) {
            tf.Close();
            return;
        }
        ls = &level_script_table[i];
        tf.GetPosition(&pos);
        num_flags = 0;
        num_ints = 0;
        num_floats = 0;
        while (!tf.Peek("$Level:") && !tf.Peek("#End")) {
            if (tf.Accept("+Flag:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                num_flags++;
            } else if (tf.Accept("+Int:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                num_ints++;
            } else if (tf.Accept("+Float:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                num_floats++;
            }
        }
        ls->flag_hashes = (unsigned int *)((ubyte *)events_int_pool + events_int_pool_offset);
        events_int_pool_offset += num_flags * 4;
        ls->int_hashes = (unsigned int *)((ubyte *)events_int_pool + events_int_pool_offset);
        events_int_pool_offset += num_ints * 4;
        ls->float_hashes = (unsigned int *)((ubyte *)events_int_pool + events_int_pool_offset);
        events_int_pool_offset += num_floats * 4;
        tf.SetPosition(&pos);
        while (!tf.Peek("$Level:") && !tf.Peek("#End")) {
            if (tf.Accept("+Flag:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                hash = name_hash_nocase(buf);
                if (!level_script_flag_hash_known(hash))
                    ls->flag_hashes[ls->num_flags++] = hash;
            } else if (tf.Accept("+Int:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                hash = name_hash_nocase(buf);
                if (!level_script_int_hash_known(hash))
                    ls->int_hashes[ls->num_ints++] = hash;
            } else if (tf.Accept("+Float:")) {
                tf.ParseString(buf, 0xff, '"', '"');
                hash = name_hash_nocase(buf);
                if (!level_script_int_hash_known(hash))
                    ls->float_hashes[ls->num_floats++] = hash;
            }
        }
    }
    while (!tf.Accept("#End")) {
        tf.Expect("$Quest:");
        if (quest_count >= MAX_QUESTS)
            q = NULL;
        else
            q = &quest_table[quest_count++];
        tf.ParseString(buf, 0x7ff, '"', '"');
        if (q != NULL)
            q->name = events_string_pool.AddUnique(buf);
        q->side_quest = !tf.Accept("+Main quest");
        q->num_steps = 0;
        q->invalidate_step = -1;
        while (!tf.Peek("$Quest:") && !tf.Peek("#End")) {
            if (tf.Accept("+Invalidate:")) {
                tf.ParseString(buf, 0x7ff, '"', '"');
                q->invalidate_step = q->num_steps;
                q->steps[q->num_steps].flag_hash = name_hash_nocase(buf);
                q->num_steps++;
                break;
            }
            tf.Expect("+Stage:");
            tf.ParseString(buf, 0x7ff, '"', '"');
            localize_remap_charset(buf, 1);
            if (q != NULL)
                q->steps[q->num_steps].text = events_string_pool.AddUnique(buf);
            q->steps[q->num_steps].flag_hash = 0;
            if (tf.Accept("+Flag:")) {
                tf.ParseString(buf, 0x7ff, '"', '"');
                q->steps[q->num_steps].flag_hash = name_hash_nocase(buf);
            }
            q->num_steps++;
        }
    }
}

// 0x4d4020  Confidence: exact. The number of automap layers of each level: the highest $AutomapPC: number
// in <level>.tbl's #Automap section.
void level_parse_automap_table()
{
    TextFile tf;
    char buf[0x100];
    int i, layers, n;
    bool psx;

    for (i = 0; i < NUM_LEVEL_SCRIPTS; i++) {
        layers = 0;
        sprintf(buf, "%s.tbl", level_script_table[i].name);
        if (tf.Open(buf, path_tables_levels) == 0) {
            tf.SkipPast("#Automap", NULL);
            for (;;) {
                if (tf.Accept("$AutomapPC:"))
                    psx = false;
                else if (tf.Accept("$AutomapPSX:"))
                    psx = true;
                else
                    break;
                tf.ParseString(buf, 0x100, '"', '"');
                n = tf.ParseInt();
                if (!psx && layers <= n)
                    layers = n;
                tf.ParseString(buf, 0x100, '"', '"');
            }
            tf.Close();
        }
        level_script_table[i].automap_layers = layers;
    }
}

// 0x4d4150  Confidence: exact.
void level_script_event_flag_clear()
{
    level_script_event_active = false;
}

// 0x4d4160  Confidence: exact (EH frame for the TextFile). The buffer order is the call order; the level
// item list starts as every $Name: of level_items.tbl.
void savegame_build_layout()
{
    TextFile tf;
    char buf[0x100];

    level_script_event_active = false;
    savegame_state.clear_buffer();
    savegame_state.layout_header();
    savegame_state.layout_thumbnail();
    savegame_state.layout_level_data();
    savegame_state.layout_inventory();
    savegame_state.layout_conversation();
    savegame_state.layout_items();
    savegame_state.layout_living();
    savegame_state.layout_camera();
    savegame_state.layout_item_objects();
    savegame_state.layout_quest_flags();
    savegame_state.layout_level_items();
    savegame_state.layout_two_stage();
    savegame_state.layout_automap_labels();
    savegame_state.layout_hot_buttons();
    chain_config_reset_defaults();
    if (tf.Open("level_items.tbl", path_tables) == 0) {
        tf.Expect("#Level Items");
        while (tf.SkipPast("$Name:", "#End")) {
            tf.ParseString(buf, 0x100, '"', '"');
            savegame_state.level_item_add(buf);
        }
        tf.Close();
    }
}

// ---- game stages (console command gamestage) -------------------------------------------------------------
// 0x4d42e0  Confidence: exact (VC6 byte match) (two fall-through switches, jump tables 0x4d46f8 for stage - 1 and 0x4d4750;
// the party joins jump-thread into the second switch's level_entered_by_load test). Sets the story flags of
// every stage up to the given one, adds Jekhar / Rosalind / Flece, then (unless the level was entered from a
// save) the party level and the rings. Stage 19 sets trapped_luminar on level 14, not 22 (see NOTES).
void game_stage_set(int stage)
{
    if (game_flags & GF_MULTIPLAYER)
        return;
    switch (stage) {
    case 21:
    case 22:
        savegame_state.set_flag(23, "ready_for_end", true);
    case 20:
        savegame_state.set_flag(39, "trapped_titus", true);
        savegame_state.set_flag(22, "trapped_luminar", true);
    case 19:
        if (!savegame_state.get_flag(22, "trapped_luminar"))
            savegame_state.set_flag(39, "trapped_titus", true);
        else if (!savegame_state.get_flag(39, "trapped_titus"))
            savegame_state.set_flag(14, "trapped_luminar", true);
    case 18:
        savegame_state.set_flag(27, "trapped_pyrul", true);
    case 17:
        savegame_state.set_flag(26, "got_scrolls", true);
    case 16:
        savegame_state.set_flag(22, "found_yago", true);
    case 15:
        savegame_state.set_flag(34, "got_ring_of_jade", true);
    case 14:
        savegame_state.set_flag(20, "got_ring_of_forest", true);
    case 13:
        savegame_state.set_flag(28, "got_ring_of_wind", true);
    case 12:
        savegame_state.set_flag(16, "got_ring_of_water", true);
    case 11:
        savegame_state.set_flag(0, "found_rosalind2", true);
    case 10:
        savegame_state.set_flag(29, "found_joseph", true);
    case 9:
    case 8:
        savegame_state.set_flag(25, "found_galienne", true);
    case 7:
        savegame_state.set_flag(14, "got_ring_of_fire", true);
        savegame_state.set_flag(12, "got_ring_of_stone", true);
    case 6:
        if (!savegame_state.get_flag(14, "got_ring_of_fire"))
            savegame_state.set_flag(12, "got_ring_of_stone", true);
        else if (!savegame_state.get_flag(12, "got_ring_of_stone"))
            savegame_state.set_flag(14, "got_ring_of_fire", true);
    case 5:
        if (!jekhar_joined && !level_entered_by_load) {
            party_member_join("Jekhar", 1, 0, 1, -1);
            jekhar_joined = true;
        }
    case 4:
        savegame_state.set_flag(1, "got_ring_of_light", true);
    case 3:
        savegame_state.set_flag(15, "iona_ext_found_rosalind", true);
        if (!rosalind_joined && !level_entered_by_load) {
            party_member_join("Rosalind", 1, 0, 1, -1);
            rosalind_joined = true;
        }
    case 2:
        savegame_state.set_flag(5, "sewer_grate_open", true);
        savegame_state.set_flag(9, "sewer_found_tb", true);
        savegame_state.set_flag(24, "got_ring_of_darkness", true);
        savegame_state.set_flag(24, "lpalaceint_yago_found", true);
    case 1:
        savegame_state.set_flag(6, "lenele1c_flag_found_flece", true);
        if (!flece_joined && !level_entered_by_load) {
            party_member_join("Flece", 1, 0, 1, -1);
            flece_joined = true;
        }
    }
    if (level_entered_by_load)
        return;
    switch (stage) {
    case 22:
        party_set_level(30);
    case 21:
    case 20:
        party_set_level(24);
    case 19:
        party_set_level(23);
    case 18:
        party_set_level(22);
    case 17:
    case 16:
    case 15:
        script_party_acquire_item("Ring of Jade", 1);
        party_set_level(21);
    case 14:
        party_set_level(20);
        script_party_acquire_item("Ring of Forest", 1);
    case 13:
        party_set_level(19);
        script_party_acquire_item("Ring of Four Winds", 1);
    case 12:
        party_set_level(18);
        script_party_acquire_item("Ring of Water", 1);
    case 11:
        party_set_level(17);
    case 10:
        party_set_level(15);
    case 9:
        party_set_level(14);
    case 8:
    case 7:
        party_set_level(12);
        script_party_acquire_item("Ring of Fire", 1);
    case 6:
        party_set_level(11);
        script_party_acquire_item("Ring of Stone", 1);
    case 5:
        party_set_level(10);
    case 4:
        party_set_level(9);
        script_party_acquire_item("Ring of Light", 1);
    case 3:
    case 2:
        party_set_level(7);
        script_party_acquire_item("Ring of Darkness", 1);
    case 1:
        party_set_level(5);
    case 0:
        party_set_level(4);
    }
}

// 0x4d47b0  Confidence: exact. The highest stage whose flags are set (act2_done and found_jekhar are tested
// although game_stage_set never sets them).
int game_stage_get()
{
    if (savegame_state.get_flag(23, "ready_for_end"))
        return 21;
    if (savegame_state.get_flag(39, "trapped_titus") && savegame_state.get_flag(22, "trapped_luminar"))
        return 20;
    if (savegame_state.get_flag(22, "trapped_luminar") || savegame_state.get_flag(39, "trapped_titus"))
        return 19;
    if (savegame_state.get_flag(27, "trapped_pyrul"))
        return 18;
    if (savegame_state.get_flag(26, "got_scrolls"))
        return 17;
    if (savegame_state.get_flag(22, "found_yago"))
        return 16;
    if (savegame_state.get_flag(34, "got_ring_of_jade"))
        return 15;
    if (savegame_state.get_flag(20, "got_ring_of_forest"))
        return 14;
    if (savegame_state.get_flag(28, "got_ring_of_wind"))
        return 13;
    if (savegame_state.get_flag(16, "got_ring_of_water"))
        return 12;
    if (savegame_state.get_flag(0, "found_rosalind2"))
        return 11;
    if (savegame_state.get_flag(29, "found_joseph"))
        return 10;
    if (savegame_state.get_flag(17, "act2_done"))
        return 9;
    if (savegame_state.get_flag(25, "found_galienne"))
        return 8;
    if (savegame_state.get_flag(14, "got_ring_of_fire") && savegame_state.get_flag(12, "got_ring_of_stone"))
        return 7;
    if (savegame_state.get_flag(14, "got_ring_of_fire") || savegame_state.get_flag(12, "got_ring_of_stone"))
        return 6;
    if (savegame_state.get_flag(24, "found_jekhar"))
        return 5;
    if (savegame_state.get_flag(1, "got_ring_of_light"))
        return 4;
    if (savegame_state.get_flag(15, "iona_ext_found_rosalind"))
        return 3;
    if (savegame_state.get_flag(24, "got_ring_of_darkness"))
        return 2;
    return savegame_state.get_flag(6, "lenele1c_flag_found_flece") != 0;
}

// 0x4d4a50  Confidence: exact (VC6 8966 byte match: each case assigns result and breaks, one return, so the
// compiler duplicates the return into each case instead of merging the three flag cases, D13802; byte-indexed
// jump table 0x4d4b98 / 0x4d4b7c over cur_level_index + 1). 0 the
// game may be saved, 1 a level-script event or a monster fighting the party, 2 not in this area, 3 the
// summon is emerging (or inactive).
int savegame_location_allowed()
{
    Character *c;
    Player *p;

    if (level_script_event_active)
        return 1;
    if (cur_summon && (cur_summon->ai.is_mode(AI_STATE_SUMMONING, -1) || !(cur_summon->obj_flags & 1)))
        return 3;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if ((c->type == 5 || c->type == 9) && c->ai.is_mode(AI_STATE_COMBAT, -1))
            for (p = party_list.next; p != &party_list; p = p->next)
                if (p->handle == c->ai.target)
                    return 1;
    }
    int result;
    switch (cur_level_index) {
    case -1:
        result = 2;
        break;
    case 9:
        result = 0;
        break;
    case 14:
        result = savegame_state.get_flag(cur_level_index, "room8") ? 0 : 2;
        break;
    case 27:
        result = savegame_state.get_flag(cur_level_index, "pyrulstart") ? 2 : 0;
        break;
    case 34:
        result = savegame_state.get_flag(cur_level_index, "got_laharah_spirit") ? 2 : 0;
        break;
    case 37:
        result = savegame_state.get_flag(cur_level_index, "mach_intro") ? 2 : 0;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

// 0x4d4bc0 static_init_ccmd_gamestage (jmp) /
// 0x4d4bd0 init_ccmd_gamestage. Compiler-generated. Confidence: exact.
console_command ccmd_gamestage("gamestage", "Set the important event flags to different stages of the game.",
                               cmd_gamestage);   // 0x24dfb30

// 0x4d4bf0  Confidence: exact (the call path returns at once).
void cmd_gamestage()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        game_stage_set(command_argument_int);
        return;
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Can't properly judge game stage.");
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: gamestage <x>, where x is the stage of the game.", 0);
}

// 0x4d4c50 static_init_ccmd_savegame (jmp) /
// 0x4d4c60 init_ccmd_savegame. Compiler-generated. Confidence: exact.
console_command ccmd_savegame("savegame", "Save the game to a file", cmd_savegame);   // 0x2494b20

// 0x4d4c80  Confidence: exact.
void cmd_savegame()
{
    char filename[0x20];
    char description[0x100];
    int len;

    if (doing_help_for_comand) {
        print_to_console("Usage: savegame <x>, where x is the filename (do not include extension)", 0);
        return;
    }
    if (!calling_a_command_function)
        return;
    if (gameseq_get_state() != 4) {
        print_to_console("Can only save in gameplay mode.", 0);
        return;
    }
    switch (savegame_location_allowed()) {
    case 1:
        print_to_console("Cannot save in combat.", 0);
        return;
    case 2:
        print_to_console("Cannot save in this area.", 0);
        return;
    case 3:
        print_to_console("Cannot save while summon is emerging.", 0);
        return;
    }
    process_command(ARG_STRING);
    len = strlen(next_arg);
    if (len <= 0) {
        print_to_console("No filename specified.", 0);
        return;
    }
    if (len > 0x1f - (int)strlen(".sav")) {
        print_to_console("Filename is too long.", 0);
        return;
    }
    if (strpbrk(next_arg, "\\/:*?\"<>|.")) {
        print_to_console("Filename contains invalid characters.", 0);
        return;
    }
    file_change_ext(filename, next_arg, ".sav");
    saveload_capture_thumbnail();
    conversation_db.save_state();
    strcpy(description, "No description");
    if (savegame_state.write(filename, 0, description))
        sprintf(string_buffer, "Saved \"%s\".\n", filename);
    else
        sprintf(string_buffer, "Error saving \"%s\".\n", filename);
    print_to_console(string_buffer, 0);
}

// 0x4d4e70 static_init_ccmd_loadgame (jmp) /
// 0x4d4e80 init_ccmd_loadgame. Compiler-generated. Confidence: exact.
console_command ccmd_loadgame("loadgame", "Load a previously saved game", cmd_loadgame);   // 0x2493a18

// 0x4d4ea0  Confidence: exact.
void cmd_loadgame()
{
    float time;
    int unk_a8;
    char filename[0x20];
    char level_name[0x40];
    char script_name[0x40];
    char description[0x100];
    int len;

    if (doing_help_for_comand) {
        print_to_console("Usage: loadgame <x>, where x is the filename (do not include extension)", 0);
        return;
    }
    if (!calling_a_command_function)
        return;
    process_command(ARG_STRING);
    len = strlen(next_arg);
    if (len <= 0) {
        print_to_console("No filename specified.", 0);
        return;
    }
    if (len > 0x1f - (int)strlen(".sav")) {
        print_to_console("Filename is too long.", 0);
        return;
    }
    if (strpbrk(next_arg, "\\/:*?\"<>|.")) {
        print_to_console("Filename contains invalid characters.", 0);
        return;
    }
    file_change_ext(filename, next_arg, ".sav");
    if (savegame_state.read_info(filename, level_name, script_name, &unk_a8, &time, description, NULL)) {
        savegame_state.set_pending_load(filename, 0);
        game_change_level(level_name, script_name);
    } else
        print_to_console("Unable to read header from file.\n", 0);
}

// 0x4d4ff0 static_init_script_object_timers (jmp) /
// 0x4d5000 init_script_object_timers: the implicit
// constructor of the 200 records (one GameTimer each, at +0x518). Confidence: exact.
script_object script_objects[MAX_SCRIPT_OBJECTS];   // 0x24f6230

// 0x4d5020 static_init_empty_4d5030 (jmp) /
// 0x4d5030 init_empty_4d5030: an empty dynamic initialiser (an
// object with an inline empty constructor whose definition is not known). Not reproduced (see NOTES).
// Confidence: unsure. VC6 /O2 /Ob1 emits exactly this `jmp` / bare `ret` pair for a global of a class
// whose inline constructor is empty (an array of vector3 does it: tested, D5600), so the object is such
// a definition; which one is not known, so none is written.

// 0x4d5040 static_init_stringpool_2539f18 (call; jmp) /
// 0x4d5050 init_stringpool_2539f18: StringPool(buffer, 0x5000, false) /
// 0x4d5070 register_atexit_stringpool_2539f18 /
// 0x4d5080 atexit_dtor_stringpool_2539f18 (~StringPool). Compiler-generated. Confidence: exact.
StringPool level_script_stringpool(level_script_string_buffer, 0x5000, false);           // 0x2539f18
// 0x4d5090 static_init_stringpool_24f6218 /
// 0x4d50a0 init_stringpool_24f6218 /
// 0x4d50c0 register_atexit_stringpool_24f6218 /
// 0x4d50d0 atexit_dtor_stringpool_24f6218. Compiler-generated. Confidence: exact.
StringPool events_string_pool(events_string_buffer, 0x8400, false);                     // 0x24f6218
// 0x4d50e0 static_init_stringpool_24ed600 /
// 0x4d50f0 init_stringpool_24ed600 /
// 0x4d5110 register_atexit_4d5110 /
// 0x4d5120 atexit_dtor_stringpool_24ed600. Compiler-generated. Confidence: exact.
StringPool script_filenames_string_pool(script_filenames_string_buffer, 0x800, false);  // 0x24ed600

// ---- .data: the level script table and the level table ------------------------------------------------
int cur_level_index = -1;                            // 0x59f120

// 0x59f128  Confidence: exact (every field from the image). The hooks are the per-level units' functions;
// records that share a folded body name the sum.symbols copy (ls_test_use, empty_function). The name
// hash lists, their counts and automap_layers are filled at start-up.
#define LS_EMPTY_ACTION ((void (*)(dialogue_action *))empty_function)
level_script_info level_script_table[NUM_LEVEL_SCRIPTS] = {
    { "Wolong", ls_wolong_init, ls_wolong_frame, empty_function, ls_wolong_use, 0, ls_wolong_action, 7000, "Wolong" },
    { "Catacombs", ls_catacombs_init, ls_catacombs_frame, empty_function, ls_catacombs_use, 0, LS_EMPTY_ACTION, 4000, "Catacombs" },
    { "test", ls_test_init, ls_test_frame, ls_test_leave, ls_test_use, 0, LS_EMPTY_ACTION, 1170, 0 },
    { "masad", ls_masad_init, ls_masad_frame, ls_masad_leave, ls_masad_use, 0, ls_masad_action, 4000, "Masad" },
    { "worldmap1", ls_worldmap1_init, ls_worldmap1_frame, ls_worldmap1_leave, 0, 0, 0, 2300, "worldmap1" },
    { "lenele1b", ls_lenele1b_init, ls_lenele1b_frame, empty_function, ls_lenele1b_use, 0, ls_lenele1b_action, 8500, "lenele slumdock" },
    { "lenele1c", ls_lenele1c_init, ls_lenele1c_frame, empty_function, ls_lenele1c_use, 0, LS_EMPTY_ACTION, 4300, "lenele marketplace" },
    { "lenele1d", ls_lenele1d_init, ls_lenele1d_frame, empty_function, ls_lenele1d_use, 0, LS_EMPTY_ACTION, 3300, "lenele slumdock" },
    { "lenele1e", ls_lenele1e_init, empty_function, empty_function, ls_lenele1e_use, 0, LS_EMPTY_ACTION, 2500, "lenele marketplace" },
    { "sewer", ls_sewer_init, ls_sewer_frame, empty_function, ls_sewer_use, 0, ls_sewer_action, 4700, "Sewer" },
    { "rand-hills01", ls_rand_hills01_init, ls_rand_hills01_frame, empty_function, 0, 0, ls_rand_hills01_action, 2500, "rand-hills01" },
    { "IkaemosBottomInt", ls_ikaemosbottomint_init, empty_function, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 3400, "IkaemosPalace" },
    { "IkaemosTopInt", ls_ikaemostopint_init, ls_ikaemostopint_frame, empty_function, ls_ikaemostopint_use, 0, LS_EMPTY_ACTION, 3200, "IkaemosPalace" },
    { "IkaemosExt", ls_ikaemosext_init, ls_ikaemosext_frame, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 2700, "Ikaemos1" },
    { "KhosaniLab", ls_khosanilab_init, ls_khosanilab_frame, ls_khosanilab_leave, ls_khosanilab_use, 0, LS_EMPTY_ACTION, 3050, "KhosaniLab" },
    { "IonaExt", ls_ionaext_init, ls_ionaext_frame, empty_function, ls_test_use, (void *)ls_test_use, ls_ionaext_action, 3900, "IonaExt" },
    { "WolongCaverns", ls_wolongcaverns_init, ls_wolongcaverns_frame, empty_function, ls_wolongcaverns_use, 0, ls_wolongcaverns_action, 7600, "WolongCaverns" },
    { "TempleInt", ls_templeint_init, ls_templeint_frame, empty_function, ls_templeint_use, 0, ls_templeint_action, 3250, "TempleInt" },
    { "rand-forest01", ls_rand_forest01_init, ls_rand_forest01_frame, empty_function, 0, 0, ls_rand_forest01_action, 2700, "rand-forest01" },
    { "rand-forestnite1", ls_rand_forestnite1_init, ls_rand_forestnite1_frame, empty_function, 0, 0, LS_EMPTY_ACTION, 2600, "rand-forestnite1" },
    { "Liangshan", ls_liangshan_init, ls_liangshan_frame, empty_function, ls_liangshan_use, 0, ls_liangshan_action, 5600, "Liangshan" },
    { "Rand-Desert", ls_rand_desert_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1600, "Rand-Desert" },
    { "IonaExt02", ls_ionaext02_init, ls_ionaext02_frame, ls_ionaext02_leave, ls_test_use, 0, LS_EMPTY_ACTION, 4700, "IonaExt2" },
    { "KhosaniStrng", ls_khosanistrng_init, ls_khosanistrng_frame, empty_function, ls_test_use, 0, ls_khosanistrng_action, 3550, "KhosaniStrng" },
    { "LPalaceInt", ls_lpalaceint_init, ls_lpalaceint_frame, empty_function, ls_lpalaceint_use, 0, ls_lpalaceint_action, 6700, "LPalaceInt" },
    { "tancredhouse", ls_tancredhouse_init, empty_function, empty_function, ls_test_use, 0, ls_tancredhouse_action, 1500, "tancredhouse" },
    { "LPalaceInt02", ls_lpalaceint02_init, ls_lpalaceint02_frame, ls_lpalaceint02_leave, ls_lpalaceint02_use, 0, LS_EMPTY_ACTION, 2700, "LPalaceInt02" },
    { "KhosaniLab2", ls_khosanilab2_init, ls_khosanilab2_frame, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 3100, "KhosaniLab" },
    { "Wolong2", ls_wolong2_init, ls_wolong2_frame, empty_function, ls_wolong2_use, 0, ls_wolong2_action, 5800, "Wolong" },
    { "eleh", ls_eleh_init, ls_eleh_frame, empty_function, ls_eleh_use, 0, ls_eleh_action, 4800, "eleh" },
    { "lenele2aa", ls_lenele2aa_init, ls_lenele2aa_frame, empty_function, ls_lenele2aa_use, 0, ls_lenele2aa_action, 8800, "lenele2" },
    { "lenele2ab", ls_lenele2ab_init, ls_lenele2ab_frame, empty_function, ls_test_use, 0, ls_lenele2ab_action, 7700, "lenele2" },
    { "lenele3a", ls_lenele3a_init, empty_function, ls_lenele3a_leave, ls_lenele3a_use, 0, LS_EMPTY_ACTION, 7000, "lenele3" },
    { "lenele3d", ls_lenele3d_init, ls_lenele3d_frame, ls_ionaext02_leave, ls_lenele3d_use, 0, LS_EMPTY_ACTION, 3300, "lenele3" },
    { "jadetemple", ls_jadetemple_init, ls_jadetemple_frame, empty_function, ls_test_use, 0, ls_jadetemple_action, 4100, "jadetemple" },
    { "lenele1aa", ls_lenele1aa_init, ls_lenele1aa_frame, empty_function, ls_lenele1aa_use, 0, ls_lenele1aa_action, 7600, "lenele marketplace" },
    { "lenele1ab", ls_lenele1ab_init, empty_function, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 7200, "lenele marketplace" },
    { "TempleInt2", ls_templeint2_init, ls_templeint2_frame, empty_function, ls_templeint2_use, 0, LS_EMPTY_ACTION, 3700, "TempleInt2" },
    { "IkaemosExt2", ls_ikaemosext2_init, ls_ikaemosext2_frame, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 2400, "IkaemosExt" },
    { "IkaemosBottomInt2", ls_ikaemosbottomint2_init, ls_ikaemosbottomint2_frame, empty_function, ls_test_use, 0, LS_EMPTY_ACTION, 2500, "IkaemosPalace" },
    { "Rand-HillsNite01", ls_rand_hillsnite01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 2300, "rand-hills01" },
    { "Rand-Iceland01", ls_rand_iceland01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 2000, "rand-iceland01" },
    { "Rand-Grassland01", ls_rand_grassland01_init, empty_function, empty_function, 0, 0, ls_rand_grassland01_action, 1700, "rand-grassland01" },
    { "Rand-Orenia01", ls_rand_orenia01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1400, "rand-orenia01" },
    { "Rand-OreniaNite01", ls_rand_orenianite01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1300, "rand-orenia01" },
    { "Rand-DesertNite01", ls_rand_desertnite01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1500, "Rand-Desert" },
    { "Rand-GrasslandNite01", ls_rand_grasslandnite01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1700, "rand-grassland01" },
    { "Rand-IcelandNite01", ls_rand_iceland01_init, empty_function, empty_function, 0, 0, LS_EMPTY_ACTION, 1900, "rand-iceland01" },
    { "endgame", 0, 0, 0, 0, 0, 0, 10000, 0 },
    { "lenele2d", ls_lenele2d_init, ls_lenele2d_frame, empty_function, ls_lenele2d_use, 0, ls_lenele2d_action, 3400, "lenele2" },
    { "sewerboss", ls_sewerboss_init, ls_sewerboss_frame, ls_sewerboss_leave, ls_sewer_use, 0, ls_sewer_action, 1700, "Sewer" },
};

// 0x4d5130 static_init_level_table (jmp) /
// 0x4d5140 init_level_table, the table at 0x59fde8. Confidence: likely (the
// localize_strings slot is not a constant expression to VC6, so everything after entry 0's id is stored by
// the initialiser, as for level_display_names, D602; the order of the stores is the compiler's).
level_table_entry level_table[26] = {
    { 3,  XSTR_SLOT("Masad Village", 956), 0x3, 0 },
    { 7,  XSTR_SLOT("Lenele Outskirts -- Summer", 957), 0x5, 2 },
    { 9,  XSTR_SLOT("Lenele Sewers", 958), 0x4, 2 },
    { 50, XSTR_SLOT("Lenele Palace Aqueduct", 959), 0x4, 2 },
    { 24, XSTR_SLOT("Lenele Palace", 960), 0x4, 2 },
    { 15, XSTR_SLOT("Iona Monastery", 961), 0x9, 4 },
    { 1,  XSTR_SLOT("Iona Catacombs", 962), 0x8, 4 },
    { 13, XSTR_SLOT("Ikaemos Swamp -- Night", 963), 0x11, 4 },
    { 12, XSTR_SLOT("Ikaemos Palace Upper Level", 964), 0x10, 4 },
    { 23, XSTR_SLOT("City of the Khosani", 965), 0x21, 4 },
    { 14, XSTR_SLOT("Labyrinth of the Khosani", 966), 0x20, 4 },
    { 30, XSTR_SLOT("Lenele Marketplace", 967), 0x41, 0x38 },
    { 31, XSTR_SLOT("Lenele Crown District", 968), 0x40, 0x38 },
    { 17, XSTR_SLOT("Temple of Urath", 969), 0x40, 0x38 },
    { 29, XSTR_SLOT("Tower of Eleh", 970), 0x81, 0x40 },
    { 0,  XSTR_SLOT("Wolong", 971), 0x101, 0x80 },
    { 16, XSTR_SLOT("Caverns of Wolong", 972), 0x100, 0x80 },
    { 20, XSTR_SLOT("Liangshan Forest", 973), 0x201, 0x80 },
    { 34, XSTR_SLOT("The Jade Temple", 974), 0x401, 0x300 },
    { 27, XSTR_SLOT("Ruins of Khosani Labyrinth", 975), 0x801, 0x400 },
    { 38, XSTR_SLOT("Ikaemos Swamp -- Day", 976), 0x1001, 0x400 },
    { 39, XSTR_SLOT("Ikaemos Palace", 977), 0x1000, 0x400 },
    { 22, XSTR_SLOT("Ruins of Iona Monastery", 978), 0x2001, 0x400 },
    { 33, XSTR_SLOT("Lenele Outskirts -- Winter", 979), 0x4001, 0x3800 },
    { 32, XSTR_SLOT("Lenele City", 980), 0x4000, 0x3800 },
    { 37, XSTR_SLOT("Temple of Urath Revisited", 981), 0x4000, 0x3800 },
};

// ---- level script selection and helpers ---------------------------------------------------------------
// 0x4d54e0  Confidence: exact (an unused File local; EH frame).
void level_create_world_effects()
{
    File f;
    int i;

    for (i = 0; i < level_effect_count; i++)
        level_effects[i].handle = fx_create_world(level_effects[i].name, level_effects[i].flag);
}

// 0x4d5570  Confidence: exact. Without a script
// (or in multiplayer) "<level>.vfx" is written into the next effect record, which level_effect_count does
// not cover (see NOTES); in multiplayer the music comes from the table.
void level_script_select()
{
    char buf[0x100];


    level_script_init_fn = NULL;
    level_script_frame_fn = NULL;
    level_script_leave_fn = NULL;
    level_script_use_fn = NULL;
    level_script_unk14_fn = NULL;
    level_script_action_fn = NULL;
    cur_level_index = level_script_find(level_filename);
    if (cur_level_index != -1 && level_script_table[cur_level_index].init != NULL &&
        !(game_flags & GF_MULTIPLAYER)) {
        level_script_init_fn = level_script_table[cur_level_index].init;
        level_script_frame_fn = level_script_table[cur_level_index].frame;
        level_script_leave_fn = level_script_table[cur_level_index].leave;
        level_script_use_fn = level_script_table[cur_level_index].use;
        level_script_unk14_fn = level_script_table[cur_level_index].unused14;
        level_script_action_fn = level_script_table[cur_level_index].action;
    } else {
        if (!(game_flags & GF_MULTIPLAYER))
            sprintf(buf, "Unable to locate level script info for %s\n", level_filename);
        strcpy(level_effects[level_effect_count].name, level_filename);
        strcat(level_effects[level_effect_count].name, ".vfx");
        level_create_world_effects();
    }
    level_script_reparse();
    if ((game_flags & GF_MULTIPLAYER) && cur_level_index != -1)
        level_music = music_find_by_name(level_script_table[cur_level_index].music);
}

// 0x4d56d0  Confidence: exact.
int level_script_load_steps(const char *level_file)
{
    char name[0x100];
    int i;

    file_strip_ext(name, level_file);
    i = level_script_find(name);
    if (i != -1)
        return level_script_table[i].load_steps;
    return 10000;
}

// 0x4d5720  Confidence: likely (the vector arithmetic is the inline vector3 operators, D500). The navpoint
// is dropped onto the geometry below it (a ray 1 up to 1000 down) and takes that face's layer.
bool level_add_navpoint(s3d_navpoint *src)
{
    lobj_ray ray;
    lobj_hit hit;
    float p, b, h;
    vector3 hit_pos;
    int n;
    int layer;
    char buf[0x100];

    n = navpoint_count++;
    strcpy(navpoints[n].name, src->name);
    navpoints[n].pos = src->pos;
    src->orient.extract_angles(&p, &b, &h);
    navpoints[n].heading = h;
    navpoints[n].ground_height = 0;
    navpoints[n].layer = 0;
    ray.start.x = src->pos.x;
    ray.start.y = src->pos.y + 1.0f;
    ray.start.z = src->pos.z;
    ray.dir = vector3(0.0f, -1000.0f, 0.0f);
    ray._unk18 = 0.0f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0x400;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits != 0) {
        hit_pos = ray.start + ray.dir * hit.t;
        layer = lobj_from_index(hit.face)->get_layer_n(0);
        if (layer == -1) {
            sprintf(buf, "Layer for navpoint %s is invalid -- assuming no layer set exists and setting layer to 0\n",
                    src->name);
            layer = 0;
        }
        navpoints[n].layer = layer;
        navpoints[n].pos = hit_pos;
    }
    return true;
}

// 0x4d58e0  Confidence: exact. Caller level_load_files.
void navpoints_reset()
{
    navpoint_count = 0;
    level_item_count = 0;
}

// 0x4d58f0  Confidence: exact.
void script_object_suspend(int index)
{
    script_object *so;
    Character *c;

    if (index == -1)
        return;
    so = &script_objects[index];
    c = living_from_handle(so->handle);
    if (!so->suspended) {
        c->ai.goals_clear();
        c->ai.go_idle();
    }
    so->suspended = 1;
    if (c->get_current_move())
        c->instance->clear_action_anims();
}

// 0x4d5960  Confidence: exact.
void script_object_resume_index(int index)
{
    script_object *so;

    if (index == -1)
        return;
    so = &script_objects[index];
    if (so->suspended)
        script_object_resume(so);
}

// 0x4d5990  Confidence: likely (jump table 0x4d5dec; the tails of kinds 7 and 11 are shared in the binary).
// The +Action: kinds of a dialogue line: 0 the level script's action hook, 1 xp, 2 skill points, 3 gold,
// 4 gain an item, 5 lose an item, 6 set a flag, 7-12 make the current speaker's topic `name` available
// (or not) by item, flag or gold.
void conversation_execute_action(dialogue_action *a)
{
    char buf[0x100];
    char item_name[0x100];
    Player *p;
    Character *c;
    int amount, slot;

    switch (a->kind) {
    case 0:
        if (level_script_action_fn)
            level_script_action_fn(a);
        break;
    case 6:
        savegame_state.set_flag(cur_level_index, a->name, true);
        break;
    case 7:
        if (party_count_item_by_name(item_types[a->value].name))
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, true);
        else
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, false);
        break;
    case 8:
        if (savegame_state.get_flag(a->value, a->flag))
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, true);
        break;
    case 9:
        if (party_count_item_by_name(item_types[a->value].name))
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, false);
        break;
    case 10:
        if (savegame_state.get_flag(a->value, a->flag))
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, false);
        break;
    case 11:
        if (party_gold < a->value)
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, false);
        else
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, true);
        break;
    case 12:
        if (party_gold >= a->value)
            conversation_db.set_topic_available(conversation_db.speaker->name, a->name, false);
        break;
    case 1:
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if ((c->flags & CF_SUMMON) || (c->flags & CF_HIDDEN))
                continue;
            if (a->species == -1)
                c->add_xp(a->value, 0, 0);
            else if (c->info->species == a->species) {
                c->add_xp(a->value, 0, 0);
                sprintf(buf, XSTR("<%s gained %d XP>", 469), c->get_name(), a->value);
                dialogue_show_line(buf, 1, 0);
                break;
            }
        }
        if (a->species == -1) {
            sprintf(buf, XSTR("<party gained %d XP>", 470), a->value);
            dialogue_show_line(buf, 1, 0);
        }
        break;
    case 2:
        if (a->species == -1)
            break;
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if ((c->flags & CF_SUMMON) || (c->flags & CF_HIDDEN))
                continue;
            if (c->info->species == a->species) {
                p->skill_points += a->value;
                sprintf(buf, XSTR("<%s gained %d skill points>", 471), c->get_name(), a->value);
                dialogue_show_line(buf, 1, 0);
                break;
            }
        }
        break;
    case 3:
        party_gold += a->value;
        if (party_gold < 0)
            party_gold = 0;
        amount = a->value;
        if (amount >= 0)
            sprintf(buf, XSTR("<party received %d gold>", 472), amount);
        else
            sprintf(buf, XSTR("<party paid %d gold>", 473), -amount);
        dialogue_show_line(buf, 1, 0);
        break;
    case 4:
        party_inventory_add_type(a->value, 1, -1, 1);
        item_get_display_name(item_name, &item_types[a->value]);
        sprintf(buf, XSTR("<gained item: \"%s\">", 474), item_name);
        dialogue_show_line(buf, 1, 0);
        break;
    case 5:
        slot = party_inventory_find_type(&item_types[a->value]);
        if (slot < 0)
            break;
        if (party_inventory[slot].count > 1)
            party_inventory[slot].count--;
        else
            party_inventory_remove(slot, 1, 1);
        item_get_display_name(item_name, &item_types[a->value]);
        sprintf(buf, XSTR("<lost item: \"%s\">", 475), item_name);
        dialogue_show_line(buf, 1, 0);
        break;
    }
}

// 0x4d5e20  Confidence: exact. Callers character_use_skill, container_examine, ...
char level_script_use(const char *name)
{
    if (level_script_use_fn)
        return level_script_use_fn(name);
    return 0;
}

// 0x4d5e40 static_init_ccmd_door_open (jmp) /
// 0x4d5e50 init_ccmd_door_open. Compiler-generated. Confidence: exact.
console_command ccmd_door_open("door_open", "open a specified door in the level.", cmd_door_open);   // 0x25388b0

// 0x4d5e70  Confidence: exact. door_find_by_name's result is not tested: an unknown name passes an
// uninitialised lobj (see NOTES).
void cmd_door_open()
{
    lobj *l;
    door_info *d;

    if (calling_a_command_function) {
        process_command(ARG_STRING);
        door_find_by_name(next_arg, &l, &d);
        door_set_open(l, -1.0f, 1, 1);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: door_open door_name", 0);
}

// 0x4d5ed0  Confidence: exact.
int level_script_flag_hash_known(unsigned int hash)
{
    int level, i;

    for (level = 0; level < NUM_LEVEL_SCRIPTS; level++)
        for (i = 0; i < level_script_table[level].num_flags; i++)
            if (hash == level_script_table[level].flag_hashes[i])
                return 1;
    return 0;
}

// 0x4d5f10  Confidence: exact.
int level_script_int_hash_known(unsigned int hash)
{
    int level, i;

    for (level = 0; level < NUM_LEVEL_SCRIPTS; level++)
        for (i = 0; i < level_script_table[level].num_ints; i++)
            if (hash == level_script_table[level].int_hashes[i])
                return 1;
    return 0;
}

// 0x4d5f50  Confidence: exact (the Crc32 constructor is inline, then init 0x552120 and update 0x552130).
// A name longer than 127 characters overruns the local copy.
int name_hash_nocase(const char *name)
{
    Crc32 crc;
    char buf[0x80];
    int len, i;

    if (!name)
        return 0;
    len = strlen(name);
    strcpy(buf, name);
    for (i = 0; i < len; i++)
        buf[i] = tolower(buf[i]);
    crc.init();
    return crc.update(buf, len);
}

// 0x4d5ff0  Confidence: exact. A NULL name gives quest 0, not -1.
int quest_find_by_name(const char *name)
{
    int i;

    if (!name)
        return 0;
    for (i = 0; i < quest_count; i++)
        if (_stricmp(name, quest_table[i].name) == 0)
            return i;
    return -1;
}

// 0x4d6040  Confidence: exact. Shows "Quest Journal Updated" whatever the value (also for the 200 clears of
// savegame_layout_quest_flags).
void quest_journal_set_entry(int quest, int value)
{
    if (quest < 0 || quest >= MAX_QUESTS)
        return;
    if (value)
        savegame_state.set_quest_bit(quest, true);
    else
        savegame_state.set_quest_bit(quest, false);
    hud_show_message(XSTR("Quest Journal Updated", 476));
}

// 0x4d6080  Confidence: exact. The index of the last stage reached: stages count while their flag is set
// (or they have none), up to the +Invalidate: entry.
int quest_get_current_step(int quest)
{
    quest_info *q;
    int step, i;

    if (quest < 0 || quest >= quest_count)
        return -1;
    if (!savegame_state.get_quest_bit(quest))
        return -1;
    step = -1;
    q = &quest_table[quest];
    for (i = 0; i < q->num_steps; i++) {
        if (q->invalidate_step >= 0 && i == q->invalidate_step)
            break;
        if (q->steps[i].flag_hash && !savegame_state.find_flag_level(q->steps[i].flag_hash))
            break;
        step++;
    }
    return step;
}

// 0x4d6100  Confidence: exact (m-1020c: both loops are top-tested whiles, `while (!Accept("#End"))` and
// `while (Accept("+Script:"))`, and every save info field goes through CONVERSATION_SAVE_INFO; D14702).
// Up to 7 +Script: names per level with their +Dialogue: speaker and topic
// counts (the conversation save block sizes) and +Levelcap: (100).
void script_filenames_parse_table()
{
    TextFile tf;
    char buf[0x100];
    int level, slot;

    script_filenames_string_pool.Reset();
    for (level = 0; level < NUM_LEVEL_SCRIPTS; level++) {
        for (slot = 0; slot < SAVEGAME_NUM_SLOTS; slot++) {
            CONVERSATION_SAVE_INFO(level, slot)->name = NULL;
            CONVERSATION_SAVE_INFO(level, slot)->num_speakers = 0;
            CONVERSATION_SAVE_INFO(level, slot)->num_topics = 0;
        }
    }
    if (tf.Open("script_filenames.tbl", path_tables) == 0) {
        Script_filenames_tbl_checksum = tf.Checksum();
        while (!tf.Accept("#End")) {
            tf.Expect("$Level:");
            tf.ParseString(buf, 0x100, '"', '"');
            level = level_script_find(buf);
            if (level == -1)
                break;
            slot = 0;
            while (tf.Accept("+Script:")) {
                if (slot >= SAVEGAME_NUM_SLOTS)
                    break;
                tf.ParseString(buf, 0x100, '"', '"');
                CONVERSATION_SAVE_INFO(level, slot)->name = script_filenames_string_pool.AddUnique(buf);
                if (tf.Accept("+Dialogue:")) {
                    CONVERSATION_SAVE_INFO(level, slot)->num_speakers = tf.ParseInt();
                    CONVERSATION_SAVE_INFO(level, slot)->num_topics = tf.ParseInt();
                }
                if (tf.Accept("+Levelcap:"))
                    CONVERSATION_SAVE_INFO(level, slot)->level_cap = tf.ParseInt();
                else
                    CONVERSATION_SAVE_INFO(level, slot)->level_cap = 100;
                slot++;
            }
        }
        tf.Close();
    }
}

// 0x4d6300  Confidence: exact.
int level_script_find(const char *level_name)
{
    int i;

    for (i = 0; i < NUM_LEVEL_SCRIPTS; i++)
        if (_stricmp(level_name, level_script_table[i].name) == 0)
            return i;
    return -1;
}

// 0x4d6340  Confidence: exact.
int level_script_find_index(int level, const char *script_name)
{
    conversation_save_info *info;
    int i;

    for (i = 0; i < SAVEGAME_NUM_SLOTS; i++) {
        info = CONVERSATION_SAVE_INFO(level, i);
        if (!info->name)
            return -1;
        if (_stricmp(info->name, script_name) == 0)
            return i;
    }
    return -1;
}

// 0x4d6390  Confidence: exact.
int level_table_find_id(int level)
{
    int i;

    for (i = 0; i < 26; i++)
        if (level_table[i].id == level)
            return i;
    return -1;
}

// 0x4d63b0  Confidence: likely (block order). The localized name up to its first digit or '#'.
void level_format_display_name(char *out, int level)
{
    int i, n;

    if (level != -1) {
        for (i = 0; i < 51; i++)
            if (_stricmp(level_display_names[i].name, level_script_table[level].name) == 0)
                break;
        if (i < 51 && strlen(localize_string_or_invalid(level_display_names[i].text)) != 0) {
            n = strcspn(localize_string_or_invalid(level_display_names[i].text), "#01234567890");
            if (n) {
                strncpy(out, localize_string_or_invalid(level_display_names[i].text), n);
                out[n] = 0;
            } else
                strcpy(out, localize_string_or_invalid(level_display_names[i].text));
            return;
        }
    }
    strcpy(out, XSTR("Unknown", 352));
}

// 0x4d64c0  Confidence: exact.
char *level_table_display_name(int level)
{
    if (level < 0 || level >= 26)
        return NULL;
    return localize_string_or_invalid(level_table[level].display_name);
}

// 0x4d64f0  Confidence: exact. Cutscene start.
void level_ambient_pause_all()
{
    int i;

    for (i = 0; i < level_ambient_count; i++)
        sound_emitter_pause(level_ambients[i].emitter);
}

// 0x4d6520  Confidence: exact.
void level_ambient_unpause_all()
{
    int i;

    for (i = 0; i < level_ambient_count; i++)
        sound_emitter_unpause(level_ambients[i].emitter);
}

// 0x4d6550 static_init_ccmd_door_close (jmp) /
// 0x4d6560 init_ccmd_door_close. Compiler-generated. Confidence: exact.
console_command ccmd_door_close("door_close", "closes a specified door in the level.", cmd_door_close);   // 0x253ef30

// 0x4d6580  Confidence: exact.
void cmd_door_close()
{
    lobj *l;
    door_info *d;

    if (calling_a_command_function) {
        process_command(ARG_STRING);
        door_find_by_name(next_arg, &l, &d);
        door_set_closed(l);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: door_close door_name", 0);
}

// 0x4d65d0 static_init_ccmd_door_toggle (jmp) /
// 0x4d65e0 init_ccmd_door_toggle. Compiler-generated. Confidence: exact.
console_command ccmd_door_toggle("door_toggle", "opens or closes a specified door in the level.",
                                 cmd_door_toggle);   // 0x24ed5f0

// 0x4d6600  Confidence: exact.
void cmd_door_toggle()
{
    lobj *l;
    door_info *d;

    if (calling_a_command_function) {
        process_command(ARG_STRING);
        door_find_by_name(next_arg, &l, &d);
        door_toggle_state(l, 1);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: door_toggle door_name", 0);
}
