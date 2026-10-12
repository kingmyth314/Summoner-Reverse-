---
title: Timing bugs
parent: Sum.exe
nav_order: 2
---

# Timing bugs
{: .no_toc }

1. TOC
{:toc}

## 1. QPC frequency truncation (">4 GHz" bug)

**Status:** Observed in the code. Timeslip's patch fixes it (see
[Timeslip's patch](../patches/timeslip.md#the-4-ghz-fix)).

### The defect

`FrameClockInit` (`FUN_00532840`) keeps only the low 32 bits of the
`QueryPerformanceFrequency` result:

```nasm
0x532853: CALL [QueryPerformanceFrequency]   ; LARGE_INTEGER at [ESP+8]
0x532859: MOV  ECX, [ESP+0x8]                 ; LowPart only - HighPart ignored
0x532868: MOV  [0x02cbffdc], ECX              ; freq_low32
```

`FrameClockRead` (`FUN_005328d0`) then divides by `freq_low32` (`DIV EBX` at `0x5329a2`).

### When it triggers

When the reported QPC frequency is 2^32 Hz (4.295 GHz) or more. On Windows versions or
machines where QPC is backed directly by the CPU's time-stamp counter (common on Windows
XP and 7), the frequency is the CPU clock. Windows 10 1809 and later normally report a
fixed 10 MHz, so modern systems are usually unaffected. *Inferred* from general Windows
behaviour, not tested here.

### Effect

The game believes the clock ticks at `freq - 2^32` Hz. For example, at 4.5 GHz it uses
about 205 MHz, so game time runs about **22 times too fast**. Every user of
`FrameClockRead` is affected: 33 callers, including `FrameTimer`.

### False lead

`FrameClockInit` also measures RDTSC over two `Sleep(250)` calls and does 32-bit signed
arithmetic on the result (`SAR EAX,1` at `0x5328b8`), which overflows above about
4.29 GHz as well. But the result (`0x2cbffb0`) is **written and never read**, so it is
harmless.

## 2. Cutscene timer sign bug (~24.9-day uptime)

**Status:** Inferred from the assembly. Not reproduced in-game. **Not** fixed by
Timeslip's patch.

### The defect

`SequenceTimer` (`FUN_00436a90`, our label) measures elapsed time since a sequence started
using `timeGetTime`, which counts milliseconds since Windows booted:

```nasm
0x436ad9: CALL  [timeGetTime]
0x436adf: MOV   [EBP-0x10], EAX
0x436ae2: MOV   dword ptr [EBP-0xc], 0     ; zero-extend: "now" read as unsigned 64-bit
0x436ae9: FILD  qword ptr [EBP-0x10]
0x436aec: MOV   EDX, [0x00601e78]          ; start time
0x436af5: MOV   dword ptr [EBP-0x14], 0    ; zero-extended copy built here...
0x436afc: FISUB dword ptr [EBP-0x18]       ; ...but FISUB reads a SIGNED 32-bit value
0x436aff: FDIV  float ptr [0x0057ef28]     ; / 1000.0 -> seconds
```

The compiler builds a zero-extended 64-bit copy of the start time but subtracts it with a
32-bit **signed** integer instruction.

### When it triggers

When the start time is 2^31 ms or more, that is when **Windows uptime exceeds about 24.86
days**. Windows Fast Startup does not reset this counter on a normal shutdown, so uptime
can quietly build up for weeks.

### Effect

`start` is read as about -2.1 billion, so `elapsed` comes out at about 4.3 million seconds.
Every sequence event (`0x601e90 + i*0x70`) then looks overdue. `SequenceTimer` fires one
event per frame until the sequence ends, so the cutscene appears to run at "mach speed".

### How this matches user reports

Steam users report the intro cutscene skipping almost instantly. Some said it stopped
after a reboot, a Windows reinstall or an upgrade, and others still had it with
Timeslip's patch installed. All of that fits an uptime-dependent bug in a `timeGetTime`
path that the patch does not hook. Not every report fits: one user only partly improved
it by limiting CPU speed, which this bug does not explain.

### Possible fix (not implemented)

Hook `timeGetTime` the same way Timeslip hooked QPC: redirect the IAT slot `0x57e3fc` to a
function that returns `timeGetTime() - value_at_startup`, so the value stays small.

### Other users of the same start time

`0x601e78` is also referenced at `0x43703f` (`FUN_00436c00`) and `0x49431e`
(`FUN_00494310`). Both may share the bug. **Unknown**; not yet checked.

## 3. Frame limiter double-counts its sleep (2x game speed)

**Status:** Observed at runtime on Windows 11 (2026-10-12). Fixed by
[SummonerFix](../summonerfix/index.md#frame-limiter-2x-speed).

### The defect

`FrameTimer` (`FUN_00501510`) stores `last = now` (`0x501577`) **before** it sleeps
(`CALL SleepMs` at `0x5015bc`). A frame shorter than `MIN_DT` (1/120 s) sleeps the remainder
and reports `dt = MIN_DT`, but the next frame then measures from that pre-sleep timestamp, so
it counts the same sleep again.

### When it triggers

When frames take far less than `MIN_DT`. That is every frame on modern hardware: the menu
frames measured about 0.1-0.6 ms. On 2000-era PCs frames were slower than 8.3 ms, so the
limiter rarely ran.

### Effect

Frames alternate: "sleep, count 8.3 ms" then "don't sleep, count the 8 ms sleep". The result
is 2 x 8.3 ms of game time per 8.3 ms of real time, so **the game runs at double speed**.
Measured with SummonerFix diagnostics: about 218 `FrameTimer` calls/s, about 112 sleeps/s of
about 7.8 ms, while the game believed it ran at about 115 FPS. Players reported the game
"runs super fast".

This corrects the earlier note on the [main loop page](main-loop.md#behaviour-implications),
which said the effect was only that the slept time was "counted in the next frame".

## Evidence

| Claim | REA evidence ID |
|---|---|
| `FrameClockInit` assembly | `ev_6d63a360cc4cfe55f9cdcf1c79c39e0ad6d9d9d1d4db37276460c5986b3801cc` |
| `FrameClockRead` assembly | `ev_2a63b0c40da8d738c1d8775fb3148f6fe942bfb42cf521d23133078825a5ba25` |
| RDTSC result never read (xrefs `0x2cbffb0`) | `ev_06253e9933cdf2b1caf4804d98022349de42794766525ae5cdd74452d41662e6` |
| `SequenceTimer` assembly | `ev_1234f8a94618af3e68f160ac6f5fa65c6c12a2ca359dc6ca3f84e38064599ab5` |
| Start-time xrefs (`0x601e78`) | `ev_d272ed8ae15c026baf2544857bb8f4349de69a965cb7c1c6765cf4bef430ac9f` |
