// stubs_status.h - declarations that new_status.cpp needs beyond stubs_menus_dialogue.h, ui_screen.h and
// stubs_gamecreate_journal.h. Names and addresses from symbols/sum.symbols; argument lists read from the call
// sites in Sum.exe v1.40 (build/sum.asm). "(type guessed)" marks return or parameter types the call sites do
// not show. Not original source.
#ifndef STUBS_STATUS_H
#define STUBS_STATUS_H

#include "stubs_menus_dialogue.h"     // types.h (Character, Player, ai_state, gr__Screen), gr_*, ui_*, game_*
#include "stubs_gamecreate_journal.h" // RealtimeTimer, gr_split_str, mouse_get_delta, gr_get_clip, gr__set_clip,
                                      // statscreen_refresh_anim, shop_set_shopkeeper_from_char, ui_has_owner_character

#ifndef XSTR
#define XSTR(text, id) (localize_strings[id])
#endif

// ---- thiscall methods on the C structs of symbols/types.h ------------------------------------------
// types.h declares Character, ai_state and matrix3 as C structs, which cannot carry member functions. Until
// the headers are merged, the methods the screen calls are declared on these layout-free shims and the
// pointer is converted with the *_M macros. The call is the same thiscall (ECX = the object) either way;
// in the merged source these become status_character->get_skill(2) and so on.
class Character_M {                                   // living_entity methods (living_entity.cpp)
public:
    float get_skill(int skill);                       // 0x462440 character_get_skill
    int   equip_bonus(int stat);                      // 0x4627c0 character_equip_bonus (0 HP, 1 AP)
    int   get_max_stat(int stat);                     // 0x462840 character_get_max_stat (0 HP, 1 AP)
    float get_resistance(int type);                   // 0x4626b0 character_get_resistance
    float resist_equip_bonus(int type);               // 0x462660 character_resist_equip_bonus
    bool  has_status(int status);                     // 0x455950 character_has_status
    const char *hero_model_prefix();                  // 0x462ec0 character_hero_model_prefix
};
#define CHAR_M(c) ((Character_M *)(c))

class living_combat_M {                               // the sub-object at Character +0x558
public:
    float get_protection();                           // 0x464de0 character_get_protection
};
#define COMBAT_M(c) ((living_combat_M *)(c)->combat)

class ai_state_M {                                    // the sub-object at Character +0x324
public:
    void set_18(int style);                           // 0x473610 ai_set_18 (combat style, ai_state +0x18)
    int  get_combat_style();                          // 0x473620 ai_get_combat_style
};
#define AI_M(c) ((ai_state_M *)(c)->ai)

class matrix3_M {
public:
    void make_identity();                             // 0x511050 matrix3__make_identity
};
#define MATRIX3_M(m) ((matrix3_M *)(m))

// ---- vsdk\gr ----------------------------------------------------------------------------------------
extern gr__Screen gr_screen;                          // 0x25d8d30 (max_w +4, max_h +8, offset_x +0x2c, offset_y +0x30)
extern unsigned int gr_rect_mode;                     // 0x25d88ac
extern unsigned int gr_mode_flat_alpha;               // 0x25d8d28
extern float reported_frametime;                      // 0x5a5740 (frametime.cpp)
extern bool gr_byte_25d921d;                         // 0x25d921d unnamed; set to 1 by the Direct3D code (0x539efd)
void gr_rect(int x, int y, int w, int h, unsigned int mode, float z);   // 0x5077d0 (z: 1.0f or FLT_MAX here)
void gr_bitmap(int bm, int x, int y, unsigned int mode);               // 0x506f90
void gr_set_alpha_test(int enable);                                     // 0x508110 (type guessed)
void gr_setup_3d(matrix3 *orient, vector3 *pos, float fov, int a, int b); // 0x50dec0 (a, b = 1 here)

// The record at the third argument of character_instance::render. The symbol calls its constructor
// gr_light_desc (it builds a vector3 at +0xc and resets +0..+0x1c); status_draw_model stores the
// multiplayer appearance at +0x10 and the hero model prefix at +0x1c. 0x20 bytes on the stack.
class gr_light_desc {
public:
    gr_light_desc();                  // 0x50f3e0
    int _unk00;                       // +0x00
    int _unk04;                       // +0x04 -1
    int _unk08;                       // +0x08 0xff
    float _unk0c;                     // +0x0c
    void *appearance;                 // +0x10 charinfo_get_multi_appearance
    float _unk14;                     // +0x14 -1.0f
    int _unk18;                       // +0x18
    const char *model_prefix;         // +0x1c character_hero_model_prefix
};

