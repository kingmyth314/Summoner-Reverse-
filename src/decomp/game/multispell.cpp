// multispell.cpp - Sum.exe v1.40, code 0x4afca0-0x4b0957 (spell casts and summons in a network game:
// packets 0x34 CST_ANM, 0x35 RLS_SPL, 0x36-0x3a SMN_RQS / SMN_DNY / SMN_ALD / SMN_HND / SMN_QCK).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl id 238 in mp_handle_summon_deny / mp_summon_try_claim). It
// follows mp_page_in_player_items (0x4afbe0, multiplayerdata.cpp) and ends before mp_send_level_complete
// (0x4b0960, module "-"). Data: no .data of its own; .bss 0x247e824-0x247e82f (the summon claim).
//
// Casts: whoever casts (spell_prepare_cast, then mp_spell_cast_sync, then spell_cast_launch_effects) sends
// RLS_SPL with the whole request; the other machines rebuild it (mp_handle_release_spell) and launch the
// same effects; the host forwards. CST_ANM only plays the cast move on the other machines.
// Summons: one per game. The host owns the claim (mp_summon_owner): a client asks with SMN_RQS, the host
// answers SMN_DNY to the asker or broadcasts SMN_ALD with the new owner; SMN_HND carries the summon's net
// handle, SMN_QCK dismisses it everywhere.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/spells/spells.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define SPELL_TARGET_POINT_A 0x1d   // the two spells whose request carries the extra point (+0x1c)
#define SPELL_TARGET_POINT_B 0x1f

// The packet writers and readers of the request's extra fields (names ours). The writers copy through
// a local (a stack temporary in the code), the readers straight into the field.
#define MP_PUT_INT(data, offset, v)  { int put_ = (v); memcpy((data) + *(offset), &put_, 4); *(offset) += 4; }
#define MP_PUT_FLOAT(data, offset, v)  { float put_ = v; memcpy((data) + *(offset), &put_, 4); *(offset) += 4; }
#define MP_GET_INT(data, offset, v)  { memcpy(&(v), (data) + *(offset), 4); *(offset) += 4; }

// ---- .bss ------------------------------------------------------------------------------------------
int mp_summon_owner;                                          // 0x247e824
int mp_summon_net_handle;                                     // 0x247e828
int mp_summon_request_pending;                                // 0x247e82c

// 0x4afca0  Confidence: exact (VC6 byte match, w6 pass; was likely) (the stack temporaries).
void mp_write_spell_target_point(spell_cast_request *req, ubyte *data, ushort *offset)
{
    switch (req->spell) {
    case SPELL_TARGET_POINT_A:
    case SPELL_TARGET_POINT_B:
        MP_PUT_FLOAT(data, offset, ((vector3 *)req->_unk1c)->x);
        MP_PUT_FLOAT(data, offset, ((vector3 *)req->_unk1c)->y);
        MP_PUT_FLOAT(data, offset, ((vector3 *)req->_unk1c)->z);
        break;
    }
}

// 0x4afd10  Confidence: exact (VC6 byte match, match14; was exact). The spell is passed separately (the request's copy is the same value).
void mp_read_spell_target_point(spell_cast_request *req, ubyte *data, int *offset, int spell)
{
    switch (spell) {
    case SPELL_TARGET_POINT_A:
    case SPELL_TARGET_POINT_B:
        MP_GET_INT(data, offset, req->_unk1c[0]);
        MP_GET_INT(data, offset, req->_unk1c[1]);
        MP_GET_INT(data, offset, req->_unk1c[2]);
        break;
    }
}

