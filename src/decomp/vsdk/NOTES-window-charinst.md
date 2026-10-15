# window.cpp and character_instance.cpp (source half)

Written from Sum.exe v1.40 only (`tools/asm.mjs fn`, `build/sum.asm`, raw .data/.rdata bytes of
`Summoner/Sum.exe` for literals, vtables and float constants). No Summoner 2 or Dash Faction code was read.
Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -I . -I <repo>/src/decomp/include -I <repo>/src/decomp/vsdk -I <repo>/symbols`
(warnings only: `delete` of a class with virtual functions and a non-virtual destructor, and `memset` on
classes with constructors; both are what the original does).

Files: `window.h/.cpp`, `character_instance.h/.cpp`, `stubs_window_charinst.h` (one header for both units;
includes `stubs_cmdline_grfont.h` for DebugError / VSDK_ERROR).

## vsdk\ui\window.cpp (0x528430-0x52bf92, COMDATs to 0x52bfdb)

Unit bounds: `ui_static__draw` (0x5280e0, static.cpp) ends before 0x528430; 0x52bfe0 `ui_gadget__unlink`
is gadget.cpp (only caller `ui_gadget__set_parent`). The 14 tagged functions are interleaved with 26
untagged `ui_window__*` methods, so the unit is the whole ui_window class (docs/sum-exe/vsdk-ui.md gives
0x528430-0x52b9c0, which stops before `ui_window__draw` .. `ui_window__is_disabled`).

| Address | Function | Confidence |
|---|---|---|
| 0x528430 | ui_window::parse_hotkey (ui_parse_hotkey) | likely |
| 0x528530 | ui_window::parse_button (ui_parse_button) | likely |
| 0x528a40 | ui_window::parse_checkbox | likely |
| 0x528ec0 | ui_window::parse_container | likely |
| 0x5292f0 | ui_window::parse_inputbox | likely |
| 0x5298c0 | ui_window::parse_listbox | likely |
| 0x529d70 | ui_window::parse_radio_button | likely |
| 0x52a180 | ui_window::parse_slider | unsure |
| 0x52a700 | ui_window::parse_static | likely |
| 0x52abc0 | clear_bitmaps | exact |
| 0x52abf0 | handle_arrow_key | likely |
| 0x52aea0 / 0x52aef0 / 0x52af40 / 0x52af90 | nav_up / nav_down / nav_left / nav_right | exact x4 |
| 0x52afe0 | ui_stub_ret_52afe0 (empty, unreferenced) | unsure |
| 0x52aff0 | send_event (vtable slot 1) | exact |
| 0x52b020 | ui_window::ui_window | likely |
| 0x52b0a0 | ui_window::~ui_window | exact |
| 0x52b100 | init | unsure |
| 0x52b200 | ui_window::load_table (ui_load_table) | likely |
| 0x52b630 | dtor (vtable slot 0, ui_window__destroy) | likely |
| 0x52b700 | get_flags | exact |
| 0x52b710 | set_tooltip | likely |
| 0x52b7a0 / 0x52b7b0 | get_selected_gadget / get_mouse_over_gadget | exact x2 |
| 0x52b7c0 | contains_point | exact |
| 0x52b800 | gadget_at | exact |
| 0x52b850 / 0x52b890 | find_gadget_by_id / find_gadget_by_name | exact x2 |
| 0x52b8e0 / 0x52b930 | load_mask_bitmap / set_mask_bitmap | exact x2 |
| 0x52b970 / 0x52b9c0 | load_background_bitmap / set_background_bitmap | exact x2 |
| 0x52b9f0 | draw | likely |
| 0x52bc20 | process | likely |
| 0x52be90 | get_mask_pixel | exact |
| 0x52bee0 / 0x52bf30 | is_hidden / is_shown | exact x2 |
| 0x52bf80 | is_disabled (vtable slot 4) | exact |
| 0x52bfa0 | ui_slider::ui_slider, implicit (from `new ui_slider`) | compiler-generated |
| 0x52bfc0 | ui_gadget scalar deleting destructor (from `delete g` in dtor) | compiler-generated |

40 functions written: 22 exact, 15 likely, 3 unsure; 2 compiler-generated COMDATs. No static initialisers.
.data 0x5a8b6c-0x5a95e3 (literals only, no pooling: every "," has its own copy). .bss defined here:
`ui_nav_best_gadget` 0x2cbf554, `ui_nav_best_distance` 0x2cbf558, `ui_window_processing` 0x2cbf55c;
0x2cbf560-0x2cbf580 are nine "" literals (one per parser, the slider has two).

## vsdk\ca\character_instance.cpp (0x5194b0-0x51b6a1, COMDATs to 0x51b75f)

The tu-split2 unit (0x5194b0-0x51bb50) also holds InetGetFile (0x51b760-, a FreeSpace-style inetfile.cpp)
and two http/ftp scalar deleting destructors; those are not included. Everything from the first initialiser
to `cmd_high_lod` is one file: the 6 tagged functions are mixed with 46 untagged `character_instance__*`,
`ca_*` and helper functions, `process` reads `High_lod`, and the `high_lod` command is the last function.
docs/sum-exe/vsdk-ca.md gives 0x51a230-0x51b4a0 (first to last tagged function).

| Address | Function | Confidence |
|---|---|---|
| 0x5194b0/c0, 0x5194e0/f0 | dynamic initialisers of `ci_vertex_buf_a/b` (thunk + body) | likely (generated) |
| 0x519510 | free_buffers | exact |
| 0x519570 | alloc_buffers | likely |
| 0x5196b0 | character_instance() | exact |
| 0x519700 | character_instance(ca_character *) | likely |
| 0x519840 | ~character_instance | exact |
| 0x519850 | reset_anims | exact |
| 0x5198a0 | rewind_anim | exact |
| 0x5198d0 | blend_bone_keys | unsure |
| 0x519ac0 | build_bone_matrices | unsure |
| 0x51a130 | skeleton_get_bone_blend_weight | likely |
| 0x51a1e0 | advance_anim_time | exact |
| 0x51a230 | increment_tick (tagged) | unsure |
| 0x51a6a0 | get_active_anims (static member, cdecl) | exact |
| 0x51a6f0 | setup_matrices (tagged) | exact |
| 0x51a780 | setup_matrices_b (tagged) | likely |
| 0x51a870 / 0x51a880 | ca_noop_1arg / ca_noop_2arg | exact x2 |
| 0x51a890 | process (tagged) | likely |
| 0x51a950 | get_root_velocity | likely |
| 0x51aab0 | get_motion_delta | likely |
| 0x51abb0 | get_root_offset | exact |
| 0x51abd0 | sum_root_velocity (tagged) | likely |
| 0x51acb0 | take_turn_delta | likely |
| 0x51ad00 | get_tag_pos | likely |
| 0x51adc0 | get_bounding_sphere | exact |
| 0x51ae00 | get_bone_bind_inverse | likely |
| 0x51ae70 / 0x51ae80 | ca_system_init_stub / ca_system_close_stub | exact x2 |
| 0x51ae90 | set_state_weight | exact |
| 0x51aee0 | start_anim | likely |
| 0x51af70 | set_anim_speed | exact |
| 0x51afe0 | get_set_time | exact |
| 0x51b000 | get_anim_time_left | exact |
| 0x51b070 | get_anim_elapsed | likely |
| 0x51b0c0 | get_anim_length | exact |
| 0x51b100 | clear_state_anims | exact |
| 0x51b140 | clear_action_anims | likely |
| 0x51b1c0 | stop_anim | exact |
| 0x51b1e0 | get_anim_root_velocity | exact |
| 0x51b250 | get_anim_root_speed | exact |
| 0x51b290 | anim_trigger_hit | exact |
| 0x51b330 | page_in_textures | exact |
| 0x51b380 | ca_stub_ret30_51b380 (empty, ret 0x30) | unsure |
| 0x51b390 | set_pos (formerly get_pos; assigns pos = *v) | exact |
| 0x51b3b0 | compute_bone_matrix | exact |
| 0x51b3f0 | get_bone_world_transform | exact |
| 0x51b460 | get_bone_transform | exact |
| 0x51b4a0 | report_memory (tagged) | likely |
| 0x51b560 / 0x51b580 | ca_system_init / ca_system_close | exact x2 |
| 0x51b590 | set_tint | exact |
| 0x51b5c0/d0 | initialiser of `ccmd_high_lod` | likely (generated) |
| 0x51b5f0 | cmd_high_lod | exact |
| 0x51b6b0-0x51b75f | profile_label ctor, matrix43 operator=, matrix43::get_position, vector3 unary -, quaternion copy ctor, imax | header inlines, not written |

52 functions written: 33 exact, 15 likely, 4 unsure; plus 3 initialisers from the definitions.
.data defined here: `ci_root_to_position` 0x5a8484 (= 1, cleared by GameInit; label ours). .bss:
`ccmd_high_lod` 0x2710ec0, `ci_vertex_buf_a/b` 0x2710ed0 / 0x2719b70 (3000 vector3 each, unreferenced;
labels ours), `High_lod` 0x2722810, and the "" literal 0x2722818.

## Facts used

- **The table parsers are ui_window members.** `ui_parse_*` and `ui_load_table` are thiscall: ECX is the
  window (children are created in `this`, its x/y are the offset for a window parent, nested parsers are
  called with the same ECX), callee pops the arguments (`ret 4` / `ret 8`). Written as
  `ui_window::parse_*` / `load_table`; sum.symbols could rename them `ui_window__parse_*`.
- **Vtables.** ui_window 0x581df4 = {0x52b630 destroy, 0x52aff0 send_event, 0x542f10, 0x542f70,
  0x52bf80 is_disabled}: slot 0 is the "dtor" slot of stubs_container_targa.h (not a C++ destructor). The
  scroll-bar slider vtable 0x581dd0 has the same nine entries as the slider's 0x582308; the derived class
  adds 4 bytes and has no user constructor (0x52bfa0 only calls the base constructor and stores the
  vtable). Radio button 0x581c7c differs from the checkbox 0x581f68 only in slot 5.
- **"" literals in .bss.** Nine "" at 0x2cbf560..0x2cbf580 (one per use, 4-byte aligned, in function order)
  and 0x2722818. `char marker_bitmap[64] = ""` in parse_slider copies byte 0x2cbf578 and zero-fills 63
  bytes, the VC6 pattern for an array initialised from a literal, so these are literals, not variables;
  this also answers open question 1 of NOTES-cmdline-grfont.md (gr_font's two "" are literals).
- **Header inlines are called in character_instance.cpp**: profile_label's constructor, matrix43 copy,
  get_position, vector3 negation, quaternion copy, imax are COMDATs emitted after the unit and called.
  Either the unit was built without inline expansion (/Ob0) or VC6 declined them. window.cpp shows no such
  COMDATs. The stub header declares them out of line so the calls stay.
- **Folded empty constructors.** vector3() 0x505530 is `mov eax, ecx; ret`; quaternion() and matrix43()
  are the same body (vector constructor iterator calls with 0x10 and 0x30 element sizes). The 0x34-byte
  bone cache element's implicit constructor (call matrix43(); return this) is byte-identical to
  gr::Vertex's, hence the shared 0x5081c0 and sum.symbols' "vertex buffer" label for +0x48.
- **Return by value.** vector3::operator= (0x413190) returns a copy through a hidden pointer; matrix43's
  copy (0x51b6c0) returns nothing; `matrix43 m = cache.m` and `return cache.m` are inline rep movsd (the
  implicit copy constructor).
- **Constants.** 0.1f twice (0x580214 in build_bone_matrices, 0x5819a8 in get_root_velocity: VC6 merges
  identical float constants, so one is probably a named const from a header); 1440.0f, 4800 (0x12c0),
  30.0f, 160.0f, 1/160 (0x581d64), 1/30 (0x581d60), 1e-6f, -999.9f (0xc479f99a), 2.0f, 0.8f.
- **-999.9 is compared as an int** (`cmp dword ptr [edi+0xa0], 0xc479f99a`) and stored with integer moves;
  written with `*(unsigned int *)&` casts (CI_ANGLE_UNSET).
- Error lines: window.cpp 2532 (mask bitmap) and 2599 (background), each with its own literals;
  character_instance.cpp 1435 ("Character has NULL anim").
- KEY_SHIFTED 0x1000 / KEY_CTRLED 0x4000 are the FreeSpace modifier bits; there is no 0x2000 (alt).

## Behaviour worth documenting (observed)

1. `ui_parse_hotkey`: "Alt+" sets the shift bit, so Alt+X hotkeys are Shift+X.
2. `ui_window__process`: with "+Always a selection" and no selection, the loop that looks for the first
   selectable gadget never advances: it hangs if the first gadget in the list is not selectable.
3. `ui_window__init`: both list walks call get_flags and ignore it, so every non-modal window is appended
   last, like the modal ones (the intended "insert before the first modal window" is lost).
4. `ui_load_table`: a missing table only formats "unable to open UI table file" into string_buffer and
   parses on; without "+Name:" the window name is copied from an uninitialised buffer.
5. `ui_parse_button`: a state name that matches nothing reuses the previous target (NULL on the first);
   "+Add both scrollbars" (list box) sets only the vertical flag; the slider's "+Marker bitmap:" and
   "+Background bitmap:" are parsed and dropped; container tooltips are read only after "+Mask bitmap:".
6. `ui_window__set_mask_bitmap` never fills mask_data (+0x4d8), so `ui_window__get_mask_pixel` always
   returns -1 unless something else sets it; clear_bitmaps and destroy forget bitmaps without releasing.
7. `character_instance__reset_anims(true)` clears 100 bytes, i.e. only the first 25 weights.
8. `character_instance__start_anim` with a speed != 1 writes the speed into every free slot (no break).
9. `character_instance__get_anim_root_velocity` zeroes the output for action anims and then overwrites it.
10. `character_instance__get_bone_bind_inverse` computes the inverse into a temporary and drops it.
11. `character_instance__set_pos` (formerly `character_instance__get_pos`) (sum.symbols name) is a setter: pos = *v.
12. In build_bone_matrices, while an action anim fades, the computed fade factor is kept only for state
    anims; for other action anims it is replaced by their own blend weight (or 10 for the held anim).

## Existing declarations that are wrong or incomplete (not changed)

1. `stubs_container_targa.h` ui_element / ui_gadget lack ui_element::init/get_kind/get_size/is_hidden/
   is_shown and ui_gadget::get_type/set_parent/set_hotkey/set_hotspot/is_owned/hit_test/clear_focus/
   find_nearest_*; ui_element::callback is `void *` (it is called as cdecl (event, value) returning the
   result). Its File clashes with stubs_cmdline_grfont.h's. Redeclared here with the same names; a merge
   should take the superset.
2. `ui_container.h` set_tooltip(text, font, x, y, delay) agrees with ui_window::set_tooltip's node stores
   (+0x11c font, +0x114 x, +0x118 y, +0x10c delay); the gadget set_tooltip functions take (text, x, y,
   delay) with three values on the table line.
3. The TextFile classes in stubs_assess_pickup.h / stubs_credits_help.h / stubs_sound_items_registry.h lack
   SkipWhitespace, CopyUntil, ParseInt, ParseString; `SkipTo` returns a bool that parse_slider tests.
4. `stubs_player_effectmem.h` vector3 / matrix have no operators; this unit needs the full math interface.
5. sum.symbols: `ui_window__init` comment says (x, y, w, h, flags, id); the call is (x, y, w, h, id, flags).
   `ui_gadget__destroy` (formerly `ui_gadget__dtor`) (0x52c3d0) is slot 0 (no flags, never frees), not a destructor.
   `character_instance__alloc_buffers` "vertex buffers" are the bone cache (0x34) and skin matrices (0x30).
   `ui_slider__ui_slider` (0x52bfa0) is the implicit constructor of the derived scroll-bar class.
6. symbols/types.h ui_element: +0x248 is `owned` for gadgets but the window's flags; +0x24c is the window's
   group; types.h's matrix43 / vector3 C structs match the layouts used here.

## Open questions

1. **ccmd_high_lod placement.** Its object is first in .bss and its literals first in .data, but its
   initialiser is the last function. Written at the top (definition order for data); the initialiser order
   will tell where VC6 emits the dynamic initialiser of a global.
2. **0x5a8480** (int 10, unreferenced) sits between the net strings and 0x5a8484; owner unknown, not
   defined here.
3. **Compound float operands.** `total += w` compiles to `fld w; fadd total` here; whether VC6 does that
   for `+=` or the source said `total = w + total` decides several unsure lines in increment_tick,
   blend_bone_keys, build_bone_matrices and get_root_velocity.
4. **Locals vs reloads in build_bone_matrices**: the binary keeps `character` in a local for some reads
   and reloads `this->character` for others; written to follow the loads, which is unusual source.
5. **ui_window::init's lost test** and parse_slider's two pointers (create_scrollbar is called through the
   `ui_slider *` that is NULL for a plain slider): shapes guessed.
6. `ui_stub_ret_52afe0` (bare ret) and `ca_stub_ret30_51b380` (ret 0x30): unreferenced empty functions;
   signatures unknown (the second written as a member taking a matrix43 by value).
7. handle_arrow_key's distance expressions: the binary computes `y - g->h - g->y + h`; written
   `(y + h) - (g->y + g->h)`.
