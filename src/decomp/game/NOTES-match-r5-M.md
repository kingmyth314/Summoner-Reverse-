# match23: VC6 (cl 12.00.8966) byte-match pass over 26 units

Tree: `match23/src` (copy of `snapN/src/decomp`). Compiler: VC6 SP5 (`scratchpad/vc6sp5/VC98`). Flags: the
tree's `vc6-flags.txt` with its vsdk lines replaced by `scratchpad/vc6-flags.vsdk-ow.txt` (D7502, `/Ox /Ow`
for the vsdk library, as the coordinator asked); game lines unchanged. Decisions D8200-D8203 in
`src/include/DECISIONS.md`.

## Counts (tools/vc6-compare.mjs)

vsdk "before" is re-taken under the new /Ow flags (the old /O2 count in brackets). Game units stay on /O2.

| unit | before | after | of |
|---|---:|---:|---:|
| game/multispell.cpp | 15 | 15 | 22 |
| game/multi_chattracker.cpp | 13 | 15 | 20 |
| game/unnamed_snow_0x4946a0.cpp | 8 | 8 | 11 |
| game/level_script_khosani_lab.cpp | 39 | 39 | 45 |
| game/new_automap.cpp | 26 | 26 | 32 |
| game/unnamed_tentacle_beast_0x4c6480.cpp | 19 | 19 | 25 |
| game/unnamed_netsession_0x440b40.cpp | 16 | 17 | 21 |
| game/unnamed_decals_0x4bbe80.cpp | 9 | 9 | 15 |
| game/unnamed_shadows_0x4a37d0.cpp | 7 | 7 | 13 |
| game/unnamed_netpackets_0x441820.cpp | 6 | 6 | 12 |
| game/levelrender.cpp | 6 | 6 | 12 |
| vsdk/unnamed_clip_0x5590b0.cpp | 13 (10) | 13 | 17 |
| vsdk/gr_font.cpp | 11 (10) | 13 | 17 |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 7 (7) | 7 | 9 |
| vsdk/unnamed_string_0x5566e0.cpp | 66 (63) | 69 | 69 |
| vsdk/gr.cpp | 55 (55) | 55 | 61 |
| vsdk/character_instance.cpp | 48 (47) | 48 | 53 |
| vsdk/unnamed_gadget_0x52bfe0.cpp | 44 (42) | 45 | 48 |
| vsdk/window.cpp | 34 (34) | 35 | 40 |
| vsdk/character.cpp | 31 (28) | 31 | 34 |
| vsdk/unnamed_os_0x530b90.cpp | 20 (19) | 20 | 23 |
| vsdk/targautils.cpp | 16 (15) | 16 | 21 |
| vsdk/plainstring.cpp | 14 (14) | 19 | 20 |
| vsdk/unnamed_vfx_material_0x564d70.cpp | 9 (9) | 14 | 15 |
| vsdk/unnamed_http_0x55a8e0.cpp | 10 (9) | 10 | 15 |
| vsdk/unnamed_poly_split_0x55f110.cpp | 1 (1) | 1 | 5 |
| **total** | **543** (527 with the old vsdk flags) | **563** | 675 |

The flags change alone won 16 vsdk functions (String copy constructor, to_lower / to_upper, set_focus /
clear_focus, gr_clip_edge, bbox_from_points / bbox_union, gr_font_read_file, start_anim, load_shadow,
character_create_base, ca_preload_find, cpu_identify, m2v_read_slice_quantizer_scale, http_resolve_host);
none lost. Source changes then won 20 more, address by address none lost (`base0.json` -> `after2.json`).

Whole tree after the header change (sweep, 259 units, these flags): old vfx.h 4,674 / new 4,675 matches; the
per-address lists differ only in vfx_material::reset. mingw `-fsyntax-only -std=c++98`: 259 files, 0 errors.

## What worked (won functions)

- String operator+ x3 (0x556f70 / 0x557070 / 0x557180): the binary tests `r.max_len`, not `r.buf`.
- plainstring: format_indented_column / _b (then-block = fall-through, `if (width <= ...) pad = 1; else`),
  gr_string_truncate_ellipsis (`strcpy(dest, buf); return dest;`), string_to_wide_charmap (`while (1)` with
  `int c = src[i]; if ((char)c == 0) break;`), wide_charmap_to_string (`while (1)` with an explicit `*src == 0`
  break).
