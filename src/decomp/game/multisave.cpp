// multisave.cpp - Sum.exe v1.40, code 0x46cc60-0x46dd2f (the multiplayer character files, the autosave
// timer, multi.cfg)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl id 237 in mp_player_file_write). It starts right after
// item_paging.cpp's last jump table (0x46cc50-0x46cc5f) and ends before s3d.cpp's sinit_level_names_a5a2a0
// (0x46dd30).
// Data: .data 0x59a978-0x59aa53 ("Book of Novices", the multi.cfg strings); ".plr" (0x58a0e0) and
// "Health Tonic" (0x593f08) are pooled copies in earlier units. .bss 0xa59a84-0xa5a29b.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define MP_NUM_EQUIP_SLOTS  9         // Character.equipment[0..8] (slot 5 the weapon)
#define MP_NUM_SAVED_SKILLS 36        // the base skills, Character.skill[0..0x23] (skill_names)
#define MP_NUM_HOT_BUTTONS  6

// ---- .bss ----------------------------------------------------------------------------------------
// The compiler's dynamic initialiser pair of this definition (static_initializers entry 206). Moved here
// from item_paging.cpp (decomp24, D405): the code range is this unit's in sum.symbols, and only this
// unit's autosave functions use the timer (one marker per line, decomp76):
// 0x46cc60 init_realtimetimer_a59a84_thunk (jmp 0x46cc70; the listing mis-decodes it after item_paging.cpp's
// jump table). Confidence: exact.
// 0x46cc70 init_realtimetimer_a59a84 (mov ecx, 0xa59a84 / jmp RealtimeTimer__RealtimeTimer). Confidence: exact.
RealtimeTimer mp_player_autosave_timer;                       // 0xa59a84
pxo_config pxo_cfg;                                           // 0xa59a88 (0x610 bytes)
ushort inet_proxy_port;                                       // 0xa5a098 read by InetGetFile__InetGetFile only
char inet_proxy_server[0x200];                                // 0xa5a09c (then 4 bytes to s3d.cpp's 0xa5a2a0)

// 0x46cc80  Confidence: exact (D13503: the directory result through a local, compared with the zero register;
// the default case clears the pointer first. The species switch: jump table 0x46d098 maps 0,1,2,3 to bodies laid out
// as 0,2,3,1, so the source lists Jekhar last; the default stores only the pointer, the count register
// is already 0). A new character: level 1, no experience, full base HP / AP of its char_info, the
// default weapon in slot 5, 3 Health Tonics and a Book of Novices, 30 gold, the level-1 skills (Speed
// always 10), the four chain directions of the char_info at 0 points and no hot buttons.
int mp_player_file_create(const char *name, int char_info_index, const char *description)
{
    File f;
    char filename[64];

    file_change_ext(filename, name, ".plr");
    if (f.find(filename, path_players))
        return 1;
    int err = file_make_path_dirs(path_players);
    if (err)
        return 2;
    if (f.open(filename, 2, path_players))
        return 3;

    char_info *info = &char_infos[char_info_index];
    skill_progression *prog;
    int num_prog;
    int i, j;

    switch (info->species) {
    case SPECIES_JOSEPH:
        num_prog = joseph_skill_progression_count;
        prog = joseph_skill_progression;
        break;
    case SPECIES_FLECE:
        num_prog = flece_skill_progression_count;
        prog = flece_skill_progression;
        break;
    case SPECIES_ROSALIND:
        num_prog = rosalind_skill_progression_count;
        prog = rosalind_skill_progression;
        break;
    case SPECIES_JEKHAR:
        num_prog = jekhar_skill_progression_count;
        prog = jekhar_skill_progression;
        break;
    default:
        prog = NULL;
        num_prog = 0;
        break;
    }

    f.write_int(MP_PLAYER_FILE_MAGIC);
    f.write_int(MP_PLAYER_FILE_VERSION);
    f.write_string_z(name);
    f.write_int(char_info_index);
    f.write_int(0);                               // flags
    f.write_string_z(description);
    f.write_int16(1);                             // level
    f.write_int(0);                               // experience
    f.write_float((float)info->max_hp);
    f.write_int16(info->max_ap);
    f.write_int(0);                               // skill points

    for (i = 0; i < MP_NUM_EQUIP_SLOTS; i++) {
        if (i == 5)
            f.write_int(info->default_weapons[0]);
        else
            f.write_int(-1);
    }

    f.write_int(2);                               // party inventory: type, count, charges
    f.write_int(item_type_find("Health Tonic"));
    f.write_int(3);
    f.write_int(1);
    f.write_int(item_type_find("Book of Novices"));
    f.write_int(1);
    f.write_int(-1);
    f.write_int(30);                              // gold

    for (i = 0; i < MP_NUM_SAVED_SKILLS; i++) {
        if (i == SKILL_SPEED) {
            f.write_uint8(10);
        } else {
            for (j = 0; j < num_prog; j++)
                if (prog[j].skill == i)
                    break;
            if (j < num_prog && prog[j].level <= 1)
                f.write_uint8((ubyte)prog[j].value);
            else
                f.write_uint8(0);
        }
    }

    f.write_int(4);                               // chain skills: skill, points, slot
    f.write_int(info->up_skill);
    f.write_int(0);
    f.write_int(0);
    f.write_int(info->right_skill);
    f.write_int(0);
    f.write_int(3);
    f.write_int(info->down_skill);
    f.write_int(0);
    f.write_int(1);
    f.write_int(info->left_skill);
    f.write_int(0);
    f.write_int(2);

    for (i = 0; i < MP_NUM_HOT_BUTTONS; i++) {    // kind, id
        f.write_int(0);
        f.write_int(-1);
    }

    f.close();
    return 0;
}

