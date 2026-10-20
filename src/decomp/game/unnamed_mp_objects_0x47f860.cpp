// unnamed_mp_objects_0x47f860.cpp - Sum.exe v1.40, code 0x47f860-0x481196 (the network game's object
// packets: the host's monster list and the clients' handle map, new and deleted living entities, paths,
// position updates, HP / AP / XP / skills, status effects, knock-downs, respawns). THE FILE NAME IS A
// PLACEHOLDER: no path string, no strings.tbl block, no literals. Written from the Sum.exe disassembly
// (tools/asm.mjs) by decomp76. Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression
// form is a guess), unsure (the shape is a guess).
//
// Unit: after spells.cpp (ccmd_summon 0x47f700-0x47f85f) and directly before multiendgame.cpp's static
// initialiser (0x4811a0). mp_handle_new_path reads multiendgame.cpp's multi_path_type, and the handle map
// (0x22e94f4) lies 0xc bytes after ccmd_Multi_path_type (0x22e94e8) in .bss: this code may be the head of
// multiendgame.cpp (whose static initialiser would then not open the file). Kept separate, as source-map.md
// has it (D5810). The map is defined here (multi.h left its owner open).
//
// The host sends its local handles; a client sends net handles (mp_handle_to_net) and maps received ones
// through mp_net_handle_map (mp_local_handle_from_net / mp_obj_from_net_handle).

#include <string.h>
#include <math.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/new_hud.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multiendgame.h"
#include "summoner/multi/unnamed_netpackets_0x441820.h"
#include "summoner/multi/unnamed_netsession_0x440b40.h"
#include "summoner/multi/unnamed_mp_objects_0x47f860.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/camera/camera.h"
#include "engine/characterinfo/characterinfo.h"

#define MP_PI 3.14159265f
// The heading of OBJ_UPD: (h + pi) / (2 pi) * 65535 in a 16-bit word, back as w * (2 pi / 65535) - pi.
#define MP_HEADING_SCALE (1.0f / (2.0f * MP_PI))
#define MP_HEADING_UNSCALE 9.5875264e-5f  // 0x38c910a4 (the binary's constant)

// ---- .bss --------------------------------------------------------------------------------------------
int mp_net_handle_map[MP_NET_HANDLE_MAP_SIZE];          // 0x22e94f4

// 0x47f860  Confidence: exact (VC6 byte match, match14; was likely) (one byte: a short jump that is near in the original). From mp_handle_obj_update for a traveling party member that is not the
// local leader: when the time since the last update over the mean sampled interval (100 ms with no
// samples) is positive, the AI walks to the received point; the interval is pushed into the samples, the
// timer restarted and the position kept.
void mp_remote_travel_update(mp_player *p, const vector3 *pos)
{
    Character *c;
    float sum;
    float mean;
    int i, n;

    c = living_from_handle(p->object->handle);
    if (!c || c->type != 7 || c == cur_character)
        return;
    n = p->swi_num_samples;
    sum = 0.0f;
    for (i = 0; i < n; i++)
        sum += p->swi_samples[i];
    if (n > 0)
        mean = sum / n;
    else
        mean = 100.0f;
    if (p->swi_timer.GetElapsedTime() / mean > 0.0f) {
        vector3 dest = *pos;
        c->ai.travel_to_xyz(dest);
        memmove(&p->swi_samples[1], &p->swi_samples[0],
                (p->swi_num_samples > 15 ? 15 : p->swi_num_samples) * sizeof(int));
        p->swi_samples[0] = p->swi_timer.GetElapsedTime();
        p->swi_num_samples++;
        p->swi_num_samples = p->swi_num_samples < MP_TRAVEL_SAMPLES ? p->swi_num_samples : MP_TRAVEL_SAMPLES;
        p->swi_timer.SetTimeout(0);
        *(vector3 *)p->swi_pos = c->pos;
    }
}

