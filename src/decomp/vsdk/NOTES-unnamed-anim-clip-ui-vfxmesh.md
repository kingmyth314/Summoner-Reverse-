# decomp70: Sum.exe v1.40, 0x550660-0x5534d0, 0x553ba0-0x5548b0, 0x562460-0x562550, 0x562a20-0x564d60, 0x565610-0x5664a0

Written from the Sum.exe disassembly (tools/asm.mjs) against the include copy of snap9147 (commit 9147a03).
Every file is a placeholder (`vsdk/unnamed_<what>_<address>.cpp`): no unit in these ranges has a path string, a
strings.tbl block or another hard name, so modules.txt is empty. Decisions: D5100-D5111 (include/DECISIONS.md).
Header changes: CHANGED-HEADERS.txt (four SIGNATURE changes). Check: all snap9147 game/ and vsdk/ .cpp files and
the six new ones give 0 `error:` lines with `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Idecomp70/include`.

None of the new code extends an existing .cpp: each range is its own object (D5100), so there is nothing to fold
into an existing file. Two notes for the lead: vfx_mesh_face_uvs (0x2c98a90, 2200 x 0xc bytes) should be defined
in vsdk/effect_mem.cpp (its .bss; D5110), and game/unnamed_render_scene_0x481680.cpp line 1449 should read
`base->pos` instead of `rec->glow_pos` (D5107).

## Units

| File | Range | What | Functions | exact / likely / unsure |
|---|---|---|---|---|
| vsdk/unnamed_anim_track_0x550660.cpp | 0x550660-0x5519b9 | anim_track (vsdk\ca): Bezier position / scale keys, TCB rotation keys (squad), 3DS ease, relocation | 13 + 20 COMDATs (accessors in skeleton.h, quaternion::unpack_short4) | 31 / 2 / 0 |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 0x5519c0-0x5520c0 | polygon clipping against planes and a convex 2D polygon, plane / sphere tests (vsdk\geom) | 9 + 4 globals | 6 / 3 / 0 |
| vsdk/unnamed_crc32_0x5520d0.cpp | 0x5520d0-0x552192 | Crc32 (table built on first use, no inversion) | 4 + 2 globals | 4 / 0 / 0 |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 0x5521a0-0x5534e1 | ui_inputbox gadget (vsdk\ui, vtable 0x581f1c) | 13 | 7 / 6 / 0 |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 0x553ba0-0x5549b9 | ui_checkbox and ui_radio_button (vtables 0x581f68 / 0x581c7c) | 12 | 8 / 4 / 0 |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 0x562a30-0x564d62 | the vfx mesh object: dump_mesh_frames ccmd, fx_rec98 (mesh instance) init / render / billboards / mesh drawing / frame update, vfx_node_desc relocate / load_bitmap | 11 + 28 COMDATs (accessors in vfx.h) + 2 globals | 32 / 5 / 2 |

Already written before this pilot (no new code; the lead's list counted them because their address comments are
in headers or in a COMDAT list, not on a `// 0xADDR` line): 0x562460-0x562520 vfx_emitter_desc::get_offset ..
eval_channel_d0, 0x562540 fx_rec28::get_size, 0x562a20 get_fixed8_32 (vfx.h / effect_mem.h, listed at the end of
unnamed_vfx_emitter_0x55ff00.cpp), 0x564c60 / 0x564c70 (fx_rec28, effect_mem.h), 0x564d50 ptr_relocate (vfx.h),
0x565610 vfx_vec_channel::eval_fixed12 (vfx.h, listed in unnamed_vfx_material_0x564d70.cpp). 0x565660-0x5664a0 is
libjpeg 6b (jcapimin.c, jcapistd.c, jcparam.c, jdatadst.c, jerror.c), identified in decomp66 and not rewritten.

### anim_track (vsdk\ca)
No data. The regular functions run to relocate (0x551700); the accessors (0x551770-0x55195d) are the inline
members' COMDATs, quaternion::unpack_short4 (0x551960) the last. Built without inline expansion (D5102).
anim_track_eval_pos_bezier and anim_track_interp_rotation are cdecl helpers (static here). The track also holds
scale keys (D5101), used only by the vfx meshes.

### polygon clipper (vsdk\geom)
Opened by the static initialiser of a 64-entry vector2 buffer (0x5519c0); .bss 0x2f13368-0x2f136e7
(geom_poly2d_clip_points 0x2f13368 (name ours), _dists 0x2f13568, _list 0x2f135e8, _list2 0x2f13668 (name
ours)). polygon_clip_2d, polygon_any_point_in_front and the two sphere tests have no caller.

### CRC-32
Crc32::init_table uses the object's value as its scratch (it is stored at every step) and resets it; the table
flag is set once. No initial or final XOR.

### ui_inputbox (vsdk\ui)
Keys: Backspace, Left and Delete all delete the last character; Esc (unless flag 2) and Home clear the text;
Enter is swallowed; Alt / Ctrl combinations are ignored; keypad digits are typed as digits whatever Num Lock is.
Valid / invalid character sets are parsed from strings like "A-Za-z0-9$_-" and rebuilt in the order of the
95-character table 0x581ebc, which lacks '&' (so '&' can never be allowed by a valid set). Events 0x14 / 0x15 /
0x16 to the window callback (key, changed, full; names ours).

