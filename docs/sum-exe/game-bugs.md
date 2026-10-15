---
title: Bugs in the original game
parent: Sum.exe
nav_order: 6.95
---

# Bugs in the original game
{: .no_toc }

1. TOC
{:toc}

**Summary:** This page lists the defects in `Sum.exe` v1.40 that we have found so far: 36 in all.
Three of them hurt every player on a modern PC: double game speed, intros that skip after about 25 days
of Windows uptime, and a crash on Start New Game when no audio device is connected. SummonerFix fixes all
three. Most of the rest are small UI slips. Others are latent: they need modded data or an unusual
setup before they show. Every entry below was checked again in the disassembly
(`node tools/asm.mjs fn <name>`) on 2026-10-15.

{: .note }
**Draft.** Severity says what the player gets when the bug triggers. **Observed** means we read the
deciding code, or saw the bug at runtime where the entry says so. *Inferred* means the in-game effect
comes from reading the code and has not been tested. "Latent" means we know of no trigger in retail play.

## Overview

| Severity | Live | Latent | Fixed by SummonerFix |
|---|---|---|---|
| Crash or hang | 3 | 7 | 2 |
| Gameplay | 9 | 0 | 3 |
| Cosmetic | 11 | 0 | 1 |
| Invisible | 3 | 3 | 0 |

Two bugs only matter with the OpenGL renderer. The game never selects that renderer, but SummonerFix
does (see [SummonerFix](../summonerfix/index.md)).

## Crash or hang

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| C1 | Start New Game crashes when Windows has no audio output device | `snd_init` `0x5205b0`, `ds_channel_release` `0x55c5b0` | **Observed**, reproduced in agent mode | Fixed (`bugfix.c`) |
| C2 | Crash at start-up when the 3D primary sound buffer cannot be created | `ds_init` `0x55c1a0` | **Observed** code, *inferred* crash | No |
| C3 | "Failed to allocate 1073741824 bytes" with the OpenGL renderer | `GlTextureInit` `0x54e690` | **Observed** at runtime | Fixed |
| C4 | Latent: more than 300 lamp glows corrupt memory | `lamp_glow_add` `0x4c0a90` | **Observed** code, *inferred* effect | No |
| C5 | Latent: a cutscene with more than 175 living characters overwrites memory | `cutscene_play` `0x4361d0` | **Observed** code, *inferred* effect | No |
| C6 | Latent: more than 50 command-line arguments overwrite memory | `cmdline_split` `0x53e060` | **Observed** code, *inferred* effect | No |
| C7 | Latent: a UI window with "+Always a selection" can hang the game | `ui_window__process` `0x52bc20` | **Observed** code, *inferred* hang | No |
| C8 | Latent: a level with more than 15 shopkeepers or 1000 shop entries overwrites memory | `level_parse_shopkeeper` `0x4cc4d0` | **Observed** code, *inferred* effect | No |
| C9 | Latent: a surface name longer than about 56 characters in the surfaces table overflows the stack | `surfaces_parse_table` `0x468e40` | **Observed** code, *inferred* effect | No |
| C10 | Latent: a dialogue topic that removes two initial topics, the first of them the current hail topic, hangs the game | `conversation_apply_topic_effects` `0x4d7eb0` | **Observed** code and retail data, *inferred* hang | No |

