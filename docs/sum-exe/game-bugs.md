---
title: Bugs in the original game
parent: Sum.exe
nav_order: 6.95
---

# Bugs in the original game
{: .no_toc }

1. TOC
{:toc}

**Summary:** This page lists the defects in `Sum.exe` v1.40 that we have found so far: 128 in all.
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
| Crash or hang | 3 | 23 | 2 |
| Gameplay | 21 | 10 | 3 |
| Cosmetic | 27 | 8 | 2 |
| Invisible | 12 | 24 | 0 |

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
| C11 | Latent: more than 1000 item objects in a level overwrite the object list | `item_obj_spawn` `0x46a1b0` | **Observed** code, *inferred* effect | No |
| C12 | Latent: more than 1224 live items crash on the next item created | `item_alloc` `0x44b380` | **Observed** code, *inferred* crash | No |
| C13 | Latent: an object still in the active list when the game returns to the title hangs or corrupts the heap | `objects_free_all` `0x469fc0` | **Observed** code, *inferred* effect | No |
| C14 | Latent: a character whose name has 32 or more characters overwrites its own mesh pointers | `character_create_base` `0x541500` | **Observed** code and retail data, *inferred* effect | No |
| C30 | Latent: a character preload index with more than 70 entries or more than 1 MB overwrites memory | `ca_load_preload_index` `0x541790` | **Observed** code and retail data | No |
| C40 | Latent: the `door_open`, `door_close` and `door_toggle` console commands with an unknown door name use an uninitialised pointer and can crash | `cmd_door_open` `0x4d5e70`, `cmd_door_close` `0x4d6580`, `cmd_door_toggle` `0x4d6600` | **Observed** code, *inferred* crash | No |
| C50 | Latent: the OpenGL renderer with window mode 202 (desktop size) crashes at start-up | `gr_opengl_init` `0x54f6f0` | **Observed** code, *inferred* crash | No |
| C51 | Latent: the OpenGL renderer crashes at start-up on a driver without `GL_ARB_multitexture` | `gl_init_multitexture` `0x54f370`, `gl_setup_multitexture_state` `0x54ffa0` | **Observed** code, *inferred* crash | No |
| C52 | Latent: in the Khosani lab's room 4, a missing prisoner makes the script write into the event-name pool | `ls_khosanilab_room4_frame` `0x4f8d70` | **Observed** code and retail data, *inferred* effect | No |
| C53 | Latent: a world map without `$hills`, forest or grass navpoints crashes the stage 4 to 8 encounter roll | `ls_worldmap1_pick_encounter_stage4` `0x4fc4b0` | **Observed** code, *inferred* crash | No |
| C70 | Latent: opening the Character screen while Rosalind wears heavy body armour crashes, because she has no heavy stat-screen animation | `statscreen_load_anim` `0x448f60` | **Observed** code and retail data, *inferred* crash | No |
| C71 | Latent: in a town, a character type with a hide or cloaked state move but no "stand" state move crashes while its animations load | `charinfo_remap_town_moves` `0x447850` | **Observed** code and retail data, *inferred* crash | No |
| C72 | Latent: a move with more than three `$Trigger:` lines overwrites its sound reference, and from the fifth trigger on its move list link | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data, *inferred* effect | No |
| C73 | Latent: a translated chain skill name of more than about 60 characters overflows the "%s learned %s!" message on the stack | `character_learn_chain_skill` `0x449650` | **Observed** code and retail data, *inferred* effect | No |
| C90 | Latent: with the OpenGL renderer, a multi-pass polygon of more than 32 vertices after clipping overwrites the stack | `gr_tmapper_multipass` `0x507a80` | **Observed** code, *inferred* effect | No |
| C91 | Latent: Print Screen with the Direct3D renderer quits the game with "see your graphics guy" when the back buffer cannot be locked | `gr_screenshot` `0x5061f0`, `gr_d3d_read_backbuffer` `0x53a7f0` | **Observed** code, *inferred* trigger | No |

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
but `edi` never moves to the next gadget. Flag `0x10` ("+Always a selection") is set only by
the UI table loader, `ui_load_table`, which has no caller (V9). The script editor builds its windows with constant
flags that do not include `0x10`. The only other path is `editor_dialog_show`, which builds its flags at run time
(`or ecx, 0x20` at `0x430ebc`) and was not traced, so this is reachable at most from that debug dialog.

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

**C11. No full test on the item pool.** The search loop for a free `item_obj` stops at 1000
(`cmp [ebp-0x88], 0x3e8; jge` at `0x46a1db`). It then uses `item_obj_pool[i]` anyway (`0x46a200`..`0x46a20c`).
The pool holds 1000 records of 0xac bytes (`0x946790`..`0x970770`), so record 1000 is `obj_active_list`
(`0x970770`) itself. `obj_link_new` (`0x469b60`) then links the list head into its own list, and the following
stores write over the globals after it, including `objects_initialized` (`0x970809`). Item objects are freed when
they are picked up. `items_init_pool` only runs from `objects_reset` (title screen and multiplayer reset), so the
limit is per level visit. We know of no retail level that comes near it.

**C12. No test for an empty free list.** `item_alloc` takes `item_free_list` and reads its `next` with no NULL
test (`mov ecx, [eax]` at `0x44b385`). `items_init_pool` (`0x44b280`) builds the list from 1224 records of 0x10
bytes (`0x8b50c8`..`0x8b9d48`). The last record's `next` is 0, so item 1225 dereferences NULL. Every `item_create*`
goes through it: party inventory, equipment of every living character, loot and shop stock.

**C13. The delete loop does not advance.** `objects_free_all` repeats `delete obj_active_list.next` until the
list is empty (`0x469fc6`..`0x46a01b`). `obj::~obj` (`0x469b40`) only stores the vtable and does not unlink, and
`operator delete` (`0x512570`) does not unlink either. So any object still linked is deleted again and again.
Item objects live in a static pool (`0x946790`), so deleting one also passes a non-heap pointer to the heap. Callers:
`objects_reset` (`0x46b203`, from `state_title_enter` and `mp_session_reset`) and `game_cleanup` (`0x43e228`).
*Inferred:* the level unload always empties the list first, because returning to the title works in retail
play. A path that leaves an object linked would hang.

**C14. Name copied into a 32-byte field.** `character_create_base` strips the extension into a 41-byte stack buffer
(`0x54150c`..`0x54151b`, `file_strip_ext` at `0x5415a0`) and copies it into the character with no length test
(`0x5415b1`..`0x5415c7`). After `character_load_shadow` has stored the mesh at slot `+0x28` (`0x540e18`) and the
shadow mesh at `+0x2c` (`0x540e98`), it copies the name again into slot `+8` (`0x54160e`..`0x541636`). The slots are
0x30 bytes (`ca_preload_alloc_slot`, `0x2ddfa48`..`0x2de20b8`, 205 slots), so a name of 32 characters or more writes
over both mesh pointers, and one of 41 or more also overruns the stack buffer. The longest retail character name has
28 characters ("Guardian of the Stone Portal"). Found in the [`character.cpp`](../../src/decomp/vsdk/character.cpp) notes.

**C30. No bounds on the preload index.** `ca_load_preload_index` reads the entry count (`ca_preload_count`,
`0x54180e`) and the data size (`ca_preload_size`, `0x541818`) from the `.svf` file and uses both as they are.
The loop stores each entry's offset and size into two 70-slot arrays (`[esi - 0x118]` and `[esi]` from
`0x2ee21dc`, `0x541859`..`0x541864`) until the count. It then reads `size` bytes into `ca_preload_data`
(`0x2de20c4`, 1 MB, `0x541894`). Entry 71 writes past the arrays, and a file over 1 MB writes past the buffer
into the arrays that follow. Retail: the largest of the 156 `.svf` files in `chars.vpp` is `evil7724.svf`
with 61 entries and 626,685 bytes. When the file does not open, the message is formatted into a stack buffer
and dropped (`0x5418b0`..`0x5418c2`). Found in the `character.cpp` notes.

**C40. The lookup result is not tested.** Each command calls `door_find_by_name(next_arg, &door, &lobj)` and passes the
`lobj` local straight on (`mov edx, [esp+0x10]` at `0x4d5e97`, `0x4d65a7`, `0x4d6627`). `door_find_by_name` returns -1
without writing either output when the `bsearch` fails or the index is out of range (`0x477308`). The local is never
initialised, so `door_set_open` (`0x476e00`), `door_set_closed` or `door_toggle_state` reads `[lobj+8]` from whatever
was left on the stack. A missing argument has the same effect. These are developer console commands (see
[Console commands](console.md)), so no retail path reaches them.

**C50. Mode 202 skips the context.** `gr_opengl_init` switches on the window mode (`0x54f6f0`..`0x54f700`): 200 calls
`gr_opengl_init_context` (`0x54f739`), 201 loops in `DebugError` ("OpenGl can only be used in windowed modes"), and 202
jumps straight to `0x54f741`. There it calls `glGetString(GL_VERSION)` and `glGetString(GL_EXTENSIONS)` with no current
context, which return NULL, and passes the result to `strstr` (`0x54f761`). The error for any other mode names
`gr_direct3d_init` (`0x54f702`, a copied line). The only `gr_init` call pushes 201 (`push 0xc9` at `0x43db45`; SummonerFix
patches it to 200), so no unpatched game reaches this.

**C51. Entry points left NULL.** `gl_init_multitexture` looks for `GL_ARB_multitexture` in the extension string
(`0x54f382`..`0x54f38c`). When it is missing, the `je 0x54f3e0` skips both `wglGetProcAddress` calls (`0x54f3c0`..`0x54f3da`)
but still calls `gl_setup_multitexture_state` (`0x54f3e0`), whose first instructions call `gl_active_texture_arb` and
`gl_client_active_texture_arb` (`0x54ffa7`, `0x54ffb2`). Both are 0. The "does not support multitexturing" box only covers
fewer than two units (`cmp [esp], 2` at `0x54f39e`). Every vendor driver exposes the extension. *Inferred:* Windows' own
software OpenGL 1.1 ("GDI Generic", used without a vendor driver, for example in some virtual machines) does not, so
SummonerFix's OpenGL setup would crash there.

**C52. No test for -1.** When the player reaches `$platform` (once, before `room4_started` is set, `0x4f8b4a`..`0x4f8b64`), the
function opens `prisonlid01`..`08` and sets `end_mode = 1` on each prisoner through
`script_objects[script_object_find("Khosani Prisoner#n")]` (`0x4f8e0b`..`0x4f8e9a`). `script_object_find` returns -1 when no
living or dead character of that name is found (`or eax, -1` at `0x4cde57`). Index -1 writes the dword 1 at `0x24f5ce0`,
0x550 bytes before `script_objects` (`0x24f6230`), inside the event-name buffer of `events_string_pool` (`0x24ede14`, 0x8400
bytes). The four prisoners are in the retail level scripts (`tables.vpp`) and are still locked in at that moment.

**C53. NULL name compared.** `ls_worldmap1_nearest_hills_nav` returns NULL when no navpoint starts with `$hills` (`0x4fc332`..`0x4fc337`)
and reports distance 10000 (`0x461c4000`, `0x4fc2ad`). The picker switches to the forest or grass result only when that one
is strictly nearer (`0x4fc685`..`0x4fc6c2`). With no terrain navpoint nearer than 10000 units it keeps the NULL and calls
`_stricmp(NULL, "$hills001")` (`0x4fc6e2`..`0x4fc6e8`). The retail world map has these navpoints.

None.

**C70. No NULL check after the move search.** `statscreen_refresh_anim` (`0x412e50`) asks for the heavy
version when the item in equipment slot 7 has slot type 8, "heavy" (`cmp dword ptr [eax+0x1c], 0x8` at
`0x412ee8`). `statscreen_load_anim` walks the state list for variation `0x9f` ("statscreen heavy") and
leaves `m` NULL when there is none. It then reads `m->anim` for the party member (`mov ebx, [ebx+0x8]` at
`0x4490dc`). The multiplayer branch does the same with the result of `charinfo_find_move(info, 0xe, 0x9f, 0)`.
Joseph, Jekhar and Flece have both stat-screen moves; Rosalind has only "statscreen light". All 48 heavy
items in `items.tbl` are `+Restricted:` to Joseph, Flece and Jekhar, so no retail item triggers it.

**C71. The town remap does not check its source.** In a town, or in templeint without the full set,
`charinfo_remap_town_moves` points the state moves hide, ready hurt, hide_ready, ready_to_cloaked,
cloaked_to_ready, cloaked_ready and cloaked_sneak at the anim of `charinfo_find_move(info, 0xe, 0x92, 1)`,
"stand". The result is used without a test (`mov ecx, [eax+0x8]` at `0x447af8`). The function returns
early only when the record has no movement move of variation 1. In the retail tables the only record with
such state moves and no "stand" is Serpent Rider, which has no movement variation 1 move.

**C72. No bound on the triggers.** `$Trigger:` lines fill `move_info +0x1c` in 8-byte steps with no
counter (`0x4462e0`..`0x446318`). A fourth trigger lands on the sound reference at `+0x34` (name and
handle), which was parsed just before. A fifth reaches `+0x3c` and the list link at `+0x40`, so the next
walk of that move list follows a float as a pointer. No retail move has more than three triggers.

