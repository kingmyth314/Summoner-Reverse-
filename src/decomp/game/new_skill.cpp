// new_skill.cpp - Sum.exe v1.40, code 0x427240-0x42984f (screen 6, the Skills roll-out: six skill groups
// as tabs, the active skills of the group as rows, the passive ones greyed below them; the summon and
// aimed-attack choice popups; at the end skill_is_usable / skill_is_available / character_use_skill, which
// the HUD hot buttons, the inventory's identify and world_mouse_frame call too).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x580710-0x58075b (layout tables, the 1/710 literal), .data 0x590670-0x59185f (button table,
// icon names, literals), .bss 0x5f4a78-0x5f5377.
//
// skill_table +0x14 is the group (0 combat, 1 stealth, 2 creation, 3 examination, 4 weapons, 5 spells),
// +0x20 bit 0 marks a passive skill: such skills go to skills_groups and are drawn greyed under the active
// rows (sum.symbols calls them "group headers"). A summon (Character flag 0x100) lists only passive skills
// of the groups 0, 4 and 5.

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/rendering/levelrender.h"
#include "engine/spells/spells.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/interface/new_hud.h"
#include "summoner/interface/new_spell.h"
#include "summoner/interface/new_skill.h"
#include "experimental/ui_shift.h"   // EXPERIMENTAL_BUILD only (widescreen); keep it the last include

#define SKILLS_NUM_BUTTONS 49
#define SKILLS_NUM_ICONS   56      // skill ids 0..0x37
#define SKILLS_NUM_LISTED  36      // the screen walks skill_table 0..0x23
#define SKILLS_MAX_ROWS    7
#define SKILLS_MAX_POPUP   16

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024); names ours.
const int skills_list_pos[3][2]    = { { 27, 171 }, { 34, 214 }, { 43, 274 } };     // 0x580710 first row icon
const int skills_text_offset[3][2] = { { 45, 8 }, { 56, 10 }, { 72, 4 } };         // 0x580728 name from icon
const int skills_text_width[3]     = { 270, 338, 432 };                            // 0x580740 level right edge
const int skills_row_height[3]     = { 34, 40, 40 };                               // 0x58074c
// 0x580758 is the 1/710 (0.0014084507f) literal of character_use_skill's identify test.

