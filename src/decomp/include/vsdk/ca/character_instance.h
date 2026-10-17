// character_instance.h - Sum.exe v1.40 animated character instance (vsdk\ca\character_instance.cpp,
// 0x5194b0-0x51b6a1, COMDATs to 0x51b75f). Written from the Sum.exe disassembly. Names from
// symbols/sum.symbols; field names ours unless a symbol comment names them.
#ifndef CHARACTER_INSTANCE_H
#define CHARACTER_INSTANCE_H

#include "vsdk/ca/character.h"

#define CI_MAX_ANIMS        100       // times / weights entries (ca_character::skeletons[100])
#define CI_NUM_ANIM_SPEEDS  5
#define CI_NUM_TRIGGERS     5         // skeleton::triggers
#define CI_MAX_ACTIVE       8         // get_active_anims
#define CI_MAX_BLEND        256       // build_bone_matrices locals
#define CI_TICKS_PER_FRAME  160       // anim time units: 160 ticks per frame, 30 frames per second
#define CI_FRAMES_PER_SEC   30
#define CI_ANGLE_UNSET      0xc479f99a  // -999.9f, compared as an int (build_bone_matrices)

struct ci_anim_speed {                // 8 bytes, +0x00 x5
    int anim;                         // -1 free
    float scale;                      // 1.0 normal speed
};

// Per-bone cache (0x34 bytes). Its implicit constructor (matrix43(), i.e. the shared empty constructor)
// was folded with gr::Vertex's into 0x5081c0, which is why sum.symbols calls this a vertex buffer.
struct ci_bone_matrix {
    matrix43 m;                       // +0x00 model-space bone matrix
    int frame;                        // +0x30 character_instance::frame it was built in
};
LAYOUT_CHECK(ci_bone_matrix, sizeof(ci_bone_matrix) == 0x34);

// One animated copy of a ca_character (0xcc bytes): the anim times and weights, the bone matrices and
// the root motion between frames. Created by charinfo_create_character_instance (new, 0xcc bytes) or as
// the static ui_character_instance (0x5bd820, default constructor).
class character_instance {
public:
    character_instance();                             // 0x5196b0 character_instance_default
    character_instance(ca_character *ch);             // 0x519700
    ~character_instance();                            // 0x519840

    void free_buffers(int unused);                    // 0x519510
    void alloc_buffers();                             // 0x519570
    void reset_anims(bool clear_weights);             // 0x519850
    void rewind_anim(int anim);                       // 0x5198a0
    void blend_bone_keys(matrix43 *out, quaternion *rots, vector3 *positions, float *weights,
                         int count);                  // 0x5198d0
    matrix43 build_bone_matrices(int bone, int count, ubyte *anims);   // 0x519ac0
    void advance_anim_time(int anim, int dt);         // 0x51a1e0
    void increment_tick(int dt);                      // 0x51a230
    static int get_active_anims(character_instance *ci, ubyte *out);   // 0x51a6a0 (cdecl)
    void setup_matrices();                            // 0x51a6f0
    void setup_matrices_b();                          // 0x51a780
    void ca_noop_1arg(float frametime);               // 0x51a870
    void ca_noop_2arg(int a, int b);                  // 0x51a880
    void process(float frametime, bool culled, int lod);   // 0x51a890
    const vector3 &get_root_velocity();               // 0x51a950
    vector3 get_motion_delta(float dt);               // 0x51aab0
    vector3 get_root_offset(float unused);            // 0x51abb0
    vector3 sum_root_velocity();                      // 0x51abd0
    quaternion take_turn_delta(float unused);         // 0x51acb0
    void get_tag_pos(vector3 *out, int *value, int bone);       // 0x51ad00
    void get_bounding_sphere(vector3 *center, float *radius);   // 0x51adc0
    void get_bone_bind_inverse(int unused1, int unused2, vector3 *pos, matrix43 *m, int bone); // 0x51ae00
    void set_state_weight(int anim, float weight);    // 0x51ae90
    void start_anim(int anim, float weight, bool held, float speed);   // 0x51aee0
    void set_anim_speed(int anim, float scale);       // 0x51af70
    void get_set_time(float *value, bool set);        // 0x51afe0
    float get_anim_time_left(int anim);               // 0x51b000
    float get_anim_elapsed(int anim);                 // 0x51b070
    float get_anim_length(int anim);                  // 0x51b0c0
    void clear_state_anims();                         // 0x51b100
    void clear_action_anims();                        // 0x51b140
    void stop_anim(int anim);                         // 0x51b1c0
    void get_anim_root_velocity(int anim, vector3 *out);        // 0x51b1e0
    float get_anim_root_speed(int anim);              // 0x51b250
    bool anim_trigger_hit(const char *name);          // 0x51b290
    void page_in_textures();                          // 0x51b330
    void ca_stub_ret30_51b380(matrix43 m);            // 0x51b380 (ret 0x30)
    void set_pos(const vector3 *v);                   // 0x51b390 character_instance__set_pos (formerly get_pos)
    matrix43 compute_bone_matrix(int bone);           // 0x51b3b0
    void get_bone_world_transform(int bone, matrix *inst_orient, vector3 *inst_pos,
                                  matrix *out_orient, vector3 *out_pos);   // 0x51b3f0
    // Parameter order from the body (DECISIONS.md D302): bone [esp+0xc], rotation [esp+8], position [esp+4].
    void get_bone_transform(vector3 *out_pos, matrix *out_orient, int bone);   // 0x51b460
    int report_memory();                              // 0x51b4a0
    void set_tint(ubyte r, ubyte g, ubyte b);         // 0x51b590
    // Not in this unit (vsdk\ca, file not known); thiscall, ret 0xc. Used by new_status.cpp.
    void render(vector3 *pos, matrix *orient, gr_light_desc *light);   // 0x520420 character_instance__render

