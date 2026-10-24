// unnamed_lights_0x51e070.cpp - Sum.exe v1.40, code 0x51e070-0x51fb08: the dynamic light table
// (vsdk\gr in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block, and
// Summoner 2 has no unit with these functions.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. The object starts with its two static initialisers (static_initializers entries 589 / 590,
// after effect_mem.cpp's 588) and ends with two COMDATs (light__light 0x51fac0, the implicit constructor,
// and float_min 0x51faf0); the attached-light unit's initialiser (entry 591) follows at 0x51fb10.
// .data 0x5a8900-0x5a890f (after effect_mem.cpp's literals, before the sound unit's "-nosound"), .bss
// 0x2c9f9a0-0x2cb45cb (after effect_mem.cpp's 0x2c9f99c, before attached_light_slots 0x2cb45d0).
//
// Build flags: like effect_mem.cpp, no inline expansion (vector3's operators, len, dot, rotated and the
// float_max / float_min COMDATs are called).

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "experimental/experimental.h"   // EXPERIMENTAL_BUILD only (HiResInterface)

// ---- .data -----------------------------------------------------------------------------------------
int lights_max_per_object = 2;                    // 0x5a8900
float light_ambient_r = 1.0f;                     // 0x5a8904
float light_ambient_g = 1.0f;                     // 0x5a8908
float light_ambient_b = 1.0f;                     // 0x5a890c

// ---- .bss ------------------------------------------------------------------------------------------
int lights_selection_counts[MAX_LIGHT_SELECTIONS];             // 0x2c9f9a0
light *lights_selection[MAX_LIGHT_SELECTIONS][MAX_LIGHTS];     // 0x2c9f9e0
// 0x51e070 init_lights (jmp 0x51e080: `vector constructor iterator` with light__light). Confidence: exact.
light lights[MAX_LIGHTS];                         // 0x2ca5de0
// 0x51e0a0 init_light_list_head (jmp 0x51e0b0: mov ecx; jmp light__light). Confidence: exact.
light light_list_head;                            // 0x2cb4520
int light_change_count;                           // 0x2cb45b4
int light_count;                                  // 0x2cb45b8
int lights_selection_depth;                       // 0x2cb45bc
bool lights_detail_flag;                          // 0x2cb45c0
bool lights_dynamic_enabled;                      // 0x2cb45c1
int lights_shade_calls;                           // 0x2cb45c4
int lights_shade_light_total;                     // 0x2cb45c8

// 0x51e0c0  Confidence: exact (memset as REP STOSD of 0x39d0 dwords; light_count = 0 written before it, VC6
// stores the constant after).
void light_reset_all()
{
    light_change_count++;
    list_init(&light_list_head);
    light_count = 0;
    memset(lights, 0, sizeof(lights));
    lights_collect_enabled();
}

// 0x51e100  Confidence: exact (a bool argument into the signed one-bit field: `and cl, 1 / movzx cx, cl`, and
// `and dh, -0x41` for the clear; D7904 amends D4006).
void light_set_all_enabled(bool on)
{
    light *l;

    for (l = light_list_head.next; l != &light_list_head; l = l->next)
        l->enabled = on;
}

// 0x51e140  Confidence: exact (the temporaries of the out-of-line operators share stack slots).
// The position (and a spot light's direction) in the space of the current gr instance.
void light_transform_to_instance(light *l)
{
    switch (l->type) {
    case LIGHT_TYPE_POINT: {
        vector3 v;
        v = l->pos - gr_light_base;
        l->inst_pos = v.rotated(gr_light_matrix);
        break;
    }
    case LIGHT_TYPE_SPOT: {
        vector3 v;
        v = l->pos - gr_light_base;
        l->inst_pos = v.rotated(gr_light_matrix);
        l->inst_dir = l->dir.rotated(gr_light_matrix);
        break;
    }
    }
}

// 0x51e230  Confidence: exact.
void light_set_ambient(float r, float g, float b)
{
    light_change_count++;
    light_ambient_r = r;
    light_ambient_g = g;
    light_ambient_b = b;
}

