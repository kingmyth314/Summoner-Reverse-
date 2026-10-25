---
title: Modern C++ port (research)
---

# Rewriting the code to modern standards (research, 2026-10-23)

**Summary:**
- The decompiled source is much closer to modern C++ than its age suggests. Checked with a current compiler (clang,
  C++17, 32-bit Windows target), **257 of the 259 units already parse**, with 11 errors in all (**Observed**).
- The work is not in the syntax. It is in four things:
  - the matching devices that exist only for VC6;
  - the platform APIs of 2000 (DirectDraw / Direct3D 7, Glide, DirectSound with EAX, Bink, DirectPlay-era networking);
  - x87 floating-point behaviour;
  - for 64-bit, the 32-bit struct layouts.
- The byte-match proof and the modern source **cannot be the same text**. So the `shipped` configuration stays as the
  reference, and a modern build is checked by behaviour, as the fixed build is.

## The measurements

Every `game/` and `vsdk/` unit (176,000 lines) was checked with clang 23.1 (llvm-mingw), syntax only, with
`-std=c++17 -fms-extensions -fpermissive` (**Observed**, 2026-10-23).

**32-bit (`i686-w64-mingw32`):** 257 of 259 units clean.
- 9 errors in `game/unnamed_decals_0x4bbe80.cpp`: taking the address of a temporary. This is the `&(a+b)` matching
  device, D15902.
- 2 errors in `vsdk/gr_font.cpp`: a case value of -16 narrowed to an unsigned type.

**Warnings with `-Wall -Wextra`** (the noisy unused-variable classes switched off):

| Warning | Count | Meaning |
|---|---|---|
| `-Wwritable-strings` | 570 | string literals passed as `char *` (C++98 allowed it) |
| `-Wformat-security` | 55 | `printf(buf)` without a format |
| `-Wnontrivial-memcall` | 14 | `memset` / `memcpy` on classes |
| `-Wdelete-non-abstract-non-virtual-dtor` | 11 | deleting through a base without a virtual destructor |
| `-Wformat` | 9 | format and argument types differ |
| `-Wsometimes-uninitialized` | 4 | a local read before it is set on some path: candidates for original-game bugs |
| `-Warray-bounds` | 4 | constant indexes past an array |
| `-Wreturn-type` | 2 | functions that fall off their end: D15400 and D21001, the documented matching aids |
| others | about 30 | parentheses, self-assignment (`gain = gain;`, a matching aid), bit-field constants |

**64-bit (`x86_64-w64-mingw32`):** 244 units fail. All but about 25 of the 14,600 errors are the source's own
`LAYOUT_CHECK` tests (`include/vsdk/vsdk.h`), which check every struct's size and field offsets against the binary.
209 different checks fail, because pointers become 8 bytes. That is the map of what 64-bit has to touch.

**Platform code:** `__asm` blocks in 4 units (`timer.cpp`, the exception handler, `floatmath`, the os unit), and calls
to DirectDraw (28), Direct3D (about 170), DirectSound (16), EAX (92), Bink (4) and Glide.

## Why the reference stays

Each device on [Matching devices](../matching-devices.md) exists because VC6 build 8966 needs it to produce the
original bytes. Examples: unused locals, `volatile` reads, names chosen for a hash, a function without a `return`.
A modern compiler gives different code whatever the source looks like. So the byte-match proof only ever applies to
the `shipped` configuration with VC6.

