// summoner/multi/multi.h - the multiplayer game: players, session, packets, game list, PXO lobby, character
// select records; gathering header of the multi*.cpp units. Units (strings.tbl / sum.symbols):
// the units with headers of their own, included at the end: multiplayerdata.cpp (0x4af340-0x4afc9f,
// multiplayerdata.h; decomp47), multiendgame.cpp (0x4811a0-0x481673, multiendgame.h; decomp47), multisave.cpp (0x46cc60-0x46dd2f, multisave.h), multitrade.cpp (0x470e30-0x471def,
// multitrade.h), multichat.cpp (0x487b90-0x488223, multichat.h), multicontainer.cpp (0x488c00-0x48972a,
// multicontainer.h), multispell.cpp (0x4afca0-0x4b0957, multispell.h), multi_chattracker.cpp
// (0x4abb80-0x4ae32d, multi_chattracker.h; decomp35). Directory and file name are ours. Not original source.
#ifndef SUMMONER_MULTI_MULTI_H
#define SUMMONER_MULTI_MULTI_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"

class Player;
class Character;

// A multiplayer player record (partial). The list is circular through +0x00, head mp_player_list (a
// whole record). Merged from three stub copies (DECISIONS.md D10); +0x18 and +0x30 named in decomp24 (D402).
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
    int id;                           // +0x1c player id (hud_confirm_quit passes it to mp_respawn_and_notify)
    ubyte _unk20[0x08];
    Player *object;                   // +0x28 the party record (its +0xc handle is read; party_remove_all clears it)
    int char_info;                    // +0x2c char_infos index (level_spawn_party passes it to party_member_join; decomp42)
    int profile_flags;                // +0x30 the .plr flags word (mp_char_record +0x4c): written and read by
                                      //       mp_player_file_write / mp_player_file_load
    char name[0x20];                  // +0x34 copied from the character profile by mp_local_player_create
    char display_name[0x10];          // +0x54 name cut to 50 pixels with "..." (gr_string_truncate_ellipsis in
                                      //       mp_add_new_player; decomp47, name ours)
    GameTimer move_update_timer;      // +0x64 100 ms OBJ_UPD resend while traveling (ai_frame; decomp39, name ours)
};
extern mp_player mp_player_list;      // 0x651418 head
extern mp_player *mp_local_player;    // 0x651598
// mp_player::flags bits that ai_frame keeps for the party member's player (decomp39; names ours).
#define MP_PF_TRAVELING      0x8      // traveling sub-mode 1 this frame
#define MP_PF_WAS_TRAVELING  0x10     // ... the frame before
// decomp47 (multiplayerdata.cpp, multiendgame.cpp, new_multi.cpp; names ours).
#define MP_PF_DATA_RECEIVED  0x4      // PLY_DAT handled for this player (mp_handle_player_data)
#define MP_PF_TRACKED        0x20     // the host's game is on the PXO game tracker (tracker_end_game)
#define MP_PF_INFO_SENT      0x40000000   // NEW_INF broadcast (mp_broadcast_new_info)
#define MP_PF_LEAVING        0x80000000   // mp_player_disconnected ran (ui_screen_stop_player tests it)

// The AI's multiplayer senders (module "-", 0x480070-0x488b00; decomp39). Argument lists from ai.cpp.
void mp_send_new_path(Character *c);                  // 0x480070 NEW_PTH
void mp_send_obj_update(Character *c, int a);         // 0x480530 OBJ_UPD (ai_frame passes 0)
void mp_broadcast_ai_disable(Character *c, int kind, int ms);   // 0x480eb0 kind 0 knock-down, 1 push
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

#define MP_NET_INF 0x05
#define MP_NEW_INF 0x06
#define MP_PLY_DAT 0x07
#define MP_GM_OVER 0x08
#define MP_LV_GAME 0x09
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

