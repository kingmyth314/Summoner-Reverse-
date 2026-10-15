# Pilot notes: new_credits.cpp and new_help.cpp

Source written from the Sum.exe v1.40 disassembly (`tools/asm.mjs fn`, raw data bytes read from
`Summoner/Sum.exe`). The Summoner 2 decomp and Dash Faction were not used for code. The only
check is `i686-w64-mingw32-g++ -fsyntax-only -Wall`: both files pass, with only the expected
"string literal to char *" warnings, which VC6 does not give.

## new_credits.cpp (0x408590-0x408d72)

| Address | Function | Confidence |
|---|---|---|
| 0x408590 | init_realtimetimer_5bc408_thunk (compiler-generated, from `RealtimeTimer credits_timer;`) | exact |
| 0x4085a0 | init_realtimetimer_5bc408 (compiler-generated) | exact |
| 0x4085b0 | credits_init | exact |
| 0x408620 | credits_parse_table | likely |
| 0x408970 | credits_exit | exact |
| 0x408980 | credits_get_line_size | exact |
| 0x4089c0 | credits_leave | exact |
| 0x4089f0 | credits_frame | likely |
| 0x408a90 | credits_finish | exact |
| 0x408ac0 | credits_draw | likely |
| 0x408ba0 | credits_draw_lines (ends at 0x408d72, past the "about 0x408ba0" range) | likely |

Data placed in this unit, with the names I gave to unnamed addresses (*new*):
- .rdata 0x57eb34-0x57eb7b: six `const int[3]` per-resolution tables (*new*: `credits_text_width`,
  `credits_column_x`, `credits_fade_top_start/end`, `credits_fade_bottom_start/end`); then
  the float constants 10.0, 255.0 and 0.0 (`__real@` COMDATs; other units use them too).
- .data 0x58a0e8 `credits_bkgrnd_names[2]` (*new*), 0x58a0f0 `float credits_scroll_speed = 20` (*new*),
  then the string literals.
- .bss 0x5bc3f8-0x5bc413: `credits_lines`, `credits_left_align` (0x5bc3fc, *new*),
  `credits_saved_mempool_offset` (0x5bc400, *new*: `level_mempool.offset` saved in init and restored in
  leave, which frees the 0x2000-byte line buffer), `credits_line_count`, `credits_timer`,
  `credits_bkgrnd_bm`, `credits_scroll_y`.

credits.tbl format (from the code): section `#<language>` in lower case, one line per row until `#end`.
Line prefixes: `-` ends `@` mode, `!` left-aligns this line, `@` left-aligns this and the following lines,
then `&` puts it in the right column of the previous row and `^` in the right column of this row, then `*`
makes it a heading (highlight colour, half a line extra). Lines wider than the column are split by
`gr_split_str` (at most 16 pieces).

## new_help.cpp (0x40bf30-0x40cb55)

| Address | Function | Confidence |
|---|---|---|
| 0x40bf30 / 0x40bf40 | init_strptrs_5bc928 thunk + initialiser (`help_mouse_lines`, 8 pairs) | likely |
| 0x40bff0 / 0x40c000 | init_strptrs_5bc968 (`help_camera_lines`, 3 pairs) | likely |
| 0x40c040 / 0x40c050 | init_strptrs_5bc980 (`help_keyboard1_lines`, 13 pairs) | likely |
| 0x40c160 / 0x40c170 | init_strptrs_5bc9e8 (`help_keyboard2_lines`, 10 pairs) | likely |
| 0x40c240 / 0x40c250 | init_strptrs_5bca38 (`help_automap_lines`, 4 pairs) | likely |
| 0x40c2b0 | help_draw_page_mouse | likely |
| 0x40c4a0 | help_draw_page_keyboard1 | likely |
| 0x40c5b0 | help_draw_page_keyboard2 | likely |
| 0x40c6c0 | help_draw_page_automap | likely |
| 0x40c7d0 | help_reset_page | exact |
| 0x40c7e0 | help_init | exact |
| 0x40c850 | help_leave | exact |
| 0x40c860 | help_frame | likely |
| 0x40ca10 | help_prev_page | exact |
| 0x40ca30 | help_next_page | exact |
| 0x40ca50 | help_draw | likely |

Data: .rdata 0x57edd0 `help_origin[3]` {x, y} and 0x57ede8 `help_size[3]` {w, h} (*new*; only help code
reads 0x57edd0-0x57edff); .data 0x58ac48 `help_bkgrnd_names[2]` (*new*), 0x58ac50
`help_page_draw_table[4]`, strings; .bss 0x5bc91c-0x5bc927 three ints, then the five tables
(0x5bc928-0x5bca57).

## Open questions

1. **Why the help tables are built at run time.** No table in the whole of .data stores the address of a
   `localize_strings` slot, and 87 `init_strptrs_*` initialisers exist. I wrote the tables as aggregates of
   `char *&` reference members bound to `localize_strings[id]` (via `XSTR`), which explains both the dynamic
   initialisation and the `localize_string_or_invalid(char **)` calls. A plain `char **tbl[] =
   { &localize_strings[835], ... }` should be a constant initialiser, so VC6 would have put it in .data;
   test both once the compiler exists.
2. **The `&` test in credits_parse_table** reads `credits_lines[start]` (the output buffer indexed by the input
   offset). Reproduced as is; it is probably a bug for `line[start]`.
3. **credits_draw_lines alpha clamp:** written as one nested ternary after an if / else-if. The middle case jumps
   past the `> 0` test, which I read as VC6 jump threading on the constant 1.0. If VC6 does not do that, the
   source had the `> 0` test inside the two fade branches instead.
4. **Global order:** .bss and .rdata placement follows definition order in my files. That VC6 keeps .bss in
   definition order is an assumption (the observed addresses go up in the order the functions use them).
5. **Locals:** the stack layouts of credits_parse_table (w 0x1c, pos 0x20, h 0x28, TextFile 0x2c, section 0x7c,
   starts 0xbc, line 0xfc, lengths 0x2fc) and help_draw (page text 0x10, text 0x30) depend on declaration
   order and VC6's allocator. The declaration order is a guess.
6. **credits_frame switch:** `case 0:` is there because the code starts with `sub eax, 0`. It is merged with
   `default` (both give 1.0); two separate bodies would probably tail-merge to the same code.
7. **Signatures guessed from the calls:** `TextFile::Open(name, 9999999)` (nonzero = failure), `bm_load(name, -1, -1)`,
   `game_play_sound(handle, 1.0f, 0, 0, 0)`, `game_pause(0, 0)`, `help_init(bool)` (interface_set_screen passes
   `prev_screen != 14`). `new char[0x2000]` is assumed to reach the game's `operator_new` (0x512550).
8. Key codes in help_frame are DirectInput scan codes; ESC closes the screen and goes back to screen 14 (main menu)
   when help was opened from there, else to 0 (HUD).
