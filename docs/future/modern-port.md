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

**Warnings: 708 distinct before, 6 now.** The 6 are the script editor's door sound boxes, an original bug (K220; below).

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

**What the warnings pointed at** (each candidate checked against the binary on 2026-10-25; see
[Bugs in the original game](../sum-exe/game-bugs.md)):
- **Script editor, door sounds:** already listed as K220. The six `sprintf("%.1f", byte)` calls are the 6 remaining
  warnings.
- **Script editor, creature level:** already listed as G226.
- **EAX check:** new, V303. `support & SET | GET` tests only Set (`and edx, 0x2; or edx, 1; cmp dl, 3` at `0x55df86`).
- **`File::read_matrix43`** writing past the matrix: already listed as V256 (dead code).
- **Did not hold** (added to the page's "Claims that did not hold"):
  - The projected-bitmap flag test `if (!pnt->flags & 1)` is a precedence slip that never returns. Both functions
    still reject a point behind the viewer through the clip codes, so it has no effect.
  - `objects_free_all` deleting through `obj *` skips nothing: `~Character` only sets `type`.
- **Left as notes:**
  - The Glide texture paths store an uninitialised format for two bitmap formats that no texture uses.
  - The cutscene table parse writes an int into a `short`, which is harmless in VC6's frame.
  - A debug trace prints an int with `%f`.
- **Not seen in game:** K220 and G226 stay "Observed code". The editor opens in agent mode (game flag `0x80`), but its
  buttons did not respond to agent-mode clicks in the test, so the door and creature windows could not be opened.

## L3: 64-bit (begun 2026-10-25)

**Platform facts** (**Observed** on the test PC):
- Direct3D 7 immediate mode (`d3dim700.dll`) exists only as a 32-bit DLL (SysWOW64). So do Glide, EAX and Bink
  (`binkw32.dll`).
- DirectDraw, DirectSound, DirectInput and OpenGL have 64-bit DLLs.
- llvm-mingw has the x86_64 headers and import libraries for them.

**Decisions** (the user, 2026-10-25):
1. **Renderer:** the 64-bit build has the OpenGL renderer only, the one the fixed build already uses, and DirectSound
   without EAX. The 32-bit builds keep everything.
2. **Movies:** decoded with FFmpeg's libavcodec, which reads Bink 1 (LGPL, loaded as DLLs, never committed; the
   download is asked for first).
3. **Testing:** agent mode, and the test-save, dump and key hooks the checks use, move from SummonerFix (a 32-bit DLL)
   into the source, so any build, 32 or 64-bit, can be driven.

**Stages:**

| Stage | Work | Tested in |
|---|---|---|
| L3a | portable code: no `__asm`, the 64-bit crash context, pointer-sized callback parameters | the 32-bit clang build |
| L3b | agent mode in the source | the 32-bit clang build, then 64-bit |
| L3c | the 211 failing layout checks: memory-only layouts freed, file and network formats read and written field by field | both |
| L3d | the x86_64 build: `link-build --compiler clang --arch x64`, OpenGL only, no EAX, FFmpeg for Bink | 64-bit |

**L3a done.** An x86_64 syntax check of every unit found 226 errors: 211 layout checks and 15 others. The 15 are now
fixed, and the `__asm` blocks have `MODERN_BUILD` forms:

| Where | Modern form |
|---|---|
| `timer_get` (64-bit by 32-bit division in `__asm`) | the same division in C: the low 32 bits of the quotient |
| `read_tsc`, `cpu_read_tsc` | `__builtin_ia32_rdtsc()` |
| `cpu_identify` (CPUID levels 0, 1, 0x80000000-4) | `<cpuid.h>`'s `__cpuid` (clang's; `<intrin.h>` needs a newer C runtime than VC6's) |
| the `sysinfo` memory benchmark (`rep movsd`) | `memcpy`, with a volatile read so it is not optimised away |
| `float_to_int_round` (`fld` / `fistp`) | rounding to nearest, ties to even, written out: equal to `fistp` on 2,000,013 test values. The old syntax-check stub truncated instead |
| the crash handler: `Eip` / `Esp` / the register dump, the stack top from `fs:[4]` | `CONTEXT_IP` / `CONTEXT_SP` (the same text in 32-bit builds), a `Rxx` dump under `_WIN64`, the stack top from `VirtualQuery` |
| `snd_timer_proc` and `acm_driver_enum_callback` | `snd_callback_param`: `DWORD` in 32-bit builds, `DWORD_PTR` under `_WIN64` |