// 0x47f9a0  Confidence: exact. Every net handle unmapped.
void mp_net_handle_map_reset()
{
    memset(mp_net_handle_map, 0xff, sizeof(mp_net_handle_map));
}

// 0x47f9c0  Confidence: exact (VC6 byte match). Once a second each: OBJ_UPD of the local leader (unless hidden or dead) and
// of the locally controlled summon, and on the host of every monster (types 5 and 9) in AI mode 3 (the
// others' timers are invalidated). The summon's update sends the leader again, not the summon (bug,
// D5810).
void mp_send_local_updates()
{
    Character *c;
    Character *m;

    c = living_from_handle(mp_local_player->object->handle);
    if (!(c->flags & (CF_HIDDEN | CF_DEAD))) {
        if (!c->net_update_timer.valid() || c->net_update_timer.elapsed()) {
            mp_send_obj_update(c, 0);
            c->net_update_timer.SetTimeout(1000);
        }
    }
    if (cur_summon && character_is_locally_controlled(cur_summon)) {
        if (!cur_summon->net_update_timer.valid() || cur_summon->net_update_timer.elapsed()) {
            mp_send_obj_update(c, 0);
            cur_summon->net_update_timer.SetTimeout(1000);
        }
    }
    if (MP_IS_HOST()) {
        for (m = living_list.live_next; m != &living_list; m = m->live_next) {
            if (m->type != 5 && m->type != 9)
                continue;
            if (!m->ai.is_mode(3, -1)) {
                m->net_update_timer.invalidate();
                continue;
            }
            if (!m->net_update_timer.valid() || m->net_update_timer.elapsed()) {
                mp_send_obj_update(m, 0);
                m->net_update_timer.SetTimeout(1000);
            }
        }
    }
}

// The living entity types the monster list covers (4, 5, 9). Macro ours.
#define MP_IS_LISTED_TYPE(t) ((t) >= 4 && ((t) <= 5 || (t) == 9))

// 0x47fb20  Confidence: exact (VC6 byte match). Host: OBJ_HND {type, name\0, handle}... of every living entity of type 4,
// 5 or 9, sixteen per packet, each packet ended by a 0 type byte.
void mp_send_monster_list()
{
    int count;
    ushort len;
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    obj *o;

    packet_size = 0;
    if (!MP_IS_HOST())
        return;
    count = 0;
    for (o = obj_active_list.next; o != &obj_active_list; o = o->next) {
        if (!MP_IS_LISTED_TYPE(o->type))
            continue;
        if (count == 0)
            MP_BUILD_HEADER(MP_OBJ_HND);
        data[packet_size++] = (ubyte)o->type;
        if (MP_IS_LISTED_TYPE(o->type)) {
            len = (ushort)strlen(((Character *)o)->get_name());
            memcpy(data + packet_size, ((Character *)o)->get_name(), len);
            packet_size += len;
            data[packet_size++] = 0;
            MP_ADD_DATA(o->handle);
            count++;
        }
        if (count >= 16) {
            data[packet_size++] = 0;
            MP_END_PACKET();
            mp_broadcast_packet(data, packet_size, true);
            count = 0;
        }
    }
    if (count > 0) {
        data[packet_size++] = 0;
        MP_END_PACKET();
        mp_broadcast_packet(data, packet_size, true);
    }
}

// 0x47fcc0  Confidence: exact (VC6 byte match). Client: maps each listed (type, name, host handle) to the local living
// entity of that name and type.
void mp_handle_obj_hnd(ubyte *data, void *from)
{
    int off;
    int type;
    int net;
    char name[0x40];
    Character *c;

    off = 0;
    if (!MP_IS_CLIENT())
        return;
    do {
        type = data[off++];
        if (MP_IS_LISTED_TYPE(type)) {
            strcpy(name, (char *)data + off);
            off = (ushort)(strlen((char *)data + off) + off + 1);
            net = *(int *)(data + off);
            off += 4;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (strcmp(c->name, name) == 0 && c->type == type) {
                    mp_map_net_handle(net, c->handle);
                    break;
                }
            }
        }
    } while (type);
}