**C73. Fixed message buffer.** The message is built with `sprintf` at `[esp+0x60]` (`0x4497d8`). The frame
is 0xa0 bytes (`sub esp, 0xa0` before the three register pushes), so the buffer has 76 bytes before the
return address. The name comes from
`character_get_display_name` (at most 8 characters for the heroes), the skill name from
`localize_string_or_invalid`. The longest retail chain skill names are 19 (English, "Chain Stamina
Burst"), 13 (German) and 17 (French) characters. It needs a modded `strings.tbl`.

**C90. No bound on the pass vertex list.** For every renderer other than Direct3D and Glide (in practice
OpenGL), `gr_tmapper_multipass` copies the `nv` vertex pointers into a 32-entry stack array (`sub esp, 0x8c`
at `0x507a85`, array at `[esp+0x18]`, copy loop `0x507aab`-`0x507aca`) without testing `nv`. Its callers are
`gr_poly_clip_and_draw` (`0x54595c`) and `gr_poly_split_sections_draw` (`0x55f556`). `gr_poly_vertex`
(`0x5452f0`-`0x54530c`) appends to a 32-record pool (`gr_poly_vertices`, 0x680 bytes of 0x34-byte records) with
no check either, and the clipper (`gr_poly_clip_planes` `0x545970`) can add one vertex per clip plane, up to
six. So a polygon of 27 or more vertices that is clipped can pass 33 or more vertices. *Inferred:* retail
geometry uses far smaller polygons, so this is latent. Direct3D and Glide take their own path
(`0x507ce3`, `0x507cb1`).

**C91. A failed read-back is fatal.** `gr_screenshot` switches on the format `gr_read_backbuffer` returns: 3,
6 and 7 (565, 888, 8888) are handled, and anything else calls `DebugError` with "The current screen mode
doesn't support print screen... see your graphics guy." (`0x50625d`-`0x506279`). `DebugError` shows a message
box and calls `exit(1)` (`0x531d08`, `0x531d10`). `gr_d3d_read_backbuffer` returns 0 when the back buffer's
`Lock` fails (`test eax, eax; je` at `0x53a827`, `xor eax, eax` at `0x53a82b`), so a screenshot taken while the
surface cannot be locked ends the game. Glide and OpenGL always return 3 and 7. *Inferred:* a lock should fail
only while the surface is lost (for example around a task switch), so this is latent. SummonerFix's OpenGL
renderer is not affected. The failed-lock path also skips `gr_d3d_begin_scene` (`0x53a8ae`); see Still unclear.

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
| G10 | Latent: adding gold to a container that already holds gold loses the gold and leaves a stale slot | `container_add_item` `0x46aa30` | **Observed** code, *inferred* effect | No |
| G11 | Latent: using aimed attack (or forge, mine, enchant) as a skill takes the AP and does nothing | `character_use_skill` `0x429410` | **Observed** code, *inferred* effect | No |
| G12 | Spell cooldowns are the spell's record number in seconds, not the table's recovery time (fireball 26 s instead of 20; monsters' AI recovery times likewise) | `spells_parse_table` `0x47a190` | **Observed** code and retail data, *inferred* effect | No |
| G13 | The blind / cripple / slow chance of an aimed attack (and of blindness stab, arm slice, leg swipe) uses the selected character's aimed attack skill, not the attacker's | `combat_apply_melee_hit` `0x465df0` | **Observed** code, *inferred* effect | No |
| G14 | Multiplayer: when a player closes a container after taking some items, the other machines drop the container's last entries instead of the taken ones | `mp_handle_container_close` `0x488eb0` | **Observed** code, *inferred* effect | No |
| G15 | Damage spells roll each die from 0 to die-1 (lightning d6 averages 2.5), while healing dice roll 1 to die | `living_entity_get_hit_by_spell` `0x45dea0`, `spell_roll_amount` `0x47f4d0` | **Observed** code and retail data; intent *inferred* | No |
| G16 | Latent: with all 20 missile slots taken, the next spell missile is not fired | `spell_launch_missile` `0x47e6c0` | **Observed** code, *inferred* effect | No |
| G17 | Latent: if an offered item is gone when a trade completes, the partner gets it anyway and the `add_gold` console command is corrupted | `mp_trade_complete` `0x4718d0` | **Observed** code, *inferred* effect | No |
| G18 | Multiplayer: a lock picked through the host can stay locked on the other machines, or another container unlocks there | `mp_handle_container_lockpick` `0x489660` | **Observed** code, *inferred* effect | No |
| G30 | Boss moves aimed by offense point are measured from about the world origin: Pyrul's slash and fire breath, Machival's area attacks and every Giant Salamanka melee hit should rarely land; the Ghost's sweep hits only its main target | `boss_ai_parse_table` `0x4a1790` | **Observed** code and retail data, *inferred* effect | No |
| G31 | A boss's `+Plus Spell:` (Serpent sickle poison; Ghost paralysing fear and "ghost death") lands even when the hit itself is dropped as out of range | `living_combat_resolve_hit` `0x467df0` | **Observed** code, *inferred* effect | No |
| G32 | Melee party members fight the mounted Phoenix Rider in melee (unless the Serpent is in its stage 1), and the mounted Ghost Rider always | `boss_species_mounted_stage` `0x4a2480` | **Observed** code, *inferred* effect | No |
| G40 | A loose gold pile on the ground holds only 1 gold after the game is loaded | `savegame_read_item_objects` `0x4d31e0` | **Observed** code, *inferred* effect | No |
| G41 | With more than 32 loose gold piles in a level, the extra piles are gone after a load | `savegame_write_item_objects` `0x4d2370` | **Observed** code, *inferred* effect | No |
| G42 | Latent: a door whose name plus the level script name has 64 characters or more makes every save of that level fail to load | `savegame_read_two_stage` `0x4d34e0` | **Observed** code, *inferred* effect | No |
| G50 | After a reload or a second visit to the Khosani lab, the flipper puzzle's board no longer matches the flippers: raised flippers show no lava platform, and the next presses move the wrong flippers | `ls_khosanilab_init` `0x4f8920`, `ls_khosanilab_set_flippers` `0x4f9ca0` | **Observed** code, *inferred* effect | No |
| G51 | Latent: once the 1000 path nodes run out, every failed path search keeps its nodes until the next level load | `pf_graph_search` `0x49fa70` | **Observed** code, *inferred* effect | No |
| G52 | Latent: a path to a goal outside the walkable area usually fails instead of ending at the area's edge | `pf_find_cell_crossing` `0x49ecb0` | **Observed** code, *inferred* effect | No |
| G60 | Latent: when the chat server connection fails at once (any error except "would block"), the multiplayer lobby never reports it, retries every frame and opens a new socket each time | `irc_connect` `0x4abc10` | **Observed** code, *inferred* effect | No |
| G70 | If a cutscene starts while a summon is still waiting to emerge, every later summon of that creature type ends its emerge at once, until the game is closed | `summon_stash_for_cutscene` `0x448000` | **Observed** code, *inferred* effect | No |
| G71 | Latent: a misspelled `$Skill:` name in a character table overwrites the record's up-direction skill; a misspelled direction skill becomes id `0x54`, past the skill table | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data | No |
| G72 | Latent: a character with more than two `$Default Weapon:` lines gets wrong materials on its first weapons and, from the eighth line on, junk weapons | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data, *inferred* effect | No |

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

**G10. The merge branch falls into the insert.** When slot 0 already holds gold, the new count is added to it
(`0x46aa79`..`0x46aa9a`) and the new gold object is deinit-ed (`call [edx]` at `0x46aaa8`). The code then jumps
to the common tail (`jmp 0x46aae5`). That tail stores the deinit-ed object in slot 0 over the merged gold
(`mov [edx+0x98], eax` at `0x46aaf7`) and increments `num_items` (`0x46ab06`). The merged sum is lost. The new
last slot was never written, and the slot-0 object is free in the pool again. Callers: `monster_drop_loot`
(`0x452f00`), `level_item_spawn` (`0x46a681`, always a new container), `mp_handle_item_drop` (`0x4892fa`) and
`savegame_read_item_objects` (`0x4d3341`). A monster drop creates at most one gold object (`item_create_misc_by_name("Gold")`
at `0x450fee`), so we know of no retail path that adds two.

**G11. AP first, action second.** `character_use_skill` pays the AP for every skill outside the 0x12..0x33
switch range, so also for 0x11 (`lea ecx, [edi-0x12]; cmp ecx, 0x21; ja` at `0x42948f`, then `character_add_ap`
at `0x4294b6`). The action switch has no case for 0x11 or 0x18..0x1a, so it returns false. The skills panel
sends aimed attack to its popup (`skills_handle_mouse`), and the popup uses the per-part skills (eyes, arms,
legs). The panel also refuses to drag 0x11 or 0x1b onto a hot button (`cmp esi, 0x11` at `0x4288aa`,
`cmp esi, 0x1b` at `0x4288af`). The creation skills have no icon (`skills_icon_names[24..26]` = 0), so no retail
path reaches the bad case. Found in the [`new_skill.cpp`](../../src/decomp/game/new_skill.cpp) notes.

**G12. Record index stored for three fields.** `spells_parse_table` parses `+Recovery time:`, `+AI Recovery time:`
and `+Loop Time:` and then stores the loop counter `bp`, not the parsed value (`mov word [esi-0x12], bp` at
`0x47a49c`, `[esi-0x10]` at `0x47a4c1`, `[esi-0xe]` at `0x47a50b`; `ebp` is the record index, `inc ebp` at
`0x47a648`). `TextFile__Accept` compares with `_strnicmp` (`0x516134`), so the table's "+AI Recovery Time:" and
"+Loop time:" spellings do match. `character_perform_spell_cast` passes `+0x66` to
`character_start_spell_recovery` (`0x45dc7a`), which starts the cooldown as the cast effect's length plus that
value (`0x44dcf0`..`0x44dd1f`). For type 5 characters (the type of a summon that turned hostile, see
[Summons](summons.md)) it uses `+0x68` instead (`0x44dce6`); `ai_consider_area_spell` also reads `+0x68`
(`0x4bf068`). Retail `spells.tbl` has `+Recovery time:` on six spells: invisibility 0 -> 11, fireball 20 -> 26,
wall of fire 20 -> 29, dark wall 20 -> 31, blizzard 25 -> 35, Pyrul fireball 20 -> 78. Twenty spells have
`+AI Recovery Time:`, for example vitalize 10 -> 4, death 30 -> 12, curse 10 -> 23, ice coffin 25 -> 36.
Spells without the line keep their `+Duration:` (or -1, no cooldown) as before. `+Loop Time:` is read only by
`status_effect_duration` for status 8 (`movsx eax, word [0x22e5512]` at `0x47ab90`, spell 11 invisibility,
10 -> 11 s). That is the tick period of the status, and `character_update_status_effects` has no tick action
for status 8 (case byte 3 at `0x45bf3c + 3`), so this part has no effect. See
[Spells](game-tables.md#spells).

**G13. `cur_character` instead of the attacker.** The three status branches load `mov ecx, [0x5fcc88]` before
`character_get_skill(0x11)` (`0x4663c0`, `0x466468`, `0x466510`) and compare the result with `frand()`. The
cases also serve blindness stab, arm slice and leg swipe. In multiplayer every machine resolves other players'
hits with its own selected character. In single player the aimed attacks are started through
`character_use_skill` for the selected character, so the wrong skill is used only when the selection changes
before the hit lands. Monsters never pick these skills (`living_combat::ai_choose_special` only takes ids below
`0x24`).

**G14. The container handle is read for every item.** The loop pushes `ebp`, the container's net handle from
`data + 4`, for each taken item (`push ebp` at `0x488f00`), not the item handles that `mp_send_container_close`
appends after the count (`0x488e08`..`0x488e2a`). So `container_remove_item` gets the container itself. It does
not find it, but still decrements `num_items` (`0x46abf5`..`0x46ac04`). Each taken item therefore drops the
*last* entry on the receiving machines (the host too, which then forwards the packet). When every item was
taken the count reaches 0 and the container is deleted, as intended. Otherwise the taken items stay listed
there and as many untaken ones vanish from the list.

**G15. Negated die.** For damage the handlers pass the table's die negated (`neg eax` at `0x45e4d6` for
lightning; nine `neg` sites in all), and `spell_roll_amount` rolls `floor(frand() x die + 1)` per die
(`0x47f51e`..`0x47f549`). With die 6 that is 1..6 for a heal but 0..-5 for damage. Lightning (d6) and the other
damage spells with a `+Die:` in retail `spells.tbl` lose one point per die on average. A designer may have tuned the tables to this, so the intent is *inferred*. See
[Combat](combat.md#spell-hits).

**G16. Both exits return.** When the free-slot search fails (`0x47e705`..`0x47e716`), the function clears the
slots whose effect object is gone (`0x47e71d`..`0x47e745`) and then searches again, but both outcomes return
(`je 0x47e821` at `0x47e74f`, `ret` at `0x47e765`). The missile is never recorded, so `spell_fire_missile`
finds no slot (`spell_missile_remove` returns -1) and creates no missile. Slots are also freed when a missile
fires, when its caster dies or plays a new move (`fx_remove_missiles_from`, callers `0x4538cf`, `0x460fcd`) and
on entering a level (`spells_reset_runtime`), so 20 pending missiles at once are needed. We know of no retail
fight that reaches that.

**G17. No test for -1.** For each offered item that is not gold, `mp_trade_complete` subtracts the count at
`party_inventory[party_inventory_find_type_index(type)]` (`0x471914`..`0x471929`) without testing for -1. Slot
-1 is `0x970818`, the handler pointer of `ccmd_add_gold` (`0x970810`, set by `console_command__console_command`
at `0x50a1dc`). The pointer stays positive, so `party_inventory_remove` is not called. The offered item stays in
the inventory, the partner still gets it, and "add_gold" later jumps into the wrong code. The offer is built
from the inventory with merged types and the count checked (`itemtrade` click handler), and the trade is
cancelled when the character dies, hides or leaves AI mode 1 (`NOTES-multi.md`). We found no way to lose an offered item while the
trade screen is open.

**G18. Host handle used without mapping.** On success the host clears the lock (`mov [ebx+0xe4], 0` at `0x489564`)
and sends its own object handle (`container_lockpick` `obj_from_handle` at `0x4894b9`, stored by
`mp_send_lockpick_result` at `0x489614`). The receiver looks it up with `obj_from_handle` (`0x4896a2`), where
`mp_handle_container_open` uses `mp_obj_from_net_handle` (`0x488d5b`). That function maps the handle through
`mp_net_handle_map` (`0x22e94f4`) on a client (`0x475265`..`0x47526b`). When the handles differ, the lookup misses
and the lock stays on that machine, or it finds another container (type test `0xb` at `0x4896b0`) and clears that
one's lock (`0x4896ce`). `object_use_container` tests the local lock (`fld [ecx+0xe4]` at `0x43cc7c`), so on that
machine the container still reports itself locked. *Inferred:* how often host and client handles differ was not
measured. Found in the `NOTES-multi.md` notes (bug 3).

**G30. Tag names parsed as numbers.** `boss_ai_parse_table` reads `$Offense point:` as a string and stores
`atoi(string) - 1` in the move record (`0x4a18b7`..`0x4a18c4`). All 89 records of the nine loaded retail
tables give a tag name (`"$offense01"`..`"$offense07"`), so `atoi` returns 0 and every move has point -1.
The loaders keep the found `$offenseNN` tags in packed 0x1c-byte records, with the count just before the array
(Pyrul `0x247fdb4`). The point tests index that array with the move's number and no check (`lea ecx,
[8*eax]; sub ecx, eax` at `0x4b9c2b` and in the four other tests below). Point -1 is therefore the 0x1c bytes
before the array: its position `+0x10` / `+0x18` is a word nobody writes and the count (an integer read as a
float, about 0); for Pyrul it is the fifth threat record. Its direction is near zero. The tests are
`pyrul_point_in_attack_cone` (Pyrul move classes 0 and 2, slash and fire breath), `ghost_point_in_attack_cone`
(Ghost class 4), `machival_offense_point_reaches` (Machival's filter and kinds 3 and 7),
`giant_salamanka_target_in_reach` (every Giant Salamanka single-target hit, through
`boss_move_in_range_dispatch`, and its action choice at `0x4c8a32`) and `murod_oni_offense_point_reaches`
(only Murod Oni's "in reach" target flag). For the cone tests, the target must be within the move's range of
about (0, 0) and, with a zero direction, the move needs a max angle of 90 degrees or more (`dot >= cos`). When
the test finds nobody, the resolver still returns true for Pyrul (`0x4a2c36`) and Machival (`0x4a2c9d`), so no
fallback hit is made. The Ghost's resolver returns false (`0x4a2be1`), so its main target is still hit through
`living_combat_resolve_hit` with the plain edge distance (`boss_move_target_in_range`); only the extra victims
are lost. *Inferred:* the Pyrul, Machival and Giant Salamanka moves deal no melee damage unless the target
stands near the world origin. This needs a
runtime check (agent mode). See [Bosses](bosses.md#boss-table-files).

**G31. Spell before the range test.** `living_combat_resolve_hit` calls `boss_counter_hit_dispatch(attacker,
defender)` at `0x467e93` and only then, for a type-9 attacker, `boss_move_in_range_dispatch` (`0x467ea8`),
which ends the hit when it fails. The dispatcher hits the defender with the move's spell when the Serpent
Rider's or Ghost Rider's current move has flag `0x10` (`+Plus Spell:`): spell `0x54` through
`living_entity_get_hit_by_spell` (`0x4a228c`), others through `fx_create_spell_effect_at` (`0x4a22ab`). The
hit count `+0x40` is set when the swing starts (`living_combat_roll_defense` `0x464112`), and the range is
tested at the hit frame, so a target that steps out of range during the swing still gets the spell. Retail
moves with the flag: Serpent stage 1 sickle ("poison"), Ghost stage 3 scythe ("ghost paralyzing fear") and
claw ("ghost death").

**G32. Wrong stage variable.** `ai_get_combat_style` returns style 2 (Range) instead of 0 (Melee) while the
character's target is a mounted rider (`0x473644`). `boss_species_mounted_stage` tests `tiger_stage == 1 or 2`
for species `0x57`, but `serpent_stage` (`0x2451d98`) `== 1` for both `0x58` (Phoenix) and `0x59` (Serpent),
and returns false for `0x5a` (Ghost; `jg 0x4a249f` at `0x4a248e`). `phoenix_stage` is `0x2452420`, and
`character_target_point_count` in the same file reads it correctly (`0x4a21e8`). So in the mounted Phoenix
fight (`rand-hills01`, stage 1) the result depends on whatever `serpent_stage` holds from an earlier Serpent
fight, and the Ghost is never treated as mounted. In `Liangshan` both riders are on foot (forced stage 3), so
the result is right there. The callers include the group orders, `ai_score_target`, `ai_combat_approach` and
`character_get_attack_range`.

**G40. The amount is skipped.** `savegame_write_item_objects` saves each loose gold pile as x, y, z and its amount
`+0x9c` (`0x4d2550`..`0x4d2559`). The reader takes x, y and z, then skips z and the amount together (`add esi, 8` at
`0x4d33a7`), and creates the pile with `item_obj_spawn(item, pos, matrix, 1, -1, -1)` (`push 1` at `0x4d33c2`).
`item_obj_spawn` stores its fourth argument at `+0x9c` (`0x46a619`..`0x46a622`), and `item_obj_pickup` adds `+0x9c`
to `party_gold` (`0x46b6ff`..`0x46b70d`). Nothing else writes `+0x9c` of the pile after the load. A loose pile is
what `monster_drop_loot` leaves when gold is the monster's only loot (`0x452e38`..`0x452e48`); the amount is
`_ftol(rand x level x constant) + 1` (`0x450f80`..`0x450fa4`). So a pile that was not picked up before saving gives
1 gold after loading. Gold inside a container keeps its amount, because the container items are read with their
saved count (`0x4d3307`, `0x4d3326`). The loaded pile is also dropped to the floor again by `item_obj_spawn`'s
probe.

**G41. A silent limit of 32.** The gold-pile loop stops at 32 (`cmp ebp, 0x20; jge` at `0x4d24e3`) and the save still
succeeds. The level does not recreate monster drops, so piles 33 and later are lost on load.

**G42. A length check that fails the load.** For every saved two-stage hash of the current level, the reader walks
the doors and builds `level_script_name + door name` in a 64-byte buffer. Before each comparison it tests the
combined length and returns 0 when it is 64 or more (`cmp ebx, 0x40; jae 0x4d3730` at `0x4d3588`, repeated at
`0x4d35ac`). `savegame_read` then prints `"12a"` and the load ends in the main menu (`gameseq_set_state(1)` at
`0x438211`). The test runs for every door it compares, not only the matching one, and only when the level has at
least one saved two-stage entry. `savegame_two_stage_get` and `savegame_two_stage_set` build the same string with
`strncpy` / `strncat` and no test. We did not check the retail door names.

**G50. The board is cleared, the doors are not.** `ls_khosanilab_init` clears the 10 x 7 board `ls_khosanilab_flippers` on every
entry (`rep stosd` of 0x46 bytes, `0x4f895c`..`0x4f897e`) and then hides the platform of every cell
(`ls_khosanilab_apply_lava_platforms`, `0x4f8980`). The flippers are doors, and a door's state is its two-stage bit in
`savegame_state`, which `door_start_move` flips (`savegame_two_stage_set` at `0x476fe4`). That bit is kept across level changes
and saved. A press toggles the used cell (`0x4f9b66`..`0x4f9b79`) and its four neighbours through `set_flippers`, which
toggles the cell and calls `door_apply_saved_state(name, cell)` (`0x4f9cc4`..`0x4f9cf4`). That moves the door only when its
bit differs from the new value (`0x47739f`..`0x4773ad`). So after a reload, a flipper that was raised keeps its door raised
while its cell reads 0: its platform stays hidden, the beam grid (`ls_khosanilab_puzzle_build_grid`) is built from the board
and not from the doors, and the first press on a neighbour leaves that door where it is while the cell becomes 1. The board
catches up only as cells are toggled. *Inferred:* how far this blocks the crossing was not tested; it needs agent mode.

**G51. `ai_path_free(NULL)`.** When the graph search has found the goal, it builds the path from the goal backwards with
`path = ai_path_node_create(x, z, layer, path)` (`0x4a0180`..`0x4a019b`). When the pool is empty `ai_path_alloc_node` returns 0
(`0x4a02d7`..`0x4a02f2`), the loop jumps to `0x4a01b5` and calls `ai_path_free(0)` (`push 0` at `0x4a01b5`), which does nothing
(`test eax, eax; je` at `0x4a0484`). The nodes already chained, including the goal node from `ai_path_node_create_b`
(`0x49fbec`), are lost. Only `ai_path_pool_init` refills the pool, from `pathfinding_load_graph` (`0x49ce04`). So each failure
shrinks the pool for the rest of the level and makes the next failure likelier. We know of no retail fight that uses 1000
nodes at once.

**G52. The z test is reversed.** For a goal outside every cell, `pf_graph_search` looks for the nearest crossing of the
start-to-goal segment with a cell edge (`0x49fafd`..`0x49fb08`) and gives up when there is none (`0x49fb3d`).
`pf_find_cell_crossing` tests each cell's bounding box first. A cell keeps `max z` at `+0x0c`, `min z` at `+0x10`, `min x`
at `+0x14` and `max x` at `+0x18` (`pf_compute_graph_bounds` `0x49cb17`..`0x49cb74`, `pf_find_nearest_navigable_point`
`0x49e972`..`0x49e9ad`). The x test is a normal overlap (`seg min x < cell max x`, `seg max x > cell min x`, `0x49ed42`..`0x49ed60`),
but the z test is `seg min z < cell min z` and `seg max z > cell max z` (`0x49ed66`..`0x49ed84`): only cells whose whole z
extent lies inside the segment's are examined. A segment that runs mostly along x finds no crossing, and the search returns
no path. Clicked destinations are pulled onto the mesh first (`world_click_move` calls `pf_find_nearest_navigable_point`,
`0x43bf26`), as are most AI targets, so *inferred:* this needs a goal that no caller clamps (see "Still unclear").

**G60. An error returned as "in progress".** `irc_connect` makes a non-blocking socket and calls `connect`
(`0x4abdb6`). When that returns -1 it sets `irc_connecting` (`0x247a4dc`) only for `WSAEWOULDBLOCK` (10035,
`cmp eax, 0x2733` at `0x4abdc5`), but returns 0 for every error (`xor eax, eax` at `0x4abdd3`). Its only caller,
the lobby connect step that `state_mp_lobby_frame` jumps to every frame until lobby flag 1 is set (`jmp 0x43f110`
at `0x43f0bb`), treats 0 as "still connecting" and returns (`test eax, eax; je` at `0x43f181`). Only -1 sets
`lobby_connect_state` to the error state (`0x43f18e`). With `irc_connecting` still 0, the next frame runs the whole
connect again: a new `socket` (`0x4abce5`) whose handle overwrites `0x2479740`, and the old socket is never closed.
The usual failures still get through: a refused or timed-out connection shows up later in `select` on the
connecting socket (`0x4abea7`..`0x4abed4`). Only an immediate error (no route to the host, an address that cannot
be used, a firewall that rejects the call) takes this path. PXO's chat servers are gone, so this needs a
replacement server. Found in the `NOTES-target-chattracker-multispell.md` notes.

**G70. The shared move record is edited.** When the summon has not yet started its emerge (object flag bit
0 clear, AI not in mode 6), `summon_stash_for_cutscene` starts it with `ai_begin_emerge` (`0x448068`),
which plays the emerge move and stores its start time (`character_play_move`, `0x461005`). It then
fast-forwards the anim and subtracts `character_move_trigger_time_left(id) + 0.1` from each of the three
trigger times (`fsubr`/`fstp [esi-0x8]` at `0x4480c4`..`0x4480c7`). The record is `cur_move`, the move in
`move_infos` that every character of that char_info (and its alternates) shares. The tables load once per
session, so the edit lasts until the game exits. Elapsed time is 0 at this point, so each trigger becomes
about -0.1 s. A later stash of the same type subtracts 0, so the times do not drift further (the note's
"shifts again" does not hold). `summon_emerge_update` (`0x4b4660`) ends the emerge when the move is finished
or trigger 6, "critical end", has fired (`0x4b46e9`..`0x4b4701`). *Inferred:* with that trigger at -0.1 s,
each later summon of that creature leaves its emerge state on the first update instead of at frame 80 to
499 (for example the Golem's 304). The window is the delay before the emerge (`ai_frame` waits for the
GameTimer at `ai_state +0x1a0`, `0x472c38`), so it needs a cutscene trigger right after a summon is cast.

**G71. Search end used as an index.** The skill name search stops at the count, 84 (`cmp esi, 0x595cb4` at
`0x445d93`), and the value is stored at `[edi+ebp+0x94]` (`0x445dad`). `skill` is 0x54 bytes
(`mov ecx, 0x15; rep stosd` at `0x445bad`), so index 84 is `+0xe8`, the up skill. The four direction lines
store the same 84 when the name is unknown (`0x445c13`). Id 84 is past `skill_table` and reads
`resistance_level_names` (`0x8b5098`) as its first fields. Every skill name in the retail tables matches.

**G72. No bound on the default weapons.** `$Default Weapon:` indexes `char_info +0x116` (two shorts) and
`+0x11c` (two materials) by the running count `+0x114` with no compare (`0x44583f`, `0x44587e`..`0x44589c`).
The third line writes the padding at `+0x11a` and `+0x124`. The fourth to seventh write their item indices
over the two materials already stored at `+0x11c`/`+0x120`. Materials from the third line on land in
`+0x124`..`+0x168`, which the prop tag code resets afterwards (`0x445b07`..`0x445b26`: prop tag 0x11, tag
names 0). So weapons 8 and 9 read back as item types 0x11 and 0, and later ones as 0. `living_entity_init`
creates one item per counted weapon (`0x44e8e9`..`0x44e911`). No retail record has more than two
`$Default Weapon:` lines.

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
| K12 | In a container's pickup list, the scrollbar thumb fills the whole track and runs past its end | `pickup_update_scrollbar` `0x420850` | **Observed** code | No |
| K13 | In the multiplayer level select, the player list's hover and click rows drift from the drawn names | `mp_levelselect_handle_click` `0x41c450` | **Observed** code | No |
| K14 | The PXO chat banner is never clickable | `pxo_parse_chat_banners_cfg` `0x4223d0` | **Observed** code | No |
| K15 | Latent: Print Screen saved as TGA (console `targa`) from a 32-bit back buffer gives garbled colours | `tga_convert_pixels` `0x555020` | **Observed** code, *inferred* effect | No |
| K16 | Latent: resetting a character's animations clears only the first 25 of 100 weights | `character_instance__reset_anims` `0x519850` | **Observed** code, *inferred* effect | No |
| K17 | Latent: a summon's second weapon (slot 9) pages the first weapon's model | `items_level_load` `0x44d540` | **Observed** code, *inferred* effect | No |
| K18 | Latent: paging out the popup model slot 18 can drop another slot's item model | `item_page_out_all` `0x46ca50` | **Observed** code, *inferred* effect | No |
| K19 | The launcher's Speed tab ("Default Detail Level") has no effect on the game's detail settings | `GameInit` `0x43df40`, `summoner_cfg_load` `0x43e250` | **Observed** code and launcher strings, *inferred* effect | No |
| K20 | Brass, Steel and Iron Golems show no hit effect when struck (stone golems do) | `combat_spawn_hit_vfx` `0x4669c0` | **Observed** code and retail data, *inferred* effect | No |
| K30 | Tall bosses (Tentacle Beast, Evil Urath, Luminar, Machival, Giant Salamanka, Titus, Pyrul) get their size class's height: health bar and floating text sit too low, and clicking the upper body misses | `character_size_height` `0x4542f0` | **Observed** code, *inferred* effect | No |
| K40 | With 128 map labels saved, deleting or editing a label is not saved, and the old label comes back on the next visit or load | `savegame_automap_label_remove` `0x4d1390` | **Observed** code, *inferred* effect | No |
| K50 | With the OpenGL renderer, the smallest mipmaps of the 16-bit water textures are built from the top half of the larger level only | `gl_build_mipmaps` `0x54e0e0` | **Observed** code and retail data, *inferred* effect | No |
| K51 | Latent: with the OpenGL renderer, a tint on a 2D bitmap, line or polygon wraps bright colours round to dark | `gr_opengl_apply_tint` `0x54f9a0` | **Observed** code, *inferred* effect | No |
| K52 | Latent: `help change_texture_res` (or `? change_texture_res`) switches the low-resolution textures on or off | `cmd_change_texture_res` `0x501c90` | **Observed** code | No |
| K60 | With the Direct3D renderer, the depth clear misses the last column and the last row of the clip area, so those pixels keep the previous frame's depth | `gr_d3d_zbuffer_clear` `0x5355e0` | **Observed** code, *inferred* effect | No |
| K61 | A dropped item's 5-second highlight gets no blue at all: it is yellow instead of the pale gold in the data | `world_tint_gold_object` `0x43a600` | **Observed** code, *inferred* effect | No |
| K62 | In the multiplayer chat, a kick message shows the kicked player's name where the reason should be | `chat_process_irc_message` `0x4ac240` | **Observed** code | No |
| K63 | Clicking a character tests the horizontal distance squared against the radius: small and normal-sized characters can be clicked further from their middle, characters with a radius above 1 only nearer to it | `pick_test_character` `0x43c620` | **Observed** code, *inferred* effect | No |
| K64 | With the OpenGL renderer, a start without `summoner.cfg` (a fresh install) switches on the low-resolution textures | `summoner_cfg_load` `0x43e250`, `gr_get_video_memory_mb` `0x5079f0` | **Observed** code, *inferred* effect | Fixed (`bugfix.c`) |
| K80 | A knock-down that hits the selected character while it casts shows "Knocked Down!", but the character stays on its feet | `ai_knock_down_local` `0x4742b0`, `ai_set_mode` `0x473320` | **Observed** code, *inferred* effect | No |
| K90 | With the OpenGL renderer, the gamma (brightness) slider does not change the brightness | `gr_set_gamma` `0x506a90`, `gr_opengl_tmapper` `0x550080` | **Observed** code, *inferred* effect | No |
| K91 | With the Direct3D renderer and a hardware gamma ramp, lit polygons get the gamma curve twice | `gr_d3d_tmapper` `0x5361a0`, `gr_d3d_tmapper_multipass` `0x536360`, `gr_d3d_set_gamma` `0x53ab20` | **Observed** code, *inferred* effect | No |
| K92 | Latent: a fog update with a negative near distance or a far distance below the near one keeps the old fog distances | `gr_set_fog` `0x507db0` | **Observed** code, *inferred* effect | No |
| K93 | Latent: when anti-aliasing is on in the options, the first `antialiasing` console command reports "antialiasing toggled!" but changes nothing | `cmd_antialiasing` `0x505db0` | **Observed** code | No |

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

**K12. Full-height thumb.** Both branches set the thumb height to the full track height
(`mov [0x5c3b34], edx` at `0x4208f8` and `mov [0x5c3b34], edi` at `0x420943`). The scrolled branch still moves
the thumb down by `scroll_pos / num_items x track` (`0x420901`..`0x420941`). `pickup_draw_scrollbar` (`0x4212c0`)
tiles the thumb bitmap over that height, so once the list is scrolled the thumb runs past the bottom of the track.
The hit area grows with it. The assess screen scales the height with `page / count` (`0x4017a6`..`0x4017d7`).
This shows for any container with more items than one page.

**K13. Row counter steps for every player.** The click loop adds the font height for every entry of
`mp_player_list` (`add ebp, esi` at `0x41c7e0`), but tests only type-3 players (`cmp [edi+0x14], 3` at
`0x41c79d`). `mp_levelselect_draw_players` moves down only after drawing a type-3 player (`0x41d522`..). A
player in another state, for example one who is still joining (state 2, `mp_accept_join_request` `0x4ab6a8`),
shifts every later name by one row. The hover highlight then lands on another line, and a click can select a
different player. Kick uses the selected *name*, which is drawn highlighted, so the host can see the mistake
before kicking. Only the first four rows are hit-tested (the box is `font_h * 4` high).

**K14. Link parsed into a scratch buffer.** Each `+Link:` value is parsed into one stack buffer
(`lea eax, [esp+0x294]` at `0x4225e3`) and never copied into `links[count]`. `pxo_chat_banner_link_url` is
therefore always empty, and `pxochat_banner_has_url` (`0x422790`) is always false. This was live while PXO
served banners. The service is gone, so it no longer shows.

**K15. Copy size and advance disagree.** For a 4-byte source pixel, `tga_convert_pixels` falls through to the
plain copy. It copies `count x src_bpp` bytes (`imul ecx, esi` at `0x5550ab`) but returns `count x dst_bpp`
(`imul eax, edx` at `0x5550ae`). `gr_screenshot` always asks for 3-byte output (`push 3` at `0x5063fa`) and uses
`src_bpp` 4 when `gr_read_backbuffer` reports format 7 (`0x506283`). In raw packets of more than one pixel, each
BGRA quad is then read back as a BGR triple, so the channels shift. 16-bit (format 3) and 24-bit back buffers
convert correctly. By default screenshots are JPEG (`Print_screen_as_targa` `0x5fc022` tested at `0x43251e`), so
this needs the console command `targa` and a renderer that reports 32 bits.

**K16. Byte count passed as an element count.** `reset_anims(true)` clears `weights` with `mov ecx, 0x19; rep
stosd` (`0x51986b`), which is 25 floats of the 100 that `alloc_buffers` allocates. `character_reinit_anims`
(`0x447f20`) clears every weight anyway through `clear_state_anims` and `clear_action_anims`. `level_load_preload`
(`0x447ce9`) does not: it resets every living character of a char_info after `charinfo_remap_town_moves`, so a
nonzero weight at index 25 or above keeps blending into the new animation set. *Inferred:* it needs an animation
with index 25 or above playing at that moment, and it would only last until that animation ends.

**K17. Index 5 for slot 9.** For characters with flag `0x100` (summons) the loop tests `equipment[9]`
(`[esi+0x778]` at `0x44d72d`) but pages `equipment[5]`'s type (`mov ecx, [esi+0x768]` at `0x44d737`) into paging
slot 9. The second weapon's model is never paged, and if slot 5 is empty while slot 9 is not, `[ecx+4]` reads
through NULL. Whether any retail summon carries a slot-9 item is not checked.

**K18. The popup branch evicts the wrong slot.** When slot 18 owns a mesh that another slot (0..16) also shows,
`item_page_out_all(18)` clears the *other* slot and sets the type's mesh to NULL (`0x46caab`..`0x46caca`). It then
returns without clearing slot 18 (`ret` at `0x46cad2`). The general branch swaps the pools instead
(`0x46cae4`..`0x46caf9`). Slot 18 keeps its type with a NULL mesh, so a later `item_page_in(type, 18)` returns at
once (`s->type == type`) with no mesh, and the other slot's item loses its model until it is paged again. Callers
of slot 18 are the shop model view, the inventory drag and the item info popup (`buysell_handle_click` `0x405507`,
`inventory_drag_to_equip` `0x417442`, `item_info_popup_close` `0x418c22`). We found no retail sequence where slot
18 owns a mesh that another slot shares.

**K19. Presets reset straight away.** `GameInit` reads `ComputerSpeed` (`os_registry_read_dword` at `0x43e080`),
clamps it to 0..2 (`0x43e090`..`0x43e09e`) and presets fog and three detail options (`0x43e0ac`..`0x43e0fe`). It then
calls `summoner_cfg_load` (`0x43e104`), which first stores every default (`0x43e295`..`0x43e320`, among them
`config_detail_14` `0x43e2d4`, `config_fog_quality` `0x43e2f9`) and then reads `summoner.cfg`. The presets are lost
whether or not the file exists. The launcher (`Summoner.exe`) writes `ComputerSpeed` from its Speed tab (strings
"SpeedTab", "ComputerSpeed", "Default Detail Level:"), and `launcher_help.txt` describes that tab as setting "the
default Detail Level". The presets themselves are also odd: value 1 gives `config_detail_14` 2 and value 2 gives 1.
Found in the [`summoner.cpp`](../../src/decomp/game/summoner.cpp) notes.

**K20. The name switch and the spawn tests disagree.** `combat_spawn_hit_vfx` picks a name from the target's skin
(char_info `+0x218`, case bytes at `0x466a64`): flesh and leather (1, 2) get `blood.vfx`, stone, ice and the metals
(3, 9..16) get `sparkhit.vfx`, the rest none. It then spawns only for skin 1 (`blood.vfx`, `cmp edi, 1` at
`0x466a0c`) and skins 3 and 9 (`sparkhit01.vfx`, `0x466a2f`..`0x466a37`). Leather and the metals (copper, gold,
bronze, iron, silver, admantium, volitionite; `material_names` `0x598f5c`) get nothing, and `sparkhit.vfx` is never
used. Retail tables: Brass Golem, Broken Brass Golem and Steel Golem are "bronze", Iron Golem is "iron"; no
character is "leather". Both effect names exist in `effects.vpp`. All 13 callers are in `combat_apply_melee_hit`.
Found in the `NOTES-spells-combat.md` notes.

**K30. Boss height dropped.** For type 9, `character_size_height` calls `charinfo_boss_height` and discards
the result (`fstp st(0)` at `0x45430f`), then returns the size-class height (`fld [4*edx + 0x5818d8]` at
`0x454323`; 0.9, 1.8, 2.5). The radius function next to it returns the boss value (`jmp 0x4542ea` at
`0x4542d2`). The lost heights are Tentacle Beast 15, Evil Urath 8, Luminar 5.5, Machival 5, Giant Salamanka 4,
Titus and Pyrul 3 (the riders, Murod, Murod Oni and Evil Joseph return the size table anyway). Callers:
`levelrender_draw_health_bars`, `_draw_speech_text`, `_draw_float_text`, `pick_test_character`,
`character_has_line_of_sight_to` / `_to_point`, `character_find_overlap`, `script_guard_sees_player`, the
conversation camera and the script editor. *Inferred:* the visible part is the bar and text placement and
picking; the line-of-sight and overlap use may also change what a boss can see or touch, which was not traced.

**K40. Remove returns at the limit.** `savegame_automap_label_remove` returns at once when the list holds 128 labels
(`cmp eax, 0x80; jge` at `0x4d13ca`), and so does `savegame_automap_labels_reset_level` (`0x4d1526`). The test was
probably meant for the add. Deleting a label (`automap_handle_click`, `0x402bf3`) removes it from the live table
(`automap_label_delete`) but not from the saved list. Editing one calls remove and then add (`0x40345e` /
`0x403470`, `0x4034e6` / `0x4034f9`), and the add also refuses at 128 (`0x4d12d6`), so the new text is dropped.
`savegame_read_automap_labels` rebuilds the level's labels from the saved list after a load and on every level entry
(`state_game_enter`, `0x438233`), so the deleted label or the old text comes back. The list counts the labels of all
levels together, and the editor refuses new labels at 128 (`0x40316c`), so the player can reach the limit.

**K50. The 565 filter never skips a row.** Each generated 565 texel averages `src[0]`, `src[1]`, `src[w]` and `src[w + 1]`
(`0x54e4f9`..`0x54e50d`) and the source pointer then moves on by two pixels (`add eax, 4` at `0x54e5a8`). At the end of an
output row it is not moved past the second source row (`0x54e5c2`..`0x54e5cb`), so output row k reads source rows k and
k + 1. The 888 and 8888 cases add two rows (`0x54e49f`..`0x54e4af`, `lea eax, [8*edi]` at `0x54e348`). The filter runs for
the levels below the stored ones when a bitmap has more than one level (`cmp ecx, 1; jle` at `0x54e9e3`, call at
`0x54ea00`). TGAs load as formats 5, 6 and 7 only (`bm_read_header` `0x502b4a`..`0x502b99`); a VBM with format code 2 or
more loads as 565 (`0x501df8`..`0x501e1b`). Retail: 7 of the 104 VBMs are 565 with stored mipmaps (`grslnd-h2o-nite.vbm`,
`ks-h2o.vbm`, `S-WatClean.vbm`, `water-test.vbm`, `water-testsmall.vbm`, `whirl.vbm` and `wlng-h2o.vbm`, all in `levelt.vpp`, 64 or 128 pixels with 1 to 3 stored
levels), so only their 16x16 and smaller levels are wrong. Those levels are drawn only at a distance (trilinear filter
`0x2703`, `0x54ebed`..`0x54ec0b`), so this is hard to see. Direct3D and Glide do not use this code.

**K51. Truncated before the clamp.** `gr_opengl_apply_tint` computes `(r + g + b) x tint_r` and stores it as a byte
(`mov byte [esi], al` at `0x54f9dd`) before `clamp_ubyte(.., 0, 255)` (`0x54f9df`), which can then change nothing. Same for
green and blue (`0x54f9fb`, `0x54fa19`). `gr_d3d_apply_tint` clamps the int (`clamp_int`, `0x5354ba`..`0x5354f8`) and
`gr_glide_apply_tint` the float (`0x54a56d`..`0x54a5a1`). So a white vertex (sum 765) with a tint of 0.5 gives 126 instead of
255. The function runs from `gr_opengl_bitmap`, `gr_opengl_line`, `gr_opengl_line_3d` and `gr_opengl_tmapper` while
`gr_tint_enabled` is set (`0x5501a4`..`0x5501af`). The hover and effect tints of `levelrender_draw_list` reach meshes
(`ca_render_mesh`, which clamps first at `0x50d3a1`..`0x50d3c3`) and level geometry (`levelrender_face_vertex_colors`), and
`gr_poly_buffer_flush` clears the flag while it replays (`0x545d9e`, `0x546028`). We found no draw that reaches this
function with a tint set.

**K52. No `calling_a_command_function` test.** `execute_command_line` also calls a command's function for a status query
("?", `0x50a2ed`..`0x50a319`, flags at `0x50a4af`..`0x50a4bb`) and for help ("help", "man" or a "?" in the line,
`0x50a324`..`0x50a389`, flags at `0x50a4ed`..`0x50a4fd`; call at `0x50a50a`), with `calling_a_command_function` 0.
`cmd_load_mipmaps` tests it (`0x501b50`..`0x501b57`). `cmd_change_texture_res` does not: it calls
`bm_set_reduce_textures(!bm_reduce_textures)` and prints "Low resolution textures are now %s!" every time
(`0x501c90`..`0x501cd3`), and it has no help text. A developer console command, so no retail path reaches it.

**K60. No + 1 on the depth clear.** The clip rectangle keeps its right and bottom edges inclusive: `gr__set_clip`
stores `w - 1` and `h - 1` (`lea edx, [ecx - 1]` at `0x50684f`, `dec ecx` at `0x50685b`, stored at `0x50686f` and
`0x50687b`). `gr_d3d_clear` turns them into an exclusive `D3DRECT` with `+ 1` (`lea eax, [edx + eax + 1]` at
`0x5355a3`, `lea eax, [edx + ecx + 1]` at `0x5355b3`), and so does `gr_opengl_clear` (`0x54f828`, `0x54f839`).
`gr_d3d_zbuffer_clear` builds the same rectangle without it (`add edx, eax` at `0x53562f`, `add eax, ecx` at
`0x535640`) and clears only the z buffer (`push 2`, `D3DCLEAR_ZBUFFER`, at `0x535654`). So column `x2` and row `y2`
of the clip area are never cleared. `gr_zbuffer_clear` runs once per scene, from `levelrender_render_scene`
(`0x482545`) and `gr_setup_3d` (`0x50df38`). `gr_opengl_zbuffer_clear` clears the whole buffer with `glClear` and has
no rectangle (`0x54f8af`). *Inferred:* with the reversed depth test (`D3DCMP_GREATEREQUAL` against a clear value of
0.0 on every card except S3, `0x539f04`..`0x539f0e`), a new pixel in that column or row is drawn only when it is at
least as near as last frame's, so the right and bottom edge of the 3D view can show a one-pixel line of the clear
colour while the camera moves. Not tested. Direct3D only. Found in the `NOTES-gr-direct3d.md` notes.

**K61. Index 3 of a three-float colour.** `world_tint_gold_object` passes red `[0x594424]` (0.98), green
`[0x594428]` (1.0) and blue `[0x594430]` (`0x43a62e`..`0x43a63b`). The colour's third float, 0.51, is at `0x59442c`,
and nothing reads that address. `0x594430` is the string "gold" (`646c6f67`, about 1.7e22 as a float; `obj_is_gold`
pushes it as a string at `0x46a7b2`). `world_hover_tint_object` scales each channel by `(pulse + 1) x 0.5`
(`0x43a681`..`0x43a6cc`), and `levelrender_draw_list` hands the result to `gr_set_tint` for the tinted object
(`0x48628c`..`0x4862a2`). The mesh renderers multiply the vertex brightness by the tint and convert with `_ftol`
without a clamp (Direct3D `0x50c8f2`..`0x50c90c`, OpenGL `0x50d3f3`..`0x50d40e`). `_ftol` uses `fistp qword`
(`0x56c7fd`), so a product of about 1e22 gives the 64-bit "integer indefinite" `0x8000000000000000`, whose low byte
is 0. The highlight is started for 5 s by `hud_set_timed_highlight`, whose only caller is `monster_drop_loot`
(`0x45306f`). *Inferred:* every loot drop flashes yellow (red and green only) instead of a pale gold. Any renderer
that goes through these mesh paths. Found in the `NOTES-target-chattracker-multispell.md` notes.

**K62. The reason pointer is a shared buffer.** For `KICK <channel> <nick> :<reason>`, the parser copies word 0
(channel) and word 1 (nick) into stack buffers (`0x4ace72`..`0x4ace9b`, `0x4acea4`..`0x4acecd`) but keeps word 2
as the pointer `irc_get_word` returns (`mov esi, eax` at `0x4acedf`). That pointer is always `irc_word_buf`
(`0x247999c`, `mov eax, 0x247999c` at `0x4ad982`). The next call, `irc_get_word(1)` for the "were we kicked" test
(`0x4acee4`), overwrites that buffer with the nick. `sprintf` then gets `esi` as the last `%s` of
"*** %s has kicked %s from channel %s (%s)" (string 241, `push esi` at `0x4acf18`). This needs a working chat
server (PXO is offline). Found in the `NOTES-target-chattracker-multispell.md` notes.

**K63. Squared distance against a plain radius.** For a character, the pick radius is `2 x character_size_radius`
(`fadd st(0), st` at `0x43c6eb`). After the sphere test against the closest point on the camera ray (`r^2 > d^2`,
`0x43c78c`..`0x43c79d`), the radius is halved (`0x43c79f`..`0x43c7a9`) and compared with `dz^2 + dx^2`, which is not
square-rooted (`fld [esp+0x14]; fadd st, st(1); fcomp [esp+0x48]` at `0x43c7ad`..`0x43c7b3`). So the horizontal limit
is `sqrt(R)` instead of `R`. The size radii are 0.15, 0.47, 1.0 and 99.9 (`character_size_radii` `0x59d7c8`):
for 0.47 the limit is 0.69, and for 0.15 the sphere test (0.30) is what stops it. Bosses take their radius from
`charinfo_boss_radius` (`0x4542ca`), so a boss radius above 1 gets a narrower test than intended. Type 3 objects
and type 11 (`0x43c6c2`, r = 2.0, limit 1.0) go through the same code. *Inferred:* the effect on clicks was not
tested. Found in the `NOTES-target-chattracker-multispell.md` notes.

**K64. OpenGL reports a constant 8 MB.** `summoner_cfg_load` sets texture quality 2 (`0x43e2e0`) and, when
`summoner.cfg` is missing (`jne 0x43e36c` at `0x43e32e`) or shorter than 64 bytes (`0x43e366`..`0x43e36a`), lowers it
to 0 when `gr_get_video_memory_mb() <= 8` (`0x43e36c`..`0x43e376`). For OpenGL that function returns the constant 8
(`mov eax, 8` at `0x507a03`), so the test is always true. Texture quality 0 is `bm_set_reduce_textures(1)`
(`sete al` at `0x4872d8`, `0x4872dc`). `summoner.cfg` is written only by `summoner_cfg_save` from
`options_apply_and_save` (`0x41faa3`), so this happens on a fresh install until the player saves the options, and
the saved file then keeps 0. Glide stores the small value it tests
against 4 and 12, probably MB (`gr_glide_init` `0x549e5d`..`0x549ea2`). Direct3D: see V60. Found while checking V60. SummonerFix now patches the constant to 64 (`bugfix.c`, 2026-10-15; the patch applies, checked in the log; the texture choice itself was not tested because that needs a missing `summoner.cfg`).

**K80. The message does not check the result.** `ai_can_be_knocked_down` (`0x475030`) refuses sub-modes `0xe`,
`0xc` and `0xd` (`0x475047`), but not casting. `ai_knock_down_local` then stores the duration, invalidates the
disable timer and calls `ai_set_mode(8, 0x2c)` (`0x4742fb`..`0x474316`). `ai_set_mode` refuses `AI_STATE_DISABLED`
while `ai_is_casting` (`0x47333e`..`0x47335e`) and returns 0. `ai_knock_down_local` ignores that result and still
shows "Knocked Down!" for `cur_character` (`0x47431b`..`0x47432f`). It also returns 1, so `ai_knock_down` sends
`mp_broadcast_ai_disable` in multiplayer (`0x47435d`..`0x474377`). The other machines run the same test on their own
copy (`mp_handle_ai_disable` `0x480f9f`), so a copy that is not casting there would fall (*inferred*, not traced).
Melee knock-downs come from `combat_apply_melee_hit` (3000-7000 ms, `0x466286`..`0x4662a4`), spells and the
Titus and Machival party knock-downs. Pushes (`ai_disable_2f_local`) and stuns (`ai_disable_31`) are dropped the
same way, without a message. Whether the game means casting to be uninterruptible is *unknown*; the message is the
slip.

**K90. No gamma under OpenGL.** `gr_set_gamma` builds `gr_gamma_ramp` with the curve
`255 * (i/255)^(1/gamma)` (`0x506ac1`-`0x506b10`). The second table, `gr_color_gamma_lut`, gets only `i` clamped
to 0..255 (`0x506b12`-`0x506b63`): an identity table. The back-end call has cases for Direct3D and Glide only
(`0x506b65`-`0x506b7a`). Direct3D uploads the ramp with `IDirectDrawGammaControl::SetGammaRamp` (`0x53ab6d`) and
Glide calls `grGammaCorrectionValue` (`0x54bac6`). The OpenGL back end has neither, and Sum.exe imports no gamma
ramp function (the only gamma string in the file is `grGammaCorrectionValue`). `gr_opengl_tmapper` maps
vertex colours through the identity table (`0x550150`-`0x550182`). The ramp reaches OpenGL only in
`gl_convert_a8_to_rgba8888` (`0x54d5f3`): the alpha of 8-bit-alpha bitmaps, at upload time. The option slider
(0.7 to 2.2, `options_apply_gamma` `0x41f380`) therefore changes only how opaque those textures are, and only for
textures uploaded after the change: nothing flushes the texture cache. Fonts are built in `GameInit`
(`0x43df76`-`0x43df9e`) before the saved gamma is applied (`0x43e110`), so they use gamma 1.0. The identity table
is right for Glide, where the hardware applies the curve; the OpenGL renderer never got a replacement.

Live with SummonerFix, which runs the game on OpenGL. **How a fix could work** (not written):

1. *Hardware ramp, smallest change.* Build a `WORD[3][256]` ramp from `gr_gamma_ramp` (`0x25d8e0c`, 256 ints,
   shifted left by 8 as `gr_d3d_set_gamma` does at `0x53ab3d`-`0x53ab61`) and pass it to GDI
   `SetDeviceGammaRamp` on the window's DC. SummonerFix already hooks `SwapBuffers` (`display.c`), so it can
   compare `gr_gamma` (`0x5a6064`) with the last value each frame without patching game code. Alternatively
   patch the OpenGL fall-through of `gr_set_gamma` (`0x506b71`, `jne 0x506b7f`) into a call to a stub. It needs
   the original ramp saved with `GetDeviceGammaRamp` and restored on exit, on focus loss and in the crash path.
   It changes the whole monitor, like the Direct3D and Glide renderers do. Windows refuses ramps that differ
   too much from identity, so the 2.2 end of the slider may fail; HDR displays and some drivers ignore it.
2. *Post-process in the window.* In the `SwapBuffers` hook, before the real call, copy the scaled back buffer
   to a texture (`glCopyTexSubImage2D`) and draw it back over the game area through the same curve: a fragment
   program with a 256-entry lookup texture (through `wglGetProcAddress`), or, with only OpenGL 1.1, `glPixelMapfv`
   with `GL_MAP_COLOR` and `glCopyPixels`. Skip it when `gr_gamma` is 1.0. This touches only the game window and
   needs no restore. It is more code, and the `glCopyPixels` path is slow on modern drivers. Savegame thumbnails
   are read earlier in the frame (`saveload_capture_thumbnail`), so they stay uncorrected, as with a hardware
   ramp.
3. *Vertex colours only.* Fill `gr_color_gamma_lut` with the ramp values (patch the second loop or write the
   table after each `gr_set_gamma`). That matches what the Direct3D software path does to vertex colours, but it
   leaves textures, the UI and bitmaps unchanged, and it would double the curve under Glide. Only a partial fix.

Option 1 matches the other two renderers. Option 2 is the cleanest for a windowed or borderless game.

**K91. Gamma twice on Direct3D.** `gr_d3d_tmapper` passes a vertex's own colour through `gr_gamma_ramp`
(`0x536247`-`0x536265`), and `gr_d3d_tmapper_multipass` does the same for the pass colours (`0x5366b6`-`0x5366fa`).
`gr_d3d_set_gamma` also loads the ramp into the display hardware when the gamma-control interface exists
(`0x53ab27`-`0x53ab6d`). `gr_d3d_init_device` gets that interface only when DirectDraw reports caps bit `0x20000`
(DDCAPS2_PRIMARYGAMMA, `test [0x2dc0610], 0x20000` at `0x539d8a`). On such a device lit polygons get the curve
in software and again on the screen, while textures and the UI get it once. Without the cap only those lit
polygons get any gamma. Glide avoids this by using the identity table for vertex colours (`0x54b2bf`).
*Inferred:* the visual effect was not tested. Live with the retail Direct3D renderer at any gamma other than 1.0.

**K92. Stale fog distances.** `gr_set_fog` stores the fog colour and the flag first (`0x507dc9`, `0x507dd4`). With
fog on it limits `near` to half of `far` (a quarter on Glide), then jumps to the back-end call when `near < 0`
(`0x507e1c`) or `far < near` (`0x507e2f`). The fog stays on with the previous call's distances and ranges, and
the back end is told to apply them (`push 1` at `0x507f4f`, `0x507f5a`, `0x507f65`). The game's callers are
`levelrender_fog_enable` and `levelrender_fog_disable` (fog off). `fog_set_distances` replaces values `<= 0` and
keeps far above near (`0x486b10`-`0x486b7a`), but `level_set_current_layer` copies a layer's two fog values
straight into the globals (`0x48218a`-`0x4821b6`), and only the first is tested `> 0`. So a level whose layer fog
has a negative distance would trigger it. *Inferred:* no retail level is known to do so (latent). A far distance
of exactly 0 with a positive near one is the related case in Still unclear.

**K93. Console anti-aliasing flag.** `cmd_antialiasing` toggles its own flag `cmd_antialiasing_on`
(`0x25d9224`, 0 at start) and passes the new value to `gr_set_antialias` (`0x505db9`-`0x505dda`). The options
setting goes through `config_antialias` (`0x60acf4`) and `options_apply_detail` (`0x487330`-`0x48733c`) and never
touches that flag. With anti-aliasing on in the options, the first command calls `gr_set_antialias(1)` again and
prints "antialiasing toggled!", and the options screen keeps showing the old setting after a real toggle. Only
reachable with the Direct3D renderer on a device that sets `gr_antialias_enabled` (`0x539dd5`); elsewhere the
command prints "not supported". A developer console command, so latent.

## Invisible

| # | Effect | Function | Evidence |
|---|---|---|---|
| V1 | Sets the time of every trigger slot whose name does *not* match | `character_set_skeleton_trigger_time` `0x540d70` | **Observed**; no caller |
| V2 | Only 4 of the 6 large streamed-effect slots are used | `effects_alloc_stream_slot` `0x51c520` | **Observed** code, *inferred* effect |
| V3 | Latent: "Alt+" in a UI table hotkey gives Shift | `ui_parse_hotkey` `0x528430` | **Observed** code |
| V4 | Latent: a TGA with an image ID field is read from the wrong offset | `tga_read_bitmap` (`File__seek` at `0x5556f7`) | **Observed** code |
| V5 | A speed change on one animation fills all 5 speed slots | `character_instance__start_anim` `0x51aee0` | **Observed** code, *inferred* effect |
| V6 | Latent: the ring of small streamed-effect slots restarts at slot 0, 1, 4 or 5 only | `effects_find_stream_slot` `0x51c5c0` | **Observed** code |
| V7 | Latent: the registry helpers read past an "HKEY_..." string without a backslash; delete formats its error with NULL | `os_registry_resolve_key` `0x511e50`, `os_registry_delete` `0x5123e0` | **Observed** code |
| V8 | Freed font slots are not reused, and the kerning search can apply another character's pair | `gr_font_create` `0x508560`, `gr_font_get_char` `0x508440` | **Observed** code and retail data |
| V9 | The UI table loader has no caller; it also ignores a missing file, never sets window mask data and drops several button options | `ui_load_table` `0x52b200`, `ui_window__set_mask_bitmap` `0x52b930`, `ui_parse_button` `0x528530` | **Observed**; no caller |
| V10 | Every line skipped by `SkipToNextLine` is counted twice in parser line numbers | `TextFile__SkipToNextLine` `0x515af0` | **Observed** code |
| V11 | Latent: a negative number with 3k digits is printed as "-,123,456" | `int_to_string_with_commas` `0x5152a0` | **Observed** code |
| V12 | Latent: a hex number at the very end of a table reads past the text | `TextFile__ParseInt` `0x516290`, `TextFile__ParseUInt` `0x516380` | **Observed** code |
| V13 | Latent: the list parsers ignore their maximum count | `TextFile__ParseIntList` `0x516940`, `TextFile__ParseFloatList` `0x516a30`, `TextFile__ParseStringList` `0x5168b0` | **Observed** code and retail data |
| V14 | The 16-to-16-bit TGA converter writes nothing to its output | `tga_convert_pixels` `0x555020` | **Observed**; no caller reaches it |
| V15 | An item placed on a pick layer is always placed again by the general floor probe | `item_obj_spawn` `0x46a1b0` | **Observed** code, *inferred* effect |
| V16 | Two character-instance helpers lose their result | `character_instance__get_anim_root_velocity` `0x51b1e0`, `character_instance__get_bone_bind_inverse` `0x51ae00` | **Observed**; no caller |
| V17 | Latent: the `-glide` and `-pack` command-line options are accepted but never read | `cmdline_read_params` `0x43dc30`, `LoadSettings` `0x43d7f0` | **Observed** code |
| V18 | Latent: `extract_angles` gets the pitch wrong for a pitched matrix whose heading is not a multiple of 90 degrees | `matrix3__extract_angles` `0x510240` | **Observed** code; its only pitch user is unreachable |
| V19 | `matrix43::transpose_in_place` is not a transpose; `skeleton::get_root_rotation` slerps from an uninitialised quaternion | `matrix43__transpose_in_place` `0x511bb0`, `skeleton__get_root_rotation` `0x544010` | **Observed**; no caller |
| V20 | Latent: a mesh file older than version 6 (or one that fails to read) leaks its mesh record | `vim_load_file` `0x5438b0`, `vim_alloc` `0x543040`, `vim_free` `0x5430b0` | **Observed** code and retail data |
| V21 | Latent: a mesh with more than 6 textures overwrites its texture count and flags | `vim_read` `0x543430` | **Observed** code and retail data |
| V22 | The level colour-record reader clamps green into blue and never clamps green | `s3d_read_color_record` `0x470300` | **Observed** code; nothing reads the records |
| V30 | Latent: `level_free_s3d` clears the navpoint and colour-record pointers but keeps their counts | `level_free_s3d` `0x46fdc0` | **Observed** code |
| V31 | Latent: waving level objects add their corner normals into two arrays that are never cleared or read | `levelrender_wave_lobj` `0x48de60` | **Observed** code |
| V40 | Game stage 19 sets `trapped_luminar` on level 14 (KhosaniLab) instead of 22 (IonaExt02), which does nothing | `game_stage_set` `0x4d42e0` | **Observed** code and retail data |
| V41 | Latent: the automap bit functions accept an index up to 8 times too large | `savegame_get_bit` `0x4cff20`, `savegame_set_bit` `0x4cff80` | **Observed** code |
| V50 | The OpenGL texture memory count is wrong, and an updated texture is uploaded twice | `gr_opengl_create_texture` `0x54e840` | **Observed** code |
| V51 | `gr_opengl_setup_view` looks at the forward vector instead of eye + forward and uses a near plane of 0 | `gr_opengl_setup_view` `0x550500` | **Observed**; no caller |
| V60 | Latent: Direct3D reports its video memory in bytes, so the start-up check that lowers texture quality on cards with 8 MB or less never fires | `gr_d3d_init_device` `0x5395c0`, `gr_d3d_get_video_memory_mb` `0x53aaf0` | **Observed** code |
| V61 | Latent: the DirectDraw driver list stops after the first driver, so only the primary display driver's Direct3D devices are ever listed | `d3d_ddraw_enum_callback` `0x5389f0` | **Observed** code |
| V70 | Five hero moves inherit weapon-class bits from a dropped hurt move, so their anims also load for those weapon classes | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data |
| V71 | On masad (not masad_v2), Liangshan and JadeTemple, the rider moves that are skipped keep skeleton index 1 instead of -1 | `move_used_on_level` `0x444bb0` | **Observed** code, *inferred* effect |
| V80 | Latent: the `ai_state` console command rejects every state name ("Error: Invalid state name") | `cmd_ai_state` `0x474d10` | **Observed** code |
| V81 | Latent: the `ai_frame_update_rate`, `attack_point` and `offense_point` console variables are stored and never read; `attack_point` also accepts an index one past the target's last attack point | `cmd_ai_frame_update_rate` `0x4720e0`, `cmd_attack_point` `0x473260`, `cmd_offense_point` `0x473300` | **Observed** code |
| V82 | The branch of `ai_knock_down_local` for a character that is already down can never run; if it did, it would restart the lying phase with the first knock-down's duration | `ai_knock_down_local` `0x4742b0`, `ai_can_be_knocked_down` `0x475030` | **Observed** code |
| V90 | `LoadSettings` passes the vsync option as `gr_init`'s far-z argument (0.0 or 1.0) and a constant 0 as its vsync argument | `LoadSettings` `0x43d7f0`, `gr_init` `0x505e00` | **Observed** code, *inferred* intent |

**V1.** The `strcmp` result decides the store the wrong way round (`je 0x540dde` at `0x540dbe` skips a
match). Nothing calls the function, so it never runs.

**V2, V6. Mask 5 instead of a modulo.** `effects_alloc_stream_slot` indexes its 6-slot ring with
`and eax, 5` (`0x51c533`), which only gives 0, 1, 4 and 5. Slots 2 and 3 are never used, so large
streamed effects are evicted sooner than intended. `effects_find_stream_slot` scans its 8 slots with
`and eax, 7` but stores the next start with `and edx, 5` (`0x51c613`), so all 8 slots are still used.

**V3.** "Alt" sets the same flag as "Shift" (`mov bl, 1` at `0x5284a1` and `0x528481`). Only `ui_load_table`
uses this parser, and nothing calls it (V9).

**V4.** With a non-zero ID length, the reader seeks to that length from the start of the file (origin 0)
instead of past the 18-byte header. A seek is only made when the length is non-zero (`jbe` at `0x5556e4`).
*Inferred:* the retail TGAs have no ID field.

**V5.** With a speed other than 1.0, the loop at `0x51af53`..`0x51af65` stores `{anim, speed}` in every free
one of the 5 slots, with no break after the first.

**V7.** After the root name, `os_registry_resolve_key` reads `[esi+1]` (`0x511eb4`) whether `esi` stopped on the
`\` or on the terminator. The root-name copy into the 256-byte stack buffer has no length check
(`0x511e9b`..`0x511ea8`). `os_registry_delete` with `name == NULL` formats "Error removing registry key '%s'"
with that NULL name. No caller passes an "hkey..." string (the only such literals are `os_registry_hkey_from_string`'s
own, `0x5a7554`..), and `os_registry_delete` has no caller.

**V8.** `gr_font_create` remembers the first free slot but loads into slot `gr_num_fonts`. `gr_close_fonts`
(`0x5081d0`, only from `gr_close`) does not reset that count. The game loads its three fonts once in `GameInit`
(`0x43df76`..`0x43df9e`), so no slot is ever freed and reloaded. After the kern loop, `gr_font_get_char` compares
only `ch1` (`cmp edx, edi` at `0x5084d9`), not `ch0`. When the loop runs off the character's pairs, it can apply
the next character's pair. If it runs off the end of the table, it reads one record past it. None of the 7 retail
fonts has any kern pairs (`num_kern_pairs` 0), so this never runs.

**V9.** `ui_load_table` has no call and no pointer in the image (we searched the bytes for `0x52b200`). Every
`ui_parse_*` function, `ui_parse_hotkey` (V3) and `ui_window__load_mask_bitmap` (`0x52b42a`) is reached only from
it. The defects inside it are therefore dead code:
- a missing file only formats a message (`0x52b25e`);
- without "+Name:" it copies an uninitialised stack buffer (`0x52b3a3`..`0x52b3ca`);
- `set_mask_bitmap` never writes `mask_data` (`+0x4d8` is written only by the constructor, `0x52b07c`);
- `ui_parse_button` reuses the previous target for an unknown state name.

`ui_window__init` (live, used by the script editor and two debug states) calls `get_flags` in both list walks and
ignores the result (`0x52b1a1`..`0x52b1d3`). Every window is appended last, so a modeless window created while a
modal one exists goes after it. No retail path creates such a pair.

**V10.** The loop stops on the CR (`cmp byte [eax], 0xd`). It counts the line (`0x515b13`) and then tests the
same CR for LF (`cmp dl, 0xa` at `0x515b19`), so the cursor stays on the CR. The next `SkipWhitespace` counts the
CR again. Only `localize_load_level_table` calls it (`0x504a37`, `0x504bb9`), and the following `Accept` skips the
CR, so only the line numbers in parse error messages change. Live (every level string table), invisible.

**V11.** The digits before the first comma are `(len - neg) % 3 + (neg != 0)`, and 3 only when the sum is 0
(`0x515331`..`0x515344`). For -123456 this gives "-", then ",123,456". The size check counts
`(len - neg - 1) / 3` commas (`0x5152e3`..`0x5152ef`), one too few for that case. Callers are `status_draw_level`
and `ring_format_info`, which pass experience values, never negative.

**V12.** The hex loop tests each character with `strchr("0123456789abcdefABCDEF", c)` (`0x5162d4`..`0x5162e3`).
`strchr` also finds the terminator, so a NUL is taken as a digit ('A'-'F' branch, `0x516314`) and the cursor goes
past the end. It needs a `0x` number as the last bytes of a file (retail tables end with `#End`).

**V13.** The loops end only on `)` or NUL (`0x5169fe`..`0x516a02`, `0x516a83`..`0x516a8a`). The max-count
argument (8 for `+RenderMask:` at `0x44b13d`, 3 for `+LightColor:` at `0x44659b`) is never read. Retail: the
longest `+RenderMask:` list has 5 parts, and no character table has `+LightColor:`. `ParseStringList` has no caller.

**V14.** For 16-bit to 16-bit, `tga_convert_pixels` toggles bit 15 of the *source* in place (`xor word [eax],
0x8000` at `0x555042`), and the caller still advances by the returned size. Its only callers are
`tga_rle_encode_row` from `gr_screenshot`, which always asks for 3-byte output (`0x5063fa`), and the
unreferenced `tga_write_rle` (`0x555970`).

**V15.** The layer loop over all level objects has no break, so `i >= num_lobjs` is always true after it, and
`layer` becomes -1 (`0x46a52d`..`0x46a535`). The general `lobj_raycast_all` probe (`0x46a5d8`) then places the item
again from 3 units above the layer floor. It was probably meant to run only when the layer probe found nothing.
*Inferred:* the result differs only where another surface lies within 3 units above the layer floor.

**V16.** `get_anim_root_velocity` stores zero for an action animation and then overwrites it unconditionally
(`0x51b1fc` jumps only past the zero, `0x51b21a`..). `get_anim_root_speed` (`0x51b250`) does return 0 there.
`get_bone_bind_inverse` inverts a temporary copy and drops it. Neither function has a caller.

**V17.** `cmdline_read_params` stores whether `-pack` was found at `0x60ad70` (`0x43dc8b`), and nothing reads that
byte. The `-glide` object (`0x60ad50`) is only referenced by its constructor and destructor (`0x43d37c`, `0x43d3a0`).
Glide is the default renderer, but a Direct3D choice from the registry (`0x43dade`..`0x43daf0`) is not overridden by
`-glide`; only `-direct3d` is tested (`0x43daf5`..`0x43db03`). The launcher passes typed options (`CmdLineOptions`),
so a player could give them. The two bytes `0x60ad6d`/`0x60ad6e` that `LoadSettings` tests (`0x43d95b`, `0x43d968`)
are never written either (removed options, no effect).

**V18.** `set_from_sincos` builds `fvec = (cos p sin h, -sin p, cos p cos h)` (`0x510086`, `0x5100b0`, `0x5100a7`).
`extract_angles` estimates cos p as `sin h x fvec.x` or `cos h x fvec.z` (`fmul` at `0x5102b2` / `0x5102bd`), where a
divide is needed. That gives cos p x sin^2 h, so tan(pitch) comes out up to twice too large. Heading and bank are
right. Of the 12 call sites, 11 use only the heading: `character_save_state` (`0x4572b8`), `character_ai_move_frame`
(`0x45c688`), `camera_sync_yaw_to_target` (`0x4798d7`), `mp_send_obj_update` (`0x480584`), `mp_handle_obj_update`
(rebuilds with pitch 0 and bank 0, `0x48083e`), `level_add_navpoint` (`0x4d57a6`), `script_parse_navpoint(s)`
(`0x4db553`, `0x4db8ab`), `ls_worldmap1_frame` (`0x4fbdbb`) and `build_bone_matrices` (`0x51a05f`). The one that uses
the pitch, `cutscene_camera_sequence_advance` (`0x4944b9`, `0x4944d0`, then `set_from_angles` at `0x4945f2`), runs only
for a camera sequence in state 1 (`0x4794bb`..`0x4794ce`). The only sequence class is built with state 0
(`push 0` at `0x493f83`), and nothing writes the state afterwards, so that branch is dead.

**V19.** `transpose_in_place` moves `m[1] -> m[4] -> m[5] -> m[9] -> m[3] -> m[1]` and
`m[2] -> m[8] -> m[10] -> m[7] -> m[6] -> m[2]` (`0x511bb1`..`0x511c02`). A transpose would swap 1/3, 2/6 and 5/7, and
`m[9]`, `m[10]` belong to the position. `get_root_rotation` passes a default-constructed quaternion (`0x544051`, no
initialisation) as the slerp start (`quaternion_slerp` at `0x54405c`). Neither function has a call, and their addresses
do not occur in the image.

**V20.** `vim_load_file` returns 0 for a version below 6 (`0x5438e8`) and for a failed `vim_read`
(`0x543908`..`0x54390d`) without freeing the record from `vim_alloc`. `vim_alloc` writes `+0x2c` even when `new`
returned NULL (`0x54307c`..`0x543086`), and `vim_free` frees nothing when the data pointer is NULL (`0x5430bc`).
Retail: 820 of the 825 meshes are version 6. The other five (GHorse1.vim version 2; Pijan.vim, Pijan_shadow.vim,
prop-minosickle_shadow.vim and prop-sword-bastard_shadow.vim version 3) are never loaded. Characters load the mesh
named after their table (Horse1 uses Horse1.vim, version 6; `charanim.tbl`'s "GHorse1.v3d" is not read, since Sum.exe
has no "+State:" literal). No table defines a Pijan character. Items page only the "prop-*.v3d" model
(`item_page_in_mesh` `0x46c7e2`), and only characters get the "_shadow" suffix (`character_load_shadow` `0x540e60`).

**V21.** The texture loop (`0x543835`..`0x543896`) stores each `bm_load` handle at `+0x10 + 4i` and re-reads the
count at `+0x28` on every pass (`0x54388a`). With 6 slots, texture 7 overwrites the count and texture 8 the flags at
`+0x2c`. The largest retail count is exactly 6 (Flece, Heavy Flece, Rosalind, their shadows, Prop-Legs-Leather02).

**V22.** The third clamp loads green (`fld [ebp]`, `+4`, at `0x4703bf`), clamps it to the constant at `0x5819f8`
and stores the result into blue (`fstp [ebx]`, `+8`, at `0x4703f2`). `level_load_s3d` reads the records into level
`+0x5c` (count `+0x58`, `0x46e6f5`..`0x46e70d`). There is no other reference to `0xa5a2f8` / `0xa5a2fc`, and
`level_free_s3d` only clears the pointer (`0x46fed0`), so nothing reads the colours. Found in the `s3d.cpp` notes.

**V30.** `level_free_s3d` stores 0 at `+0x3c` (`level_navpoints`, `0x46fea9`) and `+0x5c` (colour records,
`0x46fed0`) but not at `+0x38` (`level_num_navpoints`) or `+0x58`. `level_marker_find` (`0x4774ac`) and
`script_editor_draw_navpoints` (`0x4e3678`) loop the count over the pointer, so a call between `level_unload`
(only caller `state_game_leave`) and the next load would read through NULL. Nothing calls them there, and the
next `s3d_read_header` sets the counts again. Found in the `s3d.cpp` notes.

**V31.** For each face of an object with draw flag `0x20`, `levelrender_wave_lobj` adds the three corner normals
into `levelrender_wave_vertex_normals` (`0x2396ca8`) and 1.0 into `levelrender_wave_vertex_face_count`
(`0x238a170`) (`0x48e342`..`0x48e3d7`), then normalises every one of the 12000 entries with a non-zero count
(`0x48e3fa`..`0x48e426`). No other code reads or clears either array (no other reference to `0x238a1xx` or
`0x2396cxx` past `0x2396ca0`), so the work is wasted and the counts only grow. Whether a retail level object has
flag `0x20` was not checked, so this is counted as latent. Found in the `levelrender.cpp` notes.

**V40. A copied level number.** Stage 19 reads `trapped_luminar` on level 22 (`push 0x16` at `0x4d4343`). If it is set
and `trapped_titus` is not, it sets `trapped_luminar` again, on level 14 (`push 0xe` at `0x4d437a`). In `events.tbl`
the flag is only in `$Level: "ionaext02"` (level 22); `khosanilab` (level 14) has `got_ring_of_fire`. Stage 6 has the
same "if not A set B, else if not B set A" shape on levels 14 and 12, so the 14 was probably copied from there.
`savegame_set_flag` ignores a name that is not in the level's list (`0x4cfd90`), and the branch runs only when level
22's flag is already set, so nothing changes.

**V41. Bound in bytes, index in bits.** Both functions load the level's automap bit count (`[esi+0x28]`, layers x
4096) and compare the index against it shifted left by 3 (`shl edi, 3` at `0x4cff44`; `shl edx, 3` at `0x4cffae` in `savegame_set_bit`).
An index past the real count reaches the level's int and float variables and the next levels' blocks. The callers
`automap_is_cell_revealed` (`0x4300c0`) and `automap_reveal_cell` (`0x430110`) check the layer but not x and y,
so a cell outside 0..63 would get through. We know of no caller that passes one.

**V50.** The byte count is `2 x w x h` of the last level the upload loop processed (`imul esi, ebp; shl esi` at
`0x54ece0`..`0x54eced`), so a mipmapped texture counts 2 bytes (its 1x1 level) and a 32-bit one half its size. It feeds
`gl_texture_mem_frame` and `gl_texture_mem_used` (`0x54ecfc`..`0x54ed08`), which `gr_opengl_tcache_flush` only prints into
`string_buffer`. With `update` set, level 0 goes up with `glTexSubImage2D` (`call [0x57e30c]` at `0x54ebd0`) and then again
with `glTexImage2D` (`call [0x57e31c]` at `0x54ec2b`), because the sub-image branch jumps to the common upload
(`jmp 0x54ec11` at `0x54ebd6`). Only the time is lost.

**V51.** `gluLookAt` gets `gr_eye_pos` as the eye and `gr_eye_matrix.fvec` (`0x26927d8`..`0x26927e0`) as the centre, not
eye + fvec (`0x55051a`..`0x550586`), and `gluPerspective` gets near 0 (`push 0; push 0` at `0x5505a8`). Neither the address
nor a call to it occurs in the image.

**V60. Bytes, not MB.** `gr_d3d_init_device` asks `IDirectDraw7::GetAvailableVidMem` (vtable `+0x5c`, `0x539fa6`) for
texture memory (`DDSCAPS_TEXTURE`, `0x1000` at `0x539f75`) and stores the free amount, in bytes, into
`gr_d3d_video_memory_mb` (`0x53a04d`). `gr_d3d_get_video_memory_mb` returns it unchanged (`0x53aaf0`), and so does
`gr_get_video_memory_mb` (`jmp 0x53aaf0` at `0x507a0e`). `summoner_cfg_load` compares the result with 8
(`cmp eax, 8; jg` at `0x43e371`) when `summoner.cfg` is missing or short (K64), so with Direct3D the test that is
meant for 8 MB cards is false on any card the game accepts (it needs 5 MB free, see "Checked, not bugs"). Glide
returns its small value (probably MB). On any card of more than 8 MB the MB test is false too, so this only matters on 1999 hardware.
Direct3D only. Found in the `NOTES-gr-direct3d.md` notes.

**V61. `DDENUMRET_CANCEL` after the first driver.** `d3d_ddraw_enum_callback` copies the GUID and names into the
next of up to several driver entries and increments `d3d_num_ddraw_devices` (`0x538a02`), but returns 0
(`xor eax, eax` at `0x538a58`, `ret 0x10` at `0x538a61`), which is `DDENUMRET_CANCEL`. `DirectDrawEnumerateA`
(`0x538867`) calls it first for the primary display driver, so the loop in `d3d_enumerate_ddraw_devices`
(`0x538884`..`0x5388f6`) always sees one driver. Of its devices, those with hardware transform and lighting are
skipped (`test [ecx], 0x10000` at `0x53892c`, `D3DDEVCAPS_HWTRANSFORMANDLIGHT`), and the last one listed is used
(`lea eax, [8*edx + 0x2dc2568]` at `0x538914`, the entry before `d3d_num_3d_devices`). `DirectDrawEnumerate`
without `Ex` lists only the primary driver and 3D-only add-in cards, and the 3dfx ones among those are refused in
Direct3D mode anyway (`0x539e73`..`0x539ec4`). With the enumeration continuing, a Voodoo 1 or 2 would be the last
device and the game would quit with message 943, so the early stop has no bad effect we know of. Direct3D only.
Found in the `NOTES-gr-direct3d.md` notes.

**V70. Bits ORed into a recycled record.** `+Animation class:` does `or ebx, edx` into `move_info +0xc`
(`0x44618d`..`0x4461a4`) and never clears it; only a move without the line gets a fresh 1 (`0x4461b2`). The
hurt-move drop frees the record at once (`0x44635f`), and the next `+Move:` gets the same record back from
the free list. Replaying the load order over the retail tables gives five such moves:

- Joseph's staff stand_to_ready (`josp_stv_stand_to_ready.mvf`) also gets pole arms;
- Rosalind's staff trip (`rosl_trip.mvf`) also gets swords, bows, crossbows and daggers;
- Rosalind's sword ready (`rosl_swd_ready.mvf`) also gets bows and crossbows;
- two Flece hurt transitions get bows, but they are dropped themselves.

In each case a later move of the same variation with the right class is nearer the list head, so
`charinfo_find_move` and `charinfo_find_anim` still pick the right anim. The trip and stand moves are not
attack variations 1-5, so `charinfo_collect_moves` does not see them. What remains is that
`characterinfo_load_party_anims` loads the extra anim into the hero's pool for those classes (the class test at
`0x4475fd`..`0x447601`).

**V71. Store before the test.** On these levels `move_used_on_level` writes `m->anim = 1`
(`mov dword ptr [esi+0x8], 0x1` at `0x444db3` and `0x444fb8`) before its anim-name tests. When a test
then skips the move (serp_, ghrd_, tigr_ and phnx_ stage anims), `charinfo_load_move_anim` returns without
loading, and the move keeps 1. The earlier skip tests (low parry and flinch, non-combat flinch, towns) return
before the store and keep -1. *Inferred:* the rider records are not heroes, so the loaded-only lookups ignore
`anim` for them. A lookup by skeleton index (`charinfo_find_move_any`, used by the level script actions) could
return a skipped move for skeleton 1; this was not traced.

**V80. Pointer compare instead of a string compare.** After `process_command(2)` the loop compares each
entry of `ai_state_names` (`0x59ad60`, 13 pointers into .data, `0x59b404`..`0x59b4e8`) with the address of the
`next_arg` buffer: `cmp dword ptr [eax], 0x25dacec` (`0x474d4f`). Other commands pass the same address to string
functions (`push 0x25dacec` in `cmd_play_cutscene` `0x437513`), so `next_arg` is a char array, and no table
pointer can equal it. The loop always ends with 13, and the command prints "Error: Invalid state name" and returns
(`0x474d62`..`0x474d77`). The help text lists the names correctly. A developer console command, so no retail path
reaches it.

**V81. Variables without a reader.** `tools/asm.mjs grep` finds `ai_frame_update_rate` (`0x59af5c`) only in
`cmd_ai_frame_update_rate` (`0x472102`, `0x472121`), `Ai_attack_point` (`0x59af60`) only in `cmd_attack_point`
(`0x4732b0`, `0x4732b7`) and `Ai_offense_point` (`0x59af64`) only in `cmd_offense_point` (`0x473318`). The help text
of `ai_frame_update_rate` says it sets the update rate for off-screen creatures, but the throttle in
`ai_update_perception` uses a 500-750 ms timer (`0x47301d`). `cmd_attack_point` stores the index and resets it to
-1 only when it is greater than the target's point count (`cmp eax, edx; ... jle` at `0x4732ae`..`0x4732b5`), so
an index equal to the count is kept. Nothing reads it, so this has no effect. Developer console commands only.

**V82. A test made twice.** `ai_can_be_knocked_down` returns false when `ai_is_knocked_down` (sub `0x2d`) is true
(`0x47506a`..`0x475073`). `ai_knock_down_local` returns 0 on that result (`0x4742b3`..`0x4742bf`) and only then
tests `ai_is_knocked_down` again (`0x4742c5`). Nothing in between changes the mode, so the branch at
`0x4742ce`..`0x4742f8` never runs. A second knock-down on a lying character is simply ignored, and is not sent in
multiplayer. If the branch ran, it would compare the new duration with the time left (`disable_ms` `+0x1a4` minus
the mode timer) and on a longer one restart the mode timer (`SetTimeout(0)` at `0x4742ec`) without storing the
new duration, so the character would lie for the first duration again. The note in `NOTES-ai.md` described that
second half; it has no effect in the game.

**V90. vsync in the wrong argument.** `LoadSettings` pushes 0 as `gr_init`'s seventh argument (`push 0x0` at
`0x43db1e`), and builds the sixth from `config_cfg_34 != 0` converted to a float (`0x43db08`-`0x43db30`).
`gr_init` stores the seventh argument to `gr_screen.vsync` (`0x25d8dd0`, `0x505eaa`) and the sixth to
`gr_screen.far_zvalue` (`0x505e87`). `config_cfg_34` is the vsync option: `options_apply_detail` copies it to
`0x25d8dd0` (`0x48734d`-`0x48735a`), which `gr_d3d_flip` (`0x53a718`) and `gr_glide_flip` (`0x54a762`) read. The
two arguments look swapped. Nothing shows: `far_zvalue` has no reader, and `GameInit` calls
`options_apply_detail` (`0x43e141`), which sets the real vsync value before play. Only the clear and flip
inside `gr_init` run with vsync 0.

