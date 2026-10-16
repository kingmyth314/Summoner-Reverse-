// vsdk/gr/unnamed_lights_0x51e070.h - the dynamic light table (vsdk\gr in sum.symbols; one object
// 0x51e070-0x51fb08, source src/decomp/vsdk/unnamed_lights_0x51e070.cpp; file name a placeholder, no
// path string or strings.tbl block; decomp60). Included by vsdk/gr/gr.h, which declared the first of
// these functions (decomp28, decomp48, decomp56). Not original source. Field names ours unless the
// docs (docs/sum-exe/effects.md "Lights") name them.
#ifndef VSDK_GR_UNNAMED_LIGHTS_0X51E070_H
#define VSDK_GR_UNNAMED_LIGHTS_0X51E070_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"

#define MAX_LIGHTS            400     // lights[], 0xe740 bytes cleared by light_reset_all
#define MAX_LIGHT_SELECTIONS  16      // selection levels (lights_select_nearest pushes one)

// light types (light::type; 0 = free record)
#define LIGHT_TYPE_NONE   0
#define LIGHT_TYPE_POINT  2           // light_create_point
#define LIGHT_TYPE_SPOT   3           // light_create_spot
// 1 and 4 are the types light_delete unlocks a bitmap for; nothing in this unit creates them.

// A light (0x94 bytes). The constructor is the implicit one (COMDAT 0x51fac0 light__light, at the end
// of the unit: the four vectors and the matrix).
class light {
public:
    light *next;                      // +0x00 active list (light_list_head)
    light *prev;                      // +0x04
    short type : 14;                  // +0x08 LIGHT_TYPE_* (signed field: SHL 2 / SAR 2)
    ushort enabled : 1;               //       bit 14
    short dynamic : 1;                //       bit 15: changes do not count in light_change_count (signed, D6002)
    short _pad0a;
    vector3 pos;                      // +0x0c
    vector3 dir;                      // +0x18 spot direction
    float angle_a, angle_b;           // +0x24 spot angles (radians; the full cone angles)
    float inner, outer;               // +0x2c radii
    float r, g, b;                    // +0x34 colour times intensity
    vector3 inst_pos;                 // +0x40 pos in the current instance's space (light_transform_to_instance)
    vector3 inst_dir;                 // +0x4c
    float inner_sq, outer_sq;         // +0x58
    float cos_a, cos_b;               // +0x60 -cos(angle / 2)
    int bitmap;                       // +0x68 projected bitmap of a spot light, -1 none
    matrix orient;                    // +0x6c projection orientation (quaternion::to_matrix_b)
    float sort_key;                   // +0x90 brightness at the selection point (light_compare_brightness)
};
LAYOUT_CHECK(light, sizeof(light) == 0x94);

// .data 0x5a8900-0x5a890f
extern int lights_max_per_object;                     // 0x5a8900 init 2
extern float light_ambient_r;                         // 0x5a8904 init 1.0 (light_set_ambient)
extern float light_ambient_g;                         // 0x5a8908
extern float light_ambient_b;                         // 0x5a890c
// .bss 0x2c9f9a0-0x2cb45cb
extern int lights_selection_counts[MAX_LIGHT_SELECTIONS];             // 0x2c9f9a0
extern light *lights_selection[MAX_LIGHT_SELECTIONS][MAX_LIGHTS];     // 0x2c9f9e0
extern light lights[MAX_LIGHTS];                      // 0x2ca5de0
extern light light_list_head;                         // 0x2cb4520 sentinel
extern int light_change_count;                        // 0x2cb45b4
extern int light_count;                               // 0x2cb45b8
extern int lights_selection_depth;                    // 0x2cb45bc
extern bool lights_detail_flag;                       // 0x2cb45c0 (lights_set_detail_flag; no reader found; name ours)
extern bool lights_dynamic_enabled;                   // 0x2cb45c1
extern int lights_shade_calls;                        // 0x2cb45c4 (name ours)
extern int lights_shade_light_total;                  // 0x2cb45c8 (name ours)

// The gr unit's 3D state this unit reads (vsdk\gr, defined with gr_setup_3d / gr_start_instance, not
// written yet).
extern vector3 gr_view_pos;                           // 0x26924a8
extern vector3 gr_light_base;                         // 0x26927f0
extern matrix gr_light_matrix;                        // 0x2692800
extern int gr_instance_depth;                         // 0x2692e94

