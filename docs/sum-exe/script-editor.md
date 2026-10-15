---
title: "Script editor"
parent: Sum.exe
nav_order: 6.35
---

# Script editor
{: .no_toc }

1. TOC
{:toc}

**Summary:** the retail `Sum.exe` v1.40 still contains Volition's in-game level script editor.
It edits navpoints, creatures, doors, ambient sounds, clickable objects, level items and the
action lists of level and cutscene scripts, and saves them back to `.tbl` files. It runs only
while bit `0x80` of `game_debug_flags` (`0x5fbfd8`) is set. **No code in the retail binary sets
that bit** (**observed**), and no console command, key or command-line switch does it.
[SummonerFix](../summonerfix/index.md) can toggle it with a key (`[Debug] ScriptEditorKey`).

{: .important }
**Tested 2026-10-14 in agent mode:** setting bit `0x80` during gameplay (first by writing memory,
then with `ScriptEditorKey=F12`) opens the editor at once. The main bar appears below the
middle of the screen with Save, Load, Pause, < Time, the action list, Change, Edit, the mode
button (Creature), < Prev, Next >, Pos, Ori, Y, Cutscene Script, New and Del. "Level time" is
shown, the HUD is hidden, and navpoint markers are drawn in the world. Clearing the bit closes the
editor and brings the HUD back. Nothing was edited or saved in the test. The rest of the page is
from the disassembly; the evidence table lists function addresses, not REA evidence IDs.

## How the editor is switched on

### The gate

`game_render_frame` (`0x438dc0`) calls `script_editor_frame` (`0x4e01f0`) every frame, with no
condition of its own. The editor function then tests one bit (**observed**):

```nasm
4e01f0: mov  al, byte ptr [0x5fbfd8]   ; game_debug_flags
4e01f8: test al, 0x80
4e01fa: jne  0x4e0205                  ; bit set: run the editor
4e01fc: call 0x4e2ad0                  ; script_editor_close (returns at once if the UI was never built)
4e0204: ret
4e0205: mov  al, byte ptr [0x256b73c]  ; script_editor_ui_created
4e020c: jne  0x4e0213
4e020e: call 0x4e0690                  ; script_editor_create_ui, first frame only
```

So the editor opens on the first rendered game frame after the bit is set. It closes, and
destroys its windows, on the first frame after the bit is cleared (**observed**).

Two other functions read the same bit (**observed**):

| Function | What bit `0x80` changes |
|---|---|
| `camera_process_input` (`0x434660`) | camera keys and mouse are ignored while `script_editor_no_window_open` (`0x4e01a0`) returns 0, that is while the action, save, load, parameter or New window is open |
| `charinfo_free_moves` (`0x4484a0`, test at `0x4484d2`) | skips freeing character move data, *inferred* so that the editor's move lists stay valid |

### Nothing in the retail game sets the bit

Every instruction that writes `game_debug_flags` was listed with
`node tools/asm.mjs grep "(mov|or|xor|and) (dword|byte) ptr \[0x5fbfd8\],"` (29 sites), and the
value each one stores was checked (**observed**):

| Writer | Effect on the low byte |
|---|---|
| `state_boot_enter` (`0x431c60`) | sets the whole value to `1` |
| `state_title_enter` (`0x431e20`) | keeps only `0x1c`, then ors in `1` or `2`: **clears `0x80`** |
| `mp_local_player_create`, `state_mp_lobby_leave` | set the whole value to `2` or `1` |
| `game_pause` / `game_unpause` | bit `0x4` |
| `cutscene_play` / `cutscene_cleanup` / `cutscene_change_level_for` / `cutscene_play_escape` | bits `0x10`, `0x20`, and `0x8000` (`or ah, 0x80`, which is the high byte) |
| `FrameEnd` (`0x4328b0`) | clears `0x40` (screenshot) and `0x8000` |
| `PollInputEvents` (`0x432710`) | sets `0x40` (PrtSc) |
| the rest | bits `0x100`, `0x200`, `0x1000`, `0x10000`, `0x20000`, `0x40000` |

