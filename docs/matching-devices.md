---
title: Matching devices
nav_order: 6.41
---

# Matching devices

**Summary:** A *matching device* is a way of writing C++ that makes Visual C++ 6.0 SP5 (build 8966) produce the
exact bytes of Sum.exe. Most devices are probably how the original programmers wrote the code. A few only help the
match and are not plausible as the original source. This page gathers the devices from the ten byte-match rounds
(2026-10-19 to 2026-10-21; see the [Decompilation plan](decomp-plan.md)), each with the decision in
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
| The store order of independent fields decides whether a zero is kept in a register. Search the order with `vc6-try.mjs`. | D23100, D24201 |
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

| Device | Decision |
|---|---|
| An int read as a float to keep a zero in a frame slot. | D11703 |
| `vector3::len` returns `(double)(float)sqrt(...)`, reproducing every unoptimised caller's double store. | D12900 |
| `#pragma optimize("w", on)` from 0x518ce0 to the end of the net unit. | D14204 |
| `ui_window::send_event` and `ui_gadget::send_event` fall off their end and return whatever is in AL. **Undefined behaviour.** | D15400, D21001 |
| `&(a+b)` temporaries under `#ifdef _MSC_VER` (a VC6 extension). | D15902 |
| A volatile dword read, to stop a byte narrowing. | D16500 |
| A `float *` view of a buffer. | D16704 |
| A `volatile float *` for three zero stores. | D18205 |
| `*(volatile int *)&g = v`, to keep a global store above the next call's pushes. | round 2026-10-20 |

## What is not merged

A device that only brings a function *closer* and is itself implausible (a `volatile` local, an unused pointer local,
a set of float locals that could round differently) is not merged. Examples: D20702, D21906, D25002. A closer change
that keeps the source plausible is merged after it has been read for behaviour against the previous version.

## What does not move

Most of the roughly 320 functions that still differ are register choice and instruction scheduling, decided by 8966
itself. The workers' notes (`src/decomp/NOTES-match-*.md`) name what was tried on each. Read them before you start on
a function. `vc6-compare --align ADDR` shows whether the remaining difference is one block or spread through the
function.
