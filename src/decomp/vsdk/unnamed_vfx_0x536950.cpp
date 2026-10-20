// unnamed_vfx_0x536950.cpp - Sum.exe v1.40, code 0x536950-0x538647: the vfx object (vsdk\geom in
// sum.symbols): vfx base relocation, bitmaps and tag lookup, the per-frame update and drawing of a vfx
// instance, its tag binding / placement / visibility, the effect camera and the effect render state.
// The file name is a placeholder: no path string (asm.mjs strings 0x536950 0x538650 gives "$vis" and
// ">>> FAILED TO GET TAG %s\n" only), no strings.tbl block, and the Summoner 2 splits name no unit with
// these functions.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (docs/sum-exe/source-map.md "gr_direct3d.cpp is two objects", DECISIONS.md D1601): the
// initialiser table runs 0x5889cc (the Direct3D state file), 0x5889d0-0x5889d8 (this object: the vertex
// pool, the camera position and orientation), then 0x5889dc-0x5889e4 (gr_direct3d.cpp's parameters);
// .data 0x5a9cfc-0x5a9d23 (vfx_render_bitmap, "$vis", the tag message), .bss 0x2d90ee8-0x2dbfd1b.
//
// Build flags: no inline expansion (the vector, matrix and quaternion members, float_max and the
// vfx_instance list helpers are called; the helpers are the COMDATs 0x538590-0x538647 at the end).
// strcpy / strcmp are the intrinsic forms.

#include <stdio.h>
#include <string.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

// ---- .bss / .data ---------------------------------------------------------------------------------------
// The three initialisers run in definition order (table 0x5889d0, 0x5889d4, 0x5889d8), so the pool is
// defined first although its .bss address is after the camera's (the .bss order is the compiler's).
// 0x536950 init_gr_d3d_vertex_buf (table 0x5889d0; jmp 0x536960: `vector constructor iterator`
// (0x404880) with gr__Vertex__Vertex 0x5081c0, 4000 x 0x30). The original gr_vertex has a default
// constructor; gr.h does not declare it (D1605, still open), so this definition has no initialiser here
// (D4703). Confidence: likely.
gr_vertex gr_vertex_pool[4000];                      // 0x2d90f10
// 0x536980 init_vfx_camera_pos (table 0x5889d4; jmp 0x536990: vector3() 0x505530). Confidence: exact.
vector3 vfx_camera_pos;                              // 0x2d90ee8
// 0x5369a0 init_vfx_camera_orient (table 0x5889d8; jmp 0x5369b0: quaternion(), folded 0x505530). Confidence: exact.
quaternion vfx_camera_orient;                        // 0x2d90ef8
int vfx_render_blend;                                // 0x2d90f08 (label ours)
void *vfx_active_instance;                           // 0x2dbfd10
ubyte vfx_render_alpha;                              // 0x2dbfd14
ubyte vfx_render_value_b;                            // 0x2dbfd15 (label ours)
int vfx_render_flag;                                 // 0x2dbfd18 (label ours)
int vfx_render_bitmap = -1;                          // 0x5a9cfc (label ours)

// 0x5369c0  Confidence: exact (the store order). sum.symbols: vfx_instance__clear (D4701).
void vfx_base::clear()
{
    flags &= 0xe000;
    num_nodes = 0;
    num_lights = 0;
    num_emitters = 0;
    num_desc21c = 0;
    num_frames = 0;
    num_rec2c = 0;
    num_rec4c = 0;
    num_cameras = 0;
    num_tags = 0;
    _unk40 = NULL;
}

// 0x5369f0  Confidence: exact. Frees the lights' attached lights and the vif records, and drops the
// effect camera when this instance owns it.
void vfx_instance::release_nodes()
{
    vfx_light_desc *light;
    vfx_inst21c *rec, *next;
    vfx_base *base;
    int i;

    light = lights;
    while (light != NULL) {
        light->free_light();
        light = light->next;
        if (light == lights)
            break;
    }
    if (insts21c != NULL) {
        base = get_base();
        next = insts21c;
        i = 0;
        do {
            rec = next;
            next = next->next;
            vif_record_release(rec, &base->descs21c[i]);
            i++;
        } while (next != insts21c);
        insts21c = NULL;
    }
    if (vfx_active_instance == this)
        vfx_active_instance = NULL;
}

