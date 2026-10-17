# match27: second 8966 pass (pathfinding, characterinfo, interface, new_hud, level_scripts_common, new_inventory, spells, new_pxochat)

Compiler: cl 12.00.8966 (scratchpad\vc6sp5\VC98), flags from src\vc6-flags.txt (all eight are /O2 /GX /MT
game units). Tree: copy of snapR\src\decomp. Header edits: none (one proposal, D8604, tested but not applied).
Decisions D8600-D8605 appended to src\include\DECISIONS.md.

## Per file (VC6 byte matches, before -> after; none lost, checked address by address)

| file | before | after | of |
|---|---|---|---|
| game/pathfinding.cpp | 19 | 22 | 36 |
| game/characterinfo.cpp | 55 | 56 | 70 |
| game/unnamed_interface_0x411270.cpp | 55 | 55 | 69 |
| game/new_hud.cpp | 46 | 49 | 60 |
| game/level_scripts_common.cpp | 65 | 67 | 78 |
| game/new_inventory.cpp | 56 | 56 | 69 |
| game/spells.cpp | 26 | 28 | 39 |
| game/new_pxochat.cpp | 41 | 43 | 57 |
| **total** | **363** | **376** | **478** |

mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 `error:` lines for the eight files (no header changed).
before.txt / after.txt hold the per-address results (before.txt also lists pathfinding_visibility.cpp, which
is not a file of this pass).

## Won (13)

- pathfinding_init 0x49d980 (known open case): the pointer NULL stores written before the two memsets; 8966
  then schedules them after with the memsets' zero EAX and emits `xor eax, eax` first.
- pf_wall_normal 0x49e050: `vector2 a(w->a->x, w->a->y), b(...); d = b - a;` (the fld / fstp copies are the
  float constructor).
- pf_slide_move 0x49e470: the ends through a file-local inline `pf_set_xz(vector2 *, float, float)`, the
  slide in vector2 operators (`n = s.hit - *s.last_corner`, `d = s.to - s.hit`, `push = 0.015f * n`,
  `proj = n.dot(d) * n`, `s.to = d + s.hit - proj + push`), size stored before layer, out written x, y, z.
- characterinfo_find_multi_slot 0x449a30: `else info = &char_infos[index]; return info;` (one exit; the
  record return is laid out before the slot return).
- hud_button_action 0x40eeb0: LOGIC FIX (D8600) case values 43 / 46 / ... / 61 from the byte table; case 32
  after 39.
- hud_portrait_action 0x40f1b0: each case with the whole select / status / sound tail (8966 tail-merges it
  down to the pushed slot and one call with merged pops).
- hud_draw_hot_buttons 0x410020: the skill icon frame through a local, as for spells (8966 copies the small
  lookup block into each branch).
- script_object_receive_go 0x4ce220: LOGIC FIX + GAME BUG (D8602), a switch for the 4 / 0xb / 0x19 tests, two
  goal_find calls for the selected constant.
- lc_entry_add 0x4cd390: the flags byte written before the strcpy.
- spell_effect_obj_set_timer 0x47e4d0 (was marked exact, differed): the x / z pairs through an inline
  `set_xz(float *, float, float)`.
- spell_effect_alloc 0x47f690: `e = NULL; for (..) if (free) { ...; break; } return e;`.
- pxochat_draw_chat_lines 0x424b80: cases 1 and 6 with their own bodies (the 0..5 jump table), two block-scope
  `char name[64]` (frame 0x94).
- pxochat_button_action 0x4241d0: case bodies in the binary's block order (14, 16, 29, 31, 2 ... 12, 18, 20,
  22, 26).

## Closer, still different

- ui_choice_popup_show 0x4136a0 (+0xc -> +0xb3) and ui_message_box_show 0x413e40 (+0x86 -> +0x12e): the popup
  and message box .bss as file-local structs (D8603). The rest: register choice for the count, and the flags
  re-read as a byte (`mov cl, byte ptr [flags]`, probably a bit-field in the original; interface.h declares
  the flags, not changed).
