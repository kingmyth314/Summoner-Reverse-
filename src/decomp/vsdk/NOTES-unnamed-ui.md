# decomp62: the gadget base, the list box, the build stamp and the UI manager (0x52bfe0-0x530b86)

Written from Sum.exe v1.40 only (`tools/asm.mjs fn`, `build/sum.asm`, raw bytes of `Summoner/Sum.exe` for the
jump tables 0x52d3ec / 0x530310, vtables 0x581e08 / 0x581e34, the build stamp .data and float constants). No
Summoner 2 code was read (Summoner 2 has no UI code, so it gives no file names either). Every file passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>` (warnings
only: `delete` of the non-virtual entry destructor, as in the original), and all 158 src/decomp .cpp files
still compile against this include copy (taken from the repo at 66fb364, after decomp61's ui_button/static).

## Units and evidence (D4200)

| File | Range | Functions | Evidence |
|---|---|---|---|
| `vsdk/unnamed_gadget_0x52bfe0.cpp` | 0x52bfe0-0x52ca53 | 48 (38 exact / 10 likely) | window.cpp's COMDATs 0x52bfa0 / 0x52bfc0 close window.obj, so 0x52bfe0 opens an object. No .data, .bss or EH. The end is not proven: it could be the head of the list box object. |
| `vsdk/unnamed_listbox_0x52ca60.cpp` | 0x52ca60-0x52f444 | 69 (42 / 27) + 3 COMDATs | .bss 0x2cbf584-0x2cbf623 (line tables) after window.cpp's "" literals (..0x2cbf583); .data 0x5a95e4 ("text too long for pc version") after window.cpp's (..0x5a95e3); EH funclets 0x57d510-0x57d556, one int3-padded block; ends with its COMDATs 0x52f400 / 0x52f420 / 0x52f440. |
| `vsdk/unnamed_build_stamp_0x52f450.cpp` | 0x52f450-0x52f568 | 3 (2 / 1) | .data 0x5a9604-0x5a966f of its own (three stamp fields, four literals in two copies: no pooling); no .bss, no initialisers. Could be the head of the UI manager (no .data there to compare). |
| `vsdk/unnamed_ui_manager_0x52f570.cpp` | 0x52f570-0x530b86 | 10 (7 / 3) + 15 initialiser functions + 14 COMDATs | Initialiser run 0x52f570-0x52f700 (table 0x588974-0x5889a0; the next entry 0x531a80 is the os unit's); .bss 0x2cbf628-0x2cbfcd9; ends with the colour set constructors and two inline getters (COMDATs), then the os unit (0x530b90). Built without inline expansion (calls Color() 0x505530 and the getters as COMDATs). |

No file is named (no path string, no strings.tbl block, no Summoner 2 counterpart), so modules.txt lists none.

## Functions

unnamed_gadget_0x52bfe0.cpp (ui_gadget, vtable 0x581e08; ui_state_bitmap):

| Address | Function | Confidence |
|---|---|---|
| 0x52bfe0 | ui_gadget::unlink | likely (store order) |
| 0x52c040 | init | likely (list append store order) |
| 0x52c0e0 | identity | exact |
| 0x52c0f0 / 0x52c180 / 0x52c210 / 0x52c2a0 | find_nearest_up / down / left / right | likely x4 (tail recursion over siblings) |
| 0x52c330 | send_event (slot 1) | exact |
| 0x52c390 | process (slot 5) | exact |
| 0x52c3d0 | dtor (slot 0, ui_gadget__destroy) | exact |
| 0x52c3f0 / 0x52c410 | ui_gadget() / ~ui_gadget() | exact x2 |
| 0x52c420 / 0x52c460 | get_screen_x / get_screen_y | exact x2 |
| 0x52c4a0 / 0x52c4b0 / 0x52c4f0 / 0x52c530 | is_owned / is_hidden / is_shown / is_disabled (slot 4) | exact x4 |
| 0x52c570 | set_parent | likely |
| 0x52c5c0-0x52c5f0 | get_window / get_parent / get_first_child / get_next_sibling | exact x4 |
| 0x52c600 / 0x52c630 | set_focus / clear_focus | exact x2 |
| 0x52c660 / 0x52c670 / 0x52c680 | is_selected / is_mouse_over / get_type | exact x3 |
| 0x52c690 / 0x52c6b0 / 0x52c6c0 | set_hotkey / clear_hotkeys / has_hotkey | exact x3 |
| 0x52c6f0 | get_mask_pixel (slot 6) | exact |
| 0x52c730 | hit_test | likely |
| 0x52c800 / 0x52c820 / 0x52c830 | set_hotspot / clear_hotspot / get_hotspot | exact x3 |
| 0x52c840 / 0x52c880 | draw (slot 7) / is_selectable (slot 8) | exact x2 |
| 0x52c890 / 0x52c8c0 / 0x52c8f0 | set_disabled / set_enabled / clear_window_refs | exact x3 |
| 0x52c930 | ui_state_bitmap() | likely (store order) |
| 0x52c950 / 0x52c960 / 0x52c970 / 0x52c980 | ~ui_state_bitmap / link / clear / set | exact x4 |
| 0x52c9a0 | ui_state_bitmap::draw | likely |
| 0x52ca50 | ui_state_bitmap::get_bitmap (no sum.symbols entry, name ours) | exact |

unnamed_listbox_0x52ca60.cpp (ui_listbox 0x581e34, ui_listbox_entry 0x581e30, ui_listbox_text_entry 0x581e2c):

| Address | Function | Confidence |
|---|---|---|
| 0x52ca60 | ui_listbox_entry::wrap_text | likely |
| 0x52cb70 / 0x52cba0 | line_top / line_bottom | exact / likely |
| 0x52cbd0 / 0x52cc20 | line_at_y_with_spacing / line_at_y | likely / exact |
| 0x52cc60 | ui_listbox::layout_entry | likely |
| 0x52cca0 / 0x52cce0 / 0x52cd20 | is_entry_visible / _fully_visible / _bottom_visible | exact x3 |
| 0x52cd50 / 0x52cd70 | update_vscrollbar / update_hscrollbar | exact x2 |
| 0x52cd90 | recalc_content_size | likely |
| 0x52ce20 / 0x52ce90 / 0x52ced0 | entry_at / entry_above / entry_at_y | exact x3 |
| 0x52cf10 / 0x52cf70 | relayout_dirty / sync_scrollbars | exact x2 |
| 0x52d080 | process (slot 5) | likely |
| 0x52d420 | dtor (slot 0, ui_listbox__destroy) | likely |
| 0x52d480 / 0x52d4a0 | ui_listbox_text_entry() / set_text | exact / likely |
| 0x52d4f0 | ui_listbox_entry::line_at_y_or_none | exact |
| 0x52d540 / 0x52d560 | ui_listbox_entry() / ~ui_listbox_entry() | likely / exact |
| 0x52d570-0x52d5b0 | set_indent / set_wrap_indent / set_line_spacing / set_font / set_text | exact x5 |
| 0x52d5c0 / 0x52d650 / 0x52d7e0 | ui_listbox() / create / set_tooltip | likely x3 |
| 0x52d8a0-0x52d9b0 | get_selected / select_by_data / select_by_text / select_entry / clear_selection / find_by_text | exact x6 |
| 0x52da40 / 0x52dc30 | add_entry / clear | likely x2 |
| 0x52dcc0 / 0x52dd00 | remove_by_text / remove_entry | exact / likely |
| 0x52dde0 / 0x52deb0 / 0x52df90 / 0x52dfb0 / 0x52dfd0 | select_prev / select_next / scroll_left / scroll_right / select_page_up | exact x5 |
| 0x52e050 | select_page_down | likely |
| 0x52e110 / 0x52e290 / 0x52e380 / 0x52e590 | scroll_line_up / _line_down / _page_up / _page_down | likely x4 |
| 0x52e870 / 0x52e8c0 | scroll_page_right / _page_left | exact x2 |
| 0x52e8f0 / 0x52e920 | get_hscroll_fraction / get_vscroll_fraction | likely x2 |
| 0x52ea00 / 0x52ea50 | set_hscroll / set_vscroll | exact x2 |
| 0x52eb20 / 0x52ebe0 | snap_scroll_top / snap_scroll_bottom | likely x2 |
| 0x52ed00 / 0x52ed70 | set_hscroll_fraction / set_vscroll_fraction | likely x2 |
| 0x52f020-0x52f060 | get_view_width / _height / get_content_width / _height / is_empty | exact x5 |
| 0x52f080 | draw (slot 7) | likely |
| 0x52f3d0 | set_sorted | exact |
| 0x52f400 / 0x52f420 / 0x52f440 | scalar deleting destructors (entry, text entry), text entry implicit destructor | compiler-generated |

unnamed_build_stamp_0x52f450.cpp: 0x52f450 build_stamp_check (likely), 0x52f530 build_stamp_get_name_String
(exact), 0x52f540 build_stamp_get_name (exact).

unnamed_ui_manager_0x52f570.cpp:

| Address | Function | Confidence |
|---|---|---|
| 0x52f570-0x52f700 | the twelve dynamic initialisers (thunk + body each; the sentinel's 0x52f590 / 0x52f5a0 / 0x52f5b0 / 0x52f5c0 with its atexit destructor) | compiler-generated (from the global definitions) |
| 0x52f710 | atexit_nop_52f710 | exact |
| 0x52f720 | ui_play_sound | likely |
| 0x52f760 | ui_init | exact |
| 0x52ff30 | ui_draw | likely |
| 0x530330 | ui_process | likely |
| 0x5308e0 / 0x530910 / 0x530920 / 0x530930 / 0x530970 | ui_destroy_all_windows / ui_push_group / ui_pop_group / ui_find_gadget_by_name / ui_get_top_modal_window | exact x5 |
| 0x5309c0-0x530b50 | the twelve colour set constructors (ui_colors*__ctor) | compiler-generated (implicit) |
| 0x530b70 / 0x530b80 | ui_container::get_mask_buffer / ui_window::get_mask_data | COMDATs of the header inlines (D4206) |

Totals: 130 functions written (89 exact / 41 likely / 0 unsure), 32 compiler-generated (3 + 15 + 14).

Globals defined: ui_listbox_line_starts (0x2cbf584, char *[20]) and ui_listbox_line_lengths (0x2cbf5d4) in
the list box unit; build_stamp_checksum / _expiry / _name (0x5a9604-0x5a9653) in the stamp unit; in the UI
manager the twelve constructed objects in initialiser order (ui_tooltip_timer, ui_window_sentinel, the ten
colour sets) and the 24 plain .bss variables in address order (ui_left_pressed 0x2cbf628 .. 0x2cbfcd9; four
names ours: ui_last_mouse_x / _y 0x2cbfcb0 / 0x2cbfcb4, ui_inited 0x2cbfcd0, ui_left / ui_right_double_click_ok
0x2cbfcd8 / 0x2cbfcd9). Not referenced anywhere in the unit (left out): 0x2cbf62b-0x2cbf62f, 0x2cbf6a4,
0x2cbf704, 0x2cbf714, 0x2cbfc1c, 0x2cbfc6c.

## Open questions

- The cuts gadget / list box and build stamp / UI manager (D4200).
- Whether the find_nearest_* functions were tail-recursive or loops (same code either way, probably).
- ui_listbox::process's `key = 0; return key;` per arrow key (the code stores 0 into the argument slot).
- ui_draw's tooltip selection: the code jumps straight past the final test for the "no tooltip" cases,
  which a `t = NULL` start does not obviously give; a goto version is the alternative.
- The order of the six stores that restore the click flags in ui_process (lp, rp, rr, restored, lr, ld, rd in
  the code).
- build_stamp_name's size (69 to 72, D4204).
- ui_button +0x424 (decomp61's drag_data, "this, from create"): ui_process treats it as the drop target of the
  drag (NULL = dropped on nothing, another button = move drag_amount to it). With the create value (the
  button itself) ui_process does nothing; some game code must set it.
- 0x2cbf714 (after the three tooltip colours) and the other unreferenced gaps.

## Doc disagreements

- docs/sum-exe/source-map.md row `0x52da40`-`0x530b80` "UI listbox gadget and window mask helpers": the range
  holds the gadget base (from 0x52bfe0), the list box (from 0x52ca60, not 0x52da40), the build stamp and the UI
  manager; "window mask helpers" are two COMDAT getters at its very end.
- source-map "Engine ranges" row 0x52f570-0x530b80: the .bss starts at 0x2cbf628 (ui_left_pressed), not
  0x2cbf634 (0x2cbf630 is ui_checkbox_colors).
- docs/sum-exe/vsdk-ui.md "Still to do: the widget classes ... (0x52b9c0-0x530c00)": 0x52bfe0-0x530b86 is now
  written; the list box parser's "+Add both scrollbars" sets only the vertical flag 0x2 (window.cpp).
- sum.symbols: ui_tooltip_colors is Color[3], not Color[4] (D4202); ui_listbox_line_starts holds char
  pointers (not int); ui_listbox_entry__wrap_text is (width, y, callback), not (width, callback, ...) (D4208);
  ui_listbox__destroy is slot 0 dtor() (it deletes the entries, it is not the C++ destructor);
  ui_listbox__set_hscroll_fraction / _vscroll_fraction take an unread second argument; build_stamp_get_name's
  size argument is unread.

## Game bugs

| Id | Where | What | When it matters |
|---|---|---|---|
| F6201 | ui_listbox_entry setters / ui_listbox::relayout_dirty 0x52cf10 | dirty is set by set_font / set_indent / ... and never cleared: such entries are re-wrapped (and the sizes recomputed) in every process and draw | performance only |
| F6202 | ui_listbox_entry::wrap_text 0x52ca60 | no bound on the 20-entry line tables: line 21+ overwrites ui_listbox_line_lengths, line 41+ the UI manager's .bss (0x2cbf624 on, ui_left_pressed ...) | an entry wrapped to more than 20 lines (preallocated text or a narrow box) |
| F6203 | ui_listbox::recalc_content_size 0x52cd90 | an empty list gets content_height = -entry_spacing (-2) | harmless (callers test for empty) |
| F6204 | ui_state_bitmap::draw 0x52c9a0 | state 1 without a bitmap returns before gr::reset_clip, leaving the clip rectangle set | a state bitmap set to a failed load with a clip offset (u >= 0) |
| F6205 | ui_listbox::process 0x52d080 | the click y passed to entry_at includes v_margin: a click hits the entry v_margin (2) pixels lower | clicks within 2 px of an entry edge |
| F6206 | ui_listbox::set_tooltip 0x52d7e0 | the replaced tooltip is unlinked, not deleted (as F623 for ui_button) | each re-set tooltip |
| F6207 | ui_listbox::remove_entry 0x52dd00, add_entry 0x52da40 (bottom-aligned) | vscroll is changed without the scroll bar; the next sync_scrollbars sees a different marker position and moves the view back to it (unverified: depends on ui_slider::set_range) | list boxes with a vertical scroll bar |
| F6208 | ui_listbox::select_page_down 0x52e050 | when the selection is the last entry and taller than the view, `selected->next` is the ring head, which becomes the selection (get_selected returns the head: text unset) | Page Down on a tall last entry |
| F6209 | get_vscroll_fraction 0x52e920, set_vscroll_fraction 0x52ed70 | the line stops use the font height without line_spacing | no caller in the game |
| F6210 | ui_listbox::snap_scroll_bottom 0x52ebe0 | at the end of the content the search stops on the ring head and reads its num_lines / y, which no constructor sets (zero in a static list box, garbage in a `new` one) | bottom-aligned, line-snapped list boxes scrolled to the end |
| F6211 | ui_process 0x530330 | the hover search over the windows does not stop at the first hit, so the bottom-most window under the mouse provides the tooltip element | overlapping windows with tooltips |
| F6212 | ui_listbox::process 0x52d080 | a hidden list box plays UI sound 7 when its area is clicked or its hotkey pressed | hidden list boxes with a hotkey |

## What is left

Nothing in 0x52bfe0-0x530b86. Declared here but not written (other units): ui_tooltip::draw 0x55ef60,
ui_element::get_position 0x542e40, ui_slider::get_pos 0x55eb10 / set_range 0x55eeb0. Neighbours not written:
ui_element (0x542cd0-0x543010), the bevel / slider / tooltip code (0x55e040-0x55ef60), ui_checkbox and
ui_radio_button (0x553ba0-), ui_inputbox (0x5521a0-0x5534c0).