// 0x536a70  Confidence: likely. The instance starts at the base's last frame with the done and lights
// bits set (reset_flags starts it); the vif records are made here.
int vfx_instance::init_nodes(vfx_base *base, bool no_light, bool level)
{
    fx_rec98 *node;
    vfx_light_desc *light;
    fx_recb0 *em;
    fx_rec2c *d;
    vfx_inst21c *rec;
    int i;

    time = base->num_frames;
    elapsed = time * (1.0f / 15.0f);
    node = nodes;
    i = 0;
    while (node != NULL) {
        node->init(&base->node_descs[i]);
        node = node->next;
        i++;
        if (node == nodes)
            break;
    }
    light = lights;
    i = 0;
    while (light != NULL) {
        light->create_light(&((vfx_light_track *)base->light_tracks)[i], no_light);
        light = light->next;
        i++;
        if (light == lights)
            break;
    }
    em = emitters;
    i = 0;
    while (em != NULL) {
        em->init(&base->emitters[i]);
        em = em->next;
        i++;
        if (em == emitters)
            break;
    }
    d = rec2c_list;
    while (d != NULL) {
        d->reset();
        d = d->next;
        if (d == rec2c_list)
            break;
    }
    id_bits = (id_bits & ~0xb) | 0x24;
    for (i = 0; i < base->num_desc21c; i++) {
        rec = vif_instantiate_level(&base->descs21c[i], level);
        if (rec != NULL)
            plist_append(insts21c, rec);
    }
    return 0;
}

// 0x536bc0  Confidence: exact. Names against the header base, table pointers against the base; then
// every record; the counts are capped afterwards.
void vfx_base::relocate_node(char *header_base, char *base)
{
    int i;

    reloc_ptr(&name, header_base);
    ptr_relocate(&node_descs, base);
    ptr_relocate(&light_tracks, base);
    ptr_relocate(&emitters, base);
    ptr_relocate(&descs21c, base);
    ptr_relocate(&dummies, base);
    ptr_relocate(&rec4c_table, base);
    ptr_relocate(&cameras, base);
    ptr_relocate(&tags, base);
    ptr_relocate(&_unk40, base);
    ptr_relocate(&materials, base);
    for (i = 0; i < num_nodes; i++)
        node_descs[i].relocate(header_base, base);
    for (i = 0; i < num_lights; i++)
        ((vfx_light_track *)light_tracks)[i].relocate(header_base, base);
    for (i = 0; i < num_emitters; i++)
        emitters[i].relocate(header_base, base);
    for (i = 0; i < num_desc21c; i++)
        descs21c[i].relocate(header_base);
    for (i = 0; i < num_rec2c; i++)
        dummies[i].relocate(header_base, base);
    for (i = 0; i < num_rec4c; i++)
        ((vfx_force_desc *)rec4c_table)[i].relocate(header_base, base);
    for (i = 0; i < num_cameras; i++)
        cameras[i].relocate(header_base, base);
    for (i = 0; i < num_tags; i++)
        tags[i].relocate(header_base, base);
    for (i = 0; i < num_materials; i++)
        materials[i].relocate(header_base, base);
    if (num_nodes > 300)
        num_nodes = 300;
    if (num_lights > 15)
        num_lights = 15;
    if (num_emitters > 90)
        num_emitters = 90;
    if (num_rec2c > 50)
        num_rec2c = 50;
    if (num_rec4c > 100)
        num_rec4c = 100;
}

// 0x536e50  Confidence: exact.
void vfx_base::load_bitmaps(int flag)
{
    int i;

    for (i = 0; i < num_nodes; i++)
        node_descs[i].load_bitmap(flag);
    for (i = 0; i < num_materials; i++)
        materials[i].load_bitmaps(flag);
    for (i = 0; i < num_desc21c; i++)
        descs21c[i].load_bitmap(flag);
}

