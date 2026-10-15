// summoner/interface/new_skill.h - the skills panel (new_skill.cpp, 0x427240-0x42984f; screen 6).
// Not original source.
#ifndef SUMMONER_INTERFACE_NEW_SKILL_H
#define SUMMONER_INTERFACE_NEW_SKILL_H

#include "vsdk/vsdk.h"

class Character;

void skills_load_bitmaps();                          // 0x427240
void skills_init();                                  // 0x427520 screen 6
int  skills_count_in_group(int group);               // 0x427570
void skills_update_species_tab();                    // 0x4276a0
void skills_count_page();                            // 0x427950 (no argument)
void skills_set_page(int page);                      // 0x427b50 (-1 = none)
void skills_select_first_page();                     // 0x427db0
void skills_leave();                                 // 0x427df0
int  skills_frame(int key);                          // 0x427e10
void skills_update_member_buttons();                 // 0x427ed0
int  skills_handle_click(int key);                   // 0x427f30
bool skills_row_hit_icon(int row);                   // 0x4281e0
bool skills_row_hit_text(int row);                   // 0x428230
void skills_category_tooltip(int button);            // 0x4282a0
void skills_show_tooltip(int row, bool icon);        // 0x428390
void skills_update_buttons();                        // 0x4284d0
void skills_button_action(int button);               // 0x428550
void skills_goto_spells();                           // 0x4286a0
void skills_page0();                                 // 0x4286b0
void skills_page1();                                 // 0x4286c0
void skills_page2();                                 // 0x4286d0
void skills_page3();                                 // 0x4286e0
void skills_page4();                                 // 0x4286f0
void skills_page5();                                 // 0x428700
void skills_handle_mouse();                          // 0x428710
void skills_build_summon_menu();                     // 0x428980
void skills_build_aimed_attack_menu();               // 0x428a80
bool skills_row_hit(int row);                        // 0x428b10
void skills_release_drag();                          // 0x428b40
int  skills_summon_popup_frame(int key);             // 0x428b60
int  skills_aimed_attack_popup_frame(int key);       // 0x428bd0
void skills_draw();                                  // 0x428c30
void skills_draw_list();                             // 0x428cf0
void skills_refresh_for_character();                 // 0x429050
int  skills_get_panel_width();                       // 0x429060
bool skills_mouse_in_panel(int x, int y);            // 0x429070

// The character argument is a thiscall `this` inside (0 = cur_character: 0x4291dc, 0x42941b).
bool skill_is_usable(int skill, Character *c);       // 0x4290a0
bool skill_is_available(int skill, Character *c);    // 0x4291d0
// (skill, target, user): user is arg 3 (0x429412 -> ESI, 0 = cur_character); the target is an item for
// Identify (inventory_identify_item) and the summon for Banish (hud_use_hot_button). DECISIONS.md D48.
bool character_use_skill(int skill, void *target, Character *user);   // 0x429410

extern int skills_icon_bms[56][4];                   // 0x5f4fe8 4 bitmaps per skill icon

#endif
