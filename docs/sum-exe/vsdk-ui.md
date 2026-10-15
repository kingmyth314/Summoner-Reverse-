---
title: "vsdk: user interface (ui)"
parent: Sum.exe
nav_order: 5.98
---

# vsdk: user interface (`ui`)

**Summary:** Phase 1, module 8 (started). `vsdk\ui\window.cpp` (`0x528430`-`0x52b9c0`)
builds menus and screens from **text tables**. Each widget type has its own parser that reads
`$Field:` entries with the [TextFile parser](vsdk-parse.md). Names are by behaviour
(**I**); there is no Summoner 2 or Dash Faction counterpart.

## Widget parsers

| Address | Widget | Fields (from the strings) |
|---|---|---|
| `0x52b200` | (window table) | "unable to open UI table file %s"; `$Position`, `$Size`, `$Name`, `+Allow arrows`, `+Always a selection` |
| `0x528ec0` | Container | `$Mask bitmap`; children `$Button`, `$Checkbox`, `$Container`, `$Inputbox`, `$Listbox`, `$Radio button`, `$Slider`, `$Static`; `$Inventory start`/`end` |
| `0x528530` | Button | `+Drag and drop`, `+Repeats`, `+Always use face color`, `+Use right mouse button`, `$Text`, `$Font`, `$Hotkey`, `$Hotspot`, `$State bitmap` (Normal/Highlighted/Pressed/Disabled), `$Tooltip` |
| `0x528a40` / `0x529d70` | Checkbox / radio button | `$Box size`, `$Label`, `$Label offset`, `+Initially checked`, states Checked/Unchecked/Pressed/Disabled |
| `0x5292f0` | Input box | `+Ignore esc`, `+No scroll`, `+Password`, `$Valid chars`, `$Invalid chars`, `$Max text length`, `$Cursor bitmap` |
| `0x5298c0` | List box | `+Allow wrap`, `$Content size`, `+Align view with bottom`, `+No background`, `+Mouseover highlights`, `+Preallocated text`, `+Add horizontal scrollbar` / `+Add vertical scrollbar` / `+Add both scrollbars` (the last sets only the vertical flag `0x2`, and a horizontal scroll bar skips the other two tests), `$Entry spacing`, `+Margin:`, `+Vertical margin:` |
| `0x52a180` | Slider | `+Horizontal`, `+Dot slider`, `$Marker bitmap/size/position`, arrow buttons |
| `0x52a700` | Static | Horizontal/vertical justification, `$Text`, `$Font` |
| `0x528430` | (hotkey) | Optional `Shift`/`Ctrl`/`Alt` prefixes, then a key name |

Widget positions and sizes are absolute pixels in the table files, which is why the UI is
laid out for one resolution. Any widescreen or native-resolution UI work starts here.

## Widget classes

The C++ widget classes were found from their vtables (2026-10-14). There is no RTTI, so the class names are
ours. Every widget constructor first calls the base constructor `ui_gadget__ui_gadget` (`0x52c3f0`).

| Class | Constructor | Vtable | Parser |
|---|---|---|---|
| `ui_gadget` (base) | `0x52c3f0` | `0x581e08` | |
| `ui_button` | `0x526f70` | `0x581d88` | `ui_parse_button` |
| `ui_static` | `0x527ea0` | `0x581dac` | `ui_parse_static` |
| `ui_slider` | `0x52bfa0` | `0x581dd0` | `ui_parse_slider`; also the list-box scrollbars |
| `ui_listbox` | `0x52d5c0` | `0x581e34` | `ui_parse_listbox` |
| `ui_inputbox` | `0x5526b0` | `0x581f1c` | `ui_parse_inputbox` |
| `ui_container` | `0x553560` | `0x581f40` | `ui_parse_container` |
| `ui_checkbox` | `0x553ce0` | `0x581f68` | `ui_parse_checkbox`, radio buttons |
| `ui_window` | `0x52b020` | `0x581df4` (5 slots) | one per screen, built by static initialisers |