// 0x536ed0  Confidence: exact (the mesh flags through the 16-bit vis_flags view, a tag pointer, the
// reference indexed: D11701). Hides every mesh, emitter and vif record, then shows the members of
// the groups whose names start with "$vis". No caller found.
void vfx_base::init_vis_flags()
{
    vfx_tag_ref *e;
    int i, j;

    for (i = 0; i < num_nodes; i++)
        node_descs[i].vis_flags |= 0x400;
    for (i = 0; i < num_emitters; i++)
        emitters[i].vis_flags |= 0x800000;
    for (i = 0; i < num_desc21c; i++)
        descs21c[i].vis_flags |= 0x80;
    for (i = 0; i < num_tags; i++) {
        vfx_base_tag *t = &tags[i];
        if (_strnicmp(t->name, "$vis", 4) == 0) {
            for (j = 0; j < t->count; j++) {
                e = &t->refs[j];
                switch (e->kind) {
                case 0:
                    ((vfx_node_desc *)e->rec)->vis_flags &= ~0x400;
                    break;
                case 1:
                    ((vfx_emitter_desc *)e->rec)->vis_flags &= ~0x800000;
                    break;
                case 2:
                    ((vfx_desc21c *)e->rec)->vis_flags &= ~0x80;
                    break;
                }
            }
        }
    }
}

// 0x536fb0  Confidence: likely. Advances the instance: past the last frame a looping instance wraps
// (bit 0x10 for the frame), another one stops. A stopped instance only keeps the first emitter that
// still has live particles going (and its vif records) and is done when none has; a running one moves
// its lights and dummies and takes the effect camera when no other instance holds it. The emitters
// of a running instance are updated by render. The second num_cameras test is in the binary
// (0x5371d5 and 0x5371dc).
bool vfx_instance::process(float frametime, const vector3 *pos, const matrix *orient)
{
    vfx_base *base;
    vfx_light_desc *light;
    fx_recb0 *em;
    fx_rec2c *d;
    vfx_inst21c *rec;
    float frames;
    int i;

    base = get_base();
    if (base == NULL || (base->flags & 0x4000) || (id_bits & 4))
        return false;
    elapsed += frametime;
    time = elapsed * 15.0f;
    frames = base->num_frames;
    if (time >= frames) {
        if (id_bits & 2) {
            frames = frames * (1.0f / 15.0f);
            while (elapsed >= frames)
                elapsed -= frames;
            id_bits |= 0x10;
            time = elapsed * 15.0f;
        } else {
            stop();
        }
    } else {
        id_bits &= ~0x10;
    }

    if (id_bits & 1) {
        if (vfx_active_instance == this)
            vfx_active_instance = NULL;
        em = emitters;
        i = 0;
        if (em != NULL) {
            while (1) {
                if (em->num_live > 0) {
                    em->update(&base->emitters[i], time, pos, orient, (id_bits >> 4) & 1);
                rec = insts21c;
                i = 0;
                while (rec != NULL) {
                    if (vif_record_num_live(rec))
                        vif_emitter_update(rec, &base->descs21c[i], 0, pos, orient, _unk0c);
                    rec = rec->next;
                    i++;
                    if (rec == insts21c)
                        break;
                }
                return true;
                }
                em = em->next;
                i++;
                if (em == emitters)
                    break;
                if (em == NULL)
                    break;
            }
        }
        id_bits |= 4;
        time = base->num_frames;
        return false;
    }

    if (id_bits & 0x20) {
        light = lights;
        i = 0;
        while (light != NULL) {
            light->update_light(&((vfx_light_track *)base->light_tracks)[i], time, pos, orient);
            light = light->next;
            i++;
            if (light == lights)
                break;
        }
    }
    d = rec2c_list;
    i = 0;
    while (d != NULL) {
        d->update(&base->dummies[i], time);
        d = d->next;
        i++;
        if (d == rec2c_list)
            break;
    }
    if (vfx_active_instance == NULL || vfx_active_instance == this) {
        vfx_active_instance = NULL;
        if (base->num_cameras > 0) {
            for (i = 0; i < base->num_cameras; i++) {
                if (base->cameras[i].get_transform(time, &vfx_camera_pos, &vfx_camera_orient)) {
                    vfx_active_instance = this;
                    break;
                }
            }
        }
    }
    return true;
}

// 0x537230  Confidence: exact.
void vfx_instance::stop()
{
    fx_recb0 *em;

    if (id_bits & 1)
        return;
    id_bits |= 1;
    em = emitters;
    while (em != NULL) {
        em->flags |= 1;
        em = em->next;
        if (em == emitters)
            break;
    }
}

// 0x537270  Confidence: exact.
bool vfx_instance::is_stopped()
{
    return id_bits & 1;
}

