# matrix.cpp and skeleton.cpp (decomp25)

Written from Sum.exe v1.40 only (`tools/asm.mjs`, `build/sum.asm`, raw .data bytes for literals and
float constants). No Summoner 2 or Dash Faction code was read. Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>`
(warnings only: memcpy / memset on classes with constructors, which is what the original does), and
every `src/decomp` .cpp of the repo at commit d790a53 compiles against `include/` (synced from the repo
tree at that commit plus the changes in CHANGED-HEADERS.txt).

Header decisions: D500-D504 at the end of `include/DECISIONS.md`.

## vsdk\math\matrix.cpp (0x50f9d0-0x511c06; header inline COMDATs to 0x511d73)

Bounds: 0x50f990 / 0x50f9b0 (quaternion constructor / operator=) belong to the gr unit's COMDAT tail;
the first matrix.cpp function is the identity_matrix initialiser (initialiser table 0x5888f0). The last
own function is matrix43::transpose_in_place 0x511bb0; 0x511c10-0x511d73 are header inlines (D500);
0x511d80 is registry.cpp. The untagged functions inside the range (sum.symbols module `vsdk\math`:
0x50fb30, 0x50fb80, 0x50fff0) are matrix constructors and belong to the unit.

| Address | Function | Confidence |
|---|---|---|
| 0x50f9d0/e0, 0x50fa30/40 | initialisers of identity_matrix / identity_matrix43 (thunk + body, from the definitions) | likely x2 (generated) |
| 0x50faa0 | matrix(const matrix &) | exact |
| 0x50fad0 | matrix(r, u, f) | exact |
| 0x50fb30 / 0x50fb80 | matrix(sin/cos x6) / matrix(p, b, h) | exact x2 |
| 0x50fbc0 | operator* | likely (term order) |
| 0x50fcf0 / 0x50fe20 | multiply_by_transpose / multiply_transposed | likely x2 (term order) |
| 0x50ff50 | rows_match | exact |
| 0x50fff0 | matrix(axis, angle) | exact |
| 0x510020 | set_from_sincos | exact |
| 0x5100c0 | set_from_angles | exact |
| 0x510110 | set_from_axis_angle | likely |
| 0x510240 | extract_angles | likely |
| 0x510390 | get_axis_angle | likely |
| 0x510510 | has_zero_axis | exact |
| 0x510660 | is_valid | exact |
| 0x5106a0 | determinant | likely (term order) |
| 0x5106e0 | orthogonalized_from_fvec | exact |
| 0x510830 | orthogonalized | likely |
| 0x510a80 | transpose | likely |
| 0x510b10 | inverse | exact |
| 0x510b40 | orthogonalize | exact |
| 0x510c90 | orthogonalize_b | likely |
| 0x510e70 | transpose_in_place | exact |
| 0x510ea0 | set_random | exact |
| 0x510ed0 | invert | likely |
| 0x511050 | make_identity | exact |
| 0x511080 / 0x511140 | set_from_forward / set_from_unit_forward | likely x2 (store order) |
| 0x511200 / 0x5112d0 | set_from_forward_up / set_from_forward_right | exact x2 |
| 0x5113a0 / 0x5113f0 / 0x511450 | matrix43(rot) / (r, u, f, p) / (pos, rot) | exact x3 |
| 0x5114b0 | matrix43::operator* | likely |
| 0x511570 | matrix43::equals | likely |
| 0x5116f0 | matrix43::inverse | likely |
| 0x5117e0 | matrix43::make_identity | exact |
| 0x511840 | matrix43::set(q, pos) | exact |
| 0x511860 | matrix43_interpolate | likely |
| 0x511920 / 0x5119c0 | matrix43_interpolate_quat / matrix43_set_interpolated | exact x2 |
| 0x511a40 | matrix::rotate_between | exact |
| 0x511ab0 | matrix43::set_product_4x4 | likely |
| 0x511b70 | matrix43::get_rotation | exact |
| 0x511bb0 | matrix43::transpose_in_place | unsure |
| 0x511c10-0x511d73 | vector3::set, operator==, matrix43::transform, set_position, get_position, set_rotation | header inlines (bodies in vector.h / matrix.h) |

48 functions written: 27 exact, 20 likely, 1 unsure; plus 6 header inlines emitted here. The other
inline bodies now in vector.h / matrix.h (vector3 constructors, =, +, -, *, unary -, +=, -=, *=, cross,
len; matrix(), matrix::operator=; quaternion constructors and =; matrix43(), operator=, get_position(out))
are written from their COMDATs elsewhere (D500).

.data defined here: `Homogeneous_column` 0x5a7480 and set_product_4x4's own copy 0x5a7490 (function
static), then the two get_axis_angle error literals. .bss: `identity_matrix` 0x2692ea0, `identity_matrix43`
0x2692ec8 (to 0x2692ef7; registry.cpp starts at 0x2692ef8). Error line: 505 (get_axis_angle).

## vsdk\ca\skeleton.cpp (0x543040-0x544a8d; inline / generated COMDATs to 0x544b7e)

Bounds: 0x543030 is the ui unit's stub; 0x544b80 StringPool::Init is stringpool.cpp. The scalar deleting
destructor 0x541980 (calls ~skeleton) sits in character.cpp's block (compiler-generated, not here).

| Address | Function | Confidence |
|---|---|---|
| 0x543040 | vim_alloc | exact |
| 0x5430b0 | vim_free | exact |
| 0x5430e0 | vim_write_cache | exact |
| 0x543230 | vim_write_header | exact |
| 0x5433d0 | vim_read_header | exact |
| 0x543430 | vim_read | likely |
| 0x5438b0 | vim_load_file | exact |
| 0x543920 | vim_load | likely |
| 0x543a80 | vim_reload_bitmaps | exact |
| 0x543bb0 / 0x543bd0 / 0x543c00 | vim_copy_bitmaps / vim_save_bitmaps / vim_restore_bitmaps | likely x3 (loops turned into rep movsd) |
| 0x543c20 | vim_find_tag | exact |
| 0x543c70 | vim_get_tag_transform | likely |
| 0x543cd0 | vim_bitmaps_loaded | exact |
| 0x543d00 / 0x543d10 | vim::vim / vim_header::vim_header | likely x2 (explicit or implicit) |
| 0x543d20 | skeleton::skeleton() | exact |
| 0x543d50 | skeleton::skeleton(filename, name, is_state, unused, root_rotation) (sum.symbols skeleton_add) | likely |
| 0x543e10 | skeleton::~skeleton | exact |
| 0x543fc0 | skeleton::get_root_velocity | likely |
| 0x544010 | skeleton::get_root_rotation | likely |
| 0x544090 | skeleton::read (skeleton_read) | likely |
| 0x544a10 | ca_init_globals_2ee4598 | exact |
| 0x544a40 | ca_close_globals_544a40 (bare ret) | exact |
| 0x544a50 / 0x544a90 | skeleton_read_tag_record / skeleton_read_tag_records | exact x2 |
| 0x544ad0 / 0x544b20 / 0x544b30 | anim_pos_key(time, m), anim_key(time), anim_pos_key::get_size | header inlines (skeleton.h) |
| 0x544b40 / 0x544b60 / 0x544b70 | anim_rot_key(), anim_pos_key default-constructor closure, anim_vec_key() | compiler-generated |

27 functions written: 15 exact, 12 likely, 0 unsure; plus 3 header inlines and 3 compiler-generated.
Nothing of the unit is left.

.data (0x5ac8e0-0x5ac9e3): literals only, one copy per use (the path twice). .bss: `vim_saved_bitmaps`
0x2ee4524 (20 ints), `vim_saved_bitmap_count` 0x2ee4574, seven "" literals 0x2ee4578-0x2ee4590 (name
buffers), 0x2ee4594 unreferenced, `skeleton_count` .. `skeleton_mem_limit` 0x2ee4598-0x2ee45a8.
Error lines: 150 ("Too many skeletons"), 692 ("Skeletons using too much memory").

File formats read here (for docs/sum-exe/vsdk-ca.md): .vim version 6: header (version, flags, vertex
count, uint16 sub-mesh count, centre, radius, block size), data block, int, per sub-mesh seven uint16
sizes, bone count, root bone, extra-bone count, extra-bone names, tag count, texture count and names.
.mvf: optional "VMVF" + version, start, end, root quaternion (components below 1e-4 cleared,
normalised), root displacement (y forced to 0), bone count, (version 5+) key totals, per bone a weight
byte, rotation keys (version 5+: int16 x4 + 8 skipped int16 + two ease bytes; 2-4: the same with float
eases * 2.54; 0-1: float quaternion * 16383, three tangents * 127, eases) and position keys (time + 3
vectors), then (unless at end of file) fades in frames, five triggers (255-byte name + time) and
(version 4+) morphs (version 8+ with int16 ids, key counts and frame numbers; older files print the
"FILE ISN'T MVFREDUCED" warning).

## Floating point (D504)

VC6 without /Op keeps intermediates on the x87 stack and commutes / reassociates sums: the column sums
of matrix43::operator* come out in three different orders. The source order of terms is therefore not
recoverable; natural index order is written. Reused partial results, spills and `2 * a` = `a + a` are
reproduced as the code shows them.

## What the code says about D42 (D500)

The small vector3 / matrix / quaternion / matrix43 members (constructors, operator=, the arithmetic
operators, cross, len, set, ==, matrix43 transform / set_position / get_position / set_rotation) are
inline in the original headers: their only copies are COMDATs among other units' code (or after
matrix.cpp's last function), the vsdk units (built without inline expansion) call them, and the
optimised game units expand them (conversationctl's expanded transform has 0x511c70's expression tree;
objects.cpp expands matrix::operator=). vector.h / matrix.h now define them in the class body.

## Game bugs noticed

1. matrix::extract_angles (0x510240): cos(pitch) is estimated as sin(h) * fvec.x (or cos(h) * fvec.z),
   a multiply where a divide recovers cos p; the result is cos p * sin^2 h, so the pitch is wrong for
   a pitched matrix unless the heading is a multiple of 90 degrees. Latent for upright characters (fvec.y
   = 0 gives pitch 0 anyway); callers mp_send_obj_update, character_save_state.
2. matrix43::transpose_in_place (0x511bb0) is not a transpose (two five-element rotations through the
   position row). Unreferenced.
3. skeleton::get_root_rotation (0x544010) slerps from a default-constructed (uninitialised) quaternion,
   not the identity. No direct caller.
4. vim_load_file (0x5438b0) leaks the vim from vim_alloc when the version is below 6 or vim_read fails;
   vim_free frees nothing when the data pointer is NULL; vim_alloc does not check the new.
5. vim_read writes num_textures handles from the file into textures[6] with no bound (vim_save_bitmaps'
   buffer holds 20): a .vim with more than 6 textures overwrites num_textures / flags. Data-dependent,
   not checked against the retail meshes.
6. skeleton::read: the trigger name is read as 255 raw bytes into a 256-byte buffer and passed to strlen
   without a terminator of its own (relies on the file's NUL).

## Open questions

- D500 / D502 rest on placement: the six functions after matrix.cpp's last function and the six after
  skeleton.cpp's are taken as header inlines (COMDAT tails, as in character_instance.cpp); conversationctl's
  expansion confirms it for transform only. Their order (vector.h members, then matrix.h members) is not
  first-use order in either unit.
- matrix43::equals ends with `test al, al; setne al` after operator==: written as `? true : false`.
- The 41-byte name buffers (`mov byte; rep stosd` x10) in vim_write_cache / vim_load / vim_reload_bitmaps.
- vim::vim / vim_header::vim_header placed among the unit's functions: written as explicit empty
  constructors.
- vim_load formats "Unable to load vim" into a local buffer and drops it (a compiled-out message box?).
- 0x2ee4594 (between the last "" literal and skeleton_count) has no reference.
- The fourth constructor argument of skeleton (character_add_skeleton passes it) is not read.
- sum.symbols could rename: skeleton_add -> skeleton__skeleton_args, skeleton_read -> skeleton__read,
  matrix3__orthogonalized_from_fvec (it is orthogonalize on a copy), and tag
  0x50fb30 / 0x50fb80 / 0x50fff0 / 0x511c10 / 0x511c30 with the module they sit in.