**C1. No `snd_disabled` after a failed `ds_init`.** When `ds_init` fails, `snd_init` returns -1 without
setting `snd_disabled` (`jne 0x520672` at `0x520644` jumps past `mov byte [0x2cbf480], 1`). Later
`ds_channel_release` enters a stream lock that was never initialised. See [Sound](sound.md#start-up) and
[SummonerFix](../summonerfix/index.md#fixes).

**C2. Retry through a NULL pointer.** When the 3D primary buffer fails, `ds_init` releases DirectSound and
stores 0 (`mov [0x2f79e3c], eax` with `eax = 0` at `0x55c2eb`). It then calls the next method through
that pointer (`mov edx, [eax]` at `0x55c2fa`). Without DS3D, the same failure goes on to `SetFormat` on the
primary buffer that was never created (`jne 0x55c337` at `0x55c2f0`). See [Sound](sound.md#start-up).

**C3. Texture buffer sized from the driver.** The OpenGL init allocates `size x size x 4` bytes, where
`size` is `GL_MAX_TEXTURE_SIZE` (`imul ecx, eax; shl ecx, 2` at `0x54e71c`). A driver that reports 16384
asks for 1 GB. See [SummonerFix](../summonerfix/index.md#opengl-failed-to-allocate-1073741824-bytes).

**C4. No bound in `lamp_glow_add`.** The array holds 300 records of 0x18 bytes (`0x24893e8`..`0x248b008`).
The function writes at `lamp_glow_count` with no compare. Record 301 lands on the count itself.
`fx_create_instance` (`0x48beb8`) adds glows for every effect instance with flag `0x40`. They are only
cleared on a level change. See [Effects](effects.md#lamp-glows).

**C5. No bound on the saved actors.** `cutscene_play` writes one 8-byte entry per living character into
`cutscene_saved_actors` (`0x5fcce8`, 175 entries) and increments `cutscene_num_saved_actors` (`0x436556`)
without a compare. See [Cutscenes](cutscenes.md#what-cutscene_play-changes).

**C6. No limit in `cmdline_split`.** Each argument goes to `cmdline_args[cmdline_num_args++]`
(`0x53e17b`, `0x53e1a9`) with no test against 50. Argument 51 overwrites `cmdline_params` (`0x2dc7038`).

**C7. A loop that never advances.** With window flag `0x10` and no selection, the loop at
`0x52bcd8`..`0x52bce5` calls the first gadget's "selectable" method. It repeats while that returns 0,
but `edi` never moves to the next gadget. The vsdk UI tables are used by the
[script editor](script-editor.md), so retail play does not reach this.

**C8. Shop tables.** The parser stores into `level_shopkeepers` (`0x2490ca0`, 15 records) and
`level_shop_items` (`0x2491ac0`, 1000 entries) without checking either count. The retail tables fit. See
[Shops and loot](shops-loot.md#where-shops-come-from).

**C9. Wrong buffer size.** The second `ParseString` call passes size 0x100 (`push 0x100` at `0x468f68`)
for a stack buffer at `esp+0xa4`. The next local starts at `esp+0xdc`, so the buffer has 0x38 bytes.

**C10. Inner loop steps the wrong list.** For each `+Remove Initial Topic:` entry that removes the speaker's
current hail topic, the game picks a new hail topic from the `+Add Initial Topic:` list. That inner loop advances
with the *outer* entry's next pointer (`mov ecx, [eax+8]` at `0x4d7fcd`, with `eax` the remove entry). It ends only
when the remove entry is the last in its list. The parser prepends entries (`conversation_topic__push_list24`
`0x4fecb0`), so the list runs in reverse file order. Four retail topics remove two initial topics: "I have your
book!" in `lenele1ab_dlg.tbl`, its `_v2`/`_v3` copies and `Miniquests_dlg.tbl`. In each, the hail topic in effect
at that point (`"Hail"#2`) is the first line in the file, so it is last in the list, and the loop exits after one
pass. A modded table that swaps the two lines would hang. Found while writing
[`conversation.cpp`](../../src/decomp/game/conversation.cpp).

## Gameplay

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| G1 | The game runs at double speed on modern PCs | `FrameTimer` `0x501510` | **Observed** at runtime | Fixed |
| G2 | The game runs many times too fast when the QPC frequency is 4.29 GHz or more | `timer_init` `0x532840` | **Observed** code | No (Timeslip's patch fixes it) |
| G3 | Cutscenes skip almost at once after about 24.9 days of Windows uptime | `cutscene_sequence_update` `0x436a90` | **Observed**, reproduced | Fixed |
| G4 | With the save list scrolled, saving on the top row overwrites a save without asking | `saveload_try_save` `0x426320` | **Observed** code, *inferred* effect | No |
| G5 | Buying with a full inventory takes the gold and gives no item | `buysell_buy_item` `0x4057f0` | **Observed** code, *inferred* effect | No |
| G6 | At game stages 4 to 8, the Zane grassland encounter gets no extra chance | `0x4fc4b0` (grass picker) | **Observed** code, *inferred* effect | No |
| G7 | If a cutscene's data fails to load, input stays locked and the music stays off | `cutscene_play` `0x4361d0` | **Observed** code, *inferred* effect | No |
| G8 | When any party record is removed (for example a summon dies), control may jump to the first member | `party_remove_player` `0x4335a0` | **Observed** code, *inferred* effect | No |
| G9 | Alt+Tab does not work: the game takes the focus straight back | `os_wndproc` `0x530ff0` | **Observed** at runtime | Fixed |

**G1. Frame limiter.** `FrameTimer` stores `last = now` (`0x501577`) before it sleeps (`0x5015bc`). The
next frame counts the same sleep again. See
[Timing bugs](timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed).

**G2. QPC frequency.** `timer_init` keeps only the low 32 bits of `QueryPerformanceFrequency`
(`mov ecx, [esp+8]` at `0x532859`, stored at `0x532868`). Modern Windows reports 10 MHz, so most PCs are
not affected. See [Timing bugs](timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug).

**G3. Signed subtract.** The start time is subtracted as a signed 32-bit value (`fisub dword` at
`0x436afc`). After 2^31 ms of uptime every shot looks overdue. See
[Timing bugs](timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime).

**G4. Row index instead of list index.** The overwrite test reads
`saveload_entries[selected_row].level` (`cmp [edx+0x5e3640], -1` at `0x426352`) without adding
`saveload_scroll_pos`. Entry 0 is the `<New Savegame>` row, so when the list is scrolled the top visible
row skips the "Overwrite existing savegame?" box. `saveload_save_to_slot` does add the scroll position
(`0x4263a0`), so it writes over the real save. See [Interface screens](interface.md).

**G5. Gold first, item second.** `buysell_buy_item` subtracts the price (`0x4058b7`) and then calls
`party_inventory_add_type`. That function returns without adding a new item type when the inventory
has 0x209 entries (`cmp edx, 0x209` at `0x46b9e4`). Hard to reach. See
[Shops and loot](shops-loot.md#buying-and-selling).

**G6. Missing jump.** The Zane case copies `rand-grassland01` and script `_v3` (`0x4fc9d8`..`0x4fca20`)
and then falls through into the general rolls at `0x4fca22`, which overwrite both names. See
[World map](worldmap.md#encounter-table).

**G7. Early return.** `cutscene_play` locks input and stops the music, then returns when
`cutscene_load_data` fails (`jmp 0x4365d9` at `0x43634c`) without setting `game_flags` bit `0x20`.
`cutscene_cleanup` does nothing while that bit is clear (`0x436630`), so nothing is restored. Retail
files are all present, so this needs a missing or broken `.csc` file. See
[Cutscenes](cutscenes.md#what-cutscene_play-changes).

**G8. Leader re-picked every time.** After unlinking any record, single player sets the leader to the
first living member of `party_list` (`call party_set_leader` at `0x433638`), whether or not the removed
record led. In multiplayer it calls `party_set_leader` for every local member (no break, `0x433623`).
The intent is unknown. See [Party](party.md#leaving-and-coming-back).

**G9. Focus loss re-activates the window.** The `WM_ACTIVATE` handler calls `SetWindowPos` with flags 0
on losing focus too (`0x5310e9`), and flags 0 include activation. See
[Controls](controls.md#switching-away-from-the-game-alttab).

## Cosmetic

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| K1 | A hot button used with a number key lights up for one frame instead of 100 ms | `hud_frame` `0x40cf40` | **Observed** code | No |
| K2 | In the multiplayer character screen, the info panel's up arrow scrolls down | `mp_charselect_button_action` `0x4074e0` | **Observed** code | No |
| K3 | In a shop's sell list, the category buttons jump to the wrong row | `buysell_category_first_index` `0x404f70` | **Observed** code | No |
| K4 | In the inventory's identify mode, the category buttons jump to the wrong row | `inventory_category_first_index` `0x4153d0` | **Observed** code | No |
| K5 | With the save list scrolled, the top row shows the wrong thumbnail | `saveload_load_thumbnail` `0x425c10`, `saveload_draw_thumbnail` `0x426fd0` | **Observed** code, *inferred* effect | No |
| K6 | Adding a 33rd label to a level's map replaces the first label | `automap_label_alloc` `0x430230` | **Observed** code, *inferred* effect | No |
| K7 | A character missing from the font is drawn as a garbage glyph | `gr_string` `0x508c60` | **Observed** code, *inferred* effect | No |
| K8 | The credits line count can be off when a long line wraps | `credits_parse_table` `0x408620` | **Observed** code, *inferred* effect | No |
| K9 | The player count and level limits on the Create Game screen are a few pixels off centre | `gamecreate_draw_player_count` `0x40a4f0` | **Observed** code | No |
| K10 | Windowed mode stretches the 3D view to the desktop's aspect ratio | `gr_init` `0x505e00` | **Observed** code and at runtime | Fixed |
| K11 | The dialogue camera's side offset depends on an uninitialised value | `conversation_camera_compute_shot` `0x4d8f50` | **Observed** code, *inferred* effect | No |

**K1. Wrong timer test.** `hud_frame` clears `hud_hot_button_used` when the timer is valid *or* elapsed
(`jne 0x40cf74` after `GameTimer__valid` at `0x40cf5d`). A running timer is valid, so the highlight that
`hud_handle_keys` sets for 100 ms (`0x40dd1c`) is gone on the next frame. It was probably meant to be
`!valid() || elapsed()`.

**K2. Two buttons, one case.** The case table at `0x407630` maps button 31 (up) and button 33 (down) to the
same body, `mp_charselect_scroll_down(2)` (`0x4075d0`). There is no `scroll_up(2)` case. See
[Multiplayer](multiplayer.md).

**K3, K4. Index into the wrong list.** In sell mode the shop list shows only owned items, in
party-inventory order (`buysell_get_list_item` `0x40497d`). The jump returns an index into the shop's buy list
(`0x404f98`..`0x404fcc`). In identify mode the inventory shows only unidentified items, but
`inventory_category_first_index` returns a party-inventory index (`mov eax, esi` at `0x41543e`).

**K5. Row index again.** In save mode `saveload_load_thumbnail` loads no file for row 0 (`test ebx, ebx;
jle` at `0x425c65`). `saveload_draw_thumbnail` then shows the current scene for row 0 (`0x426fe5`). Both
use the row, not scroll + row, so a scrolled top row shows the scene instead of its save.

**K6. Full returns 0.** With 32 labels, `automap_label_alloc` returns 0 (`xor eax, eax` at `0x430245`)
instead of -1. `automap_add_label_at_mouse` tests for a negative result (`jl` at `0x403180`), so it
writes over label 0.

**K7. No check for -1.** `gr_font_get_char` returns -1 for a character outside the font (`or eax, -1` at
`0x508503`). `gr_string` uses the result as an index without a test (`mov edx, [edx+4*eax]` at `0x508d21`).

**K8. Wrong buffer.** For wrapped lines the `&` (right column) test reads `credits_lines[start]`, the
output buffer at the input offset (`cmp byte [edx+esi], 0x26` at `0x408862`). It was probably meant to
read `line[start]`. Only the line count changes.

**K9. Height used twice.** `gr_get_string_size` returns the width and the height, but both centring
offsets read the height (`mov eax, [esp+0x30]` at `0x40a541` and `mov eax, [esp+0x34]` at `0x40a553`
are the same local after one push). The width is never read. `gamecreate_draw_limits` (`0x40a570`) does
the same.

**K10. Desktop aspect.** In windowed mode `gr_init` takes the aspect ratio from
`GetSystemMetrics(SM_CXSCREEN) / GetSystemMetrics(SM_CYSCREEN)` (`0x505ef5`..`0x505f0f`), not from the
window. Only windowed mode is affected, and the retail game runs fullscreen. See
[SummonerFix](../summonerfix/index.md#fixes).

**K11. Missing component.** The shot code builds a side vector from the speaker-to-player direction. It writes only
its x and z (`[esp+0x54]` and `[esp+0x5c]`, `0x4d8fcb`..`0x4d8fd7`), never y (`[esp+0x58]`), and then normalises it
(`vector3__normalize_safe` at `0x4d8fe4`). Whatever the stack held decides how much of the camera offset goes up
instead of sideways. *Inferred:* the value is often the same from call to call, so this may not show.

## Invisible

| # | Effect | Function | Evidence |
|---|---|---|---|
| V1 | Sets the time of every trigger slot whose name does *not* match | `character_set_skeleton_trigger_time` `0x540d70` | **Observed**; no caller |
| V2 | Only 4 of the 6 large streamed-effect slots are used | `effects_alloc_stream_slot` `0x51c520` | **Observed** code, *inferred* effect |
| V3 | Latent: "Alt+" in a UI table hotkey gives Shift | `ui_parse_hotkey` `0x528430` | **Observed** code |
| V4 | Latent: a TGA with an image ID field is read from the wrong offset | `tga_read_bitmap` (`File__seek` at `0x5556f7`) | **Observed** code |
| V5 | A speed change on one animation fills all 5 speed slots | `character_instance__start_anim` `0x51aee0` | **Observed** code, *inferred* effect |
| V6 | Latent: the ring of small streamed-effect slots restarts at slot 0, 1, 4 or 5 only | `effects_find_stream_slot` `0x51c5c0` | **Observed** code |

**V1.** The `strcmp` result decides the store the wrong way round (`je 0x540dde` at `0x540dbe` skips a
match). Nothing calls the function, so it never runs.

**V2, V6. Mask 5 instead of a modulo.** `effects_alloc_stream_slot` indexes its 6-slot ring with
`and eax, 5` (`0x51c533`), which only gives 0, 1, 4 and 5. Slots 2 and 3 are never used, so large
streamed effects are evicted sooner than intended. `effects_find_stream_slot` scans its 8 slots with
`and eax, 7` but stores the next start with `and edx, 5` (`0x51c613`), so all 8 slots are still used.

**V3.** "Alt" sets the same flag as "Shift" (`mov bl, 1` at `0x5284a1` and `0x528481`). Only the vsdk UI
tables of the [script editor](script-editor.md) use this parser.

**V4.** With a non-zero ID length, the reader seeks to that length from the start of the file (origin 0)
instead of past the 18-byte header. A seek is only made when the length is non-zero (`jbe` at `0x5556e4`).
*Inferred:* the retail TGAs have no ID field.

**V5.** With a speed other than 1.0, the loop at `0x51af53`..`0x51af65` stores `{anim, speed}` in every free
one of the 5 slots, with no break after the first.

## Claims that did not hold

| Claim | Where it was made | What the code does |
|---|---|---|
| A load sets `level_time` from header field `0xa8` (always 0) | [Savegame](savegame.md#header-observed) note | `savegame_read` pushes `&level_time` as the **fourth** argument (`push 0x60a654` at `0x4d2627`). `savegame_read_header` copies field `0xa8` into the third (a local, `[esp+0x20]` at `0x4d2ad7`) and field `0xac`, the saved time, into `&level_time` (`[esp+0x24]` at `0x4d2aea`). No swap; the savegame page needs a correction |
| `hud_portrait_get_color` case 3 computes red from green | HUD decompilation notes | True (`0x40f987`, `0x40f98b`), but `color_pulse_scale` (`0x414ae0`) scales three equal 1.0 inputs the same way, so the result does not change. Not counted |
| Loot gold is lost when the inventory is full | earlier notes | The gold loss is in the shop **buy** path (G5), not in loot pickup |
| Level select button 31 scrolls the wrong way | earlier notes | The button is in the multiplayer **character** select screen (K2) |
| Only slots 0, 1, 4 and 5 of the 8-entry stream ring are reachable | [`src/decomp` notes](../../src/decomp/README.md) | True for the 6-slot ring (V2). The 8-slot ring reaches all 8 (V6) |

Not counted as bugs: `camera_key_extra_axis` (`0x4348a0`) always returns 0 and `camera_cut_to_view_stub`
(`0x479430`) is empty. They look like cut features ([Camera](camera.md)). Other oddities in the decompilation
notes have not been checked again and are left out for now: the pickup scrollbar thumb height, hit rows vs
drawn rows in the multiplayer level select, `item_page_out_all` slot 18, the registry helpers, font slot
reuse and kerning, `ui_window` insert order and mask bitmaps, `ui_load_table`, `ui_parse_button`,
`character_instance__reset_anims` (25 weights), the targa 16-bit converter, and the one-pixel lines in
`ui_container::draw`.

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Where |
|---|---|
| C1 `snd_disabled` skipped | `snd_init` `0x520642`..`0x52066b`; `ds_channel_release` `0x55c5db`..`0x55c5e7` |
| C2 NULL retry | `ds_init` `0x55c2d5`..`0x55c314` |
| C3 texture buffer size | `0x54e70b`..`0x54e722` |
| C4 lamp glows | `lamp_glow_add` `0x4c0a90`..`0x4c0ae4`; callers `0x4868b8`, `0x48beb8` |
| C5 saved actors | `cutscene_play` `0x43646c`..`0x43655f` |
| C6 arguments | `cmdline_split` `0x53e16b`..`0x53e1a9` |
| C7 loop | `ui_window__process` `0x52bcc8`..`0x52bce9` |
| C8 shop tables | `level_parse_shopkeeper` `0x4cc4f5`..`0x4cc755` |
| C9 buffer size | `surfaces_parse_table` `0x468f35`..`0x468f72` |
| C10 topic loop | `conversation_apply_topic_effects` `0x4d7f9a`..`0x4d7fd9`; `conversation_topic__push_list24` `0x4fecb0` |
| G1 limiter | `FrameTimer` `0x501540`, `0x501577`, `0x5015bc` |
| G2 QPC | `timer_init` `0x532853`..`0x532868` |
| G3 cutscene timer | `cutscene_sequence_update` `0x436ad9`..`0x436aff` |
| G4 overwrite test | `saveload_try_save` `0x426340`..`0x42635b`; `saveload_save_to_slot` `0x426395`..`0x4263a0` |
| G5 buy | `buysell_buy_item` `0x4058aa`..`0x4058c4`; `party_inventory_add_type` `0x46b9e4` |
| G6 Zane | `0x4fc99c`..`0x4fca22` |
| G7 load failure | `cutscene_play` `0x4362d7`..`0x43634c`; `cutscene_cleanup` `0x436626`..`0x43663b` |
| G8 leader | `party_remove_player` `0x4335d9`..`0x43363d` |
| G9 focus | `os_wndproc` `0x5310a9`..`0x5310e9` |
| K1 hot-button timer | `hud_frame` `0x40cf58`..`0x40cf7e`; `hud_handle_keys` `0x40dd17`; `GameTimer__valid` `0x501990` |
| K2 arrow | case bytes `0x407630`, jump table `0x4075f0` |
| K3 sell list | `buysell_category_first_index` `0x404f98`; `buysell_get_list_item` `0x40497d` |
| K4 identify mode | `inventory_category_first_index` `0x4153dc`..`0x41543e` |
| K5 thumbnail | `saveload_load_thumbnail` `0x425c61`..`0x425c67`; `saveload_draw_thumbnail` `0x426fda`..`0x426fe9` |
| K6 labels | `automap_label_alloc` `0x430236`..`0x430245`; `automap_add_label_at_mouse` `0x40317e` |
| K7 glyph | `gr_font_get_char` `0x5084ee`..`0x508503`; `gr_string` `0x508d0e`..`0x508d36` |
| K8 credits | `credits_parse_table` `0x408858`..`0x408868` |
| K9 centring | `gamecreate_draw_player_count` `0x40a515`..`0x40a564` |
| K10 aspect | `gr_init` `0x505eee`..`0x505f0f` |
| K11 side vector | `conversation_camera_compute_shot` `0x4d8fbd`..`0x4d8fe4` |
| V1 trigger time | `character_set_skeleton_trigger_time` `0x540dbc`..`0x540ddb` |
| V2, V6 masks | `effects_alloc_stream_slot` `0x51c533`, `0x51c573`; `effects_find_stream_slot` `0x51c5d3`, `0x51c613` |
| V3 hotkey | `ui_parse_hotkey` `0x52847a`..`0x5284a3` |
| V4 seek | `tga_read_bitmap` `0x5556de`..`0x5556f7`; `File__seek` `0x51492f` (origin 0 = absolute) |
| V5 speed slots | `character_instance__start_anim` `0x51af3d`..`0x51af65` |
| `level_time` claim | `savegame_read` `0x4d261d`..`0x4d263f`; `savegame_read_header` `0x4d2ac6`..`0x4d2aea` |