// 0x4afd60  Confidence: exact (VC6 byte match, w6 pass; was likely) (jump table 0x4aff80 for modes 1..10; the case bodies are not merged, the
// stack temporaries differ between them). Only the handle lists of modes 4 and 5 become net handles.
void mp_write_spell_targets(spell_cast_request *req, ubyte *data, ushort *offset)
{
    spell_cast_params *p = &req->params;
    int i;

    switch (p->mode) {
    case 1:
        MP_PUT_FLOAT(data, offset, p->ftarget[0]);
        break;
    case 2:
        MP_PUT_FLOAT(data, offset, p->ftarget[0]);
        break;
    case 3:
        MP_PUT_INT(data, offset, p->target[0]);
        MP_PUT_FLOAT(data, offset, p->ftarget[1]);
        break;
    case 4:
        for (i = 0; i < 6; i++) {
            MP_PUT_INT(data, offset, mp_handle_to_net(p->target[i]));
            MP_PUT_FLOAT(data, offset, p->ftarget[6 + i]);
        }
        break;
    case 5:
        for (i = 0; i < 6; i++)
            MP_PUT_INT(data, offset, mp_handle_to_net(p->target[i]));
        MP_PUT_INT(data, offset, p->target[6]);
        MP_PUT_FLOAT(data, offset, p->ftarget[7]);
        break;
    case 6:
        MP_PUT_FLOAT(data, offset, p->ftarget[0]);
        MP_PUT_INT(data, offset, p->target[1]);
        break;
    case 7:
        MP_PUT_FLOAT(data, offset, p->ftarget[0]);
        MP_PUT_FLOAT(data, offset, p->ftarget[1]);
        break;
    case 8:
        break;
    case 9:
        MP_PUT_INT(data, offset, p->target[0]);
        break;
    case 10:
        for (i = 0; i < 5; i++) {
            MP_PUT_INT(data, offset, p->target[i]);
            MP_PUT_FLOAT(data, offset, p->ftarget[5 + i]);
        }
        break;
    }
}

// 0x4affb0  Confidence: likely (jump table 0x4b0100; modes 2 / 9 and 3 / 6 / 7 share their bodies here).
// m-1021d w9 D21602: a pointer local for the target array (82 -> 73: the pops are now late as in the
// binary); left: Sum.exe keeps req itself in EDI (all four pushes first) and reuses it as the loop pointer.
// The 4th argument (the spell, unused) is pushed by mp_handle_release_spell (D21603). D25103: case 10's
// loop runs i over 5..9 (t[i - 5], t[i]), which gives the binary's loop pointer at the second half.
void mp_read_spell_targets(spell_cast_request *req, ubyte *data, int *offset, int spell)
{
    spell_cast_params *p = &req->params;
    int *t = p->target;
    int net;
    int i;

    switch (p->mode) {
    case 1:
        MP_GET_INT(data, offset, t[0]);
        break;
    case 4:
        for (i = 0; i < 6; i++) {
            MP_GET_INT(data, offset, net);
            MP_GET_INT(data, offset, t[6 + i]);
            t[i] = mp_local_handle_from_net(net);
        }
        break;
    case 5:
        for (i = 0; i < 6; i++) {
            MP_GET_INT(data, offset, net);
            t[i] = mp_local_handle_from_net(net);
        }
        MP_GET_INT(data, offset, t[6]);
        MP_GET_INT(data, offset, t[7]);
        break;
    case 3:
    case 6:
    case 7:
        MP_GET_INT(data, offset, t[0]);
        MP_GET_INT(data, offset, t[1]);
        break;
    case 2:
    case 9:
        MP_GET_INT(data, offset, t[0]);
        break;
    case 10:
        for (i = 5; i < 10; i++) {
            MP_GET_INT(data, offset, t[i - 5]);
            MP_GET_INT(data, offset, t[i]);
        }
        break;
    case 8:
        break;
    }
}

// 0x4b0130  Confidence: exact. Unreliable, to the one player; nothing is sent when the id is unknown.
void mp_send_summon_deny(int player_id)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    mp_player *p = mp_player_find_by_id(player_id);
    if (!p)
        return;
    MP_BUILD_HEADER(MP_SMN_DNY);
    MP_END_PACKET();
    multi_send_packet(data, packet_size, p, 0);
}

// 0x4b0180  Confidence: exact. (caster net handle, spell, flag byte); a client sends to the server, the
// host (and a non-network game) broadcasts.
void mp_send_cast_anim(Character *caster, int spell, bool flag)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_CST_ANM);
    MP_ADD_DATA(caster->net_handle);
    MP_ADD_DATA(spell);
    MP_ADD_DATA(flag);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, 1);
    else
        mp_broadcast_packet(data, packet_size, 1);
}