// 0x47fdf0  Confidence: likely. NEW_LEP {type, char_info index, handle, HP, position, packed orientation}.
void mp_send_new_lep(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    mp_packed_orient packed;
    int info;
    float f;

    info = c->info - char_infos;
    MP_BUILD_HEADER(MP_NEW_LEP);
    MP_ADD_DATA(c->type);
    MP_ADD_DATA(info);
    MP_ADD_DATA(c->handle);
    f = c->hp;
    MP_ADD_DATA(f);
    f = c->pos.x;
    MP_ADD_DATA(f);
    f = c->pos.y;
    MP_ADD_DATA(f);
    f = c->pos.z;
    MP_ADD_DATA(f);
    matrix3_pack_orientation(&packed, &c->orient);
    MP_ADD_DATA(packed.r0);
    MP_ADD_DATA(packed.r1);
    MP_ADD_DATA(packed.u0);
    MP_ADD_DATA(packed.u1);
    MP_ADD_DATA(packed.signs);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, true);
    else
        mp_broadcast_packet(data, packet_size, true);
}

// 0x47ff10  Confidence: exact (VC6 byte match, match29; was likely). The entity is created where the sender has it; a failed create ends the
// game for this machine (reason 3), but the map and HP stores still go through the NULL pointer (latent,
// D5810).
void mp_handle_new_lep(ubyte *data, void *from)
{
    int type, info, net;
    float hp;
    vector3 pos;
    mp_packed_orient packed;
    matrix orient;
    Character *c;

    type = ((int *)data)[0];
    info = ((int *)data)[1];
    net = ((int *)data)[2];
    hp = ((float *)data)[3];
    pos.x = ((float *)data)[4];
    pos.y = ((float *)data)[5];
    pos.z = ((float *)data)[6];
    data += 0x1c;
    memcpy(&packed, data, 17);
    matrix3_unpack_orientation(&packed, &orient);
    c = living_entity_create(type, info, &pos, &orient, true, 0, 0, 0, false);
    if (!c)
        mp_player_disconnected(3, false);
    mp_map_net_handle(net, c->handle);
    c->hp = hp;
    c->ai.reset();
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x480000  Confidence: exact (VC6 byte match). DEL_LEP {handle}: the living entity is deleted.
void mp_handle_delete_entity(ubyte *data, void *from)
{
    int h = *(int *)data;
    Character *c;

    if (!MP_IS_HOST())
        h = mp_local_handle_from_net(h);
    c = living_from_handle(h);
    if (c)
        delete c;
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x480070  Confidence: exact (VC6 byte match, match29; was likely). NEW_PTH {handle, target, node count, x, z and layer per node, the last
// node's cost as a byte} of a locally controlled character's AI path.
void mp_send_new_path(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int h, t;
    ubyte n;
    ai_path *p;
    float x, z;

    if (!character_is_locally_controlled(c))
        return;
    MP_BUILD_HEADER(MP_NEW_PTH);
    if (MP_IS_HOST()) {
        h = c->handle;
        t = obj_from_handle(c->ai.target) ? c->ai.target : -1;
    } else {
        h = mp_handle_to_net(c->handle);
        t = mp_handle_to_net(c->ai.target);
    }
    MP_ADD_DATA(h);
    MP_ADD_DATA(t);
    n = 0;
    for (p = c->ai.path; p; p = p->next)
        n++;
    data[packet_size++] = n;
    for (p = c->ai.path; p; p = p->next) {
        ubyte layer = (ubyte)p->layer;
        x = p->pos.x;
        MP_ADD_DATA(x);
        z = p->pos.z;
        MP_ADD_DATA(z);
        data[packet_size++] = layer;
        if (!p->next)
            data[packet_size++] = (ubyte)(int)p->cost;
    }
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet(data, packet_size, NULL, true);
    else
        mp_broadcast_packet(data, packet_size, true);
}

// 0x4801f0  Confidence: likely (match29, D8800: n is a ubyte, each node read through a vector2 with memcpy, the
// direction loop without a second path test; still differs: the vector2 is stored to the stack before the call). multi_path_type 0 (full path): the nodes are
// rebuilt as sent, with their directions and the remaining cost counted back from the last node's;
// 1 (generate): the AI searches from its position to the first node and appends a search to the last.
void mp_handle_new_path(ubyte *data, void *from)
{
    int h, t;
    int ch, th;
    Character *c;
    ubyte n;
    int i;
    int off;
    ubyte layer;
    float cost;
    ai_path *last, *node, *prev, *p;
    vector3 v;

    h = ((int *)data)[0];
    t = ((int *)data)[1];
    if (MP_IS_HOST()) {
        ch = h;
        th = t;
    } else {
        ch = mp_local_handle_from_net(h);
        th = t < 0 ? t : mp_local_handle_from_net(t);
    }
    c = living_from_handle(ch);
    if (!c || c == cur_character)
        return;
    c->ai.target = th;
    ai_path_free(c->ai.path);
    c->ai.path = NULL;
    n = data[8];
    last = NULL;
    off = 9;
    cost = 0.0f;
    for (i = 0; i < n; i++) {
        v.zero();
        vector2 p;
        memcpy(&p.x, data + off, 4);
        off += 4;
        memcpy(&p.y, data + off, 4);
        off += 4;
        v.x = p.x;
        v.z = p.y;
        layer = data[off++];
        if (multi_path_type == 0) {
            if (i == 0) {
                last = ai_path_node_create(p, layer, NULL);
                c->ai.path = last;
                if (last)
                    c->ai.path_start = last->pos;
            } else {
                node = ai_path_node_create(p, layer, NULL);
                last->next = node;
                if (node) {
                    cost += (node->pos - last->pos).len();
                    last = node;
                }
            }
        } else if (multi_path_type == 1) {
            if (i == 0)
                c->ai.path_find_new(&c->pos, c->layer, &v, layer, c->info->size, 0.0f);
            else if (i == n - 1)
                c->ai.path_find_append(&c->pos, c->layer, &v, layer, c->info->size, 0.0f);
        }
    }
    cost += (signed char)data[off];
    if (multi_path_type == 0) {
        prev = NULL;
        for (p = c->ai.path; p; prev = p, p = p->next) {
            if (p == c->ai.path) {
                p->dir = -c->pos;
                p->dir += p->pos;
                p->dir.y = 0.0f;
            } else {
                p->dir = p->pos - prev->pos;
                cost -= (p->pos - prev->pos).len();
            }
            p->cost = cost;
        }
    }
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x480530  Confidence: likely. OBJ_UPD {handle, traveling, quantized position, heading, layer, target
// and forward speed}, sent buffered.
void mp_send_obj_update(Character *c, int a)
{
    ushort qx, qy, qz;
    float p, b, h;
    float t;
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int handle;
    ushort heading;
    ushort layer;
    float f;

    mp_quantize_position((short *)&qx, (short *)&qy, (short *)&qz, c->pos);
    c->orient.extract_angles(&p, &b, &h);
    t = (h + MP_PI) * MP_HEADING_SCALE;
    heading = (ushort)(int)(t * 65535.0f);
    layer = (ushort)c->layer;
    MP_BUILD_HEADER(MP_OBJ_UPD);
    if (MP_IS_HOST())
        handle = c->handle;
    else
        handle = mp_handle_to_net(c->handle);
    MP_ADD_DATA(handle);
    data[packet_size++] = (ubyte)a;
    MP_ADD_DATA(qx);
    MP_ADD_DATA(qy);
    MP_ADD_DATA(qz);
    MP_ADD_DATA(heading);
    MP_ADD_DATA(layer);
    f = c->target_speed;
    MP_ADD_DATA(f);
    f = c->forward_speed;
    MP_ADD_DATA(f);
    MP_END_PACKET();
    if (MP_IS_CLIENT())
        multi_send_packet_buffered(data, packet_size, NULL, false);
    else
        mp_broadcast_packet_buffered(data, packet_size, false);
}

// 0x4806a0  Confidence: exact (cp / cb / ch in the angle block, so they share the dead slots; the two float
// constants are the binary's, 1 ulp off before. match29, D8800: the decision tree with goto set / done, the position assigned
// from the call, the target speed read with memcpy; instruction-identical, the frame differs: the binary keeps
// ch / traveling in the dead data slot). The target speed is always taken. A traveling party
// member walks through mp_remote_travel_update. The position is set (and the path searched again from it)
// when the sender says it travels, when a traveling member is more than 10 units off, or otherwise when
// the distance is above half the forward speed plus 1.5 or the heading differs by more than 10 degrees.
void mp_handle_obj_update(ubyte *data, void *from)
{
    int h = *(int *)data;
    Character *c;
    ubyte traveling;
    ushort x, y, z, heading, layer;
    float target_speed, forward_speed;
    float new_h;
    bool remote;
    ai_path *path, *n;

    if (!MP_IS_HOST())
        h = mp_local_handle_from_net(h);
    c = living_from_handle(h);
    if (!c || c == cur_character)
        return;
    traveling = data[4];
    x = *(ushort *)(data + 5);
    y = *(ushort *)(data + 7);
    z = *(ushort *)(data + 9);
    heading = *(ushort *)(data + 0xb);
    layer = *(ushort *)(data + 0xd);
    memcpy(&target_speed, data + 0xf, 4);
    forward_speed = *(float *)(data + 0x13);
    vector3 pos;
    pos = mp_dequantize_position(x, y, z);
    new_h = heading * MP_HEADING_UNSCALE - MP_PI;
    c->set_6ac(target_speed);
    remote = false;
    if (c->type == 7) {
        mp_player *mp = player_from_handle(c->handle)->mp_owner;
        if (mp->flags & MP_PF_TRAVELING) {
            mp_remote_travel_update(mp, &pos);
            remote = true;
        }
    }
    if (!traveling) {
        if (remote) {
            if (distance(&c->pos, &pos) > 10.0f)
                goto set;
            goto done;
        }
        if (!(distance(&c->pos, &pos) > forward_speed * 0.5f + 1.5f)) {
            float cp, cb, ch;
            c->orient.extract_angles(&cp, &cb, &ch);
            if (!(fabsf(ch - new_h) > 0.17453294f))
                goto done;
        }
    }
set:
    c->pos = pos;
    c->orient.set_from_angles(0.0f, 0.0f, new_h);
    c->layer = layer;
    c->find_ground();
    path = c->ai.path;
    if (path) {
        c->ai.path = NULL;
        for (n = path; n; n = n->next) {
            if (n == path)
                c->ai.path_find_new(&c->pos, c->layer, &n->pos, n->layer, c->info->size, 0.0f);
            else if (!n->next)
                c->ai.path_find_append(&c->pos, c->layer, &n->pos, n->layer, c->info->size, 0.0f);
        }
        ai_path_free(path);
    }
done:
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}

// 0x480920  Confidence: exact (VC6 byte match). Host: ADJ_HPS {handle, HP, the overflow, source}.
void mp_broadcast_hp(Character *c, int source)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    float f;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_ADJ_HPS);
    MP_ADD_DATA(c->net_handle);
    f = c->hp;
    MP_ADD_DATA(f);
    f = c->hp_overflow;
    MP_ADD_DATA(f);
    MP_ADD_DATA(source);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x4809b0  Confidence: exact (VC6 byte match, match29; was likely). Client: the HP before the overflow, then the overflow added from the net.
void mp_handle_adjust_hp(ubyte *data, void *from)
{
    int h;
    float hp, overflow;
    int source;
    Character *c;

    if (!MP_IS_CLIENT())
        return;
    memcpy(&h, data, 4);
    memcpy(&hp, data + 4, 4);
    memcpy(&overflow, data + 8, 4);
    memcpy(&source, data + 12, 4);
    c = (Character *)mp_obj_from_net_handle(h);
    if (!c)
        return;
    c->hp = hp - overflow;
    c->add_hp(overflow, source, 1, true);
}

// 0x480a10  Confidence: exact (VC6 byte match). Host: ADJ_APS {handle, the AP before, the new AP}.
void mp_broadcast_ap(Character *c, short ap)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_ADJ_APS);
    MP_ADD_DATA(c->net_handle);
    MP_ADD_DATA(c->ap);
    MP_ADD_DATA(ap);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x480a90  Confidence: exact (VC6 byte match).
void mp_handle_adjust_ap(ubyte *data, void *from)
{
    int h;
    short before, after;
    Character *c;

    if (!MP_IS_CLIENT())
        return;
    h = ((int *)data)[0];
    before = *(short *)(data + 4);
    after = *(short *)(data + 6);
    c = (Character *)mp_obj_from_net_handle(h);
    if (!c)
        return;
    c->ap = before;
    c->add_ap((short)(after - before));
}

// 0x480af0  Confidence: exact (VC6 byte match). Host: ADJ_EXP {handle, xp}.
void mp_broadcast_xp(Character *c, int xp)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_ADJ_EXP);
    MP_ADD_DATA(c->net_handle);
    MP_ADD_DATA(xp);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x480b60  Confidence: exact (VC6 byte match).
void mp_handle_adjust_xp(ubyte *data, void *from)
{
    int h;
    int xp;
    Character *c;

    if (!MP_IS_CLIENT())
        return;
    h = ((int *)data)[0];
    xp = ((int *)data)[1];
    c = (Character *)mp_obj_from_net_handle(h);
    if (c)
        c->add_xp(xp, false, true);
}

// 0x480ba0  Confidence: exact (VC6 byte match). ADJ_SKL {handle, the 36 skill bytes} of a party member.
void mp_send_character_skills(Character *c)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    int i;

    if (c->type != 7)
        return;
    MP_BUILD_HEADER(MP_ADJ_SKL);
    MP_ADD_DATA(c->net_handle);
    for (i = 0; i < MP_NUM_SKILL_BYTES; i++)
        data[packet_size++] = c->skill[i];
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, true);
    else
        multi_send_packet(data, packet_size, NULL, true);
}

