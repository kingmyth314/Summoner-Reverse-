// stubs_buysell_itemtrade.h - declarations that new_buysell.cpp and new_itemtrade.cpp need beyond
// stubs_menus_dialogue.h, ui_screen.h and stubs_gamecreate_journal.h. Names and addresses from
// symbols/sum.symbols; argument lists read from the call sites in Sum.exe v1.40 (tools/asm.mjs).
// "(type guessed)" marks return or parameter types that the call sites do not show.
#ifndef STUBS_BUYSELL_ITEMTRADE_H
#define STUBS_BUYSELL_ITEMTRADE_H

#include "stubs_menus_dialogue.h"     // gr_*, bm_*, mouse_*, ui_*, interface_*, game_*, mp_player
#include "stubs_gamecreate_journal.h" // RealtimeTimer, mouse_get_delta, gr_get_clip, gr__set_clip,
                                      // key_to_ascii, MP_PLAYER_NAME, KEY_B/I/R/BACKSPACE/ENTER,
                                      // shop_set_shopkeeper_from_char, ui_has_owner_character,
                                      // statscreen_refresh_anim
#include "types.h"                    // ItemType, shopkeeper, shop_item, Character, vector3, matrix3, gr__Screen

// windef.h style min / max (as in stubs_assess_pickup.h; the code has the setle / dec / and pattern)
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// ---- vsdk\gr ----------------------------------------------------------------------------------------
extern gr__Screen gr_screen;                                         // 0x25d8d30 (offset_x +0x2c, offset_y +0x30)
int  gr_screen_width();                                              // 0x506170
int  gr_screen_height();                                             // 0x506180
void gr_setup_3d(matrix3 *orient, vector3 *pos, float fov, int a, int b);  // 0x50dec0 (a, b both 1 here)

// gr_light_desc (vsdk\gr): 0x20 bytes on the stack of buysell_render_item_mesh, built by the thiscall
// constructor 0x50f3e0 (no destructor). The float at +0x14 is set to 0.15 before ca_render_mesh.
class gr_light_desc {
public:
    gr_light_desc();                  // 0x50f3e0 gr_light_desc__gr_light_desc
    unsigned char _unk00[0x14];
    float f14;                        // +0x14 (the third float of the vector3 the ctor builds at +0xc)
    unsigned char _unk18[0x08];
};

// types.h describes matrix3 as a C struct; matrix3__make_identity (0x511050) is a thiscall member.
// Until the headers are merged the method is declared on this layout-identical wrapper.
struct matrix3_t : public matrix3 {
    void make_identity();             // 0x511050 matrix3__make_identity
};

// ---- vsdk\ca -----------------------------------------------------------------------------------------
void ca_render_set_mesh(void *mesh);                                 // 0x50d6e0
void ca_render_mesh(vector3 *pos, matrix3 *orient, gr_light_desc *light);   // 0x50d6f0 (return unused)

// ---- vsdk\os\mouse -----------------------------------------------------------------------------------
bool mouse_was_double_clicked(int button);                           // 0x5012e0

// ---- interface / ui helpers -------------------------------------------------------------------------
void ui_tooltip_set(int x, int y, int w, int h, const char *text);   // 0x414750 (sum.symbols says (x, y, text):
                                                                     // the text is the fifth argument)
bool interface_input_ready();                                        // 0x4148e0
void ui_3d_begin(vector3 *pos);                                      // 0x412400 (argument type guessed)
void ui_3d_end();                                                    // 0x4125a0
void new_icam_load(void *table, const char *filename);               // 0x410760 (return unused)
void new_icam_get(void *table, const char *name, matrix3 *orient, vector3 *pos);  // 0x410b70
void item_info_popup_open_type(int type);                            // 0x418920
int  item_info_popup_frame(int key);                                 // 0x418960
void item_info_popup_render();                                       // 0x418e40
bool item_info_popup_is_active();                                    // 0x419160
void chat_input_cancel();                                            // 0x41ed50
extern Color ui_color_gold;                                          // 0x5bd7a0
extern Color ui_color_blue;                                          // 0x5bd328
extern Color ui_color_red;                                           // 0x5bd954
extern Character *status_character;                                  // 0x5bd810 (new_status.cpp)

