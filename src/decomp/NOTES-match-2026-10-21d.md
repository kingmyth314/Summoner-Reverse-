# Byte-match round 2026-10-21, fourth: the fourteen workers' notes

The 129 functions at distance 13-40 that round 3 did not have, at 81-150, and the follow-ups earlier workers named
(the `*_anim_remap_in` functions, `levelrender_add_lobj`, `snd_stream_fill_pcm`), in fourteen batches of whole
files. Result: 29 new matches (strict 5,520 -> 5,549, none lost), about 35 closer. Not merged: worker 12's
`ix_box_oriented_box_oriented` change (D21906: many products moved into float locals for a non-match, which can
round differently on x87). The decisions are D20800-D22104 in include/DECISIONS.md.

## Worker 1

### m-1021d worker 1 (D20800-D20899)

Batch: game/level_scripts.cpp (8 functions), game/new_inventory.cpp inventory_drag_to_equip,
game/unnamed_phoenix_0x497ac0.cpp phoenix_anim_remap_in.

## New matches

None.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| game/unnamed_phoenix_0x497ac0.cpp | 0x499ad0 | phoenix_anim_remap_in | 78 -> 36 | D20800 |
| game/level_scripts.cpp | 0x4d06e0 | savegame::set_speaker_bit | 129 -> 73 | D20801 |
| game/level_scripts.cpp | 0x4d0870 | savegame::get_topic_bit | 110 -> 82 | D20802 |
| game/level_scripts.cpp | 0x4d1110 | savegame::two_stage_get | 141 -> 83 | D20803 |
| game/new_inventory.cpp | 0x417220 | inventory_drag_to_equip | 112 -> 65 | D20804 |

**D20800 (game/unnamed_phoenix_0x497ac0.cpp; closer). phoenix_anim_remap_in 0x499ad0: 78 -> 36.** Tiger's D19401
applied: the weight store written in every branch, including both arms of the ready test, no `continue`. Same
result as tiger: zero in EBP, ready in c's slot, unfolded 3 / 0 stores, both arrays from the one set pointer. Left
(as tiger): the three loop registers rotated (slot base / weight base / temporary ECX / EDX / ESI in ours, EDX /
ESI / ECX in the binary) and push esi placed earlier in the binary. Tried on top, all 36: unsigned `a`, set
declared late, a mount copy `m`, `a` read through a const int pointer, a while loop; the count store before the
rider test is worse (53). ghost_anim_remap_in / serpent_anim_remap_in should take the same change (not in batch).

**D20801 (game/level_scripts.cpp; closer). savegame::set_speaker_bit 0x4d06e0: 129 -> 73.** The bit written
as set_bit's D12300: `int bit = i + kind; int mask = 1 << (bit & 7); int byte = (bit >> 3) + bits_off;` before
the value test, then `data[byte] |= mask` / `&= ~mask`. The bit code now matches the binary's. Left: the
prologue registers (binary: this in EBX and stored to the frame at once, level only in ECX and reloaded into EBP
for the propagate loop; ours: level in EBX for the whole function, this in EBP), so the propagate loop's table
pointer / level registers are swapped and `this` is reloaded differently; and the search loop (the binary walks
an `hp` pointer created after the count test, as get_speaker_bit). Tried, not better: the get_speaker_bit hp
loop with the count through the macro (80: hp before the test, level reloaded after the loop), `kind + i` (73 /
80), `bits_off + (bit >> 3)` (80 with hp), `savegame *self = this` for the propagate calls, an `int lv = level`
copy for the propagate loop, `if (propagate) { ... }`, the propagation as a static inline helper taking (sg,
level, slot, speaker, kind, value) (73 / 80), level through `const int *lp = &level` (91 / 80 / 73).