// 0x480c60  Confidence: exact (VC6 byte match, match29; was likely).
void mp_handle_character_skills(ubyte *data, void *from)
{
    Character *c = (Character *)mp_obj_from_net_handle(*(int *)data);
    int i;

    if (!c)
        return;
    int offset = 4;
    for (i = 0; i < MP_NUM_SKILL_BYTES; i++)
        c->skill[i] = data[offset++];
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}

// 0x480cc0  Confidence: exact (VC6 byte match). Host: SET_STS {handle, status, ms, spell, value, source}.
void mp_broadcast_status_add(Character *c, int status, int ms, int spell, float value, int source)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;
    float f;
    int net_source;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_SET_STS);
    MP_ADD_DATA(c->net_handle);
    MP_ADD_DATA(status);
    MP_ADD_DATA(ms);
    MP_ADD_DATA(spell);
    f = value;
    MP_ADD_DATA(f);
    net_source = mp_handle_to_net(source);
    MP_ADD_DATA(net_source);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x480d70  Confidence: likely.
void mp_handle_status_add(ubyte *data, void *from)
{
    Character *c;
    int status, ms, spell, source;
    float value;

    if (!MP_IS_CLIENT())
        return;
    c = (Character *)mp_obj_from_net_handle(((int *)data)[0]);
    if (!c)
        return;
    value = ((float *)data)[4];
    ms = ((int *)data)[2];
    spell = ((int *)data)[3];
    status = ((int *)data)[1];
    source = ((int *)data)[5];
    source = mp_local_handle_from_net(source);
    c->status_effect_add(status, ms, spell, value, source, false);
}

