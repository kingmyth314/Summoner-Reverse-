// summoner/multi/multi.h - the multiplayer game: players, session, packets, game list, PXO lobby, character
// select records; gathering header of the multi*.cpp units. Units (strings.tbl / sum.symbols):
// the units with headers of their own, included at the end: multiplayerdata.cpp (0x4af340-0x4afc9f,
// multiplayerdata.h; decomp47), multiendgame.cpp (0x4811a0-0x481673, multiendgame.h; decomp47), multisave.cpp (0x46cc60-0x46dd2f, multisave.h), multitrade.cpp (0x470e30-0x471def,
// multitrade.h), multichat.cpp (0x487b90-0x488223, multichat.h), multicontainer.cpp (0x488c00-0x48972a,
// multicontainer.h), multispell.cpp (0x4afca0-0x4b0957, multispell.h), multi_chattracker.cpp
// (0x4abb80-0x4ae32d, multi_chattracker.h; decomp35); and four units whose file names are not known
// (placeholder headers, decomp53): the multiplayer menu states (0x43eb00-0x43ef20,
// unnamed_mpmenus_0x43eb00.h), the PXO lobby (0x43ef30-0x43f9cc, unnamed_lobby_0x43ef30.h), the net session
// (0x440b40-0x441817, unnamed_netsession_0x440b40.h) and the packet layer (0x441820-0x4425a6,
// unnamed_netpackets_0x441820.h). Directory and file name are ours. Not original source.
#ifndef SUMMONER_MULTI_MULTI_H
#define SUMMONER_MULTI_MULTI_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"

class Player;
class Character;

// A multiplayer player record (0xbc bytes). The list is circular through +0x00, head mp_player_list (a
// whole record). Merged from three stub copies (DECISIONS.md D10); +0x18 and +0x30 named in decomp24 (D402).
// decomp53 (D3302): the size and the GameTimers +0x24 / +0x68 from the net session unit's initialisers
// (init_gametimer_array_651604 constructs +0x24, +0x64, +0x68 of each of the four mp_players with a 0xbc
// stride; init_gametimers_65143c / _6514fc do the same for the two list heads).
struct mp_player {
    mp_player *next;                  // +0x00 mp_player_list
    mp_player *prev;                  // +0x04 (inferred: list pair)
    ubyte addr[0x08];                 // +0x08 net address (mp_player_find_by_addr, nw_addr_equal; mp_handle_net_info
                                      //       copies mp_server_addr here for the server's record; decomp47)
    unsigned int flags;               // +0x10 bit 0 host, MP_PF_* below
    int type;                         // +0x14 state: 3 listed on the level select screen, 4..9 loading steps
                                      //       (6 linking players, 9 ready; mp_draw_loading_status)
    int socket;                       // +0x18 reliable socket, -1 not connected (mp_broadcast_packet skips
                                      //       those; chat_cmd_ping passes it to net_get_reliable_socket_ping)
    int id;                           // +0x1c player id (hud_confirm_quit passes it to mp_respawn_and_notify);
                                      //       also the index of its send buffers (multi_send_packet*; decomp53)
    int _unk20;                       // +0x20
    GameTimer join_timer;             // +0x24 SetTimeout by mp_accept_join_request (decomp53, name ours)
    Player *object;                   // +0x28 the party record (its +0xc handle is read; party_remove_all clears it)
    int char_info;                    // +0x2c char_infos index (level_spawn_party passes it to party_member_join; decomp42)
    int profile_flags;                // +0x30 the .plr flags word (mp_char_record +0x4c): written and read by
                                      //       mp_player_file_write / mp_player_file_load; bits 26.. the completed
                                      //       difficulties (mp_player_level_available; decomp53)
    char name[0x20];                  // +0x34 copied from the character profile by mp_local_player_create
    char display_name[0x10];          // +0x54 name cut to 50 pixels with "..." (gr_string_truncate_ellipsis in
                                      //       mp_add_new_player; decomp47, name ours)
    GameTimer move_update_timer;      // +0x64 100 ms OBJ_UPD resend while traveling (ai_frame; decomp39, name ours)
    GameTimer swi_timer;              // +0x68 (mp_handle_swi_2dir; decomp53, name ours)
    ubyte _unk6c[0xbc - 0x6c];
};
LAYOUT_CHECK(mp_player, sizeof(mp_player) == 0xbc);
#define MP_MAX_PLAYERS 4              // mp_players[] and the send buffers (decomp53)
// mp_player::flags bits that ai_frame keeps for the party member's player (decomp39; names ours).
#define MP_PF_TRAVELING      0x8      // traveling sub-mode 1 this frame
#define MP_PF_WAS_TRAVELING  0x10     // ... the frame before
// decomp47 (multiplayerdata.cpp, multiendgame.cpp, new_multi.cpp; names ours).
#define MP_PF_DATA_RECEIVED  0x4      // PLY_DAT handled for this player (mp_handle_player_data)
#define MP_PF_TRACKED        0x20     // the host's game is on the PXO game tracker (tracker_end_game)
// NEW_INF broadcast (mp_broadcast_new_info); the next mp_broadcast_packet with skip_flagged clears it
// instead of sending to this player (decomp53).
#define MP_PF_INFO_SENT      0x40000000
#define MP_PF_LEAVING        0x80000000   // mp_player_disconnected ran (ui_screen_stop_player tests it)
// decomp53 (net session): the host announced the player (mp_host_update_session, session state 1); a
// client sets it on itself in mp_handle_gm_stat. Name ours.
#define MP_PF_ANNOUNCED      0x2