// 0x51e260  Confidence: exact.
void light_get_ambient(float *r, float *g, float *b)
{
    *r = light_ambient_r;
    *g = light_ambient_g;
    *b = light_ambient_b;
}

// 0x51e290  Confidence: exact. No check of light_alloc's -1: with 400 lights in use the record before
// the table (the end of lights_selection) is written (game bug, NOTES.md).
int light_create_point(const vector3 *pos, float inner, float outer, float intensity, float r,
                       float g, float b, bool dynamic)
{
    int i = light_alloc();
    light *l = light_get(i);

    l->type = LIGHT_TYPE_POINT;
    l->pos = *pos;
    l->inner = inner;
    l->r = intensity * r;
    l->g = intensity * g;
    l->b = intensity * b;
    l->outer = float_max(outer, inner + 2.0f);
    l->inner_sq = inner * inner;
    l->outer_sq = l->outer * l->outer;
    l->dynamic = dynamic;
    l->enabled = 1;
    if (!dynamic)
        light_change_count++;
    return i;
}

// 0x51e360  Confidence: exact.
light *light_get(int index)
{
    return &lights[index];
}

// 0x51e380  Confidence: exact (the loop's normal exit returns -1 directly; the >= 400 test after it
// only guards the break).
int light_alloc()
{
    int i;

    for (i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i].type == LIGHT_TYPE_NONE)
            break;
    }
    if (i >= MAX_LIGHTS)
        return -1;
    light_count++;
    list_append(&light_list_head, &lights[i]);
    return i;
}

// 0x51e3f0  Confidence: exact (D19600: the clamp stored through a one-float array, which places its fstp
// after the type store). Intensity clamped to 0..1 (the point lights are not clamped).
int light_create_spot(const vector3 *pos, const vector3 *dir, float angle_a, float angle_b, float inner,
                      float outer, float intensity, float r, float g, float b, bool dynamic, int bm,
                      const quaternion *orient)
{
    int i = light_alloc();
    light *l = light_get(i);
    float in[1];

    in[0] = float_min(float_max(intensity, 0.0f), 1.0f);
    intensity = in[0];
    l->type = LIGHT_TYPE_SPOT;
    l->pos = *pos;
    l->dir = *dir;
    l->inner = inner;
    l->r = intensity * r;
    l->g = intensity * g;
    l->b = intensity * b;
    l->outer = float_max(outer, inner + 2.0f);
    l->inner_sq = inner * inner;
    l->outer_sq = l->outer * l->outer;
    l->dynamic = dynamic;
    l->enabled = 1;
    l->bitmap = bm;
    if (bm != -1 && orient != NULL)
        orient->to_matrix_b(&l->orient);
    light_set_spot_angles(i, angle_a, float_max(angle_b, angle_a + 0.5f));
    if (!dynamic)
        light_change_count++;
    return i;
}

// 0x51e540  Confidence: exact. Unlocks the bitmap only for types 1 and 4, which this unit never makes:
// a spot light's projected bitmap (type 3, locked by light_sample_projected_bitmap) stays locked.
void light_delete(int index)
{
    light *l = light_get(index);
    void *data;
    void *palette;

    if (l == NULL || l->type == LIGHT_TYPE_NONE)
        return;
    if (!l->dynamic)
        light_change_count++;
    if ((l->type == 1 || l->type == 4) && l->bitmap != -1) {
        bm_get_data(l->bitmap, &data, &palette);
        if (data != NULL)
            bm_unlock(l->bitmap);
    }
    l->type = LIGHT_TYPE_NONE;
    list_remove(l);
    light_count--;
}

// 0x51e5e0  Confidence: exact.
void light_set_pos(int index, const vector3 *pos)
{
    light *l = light_get(index);

    l->pos = *pos;
    if (!l->dynamic)
        light_change_count++;
}

// 0x51e620  Confidence: exact.
void light_get_pos(int index, vector3 *pos)
{
    *pos = light_get(index)->pos;
}

