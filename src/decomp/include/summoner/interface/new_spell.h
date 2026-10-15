// summoner/interface/new_spell.h - the spells panel (new_spell.cpp, 0x429850-0x42b1b7; screen 5) and the
// casting entry points at its end. Not original source.
#ifndef SUMMONER_INTERFACE_NEW_SPELL_H
#define SUMMONER_INTERFACE_NEW_SPELL_H

#include "vsdk/vsdk.h"

class Character;
class vector3;

void spells_load_bitmaps();                          // 0x429850
void spells_init();                                  // 0x429b30 screen 5
void spells_update_species_tab();                    // 0x429b80
void spells_count_page();                            // 0x429e30 (no argument)
void spells_set_page(int page);                      // 0x42a030 (-1 = none)
void spells_select_first_page();                     // 0x42a150
int  spells_frame(int key);                          // 0x42a190
void spells_update_member_buttons();                 // 0x42a210
int  spells_handle_click(int key);                   // 0x42a270
bool spells_row_hit_icon(int row);                   // 0x42a4b0
bool spells_row_hit_text(int row);                   // 0x42a510
void spells_category_tooltip(int button);            // 0x42a5b0
void spells_show_tooltip(int row, bool icon);        // 0x42a6a0
void spells_update_buttons();                        // 0x42a780
void spells_button_action(int button);               // 0x42a800
void spells_goto_skills();                           // 0x42a950
void ui_party_prev();                                // 0x42a960 party_cycle(0), also the skills buttons
void ui_party_next();                                // 0x42a970 party_cycle(1)
void spells_page0();                                 // 0x42a980
void spells_page1();                                 // 0x42a990
void spells_page2();                                 // 0x42a9a0
void spells_page3();                                 // 0x42a9b0
void spells_page4();                                 // 0x42a9c0
void spells_page5();                                 // 0x42a9d0
void spells_handle_mouse();                          // 0x42a9e0
bool spells_row_hit(int row);                        // 0x42ab00
void spells_release_drag();                          // 0x42ab40
void spells_draw();                                  // 0x42ab60
void spells_draw_list();                             // 0x42ac10
void spells_refresh_for_character();                 // 0x42ad90
int  spells_get_panel_width();                       // 0x42ada0
bool spells_mouse_in_panel(int x, int y);            // 0x42adb0

bool character_can_cast_now(Character *c);           // 0x42ade0 (0 = cur_character)
// (spell, character (0 = cur_character), ignore_cost: skip the AP and cooldown tests; D105)
bool spell_is_usable(int spell, Character *c, bool ignore_cost);   // 0x42ae20
int  spells_count_in_group(int group);               // 0x42aee0
void character_cast_spell(int spell, Character *c);  // 0x42af30 (0 = cur_character)
// The pending spell (ui_pending_use_skill) on a picked character / point (world_mouse_frame), and the
// backstab / assess / banish spells of character_use_skill.
void character_use_skill_on(int spell, Character *target, int priority, Character *user);   // 0x42b050
void character_cast_at_point(int spell, vector3 *pos, int layer, int priority, Character *c); // 0x42b170

extern int spells_icon_bms[38][4];                   // 0x5f5820 4 bitmaps per spell icon

#endif