// The AI's multiplayer senders (module "-", 0x480070-0x488b00; decomp39). Argument lists from ai.cpp.
void mp_send_new_path(Character *c);                  // 0x480070 NEW_PTH
void mp_send_obj_update(Character *c, int a);         // 0x480530 OBJ_UPD (ai_frame passes 0)
void mp_broadcast_ai_disable(Character *c, int kind, int ms);   // 0x480eb0 kind 0 knock-down, 1 push
// decomp56 (AI combat unit): packet 0x1b (the character handle and a byte), sent by the AI move goals.
void mp_send_character_walk(Character *c, int a);    // 0x488230
void mp_send_swi_2dir();                              // 0x4884f0 SWI2DIR (cur_character)
void mp_send_swi_ndir();                              // 0x488700 SWI!DIR
void mp_send_swi_hide(Character *c);                  // 0x488b00 SWI_HID

// The pilots read these fields through macros while the stub record was short (kept, DECISIONS.md D10).
#define MP_PLAYER_NAME(p) ((p)->name)
#define MP_PLAYER_NEXT(p) ((p)->next)
#define MP_PLAYER_TYPE(p) ((p)->type)
#define MP_PLAYER_STATE(p) ((p)->type)  // new_hud.cpp's name for +0x14 (D50)
#define MP_PLAYER_ID(p)    ((p)->id)

// The host test every sender and handler makes: multiplayer (game_flags bit 1, summoner/summoner.h), a
// local player, and its host bit, in this order. Macro names ours (D403).
#define MP_IS_HOST()   ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
#define MP_IS_CLIENT() ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1))

// ---- packets (mp_packet_table, docs/sum-exe/multiplayer.md) -----------------------------------------
// A packet is {ubyte type, ushort size of the data, data[size]}; the handler gets a pointer to the data.
// The senders build it in a 0x200-byte local buffer with a ushort size counter (the variable-length
// senders keep it in a register: mp_send_container_close, container_spawn_loot, chat_build_packet) and
// bump the send counter of the type. Type names from the table; the macros are ours (D403). They expect
// `ubyte data[MP_MAX_PACKET]` and `ushort packet_size` in the sender.
#define MP_HEADER_SIZE 3
#define MP_MAX_PACKET  0x200
#define MP_NUM_PACKET_TYPES 66        // mp_packet_table entries (decomp53)