// 0x51e650  Confidence: exact (bool argument, signed field: D7904 amends D4006).
void light_set_enabled(int index, bool on)
{
    light *l = light_get(index);

    l->enabled = on;
    if (!l->dynamic)
        light_change_count++;
}

// 0x51e690  Confidence: exact (`? true : false` for the `shr / and al, 1` of the signed field, D7904).
bool light_is_enabled(int index)
{
    return light_get(index)->enabled ? true : false;
}

// 0x51e6b0  Confidence: exact.
void light_set_radius(int index, float inner, float outer)
{
    light *l = light_get(index);

    l->inner = inner;
    l->outer = outer;
    l->inner_sq = inner * inner;
    l->outer_sq = outer * outer;
    if (!l->dynamic)
        light_change_count++;
}

// 0x51e700  Confidence: exact.
void light_get_radius(int index, float *inner, float *outer)
{
    light *l = light_get(index);

    *inner = l->inner;
    *outer = l->outer;
}

// 0x51e720  Confidence: exact.
void light_set_color(int index, float intensity, float r, float g, float b)
{
    light *l = light_get(index);

    l->r = intensity * r;
    l->g = intensity * g;
    l->b = intensity * b;
    if (!l->dynamic)
        light_change_count++;
}

// 0x51e770  Confidence: exact. The colour comes back premultiplied, with intensity 1.
void light_get_color(int index, float *intensity, float *r, float *g, float *b)
{
    light *l = light_get(index);

    *r = l->r;
    *g = l->g;
    *b = l->b;
    *intensity = 1.0f;
}

// 0x51e7b0  Confidence: exact.
void light_set_spot_dir(int index, const vector3 *dir)
{
    light *l = light_get(index);

    if (l->type == LIGHT_TYPE_SPOT) {
        l->dir = *dir;
        if (!l->dynamic)
            light_change_count++;
    }
}

// 0x51e800  Confidence: exact.
void light_get_spot_dir(int index, vector3 *dir)
{
    light *l = light_get(index);

    if (l->type == LIGHT_TYPE_SPOT) {
        *dir = l->dir;
    } else {
        dir->zero();
        dir->x = 1.0f;
    }
}

// 0x51e850  Confidence: exact (FCOS inline: cos with /Oi).
void light_set_spot_angles(int index, float a, float b)
{
    light *l = light_get(index);

    if (l->type == LIGHT_TYPE_SPOT) {
        l->angle_a = a;
        l->angle_b = b;
        l->cos_a = -(float)cos(a * 0.5f);
        l->cos_b = -(float)cos(b * 0.5f);
        if (!l->dynamic)
            light_change_count++;
    }
}

// 0x51e8c0  Confidence: exact.
void light_get_spot_angles(int index, float *a, float *b)
{
    light *l = light_get(index);

    if (l->type == LIGHT_TYPE_SPOT) {
        *a = l->angle_a;
        *b = l->angle_b;
    } else {
        *a = 0.0f;
        *b = 6.2831855f;
    }
}

// 0x51e910  Confidence: exact (the count is kept in a register and stored at the end). Selection
// level 0: every light with a type and the enabled bit; one level is in use afterwards.
void lights_collect_enabled()
{
    light *l;

    lights_selection_depth = 1;
    lights_selection_counts[0] = 0;
    for (l = light_list_head.next; l != &light_list_head; l = l->next) {
        if (l->type != LIGHT_TYPE_NONE && l->enabled)
            lights_selection[0][lights_selection_counts[0]++] = l;
    }
}

