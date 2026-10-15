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

Syntax check: `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wextra src/decomp/vsdk/frametime.cpp` (mingw is not the
target compiler; it only catches syntax and type errors).
