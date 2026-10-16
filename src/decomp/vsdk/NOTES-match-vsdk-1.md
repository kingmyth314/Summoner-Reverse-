# VC6 match pass 2 (ten vsdk units)

Base: `snapB/src/decomp` = commit 95bd3d5. Compiler: VC6 Professional cl 12.00.8168 (`vc6p`), per-unit flags
from the repository's `src/decomp/vc6-flags.txt` (unchanged), headers from `match2/src/include`.
`before_<unit>.txt` in this folder are the baseline `vc6-compare -v` outputs.

| Unit | Flags | Before | After |
|---|---|---|---|
| vsdk/unnamed_snd_0x5204c0.cpp | /O2 /Ob0 /GX /MT | 24 / 58 | 43 / 58 |
| vsdk/gr_glide.cpp | /O2 /Ob0 /GX /MT | 31 / 61 | 44 / 61 |
| vsdk/unnamed_vfile_0x5128b0.cpp | /O2 /GX /MT | 53 / 82 | 57 / 82 |
| vsdk/gr_opengl.cpp | /O2 /Ob0 /GX /MT | 25 / 54 | 31 / 54 |
| vsdk/unnamed_lights_0x51e070.cpp | /O2 /Ob0 /GX /MT | 15 / 41 | 24 / 41 |
| vsdk/unnamed_listbox_0x52ca60.cpp | /O2 /Ob0 /GX /MT | 44 / 69 | 51 / 69 |
| vsdk/bmpman.cpp | /O2 /GX /MT | 32 / 56 | 37 / 56 |
| vsdk/unnamed_quaternion_0x5225b0.cpp | /O2 /GX /MT | 9 / 32 | 10 / 32 |
| vsdk/unnamed_ds_0x55bf90.cpp | /O2 /Ob0 /GX /MT | 37 / 60 | 40 / 60 |
| vsdk/parse.cpp | /O2 /Ob0 /GX /MT | 29 / 51 | 32 / 51 |
| **Total** | | **299 / 564** | **369 / 564** |

No function lost its match. Every matching function's comment now says `Confidence: exact` (71 comments raised,
including functions that already matched before this pass). Decisions D6000-D6005 are appended to
`src/include/DECISIONS.md`. mingw (`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`): 0 `error:`
lines for all 253 .cpp files of the tree (three headers changed). The one other unit that reads a changed
declaration, unnamed_ca_mesh_0x50af80.cpp (glide_pending_deactivations), keeps its 8 of 12 matches.

## What worked (general)

Most of the remaining differences are build 8966 against our 8168, not the source: register allocation,
scheduling, and a better alias analysis in 8966 (it keeps globals and fields in registers across stores
through unrelated pointers, moves loads above such stores, and keeps loop bounds in registers). The source
forms that do make 8168 produce the original (D6005):

- **One-case switches** for `snd_backend` (`mov eax, [x]; dec eax; jne`), with a result variable preset to
  the failure value where the function returns the backend's result (D6003). 17 functions.
- **`volatile`** for the two Glide focus counters (D6001): register loads instead of memory operands. 7.
- **bool returns in if-form** (`if (f() == 0) return true; return false;`) where the original has
  `test / setcc al` without a zeroed register or `mov al, 1 / xor al, al`. 11 (vfile, listbox, opengl, ds,
  parse, bmpman). A plain member comparison is right as an expression (three vfile functions got worse in the
  if-form and were put back).
- **Local copies** where 8966 kept a value in a register: loop bounds read once before loops that call out
  (lights_debug_draw, lights_transform_selection), a local alias of a global pointer for an init loop
  (gr_opengl_texture_init), `next = free_head` after the call (glide_record::release_block), the unlink of
  bm_alloc_entry through `prev` / `next`.
- **Pointer-walking counted loops** (`for (i = 0; i < N; i++, p++)`) where the original compares the counter
  (snd_level_page_in, gr_opengl_tcache_frame).