// ---- objects / items --------------------------------------------------------------------------------
// The living-entity list sentinel; sum.symbols' living_list_first (0x8ecd78) is its live_next (+0x318).
extern Character living_list;                                        // 0x8eca60

extern ItemType item_types[];                                        // 0x8cbec0, 0x100 bytes each
int  item_type_find(const char *name);                               // 0x44b450, index or -1
bool item_type_is_stackable(ItemType *type);                         // 0x44b500
bool character_meets_item_requirements(Character *c, ItemType *type);   // 0x44b550
bool item_type_is_identified(ItemType *type);                        // 0x44b6b0
void item_get_display_name(char *buf, ItemType *type);               // 0x44d8a0

// ItemType +0x58 (+Charge Effect:), 0x18 bytes; only the charge maximum is read here.
struct item_charge_effect {
    int spell;                        // +0x00
    int max_charges;                  // +0x04 (buysell_buy_item compares the owned item's charges with it)
    unsigned char _unk08[0x10];
};

// An item object as the party inventory and the trade code see it (partial).
struct inv_item {
    int _unk00;
    ItemType *type;                   // +0x04
    int charges;                      // +0x08 (read by buysell_buy_item and mp_send_it_addi)
};

// One party_inventory entry (8 bytes; stubs_assess_pickup.h declares the array as int[521][2]).
struct party_inventory_entry {
    int count;                        // +0x0
    inv_item *item;                   // +0x4
};
extern party_inventory_entry party_inventory[521];                   // 0x970820
extern int party_gold;                                               // 0x971868
extern int party_inventory_count;                                    // 0x97186c

int  party_inventory_find_type(ItemType *type);                      // 0x46bd00, index or -1
void party_inventory_add_type(int type, int count, int a, int b);    // 0x46b980 (called (type, 1, -1, 1); return unused)
void party_inventory_remove(int slot, int a, int b);                 // 0x46bb30 (called (slot, 1, 1))
int  shop_inventory_find_first_in_category(int category, shop_item *list, int count);  // 0x46b5d0
int  item_type_shop_category(int type);                              // 0x46b5f0 (0 weapons .. 4 misc, -1)
void item_page_out_all(int slot);                                    // 0x46ca50
// item_page_in: the call sites push (type, slot, 0) and the callee compares its SECOND argument with 17
// (ITEM_PAGE_SLOT_TYPE); item_paging.h declares it as (int slot, int type). See NOTES.md.
int  item_page_in(int type, int slot, int unused);                   // 0x46c940

shopkeeper *level_shopkeeper_find(const char *name);                 // 0x4ce950

// ---- multitrade.cpp ---------------------------------------------------------------------------------
// One offered item (0xc bytes; mp_trade_remote_items 0x22e4630, mp_trade_local_items 0x22e46f8, 16 each).
struct mp_trade_item {
    ItemType *type;                   // +0x0 (compared with inv_item.type, passed to mp_send_it_rmvi)
    int count;                        // +0x4
    int charges;                      // +0x8 (copied from inv_item +8 by mp_send_it_addi)
};
// The trade partner record (partial): +0xc id sent in the IT_* packets, +0x14 the mp_player.
struct mp_trade_peer {
    unsigned char _unk00[0x0c];
    int id;                           // +0x0c
    unsigned char _unk10[0x04];
    mp_player *player;                // +0x14 (its name is at +0x34, MP_PLAYER_NAME)
};
extern int mp_trade_state;                                           // 0x22e46f0 (3, 4 read here)
extern mp_trade_item mp_trade_remote_items[16];                      // 0x22e4630
extern mp_trade_item mp_trade_local_items[16];                       // 0x22e46f8
extern int mp_trade_local_count;                                     // 0x22e47b8
extern mp_trade_peer *mp_trade_partner;                              // 0x22e47bc
extern int mp_trade_remote_count;                                    // 0x22e47c8
void mp_send_it_addi(inv_item *item, int count);                     // 0x471370
void mp_send_it_rmvi(ItemType *type);                                // 0x4715a0
void mp_send_it_okay();                                              // 0x471820
void mp_trade_cancel();                                              // 0x471a70

#endif
