# Notes: new_assess.cpp and new_pickup.cpp

Source written from the Sum.exe v1.40 disassembly (`tools/asm.mjs fn`, data bytes read from
`Summoner/Sum.exe`, English text of the localize slots from `extracted/tables/strings.tbl`). The
Summoner 2 decomp and Dash Faction were not used. Check: `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings -I . -I symbols <file>`: both pass; the only warning is `-Wformat-security` on
`sprintf(msg, XSTR(... 700))`, which is what the binary does (L700 has no conversion). The struct
offsets added to `engine_stubs.h` were checked with `static_assert(offsetof(...))` in a scratch file.

`engine_stubs.h` is the pilot header unchanged plus a block at the end ("added for new_assess.cpp /
new_pickup.cpp"): strcspn, _stricmp, memset, min/max macros, `gr::set_clip`, gr_get_clip,
bm_get_dimensions, progress_update, mouse_is_down / mouse_was_pressed, point_in_rect, ui_get_scale,
interface_close_screen, hud_show_message, sound_play_2d, `screen_button_def` (0x40-byte .rdata button
definition) and `screen_button` (0x18-byte run-time record), partial `Character` / `char_info` /
`ItemType` / `item_info` / `item_obj` / `container` classes with the thiscall methods these units call,
and the prototypes of both units. Other screens (options, play menu, ...) use the same two button
structures, so the other agents' headers may have their own names for them; merge when combining.

## new_assess.cpp (0x4011b0-0x401e7c), screen 20

| Address | Function | Confidence |
|---|---|---|
| 0x4011b0 | init_realtimetimer_5b28bc_thunk (compiler-generated, `RealtimeTimer assess_repeat_timer;`) | exact |
| 0x4011c0 | init_realtimetimer_5b28bc (compiler-generated) | exact |
| 0x4011d0 | assess_load_bitmaps | exact |
| 0x401380 | assess_init | exact |
| 0x4013f0 | assess_format_info | likely |
| 0x4016d0 | assess_update_scrollbar | likely |
| 0x4017e0 | assess_leave | exact |
| 0x4017f0 | assess_frame | likely |
| 0x401840 | assess_handle_click | likely |
| 0x401a20 | assess_page_up | exact |
| 0x401a50 | assess_page_down | exact |
| 0x401a80 | assess_button_action | likely |
| 0x401ad0 | assess_scroll_up | exact |
| 0x401af0 | assess_scroll_down | exact |
| 0x401b20 | assess_update_buttons | likely |
| 0x401bf0 | assess_drag_scrollbar | exact |
| 0x401c80 | assess_draw | exact |
| 0x401ce0 | assess_draw_text | likely |
| 0x401da0 | assess_draw_scrollbar | exact |
| 0x401e50 | assess_mouse_in_panel | exact |

20 functions: 12 exact, 8 likely, 0 unsure.

Data (*new* = name I gave): .rdata 0x57e4c0 `assess_button_defs[7]` (*new*), 0x57e680
`assess_text_origin[3]` (*new*), 0x57e698 `assess_text_width[3]` (*new*), 0x57e6a4 `assess_page_size[3]`
= 9 (*new*), then the `__real@` 0.5f / 100.0f. .data 0x588b48 `assess_scrollbar_names[2]` (*new*),
0x588b50 `assess_scrollbar_track[3]` {x, y} (sum.symbols names only the y member, 0x588b54),
0x588b68 `assess_scrollbar_track_height[3]`, then ": ", "\n", "#01234567890", "onif". .bss
0x5b27f0-0x5b29d7 as named in sum.symbols.

## new_pickup.cpp (0x4205b0-0x42158c), screen 4

| Address | Function | Confidence |
|---|---|---|
| 0x4205b0 | init_realtimetimer_5c3b3c_thunk (compiler-generated, `RealtimeTimer pickup_button_timer;`) | exact |
| 0x4205c0 | init_realtimetimer_5c3b3c (compiler-generated) | exact |
| 0x4205d0 | pickup_load_bitmaps | exact |
| 0x4207d0 | pickup_init | likely |
| 0x420850 | pickup_update_scrollbar | likely |
| 0x420960 | pickup_leave | exact |
| 0x4209b0 | pickup_frame | likely |
| 0x420a30 | pickup_handle_click | likely |
| 0x420d20 | pickup_page_up | exact |
| 0x420d50 | pickup_page_down | exact |
| 0x420da0 | pickup_take_selected | likely |
| 0x420f10 | pickup_close | exact |
| 0x420f60 | pickup_button_action | likely |
| 0x421000 | pickup_scroll_up | exact |
| 0x421020 | pickup_scroll_down | exact |
| 0x421050 | pickup_update_buttons | likely |
| 0x421110 | pickup_list_handle_click | exact |
| 0x4211c0 | pickup_drag_scrollbar | likely |
| 0x421260 | pickup_draw | exact |
| 0x4212c0 | pickup_draw_scrollbar | exact |
| 0x421370 | pickup_draw_item_list | likely |
| 0x421560 | pickup_mouse_in_panel | exact |

22 functions: 12 exact, 10 likely, 0 unsure.

Data: .rdata 0x580220 `pickup_button_defs[9]` (*new*), 0x580460 `pickup_list_origin[3]` (*new*;
0x580478 belongs to the next unit). .data 0x58f390-0x58f4b7: row width, 0x58f39c
`pickup_list_count_x[3]` (*new*), list button x, 0x58f3b4 `pickup_checkbox_names[2][2]` (*new*,
-> `pickup_item_pressed_bm`), 0x58f3c4 `pickup_crossbox_names[2][2]` (*new*, -> `pickup_item_bm`),
0x58f3d4 `pickup_scrollbar_names[2]` (*new*), 0x58f3dc `pickup_scrollbar_track[3]`, track height,
page size (7), strings. .bss 0x5c3a30-0x5c3b70.

Behaviour notes: buttons are hilite/press pairs; the hilite index i is hit-tested and the action is
id = i + 1, and `*_update_buttons` hit-tests and draws record i + 1 (the "Press" bitmap). Def flags:
1 fires on release, 4 fires on press and repeats (500 ms first, then 100 ms). Pickup actions: 2 take,
4 up, 6 down, 8 close; ENTER = take (sound 0x23), ESC = close (0x24); a row toggle plays 0x27.
pickup_take_selected counts `item_obj +0x9c` (stack count) of non-gold items for "%d items picked up"
(L701) / "1 item picked up" (L700). Assess lists every damage type except magic (index 3, already in
L828) whose resistance level is not 2 ("standard").

## Open questions

1. **pickup .bss hole at 0x5c3a44** (between `pickup_mouse_mode` and `pickup_buttons`): never
   referenced. Written as an unused `int pickup_unused_5c3a44`; it could also be padding if VC6 aligns
   the button array, or a variable whose only users were removed. Same question for 0x5c3b71-0x5c3b77
   before new_playmenu's .bss (probably the next object's alignment).
