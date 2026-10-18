---
title: Tentacle Beast grab and throw (future project)
---

# Tentacle Beast grab and throw (future project, G250)

**Status (2026-10-18):** implemented as an experimental SummonerFix switch, off by default
(`src/summonerfix/tentacle.c`; `summonerfix.ini` `[Experimental] TentacleGrab=1`, `TentacleGrabChance=10` percent per
decision; `[Debug] TraceTentacle=1` logs every step), and tested in game in the sewer boss room with a full party
(chance 100 for the test). Seen working: grab, 30 s hold dealing 3 damage every 6 s, throw, landing on the floor,
one grab at a time with the 10 s cooldown, the battle ending mid-hold (the hero is put down and gets its AI back), a
held hero dying (put down at once), no crashes. Not yet tried in game: a throw over the water, a summon as target,
the chance-10 feel over a long fight. The analysis below was done first (by a research agent, checked against the
disassembly and the retail tables); the code follows its section 7.3, with the changes listed at the top of
`tentacle.c` (for example the release tests 0x80, not 0x88, because the grab itself sets 0x8). The grab is switched off in the shipped game, probably on purpose. The goal is to get
every part of it working and offer it as an experimental SummonerFix switch, off by default. Linked from the
[Roadmap](../roadmap.md#future-projects); the bug entries are G250 and G251 in
[Bugs in the original game](../sum-exe/game-bugs.md).


Sum.exe v1.40. Evidence is given as addresses. **Observed** = seen in instructions or retail data.
*Inferred* = a conclusion, not seen run. Disassembly from `tools/asm.mjs`; constants read from `Summoner/Sum.exe`.
The decompiled file `src/decomp/game/unnamed_tentacle_beast_0x4c6480.cpp` matches the disassembly on every point
checked below.

## 1. Functions on the path

| Function | Address | Role |
|---|---|---|
| `tentacle_beast_ai_select_action` | `0x4c6520` | picks swipe / squeeze / (grab) and a target |
| `tentacle_beast_ai_frame` | `0x4c7070` | boss AI frame; hold tick; starts the grab; sub-mode switch |
| `tentacle_beast_start_grab` | `0x4c6800` | plays move (0, 0x86), sub-mode 0x35 |
| `tentacle_beast_ai_grab` | `0x4c6850` | sub-mode 0x35: at trigger 7 takes the victim |
| `tentacle_beast_check_stage` | `0x4c7360` | starts the release (HP, 30 s, tentacle at 0 HP) |
| `tentacle_beast_start_release` | `0x4c69e0` | plays move (0, 0x87), sub-mode 0x37, MP packets |
| `tentacle_beast_ai_release` | `0x4c6a80` | sub-mode 0x37: at trigger 8 throws the victim |
| `tentacle_beast_frame_hook` | `0x4c7960` | every frame: pins the held victim, flies the thrown one, lands it |
| `tentacle_beast_battle_update` | `0x4c7640` | re-arms `tentacle_beast_grab_ready` |
| `tentacle_begin_sink` | `0x4c6c50` | battle end: releases a held victim instead of sinking |
| `tentacle_beast_boss_load` | `0x4c6d10` | loads moves and tag points |
| `ls_sewerboss_frame` | `0x4ee0e0` | level script; ends the battle when the golems die |
| Luminar twins (the template) | `0x4c9000`, `0x4c9410`, `0x4c9b00`, `0x4c9cd0` | same grab/throw, working in retail |

Jump table of the sub-mode switch in `tentacle_beast_ai_frame` (**Observed**, bytes at `0x4c72ec` and `0x4c7300`):
0xf -> `0x4c7230` (combat update), 0x16 -> `0x4c70c4` (back to 0xf), 0x35 -> `0x4c72d7` (`ai_grab`),
0x37 -> `0x4c72e2` (`ai_release`), others -> return.

## 2. Tentacle record (0x110 bytes, 4 records at `0x248c328`)

**Observed** from the instructions: `+0x00..+0x77` five 0x18-byte slots (handle at +0, set to -1 by boss_load
`0x4c7005`, nothing else in the unit writes them, so they are always -1); `+0x7c` chosen move; `+0x80` the tentacle
character; `+0x84` flags (1 down, 2 rising, 4 sinking); `+0x8c` target handle; `+0x90` hit count; `+0x94..+0xa3`
4 hit slots; `+0xa4` death move; `+0xa8` think timer; `+0xac` pickup point ("$tag-26-pickup", world, never read);
`+0xb8` grab tag; `+0xbc` victim handle; `+0xc0` unused timer; `+0xc4` hold timer; `+0xc8` squeeze-tick timer;
`+0xcc` thrown handle; `+0xd0` throw velocity; `+0xdc` throw gravity; `+0xe0` release HP; `+0xe4` reach;
`+0xe8` swipe point; `+0xf4` squeeze point; `+0x100` sink timer; `+0x104` hold time; `+0x108` up time;
`+0x10c` HP mark. `+0x104`, `+0x108`, `+0x10c` are written but never read anywhere in the code (grep).

Globals: `0x248c768` grab timer, `0x248c76c` battle started, `0x248c770` grab_ready, `0x248c7a4` move 0x86,
`0x248c7a0` move 0x87 (stored, never read), `0x248c7a8` active record.

## 3. The data

- **Observed** variation-name table `0x595cf8` (index = id): 0x86 "Luminar grab", 0x87 "Luminar throw",
  0x8c "flail", 0x8d "pick up ready", 0x93 "ready", 0x95 "ready hurt". Trigger table `0x5960b4`: 7 "impact start",
  8 "impact end". Move categories (from `0x596064`): 0 attack, 4 flinch, 6 death, 8 movement, 10 transition, 14 state.
- **Observed** `Tentacle Beast.tbl`: attack "Luminar grab" = `tent_pickup.mvf`, "impact start" 25 (trigger 7),
  "no interrupt" 0. Attack "Luminar throw" = `tent_pickup_throw.mvf`, "impact end" 21 (trigger 8). State
  "pick up ready" = `tent_pickup_ready.mvf`. "ready" and "ready hurt" exist. HP 200, Damage 15.
- **Observed** `tentacle_beast_boss_load` loads the grab move correctly: `character_find_move(0, 0x86)` at
  `0x4c6fe0`, stored at `0x4c6fe5`. All four tentacles share one table, so the shared global is fine (*Inferred*).
- **Observed** the victim side: `Joseph.tbl`, `Flece.tbl`, `Jekhar.tbl`, `Rosalind.tbl` each have state "flail"
  (the held pose, `charinfo_find_move(info, 14, 0x8c)`) and movement "variation 1" = `*_throwndown.mvf` (the
  landing move, `character_find_move(8, 1)`). Summons and other tables tested have no "flail" (*Inferred* from the
  grep of alltbl: only the four heroes and Evil Joseph have it).
- **Observed** `sewerboss_script.tbl`: four `Tentacle Beast#$npc057..060`, `+Boss`. Navpoint `$tbintro`. Cutscene
  "TB-Intro", level "Sewerboss" (`cutscene.tbl` line 796).

## 4. Intended mechanic, step by step

### 4.1 Selection (`0x4c6520`)
1. Returns 5 (busy) while `living_combat_is_busy` or the last own move (`+0x194`) plays (`0x4c6536`, `0x4c6559`). **Observed**
2. Clears `+0x7c` (`0x4c6575`), waits for the think timer, 2.5..5 s random (`0x4c6592`..`0x4c65d5`). **Observed**
3. Builds up to 5 candidates: first a live slot (never, slots are -1), then living entities of type 7 without
   flags 0x88 (dying/hidden or dead) (`0x4c6628`..`0x4c6655`). A held victim has flag 8, so no tentacle targets it. **Observed**
4. `r = frand()` (`0x4c666d`). `r < 0.0` (`0x57eb84`, the shared 0.0 constant) -> move 0x86; `r < 0.3`
   (`0x581970`) -> squeeze (move 2); else swipe (move 1) (`0x4c6672`..`0x4c669a`). **Observed**
5. `+0x7c` is cleared again (`0x4c66a5`). Only the squeeze compares the choice (`0x4c66b4`). A grab choice falls
   into the swipe search, which stores move 1 (`0x4c67de`). So the grab is lost twice. **Observed**
6. Squeeze target: nearest candidate within 3.0 (`0x57ef1c`) of the squeeze point on X/Z (`0x4c66d0`..`0x4c6762`).
   Swipe target: first candidate with edge distance < reach `+0xe4` (`0x4c679d`..`0x4c67f0`). **Observed**
7. *Inferred* intent for the grab: like the squeeze, a candidate near the pickup point `+0xac` (computed at load,
   `0x4c6f7f`..`0x4c6f9a`, then never read). The intended gate is `tentacle_beast_grab_ready` (`0x248c770`):
   1 at load, 0 after a grab (`0x4c683c`), 1 again 10 s after a thrown victim lands (`0x4c7c28` arms the timer,
   `0x4c765c`..`0x4c7673` re-arms the flag), and forced 0 while only one party member lives (`0x4c76af`..`0x4c76b4`).
   **Observed**: nothing reads `0x248c770` except `battle_update` itself. The gate is not wired in.

### 4.2 Start grab (`0x4c6800`)
- `tentacle_beast_ai_frame` case 3 copies the record target into the AI target (`+0x60`) and the combat target
  (`+0x55c`) (`0x4c7262`..`0x4c728d`), then calls `start_grab` only when `+0x7c == move_86` (`0x4c72b6`..`0x4c72cf`). **Observed**
- `start_grab`: plays (0, 0x86) at speed 1.0 into `+0x194`, `ai_set_mode(3, 0x35)`, grab_ready = 0, grab timer
  invalidated (`0x4c6803`..`0x4c6846`). No MP packet. No turn toward the target. **Observed**

### 4.3 Grab (`0x4c6850`, sub-mode 0x35)
- Each frame `tentacle_beast_ai_frame` passes `living_from_handle(+0x8c)` read before the select (`0x4c71c6`..`0x4c71dd`). **Observed**
- Grab move finished -> `+0x194 = -1`, `ai_go_idle` (back to 0xf through `tentacle_beast_ai_idle`). **Observed**
- On trigger 7 with a non-NULL target (`0x4c6895`..`0x4c68a8`): looks up the victim's "flail" (14, 0x8c); if absent
  or anim -1, returns and nothing is held (`0x4c68be`..`0x4c68d4`). **Observed**
- Victim: state anims cleared, flail weight 1.0 (`0x4c68e0`..`0x4c68f1`); `+0xbc` = victim handle;
  `+0xe0` = **victim** HP x 0.75 (`fld [esi+0x5f8]` at `0x4c6912`, esi = victim, `0x580e04` = 0.75); hold timer
  30 s (`0x4c6901`, `0x4c6930`); tick timer 0 (first tick at once, `0x4c6942`); `set_flag_2_4(-1)` (flag 2);
  flags |= 0x1028 (8 dying/hidden, 0x20, 0x1000) (`0x4c6950`..`0x4c6963`). **Observed**
- Tentacle: ready and ready-hurt anims = "pick up ready" (14, 0x8d), no NULL test on the result (`0x4c697d`) (data
  has it). **Observed**
- Leadership: goes to the first party entry whose handle is not the victim's, with no alive test and even when the
  victim is not the leader (`0x4c69a0`..`0x4c69c7`). **Observed**
- No distance test at trigger 7: the victim is taken from wherever it is. No damage at the grab (the Luminar deals
  96, `push 0xc2c00000` at `0x4c9b63`). **Observed**

### 4.4 Hold
- Held victim position: every frame 1.5 below the grab bone, orientation = bone (`0x4c7998`..`0x4c7a0c`). **Observed**
- Victim AI: `ai_frame` sends any non-boss with flag 8 to idle + inactive every frame (`0x472b89`..`0x472ba6`).
  So the victim cannot act. Other members act normally. **Observed**
- Tentacle: while `+0xbc != -1` and sub-mode 0xf, `ai_frame` only adds frame time to `+0x104` and returns
  (`0x4c716a`..`0x4c71c5`): no attacks while holding. **Observed**
- Tick: when timer `+0xc8` elapses, `character_add_hp(+3.0, tentacle, 1, 0)` on the victim (`0x4c7122`..`0x4c7150`),
  then 6 s again (`0x4c715a`). +3 is a heal (Luminar damage is negative). 5-6 ticks in 30 s. **Observed**
- Release triggers in `check_stage` (`0x4c7499`..`0x4c74db`): tentacle HP (`[esi+0x80]+0x5f8`, `0x4c74b1`) <
  `+0xe0`, or hold timer 30 s elapsed. Not while sub-mode 0x37. **Observed**
- Slip: `+0xe0` comes from the victim's HP but is compared with the tentacle's HP. The Luminar uses its own HP minus
  250 (`0x4c9bc3`..`0x4c9bd4`, `0x581c3c` = 250) for the same test. *Inferred* intent: release when the tentacle
  has lost a quarter of its HP. As shipped: a victim with HP above 267 (0.75 x HP > 200) is released as soon as the
  grab move ends; a weaker one is held until the tentacle drops below 0.75 x victim HP, or 30 s.

### 4.5 Release and throw
- `start_release` (`0x4c69e0`): returns when already 0x37 or any move plays (`0x4c69e7`..`0x4c69fe`); plays (0, 0x87)
  with flag 1, sub-mode 0x37, priority 99999 (`0x4c6a33`), host sends `mp_send_obj_update` and
  `mp_send_swi_boss(owner, 3, 0x37)` (`0x4c6a40`..`0x4c6a74`). Retried every frame by `check_stage`. **Observed**
- `ai_release` (`0x4c6a80`): on trigger 8, `+0xcc` = victim, `+0xbc` = -1, gravity 0, victim =
  `living_from_handle(+0xcc)` with no NULL test (`0x4c6b11`..`0x4c6b19`, first use `0x4c6b53`). Velocity =
  (swipe point - victim pos) x 2/3 (`0x580204`) (`0x4c6b4d`..`0x4c6ba1`). Tick timer invalidated; tentacle anims back to
  ready / ready hurt; camera follows the victim if it is `cur_character` (`0x4c6c07`). The grab-bone transform at
  `0x4c6b43` goes into unused locals. **Observed**
- Flight (`frame_hook`, `0x4c7a0f`..`0x4c7ba0`): step = velocity x dt; ray against geometry with flag 0x40; on a
  hit the victim stands 0.5 above it. Horizontal velocity x (1 - 0.75 dt); vertical -= gravity x dt; gravity +=
  9.8 dt (`0x581c2c`). **Observed** *Inferred*: the victim lands about at the swipe point (the horizontal decay
  sums to ~0.89 of the offset), a short toss in front of the tentacle.
- Landing (`0x4c7bac`..`0x4c7c77`): flags &= ~0x1028, `ai_go_idle`, face level, `character_find_ground`,
  `+0xcc = -1`, grab timer 10 s, plays movement 1 ("throwndown") and `set_flag_2_4(move)`, else clears flags 2/4. No
  damage, no knock-down call (the throwndown move is the knock-down). **Observed**
- The Luminar version also snaps the thrown body to the nearest navigable point each frame
  (`pf_point_is_navigable` `0x4c92a8`, `pf_find_nearest_navigable_point` `0x4c92c9`). The tentacle version has no
  such test. **Observed**

### 4.6 Early releases
| Case | What the code does | Status |
|---|---|---|
| Tentacle at 0 HP while holding | `character_kill` gives a type-9 boss flag 8 only (`0x450e0f`..`0x450e1e`); `ai_frame` keeps running it (dead test is 0x80, `0x474be2`; flag-8 inactivation skips type 9, `0x472b98`). `check_stage` calls `start_release` (`0x4c73e4`..`0x4c73ef`, `0x4c74d9`), the throw runs, then the death move (6, 0x22). | **Observed** path; works (*Inferred*) |
| Tentacle killed during the grab move | grab still fires at trigger 7, release follows when the grab move ends. | *Inferred* works |
| Victim dies while held | Nothing tests it. The body stays on the bone until 30 s / HP release, is thrown, and on landing gets `ai_go_idle` (`0x4c7bc4`), which sets AI mode 1 unconditionally (`0x4738d3`). | **Observed** missing test; *Inferred* risk of a "living" corpse |
| Battle ends (golems dead) while holding | `ls_sewerboss_frame` calls `tentacle_begin_sink` (`0x4ee201`), which only calls `start_release` (`0x4c6c56`..`0x4c6c6b`) and does not sink; then `battle_started = 0` (`0x4ee217`). `tentacle_beast_ai_frame` stops before the sub-mode switch when the battle is not started (`0x4c70e4`..`0x4c70eb`), so `ai_release` never runs. `start_release` also refuses while a move plays. | **Observed**; *Inferred*: victim held forever with flags 0x1028 (AI off) and the tentacle stays up |
| Battle ends while a victim flies | `begin_sink` returns without sinking (`0x4c6c73`..`0x4c6c7a`); `frame_hook` is not gated by the battle (`0x474c54`..`0x474c64`), so it lands; that tentacle never sinks. | **Observed**; cosmetic |
| Tentacle removed from the active list while a victim flies | `frame_hook` runs only for type-9 objects on `obj_active_list` (`0x474c46`..`0x474c70`); `check_stage` clears the tentacle's `+0x84` bit 0 after the death move (`0x4c7487`..`0x4c748f`). | *Inferred* small risk: body frozen in the air |
| Level end / save / load | Exits open and saving is allowed only after the golems die (`level_trigger_set_exit` `0x4ee1bd`; `level_script_event_active` set at `0x4ee37e`, cleared `0x4ee1d9`, read by `savegame_location_allowed` `0x4d4a50`). So save/exit with a held victim only follows the battle-end slip. `tentacle_beast_unload` clears only `0x248c7ac`. | **Observed**; *Inferred*: whether flags 0x1028 survive a save is unknown |
| Cutscene | None during the fight. | **Observed** in `ls_sewerboss_frame` |
| Multiplayer | Only the release sends packets. A client that gets SWI_BSS 0x37 would run `ai_release` with no victim and crash at `0x4c6b53`. Sewer boss in MP is unlikely. | **Observed** packets; *Inferred* client effect |

## 5. Slips and risks (all only reachable once the grab is enabled)

1. Grab chance is `frand() < 0.0` (`0x4c666d`..`0x4c667f`). **Observed**. Do not patch `0x57eb84`: it is the shared
   0.0 constant of the whole program.
2. The grab pick is cleared (`0x4c66a5`) and overwritten by the swipe search (`0x4c67de`). **Observed**
3. `grab_ready` is never read; no "one grab at a time", no cooldown, no "not the last member". **Observed**
4. A whiffed grab (no target, target without "flail", e.g. a summon) leaves grab_ready 0 and the grab timer invalid
   (`0x4c6837`..`0x4c6846`), and only a landing re-arms it (`0x4c7c28`). With the gate wired in, grabs would stop for
   the rest of the fight. **Observed**; *Inferred* effect
5. No range test at trigger 7, no alive / flag-0x88 test on the victim. **Observed**
6. Hold tick heals +3 (`push 0x40400000` at `0x4c714b`). **Observed**
7. Release HP mixes victim and tentacle HP (`0x4c6912` vs `0x4c74b1`). **Observed**
8. `ai_release` uses the victim without a NULL test (`0x4c6b53`). Also `character_move_trigger_fired` returns -1
   when the current move lacks the trigger, and both callers treat -1 as fired (`test eax, eax ; je`,
   `0x4c689a`, `0x4c6acc`). **Observed** (`0x461777`, `0x4617da`)
9. Leadership goes to the first other party entry, dead or alive, even when the victim is not the leader (`0x4c69ac`..`0x4c69c7`). **Observed**
10. `charinfo_find_move(tentacle, 14, 0x8d/0x93/0x95)` results used without NULL tests (`0x4c697d`, `0x4c6bcf`,
    `0x4c6bf3`). Retail data has all three. **Observed**
11. Victim death while held not handled (section 4.6). **Observed**
12. Battle end while held leaves the victim stuck (section 4.6). **Observed** code path
13. Thrown body has no navigable test or flight timeout; over water it can land off the nav mesh or fall without end
    (ray needs flag 0x40, `0x4c7aa4`). **Observed**; *Inferred* effect
14. Squeeze point uses identity orientation (`0x4c6f26`..`0x4c6f6e`), swipe and pickup use the load-time
    orientation; all are fixed at load. If tentacles turn, the points go stale. *Inferred*
15. G251 (4 hit slots, `0x4c7870`..`0x4c7887`): independent of the grab. A held victim has flag 8 and is skipped by
    `tentacle_collect_area_hits`, so the grab makes G251 slightly less likely. A fifth hit writes `+0xa4`; the death
    path then misfires. Ship the G251 bound with this switch. **Observed** code; *Inferred* interplay
16. Unused bookkeeping: `+0x104` (hold time, odd 7.5 -> 11.25 bump at `0x4c718e`..`0x4c71a1`), `+0x108`, `+0x10c`,
    `+0xac`, `+0xc0`, `0x248c7a0`. Hints of a larger plan (time-based throw?), not needed. **Observed**
17. Victim state anims are not restored at landing (only the throwndown move plays). The Luminar does the same and
    its grab is observed working, so *Inferred* fine; check in game.

## 6. Why it looks disabled on purpose (*Inferred*)
- The grab branch is complete and its data is present, but two separate edits kill it: the threshold is the
  literal 0.0 and the store was removed. A typo would not do both.
- The gate globals are maintained but unread, as if the reader was removed with the store.
- The dead code has untested slips (heal tick, HP mix-up, battle-end hang). Likely it was turned off when it
  misbehaved, perhaps because of the battle-end hang (the fight ends on the golems, not the tentacles).

## 7. Enable plan for SummonerFix

### 7.1 Switches (new section, off by default)
```
[Experimental]
TentacleGrab=0          ; G250: the tentacles can pick up and throw a party member
TentacleGrabChance=10   ; percent per tentacle decision while a grab is allowed
```
Grab chance: no constant in the code or data gives it. Hints: the squeeze keeps its 30 % (`0x581970`); the
Luminar grabs once per 30 s cooldown (`0x4c94e2`, 30.0); each tentacle decides every 2.5-5 s (`0x4c6592`).
Four tentacles make about one decision per second, so 10 % gives a grab about 10 s after the gate opens; with the
30 s hold and the 10 s landing cooldown, about one grab per 50 s. Test the feel at 10 and 20.

### 7.2 Quick proof of concept (agent mode only, not the shipped fix)
- `pokecode 4c67d6 a4`: the swipe store `mov edx, [0x248c794]` (`8B 15 94 C7 48 02` at `0x4c67d4`) becomes
  `mov edx, [0x248c7a4]`. Every swipe decision becomes a grab of the first candidate within reach. Undo: `pokecode 4c67d6 94`.
- `pokecode 4c714f c0`: the tick `push 0x40400000` (`68 00 00 40 40` at `0x4c714b`) becomes `push 0xC0400000` (-3.0).
  Undo: `pokecode 4c714f 40`.

### 7.3 Shipped patch (C stubs, like the MountedRiders entry jump)
| # | Site | Original bytes | New | Purpose |
|---|---|---|---|---|
| 1 | `0x4c6520` select entry | `83 EC 34 53 55` | `E9 rel32` -> `TentacleSelectAction` (`__fastcall(ai_state*)`, returns int, no stack args) | the original logic plus the grab branch |
| 2 | `0x4c6850` ai_grab entry | `53 56 8B 35 CC` | `E9 rel32` -> `TentacleAiGrab` (`__fastcall(ai, edx, Character* victim)`, pops 4) | guarded grab |
| 3 | `0x4c714b` tick | `68 00 00 40 40` | `68 00 00 40 C0` | damage 3 instead of heal (slip 6) |
| 4 | `0x4c6a80` ai_release entry | `83 EC 48 56 57` | `E9 rel32` -> `TentacleAiRelease` (`__fastcall(ai)`) | NULL / dead tests (slip 8) |
| 5 | `0x4c6c50` begin_sink entry | `53 57 8B 7C 24` | `E9 rel32` -> `TentacleBeginSink` (cdecl `(record*, int ms)`) | battle end: drop a held or flying victim now, then sink (slip 12) |
| 6 | `0x4c7960` frame_hook entry | `81 EC A8 00 00 00` | `E9 rel32` + `90` -> `TentacleFrameHook` (cdecl `(Character*)`) | dead-victim drop, nav snap, flight timeout (slips 11, 13) |
| 7 | G251 | see the G251 fix | bound `num_hits` at 4 | slip 15 |

If an in-place fix for slip 7 is preferred over stub 2: replace the 24 bytes at `0x4c690c`
`8B 0D A8 C7 48 02 D9 86 F8 05 00 00 D8 0D 04 0E 58 00 D9 99 E0 00 00 00` with
`8B C8 8B 57 4C D9 82 F8 05 00 00 D8 0D 04 0E 58 00 D9 99 E0 00 00 00 90`
(`mov ecx, eax` (eax is still the record from `0x4c68f6`), `mov edx, [edi+0x4c]`, `fld [edx+0x5f8]`,
`fmul [0x580e04]`, `fstp [ecx+0xe0]`, `nop`). Release then happens when the tentacle loses a quarter of its HP.

Stub behaviour:
- **TentacleSelectAction**: copy the original (busy checks, think timer, candidates). Before `frand` for the
  ladder: if `TentacleGrab` and `grab_ready` (`0x248c770`) and not multiplayer (`game_flags 0x5fbfd8` bit 2) and
  `frand() < chance`: pick the nearest candidate whose info has "flail" (14, 0x8c) and "throwndown" (8, 1), is not
  0x88, and is within 3.0 (as the squeeze, `0x57ef1c`) of the pickup point, read live from "$tag-26-pickup"
  (`character_find_mesh_tag` `0x540f70`, `get_bone_world_transform` `0x51b3f0`); fall back to edge distance < reach.
  Store `+0x7c = move_86`, `+0x8c = handle`, set grab_ready 0 here, return 3. Otherwise the original ladder.
- **TentacleAiGrab**: as the original, plus at trigger 7 (`== 1` only): victim non-NULL, not 0x88, has flail, edge
  distance < reach + 1. `+0xe0` = tentacle HP x 0.75. Move the lead only if the victim is the leader / cur_character
  (`0x5fcc88`), to the first other member without 0x88. On a whiff (move finished, no victim held) arm the grab timer
  10 s so the gate re-opens.
- **TentacleAiRelease**: as the original, trigger `== 1` only; if the victim is NULL or 0x88, clear `+0xbc`, keep
  `+0xcc = -1`, restore the tentacle anims, `ai_go_idle`.
- **TentacleBeginSink**: if `+0xbc != -1` or `+0xcc != -1`: `TentacleDrop(record)` (below), then run the original
  sink part (timer, flag 4, party goals).
- **TentacleFrameHook**: if the victim is NULL or has 0x80 -> `TentacleDrop`. Else the original. During flight add
  the Luminar test (`pf_point_is_navigable` `0x4a5dc0` with `info+0x90` and `+0x68c`, else
  `pf_find_nearest_navigable_point` `0x49e8c0` and land) and a 3 s flight timeout that lands at the nearest
  navigable point.
- **TentacleDrop(record)**: victim/thrown character: flags &= ~0x1028, if not 0x80 then `ai_go_idle`, face level
  (`0x511200`), nearest navigable point, `character_find_ground` (`0x455fb0`), throwndown if alive; record
  `+0xbc = +0xcc = -1`, tick timer invalid, tentacle anims back to 0x93 / 0x95 (`0x45a120`), grab timer 10 s.

Game functions the stubs call (all **Observed** addresses): `living_from_handle 0x46b1a0`, `frand 0x5426e0`,
`living_combat_is_busy 0x468740`, `character_move_playing 0x461450`, `GameTimer valid/elapsed/invalidate/SetTimeout/
SetRandomTimeout 0x501990/0x501910/0x501900/0x501880/0x5018d0`, `distance 0x524810`,
`character_distance_between_edges 0x454330`, `charinfo_find_move 0x446cb0`, `character_find_move 0x461370`,
`character_play_move 0x460f20`, `character_move_finished 0x4614d0`, `character_move_trigger_fired 0x461740`,
`ai_go_idle 0x4738c0`, `ai_set_mode 0x473320`, `character_instance__clear_state_anims 0x51b100`,
`set_state_weight 0x51ae90`, `character_set_flag_2_4 0x454670`, `character_clear_flag_2_4 0x4546b0`,
`party_set_leader 0x433d60`, `camera_set_target 0x478ff0` (this `0x60a018`), `lobj_raycast_all 0x4af2c0`,
`character_add_hp 0x450850`. Lists: `party_list 0x5fc350`, `living_list 0x8eca60` (first `0x8ecd78`),
`reported_frametime 0x5a5740`, `ai_move_flags 0x22e48cc`.

## 8. Agent-mode test plan (when the game is free)

Character offsets: pos `+0x10`, handle `+0x80`, flags `+0x320`, HP `+0x5f8`. Record bases: `0x248c328`,
`0x248c438`, `0x248c548`, `0x248c658`. Victim fields: `+0xbc` at `0x248c3e4`, `0x248c4f4`, `0x248c604`,
`0x248c714`; thrown `+0xcc` at `0x248c3f4` ...; release HP `+0xe0`.

1. Load a save with at least two heroes. `level Sewerboss sewerboss`. `peek 248c7a4 4` is non-zero (move 0x86
   loaded). `peek 248c76c 1` is 0.
2. `flag sewer_found_tb 0` if needed, `navgoto $tbintro`; TB-Intro plays; after it `peek 248c76c 1` is 1.
   `immortal` via `[ConsoleKeys]` keeps the party up.
3. Baseline 60 s near a tentacle (`npcgoto Tentacle Beast#$npc058`): the four `+0xbc` stay `ff ff ff ff`.
4. PoC: `pokecode 4c67d6 a4`. Capture every 300 ms: grab move, victim in "flail" at the tip, `peek 5fcc88 4`
   (leader moved), victim `+0x320` has 0x1028, `+0xe0` vs 0.75 x victim HP. Read victim HP every 6 s: +3 (slip 6).
   Wait 30 s or hit the tentacle: release move, throw arc, landing, "throwndown", flags cleared, grab timer.
5. `pokecode 4c714f c0`: HP now drops 3 every 6 s.
6. Early cases (with the PoC on):
   - tentacle death while holding: `npckill Tentacle Beast#$npc058` -> release, throw, then death move.
   - battle end while holding: `npckill Broken Brass Golem#boss1`, `...#boss2`, `Iron Golem#boss` -> expect the
     stuck victim (slip 12). Then try the exit and a save; reload and check the hero.
   - victim death while held: `callt 450850 VICTIM 0xC47A0000 TENTACLE_HANDLE 1 0` (-1000 HP) with `immortal` off
     -> body stays on the bone (slip 11); watch landing.
   - throw over water: stand so the swipe point is over the water; watch the landing (slip 13).
   - summon as target: summon, let it be grabbed -> whiff (slip 4).
7. Undo the pokes. Build with `[Experimental] TentacleGrab=1`, `TentacleGrabChance=100`, repeat 4-6: no hang, no
   heal, release at 25 % tentacle HP lost, victim dropped at battle end, landing on the nav mesh. Then chance 10:
   play 3 minutes, count grabs, check only one at a time and none with one hero left.
8. `TentacleGrab=0`: bytes at all sites equal the originals in the table (no change to retail).