#define MP_NET_INF 0x05
#define MP_NEW_INF 0x06
#define MP_PLY_DAT 0x07
#define MP_GM_OVER 0x08
#define MP_LV_GAME 0x09
#define MP_GM_STAT 0x0a               // decomp53 (multi_set_netgame_state)
#define MP_CT_STAT 0x0b               // decomp53 (multi_process_client_status)
#define MP_CHAT_LN 0x0c
#define MP_ITM_DRP 0x23
#define MP_ITM_GET 0x24
#define MP_CNT_OPN 0x25
#define MP_CNT_CLS 0x26
#define MP_CNT_PCK 0x27
#define MP_DOOR_TG 0x28
#define MP_DOR_PCK 0x29
#define MP_IT_RQST 0x2a
#define MP_IT_DENY 0x2b
#define MP_IT_ACPT 0x2c
#define MP_IT_ADDI 0x2d
#define MP_IT_RMVI 0x2e
#define MP_IT_OKAY 0x2f
#define MP_IT_CNCL 0x30
#define MP_IT_QURY 0x31
#define MP_IT_STAT 0x32
#define MP_CST_ANM 0x34
#define MP_RLS_SPL 0x35
#define MP_SMN_RQS 0x36
#define MP_SMN_DNY 0x37
#define MP_SMN_ALD 0x38
#define MP_SMN_HND 0x39
#define MP_SMN_QCK 0x3a

// One packet type (0x1c bytes, symbols/types.h mp_packet_info). decomp53 (D3303): multi_process_packet
// compares +0x08 as a byte (`cmp byte ptr [ebp + 0x5951c0], 1`), so the flag is a byte; it lets the
// handler run for a sender that is not in the player list. +0x18 / +0x19 are the net-rate flags of the
// compiled-out rate display (cmd_disp_rate): [0] the type has rate info (listed by the help), [1] its
// display is on (toggled). Both are set only for SWI_PTH..SWI_BSS ([1] also) and SWI_SNK / SWI_HID.
struct mp_packet_info {
    int type;                                         // +0x00
    void (*handler)(ubyte *data, void *from);         // +0x04 (data, sender address)
    ubyte flag;                                       // +0x08
    ubyte _pad09[3];
    int sent;                                         // +0x0c send counter, bumped by the senders
    char name[8];                                     // +0x10 "GM_QURY" ...
    ubyte rate_flags[4];                              // +0x18
};
LAYOUT_CHECK(mp_packet_info, sizeof(mp_packet_info) == 0x1c);
// mp_packet_table (0x5951b8) and the send / forward functions are in unnamed_netpackets_0x441820.h.

#define MP_BUILD_HEADER(t) (data[0] = (ubyte)(t), packet_size = MP_HEADER_SIZE, mp_packet_table[t].sent++)
#define MP_ADD_DATA(v)     (memcpy(data + packet_size, &(v), sizeof(v)), packet_size += sizeof(v))
#define MP_END_PACKET()    (*(ushort *)(data + 1) = (ushort)(packet_size - MP_HEADER_SIZE))
// A float goes through a stack temporary (mp_send_player_data 0x4af8c6: `mov [esp+8], edx / mov esi,
// [esp+8]` before the store into the packet; decomp47, macro ours).
#define MP_ADD_FLOAT(v)    do { float mp_tmp_f = (v); memcpy(data + packet_size, &mp_tmp_f, 4); \
                                packet_size += 4; } while (0)