Widget vtables have nine slots:

| Slot | Meaning | Evidence |
|---|---|---|
| 0 | destructor | |
| 1 | send event (event, value) | calls the callback at `+0x244`, else forwards to the parent container (`+0x268`) or the window (`+0x25c`) |
| 2, 3 | `set_disabled` / `set_enabled` | set or clear the disabled byte `+0x1d` (`0x542f10` / `0x542f70`) |
| 4 | `is_disabled` | true if the window or parent is disabled, else byte `+0x1d` |
| 5 | **process input** | overridden by every widget; calls `mouse_is_down` / `key_to_ascii` |
| 6 | forwarded to the parent | overridden by the container |
| 7 | **draw** | overridden by every widget; calls `gr_string`, `gr_bitmap` and `gr_rect` |
| 8 | `is_selectable` | false in the base class |


Base element fields (`ui_element`, vtable `0x581ea0`): kind `+0x8`, rectangle `+0xc`..`+0x18`, hidden `+0x1c`,
disabled `+0x1d`, name `+0x1e`, id `+0x120`, tooltips `+0x124`, event callback `+0x244`. The gadget type is at `+0x254`
(1 container, 2 button, 3 static, 4 checkbox, 5 radio button, 6 input box, 8 list box). A window keeps the gadget under
the mouse at `+0x4c8` and the keyboard-selected one at `+0x4cc`.

## Build stamp (press/beta watermark)

`build_stamp_show` (`0x431cb0`) runs once at start-up and calls `build_stamp_check`
(`0x52f450`). A stamped build shows "This version of Summoner has been built for:" with the
recipient's name. Past the stamped expiry date it shows "This version of Summoner has
expired." and exits.

In the retail executable both stamps are placeholders:
- the checksum is `0x01010101` and the expiry is `0x02020202`;
- the name is "Read the Foundation Novels from Asimov.  I liked them."

The check therefore returns "unstamped" and nothing is shown (**observed** in the data at
`0x5a9604`).

## Table files named in Sum.exe

Data tables referenced by name (most live in the [packfiles](vsdk-packfile.md)):

- **Localization:** `strings.tbl`, `item_names.tbl`, `quest_names.tbl`, `topic_names.tbl`,
  `script_names.tbl`
- **Characters and animation:** `charanim.tbl`
- **Items and gameplay:** `items.tbl`, `level_items.tbl`, `spells.tbl`, `experience.tbl`,
  `materials.tbl`, `_surfaces.tbl`, `alt_textures.tbl`, `shadows.tbl`, `detail_levels.tbl`
- **Scripts:** `_script.tbl`, `_cscript.tbl`, `s_script.tbl`, `s.tbl`,
  `script_filenames.tbl`, `events.tbl`, `_dlg.tbl`, `Dismount_cscript.tbl`,
  `Escape_cscript.tbl`
- **Cameras:** `bs_cameras.tbl`, `inv_cameras.tbl`, `status_cameras.tbl`
- **Cutscenes:** `cutscene.tbl`
- **Bosses:** `serp_boss.tbl`, `phoenix_boss.tbl`, `pyrul_boss.tbl`,
  `ghost_rider_boss.tbl`, `machival_boss.tbl`, `titus_boss.tbl`, `darkjo_boss.tbl`,
  `murod_oni_boss.tbl`, `gsal_boss.tbl`
- **Other:** `automap_labels.tbl`, `credits.tbl`, `feedback.tbl`, `music.tbl`, `sounds.tbl`

## Still to do

- A page section on the widget classes and their draw/process methods (`0x52b9c0`-`0x530c00`). Their code is
  written as placeholder units in `src/decomp/vsdk`: ui_button and ui_static, then the gadget base, list box,
  build stamp and UI manager (`0x52bfe0`-`0x530b86`, D4200).
- `ui_container.cpp` (`0x5536ef`).
- A `.vpp` extractor, so the UI table files themselves can be read.
