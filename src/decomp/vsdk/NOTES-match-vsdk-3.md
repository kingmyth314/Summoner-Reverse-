# VC6 match pass 8 (six vsdk units + four extra)

Base: `snapD/src/decomp` (the repo's src/decomp). Compiler: VC6 Professional cl 12.00.8168 (`vc6p`), per-unit flags
from `src/vc6-flags.txt` (unchanged), headers from `match8/src/include`. `before/<unit>.txt` and `after/<unit>.txt`
are the `vc6-compare -v` outputs; `hdr_before/` and `hdr_after/` the 21 units compared for the ui.h change.

| Unit | Before | After | Won |
|---|---|---|---|
| vsdk/character_instance.cpp | 34 / 53 | 43 / 53 | reset_anims, get_active_anims, process, get_anim_time_left, get_anim_elapsed, get_anim_length, clear_state_anims, clear_action_anims, report_memory |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 34 / 48 | 42 / 48 | find_nearest_up / down / left / right, is_selected, is_mouse_over, clear_hotkeys, ui_state_bitmap::draw |
| vsdk/window.cpp | 28 / 40 | 34 / 40 | parse_button, parse_container, parse_inputbox, parse_listbox, parse_static, process |
| vsdk/unnamed_string_0x5566e0.cpp | 57 / 69 | 63 / 69 | assign_char, size, trim_right, operator=(char), TextBuffer::TextBuffer, TextBuffer::append |
| vsdk/unnamed_sndstream_0x566560.cpp | 14 / 26 | 17 / 26 | snd_stream_get_writable, snd_stream_service, acm_init |
| vsdk/skeleton.cpp | 16 / 28 | 17 / 28 | vim_restore_bitmaps (19 / 28 with /Ox, D6602) |
| **Assigned total** | **183 / 264** | **216 / 264** | +33 |
| vsdk/console.cpp (extra) | 10 / 17 | 15 / 17 | get_next_char, get_token, read_word, read_quoted_string, cmd_script |
| vsdk/registry.cpp (extra) | 8 / 14 | 12 / 14 | os_registry_read, read_dword, read_int, os_registry_delete |
| vsdk/unnamed_checkbox_0x553ba0.cpp (extra) | 6 / 12 | 8 / 12 | ui_checkbox::process, ui_radio_button::process |
| vsdk/unnamed_ui_element_0x542cd0.cpp (extra) | 9 / 16 | 11 / 16 | is_hidden, is_disabled (header change D6601) |
| **All** | **216 / 323** | **262 / 323** | +46 |

No function lost its match (checked address by address, also in the 21 units compared for the header change).
Every matching function's comment now says `Confidence: exact` (52 raised; some had matched before with likely /
unsure). 23 functions in these files were already marked exact and still differ (all of the D6603 kinds); left as
they were, none of them marked by this pass. mingw (`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`):
0 `error:` lines in all 259 .cpp. Decisions D6600-D6603 are appended to `src/include/DECISIONS.md`.

## What worked (D6600)

- **Unfolded constant products**: the binary multiplies by 1/160 and 1/30 separately; 8168 folds `x * c1 * c2`. A
  float local per product reproduces it (four character_instance time functions, process; skeleton's velocity got
  closer). An `int` local for the tick difference gives the original register choice.
- **Locals for loop bounds and arrays** (`ca_character *ch = character; int n = ch->num_anims;`, the array pointer
  loaded after `if (n > 0)`): reset_anims, clear_state_anims, clear_action_anims, get_active_anims.
- **Declaration order = prologue store order**: five window parse functions matched by reordering the initialised
  locals (found by permuting them, `m8tools/perm.mjs`); report_memory; `int miss = 0` hoisted in find_nearest_*.
- **`while (1) { if (!Accept()) break; ... }`** plus the body's locals before its first call (parse_button).
- **Stores through a local pointer where the binary does not reload** (String, console's token writer, TextBuffer's
  minimum size, the writable-size result).
