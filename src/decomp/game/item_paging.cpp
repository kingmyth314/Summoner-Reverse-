// item_paging.cpp - Sum.exe v1.40, code 0x46c5d0-0x46cc5f (Engine\objects\item_paging.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: symbols/sum.symbols tags 0x46c7d0-0x46cb50 with item_paging.cpp. The unit starts
// earlier, at the static initialiser 0x46c5d0 (init_mempools_ip_general), because it builds the
// 0xa59870 pools that items_reset_paging resets (symbols already put that pool here); the stub
// 0x46c7c0 sits between them. The previous unit (add_gold console command) ends with cmd_add_gold's RET at 0x46c5cd.
// The RealtimeTimer at 0xa59a84 and its initialiser pair 0x46cc60/0x46cc70 moved to multisave.cpp
// (decomp24); see the end of this file.

#include <stdio.h>
#include "engine/objects/item_paging.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

// Original __FILE__ of this unit (literal 0x59a914).
#define ITEM_PAGING_CPP "D:\\projects\\Summoner\\pccode\\Engine\\objects\\item_paging.cpp"

// ---- .bss, in address order ----------------------------------------------------------------
// Buffer labels are ours. The general buffers have room for 19 pools (0x9a9870 + 19 * 0x8000 =
// 0xa41870, where the popup buffer starts) but only 17 are used: 0xa31870-0xa41870 is never
// referenced.
static char ip_level_buffer[0x38000];                          // 0x971870
static char ip_general_buffers[MAX_ITEM_PAGE_SLOTS][0x8000];   // 0x9a9870
static char ip_info_popup_buffer[0x18000];                     // 0xa41870

// 0x46c5d0 init_mempools_ip_general_thunk (jmp) and 0x46c5e0 init_mempools_ip_general: the
// dynamic initialiser of this array, 19 Mempool::Copy calls in index order.
// Confidence: unsure. One initialiser function means one definition; the inline constructor that
// forwards to Copy (engine_stubs.h) is an assumption. A temporary-and-copy form would add copies.
Mempool ip_general_mempool[MAX_ITEM_PAGE_SLOTS] = {            // 0xa59870 (0x1c bytes each)
    Mempool(ip_general_buffers[0], 0x8000, "IP general"),
    Mempool(ip_general_buffers[1], 0x8000, "IP general"),
    Mempool(ip_general_buffers[2], 0x8000, "IP general"),
    Mempool(ip_general_buffers[3], 0x8000, "IP general"),
    Mempool(ip_general_buffers[4], 0x8000, "IP general"),
    Mempool(ip_general_buffers[5], 0x8000, "IP general"),
    Mempool(ip_general_buffers[6], 0x8000, "IP general"),
    Mempool(ip_general_buffers[7], 0x8000, "IP general"),
    Mempool(ip_general_buffers[8], 0x8000, "IP general"),
    Mempool(ip_general_buffers[9], 0x8000, "IP general"),
    Mempool(ip_general_buffers[10], 0x8000, "IP general"),
    Mempool(ip_general_buffers[11], 0x8000, "IP general"),
    Mempool(ip_general_buffers[12], 0x8000, "IP general"),
    Mempool(ip_general_buffers[13], 0x8000, "IP general"),
    Mempool(ip_general_buffers[14], 0x8000, "IP general"),
    Mempool(ip_general_buffers[15], 0x8000, "IP general"),
    Mempool(ip_general_buffers[16], 0x8000, "IP general"),
    Mempool(ip_level_buffer, 0x38000, "IP level"),
    Mempool(ip_info_popup_buffer, 0x18000, "IP info popup"),
};

