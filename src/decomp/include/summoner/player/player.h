// player.h - Sum.exe v1.40 party lists and Player records (Summoner\player\player.cpp, 0x433340-0x434390)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef PLAYER_H
#define PLAYER_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/living_entity.h"

struct mp_player;   // summoner/multi/multi.h

#define MAX_PLAYERS 5                 // five records, 0x188 apart, 0x5fc4d8..0x5fcc80

// Player +0x10 flags
#define PF_SUMMON 0x2
#define PF_HOLD   0x4                 // solo mode: stay put, ignore group orders
#define PF_LEADER 0x8

// {skill, level, value}: from character level `level` on, Character.skill[skill] = value.
struct skill_progression {
    int skill;
    int level;
    int value;
};

// 0x188 bytes. No constructor of its own: the compiler-generated one builds the 84 GameTimers
// (static initialisers 0x433350, 0x4333a0, 0x4333d0).
class Player {
public:
    // thiscall members; the symbol names are player__<name> or player_<name>
    void set_ai_control(bool on);             // 0x433750 player__set_ai_control
    Character *create(const vector3 *pos, const matrix *orient, const char *name, bool leader); // 0x433790 player_create
    bool make_leader();                       // 0x433960 player__make_leader
    bool drop_leader();                       // 0x4339a0 player__drop_leader
    bool is_leader();                         // 0x4339e0 player__is_leader
    bool skill_unlocked(int skill);           // 0x4339f0 player_skill_unlocked
    int  skill_unlocked_at_level(int skill);  // 0x433b10 player_skill_unlocked_at_level
    void init_from_character(Character *c);   // 0x433b70 player_init_from_character
    void input_reset();                       // 0x433730 player_input_reset (touches only globals)

    Player *next;                     // +0x000 circular list
    Player *prev;                     // +0x004
    int char_info;                    // +0x008 char_infos index
    int handle;                       // +0x00c character object handle, -1 none
    unsigned int flags;               // +0x010 PF_*
    union {
        mp_player *mp_owner;          // +0x014 owning multiplayer player
        mp_player *player;            //        new_itemtrade.cpp's name through mp_trade_peer (D401)
    };
    int _unk18;                       // +0x018
    int _unk1c;                       // +0x01c
    int num_progression;              // +0x020
    skill_progression *progression;   // +0x024 (not const: Max_skills rewrites the levels)
    GameTimer skill_cooldown[MAX_SKILLS]; // +0x028
    float lent_max_hp;                // +0x178
    int skill_points;                 // +0x17c
    int skill_cap;                    // +0x180
    int default_chain;                // +0x184
};
LAYOUT_CHECK_32(Player, sizeof(Player) == 0x188);

// The list macros (list_init / list_insert / list_append / list_remove) are in vsdk/vsdk.h.

extern const char *hero_model_names[14];                  // 0x5934d0 (label ours)
extern skill_progression joseph_skill_progression[17];   // 0x593508
extern int joseph_skill_progression_count;               // 0x5935d4
extern skill_progression jekhar_skill_progression[13];   // 0x5935d8
extern int jekhar_skill_progression_count;               // 0x593674
extern skill_progression flece_skill_progression[16];    // 0x593678
extern int flece_skill_progression_count;                // 0x593738
extern skill_progression rosalind_skill_progression[14]; // 0x593740
extern int rosalind_skill_progression_count;             // 0x5937e8

extern Player party_list;             // 0x5fc350
extern Player player_free_list;       // 0x5fc1c8
extern Player player_list_other;      // 0x5fc030
extern Player Players[MAX_PLAYERS];   // 0x5fc4d8 (label ours)
extern Player *party_leader;          // 0x5fcc80
extern Player *summon_player;         // 0x5fcc84
extern Character *cur_character;      // 0x5fcc88
extern Character *cur_summon;         // 0x5fcc8c
extern bool combat_mode;              // 0x5fccb4 (combat_mode_set, hud_toggle_combat_mode)
extern bool party_lists_initialized;  // 0x5fcc90 (label ours)
extern bool Max_skills;               // 0x5fcc91
extern unsigned int player_input[3];  // 0x5fcc98 {flags, mouse x, mouse y}

void cmd_max_skills();
Player *player_alloc_slot();
void player_clear_handle_of(int handle);
void party_remove_player(Player *p, bool delete_obj);
void party_remove_all();
void party_lists_init();
void party_set_leader(Player *p);
Player *player_from_handle(int handle);
void player_start_skill_cooldown(Character *c, int skill);
bool party_remove_member(const char *name);
Player *party_add_member(const char *name);
void party_sort_by_species();
int party_count_active();

#endif
