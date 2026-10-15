---
title: "vsdk: math (matrix, vector)"
parent: Sum.exe
nav_order: 5.95
---

# vsdk: math (`matrix.cpp` and vector helpers)

**Summary:** Phase 1, module 6. `vsdk\math\matrix.cpp` (around the source-file reference at
`0x510474`) matches the [Summoner 2 decomp](../reference/engine-references.md)'s
`matrix.cpp`. Its functions appear in the same order and carry their original names
(**V**). Small vector and matrix helpers around it are named by behaviour (**I**). 44
symbols in total; the types are in `symbols/types.h`.

## Matched to Summoner 2

| Address | Name | Evidence |
|---|---|---|
| `0x510390` | `matrix3::get_axis_angle` | Same "invalid index - this should never happen" assert, and the same 0.9999999 trace tests |
| `0x510b40` | `matrix3::orthogonalize` | Same fallbacks when the up or right vector is zero |
| `0x511050` | `matrix3::make_identity` | |
| `0x511080` / `0x511140` / `0x511200` | `set_from_forward` / `set_from_unit_forward` / `set_from_forward_up` | `set_from_forward_up` falls back to `set_from_forward`, as in Summoner 2 |
| `0x5113a0` / `0x5113f0` / `0x511450` | the three `matrix43` constructors | `(rotation)`, `(r, u, f, position)`, `(position, rotation)` |
| `0x5116f0` | `matrix43::inverse` | Identity, transpose, then `-position * result` |
| `0x511840` | `matrix43::set(quaternion, position)` | Through `quaternion__to_matrix` (`0x522bf0`) |
| `0x511b70` | `matrix43::get_rotation` | |
| `0x5a7480` | `Homogeneous_column` | `{0, 0, 0, 1}`, used by `multiply_4x4` (`0x5114b0`) |

Differences: the PC build has two `multiply_4x4`-style functions (`0x5114b0`, `0x511ab0`),
each with its own copy of the `{0,0,0,1}` table. Summoner 2's hand-written R5900 `multiply`
has no PC counterpart.

## Helpers (behaviour only)

- **matrix3:**
  - multiply `0x50fbc0` and multiply-transposed `0x50fe20`;
  - `set_from_angles(p, b, h)` `0x5100c0` and `extract_angles` `0x510240`;
  - `set_from_axis_angle` `0x510110`;
  - `is_valid` `0x510660` (no NaNs) and `is_normalized` `0x50ff50` (rows within 5e-7 of unit
    length).
- **vector3:**
  - constructor `0x43b580`, `set` `0x511c10`, assignment `0x413190`;
  - `cross` `0x50d7e0`;
  - `normalize` / `normalized` / `normalize_length` (`0x524430`, `0x524270`, `0x524470`), and
    `normalize_safe_length` (`0x524510`), which sets a zero vector to (1, 0, 0) and returns 1.0;
  - `is_valid` `0x524220`, `equals` `0x511c30`;
  - scaling and subtraction (`0x50f6a0`, `0x50f790`, `0x50f840`).
- `atan2_fast` (`0x542790`), and the C runtime `_CIacos` (`0x56dd70`) and `_isnan` (`0x56e90c`).
- **vector2:** `0x5254d0` is a segment / circle test, not a projection: (a, b, p, radius, out t) gives the
  first t in 0..1 where `a + t (b - a)` is `radius` from `p` (t 0 when `a` is already inside), caller
  `pf_sweep_test_wall`.
- **quaternion** (`0x5225b0`-`0x523f29`, written in `src/decomp/vsdk`): the second matrix conversion
  `0x522790` reads `m[0][1]` for `m[0][0]` in its z case (`0x522841`; [game bugs](game-bugs.md), K262), and the packed
  `cquat` normalisation (`0x523050`, `0x5232b0`) divides with integers (C262).

Many vector helpers live in game code (for example `0x413190` and `0x43b580` sit in the
`0x400000` block). MSVC emitted out-of-line copies of inline methods where they were used.
A byte-matching rebuild will need the same inlining choices.