// 0x51e970  Confidence: exact (D11203). Rebuilds level 0, then pushes a level with the enabled point and spot
// lights (dynamic ones only when lights_dynamic_enabled) whose outer radius reaches pos, sorted by
// their brightest channel at pos, cut to lights_max_per_object. a and radius are not read.
int lights_select_nearest(int a, const vector3 *pos, float radius)
{
    int d;
    int i, n;
    light *l;

    lights_collect_enabled();
    int p = lights_selection_depth - 1;
    d = lights_selection_depth++;
    lights_selection_counts[d] = 0;
    n = lights_selection_counts[p];
    for (i = 0; i < n; i++) {
        l = lights_selection[p][i];
        if (!l->enabled)
            continue;
        if (!lights_dynamic_enabled && l->dynamic)
            continue;
        int type = l->type;
        if (type < LIGHT_TYPE_POINT || type > LIGHT_TYPE_SPOT)
            continue;
        if ((l->pos - *pos).len_sq() < l->outer_sq)
            lights_selection[d][lights_selection_counts[d]++] = l;
    }
    for (i = 0; i < lights_selection_counts[d]; i++) {
        l = lights_selection[d][i];
        vector3 c(0.0f, 0.0f, 0.0f);
        lights_shade_point(l, &c, pos, NULL, true);
        l->sort_key = float_max(float_max(c.x, c.y), c.z);
    }
    qsort(lights_selection[d], lights_selection_counts[d], sizeof(light *), light_compare_brightness);
    if (lights_selection_counts[d] > lights_max_per_object)
        lights_selection_counts[d] = lights_max_per_object;
    return lights_selection_counts[d];
}

// 0x51eb10  Confidence: exact. Brightest first.
int light_compare_brightness(const void *a, const void *b)
{
    light *la = *(light **)a;
    light *lb = *(light **)b;

    if (la->sort_key > lb->sort_key)
        return -1;
    if (lb->sort_key > la->sort_key)
        return 1;
    return 0;
}

// 0x51eb50  Confidence: exact.
void lights_pop_selection()
{
    lights_selection_depth--;
}

// 0x51eb60  Confidence: exact.
void lights_transform_selection()
{
    int i, n, d;

    if (gr_instance_depth) {
        d = lights_selection_depth - 1;
        n = lights_selection_counts[d];
        for (i = 0; i < n; i++)
            light_transform_to_instance(lights_selection[d][i]);
    }
}

// 0x51ebb0  Confidence: exact.
int lights_get_selection_count()
{
    return lights_selection_counts[lights_selection_depth - 1];
}

// 0x51ebc0  Confidence: exact.
void lights_get_selected_info(int i, vector3 *pos, float *inner, float *outer, float *r, float *g,
                              float *b, bool *dynamic)
{
    light *l = lights_selection[lights_selection_depth - 1][i];

    if (gr_instance_depth == 0)
        *pos = l->pos;
    else
        *pos = l->inst_pos;
    *inner = l->inner;
    *outer = l->outer;
    *r = l->r;
    *g = l->g;
    *b = l->b;
    *dynamic = l->dynamic != 0;
}

// 0x51ec50  Confidence: exact.
light *lights_get_selected(int i)
{
    return lights_selection[lights_selection_depth - 1][i];
}

