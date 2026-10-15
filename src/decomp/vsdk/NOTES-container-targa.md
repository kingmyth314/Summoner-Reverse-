# ui_container.cpp and targautils.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and raw reads of Sum.exe's .data only. No Summoner 2 or
Dash Faction code was read; names are from symbols/sum.symbols. Syntax-checked with
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings` (clean for both files).

Files: `ui_container.cpp/.h`, `targautils.cpp/.h`, `stubs_container_targa.h` (only the declarations no
compatible existing header has; see its top comment for what it cannot be combined with).

## ui_container.cpp (0x5534f0-0x553b9b)

| Address | Function | Confidence |
|---|---|---|
| 0x5534f0 | ui_container::process (slot 5) | exact |
| 0x553530 | ui_container::dtor (slot 0) | exact |
| 0x553560 | ui_container::ui_container | likely |
| 0x553590 | ui_container::create | likely |
| 0x5535f0 | ui_container::set_tooltip | likely |
| 0x553680 | ui_container::set_label | exact |
| 0x5536b0 | ui_container::load_mask_bitmap | exact |
| 0x553710 | ui_container::set_mask_bitmap | likely |
| 0x5537a0 | ui_container::get_mask_pixel (slot 6) | exact |
| 0x553800 | ui_container::draw (slot 7) | likely |

10 functions: 5 exact, 5 likely, 0 unsure. No static initialisers or thunks in the range; the unit has
no .data/.bss variables (strings 0x5ad9e0-0x5ada4f only).

Class layout and vtable: `ui_element` (vtable 0x581ea0, 5 slots, slot 1 `_purecall`) -> `ui_gadget`
(0x581e08, 9 slots) -> `ui_container` (0x581f40, 0x388 bytes). ui_container overrides slots 0, 5, 6, 7
and inherits 1-4 and 8 from ui_gadget, which with the declarations in the headers gives the vtable
0x553530 0x52c330 0x52c890 0x52c8c0 0x52c530 0x5534f0 0x5537a0 0x553800 0x52c880 as in the binary.

Facts that shaped the source:
- Slot 0 is not a C++ destructor (no flags argument, no delete). The real destructors ~ui_element
  (0x542d40) and ~ui_gadget (0x52c410) are non-virtual; the atexit handlers of the five static
  ui_containers (script editor) jump to ~ui_gadget, so ui_container has no user-declared destructor.
- ui_gadget's slot 5 is 0x52c390 (`ui_gadget__process` (formerly `ui_gadget__process_children`)) and slot 7 is 0x52c840
  (`ui_gadget__draw` (formerly `ui_gadget__draw_children`)): these are the base-class `process`/`draw`, and the container calls them
  as `ui_gadget::process(key)` / `ui_gadget::draw()`. Likewise `ui_gadget::dtor()` -> 0x52c3d0, which
  tail-jumps to `ui_element::dtor()` 0x542cd0 (`ui_element__free_tooltips`).
- gr_line takes float coordinates (x1, y1, x2, y2, mode); all frame arithmetic is float.

## targautils.cpp (0x5549c0-0x5565d6)

| Address | Function | Confidence |
|---|---|---|
| 0x5549c0 | m2v_skip_extension_and_user_data | likely |
| 0x554ac0 | m2v_skip_extra_bit_information | exact |
| 0x554af0 | m2v_read_slice_quantizer_scale | exact |
| 0x554b10 | m2v_skip_macroblock_stuffing | likely |
| 0x554b60 | m2v_read_header | likely |
| 0x554d00 | m2v_read_bitmap | exact |
| 0x554ea0 | tga_rle_encode_row | likely |
| 0x555020 | tga_convert_pixels | exact |
| 0x5550d0 | bytes_equal | exact |
| 0x555110 | tga_read_header | likely |
| 0x555330 | tga_rle_decode | likely |
| 0x555410 | copy_pixels_advance | exact |
| 0x555450 | tga_read_bitmap | likely |
| 0x555970 | tga_write_rle | likely |
| 0x555b50 | tga_write | likely |
| 0x555cf0 | pcx_read_header | exact |
| 0x555f60 | pcx_read_bitmap | likely |
| 0x5562a0 | pcx_write | likely |
| 0x556470 | pcx_encode_line | likely |
| 0x556550 | pcx_encode_byte | exact |
| 0x5565a0 | bm_image_error_string | likely |

21 functions: 8 exact, 13 likely, 0 unsure. No static initialisers. 0x5565e0 (jpeg_write_file) starts
the next unit ("wb" at 0x5adb94 is its literal).

Data placed by its position in .data (VC6 emits literals and initialised globals in source order):
`tga_last_path_id = -1` (0x5ada64) sits between m2v_read_bitmap's ".m2v" and tga_read_header's ".tga";
the error table (0x5adad8, one literal with embedded NULs, 157 bytes) between tga_write's ".tga" and
pcx_read_header's ".pcx". .bss: only 0x2f53aec (`m2v_quantizer_scale`), just before gr_clip_temp_verts.

Facts that shaped the source:
- There is no MPEG-2 decoder in this unit. The four bit-reader helpers have no callers and no pointer to
  them exists anywhere in the image; m2v_read_bitmap only loads the file (bm_lock_m2v decodes it). The
  bit reader is an unknown class with get_bits/show_bits/next_start_code at vtable +0x14/+0x18/+0x1c
  (`m2v_bitstream`, placeholder).
- m2v_read_header: the size word is byte-swapped by a half swap then a byte swap per half; this source
  form reproduces the register use (cl reused for b[0], ah/al for b[2]/b[3]).
- TGA headers are read field by field into a naturally aligned 0x14-byte struct (field offsets on the
  stack match); unused fields are dead stores the compiler dropped. Header bytes are unsigned
  (`and 0xff` + `shr`, `jbe` for `id_length > 0`); sizes are signed shorts (`movsx`).
- tga_read_bitmap's row loops strength-reduce to a top pointer (+row) and a bottom pointer (-row)
  selected per row by descriptor bit 0x20, as in the binary.
- pcx_read_bitmap pushes EBP (count, still 0) for the File::read zero arguments, which places
  `count = 0` at the top of the function.

## Open questions

1. **ui_container::create's eighth argument** (1 from ui_parse_container, 0 from script_editor_create_ui)
   is never read; ui_gadget::init receives the member `owned` (+0x248) instead. Written as a parameter
   `allocated` plus the member, i.e. the original probably had a name clash. Other widget create
   functions should be compared.
2. **Two one-point gr_line calls** in ui_container::draw: (x+7, y)-(x+7, y) in colour 0 and
   (x+w+11, y+1)-(x+w+11, y+1) in colour 1. They are in the binary exactly so (same registers for both
   ends); probably intended as the corner pixels of the label gap.
3. **tga_convert_pixels 16 -> 16** toggles bit 15 of the source pixels in place and writes nothing to
   dst, while tga_rle_encode_row still advances dst by count*2 (a bug in the original; gr_screenshot
   only uses 3-byte output). tga_rle_encode_row also compares dst_bpp bytes of pixels that are src_bpp
   apart.
4. **tga_read_bitmap** seeks over the image id with origin 0 (absolute offset id_length) instead of past
   the 18-byte header; grey images (3, 11) pass the checks but are not decoded.
5. **Packet header stores** in tga_rle_encode_row (`*out = 0xff` / `(count - 1) | 0x80`) could have been
   `*out |= 0x80`; the raw-packet header uses min_run where it equals dup. Compiler test needed.
6. **Names ours**: m2v_quantizer_scale, tga_last_path_id, bm_image_error_strings (static char array vs a
   plain literal pointer: the code loads the address as an immediate, so either fits; an array keeps it
   in its .data slot), bm_lock_format_unused (0x25c2ff8, bmpman.cpp's .bss, first dword after
   bm_bitmaps), m2v_bitstream, ui_tooltip_head. tga_read_header's arguments 5 and 6 (always 0) and
   tga_read_bitmap's argument 4 (0) are unused; tga_write/tga_write_rle's argument 3 has no caller.
7. **ui_tooltip field meanings**: +0x10c (`delay`, from the ui_button__set_tooltip symbol comment) and
   +0x11c (font, -1 in the button version) are not confirmed.

## Wrong or incomplete existing declarations (not changed)

- `symbols/sum.symbols` copy_pixels_advance: "(n, size, &dst, &src)" - the binary is
  (n, &dst, &src, size) (size is read from [esp+0x10] and multiplied with n).
- `symbols/sum.symbols` ui_container__create: "mask bitmap +0x378 = -1" - create sets +0x380 (the label
  font) to -1; +0x378 is set to -1 by the constructor. It also stores flags +0x374 and the colour set
  +0x384.
- `symbols/sum.symbols` ui_gadget__process (formerly ui_gadget__process_children) / ui_gadget__draw (formerly ui_gadget__draw_children) are ui_gadget's slot 5
  and slot 7 (the base-class process/draw); ui_gadget__destroy (formerly ui_gadget__dtor) / ui_container__destroy (formerly ui_container__dtor) are slot 0 functions,
  not C++ destructors (the destructor is ui_gadget__dtor_base 0x52c410).
- `src/decomp/include/stubs_sound_items_registry.h` File: only find(); it lacks open/close/size/seek/
  tell/error/read/read_int8/read_uint8/read_int16/write/write_uint8/write_uint16 and the public path id at
  +0 (tga_read_header/tga_read_bitmap copy it to tga_last_path_id). stubs_container_targa.h therefore
  defines its own File; the two must be merged into one header.
- `symbols/types.h` ui_element is a C struct while the game uses it as the C++ base class of ui_window
  and ui_gadget; a header including types.h cannot also declare the classes. Its fields +0x248..+0x270
  (owned, hotkeys, gadget_type, window, lists) belong to ui_gadget (a window keeps other data there).
- bm_load is declared as (name, a, b) in stubs_credits_help.h/stubs_assess_pickup.h and
  (name, path_id, unk) in stubs_menus_dialogue.h; bm_get_dimensions is in three headers. DebugError
  had to be repeated in stubs_container_targa.h because stubs_sound_items_registry.h and
  stubs_credits_help.h both define TextFile and cannot be included together.
- docs/sum-exe/vsdk-ui.md "Still to do: ui_container.cpp (0x5536ef)" - 0x5536ef is only the DebugError
  push in load_mask_bitmap; the unit is 0x5534f0-0x553b9b.
