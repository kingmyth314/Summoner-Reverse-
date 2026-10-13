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
| `0x5298c0` | List box | `+Allow wrap`, `$Content size`, `+Align view with bottom`, `+Mouseover highlights`, scrollbars, `$Entry spacing` |
| `0x52a180` | Slider | `+Horizontal`, `+Dot slider`, `$Marker bitmap/size/position`, arrow buttons |
| `0x52a700` | Static | Horizontal/vertical justification, `$Text`, `$Font` |
| `0x528430` | (hotkey) | Optional `Shift`/`Ctrl`/`Alt` prefixes, then a key name |

Widget positions and sizes are absolute pixels in the table files, which is why the UI is
laid out for one resolution. Any widescreen or native-resolution UI work starts here.

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

- The widget classes and their draw/process methods (`0x52b9c0`-`0x530c00`).
- `ui_container.cpp` (`0x5536ef`).
- A `.vpp` extractor, so the UI table files themselves can be read.
