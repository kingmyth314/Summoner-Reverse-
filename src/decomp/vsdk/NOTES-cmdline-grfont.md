# cmdline.cpp and gr_font.cpp (source half)

Written from Sum.exe v1.40 only (`tools/asm.mjs fn`, `build/sum.asm`, raw bytes of `Summoner/Sum.exe`
for the string literals). Dash Faction headers were read for names and argument lists only; Summoner 2
was not used. Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -I . -I <repo>/symbols` with no output.

Files: `cmdline.h/.cpp`, `gr_font.h/.cpp`, `engine_stubs.h` (DebugError + `VSDK_ERROR` loop macro,
os_registry_read_string, File, String, bm_create, gr_bitmap_ex, gr_gamma_ramp, localize_french_punct).

## vsdk\os\cmdline.cpp (0x53dec0-0x53e514)

| Address | Function | Confidence |
|---|---|---|
| 0x53dec0 | init_cmdline_params (compiler-generated from `static CmdLineParam cmdline_params(NULL, NULL, true)`) | exact |
| 0x53ded0 | cmdline_params_ctor (generated) | exact |
| 0x53def0 | cmdline_params_register_dtor (generated, atexit) | exact |
| 0x53df00 | cmdline_params_dtor (generated atexit thunk, jmp ~CmdLineParam) | exact |
| 0x53df10 | cmdline_init(check_unknown, unused) | likely |
| 0x53dfd0 | cmdline_free | exact |
| 0x53e030 | cmdline_parse(cmd_line, check_unknown, skip_first) | exact |
| 0x53e060 | cmdline_split(cmd_line, skip_first) | likely |
| 0x53e1e0 | cmdline_match_params | exact |
| 0x53e250 | cmdline_take_argument(param, index) | likely |
| 0x53e310 | cmdline_check_unknown(cmd_line) | likely |
| 0x53e3c0 | cmdline_reparse | exact |
| 0x53e3e0 | CmdLineParam::CmdLineParam | likely |
| 0x53e450 | CmdLineParam::~CmdLineParam | exact |
| 0x53e480 | CmdLineParam::free_arg | exact |
| 0x53e4a0 | CmdLineParam::found | exact |
| 0x53e4b0 | CmdLineParam::get_arg_int | exact |
| 0x53e4c0 | CmdLineParam::get_arg_float | likely |
| 0x53e4d0 | CmdLineParam::get_arg | exact |
| 0x53e4e0 | cmdline_get_unclaimed_arg | exact |

20 functions (17 in the module column + the 3 initialiser functions before 0x53df00): 14 exact, 6 likely.
The other static `CmdLineParam` objects are built by initialisers in their own files and are not here.

.bss defined here: `cmdline_args[50]` 0x2dc6ea8, `cmdline_params` 0x2dc7038 (static), `cmdline_params_initialized`
0x2dc7050 (static), `cmdline_num_args` 0x2dc7054, and *new* `cmdline_no_options[4]` 0x2dc7058.
Literals (.data 0x5abf08-0x5abfdf): " ", "CmdLineOptions", " \t" x3 plus " " (separate copies), the two
messages, the source path.

## vsdk\gr\gr_font.cpp (0x5081d0-0x50939a)

| Address | Function | Confidence |
|---|---|---|
| 0x5081d0 | gr_close_fonts | exact |
| 0x508290 | gr_font_is_whitespace | exact |
| 0x5082b0 | gr_font_is_space | exact |
| 0x5082d0 | gr_font_french_punct | exact |
| 0x508300 | gr_get_font_height | exact |
| 0x508330 | gr_get_font_height2 | exact |
| 0x508360 | gr_get_string_size | likely |
| 0x508440 | gr_font_get_char | likely |
| 0x508510 | gr_load_font | exact |
| 0x508560 | gr_font_create | likely |
| 0x508970 | gr_font_read_file | likely |
| 0x508c60 | gr_string | likely |
| 0x508d70 | gr_font_center_x | exact |
| 0x508dc0 | gr_string_at_text_pos | exact |
| 0x508df0 | gr_fit_string_String | likely |
| 0x509030 | gr_font_fit_string | likely |
| 0x509130 | gr_split_str | unsure |

17 functions: 9 exact, 7 likely, 1 unsure. No static initialisers in this range.

.bss defined here: `gr_fonts[6]` 0x25d9238, `gr_num_fonts`, `gr_current_font`, `gr_default_font`,
`gr_text_pos_x/y`, and *new* `gr_font_empty_line[4]` 0x25d94d4, `gr_font_empty_rest[4]` 0x25d94d8.
Error lines: 676 ("Too many fonts!"), 731 ("Font too big!"), 532 (newer font version); each uses its
own copy of the path literal.

## Facts used

- Every DebugError call site loops back to re-push its arguments: the vsdk error macro is an endless
  loop around the call (`VSDK_ERROR`). Real macro name unknown.
- Font selection: `gr_get_font_height`, `_height2` and `gr_get_string_size` compute the pointer in one
  register and store it once (conditional expression); `gr_string` stores in each branch (if/else).
- `gr_font_is_*` return int (MOV EAX,1; callers TEST EAX).
- `gr_string` compares the character with JAE 0x20, so it is unsigned char there.
- `gr_font_create`: the 16-bit size is halved with SHR (unsigned), the 4-bit gamma index is an
  unsigned divide by 14 (magic 0x24924925), the alpha clamp is JBE (unsigned).
- `gr_split_str` compares i with strlen signed (JL), so `(int)strlen(s)`.

## Behaviour worth documenting (observed)

1. `cmdline_split` has no 50-argument limit check (cmdline_args overflows past 50 arguments).
2. `cmdline_take_argument`: a parameter followed by an option gets no argument; the message is formatted
   and thrown away.
3. `gr_font_create` finds the first free slot but never uses it: freed slots are not reused.
4. `gr_string` draws an out-of-font character with chars[-1] / coords[-1] (gr_font_get_char returns -1).
5. `gr_font_get_char` only compares the second character after the kern search loop.
6. `gr_fit_string_String` returns "" (and leaves str alone) when the whole string fits.

## Open questions

1. **Empty strings in .bss** (0x2dc7058, 0x25d94d4, 0x25d94d8): passed as `const char*` "" but they
   are zero-fill addresses, so not literals. Zero-initialised `static char x[N]` is my guess; the
   size (4) is only the gap to the next symbol.
2. **cmdline_init's second argument** is pushed by the caller but never read; its meaning is unknown.
3. **gr_font_create switches**: case order and whether they were switches or if chains (see the
   comment). The `bm_w = bm_h = N` pair of variables is inferred from `mov eax,N; mov ecx,eax`.
4. **8-bit palette packing**: the nibble order (byte 3 -> bits 12-15, byte 0 -> 8-11, byte 1 -> 4-7,
   byte 2 -> 0-3) is exact; the source form (Horner with 16-bit pieces) is a guess.
5. **gr_split_str**: the shared emit block for '\n' and overflow (local `end` vs duplicated code
   or goto); the source likely used `offset_array[num_lines]` indexing (the binary strength-reduces
   it to pointers inside the loop, but indexes after it).
6. **CmdLineParam constructor store order** and the list-append form (local tail).
7. **get_arg_float** returns float or double (no difference in the code).
8. `gr_bitmap_ex` / `gr_string` mode is a 4-byte `gr::Mode` by value; written as int.
9. `.bss` order follows addresses; the frametime pilot's open question about VC6 .bss ordering applies.