// 0x46d0b0  Confidence: likely (the description: an existing file keeps its own, read back through
// mp_player_file_read_summary into a local summary; a new one gets L237). The flags word is the owner's
// mp_player +0x30; the HP written is the base plus the HP lent to a summon (Player +0x178).
int mp_player_file_write(Player *p)
{
    File f;
    char filename[64];
    mp_char_summary summary;
    int i;

    file_change_ext(filename, p->mp_owner->name, ".plr");
    if (f.find(filename, path_players))
        mp_player_file_read_summary(&summary, filename);
    else
        strcpy(summary.description, XSTR("No description", 237));

    if (file_make_path_dirs(path_players))
        return 2;
    if (f.open(filename, 2, path_players))
        return 3;

    Character *c = living_from_handle(p->handle);

    f.write_int(MP_PLAYER_FILE_MAGIC);
    f.write_int(MP_PLAYER_FILE_VERSION);
    f.write_string_z(p->mp_owner->name);
    f.write_int(p->char_info);
    f.write_int(p->mp_owner->profile_flags);
    f.write_string_z(summary.description);
    f.write_int16(c->level);
    f.write_int(c->xp);
    f.write_float(c->base_max_hp + p->lent_max_hp);
    f.write_int16(c->base_max_ap);
    f.write_int(p->skill_points);

    for (i = 0; i < MP_NUM_EQUIP_SLOTS; i++) {
        item_info *item = c->equipment[i];
        if (item)
            f.write_int(item->type - item_types);
        else
            f.write_int(-1);
    }

    f.write_int(party_inventory_count);
    for (i = 0; i < party_inventory_count; i++) {
        f.write_int(party_inventory[i].item->type - item_types);
        f.write_int(party_inventory[i].count);
        f.write_int(party_inventory[i].item->charges);
    }
    f.write_int(party_gold);

    for (i = 0; i < MP_NUM_SAVED_SKILLS; i++)
        f.write_uint8(c->skill[i]);

    int num_chain = chain_count_unlocked_skills(p);
    f.write_int(num_chain);
    for (i = 0; i < num_chain; i++) {
        int skill = chain_get_unlocked_skill(p, i);
        int points = chain_skill_get_points(p, skill);
        int slot = chain_config_find_skill(p, skill);
        f.write_int(skill);
        f.write_int(points);
        f.write_int(slot);
    }

    for (i = 0; i < MP_NUM_HOT_BUTTONS; i++) {
        int kind, id;
        if (savegame_state.hot_button_get(&kind, &id, c->info->species, i)) {
            f.write_int(kind);
            f.write_int(id);
        } else {
            f.write_int(0);
            f.write_int(-1);
        }
    }

    f.close();
    return 0;
}