The x86_64 check now finds only the layout checks. 111 size warnings remain, nearly all `size_t` to `int`. VC6 still
matches every function. The 32-bit clang build plays with the modern forms, with its clock at 96.3% of real time.

### The 64-bit spike (2026-10-25)

Done before porting anything (the advisor's order), to find out what a 64-bit exe needs instead of estimating from
what SummonerFix gives the 32-bit one. **Observed**; the game state was read from outside with `ReadProcessMemory` at
the link map's addresses, since screen captures of the OpenGL window come back black.

**Baseline: the 32-bit clang build without SummonerFix** (`dinput.dll` moved aside).
- It opens a framed 1024x768 window and runs the main menu at 120 fps, with the real-time clock advancing normally.
- Real mouse clicks open the load screen and load the newest save, and the game plays (level time counting from the
  save's 66.6 s).
- It reads its settings from `HKCU\Software\Volition\Summoner`. 64-bit Windows does not redirect that key, so a 64-bit
  exe sees the same settings.

**The x86_64 build** (`link-build --config fixed --compiler clang --arch x64`, llvm-mingw's headers and libraries,
layout checks off under `_WIN64`):
- Every unit compiles. The link needed only three GUIDs taken from MinGW's `libdxguid` instead of the game's own
  definitions, and stand-ins for 8 Bink functions and `EAXDirectSoundCreate` (`src/link/x64_stubs.c`).
- Glide and Direct3D 7 need nothing at link time: the game reaches them at run time.
- **It runs:** the window, the main menu at 120 fps, the clock right, and mouse input through window messages.
- **First crash, fixed:** the load screen's save list. `_findfirst` returns a pointer-sized handle that four
  functions kept in a `long`. They now use `find_handle` (`vsdk.h`): `long` in 32-bit builds, `intptr_t` in 64-bit.
- **Second crash, fixed in L3c below:** the level load, in the effect files. `.vfx` effects (and the animation tracks and attached
  lights) are loaded as one block of the file, and `relocate` turns their 32-bit offset fields into pointers in place
  (`reloc_ptr`, `ptr_relocate`). That is about 140 relocation calls in 10 units.

**What this means for the plan:**
- **L3b (agent mode in the source) is smaller than estimated.** Without SummonerFix the game already starts, takes
  input and loads saves. What remains is agent mode itself and the test-save folder; the registry needs nothing.
- **L3c is concentrated in one place, the relocated file formats.** Whole-struct reads elsewhere are few: a TGA
  header, the options file, one effect header. The size warnings (111, nearly all `size_t` to `int`) are where to look
  for more truncated handles like `_findfirst`'s.
- **The relocated formats have two practical fixes:**
  1. **32-bit pointer fields:** link the 64-bit exe so that every address stays below 2 GB (`/LARGEADDRESSAWARE:NO`),
     and declare the pointer fields of the relocated structs as a 4-byte pointer type under `_WIN64`. The file
     blocks then stay usable in place and the layouts stay those of the binary. The game uses far less than 2 GB.
  2. **Convert on load:** read each relocated block into separately allocated 64-bit structs, one converter per
     record type. That is more code, but it also lifts the 2 GB limit, which L4 may want.

### L3c, first stage: the 64-bit exe loads a save (2026-10-25)

**Option 1 chosen** (with the advisor). The 2 GB limit is not a regression: the shipped Sum.exe is a 32-bit exe without
large-address awareness, so the game, texture pack included, already lives under 2 GB. Option 2 can replace it format
by format later if L4 needs the room.

**Checked first (Observed, a test program):**
- Linked without large-address awareness, the stack, the heap (64 MB blocks too), `VirtualAlloc`, the exe and
  `kernel32` all lie below 2 GB. Only `ntdll` stays high: it is mapped before the flag applies, and no game data
  points into it.
- clang's `__ptr32 __uptr` makes a 4-byte pointer that converts to and from an ordinary one. MinGW's `_mingw.h`
  defines `__ptr32` as nothing; `vsdk.h` undoes that after including it.

**What changed:**

| Part | Change |
|---|---|
| `vsdk.h` | `PTR32`: `__ptr32 __uptr` under `_WIN64`, nothing in 32-bit builds. `LAYOUT_CHECK` holds in every build again; `LAYOUT_CHECK_32` holds in 32-bit builds only |
| `vfx.h`, `effect_mem.h`, `skeleton.h` (the relocated formats) | all 122 pointer members are `PTR32`. Their 38 layout checks pass on x64: the file layouts are proven, not assumed |
| `reloc_ptr`, `ptr_relocate` | work on 4-byte slots under `_WIN64` |
| the other 173 failing layout checks | `LAYOUT_CHECK_32`: memory-only layouts. The save structures are written field by field. The packfile, file and line-file structures are in-memory handles. The multiplayer packets (`multi.h`, `net.h`) are to be looked at again when multiplayer is ported |
| the 11 unions that mix a pointer with an `int` | the pointer is `PTR32`, so both halves are 4 bytes. The ground probe wrote a face pointer through the `int` half and the raycast read 8 bytes |
| `delete` of a `PTR32` field | a cast to the field's own type gives an ordinary pointer |
| sockets | `net_socket`: `unsigned int` in 32-bit builds, pointer-sized under `_WIN64`. An `unsigned int` never equals the 64-bit `INVALID_SOCKET` |
| `link-build --arch x64` | links at 0x400000 and clears the large-address-aware and high-entropy flags in the PE header |

The other narrowing warnings were checked by kind rather than one by one: `size_t` and pointer differences (265),
`WPARAM` / `LPARAM` (5) and `time_t` (1) carry small values. Pointer-to-`int` casts lose nothing when every address is
below 2 GB. Window styles (`SetWindowLongA` with `GWL_STYLE` / `GWL_EXSTYLE`), `ShellExecute`'s result and
`_beginthread`'s result are used as 32-bit values.

**Observed** with `tools/standalone-probe.ps1`: the 64-bit exe opens the load screen, loads the newest test save
(level time from 66.5 s, as the 32-bit build loads it) and plays at 120 fps. VC6 still matches all 5,613 functions,
and every build's address table is unchanged.

### L3b: the in-source host (2026-10-25)

What SummonerFix gives a rebuilt exe, now in the source for when it is not there: a 64-bit exe (SummonerFix is a
32-bit DLL), or a 32-bit clang build run without it. `src/decomp/modern/` (`include/modern/modern.h`), compiled into
the clang builds only (`MODERN_BUILD`). It reads SummonerFix's own `summonerfix.ini` keys and writes
`summonerfix.log`, so the settings and the test tools carry over.

| Unit | What it does |
|---|---|
| `host.cpp` | detects SummonerFix (a `dinput.dll` beside the exe; for a 64-bit exe Windows skips that 32-bit DLL and loads the system one), reads the settings, loads `<exe>.addr` (the Sum.exe-address table, so commands with Sum.exe addresses work), picks the save folder (`savetest` with `[Debug] TestSaves=1`), keeps the log |
| `display.cpp` | SummonerFix's display layer: borderless full screen (`[Display] Mode`) or a sized window, every `glViewport` scaled into a centred 4:3 area, the bars cleared before each swap, the mouse mapped between window and game pixels, the save thumbnail's read-back mapped and box-filtered, the focus handling that lets Alt+Tab leave, and captures at half size |
| `agent.cpp` | agent mode with SummonerFix's protocol (`agent\cmd.txt`, `result.txt`, `ready.txt`) and commands, so `tools/agent.mjs` and every check tool work unchanged. Game-thread calls go through the game's own message pump. Characters are reached through their members (the layouts differ in 64-bit) |

The game calls it at six places, each under `MODERN_BUILD` and passing straight through while SummonerFix is loaded:
WinMain (settings), `os_init` (agent mode), `path_savegame`, `gr_init`'s windowed branch (the output window),
`os_wndproc`'s activation (the focus), and the GL and mouse calls of `gr_opengl.cpp` and the mouse unit (`glViewport`,
`SwapBuffers`, `glReadPixels`, `ScreenToClient`, `ClientToScreen`). Not yet in the source: SummonerFix's centred
widescreen menus, brightness and fast-mode badge, and its controller (the experimental build has its own).

`link-build --arch x64` now also writes the 64-bit exe's `.addr`. lld's map lists demangled names, and the hybrid
build's names are MSVC-mangled; both are reduced to the qualified name, and the names that occur once on each side
are joined. That gives 8,884 of the 9,330 rows. Rows are written against the base 0x400000, and the host adds the
actual load address.

**Found on the way** (all fixed, VC6's code unchanged):
- **Fixed-size allocations:** six cutscene records were allocated with their 32-bit sizes (`Alloc(0x10)` for a
  structure that is 0x20 bytes in 64-bit), and the script editor allocated a pointer array as `count * 4`. They now
  use `sizeof`, which is the same constant in 32-bit builds. Starting a new game crashed on the first one.
- **Fixed-size pools:** the game's static memory pools (character and model pools, items, localisation, dialogue,
  the script editor, cutscenes, and the 25.6 MB level pool) are sized for 32-bit objects; the cutscene pool ran out
  once its records had their real size. `POOL64(n)` (`vsdk.h`) is `n` in 32-bit builds and twice that in 64-bit.
- **`va_start` in the 32-bit clang build:** VC6's macro takes the address of the last named parameter, which clang
  does not keep in place in a static variadic function. Agent mode's answers came out empty; the crash handler's
  `hprintf` and the experimental tentacle trace had the same flaw. `vsdk.h` and `modern.h` use clang's builtins
  there.

**Observed:**
- **The 32-bit clang build with SummonerFix:** the host stays off; SummonerFix's log, window and agent are the ones
  in use, and the clock runs at 96.0%.
- **The same build without SummonerFix:** the in-source agent drives it; the menu, a save load and play work.
- **The 64-bit exe:** borderless at 2560x1440 with the 4:3 picture centred. Captures, the main menu, the load screen,
  a save load and play, and Start New Game with the prologue all work.
- **The save and screen checks on the 64-bit exe** (`link-savecheck` / `link-screencheck --exe
  Sum-fixed-clang-x64.exe --fixes`, driven by the in-source agent):
  - Screens: the main menu, the prologue at 12 s and the character screen are identical to the original's; the
    inventory differs by 20 pixels.
  - Saves: all four levels load and save; 82 / 246 / 207 / 94 dwords lie beyond the original runs' spread. They are
    the NPC goal difference below (and its consequences), which the 32-bit clang build shows too; the party records
    match, timers included.
  - The memory check does not apply: the layouts differ by design.
- **Open: clang's NPC goals.** In both clang builds, some NPCs and monsters have a second AI goal that the original
  does not (the save check's differences at record offsets `+0xb2`..`+0xe1`, the second of the five saved goals). The
  VC6 fixed build shows none. Compiler-dependent behaviour, such as argument evaluation order or floating-point
  evaluation, is the suspect; it is not specific to 64-bit.

**Next:**
- The clang NPC goal difference above.
- FFmpeg for the Bink movies.
- Longer play in 64-bit: combat, spells, every level.

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
