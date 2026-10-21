// summoner/multi/multispell.h - multispell.cpp (0x4afca0-0x4b0957: spell casts and the one-summon-per-game
// rule of a network game; packets 0x34-0x3a CST_ANM .. SMN_QCK; strings.tbl id 238). File name from the
// module column of sum.symbols, directory ours. Included by summoner/multi/multi.h. Not original source.
#ifndef SUMMONER_MULTI_MULTISPELL_H
#define SUMMONER_MULTI_MULTISPELL_H

#include "vsdk/vsdk.h"

class Character;
struct spell_cast_request;

// The summon claim (.bss 0x247e824-0x247e82f; names ours): the id of the player whose summon is out (-1
// none; the host decides), the summon's net handle (-1 none) and our pending SMN_RQS.
extern int mp_summon_owner;                 // 0x247e824
extern int mp_summon_net_handle;            // 0x247e828
extern int mp_summon_request_pending;       // 0x247e82c

// The extra fields of a cast, appended to RLS_SPL (offset: the packet size counter / read position).
void mp_write_spell_target_point(spell_cast_request *req, ubyte *data, ushort *offset);          // 0x4afca0
void mp_read_spell_target_point(spell_cast_request *req, ubyte *data, int *offset, int spell);   // 0x4afd10
void mp_write_spell_targets(spell_cast_request *req, ubyte *data, ushort *offset);               // 0x4afd60
void mp_read_spell_targets(spell_cast_request *req, ubyte *data, int *offset, int spell);        // 0x4affb0 (spell unused, D21603)

void mp_send_summon_deny(int player_id);                     // 0x4b0130 SMN_DNY to that player
void mp_send_cast_anim(Character *caster, int spell, bool flag);   // 0x4b0180 CST_ANM (ai_cast_spell_update)
void mp_handle_cast_anim(ubyte *data, void *from);           // 0x4b0210
// Between spell_prepare_cast and spell_cast_launch_effects: sends the cast as RLS_SPL.
void mp_spell_cast_sync(spell_cast_request *req);            // 0x4b02a0
void mp_handle_release_spell(ubyte *data, void *from);       // 0x4b04a0
void mp_handle_summon_deny(ubyte *data, void *from);         // 0x4b0640
void mp_broadcast_summon_allowed(int player_id);             // 0x4b0660 SMN_ALD
void mp_handle_summon_allowed(ubyte *data, void *from);      // 0x4b06a0
void mp_send_summon_request();                               // 0x4b06b0 SMN_RQS
void mp_handle_summon_request(ubyte *data, void *from);      // 0x4b0700 (host)
void mp_summon_claim_reset();                                // 0x4b0730 (mp_level_start)
// 1 when the local player may summon now (ai_cast_spell_update); 0 asks the host or refuses.
int  mp_summon_try_claim();                                  // 0x4b0750
void mp_broadcast_summon_handle(int net_handle);             // 0x4b07d0 SMN_HND (summon_create)
void mp_handle_summon_handle(ubyte *data, void *from);       // 0x4b0820
void mp_summon_map_handle(int handle);                       // 0x4b0850 (summon_create)
void mp_send_summon_quick(int handle);                       // 0x4b0870 SMN_QCK: the summon is dismissed
void mp_handle_summon_quick(ubyte *data, void *from);        // 0x4b08f0
void mp_summon_claim_release();                              // 0x4b0940 (character_on_death, summon_dismiss)

#endif