// 0x46d450  Confidence: exact (VC6 byte match, D18900: the version-8 remap through an int local with `|=`;
// was likely) (the void* parameter: the header keeps the callers' spelling, see multisave.h). Version 8
// files had the flag bits one higher, except bit 0.
int mp_player_file_read_summary(void *out_summary, const char *filename)
{
    File f;
    mp_char_summary *out = (mp_char_summary *)out_summary;
    int version;

    if (f.open(filename, 1, path_players))
        return 2;
    if (f.read_int(0, 0) != MP_PLAYER_FILE_MAGIC) {
        f.close();
        return 3;
    }
    version = f.read_int(0, 0);
    if (version < 8) {
        f.close();
        return 4;
    }

    f.read_string(out->name, sizeof(out->name), 0, 0);
    out->char_info = f.read_int(0, 0);
    out->flags = f.read_int(0, 0);
    if (version < 9) {
        int fl = out->flags;
        fl = (fl >> 1) & ~3;
        fl |= out->flags & 1;
        out->flags = fl;
    }
    f.read_string(out->description, sizeof(out->description), 0, 0);
    out->level = f.read_int16(0, 0);
    f.close();
    return 0;
}

// 0x46d610  Confidence: exact (VC6 byte match, D18901: the version-8 remap as in D18900; was likely)
// (the slot loop: equip_item's two replaced items and unequip_slot's result share one local in the code). Loads the file into the party character c and its Player record; the
// name, char_info and description are read and dropped. HP and AP are refilled to the maximum. An empty
// weapon slot (5) keeps the current weapon.
int mp_player_file_load(Character *c, const char *filename)
{
    File f;
    char name[0x20];
    char description[0x100];
    int version, i, n;
    Player *p;

    p = player_from_handle(c->handle);
    if (!p)
        return 1;
    if (f.open(filename, 1, path_players))
        return 2;
    if (f.read_int(0, 0) != MP_PLAYER_FILE_MAGIC) {
        f.close();
        return 3;
    }
    version = f.read_int(0, 0);
    if (version < 8) {
        f.close();
        return 4;
    }

    f.read_string(name, sizeof(name), 0, 0);
    f.read_int(0, 0);                             // char_info
    p->mp_owner->profile_flags = f.read_int(0, 0);
    if (version < 9) {
        int fl = p->mp_owner->profile_flags;
        fl = (fl >> 1) & ~3;
        fl |= p->mp_owner->profile_flags & 1;
        p->mp_owner->profile_flags = fl;
    }
    f.read_string(description, sizeof(description), 0, 0);
    c->level = f.read_int16(0, 0);
    c->xp = f.read_int(0, 0);
    c->base_max_hp = f.read_float(0, 0);
    c->base_max_ap = f.read_int16(0, 0);
    p->skill_points = f.read_int(0, 0);

    for (i = 0; i < MP_NUM_EQUIP_SLOTS; i++) {
        int type = f.read_int(0, 0);
        item_info *old1, *old2;
        if (type >= 0) {
            character_equip_item(&old1, &old2, c, i, item_create(type));
            if (old1)
                item_free(old1);
            if (old2)
                item_free(old2);
        } else if (i != 5) {
            old1 = character_unequip_slot(c, i, 1);
            if (old1)
                item_free(old1);
        }
    }

    party_inventory_clear();
    n = f.read_int(0, 0);
    for (i = 0; i < n; i++) {
        int type = f.read_int(0, 0);
        int count = f.read_int(0, 0);
        int charges = f.read_int(0, 0);
        party_inventory_add_type(type, count, charges, 1);
    }
    party_gold = f.read_int(0, 0);

    for (i = 0; i < MP_NUM_SAVED_SKILLS; i++)
        c->skill[i] = f.read_uint8(0, 0);

    chain_config_reset_defaults();
    n = f.read_int(0, 0);
    for (i = 0; i < n; i++) {
        int skill = f.read_int(0, 0);
        int points = f.read_int(0, 0);
        int slot = f.read_int(0, 0);
        chain_skill_set_points(p, skill, points);
        if (slot != -1)
            character_set_chain_skill(p, slot, skill);
    }

    savegame_state.hot_buttons_clear_member(c->info->species);
    for (i = 0; i < MP_NUM_HOT_BUTTONS; i++) {
        int kind = f.read_int(0, 0);
        int id = f.read_int(0, 0);
        if (id >= 0)
            savegame_state.hot_button_set(kind, id, c->info->species, i);
    }

    f.close();
    c->hp = (float)c->get_max_stat(0);
    c->ap = (short)c->get_max_stat(1);
    return 0;
}