// 0x51ec70  Confidence: exact (D19601: the spot case has its own dot, keeps the minimum in atten and
// takes ang_u through a one-float array). Point lights: the cosine to the normal (or 1 without one),
// full inside the inner radius, fading linearly to 0 at the outer one. Spot lights: within the cone
// (cos_b), the smaller of the distance factor and the fade between cos_a and cos_b; with a projected
// bitmap the colour is the bitmap's at the angles of the point in the light's frame instead, also
// outside the outer radius.
void lights_shade_point(light *l, vector3 *color, const vector3 *pos, const vector3 *normal, bool world)
{
    vector3 lp;
    float dot, dist, cosang, f, atten;
    float r, g, b;

    if (!world && gr_instance_depth)
        lp = l->inst_pos;
    else
        lp = l->pos;

    switch (l->type) {
    case LIGHT_TYPE_POINT: {
        vector3 d = lp - *pos;
        dot = normal == NULL ? d.len() : normal->dot(d);
        if (dot <= 0.0f)
            return;
        dist = d.len();
        if (dist > 0.0f)
            dot /= dist;
        if (dist < l->inner) {
            color->x += dot * l->r;
            color->y += dot * l->g;
            color->z += dot * l->b;
        } else if (dist < l->outer) {
            f = (1.0f - (dist - l->inner) / (l->outer - l->inner)) * dot;
            color->x += f * l->r;
            color->y += f * l->g;
            color->z += f * l->b;
        }
        break;
    }
    case LIGHT_TYPE_SPOT: {
        vector3 d = lp - *pos;
        float dot;      // D19601: the spot case's own dot (the binary gives it its own slot)
        dot = normal == NULL ? d.len() : normal->dot(d);
        if (dot <= 0.0f)
            return;
        dist = d.normalize_safe_length();
        vector3 dir;
        if (gr_instance_depth == 0)
            dir = l->dir;
        else
            dir = l->inst_dir;
        cosang = d.dot(dir);
        if (!(cosang < l->cos_b))
            return;
        r = 0.0f;
        g = 0.0f;
        b = 0.0f;
        if (dist < l->outer) {
            dot /= dist;
            if (dist < l->inner)
                atten = dot;
            else
                atten = (1.0f - (dist - l->inner) / (l->outer - l->inner)) * dot;
            f = dot;
            if (cosang > l->cos_a)
                f = (1.0f - (cosang - l->cos_a) / (l->cos_b - l->cos_a)) * dot;
            atten = float_min(atten, f);    // D19601: the minimum kept in atten
            r = atten * l->r;
            g = atten * l->g;
            b = atten * l->b;
        }
        if (l->bitmap != -1) {
            vector3 t = *pos - lp;
            vector3 v = t.unrotated(gr_light_matrix);
            v = v.rotated(l->orient);
            vector3 down(0.0f, -1.0f, 0.0f);
            vector3 h(v);
            h.z = 0.0f;
            h.normalize();
            vector3 w(v);
            w.x = 0.0f;
            w.normalize();
            float au[1];    // D19601: through a one-float array, ang_u's fstp follows the next dot's setup
            au[0] = (float)acos(down.dot(h));
            float ang_u = au[0];
            float ang_v = (float)acos(down.dot(w));
            if (h.x > 0.0f)
                ang_u = -ang_u;
            if (w.z < 0.0f)
                ang_v = -ang_v;
            float span = l->angle_b + l->angle_b;
            vector3 c = light_sample_projected_bitmap((ang_u + l->angle_b) / span,
                                                      (ang_v + l->angle_b) / span, l->bitmap);
            r = c.x;
            g = c.y;
            b = c.z;
        }
        color->x += r;
        color->y += g;
        color->z += b;
        break;
    }
    }
}

// 0x51f0d0  Confidence: exact. (u, v) wrapped onto the bitmap; locked when it has no data (and never
// unlocked here). Formats 1 and 3-7; any other leaves the colour uninitialised (game bug, NOTES.md).
vector3 light_sample_projected_bitmap(float u, float v, int bm)
{
    vector3 c;
    int w, h;
    ubyte *data;
    ubyte *palette;
    int format;
    int x, y;
    ubyte *p;
    ushort px;

#ifdef EXPERIMENTAL_BUILD
    // experimental build: HiResInterface. The pixels are read here, so the real size (bm_get_dimensions may report
    // an upscaled file's original size).
    {
        int num_pixels, num_levels;
        bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    }
#else
    bm_get_dimensions(bm, &w, &h);
#endif
    format = bm_get_data(bm, (void **)&data, (void **)&palette);
    if (data == NULL)
        format = bm_lock(bm, &data, &palette);
    x = (int)(w * u + 0.5f) % w;
    if (x < 0)
        x += w;
    y = (int)(h * v + 0.5f) % h;
    if (y < 0)
        y += h;
    switch (format) {
    case BM_FORMAT_8_PALETTED:
        p = data + y * w + x;
        c.x = palette[*p * 3] * (1.0f / 255.0f);
        c.y = palette[*p * 3 + 1] * (1.0f / 255.0f);
        c.z = palette[*p * 3 + 2] * (1.0f / 255.0f);
        break;
    case BM_FORMAT_565:
        px = ((ushort *)data)[y * w + x];
        c.x = (px >> 11) * (1.0f / 32.0f);
        c.y = ((px >> 5) & 0x3f) * (1.0f / 64.0f);
        c.z = (px & 0x1f) * (1.0f / 32.0f);
        break;
    case BM_FORMAT_4444:
        px = ((ushort *)data)[y * w + x];
        c.x = ((px >> 8) & 0xf) * (1.0f / 16.0f);
        c.y = ((px >> 4) & 0xf) * (1.0f / 16.0f);
        c.z = (px & 0xf) * (1.0f / 16.0f);
        break;
    case BM_FORMAT_1555:
        px = ((ushort *)data)[y * w + x];
        c.x = ((px >> 10) & 0x1f) * (1.0f / 32.0f);
        c.y = ((px >> 5) & 0x1f) * (1.0f / 32.0f);
        c.z = (px & 0x1f) * (1.0f / 32.0f);
        break;
    case BM_FORMAT_888:
        p = data + (y * w + x) * 3;
        c.x = p[2] * (1.0f / 255.0f);
        c.y = p[1] * (1.0f / 255.0f);
        c.z = p[0] * (1.0f / 255.0f);
        break;
    case BM_FORMAT_8888:
        p = data + (y * w + x) * 4;
        c.x = p[2] * (1.0f / 255.0f);
        c.y = p[1] * (1.0f / 255.0f);
        c.z = p[0] * (1.0f / 255.0f);
        break;
    }
    return vector3(c);
}