// ---- vsdk\ca ----------------------------------------------------------------------------------------
class character_instance {            // Character +0x314
public:
    void process(float frametime, int a, int b);                          // 0x51a890 (a, b = 0 here)
    void render(vector3 *pos, matrix3 *orient, gr_light_desc *light);     // 0x520420 (argument names guessed)
};

// ---- localize / strings -----------------------------------------------------------------------------
char *localize_string_or_invalid(char **p);          // 0x5054f0: *p, or "INVALID"
void  int_to_string_with_commas(char *buf, int size, int value);         // 0x5152a0 (return unused)

// ---- interface (new_interface.cpp) ------------------------------------------------------------------
extern Character *status_character;                  // 0x5bd810
extern Color ui_color_white;                          // 0x5bd324
extern Color ui_color_blue;                           // 0x5bd328
extern Color ui_color_dark_blue;                      // 0x5bd790
extern Color ui_color_disabled;                       // 0x5bd794
extern Color ui_color_dark_green;                     // 0x5bd8f0
extern Color ui_color_dark_red;                       // 0x5bd924
extern Color ui_color_red;                            // 0x5bd954
extern unsigned int ui_choice_popup_flags;            // 0x5bd77c bit 31 shown
extern const int ui_choice_popup_margin[3];           // 0x57ef64 unnamed int[3] {5, 5, 5} per resolution_index, also
                                                      // read by ui_choice_popup_show (name ours)
void ui_choice_popup_show(const char *title, int x, int y, int num, char **items,
                          int a, int b, int c);       // 0x4136a0 (title and a..c always 0 here; types guessed)
int  ui_choice_popup_frame(int key);                  // 0x4138b0: choice, -1 none yet, -2 cancelled
void ui_choice_popup_draw();                          // 0x413c70
void ui_tooltip_set(int x, int y, int w, int h, const char *text);   // 0x414750
bool interface_input_ready();                         // 0x4148e0
void ui_apply_pulse_rgb(float *r, float *g, float *b, float offset); // 0x414bb0 (offset 0 here)
void ui_3d_begin(vector3 *pos);                       // 0x412400 (type guessed)
void ui_3d_end();                                     // 0x4125a0
void statscreen_show_next_member(int a);              // 0x413320 (1 here)
void statscreen_show_prev_member(int a);              // 0x413340 (1 here)

// new_icam: the camera table of status_cameras.tbl (0x3628 bytes). The inline empty constructor is our
// explanation of the empty static initialiser init_empty_42b1d0 at the head of the unit (see NOTES).
class new_icam_table {
public:
    new_icam_table() {}
    unsigned char data[0x3628];
};
void new_icam_load(new_icam_table *table, const char *filename);         // 0x410760 (return unused)
void new_icam_get(new_icam_table *table, const char *name, matrix3 *orient, vector3 *pos); // 0x410b70

// ---- objects / characters ---------------------------------------------------------------------------
extern Character living_list;                         // 0x8eca60 sentinel; living_list.live_next = 0x8ecd78
extern Player *summon_player;                         // 0x5fcc84
Player *player_from_handle(int handle);               // 0x433eb0
int  party_count_active();                            // 0x434350
void statscreen_unload_anim(Character *c);            // 0x449100
void *charinfo_get_multi_appearance(char_info *info); // 0x449b80
int  xp_for_level(int level);                         // 0x489a60
float xp_level_skill_cap(int level);                  // 0x489bf0
void mp_send_character_skills(Character *c);          // 0x480ba0

// An equipped item (Character.equipment[i]); +4 is the item type.
struct item_info {
    int _unk00;
    ItemType *type;                                   // +0x04
};

// ---- skills and chain attacks (characterinfo.cpp) ---------------------------------------------------
struct skill_info {                                   // 0x34 bytes, skill_table 0x8b3f88
    char **name;                                      // +0x00 localize_strings slot
    unsigned char _unk04[0x2c - 0x04];
    char **description;                               // +0x2c localize_strings slot (tooltip)
    unsigned char _unk30[0x34 - 0x30];
};
extern skill_info skill_table[84];                    // 0x8b3f88
int  chain_count_unlocked_skills(Player *p);          // 0x449830
int  chain_get_unlocked_skill(Player *p, int n);      // 0x449890
int  chain_config_find_skill(Player *p, int skill);   // 0x449910: slot 0..3, -1 none
void character_set_chain_skill(Player *p, int slot, int skill);   // 0x4493e0 (sum.symbols says char; the call passes the Player)

// ---- localize_strings tables of other units (stored values are slot addresses) ---------------------
extern char **ai_combat_style_names[6];               // 0x22e49a0 (ai.cpp, init_strptrs_22e49a0)
extern char **status_effect_names[18];                // 0x8ec9e8 (items.cpp, init_strptrs_8ec9e8)

#endif
