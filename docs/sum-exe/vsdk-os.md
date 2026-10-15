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

| `0x512530` | `operator_new_array_dbg` (*inferred*) | Forwards `(size, file, line)`; its only caller is `allocate_in_commonblock` |

### Mempools and the common block

These follow Summoner 2's `memory.cpp` in order and behaviour (**observed**), with PC differences:

| Address | Name | PC difference from Summoner 2 |
|---|---|---|
| `0x512770` | `Mempool::Copy(dest, size, name)` | No alignment argument; name copied to +0xC |
| `0x5127c0` | `allocate_in_mempool(size)` | `thiscall` on the pool; alignment fixed at 16 bytes; returns NULL **without** an error message |
| `0x5127f0` / `0x512800` / `0x512810` | `set_current_mempool` / `get_current_mempool` / `is_mempool_active` | Same; `Pool` is `0x2693048` |
| `0x512820` | `allocate_in_commonblock(size)` | Same "Common block is already locked." check, but the block is **heap-allocated** (`new` at line 600) instead of carved from a static 0x47000-byte array |
| `0x512880` | `deallocate_from_commonblock(p)` | Frees the block |

Globals: `Common_block_index` (`0x2693050`) and a two-slot `Common_block_allocations`
(`0x2693054`). `TextFile::Open` (`0x515d20`) reads its file into a common-block buffer, as
in Summoner 2's `parse.cpp`. That confirms the `Open` placement from the
[parser page](vsdk-parse.md).

PC layout of `Mempool` (from `Copy` and `allocate_in_mempool`): `destination` +0,
`size` +4, `offset` +8, `name` +0xC.

### Memory-usage report (disabled)

`0x512600`-`0x512640` print an indented memory report (`mem_report_indent`,
`mem_report_unindent`, `mem_report_is_verbose`, `mem_report_line`). One caller is
`0x51b4a0`, which reports an animation's "cur time", "anim pct", "bone info" and "TOTAL" sizes.
The output file (`0x269303c`) and the verbose flag (`0x2693044`) are never written. The
report is therefore compiled in but always off (*observed*: no write references).
`0x56f285` (`vsprintf`) and `0x56e4dd` (`fprintf`) were identified here.

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

### Crash reports: `errorlog.txt`

`WinMain` (`0x43e5e0`) runs `MainLoop` inside `__try`/`__except`. The filter calls
`RecordExceptionInfo(GetExceptionInformation(), "Summoner Main Thread")` (`0x5419a0`), which
**appends a crash report to `errorlog.txt` in the game folder**. The report contains:
- the exception type and address;
- registers, the 16 bytes at EIP ("??" for unreadable ones) and a stack dump of at most 8 KB (from ESP to
  ESP + 0x2000, or to the stack top `fs:[4]` when that is lower, `0x541d21`..`0x541d3b`);
- a module list with load addresses and sizes;
- the time, the user, processors and memory.

Only the first exception is reported (static flag `0x2ee250c`, `0x5419c9`..`0x5419d6`). The filter returns
`EXCEPTION_CONTINUE_SEARCH` (0) on every path (`xor eax, eax` at `0x541a99` and `0x541ea9`; the two
`mov eax, 1` at `0x541cd0` and `0x541e75` are its own inner `__except` filters), as Dawson's original
does. So `WinMain`'s `__except` block never runs: after the report the exception goes on to Windows' own
crash handling, and `WinMain` does not return -1. **Observed.**

The code and strings match Bruce Dawson's public `ExceptionHandler.cpp` (Game Developer
magazine, 1999), so its function names are used:
- `RecordExceptionInfo`, `hprintf`, `RecordModuleList`, `ShowModuleInfo`;
- `PrintTime`, `RecordSystemInformation`, `GetExceptionDescription`, `GetFilePart`
  (`0x541ec0`-`0x5424e0`).