int  mp_handle_to_net(int handle);                                         // 0x4752c0
int  mp_local_handle_from_net(int net_handle);                             // 0x475280
obj *mp_obj_from_net_handle(int net_handle);                               // 0x475240
void mp_map_net_handle(int net_handle, int handle);                        // 0x475300 (return unused)
mp_player *mp_player_find_by_id(int id);                                   // 0x4751d0
mp_player *mp_player_find_by_name(const char *name);                       // 0x475200 (stricmp)
// Three 16-bit values scaled to the level bounds; the position is passed by value (last).
void mp_quantize_position(short *x, short *y, short *z, vector3 pos);      // 0x475660
// decomp58: the three values are unsigned (`and eax, 0xffff` before fild at 0x4756f7; was short).
vector3 mp_dequantize_position(ushort x, ushort y, ushort z);              // 0x4756e0 (hidden result)
// An orientation packed for NEW_LEP (0x14 bytes; type and field names ours, decomp58): two terms of the
// first two rows and the signs of five dropped terms (bits 0..4: m[2], m[5], m[8], m[6], m[7] < 0).
struct mp_packed_orient {
    float r0, r1;                     // +0x00 rvec.x, rvec.y
    float u0, u1;                     // +0x08 uvec.x, uvec.y (the packer stores rvec.x, rvec.y again: a bug)
    ubyte signs;                      // +0x10
    ubyte _pad11[3];
};
LAYOUT_CHECK(mp_packed_orient, sizeof(mp_packed_orient) == 0x14);
void matrix3_pack_orientation(mp_packed_orient *out, const matrix *m);     // 0x475380 (mp_send_new_lep)
void matrix3_unpack_orientation(const mp_packed_orient *in, matrix *out);  // 0x475430 (mp_handle_new_lep)
#define MP_NEW_MON 0x3e               // decomp58
#define MP_SRV_REQ 0x3f               // decomp58
#define MP_INV_EQP 0x33               // decomp63 (mp_send_equip_item; name from mp_packet_table)
#define MP_INV_UQP 0x41               // decomp63 (mp_send_unequip_item)
// decomp53: the rest of the handle unit's calls the session and packet units make. (address, check_port):
// the list entry whose +0x08 address matches (nw_addr_equal), NULL.
mp_player *mp_player_find_by_addr(const void *addr, bool check_port);       // 0x475180
bool mp_all_players_have_status(int status);                               // 0x475350 every list entry's +0x14
void mp_broadcast_packet_3e();                                             // 0x4755d0 NEW_MON (monster_layout_load)

// ---- session -----------------------------------------------------------------------------------
// The session state machine, the player records and the netgame block are in
// unnamed_netsession_0x440b40.h (decomp53); mp_host_game and the level checks in unnamed_mpmenus_0x43eb00.h.
void tracker_end_game();                                // 0x4aba20 (mp_leave_game, host of a tracked game)
void mp_remove_player(mp_player *p, int reason);        // 0x471df0 (reason 1 = kicked)
void mp_respawn_and_notify(int id);                     // 0x481130 (hud_confirm_quit)
// mp_player_disconnected (0x4814e0) is in multiendgame.h (decomp47: (reason, quit), D2601).
void mp_send_character_skills(Character *c);            // 0x480ba0
void mp_change_level(int entry_point);                  // 0x4ab780 (return unused)
// The host's broadcasts of living_entity.cpp (decomp37; argument lists from its calls).
void mp_broadcast_hp(Character *c, int source);         // 0x480920
void mp_broadcast_ap(Character *c, short ap);           // 0x480a10
void mp_broadcast_xp(Character *c, int xp);             // 0x480af0
void mp_broadcast_status_add(Character *c, int status, int ms, int spell, float value, int source);   // 0x480cc0
void mp_broadcast_status_cure(Character *c, int status, bool restore_fx);   // 0x480de0