// ---- .data (0x59a750-0x59a87f) ---------------------------------------------------------------
// Static address constants, so this is .data, not a dynamic initialiser. Entry 18 has no
// initialiser (all zero, pool NULL); items_reset_paging fills in every pool pointer.
item_page_slot item_page_slots[MAX_ITEM_PAGE_SLOTS] = {        // 0x59a750
    { -1, -1, &ip_general_mempool[0], 0 },
    { -1, -1, &ip_general_mempool[1], 0 },
    { -1, -1, &ip_general_mempool[2], 0 },
    { -1, -1, &ip_general_mempool[3], 0 },
    { -1, -1, &ip_general_mempool[4], 0 },
    { -1, -1, &ip_general_mempool[5], 0 },
    { -1, -1, &ip_general_mempool[6], 0 },
    { -1, -1, &ip_general_mempool[7], 0 },
    { -1, -1, &ip_general_mempool[8], 0 },
    { -1, -1, &ip_general_mempool[9], 0 },
    { -1, -1, &ip_general_mempool[10], 0 },
    { -1, -1, &ip_general_mempool[11], 0 },
    { -1, -1, &ip_general_mempool[12], 0 },
    { -1, -1, &ip_general_mempool[13], 0 },
    { -1, -1, &ip_general_mempool[14], 0 },
    { -1, -1, &ip_general_mempool[15], 0 },
    { -1, -1, &ip_general_mempool[16], 0 },
    { -1, -1, &ip_general_mempool[17], 0 },
};

// 0x46c7c0  Confidence: exact. Always 0 (a disabled cache lookup?); only caller item_page_in.
int item_page_in_stub(ItemType *type, item_page_slot *slot)
{
    (void)type;
    (void)slot;
    return 0;
}

// 0x46c7d0  Confidence: exact. The mesh pointer is reloaded from the type after every call.
// Drop points are looked up for weapons (kind 0) only.
int item_page_in_mesh(ItemType *type, item_page_slot *slot)
{
    type->mesh = vim_load(type->model, slot->vim_arg);
    if (type->mesh == NULL) {
        printf("FAILED TO PAGE VIF MESH FOR ITEM PAGE!\n");
        return 0;
    }

    for (int i = 0; i < type->mesh->num_textures; i++)
        gr_page_in(type->mesh->textures[i], 0);

    slot->type = type - item_types;     // SUB / SAR 8

    if (type->mesh != NULL && type->kind == 0) {
        type->drop_tag1 = vim_find_tag(type->mesh, "$drop-ground01");
        type->drop_tag2 = vim_find_tag(type->mesh, "$drop-ground02");
    }
    return 1;
}

// 0x46c880  Confidence: exact. Pages a whole item type into the "IP level" pool (slot 17); the
// pool is not rewound first. The shared PUSH 0 before the branch is the two
// set_current_mempool(NULL) calls merged.
int item_type_page_in(int type)
{
    if (type < 0 || type >= num_item_types)
        return 0;
    if (item_type_is_paged_in(type))
        return 1;

    set_current_mempool(item_page_slots[ITEM_PAGE_SLOT_TYPE].pool);
    if (!item_page_in_mesh(&item_types[type], &item_page_slots[ITEM_PAGE_SLOT_TYPE])) {
        set_current_mempool(NULL);
        return 0;
    }
    set_current_mempool(NULL);
    return 1;
}

// 0x46c8f0  Confidence: exact. The first loop counts down (DEC ECX) because i is not otherwise
// used; the second walks the pools and the slots together.
void items_reset_paging()
{
    int i;

    for (i = 0; i < num_item_types; i++)
        item_types[i].mesh = NULL;

    for (i = 0; i < MAX_ITEM_PAGE_SLOTS; i++) {
        item_page_slots[i].type = -1;
        item_page_slots[i].vim_arg = -1;
        item_page_slots[i].paged = 0;
        ip_general_mempool[i].offset = 0;
        item_page_slots[i].pool = &ip_general_mempool[i];
    }
}

// 0x46c940  Confidence: likely (the stub test: when item_page_in_stub returns non-zero the code
// skips straight to the success tail; the pool rewind stores the stub's 0 from EAX).
int item_page_in(int type, int slot, int unused)
{
    if (slot == ITEM_PAGE_SLOT_TYPE)
        return item_type_page_in(type);

    if (slot < 0 || slot >= MAX_ITEM_PAGE_SLOTS || type < 0 || type >= num_item_types)
        return 0;

    item_page_slot *s = &item_page_slots[slot];
    ItemType *t = &item_types[type];

    if (s->type == type)
        return 1;
    if (s->type >= 0)
        item_page_out_all(slot);

    if (item_type_is_paged_in(type)) {
        s->type = type;
        s->vim_arg = -1;
        s->paged = 0;
        return 1;
    }

    if (!item_page_in_stub(t, s)) {
        s->pool->offset = 0;
        set_current_mempool(s->pool);
        if (!item_page_in_mesh(t, s)) {
            printf("MESH PAGE FAILED!\n");
            set_current_mempool(NULL);
            return 0;
        }
    }
    s->paged = 1;
    set_current_mempool(NULL);
    return 1;
}