**D20802 (game/level_scripts.cpp; closer). savegame::get_topic_bit 0x4d0870: 110 -> 82.** The speaker count read
through CONVERSATION_SAVE_INFO in the loop condition and the re-test (D16806's device; the `n` local removed),
the counts read through `int *counts = (int *)(data + counts_off)`, and `first = 0; count = 0;` in that order.
This gives the binary's 8-byte frame (`sub esp, 8`, hash spilled), the zeroed count and the re-test. Left: 8966
walks two pointers (counts and hashes) where the binary walks the counts pointer and reaches each hash by a
loop-invariant difference (`mov eax, 4; sub eax, ebx; mov ebx, [eax + ecx]`), so data / first / hash sit in other
places (binary: data in level's argument slot, hash in slot's argument slot, first in EBP). Tried: the macro alone
(101), both pointer locals in either order (98), only a hashes pointer (111), the hash read first into a local
(no change), eight spellings of the two address expressions (all 101), the topic loop as an `hp` walk (95-125),
`(data + (i >> 3))[bits_off]` at the end, a separate `thash` for the topic hash, `count = 0` dropped (128), the
counts pointer declared before the hash call (87).

**D20803 (game/level_scripts.cpp; closer). savegame::two_stage_get 0x4d1110: 141 -> 83.** The hash row read
through a byte offset computed at the top with base: `int hoff = SAVEGAME_NUM_LEVELS * 4 + level *
SAVEGAME_MAX_TWO_STAGE * 4;` and `((unsigned int *)(two_stage.data + hoff))[i]`. 8966 then computes the offset
before the calls (`lea ebp, [4*edx + 0xcc]`) and re-reads two_stage.data after them, as the binary; the prologue
matches. Same value as `TWO_STAGE->hashes[level][i]`. Left: the binary keeps hash in the frame (0x48 frame) and
loads it into EBP each pass with the pointer in EDX, where ours compares EAX with [EBP]; the door loop keeps
obj->name in EBX in the binary (ours spills it). Tried: the same offset as `(int)&((savegame_two_stage
*)0)->hashes[level]` and two other spellings (all 83; offsetof is not available here), an `hp` walk (144), `const
unsigned int *hk = &hash`, `unsigned int hash[1]`, the operands swapped (all 83), `volatile` hash (101). Without
hoff: a hashes row pointer (106), h local, hp walk, hash pointer (141-144).

**D20804 (game/new_inventory.cpp; closer). inventory_drag_to_equip 0x417220: 112 -> 65.** Case 6 / 8 written
`if (...) goto shared_27; buttons[0] = 18; n = 1; break;` with the label inside case 7's `if` block (`shared_27:
buttons[0] = 27; n = 1;` / else 19), the binary's cross-jumped 27 tail (79, only the ESI / EDI swap left). The
default case's `n = 0;` (n is still 0 there, so the same behaviour) then gives n ESI and entry / the button
pointer EDI as the binary (65). Left: that store (`xor esi, esi; mov [mode], esi` where the binary has `mov
[mode], 0`). Hunting for a zero-cost extra use of n: `n = 0` at the end, `n = n`, `n = 0` in an else of `if (ok)`
(58, but one more xor), `slots[1] = n` in case 1, `register int n`, four declaration orders (79-83), a block
`b`, an entry copy; none flips the registers without code. Without the goto: n in both arms of both tests flips
the registers but loses the tail sharing (86); the inverted test in case 6 is re-normalised by 8966 (86).

## Still differs (tried; do not repeat)

- **level_format_display_name 0x4d63b0 (16)**: the four text loads' registers (EAX / ECX / EDX / EAX vs the
  binary's ECX / EDX / EAX / ECX). Tried beyond D18303's list: a static inline text helper (102), the search as a
  for condition, `int n, i`, n as size_t / unsigned / declared at the strcspn, `!strlen`, `strlen < 1` (46),
  `n > 0` (18) / `n != 0`, `!(i < 51)`, `n == 0` first (123), a return after the strncpy branch, an lname local
  for the table name (104), a display-name table pointer local (16; set late 118), a `char *o = out` copy, a
  `char **tp` text pointer (89). Treat as exhausted (register choice).
- **savegame::get_speaker_bit 0x4d05f0 (18)**: hp created before the count test; `add eax, ebp` (bits_off) before
  data at the end. Tried beyond D18305: hp = data with pre-increment (30), a data pointer copy, the end through
  `ubyte *p = data + (bit >> 3)`, `p += bit >> 3`, `const int *bo = &bits_off`, a byte local, an if-return form
  (22), `i += kind` (45), a ubyte value local, three static inline bit-test helpers, the loop inside an
  explicit count test, a bool found local. All 18 otherwise; exhausted.
- **savegame::get_int 0x4d0030 (22)**: ECX / EDX swap only; not retried (D18305's list is complete).
- **savegame::write_item_objects 0x4d2370 (36)**: count / f frame slots swapped and `lea ecx, [ebx-16]` hoisted
  above the tests. Tried beyond D18306: c set after the tests (56), `c->get_item(0)`, count through `int *cp`
  (twice), piles_off through a pointer, f through a `float *fp`, piles_off replaced by count, a float / int union
  for the temporary (one loop or both), piles_off declared at its store, unsigned count. All 36.
- **savegame::read_items 0x4d2b70 (123)**: 8966 strength-reduces `8 + i * 4` into a frame temporary and spills i;
  the binary keeps i in EBP with `[eax + 4*ebp + 8]` and spills the ring hash. Tried: unsigned i with a signed
  test (125), an array index, the remap helper written out in a block or flat, a raw hash local, memcpy of the
  hash, an `off` int local, `i << 2`, `this->`, a data local, a this copy, a while loop, `register i`, the bit
  test operand order. All 123; exhausted.

## Behaviour errors / bug candidates

None found. Every change keeps the behaviour (the hoff offset is the byte offset of hashes[level]; the default
case's n is already 0; the goto reaches the same stores).

## Header changes

None.

## Changed files

- src/game/unnamed_phoenix_0x497ac0.cpp (phoenix_anim_remap_in, D20800)
- src/game/level_scripts.cpp (set_speaker_bit D20801, get_topic_bit D20802, two_stage_get D20803)
- src/game/new_inventory.cpp (inventory_drag_to_equip, D20804)

## --check

`VC6_STRICT=1 vc6-sweep --only "level_scripts\.cpp|new_inventory\.cpp|unnamed_phoenix_0x497ac0\.cpp" --check
src/decomp/vc6-matches.txt`: 3 units: 209 match, 0 wrong target, 20 differ, 0 not found, 0 errors; 209 matched
there, 209 now, 0 lost.

Notes for the merge: two_stage_set 0x4d0e20 (327) and read_two_stage 0x4d34e0 (173) index the same hash rows and
might take D20803's hoff offset; ghost / serpent_anim_remap_in take D20800 as tiger / phoenix.

## Worker 2

### Worker W2 notes (round m-1021d, decisions D20900-D20999)

Batch: ten functions (script editor x6, m2v_skip_extension_and_user_data, pickup_draw_item_list,
options_load_bitmaps, serpent_anim_remap_in). Helper scripts in W/try (v.mjs prints the distance per
variant; side.mjs is a copy of tools/vc6-try.mjs that prints the side-by-side from line 0; the *.js files
build the variant lists).

## New matches

- game/unnamed_script_editor_0x4de0b0.cpp 0x4e7460 script_editor_select_prev, **D20901**.

**D20901 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_select_prev 0x4e7460 matches with the
character walk written as a while loop over the "not wanted" test, the previous link in a local, and
`script_editor_sel.object == p` as the end test:**

    script_editor_sel.object = ((Character *)script_editor_sel.object)->live_prev;
    while (!(living && (species != 0x59 || !character_is_serpent_part(...)))) {
        p = ((Character *)script_editor_sel.object)->live_prev;
        if (script_editor_sel.object == p)
            break;
        script_editor_sel.object = p;
    }

The binary reads the link once for the end test and stores that same register as the next object (its
`jne` goes back to the `mov ecx, eax / mov [sel.object], eax`, not to a reload). The old do / while
(`sel.object = sel.object->live_prev; ... } while (sel.object != sel.object->live_prev)`) folds the end
test's read into `cmp eax, [eax+0x31c]` (81). On the way: p local with do / while (70), the same with the
compare swapped (68), the while form with `p == sel.object` (15: only `cmp eax, ecx` against the binary's
`cmp ecx, eax`); `sel.object == p` gives the binary's order (so does `(Character *)sel.object == p` and an
`if (sel.object != p) store; else break;`). Same behaviour: each pass stores the link and tests the new
object, and stops when the link points at the object itself, as before.

## Closer, kept

- vsdk/targautils.cpp 0x5549c0 m2v_skip_extension_and_user_data: 19 -> 11, **D20900**.
- game/unnamed_serpent_0x495290.cpp 0x495d00 serpent_anim_remap_in: 78 -> 36, **D20903**.
- game/unnamed_script_editor_0x4de0b0.cpp 0x4ebb20 script_editor_door_sound_inputbox_cb: 98 -> 90, **D20902**.
- game/unnamed_script_editor_0x4de0b0.cpp 0x4e8020 script_editor_edit_button_cb: 123 -> 67, **D20904**.
- game/unnamed_script_editor_0x4de0b0.cpp 0x4ecbe0 script_editor_level_item_ok: 140 -> 74, **D20905**.

**D20900 (vsdk/targautils.cpp; closer only). m2v_skip_extension_and_user_data 0x5549c0: the picture coding
extension's composite-display test with a next_start_code call in each arm** (`if (get_bits(1)) {
get_bits(20); next_start_code(); } else next_start_code();`). 8966 then keeps the display extension's
(case 2) tail separate as the binary does, and the registers of case 2 (EDX first) match. Left (11): the
first vcall loads EAX before ECX (binary ECX first), and the sequence extension's (case 1) tail is merged
into case 8's (`jmp` to it) where the binary keeps case 1's tail and sends case 8's no-composite branch into
it. Tried with this form (all 11): case 1 / case 2 with `continue`, case 2 with both arms of its test
holding the 29-bit read and the call, case 2's test braced, case 8's composite branch `continue`; the
inverted test (`if (!get_bits(1)) nsc(); else {...}`, 18). Tried without it (all 19): a shared
next_start_code after the switch (case 1 break, the others `continue`), a `tail:` label in case 1 that case
8's no-composite branch jumps to (also with every case 2 form above), the six source orders of the three
cases with each case 8 form (order changes nothing). 8966 merges identical "next_start_code; jmp top" tails
into whichever block it takes as canonical; the binary merges only case 8's no-composite edge.

**D20903 (game/unnamed_serpent_0x495290.cpp; closer only). serpent_anim_remap_in 0x495d00: tiger's D19401
form** (the weight stored in each branch, both arms of the ready test too, no `continue`): 78 -> 36, the same
residue as tiger_anim_remap_in (the three loop registers ECX / EDX / ESI in ours, EDX / ESI / ECX in the
binary, push esi early). Also tried on top of it (36 or worse): a `serpent_boss *sb = &serpent` pointer, a
`loco_blend_set *so = set` copy for the weight reads, memcpy for the weight, a float weight local (77). The
same change should apply to ghost_anim_remap_in 0x4bb610 (not in this batch; phoenix has D20702's volatile
form at 70, the per-branch store gave tiger 36).

**D20902 (game/unnamed_script_editor_0x4de0b0.cpp; closer only). script_editor_door_sound_inputbox_cb
0x4ebb20: the Tab case's two set_focus calls each with its own `return true`** (the binary has two calls,
8966 joined them through ECX). Left (90): the binary pushes ESI right after the value test (its early
return pops it) and keeps the chosen list both in value's dead argument slot and in ESI (`mov esi, imm;
mov [slot], esi` for the open list, `mov [slot], imm` / `mov esi, [slot]` for the close list), then walks
the entries in ESI. Tried (90 or worse): the radio test as nested if / else (110) or with the open list
stored first (101), a block-local alias, `list` declared in the case block, `register list`, list declared
before first, a `ui_listbox &` alias, a second pointer copy for the macro, the value test as one exit (138),
first = NULL inside the case (96), `default:` first.

**D20904 (game/unnamed_script_editor_0x4de0b0.cpp; closer only). script_editor_edit_button_cb 0x4e8020:
SCRIPT_ACTION_FOLLOW's case before SHOW_HIDE / SYNCH in the source.** The binary's case blocks (and its
string literals: "NPC to follow" 0x5a0ed0, "time (-1=inf)" 0x5a0ec0, "show(1) hide(0)" 0x5a0eb0,
"synch_point" 0x5a0ea4) come in that order; 8966 lays case blocks out in source order. Same behaviour (case
order only). Left (67, most of it the jump tables read as code): the binary pushes ESI / EDI after the
event / is_disabled tests (its return-false block has no pops); 8966 pushes them at entry in every form
tried: the two tests as separate ifs (308), a bool local for is_disabled, the body inside `if (event ==
PRESSED && is_disabled() == true) { }`, a `goto fail` to a final `return false`, the clearing loop as a file
inline helper or with a block-scope index.

**D20905 (game/unnamed_script_editor_0x4de0b0.cpp; closer only). script_editor_level_item_ok 0x4ecbe0: the
name search as `again: for (i = 0; level_item_count > 0;) { if (strcmp(...) == 0) { sprintf(...); goto
again; } i++; if (i >= level_item_count) break; }`.** The binary's loop is rotated: entry test `test eax,
eax / jle` on the count, the bottom test `cmp edx, eax / jge out / jmp top`; the old `for (i = 0; i <
level_item_count; i++)` tested at the top each pass (140). Same passes: the entry test is `0 < count` with
i = 0, and on every back edge count > i >= 1. Steps: `for (i = 0; i < level_item_count;)` with the break
at the bottom (90; also with `++i`, `!(i < count)`, a while), `0 < level_item_count` / `level_item_count >
0` as the for test (74). Forms with an explicit `if (level_item_count > 0)` around a do / while or for(;;)
or a `goto` past the loop make 8966 peel the first pass (911-913 bytes, 215-234). Left: the table pointer
`mov edi, level_items` is set before the entry test (binary after), and the strcpy's entry pointer load is
scheduled before the copy (binary after the count load). Not changed: li->count / strcpy order (122), a
type local, memcpy for the script name (128).

## Still differs (tried; do not repeat)

**D20906 (notes only). script_editor_default_action_navpoint 0x4e8cf0 (28).** Tried (28 unless noted): a
`goto done` to the shared return, the found store through `navpoint **fp = &found`, a bool local for the
type test, `found` in the branch initialised from np (37), a do / while with the entry test (43), one exit
through a result variable, `break` with a return after the loop inside the count test (32), np declared
after found with a `cur` local returned from the loop, np set to found and returned. The binary gives found
its own register (EAX) with the NULL in ESI copied on the empty exits; 8966 keeps found in ESI. Register
choice.

**D20907 (notes only). pickup_draw_item_list 0x421370 (32).** Tried (all 32 unless noted): x through an
int[2] array, the origin copied as a pickup_point struct (plain and memcpy), a `const int *px = &x` for the
first gr_string, a `bool *sel` for the selected test, a do / while loop with an early return, a while loop,
`pickup_hover_item == i`, `y += fh` in the for increment (36). x / i keep EBP / EBX (binary EBX / EBP).

**D20908 (notes only). script_editor_action_type_allowed 0x4e7a40 (116; all code identical, the distance
is the jump tables read as code plus three branch targets).** The three latched-false `je` go to the do
state case's loop-exit "return false" in the binary; 8966 sends them to the mount case's species-fail
block (L62809) in every form: do state ending `return false`, the mount case as `if (!latched) break;`
(+4 bytes), as if / else returns, with a `found = false; return found`, the mount test as `return a && b`
(both forms change the code), a `goto fail` to a label at the do state's return (8966 moves the label to
the mount block), source case orders (mount first / last, do state last, reversed, default first: all
worse or the same). 8966 picks the canonical "return false" block itself.

**D20909 (notes only). options_load_bitmaps 0x41ef30 (112; one register, the rest is the jump table).**
The increment of i reloads i into ESI in ours, EDI in the binary. Tried this round (all 112 or 114): i
declared before scale, `++i`, a do / while, `int *hp = &b->h` for the h scale or for bm_get_dimensions, w /
h pointer locals. Earlier rounds tried five more. Register choice.

m2v_skip_extension_and_user_data, serpent_anim_remap_in, door_sound_inputbox_cb, edit_button_cb and
level_item_ok: see their closer entries above for what was tried.

## Behaviour errors / bug candidates

None found. Every change keeps the behaviour (D20901 and D20905 are loop reshapes with the same passes;
D20904 only reorders case labels).

## Header changes

None.

## Changed files (in W/src)

- game/unnamed_script_editor_0x4de0b0.cpp (D20901, D20902, D20904, D20905)
- game/unnamed_serpent_0x495290.cpp (D20903)
- vsdk/targautils.cpp (D20900)

## --check result (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

- unnamed_script_editor_0x4de0b0: 106 matched there, 107 now, 0 lost (new: script_editor_select_prev)
- unnamed_serpent_0x495290: 39 matched there, 39 now, 0 lost
- targautils: 18 matched there, 18 now, 0 lost

## Worker 3

### W3 notes (m-1021d, decisions D21000-D21099)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_render_scene_0x481680.cpp | 0x4838f0 | levelrender_add_lobj | D21000 | `? true : false` on the small-mesh flag, centre pointer assigned inside the cull call, D19402 double copies for the distance, float copy of d.z for the depth |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 0x52c330 | ui_gadget::send_event | D21001 | no return after the handled test (D15400's form) |
| game/unnamed_dialogue_box_0x4d6660.cpp | 0x4d6e70 | dialogue_topic_entry::draw | D21003 | line y as `i * font_h + top + scroll`, wrap width in an int local |
| game/unnamed_dialogue_box_0x4d6660.cpp | 0x4d6b80 | dialogue_line::draw | D21004 | both line y's as `i * font_h + top + scroll`, `x = 0` before line_y |

**D21000 (game/unnamed_render_scene_0x481680.cpp). levelrender_add_lobj 0x4838f0 is exact (178 -> 0).** Four
changes, each needed:
1. The small-mesh argument of both gr_cull_sphere calls as `x > Small_mesh_threshold ? true : false`. The binary
   makes the flag as a byte (`mov al, 1` / `xor al, al`). The plain comparison passed to the bool parameter
   gives `mov eax, 1`. A `(bool)` cast does not change it. This took the distance from 192 to 48.
   NEW DEVICE: `? true : false` on a comparison passed to a bool parameter gives the byte form.
2. `vector3 *c;` with `gr_cull_sphere(c = &l->centre, ...)` in both branches, and d read through `c`. The
   pointer gives the binary's EDI for l and ESI for the centre (16). Assigning it inside the call puts the
   `lea esi` just before the push, as the binary; `c = &l->centre;` on its own line schedules it earlier (10).
3. The distance as `double dz = d.z, dy = d.y; float dist_sq = (dy * dy + dz * dz) + d.x * d.x - radius * radius;`
   (D19402's form). This keeps y / z on the FPU stack. The binary keeps dist_sq on the stack for the three
   compares, and the float local already does that.
4. `float tz = d.z; e->depth = tz * fvec.z + ...`. This gives the binary's d.z load first in the depth (a
   `double` copy works too; a pointer to d, to fvec or to d.z does not).

Same behaviour: the x87 code never rounds the products (as the binary), and the flag value is the same.

**D21001 (vsdk/unnamed_gadget_0x52bfe0.cpp). ui_gadget::send_event 0x52c330 is exact (20 -> 0)** with
`if (!callback || !callback(event, value)) { if (parent) return parent->...; return window->...; }` and no return
after it. On a handled event the function falls off the end with the callback's AL. That is the binary's jump to
the window call's epilogue (as ui_window::send_event, D15400). VC6 warns C4715. Same behaviour on VC6 (the
caller reads AL, which is nonzero). A `do { ... } while (0)` around the callback test also fixes the ESI / EDI swap
(15). On its own the missing return is enough, and it is the form kept.

**D21003 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_topic_entry::draw 0x4d6e70 is exact (81 -> 0).**
- The wrap width goes into an int local before gr_split_str (`int wd = width;`). This sets the binary's
  ebx / eax / ecx / edx order around the call (81 -> 73). Without it, the other change alone gives 142.
- The drawing loop is `for (i = 0; i < n; i++) gr_string(0, i * font_h + top + scroll, ...)`. 8966
  strength-reduces the product into the binary's y register and gives the binary's (y + top) + scroll. A separate
  `y += font_h` associates as (y + scroll) + top, which is the old form.
- The unused `y` local is removed. Same behaviour.

**D21004 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_line::draw 0x4d6b80 is exact (100 -> 0).**
- Both line y's are written `i * font_h + top + scroll`: kind 0's line_y and the kind 1 / 2 loop. Here the binary
  sums (y + scroll) + top. With the product written out, 8966 reverses the source order of the last two terms, so
  `+ top + scroll` gives it.
- `x = 0;` goes before `int line_y = ...`. This gives the binary's `xor edi, edi` after the line_y add (2 -> 0).
- The y local is removed (no longer used). Same behaviour.
- NEW DEVICE: in a loop, `i * h + a + b` instead of an accumulated y gives the other association of the last two
  terms. 8966 reverses the source order there; with the accumulated y it keeps it. The D12008 notes had 14 orders
  of `y + scroll + top`, all with the accumulated y.

## Closer, kept

- game/unnamed_tiger_0x499e50.cpp 0x49b0a0 tiger_pick_target: 32 -> 24, **D21002**.

**D21002 (game/unnamed_tiger_0x499e50.cpp; closer only). tiger_pick_target 0x49b0a0:**
- The base-threat sum through a float local: `float s = threat[1]; s += threat[0]; base_threat = -s;`. This loads
  threat[1] first, as the binary (`fld +0x38; fadd +0x34`). The distance does not see this, because the two absolute
  operands are masked. A `double` copy and a `float t[1]` give the same code. A `const float *` to threat[1] does
  not.
- The weight base in two statements: `float base = base_threat * 0.1f; base += 10.0f;`. This moves the two handle
  loads before the fadd, as the binary (32 -> 24).
- Left: the start-up zero is in EBX (the binary has EDX). 8966 reuses it in the first loop (`cmp esi, ebx`,
  `q.result = ebx`, `xor ebx, ebx` at the loop top).

## Still differs (tried; do not repeat)

- **dialogue_box::draw 0x4d7680 (19)** and **dialogue_topic_list::draw 0x4d7930 (29)**: the binary loads `first`
  into its own register (EBP / EDI) before the add. Tried this round, on top of D19102:
  - box: a `const int *fp = &first` in the loop and at function scope, `int f[1]`, `idx = i; idx += first`, an
    int copy of the scroll for the draw call, the line pointer stored through `dialogue_line **`;
  - topic list: `(i + first)`, the pointer to first, an idx local.

  All unchanged. Probably 8966's register choice.
- **ui_gadget**: none left in my batch.
- **tiger_pick_target 0x49b0a0 (24, after D21002)**: the zero register. Tried this round:
  - a do / while loop;
  - the query block in an inline helper (whole candidate test, or the score only);
  - count incremented through a pointer, q.result stored through a pointer, `q.result = NULL`;
  - the ok store before the c store;
  - count assigned after the g test;
  - `for (i = count; ...)`.

  Unchanged or worse. For the weight base, `float bt = base_threat` and a `const float *` to base_threat were
  unchanged; `int h = handle` first was worse.
- **vfx_instance::process 0x536fb0 (84)**: eax / ecx / edx rotations of call arguments (vif_emitter_update,
  update_light, the dummy update, the camera's get_transform). Tried:
  - `base->dummies + i` and a desc pointer local;
  - float copies of time for the dummy (83), light (87) and camera (78) calls. The copy is stored to a slot, so
    it is not the binary's form and was not kept;
  - `const float *` to time (91);
  - a light-track pointer (89), a camera pointer.
- **vfx_instance::init_nodes 0x536a70 (94)**: `this` in EDI and the node pointers in ESI (the binary has the
  reverse; push edi comes late). Tried:
  - node = nodes first;
  - reversed declarations;
  - a `self` copy, a base copy;
  - a float time local (worse);
  - `elapsed = (time = ...) * (1/15)` (87, same start; not kept).
- **assess_format_info 0x4013f0 (117)**: the binary keeps protection in EBP and damage in the argument slot; ours
  has the reverse. Tried:
  - declaration orders, protection declared late, `register int protection`;
  - a damage pointer local, `int damage[1]`, `int protection[1]`;
  - an inline damage helper, an inline percent helper;
  - the damage as a ternary (worse), the default damage first then the override (worse);
  - a second use of protection;
  - magic inline in the sprintf call (106, but the res copy moves before the call; not kept);
  - a resistance pointer copy, a memcpy of res.

  The source file is CRLF (the try tools need `\r\n` in multi-line patterns).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- game/unnamed_render_scene_0x481680.cpp (levelrender_add_lobj, D21000)
- vsdk/unnamed_gadget_0x52bfe0.cpp (ui_gadget::send_event, D21001)
- game/unnamed_tiger_0x499e50.cpp (tiger_pick_target, D21002)
- game/unnamed_dialogue_box_0x4d6660.cpp (dialogue_topic_entry::draw D21003, dialogue_line::draw D21004)

## --check (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- unnamed_render_scene_0x481680: 62 matched there, 63 now, 0 lost
- unnamed_gadget_0x52bfe0: 46 matched there, 47 now, 0 lost
- unnamed_tiger_0x499e50: 43 matched there, 43 now, 0 lost
- unnamed_dialogue_box_0x4d6660: 28 matched there, 30 now, 0 lost

## For later

- `? true : false` (D21000) may help other calls that pass a float comparison to a bool parameter where the
  binary has `mov al, 1`.
- levelrender_add_items / add_containers (D19402, distance 6) have the same distance test. Their remaining QWORD
  400 constant was not looked at here.

## Worker 4

### m-1021d worker 4 (D21100-D21199)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_localize_0x503f60.cpp | 0x504400 | localize_set_string | D21100 | the slot store and the room test read `localize_pool_used` itself; `dest` is used only for the strcpy and the return |

**D21100 (vsdk/unnamed_localize_0x503f60.cpp). localize_set_string 0x504400 is exact.** `char *dest =
localize_pool_used;` stays first, but `localize_strings[id] = localize_pool_used;` and `if (len >=
localize_text_buffer + LOCALIZE_TEXT_SIZE + 1 - localize_pool_used)` read the global again (nothing writes it in
between, so the behaviour is the same). 8966 then keeps the loaded value in EDX for the store and the
subtraction, gives `dest` its own frame slot (the binary's `push ecx`) and keeps id in EBX, as Sum.exe does.
Only the room test through the global (dest still in the slot store) gives distance 59 (a `mov eax, edx`
copy left); the slot store through the global as well removes it. Also tried: `char *dest[1]`, `char **dp = &d`
(both optimised to the same code as the plain local), an `int sid = id` copy (two places), `len` declared first,
the max-id store before strlen (81), the room as an int local or `(int)` cast, `dest + len >= end`, dest
declared late / after the room test (82, 73).

## Closer, kept

None.

## Still differs (tried; do not repeat)

**D21101 (notes only). Tried without a match:**

- ai_state::choose_support_spell 0x4bea80 (20): EBX / EBP swap only (binary: cure and then out in EBX,
  vitalize in EBP). Tried this round: a `pick` index set in the loop and tested after it (20; with an early
  `return -1` 58), a `boss_target *t = &list[i]` for the store and flag tests (29), the vitalize test nested
  (20), `vitalize = 0; if (type != 5 || elapsed()) vitalize = ...` (37), a bool local for the vitalize test
  (20), `bool vitalize` (31, twice), a bool `vit_ok` local (30), a goto past the can_cast call (20).
- ai_state::combat_toggle_ready_stance 0x4bdce0 (24): the binary keeps owner in EAX from the ready test to the
  stance_blend store, so ready goes to BL (the constant-1 register) and the toggle uses CL; ours has owner in
  ECX and ready in AL. Tried: `ready = (flags & READY) != 0; if (!ready) ready = flip;`, an int `rf` copy, a
  `(bool)` cast, `||`, an assignment inside the if, each with both toggles (`ready = !ready; flip = ready` 24,
  `flip = !ready; ready = flip` 46 or worse).
- glide_record::load_section 0x549a20 (21): one-float arrays for v (plain, initialised, in a block), for u and
  v, `const float *vp = &v`, memcpy of v (all 21, same code); file-local inline helpers for the out pointers,
  the record stores or all six stores (27-64); a `float *vs = &this->v_scale` (two places), a `u_scale` copy,
  the bitmap as an int local (21), v read before u (20, not kept: no match), a `tv` copy (22). The binary's
  register choice (u_scale pointer in EAX, v's int in ECX) is not moved by any of these.
- ai_state::consider_life_drain 0x4bf5a0 (92): ours walks the first loop from list + 8 and shares it with the
  later loops (EDI pushed late); a `boss_target *p = list` copy (also in a block) and a file-local inline helper
  for the first loop all give the same code (92).
- ai_state::combat_approach 0x4bda50 (82): the goal_do_cast return (neg / sbb / inc then test / setne): `!x ?
  true : false`, an int `ok` local, `(x == 0) != 0`, `!x != 0`, `(bool)!x`, `(int)!x ? :`, `x ? false : true`,
  `(x == 0 ? 1 : 0)` both ways, a bool local, `int r = !x; return r != 0`, unsigned / char casts, inline helpers
  returning `!x` / `x == 0` / a bool (82-83; `helper(x) != 0` gives the neg / sbb / inc without the test /
  setne). The boss branch's info reload: the species test as a switch (94) and as a nested if (82).
- localize_load_names 0x504520 (131): the case-1 block that Sum.exe places after the epilogue (the only
  difference: the frame and the rest of the body are the same). All six switch case / default orders (131,
  the same code), an if return-first form (139), a goto form (155).
- d3d_compute_pixel_format_shifts 0x53a370 (117): register and spill choice (the binary keeps desc in EBX, the
  shift count in ECX, r_bits in desc's dead argument slot and g_mask / b_mask in EDI / ESI). A `register` desc
  copy, a DDPIXELFORMAT pointer, the declarations in the binary's slot order, a mask copy per channel,
  DWORD counters, while loops: all the same code (117); an inline walker with pointer outputs (170).
- ui_slider::create_scrollbar 0x55ed30 (38): not retried (pure this / listbox ESI / EDI swap, 38; earlier
  rounds' forms cover the devices).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_localize_0x503f60.cpp (localize_set_string, D21100; comment updated to exact)

## --check

vsdk/unnamed_localize_0x503f60.cpp: `1 units: 27 match, 0 wrong target, 1 differ, 0 not found, 0 errors`;
`check against src/decomp/vc6-matches.txt: 26 matched there, 27 now, 0 lost`.

## Worker 5

### m-1021d worker 5 (D21200-D21299)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/character.cpp | 0x541020 | ca_character::report_memory | D21200 | the position and rotation counts each through a local and a pointer to it (`int pk = s->num_pos_keys; int *pp = &pk; int rk = s->num_rot_keys; int *rp = &rk; skel += *pp * 64 + *rp * 36 + s->num_bones * 45;`) |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 0x545870 | gr_poly_clip_and_draw | D21202 | the multitexture draw as `if (...) { split; reset; } else tmapper;` falling into the shared `done: free; return true;` (one exit after the texture-count test, so the EBP / ESI / EDI pushes are shrink-wrapped as in the binary, D8701) |
| game/unnamed_avoidance_0x4a6710.cpp | 0x4a6810 | cmd_avoidance | D21203 | one `checking_status_for_command = 0;` after the keyword if / else-if chain instead of one per branch: 8966 copies it into each branch itself and no longer threads the help branch's jump past the doing_help test |

**D21200 (vsdk/character.cpp).** ca_character::report_memory 0x541020 is exact. Only `pk` through `pp` gives
distance 12 (the sum is now (skel + pos * 64) + (rot * 36 + bones * 45), as the binary); adding `rk` through `rp`
gives the binary's load order and registers. A pointer for the bones count instead goes to 80; `*sp` pointer to
skel, a pointer straight to `s->num_rot_keys`, `pk` declared at function scope and the term orders did not.

**D21202 (vsdk/unnamed_gr_poly_0x5450c0.cpp).** gr_poly_clip_and_draw 0x545870 is exact; the multitexture branch's
own `free; return true;` was the region's second exit. `goto done;` in place of it matches as well. The unit is now
24 / 24.

**D21203 (game/unnamed_avoidance_0x4a6710.cpp).** cmd_avoidance 0x4a6810 is exact. Leaving the stores in the other
branches and moving only the help branch's one after the chain matches too; the single store is the plainer form.
The same shape (a status flag cleared in every branch of a console command's keyword chain, help branch threaded
past its test) may explain other `cmd_*` near misses.

## Closer, kept

- vsdk/character.cpp 0x540970 ca_character::add_skeleton: 102 -> 54 (**D21201**). offset / size in a block opened
  after `new` / reader / f, so they share the new-expression temporary's slot: frame 0x1b0 as in the binary (was
  0x1b4). Left: the epilogue follows the dup-found `return i` in ours (the binary's main path falls into it and the
  dup return jumps there), and the tail loads the slot index into ECX where the binary uses EAX. Tried for those: a
  `goto done` single exit with a result local (60), a found index with break (72), the new / file part in its own
  block (54), `goto found` to a `return i` at the end (54), `i = num_anims - 1` returned after the block (54), a
  `skeleton **` store pointer (58), `num_anims = num_anims + 1`, the stores inside the block (54).
- game/unnamed_avoidance_0x4a6710.cpp 0x4a8390 avoid_do_poly_seg_list_links: 131 -> 58 (**D21204**).
  `vec2_set(p, node->pos->x, node->pos->y);` (fld y, fld x as the binary), the door offset as
  `vector2 d = 0.03f * sl->corners[k].offset; p.x = d.x + node->pos->x; p.y = d.y + node->pos->y;` (frame 0x24,
  d.y in the binary's temporary slot, and the node pointer without the +4 bias), and `int nc = o->num_corners;`
  for the other lists' corner loop (the binary's count-down loop with a corner pointer). Same values (p equals
  node->pos before the door add; 0.03 * x is commutative). Left: the binary keeps p.y on the FPU across the door
  test (`fst`, then `fadd st, st(1)` and a `fstp st(0)` at the join) and adds p.x from memory; the o loop's end
  test (`cmp edi, end; jl` against ours `lea eax, [edi-4]; cmp; jb`); the two exits' register for saved_size.

## Still differs (tried; do not repeat)

- game/unnamed_avoidance_0x4a6710.cpp 0x4a6e00 avoid_prim_overlap (13, unchanged). On top of D18503, 20 more forms:
  the projection in a float local (30), a `const vector2 &` of *u (30), a pointer local to d in the loop (30), u->x in
  a float local (30), d.x in a float local (26), `u[0].x` indexing (13), `ku * ... < fabs(...)` (37),
  `!(fabs(...) <= ...)` (13), `d.y * u->y + d.x * u->x` (13, source order is canonicalised), a pointer copy uo
  (30), d assigned after u / v (17). For the dead `fst` into b's slot: kw on the u term with the u term written first
  (23), both terms through kw (91), volatile kw on either term (34), the u term written first without kw (13).
  A file-static inline dot helper loses the function in the compare tool (do not put a helper between comments).
- game/unnamed_ai_think_0x4b5140.cpp 0x4b5900 ai_state::pick_attack_target (148, unchanged). The binary holds -1 in
  EBP (`or ebp, -1`, this in ESI); ours uses immediates, this in EDI. Tried: an `int none = -1;` local for every -1
  (at the declarations and after the weapon test): folded back to immediates. The double max_attackers store: the
  union's `_unk08` and `max_attackers` both assigned, 3 orders: the dead store is dropped, no change.
- game/unnamed_ai_think_0x4b5140.cpp 0x4b6a90 ai_choose_attack_slot (148): not retried. D9600 / D9601 / D9810 /
  D17005: under `#pragma inline_depth(0)` on purpose (the original's rounding); a byte match needs the out-of-line
  vector3 constructor call that no source form gives.
