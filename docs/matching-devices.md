---
title: Matching devices
nav_order: 6.41
---

# Matching devices

**Summary:** A *matching device* is a way of writing C++ that makes Visual C++ 6.0 SP5 (build 8966) produce the
exact bytes of Sum.exe. Most devices are probably how the original programmers wrote the code. A few only help the
match and are not plausible as the original source. This page gathers the devices from the eleven byte-match rounds
(2026-10-19 to 2026-10-22; see the [Decompilation plan](decomp-plan.md)), each with the decision in
`src/decomp/include/DECISIONS.md` that records it. It is written for whoever works on the next round: try these first
on a function that still differs. The tools are in [Tools](tools.md#byte-matching-workflow-tools).

Evidence: each device is **Observed**: it produced the original bytes under VC6 8966 in the function its decision
names. That a device is the original programmers' form is **Inferred**, except in the matching-aids table, where it
is not claimed.

The general rule from every round is that 8966 ignores much of the source's form: it renames registers, reorders
independent statements and merges tails as it likes. A device works by changing something the compiler cannot see
through: a name, a scope, a local, an aliasing pointer, a control-flow edge or a call boundary.

## Frame layout (stack slots)

| Device | Where it helps | Decision |
|---|---|---|
| **Unoptimised functions place locals by a hash of their names.** Bucket = `(h ^ (h >> 16)) & 15`, with `h = h * 4 + (h >> 4) + c` over the name's characters (32-bit). Bucket 0 is the highest address; within a bucket, the later declaration is higher. Block-scope locals come after the function scope's, and `this` comes last. A wrong order is fixed by **renaming** locals, not by reordering their declarations. `tools/vc6-localhash.mjs` predicts the order and `--probe` checks it. | `/Od` units (`objects.cpp`) and every `#pragma optimize("", off)` function (`living_entity.cpp`, the cutscene file, `pathfinding_visibility.cpp`) | D6101, D23500, D25200 |
| Block-scope locals are placed by the same hash, block by block in source order (the `$NNN` suffix in the listing is not part of the name). Parameters that an inline helper takes by value become temporaries after all named locals. In `/Od` code an inline expansion can drift across the whole function from a change that gives the same code; the order of the out-of-line `vector3` calls, compared with the binary's, shows where. The name hash has **no** effect in optimised code (about 90 renames in three functions moved nothing). | four `Character` functions in `living_entity.cpp`, from distance 500-2,300 | D25500-D25504 |
| A local in a block of its own, closed before the next copy, can take dead argument slots. | `matrix3_pack_orientation` (closer only) | D25400, D7004 |
| Local names also decide the slot order in SEH frames, even at `/Ox`. | the exception handler unit (its locals took the names of the published ExceptionHandler.cpp it follows) | round 2026-10-20 |
| Buffer sizes read from the binary's frame offsets. | any char buffer | round 2026-10-19 |
| Locals moved into sibling blocks so that they share a stack slot. | functions whose frame is smaller than ours | round 2026-10-19 |
| A two-float array puts a local in the binary's slot. | `savegame::automap_label_remove`, `read_automap_labels` | D17401, D17402 |
| A one-float array for a clamped value. | clamps | D19600, D19601 |
| Separate locals instead of a small array. | | D21502 |
| A `pf_edge` local whose extra bytes are the binary's unused top of frame. A `pf_edge` local stands in for a `seg2d` one. | pathfinding | D19301, D19500 |
| Declarations moved into the block that uses them. | | D19302 |
| A pointer local for a scalar passed by address (`float *tp = &p1;` in place of `&p1`) moves the scalar in the frame. | `camera_blend_sequence::advance` | D19700 |

## Register choice and operand order

| Device | Decision |
|---|---|
| A pointer local or a pointer copy sets the operand order, where the source order is ignored. | D17202, D17203, D17500 |
| A store through a pointer local stops a register reuse the binary does not have. | D18100 |
| A member copied into a local and used in an earlier expression orders a `lea`'s registers. | D17600 |
| An unsigned loop index with a signed test sets an address's base / index order. | D17702 |
| Copy a float into a local before its products, and use `const float *pw = &w`. | D18200-D18206 |
| An int local for a flag remap. | D18900 |
| A byte-offset key loop for `[base + off]` addressing. | D21800 |
| **A local that caches a pointer field** (`c = list[i].character`) steers the loop pointer, the callee-saved registers and the frame size. Writing the field out at each use, or caching it, is the choice to try; it matched two cases that looked like plain register choice. | D23000, D23001 |
| `(r = frand()) < x` stops pointers being cached across the call. | D22503, D22504 |
| The store order of independent fields decides whether a zero is kept in a register. Search the order with `vc6-try.mjs`. | D23100, D24201, D26200 |
| A field assigned straight from the value just stored through an out pointer (`*u = s->u; this->u = *u;`), not through a local. Under `/Ow` 8966 then forwards the store as the binary does. | D25702 |
| The same two-store form in both branches (`next = head; head = this;`), with no shared temporary. | D25700 |
| The order of a dot product's terms, or of independent stores, searched with `vc6-try.mjs`. | round 2026-10-19 |

## Control flow and block order

| Device | Decision |
|---|---|
| A ternary call argument written as an if / else of two calls. | round 2026-10-19 |
| A case group with several labels compiles as a range test; separate labels give the compare tree. | round 2026-10-20 (second) |
| An error exit inside `for (;;)` keeps duplicated tails apart. | round 2026-10-20 (second) |
| A range test through a bool local puts the early return first. | D17200 |
| An inline helper around a group of stores moves a scheduled instruction past them. This one matched after 360 orders had been tried. | D17300 |
| A `do { } while` loop stops 8966 rotating a top-tested loop. | D18300 |
| A `do { ... break; } while (0)` clamp puts both stores out of line. | D18400 |
| A single exit, through `goto` or one `if / else`, so that a callee-saved register is pushed late. | D18704, D19101 |
| A second test written in both branches. | D19800 |
| The body inside `if (key != 0)`, with one exit. | D20402 |
| A bool local with one if / else-if chain, for block order. | D21500 |
| A redundant repeated test keeps a control-flow edge that 8966 would otherwise remove. | D21803 |
| **The source order of switch cases sets the binary's block order.** | D23400, D24101, D24702 |
| `result = K; break;` in place of returns, where the binary keeps the result in a callee-saved register. | round 2026-10-21 (fifth) |
| An early `return` rewritten as an `if (...) { rest }` block, giving the binary's `jne` to the end. | D23500 |
| 8966 puts a shared `fail:` block right after the last `goto` that reaches it. Letting the last failure fall into the label puts the block at the end of the function, where the binary has it. | D25901 |
| Which reads of a global go through a local copy decides how far 8966 threads jumps: one test on the global and the next on the copy threaded only the path the binary threads. | D26101 |
| A duplicate test that ends in `goto found;`, the rest in a block, and one `found: return i;`. | D25800 (closer) |

## Calls and inlining

| Device | Decision |
|---|---|
| Calls hoisted out of argument lists into locals. | round 2026-10-19 |
| A call nested in another call's arguments: the original kept the inner result in a local. | D22701, D22703 |
| `#pragma inline_depth(0)` where the binary calls a small helper out of line. This also decides x87 rounding, because out-of-line vector operators round at each call. | D9810, round 2026-10-20 |
| `/Ob0` for whole units that the binary built without inlining (`vfile`, `gr`, `movement`, some vsdk units). | D7502, round 2026-10-20 |
| `memcpy` for a corner or `vector2` copy. | D18401, D19500 |
| An empty member that the linker folded into `empty_function`. | D18000 |
| A `cond ? true : false` argument, for a `mov al, 1`. | D21000 |
| A function that looks `void` but whose register choice is wrong may return a value no caller reads: check what EAX holds at every `ret`. | D26000 |
| Unused `vector3` locals that make VC6 emit the binary's out-of-line constructor calls (in `camera_blend`; they decide D9807's rounding). | D23900 |