// decomp53: the calls of other unwritten multiplayer units the session, lobby and menu units make.
// The object unit 0x47f9a0-0x480000.
void mp_net_handle_map_reset();                         // 0x47f9a0 the 1000-entry map 0x22e94f4 = -1
// decomp58: net handle index -> local handle (a client's map; the .bss lies after spells.cpp's, so it is
// not the handle unit's; which unit defines it is open).
#define MP_NET_HANDLE_MAP_SIZE 1000
extern int mp_net_handle_map[MP_NET_HANDLE_MAP_SIZE];   // 0x22e94f4
void mp_send_local_updates();                           // 0x47f9c0 OBJ_UPD of the locally controlled characters
void mp_send_monster_list();                            // 0x47fb20 host: OBJ_HND of every type 4 / 5 / 9 object
void mp_send_new_lep(Character *c);                     // 0x47fdf0 NEW_LEP
// The join / tracker unit 0x4ab0f0-0x4abb7f (before multi_chattracker.cpp).
void mp_join_timer_reset();                             // 0x4ab110 (state14_enter)
void chat_history_pump();                               // 0x4ab130 (state14_frame)
void mp_unpause();                                      // 0x4ab920 jmp game_unpause
void tracker_init();                                    // 0x4ab930 once
void tracker_start_game();                              // 0x4ab960 (mp_host_game)
void tracker_frame();                                   // 0x4abb50 (multi_do_frame)
// The pending join: the player mp_accept_join_request took (cleared by mp_handle_ct_stat on status 3 or by
// mp_server_check_clients when the timer runs out) and its timer. Names ours.
extern mp_player *mp_join_accept_player;                // 0x24794bc
extern RealtimeTimer mp_join_accept_timer;              // 0x24794c0
// The lobby channel the hosted game is listed under, and the game list filter; both set from the joined
// channel by state_mp_lobby_leave (tracker_start_game, tracker_request_games). Names ours.
extern char tracker_game_channel[0x100];                // 0x2479520
extern char tracker_filter_channel[0x100];              // 0x2479620
// One gtrack channel query (0x28 bytes on the stack of lobby_query_channel; gtrack_send_channel_query
// sends 0x24 bytes from +4). Type and field names ours.
struct gtrack_channel_query {
    int unk00;                        // +0x00 0
    char channel[0x24];               // +0x04
};
LAYOUT_CHECK(gtrack_channel_query, sizeof(gtrack_channel_query) == 0x28);
void gtrack_send_channel_query(gtrack_channel_query *q);   // 0x4ae830
// multispell's neighbour 0x4b0b50-0x4b11a0 (level completion).
void mp_level_find_boss();                              // 0x4b0b50 (mp_level_start, host)
void mp_level_complete_check();                         // 0x4b1150 (mp_game_frame, host)

// The packet handlers of mp_packet_table that no written unit declares yet (decomp53; all (data, sender)).
void mp_handle_game_query(ubyte *data, void *from);           // 0x4ab1c0 GM_QURY
void mp_handle_actv_gm(ubyte *data, void *from);              // 0x470860 ACTV_GM
void mp_handle_join_request(ubyte *data, void *from);         // 0x4ab350 JN_RQST
void mp_handle_acpt_jn(ubyte *data, void *from);              // 0x470a80 ACPT_JN
void mp_handle_deny_jn(ubyte *data, void *from);              // 0x470b60 DENY_JN
void mp_handle_obj_hnd(ubyte *data, void *from);              // 0x47fcc0 OBJ_HND
void mp_handle_new_lep(ubyte *data, void *from);              // 0x47ff10 NEW_LEP
void mp_handle_delete_entity(ubyte *data, void *from);        // 0x480000 DEL_LEP
void mp_handle_new_path(ubyte *data, void *from);             // 0x4801f0 NEW_PTH
void mp_handle_obj_update(ubyte *data, void *from);           // 0x4806a0 OBJ_UPD
void mp_handle_adjust_hp(ubyte *data, void *from);            // 0x4809b0 ADJ_HPS
void mp_handle_adjust_ap(ubyte *data, void *from);            // 0x480a90 ADJ_APS
void mp_handle_adjust_xp(ubyte *data, void *from);            // 0x480b60 ADJ_EXP
void mp_handle_character_skills(ubyte *data, void *from);     // 0x480c60 ADJ_SKL
void mp_handle_status_add(ubyte *data, void *from);           // 0x480d70 SET_STS
void mp_handle_status_cure(ubyte *data, void *from);          // 0x480e60 CLR_STA
void mp_handle_ai_disable(ubyte *data, void *from);           // 0x480f60 DISABLD
void mp_handle_respawn(ubyte *data, void *from);              // 0x481050 RESTART
void mp_handle_forcepr(ubyte *data, void *from);              // 0x481150 FORCEPR
void mp_handle_swi_path(ubyte *data, void *from);             // 0x4882f0 SWI_PTH
void mp_handle_swi_attack(ubyte *data, void *from);           // 0x488450 SWI_ATK
void mp_handle_swi_2dir(ubyte *data, void *from);             // 0x4885f0 SWI2DIR
void mp_handle_swi_ndir(ubyte *data, void *from);             // 0x4887e0 SWI!DIR
void mp_handle_swi_boss(ubyte *data, void *from);             // 0x4889c0 SWI_BSS
// decomp59 (the Machival and Luminar teleports): (boss, ai mode, sub-mode) SWI_BSS sender (packet 0x1f).
void mp_send_swi_boss(Character *boss, int mode, int submode);   // 0x4888d0
void mp_handle_swi_sneak(ubyte *data, void *from);            // 0x488ab0 SWI_SNK
void mp_handle_swi_hide(ubyte *data, void *from);             // 0x488ba0 SWI_HID
void mp_handle_combat_sequence(ubyte *data, void *from);      // 0x4b1280 COM_SEQ
void mp_handle_equip_item(ubyte *data, void *from);           // 0x487950 INV_EQP
void mp_handle_kick_pl(ubyte *data, void *from);              // 0x471f90 KICK_PL
void mp_handle_level_complete(ubyte *data, void *from);       // 0x4b09c0 L_CMPLT
void mp_handle_difficulty_complete(ubyte *data, void *from);  // 0x4b0ab0 D_CMPLT
void mp_handle_new_mon(ubyte *data, void *from);              // 0x475610 NEW_MON
void mp_handle_srv_req(ubyte *data, void *from);              // 0x4767a0 SRV_REQ
void mp_handle_unequip_item(ubyte *data, void *from);         // 0x487b10 INV_UQP