A modern build is checked the way the fixed build is ([Fixed and experimental builds](../builds.md#testing)):
- the save, memory and screen comparisons against the original;
- the scenario runs.

That keeps one tree: a `MODERN_BUILD` define selects plain C++ where a device stands. Every later decompilation fix
then reaches all builds at once. A separate fork would have to be merged by hand forever.

## The ladder

Each rung is usable on its own, and each has its own test.

**L1: builds and runs with clang, 32-bit, same tree.**
- Fix the 11 errors.
- Add the two missing returns.
- Replace the 4 `__asm` blocks with intrinsics or C.
- Link the DirectX / Glide / EAX / Bink imports (import libraries, or loading at run time as the game already does for
  Glide).
- Produce the address table that agent mode and the checks use (`link-build.mjs` writes it for VC6's linker; lld needs
  an equivalent).
- Keep x87 maths on this rung, so that floating-point differences do not hide real ones.
- **Test:** the fixed build's save, memory and screen checks.
- **Bonus:** sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer) become available. They find out-of-bounds and
  uninitialised reads that the byte match cannot see, which means original-game bugs for the bug list.

**L2: warning-clean, no undefined behaviour.**
- The 570 writable strings become `const char *`.
- The `printf(buf)` calls get a format.
- Every entry in the "matching aids" table of [Matching devices](../matching-devices.md) gets its plain form under
  `MODERN_BUILD`: that table is the work list.
- Argument evaluation order is a known risk: VC6 evaluates arguments right to left, and code that calls two
  side-effecting functions in one argument list may rely on it. The warnings do not show these; reviews must look for
  them.
- **Test:** as L1.

**L3: 64-bit.**
- The 209 failing layout checks are sorted into two kinds:
  - **In memory only:** the layout is free, and the check is switched off for 64-bit.
  - **Written to disk or the network:** save files, packfile directories, image headers, tables, net packets, and the
    structures DirectX, Bink and EAX expect. These get explicit readers and writers instead of whole-struct copies.
- **Hard rule:** the save format stays the 32-bit one, so existing saves keep loading.
- The 120 still-unknown (`_unk`) fields in 47 headers block this wherever they sit in a saved structure.
- x87 gives way to SSE here, so last-bit float differences appear. The checks already classify those (D9203, D9807
  were this kind), so they are visible, not silent.
- **Test:** as L1, plus loading every test save.

**L4: idiomatic rewrite.**
- Real types instead of `int` handles.
- Ownership through RAII instead of the game's own pools.
- Fewer globals.
- A current graphics API in place of DirectDraw / Glide / Direct3D 7, and current networking.
- Done module by module, each with its own behavioural tests.
- This is a new engine port. It is open-ended, and it slowly gives up the equivalence anchor. The `_unk` fields cannot
  be redesigned until they are understood.

## L1 done: the clang build (2026-10-25)

`node tools/link-build.mjs --config fixed --compiler clang` (or `experimental`) builds every unit with clang and links
with lld: `build/link-fixed-clang/Sum-fixed-clang.exe`, `build/link-exp-clang/Sum-exp-clang.exe`
([Tools](../tools.md#toolslink-buildmjs-the-standalone-rebuilt-exe)). Same tree, same defines, same units.

**How the rung was met:**
- **Compiler mode.** clang's MSVC mode (`--target=i686-pc-windows-msvc`, `_MSC_VER` 1200) with VC6's own headers and
  libraries, not mingw. That keeps the MSVC name mangling, which the address table needs. It also keeps VC6's CRT,
  so the game's own `operator new` / `delete` and its memory pools stay as they are.
- **The 11 errors.** The address of a temporary (D15902) and the narrowing case value are accepted as warnings. Their
  plain forms are L2 work.
- **The two missing returns** (D15400, D21001) return their VC6 value under `__clang__`. VC6's bytes are unchanged:
  the strict sweep still matches both units. clang now treats a missing return as an error, so no new one can slip in.
- **`__asm` blocks.** clang's MSVC mode compiles them as they are. Nothing had to be replaced.
- **Imports.** VC6's import libraries, plus the DirectX, Bink and EAX ones `src/link` already builds.
- **Address table.** lld's map file is read like VC6's (16-digit addresses). The table has 9,373 entries; the VC6
  build's has 9,330.
- **x87.** `-march=pentiumpro -mfpmath=387`: no SSE, as in VC6.

**What lld and clang needed that VC6 did not:**

| Problem | Fix |
|---|---|
| clang calls `operator new[]`, which VC6's runtime does not have | `src/link/clang_support.cpp` forwards it to the plain operator |
| sized `operator delete` | `-fno-sized-deallocation` |
| clang's MSVC exception tables need a newer runtime | `-fno-exceptions` (the game throws none; SEH is kept) |
| lld pulls in LIBCMT's console startup first, which clashes with the WinMain one | `/INCLUDE:_WinMainCRTStartup` |
| lld turns on ASLR; the address table and SummonerFix need the exe at 0x400000 | `/DYNAMICBASE:NO /FIXED` |
| the game writes into string literals (`localize_set_string`); clang puts them in read-only `.rdata` | `/MERGE:.rdata=.data` at first; since L2 the writer is fixed and literals are read-only (`--writable-literals` restores the merge) |
| VC6's objects have no SafeSEH tables | `/SAFESEH:NO` |

**Test (Observed, 2026-10-25):** the fixed build's checks, `Sum-fixed-clang.exe` against three runs of the original,
SummonerFix's fixes on, and the VC6 `Sum-fixed.exe` through the same checks the same day for comparison:

| Check | clang | VC6 fixed |
|---|---|---|
| save, Wolong2 / WolongCaverns / sewer / lenele2aa (dwords beyond the original runs' spread) | 217 / 622 / 227 / 225 | 155 / 510 / 256 / 155 |
| memory (symbols that differ, of about 3,290) | 10 / 10 / - / - (the other two were counted after the exe was rebuilt, against a table that no longer matched the dumps) | 13 / 11 / 22 / 11 |
| screens: main menu / inventory / character | 0 / 1-30 / 59-94 pixels | |

- The clang build plays, loads every test save, and behaves like the VC6 fixed build. Its save differences are the
  same kinds and roughly the same counts.
- Most of the save differences in both builds come from one source: the party's 36 player timers show about 6.0 s of
  game time since the level started where the original shows about 7.1 s. Positions of walking characters and
  counters follow from that. It is a property of the fixed build, not of clang, and it is new since 2026-10-21, when
  the same check found 1-10 dwords. Found and fixed the same day: the fixed build's frame limiter dropped late
  wake-ups from `Sleep`, and the clock ran at 82% of real time
  ([Timing bugs](../sum-exe/timing-bugs.md#the-fixed-builds-port-of-the-fix-lost-time-found-and-fixed-2026-10-25)).
- The memory counts are with `link-memcheck.mjs --exe`. Before that fix the tool translated every build's pointers
  with `Sum-rebuilt`'s address table, which showed about 475 differing symbols for clang.
- The two return fixes were made after the save run. The screens were checked again with them (the lower pixel
  counts above).

## L2: warning-clean (2026-10-25)

The clang build defines `MODERN_BUILD` and writes its warnings to `build/<folder>-clang/warnings.txt`. They are
`-Wall -Wextra` without the classes that only report the decompiled style: unused names, signed/unsigned compares,
and the upper-case file names of VC6's headers. VC6 never sees `MODERN_BUILD`, and the full strict sweep still matches
all 5,613 functions.

**Warnings: 708 distinct before, 6 now.** The 6 are listed under the candidates below (the door sound boxes).

| Kind | Count | Fix |
|---|---|---|
| writable strings | 570 | 39 name tables and the read-only literal pointers became `const char *`. `character_load` / `character_create_base` take `const char *const *`. The few `char *` APIs that return or store a literal cast it |
| `sprintf(buf, XSTR(...))` with translated text as the format | 56 | `"%s"` under `MODERN_BUILD` (a `%` in a translation was read as a format) |
| delete through a type with a non-virtual destructor | 17 | `Character`, `ui_window`, `ui_static` and `ui_button` are `final` in the modern build (`MODERN_FINAL`, `vsdk.h`). Elsewhere the destructor the binary calls is written out |
| `memset` / `memcpy` on a class | 14 | `(void *)` casts, in every build |
| the address of a temporary (D15902) | 11 | named locals under `MODERN_BUILD` |
| self-assignments (the D10702 / D25504 matching aid) | 4 | the clamp written out under `MODERN_BUILD` |
| uninitialised on some path, out of bounds, format types, parentheses, explicit constructor calls, a deprecated implicit copy | about 36 | each read in the code: braces and parentheses that keep the parse, defined values under `MODERN_BUILD`, defaulted copy constructors for `vector2` / `matrix43` under `MODERN_BUILD` |

**No more writes into string literals.** The original rewrote one: `localize_load_strings` passes "NULL Format
String" to `localize_set_string`, which unescapes it in place. The modern build passes a copy. So the clang build now
links with read-only literals: lld's `/MERGE:.rdata=.data` is gone, and `--writable-literals` brings it back if
another writer shows up.

**Test (Observed, 2026-10-25),** with read-only literals and the fixed clock
([Timing bugs](../sum-exe/timing-bugs.md#the-fixed-builds-port-of-the-fix-lost-time-found-and-fixed-2026-10-25)):
- No fault anywhere: menu, save load, play, level warps, the four save levels and the four screens.
- The game clock runs at 95.7% of real time, as the original does (96.1%).
- Memory: 8 / 9 / 15 / 7 symbols differ of 3,290.
- Screens: the main menu, the prologue at 12 s and the inventory are identical; the character screen differs by
  117 pixels.
- Saves: about 230 dwords per level beyond the original runs' spread. They are timing, not logic: the party timers
  show 7,231 ms since the warp against 7,197-7,203 ms in the original runs, which this time agreed unusually closely.
  The walking characters' positions follow from those 30 ms. The VC6 fixed build was 3-5 dwords in the run before.

**Candidates for the bug list** (found in the code while fixing warnings; not yet seen in game, so not yet on
[Bugs in the original game](../sum-exe/game-bugs.md)):
- **Script editor, door sounds** (`script_editor_list_door_sounds` `0x4e53a0`): six `sprintf("%.1f", byte)` calls
  print a double made of the byte and the next stack dword. The minimum range, volume and roll-off boxes never show
  the real values, and OK writes the shown text back. The same file converts correctly elsewhere (volume x 1/255).
  These are the 6 remaining warnings.
- **Script editor, creature level** (`0x4ec730`): `if (0.0f < v || 100.0f > v) v = 100.0f;` is true for every number,
  so every skill but index 2 becomes 100. A clamp to 0..100 was meant.
- **Projected bitmaps** (`gr_bitmap_3d_projected` and its neighbour in `unnamed_gr_line_0x53e520.cpp`):
  `if (!pnt->flags & 1)` returns only for flags 0, so a vertex behind the viewer (flag 2, no screen coordinates) is
  drawn from coordinates never computed. `!(pnt->flags & 1)` was meant.
- **EAX check** (`unnamed_ds3d_0x55ddb0.cpp`): `support & SET | GET` tests only Set support.
- **`objects_free_all`** deletes every object through `obj *`, whose destructor is not virtual, so a derived
  object's own destructor never runs (a leak at level unload, not a corruption).
- Smaller: `File::read_matrix43` writes one float past the matrix (dead code); the Glide texture paths store an
  uninitialised format for two bitmap formats; the cutscene table parse writes an int into a `short` (harmless in
  VC6's frame); a debug trace prints an int with `%f`.

## Recommendation

L1 and L2 together as "the modern build", in the same tree, behind `MODERN_BUILD`, tested against the fixed build.
L3 as the next milestone. L4 per module, later, only where it buys something (the renderer first, as it also helps
the [texture upscaling](texture-upscale.md) and display work).

## Decisions

1. How far "modern" goes: **the full L4 rewrite, eventually**, reached through L1 to L3, so that modern features and
   graphics options can be built natively.
2. Same tree, with the clang build selected by `--compiler clang` (and later `MODERN_BUILD`).
3. L1 was done after the planned in-game work (the original-game texture pack, animation flicker, the tentacle grab
   test).
