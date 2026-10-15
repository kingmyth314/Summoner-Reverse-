// item_paging.h - Sum.exe v1.40 item mesh paging (Engine\objects\item_paging.cpp, 0x46c5d0-0x46cc7a)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef ITEM_PAGING_H
#define ITEM_PAGING_H

#include "stubs_sound_items_registry.h"

// 19 paging slots, each with its own mempool:
//   0-15  ?  general slots (item_paging_find_player_slot hands out 0-7, two per hero)
//   16       general
//   17       ITEM_PAGE_SLOT_TYPE: whole item types (item_type_page_in), "IP level" pool, never paged out
//   18       ITEM_PAGE_SLOT_POPUP: the item info popup, "IP info popup" pool (no pool in the .data
//            initialiser; items_reset_paging assigns it)
#define MAX_ITEM_PAGE_SLOTS 19
#define NUM_IP_GENERAL_POOLS 17
#define ITEM_PAGE_SLOT_TYPE 17
#define ITEM_PAGE_SLOT_POPUP 18

struct item_page_slot {              // 0x10 bytes
    int type;                         // +0x0 item_types index paged into this slot, -1 none
    int vim_arg;                      // +0x4 second argument of vim_load, -1 (meaning unknown)
    Mempool *pool;                    // +0x8
    int paged;                        // +0xC 1 when the mesh in the pool belongs to this slot
};

extern item_page_slot item_page_slots[MAX_ITEM_PAGE_SLOTS]; // 0x59a750

int item_page_in_stub(ItemType *type, item_page_slot *slot);
int item_page_in_mesh(ItemType *type, item_page_slot *slot);
int item_type_page_in(int type);
void items_reset_paging();
int item_page_in(int type, int slot, int unused); // callers push (type, slot, 0); the third argument is never read
void item_page_out_all(int slot);
int item_type_is_paged_in(int type);
void item_paging_find_player_slot(Character *character, int *slot1, int *slot2);

#endif
