# decomp22: memory.cpp, stringpool.cpp, plainstring.cpp, parse.cpp

Written from Sum.exe v1.40 only (`node tools/asm.mjs fn|callers`, raw bytes of `Summoner/Sum.exe` for the
literals and the wide-char table; every string literal in the four files was checked to exist in the exe).
Summoner 2 and Dash Faction were not used. All four files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>` with no
output, and all 38 existing `src/decomp/{vsdk,game}/*.cpp` (repo at 9e83548) compile against this
`include/` copy with the same diagnostics as against the repo's own headers (the copy was re-synced with the
upstream header changes of 86d1792 / 9e83548 before checking).

Files: `vsdk/memory.cpp`, `vsdk/stringpool.cpp`, `vsdk/plainstring.cpp`, `vsdk/parse.cpp`; headers in
`include/` (see CHANGED-HEADERS.txt); decisions D200-D207 in `include/DECISIONS.md`.

Total: 106 functions, 76 exact / 29 likely / 1 unsure. Nothing of the four units is left.

## vsdk\os\memory.cpp (0x5124c0-0x5128ab)

| Address | Function | Confidence |
|---|---|---|
| 0x5124c0 | operator new(size, file, line) (operator_new_dbg) | exact |
| 0x512530 | operator new[](size, file, line) (operator_new_array_dbg) | exact |
| 0x512550 | operator new(size) | exact |
| 0x512570 | operator delete | exact |
| 0x512580 | operator delete[] (no caller) | exact |
| 0x512590 | atexit_nop_512590 (empty) | exact |
| 0x5125a0 | mem_report_open | exact |
| 0x5125e0 | mem_report_close | exact |
| 0x512600 | mem_report_indent | exact |
| 0x512610 | mem_report_unindent | exact |
| 0x512630 | mem_report_is_verbose | exact |
| 0x512640 | mem_report_line | likely |
| 0x512770 | Mempool::Copy | exact |
| 0x5127c0 | Mempool::allocate_in_mempool | exact |
| 0x5127f0 | set_current_mempool | exact |
| 0x512800 | get_current_mempool | exact |
| 0x512810 | is_mempool_active | exact |
| 0x512820 | allocate_in_commonblock | exact |
| 0x512880 | deallocate_from_commonblock | exact |

19 functions: 18 exact, 1 likely. No static initialiser (0x5128b0.. is vfile's StringPool initialiser).
.bss defined: level counters 0x2693004-0x2693020 and mem_bytes_allocated 0x2693034 (D202, placement
"likely"), mem_report_file / depth / verbose 0x269303c-44, Pool 0x2693048, Common_block_index 0x2693050,
Common_block_allocations[2] 0x2693054. Unreferenced slots: 0x2693024-30, 0x2693038, 0x269304c.
The two `= ""` initialisers of mem_report_line read zero bytes 0x269305c / 0x2693060: the unit is not
string-pooled (three copies of the path), and the "" literals of these units sit in .bss (plainstring
0x26b6390-9c, parse 0x26b63d4-e0; the pooled "" of the game code is 0x5bd0d0 in the zero tail of .data).

## vsdk\os\stringpool.cpp (0x544b80-0x544e3e)

| Address | Function | Confidence |
|---|---|---|
| 0x544b80 | StringPool::Init | likely (store order) |
| 0x544ba0 | StringPool::Allocate | exact |
| 0x544bd0 | StringPool::Reset | exact |
| 0x544be0 | StringPool::Release | exact |
| 0x544c10 | StringPool::GetLength | exact |
| 0x544c20 | StringPool::GetRemaining | exact |
| 0x544c30 | StringPool::GetBuffer | exact |
| 0x544c40 | StringPool::Add | exact |
| 0x544cd0 | StringPool::AddUnique | exact |
| 0x544d40 | StringPool::AddQuoted | exact |
| 0x544d80 | StringPool::AddUniqueQuoted | exact |
| 0x544dc0 | StringPool::StringPool() | exact |
| 0x544de0 | StringPool::StringPool(pool, max, allocated) | exact |
| 0x544e00 | StringPool::StringPool(max) | exact |
| 0x544e20 | StringPool::~StringPool | exact |
| 0x544e30 | StringPool::SetLength | likely (unsigned compare) |

16 functions: 14 exact, 2 likely. No data besides the two literals.

## plainstring.cpp (0x514ec0-0x515955; written as vsdk/plainstring.cpp, original folder unknown, guessed vsdk\os)

| Address | Function | Confidence |
|---|---|---|
| 0x514ec0 | is_blank | likely |
| 0x514f00 | trim_leading_whitespace | exact |
| 0x514f60 | trim_trailing_whitespace | exact |
| 0x514fb0 | trim_whitespace | exact |
| 0x515030 | format_indented_column | likely |
| 0x515170 | format_indented_column_b | likely |
| 0x5152a0 | int_to_string_with_commas | likely |
| 0x5153b0 | gr_string_truncate_ellipsis | likely |
| 0x5154d0 | strcat_char | exact |
| 0x515500 | stristr | exact |
| 0x5155e0 | strlwr | exact |
| 0x515620 | strupr | exact |
| 0x515660 | int_to_string | exact |
| 0x5156b0 | int_to_hex_string | exact |
| 0x515700 | float_to_string | exact |
| 0x515760 | byte_count_to_string | likely |
| 0x515800 | vector3_to_string_buf | exact |
| 0x515880 | ptr_to_hex_string | exact |
| 0x5158d0 | string_to_wide_charmap | likely |
| 0x515910 | wide_charmap_to_string | likely |

20 functions: 12 exact, 8 likely. .data: wide_charmap_table[86] 0x5a783c (Shift-JIS full-width forms of
0x20..0x75) and the literals to 0x5a794b. Not string-pooled ("%d", "0x%x" and " " repeat).

## vsdk\parse\parse.cpp (0x515960-0x5172eb)

| Address | Function | Confidence |
|---|---|---|
| 0x515960 | TextFile::TextFile() | exact |
| 0x515970 | TextFile::TextFile(name, path_id) | exact |
| 0x5159d0 | TextFile::~TextFile | exact |
| 0x5159e0 | SkipBlockComment | exact |
| 0x515a20 | SkipSpaces | likely |
| 0x515a80 | SkipWhitespace | likely |
| 0x515af0 | SkipToNextLine | exact |
| 0x515b30 | SkipToWrap | exact |
| 0x515ba0 | SkipTo | exact |
| 0x515c20 | SkipPastWrap | exact |
| 0x515c50 | SkipPast | exact |
| 0x515c80 | CopyUntil | exact |
| 0x515ce0 | GetPosition | exact |
| 0x515d00 | SetPosition | exact |
| 0x515d20 | Open | likely |
| 0x515e90 | Close | exact |
| 0x515ec0 | Rewind | exact |
| 0x515ee0 | Error | likely |
| 0x515f90 | SkipLine (String version of ReadLine) | exact |
| 0x516010 | ReadLine | likely |
| 0x516090 | SkipWhitespaceAndLine | exact |
| 0x5160b0 | ReadNextLine | exact |
| 0x5160d0 | Peek | exact |
| 0x516110 | Accept | exact |
| 0x516150 | AcceptQuoted | likely |
| 0x5161e0 | Expect | exact |
| 0x516230 | ParseBool | exact |
| 0x516290 | ParseInt | likely |
| 0x516380 | ParseUInt | likely |
| 0x516460 | ParseFloat | likely |
| 0x516510 | ParseVector | exact |
| 0x5165a0 | ParseMatrix | exact |
| 0x516630 | ParseDelimitedString | exact |
| 0x5166b0 | ParseString | likely |
| 0x516780 | ParseMultiline | likely |
| 0x516830 | ParseMultilineJoined | exact |
| 0x5168b0 | ParseStringList | exact |
| 0x516940 | ParseIntList | likely |
| 0x516a30 | ParseFloatList | exact |
| 0x516ab0 | ParseFlags | likely |
| 0x516bd0 | ParseTypeString | exact |
| 0x516c70 | Checksum | likely |
| 0x516ca0 | bool_to_String | exact |
| 0x516cd0 | vector3_to_string | likely |
| 0x516e80 | matrix3_to_string | likely |
| 0x517000 | bool_to_cstr | exact |
| 0x517060 | vector3_to_cstr | exact |
| 0x5170d0 | matrix3_to_cstr | likely |
| 0x517180 | file_write_text_block | unsure |
| 0x517210 | TextFile::AtEnd | exact |
| 0x517220 | file_blast_into_buffer (sum.symbols module vsdk\vfile; moved here, D206) | likely |

51 functions: 32 exact, 18 likely, 1 unsure. .data 0x5a7950 file_blast_size = -1 (name ours) then the
literals; .bss file_blast_buffer 0x26b63a0, file_blast_filename[0x30] 0x26b63a4, "" literals.

## Open questions

- VC6 and `operator new[]` / `operator delete[]` overloads: Sum.exe has them (0x512530, 0x512580), so the
  compiler supported them; whether other units' `new T[n]` would then call 0x512530 if it were declared in
  a header is why it is not (D200). `throw()` on the deletes is for the checker only.
- Crc32: why TextFile::Checksum calls the inline constructor out of line (0x514eb0) when name_hash_nocase
  expands it. Either parse.cpp is built /Ob0 or the constructor is not inline (D207).
- Stack layout of TextFile::Error (found / prior / msg order), ParseIntList and ParseFlags: VC6's local
  allocation order is not modelled; the code shape is otherwise fixed.
- file_write_text_block: the `inc esi / jns` after the backward search does not match a plain
  `if (i >= 0)`; the loop is written semantically.
- file_blast_filename size (0x30 here, 0x40 in sum.symbols) and the unreferenced .bss byte 0x26b63d8.
- matrix3_to_cstr's `and esp, -8` frame: not explained by the source (doubles are only pushed).
- The level counters 0x2693004-0x2693020 (D202): six of them are only ever zeroed; their names and whether
  they belong to memory.cpp or a neighbour are guesses.
- byte_count_to_string: whether the source divides by 1024.0f or multiplies by its reciprocal.
- sum.symbols comment fixes to make: format_indented_column (0x515030) arguments are (dest, indent, width,
  label, fmt, ...); vector3_to_cstr / matrix3_to_cstr are (dest, size, v / m, precision);
  packfile_find_in_packfile is (vpp, name, &size, &offset); file_blast_into_buffer's module is
  vsdk\parse\parse.cpp; TextFile__SkipLine is a String ReadLine; SkipToNextLine does not step past CR LF.

## Game bugs noticed (latent)

- int_to_string_with_commas: a negative number with 3k digits gets a comma after the sign ("-,123,456")
  and the size check counts one comma too few. Callers pass experience / values (non-negative).
- TextFile::SkipToNextLine: tests the CR itself for LF, so the cursor stays on the CR; the line is counted
  twice (here and by the next SkipWhitespace). Caller localize_load_level_table (twice).
- TextFile::ParseInt / ParseUInt: strchr("0123..ABCDEF", c) also matches the terminator, so a hex number at
  the very end of a file reads past the buffer.
- TextFile::Error copies 40 bytes after the cursor regardless of the terminator (reads past the buffer
  end near the end of a file).
- TextFile::ParseMultiline: the trailing-whitespace trim has no lower bound (empty text before the
  terminator walks back before the start, negative length).
- TextFile::ParseStringList / ParseIntList / ParseFloatList ignore max_count (no overflow check).
- TextFile::ParseFlags: a NULL entry in the name table ends the search without error, so unknown flag
  names are ignored silently in NULL-terminated tables.
- TextFile::AcceptQuoted: Accept skips spaces after the opening quote but the advance is strlen + 2.
- wide_charmap_to_string: a char not in the table leaves its byte of dest unwritten.
- stristr: 512-byte buffers, no length check.
- allocate_in_commonblock: Common_block_allocations has 2 slots; a third nested TextFile::Open writes past
  it (no check on the index, only on the slot).
