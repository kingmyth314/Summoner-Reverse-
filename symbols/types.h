// Structure layouts for Sum.exe v1.40, referenced by symbols/sum.symbols.
// Field offsets are observed in Sum.exe unless noted. Reference names come from the projects listed
// in docs/reference/engine-references.md (layouts only; no code is copied).
#pragma once
#include <stdint.h>

// gr::Screen at 0x25d8d30 (zeroed by gr__init, 0x2a dwords). Layout matches Dash Faction's
// rf::gr::Screen for every field Sum.exe has been seen to use.
typedef struct gr__Screen {
    int32_t signature;      // +0x00
    int32_t max_w;          // +0x04 screen width
    int32_t max_h;          // +0x08 screen height
    int32_t mode;           // +0x0C renderer: 0x66 Direct3D, 0x67 Glide, 0x68 OpenGL
    int32_t window_mode;    // +0x10 200 windowed, 201 fullscreen, 202 desktop size
    int32_t dual_monitors;  // +0x14 (bool in DF; padded)
    float   aspect;         // +0x18 (width / height) * 0.75
    int32_t rowsize;        // +0x1C
    int32_t bits_per_pixel; // +0x20
    int32_t bytes_per_pixel;// +0x24
    float   far_zvalue;     // +0x28
    int32_t offset_x;       // +0x2C clip origin x
    int32_t offset_y;       // +0x30 clip origin y
    int32_t clip_width;     // +0x34
    int32_t clip_height;    // +0x38
    int32_t max_bitmap_w;   // +0x3C from GL_MAX_TEXTURE_SIZE (OpenGL) / device caps (D3D)
    int32_t max_bitmap_h;   // +0x40
    int32_t clip_left;      // +0x44
    int32_t clip_right;     // +0x48
    int32_t clip_top;       // +0x4C
    int32_t clip_bottom;    // +0x50
    // +0x54.. colour, textures, fog, tint (Dash Faction layout; not yet verified in Sum.exe)
} gr__Screen;

// A developer console command (Summoner 2: console_command). On PC the constructor takes
// (name, description, callback); there is no type field at +0xC as in Summoner 2's version.
typedef struct console_command {
    const char* name;        // +0x0
    const char* description; // +0x4
    void (*callback)(void);  // +0x8
} console_command;

// Text-table parser (Summoner 2: TextFile, vsdk\parse\parse.cpp). Partial layout observed in the
// TextFile__* methods; the gap holds the file name and other state (not yet mapped).
typedef struct TextFile {
    char* buffer;            // +0x00 file contents (freed by Close unless a static buffer is used)
    char* cursor;            // +0x04 current parse position
    uint8_t flag08;          // +0x08 zeroed by the constructor
    uint8_t _unmapped[0x3f]; // +0x09
    int32_t line;            // +0x48 current line number (Rewind sets 1)
    int32_t size;            // +0x4C buffer length (SkipTo bounds check)
} TextFile;

// .vpp packfile records (vsdk\vfile\packfile\file_packfile.cpp). PC layouts differ from Summoner 2's:
// observed in packfile_find_packfile / packfile_find_in_packfile / packfile_fopen / fseek.
typedef struct Packfile Packfile;
typedef struct PackfileEntry {
    uint32_t hash;           // +0x00 packfile_hash(name)
    const char* name;        // +0x04
    int32_t sector;          // +0x08 offset in 2048-byte sectors from the packfile start
    int32_t size;            // +0x0C bytes
    Packfile* packfile;      // +0x10
    void* hdd_file;          // +0x14 FILE* when opened from the hard drive
    void* cd_file;           // +0x18 FILE* when the packfile is on the CD (Packfile.flags & 1)
} PackfileEntry;             // 0x1C bytes
struct Packfile {
    char name[0x20];         // +0x00 e.g. "summoner.vpp"
    uint8_t flags;           // +0x20 bit 0: on CD
    uint8_t _pad[7];
    int32_t num_files;       // +0x28
    PackfileEntry* entries;  // +0x2C
    int32_t directory_size;  // +0x30
};                           // 0x34 bytes
typedef struct PackfileLookup {
    uint32_t hash;           // -1 = empty slot
    PackfileEntry* entry;
} PackfileLookup;            // table of 0x50e9 entries at 0x2d68780

// Bump allocator (vsdk memory.cpp). PC layout from Mempool__Copy / allocate_in_mempool; Summoner 2's
// version also has an alignment byte at +0xC, which PC replaces with the name (alignment fixed at 16).
typedef struct Mempool {
    uint8_t* destination;    // +0x00
    uint32_t size;           // +0x04
    uint32_t offset;         // +0x08
    char name[1];            // +0x0C length not yet known
} Mempool;

// Command-line parameter (vsdkoscmdline.cpp). Same layout as Dash Faction's rf::CmdLineParam.
typedef struct CmdLineParam {
    struct CmdLineParam* next; // +0x00
    struct CmdLineParam* prev; // +0x04
    const char* name;        // +0x08 e.g. "-glide"
    const char* help;        // +0x0C
    char* arg;               // +0x10 heap copy of the following argument
    uint8_t name_found;      // +0x14
    uint8_t has_argument;    // +0x15
} CmdLineParam;
typedef struct CmdArg {
    char* arg;               // +0x0
    uint8_t is_done;         // +0x4
} CmdArg;                    // cmdline_args[50] at 0x2dc6ea8

// Bitmap manager entry (vsdk\bmpman\bmpman.cpp), bm_bitmaps[0xaf0] at 0x2591c78; handle = index.
// Smaller than Red Faction's 0x6C-byte BitmapEntry (Dash Faction): the name is a pointer into
// bm_name_pool instead of char[32]. Offsets observed in bm_load_internal, bm_create, bm_lock_*.
typedef struct bitmap_entry {
    const char* name;        // +0x00 in bm_name_pool
    int32_t name_checksum;   // +0x04 bm_hash_name(name); -1 for user bitmaps
    uint16_t width;          // +0x08
    uint16_t height;         // +0x0A
    int32_t num_pixels;      // +0x0C all mip levels (bm_calc_num_pixels)
    int32_t type;            // +0x10 0 none, 1 pcx, 2 tga, 3 user, 4 vbm, 5 m2v
    int32_t anim_type;       // +0x14 0 single, 1 animation header, 2 frame
    int32_t format;          // +0x18 same numbering as Dash Faction bm::Format
    uint8_t num_levels;      // +0x1C mip levels
    uint8_t num_frames;      // +0x1D header: frame count; frame: frame number (1-based)
    uint8_t _pad1e[2];
    void* data;              // +0x20 locked pixels
    float frames_per_ms;     // +0x24
    void* palette;           // +0x28 0x300 bytes for 8-bit paletted
    struct bitmap_entry* next; // +0x2C used/free list
    struct bitmap_entry* prev; // +0x30
    uint8_t lock_count;      // +0x34
    uint8_t _pad35[3];
    int32_t total_bytes;     // +0x38
    int32_t path_id;         // +0x3C File::open path id (9999999 = default)
    uint8_t reduced;         // +0x40 loaded at half size (bm_reduce_textures)
    uint8_t _pad41[3];
    int32_t mark_count;      // +0x44 bm_set_mark nesting
} bitmap_entry;              // 0x48 bytes

// Bitmap font (vsdk\gr\gr_font.cpp), gr_fonts[6] at 0x25d9238. Same 0x6C-byte layout as Dash
// Faction's rf::gr::Font; offsets observed in gr_font_create, gr_font_get_char, gr_string.
typedef struct FontCharInfo {
    int32_t spacing;         // +0x0
    int32_t width;           // +0x4
    int32_t pixel_data_offset; // +0x8
    int16_t kerning_entry;   // +0xC first kern pair for this char, -1 none
    int16_t user_data;       // +0xE
} FontCharInfo;
typedef struct Font {
    char filename[32];       // +0x00
    int32_t signature;       // +0x20 0 = free slot
    int32_t format;          // +0x24 0xF 4-bit alpha, 0x0F0F0F0F 16-bit, -0x10 8-bit paletted
    int32_t num_chars;       // +0x28
    int32_t first_ascii;     // +0x2C
    int32_t default_spacing; // +0x30
    int32_t height;          // +0x34
    int32_t height2;         // +0x38
    int32_t num_kern_pairs;  // +0x3C
    int32_t pixel_data_size; // +0x40
    uint8_t* kern_data;      // +0x44 3-byte {ch0, ch1, offset}
    FontCharInfo* chars;     // +0x48
    uint8_t* pixel_data;     // +0x4C
    int32_t* palette;        // +0x50
    int32_t bm_handle;       // +0x54 4444 texture made with bm_create
    int32_t bm_width;        // +0x58 64, 128 or 256
    int32_t bm_height;       // +0x5C
    int32_t* char_x_coords;  // +0x60
    int32_t* char_y_coords;  // +0x64
    uint16_t* bm_bits;       // +0x68
} Font;                      // 0x6C bytes

// Math types (vsdk\math), Summoner 2 names. matrix3 rows are right/up/forward vectors.
typedef struct vector3 { float x, y, z; } vector3;
typedef struct matrix3 { vector3 rvec, uvec, fvec; } matrix3;          // 0x24 bytes
typedef struct matrix43 { float m[4][3]; } matrix43;                     // rotation rows + position (m[3])
typedef struct quaternion { float x, y, z, w; } quaternion;              // w at +0xC (quaternion__to_matrix)

// One entry of gameseq_state_table (0x588a70): handlers for a game state.
typedef struct GameStateHandlers {
    void (*enter)(void);    // +0x0 called after the state becomes current
    void (*frame)(void);    // +0x4 called every GameTick while current
    void (*leave)(void);    // +0x8 called before switching away
} GameStateHandlers;