// 0x480de0  Confidence: exact (VC6 byte match). Host: CLR_STA {handle, status, restore_fx}.
void mp_broadcast_status_cure(Character *c, int status, bool restore_fx)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_CLR_STA);
    MP_ADD_DATA(c->net_handle);
    MP_ADD_DATA(status);
    MP_ADD_DATA(restore_fx);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x480e60  Confidence: exact (VC6 byte match, match29; was likely).
void mp_handle_status_cure(ubyte *data, void *from)
{
    Character *c;

    if (!MP_IS_CLIENT())
        return;
    c = (Character *)mp_obj_from_net_handle(((int *)data)[0]);
    if (c) {
        int status;
        memcpy(&status, data + 4, 4);
        c->status_effect_cure(status, false, data[8] != 0);
    }
}

// 0x480eb0  Confidence: exact (VC6 byte match). DISABLD {handle, kind, the knock-down time for kind 0}: the host
// broadcasts, a client sends to the host.
void mp_broadcast_ai_disable(Character *c, int kind, int ms)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_DISABLD);
    MP_ADD_DATA(c->net_handle);
    data[packet_size++] = (ubyte)kind;
    if (kind == 0) {
        ushort t = (ushort)ms;
        MP_ADD_DATA(t);
    }
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, true);
    else
        multi_send_packet(data, packet_size, NULL, false);
}