When a test crashes, check `Summoner\errorlog.txt` first. None existed on 2026-10-13. The
Direct3D failure path is not a crash: it calls `exit(0)`
([SummonerFix notes](../summonerfix/index.md#direct3d-silent-exit-not-fixed)).

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

## `registry.cpp`: settings in the Windows registry

No Summoner 2 counterpart (the PS2 has no registry). Names are ours (**O**), from behaviour
and the error strings.

| Address | Name | Notes |
|---|---|---|
| `0x512110` | `os_registry_set_app_key(company, app)` | Builds `Software<company><app>` in `os_registry_app_key` (`0x2692ef8`) |
| `0x511e50` | `os_registry_resolve_key(out, subkey)` | A NULL subkey means the app key under `HKEY_CURRENT_USER`. A subkey without an `hkey` prefix is relative to the app key. Otherwise `HKEY_...path` is split |
| `0x511f90` | `os_registry_hkey_from_string` | The four root names; anything else is a fatal "Bogus HKEY string" |
| `0x512050` | `os_registry_read(subkey, name, buf, size)` | `RegOpenKeyExA` + `RegQueryValueExA`; -1 on failure (the error text is formatted but never shown) |
| `0x511d80` | `os_registry_write(subkey, name, type, data, size)` | `RegCreateKeyExA` + `RegSetValueExA` |
| `0x512280` / `0x512210` | `os_registry_read_string` (with default) / `_nodefault` | |
| `0x512340` / `0x512370` | `os_registry_read_dword` / `os_registry_write_dword` | |
| `0x512310` | `os_registry_write_string` | `REG_SZ` |

[SummonerFix](../summonerfix/index.md) emulates these keys by hooking the registry imports.

## `cmdline.cpp`: command-line parameters

This is the same system as Red Faction's: `CmdLineParam` objects link themselves into a list
when constructed. Their layout matches Dash Faction's `rf::CmdLineParam` field for field:
`next`, `prev`, `name`, `help`, `arg`, `name_found` (+0x14), `has_argument` (+0x15).
`cmdline_args` (`0x2dc6ea8`) is `CmdArg[50]`, also as in Dash Faction (**observed**).

| Address | Name | Notes |
|---|---|---|
| `0x53e3e0` | `CmdLineParam::CmdLineParam(name, help, has_argument)` | Links into `cmdline_params` (`0x2dc7038`) |
| `0x53e030` | `cmdline_parse` | Split, match, then optionally reject unknown `-x` |
| `0x53e060` | `cmdline_split` | `strtok` on spaces; keeps `"quoted arguments"` together |
| `0x53e1e0` / `0x53e250` | `cmdline_match_params` / `cmdline_take_argument` | "Command line parameter %s requires an argument" |
| `0x53e310` | `cmdline_check_unknown` | Fatal "Unrecognized command line parameter" |
| `0x53e4a0` / `0x53e4b0` | `CmdLineParam::found` / `get_arg_int` | |

Parameters registered in Sum.exe (found by scanning for constructor calls):

| Parameter | Object | Registered in |
|---|---|---|
| `-port` (with argument) | `0x60aca8` | game startup (`0x43d241`) |
| `-directsound3d`, `-eax`, `-dsinit` | `0x60ac38`, `0x60abf8`, `0x60ac90` | game startup |
| `-pack` | `0x60ac68` | game startup |
| `-glide`, `-direct3d` | `0x60ad50`, `0x60abb8` | game startup: renderer choice |
| `-800`, `-1024` | `0x60ad18`, `0x60ad30` | game startup: resolution |
| `-nosound` | `0x2cb5440` | sound (`0x5204e1`) |
| `-fb2`, `-vertexfog`, `-rz` (with argument) | `0x2dc6c10`, `0x2dc05e0`, `0x2dc2908` | renderer (`0x538671`-`0x53870e`) |

What each parameter does is not yet traced (*unknown*); `-glide`/`-direct3d` and
`-800`/`-1024` probably override the [video settings](video-settings.md) (*inferred*).

## Networking (`0x517590`-`0x5194ab`)

Summoner shipped without multiplayer, but its engine has a complete UDP layer with
"reliable sockets", and the game code uses it. Names are by behaviour (**O**), apart from
five functions that name themselves in their messages (`nw_NewReceiveReliable`,
`nw_NewSendReliable`, `nw_NewCloseSocket`, `net_get_reliable_socket_status`,
`net_get_reliable_socket_ping`) and `nw_ConnectToServer`. The reliable layer's messages also use the
older names `nw_WorkReliable`, `nw_SendReliable` and `nw_ReceiveReliable`, and `nw_send` reports errors
as `psnet_send` (the Descent 3 / FreeSpace psnet names). The decompiled unit is
`src/decomp/vsdk/unnamed_net_0x517590.cpp` (placeholder name).

{: .important }
Corrected 2026-10-15: the unit spans `0x517590`-`0x5194ab` (it was given as `0x5175e0`-`0x519370`), and
only the "slow" and "56k" settings load RAS.

- **Start-up:** `nw_init_networking` (`0x5188d0`) is called by `LoadSettings` at start-up.
  - It reads `ConnectionSpeed` from the registry (default "None"). "none", "slow", "56k" and "fast"
    (case-insensitive) set `nw_network_status` (`0x270812c`) to 0, 1, 2 and 3; any other value counts
    as "none". With 0 it stops there: no WinSock and no socket.
  - Only "slow" and "56k" load the RAS (dial-up) API (`nw_ras_init`, `0x5178c0`), which returns the
    dial-up connection's IP address, or 0 when there is none, into `nw_networking_ok` (`0x2710e9c`).
    With 1 or 2 and no dial-up connection, the multiplayer button shows L948 "Internet connection not
    detected" (`mainmenu_check_network` `0x41dae0`). "fast" skips RAS.
  - It calls `WSAStartup(1.1)` and opens one UDP socket on the [`-port`](#cmdlinecpp-command-line-parameters) port
    (`nw_init_socket`, `0x517c10`; port 4200 when the value is 1023 or less, or 65535). The messages say
    "TCP", but the socket is `SOCK_DGRAM` / `IPPROTO_UDP`.
- **Traffic:** `nw_receive_all` (`0x518770`) sorts incoming datagrams into per-type buffers
  by their first byte. There are five buffers of 0x60 packets (`nw_recv_buffers`, `0x26b63f0`,
  0x1050c bytes each).
- **Reliable sockets:** `nw_WorkReliable` (`0x517e00`) runs the reliable protocol:
  - connection requests (type 3, version 0x142) and the "IAMHERE" (type 6) / ACK (type 0) handshake;
  - data (types 1 and 2) with 16-bit sequence numbers, 150 buffers of 600 bytes per link, ACKs that
    carry the acknowledged sequence number and the sender's time (pings are the median of the last 10);
  - heartbeats (type 5) and resends, and time-outs after 100 s without a packet.
  - The table `nw_reliable_sockets` (`0x2708ad0`) has 10 records of 0xd2d bytes; slots 1 to 9 are used.
- **Game side:** `multi_process_incoming` (`0x441b10`) and `multi_send_packet` (`0x442060`,
  which keeps per-type "net rate info") sit on top. `multi_send_packet` always sends reliably:
  it appends the packet to the receiver's buffer (`multi_reliable_send_bufs`, `0x65d8b0`) and
  sends that buffer with `nw_NewSendReliable` when the packet would take it past 0x200 bytes,
  or at once when its last argument is set (`0x442178`, `0x4421e6`). The unreliable path is
  `multi_send_packet_buffered` (`0x441e90`, `nw_send`). The `net_stats` console command shows
  the statistics.
- **Imports:** WinSock is imported by ordinal. The import thunks at `0x57b4b0`-`0x57b5ca`
  are now named (`socket`, `sendto`, `recvfrom`, `select`, …), along with the DirectInput,
  DirectSound, ACM and GLU thunks.

This is the remains of a **multiplayer mode**. `strings.tbl` names multiplayer source files
(`multichat.cpp`, `multitrade.cpp`, `new_gamelist.cpp`, `new_pxochat.cpp`, …); see the
[source map](source-map.md#more-file-names-from-stringstbl). Whether any of it can still be
reached in the retail build is *unknown*; testing it would need a live run with `-port`.

## OS window and single instance (`0x530b90`-`0x531c0f`)

The decompiled units are `src/decomp/vsdk/unnamed_os_0x530b90.cpp` and `unnamed_os_0x531bf0.cpp`
(placeholder names; no file-name evidence). Names follow Dash Faction where it has one (`main_wnd`,
`os_foreground`, `os_add_msg_handler`, `os_poll`), otherwise they are ours (**O**). **Observed:**

- `os_init` (`0x530d10`, from `LoadSettings`) copies its first argument as the mutex name and the window
  class name, and its second as the window title (128-byte buffers). If `OpenMutexA` finds the mutex, or
  `CreateMutexA` reports `ERROR_ALREADY_EXISTS`, it brings the running copy's window to the front with
  `FindWindowA` / `SetForegroundWindow` and calls `exit(1)`. Otherwise it calls `CoInitialize`, creates
  the window (`os_create_window`, `0x530ef0`: a 32x32 popup with `os_wndproc`, icon "APP_ICON") and runs
  `os_find_previous_instance` (`0x530e40`, an `EnumWindows` search for "[run] - <exe name> -"), whose
  result `0x2cbff0c` nothing reads.
- `os_wndproc` (`0x530ff0`) calls the registered message handlers first (mouse, keyboard and Glide
  register one each through `os_add_msg_handler`, 32 slots). On activation it sets `is_main_wnd_active`
  and calls `SetWindowPos` (see [Controls](controls.md)) and `SetThreadPriority` (highest when active,
  normal when not) on a thread handle `0x2cbff14` that nothing sets, so the priority call always fails.
- `os_close_main_window` (`0x531260`) releases the instance mutex and posts `WM_DESTROY`;
  `os_seed_random` (`0x531280`) seeds `rand` from `time(0)` for -1.
- `timer_wait` (`0x530bf0`, a `Sleep` wrapper) sits inside this object. Its last part (CPU detection,
  the `sysinfo` console command, `os_clipboard_get_text` `0x531b44`) is compiled without optimisation;
  `os_clipboard_get_text` has no caller.

## C runtime functions identified

`sprintf` (`0x56c740`), `exit` (`0x56da99`), `free` (`0x56ea9b`), `_stricmp` (`0x57ae80`), `strtok` (`0x56d5b7`), `atoi` (`0x56e311`): identified from their
use in the functions above (*observed* behaviour).

## Still to do in this module

- `timer.cpp`: `timer_init` (`0x532840`), `timer_get` (`0x5328d0`) and `timer_wait` (`0x530bf0`) are named (see [main loop](main-loop.md)); the rest of the file is still unread.
- The six -1 setters in `timestamp.cpp` and the PC console command type mechanism.
- Console: the key-binding layer between key_* and commands (Timeslip binds keys to commands; see
  [Timeslip's patch](../patches/timeslip.md)).
