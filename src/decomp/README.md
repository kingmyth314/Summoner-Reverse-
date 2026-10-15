# Decompiled source (Phase 3 pilot)

C++ source rewritten from Sum.exe's disassembly, one file per original translation unit, for a future
byte-matching build with Visual C++ 6.0 (see [docs/decomp-plan.md](../../docs/decomp-plan.md), "Phase 3" and
"Toolchain"). **None of it has been compiled with VC6 yet**, so "exact" in the notes means "the control flow,
constants and globals match the disassembly", not "the bytes match".

Rules:
- Everything is written from Sum.exe's own code (`tools/asm.mjs`). The Summoner 2 decompilation has no licence and
  is used for names only, never for code.
- Function and global names come from `symbols/sum.symbols`; each function carries its address and a confidence
  comment (exact / likely / unsure).
- Each folder has a `NOTES.md` with the function list and open questions.

| File | Original unit | Functions | Status |
|---|---|---|---|
| `vsdk/frametime.cpp` | `frametime.cpp` (`0x501470`-`0x5017e6`) | 14 | written, syntax-checked with mingw |
| `vsdk/timestamp.cpp` | `timestamp.cpp` (`0x5017f0`-`0x501b16`) | 23 | written, syntax-checked with mingw |
| `game/new_credits.cpp` | `new_credits.cpp` (`0x408590`-`0x408d72`) | 11 | written (7 exact, 4 likely), syntax-checked |
| `game/new_help.cpp` | `new_help.cpp` (`0x40bf30`-`0x40cb55`) | 16 + 5 initialiser pairs | written (5 exact, 11 likely), syntax-checked |

The game files declare the engine and UI functions they call in `game/engine_stubs.h`; some signatures there are
inferred from the calls. Check them with `-Wno-write-strings` (VC6 accepts string literals as `char*`).

Syntax check: `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wextra src/decomp/vsdk/frametime.cpp` (mingw is not the
target compiler; it only catches syntax and type errors).