- **Statement order and branch layout**: `if (selected) {...} else scroll_page_*()` instead of an early
  return (listbox select_page_up / down); FileReader::seek with the memory branch first; the colour locals
  interleaved with the vertex stores (gr_glide_line / line_3d); `oy` computed after `v1.x`; `neg = false` after
  SkipWhitespace (ParseFloat); `free_index = -1` before the hash call (ds_find_buffer); local declaration
  order reversed (cquat::normalize).
- **Operand order**: `clip_right + offset_x + 1` in glRecti / grClipWindow (the lea operand order), `z * w` in
  quaternion::to_matrix (closer, not yet a match), `n <= 128 ? 128 : 256` in GLIDE_TEXTURE_SIZE.
- **Split constants**: `float s = v * 0.0625f; ... s * 0.001f` (gr_opengl_set_scale_2f13360: VC6 had folded
  the two constants into one). The constant order was checked against Sum.exe's .rdata (0x580dec = 0.0625,
  0x57ef24 = 0.001): vc6-compare masks absolute addresses, so it cannot see a swapped constant.
- **Call hoisting**: a local for `gr_get_aspect()` before gluPerspective (8966 evaluates call arguments
  first).
- **Switch cases**: bm_bytes_per_pixel lists cases 1 and 2 (the table covers formats 1-8) with a return after
  the switch; snd_init_group_volumes is a 4-iteration loop (the constant in EAX).

## Per unit: still differing and why

**snd (15 left).** The four clamps (snd_set_volume, _in_group, _set_pan, _set_group_volume): the original
places both clamp stores after the second test; no if / else / ternary form reproduces it. snd_load now
differs only at `s->loaded = 1` (8168 `or [mem], imm`, original load / or / store); its frame (path 0x200,
D6004) and the if / else strcpy are fixed. snd_calc_volume_3d, sound_emitter_calc_volume / _pan, snd_calc_pan:
the original keeps a `zero` / `pan` float in a stack slot (`mov [esp+x], 0`, later `fld [esp+x]`) that 8168
constant-propagates (`/Op` loses 9 other matches). snd_init_sounds, snd_level_release, snd_unload: scheduling
(the release decrement of the 3-bit field is `dec ecx` in the original, `sub ecx, 0x10000000` in ours).
sounds_parse_table / _entry: register allocation.

**gr_glide (17 left).** glide_load_dll: the original keeps the DLL handle in ESI across the 112 GetProcAddress
calls; a local handle gets within one instruction (the first call still pushes EAX) and was not kept.
gr_glide_tcache_free_entry: if / else with the empty-entry branch last gets the layout, but the TMU-clear loop
hoists entry->sections and the count (alias analysis); not kept. The texture creation functions, bitmap,
tmapper(_multitexture), the glide_record allocator: register allocation and frame size. glide_msg_handler:
msg in EAX against ECX (a switch does not change it).

**vfile (25 left).** The seven FileReader::read_* readers: the original pushes ESI for the whole function and
stores `pos` before the load; every form tried (12 variants) gives either the push inside the branch or the
load first. file_make_path / _cd_path / _add_path / _change_ext / _chdir: 8966 inlines `strcat(out, "\\")`
as a word move of the literal; 8168 calls the strlen / copy sequence. File::open_found, seek, close,
make_backup, calc_crc32: register allocation / frame order.

**gr_opengl (23 left).** The eleven gl_convert_* functions: the original keeps tex_h in memory and the loop
bound in a different register (systematic, compiler). gr_opengl_delete_texture: a cached `bm` gets to the
last two stores (scheduling), not kept. set_viewport, init_context, set_mode, create_texture, bitmap,
tmapper, read_backbuffer, apply_tint (the sum stays in the FPU with `fst`), get_texture_scale (load moved above
the store): scheduling / alias.

