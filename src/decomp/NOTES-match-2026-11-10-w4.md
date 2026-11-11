# Byte-match round 2026-11-10, worker 4 (D26700-D26799)

Batch: 40 differing functions in 15 units (render scene, level scripts common, serpent, vector, gr_glide,
new_inventory, tentacle beast, vfx mesh, multicontainer, new_gamecreate, summoner, ghost rider, character,
ui_container, ui_button). Every one had been worked on in at least four earlier rounds, so the time went to the
functions with the smallest distances and to new kinds of forms, not to repeating the earlier lists.

Helpers (not in the repository): `v/tryd.mjs` (vc6-try with the --score distance per variant, LF in the variants
turned into CRLF for CRLF files, `--align` prints each variant's differing lines), standalone probe files compiled
with the unit's flags to see what 8966 does with one construct.

Result: 1 new match (strict), 0 closer. Sweep `--only unnamed_serpent_0x495290` with `--check`: 39 matched there,
40 now, 0 lost. The other 14 units are unchanged; the sweep over all 15 units with `--check` (after the
vc6-matches.txt update): 529 match, 37 differ, 1 not found (WinMain), 529 matched there, 529 now, 0 lost.
`node tools/link-build.mjs --config fixed` in the worktree:
259 units, unresolved 0, multiply defined 0.

## New matches

| file | address | name | D | what did it | distance before / after |
|---|---|---|---|---|---|
| game/unnamed_serpent_0x495290.cpp | 0x496d60 | ai_state::serpent_illusion_update | D26700 | the fade as `a = 255; if (i != 0) a = 230; a += (rem - 300) * a / -1000;` | 4 / match |

**D26700 (game/unnamed_serpent_0x495290.cpp). ai_state::serpent_illusion_update 0x496d60 is a byte match.** The
copies' fade is written on one variable that holds the base alpha and then the faded value:
```
int a = 255;
if (i != 0)
    a = 230;
a += (rem - 300) * a / -1000;
```
in place of a separate `base` and `int a = (rem - 300) * base / -1000 + base;`. With a separate base, 8966 rewrites
`base + x / -1000` as `base - x / 1000` (magic 0x10624dd3 and `sub`); the binary divides by -1000 (magic
0xef9db22d) and adds. Standalone probes (Observed, /O2) show the rule: 8966 keeps the negative divisor when the
quotient is not added to another value (`return x / -1000`, a compare), when the sum is formed through a pointer
store or an if / else, and in every form tried where the variable that receives the sum is also the dividend's
factor (`a += (r - 300) * a / -1000`, `a = (r - 300) * a / -1000 + a`, even `a -= (r - 300) * a / 1000`, which it
turns into the add of the negative divide). The pointer and if / else forms keep the divide but change the
function's register allocation (distance 89-90); the one-variable form gives the binary's code exactly (also
`a = (rem - 300) * a / -1000 + a` and `a -= (rem - 300) * a / 1000`; the `+=` form keeps the -1000 visible). Same
behaviour: a is the base when the product is formed, so the value is `base + (rem - 300) * base / -1000` as
before, and the clamp and the store are unchanged. Earlier forms (D20501's list, the 16 standalone forms of
round 2026-10-22: split statements, `x * -b`, `-x * b`, `b - -(...)`, `(300 - 1300)`, constants, inline helpers)
all kept two variables, which is why they were rewritten. Inferred: the original source kept the alpha in one
variable, as the binary's single ECX (base, then sum, then clamp) also suggests. This new device is worth trying
wherever a binary has a divide by a negative constant that 8966 turns into a subtract.

## Closer

None kept.

## Still differs (tried this round; do not repeat)

- **level_parse_shopkeeper 0x4cc4d0 (17).** Finding: the binary's dead `lea ecx, [8*ecx + level_shop_items]` and
  the count reload after the two entry stores are exactly what an element pointer gives:
  `{ shop_item *si = &level_shop_items[level_shop_item_count]; si->item_type = type; si->price = price; }
  level_shop_item_count++;` (Observed: that part of the listing then equals the binary, the pointer stores make
  8966 reload the count and leave the lea). But the pointer moves the register allocation of the whole function:
  sell goes into BL and tf into EDI (binary: sell in its stack slot, tf in EBX), distance 62. The same 62 with si at
  function scope in all seven declaration positions, si with the increment inside the block, sell in five
  declaration positions, four orders of the int declarations, the stores in an outer block; `si =
  &...[count++]` 83, an `int *ip` view 62, the pointer for the second store only 71, `int sell` 77,
  `(&level_shop_items[count])->` 17 (no pointer, no change). Something in the original makes sell lose its
  register while keeping the element pointer; not found.
- **serpent_anim_remap_in 0x495d00 (36)** (and ghost_anim_remap_in 0x4bb610, the same residue): c reused for the
  rider (`c = mount->get_rider()`), c reused for the mount, the set cast after the rider code: all 36; ready from
  a ternary 41; the ready test folded into one store (`ready ? 3 : 0`) 57.
- **ai_state::serpent_illusion_start 0x496ba0 (34)**: the alpha `(rem - 1000) * 255 / 1000` as one variable
  (`a = 255; a = (rem - 1000) * a / 1000`, `a = 255; a *= rem - 1000; a /= 1000`, `a = rem - 1000; a = a * 255 /
  1000`, the same with `*=` / `/=`, rem reused for the alpha, rem reduced by 1000 first): all 34. The rotation is
  not in the divide.
- **party_set_level 0x4cec00 (0, SIB order)**: unsigned casts on the source index, `i[c->info->skill]`,
  `*(i + ...)`, `i[c->skill] = i[...]`, a char_info cast, a do / while: all the same (D17901 stands).
- **level_vis_reset 0x487030 (12)**: an unsigned index loop gives the binary's biased pointer (`[eax - 4]`) but
  keeps 1 in EDX for both the count global and the loop store (15); the same with the element pointer, with the
  set list store first, and with sets stored before count; `v->count = level_num_vis_sets` in the pointer loop 12.
- **vector3::random_unit_vector / random_direction / random_direction_in_cone (5)**: standalone probes with the
  unit's flags (/Ox /Ob0 /GX /MT /Ow) show that 8966 gives `fld st(0); fmulp` for z * z only when r stays in an FPU
  register (r used in a double expression: `(float)(cos(a) * r)`), and then r is not stored, so the binary's form
  (r in memory, fmulp) was not reproduced. Tried (5 unless noted): the sqrt written twice (`cos(a) *
  sqrt(1 - z*z)`, CSE'd into the binary's slot but the same FPU code), the same in double (11), `r = z; r =
  sqrt(1 - r * r)` and `r = z * z; r = sqrt(1 - r)`, `r = z = frand_range(...)` (13), and in the probes a one-float
  array, a pointer to r, a struct member r, `this->z`, `sqrtf` / `cosf`, a double a, x and y in either order.
- Looked at, earlier lists stand, nothing new tried beyond reading the align: tentacle_beast_boss_load (2, the
  last `mov ecx, [esi+0x4c]` scheduled before `push 0x87`), vector3_angle_and_axis (2, the acos fstp after two
  pushes; the angle already shares a's dead slot), inventory_drag_to_equip (31, of which the jump-table bytes
  disassembled as code count most; the real difference is the default case's `xor esi, esi` from D20804's `n = 0`),
  tentacle_beast_frame_hook (24, scheduling of the ray copy), level_parse_table (14, buffer slots, D26002).
- **WinMain 0x43e5e0 (game/summoner.cpp)**: still "not found": vc6-compare does not pair `_WinMain@16` (a tool
  limit, round 2026-10-20's note); not a source question.

Not reached in the time box: render_scene's large functions (levelrender_render_scene, add_effects,
overhead_label_place, draw_name_labels, draw_list, level_vis_parse), level_script_parse_table, serpent_choose_attack,
gr_glide's three, inventory_handle_click, fx_rec98::render_mesh / update_frame, mp_handle_item_drop,
gamecreate_handle_click, ca_character::add_skeleton, ui_container::draw, ui_button::draw_arrow.