// A character (party member, NPC or monster); cur_character (0x5fcc88) points at one. Partial layout from
// character_add_xp (0x44fab0), character_get_max_stat (0x462840), character_equip_bonus (0x4627c0),
// party_level_reset (0x437e50) and a runtime dump of Joseph at level 3 (HP 128, AP 26, 200 xp).
// The class is living_entity (constructor living_entity__living_entity, 0x44e060) on top of the base object (obj__obj).
typedef struct Character {
    void*   vtable;          // +0x000 living_entity__vtable (0x581914)
    void*   _link04;         // +0x004 obj_active_list next (obj_link_new links +4/+8 into 0x970770; that list, not the living list)
    struct Character* obj_prev; // +0x008 obj_active_list prev: obj_link_new (+8 = old tail 0x970778), unlinked with +4 in summon_stash_for_cutscene
    int32_t type;            // +0x00C 7 for player party members (level-up message, skill copy)
    float   pos[3];          // +0x010 world position
    float   last_pos[3];     // +0x01C previous position: ai_reset copies +0x10 here; character_place_at_party_start/_at_marker, character_load_state set it; level_check_triggers reads it with pos
    float   centre_offset[3];// +0x028 offset from pos to the body centre (inferred)
    void*   start_navpoint;  // +0x034 start navpoint / marker: character_place_at_party_start stores level_player_starts[slot] (+0x14 pos, +0x10 heading, +0x20 layer); script_replace_navpoint_references, script_editor_navpoint_in_use compare it; obj__obj zeroes it
    matrix3 orient;          // +0x038 orientation (rvec/uvec/fvec): matrix3__set_from_angles(this+0x38) in place_at_party_start, fvec +0x50 in character_get_forward_velocity, axis-angle turn in living_entity_move_frame
    matrix3 last_orient;     // +0x05C previous orientation: ai_reset copies +0x38..+0x58 here dword by dword; place_at_party_start/_at_marker/load_state also write it
    int32_t handle;          // +0x080 object handle (obj_create); player_from_handle(handle) gives the Player record
    uint32_t obj_flags;      // +0x084 bit 0 active
    float   bound_radius;    // +0x088 out-param &radius of character_instance__get_bounding_sphere in living_entity_init (only writer)
    int32_t net_handle;      // +0x08C multiplayer handle (obj_create: -1 unless multiplayer); mp_send_it_rqst, mp_handle_it_*, mp_broadcast_hp, multi_do_frame
    int32_t _unk90;          // +0x090
    int32_t _unk94;          // +0x094
    int32_t team;            // +0x098
    int32_t move_start_time; // +0x09C
    uint8_t trigger_latch[3];// +0x0A0 per-move trigger fired bytes (character_move_trigger_mark/_fired byte [this+0xa0+i], i<3; character_play_move clears). Was int32_t[3]: +0xA4/+0xA8 are separate fields
    uint8_t _padA3;          // +0x0A3
    int32_t death_move_sub;  // +0x0A4 death move sub id: character_kill(arg) stores it (callers pass 0x22), character_find_move(6, +0xa4) in place_at_party_start/_at_marker/on_death; saved as a byte by character_save_state
    void*   death_move;      // +0x0A8 move record from character_find_move(6, death_move_sub); [+8] = anim for character_instance__start_anim / get_anim_time_left; read by living_entity_move_frame, character_update_head_look, character_load_state
    uint8_t status_slots[18][0x20]; // +0x0AC status effects by status_effect_id: +0 state (-1..2), +4 and +0x14 GameTimers
    int32_t timer2ec;        // +0x2EC GameTimer
    uint32_t enemy_team_mask;// +0x2F0 teams this character fights (defaults at 0x581904)
    char    name[0x20];      // +0x2F4
    void*   instance;        // +0x314 character instance (vsdk ca)
    struct Character* live_next; // +0x318 living-entity list
    struct Character* live_prev; // +0x31C used by "%s gained level %d!" (length not yet confirmed)
    uint32_t flags;          // +0x320 0x80 dead (revived by party_level_reset), 0x100, 0x402 cleared on restore
    uint8_t ai[0x234];       // +0x324 ai_state sub-object (ai_state__ctor 0x4722f0 on this+0x324, touches up to +0x22c; +0x554 = ai+0x230 flag byte in tiger_approach_target); owner ai+0x4c = this (living_entity_init writes +0x370)
    uint8_t combat[0x54];    // +0x558 living_combat sub-object (living_combat__ctor 0x4681b0, set_owner: [0] = this; living_combat_reset touches up to +0x4c, update_special byte +0x50); character_get_protection
    // +0x5AC..+0x5F0 locomotion animation ids, one per name of locomotion_anim_names (0x599648, 18 entries), filled by
    // character_fill_locomotion_anims (charinfo_find_anim, loop < 0x12) with fallbacks; read by character_update_locomotion_anim
    int32_t anim_stand;          // +0x5AC "stand"
    int32_t anim_norm_walk;      // +0x5B0 "norm_walk"
    int32_t anim_norm_run;       // +0x5B4 "norm_run"
    int32_t anim_ready;          // +0x5B8 "ready" (ai_combat_toggle_ready_stance swaps the +0x5ac/+0x5b8 sets)
    int32_t anim_walk;           // +0x5BC "walk"
    int32_t anim_run;            // +0x5C0 "run"
    int32_t anim_stand_hurt;     // +0x5C4 "stand_hurt"
    int32_t anim_norm_walk_hurt; // +0x5C8 "norm_walk_hurt"
    int32_t anim_norm_run_hurt;  // +0x5CC "norm_run_hurt"
    int32_t anim_ready_hurt;     // +0x5D0 "ready_hurt"
    int32_t anim_walk_hurt;      // +0x5D4 "walk_hurt"
    int32_t anim_run_hurt;       // +0x5D8 "run_hurt"
    int32_t anim_sneak_ready;    // +0x5DC "sneak_ready"
    int32_t anim_hide_ready;     // +0x5E0 "hide_ready"
    int32_t anim_tripped_liedown;// +0x5E4 "tripped_liedown" (knocked-down branch of character_update_locomotion_anim)
    int32_t anim_cloaked_ready;  // +0x5E8 "cloaked_ready"
    int32_t anim_cloaked_sneak;  // +0x5EC "cloaked_sneak"
    int32_t anim_statscreen;     // +0x5F0 "statscreen" (statscreen_load_anim / _unload_anim / _refresh_anim)
    float   base_max_hp;     // +0x5F4 max HP without equipment; written by the level table
    float   hp;              // +0x5F8 current HP (119 code references)
    int16_t base_max_ap;     // +0x5FC
    int16_t ap;              // +0x5FE current AP
    int16_t level;           // +0x600
    int16_t _pad602;
    int32_t xp;              // +0x604 experience
    uint8_t hit_by_party;    // +0x608 byte: character_add_hp sets 1 when the damage source (living_from_handle) is type 7, else 0; character_on_death awards party XP only when set; living_entity_init clears
    uint8_t _pad609[3];      // +0x609
    int32_t move_fx;         // +0x60C effect handle of the current move (character_play_move, fx_is_finished; -1 none); character_get_move_effect returns it; living_entity_move_frame obj_from_handle
    float   frametime;       // +0x610 this character's frame time: fmul in character_ai_move_frame (accel/brake), ai_turn_towards, update_regen_and_status; mp_handle_forcepr / script_object_receive_go set it to reported_frametime
    uint8_t skill[0x60];     // +0x614 skill/attribute values by skill_id, value = byte * 0.01 (character_get_skill); skill level shown = value * 10; speed (id 2) shown = value * 1000 (11 -> 110%)
    int32_t move_sound;      // +0x674 sound handle of the current move: character_play_move snd_is_playing / snd_stop_fade / sound_play_3d_at result; living_entity_init 0
    int16_t foot_tag_l;      // +0x678 mesh tag "BDBN-Foot-L" (character_find_foot_bones), read by character_update_footsteps
    int16_t foot_tag_r;      // +0x67A mesh tag "BDBN-Foot-R"
    float   ground_hit[3];   // +0x67C vector from the ground probe hit record (character_probe_lobj_ground out-arg in character_find_ground, 4 calls); passed with +0x688/+0x68a to lighting_sample_stub (update_regen_and_status, combat_spawn_blood); point vs normal not settled
    int16_t ground_lobj;     // +0x688 level object under the character (character_find_ground; -1 none); lobj_get_layer in character_update_layer, footsteps, lighting
    int16_t ground_face;     // +0x68A face of that level object (-1 none)
    int32_t layer;           // +0x68C level layer (character_update_layer = lobj layer of +0x688); 88 users: automap, camera, character_find_ground, ai_path_to (pos+0x67c), mp_send_obj_update, place_at_party_start (start +0x20)
    int16_t _unk690;         // +0x690 word: living_entity_init 0, ai_consider_life_drain compares < 5 (only 2 users)
    int16_t _pad692;
    float   stance_blend;    // +0x694 weight between the two locomotion sets (character_update_locomotion_anim 1 - x); ai_combat_toggle_ready_stance blends it by cosine; ai_reset / init 1.0; hiding/sneaking frames write it
    float   forward_speed;   // +0x698 current forward speed: character_get_forward_velocity (* fvec +0x50), character_get_698, ai_follow_path, ai_turn_towards, character_find_overlap; written by character_ai_move_frame
    float   velocity[3];     // +0x69C
    float   loco_blend;      // +0x6A8 locomotion blend 0..0.5 walk, 0.5..1 run (character_set_locomotion_speed, character_anim_speed_at_blend, character_anim_lod_skip, living_combat_roll_defense)
    float   target_speed;    // +0x6AC speed character_ai_move_frame accelerates (char_info speedup +0x228) or brakes (+0x6b0) toward; character_set_6ac; mp_send_obj_update
    float   slowdown_rate;   // +0x6B0 braking rate, copied from char_info +0x22c ($Slowdown rate) in living_entity_init and character_ai_move_frame; character_stopping_distance, distance_to_reach_*
    float   run_speed;       // +0x6B4 root speed of the run anim (character_update_locomotion_anim, falls back to walk); clamp in set_locomotion_speed; speed_scale default (living_entity_init, set_speed_scale < 0, ai_start_travel run)
    float   walk_speed;      // +0x6B8 root speed of the walk anim (character_update_locomotion_anim); ai_start_travel walk, ai_goal_do_move_to, status_effect_add
    float   speed_scale;     // +0x6BC
    float   _vec6c0[3];      // +0x6C0 vector3, zeroed by living_entity_init (vector3__zero); no other user found
    int32_t cur_anim;        // +0x6CC current animation
    int32_t cur_move;        // +0x6D0 current move (holds a move record pointer: [+8] anim, [+0x1c + 8*i] trigger ids)
    int32_t timer6d4;        // +0x6D4 GameTimer (constructed in living_entity__living_entity); no other user found
    int32_t cutscene_object; // +0x6D8 index of the cutscene script object for this actor (0x550-byte records, -1 none): cutscene_script_unload / _rewind_actors, level_script_entries_frame
    int32_t script_object;   // +0x6DC index of the level script object (0x550-byte records at 0x24f6230, -1 none): character_save_state / load_state (its +0x518 GameTimer), level_script_restart, ls_lpalaceint_sneak_frame
    int32_t mount_link;      // +0x6E0 handle of the linked mount or rider (character_get_mount, rider_unlink_mount 0x4a33b0)
    uint32_t mount_flags;    // +0x6E4 bit 0 = is a rider, bit 1 = is a mount
    float   hp_overflow;     // +0x6E8 healing beyond max HP (character_add_hp)
    float   pos_mod_text;    // +0x6EC attacker's position modifier (living_combat +0x48, combat_position_modifier) shown as "%d%%" by levelrender_draw_float_text; set in combat_apply_melee_hit
    float   hp_text_timer;   // +0x6F0 display timer of the HP-change number (+0x6e8): character_add_hp sets 2.0, living_entity_move_frame / update_regen count down, alpha in levelrender_draw_float_text
    float   pos_mod_text_timer; // +0x6F4 display timer of +0x6ec: combat_apply_melee_hit sets 2.0, counted down like +0x6f0
    char    float_text[16];  // +0x6F8 floating text above the character ("%d AP"; 0x461fe0)
    char    float_text_nul;  // +0x708 terminator after strncpy(+0x6f8, s, 16) in character_set_floating_text
    uint8_t float_text_rgb[3]; // +0x709
    float   float_text_scale;  // +0x70C
    int32_t _unk710;         // +0x710 living_entity_init -1; no other user found
    float   move_turn_angle; // +0x714 turn applied over the current move: living_entity_move_frame rotates orient (+0x38, matrix3__set_from_axis_angle) by anim progress * this; character_update_head_look sets / clears it
    float   move_turn_applied; // +0x718 part of +0x714 already applied (living_entity_move_frame)
    struct char_info* info;  // +0x71C character info record (char_infos); +0x10 species, +0x258 flags
    int32_t prop_tag;        // +0x720 current prop tag: living_entity_init = char_info +0x124 ($Prop Tag), 0 when stand==ready or multiplayer; inventory_equip_item, monster_drop_loot, save/load_state, ai_reset_party_member (= +0x724)
    int32_t default_prop_tag;// +0x724 prop tag to restore: = +0x720 at init; character_reinit_anims sets {10,2,4,3,5,6,7,8,9}[n]; inventory_equip_item / level_script_restart copy it back
    int32_t num_attachments; // +0x728 count of +0x72c entries drawn by levelrender_add_character; living_entity_init 0 (1 for species 0xce), tiger_equip_weapon 5
    struct { int32_t slot; int32_t item_type; } attachments[5]; // +0x72C bone-attached models: slot indexes char_info +0x168 (0x11 = none), item_type indexes item_types (levelrender_add_bone_attachment); tiger_*, serpent_spawn_rider, phoenix/ghost/evil_joseph boss loads
    void*   equipment[10];   // +0x754 equipped items (item +4 = ItemType*); +0x768 (slot 5) weapon; +0x760/764/76C/770/774 armour
    // +0x758/+0x75C (slots 1-2) also receive a share of experience
    // The object is 0x788 bytes: the living-entity pool (init_living_entity_pool_8ed980) and living_entity_create use that stride.
    int32_t anim_lod_counter; // +0x77C character_anim_lod_skip: skips 4 of every 5 anim updates
    float   light_level;      // +0x780 ground light (avg of lighting_sample rgb) from character_update_regen_and_status; -1.0 at init; levelrender_add_bone_attachment, script_guard_sees_player
    int32_t net_update_timer; // +0x784 GameTimer (ctor; living_entity_init invalidates; mp_send_local_updates)
} Character;