void light_reset_all();                               // 0x51e0c0
void light_set_all_enabled(int on);                   // 0x51e100 bit 14 of every light's flags (+8); D4006
void light_transform_to_instance(light *l);           // 0x51e140
void light_set_ambient(float r, float g, float b);    // 0x51e230
void light_get_ambient(float *r, float *g, float *b); // 0x51e260
// (pos, inner, outer, intensity, r, g, b, dynamic); outer at least inner + 2 (decomp28; D4006).
int  light_create_point(const vector3 *pos, float inner, float outer, float intensity, float r,
                        float g, float b, bool dynamic);   // 0x51e290
light *light_get(int index);                          // 0x51e360
int  light_alloc();                                   // 0x51e380 -1 when all 400 are used
// (pos, dir, angle a, angle b, inner, outer, intensity, r, g, b, dynamic, bitmap, orientation): intensity
// clamped to 0..1, angle b at least angle a + 0.5.
int  light_create_spot(const vector3 *pos, const vector3 *dir, float angle_a, float angle_b, float inner,
                       float outer, float intensity, float r, float g, float b, bool dynamic, int bm,
                       const quaternion *orient);     // 0x51e3f0
void light_delete(int index);                         // 0x51e540
void light_set_pos(int index, const vector3 *pos);    // 0x51e5e0
void light_get_pos(int index, vector3 *pos);          // 0x51e620
void light_set_enabled(int index, int on);            // 0x51e650
bool light_is_enabled(int index);                     // 0x51e690
void light_set_radius(int index, float inner, float outer);     // 0x51e6b0
void light_get_radius(int index, float *inner, float *outer);   // 0x51e700
void light_set_color(int index, float intensity, float r, float g, float b);       // 0x51e720
void light_get_color(int index, float *intensity, float *r, float *g, float *b);   // 0x51e770 intensity 1.0
void light_set_spot_dir(int index, const vector3 *dir);         // 0x51e7b0 spot lights only
void light_get_spot_dir(int index, vector3 *dir);               // 0x51e800 (1, 0, 0) for other types
void light_set_spot_angles(int index, float a, float b);        // 0x51e850
void light_get_spot_angles(int index, float *a, float *b);      // 0x51e8c0 (0, 2 pi) for other types
void lights_collect_enabled();                        // 0x51e910 selection level 0 = the enabled lights
// decomp56 (decals): light selection around a point for per-vertex shading, (-1, pos, radius). Pushes a
// selection level of the lights whose outer radius reaches pos, brightest first, at most
// lights_max_per_object; returns their count. The first and third arguments are not read.
int  lights_select_nearest(int a, const vector3 *pos, float radius);   // 0x51e970
int  light_compare_brightness(const void *a, const void *b);// 0x51eb10 qsort, brightest first
void lights_pop_selection();                          // 0x51eb50
void lights_transform_selection();                    // 0x51eb60 (inside an instance only)
int  lights_get_selection_count();                    // 0x51ebb0
void lights_get_selected_info(int i, vector3 *pos, float *inner, float *outer, float *r, float *g,
                              float *b, bool *dynamic);   // 0x51ebc0
light *lights_get_selected(int i);                    // 0x51ec50
// (light, colour sum, pos, normal or NULL, world): adds the light's contribution at pos.
void lights_shade_point(light *l, vector3 *color, const vector3 *pos, const vector3 *normal, bool world);  // 0x51ec70
// (u, v, bitmap): the bitmap's colour at (u, v), wrapped (hidden result).
vector3 light_sample_projected_bitmap(float u, float v, int bm);   // 0x51f0d0
// decomp56: the shading writes r / g / b bytes for one point (r, g, b, point, normal, dynamic lights,
// no ambient / static lights, colour). The decals pass (.., 1, 0, colour); the colour is not read (D4006).
void lights_shade_vertices(ubyte *r, ubyte *g, ubyte *b, const vector3 *pos, const vector3 *normal,
                           bool dynamic, bool dynamic_only, int colour);   // 0x51f3b0
void lights_set_max_per_object(int n);                // 0x51f530 lights_max_per_object = n
void lights_set_detail_flag(bool on);                 // 0x51f540
void lights_set_dynamic_enabled(bool on);             // 0x51f550
// (pos, normal or NULL, r, g, b, intensity, threshold, dynamic): the specular highlight of the point
// lights; true when a light contributed.
bool lights_calc_point_specular_color(const vector3 *pos, const vector3 *normal, ubyte *r, ubyte *g,
                                      ubyte *b, float intensity, float threshold, bool dynamic);   // 0x51f560
// (u, v, pos, normal): sphere-map coordinates of the reflected view vector.
void gr_env_map_uv(float *u, float *v, const vector3 *pos, const vector3 *normal);   // 0x51f960
void light_set_dynamic(int index, bool dynamic);      // 0x51fa20
bool light_is_dynamic(int index);                     // 0x51fa50
void lights_debug_draw();                             // 0x51fa70 green spheres

#endif