// 0x51f3b0  Confidence: exact (D11203). The ambient light (unless dynamic_only) plus every enabled light of the
// current selection: dynamic ones when `dynamic`, static ones unless `dynamic_only`; each channel
// clamped to 0..1. colour is not read.
void lights_shade_vertices(ubyte *r, ubyte *g, ubyte *b, const vector3 *pos, const vector3 *normal,
                           bool dynamic, bool dynamic_only, int colour)
{
    vector3 c;
    int i, n;
    light *l;

    if (dynamic_only)
        c.set(0.0f, 0.0f, 0.0f);
    else
        c.set(light_ambient_r, light_ambient_g, light_ambient_b);
    int cur = lights_selection_depth - 1;
    n = lights_selection_counts[cur];
    lights_shade_light_total += n;
    lights_shade_calls++;
    for (i = 0; i < n; i++) {
        l = lights_selection[cur][i];
        if (!l->enabled)
            continue;
        if (l->dynamic) {
            if (!dynamic)
                continue;
        } else if (dynamic_only) {
            continue;
        }
        lights_shade_point(l, &c, pos, normal, false);
    }
    c.x = float_min(float_max(c.x, 0.0f), 1.0f);
    c.y = float_min(float_max(c.y, 0.0f), 1.0f);
    c.z = float_min(float_max(c.z, 0.0f), 1.0f);
    *r = (ubyte)(c.x * 255.0f);
    *g = (ubyte)(c.y * 255.0f);
    *b = (ubyte)(c.z * 255.0f);
}

// 0x51f530  Confidence: exact.
void lights_set_max_per_object(int n)
{
    lights_max_per_object = n;
}

// 0x51f540  Confidence: exact.
void lights_set_detail_flag(bool on)
{
    lights_detail_flag = on;
}

// 0x51f550  Confidence: exact.
void lights_set_dynamic_enabled(bool on)
{
    lights_dynamic_enabled = on;
}

