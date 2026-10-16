# match3: living_entity.cpp split, /Od match rate

Base: snapB/src/decomp (commit 95bd3d5). Result: match3/src. Files changed: CHANGED.txt. Decisions D6100-D6107
(match3/src/include/DECISIONS.md). Compiler: VC6 Professional, cl 12.00.8168 (scratchpad vc6p).

## 1. The split: not a separate unit (D6100)

The survey's premise does not hold. Evidence from Sum.exe and from VC6 itself:

- **VC6 layout test** (compiled and linked with VC6's cl / link, map file): an object's .data is all of its
  initialised globals first, then its literals, in function order, even for a global defined after functions or
  after `#pragma optimize("", off)`. An object's .bss is one contiguous block.
- **.data**: the tail-only tables `character_speech_times` (0x599698) and `hero_model_prefix_table` (0x5996a4) sit
  among living_entity's other initialised globals (0x599644-0x5996b3), before the unoptimised part's literals. A
  separate tail object would have them at the start of its own .data (after 0x599eac). The tail's literals
  (0x599eb0-) follow the unoptimised literals directly.
- **.bss**: the tail's console command objects (ccmd_chance_gold 0x8eca30, ccmd_Spell_ap 0x8eca40,
  ccmd_kill_summon 0x8eca50, ccmd_no_resist 0x940428; their initialisers are tail code) lie before and between
  living_list (0x8eca60), the pool and spell_cooldowns (initialised by unoptimised code). Two objects cannot
  interleave like that.
- **Code style**: the unoptimised part's static initialisers are frame-pointer wrappers; the tail's are tail jumps,
  exactly where `#pragma optimize("", on)` sits. The unoptimised literals are pooled across objects ("fire_ring_xp"
  0x593e54, "right-foot" 0x596350): an /O2 compile (string pooling), not plain /Od.

So 0x44d9f0-0x46335f is ONE object: /O2 with `#pragma optimize("", off)` at the top and `("", on)` before
character_say, as in the cutscene file (D3201). Added to living_entity.cpp; its vc6-flags.txt line is now
`/O2 /GX /MT` (not /Od /Ob1: with /Od the tail cannot be optimised). /O2 + pragma also beats /Od /Ob1 on the
unoptimised part itself (keeps /Oi: set_name, status_effect_add_from_item match).

The real object boundary is at **0x463360**: Debug_death = 1 (.data 0x59a16c) lies between the Spell_ap literals and
the bloodspots literals (start of a new object's .data), and ccmd_death 0x941710 / ccmd_bloodspots 0x941730 /
Bloodspots 0x94173c interleave with combat.cpp's ccmd_Super_power 0x941720 / Super_power 0x94173d, while
living_entity's .bss ends at move_play_serial 0x941708 (16-byte aligned start 0x941710). So init_ccmd_bloodspots ..
cmd_death (0x463360-0x46353f) are the head of combat.cpp's object: moved there (with Debug_death, Bloodspots). No
`unnamed_*_0x461dc0.cpp` and no README row for a new unit; the README rows of living_entity / combat / objects /
cutscene are updated instead. sum.symbols and docs/sum-exe/source-map.md (not edited) still tag 0x463360-0x463480
living_entity.cpp.

## 2. Match rate (VC6 12.00.8168, tools/vc6-compare.mjs)

| Unit | Before (95bd3d5) | After | Flags |
|---|---|---|---|
| game/living_entity.cpp | 49 / 134 (/Od /Ob1 /GX /MT) | 88 / 132 (two functions moved) | /O2 /GX /MT + pragmas |
| game/objects.cpp | 35 / 40 | 38 / 40 | /Od /Ob1 /GX /MT (unchanged) |
| game/unnamed_cutscene_0x434900.cpp | 16 / 26 | 25 / 26 | /O2 /GX /MT (line added) |
| game/combat.cpp | 33 / 61 | 35 / 63 (the two moved commands match) | default |
| Units won through profile.h (D6102) | | +9 (ai, pathfinding, ai_move, ai_think x2, pfgeom, shadows, character_instance x2) | |

No match lost anywhere (checked per function against the base run of each unit and of every user of a changed
header). Total +62 byte-identical functions (39 + 3 + 9 + 2 + 9). Confidence raised to exact for every matching function.

## 3. Best /Od flags

Sweep on the base sources: objects.cpp 35 with /Od /Ob1 /GX /MT, /Od /Ob1 /MT, /Od /Ob2, + /Oi, + /Gf, + /GF, + /Z7,
+ /Zi, + /Gy; 28 with /Od /Ob0 or plain /Od. living_entity.cpp 49 (/Od /Ob1 /GX), 48 without /GX, 51 with /Oi.
So: inline expansion /Ob1 is required, /GX matters where there are EH frames, /Z7 / /Zi / pooling change nothing in
the bytes. The original form is **/O2 /GX /MT with `#pragma optimize("", off)`** (living_entity, the cutscene file;
it is /Od + /Ob1 + /Oi + pooling); objects.cpp ties between that and /Od /Ob1 /GX (its literal "gold" is pooled with
gameplay's 0x594430, so it too was built with pooling: /O2 + pragma or /Od /Gf; kept as is).

## 4. Why the frames differed: VC6 orders /Od locals by name hash (D6101)

Found with probe functions: 8168 places one scope's locals in bucket order, bucket = (h ^ (h >> 16)) & 15,
h = h * 4 + (h >> 4) + c; newest declaration first within a bucket; block scopes after, `this` last. The file
headers' "first declared local = ebp-4" rule held only by chance. Tools in scratchpad/m3test: lvmap.mjs (maps every
named slot of the listing to the original's ebp offset by an LCS alignment), sug.mjs / b.mjs (bucket search),
apply.mjs (rename + re-declare). 38 functions renamed (names are ours; none are in the binary). Inferred, open: the
original 8966 may order differently (refresh_item_status_effects' 11 locals are in first-use order).

Other source fixes (D6103-D6106): place_at_party_start reads start_navpoint; play_move's trigger-9 test was inverted
(BEHAVIOUR FIX, D6104); three int parameters are bool (living_entity.h, D6105); profile_label inline (profile.h,
D6102); switch / if shapes in status_effect_add, LE_PUT_STATUSES, can_cast_spell, cmd_kill_summon,
set_floating_text, get_display_name, hero_model_prefix, ai_suspended, skill_equip_mod, cutscene::load_files.

## 5. What still differs

- living_entity.cpp (44): unoptimised 31 - `x = c ? a : b` assigned without a temporary in Sum.exe (8168 always uses
  one; ring_add_xp, rings_add_xp_all, scale_stats_to_level, add_ap, ...), inline vector3 return values used in place
  (8168 copies once more: walk_to_point, is_in_view, distance_between_edges, update_head_look, get_hit_by_spell,
  ...), `bool = (byte == 1)` via sete (save_state), frames of the big functions (monster_drop_loot, init, on_death,
  clip_move_to_navmesh) with many such temporaries; set_locomotion_speed's frame has 16 unexplained bytes. Optimised
  tail 13 - register choice (equip_bonus, say), a 4-entry jump table (apply_hero_model), cross-jumping
  (set_armour_alt_texture), byte narrowing (is_mount, is_rider), get_resistance's curse in a register,
  set_underwear_textures' table build. These reproduce in small tests with every flag tried: compiler build
  (8168 vs 8966) rather than source (D6107).
- objects.cpp: character_process_anim (the inline return copy), item_obj_spawn (inline temporaries).
- cutscene: cutscene_parse_table (16 locals need a consistent renaming; no plausible name set found).