## Claims that did not hold

| Claim | Where it was made | What the code does |
|---|---|---|
| A load sets `level_time` from header field `0xa8` (always 0) | [Savegame](savegame.md#header-observed) note | `savegame_read` pushes `&level_time` as the **fourth** argument (`push 0x60a654` at `0x4d2627`). `savegame_read_header` copies field `0xa8` into the third (a local, `[esp+0x20]` at `0x4d2ad7`) and field `0xac`, the saved time, into `&level_time` (`[esp+0x24]` at `0x4d2aea`). No swap; the savegame page needs a correction |
| `hud_portrait_get_color` case 3 computes red from green | HUD decompilation notes | True (`0x40f987`, `0x40f98b`), but `color_pulse_scale` (`0x414ae0`) scales three equal 1.0 inputs the same way, so the result does not change. Not counted |
| Loot gold is lost when the inventory is full | earlier notes | The gold loss is in the shop **buy** path (G5), not in loot pickup |
| Level select button 31 scrolls the wrong way | earlier notes | The button is in the multiplayer **character** select screen (K2) |
| Only slots 0, 1, 4 and 5 of the 8-entry stream ring are reachable | [`src/decomp` notes](../../src/decomp/README.md) | True for the 6-slot ring (V2). The 8-slot ring reaches all 8 (V6) |
| Invisibility lasts 11 s instead of 10 because of the parser | `src/decomp/game/NOTES-spells-combat.md`, `spells.cpp` comment at `status_effect_duration` | `+Loop Time:` sets status 8's tick period (`0x454b29`..`0x454b3a`), not its length, and status 8 has no tick action. The real effect of the parser slip is on cooldowns (G12) |
| After a container close the other machines keep the taken items and never delete an emptied container | `src/decomp/game/NOTES-multi.md` bug 1 | `container_remove_item` decrements the count even on a miss, so the last entries go and an emptied container is deleted (G14) |
| Life leech is `0x450c70` with -0.1 / 0.1 of the damage | [Combat](combat.md) | `0x450c70` is `character_add_ap`, used by mind drain; life leech heals with `character_add_hp` (corrected on the page) |
| Melee calls `character_get_resistance(-1)`, which may read out of bounds | `NOTES-spells-combat.md` open question 7 | Type -1 returns 0 before any table access (`0x4626bb`..`0x4626cb`) |

Not counted as bugs: `camera_key_extra_axis` (`0x4348a0`) always returns 0 and `camera_cut_to_view_stub`
(`0x479430`) is empty. They look like cut features ([Camera](camera.md)). The other oddities in the decompilation notes were checked again on 2026-10-15. Those that are bugs are in
the tables above; the rest are below.

**Checked, not bugs:**

- `ui_container::draw` one-pixel lines: both calls pass the same point twice on purpose (`0x5539fd`..`0x553a02`,
  `0x553b2c`..`0x553b43`). They are the corner dots of the label gap in the etched frame.
- `skills_handle_mouse` reading `skills_list[-1]`: mouse mode 2 is entered only after `skills_handle_click` has
  set `skills_hover_row` to the hit row, so the index is never -1 there (`0x428711`).
- `spells_update_member_buttons` without a multiplayer test: the buttons call `ui_party_prev` / `ui_party_next`, and
  `party_cycle` stops only on members of the local player in multiplayer ([Party](party.md)), so they do nothing harmful.
- `hud_interact_nearest` skipping items that are "in view": `gr_cull_sphere` returns 1 when the sphere is
  *outside* a plane (`mov al, 1` at `0x50ed2b`), so `item_obj_is_in_view` means "culled" and the skip is right.
- `pxochat_new_version_popup_frame` leaving for every choice above 0: the popup has only two buttons.
- `status_apply_skill_points` dead sum and `inventory_destroy_selected_item` testing `item_can_destroy` twice:
  no effect on the result.
- `gamecreate_levels_below_dec` stopping at 0: the code is consistent with `levels_above_dec`. Only the
  sum.symbols comment ("min 1") is wrong.
- `buysell_page_up` testing `jl`: equal in effect to the other screens' `> 0`.
- Loops without a break in `itemtrade_party_handle_mouse` and `pxochat_handle_click`: the matches are unique,
  so the last match is the only one.
- `cmdline_take_argument` formatting a message and dropping it: the debug print is compiled out.
- `gr_fit_string_String` returning "" when the string fits: no caller (`0x508df0`).
- `mainmenu_start_new_game`'s argument from `conversation_save_table[84]`: a naming problem, not a code fault.
- `assess_update_scrollbar` inner test on `line_count`: a dead branch, with no effect.
- `hud_portrait_get_color` case 3 (already on the page): not counted.
- `game_abandon_to_menu` dereferencing `mp_local_player` when `cur_character` is dead (`0x421d7e`). In single
  player that pointer is NULL, but an all-dead single-player party goes to HUD mode 5 (`0x4394ee`), which runs no
  key handling. Multiplayer has the pointer. *Unclear* rather than disproved, because we have not proved that the
  play menu can never open in mode 5.

- Backstab halved when the target is in combat mode (`je 0x4651b1` at `0x465193`): shared with the aimed
  attacks' `x 0.5`, and consistent with x2.5 against an unaware target. A design choice, not a slip.
- Status 16 blocking heals and dropping damage beyond its value (`0x450abc`..`0x450b1d`): consistent for a full
  shield; intent unknown, not counted.
- `spell_roll_amount` letting a resistance above 1 turn spell damage into healing (`0x47f583`..`0x47f592`): it
  looks deliberate (absorption), not counted.

- `cmd_stop_sound`'s help text says "Stops all sounds that are playing" (`0x5949cc`), but it stops only the `play_sound` stream (`console_sound_stop` `0x43d710`..`0x43d72e`). This is a wording mismatch in a console-only command. The handle that `console_sound_stop` forgets is no growing leak: `snd_get_handle` reuses a loaded sound by name (`_stricmp` at `0x52083c`). Not counted.
- `cfg_34` reaches `gr_init` as the constructor default (`0x43db08`), because `summoner_cfg_load` runs later in `GameInit`. `options_apply_detail` does apply the saved value (`0x48734d`), and no options screen writes it (only `init_game_config_defaults` and `summoner_cfg_load` do).
- `s3d_read_door` returns -1 on success and 0 on error (`neg; sbb; neg; dec` at `0x4702e9`..`0x4702f1`): `level_load_s3d` ignores the result (`0x46e63b`).
- `spell_target_value` (cure) tests status 0 (blind) twice (`0x47f2f3`, `0x47f304`). The id it skips, 2, is Faith, a buff, so the result is right.
- `levelrender_face_vertex_colors` takes the tint brightness from the sum of R+G+B clamped to 255 (`0x48f99e`..`0x48f9ee`). `levelrender_draw_list_glide` does the same (`0x48f3d4`..`0x48f418`), and character meshes use the sum x 0.75 (`0x50bb45`..`0x50bb6d`, constant `0x580e04`), so this is the tint model, not a slip.
- `skeleton::get_root_velocity` (`0x543fc0`) has no caller either (no pointer in the image); nothing wrong was found in it.

- Evil Urath's melee "never lands" (`boss_move_in_range_dispatch` has no `0xb1` case, table `0x4a100c`): true, but Urath never makes a melee attack. A type-9 character only acts through `boss_ai_frame` (`0x4bffad`), and `evil_urath_ai_select_action` only casts one of seven spells (`0x55`..`0x5b`, `evil_urath_cast_spell` `0x4c50cb`) or leaps (`0x4c4eff`). Not counted.
- `character_unload` never freeing the shadow mesh (`0x5413b0`..`0x5413c0`), moved from "Still unclear": every character load runs with a mempool active. `GameInit` sets each party mempool before its `character_load` (`0x43e146`..`0x43e1d2`), `charinfo_create_character_instance` sets `charinfo_select_mempool`'s pool (`0x448e0e`), and that function always returns a pool (player, summon or `level_mempool`, `0x448f20`..`0x448f50`). So `character_create_base` always sets flag `0x8` (`0x5416db`..`0x5416e9`) and the free branch never runs.
- `s3d_read_color_record` green into blue: already V22.
- `s3d_read_door` returning -1 on success: already in "Checked, not bugs" (`0x4702e9`..`0x4702f1`).
- Tint from the sum of R, G and B: already in "Checked, not bugs" (`0x48f99e`..`0x48f9ee`; the Glide list and character meshes use the same model).
- Character name overflow in `character_create_base`: already C14.

- The LENT "inactive list" rule on the savegame page was reversed in the documentation, not in the game: entities of `living_list_inactive` (the dead list, `character_on_death` `0x45401b`..`0x454054`) are written unless they are summons (`0x100`) without `0x40000` (`0x4d2274`..`0x4d2286`). Dead summons are not saved on purpose.
- The script-table checksum is written (`0x4cf227`..`0x4cf244`) and skipped on load (`add edx, 4` at `0x4cf4e3`; the notes give `0x4d24e3`, which is in `savegame_write_item_objects`). Not a bug, but nothing protects a save from a different `script_filenames` table.
- The `CONT` lock byte is `lock x 100` truncated (`_ftol` at `0x4d243a`) and read back x 0.01 (`0x5818f0`). It loses at most 0.01 of the lock value. Not counted.
- `savegame_find_flag_level` (`0x4d0240`) returns a flag value, not a level; its only caller `quest_get_current_step` (`0x4d60dd`) uses it as a bool. The name is ours. Not a bug.
- The thumbnail preview misses its last 8 bytes (`0x4d2977`..`0x4d298b`); already on the savegame page as cosmetic. Not counted.
- `level_time` swap: already listed under "Claims that did not hold".

- `bm_is_user_bitmap` "inverted": it returns true for type-3 entries **not** named "USERBMAP" (`0x5026c0`..`0x5026d0`). `bm_create` names every entry "USERBMAP" (`0x503d15`) and `bm_create_missing` renames its own to the missing file (`0x5030e6`..`0x5030ec`), so the function consistently means "is a missing-file placeholder" (and for handle -1 "was any placeholder made", `0x502699`). Only the name and the sum.symbols comment are wrong. It has no caller and no pointer in the image.
- `bm_create_missing` never reusing its placeholder: each missing name needs its own entry, because later loads find the bitmap by that name (`bm_find` at `0x50273d`); the entries share the static pixels `bm_missing_bitmap_pixels` (`push 0x25c5c8c` at `0x5030b7`), so each costs one table slot. `bm_missing_created` is only a flag for `bm_is_user_bitmap(-1)`. The sum.symbols "once" is wrong.
- `ai_path_node_create` never writing `pos.y` (`0x4a03d6`..`0x4a03f7` store x, z, layer and next only): the height of a path node is never used. `ai_path_find` sets each step's `dir.y` to 0 (`0x4a027c`), `ai_follow_path` overwrites it with 0 after computing it (`0x4b3181`, `0x4b33ca`, `0x4b3407`), `ai_path_begin` compares x and z only (`0x473ad7`..`0x473ae5`), and the copy to `ai+0x218` (`0x4b3127`) is never read (only writes at `0x473beb`, `0x473cdc`, `0x473dd9`, `0x4b3127`, `0x4b3882`).
- Red and blue in OpenGL screenshots and thumbnails: no swap. `gr_opengl_read_backbuffer` reads `GL_RGBA` / `GL_UNSIGNED_BYTE` (`push 0x1908` at `0x5502f0`), keeps the byte order in its row flip (`0x550340`..`0x550363`) and returns 7 (`0x55039a`). Only OpenGL returns 7 (`gr_d3d_read_backbuffer` 3 at `0x53a8b3`, Glide 3). The JPEG path copies bytes 0, 1, 2 as R, G, B (`0x506492`..`0x5064a5`; the 24-bit case swaps bytes 0 and 2 at `0x5064ec`..`0x5064f3`, so `jpeg_write_file` wants RGB), and the thumbnail writer puts byte 0 into the 565 red bits (`0x4271c6`..`0x4271eb`). SummonerFix's `glReadPixels` hook keeps RGBA order (`display.c`, `Hook_glReadPixels`). Format 7 means R, G, B, A for a read-back buffer, while 8888 *bitmaps* are B, G, R, A in the texture converters; the two never meet. The exception is the console `targa` path (K15): TGA wants B, G, R, so under OpenGL its colours are also red/blue swapped on top of K15's channel shift, and OpenGL is the renderer that "reports 32 bits" there (SummonerFix makes that the normal renderer).
- `gr_opengl_init`'s error text naming `gr_direct3d_init` (`0x54f702`): a copied message; part of C50.

- The "At least 8 MB of available video memory required." check tests 5 MB (`cmp eax, 0x500000` at `0x53a01f`): `GetAvailableVidMem` runs after the primary, back and z-buffer surfaces are created (`CreateSurface` `0x539771`, `0x5397c5`, `0x539898`, `0x539998`), so it measures free texture memory. An 8 MB card at 800x600 in 16 bits keeps about 5.1 MB after three 0.96 MB buffers. Glide shows the same message when its memory value at `0x2f0f218` is below 4 (`0x549e58`..`0x549e76`, unit not traced). The threshold matches an 8 MB card; only the wording is loose. Not counted.
- Direct3D device choice: skipping hardware T&L devices and taking the last remaining one (the HAL when there is one, else the software device) is deliberate (see V61). Not counted.
- `gr_d3d_read_backbuffer` returning format 3 and the reversed depth test on non-S3 cards: already noted on the page or design choices (the depth test is set on purpose per vendor, `0x539ed6`..`0x53a016`).

- B7 `species_is_hero` has no lower bound (`cmp ecx, 0x3; setle al` at `0x449a26`): every species id in `species_table` (`0x580e60`) is 0 to 225 and `characterinfo_parse_name` only takes ids from it, so no negative species reaches it.
- B8 console command `charinfo` does nothing: `init_ccmd_charinfo` registers `empty_function` (`0x4f13e0`, `0x449bb0`). Its help text promises a dump; like the other empty commands it looks like a debug feature compiled out. Not counted.
- B10 the hurt-drop loop reads `m->id` after `move_info_free`: the free only writes `+0x40` (`0x446770`), and the ten names are distinct, so no second match and no second unlink.
- B6 first part, `+Drop:` keeps 5 of 17: a compare against 5 (`0x446420`), no overrun. No retail record has more than five `+Drop:` lines. `+Multiplayer Drop:` stops at 17, the array size (`0x4464c4`).
- B3 second part, `statscreen_unload_anim` without a NULL check (`0x44917d`): it runs only when `Character +0x5f0` is not -1 (`0x412e76`). The stat-screen moves have no class line (class 1), and every hero mask includes bit 0 (`mask |= 1` in `characterinfo_load_party_anims`), so the loaded light or heavy move is always found. A weapon-class reload resets `+0x5f0` through `character_reinit_anims` and `character_fill_locomotion_anims`.
- The hurt-move drop itself (`0x44631a`..`0x446372`): an explicit list of ten names, applied to every non-boss record. It drops 354 of 3302 retail moves, but it reads as a design choice, not a slip. Not counted; documented in game-tables.md.
- B2 for heroes in multiplayer (move_used_on_level skips their "stand" move there, so the loaded-only lookup would fail): *inferred* unreachable, since `level_is_town` is forced to 0 with flag 2 and multiplayer loads the full set, so the templeint branch does not run either.

- `ai_pick_attack_slot` (`0x472780`) returns the best direction whether or not the slot is taken. Its only caller,
  `ai_approach_attack_slot`, first lists the slots that are free or its own (`obj_from_handle` and the handle
  compare at `0x4bd44a`..`0x4bd4f7`), takes the nearest of those (`0x4bd51b`..`0x4bd575`), and uses the picked slot
  only to route around the target when the two are two slots apart (`0x4bd5a3`..`0x4bd5d3`). `ai_claim_attack_slot`
  does take a slot over only when the occupant has more time left (`0x472966`..`0x47297a`). By design.
- Goal targets after a load: `ai_state_load` gives a goal the handle of the first living entity whose name hash
  matches (`0x47486d`..`0x4748e5`). `savegame_read_character` matches the whole character record the same way
  (`0x4cf7ac`..`0x4cf7e2`), and the script editor flags duplicate names ("*** DUPLICATE NAME ***",
  `script_write_level_script`). With duplicate names the record load would already go wrong; the goal target adds
  nothing. Not counted.
- `ai_set_mode` not freeing the path: no leak. Every new path frees the old one first (`ai_set_path` `0x473bca`,
  `ai_path_find_new`, `ai_path_to` with a bad layer), and `ai_state_shutdown`, `ai_reset` and
  `ai_reset_party_member` free it.
- `ai_do_frame` has no time budget: a documentation error, not a game bug. Off-screen characters are throttled in
  `ai_update_perception`.
- `slowdown_scaler` (`0x59af54`) is read by `ai_frame` (`0x472ce9`), `Ai_path_wandering_enabled` (`0x59af58`) by
  `ai_follow_path` (`0x4b355b`) and `Ai_paused` (`0x22e499c`) by `character_ai_suspended` (`0x462de0`) and two
  others. They are not dead.
- `ai_goal_next_id` (`0x59b7c8`) is saved and loaded (`savegame_write_living` `0x4d218d`,
  `savegame_read_living` `0x4d2e16`), so loaded goal ids do not collide with new ones.

- NOTES item 7: `gr_draw_fade` calls `gr_set_color` twice with the same value (`0x5066b0`); the second call returns early because the colour did not change (`0x506b90`). Harmless.
- NOTES item 8, first part: `gr::set_clip` tests the right edge with `>=` and the bottom edge with `>` (`0x506840`); when the values are equal the assignment gives the same number, so the result is identical.
- NOTES item 1, second part: the windowed `gr_init` window is centred horizontally but placed at the top of the work area (`0x505f9f` branch). A layout choice, retail runs fullscreen, and SummonerFix replaces the window position (`display.c` `Hook_SetWindowPos`).
- `gr_is_mode_supported` under Glide: `glide_multitexture_supported` (`0x2f132cd`) looks inverted, but the flag is set to 1 when there are fewer than 2 TMUs (`0x549e8d`, `ebx = 1`), so the test is right. Only the name is wrong (symfix.txt).
- Gamma applied to the alpha of 8-bit-alpha textures on all renderers (`0x54c233`, `0x5486bd`, `0x54d5f3`): for the white additive textures this changes their brightness, which is probably the intent.
- `cmd_gamma` (`0x43d520`) only stores `config_gamma`, so `gamma <x>` in the console shows no change until the options are accepted or a level loads. The help text says only that it "Sets" the value; working as written.

**Still unclear (left out):**

- `objects_update_frame` animates objects of type 1 as characters (`0x46b3d0`). We found no `obj_link_new` caller
  with type 1, but `living_entity_init` passes its type from a register (`0x461c05`, not traced).
- `automap_label_margin` read at +0xc instead of +8 for the cursor y, `pickup_list_origin[2] = {210, 293}`,
  `itemtrade_gold_input_size` 303 at 800x600 and the larger 640 values in pxochat: possible data slips. They need
  screenshots to judge.
- `build_bone_matrices` keeping the fade factor only for state animations: intent unknown.
- The other parse.cpp remarks in `NOTES-memory-parse.md` (Error's 40-byte copy, ParseMultiline's trim,
  ParseFlags' NULL entry, AcceptQuoted's advance, `wide_charmap_to_string`, `stristr`,
  `allocate_in_commonblock`'s 2 slots) were not checked again.
- Medevan technique (`0x40`) plays an attack move (`living_combat::play_skill_attack`) but deals 0 at the first
  attack-type pass (byte 6 at `0x465d40 + 0x36`) and has no case in either side-effect table (`0x465d8c`,
  `0x46659c + 0x36` = default). Whether the skill does its work elsewhere was not traced.
- `s3d_read_meshes` compares "s1-sewercanal" with length 7 (`push 7` at `0x46efc0`), so mesh flag `0x100000` (`0x46efea`) also goes to the `S1-SewerCol-A*/B*` meshes in `levelm.vpp`. What that flag does on a level mesh was not traced.
- `character_create_base` does not test `ca_preload_alloc_slot`'s NULL (`mov [eax], ebx` at `0x5415da`; 205 slots). The peak number of loaded meshes in retail play was not measured.
- `character_unload` frees slot `+0x28` but never the shadow mesh at `+0x2c` (`0x5413b0`..`0x5413c0`). This matters only outside a mempool (flag `0x8`, `0x541372`). `charinfo_create_character_instance` sets a mempool first (`0x448e0e`), but whether `charinfo_select_mempool` can return none was not traced.
- `skeleton::read` reads each trigger name as 255 raw bytes and runs `strlen` on them. The retail `.mvf` files were not checked for a name with no NUL.
- `cmd_summon` has no `calling_a_command_function` test (`0x47f730`). What it does during the console's help pass depends on `next_arg` at that time, which was not traced.
- From the notes, not checked again: `container_spawn_loot` reading `+0xe0` of a gold item_obj (`0x4890c3`), `mp_handle_it_addi` / `mp_handle_item_drop` trusting the peer's counts (needs a modified peer), `level_free_s3d` keeping the navpoint and colour counts, `levelrender_wave_lobj`'s arrays that are never cleared or read, `tga_read_bitmap` not decoding grey images (types 3, 11), `tga_rle_encode_row` comparing `dst_bpp` bytes of pixels `src_bpp` apart, `ca_load_preload_index` without bounds, `skills_mouse_mode` staying 2 after identify / pick lock / banish, and the "mach_killed" summon gate.
- `character_create_base` not testing `ca_preload_alloc_slot`'s NULL (`0x5415da`): already listed. The slots are freed by `character_unload` (`mov [eax], 0` at `0x5413e1`), so the 205 slots must be held at once; the peak was not measured.
- The Ghost's class-4 moves: `boss_attack_resolve_dispatch` runs `ghost_resolve_hit` on the cone victims and then returns false (`0x4a2bd9`..`0x4a2be1`), so `living_combat_resolve_hit` also hits the main target. Whether that is a second hit on a target already in the cone list (`living_combat_resolve_hit_vs` and `living_combat_resolve_hit` both dealing damage) was not traced. With G30 the cone list is usually empty anyway.
- Evil Urath's fallback `rand() % 6` when `rand_weighted_index` fails (`0x4c5064`..`0x4c5071`) never picks the seventh spell (`0x5b`). When the weighted choice can fail was not traced (all weights are positive, so probably never).
- Dead party members: `character_save_state` and `character_load_state` have no case for type 8 (tables `0x4587d7`, `0x45a103`), so a party member who is dead at save time keeps no level, XP, HP, equipment, skills or AI state in the save. What the member has after the load and a resurrection was not traced (the party bits create a fresh type-7 entity, `0x4d2e48`..`0x4d2eb5`).
- `savegame_read_character` matches records only against `living_list` (`0x4cf7ac`), but the writer also saves dead characters from `living_list_inactive`. Whether the level always recreates those by name in the active list (else the load fails with `"7a"`) was not checked.
- `savegame_two_stage_get` / `_set` (`0x4d1110`, `0x4d0e20`) `strncat` a door or object name into a 64-byte stack buffer after `level_script_name` with count 0x40; a combined length of 64 or more would overflow it. Retail names were not checked (see G42).
- `two_stage_set` with an existing container entry stores both bits as 0 (`0x4d0fdf`..`0x4d1070`, notes open question 1); the path looks unreachable.
- `INVT` entry count is not clamped on load (`0x4d2cd5`); only a hand-edited save could exceed 0x209.
- G52: which retail callers pass `ai_path_find` a goal outside every cell. `world_click_move`, `ai_follow_character`, `conversation_frame` and `ai_approach_attack_slot` clamp with `pf_find_nearest_navigable_point` first; `script_action_walk_to` (`0x4dc760`, `0x4dc7b3`), `ai_path_find_new` / `_append` (`0x473cc6`, `0x473dbe`, `0x473e40`) and `mp_handle_swi_ndir` (`0x48885a`) were not traced.
- K51: whether any 2D bitmap, line or `gr_tmapper` draw runs while `gr_set_tint(1)` is in effect under OpenGL (none found; the tints we traced go through clamped mesh and level paths).
- G50: the in-game effect (whether the hidden platforms or the out-of-step board block the crossing) needs agent mode.
- V60: `cmp eax, 8; jg` at `0x43e374` is a signed compare. With Direct3D, a driver that reports 2 GiB or more of free texture memory (`0x80000000` or above) would read as negative, and a first start would switch on the low-resolution textures as in K64. What modern Windows reports through DirectDraw's `GetAvailableVidMem` for large cards was not measured.
- K60: whether the one-pixel edge is visible depends on what the colour buffer holds there (the frame's `gr_clear`, `state_game_frame` `0x43905a`) and on the clip area that `gr_setup_3d` sets; needs a Direct3D run.
- K61: that dropped items reach `gr_d3d_render_character_mesh` / `gr_opengl_render_character_mesh` through `ca_render_set_mesh` (`0x4862b7`) was read from `levelrender_draw_list`, not traced per object type. The Glide mesh path (`0x50bba3`) and level geometry (`levelrender_face_vertex_colors` `0x48fa11`) were not checked for the same conversion.
- K63: the intended shape (a cylinder of radius R inside a sphere of radius 2R) is a guess from the two tests.
- `mp_write_spell_targets` sending `target[0..1]` of modes other than 4 and 5 as local handles (notes, last bullet): not checked in this batch.
- `statscreen_unload_anim` leaves the freed light move's `anim` set (no store to `+0x8` after `character_free_skeleton`, `0x449194`). After light, then heavy, the next unload finds the stale light move first and frees its index. `character_free_skeleton` lowers the skeleton count when it frees the last slot (`0x540bf5`..`0x540bfd`), so the heavy anim normally gets the same index and the right skeleton is freed. Whether anything adds a skeleton to a hero between two Character screen visits without a full anim reload was not traced.
- `character_fill_locomotion_anims` fills walk_hurt from norm_walk and norm_walk_hurt from walk (also run_hurt from norm_run, norm_run_hurt from run; `0x46b0b6`..`0x46b125`), while stand_hurt takes stand and ready_hurt takes ready. Non-boss characters always use these fallbacks. Whether the crossing is intended depends on which slot a hurt character uses; not traced.
- V71: whether any script action or boss stage ever plays skeleton 1 through a skipped rider move.
- A goal without a target is saved with `name_hash_nocase("")` (`ai_state_save`). On load the search takes the
  first living entity whose name has the same hash. `living_entity_init` clears the name (`mov byte [eax+0x2f4], 0`
  at `0x44e6dd`), so an entity left without a name would become the target of every such goal, instead of -1.
  Whether any living entity in retail play has an empty name was not checked.
- `ai_state_load` neither saves nor clears a goal's path pointer (goal `+0x30`; the loop writes `+0x00`..`+0x2c`).
  If a level gives a monster goals with paths before the save is applied, a loaded goal keeps the old goal's path.
  Not traced.
- A knock-down during the fall (sub `0x2c`) passes `ai_can_be_knocked_down`, overwrites `disable_ms` with the new
  duration (which can be shorter), shows "Knocked Down!" again and is sent again in multiplayer. Whether that is
  intended was not decided.
- `ai_do_frame` runs `ai_frame` for type 4 (NPCs) even when their dead flag is set (`cmp ecx, 4; je` at
  `0x474bdd`). Whether NPCs can be dead there was not traced.
- `ai_state_init` calls `ai_set_mode(-1, -1)` (`0x47238f`) before it sets the handle slot `+0x80` to -1
  (`0x4723e5`), so `handle_slot_release` gets whatever `+0x80` held. Whether `living_entity_init` (its only
  caller, `0x44e22f`) clears that memory first was not checked.
- The `0x30` sub-mode (`AIS_PUSH_STUNNED`, `ai_mode_disabled_frame` `0x4b4ca6`) has no setter in `ai.cpp`. Who sets
  it was not traced.
- `gr_set_fog` with `far = 0` and `near > 0` (possible through `level_set_current_layer`, `0x4821b6`): `near` is clamped to 0, both checks pass, and the ranges become `1 / 0` (`gr_fog_alpha_inv_range` `0x507ead`, `gr_fog_z_inv_range` `0x507ee6`). With x87 exceptions masked that is +infinity; what the back ends draw then was not traced.
- `gr_d3d_read_backbuffer` calls `gr_d3d_end_scene` (`0x53a7f7`) and on a failed lock returns without `gr_d3d_begin_scene` (`0x53a82b`). The thumbnail caller `saveload_capture_thumbnail` (`0x42718a`) would then draw the rest of the frame outside a scene. Whether it checks the returned 0 was not traced.
- `gr_d3d_read_backbuffer` always reports 565 and copies `2 * w` bytes per row (`0x53a850`, `0x53a8b3`). If the Direct3D renderer can run with a 32-bit back buffer, screenshots and thumbnails would be garbled. Whether it ever picks one was not checked.
- `gr::set_clip` with an origin right of or below the screen sets `clip_left` / `clip_top` to `max - origin - 1`, a negative value, and the clip becomes one pixel wide at the screen edge instead of empty (`0x506840`-`0x50689f`). No caller with such an origin was found.
- The per-layer fog arrays (`0x22ea4f4`, `0x22fae78`) may be named the wrong way round, like `0x59bd7c` / `0x59bd80`; depends on the "+Distance:" order in `level_parse_table` (`0x4cbddc`).

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
| C11 item pool | `item_obj_spawn` `0x46a1db`..`0x46a20c`; pool `0x946790` + 1000 x 0xac = `obj_active_list` `0x970770` |
| C12 item records | `item_alloc` `0x44b380`..`0x44b387`; `items_init_pool` `0x44b282`..`0x44b295` |
| C13 delete loop | `objects_free_all` `0x469fc6`..`0x46a01b`; `obj__dtor` `0x469b40`; callers `0x46b203`, `0x43e228` |
| G10 gold merge | `container_add_item` `0x46aa79`..`0x46aaaa`, `0x46aae5`..`0x46ab0c` |
| G11 skill AP | `character_use_skill` `0x42948f`..`0x4294b6`; `skills_handle_mouse` `0x4288aa`..`0x4288b2` |
| K12 pickup thumb | `pickup_update_scrollbar` `0x4208f8`, `0x420901`..`0x420943`; `assess_update_scrollbar` `0x4017a6`..`0x4017d7` |
| K13 player rows | `mp_levelselect_handle_click` `0x41c79d`..`0x41c7e8`; `mp_levelselect_draw_players` `0x41d522` |
| K14 banner link | `pxo_parse_chat_banners_cfg` `0x4225cd`..`0x4225f4` |
| K15 32-bit TGA | `tga_convert_pixels` `0x5550a0`..`0x5550c6`; `gr_screenshot` `0x50625d`..`0x50629d`, `0x5063fa` |
| K16 weights | `character_instance__reset_anims` `0x51986b`..`0x519872`; `level_load_preload` `0x447cd9`..`0x447ce9` |
| K17 slot 9 | `items_level_load` `0x44d72d`..`0x44d74e` |
| K18 slot 18 | `item_page_out_all` `0x46caa2`..`0x46cad2` |
| V7 registry | `os_registry_resolve_key` `0x511e8d`..`0x511ecf`; `os_registry_delete` `0x5123e0` |
| V8 fonts | `gr_font_get_char` `0x5084b6`..`0x5084e7`; `GameInit` `0x43df76`..`0x43df9e` |
| V9 UI tables | `ui_load_table` `0x52b25e`, `0x52b3a3`..`0x52b3ca`; `ui_window__init` `0x52b18f`..`0x52b1d5` |
| V10 line count | `TextFile__SkipToNextLine` `0x515af4`..`0x515b1f` |
| V11 commas | `int_to_string_with_commas` `0x5152e3`..`0x5152f4`, `0x515331`..`0x515344` |
| V12 hex | `TextFile__ParseInt` `0x5162d0`..`0x5162e3`, `0x516314` |
| V13 lists | `TextFile__ParseIntList` `0x5169e7`..`0x516a02`; `TextFile__ParseFloatList` `0x516a6f`..`0x516a8a` |
| V14 16-bit TGA | `tga_convert_pixels` `0x55502f`..`0x555052` |
| V15 floor probe | `item_obj_spawn` `0x46a52d`..`0x46a535`, `0x46a5d8` |
| V16 helpers | `character_instance__get_anim_root_velocity` `0x51b1f7`..`0x51b226` |
| G12 recovery times | `spells_parse_table` `0x47a49c`, `0x47a4c1`, `0x47a50b`, `0x47a648`; `character_start_spell_recovery` `0x44dce6`..`0x44dd1f`; `character_perform_spell_cast` `0x45dc7a`; `status_effect_duration` `0x47ab90` |
| G13 aimed attack skill | `combat_apply_melee_hit` `0x4663c0`..`0x4663df`, `0x466468`..`0x466487`, `0x466510`..`0x46652f` |
| G14 container close | `mp_handle_container_close` `0x488f00`..`0x488f15`, `0x488f4f`..`0x488f77`; `container_remove_item` `0x46abf5`..`0x46ac04`; `mp_send_container_close` `0x488e08`..`0x488e2a` |
| G15 damage dice | `living_entity_get_hit_by_spell` `0x45e4ca`..`0x45e4d8`; `spell_roll_amount` `0x47f513`..`0x47f551` |
| G16 missile slots | `spell_launch_missile` `0x47e700`..`0x47e765`; `spell_fire_missile` `0x47e883` |
| G17 trade slot | `mp_trade_complete` `0x471913`..`0x47193e`; `party_inventory_find_type_index` `0x46bd91`; `console_command__console_command` `0x50a1dc` |
| C14 character name | `character_create_base` `0x54150c`..`0x5415c7`, `0x54160e`..`0x541636`; `character_load_shadow` `0x540e18`, `0x540e98`; `ca_preload_alloc_slot` `0x541700`..`0x541730` |
| G18 lockpick handle | `mp_handle_container_lockpick` `0x4896a2`..`0x4896ce`; `container_lockpick` `0x4894b9`, `0x489564`; `mp_send_lockpick_result` `0x489614`; `mp_obj_from_net_handle` `0x475265`..`0x47526b`; `object_use_container` `0x43cc7c` |
| K19 ComputerSpeed | `GameInit` `0x43e06f`..`0x43e104`; `summoner_cfg_load` `0x43e295`..`0x43e327` |
| K20 hit effects | `combat_spawn_hit_vfx` `0x4669d2`..`0x466a54`, case bytes `0x466a64`, jump table `0x466a58`; `material_names` `0x598f5c`; `characterinfo_parse_table` `0x445f42`..`0x445f64` |
| V17 options | `cmdline_read_params` `0x43dc7c`..`0x43dc8b`; `init_cmdline_glide` `0x43d37c`; `LoadSettings` `0x43dade`..`0x43db03`, `0x43d95b`, `0x43d968` |
| V18 extract_angles | `matrix3__extract_angles` `0x51028f`..`0x5102c0`; `set_from_sincos` `0x510086`..`0x5100b0`; `cutscene_camera_sequence_advance` `0x4944b9`..`0x4945f2`; `camera_update_sequence` `0x4794ad`..`0x4794ce`; `cutscene_camera_sequence__ctor` `0x493f83` |
| V19 dead math helpers | `matrix43__transpose_in_place` `0x511bb1`..`0x511c02`; `skeleton__get_root_rotation` `0x544051`..`0x54405c` |
| V20 mesh leak | `vim_load_file` `0x5438e5`..`0x5438f1`, `0x543908`..`0x54390d`; `vim_alloc` `0x54307c`..`0x543086`; `vim_free` `0x5430b9`..`0x5430be` |
| V21 textures | `vim_read` `0x543835`..`0x543896` |
| V22 colour clamp | `s3d_read_color_record` `0x4703bf`..`0x4703f2`; `level_load_s3d` `0x46e6f5`..`0x46e70d`; `level_free_s3d` `0x46fed0` |
| C30 preload index | `ca_load_preload_index` `0x54180e`..`0x541894`, `0x5418b0`..`0x5418c2`; retail `chars.vpp` `evil7724.svf` (61 entries, 626,685 bytes) |
| G30 offense point | `boss_ai_parse_table` `0x4a18ad`..`0x4a18c4`; `pyrul_boss_load` `0x4b8a5a`..`0x4b8a9f`; `pyrul_point_in_attack_cone` `0x4b9c29`..`0x4b9c4c`; `ghost_point_in_attack_cone` `0x4bbb89`; `machival_offense_point_reaches` `0x4c0dd9`; `murod_oni_offense_point_reaches` `0x4c6239`; `giant_salamanka_target_in_reach` `0x4c8c4c`..`0x4c8c61`; `boss_attack_resolve_dispatch` `0x4a2c1e`..`0x4a2c58`; retail `tables.vpp` `*_boss.tbl` |
| G31 plus spell | `living_combat_resolve_hit` `0x467e8a`..`0x467eb8`; `boss_counter_hit_dispatch` `0x4a2254`..`0x4a22ea`; `living_combat_update_boss_attack` `0x4683ea`..`0x4683fc` |
| G32 mounted test | `boss_species_mounted_stage` `0x4a2484`..`0x4a249e`; `ai_get_combat_style` `0x473644`..`0x473653`; `character_target_point_count` `0x4a21e8` |
| K30 boss height | `character_size_height` `0x45430a`..`0x454323`; `character_size_radius` `0x4542ca`..`0x4542d2`; `charinfo_boss_height` `0x4a15a0`, case bytes `0x4a1620` |
| V30 counts kept | `level_free_s3d` `0x46fe97`..`0x46fee2`; `level_marker_find` `0x4774ac` |
| V31 wave arrays | `levelrender_wave_lobj` `0x48e342`..`0x48e426`; caller test `test cl, 0x20` at `0x48fe4b` |
| C40 door commands | `cmd_door_open` `0x4d5e83`..`0x4d5ea5`; `cmd_door_close` `0x4d6593`..`0x4d65ac`; `cmd_door_toggle` `0x4d6613`..`0x4d662e`; `door_find_by_name` `0x4772ad`..`0x47730b`; `door_set_open` `0x476e07` |
| G40 gold amount | `savegame_write_item_objects` `0x4d24ec`..`0x4d255f`; `savegame_read_item_objects` `0x4d337f`..`0x4d33cb`; `item_obj_spawn` `0x46a619`..`0x46a622`; `item_obj_pickup` `0x46b6f6`..`0x46b70d`; `monster_drop_loot` `0x450fcd`..`0x450ff7`, `0x452e38`..`0x452e48` |
| G41 32 piles | `savegame_write_item_objects` `0x4d24e3`..`0x4d24e6` |
| G42 name length | `savegame_read_two_stage` `0x4d3566`..`0x4d35af`, `0x4d3730`; `savegame_read` `0x4d2771`..`0x4d277f`; `state_game_enter` `0x43820d`..`0x438213` |
| K40 labels | `savegame_automap_label_remove` `0x4d13bf`..`0x4d13ca`; `savegame_automap_labels_reset_level` `0x4d151d`..`0x4d1526`; `savegame_automap_label_add` `0x4d12d4`..`0x4d12dc`; `automap_handle_click` `0x402bd2`..`0x402bff`; `automap_label_text_input` `0x40343a`..`0x403470`, `0x4034c3`..`0x4034f9`; `state_game_enter` `0x438233` |
| V40 stage 19 | `game_stage_set` `0x4d433e`..`0x4d4381`; `savegame_set_flag` `0x4cfd74`..`0x4cfd94`; `events.tbl` (`tables.vpp`) `$Level: "ionaext02"`, `$Level: "khosanilab"` |
| V41 bit bound | `savegame_get_bit` `0x4cff3c`..`0x4cff4a`; `savegame_set_bit` `0x4cffae`..`0x4cffb3`; `automap_is_cell_revealed` `0x4300ce`..`0x4300f8`; `automap_reveal_cell` `0x430129`..`0x430166` |
| C50 window mode 202 | `gr_opengl_init` `0x54f6f0`..`0x54f700`, `0x54f741`..`0x54f761`, `0x54f702`; `LoadSettings` `0x43db45` |
| C51 multitexture | `gl_init_multitexture` `0x54f382`..`0x54f3e0`, `0x54f39e`; `gl_setup_multitexture_state` `0x54ffa2`..`0x54ffb2` |
| C52 room 4 prisoners | `ls_khosanilab_room4_frame` `0x4f8e0b`..`0x4f8e9a`; `script_object_find` `0x4cde54`..`0x4cde57`; `ls_khosanilab_frame` `0x4f8b4a`..`0x4f8b64`; `events_string_pool` `0x24f6218` (buffer `0x24ede14`, 0x8400) |
| C53 hills navpoint | `ls_worldmap1_nearest_hills_nav` `0x4fc2ad`, `0x4fc327`..`0x4fc337`; `ls_worldmap1_pick_encounter_stage4` `0x4fc672`..`0x4fc6e8` |
| G50 flipper board | `ls_khosanilab_init` `0x4f895c`..`0x4f8980`; `ls_khosanilab_puzzle_use` `0x4f9b66`..`0x4f9b97`; `ls_khosanilab_set_flippers` `0x4f9cc4`..`0x4f9cf4`; `door_apply_saved_state` `0x47737d`..`0x4773ad`; `door_start_move` `0x476fd3`..`0x476fe4`; `ls_khosanilab_use` `0x4f95d4`..`0x4f95dc`; `ls_khosanilab_apply_lava_platforms` `0x4f89e4`..`0x4f89ff` |
| G51 path leak | `pf_graph_search` `0x4a0180`..`0x4a019b`, `0x4a01b5`..`0x4a01b7`, `0x49fbec`; `ai_path_free` `0x4a0484`; `ai_path_alloc_node` `0x4a02d7`..`0x4a02f2`; `pathfinding_load_graph` `0x49ce04` |
| G52 cell crossing | `pf_find_cell_crossing` `0x49ed02`..`0x49ed84`; `pf_compute_graph_bounds` `0x49cb17`..`0x49cb74`; `pf_find_nearest_navigable_point` `0x49e972`..`0x49e9ad`; `pf_graph_search` `0x49fae6`..`0x49fb3d`; `world_click_move` `0x43bf26` |
| K50 565 mipmaps | `gl_build_mipmaps` `0x54e4f9`..`0x54e5cb`, `0x54e49f`..`0x54e4af`, `0x54e348`; `gr_opengl_create_texture` `0x54e9df`..`0x54ea00`, `0x54ebed`..`0x54ec0b`; `bm_read_vbm_header` `0x501df8`..`0x501e68`; `bm_read_header` `0x502b4a`..`0x502b99`; retail VBMs in the `.vpp` files |
| K51 OpenGL tint | `gr_opengl_apply_tint` `0x54f9b6`..`0x54fa1b`; `clamp_ubyte` `0x550640`; `gr_d3d_apply_tint` `0x535491`..`0x5354f8`; `gr_glide_apply_tint` `0x54a546`..`0x54a5a1`; `gr_opengl_tmapper` `0x5501a4`..`0x5501af`; `gr_poly_buffer_flush` `0x545d9e`, `0x546028`; `gr_opengl_render_character_mesh` `0x50d3a1`..`0x50d3c3` |
| K52 change_texture_res | `cmd_change_texture_res` `0x501c90`..`0x501cd3`; `cmd_load_mipmaps` `0x501b50`..`0x501b57`; `execute_command_line` `0x50a2ed`..`0x50a389`, `0x50a4af`..`0x50a50a` |
| V50 texture count | `gr_opengl_create_texture` `0x54ece0`..`0x54ed08`, `0x54ebb4`..`0x54ebd6`, `0x54ec11`..`0x54ec2b`; imports `0x57e30c` (`glTexSubImage2D`), `0x57e31c` (`glTexImage2D`) |
| V51 setup_view | `gr_opengl_setup_view` `0x55051a`..`0x5505bc`; `gr_eye_matrix` `0x26927c0` |
| G60 connect error | `irc_connect` `0x4abdb6`..`0x4abddc`, `0x4abce5`..`0x4abcee`, `0x4abea7`..`0x4abed4`; lobby connect step `0x43f173`..`0x43f19a`; `state_mp_lobby_frame` `0x43f0b2`..`0x43f0bb` |
| K60 z clear | `gr_d3d_zbuffer_clear` `0x535600`..`0x53565a`; `gr_d3d_clear` `0x535570`..`0x5355ca`; `gr__set_clip` `0x50684f`..`0x50687b`; `gr_opengl_clear` `0x54f81d`..`0x54f84f`; `gr_opengl_zbuffer_clear` `0x54f8af`; callers `0x482545`, `0x50df38` |
| K61 gold tint | `world_tint_gold_object` `0x43a62e`..`0x43a644`; .data `0x594424`..`0x594430`; `world_hover_tint_object` `0x43a681`..`0x43a6d0`; `levelrender_draw_list` `0x48628c`..`0x4862a2`; `gr_d3d_render_character_mesh` `0x50c8f2`..`0x50c90c`; `gr_opengl_render_character_mesh` `0x50d3f3`..`0x50d40e`; `_ftol` `0x56c7fd`; `monster_drop_loot` `0x45306f` |
| K62 kick reason | `chat_process_irc_message` `0x4ace6c`..`0x4acf35`; `irc_get_word` `0x4ad940`, `0x4ad982` |
| K63 pick radius | `pick_test_character` `0x43c6e4`..`0x43c6ef`, `0x43c74d`..`0x43c7be`; `character_size_radius` `0x4542ca`, `0x4542e3` (`0x59d7c8`) |
| K64 OpenGL 8 MB | `summoner_cfg_load` `0x43e2e0`, `0x43e32e`, `0x43e366`..`0x43e376`; `gr_get_video_memory_mb` `0x507a03`; `options_apply_detail` `0x4872d0`..`0x4872dc`; `options_apply_and_save` `0x41faa3` |
| V60 bytes | `gr_d3d_init_device` `0x539f6a`..`0x539fab`, `0x53a01b`..`0x53a04d`; `gr_d3d_get_video_memory_mb` `0x53aaf0`; `gr_get_video_memory_mb` `0x507a0e`; `summoner_cfg_load` `0x43e36c`..`0x43e376`; `gr_glide_init` `0x549ea2` |
| V61 enumeration | `d3d_ddraw_enum_callback` `0x538a02`, `0x538a58`..`0x538a61`; `d3d_enumerate_ddraw_devices` `0x538867`, `0x538884`..`0x5388f6`, `0x538907`..`0x538914`; `d3d_enum_devices_callback` `0x53892c`; `gr_d3d_init_device` `0x539e73`..`0x539ec4` |
| C70 stat-screen heavy NULL | `statscreen_refresh_anim` `0x412edb`..`0x412ef9`; `statscreen_load_anim` loop `0x449044`..`0x44907a`, read `0x4490dc`; retail `Rosalind.tbl` (no "statscreen heavy"), `items.tbl` (48 heavy items, none for Rosalind) |
| C71 town remap NULL | `charinfo_remap_town_moves` `0x447ab2`..`0x447af8` |
| C72 trigger bound | `characterinfo_parse_table` `0x4461da`..`0x4461ee` (sound parsed first), `0x4462bb`..`0x446318` (triggers) |
| G72 default weapon bound | `characterinfo_parse_table` `0x4457fb`..`0x4458b3`, `0x445b07`..`0x445b26`; `living_entity_init` `0x44e8e9`..`0x44e911` |
| C73 chain message buffer | `character_learn_chain_skill` `0x449650` (`sub esp, 0xa0`), `0x4497ac`..`0x4497d8`; `strings.tbl` ids 7-32 in three languages |
| G70 emerge triggers | `summon_stash_for_cutscene` `0x448038`..`0x4480ca`; `character_move_trigger_time_left` `0x461670`; `character_play_move` `0x460ff6`, `0x461005`; `summon_emerge_update` `0x4b46df`..`0x4b4701`; `ai_frame` `0x472c1f`..`0x472c45` |
| G71 unknown skill name | `characterinfo_parse_table` `0x445ba7`..`0x445bb7`, `0x445be9`..`0x445c1b`, `0x445d71`..`0x445dad`; `skill_names` count `0x595cb4` = 0x54 |
| V70 recycled class bits | `characterinfo_parse_table` `0x4460fb`, `0x44617d`..`0x4461b2`, `0x446354`..`0x44635f`; `characterinfo_add_move` `0x446740`; `characterinfo_load_party_anims` `0x4475ec`..`0x447601`; retail `Joseph.tbl`, `Rosalind.tbl`, `Flece.tbl` replayed in `species_table` order |
| V71 anim = 1 | `move_used_on_level` `0x444db3`, `0x444fb8`; `charinfo_load_move_anim` `0x447792` |
| B7 species ids | `species_is_hero` `0x449a20`; `species_table` `0x580e60`..`0x5818d4` |
| B8 empty command | `init_ccmd_charinfo` `0x449bb0` |
| B3 unload guard | `statscreen_refresh_anim` `0x412e76`; `statscreen_unload_anim` `0x449149`..`0x44917d` |
| K80 cast knock-down | `ai_can_be_knocked_down` `0x475030`..`0x475087`; `ai_knock_down_local` `0x4742fb`..`0x47433e`; `ai_set_mode` `0x47333e`..`0x47335e`; `ai_knock_down` `0x47435d`..`0x474377`; `mp_handle_ai_disable` `0x480f8e`..`0x480f9f`; `combat_apply_melee_hit` `0x466286`..`0x4662a4` |
| V80 ai_state compare | `cmd_ai_state` `0x474d48`..`0x474d77`; .data `0x59ad60`..`0x59ad94`; `next_arg` as a buffer `0x437513`, `0x43995e` |
| V81 dead variables | `cmd_ai_frame_update_rate` `0x472102`, `0x472121`; `cmd_attack_point` `0x47329d`..`0x4732b7`; `cmd_offense_point` `0x473318`; `ai_update_perception` `0x47301d` |
| V82 unreachable branch | `ai_can_be_knocked_down` `0x47506a`..`0x475073`; `ai_knock_down_local` `0x4742b3`..`0x4742f8`; `ai_mode_disabled_frame` `0x4b4b6e`..`0x4b4b88` |
| C90 multipass bound | `gr_tmapper_multipass` `0x507a85`, `0x507aab`-`0x507aca`; `gr_poly_vertex` `0x5452f0`-`0x54530c`; `gr_poly_clip_and_draw` `0x5458a7`, `0x54593b`; callers `0x54595c`, `0x55f556` |
| C91 screenshot fatal | `gr_screenshot` `0x506255`-`0x506279`; `gr_d3d_read_backbuffer` `0x53a824`-`0x53a834`, `0x53a8b3`; `DebugError` `0x531d08`-`0x531d10`; `screenshot_save` `0x43263f` |
| K90 OpenGL gamma | `gr_set_gamma` `0x506ac1`-`0x506b7f`; `gr_opengl_tmapper` `0x550127`-`0x550182`; `gl_convert_a8_to_rgba8888` `0x54d5f3`; `options_apply_gamma` `0x41f380`-`0x41f3c1`; `GameInit` `0x43df76`, `0x43e110`; `gr_font_create` `0x5088c1` |
| K91 D3D double gamma | `gr_d3d_tmapper` `0x53622f`-`0x536265`; `gr_d3d_tmapper_multipass` `0x5366b6`-`0x5366fa`; `gr_d3d_set_gamma` `0x53ab27`-`0x53ab6d`; `gr_d3d_init_device` `0x539d8a`-`0x539dc4`; `gr_glide_tmapper` `0x54b2bf` |
| K92 stale fog | `gr_set_fog` `0x507dc9`-`0x507e2f`, `0x507f3f`-`0x507f6d`; `levelrender_fog_enable` `0x482850`-`0x4829c6`; `fog_set_distances` `0x486b10`-`0x486b7a`; `level_set_current_layer` `0x482176`-`0x4821b6` |
| K93 antialias flag | `cmd_antialiasing` `0x505db0`-`0x505dda`; `options_apply_detail` `0x487330`-`0x48733c`; `gr_set_antialias` `0x5080c0`; `gr_d3d_init_device` `0x539dd5` |
| V90 vsync argument | `LoadSettings` `0x43db08`-`0x43db4d`; `gr_init` `0x505e52`, `0x505e87`, `0x505eaa`; `options_apply_detail` `0x48734d`-`0x48735a`; `gr_d3d_flip` `0x53a718`; `gr_glide_flip` `0x54a762` |
