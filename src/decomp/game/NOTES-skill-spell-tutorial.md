# decomp21: new_skill.cpp, new_spell.cpp, new_tutorial.cpp

Written from Sum.exe v1.40's disassembly (tools/asm.mjs fn) only. Target VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
Confidence as in the other pilots: exact (control flow, calls, constants and globals match), likely (one
detail of statement order or expression form guessed), unsure (shape guessed). Header changes:
CHANGED-HEADERS.txt; decisions D100-D108 at the end of include/DECISIONS.md.

| File | Unit range | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/new_skill.cpp` | 0x427240-0x42984f; .rdata 0x580710-0x58075b, .data 0x590670-0x59185f, .bss 0x5f4a78-0x5f5377 | 39 | 28 / 11 / 0 |
| `game/new_spell.cpp` | 0x429850-0x42b1b7; .rdata 0x58075c-0x58079b, .data 0x591860-0x5927df, .bss 0x5f5378-0x5f5aa7 | 38 | 31 / 7 / 0 |
| `game/new_tutorial.cpp` | 0x42e630-0x42f1df; .data 0x592a20-0x592c6b, .bss 0x5f9610-0x5f966f | 15 + 7 initialiser pairs | 11 / 4 / 0 (initialisers: 1 exact, 6 likely) |

Every function of the three module tags in sum.symbols is written; nothing is left.

## new_skill.cpp (screen 6)

| Address | Function | Conf. | Note |
|---|---|---|---|
| 0x427240 | skills_load_bitmaps | likely | two filename buffers in two scopes (frame 0x10c), sizes guessed; '#' names get the resolution before the state letter |
| 0x427520 | skills_init | exact | tail jump to skills_select_first_page |
| 0x427570 | skills_count_in_group | likely | summon branch (Character flag 0x100): passive skills of groups 0, 4, 5 |
| 0x4276a0 | skills_update_species_tab | exact | byte table 0x427874 over species 0..0xce |
| 0x427950 | skills_count_page | likely | no argument (D108); switch on the button index in the loop |
| 0x427b50 | skills_set_page | likely | byte table 0x427d80; Creation with a summon out = Banish only |
| 0x427db0 | skills_select_first_page | exact | |
| 0x427df0 | skills_leave | exact | ClearFrameFlag5bd77c for modes 4/5 |
| 0x427e10 | skills_frame | exact | rebuilds page 2 every frame |
| 0x427ed0 | skills_update_member_buttons | exact | buttons 19 / 21 (sum.symbols' 0x5f4c88 / 0x5f4cb8) |
| 0x427f30 | skills_handle_click | likely | keys: Esc/S close, R status, I inventory, C spells |
| 0x4281e0 | skills_row_hit_icon | exact | no bounds test |
| 0x428230 | skills_row_hit_text | exact | |
| 0x4282a0 | skills_category_tooltip | exact | L720-725 |
| 0x428390 | skills_show_tooltip | exact | skill_table +0x30 first, +0x2c otherwise (D102) |
| 0x4284d0 | skills_update_buttons | exact | |
| 0x428550 | skills_button_action | exact | |
| 0x4286a0 | skills_goto_spells | exact | |
| 0x4286b0-0x428700 | skills_page0..5 | exact | 6 functions |
| 0x428710 | skills_handle_mouse | likely | |
| 0x428980 | skills_build_summon_menu | likely | ring gates repeated from skill_is_available |
| 0x428a80 | skills_build_aimed_attack_menu | exact | |
| 0x428b10 | skills_row_hit | exact | |
| 0x428b40 | skills_release_drag | exact | |
| 0x428b60 | skills_summon_popup_frame | exact | |
| 0x428bd0 | skills_aimed_attack_popup_frame | exact | |
| 0x428c30 | skills_draw | exact | |
| 0x428cf0 | skills_draw_list | likely | level text buffer 8 bytes |
| 0x429050 | skills_refresh_for_character | exact | |
| 0x429060 | skills_get_panel_width | exact | |
| 0x429070 | skills_mouse_in_panel | exact | |
| 0x4290a0 | skill_is_usable | likely | town filter table 0x4291b4 |
| 0x4291d0 | skill_is_available | likely | comparison returns (mov eax, 1 / xor eax, eax) |
| 0x429410 | character_use_skill | likely | four jump tables written as two switches |

## new_spell.cpp (screen 5)

| Address | Function | Conf. | Note |
|---|---|---|---|
| 0x429850 | spells_load_bitmaps | likely | one buffer (frame 0x8c); progress_update before the null test |
| 0x429b30 | spells_init | exact | |
| 0x429b80 | spells_update_species_tab | exact | byte table 0x429d54 |
| 0x429e30 | spells_count_page | likely | no argument (D108) |
| 0x42a030 | spells_set_page | exact | lists usable spells ignoring AP |
| 0x42a150 | spells_select_first_page | exact | |
| 0x42a190 | spells_frame | exact | |
| 0x42a210 | spells_update_member_buttons | exact | buttons 19 / 21 |
| 0x42a270 | spells_handle_click | likely | keys: Esc/C close, R status, I inventory, S skills |
| 0x42a4b0 | spells_row_hit_icon | exact | |
| 0x42a510 | spells_row_hit_text | exact | |
| 0x42a5b0 | spells_category_tooltip | exact | L728-733 |
| 0x42a6a0 | spells_show_tooltip | exact | spell_descriptions 0x22e92a0 |
| 0x42a780 | spells_update_buttons | exact | |
| 0x42a800 | spells_button_action | exact | |
| 0x42a950 | spells_goto_skills | exact | |
| 0x42a960 / 0x42a970 | ui_party_prev / ui_party_next | exact | |
| 0x42a980-0x42a9d0 | spells_page0..5 | exact | 6 functions |
| 0x42a9e0 | spells_handle_mouse | exact | |
| 0x42ab00 | spells_row_hit | exact | |
| 0x42ab40 | spells_release_drag | exact | |
| 0x42ab60 | spells_draw | exact | |
| 0x42ac10 | spells_draw_list | likely | |
| 0x42ad90 | spells_refresh_for_character | exact | |
| 0x42ada0 | spells_get_panel_width | exact | |
| 0x42adb0 | spells_mouse_in_panel | exact | |
| 0x42ade0 | character_can_cast_now | exact | |
| 0x42ae20 | spell_is_usable | likely | (spell, Character*, ignore_cost) D105 |
| 0x42aee0 | spells_count_in_group | exact | |
| 0x42af30 | character_cast_spell | likely | |
| 0x42b050 | character_use_skill_on | likely | spells 0x45 backstab, 0x46 assess |
| 0x42b170 | character_cast_at_point | exact | |

## new_tutorial.cpp

| Address | Function | Conf. | Note |
|---|---|---|---|
| 0x42e630/40, 0x42e670/80, 0x42e6b0/c0, 0x42e6e0/f0, 0x42e710/20 | init_strptrs_5f9628 .. 5f965c (+ thunks, entries 83-87) | likely | page tables as reference structs (new_help.cpp pattern) |
| 0x42e750/60 | init_tutorial_entries (+ thunk, entry 88) | likely | VC6 keeps the constant prefix of entry 0 in .data |
| 0x42e8e0/f0 | init_ccmd_Ignore_tutorial (+ thunk, entry 89) | exact | |
| 0x42e910 | cmd_Ignore_tutorial | exact | |
| 0x42e9d0 | tutorial_cond_basic_controls | exact | |
| 0x42ea40 | tutorial_cond_combat_part1 | exact | |
| 0x42ead0 | tutorial_cond_combat_part2 | likely | opens invis-door01, disables masad-firedoor2.vfx |
| 0x42ec00 | tutorial_cond_sewer_flece | likely | |
| 0x42ec90 | tutorial_cond_ring | exact | |
| 0x42ecf0 | tutorial_reset | exact | |
| 0x42ed00 | tutorial_show | exact | |
| 0x42ed90 | tutorial_close | exact | |
| 0x42edb0 | tutorial_layout | exact | real signature (x, y, w, h, offsets, lengths) |
| 0x42eea0 | tutorial_ok_hit_test | exact | the whole box |
| 0x42ef00 | tutorial_frame | likely | Space test through a bool |
| 0x42f020 | tutorial_draw | likely | |
| 0x42f1c0 | tutorial_is_active | exact | |
| 0x42f1d0 | tutorial_mouse_in_panel | exact | |

Entry table 0x592aa8 (5 x 0x24): {cond, pages, num_pages, level, script, screen, x, y, w}; levels 3 (masad)
x3, 9 (sewer), 0x18 (palace interior), all script slot 0 and screen 0 (the HUD).

## Data not referenced

- 0x592aa4 (new_tutorial .data, a 0 dword between the position table and the entry table), 0x5f5aa4
  (new_spell .bss), 0x5f9624 `tutorial_drawn` (written by tutorial_draw, never read).

## Open questions

1. `level_entered_by_load` (0x25443a8): the tutorial runs only while it is set, which contradicts its
   sum.symbols name / comment (D101). Check game_change_level.
2. skills_count_in_group walks `party_leader`'s progression with `cur_character`'s level and skills. Same
   result only while the selected character is the leader; check whether party_leader always is (else the
   tabs of a non-leader member are computed from the wrong progression).
3. skill_table +0x30 (shown before +0x2c for active skills) and +0x0c / +0x10 / +0x24..0x2b are not
   checked against init_skill_table.
4. The species ids of the summon tabs (4, 9, 0x90, 0xa4, 0xb0, 0xb9, 0xc0, 0xca-0xce) come from the
   bitmap names; there is no SPECIES_* list for them yet.
5. ui_choice_popup_show: the third optional argument is bool* (D103); the prototype still says int.
6. The order of the .bss objects in new_tutorial.cpp (console command first) does not follow the
   static-initialiser order (string tables first); the source order used here is the initialiser order.
7. Buffer sizes in skills_load_bitmaps (two buffers, 0x84 / 0x7c bytes of frame) and spells_load_bitmaps
   (one) are guesses.

## Game bugs / oddities seen

- character_use_skill pays the AP and then returns false for skills it has no action for: aimed attack
  itself (0x11) and, if they are ever usable, forge / mine / enchant (0x18-0x1a). The panel
  routes 0x11 to its popup first, so only a hot button can hit it.
- skills_handle_mouse reads `skills_list[skills_hover_row]` before checking the row, i.e. skills_list[-1]
  (which is skills_hover_row itself) when no row is hovered. Harmless.
- After identify / pick lock / banish from the panel, skills_mouse_mode stays 2; skills_init resets it
  when the screen opens again.
- skill_is_available blocks every summon in single player once "mach_killed" is set (story gate, not
  verified in play).
- spells_update_member_buttons does not test multiplayer (status_update_member_buttons does).
- Debug printf "Pick skill %f, container %f\n" / "Pick success!\n" left in character_use_skill (stdout only).