// 0x537280  Confidence: exact.
void vfx_instance::set_stopped(bool stopped)
{
    id_bits = (id_bits & ~1) | (stopped & 1);
}

// 0x5372a0  Confidence: exact (the emitter loop's shape). The flat tag index runs over the meshes, the
// emitters' sub-tags, the vif records, the lights, the dummies, then the emitters themselves; `sub` is
// written for the emitter sub-tags only.
void vfx_base::decode_tag(int index, int *kind, int *list, int *sub)
{
    int i;

    if (index < num_nodes) {
        *kind = VFX_TAG_MESH;
        *list = index;
        return;
    }
    index -= num_nodes;
    i = 0;
    if (num_emitters != 0) {
        while (index >= emitters[i].num_particles) {
            index -= emitters[i].num_particles;
            if (++i == num_emitters)
                break;
        }
        if (i < num_emitters) {
            *kind = VFX_TAG_EMITTER_PART;
            *list = i;
            *sub = index;
            return;
        }
    }
    if (index < num_desc21c) {
        *list = index;
        *kind = VFX_TAG_VIF;
        return;
    }
    index -= num_desc21c;
    if (index < num_lights) {
        *list = index;
        *kind = VFX_TAG_LIGHT;
        return;
    }
    index -= num_lights;
    if (index < num_rec2c) {
        *list = index;
        *kind = VFX_TAG_DUMMY;
        return;
    }
    *kind = VFX_TAG_EMITTER;
    *list = index - num_rec2c;
}

// 0x5373a0  Confidence: exact (jump table 0x537470; cases 1 and 4 share the emitter code). The name of
// the node's parent; fx_bind_character_tags compares it with "Scene Root".
const char *vfx_instance::get_node_name(int node)
{
    vfx_base *base;
    int kind, list, sub;

    base = get_base();
    base->decode_tag(node, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH:
        return base->node_descs[list].parent;
    case VFX_TAG_LIGHT:
        return (const char *)((vfx_light_track *)base->light_tracks)[list].header.link;
    case VFX_TAG_EMITTER_PART:
    case VFX_TAG_EMITTER:
        return base->emitters[list].parent;
    case VFX_TAG_VIF:
        return base->descs21c[list].parent;
    case VFX_TAG_DUMMY:
        return base->dummies[list].parent;
    }
    return NULL;
}

// 0x537490  Confidence: exact (jump table 0x53755c).
const char *vfx_instance::get_tag_name(int tag)
{
    vfx_base *base;
    int kind, list, sub;

    base = get_base();
    base->decode_tag(tag, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH:
        return base->node_descs[list].name;
    case VFX_TAG_LIGHT:
        return (const char *)((vfx_light_track *)base->light_tracks)[list].header.name;
    case VFX_TAG_EMITTER_PART:
    case VFX_TAG_EMITTER:
        return base->emitters[list].name;
    case VFX_TAG_VIF:
        return base->descs21c[list].name;
    case VFX_TAG_DUMMY:
        return base->dummies[list].name;
    }
    return NULL;
}

// 0x537580  Confidence: exact (jump table 0x5376e0). Binds the node to a mesh tag of a model.
void vfx_instance::bind_node(int node, character_instance *ci, int tag)
{
    vfx_base *base;
    int kind, list, sub;
    fx_rec98 *m;
    vfx_light_desc *l;
    fx_recb0 *e;
    vfx_inst21c *v;
    fx_rec2c *d;

    base = get_base();
    if (base == NULL)
        return;
    base->decode_tag(node, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH:
        m = list18_item(list);
        if (m != NULL) {
            m->bound = 1;
            m->ci = ci;
            m->tag = tag;
        }
        break;
    case VFX_TAG_LIGHT:
        l = list1c_item(list);
        if (l != NULL) {
            l->on_bone = 1;
            l->owner = ci;
            l->bone = tag;
        }
        break;
    case VFX_TAG_EMITTER_PART:
    case VFX_TAG_EMITTER:
        e = list20_item(list);
        if (e != NULL) {
            e->bound = 1;
            e->ci = ci;
            e->tag = tag;
        }
        break;
    case VFX_TAG_VIF:
        v = list24_item(list);
        if (v != NULL) {
            v->flags |= 4;
            v->ci = ci;
            v->tag = tag;
        }
        break;
    case VFX_TAG_DUMMY:
        d = list28_item(list);
        if (d != NULL) {
            d->bound = 1;
            d->ci = ci;
            d->tag = tag;
        }
        break;
    }
}