- vfx_material: the curve sampler's `j = i + 1` local (eval_curve34 / 44 / 48), eval_curve34's `case 0:
  return 0.0f; ... case 2: return 0.0f;`, get_frame_bitmap_frac's `if (f < 0) f += n; if (f < 0) f = 0; else
  if (f >= n) f = n - 1;`, reset through the D8202 header change.
- gr_font: gr_close_fonts with a walking `Font *f` (`f++, i++`), gr_get_string_size storing *out_h first.
- ui_gadget::init: owned before gadget_type. ui_window::set_tooltip: `&ui_tooltip_colors.c0` (the ui.h shim
  operator is an out-of-line call in this /Ob0 unit), stores delay, x, y, font, colours set before the append.
- irc_connect (the not-connected branch as the then-block with its own `return 0`), irc_get_word (`if (tok)
  {...}`), mp_handle_gm_stat (level / entry / paused set after the client test; `if (state > 2) {...} else`).

## Closer, still different (kept)

- ui_gadget::process: D8200 logic fix; the binary reloads key per child (8966 caches it in EDI).
- ui_window::init: `&ui_window_colors.c0` and the append through a `p` local (65 vs 70 instructions; the
  binary keeps `this` in ESI and pushes both registers up front).
- irc_read_line: `while (select(..))` and a signed length test `(int)strlen(..) < (int)sizeof(..) - 1` (jge);
  the loop exit returns select's zero in EAX without an `xor` in the binary.
- ca_character::add_skeleton: the open failure is the else-block (the binary's error block is last, jumping
  back to its DebugError for ever, as VSDK_ERROR); still 4 bytes of frame and a shared epilogue apart.
- gr_font_create: `Font *f` declared first and `f++, i++` (the slot loop now matches; differs later in the
  pixel conversion).

## Still different (no form found; D8203)

Dead argument slots as temporaries (khosanilab corner_nn / np / pp, multi handlers), register permutations
(send_event, set_parent, blend weight, client_status, spell writers), unshrink-wrapped prologues (irc_add_user,
window init), the plist macro store order (decals_reset, decal_place's `&e->next`, D7006), snow_start's
row step, int_to_string_with_commas' counted loop, gr_classify_sphere_view's FPU copies, get_frame_bitmap's
leftover FPU value, gr_string's gr::Mode argument (D7006), cmd_tentacles' CF_HIDDEN in EBP, eval of the frame
and FPU code in levelrender / shadows / decal clip planes / poly split / vfx mesh (large, not attempted),
mp_handle_summon_allowed / _quick (D7203: int-returning handler type), mp_reset_net_stats (D7104).

## Markers

Raised to exact (matched under 8966): every match in these units whose marker said likely / unsure (see
`markers.log`). Lowered to likely (marked exact, differ under 8966): mp_handle_summon_allowed 0x4b06a0,
mp_handle_summon_quick 0x4b08f0, gr_clip_line 0x559350, gr_clip_plane 0x5593a0, bbox_silhouette_corners
0x559890, snow_start 0x494ab0, ui_gadget::send_event 0x52c330, ui_window::send_event 0x52aff0,
automap_center_on_player 0x4023b0, mp_player_alloc 0x441610, decals_reset 0x4bc640.

## Logic errors / game bugs

- D8200 (decomp logic error, fixed): ui_gadget::process returns the last child's process() result when the
  gadget is visible and has children (0x52c3bc returns EAX from the call); the source always returned key.
- ca_character::add_skeleton: a skeleton file that fails to open loops in DebugError for ever (0x540b4a ..
  0x540b77 `jmp 0x540b5d`), as VSDK_ERROR already models; not a new bug, noted for completeness.

## Header changes

- `include/vsdk/geom/vfx.h` (D8202): `unsigned short blend : 3` in vfx_material's flag union.

## Files

`CHANGED.txt` (changed files), `before.json` / `before_vsdk_ow.json` / `base0.json` / `after2.json`
(per-address status), `sweep_old.txt` / `sweep_new.txt` (whole tree, old / new vfx.h), tools `base.mjs`,
`cmp.mjs`, `sd.mjs`, `dumpall.mjs`, `markers.mjs`, `try.mjs` (accepts `.mjs` variant modules).