- game/unnamed_level_complete_0x4b0960.cpp 0x4b1280 mp_handle_combat_sequence (120, unchanged). The binary stores
  hit / variant / skill as bytes and reloads variant / skill as dwords masked with 0xff into EBX / EBP before the
  first call; special is a raw byte stored into data's argument slot; t lives in variant's slot. Tried: ubyte special
  (129), pointer locals to variant / skill (129), int copies of variant / skill (121), memcpy reads of every field
  through an offset (129) or of the bytes only (129), the body as a static inline / __forceinline helper taking the
  fields as ubyte or bool parameters (127 / 122).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/character.cpp: report_memory (D21200, confidence exact), add_skeleton (D21201, closer).
- src/vsdk/unnamed_gr_poly_0x5450c0.cpp: gr_poly_clip_and_draw (D21202, confidence exact).
- src/game/unnamed_avoidance_0x4a6710.cpp: cmd_avoidance (D21203, confidence exact), avoid_do_poly_seg_list_links
  (D21204, closer).

## --check (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- vsdk/character.cpp: 32 matched there, 33 now, 0 lost.
- vsdk/unnamed_gr_poly_0x5450c0.cpp: 23 matched there, 24 now, 0 lost.
- game/unnamed_avoidance_0x4a6710.cpp: 57 matched there, 58 now, 0 lost.

## Worker 6

### m-1021d worker 6 (D21300-D21399)

Batch: travel_steer_update, mode_traveling_frame, turn_towards (ai_move), fast_inv_sqrt, float_format_buf
(floatmath), state_title_enter, Crc32::update, hud_hot_buttons_drag, gr_font_create.

Note on the distance: it compares bytes, so ESP frame offsets count. Two of the large distances here
(gr_font_create 120, hud_hot_buttons_drag 83) were mostly frame-slot and register-name differences that
vc6-side does not mark.

## New matches

- vsdk/gr_font.cpp 0x508560 gr_font_create, **D21300**. Two changes:
  - One function-scope `int row;` for the three glyph loops (each loop was `for (int row = ...)` before).
    The binary keeps row in name's dead argument slot. With one row for all three loops 8966 does that
    too, and src then takes the binary's slot (120 -> 53; the ALPHA4 loop also lost its entry jump).
  - The ALPHA4 gamma value goes through an int local: `int g = gr_gamma_ramp[alpha * 255 / 14] >> 4;`,
    then `(uint16_t)((g << 12) | 0xfff)`. Written as a single expression, 8966 narrows it to
    `xor edx, edx / mov dh, byte [..]`; the binary does the 32-bit `sar 4 / shl 12 / or 0xfff`. 53 -> 0.
- game/new_hud.cpp 0x40f340 hud_hot_buttons_drag, **D21301**. In the binary, hud_hot_button_hover is
  loaded into ECX and copied to EBP for dst. Changes:
  - The `>= 0` test, the first hot_button_get and the else branch's hot_button_set read the global
    `hud_hot_button_hover` itself. No call comes between them and the read of dst, so the behaviour is the
    same (83 -> 48).
  - `int member = hud_member();` is declared after `int dst` (48 -> 0).

## Closer, kept

- game/unnamed_ai_move_0x4b1410.cpp 0x4b2af0 ai_state::turn_towards, 119 -> 67, **D21302**:
  - `vector3 fr[2]` with fr[0] = right and fr[1] = forward. The array puts the forward above the right
    in the frame, as in the binary (119 -> 101).
  - The turn move lookup is written as `if (side >= 0.0f) m = find_move(9, RIGHT); else m = find_move(9,
    LEFT);`. 8966 merges the two calls and pushes the id in each branch, as the binary does (101 -> 67).
  - Left: frame slots only. The binary has t 0x14, angle 0x18, the origins 0x1c and the
    vector-subtraction temporary in right.z (0x48). Ours has the origins and the temporary at 0x14,
    angle 0x20 and t 0x24.