    ci_anim_speed speeds[CI_NUM_ANIM_SPEEDS];         // +0x00
    vector3 pos;                      // +0x28 root offset fed into the root bone
    int _unk34;                       // +0x34
    int _unk38;                       // +0x38
    ca_character *character;          // +0x3C
    int *times;                       // +0x40 per anim, ticks
    float *weights;                   // +0x44 per anim
    ci_bone_matrix *bone_cache;       // +0x48 per bone
    matrix43 *skin_matrices;          // +0x4C per bone: bind * bone (setup_matrices_b)
    int frame;                        // +0x50 0..100, invalidates bone_cache
    int held_anim;                    // +0x54 action anim that holds its last frame
    int action_anim;                  // +0x58 current action anim, -1 none
    int state_anim;                   // +0x5C dominant state anim, -1 none
    float phase;                      // +0x60 0..1 position in the state anims' cycle
    vector3 root_velocity;            // +0x64
    bool motion_valid;                // +0x70 motion_prev holds a root position
    float turn_from;                  // +0x74 take_turn_delta
    float turn_to;                    // +0x78
    vector3 motion_cur;               // +0x7C root position, previous frame
    vector3 motion_prev;              // +0x88 root position, this frame
    vector3 root_offset;              // +0x94
    float turn_start;                 // +0xA0 root yaw at the action start (CI_ANGLE_UNSET)
    float turn_end;                   // +0xA4 root yaw now
    bool trigger_hit[CI_NUM_TRIGGERS];// +0xA8
    int trigger_anim;                 // +0xB0 state anim whose triggers are tested
    ubyte _unkb4;                     // +0xB4 0xff from the constructor
    int lod;                          // +0xB8
    float _unkbc;                     // +0xBC 2.0 from the constructor
    int _unkc0;                       // +0xC0 -1 from the constructor
    float _unkc4;                     // +0xC4 0.8 from the constructor
    bool culled;                      // +0xC8 setup_matrices_b does nothing when set
    ubyte tint_r, tint_g, tint_b;     // +0xC9 r, g, b (character_instance__render); three members, not an array: the implicit operator= copies them as three byte moves (D8604)
};
LAYOUT_CHECK(character_instance, sizeof(character_instance) == 0xcc);

float skeleton_get_bone_blend_weight(skeleton *s, int bone, int time);   // 0x51a130
void ca_system_init_stub();           // 0x51ae70
void ca_system_close_stub();          // 0x51ae80
void ca_system_init(int arg);         // 0x51b560
void ca_system_close();               // 0x51b580

extern bool ci_root_to_position;      // 0x5a8484
extern bool High_lod;                 // 0x2722810
void cmd_high_lod();                  // 0x51b5f0

// new_inventory.cpp / new_hud.cpp wrote the instance calls through shims while Character was a C struct;
// they are now plain member accesses (as CHAR_M, DECISIONS.md D1).
#define CHARINST_M(c)  ((c)->instance)
#define CHARINST_HM(c) ((c)->instance)

#endif
