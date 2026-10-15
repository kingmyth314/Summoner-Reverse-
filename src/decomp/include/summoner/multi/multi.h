// summoner/multi/multi.h - the multiplayer game: players, session, trade, chat, character files.
// Units (strings.tbl / sum.symbols): multiplayerdata.cpp, multitrade.cpp (0x470f80-0x471a70),
// multiendgame.cpp (0x471df0, 0x4814e0), multichat.cpp (0x487b90), multicontainer.cpp (0x488c00-0x4895e0),
// multisave.cpp; the .plr functions are tagged item_paging.cpp. One header for now; directory and file
// name are ours. Not original source.
#ifndef SUMMONER_MULTI_MULTI_H
#define SUMMONER_MULTI_MULTI_H

#include "vsdk/vsdk.h"
#include "engine/objects/items.h"
#include "engine/objects/objects.h"

class Player;
class Character;

// A multiplayer player record (partial). The list is circular through +0x00, head mp_player_list (a
// whole record). Merged from three stub copies (DECISIONS.md D10).
struct mp_player {
    mp_player *next;                  // +0x00 mp_player_list
    mp_player *prev;                  // +0x04 (inferred: list pair)
    ubyte _unk08[0x08];
    unsigned int flags;               // +0x10 bit 0 host
    int type;                         // +0x14 state: 3 listed on the level select screen, 4..9 loading steps
                                      //       (6 linking players, 9 ready; mp_draw_loading_status)
    ubyte _unk18[0x04];
    int id;                           // +0x1c player id (hud_confirm_quit passes it to mp_respawn_and_notify)
    ubyte _unk20[0x08];
    Player *object;                   // +0x28 the party record (its +0xc handle is read; party_remove_all clears it)
    ubyte _unk2c[0x08];
    char name[0x20];                  // +0x34 copied from the character profile by mp_local_player_create
};
extern mp_player mp_player_list;      // 0x651418 head
extern mp_player *mp_local_player;    // 0x651598

// The pilots read these fields through macros while the stub record was short (kept, DECISIONS.md D10).
#define MP_PLAYER_NAME(p) ((p)->name)
#define MP_PLAYER_NEXT(p) ((p)->next)
#define MP_PLAYER_TYPE(p) ((p)->type)
#define MP_PLAYER_STATE(p) ((p)->type)  // new_hud.cpp's name for +0x14 (D50)
#define MP_PLAYER_ID(p)    ((p)->id)

// ---- session -----------------------------------------------------------------------------------
void mp_host_game(const char *name, const char *password, int max_players, int difficulty,
                  int levels_below, int levels_above);   // 0x43edc0 (return type guessed)
void mp_session_reset();                                // 0x441490
bool mp_level_available_to_all(int level, int difficulty);   // 0x43ebd0
void mp_levelselect_set_level(int level);               // 0x43ec20
void mp_levelselect_start_if_ready();                   // 0x43ec40 (return type guessed)
void mp_remove_player(mp_player *p, int reason);        // 0x471df0 (reason 1 = kicked)
void mp_respawn_and_notify(int id);                     // 0x481130 (hud_confirm_quit)
extern int mp_netgame_state;                            // 0x6513fc session state 0..5 (5 = playing)
// The level select passes (2, 0) / (0, 0): the first argument looks like a reason or mode, not an id.
void mp_player_disconnected(int id, int reason);        // 0x4814e0
void chat_send_line(const char *text);                  // 0x488070
void mp_send_character_skills(Character *c);            // 0x480ba0
void mp_send_container_close(int handle, int num_taken, int num_items,
                             item_obj **items, bool *selected);   // 0x488da0
extern int mp_netgame_level;                            // 0x6513d8
extern int mp_game_difficulty;                          // 0x6513ec
extern bool All_multi_levels;                           // 0x60ae64
extern unsigned int pxo_cfg_flags;                      // 0xa59a88 bit 0: back to state 11, else 12
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

// ---- .plr character files ---------------------------------------------------------------------------
// The character record of the character select list (mp_char_profiles 0x5b6e78, 0x154 bytes). The stubs'
// mp_char_profile (flags only) and mp_char_record are one type (DECISIONS.md D11).
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
// (out, filename): out is the record + 0x28. 0 = read; sum.symbols' "(path, out)" has the order reversed.
int mp_player_file_read_summary(void *out, const char *filename);    // 0x46d450
// 0 created, 1 player exists, 2 no player directory, 3 cannot write (jump table 0x4078a0).
int mp_player_file_create(const char *name, int char_info, const char *description);   // 0x46cc80

// ---- multitrade.cpp -------------------------------------------------------------------------------
// One offered item (0xc bytes; mp_trade_remote_items 0x22e4630, mp_trade_local_items 0x22e46f8, 16 each).
struct mp_trade_item {
    ItemType *type;                   // +0x0
    int count;                        // +0x4
    int charges;                      // +0x8
};
// The trade partner record (partial).
struct mp_trade_peer {
    ubyte _unk00[0x0c];
    int id;                           // +0x0c sent in the IT_* packets
    ubyte _unk10[0x04];
    mp_player *player;                // +0x14
};
extern int mp_trade_state;                              // 0x22e46f0 (3, 4 read here)
extern mp_trade_item mp_trade_remote_items[16];         // 0x22e4630
extern mp_trade_item mp_trade_local_items[16];          // 0x22e46f8
extern int mp_trade_local_count;                        // 0x22e47b8
extern mp_trade_peer *mp_trade_partner;                 // 0x22e47bc
extern int mp_trade_remote_count;                       // 0x22e47c8
void mp_send_it_addi(inv_item *item, int count);        // 0x471370
void mp_send_it_rmvi(ItemType *type);                   // 0x4715a0
void mp_send_it_okay();                                 // 0x471820
void mp_trade_cancel();                                 // 0x471a70
void mp_send_equip_item(Character *c, int slot, int type);   // 0x4878a0 packet 0x33 (return unused)
void mp_send_unequip_item(Character *c, int slot);           // 0x487a70 packet 0x41 (return unused)

#endif