// 0x51f560  Confidence: likely (the order of the temporaries in the reflection expression). For each
// enabled point light of the selection (dynamic ones only when `dynamic`) within its outer radius and
// facing the normal: s = max(0, (R . E - threshold) / (1 - threshold)), R the light direction reflected
// about the normal, E the direction to the viewer; s^2 * intensity (capped at 1) inside the inner
// radius, s^2 * fade * intensity between the radii. The sum is scaled down to a maximum of 1.
bool lights_calc_point_specular_color(const vector3 *pos, const vector3 *normal, ubyte *r, ubyte *g,
                                      ubyte *b, float intensity, float threshold, bool dynamic)
{
    bool found = false;
    vector3 eye = (gr_view_pos - *pos).normalized();
    float sr, sg, sb;
    float dist, s, f, atten, m;
    int i, n;
    light *l;

    sr = 0.0f;
    sg = 0.0f;
    sb = 0.0f;
    int cur = lights_selection_depth - 1;
    n = lights_selection_counts[cur];
    lights_shade_light_total += n;
    lights_shade_calls++;
    for (i = 0; i < n; i++) {
        l = lights_selection[cur][i];
        if (!l->enabled)
            continue;
        if (l->dynamic && !dynamic)
            continue;
        vector3 lp;
        if (gr_instance_depth == 0)
            lp = l->pos;
        else
            lp = l->inst_pos;
        switch (l->type) {
        case LIGHT_TYPE_POINT:
            break;
        default:
            continue;
        }
        vector3 d = lp - *pos;
        dist = d.len();
        if (!(dist < l->outer))
            continue;
        if ((normal == NULL ? dist * 0.1f : normal->dot(d)) <= 0.0f)
            continue;
        if (dist < l->inner) {
            vector3 dn = d.normalized();
            s = ((2.0f * *normal) * normal->dot(dn) - dn).dot(eye);
            s = (s - threshold) * (1.0f / (1.0f - threshold));
            s = float_max(s, 0.0f);
            if (s == 0.0f)
                continue;
            f = float_min(s * s * intensity, 1.0f);
        } else {
            if (!(dist < l->outer))
                continue;
            atten = 1.0f - (dist - l->inner) / (l->outer - l->inner);
            vector3 dn = d.normalized();
            s = ((2.0f * *normal) * normal->dot(dn) - dn).dot(eye);
            s = (s - threshold) * (1.0f / (1.0f - threshold));
            s = float_max(s, 0.0f);
            if (s == 0.0f)
                continue;
            f = (s * s) * atten * intensity;
        }
        sr += f * l->r;
        sg += f * l->g;
        sb += f * l->b;
        found = true;
    }
    m = float_max(float_max(sr, sg), sb);
    if (m > 1.0f) {
        m = 1.0f / m;
        sr *= m;
        sg *= m;
        sb *= m;
    }
    *r = (ubyte)(sr * 255.0f);
    *g = (ubyte)(sg * 255.0f);
    *b = (ubyte)(sb * 255.0f);
    return found;
}

// 0x51f960  Confidence: likely (atan2 as the FPATAN intrinsic). u from the angle of the reflected view
// vector around y, v from its height.
void gr_env_map_uv(float *u, float *v, const vector3 *pos, const vector3 *normal)
{
    vector3 r;
    vector3 e = gr_view_pos - *pos;

    r = (2.0f * *normal) * normal->dot(e) - e;
    r.normalize_safe();
    *u = ((float)atan2(r.z, r.x) * 0.31830987f + 1.0f) * 0.5f;
    *v = (1.0f - r.y) * 0.5f;
}

// 0x51fa20  Confidence: exact (no AND 1: a bool argument; the light pointer in a local).
void light_set_dynamic(int index, bool dynamic)
{
    light *l = light_get(index);

    l->dynamic = dynamic;
}

// 0x51fa50  Confidence: exact.
bool light_is_dynamic(int index)
{
    return light_get(index)->dynamic ? true : false;
}

// 0x51fa70  Confidence: exact. Walks the first light_count records of the table, not the active list,
// so freed records in front are drawn and the last used ones may not be.
void lights_debug_draw()
{
    int i, n;

    gr_set_color(0, 255, 0, 255);
    n = light_count;
    for (i = 0; i < n; i++)
        gr_draw_sphere(&lights[i].pos, 1.0f, gr_line_mode);
}

// ---- COMDATs at the end of the object --------------------------------------------------------------
// 0x51fac0 light__light: light's implicit constructor (vectors +0xc, +0x18, +0x40, +0x4c, matrix +0x6c). Confidence: exact.
// 0x51faf0 float_min (vsdk/math/vector.h, inline). Confidence: exact.
