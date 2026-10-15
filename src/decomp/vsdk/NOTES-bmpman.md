# bmpman.cpp (decomp31)

`vsdk\bmpman\bmpman.cpp`, code 0x501b20-0x503db5, written from Sum.exe's disassembly only
(`tools/asm.mjs fn`, raw reads of the jump tables and .data). Dash Faction / Summoner 2: names only
(the BM_FORMAT_* names follow Dash Faction's bm::Format; the numbers are Sum.exe's own).
Syntax-checked with `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security
-I decomp31/include` (clean).

**64 functions (every bmpman.cpp entry of sum.symbols): 42 exact, 21 likely, 1 unsure.** Nothing left.
imin (0x503dc0, module "-") is an out-of-line header inline emitted at the end of this object; it is
mentioned in the file header, not written.

Data: .data 0x5a5760 `Load_mipmaps = true`, then the literals (0x5a5764-0x5a5c20). .bss 0x257f0b0-0x25c6897
in address order (gaps at 0x2591c74 / 0x25c2ffc / 0x25c5c64 / 0x25c5c7c are unreferenced alignment).
Static initialisers 0x5887d4 (ccmd_load_mipmaps), 0x5887d8 (bm_name_string_pool + atexit), 0x5887dc
(ccmd_change_texture_res).

| Address | Function | Confidence |
|---|---|---|
| 0x501b20 | init_ccmd_load_mipmaps_thunk (compiler) | exact |
| 0x501b30 | init_ccmd_load_mipmaps (ccmd definition) | exact |
| 0x501b50 | cmd_load_mipmaps | exact |
| 0x501c10 | init_bm_name_pool (compiler) | exact |
| 0x501c20 | bm_init_name_pool (StringPool definition) | exact |
| 0x501c40 | bm_name_pool_register_dtor (compiler) | exact |
| 0x501c50 | bm_name_pool_dtor (compiler) | exact |
| 0x501c60 | init_ccmd_change_texture_res_thunk (compiler) | exact |
| 0x501c70 | init_ccmd_change_texture_res (ccmd definition) | exact |
| 0x501c90 | cmd_change_texture_res | exact |
| 0x501ce0 | bm_calc_num_pixels | exact |
| 0x501d20 | bm_read_vbm_header | likely |
| 0x501eb0 | bm_set_mark | likely |
| 0x501f20 | bm_release_from_mark | likely |
| 0x502040 | bm_hash_insert | exact |
| 0x502090 | bm_close | exact |
| 0x5020f0 | bm_free_data | likely |
| 0x5021d0 | bm_handle_to_index | exact |
| 0x5021e0 | bm_set_reduce_textures | exact |
| 0x502240 | bm_set_reduced | exact |
| 0x502300 | bm_is_reduced | exact |
| 0x502320 | bm_init | likely |
| 0x5023e0 | bm_free_entry | likely |
| 0x502450 | bm_bytes_per_pixel | likely |
| 0x5024a0 | bm_bits_per_pixel | exact |
| 0x5024c0 | bm_get_total_bytes | likely |
| 0x5024e0 | bm_get_type | exact |
| 0x502500 | bm_get_num_frames | exact |
| 0x502560 | bm_get_current_frame | exact |
| 0x5025d0 | bm_find | exact |
| 0x502650 | bm_hash_name | exact |
| 0x502690 | bm_is_user_bitmap | exact |
| 0x5026e0 | bm_load | exact |
| 0x502720 | bm_load_internal | likely |
| 0x502a00 | bm_read_header | likely |
| 0x502e30 | bm_get_type_from_ext | exact |
| 0x502eb0 | bm_alloc_entry | exact |
| 0x502ee0 | bm_alloc_entries | likely |
| 0x502f70 | bm_prefer_vbm | likely |
| 0x5030a0 | bm_create_missing | exact |
| 0x503110 | bm_load_animation_path | exact |
| 0x503190 | bm_reload | exact |
| 0x5031c0 | bm_replace_file | likely |
| 0x5033c0 | bm_is_valid_size (+ inline bm_round_texture_size, name ours) | likely |
| 0x503480 | bm_load_animation | exact |
| 0x503500 | bm_get_dimensions | exact |
| 0x503550 | bm_get_mipmap_info | exact |
| 0x5035c0 | bm_get_format | exact |
| 0x5035e0 | bm_has_alpha | likely |
| 0x503610 | bm_get_data | exact |
| 0x503650 | bm_lock | likely |
| 0x503710 | bm_lock_tga | likely |
| 0x5038f0 | bm_lock_vbm | likely |
| 0x503aa0 | bm_lock_pcx | exact |
| 0x503b20 | bm_lock_m2v | exact |
| 0x503b50 | bm_unlock | exact |
| 0x503b60 | bm_get_filename | exact |
| 0x503ba0 | bm_get_total_size (2 params, D1102) | likely |
| 0x503c00 | bm_read_pcx_header_of | exact |
| 0x503c80 | bm_release | likely |
| 0x503cf0 | bm_create | likely |
| 0x503d90 | bm_malloc | exact |
| 0x503da0 | bm_set_preload_on_load | exact |
| 0x503db0 | bm_name_pool_get_length (comment only) | unsure |