// An item type, shared by every item of that kind; an equipped item's +0x4 points at one. Fields from
// items_parse_table (items.tbl), checked against "Short Sword" and "Tunic" in a runtime dump.
// item_types (0x8cbec0) has a stride of 0x100 (item_type_find, shop_find_item_type: sar 8).
typedef struct ItemType {
    const char* name;        // +0x00 $Name: "Short Sword"
    int32_t kind;            // +0x04 $Type: 0 Weapon, 1 Armor, 2 Misc
    int32_t material;        // +0x08 $Default Material: index into materials (0x8b9d4c, 17 entries)
    const char* model;       // +0x0C $Model: "prop-sword-short.v3d"
    void*   mesh;            // +0x10 paged-in mesh: vim_load by item_page_in_mesh, 0 when paged out (items_reset_paging, item_page_out_all, item_type_is_paged_in)
    const char* animation;   // +0x14 +Animation: (kept for weapons and slot-5 armour, else 0)
    int32_t value;           // +0x18 $Value: (100 sword, 20 tunic)
    int32_t slot;            // +0x1C +Slot: equipment slot (default 4; sword 4, tunic 6)
    void*   skill_mods[4];   // +0x20 +Skill Mod: -> {skill, value} (character_skill_equip_mod)
    void*   resist_mods[4];  // +0x30 +Resist Mod: -> {resistance type, value} (character_resist_equip_bonus)
    void*   stat_mods[2];    // +0x40 +Stat Mod: -> {stat, value} (character_equip_bonus: 0 HP, 1 AP)
    int8_t  status_effects[9]; // +0x48 +Effect: "spell" level: byte [damage_type_to_status(spell)] = level; -1 none
    uint8_t _pad51[3];
    float   ap_regen;        // +0x54 +AP Regen: x0.01 (default 1.0)
    void*   charge_effect;   // +0x58 +Charge Effect: / +Self Only Charge Effect: -> 0x18-byte {spell, int, int, int, self-only byte +0x10, +Disposable float +0x14}; 0 none
    uint16_t flags;          // +0x5C +Flags: one bit per name (table 0x599024); bit 0 also sets 0x8, +Stealth sets 0x10; bit 0 = identified (shops-loot.md)
    int16_t bitmap;          // +0x5E +Bitmap: bm_load handle
    uint8_t _unk60[0x07];
    uint8_t restricted;      // +0x67 +Restricted: 1 Joseph, 2 Rosalind, 4 Flece, 8 Jekhar (0xf when absent)
    // weapon fields (armour reuses +0x68..+0xE7 for per-character data: zeroed, then +Joseph/+Flece/+Jekhar/+Rosalind blocks)
    uint16_t damage;         // +0x68 $Damage: (35 for the short sword); every read is zero-extended
    uint16_t weapon_flags;   // +0x6A weapon +Flags: one bit per name (table 0x599010); 2 for the short sword; bit 0x4 skips $Skill
    int32_t weapon_skill;    // +0x6C $Skill: skill id (3 = sword weapons for the short sword)
    float   speed;           // +0x70 $Speed:
    float   attack_radius;   // +0x74 $Attack Radius:
    float   weight;          // +0x78 $Weight:
    int32_t _unk7c;          // +0x7C -1 for weapons
    int32_t _unk80;          // +0x80 -1 for weapons
    float   render_distance; // +0x84 +Render distance: (default 1.0)
    int32_t weapon_class;    // +0x88 $Class:
    int32_t damage_type;     // +0x8C $Damage Type:
    int32_t elemental_damage;// +0x90 +Elemental Damage: (type)
    void*   weapon_triggered_effect; // +0x94 +Triggered Effect: -> 0xc-byte {spell, float, int}; 0 none
    uint8_t _unmapped98[0x50];
    // armour fields
    float   armour_speed;    // +0xE8 $Speed:
    float   armour_weight;   // +0xEC $Weight:
    float   protection;      // +0xF0 $Protection: (0.07 for the tunic, shown as 7)
    uint32_t _unkf4;
    void*   armour_triggered_effect; // +0xF8 armour +Triggered Effect: -> 0xc-byte {spell, float, int}; 0 none
    const char* description; // +0xFC +Description: (all kinds; 0 none)
} ItemType;                  // 0x100 bytes

// A materials.tbl record (materials, 0x8b9d4c), 20 bytes.
typedef struct material {
    float damage_mod;        // +0x00 +Damage Modifier:
    float weight_mult;       // +0x04 +Weight Multiplier:
    float speed_mult;        // +0x08 +Speed Multiplier:
    float protection_mult;   // +0x0C +Protection Multiplier: (character_get_protection reads 0x8b9d58 + 20*material)
    uint32_t _unk10;
} material;

// One level's script hooks in level_script_table (0x59f128, 51 records). level_script_select copies the
// handlers into globals when a level loads. 0x4f13e0 (a bare RET) is used as an empty handler.
typedef struct level_script_info {
    const char* name;                    // +0x00 level file name ("masad", "sewer", "Rand-Desert", ...)
    void (*init)(void);                  // +0x04 -> 0x249029c; level_script_init (state_game_enter), restart
    void (*frame)(void);                 // +0x08 -> 0x24902ac; level_script_frame, while Do_frame_callback
    void (*leave)(void);                 // +0x0C -> 0x2491a54; level_script_leave (state_game_leave), restart
    char (*use)(const char* name);       // +0x10 -> 0x2491a60; an object or exit was used: opens gates or speaks a refusal; returns 0
    void* unused14;                      // +0x14 -> 0x2491a64; 0 in every record
    void* action;                        // +0x18 -> 0x2490c5c; script/dialogue action handler (conversation_execute_action)
    int32_t load_steps;                  // +0x1C loading-screen progress total (LoadingScreenDraw)
    const char* music;                   // +0x20 music name (looked up with 0x4696d0 in debug mode)
    int32_t num_flags;                   // +0x24 savegame flag bits for this level (savegame_get_flag)
    const uint32_t* flag_hashes;         // +0x28 case-insensitive name hash of each flag
    int32_t automap_layers;              // +0x2C 4096 reveal bits each
    int32_t num_ints;                    // +0x30 integer script variables
    const uint32_t* int_hashes;          // +0x34
    int32_t num_floats;                  // +0x38 float script variables
    const uint32_t* float_hashes;        // +0x3C
} level_script_info;

// Multiplayer chat command (chat_commands, 0x59c180).
typedef struct chat_command {
    const char* name;        // "/kick", "/ping", "/list"
    void (*handler)(void);   // argument list not yet mapped
} chat_command;

// {skill, level, value} entry of a player's skill progression (e.g. joseph_skill_progression, 0x593508).
typedef struct skill_progression {
    int32_t skill;           // skill id (index into Character.skill)
    int32_t level;           // character level at which it applies
    int32_t value;           // new Character.skill[skill] value (hundredths)
} skill_progression;