2. **.bss order** follows definition order, as in the pilot (assumption). The assess thumb rectangle
   sits at w, h, x, y (0x5b27f0..fc), an odd order for four separate ints; it may have been a struct
   or the definition order really was w, h, x, y.
3. **assess_update_scrollbar inner test** is `line_count <= 0` (not `scroll_pos <= 0` as in pickup),
   so the "else" placement is dead code in practice. Reproduced as read.
4. **Thumb height:** assess scales it (page / count * track); pickup always uses the full track
   height. Reproduced; probably a pickup bug or an intentional fixed thumb.
5. **pickup_list_origin[2] = {210, 293}** and the pickup background's 1024 position {172, 194}
   look like data errors (assess uses x 304 / 272), but they are what the binary holds.
6. **pickup_take_selected frame:** items at 0x10 and msg at 0x50 in a 0xc0-byte frame leave 0x10
   unused bytes; `char msg[112]` fills 0x50-0xbf. The declared size (and whether another local
   existed) is a guess.
7. **switch vs if:** single-key tests are `mov eax, key; dec eax; je` (switch shape), so ESC / ENTER
   are written as switches with the wheel code after them. In pickup the ENTER body precedes the ESC
   body, so ENTER is written first. pickup_button_action's bodies are laid out 4, 6, 2, 8 and written in
   that order; assess_button_action's (6, 4, 2 with a shared 0x25 sound) may be tail merging of a
   2, 4, 6 source.
8. **max / min:** the setle / dec / and clamp and the single ftol in the drag functions mean the clamp
   is applied to a local (otherwise a macro would evaluate the ftol twice). pickup stores the clamped
   value twice (max then min), assess once; written accordingly.
9. **Signatures guessed from calls:** `bm_load(name, path_interface, -1)`, `progress_update(0)`,
   `mp_send_container_close(handle, num_taken, num_items, items, selected)`, `sound_play_2d(id, vol)`,
   `item_get_display_name(buf, ItemType *)`, `party_inventory_sort(party_inventory, count)`,
   `character_get_protection` as a method of the +0x558 sub-object. `assess_init` / `pickup_init` take
   the screen argument as `Character *` / `container *`.
10. **char_info +0x38 and ItemType +0x68 damage** are read zero-extended (`xor eax, eax / mov ax`), so
    they are declared `unsigned short` here, while symbols/types.h has `int16_t`.
11. The ScrollBarMid literals in pickup point at assess's copies, and pickup's "%d" at 0x589820
    (another unit's .data): identical literals are merged across objects, so the build must use
    string pooling (/Gf or /GF) for these addresses to come out right.