- spell_place_ground_effect 0x47e100 (+0xe -> +0x105): vector2 locals and operators; the rest is the
  scheduling of the avoidance description stores (vector2 members in the original, D8601).
- interface_init_global 0x4116c0 (+0x0 -> +0xe): the first resolution switch with case 2 first and the 0 / 1 /
  default bodies apart gives the binary's compare chain; then the FPU register variable differs (screen_w on
  the FPU in the binary, base_w in ours; declaration orders, if forms and a default case tried).
- summon_stash_for_cutscene 0x448000 (+0x12 -> +0x72): `if (s) {...} else {inactive loop}` with the stores in
  the binary's order; then process()'s nested get_anim_length call is evaluated before the two constant pushes
  in the binary (float local, block, double constant tried).
- character_can_use_summon 0x446e90 (+0x8 -> +0xa): the ring / flag tests as `if (x) return true; return
  false;` (the binary's `mov al, 1` blocks and cross-jumped calls); 8966 still emits setne for each (ternary,
  negated, local, if / else and inline-wrapper forms tried).
- hud_handle_keys 0x40d930 (+0x29a -> +0x319): an element pointer in the tooltip loop (the binary biases the
  pointer to +0x10). Then the shared sound tail: the binary keeps the ESC copy, 8966 the last one; reordering
  the cases changes nothing (compare tree).
- ui_tooltip_set 0x414750: the clamps through `lim` (`if (x < lim) lim = x; x = lim;`) and the y sum in load
  order; the max(x, 0) AND register (D7506) and the x expression's register order remain.

## Tried without result (30-minute limit)

pf_read_edges 0x49c970 `lea ebp, [eax]` (for / do / while with signed, unsigned and local counts, read_int
into the count first); the zero register shared by a compare with 0 and later stores in the pxochat version
popups 0x423370 / 0x4233d0 / 0x423420 and inventory_popup_result 0x416b80 (switch, explicit case 4, empty if,
nested, if / else); hud_drag_icon_begin 0x410570 (`or al, 1 / and eax, 1`: store orders, signed fields);
hud_portrait_get_color 0x40f880 (clamp forms; the binary's second register cl); hud_draw_tooltips 0x40eb30
(frame slot order: eight declaration orders, no effect); inventory_drag_to_equip 0x417220 (the binary
cross-jumps case 6/8's `== 8` arm into case 7's block: closer with `n = 1` in each arm but 16 bytes larger,
not applied); party_find_next_member 0x413200 (nested, NULL, direct global forms); characterinfo_load_party
0x4450c0 (zero register ebx); pf_sweep_test_wall 0x49dc20 (needs the whole function in vector2 operators,
frame 0x48).

## Logic errors and game bugs

- D8600 hud_button_action: the decomp's panel cases 42 ... 60 were one below the binary's 43 ... 61 (the
  Press entries), so those buttons did nothing in the source. Fixed.
- D8602 script_object_receive_go: GAME BUG. The action pointer is computed once from cur_action
  (0x4ce280-0x4ce283) and never advanced (loop end 0x4ce2b8-0x4ce2c1), so only the current action is tested,
  num_actions - cur_action times; a "wait for go" action later in the list is never found. The decomp walked
  the list; it now reproduces the binary.
- D8604 (proposal): character_instance's tint is three ubyte members, not ubyte[3] (statscreen_clone_instance
  0x412f20 matches with that; needs edits in vsdk/character_instance.cpp and
  vsdk/unnamed_ca_render_0x520420.cpp, files of other passes).

## Markers

Raised to exact: the 13 wins and cmd_levelchars 0x449c00 (already a match). Lowered to likely (were exact,
differ under 8966): 0x447e80, 0x4caed0, 0x4cd330, 0x416290, 0x4165c0, 0x49cc50, 0x47e830, 0x413200, 0x413290,
0x4134b0, 0x413c70, 0x414110.
