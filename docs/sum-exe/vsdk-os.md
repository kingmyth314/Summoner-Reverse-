---
title: "vsdk: OS layer"
parent: Sum.exe
nav_order: 5
---

# vsdk: OS layer
{: .no_toc }

1. TOC
{:toc}

**Summary:** Phase 1, module 1 of the [Decompilation plan](../decomp-plan.md). The engine's
OS-layer code in Sum.exe matches the [Summoner 2 decomp](../reference/engine-references.md)
function for function wherever that project has rebuilt the source, so these functions
carry their **original Volition names** (confidence **V** in `symbols/sum.symbols`). Names
and signatures come from the reference; the code itself was read in Sum.exe with
`tools/decomp.mjs`.

## `timestamp.cpp`: game and real-time millisecond timers

Directly after `frametime.cpp` in the binary (`0x5017f0`-`0x501b10`). **Observed:**
statement-level match with Summoner 2's `src/timestamp.cpp`, with `MAX_TIME` =
`0x3ff1a100` (1,072,800,000 ms, about 12.4 days) and `MAX_TIME / 2` = `0x1ff8d080` for
wrap-around comparisons.

| Address | Name | Notes |
|---|---|---|
| `0x5017f0` | `UpdateTicker(int ms)` | Called by `FrameDone`. Previously listed under `frametime.cpp`; Summoner 2 places it in `timestamp.cpp` |
| `0x501840` / `0x501850` | `IncrementPauseCounter` / `DecrementPauseCounter` | `Timestamp_paused += 1` / `-= 1` |
| `0x501880` | `GameTimer::SetTimeout(int ms)` | `this` in `ECX` (MSVC `thiscall`) |
| `0x5018d0` | `GameTimer::SetRandomTimeout(int lo, int hi)` | Uses the C runtime `rand` |
| `0x501910` | `GameTimer::elapsed()` | |
| `0x501940` | `GameTimer::GetRemainingTime()` | |
| `0x501980` | `GameTimer::GetElapsedTime()` | `-GetRemainingTime()` |
| `0x501990` | `GameTimer::valid()` | `value >= 0` |
| `0x501a00`-`0x501b10` | `RealtimeTimer::` the same six methods | Use `Timestamp_realtime_ticker` |

