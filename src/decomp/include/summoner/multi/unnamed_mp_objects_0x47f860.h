// summoner/multi/unnamed_mp_objects_0x47f860.h - the network game's object packets 0x47f860-0x481196
// (OBJ_HND, NEW_LEP, DEL_LEP, NEW_PTH, OBJ_UPD, ADJ_HPS / APS / EXP / SKL, SET_STS, CLR_STA, DISABLD,
// RESTART, FORCEPR). Source game/unnamed_mp_objects_0x47f860.cpp (decomp76). THE FILE NAME IS A PLACEHOLDER;
// the code runs straight into multiendgame.cpp (0x4811a0) and reads its multi_path_type, and the handle map
// lies right after multiendgame's console command in .bss, so it may be that file's head (D5810). The
// functions multi.h declared stay there. Not original source.
#ifndef SUMMONER_MULTI_UNNAMED_MP_OBJECTS_0X47F860_H
#define SUMMONER_MULTI_UNNAMED_MP_OBJECTS_0X47F860_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "summoner/multi/multi.h"

// Packet types (mp_packet_table entries; names from the table).
#define MP_OBJ_HND 0x0d
#define MP_NEW_LEP 0x0e
#define MP_DEL_LEP 0x0f
#define MP_NEW_PTH 0x10
#define MP_OBJ_UPD 0x11
#define MP_ADJ_HPS 0x12
#define MP_ADJ_APS 0x13
#define MP_ADJ_EXP 0x14
#define MP_ADJ_SKL 0x15
#define MP_SET_STS 0x16
#define MP_CLR_STA 0x17
#define MP_DISABLD 0x18
#define MP_RESTART 0x19
#define MP_FORCEPR 0x1a

#define MP_NUM_SKILL_BYTES 0x24       // the skill bytes ADJ_SKL carries (Character +0x614)
#define MP_TRAVEL_SAMPLES 16          // mp_player::swi_samples

// (player, the position just received): a traveling party member's AI walks there when the time since
// the last update, over the mean of the sampled update intervals, is positive.
void mp_remote_travel_update(mp_player *p, const vector3 *pos);   // 0x47f860
void mp_send_respawn(int id);                           // 0x480fd0 RESTART
void mp_respawn_player(int id);                         // 0x481090

#endif