// 0x46ca50  Confidence: likely (order of the two loads in the pool swap). Slot 17 is never paged
// out. If another general slot (0-16) shows the same type, that slot takes over the pool that
// holds the mesh; for the popup slot (18) the other slot is cleared instead and the function
// returns without clearing slot 18 itself.
void item_page_out_all(int slot)
{
    if (slot == ITEM_PAGE_SLOT_TYPE)
        return;

    item_page_slot *s = &item_page_slots[slot];
    if (s->type >= 0 && s->paged != 0) {
        int other = -1;

        for (int i = 0; i < NUM_IP_GENERAL_POOLS; i++) {
            if (i != slot && item_page_slots[i].type == s->type) {
                other = i;
                break;
            }
        }

        if (slot == ITEM_PAGE_SLOT_POPUP && other >= 0) {
            item_page_slots[other].type = -1;
            item_page_slots[other].vim_arg = -1;
            item_page_slots[other].paged = 0;
            item_types[s->type].mesh = NULL;
            return;
        }

        printf("FULL PAGEOUT!\n");
        if (other >= 0) {
            Mempool *pool = item_page_slots[other].pool;
            item_page_slots[other].pool = s->pool;
            s->pool = pool;
            item_page_slots[other].paged = 1;
        } else {
            item_types[s->type].mesh = NULL;
            s->pool->offset = 0;
        }
    }

    s->type = -1;
    s->vim_arg = -1;
    s->paged = 0;
}

// 0x46cb30  Confidence: exact (int result from SETNE CL / MOV EAX, ECX).
int item_type_is_paged_in(int type)
{
    if (type < 0)
        return 0;
    return item_types[type].mesh != NULL;
}

// 0x46cb50  Confidence: likely (case order: the jump table 0x46cc50 maps 0,1,2,3 to bodies laid
// out as 0,1,3,2, so the source lists case 3 before case 2). Two slots per hero: single player by
// species (Joseph 0, Jekhar 1, Flece 2, Rosalind 3, the feedback.tbl order), multiplayer by
// position in party_list. Error line 671 (0x29f). With an empty party list the code skips the
// i >= 4 test (i is known to be 0).
void item_paging_find_player_slot(Character *character, int *slot1, int *slot2)
{
    if (!(game_flags & 0x2)) {
        switch (character->info->species) {
        case 0:
            *slot1 = 0;
            *slot2 = 1;
            return;
        case 1:
            *slot1 = 2;
            *slot2 = 3;
            return;
        case 3:
            *slot1 = 6;
            *slot2 = 7;
            return;
        case 2:
            *slot1 = 4;
            *slot2 = 5;
            return;
        default:
            *slot1 = -1;
            *slot2 = -1;
            return;
        }
    }

    if (character->type == 7) {
        int i = 0;
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            if (p->handle == character->handle)
                break;
            i++;
        }
        if (i >= 4)
            for (;;)
                DebugError(ITEM_PAGING_CPP, 671, "Unable to find paging slot for player\n");
        *slot1 = i * 2;
        *slot2 = i * 2 + 1;
        return;
    }

    *slot1 = -1;
    *slot2 = -1;
}

// The unit ends with the jump table 0x46cc50-0x46cc5f of item_paging_find_player_slot. The
// RealtimeTimer at 0xa59a84 (mp_player_autosave_timer) and its initialiser pair 0x46cc60 / 0x46cc70,
// formerly defined here, belong to the next unit: they are now the first definition of
// multisave.cpp (decomp24), whose functions are its only users.