**lights (17 left).** light_set_enabled / set_all_enabled / set_dynamic: the original stores the bit with
`and dh, -0x41` (partial register, 8966); `bool on` gives the right `and cl, 1 / movzx`, but the 6-byte `and
edx, imm` stays, so the signatures were not changed. light_alloc, light_delete, light_get_color /
get_radius, get_selected_info, create_spot: store scheduling. The shading functions (shade_point,
calc_point_specular_color, sample_projected_bitmap): frame size and register allocation.

**listbox (18 left).** entry_at / entry_at_y / entry_above / snap_scroll_top: the original repeats the loop
test at the loop entry after the same test (8168 threads the jump). The scroll_* and draw functions: block
layout and register allocation deeper in the code. ui_listbox::ui_listbox, create, add_entry: EH state stores.

**bmpman (19 left).** bm_is_reduced (the original's dead `lea eax` then `mov al` of the byte; a `bool reduced` field was
tried and changes no function), bm_hash_name (8966 turns the shift pair into `rol`; `_rotl` gives the rol but swaps
ESI / EDI), bm_lock_pcx / bm_lock_tga (`mov ecx, [num_pixels]; imul ecx, eax` against `imul eax, [mem]`;
bm_lock_pcx now matches up to that point), bm_get_data / bm_lock / load_animation(_path) / free_entry /
release_from_mark: loads moved above stores. Note: `/Oa` (assume no aliasing) turns five of these into
matches but loses others in other units; the original is neither /Oa nor 8168's default.

**quaternion (22 left).** Every function that returns `quaternion(...)` / `cquat(...)` by value: 8966
constructs straight into the return slot, 8168 always builds a temporary and calls the copy constructor
(tested with and without a user copy constructor, /Ob0 / /Ox / /O1 / /Od). Also the unit calls its inline
members out of line in the original (slerp calls quaternion__operator_sub, conjugate calls cquat(x, y, z, w)
at 0x523d50), so it was built without inline expansion; under /O2 /Ob0 quaternion_squad matches too, but
vc6-compare then pairs the first `quaternion::quaternion` it finds with the out-of-line COMDAT constructor, so
the flags line was left alone (suggestion: /O2 /Ob0 for this unit once the pairing handles it).

**ds (20 left).** sprintf(msg, ..., ds_error_string(hr)): the original cleans the inner call's argument
before pushing sprintf's (`add esp, 4`); a local for the string does not change 8168's merged cleanup (tried
on all 12 sites, put back). ds_is_3d_buffer: `shr al, 4 / and al, 1` byte narrowing. ds_stream_read_chunk:
the original reuses `r = 0`'s register for the zero arguments and returns `or eax, eax`. The rest: register
allocation.

**parse (19 left).** GetPosition / SetPosition: both loads before both stores (locals tried, not kept).
ParseInt / ParseUInt / ParseString / the list parsers: register choice of `this`; declaration permutations
do not change it. SkipBlockComment / SkipSpaces / SkipLine / ReadLine / SkipToNextLine: the original reads
the next character before storing the cursor (alias).

## Logic errors and game bugs found

- **Header error (fixed, D6000):** bool_to_cstr takes `(char *dest, int size, bool b)`; the flag is the third
  argument. The old declaration would read the size as the flag.
- **Source error (fixed, D6004):** snd_load passed `sizeof(path)` (0x100) to File::get_packfile_path; the
  binary passes 0x80, and path is a 0x200 buffer.
- **Possible game bug (inferred, not tested):** because snd_load passes a limit of 0x80, a sound inside a
  packfile whose `install_path + packfile name` is 128 characters or longer makes get_packfile_path fail; path
  then keeps File::get_full_path()'s result with offset 0, so ds_load_buffer would read the wrong data (or
  nothing) from a deep install directory. Worth a runtime check with a long install path.
- **Type error (fixed, D6002):** light::dynamic is a signed one-bit field, so its value is 0 / -1; every user
  only tests it, so behaviour is unchanged.