// 0x537700  Confidence: exact (jump table 0x5377e4).
bool vfx_instance::node_can_bind(int node)
{
    vfx_base *base;
    int kind, list, sub;

    base = get_base();
    base->decode_tag(node, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH: {
        bool r = base->node_descs[list].bind_flags & 1;
        return r;
    }
    case VFX_TAG_LIGHT: {
        bool r = ((vfx_light_track *)base->light_tracks)[list].header._bit0;
        return r;
    }
    case VFX_TAG_EMITTER_PART:
    case VFX_TAG_EMITTER: {
        bool r = base->emitters[list]._bit31;
        return r;
    }
    case VFX_TAG_VIF: {
        bool r = (base->descs21c[list].vis_flags >> 9) & 1;
        return r;
    }
    case VFX_TAG_DUMMY:
        return base->dummies[list].can_bind;
    }
    return false;
}

// 0x537800  Confidence: exact (the bit merge).
void vfx_instance::reset_flags(bool a, bool b)
{
    set_stopped(false);
    id_bits = (id_bits & ~0xe) | ((((b & 1) << 2) | (a & 1)) << 1);
    time = 0.0f;
    elapsed = 0.0f;
}

// 0x537840  Confidence: exact (jump table 0x5379f0). The node's transform relative to the instance;
// the first mesh's when the node has none (an emitter sub-tag, a negative index, a missing record).
void vfx_instance::get_node_transform(vector3 *pos, matrix *orient, int node)
{
    int kind, list, sub;
    fx_rec98 *m;
    vfx_light_desc *l;
    fx_rec2c *d;
    fx_recb0 *e;
    vfx_inst21c *v;

    if (node >= 0) {
        get_base()->decode_tag(node, &kind, &list, &sub);
        switch (kind) {
        case VFX_TAG_MESH:
            m = list18_item(list);
            if (m != NULL) {
                *pos = m->v50;
                *orient = m->m5c;
                return;
            }
            break;
        case VFX_TAG_LIGHT:
            l = list1c_item(list);
            if (l != NULL) {
                *pos = l->pos;
                orient->make_identity();
                return;
            }
            break;
        case VFX_TAG_DUMMY:
            d = list28_item(list);
            if (d != NULL) {
                *pos = d->v08;
                d->q14.to_matrix_b(orient);
                return;
            }
            break;
        case VFX_TAG_EMITTER:
            e = list20_item(list);
            if (e != NULL) {
                *pos = e->v70;
                e->q7c.to_matrix_b(orient);
                return;
            }
            break;
        case VFX_TAG_VIF:
            v = list24_item(list);
            if (v != NULL) {
                *pos = v->pos;
                *orient = v->orient;
                return;
            }
            break;
        }
    }
    if (nodes == NULL) {
        pos->zero();
        orient->make_identity();
        return;
    }
    *pos = nodes->v50;
    *orient = nodes->m5c;
}

