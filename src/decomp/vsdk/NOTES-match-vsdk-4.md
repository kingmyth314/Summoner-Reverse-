# VC6 match pass 10 (seven vsdk units)

Base: `snapE/src/decomp` (the repo's src/decomp). Compiler: VC6 Professional cl 12.00.8168 (`vc6p`), per-unit flags
from `src/vc6-flags.txt` (unchanged), headers from `match10/src/include`. `before/<unit>.txt` and `after/<unit>.txt`
are the `vc6-compare -v` outputs; `hdr_before/` and `hdr_after/` the six other units that include a changed header;
`fl1..fl5` / `fz1..fz4` the flag surveys before / after; `v/` the variant files tried (m10tools/trys.mjs format).

| Unit | Before | After | Won |
|---|---|---|---|
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 3 / 14 | 3 / 14 | - |
| vsdk/gr.cpp | 50 / 61 | 55 / 61 | gr_fade_out, gr_fade_in, gr_set_gamma, gr_circle, gr_set_antialias (header D6801) |
| vsdk/targautils.cpp | 11 / 21 | 15 / 21 | tga_convert_pixels, copy_pixels_advance, pcx_write, pcx_encode_line |
| vsdk/file_packfile.cpp | 10 / 20 | 11 / 20 | packfile_hash |
| vsdk/character.cpp | 24 / 34 | 28 / 34 | add_skeleton_trigger, set_skeleton_trigger_time, reload_mesh, character_clear |
| vsdk/unnamed_clip_0x5590b0.cpp | 8 / 17 | 9 / 17 | bbox_straddles_plane (header D6802) |
| vsdk/unnamed_net_0x517590.cpp | 19 / 31 | 22 / 31 | nw_buffer_push, nw_init_socket, nw_flush_receive |
| **Total** | **125 / 198** | **143 / 198** | **+18** |

(r.sh counts one extra line for the two units whose output has a target warning line: 15 / 18 there.)

No function lost its match (checked address by address, also in the six header units: levelrender 5, gr_opengl 31,
unnamed_ca_mesh 8, unnamed_gr_line 2, unnamed_gr_poly 6, unnamed_vfx_mesh 3, the same addresses before and after).
Every matching function's comment now says `Confidence: exact` (44 raised: the 18 new ones and 26 that had matched
before with likely / unsure). Eleven functions already marked exact still differ (D6804); left as they were.
mingw (`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`): 0 `error:` lines in all 259 .cpp.
Decisions D6800-D6804 are appended to `src/include/DECISIONS.md`.

## Flags (D6602)

Surveyed /O2, /O2 /Ob0, /Ox /Ob0, /Ox (plus /O1, /Og-forms, /Gy, /Gf, /Op on character and file_packfile) before and
after the source changes. No set gives more matches without losing one: gr is 55 under its /O2 line, 54 under
/Ox /Ob0 (loses gr_set_gamma; before the changes it won Color::Color and gr::Mode::Mode and lost two others),
52 under /O2 /Ob0;
the other six are equal under /O2 /Ob0 and /Ox /Ob0 and worse without /Ob0 (vfx_emitter, character, clip). No line
proposed for vc6-flags.txt.

## What worked (D6800)

- Three unfolded constant factors (frame * 1/30 * 30 * 160) with float locals: both trigger functions.
- Reusing the argument: `gamma = 1.0f / gamma` (the exponent's slot is the argument's), `powf(...) * 255.0f` for the
  float multiplier.
- `ms <= 0` instead of `ms < 1` (test / jg).
- Declaration order = store order: gr_circle (angle first), pcx_encode_line (total before count).
- Loads before stores via locals: reload_mesh's pool, copy_pixels_advance's two pointers, nw_buffer_push's address.
- packfile_hash: the name walked through a local pointer (register choice).
- character_clear: the two LOD clears as a counted loop (their own zero register).
- tga_convert_pixels: `* 8` / `* 4` instead of shifts keeps the binary's masks.
- Shared `return 0` (listed as not reproducible in D6514): the found work inside the search loop and one return
  after it (nw_buffer_push exact; nw_buffer_pop, nw_accept_reliable, packfile_add closer).
- `while (1) { if (!status(i)) break; ... }` (nw_flush_receive).
- pcx_write: filename is 512 bytes (frame 0x394).
- nw_init_socket: `volatile int error` keeps the binary's dead store and frame slot (a reproduction device).

## What still differs and why (D6803)

Not reproducible with 8168: stack pops not merged across calls (report_memory, create_base, add_skeleton,
packfile_init, packfile_alloc_directory), gr_vertex's out-of-line constructor (gr_rect, gr_bitmap_float; D1605 open),
vector3 copy elision (bbox_from_segment, fx_recb0 apply_forces / move_particles / get_transform / update /
render_d3d / render / init_particle / get_billboard_axes: frames 12 bytes per temporary larger), inline strcat of a
literal as dword moves (load_shadow, tga_read_header), cross-jumping (m2v_skip_extension_and_user_data,
nw_init_networking), FPU values kept on the register stack (gr_classify_sphere_view, get_fade, get_color, spawn),
the `or eax, eax` of nw_can_write, EBP-zero pushes (pcx_read_bitmap), strength-reduced array walks
(ca_load_preload_index, gr_tmapper_multipass), and store scheduling / register choice in the rest (packfile
check_header / read_directory / assign_offsets / fseek / fread / find_in_packfile, gr_init, gr_line, the bbox
helpers, gr_clip_line / gr_clip_plane / gr_clip_edge, tga_rle_encode_row, m2v_read_slice_quantizer_scale,
nw_buffer_pop, nw_ras_init, nw_WorkReliable, nw_NewSendReliable, nw_ConnectToServer, nw_NewCloseSocket).

Partial progress kept: ca_preload_find (to +0x4a), tga_rle_decode (frame and slots now right; `and ebx` vs
`and bl`), packfile_read_directory (rotated loop, to +0x28), packfile_add (to +0x11b), nw_buffer_pop (to +0x66),
nw_accept_reliable (to +0x61), gr_clip_line (to +0x26), gr_classify_sphere_view (to +0x1b).

## Logic errors and game bugs found

- **Decomp logic error (fixed): ca_character::set_skeleton_trigger_time** had `strcmp(...) == 0`; the binary sets
  the time on every named trigger whose name DIFFERS (`test eax, eax / je` skips the equal ones at 0x540dbe). This is
  a game bug in the original (the function was meant to set the slot of that name); harmless in practice, nothing
  calls it.
- **Decomp signature error (fixed, D6802): bbox_straddles_plane** is (min, max, plane), not (plane, min, max): the
  plane comes from the third argument. No caller.
- **Declaration (fixed, D6801): gr_opengl_set_antialias** takes a bool (the call pushes the bool's dword unmasked).
- Numeric fidelity: the trigger times now multiply by 1/30, 30 and 160 in sequence as the binary does (the folded
  form could round differently in the last bit); gr_set_gamma multiplies the pow result by a float 255 as the
  binary does.
- tga_rle_decode's header byte as an int: same behaviour (the byte is zero-extended in the binary too).