// A player record (player.cpp), one per party member, in party_list (0x5fc350). Observed for Joseph in a
// runtime dump; player_from_handle finds it from the character's object handle.
typedef struct Player {
    struct Player* next;     // +0x000 circular list (head party_list)
    struct Player* prev;     // +0x004
    int32_t char_info;       // +0x008 char_infos index passed to living_entity_create (player_create); player_init_from_character sets it
    int32_t handle;          // +0x00C the character object's handle
    uint32_t flags;          // +0x010 0x2 summon, 0x4 hold, 0x8 party leader (Joseph: 8); 0x4 on the others in solo mode
    struct mp_player* mp_owner; // +0x014 owning multiplayer player record (mp_player_alloc: +0x14 state, +0x34 name); compared with mp_local_player; 0 from player_alloc_slot
    int32_t _unk18;          // +0x018 682
    int32_t _unk1c;          // +0x01C 536
    int32_t num_progression; // +0x020 17
    const skill_progression* progression; // +0x024 per-level skill gains
    int32_t skill_cooldown[0x54]; // +0x028 GameTimer per skill id (player_start_skill_cooldown; player_create sets all 84; static init 0x4333d0 builds 84 per record)
    float   lent_max_hp;     // +0x178 max HP lent to the summon (summon_create; summon_return_lent_max_hp adds it back); saved in LENT
    int32_t skill_points;    // +0x17C unspent skill points (Character screen "Skill points: 4"); character_add_xp adds to it
    int32_t skill_cap;       // +0x180 xp_level_skill_cap(level) (player_init_from_character; set by character_add_xp on level-up); 3 for Joseph
    int32_t default_chain;   // +0x184 chain-attack slot used with no modifier key (chain_attack_select); cycles 0 -> 3 -> 1 -> 2 (character_cycle_default_chain)
    // The record is 0x188 bytes: party_lists_init links five of them, 0x188 apart, at 0x5fc4d8..0x5fcc80.
    // The "self" (+0x188) and "character" (+0x190) fields listed earlier were the next record and, for the
    // last record (0x5fcaf8), cur_character (0x5fcc88): they are not part of Player.
} Player;

// Status effect ids: Character.status_slots index (status_effect_add 0x454700; floating text L326-L337).
// 7, 8 and 14-16 show no text; the id must be <= 0x10.
enum status_effect_id {
    STATUS_BLIND = 0, STATUS_SILENCE = 1, STATUS_FAITH = 2, STATUS_CURSE = 3, STATUS_BLESS = 4,
    STATUS_REGEN = 5, STATUS_POWER = 6, STATUS_SLEEP = 9, STATUS_PARALYZE = 10, STATUS_POISON = 11,
    STATUS_CRIPPLE = 12, STATUS_SLOW = 13,
};

// A sound reference in a table record (12 bytes): spells_parse_sound (0x47a6c0) and sound_ref_parse (0x446790)
// set the name and handle -1, then store three floats from the table as bytes (the first scaled before _ftol).
typedef struct sound_ref {
    const char* name;        // +0x0 sound file name (string pool)
    int32_t handle;          // +0x4 -1 until loaded (ai_call_for_help plays char_info +0xF0)
    uint8_t params[3];       // +0x8
    uint8_t _pad0b;
} sound_ref;

// A spells.tbl record (spells, 0x22e4f28, 108 x 0x80 bytes), from spells_parse_table (0x47a190). The parser
// walks it with a pointer to +0x78, so its displacements are offset - 0x78.
typedef struct spell_info {
    int32_t spell_class;     // +0x00 +Class: 0 heal, 1 dark, 2 energy, 3 holy, 4 fire, 5 ice, 6 summon, 7 misc
    int32_t index;           // +0x04 record number
    const char* name;        // +0x08 +Name: (string pool 0x22e9228)
    const char* cast_effect; // +0x0C +Cast effect:
    sound_ref cast_sound;    // +0x10 $Sound: after +Cast effect: (spells_parse_sound)
    const char* hit_effect;  // +0x1C +Hit effect: (used by the fx_create_spell_effect_* functions)
    sound_ref hit_sound;     // +0x20 $Sound: after +Hit effect: or +Preloaded Hit effect:
    const char* preloaded_hit_effect; // +0x2C +Preloaded Hit effect:
    const char* misc_effect; // +0x30 +Misc effect:
    sound_ref misc_sound;    // +0x34 $Sound: after +Misc effect: or +Banish effect:
    const char* banish_effect; // +0x40 +Banish effect:
    uint8_t _unk44[0x0c];
    int32_t variation;       // +0x50 +Variation: (variation_names 0x595cf8); the cast move (character_perform_spell_cast)
    float   cost;            // +0x54 +Cost:
    float   skill;           // +0x58 +Skill: x0.01
    float   hit_chance;      // +0x5C +Hit Chance: x0.01 (default 1.0)
    int32_t die;             // +0x60 +Die:
    int16_t duration;        // +0x64 +Duration: (default -1)
    int16_t recovery_time;   // +0x66 +Recovery time:
    int16_t ai_recovery_time;// +0x68 +AI Recovery time:
    int16_t loop_time;       // +0x6A +Loop Time:
    int32_t emerge_time;     // +0x6C +Emerge time:
    float   distance;        // +0x70 +Distance:
    uint32_t flags;          // +0x74 1 +Offensive, 2 +Target character, 4 +Target location, 8 +Server authorize, 0x10 +Fade Props, 0x20 +Beneficial, 0x40 +Status change, 0x80 +Hit only (spell_prepare_cast tests bit 1)
    int32_t damage_type;     // +0x78 +Damage type: (default -1)
    float   target_radius;   // +0x7C +Target radius:
} spell_info;

// A "$Character:" record of the character info tables (characterinfo_parse_table 0x4452f0; base 0x6d7e50,
// 0x3d0 bytes each, count at 0x714e54). Character +0x71C points at one. Offsets from the parser's stores.
typedef struct char_info {
    const char* name;        // +0x000 $Character: name (characterinfo_parse_name; the default anim folder is data\ca\<name>)
    const char* portrait;    // +0x004 +Portrait Filename: (0 by default)
    const char* texture_effect; // +0x008 +Texture Effect: (0 by default)
    const char* vfx;         // +0x00C +Vfx: (0 by default)
    int32_t species;         // +0x010 set by 0x446890 with the name ("doesn't have a species type"); passed to the level table
    uint32_t checksum;       // +0x014 TextFile__Checksum of the table file
    int32_t anim_folder;     // +0x018 +Anim Folder: path id returned by file_add_path("data\ca\<folder>", ".mvf")
    int32_t anim_class;      // +0x01C zeroed; charinfo_find_move compares it with a move's +Animation class (+0xc)
    uint32_t anim_class_mask;// +0x020 one bit per anim_class_names entry used by the moves, -1 for all
    int16_t max_hp;          // +0x024 $Max Hit Points:
    int16_t hp;              // +0x026 $Hit Points:
    int16_t max_ap;          // +0x028 $Max Ability Points:
    int16_t ap;              // +0x02A $Ability Points:
    int32_t xp_gained;       // +0x02C $Experience Gained: (awarded for a kill)
    int32_t level;           // +0x030 $Level:
    float   attack_radius;   // +0x034 $Attack Radius:
    uint16_t damage;         // +0x038 $Damage: read zero-extended by assess_format_info, combat_attack_rating, combat_melee_damage
    uint8_t _unk3a[0x02];
    int16_t special_ability; // +0x03C +Special ability: spell (spell_find_by_name)
    int16_t special_arg;     // +0x03E +Special ability: optional number after the chance (atoi of the rest of the line, default 0)
    float   special_chance;  // +0x040 +Special ability: chance, percent x 0.01
    int16_t reflex_ability;  // +0x044 +Reflex ability: spell
    int16_t reflex_arg;      // +0x046 as +0x03E
    float   reflex_chance;   // +0x048 as +0x040
    void*   moves[0x10];     // +0x04C +Move: list head per move type (move_type_names, 16); move_info next at +0x40 (charinfo_find_move)
    int32_t team;            // +0x08C $Team:
    int32_t size;            // +0x090 $Size:
    uint8_t skill[0x54];     // +0x094 $Skill: "name" value, by skill id (zeroed, then speed (id 2) = 10)
    uint8_t up_skill, down_skill, right_skill, left_skill; // +0x0E8 $Up/$Down/$Right/$Left Skill:
    sound_ref alert_sound;   // +0x0EC +Alert: (sound_ref_parse; ai_call_for_help plays +0xF0)
    uint8_t _unkf8[0x1c];
    int16_t num_default_weapons; // +0x114 $Default Weapon: count
    int16_t default_weapons[2];  // +0x116 item type index, -1 none
    uint8_t _pad11a[2];
    int32_t default_weapon_materials[2]; // +0x11C "made of" material per default weapon (0 when absent)
    int32_t prop_tag;        // +0x124 $Default Prop Tag: tag index (0x11 = none); $Prop Tag: lines fill +0x128
    const char* prop_tags[0x10]; // +0x128 $Prop Tag: / $Default Prop Tag: name per tag (0 by default)
    int16_t _unk168[0x10];   // +0x168 set to -1 with the prop tags
    uint8_t _unk188[0x12];
    int16_t num_footstep_sets; // +0x19A +Footstep sounds: count (1 with the default set)
    uint8_t _unk19c[0x04];
    int32_t footstep_sounds_a[3][5]; // +0x1A0 +Footstep sounds: first five sound handles of each set (snd_get_handle)
    int32_t footstep_sounds_b[3][5]; // +0x1DC second five of each set
    int32_t skin;            // +0x218 +Skin:
    uint8_t _unk21c[0x04];
    float   fov_range;       // +0x220 $Field of view range:
    float   detection_range; // +0x224 $Detection range:
    float   speedup_rate;    // +0x228 $Speedup rate:
    float   slowdown_rate;   // +0x22C $Slowdown rate:
    float   slow_turn_rate;  // +0x230 $Slow turn rate:
    float   fast_turn_rate;  // +0x234 $Fast turn rate:
    float   moving_turn_rate;// +0x238 $Moving turn rate:
    float   intelligence;    // +0x23C +Intelligence: (AI)
    uint8_t has_light;       // +0x240 1 with +CharacterLightPos:
    uint8_t _pad241[3];
    const char* light_pos;   // +0x244 +CharacterLightPos: tag name
    float   light_color[3];  // +0x248 +LightColor: (three floats; attached_light_create)
    float   light_atten;     // +0x254 +LightAtten:
    uint32_t flags;          // +0x258 0x8 cannot be tripped, 0x10 cannot be pushed, 0x20 cannot be backstabbed; 0x80 / 0x100 make status_effect_add skip some effects
    uint8_t _unk25c[0x08];
    float   icon_height;     // +0x264 $Icon Height: (default 2.0)
    void*   alt_textures;    // +0x268 alt_textures_find(name) / alt_textures_find_entry for +Alternate character: copies
    int32_t alt_index;       // +0x26C out value of alt_textures_find; 1 for an +Alternate character: copy
    struct char_info* alt_parent; // +0x270 record an +Alternate character: copy was made from, 0 otherwise
    int32_t num_drops;       // +0x274 +Drop: count (at most 5; monster_drop_loot)
    int32_t drop_items[0x11];// +0x278 +Drop: item type index
    float   drop_chances[0x11]; // +0x2BC +Drop: percent x 0.01
    int32_t num_mp_drops;    // +0x300 +Multiplayer Drop: count (at most 17; monster_drop_loot)
    int32_t mp_drop_items[0x11];   // +0x304
    float   mp_drop_chances[0x11]; // +0x348 percent x 0.01
    int32_t mp_drop_levels[0x11];  // +0x38C level
} char_info;                 // 0x3d0 bytes (the +Alternate character: copy is rep movsd of 0xf4 dwords)