// 0x46da20  Confidence: exact. Five minutes (mp_level_start).
void mp_player_autosave_start()
{
    mp_player_autosave_timer.SetTimeout(300000);
}

// 0x46da30  Confidence: exact.
void mp_player_autosave_frame()
{
    if (mp_player_autosave_timer.elapsed()) {
        mp_player_file_write(mp_local_player->object);
        mp_player_autosave_timer.SetTimeout(300000);
    }
}

// 0x46da60  Confidence: exact.
void mp_player_save_on_leave()
{
    if (mp_local_player->object)
        mp_player_file_write(mp_local_player->object);
    mp_player_autosave_timer.invalidate();
}

// 0x46da80  Confidence: exact (VC6 byte match, match11; was likely) (the clear of each string before its strcpy is read as a separate memset
// of the field: rep stosd 0x40 each). The defaults, then multi.cfg (any path) may name the three servers.
// Bit 1 of the flags is cleared only when the file was read.
void pxo_read_config()
{
    TextFile tf;
    char buf[0x200];

    memset(&pxo_cfg, 0, sizeof(pxo_cfg));
    pxo_cfg.port = 4200;
    pxo_cfg._unk008 = 0;
    memset(pxo_cfg.utracker_server, 0, sizeof(pxo_cfg.utracker_server));
    strcpy(pxo_cfg.utracker_server, "ut.pxo.net");
    memset(pxo_cfg.gtracker_server, 0, sizeof(pxo_cfg.gtracker_server));
    strcpy(pxo_cfg.gtracker_server, "gt.pxo.com");
    memset(pxo_cfg.chat_server, 0, sizeof(pxo_cfg.chat_server));
    strcpy(pxo_cfg.chat_server, "chat.pxo.net");
    memset(pxo_cfg.newaccount_url, 0, sizeof(pxo_cfg.newaccount_url));
    strcpy(pxo_cfg.newaccount_url, "http://www.pxo.net/newaccount.cfm");
    memset(pxo_cfg.verify_url, 0, sizeof(pxo_cfg.verify_url));
    strcpy(pxo_cfg.verify_url, "http://www.pxo.net/verify.cfm");
    memset(pxo_cfg.banners_url, 0, sizeof(pxo_cfg.banners_url));
    strcpy(pxo_cfg.banners_url, "http://www.pxo.net/files/banners");

    if (!tf.Open("multi.cfg", PATH_ANY)) {
        if (tf.Accept("UserTracker:")) {
            tf.ParseString(buf, sizeof(buf), '"', '"');
            strcpy(pxo_cfg.utracker_server, buf);
        }
        if (tf.Accept("GameTracker:")) {
            tf.ParseString(buf, sizeof(buf), '"', '"');
            strcpy(pxo_cfg.gtracker_server, buf);
        }
        if (tf.Accept("ChatTracker:")) {
            tf.ParseString(buf, sizeof(buf), '"', '"');
            strcpy(pxo_cfg.chat_server, buf);
        }
        tf.Close();
        pxo_cfg.flags &= ~2;
    }
}