## Findings (corrections to sum.symbols / docs)

- **bm_is_user_bitmap** is true only for bm_create_missing's placeholders: type 3 entries whose name is
  *not* "USERBMAP" (`test eax, eax; je false` after `_stricmp`). sum.symbols says the opposite.
- **bm_lock** returns the format and outputs data and *palette* (not the format; D1100).
- **bm_get_num_frames**'s float is the length of the whole animation in seconds, not seconds per frame.
- **bm_get_total_size** takes a second argument, a bytes-per-pixel factor (D1102).
- **bm_read_pcx_header_of(bm, palette)**: the second argument is pcx_read_header's palette buffer.
- The mip files are `<base>-mip<n>.tga` (hyphen), not `_mip` as the sum.symbols comment says.
- bm_read_header reads every header from PATH_ANY; path_id only reaches the entry (bm_lock).
- m2v bitmaps get format 1555; VBM format codes are 0 = 1555, 1 = 4444, other = 565.
- bm_load_animation returns the first frame's handle, bm_load_animation_path the header's.
- The m2v readers and bm_create have an unread trailing argument (D1101); bm_load's third is unread too.

## Open questions

- 0x503db0 (`jmp StringPool::GetLength`, ECX from bm_set_mark) sits with imin at the end of the object,
  so it looks like an out-of-line header inline forwarding to GetLength; which one is unknown.
  bm_set_mark calls GetLength directly here, which would change the call target.
- bm_get_total_bytes' failure return is `or eax, eax` (eax already -1), not VC6's usual `or eax, -1`.
- bm_read_header's "Cannot determine" message lands in the base-name buffer's stack slot (written so);
  could be VC6 stack packing of a separate buffer instead.
- tga_read_bitmap's fourth parameter is a bool at bm_lock_tga's calls and tga_read_header's sixth is 1 for
  the mip probes; targautils.h keeps int / void * (passing the bool to an int would give a movzx).
- bm_init's list loop and bm_free_entry: whether VC6 really drops list_init's / list_remove's dead stores
  (written with the vsdk.h macros).
- bm_is_valid_size's rounding: an inline helper, a macro or written out twice.
- progress_set_callback / progress_update (0x503dd0-0x503f5x, .data 0x5a5c24, .bss 0x25c6898) follow
  imin, so they are probably another object; not written here.
- .bss order vs definition order (the console commands and the name pool sit between other globals).

## Game bugs / oddities noticed

- bm_create_missing sets bm_missing_created but never tests it to reuse anything: each missing file name
  allocates its own 32x32 placeholder entry (found by name afterwards, so once per name).
- cmd_change_texture_res has no calling_a_command_function test: `help change_texture_res` or a status
  query toggles the texture resolution.
- bm_alloc_entries returns NULL when no free run is long enough and bm_load_internal writes through it
  (crash on a full table when loading an animation).
- bm_lock never checks for already-loaded data for PCX / TGA / VBM single bitmaps: locking twice leaks the
  first buffer. Animation frames share one block freed after num_frames unlocks; unlocking one frame twice
  frees it while other frames still point into it.
- bm_set_reduced does not update total_bytes.
- bm_release on an animation header frees only the header's entry; the frames stay on the used list (and
  keep type != 0) until bm_release_from_mark or bm_close.