- **Load-before-store with a local** (window process's flags, the checkbox / radio button pressed flag).
- **Branch polarity / block order** (String::size, trim_right, TextBuffer::append, cmd_script, get_next_char, the
  registry readers that return the zero EAX, the state bitmap switch, get_token's case order).
- **Signed / unsigned compares**: acm_init's version test is signed in the binary; the service test is `jbe`.
- **Header**: ui_element::hidden / disabled are bool (D6601).

## What still differs and why (D6603)

The compiler build (8966 against 8168), not the source: return / copy elision (compute_bone_matrix,
get_bone_transform, get_bone_world_transform, the two root velocity functions), reused dead argument / local slots
(skeleton_get_bone_blend_weight, increment_tick, the three snd_stream fills, ui_checkbox::create, parse_slider's
frame), stores scheduled after the next call's pushes or into an inline strcpy (set_focus / clear_focus, the four
ui_element setters, the set_tooltip functions, skeleton::skeleton, wav_descend_data, wav_read), loads hoisted above
stores (start_anim, alloc_buffers, acm_stream_open, acm_convert, String's copy constructor), register choices
(set_parent, both send_event, set_tooltip, handle_arrow_key's slots, wav_open), the dead buf copy of to_lower /
to_upper, init_char_types' shrink-wrapped early ret, ui_window::init's byte `and al`, the "\\" strcat word move,
~skeleton's list_remove, ui_element::dtor's call of the scalar deleting destructor. Partial progress kept:
alloc_buffers (only the loop registers), ui_gadget::init and send_event, skeleton::skeleton (to +0x80),
acm_stream_open, handle_arrow_key (size equal, frame slots), vim_copy / vim_save_bitmaps,
skeleton::get_root_velocity.

Not worked: build_bone_matrices, increment_tick (frame slot reuse), operator+ x3 (register caching of r.buf),
parse_radio_button (frame offsets), parse_slider, wav_open, the vim / skeleton readers.

## Flags (D6602, not applied)

- skeleton.cpp: `/Ox /Ob0 /GX /MT` (no string pooling) gives 19 / 28: the binary's name buffers read separate ""
  literals in .bss. The string unit's notes already say its literals are unpooled; whether the vsdk was built
  without /GF everywhere needs a survey (sndstream loses one with /Ox).
- Units missing from vc6-flags.txt that are better with /Ob0: anim_track 1 -> 8, d3d_tcache 3 -> 7, sound_emitter
  14 -> 17, vfx_mesh 3 -> 6, poly_clip 1 -> 3, gr_line 2 -> 3.

## Logic errors and game bugs found

- No new game bug.
- Numeric fidelity: the folded time constants (`t * (1/160 * 1/30)` against the binary's two multiplies) could round
  differently in the last bit; the new source computes what the binary computes (character_instance time functions,
  process's tick count, skeleton::get_root_velocity).
- acm_init compared the ACM version unsigned; the binary compares signed (only a version >= 0x80000000 differs).
- ui_gadget::send_event returns the callback's own result byte (the binary jumps to the shared epilogue with AL),
  not a constant true; the same for a callback that returns a proper bool.
- Confirmed while reading: ui_window::send_event returns AL of the NULL callback pointer (0) when there is none; the
  registry readers return the zero EAX of a successful read; skeleton::get_root_rotation slerps from an uninitialised
  quaternion (already in its comment).

## Tools

`../m8tools/show.mjs <unit> <addr>...` (listing next to the original; now picks the k-th overload of a name),
`trys.mjs <unit> <addr[,addr]> <variants> [keep]` (snippet variants: the first is the current text; stops at the first
match unless ALL=1), `perm.mjs` (all orders of a block of lines), `rep.mjs` (block replacements), `raise.mjs`,
`run.sh` / `runp.sh` / `survey.sh` (vc6-compare runs).