// One packet type (0x1c bytes, symbols/types.h mp_packet_info). multi_process_packet reads +0x08 as a
// byte (handled even before the session is up) and the two bytes +0x18 / +0x19 of the net-rate display.
struct mp_packet_info {
    int type;                                         // +0x00
    void (*handler)(ubyte *data, void *from);         // +0x04 (data, sender address)
    int flag;                                         // +0x08
    int sent;                                         // +0x0c send counter, bumped by the senders
    char name[8];                                     // +0x10 "GM_QURY" ...
    ubyte rate_flags[4];                              // +0x18
};
LAYOUT_CHECK(mp_packet_info, sizeof(mp_packet_info) == 0x1c);
extern mp_packet_info mp_packet_table[66];            // 0x5951b8

#define MP_BUILD_HEADER(t) (data[0] = (ubyte)(t), packet_size = MP_HEADER_SIZE, mp_packet_table[t].sent++)
#define MP_ADD_DATA(v)     (memcpy(data + packet_size, &(v), sizeof(v)), packet_size += sizeof(v))
#define MP_END_PACKET()    (*(ushort *)(data + 1) = (ushort)(packet_size - MP_HEADER_SIZE))
// A float goes through a stack temporary (mp_send_player_data 0x4af8c6: `mov [esp+8], edx / mov esi,
// [esp+8]` before the store into the packet; decomp47, macro ours).
#define MP_ADD_FLOAT(v)    do { float mp_tmp_f = (v); memcpy(data + packet_size, &mp_tmp_f, 4); \
                                packet_size += 4; } while (0)

// (data, total length, to (0 = the server), reliable). Return unused here.
void multi_send_packet(ubyte *data, int len, mp_player *to, int reliable);   // 0x442060
// Host: multi_send_packet to every other connected player. (data, total length, reliable)
void mp_broadcast_packet(ubyte *data, int len, int reliable);              // 0x441a90
// Host: resends mp_cur_packet to every other player except the sender (the handler's second argument).
void mp_forward_cur_packet(void *from);                                    // 0x4422c0
// The same through multi_send_packet_buffered (the object update and switch handlers, SMN_QCK).
void mp_forward_cur_packet_buffered(void *from);                           // 0x442230
int  mp_handle_to_net(int handle);                                         // 0x4752c0
int  mp_local_handle_from_net(int net_handle);                             // 0x475280
obj *mp_obj_from_net_handle(int net_handle);                               // 0x475240
void mp_map_net_handle(int net_handle, int handle);                        // 0x475300 (return unused)
mp_player *mp_player_find_by_id(int id);                                   // 0x4751d0
mp_player *mp_player_find_by_name(const char *name);                       // 0x475200 (stricmp)
// Three 16-bit values scaled to the level bounds; the position is passed by value (last).
void mp_quantize_position(short *x, short *y, short *z, vector3 pos);      // 0x475660
vector3 mp_dequantize_position(short x, short y, short z);                 // 0x4756e0 (hidden result)

// ---- session -----------------------------------------------------------------------------------
void mp_host_game(const char *name, const char *password, int max_players, int difficulty,
                  int levels_below, int levels_above);   // 0x43edc0 (return type guessed)