// 0x537a10  Confidence: likely (jump table 0x537ef8; the name buffer is 0x44 bytes of the frame). Places
// a node at pos / orient; with c, every tag whose parent is this node follows it by the same move (a
// mesh child gets the move itself, the others their own position moved; meshes have no recursion).
// The vif case measures from the descriptor's position, before the record test.
void vfx_instance::get_node_world_transform(int node, vector3 pos, matrix orient, int a, int b, bool c)
{
    vector3 delta;
    vfx_base *base;
    int kind, list, sub, i, n;
    char name[68];
    fx_rec98 *m;
    vfx_light_desc *l;
    fx_recb0 *e;
    vfx_inst21c *v;
    fx_rec2c *d;

    base = get_base();
    base->decode_tag(node, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH:
        m = list18_item(list);
        strcpy(name, base->node_descs[list].name);
        if (m == NULL)
            return;
        delta = m->v20;
        m->v20 = pos;
        m->m2c = orient;
        m->placed = 1;
        return;
    case VFX_TAG_EMITTER_PART:
        return;
    case VFX_TAG_LIGHT:
        l = list1c_item(list);
        strcpy(name, (const char *)((vfx_light_track *)base->light_tracks)[list].header.name);
        if (l == NULL)
            break;
        delta = l->pos - pos;
        l->pos = pos;
        l->fixed = 1;
        break;
    case VFX_TAG_EMITTER:
        e = list20_item(list);
        strcpy(name, base->emitters[list].name);
        if (e == NULL)
            break;
        delta = e->v70 - pos;
        e->v70 = pos;
        e->q7c = quaternion(&orient);
        e->placed = 1;
        break;
    case VFX_TAG_VIF:
        v = list24_item(list);
        strcpy(name, base->descs21c[list].name);
        delta = base->descs21c[list].pos - pos;
        if (v == NULL)
            break;
        v->pos = pos;
        v->orient = orient;
        v->flags |= 2;
        break;
    case VFX_TAG_DUMMY:
        d = list28_item(list);
        strcpy(name, base->dummies[list].name);
        if (d == NULL)
            break;
        delta = d->v08 - pos;
        d->v08 = pos;
        d->q14 = quaternion(&orient);
        d->placed = 1;
        break;
    }
    if (c == true) {
        n = base->get_num_tags();
        for (i = 0; i < n; i++) {
            if (i == node)
                continue;
            if (strcmp(name, get_node_name(i)) != 0)
                continue;
            vector3 cpos;
            matrix corient;
            get_node_transform(&cpos, &corient, i);
            base->decode_tag(i, &kind, &list, &sub);
            if (kind == VFX_TAG_MESH) {
                get_node_world_transform(i, -delta, corient, a, b, true);
            } else if (kind != VFX_TAG_EMITTER_PART) {
                cpos -= delta;
                get_node_world_transform(i, cpos, corient, a, b, true);
            }
        }
    }
}

// 0x537f10  Confidence: exact.
int vfx_instance::find_tag(const char *name)
{
    int i, n;
    const char *tag;

    n = get_base()->get_num_tags();
    for (i = 0; i < n; i++) {
        tag = get_tag_name(i);
        if (_stricmp(tag, name) == 0)
            return i;
    }
    printf(">>> FAILED TO GET TAG %s\n", name);
    return -1;
}

// 0x537f70  Confidence: exact (jump table 0x538248). Not a transform: hides (a) or shows the tag, and
// with b every tag whose parent it is; hiding a light frees its attached light. sum.symbols' name is kept.
void vfx_instance::get_tag_transform(int tag, bool a, bool b)
{
    vfx_base *base;
    int kind, list, sub, i, n;
    char name[68];
    fx_rec98 *m;
    vfx_light_desc *l;
    fx_recb0 *e;
    vfx_inst21c *v;
    fx_rec2c *d;

    base = get_base();
    base->decode_tag(tag, &kind, &list, &sub);
    switch (kind) {
    case VFX_TAG_MESH:
        strcpy(name, base->node_descs[list].name);
        m = list18_item(list);
        if (m != NULL)
            m->hidden = a;
        break;
    case VFX_TAG_LIGHT:
        l = list1c_item(list);
        strcpy(name, (const char *)((vfx_light_track *)base->light_tracks)[list].header.name);
        if (l != NULL) {
            l->on = !a;
            if (a == true)
                attached_light_free(l->slot);
        }
        break;
    case VFX_TAG_EMITTER:
        strcpy(name, base->emitters[list].name);
        e = list20_item(list);
        if (e != NULL)
            e->hidden = a;
        break;
    case VFX_TAG_VIF:
        strcpy(name, base->descs21c[list].name);
        v = list24_item(list);
        if (v != NULL) {
            if (a)
                v->flags |= 8;
            else
                v->flags &= ~8;
        }
        break;
    case VFX_TAG_DUMMY:
        strcpy(name, base->dummies[list].name);
        d = list28_item(list);
        if (d != NULL)
            d->hidden = a;
        break;
    }
    if (b == true) {
        n = base->get_num_tags();
        for (i = 0; i < n; i++) {
            if (i == tag)
                continue;
            if (strcmp(name, get_node_name(i)) == 0)
                get_tag_transform(i, a, true);
        }
    }
}