// 0x4b0210  Confidence: exact (VC6 byte match, match23; was likely; if / else for the two lookups). The host's net handles are its local
// handles (living_from_handle); a client maps them.
void mp_handle_cast_anim(ubyte *data, void *from)
{
    int net = ((int *)data)[0];
    Character *c;

    if (MP_IS_HOST())
        c = living_from_handle(net);
    else
        c = (Character *)mp_obj_from_net_handle(net);
    if (!c || c == cur_character)
        return;

    int spell = ((int *)data)[1];
    ubyte flag = data[8];
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
    c->perform_spell_cast(spell, flag != 0);
}

// 0x4b02a0  Confidence: exact (VC6 byte match, w6 pass; was likely) (the stack temporaries of the point; the -1 constant kept in a register).
// RLS_SPL: caster, spell, target, (layer, x, y, z) or -1, item index or -1, source type, the extra point,
// the three fields of the params' first word, the targets.
void mp_spell_cast_sync(spell_cast_request *req)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int none = -1;
    int v;

    MP_BUILD_HEADER(MP_RLS_SPL);
    if (req->caster)
        MP_ADD_DATA(req->caster->net_handle);
    else
        MP_ADD_DATA(none);
    MP_ADD_DATA(req->spell);
    v = mp_handle_to_net(req->target);
    MP_ADD_DATA(v);
    if (req->target_pos) {
        MP_ADD_DATA(req->layer);
        MP_ADD_FLOAT(req->target_pos->x);
        MP_ADD_FLOAT(req->target_pos->y);
        MP_ADD_FLOAT(req->target_pos->z);
    } else {
        MP_ADD_DATA(none);
    }
    v = none;
    if (req->item)
        v = req->item - item_types;
    MP_ADD_DATA(v);
    MP_ADD_DATA(req->source_type);
    mp_write_spell_target_point(req, data, &packet_size);
    v = req->params.mode;
    MP_ADD_DATA(v);
    v = req->params.value;
    MP_ADD_DATA(v);
    v = req->params.kind;
    MP_ADD_DATA(v);
    mp_write_spell_targets(req, data, &packet_size);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, 1);
    else
        multi_send_packet(data, packet_size, NULL, 1);
}

// 0x4b04a0  Confidence: exact (m-1021d w9 D21603: the seven-field constructor with the caster first; the
// spell and the source type read into locals; the spell, kept in EBX, is also the 4th argument of
// mp_read_spell_targets, which Sum.exe pushes (8 dwords popped after the two calls); the three bitfield
// stores are merged into one by the compiler). The caster stays NULL when its net handle is not a living
// object here.
void mp_handle_release_spell(ubyte *data, void *from)
{
    int offset = 0;
    vector3 pos;
    spell_cast_request req(NULL, -1);
    int net;
    int spell;
    int item;
    int st;
    int mode, value, kind;

    MP_GET_INT(data, &offset, net);
    obj *o = mp_obj_from_net_handle(net);
    if (o && o->is_living())
        req.caster = (Character *)o;
    MP_GET_INT(data, &offset, spell);
    req.spell = spell;
    MP_GET_INT(data, &offset, req.target);
    req.target = mp_local_handle_from_net(req.target);
    MP_GET_INT(data, &offset, req.layer);
    if (req.layer >= 0) {
        req.target_pos = &pos;
        MP_GET_INT(data, &offset, pos.x);
        MP_GET_INT(data, &offset, pos.y);
        MP_GET_INT(data, &offset, pos.z);
    }
    MP_GET_INT(data, &offset, item);
    if (item >= 0)
        req.item = &item_types[item];
    MP_GET_INT(data, &offset, st);
    req.source_type = st;
    mp_read_spell_target_point(&req, data, &offset, spell);
    MP_GET_INT(data, &offset, mode);
    MP_GET_INT(data, &offset, value);
    MP_GET_INT(data, &offset, kind);
    req.params.mode = mode;
    req.params.value = value;
    req.params.kind = kind;
    mp_read_spell_targets(&req, data, &offset, spell);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
    spell_cast_launch_effects(&req);
}