void mp_session_reset();                                // 0x441490
// decomp47 (multiplayerdata.cpp, multiendgame.cpp; module "-" in sum.symbols, units not known).
mp_player *mp_player_alloc();                           // 0x441610 NULL when the free list is empty
void mp_player_disconnect(mp_player *p);                // 0x441660 party_remove_player, closes +0x18, unlinks
void tracker_end_game();                                // 0x4aba20 (mp_leave_game, host of a tracked game)
extern uint mp_server_addr[2];                          // 0x651404 (mp_handle_acpt_jn)
extern int mp_server_player_id;                         // 0x65140c
bool mp_level_available_to_all(int level, int difficulty);   // 0x43ebd0
void mp_levelselect_set_level(int level);               // 0x43ec20
void mp_levelselect_start_if_ready();                   // 0x43ec40 (return type guessed)
void mp_remove_player(mp_player *p, int reason);        // 0x471df0 (reason 1 = kicked)
void mp_respawn_and_notify(int id);                     // 0x481130 (hud_confirm_quit)
extern int mp_netgame_state;                            // 0x6513fc session state 0..5 (5 = playing)
// decomp51 (game_pause / game_unpause): the host broadcasts the session state with the pause flag;
// send_now picks mp_broadcast_packet over mp_broadcast_packet_buffered (sum.symbols name kept).
void multi_set_netgame_state(bool send_now);            // 0x440f20
// mp_player_disconnected (0x4814e0) is in multiendgame.h (decomp47: (reason, quit), D2601).
void mp_send_character_skills(Character *c);            // 0x480ba0
extern int mp_netgame_level;                            // 0x6513d8
extern int mp_game_difficulty;                          // 0x6513ec
// decomp41 (level_script_init, level_apply_pending_change): the host's character level and the level
// scale picked by mp_host_game (frand_range(0.8, 1.2)); the host's level change ("%s_m1").
extern int mp_netgame_host_level;                       // 0x6513f0
extern float mp_netgame_level_scale;                    // 0x651410
void mp_change_level(int entry_point);                  // 0x4ab780 (return unused)
extern int mp_netgame_num_players;                      // 0x6513e4 (decomp37: the HP / gold / xp multiplier)
// The host's broadcasts of living_entity.cpp (decomp37; argument lists from its calls).
void mp_broadcast_hp(Character *c, int source);         // 0x480920
void mp_broadcast_ap(Character *c, short ap);           // 0x480a10
void mp_broadcast_xp(Character *c, int xp);             // 0x480af0
void mp_broadcast_status_add(Character *c, int status, int ms, int spell, float value, int source);   // 0x480cc0
void mp_broadcast_status_cure(Character *c, int status, bool restore_fx);   // 0x480de0
extern bool All_multi_levels;                           // 0x60ae64
void pxo_read_config_thunk();                           // 0x441560 jmp pxo_read_config (GameInit)
extern unsigned short mp_net_port;                      // 0x651594 UDP port, 10234 unless -port (cmdline_read_params)
// (completed-level mask, level, difficulty): every prerequisite of the level is in the mask.
bool mp_player_level_available(unsigned int mask, int level, int difficulty);   // 0x43eb70

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

// ---- PXO lobby (the IRC chat behind screen 11: lobby_* 0x43ef50-0x43f910 next to the multiplayer
// states, pxo_login_* 0x42f280; units not known) ---------------------------------------------------------
// One channel (0x90 bytes, lobby_channels 0x60eef8, at most 5). Type and field names ours.
struct lobby_channel {
    char name[0x21];                  // +0x00 IRC channel name ('+' prefix for one typed by the player)
    char topic[0x67];                 // +0x21 shown in the channel list
    int  num_users;                   // +0x88 first number of the list row
    int  unk8c;                       // +0x8c second number of the list row
};
LAYOUT_CHECK(lobby_channel, sizeof(lobby_channel) == 0x90);
extern int lobby_num_users;                             // 0x60ae68
extern int lobby_num_channels;                          // 0x60ae6c
// 1 connected, 4 joining a channel, 8 the typed channel is the one to join, 0x80000000 error
extern unsigned int lobby_flags;                        // 0x60ae70
extern int lobby_cur_channel;                           // 0x60ae74 joined channel, -1 none
extern int lobby_connect_state;                         // 0x60ae7c printed by the "Error: %d" status
extern char lobby_users[500][33];                       // 0x60ae80
extern lobby_channel lobby_channels[5];                 // 0x60eef8
int  lobby_add_channel(lobby_channel *ch);              // 0x43ef50 index, < 0 when the list is full
void lobby_join_channel(const char *name);              // 0x43f7d0 (return unused)
void lobby_send_chat_input(const char *text);           // 0x43f910 (return unused)
void pxo_login_reset();                                 // 0x42f280
// 0 logged in, 1 no account, 2 cannot contact the server, 3 validating (busy), 4 invalid user name.
int  pxo_login_step();                                  // 0x42f290

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

// The game-state calls of gameplay.cpp (module "-", 0x441150-0x4415d0; decomp42).
void multi_process_client_status(mp_player *p, int a);   // 0x441150 (state_game_enter: mp_local_player, 1)
void mp_game_frame();                                    // 0x4412a0 (state_game_frame)
void multi_do_frame();                                   // 0x4413d0 (LoadingScreenDraw)
void mp_level_start();                                   // 0x441530 (state_game_enter)
void mp_set_local_status(int status);                    // 0x4415d0 (state_game_enter: 5)

#endif