- same file 0x4b4100 ai_state::mode_traveling_frame, 113 -> 71, **D21303**:
  - The wander point's y is stored as `*(int *)&p.y = 0;`, so 8966 no longer folds `0 + cast_point.y`.
    The binary reloads p.y and adds it, like this form. With the float store, ours copied cast_point.y.
    The binary's result is +0 when cast_point.y is -0, and so is the new form's.
  - The code now matches instruction for instruction, apart from one register: &timer40 is in EBP in ours
    and EBX in the binary (with the float store it was EBX, but y was folded). Most of the 71 is the jump
    table, which build/sum.asm disassembles as instructions.

## Still differs (tried; do not repeat)

- **D21304 (notes only).** float_format_buf 0x542830 (110, unchanged). The binary copies a into rem
  through memory before the `>= 1` test and tests rem from memory. Its x loop has a guard with
  `fld st(1)`. It keeps p in memory in both digit loops (`fdiv [p]`, `fld [p]`), and e and i share a's
  slot (0x1c). Its frame is digit 0x12, p 0x14, rem 0x18, a/e/i 0x1c, the ftol temporary 0x20 and size-1
  0x24.
  - Tried: `x = a` first, `rem = x = a` and `x = rem = a`, `*(int *)&rem = *(int *)&a` (instruction count
    equal, but 130), memcpy, the second loop on e, the x loop on a itself, a rem pointer, `rem[1]`, all 12
    declaration orders of a / rem / p with e, i before or after (no change), p after x, an if + do-while
    guard, a `for (;;)` with break, x through an int copy, e++ first, x from rem, and e = -1 first.
    None is closer.
- **D21305 (notes only).** travel_steer_update 0x4b3c80 (17, unchanged). Tried: one local per product, a
  partial-sum local, `dot +=` in three statements, a copy of d.z, and forward-first operands. 8966 always
  emits y, z, x for the facing dot product (the forms with a local move the frame: 80).
- **D21306 (notes only).** state_title_enter 0x431e20 (14, unchanged). Tried: ternary first, a mode local,
  a file-local inline helper for the ternary (both operand orders), an inline helper for the store, a
  pointer to game_flags, prev read later (29), and a bool mp local (`?:` and `mp + 1`; 36-44). All give
  the same register roles or are worse. With D19202 this is exhausted.
- turn_towards frame (see Closer). Tried after D16304: d computed in two statements (243), a matrix
  pointer for the copies (does not compile past `goto apply`), memcpy copies (224), fr[0] = forward (no
  change), a function-scope origin (214), t / angle as a float[2] in both orders, t[1], angle[1] (no
  change), t block-scoped (109). Late declarations cannot cross the `goto apply`. The ternary in
  find_move tried as `>=`, a local id, short / unsigned char casts, an owner local and a bool local, all
  worse than the if / else.
- mode_traveling_frame EBX / EBP (see Closer). Tried: unsigned / long / int-pointer zero stores, the
  store before x, after z, and right after random_direction (all 71), a volatile int store (75), a
  volatile float store (134), a zero local (144), memset (71), the vector3 constructor / set / constructor
  + add / a temporary vector (127-210), a float pointer store (113), a GameTimer pointer local (with and
  without register: 144), a block around the case (71), and i as unsigned or declared before p.
- hud_hot_buttons_drag (matched): the other forms tried, for the record: only the test or only the
  first call on the global (83), both (64), a dst pointer, dst first, a hover copy, the macro in each
  call (169), dst declared first (14), a split had_dst, and dst in the block (50).
- Crc32::update and fast_inv_sqrt: looked at again. They are the register permutations described in
  D19202 and earlier, and the new devices do not apply. Not retried.

## Behaviour errors / bug candidates

None found. The D21303 form gives the binary's result in the -0 case (see Closer).

## Header changes

None.

## Changed files (in W\src)

- vsdk/gr_font.cpp (gr_font_create: D21300; header comment now exact)
- game/new_hud.cpp (hud_hot_buttons_drag: D21301; header comment now exact)
- game/unnamed_ai_move_0x4b1410.cpp (turn_towards D21302, mode_traveling_frame D21303; closer only)

## --check (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- gr_font: 16 matched there, 17 now, 0 lost
- new_hud: 56 matched there, 57 now, 0 lost
- unnamed_ai_move_0x4b1410: 13 matched there, 13 now, 0 lost

## Worker 7

### m-1021d worker 7 (decisions D21400-D21499)

## New matches

| file | address | name | decision | what did it |
|---|---|---|---|---|
| vsdk/unnamed_poly_split_0x55f110.cpp | 0x55f580 | gr_poly_split_at_tex_line | D21400 | chained zeroing of the edge indices, the nearest vertex stored into e2a / e2b before the corner test, bestd through a float pointer |

**D21400 (vsdk/unnamed_poly_split_0x55f110.cpp; exact). gr_poly_split_at_tex_line 0x55f580 (distance 128 -> match).**
Three changes, same behaviour:
1. `int crossings, e1a, e1b, e2a, e2b; e1a = e1b = e2a = e2b = crossings = 0;` (one chain that ends in crossings).
   This gives the binary's store order (crossings, e2b, e2a, e1b, e1a) and its frame slots (e2b 0x20, e2a 0x24).
   The chain without crossings (`e1a = e1b = e2a = e2b = 0;` after `int crossings = 0;`) gives the right store
   order but e2a / e2b in swapped slots. `e1a = e1b = e2b = e2a = 0` gives the right slots but the wrong store
   order. Separate initialisers, the declaration order, and splitting the chain in two do not give both.
2. The nearest-vertex block writes its result straight into the edge: `e2a = best; e2b = (best + 1) % p->nv;
   if (e2a == e1a || e2a == e1b || e2b == e1a || e2b == e1b) return false;`. The old form tested best / k and
   assigned e2a / e2b after the test. The new form gives the binary's EBP / EBX for e2a / e2b, keeps best in a
   register (no frame slot) and lets p->nv be reloaded in the loop. Using e2a (not best) in the test also fixes
   the base / index order of the first loop's `fld [edx + eax + 4]`. k is no longer set there; it is set again
   before every later use, and e2a / e2b are locals, so the early return behaves the same.
3. `float *bp = &bestd;` with `*bp` for the init, the compare and the store. Without it the distance is 12
   (register choice in the nearest loop).

## Closer, kept

| file | address | name | old -> new |
|---|---|---|---|
| game/unnamed_mp_objects_0x47f860.cpp | 0x4801f0 | mp_handle_new_path | 84 -> 76 |

**D21401 (game/unnamed_mp_objects_0x47f860.cpp; closer only). mp_handle_new_path 0x4801f0:** the set-up is now
in the order `n = data[8]; off = 9; cost = 0.0f; last = NULL;`. cost is then stored as an immediate 0
(`mov [esp+0x14], 0`), as in Sum.exe, instead of reusing EDI (last). Exactly one instruction changes. Same
behaviour. All 6 orders that start with n were tried; o0231 is the best. Left: the vector2 node point is still
stored to the stack before the call (see below).

## Still differs (tried; do not repeat)

**D21402 (notes only). Tried without a match:**

- interface_init_global 0x4116c0 (20): `do switch ... while (0)` (does not compile as written); the 640 / 1024
  stores in the cases inside `do { } while (0)`, through a `float *`, and through a `float bw[1]` (all 212: both
  floats go to memory); a `goto` form (not finished). Left as D16503 / D19104.
- StringPool::Add 0x544c40 (22): `length = new_length` after strcpy (with count++ before or after; 35), the
  length store through `int *` (22, the same code), the test through `const int *` (22, the same code).
  Register choice; exhausted with D19201 / D19202.
- script_action_replace_weapon 0x4dccb0 (32): the record test as one `&&` condition, a goto to a found label,
  `bool save = 1`, `int save` (145), an unused Character copy, `old1 = old2 = NULL` chains (three shapes, 34-36).
  The ESI / EDI swap is unchanged; exhausted.
- script_action_walk_to 0x4dc6e0 (94): an so copy (plain and `register`), np declared last, np declared at its
  assignment, np set through a `navpoint **`, np set after the last-node loop (123), `a` set after its
  declaration. All 94 except the one marked. Sum.exe keeps so in EBX and np in the frame; register choice.
- mp_handle_it_okay 0x4719e0 (15): nothing new tried beyond reading it. Sum.exe threads only the state-3 path,
  which looks like equality-only jump threading in the binary. The 49 earlier forms cover every shape I could
  think of.
- mp_handle_new_path 0x4801f0 (now 76): the vector2 point was tried as memcpy into v.x / v.z with
  `vector2(v.x, v.z)` (144), float locals with `vector2(x, y)` (181), a `vector2 p(v.x, v.z)` / `p(x, y)` local
  (133-135), `*(float *)` reads (181), `float xy[2]` (173), a `const vector2 *` into the packet passed as `*pt`
  in four orders (97-104), `vector2 p = *(const vector2 *)...` (133), `p.x = *(const float *)` (121),
  `p = *(const vector2 *)` (134), `memcpy(&p, data + off, 8)` (133), int stores through `(int *)&p` (123), p at
  function scope (two orders, 76), and the v stores after the layer read (75; not kept, 1 point). The layer read
  as `data[off]; off++` (84, the same) and cost through a pointer (84). Sum.exe pushes the two dwords straight
  from EAX / ECX and has no frame slot for the point; not found.
- ui_choice_popup_show 0x4136a0 (96): both zero stores through pointer locals (one or both, `pw[1]`), chained
  `w = h = 0` in both orders, h before w, `h = w`, an unsigned index with a signed test, `i = 0` before the for,
  a do / while loop, a second loop variable, a `char **` copy of the choices. Every form compiles to the same
  code: 8966 zeroes ESI early for both h and i. Note: this function's distance is inflated. Ours is 1 byte
  shorter, and vc6-compare masks the original's relocations at our offsets, so after the shift every global
  reference counts as a difference. Only about 10 instructions really differ (the ESI / EDI pair).
- ui_choice_popup_draw 0x413c70 (90): the title x as `x += popup_x` then `gr_string(x, ...)`, `popup_x + x`,
  `y += h + gap`, `y += gap; y += h`, an int local for the gap sum. All compile to the same code. Left:
  register / operand choice (EBX / EBP in the loop, `add eax, ecx` in the title).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/vsdk/unnamed_poly_split_0x55f110.cpp (gr_poly_split_at_tex_line, D21400)
