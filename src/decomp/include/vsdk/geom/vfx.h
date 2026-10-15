// vsdk/geom/vfx.h - effect bases as the effect code sees them (vsdk\geom in sum.symbols, 0x536bc0-0x536e50
// and ptr_relocate 0x564d50; file names not known). Not original source.
#ifndef VSDK_GEOM_VFX_H
#define VSDK_GEOM_VFX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"                 // vfx_light_desc::pos (decomp48)

// The loaded effect base ("vfx base") an effect id resolves to. Only +0x1c is written by effect_mem.cpp.
class vfx_base {
public:
    void load_bitmaps(int flag);              // 0x536e50 vfx_base__load_bitmaps
    void relocate_node(char *a, char *b);     // 0x536bc0 vfx_node_relocate (argument meaning unsure)
    char _unk00[0x12];
    short num_frames;                         // +0x12 (cast_effect_track_add: frames / 15 * 1.5 s; decomp39)
    char _unk14[0x08];
    int category;                             // +0x1c copied from the base record's category
    int _unk20;                               // +0x20
    char *light_tracks;                       // +0x24 0x70 bytes per light of an instance (ui_3d_begin; decomp48)
};

// One light of a vfx instance (decomp48, ui_3d_begin 0x412400; names ours). An instance's lights form a
// circular list from vfx_instance::lights through next.
class vfx_light_desc {
public:
    // Samples the light's tracks (position, size, colour) at time t. track = the base's light_tracks +
    // 0x70 * light index (thiscall, ret 8; the body reads track +0xc as a frame count).
    void sample(void *track, float t);        // 0x51fdc0 vfx_light_desc__sample
    ubyte _unk00[0x08];
    vector3 pos;                              // +0x08 offset from the instance's origin
    ubyte _unk14[0x0c];
    ubyte r, g, b;                            // +0x20 colour, 0..255
    ubyte _pad23;
    short range;                              // +0x24 range * 256
    short _pad26;
    vfx_light_desc *next;                     // +0x28
};

void ptr_relocate(void *p, char *base);       // 0x564d50 (*p += base unless NULL)

// A vfx instance (the effect object +0xa0). Members spells.cpp calls (decomp23); argument order from
// the call sites (sum.symbols lists the node index first for get_node_transform).
class vector3;
class matrix;
class vfx_instance {
public:
    void process(int a, const vector3 *pos, const matrix *orient);   // 0x536fb0 vfx_instance_process (callers pass 0)
    void get_node_transform(vector3 *pos, matrix *orient, int node); // 0x537840
    // The world transform of a node, the vector and matrix passed by value (spell_probe_ground_point).
    void get_node_world_transform(int node, vector3 pos, matrix orient, int a, int b, int c); // 0x537a10
    int  find_tag(const char *name);                                 // 0x537f10, -1 (">>> FAILED TO GET TAG %s")
    int  get_tag_transform(int tag, int a, int b);                   // 0x537f70 (result unused by spells.cpp)
    unsigned int id_bits;                                            // +0x00 effect id << 9 (cast_effect_track_add; decomp39);
                                                                     // interface_frame clears bit 2 of the UI lights' instance (decomp48)
    ubyte _unk04[0x18];
    vfx_light_desc *lights;                                          // +0x1c first light, NULL none (ui_3d_begin; decomp48)
    float get_time_left();                                           // 0x538510 (decomp37)
};

// A light attached to an object (0x98 bytes; slots at 0x2cb45d0, attached_light_create / _free). The
// constructor is inline (decomp28, D805): levelrender.cpp's array initialiser loops over reset() with
// no constructor call, while init_attached_lights calls the out-of-line copy attached_light__ctor.
// (pos, 1.0, 0, attenuation, r, g, b): the push order of living_entity_init (decomp37). Index or -1.
int  attached_light_create(const vector3 *pos, float a, int b, float atten, float r, float g, float bl);   // 0x520160
void attached_light_free(int index);                 // 0x5201f0

class attached_light {
public:
    attached_light() { reset(); }
    void reset();                             // 0x520210 attached_light__reset (identity orientation, defaults)
    ubyte type;                               // +0x00 -1 free slot
    ubyte _unk01[0x97];
};

class quaternion;
// The camera shake of the running effects (state_game_frame adds it to the view; decomp42).
void vfx_get_camera(vector3 *pos, quaternion *rot);   // 0x5383e0

#endif