// 0x4b0640  Confidence: exact (tail jump to ai_go_idle).
void mp_handle_summon_deny(ubyte *data, void *from)
{
    hud_show_message(XSTR("Another summon already active", 238));
    cur_character->ai.go_idle();
}

// 0x4b0660  Confidence: exact.
void mp_broadcast_summon_allowed(int player_id)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_SMN_ALD);
    MP_ADD_DATA(player_id);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x4b06a0  Confidence: exact (VC6 byte match, w6 pass: the owner copied out with memcpy into a local, D10500).
void mp_handle_summon_allowed(ubyte *data, void *from)
{
    int owner;

    memcpy(&owner, data, 4);
    mp_summon_owner = owner;
}

// 0x4b06b0  Confidence: exact. Unreliable, to the server; the claim stays pending until the answer.
void mp_send_summon_request()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_SMN_RQS);
    MP_ADD_DATA(mp_local_player->id);
    MP_END_PACKET();
    multi_send_packet(data, packet_size, NULL, 0);
    mp_summon_request_pending = 1;
}

// 0x4b0700  Confidence: exact. The host's answer; it does not check that it is the host.
void mp_handle_summon_request(ubyte *data, void *from)
{
    int id = ((int *)data)[0];

    if (mp_summon_owner != -1) {
        mp_send_summon_deny(id);
        return;
    }
    mp_summon_owner = id;
    mp_broadcast_summon_allowed(id);
}

// 0x4b0730  Confidence: exact.
void mp_summon_claim_reset()
{
    mp_summon_request_pending = 0;
    mp_summon_owner = -1;
    mp_summon_net_handle = -1;
}

// 0x4b0750  Confidence: exact (VC6 byte match, match14; was exact). Called again each frame of the cast until it returns 1 (the claim came
// back as ours) or the deny / another owner stops the cast.
int mp_summon_try_claim()
{
    if (mp_summon_owner != -1) {
        mp_summon_request_pending = 0;
        if (mp_summon_owner != mp_local_player->id) {
            hud_show_message(XSTR("Another summon already active", 238));
            cur_character->ai.go_idle();
            return 0;
        }
    } else if (MP_IS_HOST()) {
        mp_summon_owner = mp_local_player->id;
    } else {
        if (!mp_summon_request_pending)
            mp_send_summon_request();
        return 0;
    }
    return 1;
}

// 0x4b07d0  Confidence: exact.
void mp_broadcast_summon_handle(int net_handle)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    mp_summon_net_handle = net_handle;
    MP_BUILD_HEADER(MP_SMN_HND);
    MP_ADD_DATA(net_handle);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x4b0820  Confidence: exact. When the summon already exists here its handle is mapped now, else
// mp_summon_map_handle does it from summon_create.
void mp_handle_summon_handle(ubyte *data, void *from)
{
    int net = ((int *)data)[0];

    if (cur_summon)
        mp_map_net_handle(net, cur_summon->handle);
    mp_summon_net_handle = net;
}

// 0x4b0850  Confidence: exact.
void mp_summon_map_handle(int handle)
{
    if (mp_summon_net_handle != -1)
        mp_map_net_handle(mp_summon_net_handle, handle);
}

// 0x4b0870  Confidence: exact. Unreliable from a client, reliable from the host.
void mp_send_summon_quick(int handle)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    int net = mp_handle_to_net(handle);
    MP_BUILD_HEADER(MP_SMN_QCK);
    MP_ADD_DATA(net);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, 0);
    else
        mp_broadcast_packet(data, packet_size, 1);
}

// 0x4b08f0  Confidence: exact (VC6 byte match, w6 pass: the net handle copied out with memcpy, D10500).
void mp_handle_summon_quick(ubyte *data, void *from)
{
    int net;

    memcpy(&net, data, 4);
    Character *c = living_from_handle(mp_local_handle_from_net(net));

    if (c)
        summon_dismiss(c->handle);
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}

// 0x4b0940  Confidence: exact (same code as mp_summon_claim_reset; not folded).
void mp_summon_claim_release()
{
    mp_summon_request_pending = 0;
    mp_summon_owner = -1;
    mp_summon_net_handle = -1;
}