- src/game/unnamed_mp_objects_0x47f860.cpp (mp_handle_new_path, D21401)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "unnamed_poly_split_0x55f110|unnamed_mp_objects_0x47f860" --check src/decomp/vc6-matches.txt`:
2 units: 30 match, 0 wrong target, 6 differ, 0 not found, 0 errors; 29 matched there, 30 now, **0 lost**.

## Worker 8

### m-1021d worker W8 (D21500-D21599)

Batch: 9 functions. 2 new matches, 2 closer (kept), 5 unchanged.

## New matches

- game/level_script_worldmap.cpp 0x4fcaf0 ls_worldmap1_pick_encounter_stage9, **D21500**.
- game/pathfinding_visibility.cpp 0x4b77f0 sort_points_clockwise, **D21502**.

**D21500 (game/level_script_worldmap.cpp; m-1021d W8). ls_worldmap1_pick_encounter_stage9 0x4fcaf0 is exact**
(88 -> 0) with a bool `desert = dist < best` local (`if (desert) best = dist;`) and one if / else-if chain: desert
day first (`if (desert && frand() < 0.5f)`), then `else if (!desert)` the whole orenian branch with its own suffix
rolls and a `return`, then `else` desert night, followed by the desert suffix rolls. This gives the binary's block
order: desert day, orenian, desert night, shared desert suffix. The nested if / else (desert { day / night } else
orenian) always puts night right after day. Tried first (no match): four goto forms (a `night:` label after the
orenian block reached by `goto`, with `suffix:` after it, and the same with the inverted coin test, with the orenian
branch first under `if (!(dist < best))`, and with the day branch's `else goto night`); a `night:` label inside the
orenian else-block; orenian first under `if (dist >= best) { ...; return; }`. 8966 moves a goto target block next to
its only predecessor, so the gotos do not move it. Same behaviour: frand is called in the same order on every path.

**D21501 (game/pathfinding_visibility.cpp; closer, kept). determine_point_visibility 0x4b8350** (127 -> 123):
`int num_pending = 0;` declared and initialised before the profile_label, as make_initial_edge_tree's D20603. The
binary stores num_pending before the EH state (`xor ebx, ebx` before `push esi`). The prologue now matches. Left:
the cleanup loop computes the list head twice (`lea ecx, [esi-36]` for the entry test, `lea edi, [esi-36]` for the
loop); the binary computes it once into EDI. Tried (no change): `head != head->next`, a `pathvis_edge **` next
pointer, the head as the for initialiser, a block-scope head copy, a block-scope e, `!(head->next == head)`,
if + do/while, a head pointer loop (`head++`), `for (;;)` with a break, an unsigned index, `e = head->next` before
the loop and at the bottom (folds the head into ESI: worse), next pointer declared first.

**D21502 (game/pathfinding_visibility.cpp; m-1021d W8). sort_points_clockwise 0x4b77f0 is exact** (139 -> 0). Four
changes, all needed:
1. the quadrant heads are four separate locals `pf_vis_node quad0, quad1, quad2, quad3;` instead of an array
   (139 -> 45 with 2): 8966 then keeps quad0 / quad2 / quad3.next_open in EDX / EBX / ESI across the layer loop
   (the four different zero registers of the initialisation and the reloads before `inc ebp` in the binary).
   Declaration order does not matter (4 orders give the same code);
2. each quadrant branch stores `n->f = ...;` before it picks the head (139 -> 125): the binary's branches each
   end in `lea esi; [fchs]; fstp; jmp`, not a shared fchs / fstp tail;
3. no `q`: the branches assign the head straight to `prev` and the for loop has no initialiser (45 -> 7): q and
   prev in one variable puts the head in ESI and the mask 0xfffd in EDX;
4. the walk steps by `prev = prev->next_open` instead of `prev = p` (7 -> 0): the binary reloads the next link
   through ESI after `mov esi, edx`. (`for (; prev->next_open; prev = prev->next_open) { p = prev->next_open; ...`
   also matches; a `while` with `prev = p` at the bottom does not (7); `for (p = prev->next_open; p; p =
   prev->next_open)` 65; `while (prev->next_open) { p = ...; ...; prev = p; }` 35.)
Also tried on the way (no gain): chained NULL stores, a loop over the four heads, interleaved next_open / f stores,
pointer locals for the four heads, initialised n / q / prev / p / tail / layer / i (dead, dropped), `int layer = 0`
before the label with `for (; layer < ...)` (moves EBP's zero before read_tsc but the same distance, not kept), an
`f` float local, `q = quad + k`, `p = q->next_open; if (p)` for the first clear.

**D21503 (vsdk/window.cpp; closer, kept). ui_window::handle_arrow_key 0x52abf0** (101 -> 99): the left-case distance
as `d = w - g->w - g->x; d += x;` gives the binary's operand order (w loaded first). Still differs: the left case
computes into ECX and adds x with `lea eax, [ecx+edx]`, and its get_size argument registers (EDX / EAX in the
binary, EAX / ECX in ours). The listed distance includes the jump table disassembled as code (about 80).

**D21504 (notes only).** The functions below.

## Closer, kept

- game/pathfinding_visibility.cpp 0x4b8350 determine_point_visibility: 127 -> 123 (D21501).
- vsdk/window.cpp 0x52abf0 ui_window::handle_arrow_key: 101 -> 99 (D21503).

## Still differs (tried; do not repeat)

- game/pathfinding_visibility.cpp 0x4b7d30 pathvis_add_edge_node (22): the binary's `lea edx, [eax+0x20]` /
  `lea ecx, [eax+0x24]` with ESI / EDI saved. Tried in this round (all fold to `[eax+32]` / `[eax+36]`, identical
  code): `register` pointer locals in a block, pointer copies of the pointer locals, a `pathvis_edge **l[2]` array,
  `pn = pp + 1`, a copy of e with value locals, and `#pragma optimize` off for g, ga, p, y, w, a, t around the
  original and the pointer-local forms (g / ga: unoptimised frame; t: a different prologue; the rest no change).
  Exhausted with earlier rounds' forms.
- game/pathfinding_visibility.cpp 0x4b8350 determine_point_visibility (123): see D21501 for the head lea.
- vsdk/window.cpp 0x52b100 ui_window::init (81): `this` in ESI and both registers pushed at entry in the binary.
  Tried: the non-modal part first under `if (!(flags & MODAL)) { ...; return; }`, a sentinel pointer local for the
  modal insert, a local for `ui_window_sentinel.prev`, a goto to one end label (win / p at function scope), the
  whole body in `do { ... break; } while (0)`, if / else for both parts. All the same code.
- vsdk/window.cpp 0x52abf0 ui_window::handle_arrow_key (99): left case, see D21503; tried also `int t = w - (g->x +
  g->w); d = t + x;`, `d = x + d`, one-expression `(w - g->w - g->x) + x` (101), gx / gw locals, a `const int *`
  to w, four separate `-=` / `+=` statements (all 99). Up case (`y - g->h - g->y`, then `d += h`, separate
  statements, an int t): 103, not kept (register naming only).
- vsdk/effect_mem.cpp 0x51ca60 effects_parse_arr_line (16): only the inner loop's entry-test exit target. Earlier
  rounds' list is long (D12009, D18606); nothing new tried beyond reading it.
- vsdk/unnamed_checkbox_0x553ba0.cpp 0x553f00 ui_checkbox::draw (22): scheduling of the gr_line arguments. Tried on
  the first line pair: int locals ex / ey for the far corner (worse), float locals fx / fy, a float f[2] array, a
  gr::Mode local (worse), a `const int *` to box_w, `box_w + sx` order. None moves the lea / mov ebp or fild /
  mov ecx order.
- game/new_credits.cpp 0x408620 credits_parse_table (131): the binary keeps EBX as the zero register through the
  whole read loop (EH state, push 0 arguments, i = 0, count > 0, re-zeroed with `xor ebx, ebx` after the inner
  loop) and has out in EBP, start in ESI, end in EBX / EDI; ours coalesces out = 0 into EBX. Tried: a pointer to
  out for the zero store, `int out = 0` before tf, out = 0 before credits_line_count = 0, `out =
  credits_line_count = 0`, start at function scope, start / end / i / count at function scope. No change (one
  worse).

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/decomp/game/level_script_worldmap.cpp (D21500)
- src/decomp/game/pathfinding_visibility.cpp (D21501, D21502)
- src/decomp/vsdk/window.cpp (D21503)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "level_script_worldmap|pathfinding_visibility|vsdk/window\.cpp" --check src/decomp/vc6-matches.txt`:
3 units: 65 match, 0 wrong target, 9 differ, 0 not found, 0 errors; 63 matched there, 65 now, 0 lost.

## Worker 9

### W9 notes (m-1021d, decisions D21600-D21699)

Batch: gr_project_vertex, gr_get_frustum_planes, gr_setup_3d (vsdk/unnamed_gr_3d_0x50d9b0.cpp);
mp_read_spell_targets, mp_handle_release_spell (game/multispell.cpp); gr_opengl_bitmap (vsdk/gr_opengl.cpp);
Character::say (game/living_entity.cpp); fx_missile_hit (game/unnamed_fx_0x489c40.cpp); snd_stream_fill_pcm
(vsdk/unnamed_sndstream_0x566560.cpp).

## New matches

- vsdk/unnamed_sndstream_0x566560.cpp 0x566820 snd_stream_fill_pcm (134 -> match), **D21600**.
- game/multispell.cpp 0x4b04a0 mp_handle_release_spell (131 -> match), **D21603** (needs both header changes below).

**D21600 (vsdk/unnamed_sndstream_0x566560.cpp). snd_stream_fill_pcm 0x566820: every local declared without an
initialiser and assigned in Sum.exe's order: `rc = 0; s = &snd_streams[stream]; *written = 0; b = ...; pdsb = ...;
got = 0; p1 = NULL; p2 = NULL; n1 = 0; n2 = 0;`.** Declaration order alone never moves anything (as D20203's
notes said). Plain assignments instead of initialisers put n2 in written's argument slot (frame 0x14 -> 0x10, got
already in stream's slot) and give the binary's store order (rc, *written, got, p1, p2, n1, n2): 134 -> 118. The
rest was the zero register (ECX, xor hoisted above the lea of s) and the interleaving with the s / b / pdsb address
arithmetic: rc = 0 first, then s, then `*written = 0` before b and pdsb gives the exact code. Orders tried on the
way (all with the late assignments): s / rc / written / b / pdsb (26), rc / written / s / b / pdsb (20), s / b /
rc / written / pdsb (26), rc / written / zeros / s / b / pdsb (60), s / rc / written / b / zeros / pdsb (26),
s / b / pdsb / rc / written (118), chained zero stores (134), `*written = rc` (118). Same behaviour.
snd_stream_fill_acm / _fill_silence (same unit, not in my batch, untouched) differ in block placement and the
fill byte's slot; this device did not look relevant to fill_acm's remaining difference and was not tried there.

**D21601 (game/living_entity.cpp; closer, kept). Character::say 0x461dc0 (25 -> 15): `total += lens[i] + 1;`
between the strcat and the strncat.** The binary pushes lens[i] before it loads offsets[i]; with the total
updated between the two calls 8966 also loads lens[i] first. Left: the length test computes into ECX (binary EAX)
and the binary reloads lens[i] for the total after strncat (ours hoists the `lea ebp, [eax+ebp+1]` before the
call). Tried: total before strcat (42), `need` local assigned to total (47), do / while with a top test (92), an
unsigned index with `(int)k < n` (25), `total = total + 1 + lens[i]` (25), `total++` between and `+= lens[i]`
after (25), `++` first and `+= lens[i]` after (25), `+= lens[i]` between and `++` after (16), `+= lens[i]` first
and `++` after (22), `++; += lens[i];` both between (15), `+= 1 + lens[i]` between (15), `total = lens[i] + total
+ 1` between (15). Same behaviour (strncat does not read total).

**D21602 (game/multispell.cpp; closer, kept). mp_read_spell_targets 0x4affb0 (82 -> 73): `int *t =
p->target;` and every target access through t.** The four pops now come late in each case as in Sum.exe. Left: the
binary keeps req itself in EDI, loaded after all four pushes, and turns it into the loop pointer (`add edi, 0x44`
in case 4, `lea ebx, [edi+0x2c]` in case 5); ours loads req into EAX first and keeps t in EDI. The ~20 lines after
the jump table are the linear disassembly reading the table (not a real difference). Tried: no p local (82, also
with the 4th argument), `int mode = p->mode` (82), a `q = &p->target[6]` pointer walk in case 4 (78), a req copy
(82), gotos to one tail (82), data / offset locals (82), t declared late (73), `switch (req->params.mode)` with t
(73), t with the case 4 pointer (92), `register` on p / i / a req copy (82), i first (82), net in the loop blocks
(82).

**D21603 (game/multispell.cpp + two headers). mp_handle_release_spell 0x4b04a0: the request is built with a
seven-field constructor that stores the caster first; the spell and the source type are read into int locals; and
mp_read_spell_targets takes a 4th argument, the spell, which Sum.exe pushes and the callee never reads.** Evidence
for the argument: after the two calls the binary pops 0x20 bytes (8 dwords: 4 for mp_read_spell_target_point, 4
for mp_read_spell_targets, 0x4b0602); the `push ebx` at 0x4b05ac (between the two calls, ebx = the spell read at
0x4b04fc, also pushed for mp_read_spell_target_point at 0x4b059b) is that 4th argument, not a register save (EBX's
save is the push at 0x4b04fb, popped at 0x4b0607). With the argument (sizes equal, 131 -> 138), the spell in a local passed to
both calls (36), the source type through a local (4), and `spell_cast_request req(NULL, -1)` instead of the
default constructor followed by `req.caster = NULL; req.spell = -1;` the function matches. The binary's seven
constructor stores are in field order (+0 caster, +4 spell, +8 target ...); `req(-1)` (spell first) gives 2, as do
`req(-1)` with only the caster or only the spell assigned after it; a separate caster / spell order with the
default constructor stays at 4. Bitfield store orders (5 permutations) and a params pointer for them changed
nothing (36 / 100). Same behaviour: the same seven values; the extra argument is unused.

## Closer, kept

- game/living_entity.cpp 0x461dc0 Character::say: 25 -> 15 (D21601).
- game/multispell.cpp 0x4affb0 mp_read_spell_targets: 82 -> 73 (D21602; the header's 4th argument does not change
  it).

## Still differs (tried; do not repeat)

**D21604 (notes only).**
- gr_project_vertex 0x50ea50 (24): the binary reloads v->flags for the final return. New forms, all 24: the x
  clamp through `float *px = &x` (load and store, store only), px and py both, `float x[1]`. Not exact: the final
  return through a volatile read (36), the volatile read between the sx and sy stores into a local (32).
  Exhausted with D18802's list.
- gr_get_frustum_planes 0x50ed60 (34): count EAX / ECX swap. New forms, all 34: an early `if (!flags) return;`,
  `register` n, `register` i, a bool for the flags test, a plane reference for the destination, a const plane
  pointer for the source, the second loop bounded by the global, a do / while second loop. Register choice.
- gr_setup_3d 0x50dec0 (126; only the second Glide test is threaded, the rest of the listing is identical): new
  forms: `for (;;) { .. break; }` around the first test, a do / while (0), a pointer store for the depth scale, a
  mode local, a bool local for the first test (all 126); a switch for the first test (115: the second test reuses
  the flags, `jne` without the `cmp`), a switch for the second (117), switches for both (117), switch + `!=` if /
  else (117). Not exact.
- gr_opengl_bitmap 0x54fa30 (16): first corner's store schedule. New forms: an inline `gl_set_pt` helper for
  pts[0] (240), y stored before x (20), x / y copied into locals (152), a block with a vector3 pointer (16), the g
  store through a ubyte pointer (16), a colours-row pointer for all four (16), the arrays declared colors / tc /
  pts (16), z stored first (16), colours first (24), tc[0][0] between g and b with tc[0][1] after a (22),
  `pts[0] = vector3(..)` (260), g stored before r (16), `float xy0[2]` (52), `float xs[2]` (48), `float uv0[2]`
  (72). Exhausted.
- fx_missile_hit 0x48bac0 (89): vi / c in EBP / EBX swapped (binary: vi in EBX, c in EBP, EBP / ESI pushed only
  around the spell branch). Tried, all 89: vi / n / c declared in the spell block, c declared at its two uses
  apart, vi declared first, c first, `register` on vi / c / done; a separate weapon character local (89); done
  assigned only before the weapon part (111). Note: Sum.exe multiplies the timeout by 0.75, 1000 and 0.05 in
  three fmuls (0x48bc5d-0x48bc6e); 8966 folds our `d * 0.75f * 1000.0f * 0.05f` into one fmul by 37.5, and
  `(int)(0.05f * (1000.0f * (d * 0.75f)))` gives the three fmuls in the binary's order, but the score rises (95),
  so it was not kept; the same form inside an inline helper still folds (89). Worth combining with a fix of the
  register swap if one is found.
- snd_stream_fill_acm / fill_silence: not in my batch, not tried.

## Behaviour errors / bug candidates

None found. (mp_read_spell_targets' unused 4th argument is a signature fix, not a behaviour change.)

## Header changes

- include/engine/spells/spells.h: a third inline constructor `spell_cast_request(Character *c, int s)` that
  stores caster, spell, then the default five fields (D21603). Needed for mp_handle_release_spell's store order;
  the existing `spell_cast_request(int)` stores the spell first (combat's requests, D5506) and cannot change. No
  layout change; nothing else uses the new constructor.
- include/summoner/multi/multispell.h: `mp_read_spell_targets(.., int *offset, int spell)` (4th argument, unused;
  D21603). The only caller is mp_handle_release_spell; the mangled name changes.

## Changed files

- src/vsdk/unnamed_sndstream_0x566560.cpp (snd_stream_fill_pcm, D21600)
- src/game/living_entity.cpp (Character::say, D21601)
- src/game/multispell.cpp (mp_read_spell_targets D21602 + 4th argument; mp_handle_release_spell D21603)
- src/include/engine/spells/spells.h (D21603)
- src/include/summoner/multi/multispell.h (D21603)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only
"unnamed_sndstream_0x566560|living_entity|multispell|unnamed_gr_3d_0x50d9b0|gr_opengl|unnamed_fx_0x489c40"
--check src/decomp/vc6-matches.txt`: 6 units: 307 match, 23 differ, 0 errors; 305 matched there, 307 now,
**0 lost**.

Because of the two header changes, also the whole tree: `VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src
--check src/decomp/vc6-matches.txt`: 259 units: 5522 match, 1 wrong target, 402 differ, 1 not found, 0 errors;
5520 matched there, 5522 now, **0 lost**.

## Worker 10

### m-1021d worker 10 (D21700-D21799)

Batch: 9 functions (bmpman x3, listbox x2, vif, netsession, pxochat, khosani lab). Result: 3 new matches, 2 closer,
4 unchanged. Helper scripts in this folder (try.mjs: variants with the distance; cod.mjs / codv.mjs / sbs.sh: our
listing next to the original's, compiled into out/ here, no temp folders). Nothing in them is for the merge.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_listbox_0x52ca60.cpp | 0x52da40 | ui_listbox::add_entry | D21700 | `font` stored before `indent`; the trim loop as `do { if (h > max) { ... } else break; } while (first != &entries);`; the sorted insert inside the search loop (`if (stricmp < 0) { if (p) { insert; goto inserted; } break; }`) |
| vsdk/bmpman.cpp | 0x5038f0 | bm_lock_vbm | D21703 | the reduced seek size through `int s = e->height; s *= e->width;`; the frame loop through `bitmap_entry *frame = hdr;` stepped with `frame++` at the top of the body |
| game/new_pxochat.cpp | 0x4223d0 | pxo_parse_chat_banners_cfg | D21704 | `count = 0` after the existence test (not at the declaration): count in EBX and the -1 in EBP as in the binary; `int r = rand() % num_found; int pick = found[r];` |

**D21700 (vsdk/unnamed_listbox_0x52ca60.cpp). ui_listbox::add_entry 0x52da40 matches.** Three changes. (1) `e->font = font;`
before `e->indent = indent;` (8966 stores the second-loaded field first). (2) The binary does not rotate the trim loop: it
stores content_height, reloads it and max_content_h at the top and tests `first != &entries` at the bottom. A while, a
`do { if (<=) break; ... } while (first != &entries)` and `for (;;)` all get rotated; `do { if (content_height >
max_content_h) { body } else break; } while (first != &entries);` (also a done or more flag) keeps it at the top.
(3) The sorted search inserts inside the loop: `if (_stricmp(e->text, p->text) < 0) { if (p) { insert; goto inserted; }
break; }`, so a found entry goes straight to the NULL test and a normal exit straight to the append (the binary has no
`p != &entries` test after the loop). Same behaviour (inside the loop p != &entries already holds).

**D21701 (vsdk/unnamed_listbox_0x52ca60.cpp). ui_listbox::snap_scroll_bottom 0x52ebe0: 143 -> 114 (kept).** The binary's
block shape: `if (e == first) { if (!(flags & BY_ENTRY)) { int fh = gr_get_font_height(e->font); return e->y + fh -
get_view_height(); } } else { if (BY_ENTRY) { e = e->prev; return ...; } fh ...; line loop; if (i == 0) e = e->prev; else
return y - get_view_height(); } return e->height + e->y - get_view_height();` (the first entry's by-entry return shares
the final tail, the font height is read before the view height), and the line loop as `do { if (line_bottom > bottom)
break; y = line_bottom; i++; line_bottom += line_h; } while (i < e->num_lines);` inside the `num_lines > 0` test.
Same values. Left: register choice (binary: this in EDI, &entries in EBX, y in ESI at the head, bottom in memory;
ours: this in EBP and spilled, &entries in EDI, bottom in EBX).

**D21702 (vsdk/bmpman.cpp). bm_lock_tga 0x503710: 90 -> 12 (kept).** (1) if / else with one `return true` after both
branches instead of the early `return true`: the binary pushes EBX / ESI / EDI before the first test (no shrink-wrap).
(2) `bpp = bm_bytes_per_pixel(e->format); e->data = new ubyte[bpp * e->num_pixels];` (`mov edx, [num_pixels]; imul edx,
eax`, as D18703 for bm_lock_pcx); bpp is still re-read later as in the binary. (3) `w = e->width;` before `h = e->height;`
(keeps w in EBX and gives the binary's `h * w` operand order and the early `loaded = true` store). (4) The fourth
tga_read_bitmap argument as `*(int *)&loaded`: the binary pushes the dword of the bool's byte slot (no `and ecx, 0xff`).
The argument is unused by tga_read_bitmap (0x555450 never reads it), so the upper bytes do not matter; a test with a
`bool` prototype gives exactly the same code (distance 12), so this needs no header change. Left: the frame slots of h
and loaded are swapped (binary: i 0x10, loaded 0x14, h 0x18; ours h before loaded).

**D21703 (vsdk/bmpman.cpp). bm_lock_vbm 0x5038f0 matches.** The binary keeps `&hdr->num_pixels` in EBP for the size
product and starts its frame walk from it (`mov eax, ebp` / `add eax, 0x48` at the loop top), re-reading hdr->data in
each pass: that is `bitmap_entry *frame = hdr; for (...) { frame++; frame->data = (ubyte *)hdr->data + ...; }` (a
`hdr + 1 + i` index, a post-incremented pointer or a hoisted base local do not give it). The reduced seek is `int s =
e->height; s *= e->width; f.seek(s * bpp * 4, 1);` (the binary's `imul eax, edx` with the height in EAX). Same values.

**D21704 (game/new_pxochat.cpp). pxo_parse_chat_banners_cfg 0x4223d0 matches.** The whole-function EBX / EBP swap (count
and the -1 constant) goes away when count is set after the `file_exists_on_disk` early return (`int count;` ... `count
= 0;` before tf.Open; also right before the parse loop) instead of at its declaration. The last pair (`mov eax, found[...]`
where 8966 used EDX) is the pick index through an int local: `int r = rand() % num_found; int pick = found[r];`.

## Closer, kept

| file | address | name | old -> new |
|---|---|---|---|
| vsdk/bmpman.cpp | 0x503710 | bm_lock_tga | 90 -> 12 (D21702) |
| vsdk/unnamed_listbox_0x52ca60.cpp | 0x52ebe0 | ui_listbox::snap_scroll_bottom | 143 -> 114 (D21701) |

## Still differs (tried; do not repeat)

**D21705 (notes only). Tried, still differing.**
- vif_record_get_bounds 0x533aa0 (16): the fild temporary of `1.0f / count` in rec's dead argument slot (binary:
  radius's, shared with farthest). New forms, all 16: farthest as `float[1]`, l as `float[1]`, the reciprocal as
  `float inv[1]`, `float *fp = &farthest; *fp = 0.0f;`, `float *rp = radius; *rp += farthest;`, an int count then a
  float inv, a float count, l declared at block scope before c / first, a block with `float s = ...; float a = s;`, a
  `const vfx_desc21c *r2 = rec` for the divide, `int n; int *np = &n;` for the count. Exhausted with D19103.
- mp_player_alloc 0x441610 (25): not retried (D18802 / D7006 / D12803 / D14507 exhausted every list_remove form).
- bm_lock_pcx 0x503aa0 (33): 8966 pre-shifts 9 * index (`shl esi, 3`) even with only two uses (diagnostic: the palette
  part or the read removed still shifts); bm_read_pcx_header_of (two uses of offset 0 only) does not. Not solved; the
  earlier forms (D18703 list) not repeated.
- bm_lock_tga 0x503710 (12, after D21702): frame slots of h / loaded. Tried: all 120 declaration orders of i, w, h, bpp,
  loaded (one per line), loaded as char / ubyte, h and / or loaded declared in the else block, `int *hp = &h` for the
  product, `int *lp = (int *)&loaded`, `*(&loaded) = true`, the `loaded = false` store after bpp / after h / before the
  loop / in the for-init (14 to 18), `h /= 2` first (16, but then h takes EBX), the products as `w * h`, `(h * w) * bpp`,
  `bpp * h * w`, `w * (h * bpp)`, a size local, a bool prototype for tga_read_bitmap (diagnostic, 12). The slots do not
  move with any of these.
- ui_listbox::snap_scroll_bottom 0x52ebe0 (114, after D21701): register choice. Tried on the new shape: `int last = y`
  (in several places, as the early return too, `register`), a `head = &entries` pointer, `const int *pb = &bottom`,
  `int bottom[1]`, bottom / first declared at the top, a `ui_listbox *lb = this` copy, `y + get_view_height()`, a view
  height local, the find loop as a while or do / while (122), the line loop as the old while (same as the do / while).
- ls_khosanilab_puzzle_trace_beam 0x4f7b50 (137): the cross-jumped tails: the binary keeps case a's copy of the
  check / turn_dirs / from = to / cur = next tail (case c and case b jump into it), 8966 keeps the last copy (case c's).
  Tried: the loop as `for (;;) { if (!cond) break; ... }` (137), a `goto top` loop (137), labels in case a's tail with c
  and b jumping in (160), only b jumping to a's `cur = next` (133, not kept: a goto for 4). Not solved.

## Behaviour errors / bug candidates

None found. (bm_lock_tga's fourth argument: the binary passes the bool's byte slot as a dword, upper bytes
uninitialised; harmless because tga_read_bitmap ignores the argument.)

## Header changes

None.

## Changed files

- src/vsdk/unnamed_listbox_0x52ca60.cpp (add_entry, snap_scroll_bottom)
- src/vsdk/bmpman.cpp (bm_lock_tga, bm_lock_vbm)
- src/game/new_pxochat.cpp (pxo_parse_chat_banners_cfg)

## --check results (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- vsdk/bmpman: 51 match (50 there, 51 now), 0 lost
- vsdk/unnamed_listbox_0x52ca60: 63 match (62 there, 63 now), 0 lost
- game/new_pxochat: 56 match (55 there, 56 now), 0 lost

## Worker 11

### m-1021d worker 11 (D21800-D21899)

Batch: 9 functions. 3 new matches, 2 closer (kept), 4 unchanged.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/unnamed_cutscene_camera_0x493e50.cpp | 0x493fd0 | cutscene_camera_sequence::read_track | D21800 | each key loop as a guarded do / while with a byte-offset local |
| game/unnamed_ai_groups_0x492b00.cpp | 0x493570 | ai_group_think_style2 | D21802 | `if (!target) target = first;` and one call |
| game/unnamed_ai_groups_0x492b00.cpp | 0x493290 | ai_group_think_style1 | D21803 | a redundant third `best == NULL` in the first fallback test |

**D21800 (game/unnamed_cutscene_camera_0x493e50.cpp). cutscene_camera_sequence::read_track 0x493fd0 is exact**
with both key loops written as `k = 0; if (k < track.num_X_keys) { off = 0; do { ... k++; off +=
sizeof(key); } while (k < track.num_X_keys); }` and each key addressed as `((anim_pos_key *)((char
*)track.pos_keys + off))->field` (rot loop the same with anim_rot_key). The earlier rounds' problem was the
SIB order: 8966's own strength-reduced induction variable gives `[iv + keys]`, the binary has `[keys + iv]`;
an explicit byte offset added to the reloaded key pointer gives the binary's order. A plain `for` with the
offset fixes the order but puts `off = 0` before the loop-entry test (the binary zeroes the offset after it in
the rot loop, and in the pos loop compares with the zero register); the guarded do / while places it after
the test, as 8966 places its own induction start. `for (k = 0, off = 0; ...)`, `off` in the loop body, the
offset as `char *`, `int off = k * 40` per iteration, the do / while with plain indexing, the inline
pos_key_* accessors, an `anim_track *` local: all differ. Same behaviour. DEVICE note: for a `[reg+reg]`
base / index order that no index type changes (hot_button_set's stores were listed with read_track's keys),
an explicit byte-offset local in a guarded do / while is worth trying.

**D21802 (game/unnamed_ai_groups_0x492b00.cpp). ai_group_think_style2 0x493570 is exact** with the fallback
written `if (!target) target = first; if (target) { ai_group_order_attack(c, target, g, false); return; }`
(was two ifs with a call each). With one call site 8966 no longer keeps g in EDI, so i / the enemy pointer /
first get EDI / EBX / EBP and g stays in its argument slot, as in the binary (the binary's two call blocks
are 8966's own tail split of the one call). On the way: a ternary call argument `target ? target : first`
(49), `if (target) first = target;` (53), a `Character *t = target ? target : first` local (66), else-if
chain (80), `t` local per test (83), goto / bool / order forms (83). Same behaviour (first is not read after).

**D21803 (game/unnamed_ai_groups_0x492b00.cpp). ai_group_think_style1 0x493290 is exact** with the first
fallback test written `if (best == NULL || character_is_helpless(best) || best == NULL) best = engaged;`
(the third test is always false there). The binary's flow after a kept best is `test edi, edi; jne <second
helpless call>` with a dead fall-through into the `best = engaged` block; the redundant test reproduces that
edge, and with it 8966 colours i / the enemy pointer / best (EBX / EBP / EDI) as the binary. Steps: the
second test written in both branches of an if / else (118 -> 60: registers right, flow not), a goto into the
then branch's second test (15), a `goto done` form (13); the same edge also from `if (best != NULL &&
!character_is_helpless(best) && best != NULL) goto check;` (match, not kept: a label inside an if). Not
changing anything: unsigned index, declaration orders, a pointer to best, a bool local, nested second test,
`!= 0` comparisons. Same behaviour (character_is_helpless is called the same times). DEVICE note: a redundant
repeated test can restore an edge the binary keeps where 8966 jump-threads it away.

## Closer, kept

- game/combat.cpp 0x464de0 living_combat::get_protection: 82 -> 53, **D21801**.
- game/unnamed_tentacle_beast_0x4c6480.cpp 0x4c6520 ai_state::tentacle_beast_ai_select_action: 99 -> 2,
  **D21804**.

**D21801 (game/combat.cpp; closer only). get_protection 0x464de0: the bless tail written in both branches**
(`if (no armour) { prot = ...; if (bless) return ...; return prot; } { armour terms; prot = ...; if (bless)
return ...; return prot; }`). 8966 then places the armour block after the no-armour return, as the binary.
Left: the binary loads e6 into EDX with the other four slots and tests it there (ours reloads it at the test),
and the binary's armour block jumps back into the no-armour branch's tail (cross-jumped), where 8966 keeps two
tails and schedules `push 4; mov ecx, esi` into the fild of each. Tried: if / else or goto with a shared tail
(82: 8966 lays the join after the armour block for every form: goto into the if, goto armour, for(;;) /
break, bool locals for the test, `!(a || ...)`, an int flag), ternary (129), an inline helper for the tail
(not found / 82), e6 in p6 (136), `c->equipment[6]` in the test, `!e6 && !e6` (136), a redundant
`c->equipment[6] == NULL` (53), prot2 local (53), an e6 pointer local, combat.cpp under /Ox, /O2 /Ow, /O1,
/Ob0, /Ob2 or /Og... (nothing changes for get_protection, ai_choose_special or chain_check_window).

**D21804 (game/unnamed_tentacle_beast_0x4c6480.cpp; closer only). tentacle_beast_ai_select_action 0x4c6520:
the party-entity test as `c->type != 7 || (c->flags & CF_HIDDEN) || (c->flags & CF_DEAD)`.** 8966 folds the
two bit tests into one `test byte ptr [ecx+0x320], 0x88` immediate as in the binary; the combined mask is
hoisted into BL as a loop invariant, which moves 7 into EBP and spills `this` (99). The flags test before the
type test gives the immediate too but the tests in the wrong order (6). Left (distance 2): the first loop's
`test bl, cl` (ours `test cl, bl`) for `!(c->flags & CF_DEAD)`. Not changing it: `(CF_DEAD & c->flags)`, `== 0`,
an unsigned / ubyte / int / pointer local for the flags, a mask local, a second Character local, `continue`
form, store order, a do / while loop, `(CF_DEAD | 0)`. Same behaviour.

## Still differs (tried; do not repeat)

**D21805 (notes only). Tried, still differing.**
- living_combat::ai_choose_special 0x463d00 (36): the push block after the epilogue. New forms: the store
  written in every branch (91), in the push branch only (86), a bool local for the push flag, a pointer-local
  store, `goto add`, `goto push` to a block after the store, `if (1)` around the body, `i == SKILL_PUSH &&
  flags` as one else-if, `i != KICK && i != TRIP && i == PUSH`, the push test in a plain else (all 36), and
  the unit's flags as above. Not reproduced.
- living_combat::chain_check_window 0x466ae0 (89): the binary keeps the first window branch's learn tail and
  cross-jumps nothing else (every `return 100.0f` / `return 0.0f` is its own block); 8966 keeps the second
  copy. Swapping the two branches (`> 0.0f` first: 87, lays them out swapped), the second branch without else
  (89), swapped without else (87), `goto learn` into the first branch (142: the label makes 8966 merge every
  `return 100.0f` too). Not reproduced.
- mp_reset_net_stats 0x4419d0 (17): new forms `NULL` / `(unsigned char)0` as the memset value, memset of the
  timer (21), memset of a counter, `(int)0.0f` / `(int)0.0` / `sizeof(int) - sizeof(int)` for the counters (a
  `'\0'` memset value did not compile here, a quoting problem of mine, not retried). The second zero register is
  not reproduced.
- cutscene_script_load 0x4ed0f0 (142): `_stricmp(...) == 0` with the success part inside, `!_stricmp` with
  else, an int result local, a bool `ok` (194), an extra `|| name == NULL` test, the body in `do { ... break;
  } while (0)` and `for (;;)` with breaks, an empty else: all 142 (the failure tail stays at the end).

## Behaviour errors / bug candidates

None found. All kept changes keep behaviour.

## Header changes

None.

## Changed files

- src/game/unnamed_cutscene_camera_0x493e50.cpp (read_track, D21800)
- src/game/unnamed_ai_groups_0x492b00.cpp (ai_group_think_style2 D21802, ai_group_think_style1 D21803)
- src/game/combat.cpp (get_protection, D21801)
- src/game/unnamed_tentacle_beast_0x4c6480.cpp (tentacle_beast_ai_select_action, D21804)

All four compile with FIXED_BUILD (tools/fixed-compile.mjs; only the existing warnings).

## --check result (VC6_STRICT=1, against src/decomp/vc6-matches.txt)

`--only "unnamed_cutscene_camera_0x493e50|game/combat|unnamed_ai_groups_0x492b00|unnamed_tentacle_beast_0x4c6480"`:
4 units: 102 match, 0 wrong target, 16 differ, 0 not found, 0 errors; 99 matched there, 102 now, **0 lost**.

## Worker 12

### W12 notes (m-1021d, decisions D21900-D21999)

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| game/level_script_ionaext.cpp | 0x4f7270 | ls_ionaext_frame | D21900 | the negated bool stored in a local and tested through a pointer to it |
| vsdk/unnamed_vfile_0x5128b0.cpp | 0x5143e0 | File::set_cipher_key | D21901 | bit 0 cleared through a local one-bit bit-field, and the flag set before the key stores |
| vsdk/unnamed_ds_0x55bf90.cpp | 0x55d110 | ds_play | D21902 | the volume and group stores indexed as `ds_channels[ch]` |
| game/unnamed_evil_urath_0x4c4170.cpp | 0x4c4db0 | ai_state::evil_urath_ai_select_action | D21904 | the first no-target return as a goto to the later `return 0`, inside the hidden test's block after a separate NULL test |

**D21900 (game/level_script_ionaext.cpp). ls_ionaext_frame 0x4f7270 is exact** with
`bool not_started = !lsf_iona_ext_played_intro(); bool *ns = &not_started; if (*ns) ...`. The pointer keeps 8966
from folding the negation into the branch, which gives the binary's `test al, al / sete al / test al, al / je`.
Behaviour is unchanged. These forms did not work: `& 1`, `& true`, `== 1`, a switch on the bool or on an int,
`&& true`, `|| false`, `* 1`, `!= 0`, `> 0`, an unsigned char local, a one-bool array, `!(x != false)`.
(The earlier rounds' forms were not repeated.)

**D21901 (vsdk/unnamed_vfile_0x5128b0.cpp). File::set_cipher_key 0x5143e0 is exact** with two changes that
only match together:
- The zero branch is `((cipher_bits *)&file_slots[i].flags)->encrypted = 0;`, where `cipher_bits` is a
  function-local `struct { unsigned encrypted : 1; }`. That gives the binary's load / and / store, as D14606 found.
- In the key branch, `file_slots[i].flags |= 1;` comes before `k[0] = key; k[2] = 0; k[1] = 0;`. The key then
  stays in EDX and the zeros are stored as immediates, with no zero register.

Behaviour is unchanged: bit 0 is cleared or set and the same key array is passed. These forms got close but did
not match:
- a bool local through a pointer (it gives a `sete`);
- `memset(&k[1], 0, 8)`, which fixed the zero branch's head but not the key branch;
- a switch on key;
- the bit-field with `= { key }`, with a bool local, with a `kp` pointer or with `int *` stores;
- k[0] stored before the flag (with plain stores, chained stores or memset).

**D21902 (vsdk/unnamed_ds_0x55bf90.cpp). ds_play 0x55d110 is exact** with `ds_channels[ch].volume = volume;
ds_channels[ch].group = b->group;` indexed, and every other access through `c`. 8966 then keeps ESI as the
channel offset from ds_channels for every access, as the binary does. Behaviour is unchanged. These did not
work:
- c as `(char *)ds_channels + off`, `ds_channels + ch`, a copy, a reference or an unsigned index;
- all accesses indexed, or indexed up to Play;
- c computed before b;
- the indexed form for the 3D test, get_3d_interface, flags, buffer or signature (only the volume / group pair
  works).

**D21904 (game/unnamed_evil_urath_0x4c4170.cpp). evil_urath_ai_select_action 0x4c4db0 is exact.** Two changes:
- `if (evil_urath_target_handle == -1) goto no_target;`
- `if (t == NULL) goto no_target; if (t->flags & CF_HIDDEN) { no_target: return 0; }`

The binary shares the later `return 0`, which is laid out inline after the hidden test. This is the open point of
D14306. Behaviour is unchanged. The goto with the combined `||` test gives 54; an `ok:` label, split tests with
`ok:`, if / else and nested forms also give 54; bool / split forms without the goto give 129-145.

## Closer, kept

| file | address | name | old -> new | D |
|---|---|---|---|---|
| vsdk/unnamed_ds_0x55bf90.cpp | 0x55d400 | ds_stream_open | 40 -> 30 | D21903 |
| game/unnamed_luminar_0x4c8d10.cpp | 0x4c9000 | luminar_frame_hook | 100 -> 0 (bytes exact, strict "target") | D21905 |
| vsdk/unnamed_ix_0x53ac20.cpp | 0x53c610 | ix_box_oriented_box_oriented | 128 -> 26 | D21906 |

**D21903 (ds_stream_open 0x55d400).** `desc.dwBufferBytes` stored before `desc.lpwfxFormat` gives the binary's
descriptor stores and push order. Left: the three separate `or eax, -1` returns, which 8966 merges (see below).

**D21905 (luminar_frame_hook 0x4c9000).** These changes together make all 1025 bytes equal:
- the thrown part is written as `if (h) { ... }` instead of an early return;
- inside it, `landed = false;` comes before `lobj_ray ray;`, so the constructor's min_radius store lands where
  the binary has it and ray.start is read off ESI;
- the miss step uses `vector3 *hp = &h->pos; hp->x += ...` (EDI-relative, as in the binary);
- `fwd.y = 0.0f;` comes right after `vector3 up;`, before up's stores.

Under VC6_STRICT it counts as "wrong target", not as a match. 8966 swaps the two operands of
`luminar_throw_gravity * reported_frametime` (velocity.y): the binary does `fld gravity / fmul frametime`, ours
the reverse. Behaviour is the same because the multiply commutes. These did not fix it: both operand orders,
`-=`, `-(p) + v`, a pointer / local / product local for either operand (any new local in that block changes the
register allocation: distance 13 bytes), reusing `damp` or `p.y`, and the operand orders of the neighbouring damp
/ gravity lines. Worth one more look; it may need the globals' declarations.

**D21906 (ix_box_oriented_box_oriented 0x53c610).** The changes:
- Bf is read through `const float (*F)[3] = Bf;` (128 -> 82).
- A search hoisted single products or factors into float locals (h0-h6, g0-g1, q1_00-q5_10) and added a T copy
  (qU0) where 8966's term or operand order differed (82 -> 26).

Each sum keeps its source term order; only a product or factor is computed into a local first. The match parts
are now byte-equal to the binary.

Left:
- test 2's T terms (binary Tz*B20 first);
- test 5's last term's operands;
- test 6's B02*Tx placement;
- tests 11 and 12's F*b->x term (the binary computes it third, ours last).

Pairs of transforms on these lines gave only 26 -> 24, so that version was not kept. The source is less readable
now. If you would rather keep the clean F-only form (82), take only the `F` pointer.

## Still differs (tried; do not repeat)

- **ds_stream_open 0x55d400 (30).** Tried, all still merged:
  - the return shapes: `== 0 return 0` with -1 tails, gotos to a shared fail label (three placements), a
    bool / int result, a switch on the format tag (worse), nested if / else, an outer `>= 0 &&` if;
  - for the acm test: `~0`, a bool / int through a pointer (gives sete / setne), `ok` locals, `return *pr`.

  The binary keeps three `or eax, -1` blocks: the acm failure inline, read_chunk's shared with the
  CreateSoundBuffer failure, and fill's own. 8966 cross-jumps them.
- **ds_load_buffer_acm 0x55cbe0 (142).** The binary keeps 0 in EDI (the `cmp eax, edi` src test, the stream
  and rc stores), passes src from EAX, and loads rc from its stack slot at the end. 8966 puts src in EDI and 0
  in EBP. The frame also differs: the binary has p1 in the buffer argument slot, stream at -84 and src at -80;
  ours has stream in the argument slot. Tried with no change:
  - rc as a one-int array (also with `= { 0 }`), rc through a pointer;
  - src without its init, as `BYTE *`, declared first or last, through a pointer, assigned in the if;
  - `!src`, a malloc temp, the two failure tests combined, the size in the malloc call;
  - stream / src / rc assigned in separate statements (three orders);
  - p1 in a block (also with n1), p1 last;
  - eleven declaration orders (o2 / o8 with stream first: 140);
  - an HMMIO local (138).

  This is register choice.
- **seg2d_intersect 0x53d8f0 (84).** Not tried this round: D15306 shows it needs the by-value vector2
  parameters as plain floats (a header / signature change), so it was left.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/level_script_ionaext.cpp (ls_ionaext_frame)
- src/vsdk/unnamed_vfile_0x5128b0.cpp (File::set_cipher_key)
- src/vsdk/unnamed_ds_0x55bf90.cpp (ds_play, ds_stream_open)
- src/game/unnamed_evil_urath_0x4c4170.cpp (evil_urath_ai_select_action)
- src/game/unnamed_luminar_0x4c8d10.cpp (luminar_frame_hook)
- src/vsdk/unnamed_ix_0x53ac20.cpp (ix_box_oriented_box_oriented)

## --check (VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt)

- level_script_ionaext: 10 matched there, 11 now, 0 lost
- unnamed_vfile_0x5128b0: 81 there, 82 now, 0 lost
- unnamed_ds_0x55bf90: 57 there, 58 now, 0 lost
- unnamed_evil_urath_0x4c4170: 17 there, 18 now, 0 lost
- unnamed_luminar_0x4c8d10: 21 there, 21 now, 0 lost (luminar_frame_hook is the 1 wrong target)
- unnamed_ix_0x53ac20: 31 there, 31 now, 0 lost

## Possible use elsewhere

- D21900: the pointer-tested bool may fix other `sete al / test al, al` leftovers.
- D21904: the "goto into the later return's block" device may apply to the other bosses' select_action
  functions.

## Worker 13

### W13 notes (m-1021d, decisions D22000-D22099)

Batch: decals_draw_for_lobj, decal_place, cquat_squad, cquat::multiply, fx_recb0::apply_forces,
fx_recb0::get_transform, conversation::restore_state, level_parse_shopkeeper, lobj_sphere_test_faces.

## New matches

| file | address | name | D | what did it |
|---|---|---|---|---|
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 0x5605f0 | fx_recb0::apply_forces | D22002 | `static const float k = 0.0625f;` for the last factor (8966 reads it from .rdata and pushes it through ECX, as the binary does from 0x58232c), and `float c = forces[i].v1c;` before `exp(-(c * dist))` (gives the binary's `fld v1c; fmul dist`) |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 0x4aec20 | lobj_sphere_test_faces | D22004 | the out-of-range block `{ inside = 0; if (t > 1.0f) inside = 0; }` (8966 merges the two stores and keeps the binary's dead second compare), and `float t2` declared in a block around the edge test (t2 then takes ray's dead slot and span takes g's: the binary's 0x34 frame) |

**D22002 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp). fx_recb0::apply_forces 0x5605f0 exact.** The binary pushes the
0.0625 factor with `mov ecx, [0x58232c]; push ecx` and not as an immediate. A literal in any form (`0.0625`,
`1.0 / 16.0`, `1 / 16.0f`, a `const float` or plain float local, a one-float array) gives the immediate. A
`static const float` (or `static float`) gives the load, at distance 4. The remaining `fld dist; fmul v1c` (the binary loads v1c first)
goes away with a float copy of v1c (`float c = forces[i].v1c;`); `dist * v1c`, pointer locals to dist or to
v1c, a dist copy, a product local and a `forces[i]` pointer do not change it. The same static-const device may fit
other functions where the binary pushes a float constant from .rdata through a register.

**D22004 (game/unnamed_lobj_raycast_0x4ae890.cpp). lobj_sphere_test_faces 0x4aec20 exact.** The dead compare: of
eight forms of the in-range flag only `inside = 1; if (t < 0 || t > 1) { inside = 0; if (t > 1) inside = 0; }`
gives it (122, all code equal except the stack offsets). `if / else inside = 0` in the block (151), an outer else
(154), two separate ifs (146), an unused clamp local (147), empty bodies (147) do not. The frame: VC6 gave
t the face slot, t2 the g slot and span a frame slot (+4 bytes). The declaration order of t / span / t2 (all six
orders), the floats first, inside first, separate declarations, and t2 / t / span declared just before
their calls do not change it. t2 in a `{ }` block around the edge-test tail does.

## Closer, kept

- game/level_scripts_common.cpp 0x4cc4d0 level_parse_shopkeeper: 102 -> 81 (D22000).
- game/unnamed_decals_0x4bbe80.cpp 0x4bc320 decal_place: 149 -> 90 (D22001).
- vsdk/unnamed_vfx_emitter_0x55ff00.cpp 0x5612b0 fx_recb0::get_transform: 150 -> 116 (D22003).

**D22000 (level_parse_shopkeeper).** The binary places the unknown-item block between the sell and buy scans.
VC6 threads a repeated type test into that layout: `if (type != -1) { ItemType *it = &item_types[type]; price =
it->value; if (!it->name) continue; } if (type != -1 && sell) { sell scan } else if (type == -1) { sprintf;
continue; } else { buy scan; price }`. Behaviour is unchanged. Without the `it` pointer it is 82. `sell && type != -1` is 252, a
found bool 84, `if (type != -1) {...} else sprintf` 90, a separate index local 82. Still differs: the binary
indexes item_types with EAX (`shl eax, 8`, the call result) while ours copies EDI to ECX. Type declared
first / last / register / unsigned, price first, `it` at function scope do not change it. Other differences: the binary's
`test byte ptr [game_flags], 2` after the fild (ours loads AL before it; a float copy of price and a bool for
the test do not help) and a dead `lea ecx, [8*ecx + level_shop_items]` before the count reload (an unused
pointer, a pointer for the stores in four placements and a shop_item temporary copy do not give it).

**D22001 (decal_place).** `plist_remove(decal_free, d)` written out without the head test (d is decal_free there;
`do { if (d->next == d) { ...; decal_free = NULL; break; } decal_free = d->next; ... } while (0)`) together with
`d->lit = lit != 0` (the binary's `test cl, cl; setne al`) gives 90. Either alone is worse or the same (163 / 130). The same
removal as if / else (171), through a `decal_record **pn = &d->next` (139, with or without the append written out),
and `plist_remove(decal_free, decal_free)` (172) are worse. Still differs: &d->next in EDI (binary), the
index in EBP and the node in EBX with a zero constant only in the head (ours EDI / EBX / EBP), and the lifetime bitfield
store as `((old ^ ms) & 0x1fffffff) ^ old` then `& 0x3fffffff` (ours merges the three bit stores). All six orders
of the lifetime / lit / in_use stores, `(unsigned)ms`, `ms & 0x1fffffff`, a record pointer copy for the bits,
`(int)lit != 0`, in_use set with clipped, the timer call moved and `true` for in_use leave it as it is. The age-ring
loop tests other->lifetime in memory before the record address; four forms of the left computation (index
local, ternary, default first, no `other`) are worse or the same.

**D22003 (get_transform).** `alpha = (ubyte)(int)(... * 255.0f)` gives the binary's `and eax, 0xff` before the
shift (123). With the result of the rotation in a named quaternion (`quaternion r(*(matrix *)&m); q7c = r;`) it is 116.
The named quaternion alone is 153. Behaviour is unchanged (the field takes 8 bits). Still differs: 8966 evaluates each channel's
`eval(next) * w1` term first and keeps the product in EBP; the binary evaluates the frame term first and spills
it to the stack, and its slots differ (binary: frame / w0 share 0x10, frac 0x14, next 0x18, the end_frame
conversion temp / w1 share 0x1c; ours: frame / w1 0x10, temp / w0 0x14, frac 0x1c). None of these change it:
the sum or the products swapped, all 120 declaration orders of the five locals, int casts, a frame-term local
(at the top or in a block; 284-317), `s +=` statements (290), `frac` as a one-float array, a frac pointer, an
end_frame int local, `w0 = 255; w0 -= w1`.

## Still differs (tried; do not repeat)

- decals_draw_for_lobj 0x4bc6b0 (17): none of these change it: a separate index for the vertex loop, a tri pointer copy
  for polygon_touches_planes, a bool for the touch test, a local_planes pointer, clipped declared before tri,
  a pointer to tri[2] for the store, a clipped pointer or a gr_vertex pointer inside the vertex loop, the flag
  stored through a ubyte pointer. Worse: an unsigned index with a signed test (21), tri[2] copied first (39),
  memcpy for tri[2] (71), the flag store after u / v (85). The two hunks (tri[2].z store; the EDI / EBP induction
  inits) are 8966 scheduling.
- cquat_squad 0x523b10 (18): none of these change it: a float pointer to out->x with indexed stores, a store through
  `float *o`, a cquat pointer to r, `!r.w`, a one-float array for x, a float local for x, an `extern const float`,
  `extern float` or `static const float` in place of CQUAT_TO_FLOAT. Worse: r declared first (22), `if (r.w) {
  ...; return; }` (32). Scheduling of the movsx; stop.
- cquat::multiply 0x522a40 (28): an `extern const float`, `extern float` or `static const float` for the factor do not change it.
  Same scheduling as squad; stop.
- conversation::restore_state 0x4d84d0 (30): none of these change it: an int local for the speaker bit, int copies of level and
  slot at the top, a const int pointer to level, a while loop, a topic pointer copy for t->name. The binary's
  argument loads per call vary (it reuses EAX between pushes); looks like 8966 scheduling.

## Behaviour errors / bug candidates

None.

## Header changes

None.

## Changed files

- src/game/level_scripts_common.cpp (level_parse_shopkeeper, D22000)
- src/game/unnamed_decals_0x4bbe80.cpp (decal_place, D22001)
- src/vsdk/unnamed_vfx_emitter_0x55ff00.cpp (apply_forces D22002, get_transform D22003)
- src/game/unnamed_lobj_raycast_0x4ae890.cpp (lobj_sphere_test_faces, D22004)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root W/src --only "level_scripts_common|unnamed_decals_0x4bbe80|unnamed_vfx_emitter_0x55ff00|unnamed_lobj_raycast_0x4ae890" --check src/decomp/vc6-matches.txt`:
4 units: 99 match, 0 wrong target, 15 differ, 0 not found, 0 errors; 97 matched there, 99 now, 0 lost.

## Tooling note

To read our .cod for slot maps without leaving anything in the system Temp folder, I ran vc6-compare with
`--keep` and with TMP / TEMP set to a folder inside W, so the kept folder was in W. I then deleted its two files and the
folder by their literal paths.

## Worker 14

### m-1021d worker 14 (D22100-D22199)

Batch: ai_state::load, cast_effect_track_add (game/ai.cpp), matrix3_pack_orientation,
matrix3_unpack_orientation (game/unnamed_goals_doors_0x475180.cpp), vfx_channel::eval, fx_rec4c::update
(vsdk/unnamed_vfx_channel_0x55b440.cpp), item_page_out_all (game/item_paging.cpp), nw_init_networking
(vsdk/unnamed_net_0x517590.cpp), ghost_anim_remap_in (game/unnamed_ghost_rider_0x4b9e70.cpp).

## New matches

- vsdk/unnamed_vfx_channel_0x55b440.cpp 0x55b460 fx_rec4c::update, **D22103**: `quaternion rq` (and the
  `rot = rq` store) put in its own `{ }` block inside the on_bone block. The block scope lets VC6 give rq
  the sum temporary's frame slot ([esp+0x24], as the binary). That moves every other temporary of the
  on_bone block (brot, rm, the transform result) to the binary's slots. Earlier rounds tried rq as a
  temporary and declared first, but not in a block of its own.

## Closer, kept

- vsdk/unnamed_vfx_channel_0x55b440.cpp 0x55bc00 vfx_channel::eval: 125 -> 28 (**D22102**). Changes:
  - one shared `cursor += inc;` after the two branches (an int `inc` local). This gives the binary's
    `add word ptr [ecx+8], ax`. The binary does not keep cursor in a register.
  - the run step written as `inc = 4; if (bytes) inc = 3;` (the binary's two moves and branch), then
    `frame += n`. The literal step stays `frame++; inc = bytes ? 1 : 2;` (neg / sbb).
  - `switch (p[0]) { case 1: break; default: goto bad; }` for the run test. This gives `and ebx, 0xff;
    dec ebx; jne`, with p[0] kept in BL from the `p[0] & 1` test. A `ubyte c` local spills it, an `int c`
    zero-extends it early.
  - `unsigned n`, so the run-range compare is `jb` (it was `jl`).
  - `p = (ubyte *)data + cursor;` again before the literal-case return (the binary reloads cursor there).

  The loop now matches. Left: the found-in-run path. The binary holds p in ESI (`lea esi, [edx + eax]`),
  tests it as `xor eax, eax; mov al, [esi]; dec eax`, and loads base_value first in both returns.
  Behaviour is unchanged: bytes is bit 15 of the same word as the frame bitfield, so reading it before
  `frame += n` gives the same value, and all the values compared are non-negative.
- game/unnamed_ghost_rider_0x4b9e70.cpp 0x4bb610 ghost_anim_remap_in: 78 -> 36 (**D22100**). The weight
  is stored in each branch, including both arms of the ready test, and there is no `continue`. This is
  tiger's D19401. Left: the same three loop registers rotated as in tiger. The phoenix version should get
  the same change (its D20702 is at 70).
- game/ai.cpp 0x4750b0 cast_effect_track_add: 40 -> 30 (**D22101**). The free slot is found through a
  pointer walk set in the for's init: `cast_effect_track *e; for (i = 0, e = cast_effect_tracks; i < MAX;
  e++, i++) if (e->spell == -1) break;`. This gives the binary's counter in EAX and pointer in ECX, and
  `add ecx, 12` before `inc eax`. Left: the loop tests `cmp eax, 5` where the binary tests the pointer end
  (`cmp ecx, end; jl`). The time code's three registers are also still rotated (ours ECX / EDX / EAX,
  binary EAX / ECX / EDX). Forms of this walk tried:
  - e declared before the for: 36;
  - `i++, e++`: 36;
  - `e = ..., i = 0`: 34 / 36;
  - e declared at the top: 32;
  - a pointer-end condition `e < tracks + 5` (jb, 42); an `(int)e <` condition (40, the walk is folded);
  - stores or the time through e: 42 / 55.

## Still differs (tried; do not repeat)

- **ai_state::load 0x4747e0 (18)**: all of these stay at 18:
  - a bool local for the type test;
  - the arg copy through an int local or a float-pointer local;
  - hash through an int pointer, or as `int hash[1]`;
  - an `in` copy, or combat_style through an int local, a `self` pointer or memcpy;
  - `Character **op = &owner`, or `const int *tp = &owner->type` (18, one file match lost);
  - a switch on the type (18, one file match lost).

  Left: owner is loaded into EDX before the combat_style store, and the arg copy register.
- **matrix3_pack_orientation 0x475380 (20)**:
  - an mm / o pointer copy and `a` declared at the top: 20 / 22;
  - a `float a[2]` or `int a[2]` copy: 22;
  - `__int64` copies: 10 for a alone (frame 8, but the copies are folded away), 34 for both. Not kept:
    it is a type pun, and the reloads are gone.
  - b declared before a gives 14 (also in 21b). The second pair's reload is then folded away, so the
    form is further from the binary's shape; not kept.

  The binary keeps a in the dead out / m argument slots.
- **matrix3_unpack_orientation 0x475430 (103)**: the sign multiplies.
  - The binary has `fld sign; fld x; fmul st, st(1); fstp out; ...; fstp st(0)`.
  - Double literals `? -1.0 : 1.0` give exactly this shape (82), but with QWORD constants where the
    binary has dword ones (0x580e1c, 0x57e990). Not kept.
  - These stay at 103: casts to double on the sign or on x, float or double sign locals, a pointer local
    for x, x copied into a local, the sign first, a `float s[1]`, a sign through a pointer.
  - An int sign gives 137, volatile values 113, a volatile sign 94.
- **item_page_out_all 0x46ca50 (30)**: EBX / EBP swapped. These stay at 30: `other != -1` (47),
  `const int *ps = &slot`, s computed before the slot test (68), one exit, an unsigned i with an int
  test, a bool for the type test, other set through a pointer, mesh `= 0`, a type local, `s = slots;
  s += slot`. This is register choice.
- **nw_init_networking 0x5188d0 (102)**: three goto shapes all compile to the same layout (102):
  - the 56k / fast tests after the slow block's body, with probe_ras / start_winsock labels inside it
    and `goto started`;
  - the same with the fast test negated;
  - an else form.

  The block layout is canonical whatever the source order.

## Behaviour errors / bug candidates

None found.

## Header changes

None.

## Changed files

- src/game/ai.cpp (cast_effect_track_add, D22101)
- src/game/unnamed_ghost_rider_0x4b9e70.cpp (ghost_anim_remap_in, D22100)
- src/vsdk/unnamed_vfx_channel_0x55b440.cpp (fx_rec4c::update D22103 match, vfx_channel::eval D22102)

## --check (VC6_STRICT=1 vc6-sweep, the three units together)

`3 units: 141 match, 0 wrong target, 4 differ, 0 not found, 0 errors`
`check against src/decomp/vc6-matches.txt: 140 matched there, 141 now, 0 lost`

## Decisions

**D22100 (game/unnamed_ghost_rider_0x4b9e70.cpp; closer only). ghost_anim_remap_in 0x4bb610: 78 -> 36.**
The weight is stored in each branch (also in both arms of the ready test, no `continue`), as tiger's
D19401. Left: the three loop registers rotated.

**D22101 (game/ai.cpp; closer only). cast_effect_track_add 0x4750b0: 40 -> 30.** The free-slot search
is a pointer walk set in the for's init with `e++, i++`. Left: the loop tests i, not the pointer end,
and the time code's registers are rotated.

**D22102 (vsdk/unnamed_vfx_channel_0x55b440.cpp; closer only). vfx_channel::eval 0x55bc00: 125 -> 28.**
A shared `cursor += inc`, `inc = 4; if (bytes) inc = 3;`, `switch (p[0])` for the run test, an unsigned
n, and p recomputed for the literal return. The loop now matches; the found-in-run path is left.

**D22103 (vsdk/unnamed_vfx_channel_0x55b440.cpp; match). fx_rec4c::update 0x55b460.** rq goes in its
own block, so it shares the sum temporary's slot and the on_bone block's locals take the binary's slots.

**D22104 (notes only).** No new form found for ai_state::load, matrix3_pack_orientation,
matrix3_unpack_orientation, item_page_out_all or nw_init_networking (see "Still differs").

Helper scripts are in w14/try: tryd.mjs prints the distance for each variant; side.sh and slots.mjs
give a side-by-side with resolved stack offsets. Compile listings are kept in w14/tmp, not the system
Temp folder.