// ---- game list (the code after the session functions, 0x4705f0-0x470e20) ------------------------------
// One received game (0x50 bytes; 100 of them in mp_gamelist_entries, 0x22e4568). Filled by
// mp_handle_actv_gm (0x4709f9-0x470a6e); the type and field names are ours. Both list heads are whole
// records (mp_gamelist_used 0x22e4570 is followed 0x50 bytes later by mp_join_retry_timer).
struct mp_gamelist_entry {
    mp_gamelist_entry *next;          // +0x00 circular through mp_gamelist_used
    mp_gamelist_entry *prev;          // +0x04
    char name[0x20];                  // +0x08 game name
    int  level;                       // +0x28 level index (level_table_display_name)
    ubyte num_players;                // +0x2c
    ubyte max_players;                // +0x2d
    ubyte _pad2e[2];
    int  difficulty;                  // +0x30 0 easy, 1 medium, 2 hard
    ubyte min_level;                  // +0x34 character level range of the game
    ubyte max_level;                  // +0x35
    ubyte _pad36[2];
    ubyte addr[8];                    // +0x38 server address (nw_addr_equal)
    int  ping;                        // +0x40 ms
    int  last_seen;                   // +0x44 timer_get
    int  status;                      // +0x48 1 = open (joinable), else drawn red
    int  connection;                  // +0x4c 1 below 56k, 2 56k, 3 ISDN or faster
};
LAYOUT_CHECK(mp_gamelist_entry, sizeof(mp_gamelist_entry) == 0x50);
extern mp_gamelist_entry mp_gamelist_used;              // 0x22e4570 head of the received games
extern int mp_gamelist_count;                           // 0x22e4618
void mp_gamelist_request_refresh();                     // 0x470bb0
// (game, the 0x12c-byte character record from its name (profile + 0x28), password, flag); 1 from the
// game list, the retry in mp_gamelist_refresh passes its own. Return unused.
void mp_send_jn_rqst(mp_gamelist_entry *game, const void *character, const char *password, int flag);   // 0x470bc0
// mp_join_result (0x22e4428): 0 none, 1 request sent (waiting), 2 accepted, 3..13 the deny codes
// (4 = password needed / wrong), 0xd no answer.
int  mp_join_get_result();                              // 0x470e10
void mp_join_clear_result();                            // 0x470e20
// decomp53 (the game list state 12).
void mp_gamelist_net_frame();                           // 0x4705f0 nw_receive_all, jmp mp_gamelist_refresh
void mp_gamelist_init();                                // 0x470790
void mp_gamelist_free();                                // 0x470820

