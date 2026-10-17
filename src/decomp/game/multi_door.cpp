// multi_door.cpp - Sum.exe v1.40, code 0x4aa780-0x4aac05 (doors in a network game)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl id 246 "This door already unlocked." in the multi_door.cpp
// section). It follows avoid_record_axis_at (0x4aa760, tag "-") and ends before 0x4aac10.
// .data 0x59d34c-0x59d39b: the two "Unable to find door ..." formats. 0.0f (0x57eb84) and 0.05f (0x58021c)
// are shared float constants. No .bss.
//
// The host owns the doors. Its own clicks go through object_try_unlock_or_open -> door_find_by_mesh; a
// client sends service request 4 (open) / 5 (close) / 7 (pick) and mp_handle_srv_req calls the same
// functions with the client's id. The host then broadcasts DOOR_TG, and DOR_PCK goes to everybody on a
// success, otherwise only to the picking player. A door is named by its lobjs index ("mesh id").
//
// Compared with the container code (multicontainer.cpp): the pick never clears the door's lock value
// (door_info +0x58); the unlock is only the savegame two-stage locked flag, so "This door already
// unlocked." (result 2, lock == 0) is reached only by doors that had no lock to begin with.

#include <stdio.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define LOBJ_FLAG_DOOR 0x100

// 0x4aa780  Confidence: exact (the two ints read with memcpy, D8302). DOOR_TG on the other machines. No range or NULL check of the index.
void mp_handle_door_toggle(ubyte *data, void *from)
{
    int index, open;
    memcpy(&index, data, 4);
    memcpy(&open, data + 4, 4);
    lobj *l = &lobjs[index];

    if (l->flags & LOBJ_FLAG_DOOR) {
        if (open)
            door_set_open(l, -1.0f, 1, 1);
        else
            door_set_closed(l);
    }
}

// 0x4aa7d0  Confidence: exact (VC6 byte match, match24; was likely) (if-chain over the result; `locked` is kept in the dead argument slot).
// DOR_PCK: the result of the host's roll (door_pick_by_mesh). A success unlocks the door in the two-stage
// state and runs the level's use hook, as on the host; a failure plays the sound without the feedback.
void mp_handle_door_lockpick(ubyte *data, void *from)
{
    int player_id = ((int *)data)[0];
    int index = ((int *)data)[1];
    int result = ((int *)data)[2];
    bool open, locked;

    mp_player *p = mp_player_find_by_id(player_id);
    if (!p || !p->object)
        return;
    Character *ch = living_from_handle(p->object->handle);
    if (!ch)
        return;
    lobj *l = &lobjs[index];
    if (!l || !(l->flags & LOBJ_FLAG_DOOR))
        return;

    if (result == 0) {
        savegame_state.two_stage_get(&open, &locked, l);
        savegame_state.two_stage_set(open, false, l);
        level_script_use(l->name);
        game_play_sound(6, 1.0f, 0.0f, 0, 0);
        feedback_play(ch->info->species, 8, &ch->pos);
    } else if (result == 1) {
        game_play_sound(5, 1.0f, 0.0f, 0, 0);
    } else if (result == 2) {
        hud_show_message(XSTR("This door already unlocked.", 246));
    }
}

// 0x4aa8e0  Confidence: exact (VC6 byte match, match24; was likely) (the local `open` is overwritten for the local player and reloaded
// otherwise). For the host's own click the door has been toggled already (door_toggle_state), so the saved
// state is replaced by the state before the click and the door is started again (here with door_set_open's
// third argument 0, where the DOOR_TG handler passes 1). A closed, unlocked door opens; an open door
// closes; the others are left alone. The error text is formatted and never printed.
void door_find_by_mesh(int player_id, int lobj_index, int want_open)
{
    char error[256];
    bool open, locked;
    lobj *l = &lobjs[lobj_index];

    if (l && (l->flags & LOBJ_FLAG_DOOR)) {
        savegame_state.two_stage_get(&open, &locked, l);
        if (player_id == mp_local_player->id)
            open = !want_open;
        if (want_open) {
            if (!open && !locked) {
                door_set_open(l, -1.0f, 0, 1);
                mp_send_door_toggle(lobj_index, 1);
            }
        } else if (open) {
            door_set_closed(l);
            mp_send_door_toggle(lobj_index, 0);
        }
        return;
    }
    sprintf(error, "Unable to find door for mesh id %d\n", lobj_index);
}

// 0x4aa9c0  Confidence: exact. DOOR_TG (lobj index, open), reliable broadcast.
void mp_send_door_toggle(int lobj_index, int open)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_DOOR_TG);
    MP_ADD_DATA(lobj_index);
    MP_ADD_DATA(open);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x4aaa10  Confidence: likely (`open` reuses the stack slot of the door pointer). The host's lock pick for
// a player: pick lock skill + 0.05 against the door's lock value, as container_lockpick. The error text is
// formatted and never printed.
void door_pick_by_mesh(int player_id, int lobj_index)
{
    char error[256];
    bool open, locked;
    lobj *l = &lobjs[lobj_index];

    if (!l || !(l->flags & LOBJ_FLAG_DOOR)) {
        sprintf(error, "Unable to find door to pick for mesh id %d\n", lobj_index);
        return;
    }
    mp_player *p = mp_player_find_by_id(player_id);
    if (!p || !p->object)
        return;
    Character *ch = living_from_handle(p->object->handle);
    if (!ch)
        return;

    door_info *d = door_from_lobj(l);
    if (d->lock == 0.0f) {
        mp_send_door_lockpick(p, lobj_index, 2);
        return;
    }
    if (ch->get_skill(SKILL_PICK_LOCK) + 0.05f >= d->lock) {
        savegame_state.two_stage_get(&open, &locked, l);
        savegame_state.two_stage_set(open, false, l);
        level_script_use(l->name);
        game_play_sound(6, 1.0f, 0.0f, 0, 0);
        feedback_play(ch->info->species, 8, &ch->pos);
        mp_send_door_lockpick(p, lobj_index, 0);
    } else {
        game_play_sound(5, 1.0f, 0.0f, 0, 0);
        mp_send_door_lockpick(p, lobj_index, 1);
    }
}

// 0x4aab90  Confidence: exact. DOR_PCK (player id, lobj index, result): a success goes to everybody, the
// rest only to the picking player (unreliably).
void mp_send_door_lockpick(mp_player *p, int lobj_index, int result)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_DOR_PCK);
    MP_ADD_DATA(p->id);
    MP_ADD_DATA(lobj_index);
    MP_ADD_DATA(result);
    MP_END_PACKET();
    if (result == 0)
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, p, 0);
}