// One experience.tbl level (xp_level_table 0x2305420, 21 records x 0x18), read by 0x4899d0.
// Joseph (species group 0): level 1 = 64 HP / 5 AP, levels 2 and 3 = +32 HP / +3 AP each -> 128 HP at level 3;
// skill points 2 + 2 = 4 (both match the Character screen).
typedef struct xp_level {
    int32_t xp;              // +0x00 experience needed (0, 0, 100, 200, 450, ...)
    uint8_t skill_points;    // +0x04 skill points gained at this level (2)
    uint8_t skill_cap;       // +0x05 per-level skill cap (xp_level_skill_cap: read as a float; 1.0 below level 1, 10.0 above 20). Equals the level number in the early levels
    struct {                 // +0x06 one group per species (char_info +0x10); read as two pairs by 0x489a30
        uint8_t hp_gain;     //   (hp_gain, hp_extra) and (ap_gain, ap_extra); the second value of each pair
        uint8_t ap_gain;     //   was 0 in the levels checked (perhaps a random range)
        uint8_t hp_extra;
        uint8_t ap_extra;
    } species[4];
    uint8_t _pad16[2];
} xp_level;

// Skill ids (Character.skill index; skill_table 0x8b3f88, 0x34-byte records, built by 0x442ba0 from strings.tbl).
// 0x55 = no skill (character_get_chain_skill).
enum skill_id {
    SKILL_HAND_TO_HAND = 0x0,
    SKILL_PROTECTION = 0x1,
    SKILL_SPEED = 0x2,
    SKILL_SWORD_WEAPONS = 0x3,
    SKILL_AXE_WEAPONS = 0x4,
    SKILL_BLUNT_WEAPONS = 0x5,
    SKILL_STAFF_WEAPONS = 0x6,
    SKILL_BOW_WEAPONS = 0x7,
    SKILL_HEAVY_ARMS = 0x8,
    SKILL_PARRY = 0x9,
    SKILL_COUNTER_ATTACK = 0xa,
    SKILL_DODGE = 0xb,
    SKILL_CRITICAL_HIT = 0xc,
    SKILL_DOUBLE_ATTACK = 0xd,
    SKILL_PUSH = 0xe,
    SKILL_KICK = 0xf,
    SKILL_TRIP = 0x10,
    SKILL_AIMED_ATTACK = 0x11,
    SKILL_BACKSTAB = 0x12,
    SKILL_ASSESS = 0x13,
    SKILL_APPRAISE = 0x14,
    SKILL_PICK_LOCK = 0x15,
    SKILL_SNEAK = 0x16,
    SKILL_HIDE = 0x17,
    SKILL_FORGE = 0x18,
    SKILL_MINE = 0x19,
    SKILL_ENCHANT = 0x1a,
    SKILL_SUMMON = 0x1b,
    SKILL_HEAL = 0x1c,
    SKILL_DARK = 0x1d,
    SKILL_ENERGY = 0x1e,
    SKILL_HOLY = 0x1f,
    SKILL_FIRE = 0x20,
    SKILL_ICE = 0x21,
    SKILL_DIVINATION = 0x22,
    SKILL_MAGIC_RESIST = 0x23,
    SKILL_SUMMON_GOLEM = 0x24,
    SKILL_SUMMON_RED_MINOTAUR = 0x25,
    SKILL_SUMMON_BLADE_OF_URATH = 0x26,
    SKILL_SUMMON_BLUE_IMP = 0x27,
    SKILL_SUMMON_BLACKFIRE_ELEMENTAL = 0x28,
    SKILL_SUMMON_BLOOD_ELEMENTAL = 0x29,
    SKILL_SUMMON_POISON_ELEMENTAL = 0x2a,
    SKILL_SUMMON_MIST_ELEMENTAL = 0x2b,
    SKILL_SUMMON_WRAITH = 0x2c,
    SKILL_SUMMON_JADE_GOLEM = 0x2d,
    SKILL_SUMMON_LAHARAH = 0x2e,
    SKILL_SUMMON_SAMURAI = 0x2f,
    SKILL_SUMMON_JADE_DRAGON = 0x30,
    SKILL_SUMMON_WATER_DRAGON = 0x31,
    SKILL_SUMMON_WIND_DRAGON = 0x32,
    SKILL_SUMMON_FOREST_DRAGON = 0x33,
    SKILL_BANISH_SUMMON = 0x34,
    SKILL_AIMED_ATTACK_EYES = 0x35,
    SKILL_AIMED_ATTACK_ARMS = 0x36,
    SKILL_AIMED_ATTACK_LEGS = 0x37,
    SKILL_BLINDNESS_STAB = 0x38,
    SKILL_ARM_SLICE = 0x39,
    SKILL_LEG_SWIPE = 0x3a,
    SKILL_DESPERATION = 0x3b,
    SKILL_STAMINA_BURST = 0x3c,
    SKILL_ADDED_BLOW = 0x3d,
    SKILL_LIFE_LEECH = 0x3e,
    SKILL_MIND_DRAIN = 0x3f,
    SKILL_MEDEVAN_TECHNIQUE = 0x40,
    SKILL_ICY_TOUCH = 0x41,
    SKILL_BURNING_HANDS = 0x42,
    SKILL_SHOCKING_GRASP = 0x43,
    SKILL_SACRIFICE = 0x44,
    SKILL_CONFUSION = 0x45,
    SKILL_SILENCE = 0x46,
    SKILL_AXESPIN = 0x47,
    SKILL_EARTHSHAKER = 0x48,
    SKILL_MEND = 0x49,
    SKILL_STEELSKIN = 0x4a,
    SKILL_OVERPOWER = 0x4b,
    SKILL_BLESS = 0x4c,
    SKILL_REVITALIZE = 0x4d,
    SKILL_PRAYER = 0x4e,
    SKILL_MUG = 0x4f,
    SKILL_SERPENT_WHIP = 0x50,
    SKILL_SERPENT_WHIP_TRIP = 0x51,
    SKILL_SERPENT_WHIP_PUSH = 0x52,
    SKILL_SERPENT_WHIP_MULTI = 0x53,
    SKILL_WEAK = 0x54,
    SKILL_NONE = 0x55,
};

// The AI sub-object at Character +0x324 (ai_state__ctor 0x4722f0; fields from ai_set_mode 0x473320). Partial.
// The constructor builds GameTimers up to +0x22C and ai_set_mode writes the byte +0x230; Character +0x558
// (= +0x234) is the next sub-object, so the record is probably 0x234 bytes.
typedef struct ai_state {
    int32_t mode;            // +0x00 enum ai_state_mode
    int32_t submode;         // +0x04
    int32_t prev_mode;       // +0x08
    int32_t prev_submode;    // +0x0C
    int32_t pending_mode;    // +0x10 mode asked for while leaving hiding (ai_set_mode then sets mode 9 sub 0xb); one writer seen
    int32_t pending_submode; // +0x14
    int32_t combat_style;    // +0x18 ai_get_combat_style / ai_set_18; from species table 0x59aee0 (ai_reset), -1 after ai_state_init; saved by ai_state_save
    int32_t timer;           // +0x1C GameTimer (4 bytes: GameTimer__GameTimer writes one dword, -1); restarted by ai_set_mode
    int32_t _unk20;
    uint8_t _unk24[0x0c];
    int32_t timer30;         // +0x30 GameTimer (ai_state__ctor)
    int32_t _unk34;
    int32_t priority;        // +0x38 999999 for direct orders (ai_attack_target, ai_travel_to_xyz, ...), n from ai_travel_to_point(pos, n), 0 from ai_go_idle and mode changes (inferred: priority of the current action)
    int32_t timer3c;         // +0x3C GameTimer, timeout 0 in ai_state_init
    int32_t timer40;         // +0x40 GameTimer
    int32_t timer44;         // +0x44 GameTimer
    int32_t _unk48;
    void*   owner;           // +0x4C the Character
    uint32_t flags;          // +0x50 bit 0 player-controlled (party.md), 0x10000000 path started (ai_path_begin)
    float   travel_dest[3];  // +0x54 travel destination
    int32_t target;          // +0x60 target handle
    int32_t prev_target;     // +0x64 swapped back into +0x60 when ai_set_mode returns to the previous mode
    uint8_t _unk68[0x14];
    int32_t _unk7c;          // +0x7C -1 (ai_state_init); ai_set_mode reads and rewrites it
    int32_t handle_slot;     // +0x80 slot freed with handle_slot_release by ai_set_mode, -1 none
    int32_t mode_target;     // +0x84 -1 on a mode change, else the target handle given with the mode
    int32_t _unk88;
    uint8_t goals[5][0x34];  // +0x8C goal slots
    int32_t cur_goal;        // +0x190 current goal; compared with a handle in ai_goal_evaluate, so possibly the goal target rather than an index
    int32_t emerge_move;     // +0x194 emerge move started by ai_begin_emerge, -1 none
    int32_t _unk198;
    int32_t timer19c;        // +0x19C GameTimer
    int32_t disable_timer;   // +0x1A0 GameTimer used by the disabled states (ai_disable_2f_local, ai_disable_31, ai_enter_sneaking)
    int32_t disable_ms;      // +0x1A4 knock-down duration (ai_knock_down_local)
    int32_t _unk1a8;         // +0x1A8 zeroed by ai_state_init
    int32_t _unk1ac;         // +0x1AC zeroed by ai_state_init
    struct {                 // +0x1B0 attacker slots around this character (ai_claim_attack_slot, ai_count_attack_slots_used)
        int32_t handle;      //   attacker handle, -1 free
        int32_t timer;       //   GameTimer
    } attackers[5];
    int32_t _unk1d8[8];      // +0x1D8 set to -1 by ai_state_init and ai_reset
    int32_t path_started;    // +0x1F8 1 from ai_path_begin, 0 on a mode change
    int32_t _unk1fc;
    int32_t path_step;       // +0x200 0 from ai_path_begin and ai_start_travel
    int32_t _unk204;         // +0x204 zeroed by ai_reset
    int32_t perception_timer;// +0x208 GameTimer, random timeout 0..50 in ai_state_init (inferred: ai_update_perception)
    int32_t _unk20c;
    void*   path;            // +0x210 current path (ai_path_free; an earlier note said +0x80, which was wrong)
    float   path_dest[3];    // +0x214 path destination (ai_set_path, ai_path_find_new, ai_path_find_append; ai_path_begin reads x and z)
    int32_t _unk220;         // +0x220 -1 (ai_state_init)
    int32_t timer224;        // +0x224 GameTimer, timeout 0 in ai_state_init
    int32_t path_timer_a;    // +0x228 GameTimer, 1000 ms timeout set by ai_path_find_new
    int32_t path_timer_b;    // +0x22C GameTimer, 1000 ms timeout set by ai_path_find_new
    uint8_t path_flags;      // +0x230 ai_path_find_new ors bits in; ai_set_mode clears 0x2; ai_path_to sets or clears 0x8
    uint8_t _pad231[3];
} ai_state;