## Floats (x87)

| Device | Decision |
|---|---|
| A float copied through a parenthesised temporary; a parenthesised member in a float compare. | rounds 2026-10-19, 2026-10-20 |
| `vector2::operator+` as a constructor return (a shared header). | D9700, D9701 |
| `e->dist = (len = (float)sqrt(...))` with the parentheses gives `fst len / fstp e->dist`. | D23500 |
| A line position written as `i * h + top + scroll`, which 8966 strength-reduces in the binary's order. | D21003, D21004 |
| A `static const float` for a constant the binary loads from `.rdata`. | D22002 |
| Constants checked one unit in the last place against the binary's bytes. Several were off: 1/pi, 57.29578f, `0.025f * 0.025f`. | D9400, D11400, D12202, D12401, D17001, round 2026-10-21 (fifth) |

## Data layout

| Device | Decision |
|---|---|
| Globals that the code uses as one record became one struct (two bosses' state; the key queue `key_tables`). | D10503, D10504, D14001 |
| The footstep sound table as two arrays, as the binary indexes it from two bases. | D14305 |
| The avoidance spline ends as plain float pairs, because the binary never constructs them as vectors. | D13300 |
| A one-bit bit-field for a single flag bit. | D21901 |
| The `max(x, 0)` clamp written `x &= (x <= 0) - 1` on a global. | round 2026-10-19 |

## Matching aids (not the original source)

These change no behaviour under VC6, but they are not how anyone wrote the code. The
[fixed build](builds.md#the-fixed-build) replaces them with plain C++ when it touches the function. Two of them
return nothing, which is undefined behaviour under a modern compiler, so a compiler port has to add the return.
The clang build ([Modern C++ port](future/modern-port.md#l2-warning-clean-2026-10-25)) has the plain form of each
one a warning points at, under `MODERN_BUILD`: the returns (`__clang__`), the `&(a+b)` temporaries, the
self-assignments, and the `#pragma optimize("w")`, which it skips.

| Device | Decision |
|---|---|
| An int read as a float to keep a zero in a frame slot. | D11702 |
| `vector3::len` returns `(double)(float)sqrt(...)`, reproducing every unoptimised caller's double store. | D12900 |
| `#pragma optimize("w", on)` from 0x518ce0 to the end of the net unit. | D14204 |
| `ui_window::send_event` and `ui_gadget::send_event` fall off their end and return whatever is in AL. **Undefined behaviour.** | D15400, D21001 |
| `&(a+b)` temporaries under `#ifdef _MSC_VER` (a VC6 extension). | D15902 |
| A volatile dword read, to stop a byte narrowing. | D16500 |
| A `float *` view of a buffer. | D16701 |
| A `volatile float *` for three zero stores. | D18205 |
| `*(volatile int *)&g = v`, to keep a global store above the next call's pushes. | round 2026-10-20 |
| Unused locals (`dir`, `frame_m`, `pivot`, `norm`, `delta`), a `gain = gain;` and the file-local helpers `ghs_assign_max` / `ghs_assign_min` in four `/Od` `Character` functions; their exact local names belong to the match too. | D25500, D25501, D25504 |

{: .important }
Corrected 2026-11-10: this table cited D11703 for the int read as a float and D16704 for the `float *` view. The
decisions that record them are D11702 (`snd_calc_volume_3d`, `sound_emitter_calc_volume`) and D16701
(`ca_render_morph_vertices`); D11703 is the `/Od` local renaming of `living_entity.cpp`, D16704 the door pose of
`shadow_transform_to_lobj`. **Observed** (DECISIONS.md and the comments at the functions).

## Undefined and compiler-dependent devices (2026-11-10)

**Summary:** of the 73 devices in the tables above, 5 are undefined or compiler-dependent behaviour (class a); with
two more found by searching the source (explicit constructor calls), 7 in all. Since 2026-11-10 every build but the
shipped one compiles a plain form of each, under `PLAIN_SOURCE` (D26300). The other 68 are well-defined C++ (class
b) and stay as they are: the source remains the matching reference, and only class (a) can be miscompiled.

`PLAIN_SOURCE` is defined in `include/vsdk/vsdk.h` when `FIXED_BUILD`, `MODERN_BUILD` or `__clang__` is, so in the
fixed, experimental and clang builds, never in the shipped configuration that the sweep compiles. Before
2026-11-10 the clang builds already had most of these plain forms (under `__clang__` or `MODERN_BUILD`); the VC6
fixed build did not, so its two `send_event` functions still fell off their end. **Observed**: the strict sweep
after the change, 5,613 matched, 0 lost; the fixed (VC6), fixed (32-bit clang) and experimental (64-bit clang)
builds link with `unresolved 0; multiply defined 0`.

### Class (a): plain form under `PLAIN_SOURCE`

| Device | Why it is class (a) | Plain form | Decision |
|---|---|---|---|
| `ui_window::send_event` 0x52aff0 falls off its end; AL holds 0 from the tested NULL callback. | Undefined behaviour (a `bool` function with no return). | `return false;` | D15400 |
| `ui_gadget::send_event` 0x52c330 falls off its end after a handled event; AL holds the callback's nonzero result. | Undefined behaviour, as above. | `return true;` | D21001 |
| `v = *(float *)&zero` with `int zero = 0` in `snd_calc_volume_3d` 0x520fa0 and `sound_emitter_calc_volume` 0x5210b0. | A type-punned read (an `int` read through a `float` lvalue: strict aliasing). | `v = 0.0f;` (the bits of int 0 are float +0.0) | D11702 |
| `float *mv = (float *)ca_morph_vertices`, written as `mv[v * 3 + c]`, in `ca_render_morph_vertices` 0x50b150. | Pointer arithmetic through a `float *` across `vector3` objects (outside the object it points into). | `ca_morph_vertices[v].x / .y / .z` | D16701 |
| `#pragma optimize("w", on)` from `nw_accept_reliable` 0x518ce0 to the end of the net unit. | An aliasing assumption given to the optimiser (`/Ow`); correct only if the code never aliases as it excludes. | The pragma is skipped: the unit's `/O2`. | D14204 |
| `plane pl(&o, &(o + d), &(o + *b))` in `ix_ray_parallelogram` 0x53af40. | The address of a temporary: a VC6 extension, ill-formed C++. | Two named `vector3` locals. | D15902 |
| An explicit constructor call, `((GameTimer *)&timer)->GameTimer::GameTimer()` in `living_combat` 0x4681b0 and `((vector3 *)&_unk0c)->vector3::vector3()` in `gr_light_desc` 0x50f3e0. Not in the tables above. | An MSVC extension, ill-formed C++. | A placement new; for the empty `vector3()` no call. | D7805, D8900 |

The behaviour is the same in each case (**Inferred** from the code: each plain form computes the value the VC6 form
produced, as the decisions describe). No in-game difference is expected.

### Class (b): left as they are

The other 68 rows of the tables: frame layout (12), register choice (13), control flow (18), calls and inlining (9),
x87 floats (6), data layout (5) and five of the ten matching aids. Some of these read oddly but are defined C++:

- `vector3::len` returning `(double)(float)sqrt(...)` (D12900) is a no-op conversion back to float. It is a header
  inline, so changing it would move x87 rounding in every caller of the VC6 fixed build for no gain.
- The `volatile` reads and stores (D16500, D18205, the init flags of round 2026-10-20) are well-defined: each
  `volatile` lvalue has the object's own type (`game_flags`, `d3d_init_done`, `glide_init_done` are `int`).
- The self-assignments and unused locals (D10702, D25500-D25504) do nothing.

Not counted as devices, and not changed: the `*(int *)&f` copies of float bits into byte buffers in the save and
script code (`level_scripts.cpp`, `ai.cpp`, `spells.cpp`) are the original's serialisation, and the uninitialised
reads named in the source comments (for example `skeleton.cpp`, `vfile`, the script editor) are game bugs, not
devices. The clang builds compile with `-fno-strict-aliasing` and `-Werror=return-type` (**Observed** in
`tools/link-build.mjs`), so neither a type pun nor a missing return can be miscompiled there silently.

## What is not merged

A device that only brings a function *closer* and is itself implausible (a `volatile` local, an unused pointer local,
a set of float locals that could round differently) is not merged. Examples: D20702, D21906, D25002. A closer change
that keeps the source plausible is merged after it has been read for behaviour against the previous version.

## What does not move

Most of the roughly 310 functions that still differ are register choice and instruction scheduling, decided by 8966
itself. The workers' notes (`src/decomp/NOTES-match-*.md`) name what was tried on each. Read them before you start on
a function. `vc6-compare --align ADDR` shows whether the remaining difference is one block or spread through the
function.