// ---- .data ---------------------------------------------------------------------------------
// 0x590670 (*new* name), 49 x 0x40: the layout of new_spell.cpp's table (17/18 go to the spells, 23-40 the
// group icons, 41-46 the group names).
const ui_button_def skills_button_defs[SKILLS_NUM_BUTTONS] = {
    { "Skills-Rollout",       { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Spelllist-Jos",        { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Jek",        { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Flece",      { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Ros",        { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Golem",      { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-RedMin",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BldeUrath",  { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BlueImp",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BlkElem",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-BldElem",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-PoisElem",   { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-MstElem",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-Wraith",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-JdeGlm",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-SpirLah",    { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Spelllist-CelSam",     { { 50, 41 },   { 63, 51 },   { 105, 67 } },  0 },
    { "Icon-ArrwBRTHilite",   { { 143, 23 },  { 179, 29 },  { 225, 36 } },  1 },
    { "Icon-ArrwBRTPress",    { { 143, 23 },  { 179, 29 },  { 225, 36 } },  0 },
    { "Icon-ArrwLFHilite",    { { 17, 58 },   { 21, 73 },   { 25, 88 } },   1 },
    { "Icon-ArrwLFPress",     { { 17, 58 },   { 21, 73 },   { 25, 88 } },   0 },
    { "Icon-ArrwRTHilite",    { { 281, 58 },  { 351, 73 },  { 446, 88 } },  1 },
    { "Icon-ArrwRTPress",     { { 281, 58 },  { 351, 73 },  { 446, 88 } },  0 },
    { "Skillicon-CombatA#",   { { 27, 91 },   { 34, 114 },  { 44, 137 } },  0 },
    { "Skillicon-CombatB#",   { { 27, 91 },   { 34, 114 },  { 44, 137 } },  3 },
    { "Skillicon-CombatC#",   { { 27, 91 },   { 34, 114 },  { 44, 137 } },  0 },
    { "Skillicon-StealthA#",  { { 73, 91 },   { 91, 114 },  { 117, 137 } }, 0 },
    { "Skillicon-StealthB#",  { { 73, 91 },   { 91, 114 },  { 117, 137 } }, 3 },
    { "Skillicon-StealthC#",  { { 73, 91 },   { 91, 114 },  { 117, 137 } }, 0 },
    { "Skillicon-CreationA#", { { 119, 91 },  { 149, 114 }, { 190, 137 } }, 0 },
    { "Skillicon-CreationB#", { { 119, 91 },  { 149, 114 }, { 190, 137 } }, 3 },
    { "Skillicon-CreationC#", { { 119, 91 },  { 149, 114 }, { 190, 137 } }, 0 },
    { "Skillicon-ExamA#",     { { 165, 91 },  { 206, 114 }, { 263, 137 } }, 0 },
    { "Skillicon-ExamB#",     { { 165, 91 },  { 206, 114 }, { 263, 137 } }, 3 },
    { "Skillicon-ExamC#",     { { 165, 91 },  { 206, 114 }, { 263, 137 } }, 0 },
    { "Skillicon-WeaponsA#",  { { 211, 91 },  { 264, 114 }, { 336, 137 } }, 0 },
    { "Skillicon-WeaponsB#",  { { 211, 91 },  { 264, 114 }, { 336, 137 } }, 3 },
    { "Skillicon-WeaponsC#",  { { 211, 91 },  { 264, 114 }, { 336, 137 } }, 0 },
    { "Skillicon-SpellsA#",   { { 257, 91 },  { 321, 114 }, { 409, 137 } }, 0 },
    { "Skillicon-SpellsB#",   { { 257, 91 },  { 321, 114 }, { 409, 137 } }, 3 },
    { "Skillicon-SpellsC#",   { { 257, 91 },  { 321, 114 }, { 409, 137 } }, 0 },
    { "Skilllist-Combat",     { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Skilllist-Stealth",    { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Skilllist-Creation",   { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Skilllist-Exam",       { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Skilllist-Weapons",    { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Skilllist-Spells",     { { 75, 142 },  { 94, 178 },  { 116, 212 } }, 0 },
    { "Icon-CloseHilite",     { { 277, 21 },  { 346, 26 },  { 444, 34 } },  1 },
    { "Icon-ClosePress",      { { 277, 21 },  { 346, 26 },  { 444, 34 } },  0 },
};
// 0x5912b0 (*new* name), by skill id; '?' becomes a..d for the four icon states, 0 = no icon. The literals
// sit at 0x591390-0x5917c7 in reverse order.
const char *skills_icon_names[SKILLS_NUM_ICONS] = {
    0, 0, 0, "SwordWeaponsPassive?.tga",
    "AxeWeaponsPassive?.tga", "BluntWeaponsPassive?.tga", "StaffWeaponsPassive?.tga", "BowWeaponsPassive?.tga",
    "HvyArmsPassive?.tga", "Skill-Parry?.tga", "Skill-CouterAttack?.tga", "Skill-Dodge?.tga",
    "Skill-CriticalHit?.tga", "Skill-DoubleAttack?.tga", "Skill-Push?.tga", "Skill-Kick?.tga",
    "Skill-Trip?.tga", "Skill-AimedAttack?.tga", "Skill-Backstab?.tga", "Skill-Assess?.tga",
    "Skill-Appraise?.tga", "Skill-PickLock?.tga", "Skill-Sneak?.tga", "Skill-Hide?.tga",
    0, 0, 0, "Skill-Summon?.tga",
    "Heal-Passive?.tga", "Dark-Passive?.tga", "Energy-Passive?.tga", "Holy-Passive?.tga",
    "Fire-Passive?.tga", "Ice-Passive?.tga", 0, "MagicResistPassive?.tga",
    "Skill-Golem?.tga", "Skill-Minotaur?.tga", "Skill-blur?.tga", "Skill-BlueImp?.tga",
    "Skill-Blackfire?.tga", "Skill-BloodElem?.tga", "Skill-PoisonElem?.tga", "Skill-MistElem?.tga",
    "Skill-Wraith?.tga", "Skill-JadeGolem?.tga", "Skill-SpiritLahara?.tga", "Skill-CelSamurai?.tga",
    "Skill-JadeDragon?.tga", "Skill-WaterDragon?.tga", "Skill-4WindsDragon?.tga", "Skill-ForestDragon?.tga",
    "Skill-BanishSummon?.tga", "Skill-AimedAttackEyes?.tga", "Skill-AimedAttackArms?.tga",
    "Skill-AimedAttackLegs?.tga",
};
// Then the literals ".tga" (0x5917c8; new_spell.cpp's copy is pooled with it), "got_ring_of_water",
// "act2_done", "got_ring_of_darkness" (also new_tutorial.cpp's), "lpalaceint_yago_found", "mach_killed",
// "Pick success!\n" and "Pick skill %f, container %f\n" (0x5917d0-0x59185b).

// ---- .bss ----------------------------------------------------------------------------------
int  skills_popup_count;                                      // 0x5f4a78
int  skills_popup_flags[SKILLS_MAX_POPUP];                    // 0x5f4a7c skill * 4 | 3 per entry (ui_choice_popup_show values)
int  skills_list_count;                                       // 0x5f4abc
ui_button_state skills_buttons[SKILLS_NUM_BUTTONS];           // 0x5f4ac0
int  skills_page;                                             // 0x5f4f58 group shown, -1 none
bool skills_popup_pending;                                    // 0x5f4f5c (sum.symbols name) the row was pressed on its icon
char *skills_popup_skills[SKILLS_MAX_POPUP];                  // 0x5f4f60 the localized entry names (sum.symbols: int[16]; D104)
int  skills_group_count;                                      // 0x5f4fa0
int  skills_groups[SKILLS_MAX_ROWS];                          // 0x5f4fa4 passive skills of the page
int  skills_mouse_mode;                                       // 0x5f4fc0 0 idle, 1 button, 2 row pressed, 3 icon dragged,
                                                              //          4 summon popup, 5 aimed attack popup
int  skills_pressed_button;                                   // 0x5f4fc4
int  skills_hover_row;                                        // 0x5f4fc8 rows count on from the active list into skills_groups
int  skills_list[SKILLS_MAX_ROWS];                            // 0x5f4fcc active skills of the page
int  skills_icon_bms[SKILLS_NUM_ICONS][4];                    // 0x5f4fe8
bool skills_popup_enabled[SKILLS_MAX_POPUP];                  // 0x5f5368

// 0x427240  Confidence: exact (VC6 byte match, match31; was likely; the icon buffer at function scope beside the button
// buffer, both 128 bytes: frame 0x10c; element pointers b / d, D9003). Names with '#' get the resolution between the base and the state
// letter: "Skillicon-CombatA#" -> "Skillicon-Combat640A.tga".
void skills_load_bitmaps()
{
    float scale = ui_get_scale();
    int i, j;
    char filename2[128];

    for (i = 0; i < SKILLS_NUM_BUTTONS; i++) {
        ui_button_state *b = &skills_buttons[i];
        const ui_button_def *d = &skills_button_defs[i];

        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;
        progress_update(0);

        if (strlen(d->name)) {
            char filename[128];

            strcpy(filename, d->name);
            if (strchr(filename, '#')) {
                int len = strlen(d->name);

                filename[len - 2] = 0;
                strcat(filename, ui_bitmap_suffix[ui_hires]);
                strncat(filename, &d->name[len - 2], 1);
                strcat(filename, ".tga");
            } else if (!strchr(filename, '.')) {
                strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            }
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            if (i == 0)
                b->flags |= 1;
        }
    }

    for (i = 0; i < SKILLS_NUM_ICONS; i++) {
        for (j = 0; j < 4; j++) {
            if (skills_icon_names[i]) {
                char *q;

                progress_update(0);
                strcpy(filename2, skills_icon_names[i]);
                q = strchr(filename2, '?');
                if (q) {
                    switch (j) {
                    case 0: *q = 'a'; break;
                    case 1: *q = 'b'; break;
                    case 2: *q = 'c'; break;
                    case 3: *q = 'd'; break;
                    }
                    skills_icon_bms[i][j] = bm_load(filename2, path_interface, -1);
                } else {
                    skills_icon_bms[i][j] = -1;
                }
            } else {
                skills_icon_bms[i][j] = -1;
            }
        }
    }
    skills_page = -1;
}

// 0x427520  Confidence: exact (tail jump).
void skills_init()
{
    skills_mouse_mode = 0;
    skills_pressed_button = -1;
    skills_hover_row = -1;
    skills_update_species_tab();
    skills_count_page();
    if (skills_count_in_group(skills_page) <= 0)
        skills_select_first_page();
    else
        skills_set_page(skills_page);
}

// 0x427570  Confidence: exact (the summon branch's spell test is a for loop with the found test after it; the
// two branches as if / else with the one `return count` at the end, D7700).
// A party member counts the skills of its progression reached at its level, but the table walked is
// party_leader's (see NOTES); a summon counts its passive skills. The spells group needs a castable spell.
int skills_count_in_group(int group)
{
    int count;
    int i;

    if (!cur_character)
        return 0;
    count = 0;

    if (cur_character->flags & 0x100) {
        if (group == 0 || (group > 3 && group <= 5)) {
            for (i = 0; i < SKILLS_NUM_LISTED; i++) {
                if (skill_table[i].group == group && (skill_table[i].flags & 1)
                    && cur_character->get_skill(i) > 0.0f) {
                    if (group == 5) {
                        int k;

                        for (k = 0; k < 8; k++) {
                            if (spells_count_in_group(k) > 0)
                                break;
                        }
                        if (k >= 8)
                            continue;
                    }
                    count++;
                }
            }
        }
    } else {
        for (i = 0; i < party_leader->num_progression; i++) {
            const skill_progression *p = &party_leader->progression[i];
            int skill;

            if (cur_character->level < p->level)
                break;
            skill = p->skill;
            if (skill_table[skill].group == group
                && !((game_flags & 2) && (skill == SKILL_PICK_LOCK || skill == SKILL_APPRAISE))
                && cur_character->get_skill(skill) > 0.0f)
                count++;
        }
    }
    return count;
}

// 0x4276a0  Confidence: exact (byte table 0x427874 over the species 0..0xce into the jump table 0x427830;
// the same mapping as spells_update_species_tab). The 16 flag clears are written out: cl 8966 keeps a
// loop as a loop, the binary has 16 and / store pairs (D7700).
void skills_update_species_tab()
{
    skills_buttons[1].flags &= ~1;
    skills_buttons[2].flags &= ~1;
    skills_buttons[3].flags &= ~1;
    skills_buttons[4].flags &= ~1;
    skills_buttons[5].flags &= ~1;
    skills_buttons[6].flags &= ~1;
    skills_buttons[7].flags &= ~1;
    skills_buttons[8].flags &= ~1;
    skills_buttons[9].flags &= ~1;
    skills_buttons[10].flags &= ~1;
    skills_buttons[11].flags &= ~1;
    skills_buttons[12].flags &= ~1;
    skills_buttons[13].flags &= ~1;
    skills_buttons[14].flags &= ~1;
    skills_buttons[15].flags &= ~1;
    skills_buttons[16].flags &= ~1;

    if (cur_character) {
        int tab;

        switch (cur_character->info->species) {
        case SPECIES_JOSEPH:   tab = 1; break;
        case SPECIES_JEKHAR:   tab = 2; break;
        case SPECIES_FLECE:    tab = 3; break;
        case SPECIES_ROSALIND: tab = 4; break;
        case 4:                tab = 5; break;     // Golem
        case 0x90:             tab = 6; break;     // Red Minotaur
        case 0xb0:             tab = 7; break;     // Blade of Urath
        case 0xa4:             tab = 8; break;     // Blue Imp
        case 0xc0:             tab = 9; break;     // Blackfire Elemental
        case 0xcb:             tab = 10; break;    // Blood Elemental
        case 0xca:             tab = 11; break;    // Poison Elemental
        case 0xcc:             tab = 12; break;    // Mist Elemental
        case 0xb9:             tab = 13; break;    // Wraith
        case 9:                tab = 14; break;    // Jade Golem
        case 0xcd:             tab = 15; break;    // Spirit of Laharah
        case 0xce:             tab = 16; break;    // Celestial Samurai
        default:
            return;
        }
        skills_buttons[tab].flags |= 1;
    }
}

// 0x427950  Confidence: exact (byte table 0x427b3c over i - 23; each case body written out, D7700).
void skills_count_page()
{
    int i;

// One group's tab: shown with its arrow when the group has a skill (the case bodies are the same code
// with constant indices, so the original wrote them out; a macro here).
#define SKILLS_GROUP_TAB(btn, group)                       \
    if (skills_count_in_group(group) > 0) {                \
        skills_buttons[btn].flags |= 1;                    \
        skills_buttons[(btn) + 1].flags &= ~4;             \
    } else {                                               \
        skills_buttons[btn].flags &= ~1;                   \
        skills_buttons[(btn) + 1].flags |= 4;              \
    }

    for (i = 0; i < SKILLS_NUM_BUTTONS; i++) {
        switch (i) {
        case 23: SKILLS_GROUP_TAB(23, 0); break;
        case 26: SKILLS_GROUP_TAB(26, 1); break;
        case 29: SKILLS_GROUP_TAB(29, 2); break;
        case 32: SKILLS_GROUP_TAB(32, 3); break;
        case 35: SKILLS_GROUP_TAB(35, 4); break;
        case 38: SKILLS_GROUP_TAB(38, 5); break;
        }
    }
#undef SKILLS_GROUP_TAB
}

// 0x427b50  Confidence: exact (byte table 0x427d80 over skill - 1 into the jump table 0x427d5c; the six
// tab clears written out, D7700; the protection and speed attributes are never listed). With a summon out the Creation page holds only
// Banish summon.
void skills_set_page(int page)
{
    int i;

    skills_page = page;
    skills_buttons[41].flags &= ~1;
    skills_buttons[42].flags &= ~1;
    skills_buttons[43].flags &= ~1;
    skills_buttons[44].flags &= ~1;
    skills_buttons[45].flags &= ~1;
    skills_buttons[46].flags &= ~1;
    switch (page) {
    case 0: skills_buttons[41].flags |= 1; break;
    case 1: skills_buttons[42].flags |= 1; break;
    case 2: skills_buttons[43].flags |= 1; break;
    case 3: skills_buttons[44].flags |= 1; break;
    case 4: skills_buttons[45].flags |= 1; break;
    case 5: skills_buttons[46].flags |= 1; break;
    }

    skills_list_count = 0;
    skills_group_count = 0;
    if (page == -1)
        return;

    if (page == 2 && cur_summon && cur_character->get_skill(SKILL_SUMMON) > 0.0f) {
        if (cur_summon->type == 7)
            skills_list[skills_list_count++] = SKILL_BANISH_SUMMON;
        return;
    }

    for (i = 0; i < SKILLS_NUM_LISTED; i++) {
        if (skill_table[i].group != skills_page)
            continue;
        switch (i) {
        case SKILL_PROTECTION:
        case SKILL_SPEED:
            continue;
        case SKILL_APPRAISE:
        case SKILL_PICK_LOCK:
            if (game_flags & 2)
                continue;
            break;
        case SKILL_HEAL:
            if (spells_count_in_group(0) <= 0)
                continue;
            break;
        case SKILL_DARK:
            if (spells_count_in_group(1) <= 0)
                continue;
            break;
        case SKILL_ENERGY:
            if (spells_count_in_group(2) <= 0)
                continue;
            break;
        case SKILL_HOLY:
            if (spells_count_in_group(3) <= 0)
                continue;
            break;
        case SKILL_FIRE:
            if (spells_count_in_group(4) <= 0)
                continue;
            break;
        case SKILL_ICE:
            if (spells_count_in_group(5) <= 0)
                continue;
            break;
        }
        if ((cur_character->flags & 0x100) && !(skill_table[i].flags & 1))
            continue;
        if (cur_character->get_skill(i) > 0.0f) {
            if (skill_table[i].flags & 1) {
                if (skills_group_count < SKILLS_MAX_ROWS)
                    skills_groups[skills_group_count++] = i;
            } else {
                if (skills_list_count < SKILLS_MAX_ROWS)
                    skills_list[skills_list_count++] = i;
            }
        }
    }
}

// 0x427db0  Confidence: exact.
void skills_select_first_page()
{
    if (cur_character) {
        int i;

        for (i = 0; i < 6; i++) {
            if (skills_count_in_group(i) > 0) {
                skills_set_page(i);
                return;
            }
        }
    }
    skills_set_page(-1);
}

// 0x427df0  Confidence: exact (tail jump). Closes a popup left open.
void skills_leave()
{
    if (skills_mouse_mode >= 4 && skills_mouse_mode <= 5)
        ClearFrameFlag5bd77c();
}

// 0x427e10  Confidence: exact (jump table 0x427eb4 over the mouse modes 0..5). The Creation page is rebuilt
// every frame (the summon may come or go).
int skills_frame(int key)
{
    int i;

    skills_update_member_buttons();
    for (i = 0; i < SKILLS_NUM_BUTTONS; i++)
        skills_buttons[i].flags &= ~2;
    if (skills_page == 2)
        skills_set_page(2);

    if (player_input[0] & 0x10)
        return key;

    switch (skills_mouse_mode) {
    case 0:
        key = skills_handle_click(key);
        break;
    case 1:
        skills_update_buttons();
        break;
    case 2:
        skills_handle_mouse();
        break;
    case 3:
        skills_release_drag();
        break;
    case 4:
        key = skills_summon_popup_frame(key);
        break;
    case 5:
        key = skills_aimed_attack_popup_frame(key);
        break;
    }
    return key;
}

// 0x427ed0  Confidence: exact.
void skills_update_member_buttons()
{
    int count = party_count_active();

    if (summon_player)
        count--;
    if (count <= 1) {
        skills_buttons[19].flags |= 4;
        skills_buttons[21].flags |= 4;
    } else {
        skills_buttons[19].flags &= ~4;
        skills_buttons[21].flags &= ~4;
    }
}

// 0x427f30  Confidence: exact (VC6 byte match, match31; was likely) (as spells_handle_click; key switch: byte table 0x4281a8 over key - 1 into the
// jump table 0x428194). The passive rows only show tooltips.
int skills_handle_click(int key)
{
    int i;

    skills_pressed_button = -1;
    skills_hover_row = -1;

    for (i = 0; i < SKILLS_NUM_BUTTONS; i++) {
        const ui_button_def *d = &skills_button_defs[i];
        ui_button_state *b = &skills_buttons[i];

        if ((d->flags & 3) && point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h)) {
            if ((d->flags & 2) && interface_input_ready())
                skills_category_tooltip(i);
            if (!(b->flags & 4)) {
                skills_pressed_button = i;
                b->flags |= 2;
                if (mouse_was_pressed(0)) {
                    skills_mouse_mode = 1;
                    return key;
                }
                break;
            }
        }
    }

    for (i = 0; i < skills_list_count; i++) {
        bool on_icon = skills_row_hit_icon(i);
        bool on_text = skills_row_hit_text(i);

        if (on_icon || on_text)
            skills_hover_row = i;
        if (mouse_was_pressed(0)) {
            if (on_icon || on_text) {
                skills_popup_pending = on_icon;
                skills_mouse_mode = 2;
                return key;
            }
        } else if (interface_input_ready()) {
            if (on_icon)
                skills_show_tooltip(i, true);
            else if (on_text)
                skills_show_tooltip(i, false);
        }
        if (skills_hover_row >= 0)
            break;
    }

    for (i = 0; i < skills_group_count; i++) {
        int row = skills_list_count + i;
        bool on_icon = skills_row_hit_icon(row);
        bool on_text = skills_row_hit_text(row);

        if (on_icon || on_text)
            skills_hover_row = row;
        if (interface_input_ready()) {
            if (on_icon)
                skills_show_tooltip(row, true);
            else if (on_text)
                skills_show_tooltip(row, false);
        }
        if (skills_hover_row >= 0)
            break;
    }

    switch (key) {
    case KEY_ESC:
    case KEY_S:
        interface_set_screen(0, 0);
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    case KEY_C:
        interface_set_screen(5, 0);
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        return 0;
    case KEY_I: {
        bool flag = false;

        interface_set_screen(1, &flag);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;
    }
    case KEY_R:
        interface_set_screen(2, 0);
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        return 0;
    }
    return key;
}

// 0x4281e0  Confidence: exact (no bounds test, unlike spells_row_hit_icon).
bool skills_row_hit_icon(int row)
{
    int x = skills_list_pos[resolution_index][0];
    int y = skills_row_height[resolution_index] * row + skills_list_pos[resolution_index][1];
    int w = hud_hot_button_size[resolution_index][0];
    int h = hud_hot_button_size[resolution_index][1];

    return point_in_rect(game_mouse_x, game_mouse_y, x, y, w, h);
}

// 0x428230  Confidence: exact. The whole name column up to the level's right edge.
bool skills_row_hit_text(int row)
{
    int x = skills_list_pos[resolution_index][0] + skills_text_offset[resolution_index][0];
    int y = skills_row_height[resolution_index] * row + skills_list_pos[resolution_index][1]
            + skills_text_offset[resolution_index][1];
    int w = skills_text_width[resolution_index] - skills_text_offset[resolution_index][0];

    return point_in_rect(game_mouse_x, game_mouse_y, x, y, w, gr_get_font_height(-1));
}

// 0x4282a0  Confidence: exact (byte table 0x428380 over button - 24 into the jump table 0x428364).
void skills_category_tooltip(int button)
{
    char text[256];

    switch (button) {
    case 24: strcpy(text, XSTR("Combat skills", 720)); break;
    case 27: strcpy(text, XSTR("Stealth skills", 721)); break;
    case 30: strcpy(text, XSTR("Creation skills", 722)); break;
    case 33: strcpy(text, XSTR("Examination skills", 723)); break;
    case 36: strcpy(text, XSTR("Weapon skills", 724)); break;
    case 39: strcpy(text, XSTR("Spell skills", 725)); break;
    default:
        return;
    }
    ui_button_state *b = &skills_buttons[button];
    ui_tooltip_set(b->x, b->y, b->w, b->h, text);
}

// 0x428390  Confidence: exact. An active row shows its +0x30 text when it has one, else the +0x2c
// description; a passive row its description.
void skills_show_tooltip(int row, bool icon)
{
    int x, y, w, h;

    if (icon) {
        x = skills_list_pos[resolution_index][0];
        y = skills_row_height[resolution_index] * row + skills_list_pos[resolution_index][1];
        w = hud_hot_button_size[resolution_index][0];
        h = hud_hot_button_size[resolution_index][1];
    } else {
        x = skills_list_pos[resolution_index][0] + skills_text_offset[resolution_index][0];
        y = skills_row_height[resolution_index] * row + skills_text_offset[resolution_index][1]
            + skills_list_pos[resolution_index][1];
        w = skills_text_width[resolution_index] - skills_text_offset[resolution_index][0];
        h = gr_get_font_height(-1);
    }

    if (row < skills_list_count) {
        skill_info *s = &skill_table[skills_list[row]];

        if (strlen(localize_string_or_invalid(s->use_description)))
            ui_tooltip_set(x, y, w, h, localize_string_or_invalid(s->use_description));
        else
            ui_tooltip_set(x, y, w, h, localize_string_or_invalid(s->description));
    } else {
        ui_tooltip_set(x, y, w, h,
                       localize_string_or_invalid(skill_table[skills_groups[row - skills_list_count]].description));
    }
}

// 0x4284d0  Confidence: exact.
void skills_update_buttons()
{
    int button = skills_pressed_button + 1;

    if (skills_pressed_button >= 0 && skills_pressed_button < SKILLS_NUM_BUTTONS) {
        ui_button_state *b = &skills_buttons[button];

        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;
        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            skills_button_action(button);
    }
    skills_mouse_mode = 0;
}

// 0x428550  Confidence: exact (byte table 0x428680 over button - 18 into the jump table 0x428654).
void skills_button_action(int button)
{
    switch (button) {
    case 18:
        skills_goto_spells();
        game_play_sound(0x27, 1.0f, 0, 0, 0);
        break;
    case 20:
        ui_party_prev();
        break;
    case 22:
        ui_party_next();
        break;
    case 25:
        skills_page0();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 28:
        skills_page1();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 31:
        skills_page2();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 34:
        skills_page3();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 37:
        skills_page4();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 40:
        skills_page5();
        game_play_sound(0x25, 1.0f, 0, 0, 0);
        break;
    case 48:
        interface_close_screen();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x4286a0  Confidence: exact.
void skills_goto_spells()
{
    interface_set_screen(5, 0);
}

// 0x4286b0  Confidence: exact.
void skills_page0() { skills_set_page(0); }

// 0x4286c0  Confidence: exact.
void skills_page1() { skills_set_page(1); }

// 0x4286d0  Confidence: exact.
void skills_page2() { skills_set_page(2); }

// 0x4286e0  Confidence: exact.
void skills_page3() { skills_set_page(3); }

// 0x4286f0  Confidence: exact.
void skills_page4() { skills_set_page(4); }

// 0x428700  Confidence: exact.
void skills_page5() { skills_set_page(5); }

// 0x428710  Confidence: exact (skills_list[skills_hover_row] is read before the hit test, also when the row
// is -1; the jump table 0x428938 over skill - 0x11 through the byte table 0x428950). Released on the row: aimed
// attack and summon open their popups, identify opens the inventory in identify mode, pick lock waits for a
// world pick (hud_target_mode 2), the rest is used at once. Dragged off from the icon: the icon goes to the hot
// buttons (not for aimed attack and summon, which need the popup).
void skills_handle_mouse()
{
    int skill = skills_list[skills_hover_row];

    if (skills_row_hit(skills_hover_row)) {
        if (mouse_is_down(0))
            return;
        if (skill_is_usable(skill, 0) && skill_is_available(skill, 0)) {
            switch (skill) {
            case SKILL_AIMED_ATTACK:
                skills_build_aimed_attack_menu();
                ui_choice_popup_show(0, game_mouse_x, game_mouse_y, skills_popup_count, skills_popup_skills, 4,
                                     (int)skills_popup_flags, (int)skills_popup_enabled);
                skills_mouse_mode = 5;
                break;
            case SKILL_APPRAISE: {
                int identify;   // a dword slot; the byte at +0 is set (the binary's [esp + 4], D7700)

                *(bool *)&identify = true;
                interface_set_screen(1, &identify);
                break;
            }
            case SKILL_PICK_LOCK:
                interface_set_screen(0, 0);
                hud_target_mode = 2;
                break;
            case SKILL_SUMMON:
                skills_build_summon_menu();
                ui_choice_popup_show(0, game_mouse_x, game_mouse_y, skills_popup_count, skills_popup_skills, 4,
                                     (int)skills_popup_flags, (int)skills_popup_enabled);
                skills_mouse_mode = 4;
                break;
            case SKILL_BANISH_SUMMON:
                character_use_skill(SKILL_BANISH_SUMMON, cur_summon, 0);
                interface_set_screen(0, 0);
                break;
            default:
                character_use_skill(skill, 0, 0);
                skills_mouse_mode = 0;
                interface_set_screen(0, 0);
                break;
            }
            game_play_sound(0x23, 1.0f, 0, 0, 0);
        } else {
            skills_mouse_mode = 0;
            game_play_sound(0x24, 1.0f, 0, 0, 0);
        }
    } else if (skills_popup_pending) {
        if (skill != SKILL_AIMED_ATTACK && skill != SKILL_SUMMON) {
            hud_drag_icon_begin(skills_list_pos[resolution_index][0],
                                skills_row_height[resolution_index] * skills_hover_row
                                    + skills_list_pos[resolution_index][1],
                                hud_hot_button_size[resolution_index][0], hud_hot_button_size[resolution_index][1],
                                1, skill);
            skills_mouse_mode = 3;
        } else if (!mouse_is_down(0)) {
            skills_mouse_mode = 0;
        }
    } else if (!mouse_is_down(0)) {
        skills_mouse_mode = 0;
    }
}

// 0x428980  Confidence: exact (VC6 byte match) (the ring gates are skill_is_available's, inlined by hand or a macro; the
// count test sits after character_can_use_summon). One entry per summon skill 0x24..0x33 the character may
// use, greyed when not available now.
void skills_build_summon_menu()
{
    int skill;

    skills_popup_count = 0;
    if (!(cur_character->get_skill(SKILL_SUMMON) > 0.0f))
        return;
    if (!All_summons && (game_flags & 1)) {
        if (!savegame_state.get_flag(0x18, "got_ring_of_darkness"))
            return;
        if (savegame_state.get_flag(0x11, "act2_done") && !savegame_state.get_flag(0x10, "got_ring_of_water"))
            return;
    }
    for (skill = SKILL_SUMMON_GOLEM; skill < SKILL_BANISH_SUMMON; skill++) {
        if (character_can_use_summon(skill, cur_character)) {
            if (skills_popup_count >= SKILLS_MAX_POPUP)
                return;
            skills_popup_skills[skills_popup_count] = localize_string_or_invalid(skill_table[skill].display_name);
            skills_popup_flags[skills_popup_count] = skill << 2 | 3;
            skills_popup_enabled[skills_popup_count] = skill_is_available(skill, 0);
            skills_popup_count++;
        }
    }
}

// 0x428a80  Confidence: exact. Eyes, arms, legs (skills 0x35..0x37).
void skills_build_aimed_attack_menu()
{
    int skill;

    skills_popup_count = 0;
    if (!(cur_character->get_skill(SKILL_AIMED_ATTACK) > 0.0f))
        return;
    for (skill = SKILL_AIMED_ATTACK_EYES; skill < SKILL_AIMED_ATTACK_LEGS + 1; skill++) {
        if (skills_popup_count >= SKILLS_MAX_POPUP)
            return;
        skills_popup_skills[skills_popup_count] = localize_string_or_invalid(skill_table[skill].display_name);
        skills_popup_flags[skills_popup_count] = skill << 2 | 3;
        skills_popup_enabled[skills_popup_count] = skill_is_available(skill, 0);
        skills_popup_count++;
    }
}

// 0x428b10  Confidence: exact.
bool skills_row_hit(int row)
{
    if (skills_row_hit_icon(row))
        return true;
    return skills_row_hit_text(row) ? true : false;
}

// 0x428b40  Confidence: exact.
void skills_release_drag()
{
    if (!mouse_is_down(0))
        skills_mouse_mode = 0;
}

// 0x428b60  Confidence: exact. -1 still open, -2 cancelled (the key is eaten).
int skills_summon_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        skills_mouse_mode = 0;
        return 0;
    }
    if (choice < 0) {
        skills_mouse_mode = 0;
        return key;
    }
    {
        int skill = skill_find_by_display_name(skills_popup_skills[choice]);

        if (skill >= 0)
            character_use_skill(skill, 0, 0);
        interface_set_screen(0, 0);
    }
    return key;
}

// 0x428bd0  Confidence: exact.
int skills_aimed_attack_popup_frame(int key)
{
    int choice = ui_choice_popup_frame(key);

    if (choice == -1)
        return key;
    if (choice == -2) {
        skills_mouse_mode = 0;
        return 0;
    }
    if (choice < 0) {
        skills_mouse_mode = 0;
        return key;
    }
    character_use_skill(choice + SKILL_AIMED_ATTACK_EYES, 0, 0);
    interface_set_screen(0, 0);
    return key;
}

// 0x428c30  Confidence: exact.
void skills_draw()
{
    int h, w, y, x;
    int i;

    gr_get_clip(&x, &y, &w, &h);
    gr::set_clip(0, 0, gr_screen_width(), gr_screen_height());
    gr_set_color(255, 255, 255, 255);
    for (i = 0; i < SKILLS_NUM_BUTTONS; i++) {
        ui_button_state *b = &skills_buttons[i];
        if (b->flags & 3)
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
    }
    skills_draw_list();
    if (skills_mouse_mode >= 4 && skills_mouse_mode <= 5)
        ui_choice_popup_draw();
    gr::set_clip(x, y, w, h);
}

// 0x428cf0  Confidence: exact (VC6 byte match, match31; was likely) (as spells_draw_list; the level text buffer is 8 bytes in the frame). Level =
// skill * 10, right-aligned at skills_text_width; the passive rows use icon state 3 and the disabled colour.
void skills_draw_list()
{
    int y = skills_list_pos[resolution_index][1];
    int x = skills_list_pos[resolution_index][0];
    int i, w, h;
    char text[8];

    for (i = 0; i < skills_list_count; i++) {
        int state;

        if (skill_is_usable(skills_list[i], 0) && skill_is_available(skills_list[i], 0)) {
            if (i == skills_hover_row) {
                if (skills_mouse_mode == 2)
                    state = skills_row_hit(i) ? 2 : 0;
                else
                    state = 1;
            } else {
                state = 0;
            }
        } else {
            state = 3;
        }

        if (skills_icon_bms[skills_list[i]][state] >= 0) {
            gr_set_color(255, 255, 255, 255);
            ui_bitmap_scaled(skills_icon_bms[skills_list[i]][state], x, y, gr_bitmap_wrap_mode);
        }
        switch (state) {
        case 1:
        case 2:
            gr_set_color_ptr(&ui_color_highlight);
            break;
        case 3:
            gr_set_color_ptr(&ui_color_disabled);
            break;
        case 0:
            gr_set_color_ptr(&ui_color_text);
            break;
        default:
            gr_set_color_ptr(&ui_color_text);
            break;
        }
        gr_string(skills_text_offset[resolution_index][0] + x, skills_text_offset[resolution_index][1] + y,
                  localize_string_or_invalid(skill_table[skills_list[i]].name), -1, gr_string_mode);
        sprintf(text, "%d", (int)(cur_character->get_skill(skills_list[i]) * 10.0f + 0.5f));
        gr_get_string_size(&w, &h, text, -1, -1);
        gr_string(skills_text_width[resolution_index] - w + x, skills_text_offset[resolution_index][1] + y, text,
                  -1, gr_string_mode);
        y += skills_row_height[resolution_index];
    }

    for (i = 0; i < skills_group_count; i++) {
        if (skills_icon_bms[skills_groups[i]][3] >= 0) {
            gr_set_color(255, 255, 255, 255);
            ui_bitmap_scaled(skills_icon_bms[skills_groups[i]][3], x, y, gr_bitmap_wrap_mode);
        }
        gr_set_color_ptr(&ui_color_disabled);
        gr_string(skills_text_offset[resolution_index][0] + x, skills_text_offset[resolution_index][1] + y,
                  localize_string_or_invalid(skill_table[skills_groups[i]].name), -1, gr_string_mode);
        sprintf(text, "%d", (int)(cur_character->get_skill(skills_groups[i]) * 10.0f + 0.5f));
        gr_get_string_size(&w, &h, text, -1, -1);
        gr_string(skills_text_width[resolution_index] - w + x, skills_text_offset[resolution_index][1] + y, text,
                  -1, gr_string_mode);
        y += skills_row_height[resolution_index];
    }
}

// 0x429050  Confidence: exact (tail jump). Entry 6 of the per-screen refresh table 0x58bdc8.
void skills_refresh_for_character()
{
    skills_update_species_tab();
    skills_count_page();
    skills_select_first_page();
}

// 0x429060  Confidence: exact.
int skills_get_panel_width()
{
    return skills_buttons[0].w;
}

// 0x429070  Confidence: exact.
bool skills_mouse_in_panel(int x, int y)
{
    return point_in_rect(x, y, skills_buttons[0].x, skills_buttons[0].y, skills_buttons[0].w, skills_buttons[0].h);
}

// 0x4290a0  Confidence: exact (VC6 byte match) (town filter: byte table 0x4291b4 over skill - 0xd into the jump table
// 0x4291ac). Whether the skill can be triggered now: not passive, no second summon, Banish only for a summon
// that is out (and in multiplayer only for one's own), in towns only the non-combat skills (and no summon),
// not while casting, and attack skills (flag 2) only while attacking.
bool skill_is_usable(int skill, Character *c)
{
    bool summon;

    if (!c)
        c = cur_character;
    summon = skill_is_summon(skill);
    if (skill_table[skill].flags & 1)
        return false;
    if ((skill == SKILL_SUMMON || summon) && cur_summon)
        return false;
    if (skill == SKILL_BANISH_SUMMON) {
        if (!summon_player)
            return false;
        if (cur_summon) {
            if (cur_summon->ai.is_mode(AI_STATE_SUMMONING, -1))
                return false;
            if (!(cur_summon->obj_flags & 1))
                return false;
        }
        if ((game_flags & 2) && summon_player && summon_player->mp_owner != mp_local_player)
            return false;
    }
    if (level_is_town) {
        switch (skill) {
        case SKILL_DOUBLE_ATTACK:
        case SKILL_PUSH:
        case SKILL_KICK:
        case SKILL_TRIP:
        case SKILL_AIMED_ATTACK:
        case SKILL_BACKSTAB:
        case SKILL_SNEAK:
        case SKILL_HIDE:
        case SKILL_SUMMON:
            return false;
        }
        if (summon)
            return false;
    }
    if (c->ai.is_casting())
        return false;
    if ((skill_table[skill].flags & 2) && !c->ai.is_attacking())
        return false;
    return true;
}

// 0x4291d0  Confidence: exact (VC6 byte match, match31; was likely) (the comparison results return through mov eax, 1 / xor eax, eax). Whether the
// character has the skill ready: the summoning ring gates of the story (single player), no identify / pick
// lock in multiplayer, enough AP, the cooldown (the summon skills share skill 0x1b's, the aimed attacks 0x11's),
// a dagger (weapon class 8) for backstab when not already attacking from behind, sneak / hide only after
// "lpalaceint_yago_found" (or Max_skills / multiplayer), and a non-zero skill.
bool skill_is_available(int skill, Character *c)
{
    bool summon;
    Player *p;

    if (!c)
        c = cur_character;
    summon = skill_is_summon(skill);
    if (!All_summons && (skill == SKILL_SUMMON || summon) && (game_flags & 1)) {
        if (!savegame_state.get_flag(0x18, "got_ring_of_darkness"))
            return false;
        if (savegame_state.get_flag(0x11, "act2_done") && !savegame_state.get_flag(0x10, "got_ring_of_water"))
            return false;
        if (savegame_state.get_flag(0x25, "mach_killed"))
            return false;
    }
    if ((game_flags & 2) && (skill == SKILL_APPRAISE || skill == SKILL_PICK_LOCK))
        return false;
    if (c->ap < skill_table[skill].ap_cost)
        return false;
    p = player_from_handle(c->handle);
    if (!p)
        return false;

    if (summon) {
        if (!p->skill_cooldown[SKILL_SUMMON].elapsed())
            return false;
        return c->get_skill(SKILL_SUMMON) > 0.0f && character_can_use_summon(skill, c);
    }
    if (skill_has_flag8(skill)) {
        if (!p->skill_cooldown[SKILL_AIMED_ATTACK].elapsed())
            return false;
        return c->get_skill(SKILL_AIMED_ATTACK) > 0.0f;
    }
    if (!p->skill_cooldown[skill].elapsed())
        return false;
    if (skill != SKILL_BACKSTAB) {
        if (skill > SKILL_PICK_LOCK && skill <= SKILL_HIDE)
            return Max_skills || savegame_state.get_flag(0x18, "lpalaceint_yago_found") || (game_flags & 2);
        return Max_skills || c->get_skill(skill) > 0.0f;
    }
    if (c->ai.is_mode(AI_STATE_COMBAT, 0x11))
        return false;
    if (!c->equipment[5])
        return false;
    return c->equipment[5]->type->weapon_class == 8 ? true : false;
}

// 0x429410  Confidence: exact (VC6 byte match, D23502: the pick-lock test as `skill + 0.05f >= (double)door->lock`;
// match31: the hide / sneak test as if / else-if with a goto past the payment;
// the appraise root needs no variable: 8966 itself stores the sqrt in the dead target slot (D15203); the
// pick-lock test's fld / fcompp comes from the double cast, D23502); the AP payment's two jump tables 0x4297d0 / 0x4297d8 and the action's
// 0x4297fc / 0x42981c are written as switches; the identify root goes through a dead float store; the
// two_stage out-values live in the dead argument slots). Pays the AP (not when hide / sneak are toggled off,
// not for backstab / assess / the summons, which pay later), then: summon skills and backstab / assess wait for a
// world pick of their spell (ui_pending_use_skill = skill_table +0x18), the combat moves are queued on the
// combat sub-object, hide / sneak become AI goals, identify tests the skill against sqrt(value) / 710, pick lock
// works on the door (world_interact_object) or container (world_hover_object) under the cursor, Banish casts
// spell 0x36 on the summon. Aimed attack itself and the creation skills pay and return false.
bool character_use_skill(int skill, void *target, Character *user)
{
    if (!user)
        user = cur_character;
    if (!skill_is_usable(skill, user) || !skill_is_available(skill, user))
        return false;

    if (skill == SKILL_HIDE) {
        if (user->ai.is_mode(AI_STATE_HIDING, -1))
            goto paid;
    } else if (skill == SKILL_SNEAK && (user->flags & 0x400)) {
        goto paid;
    }
    {
        if (user->ap < skill_table[skill].ap_cost)
            return false;
        switch (skill) {
        case SKILL_BACKSTAB:
        case SKILL_ASSESS:
        case SKILL_SUMMON_GOLEM: case SKILL_SUMMON_RED_MINOTAUR: case SKILL_SUMMON_BLADE_OF_URATH:
        case SKILL_SUMMON_BLUE_IMP: case SKILL_SUMMON_BLACKFIRE_ELEMENTAL: case SKILL_SUMMON_BLOOD_ELEMENTAL:
        case SKILL_SUMMON_POISON_ELEMENTAL: case SKILL_SUMMON_MIST_ELEMENTAL: case SKILL_SUMMON_WRAITH:
        case SKILL_SUMMON_JADE_GOLEM: case SKILL_SUMMON_LAHARAH: case SKILL_SUMMON_SAMURAI:
        case SKILL_SUMMON_JADE_DRAGON: case SKILL_SUMMON_WATER_DRAGON: case SKILL_SUMMON_WIND_DRAGON:
        case SKILL_SUMMON_FOREST_DRAGON:
            break;
        default:
            user->add_ap((float)-skill_table[skill].ap_cost);
            break;
        }
    }
paid:
    ui_clear_pick_overrides();
    if (skill_is_summon(skill)) {
        hud_target_mode = 1;
        ui_pending_use_skill = skill_table[skill].spell;
        return true;
    }

    ui_pending_use_skill = -1;
    switch (skill) {
    case SKILL_BANISH_SUMMON:
        if (!target)
            return false;
        character_use_skill_on(0x36, (Character *)target, 999999, user);
        break;
    case SKILL_DOUBLE_ATTACK:
    case SKILL_PUSH:
    case SKILL_KICK:
    case SKILL_TRIP:
    case SKILL_AIMED_ATTACK_EYES:
    case SKILL_AIMED_ATTACK_ARMS:
    case SKILL_AIMED_ATTACK_LEGS:
        user->combat.set_38(skill);
        break;
    case SKILL_HIDE:
        user->ai.add_hide();
        break;
    case SKILL_SNEAK:
        user->ai.add_sneak();
        break;
    case SKILL_BACKSTAB:
    case SKILL_ASSESS:
        hud_target_mode = 0;
        ui_pending_use_skill = skill_table[skill].spell;
        break;
    case SKILL_APPRAISE: {
        ItemType *type;

        if (!target)
            return false;
        player_start_skill_cooldown(user, SKILL_APPRAISE);
        type = ((item_info *)target)->type;
        if (user->get_skill(SKILL_APPRAISE) > (float)sqrt((double)type->value) * 0.0014084507f)
            type->flags &= ~1;
        break;
    }
    case SKILL_PICK_LOCK:
        hud_target_mode = -1;
        if (world_interact_object) {
            lobj *l = (lobj *)world_interact_object;
            door_info *door = door_from_lobj(l);

            if (!door)
                break;
            if (door->lock == 0.0f)
                return true;
            if (user->get_skill(SKILL_PICK_LOCK) + 0.05f >= (double)door->lock) {   // D23502
                bool open, locked;

                savegame_state.two_stage_get(&open, &locked, l);
                savegame_state.two_stage_set(open, false, l);
                level_script_use(l->name);
                game_play_sound(6, 1.0f, 0, 0, 0);
                feedback_play(user->info->species, 8, &user->pos);
            } else {
                game_play_sound(5, 1.0f, 0, 0, 0);
            }
        } else if (world_hover_object) {
            container *box = (container *)world_hover_object;

            printf("Pick skill %f, container %f\n", user->get_skill(SKILL_PICK_LOCK), box->lock);
            if (user->get_skill(SKILL_PICK_LOCK) + 0.05f >= box->lock) {
                printf("Pick success!\n");
                box->lock = 0.0f;
                game_play_sound(6, 1.0f, 0, 0, 0);
                feedback_play(user->info->species, 8, &user->pos);
                hud_show_message(XSTR("Pick successful.", 726));
                object_open_pickup(box);
            } else {
                game_play_sound(5, 1.0f, 0, 0, 0);
                feedback_play(user->info->species, 9, &user->pos);
                hud_show_message(XSTR("Couldn't pick lock.", 727));
            }
        } else {
            return true;
        }
        break;
    default:
        return false;
    }

    if (ui_pending_use_skill != -1) {
        world_pick_set_skill_filter(ui_pending_use_skill);
        if (spells[ui_pending_use_skill].target_radius > 0.0f) {
            ui_pick_screen_override_on = true;
            ui_pick_override_x = spells[ui_pending_use_skill].target_radius;
            ui_pick_override_y = spells[ui_pending_use_skill].target_radius;
        }
    }
    return true;
}