// AI states (ai_state.mode); names from the game's own string table at 0x59ad60.
enum ai_state_mode {
    AI_STATE_INACTIVE = 0, AI_STATE_IDLE = 1, AI_STATE_TRAVELING = 2, AI_STATE_COMBAT = 3, AI_STATE_CAST_SPELL = 4,
    AI_STATE_PATROL = 5, AI_STATE_SUMMONING = 6, AI_STATE_BANISHING = 7, AI_STATE_DISABLED = 8 /* sub 0x2d = knocked down */,
    AI_STATE_HIDING = 9, AI_STATE_SNEAKING = 10, AI_STATE_MOUNT_TRAVEL = 11, AI_STATE_MOUNT_IDLE = 12,
};

// One entry of species_table (0x580e60, 223 entries): char_info +0x10 is the species id.
typedef struct species_info {
    const char* name;
    int32_t species;
    int32_t _unk08;
} species_info;

// A 3D pipeline vertex (0x30 bytes; gr_rotate_vertex / gr_project_vertex / gr_clip_*).
typedef struct gr_Vertex {
    float x, y, z;           // +0x00 view space (after gr_rotate_vertex)
    float sx, sy, sw;        // +0x0C screen x/y and 1/z (gr_project_vertex)
    uint8_t codes;           // +0x18 clip codes (gr_code_vertex)
    uint8_t flags;           // +0x19 1 projected, 2 behind the viewer, 4 temporary
    uint8_t _unk1a[0x16];
} gr_Vertex;

// One mouse button state (mouse_buttons, 0x10 bytes each).
typedef struct mouse_button {
    uint8_t down;            // +0x00
    uint8_t double_click;    // +0x01
    uint8_t _pad[2];
    int32_t presses;         // +0x04
    int32_t releases;        // +0x08
    int32_t timer;           // +0x0C RealtimeTimer
} mouse_button;

// A positional sound emitter (128 x 0x74 bytes at 0x2f07768).
// Fields from sound_create_emitter, sound_emitter__sound_emitter and sound_emitter_update_position.
typedef struct sound_emitter {
    struct sound_emitter* next; // +0x00 list head sound_emitter_list_head (0x2f076f0)
    struct sound_emitter* prev; // +0x04
    int32_t sound;           // +0x08 sound handle given at creation (sound_emitter_get_sound), -1 after reset
    int32_t playing;         // +0x0C playing handle, -1 none
    float   volume;          // +0x10 last computed volume (sound_emitter_find_quietest compares it; 0 when paused)
    float   pan;             // +0x14 last pan sent (sound_emitter_update_pan)
    float   volume_scale;    // +0x18 last scale passed to snd_set_volume; -1.0 forces an update
    int32_t frequency_sent;  // +0x1C last frequency sent (update when it moves by more than 100)
    int32_t frequency;       // +0x20 base frequency from snd_get_format (default 11025 in the getter)
    vector3 pos;             // +0x24 position (sound_emitter_update_position)
    vector3 last_pos;        // +0x30 position at the last DS3D update
    vector3 velocity;        // +0x3C
    int32_t update_ms;       // +0x48 update interval, timeout of +0x4c
    int32_t update_timer;    // +0x4C RealtimeTimer
    uint32_t flags;          // +0x50 1 used, 2 paused, 4 segment (volume and Doppler from +0x5c/+0x68), 8 playing
    int32_t repeat_delay;    // +0x54 ms between repeats, 0 = loop (sound_emitter_get_repeat_delay)
    int32_t repeat_timer;    // +0x58 GameTimer
    vector3 seg_start;       // +0x5C used with flag 4 (sound_emitter_calc_volume, sound_emitter_calc_doppler_segment); set by sound_emitter_create_at / set_pos_vel
    vector3 seg_end;         // +0x68
} sound_emitter;

// A loaded DirectSound buffer (ds_buffers, 0x100 x 0x250 bytes at 0x2f54720; cleared by ds_init_buffers).
// Field names from Dash Faction rf::DsBuffer, which has the same size; only the size is checked here.
typedef struct ds_buffer {
    char    filename[256];   // +0x000
    char    packfile[256];   // +0x100
    int32_t packfile_offset; // +0x200
    int32_t name_checksum;   // +0x204
    int32_t load_param;      // +0x208
    void*   ds_buffer;       // +0x20C IDirectSoundBuffer*
    void*   ds_3d_buffer;    // +0x210 IDirectSound3DBuffer*
    void*   wave_format_orig;  // +0x214 WAVEFORMATEX*
    void*   wave_format_fixed; // +0x218
    void*   acm_stream;      // +0x21C HACMSTREAM: acm_stream_open output (ds_stream_open), used by snd_stream_fill_acm (acm_stream_size, acm_convert)
    void*   hmmio;           // +0x220
    uint8_t data_chunk_info[0x14]; // +0x224 MMCKINFO
    uint8_t riff_chunk_info[0x14]; // +0x238 MMCKINFO
    int32_t num_refs;        // +0x24C
} ds_buffer;

// A DirectSound playing channel (32 x 0x2c bytes at 0x2f798b8; ds_channel__ctor, ds_channel_init).
// Layout as Dash Faction rf::DsChannel. Observed: ds_channel_release tests flags bit 0x8, then stops the
// stream +0xc (when >= 0) under the stream lock 0x2fbd998, then the buffer +0x00.
typedef struct ds_channel {
    void*   pdsb;            // +0x00 IDirectSoundBuffer*
    void*   pds3db;          // +0x04 IDirectSound3DBuffer*
    int32_t buf_id;          // +0x08 ds_buffers index
    int32_t stream;          // +0x0C stream slot, -1 none (Dash Faction: read_timer_id)
    int32_t sig;             // +0x10
    float   volume;          // +0x14
    int32_t load_param;      // +0x18
    int32_t _unk1c;
    float   fade_out_time;   // +0x20
    int32_t timer;           // +0x24 RealtimeTimer
    uint32_t flags;          // +0x28 1 looping, 2 paused, 4 streamed (ds_play_streamed), 8 fading (ds_stop_fade; release skips it)
} ds_channel;

// A streamed-sound slot (16 x 0x1c bytes at 0x2fbd9b0; reset by snd_stream_init, guarded by the lock 0x2fbd998).
// Fields from snd_stream_start and snd_stream_init; ds_sound_release stops every active slot on the released buffer.
typedef struct snd_stream_slot {
    int32_t buffer;          // +0x00 ds_buffers index, -1 free
    int32_t channel;         // +0x04 ds_channels index
    int32_t ring_size;       // +0x08 ring buffer bytes (2 x nAvgBytesPerSec)
    int32_t write_pos;       // +0x0C
    int32_t timer_id;        // +0x10 timeSetEvent id (250 ms)
    int32_t silence_bytes;   // +0x14 silence written after the data ended (snd_stream_service; a non-looping stream stops at 4 x ring_size)
    uint32_t flags;          // +0x18 1 active, 2 looping
} snd_stream_slot;

// One action of a script object (0x18 bytes; script_create 0x4e88a0, script_action_walk_to 0x4dc6e0).
typedef struct script_action {
    int32_t type;            // +0x00 action type (names at 0x5a045c)
    const char* name;        // +0x04 navpoint / file name argument (0 none); walk_to reads the navpoint here
    int32_t _unk08;          // +0x08 zeroed by script_create
    uint32_t arg0c;          // +0x0C default from the editor table 0x2556ab0 (12 bytes per type)
    uint32_t arg10;          // +0x10 default from 0x2556ab4; -1 or a move from charinfo_find_move for some types
    uint8_t flag14;          // +0x14 default from byte 0x2556ab8
    uint8_t _pad15[3];
} script_action;

// A level-script object (0x550 bytes; script_objects 0x24f6230, 200 records, indexed by Character +0x6dc;
// cutscene_script_objects 0x2573f68 has 32 more). Fields from script_create, script_parse_objects and
// script_write_objects. (The 0x24f6748 named here before is record 0's +0x518 timer.)
typedef struct script_object {
    int32_t end_mode;        // +0x000 what script_entry_advance does after the last action (0 restart from -1, 1 ...); script_create writes 0 or 2
    script_action actions[54]; // +0x004 at most 0x36 ("maximum script entries reached")
    void*   path;            // +0x514 path of the current walk action (ai_path_free in script_action_walk_to); 0 from script_parse_objects
    int32_t timer;           // +0x518 GameTimer
    int32_t num_actions;     // +0x51C
    int32_t cur_action;      // +0x520 action types: 33 names at 0x5a045c ("walk to" ... "one hand"); -1 before the first
    int32_t handle;          // +0x524 the scripted entity
    uint8_t suspended;       // +0x528
    uint8_t hidden;          // +0x529 +Hidden (script_parse_objects then calls character_set_ai_active(0))
    uint8_t _pad52a[2];
    int32_t trigger_level;   // +0x52C +Trigger: level_script_table index (0x40-byte records), -1 none
    char    trigger[0x20];   // +0x530 +Trigger: name (script_write_objects prints +Trigger: "level" "name")
} script_object;

// A quest record (0x6c bytes at 0x253ef40, count at 0x25443a4, at most 200), from the $Quest: part of
// events_parse_table (0x4d39c0); read by quest_get_current_step, quest_evaluate and the journal.
typedef struct quest_info {
    const char* name;        // +0x00 $Quest: name (events_string_pool)
    uint8_t side_quest;      // +0x04 1 = side quest (no +Main quest; journal_get_quest compares the byte)
    uint8_t _pad05[3];
    struct {                 // +0x08 one per +Stage: / +Invalidate: line
        const char* text;    //   +Stage: text (0 for an +Invalidate: entry)
        uint32_t flag_hash;  //   +Flag: / +Invalidate: name_hash_nocase of the story flag, 0 none
    } steps[12];
    uint8_t num_steps;       // +0x68 count of steps (byte)
    int8_t  invalidate_step; // +0x69 index of the +Invalidate: entry, -1 none (quest_get_current_step stops there)
    uint8_t _pad6a[2];
} quest_info;