// 0x480f60  Confidence: likely.
void mp_handle_ai_disable(ubyte *data, void *from)
{
    Character *c = (Character *)mp_obj_from_net_handle(*(int *)data);

    if (!c)
        return;
    switch (data[4]) {
    case 0:
        {
            ushort t;
            memcpy(&t, data + 5, 2);
            c->ai.knock_down_local(t);
        }
        break;
    case 1:
        c->ai.disable_2f_local();
        break;
    }
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}

// 0x480fd0  Confidence: exact (VC6 byte match). RESTART {player id}.
void mp_send_respawn(int id)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_RESTART);
    MP_ADD_DATA(id);
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet(data, packet_size, true);
    else
        multi_send_packet(data, packet_size, NULL, true);
}

// 0x481050  Confidence: exact (VC6 byte match, match29; was likely) (the id read with memcpy, D8302 / D8800).
void mp_handle_respawn(ubyte *data, void *from)
{
    int id;
    memcpy(&id, data, 4);
    mp_respawn_player(id);
    if (MP_IS_HOST())
        mp_forward_cur_packet(from);
}

// 0x481090  Confidence: exact (VC6 byte match). The player's character comes back at its party start with full AP, the
// death penalty applied; the local player's camera and HUD follow it again. An unknown id resurrects a
// NULL character (latent).
void mp_respawn_player(int id)
{
    mp_player *p = mp_player_find_by_id(id);
    Character *c = p ? living_from_handle(p->object->handle) : NULL;

    c->resurrect();
    c->ap = (short)c->get_max_stat(1);
    c->place_at_party_start(id);
    c->ai.goals_clear();
    character_xp_death_penalty(c);
    if (id == mp_local_player->id) {
        game_camera.set_target(c);
        game_camera.resume_follow();
        game_camera.sync_yaw_to_target();
        game_camera.snap_to_target(true, true);
        hud_mode_reset();
    }
}

// 0x481130  Confidence: exact. hud_confirm_quit's respawn.
void mp_respawn_and_notify(int id)
{
    mp_respawn_player(id);
    mp_send_respawn(id);
}

// 0x481150  Confidence: exact (VC6 byte match, match29; was likely) (the handle read with memcpy, D8302 / D8800). Host: FORCEPR {handle}: AI flag 0x400 and the frame time.
void mp_handle_forcepr(ubyte *data, void *from)
{
    Character *c;
    int h;

    if (!MP_IS_HOST())
        return;
    memcpy(&h, data, 4);
    c = living_from_handle(h);
    if (c) {
        c->ai.flags |= 0x400;
        c->frametime = reported_frametime;
    }
}