### ui_checkbox / ui_radio_button (vsdk\ui)
state +0x38c is recomputed every frame; a release over the box after a press toggles it. Four colour sets of four
(ui_checkbox_colors). Radio buttons clear their siblings.

### vfx mesh object (vsdk\geom)
fx_rec98::render computes the frame once per instance and frame (vfx_last_mesh_instance), then draws a billboard
(sprite or line quad) or the mesh, then the glow bitmap. update_frame: frame = (t / 15 - start_time) * rate
(mesh_flags bits 6-15); animated meshes (flag 4) use the anim record (billboards), the morph channels or static
vertices; skinned meshes (mesh_flags 2) the rest pose plus an anim_track at 4800 ticks per second; the rest blend
two xform frames; then the shown-frame bit, the face uvs, the line axis and the world / bone transform.
render_mesh lights each shared vertex once per frame, sorts the faces back to front (farthest corner + 61 per
material index; level meshes unsorted) and draws up to three passes per face (layer 0, layer 1 with curve 34,
the sphere-mapped environment bitmap). Dump_mesh_frames (0x2f91a6c) is set by its console command and never read.

## Game bugs

- vfx meshes: face normals are decoded with signed bytes but the unsigned mapping (D5108, 0x564a20): every
  component of a face normal comes out in -2..0, so the averaged vertex normals used for lighting effect meshes
  point into the negative octant. Live for lit effect meshes (not FULLBRIGHT, material curve 44 below 254/255),
  visual only. Fairly sure (the vertex normal code uses the same mapping with unsigned fields).
- vfx meshes, xform branch of update_frame (0x564292): the * 64 of mesh flag 4 is applied after the translation,
  so the position is scaled too (the other branches scale before adding the origin). Latent unless a file sets the
  flag on an xform mesh; not checked against the data.
- vfx_node_desc::relocate sets glow_bm to -1 and then to 0 (0x564709); load_bitmap treats a handle of 0 as a
  failure and loads again without the flags. Harmless.
- render_mesh skips faces whose material layers both have bitmap <= 0, so a texture with handle 0 is treated as
  none (0x563079). Latent.
- ui_checkbox::create / ui_radio_button::create measure the label with gr_get_string_size(.., label, font, -1):
  the font goes in as the length and -1 as the font (0x553d0e, 0x5548af). The script editor passes font_small, so
  the gadget's clickable width covers only the first font_small characters of the label (default-font widths).
  Live, script editor only. Sure.
- ui_inputbox::draw measures the text with the default font (-1) but draws it with the box font (0x553217), and
  process measures with the box font; with font_small (the script editor) the scrolling and cursor are slightly off.
  With no room at all the tail pointer starts past the terminator (n stays 1, 0x55324c). Live, cosmetic, editor only.
- ui_radio_button::process: a hotkey / Enter clears byte +0x274 of every sibling gadget, radio or not (0x5547bf;
  for a ui_inputbox that is its flags' low byte, for a ui_button its flags), and sends no event; the click path
  checks the type and sends 0x13. Reached by a radio's hotkey or by Enter while it has the keyboard selection;
  harmless for the editor's radio-only containers, not checked for mixed ones.
- ui_inputbox set_valid_chars / set_invalid_chars: in state 7 (a character followed by '-') the next character is
  dropped ("!-?" allows '!' and '-', not '?'); the allowed table has 128 entries but is indexed with unsigned
  chars, so a set with characters above 0x7f writes past it (0x5528d0). Latent.
- anim_track::get_rotation: before the first key it interpolates keys 0 and 1 at t = 0, reading key 1 of a one-key
  track (0x55138c). Harmless (t = 0). calc_rot_tangents' wrap interval (last - first - time[n - 1] + time[0]) is
  always 0, so the first and last keys of a looping track get a zero weight; every caller passes loop = 0. Latent.
- polygon_clip_2d (dead code): no bounds on the 32-entry lists or the 64-point buffer.

## Disagreements with docs and sum.symbols

- decomp63 / unnamed_render_scene_0x481680.cpp: lamp_glow_update_effects reads the base's pos, not a mesh field
  glow_pos (D5107).
- sum.symbols vfx_particle__get_age / get_lifetime (0x564c60 / 0x564c70) are, by placement, the mesh billboard's
  width / height accessors folded with the particle's (D5109).
- sum.symbols vfx_mesh_render (0x562f60) is a thiscall member of the mesh instance (fx_rec98::render_mesh).
- sum.symbols ui_inputbox_numpad_to_ascii is a thiscall member (ECX set, ret 4).
- source-map.md's unit list puts 0x5520d0-0x5534c0 as "CRC32 and UI inputbox gadget" in one row; the hand-cut
  table splits them, as here. The anim_track code (0x550660-0x5519b9) and the polygon clipper are not in its tables.
- skeleton.h's header comment said anim_track was 0x550f90-0x551350; it is 0x550660-0x5519b9.

## Left

- Nothing in the ranges is unwritten except libjpeg (identified only, by design).
- render_mesh (unsure): the shape of its long body (which locals share frame slots, the pass-2 block order) is a
  reconstruction; stub_ret10 (0x562b90, `ret 0x10`, no caller) is a guess at an empty four-argument member.
- The vfx_face bytes +0x0a..+0x15 are not read by this object.