// Base UI element / gadget (vsdk ui; base vtable 0x581ea0, gadget vtable 0x581e08). Fields from the agents' UI
// passes and docs/sum-exe/vsdk-ui.md. Widgets (button, static, listbox, ...) extend this.
typedef struct ui_element {
    void*   vtable;          // +0x000
    uint8_t initialized;     // +0x004 0 from ui_element__ui_element, 1 from ui_element__init
    uint8_t _pad005[3];
    int32_t kind;            // +0x008 1 window, 0 gadget (ui_element__init)
    int32_t x, y, w, h;      // +0x00C rectangle
    uint8_t hidden;          // +0x01C
    uint8_t disabled;        // +0x01D ui_gadget__is_disabled / set_disabled / set_enabled
    char    name[0x102];     // +0x01E
    int32_t id;              // +0x120 default event value (ui_gadget__send_event)
    void*   tooltips;        // +0x124 tooltip list head: next (empty list points at +0x124 itself)
    void*   tooltips_prev;   // +0x128 prev (ui_element__ui_element sets both to +0x124)
    uint8_t _unk12c[0x118];
    void*   callback;        // +0x244 event callback
    uint8_t owned;           // +0x248 "owned" argument of ui_gadget__init
    uint8_t _pad249[3];
    int32_t hotkeys[2];      // +0x24C -1 from ui_gadget__clear_hotkeys (a window keeps its group here instead, ui_window__is_disabled)
    int32_t gadget_type;     // +0x254 1 container, 2 button, 3 static, 4 checkbox, 5 radio, 6 inputbox, 8 listbox
    int32_t _unk258;         // +0x258 zeroed by ui_gadget__init
    void*   window;          // +0x25C owning ui_window
    struct ui_element* window_prev; // +0x260 the window's gadget list (head at window +0x250, tail pointer window +0x4b0)
    struct ui_element* window_next; // +0x264
    struct ui_element* parent;      // +0x268 parent container
    struct ui_element* first_child; // +0x26C
    struct ui_element* next_sibling;// +0x270
} ui_element;

// The game camera, one static object game_camera (0x60a018; camera methods and camera_process_input,
// docs/sum-exe/camera.md). It ends with the rendered view at +0x278..+0x2a7 (camera_view_pos 0x60a290,
// camera_view_orient 0x60a29c); level_filename follows at 0x60a2c8.
typedef struct game_camera {
    int32_t mode;            // +0x000 0 low, 1 high, 2 automatic, 3 forced high, 5 glide (camera_set_mode)
    int32_t prev_mode;       // +0x004
    uint8_t high_view_suppressed; // +0x008 from the detail level (detail_apply_level_settings)
    uint8_t high_view_requested;  // +0x009 a mode-3 request remembered while +0x008 is set
    uint8_t _unk00a[2];
    uint32_t flags;          // +0x00C 1 follow drives the view, 2 follow target, 8 sequence, 0x10 free look, 0x20 default params stored, 0x40 block zoom input (set and cleared by camera_auto_rotate_update), 0x80 sequence transform, 0x100 restore zoom, 0x200 back to mode 3
    int32_t num_saved_states;// +0x010 state stack depth, max 8 (camera_push_state / camera_pop_state)
    uint32_t saved_states[8][9]; // +0x014 9 dwords each (camera_get_state: flags, distance, yaw, zoom, elevation, fov, look height, mode, prev mode)
    float   zoom_high;       // +0x134 zoom remembered for the high view (start 1.0)
    float   zoom_low;        // +0x138 zoom remembered for the low view (start 0.8)
    void*   sequence;        // +0x13C running camera sequence (a cutscene shot), virtual dtor
    vector3 look_at;         // +0x140 target position + centre offset, y + look_height (camera_update_target_pos)
    void*   target;          // +0x14C followed object (pointer: pos +0x10 and centre offset +0x28 are read through it)
    vector3 fixed_eye;       // +0x150 eye position glided to in mode 5 (camera_set_target)
    vector3 wanted_dir;      // +0x15C wanted direction for auto-turning (camera_auto_rotate_update; world_pick_under_mouse writes it)
    int32_t orient_mode;     // +0x168 2 = set the view directly (camera_set_view_params, target moving), else smoothed
    float   default_distance;   // +0x16C camera_set_default_view_params, same order as camera_set_view_params
    float   default_elevation;  // +0x170
    float   default_zoom;       // +0x174
    float   default_look_height;// +0x178
    float   default_eye_offset; // +0x17C
    float   yaw;             // +0x180 radians, wrapped
    float   zoom;            // +0x184 clamped to [0.3, camera_zoom_max]
    float   elevation;       // +0x188 eye height above the target per unit of zoom, clamped to [-2, 9]
    float   fov;             // +0x18C vertical, degrees (73.74 by default)
    float   distance;        // +0x190
    float   look_height;     // +0x194 look-at height offset
    float   eye_offset;      // +0x198 eye height offset, current
    float   eye_offset_wanted; // +0x19C eye height offset, wanted (moves at 1.5 per second)
    float   saved_zoom;      // +0x1A0 zoom before auto-turning zoomed out, restored with flag 0x100
    vector3 eye_pos;         // +0x1A4 follow camera eye
    matrix3 eye_orient;      // +0x1B0 follow camera orientation
    int32_t auto_view_timer; // +0x1D4 GameTimer: 1 s checks, 5 s back to low (camera_auto_view_update)
    uint8_t enemy_near;      // +0x1D8 last camera_enemy_near_leader result
    uint8_t _pad1d9[3];
    vector3 dir_history[4];  // +0x1DC last target offsets, averaged by camera_update_orientation
    float   dt_history[4];   // +0x20C frame time of each
    int32_t history_index;   // +0x21C ring index (mod 4)
    int32_t history_count;   // +0x220 up to 4
    vector3 eye_velocity;    // +0x224 per-axis velocity of camera_approach_value (zeroed by camera_snap_to_target)
    int32_t _unk230;         // +0x230 zeroed by camera_snap_to_target
    float   height[2];       // +0x234 / +0x238 target height, smoothed / wanted (camera_smooth_height)
    float   input_yaw;       // +0x23C this frame's input rates (camera_set_input)
    float   input_zoom;      // +0x240
    float   input_elevation; // +0x244 always 0 in retail (camera_key_extra_axis)
    vector3 seq_origin;      // +0x248 camera_set_sequence_transform (used with flag 0x80)
    matrix3 seq_orient;      // +0x254
    vector3 view_pos;        // +0x278 rendered view = camera_view_pos (0x60a290)
    matrix3 view_orient;     // +0x284 camera_view_orient (0x60a29c)
} game_camera;               // 0x2a8 bytes

// Compressed quaternion (four int16, 1.0 = 0x3fff), used for animation keys (vsdk math).
typedef struct cquat {
    int16_t x, y, z, w;
} cquat;

// A multiplayer packet type (mp_packet_table 0x5951b8, 66 entries x 0x1c), e.g. {0, handler, 1, "GM_QURY"}.
typedef struct mp_packet_info {
    int32_t type;            // +0x00
    void  (*handler)(void);  // +0x04 receive handler (argument list not mapped)
    int32_t flag;            // +0x08
    int32_t sent;            // +0x0C send counter, bumped by the senders
    char    name[12];        // +0x10 "GM_QURY", "ACTV_GM", "JN_RQST", ...
} mp_packet_info;

// An AI group (10 x 0x54 bytes at 0x243e040; Character +0x4cc points at one). Saved in the LENT chunk.
typedef struct ai_group {
    int32_t count;           // +0x00
    int32_t members[6];      // +0x04 handles
    uint8_t _unk1c[0x2c];
    int32_t leader;          // +0x48
    uint32_t flags;          // +0x4C
    int32_t timer;           // +0x50 GameTimer
} ai_group;

// A spell/visual effect object (0x10c bytes, fx_pool 0x2305bc0, 48 records; docs/sum-exe/effects.md). Partial.
// Built on the base object (obj__obj, then vtable 0x581aac; obj_link_new(2) in fx_alloc); fields from
// fx_obj__ctor, fx_new, fx_create_instance, fx_create_missile, fx_update_missile, fx_frame, fx_obj_destroy.
typedef struct effect_object {
    void*   vtable;          // +0x00 0x581aac (slot 0 fx_obj_destroy)
    uint8_t _unk04[0x08];
    int32_t type;            // +0x0C object type, 2 for effects (fx_set_target checks it)
    vector3 pos;             // +0x10 position (fx_create_instance, fx_create_missile)
    uint8_t _unk1c[0x1c];
    matrix3 orient;          // +0x38 orientation
    uint8_t _unk5c[0x24];
    int32_t handle;          // +0x80
    uint32_t flags;          // +0x84 1 active, 4 stopping (fx_stop), 0x40 paused for a cutscene, 0x200
    uint8_t _unk88[0x10];
    struct effect_object* next; // +0x98 fx_active_list (0x2305920)
    struct effect_object* prev; // +0x9C
    void*   instance;        // +0xA0 vfx instance (vfx_instance_free in fx_obj_destroy unless +0xbc bit 0x8)
    int32_t kind;            // +0xA4
    int32_t spell;           // +0xA8 spell index, -1 none
    int32_t source;          // +0xAC source handle (-1 from fx_create_spell_effect_*)
    int32_t caster;          // +0xB0 casting character's handle (missiles; the caster in fx_create_spell_effect_facing calls)
    int32_t owner;           // +0xB4 character the effect is on or aimed at: missile target (homing bone), status holder's character
    int32_t trail_bitmap;    // +0xB8 missile trail bitmap (bm_load)
    uint32_t bits;           // +0xBC bit 0x8 instance not owned; bits 17-19 count of +0xc4; top byte set to 0xff by the creators
    int32_t target;          // +0xC0 target (fx_set_target); 0 from fx_new
    int32_t hit_handles[6];  // +0xC4 characters already hit by an area effect, -1 free (fx_frame)
    int32_t timer;           // +0xDC GameTimer (fx_obj__ctor)
    int32_t sound;           // +0xE0 sound emitter handle, -1 none (sound_emitter_delete in fx_stop)
    float   radius;          // +0xE4 growing radius of area effects (fx_frame); 0.1 for missiles
    vector3 start;           // +0xE8 missile start position (fx_update_missile: distance travelled)
    vector3 aim;             // +0xF4 missile aim point (target bone position)
    int32_t avoid_area;      // +0x100 avoid area index, -1 none (avoid_area_remove in fx_obj_destroy)
    int32_t timer2;          // +0x104 GameTimer (fx_obj__ctor)
    void*   spell_effect;    // +0x108 spell_effect_add record (spell_effect_free in fx_obj_destroy)
} effect_object;