None of them sets `0x80`. The `or ah, -0x80` writes in `mainmenu_confirm_new_game_cb`,
`cutscene_play` and `cutscene_cleanup` set `0x8000`, a different bit (**observed**). The four
bytes `d8 bf 5f 00` (the flag's address) occur 633 times in the file, all inside `.text`, so no
data table, option parser or console variable points at the flag (**observed**). The
[console command list](console.md) has no editor command; the editor's only console commands are
`remove_nav` and `render_s3d_navpoints` (**observed**). The command that sets the bit was most
likely removed from the shipping build or never compiled in (*inferred*).

### What a modder can do

- **SummonerFix key (tested).** Set `[Debug] ScriptEditorKey=F12` (any key name) in
  `summonerfix.ini`. During gameplay the key toggles bit `0x80` on the game thread
  (`src/summonerfix/keys.c`). **Back up the game first:** the editor's Save button writes loose
  `.tbl` files into `data\scripts` and `data\tables`.
- **Memory edit after reaching the game (tested).** Set bit `0x80` of the byte at `0x5fbfd8` with a
  debugger or a memory editor while a level is loaded. `state_game_enter` (mask `0xfffbffff`),
  `state_game_leave` (mask `0xfffbfcff`) and `state_game_frame` keep the bit, so it survives
  level changes. Going back to the title screen clears it.
- **One-byte patch (untested).** Changing `jne 0x4e0205` at `0x4e01fa` (`75 09`) to `jmp` (`eb 09`) runs the
  editor in every game frame. Camera input and `charinfo_free_moves` would then still behave as
  if the editor were off, so setting the flag itself is the cleaner route.
- The editor lists files with `file_find_files` in `data\sounds\ambient`, `data\sounds\Doors`
  and the script folders. Whether those calls see files packed in `.vpp` archives, or only loose
  files, is *unknown*. Saving writes loose files into `data\scripts` and `data\tables`.

## Screen layout and modes

`script_editor_create_ui` (`0x4e0690`) builds every gadget once. The main bar
`script_editor_window` (`0x254c4e0`) is 640 by 60 pixels at y 420 (**observed**:
`ui_window__init(0, 0x1a4, 0x280, 0x3c, ...)`). Each frame `script_editor_frame` casts a mouse ray
(`gr_screen_to_world_ray`), runs `script_editor_pick_hover`, `script_editor_update_buttons` and
`script_editor_handle_input`, draws markers and prints "Cutscene time: %.1f" or "Level time: %.1f"
(**observed**).

The mode button `script_editor_mode_button` (`0x2556260`) shows the name of the current mode
from `script_editor_mode_names` (`0x5a08a8`). Its callback `script_editor_mode_button_cb` (`0x4e68e0`) opens
`script_editor_choice_window`, and `script_editor_choice_ok_cb` (`0x4eb780`) stores the choice in
`script_editor_mode` (`0x25629c0`) (**observed**). The table has six entries (**observed**, read
from the file):

| Mode | Name | What a click does (*inferred* from `script_editor_handle_input` callees) |
|---|---|---|
| 0 | Creature | select or place a creature on a navpoint (`script_editor_place_creature`) |
| 1 | Navpoint | add `$npc%.3d` level or `$cut%.3d` cutscene navpoints (`script_editor_add_navpoint`) |
| 2 | Camera | *unknown* |
| 3 | Ambient sound | add or edit an emitter (`$ambient%.3d`, ambient window) |
| 4 | Door | open the door window for the picked door (`door_from_lobj`) |
| 5 | Click object | mark an object clickable (`lc_entry_add`); arrows drawn for every `lc_entries` record (**observed** in `script_editor_frame`) |

### Main bar buttons

Hotkeys are DirectInput scan codes passed to `ui_gadget__set_hotkey` (**observed**). Reading
`0x4000` as Ctrl and `0x1000` as Shift is *inferred*.

| Button | Callback | Hotkey | Action |
|---|---|---|---|
| mode name | `0x4e68e0` | | opens the mode list |
| Pos / Ori / Y | `script_editor_on_pos_ori` (`0x4e6930`) | Q / W / E | latches one drag mode: move, rotate, height (*inferred*) |
| New | `script_editor_on_new` (`0x4e69a0`) | Insert, C | opens `script_editor_new_window` (creature or object list) |
| Del | `script_editor_on_delete` (`0x4e6ad0`) | Delete, D | deletes the selection; refuses players and navpoints in use |
| Save | `0x4e6fa0` | Ctrl+S | pauses, `script_editor_show_script_dialog`, opens the save window |
| Load | `script_editor_on_load` (`0x4e70e0`) | Ctrl+L | pauses, lists scripts, opens the load window |
| Pause | `script_editor_on_pause` (`0x4e7340`) | Space | toggles `game_pause` |
| < Time | `script_editor_on_time_back` (`0x4e7380`) | Backspace | resurrects characters and restarts the level script |
| < Prev / Next > | `script_editor_on_prev_next` (`0x4e7410`) | `,` / `.` | steps through characters or navpoints |
| Cutscene Script | `0x4e7920` | | toggles editing the cutscene script instead of the level script; event `0xf` unloads it |
| name box | `0x4e73d0` | | renames the selected navpoint or character (`navpoint_set_name`) |

The action panel next to it holds the action list `script_editor_action_listbox` (`0x254c9d0`),
append and insert (`script_editor_add_action_cb`, `0x4e7970`, Ctrl+A), Del (`0x4e7c70`), up and
down (`script_editor_action_up_cb` `0x4e7dc0`, `script_editor_action_down_cb` `0x4e7ee0`, Shift+Up
and Shift+Down), Change (`0x4e8000`) and Edit (`script_editor_edit_button_cb`, `0x4e8020`, Ctrl+E)
(**observed**).

## Windows

`script_editor_close` (`0x4e2ad0`) destroys twelve windows (**observed**). All are static
`ui_window` objects built by start-up initialisers.

| Window | Address | Contents and OK handler |
|---|---|---|
| main bar | `0x254c4e0` | buttons above |
| New | `0x25508e0` | creature or object list; Ok `0x4e83a0` stores `script_editor_new_selection` |
| action type | `0x2554d28` | list of action types; `script_editor_action_window_cb` (`0x4e8440`) |
| parameters | `0x2553110` | "Edit Script Parameters"; Ok `script_editor_params_ok_cb` (`0x4eb5c0`) reads the boxes with `atoi` |
| choice | `0x2550dd0` | mode list, and lists filled by `script_editor_fill_action_lists` (moves, `*.mvf`, `*.vfx`) |
| save | `0x2552c20` | "Script name:", "Filename:", Level script / Cutscene script radios, Save |
| load | `0x254f208` | name box, script list, Level scripts / Cutscene scripts radios, Load |
| ambient sound | `0x254a8d8` | `.wav` list from `data\sounds\ambient`, Min range, Volume, Roll-off, Loop Delay; OK `script_editor_ambient_ok` (`0x4eb3d0`) |
| door | `0x25629c8` | Opening / Closing sound lists from `data\sounds\Doors`, ranges, "Originally locked", "Pick skill modifier:", key item; OK `script_editor_door_ok` (`0x4ec270`) |
| door lock | `0x254b1f0` | item box and list, "Give item", count, OK / Remove |
| creature | `0x256b250` | Team radios (Friendly, Neutral, Hostile, Evil), Level; OK `script_editor_creature_ok` (`0x4ec730`) |
| level item | `0x2564540` | item name, list, Count; OK `script_editor_level_item_ok` (`0x4ecbe0`) |

## Data it edits

| Data | Address | Editor functions |
|---|---|---|
| level navpoints (750 x 0x30) | `navpoints` `0x24e4950`, count `0x2539f2c` | `script_editor_add_navpoint`, `script_editor_delete_navpoint`, `script_replace_navpoint_references` |
| cutscene navpoints | `cutscene_navpoints` `0x256b750` | same, with `$cut` names |
| script objects and their actions | `script_objects` `0x24f6230` (0x550 bytes each) | `script_create` (`0x4e88a0`), `script_editor_apply_action_params` (`0x4e84b0`) |
| script strings | `level_script_stringpool` `0x2539f18`, `cutscene_script_stringpool` `0x2571750` | `script_editor_store_string` (`0x4e4890`) |
| clickable objects, door locks | `lc_entries` `0x248ce40`, count `0x2491a68` | `lc_entry_add`, `script_editor_door_lock_ok_cb` |
| level items | `level_items` `0x24e3b50` | `script_editor_level_item_ok` |
| ambient emitters | level ambient list | `level_ambient_add` / `level_ambient_remove` |

The action types and their per-type parameter labels ("Time:", "dist from nav:", "Walk?",
"Label id", "Goto id", "NPC to follow", "synch_point") come from
`script_editor_show_action_params` (`0x4e7cd0`). They match the runtime actions run by
`script_entry_check_done` (`0x4dd780`); see [Level scripts](level-scripts.md) (*inferred*).

## Saving and loading

Save Ok (`script_editor_save_ok_cb`, `0x4eaf00`) calls `script_save_files` (`0x4e8e80`) and then
`script_save` (`0x4eac70`) (**observed**). The files are:

| File | Folder | Written by | Sections |
|---|---|---|---|
| `<level>_script.tbl`, or `<name>_cscript.tbl` for a cutscene script | `data\scripts` (*inferred* from `path_scripts`) | `script_write_level_script` (`0x4e9310`) with `script_write_navpoint` and `script_write_objects` | `$Level:`, `#Town`, `#Navpoints`, `#Objects`, `#Doors`, `#Clicks`, `#End` |
| `<level>.tbl` | `data\tables\levels` | `script_write_level_table` (`0x4ea1e0`), which reads the old file and replaces two sections | `#Ambient Sounds`, `#Triggers` |
| `level_items.tbl` | `data\tables` | `level_items_write_table` (`0x4ea500`) | `#Level:`, `$Item:`, `$Navpoint:`, `+Count:`, `+Event:` |

Each writer first makes a `.bak` copy (`File__make_backup`) (**observed**). `script_save` reports
"Script \"%s\" saved." or a read-only error with `editor_dialog_show` (**observed**).
`script_write_level_script` also counts names that occur twice ("*** DUPLICATE NAME ***")
(**observed**). `script_save_files` also formats `"%d.int"` names in the levels and tables
folders; their purpose is *unknown*. Whether a cutscene script is saved in the same layout that
`cutscene_script_load` (`0x4ed0f0`) reads (`$Num navpoints:`) is *unknown*.

Load (`script_editor_load_selected_script`, `0x4eb080`) calls `level_script_reload`, or
`cutscene_set_data_path` and `cutscene_script_load` when the cutscene radio is checked
(**observed**).

## How gadget callbacks are wired

Every callback is stored straight into slot `+0x244` of a static gadget, for example
`mov dword ptr [0x25564a4], 0x4e68e0` (`0x2556260 + 0x244`) (**observed**). Input boxes that
share one handler get it through a register (`mov esi, 0x4e0160` for the decimal filter, `mov edi,
0x4eb2e0` for the field handler).

`ui_gadget__send_event` (`0x52c330`) calls `callback(event, id)`. When the caller passes
`0xfffe7961`, `id` is the sender's own id at `+0x120`. A callback returns 1 when it handled the
event. Otherwise the event goes on to the parent container (`+0x268`) or the window (`+0x25c`)
(**observed**). That is why window-level handlers such as `script_editor_params_window_cb`
(`0x4eb530`) see their children's keys. Each button callback first compares `id` with the id
of its own button, for example (**observed**):

```nasm
4eb3d0: mov eax, dword ptr [esp + 0x8]   ; id
4eb3d4: mov ecx, dword ptr [0x256a570]   ; ambient OK button (0x256a450) + 0x120
4eb3e0: cmp eax, ecx
4eb3e8: cmp dword ptr [esp + 0x110], 0xc ; event 0xc
```

Events seen in the editor callbacks (meanings *inferred* from what the handlers do):

| Event | Sender | Meaning |
|---|---|---|
| `0x4` | input box | editing ended; handlers `atoi` the text |
| `0xc` | button | pressed |
| `0xf` | Cutscene Script button | second action; unloads the cutscene script |
| `0x13` | check box, radio button | state changed |
| `0x14` | input box | key typed; handlers test `ui_key` for Esc (`1`), Enter (`0x1c`) and Tab |
| `0x15` | input box | text changed; the `__complete` case blocks autocomplete the paired list |
| `0x18` | list box | selection changed; copies the entry into the paired input box |
| `0x19` | list box | double click; acts like Ok |

## Evidence

Function addresses stand in for REA evidence IDs on this page.

| Claim | Where |
|---|---|
| bit `0x80` gates the editor | `0x4e01f0` (`test al, 0x80` at `0x4e01f8`) |
| caller of the editor frame | `0x438e2b` in `game_render_frame` |
| other readers of bit `0x80` | `0x434671` (`camera_process_input`), `0x4484d2` (`charinfo_free_moves`) |
| no writer sets bit `0x80` | 29 write sites listed by `asm.mjs grep`; `0x431e56` clears it |
| six mode names | pointer table `0x5a08a8`: Creature, Navpoint, Camera, Ambient sound, Door, Click object |
| windows and callbacks | `script_editor_create_ui` `0x4e0690`, `script_editor_close` `0x4e2ad0` |
| callback convention | `ui_gadget__send_event` `0x52c330` |
| save path | `0x4eaf00` -> `0x4e8e80` -> `0x4e9310`, `0x4ea1e0`, `0x4ea500`; `0x4eac70` |
| load path | `0x4eb080` |