Globals: `Timestamp_ticker` (`0x5a5758`) and `Timestamp_realtime_ticker` (`0x5a575c`), both
initialised to 1 as in the reference (**observed** live in x32dbg), and `Timestamp_paused`
(`0x257f0ac`). These were earlier labelled "game clocks" on the
[main loop page](main-loop.md#game-clocks-gameclockadvance-fun_005017f0).

**Not in the Summoner 2 source:** six one-line methods that set a timer's value to -1
(`0x501860`, `0x501870`, `0x501900`, `0x5019e0`, `0x5019f0`, `0x501a80`). They are probably
out-of-line constructors or a reset method (*inferred*), and are left unnamed until a caller
shows which is which. `0x5019a0` sets a `GameTimer` from a value read through `0x525990`
(a file-reading helper), probably for save games (*inferred*).

## `memory.cpp`: allocation

Around the `memory.cpp` reference at `0x512516`.

| Address | Name | Notes |
|---|---|---|
| `0x5124c0` | `operator new(size, file, line)` (`operator_new_dbg`) | If a mempool is active (`0x2693048`), allocates from it (`0x5127c0`); otherwise `malloc`. On failure, `DebugError` with "Failed to allocate %d bytes" at line `0xdf` (223). **This is the error SummonerFix fixed** for the OpenGL texture buffer |
| `0x512550` | `operator new(size)` | `operator_new_dbg(size, 0, 0)`, as in Summoner 2 |
| `0x512570` | `operator delete(p)` | `free` (`0x56ea9b`) |

Not yet named: `0x512530` (another forwarding overload, probably `operator new[]`), the
mempool functions around `0x5127c0` (Summoner 2's `allocate_in_mempool` and related), and a
debug allocation logger (`0x512600`-`0x512640`: an indent counter and a line builder).

## `stringpool.cpp`

| Address | Name | Notes |
|---|---|---|
| `0x544b80` | `StringPool::Init(char* pool, int max_length, bool allocated)` | Same field layout as Summoner 2: `pool` +0, `allocated` +4, `max_length` +8, `length` +0xC |
| `0x544de0` | `StringPool::StringPool(pool, max_length, allocated)` | Inline constructor that calls `Init` (*inferred* from the forwarding call) |

`bmpman.cpp` has a static initialiser (`0x501c20`) that builds a 64 KB `StringPool` at
`0x2581c74`, presumably for bitmap names.

## `debug`: `DebugError`

`0x531cb0` is `DebugError(const char* File, int Line, const char* Error)`. It uses the same
format string as Summoner 2 (`"Error: %s\nFile: %s - Line: %d"`); the PC version then adds
"A critical error has occurred and the program cannot continue.", shows a `MessageBoxA`
titled `Error!`, and calls `exit(1)`. Every `vsdk` assert goes through it. It produced
the dialogs seen while bringing up SummonerFix.

## `console.cpp`: the developer console

Strings at `0x5a7000`-`0x5a7258` match Summoner 2's `console.cpp` messages exactly, and the
functions that use them follow its structure (**observed**). See
[Console commands](console.md) for the 145 registered commands.

| Address | Name | Evidence |
|---|---|---|
| `0x50a180` | `console_command::console_command(name, description, callback)` | Inserts `this` into `Console_commands` (`0x25db3e4`, at most 300), keeping it sorted with `_stricmp`; increments `current_command_count` (`0x25db918`) |
| `0x50a210` | `execute_command_line` | "Searching for command", "Unknown command", "Calling function", "Checking status for", "Doing help for", "Ignoring the unused command line tail" |
| `0x50a5b0` | `process_command` | "next arg is '%s', was originally '%s'", "Found float number!", "Not enough parameters", "invalid type" |
| `0x50acd0` | `load_script` | "Couldn't find a script file", "Error processing script line %d" |
| `0x50a560` | `set_command_line` | Sets the cursor (`0x5a6f20`), then `get_next_char` |
| `0x50a570` | `get_next_char` | Reads `*cursor` into `current_char` (`0x25db214`) and advances |
| `0x50a9a0` | `get_token` | Dispatches on `char_types[current_char]` to `read_word` / `read_quoted_string` |
| `0x50aa30`, `0x50aa50`, `0x50aab0`, `0x50ab00`, `0x50ab60` | `skip_whitespace`, `make_lowercase_token`, `read_word`, `read_quoted_string`, `init_char_types` | Same roles as Summoner 2's static helpers |
| `0x50a5a0` | `print_to_console` (*inferred*) | Empty in this build |

The PC constructor takes three arguments where Summoner 2's takes four (name, description,
**type**, callback). The command type is therefore decided elsewhere on PC (*unknown*).

### Keyboard state (just before the console)

Three small functions at `0x509e20`-`0x509f00` manage a 256-entry key table under a
critical section (`0x25daa58`). They are named with confidence **I**:

| Address | Name | Behaviour |
|---|---|---|
| `0x509e20` | `key_get_press_count(key)` | Returns and clears `key_press_counts[key]` (`0x25da140`) |
| `0x509ec0` | `key_queue_not_empty()` | Queue head != tail |
| `0x509f00` | `key_is_down(key)` | Returns `key_down_state[key]` (`0x25da94c`) |

These are the most direct hook points found so far for the planned
[custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions).

## C runtime functions identified

`sprintf` (`0x56c740`), `exit` (`0x56da99`), `free` (`0x56ea9b`), `_stricmp` (`0x57ae80`): identified from their
use in the functions above (*observed* behaviour).

## Still to do in this module

- `timer.cpp`: `timer_initialize`, `timer_get_ticks`, `timer_get_elapsed` (Summoner 2
  names) around `0x532840`.
- `registry.cpp` (`0x512037`) and `cmdline.cpp` (`0x53e3ab`): partly named; no Summoner 2
  counterpart (PS2 has neither).
- Console: the key-binding layer between key_* and commands (Timeslip binds keys to commands; see
  [Timeslip's patch](../patches/timeslip.md)).
- Mempool functions.