// A dynamic light (0x94 bytes; lights 0x2ca5de0, 400 records; list head 0x2cb4520). Fields from
// light_create_point / light_create_spot, light_transform_to_instance and lights_shade_point.
typedef struct light {
    struct light* next;      // +0x00
    struct light* prev;      // +0x04
    uint16_t flags;          // +0x08 low bits type (2 point, 3 spot, 4 projected bitmap); bit 14 enabled (light_set_all_enabled); bit 15 dynamic
    uint16_t _pad0a;
    vector3 pos;             // +0x0C
    vector3 spot_dir;        // +0x18 spot direction
    float   spot_angle[2];   // +0x24 / +0x28 spot angles (degrees); negated cosines at +0x60
    float   radius_inner;    // +0x2C
    float   radius_outer;    // +0x30
    float   color[3];        // +0x34 colour times intensity
    vector3 local_pos;       // +0x40 position in the current instance space (light_transform_to_instance)
    vector3 local_dir;       // +0x4C direction in instance space
    float   radius_inner_sq; // +0x58
    float   radius_outer_sq; // +0x5C
    float   spot_neg_cos[2]; // +0x60 / +0x64
    int32_t bitmap;          // +0x68 projected bitmap (type 4)
    matrix3 proj_orient;     // +0x6C projection orientation (vector3__rotated in lights_shade_point; built in light__light)
    float   sort_dist;       // +0x90 distance for sorting (light_compare_dist)
} light;

// One shop list entry (8 bytes, level_shop_items 0x2491ac0, room for 1000; docs/sum-exe/shops-loot.md).
typedef struct shop_item {
    int32_t item_type;       // +0x0 item_types index (shop_find_item_type compares it)
    int32_t price;           // +0x4 sell: $Value; buy: value x 0.7 (multiplayer x 0.1, at least 1)
} shop_item;

// A shopkeeper (0x14 bytes, level_shopkeepers 0x2490ca0, room for 15, count 0x24902a0), from
// level_parse_shopkeeper (0x4cc4d0); buysell_shop (0x5b67f4) points at one (shop_set_shopkeeper_from_char).
typedef struct shopkeeper {
    const char* name;        // +0x00 the character's name (level_shopkeeper_find compares it)
    int32_t num_sell;        // +0x04 +Sell List: entries
    shop_item* sell;         // +0x08
    int32_t num_buy;         // +0x0C +Buy List: entries (shop_find_item_type searches this list)
    shop_item* buy;          // +0x10
} shopkeeper;

// A container object (0xe8 bytes, pool 0x2302fe0, 40 records; container__ctor 0x46a890, vtable 0x5819cc).
// Built on the base object (obj__obj); fields from container_add_item / container_get_item, container_lockpick
// and the multiplayer open/close handlers.
typedef struct container {
    uint8_t _obj00[0x98];    // +0x00 base object (handle +0x80)
    void*   items[16];       // +0x98 item objects (each item's +0xa4 points back here)
    int32_t num_items;       // +0xD8
    int32_t user;            // +0xDC -1 when not in use (container_request_open, mp_send_container_close)
    int32_t mesh_item_type;  // +0xE0 item type index of the container mesh, -1 none (container_is_in_view)
    float   lock;            // +0xE4 lock value, 0 unlocked (container_lockpick compares the skill roll with it)
} container;

// A cutscene.tbl entry (0x140 bytes, cutscene_entries 0x6034c8, at most 85, count 0x609f30), from
// cutscene_parse_table (0x4355e0); docs/sum-exe/cutscenes.md. List entries are pointers to records in
// cutscene_alloc_pool. Array sizes are the room the layout leaves; the parser's own limits were not traced.
typedef struct cutscene_entry {
    const char* name;        // +0x000 $Cutscene:
    uint16_t flags;          // +0x004 1 +NoAbort, 2 +HideUnscripted
    uint16_t _pad006;
    const char* level;       // +0x008 $Level: ("any" matches every level)
    const char* script;      // +0x00C $Script: (defaults to $Level)
    int16_t num_cameras;     // +0x010 $Camera: count (max 4)
    uint16_t _pad012;
    const char* cameras[4];  // +0x014 .csc file names
    int16_t num_effects;     // +0x024 $Animation: / $Character Animation: lines without a shot number
    uint16_t _pad026;
    void*   effects[2];      // +0x028 started by cutscene_bind_actors
    int16_t num_shot_effects;// +0x030 effect lines with a shot number
    uint16_t _pad032;
    void*   shot_effects[10];// +0x034 16-byte records: name, shot (word +4), character (+8), length (word +0xc)
    const char* soundtrack;  // +0x05C $Soundtrack:
    int16_t num_minitracks;  // +0x060 +Minitrack: count
    uint16_t _pad062;
    void*   minitracks[4];   // +0x064 records: name, shot (word +4)
    int16_t num_fogs;        // +0x074 $Fog: count
    uint16_t _pad076;
    void*   fogs[16];        // +0x078 records: near (+0), far (float +4), shot (word +8)
    int16_t num_vissets;     // +0x0B8 $VisSet: count (cutscene_parse_visset)
    uint16_t _pad0ba;
    void*   vissets[16];     // +0x0BC (inferred from the layout)
    int16_t num_fades;       // +0x0FC $Fade In: / $Fade Out: count
    uint16_t _pad0fe;
    void*   fades[16];       // +0x100 records: time (+4), length (+8), shot (word +0xc)
} cutscene_entry;

// One cutscene camera shot (cutscene_camera_sequence, 0x70 bytes; cutscene_sequences 0x601e80, 50 records,
// count 0x601e7c). Read by cutscene_camera_read_track (0x493fd0) from a .csc file; sampled by
// cutscene_camera_sequence_sample (0x494310) and passed to camera_start_sequence.
typedef struct cutscene_camera_shot {
    void*   vtable;          // +0x00 0x581af0 (slot 1 get_pos, slot 2 get_orient)
    int32_t state;           // +0x04 camera_sequence_get_state
    float   fov;             // +0x08 read from the file; camera_update_sequence sets the camera fov from it (x 0x581a58); cutscene_camera_sequence_time_left subtracts it as elapsed time
    float   start;           // +0x0C shot start, seconds
    float   end;             // +0x10 shot end, seconds
    uint8_t track[0x2c];     // +0x14 anim_track: key counts +0x14 (position) / +0x16 (rotation), key arrays +0x1c / +0x20 (pools cutscene_cam_keys40 / keys20)
    vector3 pos;             // +0x40 sampled position (anim_track__get_position)
    matrix3 orient;          // +0x4C sampled orientation (anim_track__get_rotation, quaternion__to_matrix_b)
} cutscene_camera_shot;

// A world map location marker (0x2c bytes, worldmap_locations 0x5f96c8, 8 records), from
// worldmap_init_locations (0x42f3b0), worldmap_frame and worldmap_draw_location (0x42fac0).
typedef struct worldmap_location {
    uint32_t flags;          // +0x00 1 shown, 2 off screen (draw the arrow), 4 on screen (draw the label)
    void*   navpoint;        // +0x04 navpoint_find record (0x30 bytes)
    float   arrow_scale;     // +0x08 worldmap_marker_scale_for_distance
    float   arrow_angle;     // +0x0C worldmap_marker_screen_angle
    float   arrow_x;         // +0x10 worldmap_marker_place_on_edge
    float   arrow_y;         // +0x14
    float   label_x;         // +0x18 worldmap_center_label: projected point minus half the box
    float   label_y;         // +0x1C
    int32_t label_w;         // +0x20 string width + font padding + 2
    int32_t label_h;         // +0x24
    const char* label;       // +0x28 "Masad", "Lenele", ... (table 0x592cd4)
} worldmap_location;

// A ground decal (0x128 bytes; 32 records 0x2480a48..0x2482f48, decal_record__ctor 0x4bbed0), from
// decal_place (0x4bc320), decal_build_clip_planes, decal_set_u_axis / decal_set_v_axis and decals_reset.
typedef struct decal {
    int32_t timer;           // +0x000 GameTimer
    int32_t bitmap;          // +0x004 decal_place argument 0, -1 from the constructor
    vector3 pos;             // +0x008
    float   radius;          // +0x014 computed by decal_build_clip_planes; passed to projector_record__set
    vector3 normal;          // +0x018
    uint32_t flags;          // +0x024 low 29 bits lifetime (0 = recycled by decals_recycle_unused), 0x20000000 clipped, 0x40000000 in use, 0x80000000 a bool argument of decal_place
    struct decal* next;      // +0x028 active ring 0x2483550 / free ring 0x248354c
    struct decal* prev;      // +0x02C
    uint32_t arg30;          // +0x030 decal_place argument 6 (0 from place_ground_decal)
    float   plane_scale;     // +0x034 decal_place argument 7 (0.75 from place_ground_decal), scales the clip planes
    uint8_t projector[0xe0]; // +0x038 projector record (projector_record__ctor / __set); bits 1-3 of its first dword pick the axes
    int32_t u_axis;          // +0x118 decal_set_u_axis
    int32_t v_axis;          // +0x11C decal_set_v_axis
    float   u_scale;         // +0x120
    float   v_scale;         // +0x124
} decal;

// A lamp glow (0x18 bytes, lamp_glows 0x24893e8, room for 300 (inferred), count 0x248b008), from
// lamp_glow_add (0x4c0a90), lamp_glow_set, lamp_glow_draw and lamp_glow_fade_in / _out.
typedef struct lamp_glow {
    vector3 pos;             // +0x00
    float   size;            // +0x0C argument x a constant (lamp_glow_add)
    float   alpha;           // +0x10 0..1, faded by frame time; scales the sprite (lamp_glow_draw)
    uint32_t _unk14;         // +0x14 zeroed by lamp_glow_add
} lamp_glow;

// A boss threat record (0x18 bytes, 5 per boss, boss_get_threat_table 0x4a1200), from boss_threat_add
// (0x4a13b0) and boss_threat_decay (0x4a1310); docs/sum-exe/bosses.md.
typedef struct boss_threat {
    int32_t handle;          // +0x00 attacker handle, -1 free (also reused when the character is gone)
    float   threat[4];       // +0x04 boss_threat_add adds the amount to +0x4, +0x8 and +0x10, and to +0xc when its flag argument is 0; boss_threat_decay scales +0x8 down by the boss's +0x610
    int32_t timer;           // +0x14 GameTimer, 20 s (0x4e20)
} boss_threat;