// 0x538260  Confidence: exact (D6304: the mesh case is the else branch; the index is read unsigned, as
// render_scene casts it). A done instance is drawn only with bit 8; a stopped mesh likewise. An
// emitter is updated here before it is drawn.
void vfx_instance::render_node(vfx_node_ref *ref, const vector3 *pos, const matrix *orient)
{
    vfx_base *base;
    fx_rec98 *m;
    fx_recb0 *e;

    if ((id_bits & 4) && !(id_bits & 8))
        return;
    base = get_base();
    if (ref->kind != VFX_TAG_MESH) {
        if (ref->kind == VFX_TAG_EMITTER) {
            e = (fx_recb0 *)ref->inst;
            e->update(&base->emitters[(ushort)ref->index], time, pos, orient, (id_bits >> 4) & 1);
            e->render(&base->emitters[(ushort)ref->index], time);
        }
    } else {
        if ((id_bits & 1) && !(id_bits & 8))
            return;
        m = (fx_rec98 *)ref->inst;
        m->alpha = render_alpha;
        m->render(&base->node_descs[(ushort)ref->index], time, pos, orient);
    }
}

// 0x538340  Confidence: exact (the sum's order).
int vfx_base::get_num_tags()
{
    int n, i;

    n = num_nodes;
    for (i = 0; i < num_emitters; i++)
        n += emitters[i].num_particles;
    return num_lights + num_rec2c + num_emitters + num_desc21c + n;
}

// 0x538390  Confidence: exact. No caller found.
void vfx_base::page_in_particle_bitmaps()
{
    int i, j;

    for (i = 0; i < num_materials; i++) {
        for (j = 0; j < 2; j++) {
            if (materials[i].layers[j].bitmap >= 0)
                gr_page_in(materials[i].layers[j].bitmap, 0);
        }
    }
}

// 0x5383e0  Confidence: exact.
void vfx_get_camera(vector3 *pos, quaternion *rot)
{
    if (vfx_active_instance != NULL) {
        *pos = vfx_camera_pos;
        *rot = vfx_camera_orient;
    } else {
        pos->zero();
        rot->make_identity();
    }
}

// 0x538430  Confidence: exact. sum.symbols: vfx_instance__setup_render_state (a cdecl function).
void vfx_instance__setup_render_state(ubyte *out, vfx_instance *vi)
{
    vfx_base *base;
    vfx_material *mat;

    if (vi == NULL)
        return;
    base = vi->get_base();
    if (base == NULL || base->_unk40 == NULL)
        return;
    mat = &base->materials[base->_unk40->material];
    vfx_render_alpha = (ubyte)(mat->eval_curve34(vi->time) * 255.0f);
    *out = (ubyte)(mat->eval_curve48(vi->time) * 255.0f);
    vfx_render_value_b = (ubyte)(mat->eval_curve44(vi->time) * 255.0f);
    unsigned int flags = mat->flags;
    vfx_render_bitmap = mat->layers[1].bitmap;
    vfx_render_blend = (flags >> 3) & 7;
    vfx_render_flag = (flags >> 6) & 1;
}

// 0x5384f0  Confidence: exact (vfx_render_blend is not reset).
void vfx_render_state_reset()
{
    vfx_render_bitmap = -1;
    vfx_render_alpha = 0;
    vfx_render_value_b = 0;
    vfx_render_flag = 0;
}

// 0x538510  Confidence: exact. The seconds to the base's last frame plus the longest emitter.
float vfx_instance::get_time_left()
{
    vfx_base *base;
    float t;

    if (id_bits & 4)
        return 0.0f;
    base = get_base();
    if (base == NULL)
        return 0.0f;
    t = (base->num_frames - time) * (1.0f / 15.0f) + length;
    return float_max(0.0f, t);
}

// 0x538570  Confidence: exact (an out-of-line definition here: it has no caller, yet it is in the
// binary; sum.symbols vfx_instance__ctor, D4701).
vfx_base::vfx_base()
{
    clear();
}

// ---- COMDATs at the end of the object (inline members, defined in effect_mem.h / vfx.h) -----------------
// 0x538590 vfx_instance::get_base. Confidence: exact.
// 0x5385a0 vif_record_num_live (sum.symbols vfx_get_word_72; vfx.h). Confidence: exact.
// 0x5385b0 vfx_instance::list18_item. Confidence: exact.
// 0x5385d0 vfx_instance::list1c_item (n -= 2). Confidence: exact.
// 0x5385f0 vfx_instance::list20_item. Confidence: exact.
// 0x538610 vfx_instance::list24_item. Confidence: exact.
// 0x538630 vfx_instance::list28_item. Confidence: exact.