// ---- PXO login (pxo_login_* 0x42f280; unit not known). The lobby itself (lobby_channel, lobby_*) is in
// unnamed_lobby_0x43ef30.h (decomp53). -------------------------------------------------------------------
void pxo_login_reset();                                 // 0x42f280
// 0 logged in, 1 no account, 2 cannot contact the server, 3 validating (busy), 4 invalid user name.
int  pxo_login_step();                                  // 0x42f290
// The login reply (pxo_read_login zeroes it and passes it to pxo_login_request); state_mp_lobby_frame
// sends it with the character name as the IRC user info ("%s %s"). decomp53.
extern char pxo_login_reply[12];                        // 0x5f96ac

// ---- the character select records ---------------------------------------------------------------------
// The character record of the character select list (mp_char_profiles 0x5b6e78, 0x154 bytes). The stubs'
// mp_char_profile (flags only) and mp_char_record are one type (DECISIONS.md D11). From +0x28 on it is
// the .plr summary (mp_char_summary, multisave.h; D400).
struct mp_char_record {
    char filename[0x28];              // +0x000 from file_find_files
    char name[0x20];                  // +0x028 mp_player_file_read_summary fills from here
    int  char_info;                   // +0x048 char_infos index
    unsigned int flags;               // +0x04c 0x4000000 / 0x8000000 unlock difficulty 1 / 2 (screen 17)
    char description[0x100];          // +0x050 the text typed on screen 16
    short level;                      // +0x150
    short _pad152;
};
LAYOUT_CHECK(mp_char_record, sizeof(mp_char_record) == 0x154);
typedef mp_char_record mp_char_profile;

void mp_send_equip_item(Character *c, int slot, int type);   // 0x4878a0 packet 0x33 (return unused)
void mp_send_unequip_item(Character *c, int slot);           // 0x487a70 packet 0x41 (return unused)
// multi_door.cpp (0x4aa780-0x4aac05, strings.tbl id 246; decomp49): the host's door changes. A level object
// is named by its index in lobjs ("mesh id"). (player id, lobj index, open): the host's toggle
// (object_try_unlock_or_open) or a client's service request 4 / 5 (mp_handle_srv_req).
void door_find_by_mesh(int player_id, int lobj_index, int open);   // 0x4aa8e0 (return unused)
void mp_handle_door_toggle(ubyte *data, void *from);               // 0x4aa780 DOOR_TG (lobj index, open)
// DOR_PCK (player id, lobj index, result 0 picked / 1 failed / 2 not locked).
void mp_handle_door_lockpick(ubyte *data, void *from);             // 0x4aa7d0
void mp_send_door_toggle(int lobj_index, int open);                // 0x4aa9c0 broadcast
// (player id, lobj index): a client's service request 7 (mp_handle_srv_req), the host rolls the pick.
void door_pick_by_mesh(int player_id, int lobj_index);             // 0x4aaa10
void mp_send_door_lockpick(mp_player *p, int lobj_index, int result);   // 0x4aab90

// decomp53: the four placeholder unit headers (file names not known).
#include "summoner/multi/unnamed_mpmenus_0x43eb00.h"
#include "summoner/multi/unnamed_lobby_0x43ef30.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_utrack_0x48d7c0.h"     // decomp64

#include "summoner/multi/multiplayerdata.h"
#include "summoner/multi/multiendgame.h"
#include "summoner/multi/multisave.h"
#include "summoner/multi/multitrade.h"
#include "summoner/multi/multichat.h"
#include "summoner/multi/multicontainer.h"
#include "summoner/multi/multispell.h"
#include "summoner/multi/multi_chattracker.h"
// decomp23 (combat.cpp, spells.cpp): the unnamed multiplayer units (mp_spell_cast_sync and
// mp_summon_claim_release moved to multispell.h, decomp35).
// Packet 0x22 of a living_combat exchange, the arguments as living_combat_send_net pushes them:
// (attacker net handle, defender net handle, move variant, defence move, state argument, skill, special).
void mp_send_living_combat(int attacker, int defender, int variant, void *defence, int arg, int skill, int special);  // 0x4b11a0

#endif
