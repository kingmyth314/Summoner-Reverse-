# Byte-match round 2026-11-10, worker 8 (D27100-D27199)

Batch: 39 differing functions in 16 units (vfx emitter, ai_move, interface, effect_mem, pfgeom, inputbox,
projectors, gr_3d, item_paging, items, new_options, new_pickup, netsession, snow, crc32, gr_line). Distances are
`vc6-compare --score` (strict, VC6_STRICT=1) on 2026-11-10. Most of the batch has been through four to six earlier
rounds; those lists were read first and not repeated.

Result: 1 new match, 2 closer. Strict sweep over the 16 units: 339 match, 0 lost. `link-build --config fixed`:
259 units, unresolved 0, multiply defined 0 (run in the worktree, 2026-11-10).

## New matches

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| vsdk/unnamed_inputbox_0x5521a0.cpp | 0x552310 | ui_inputbox::process | D27100 | success paths as the then-blocks (backspace and the typed-character path) | 419 / exact |

## Closer, kept

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| vsdk/unnamed_inputbox_0x5521a0.cpp | 0x552870 | ui_inputbox::set_valid_chars | D27101 | one function-scope `char lo, hi` for the range macro | 183 / 173 |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 0x552c50 | ui_inputbox::set_invalid_chars | D27101 | the same | 182 / 172 |

(Both distances count the jump tables, which build/sum.asm disassembles as code: about 40 of each.)

## Decisions

**D27100 (vsdk/unnamed_inputbox_0x5521a0.cpp). ui_inputbox::process 0x552310 is exact.** The binary lays out the
failure exits of the typed-character path after the success path: `is_char_allowed` fails with `je` to a
`return key` block at the very end (0x5525a9), and the length test fails with `jge` to the `send_event(0x16);
return 0;` block before it (0x55258b). Written as nested then-blocks,
```
if (is_char_allowed((char)c)) {
    if ((int)strlen(text) < max_length) {
        ... strcpy, gr_get_string_size, the FIT_WIDTH test, strcpy(text, buf); send_event(0x15); return 0;
    }
    send_event(0x16, UI_USE_ID);
    return 0;
}
return key;
```
8966 gives that layout (the early-return form put both exits inline). The backspace case is the same device on a
smaller scale: `if (strlen(text) != 0) { remove the last character; send_event(0x15); return 0; }
ui_play_sound(5); return 0;` puts the removal inline after `je` (the binary's order); `== 0` with the sound first
had it the other way round. Same behaviour: the same tests with the branches swapped, every path returns the same
value after the same calls.

**D27101 (vsdk/unnamed_inputbox_0x5521a0.cpp; closer only). ui_inputbox::set_valid_chars 0x552870 (183 -> 173) and
set_invalid_chars 0x552c50 (182 -> 172).** The binary keeps one stack slot for lo (`mov byte [esp+0x10], dl`, read
back as `mov ecx, [esp+0x10]; and ecx, 0xff` for the memset address) in all three range blocks, and its frame is
0x8c. The block-scope `char lo / hi` of INPUTBOX_MARK_RANGE gave three slots and a 0x94 frame. Declaring `char
lo, hi;` once at function scope (the macro now assigns them) gives the binary's frame, slot and prologue. Same
behaviour: lo and hi are written before every read in each expansion.
Left: the binary does not cross-jump the three range blocks. In the digit and capital blocks the `state = 0` store
(`xor esi, esi`) is scheduled inside the memset (before `rep stosb`), their `lo > hi` exits jump into the small-letter
block's `state = 0; jmp` tail, and each block has its own memset. 8966 merges the three memset tails (with the
shared lo slot it merges the whole block after the lo store; with three slots it merged from `mov eax, 0x01010101`).
The final else-switch differs the same way (8966 merges case 1's `allowed[c] = 1; state = 8` with case 5's). Tried,
none stops the merge: `state = 0` inside both branches of the `lo <= hi` test (in all three blocks, or only in the
digit / capital ones with a second macro), `state = 0` before the range code (284), a file-local `static inline`
range helper (221: the separate slots come back).

**D27102 (notes only). Tried without a match.**
- ui_message_box_show 0x413e40 (49): the binary stores the flags word, re-reads its low byte (`mov cl, byte
  [0x5bd504]`), loads `mov al, 1` and tests `test al, cl` and then `test byte [game_flags], al`. A plain byte
  read through `*(unsigned char *)&ui_message_box_flags` is forwarded from the stored value (no change); the
  volatile byte read of D20407 gives the binary's size (469) and the `mov al, 1` / `test [game_flags], al` pair, and
  it fixes the `interface_cur_screen` switch registers, but 8966 sinks the re-read below the callback and mouse_mode
  stores and swaps the flags / callback stores (48). `1 & x` operand order: no change.
  **Header proposal (not applied):** with `unsigned int flags; void (*callback)(int);` as two more fields of
  ui_message_box_state (they sit at 0x5bd504 / 0x5bd508, right after hover_button 0x5bd500, as the comment at the
  struct already suspects), the plain `ui_message_box.flags & 1` test gives `mov al, 1` and the
  `test [game_flags], al` without volatile, with the stores in the binary's order (flags, then callback); left is
  that the test byte comes from the stored register (`mov cl, dl`) instead of the binary's reload. This needs
  interface.h's `extern unsigned int ui_message_box_flags` (used by other units) to become a field, so it is a
  header change for the lead.
- items_parse_table 0x44a090 (51): the only real code difference is the register rotation of the armour-texture
  switch's case blocks. All 24 source orders of the four case blocks (case -1, 0, 2/5, 3) were measured
  (with a scratch script): 0231 and 0312 give 45, the binary's source order gives 51, the rest 47-59. The rotation follows
  the source order (each block takes the next registers after the previous block's), and the binary has the layout
  of order 0123 with the registers of 0231 for blocks 2 and 3 and ECX for block 1; no order gives both. Not kept
  (the 45 forms move the block layout away from the binary's). Also tried: `case 5: case 2:`, `tex =
  t->armour_textures[heavy][hero]` for part -1 (both 51).
- gr_setup_3d 0x50dec0 (2): the 0.98 / 65536 as an if / else (4); a comma expression in the second test (2).
  With D26001's list: exhausted.
- inputbox, the other units: read the alignments of all 39 functions. The rest are the cases the earlier lists
  describe and that no new device here touches: register choice or scheduling (Crc32::update, snow_start,
  item_page_out_all, pickup_draw_item_list, fx_recb0::spawn / get_fade / render_d3d, seg2d_point_near_middle,
  travel_steer_update's operand orders), the kept `lea` of a link field (mp_player_alloc, vfx_instance_create,
  D24507), the final flags reload of gr_project_vertex, the `fiadd` of init_particle (D25602), the second Glide
  test of gr_setup_3d (D26001), effects_parse_arr_line's entry exit target, and the large ai_move functions
  (avoid_characters, follow_path, mode_traveling_frame, turn_towards, ai_find_nearest_wall_hit: EBX pushed and
  used for 0, ternaries as branches, many scheduling differences; not attempted in depth).
