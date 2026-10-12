---
title: Timeslip's patch
parent: Patches
nav_order: 1
---

# Timeslip's patch (ddsum v3)
{: .no_toc }

1. TOC
{:toc}

## Source

| Property | Value |
|---|---|
| Author | Timeslip (also author of MGE, sfall and the Fallout Mod Manager) |
| Archive | `ddsum.7z`, 39,783 bytes |
| Archive SHA-256 | `1B8FBA997CD94317EE0D80457329EF1303120FFF924F7B46725C9DA65BBF2208` |
| Retrieved from | Internet Archive capture (2015-07-23) of the author's site: `http://timeslip.users.sourceforge.net/current/ddsum.7z` |
| Original site | `timeslip.users.sourceforge.net` now redirects to a SourceForge profile without the file |

| File | Size | Date | SHA-256 (first 16) |
|---|---|---|---|
| `dinput.dll` | 90,112 | 2009-10-03 | `06B9BEB070053840` |
| `dinput.ini` | 924 | 2009-10-03 | `9E5145EBEC369244` |
| `ddsum readme.txt` | 719 | 2009-10-03 | `00364D256F868266` |
| `console_commands.txt` | 6,678 | 2009-09-30 | `5148081FBCFC9471` |

Full `dinput.dll` SHA-256: `06b9beb070053840d9994b96949fb60122559f3bd82b8a3cd6da6a3dd66e91f9`.

## Documented features (from the readme)

- **v2:** fixes Bink movie and savegame image corruption in 32-bit mode; non-standard
  resolutions; an option to skip the opening movies.
- **v3:** a gameplay speed option; "Fixed an issue in the original game that could break
  its timing code on >4GHz processors"; camera zoom range; key bindings for console
  commands; interaction distance options.

## How it hooks the game

`dinput.dll` is a DirectInput proxy that Sum.exe loads from its own folder. It exports
`DirectInputCreateA` (`0x1000a640`). Its `DllMain` (`FUN_1000aee0`) runs on process
attach and patches Sum.exe in memory with `VirtualProtect`. Nothing is written to disk.

**Observed** patch list:

| Sum.exe address | Change | Purpose |
|---|---|---|
| `0x53983f` (read) | Must equal `0x8B52106A`, otherwise "Incorrect summoner version" and exit | v1.40 check |
| `0x57e1c8` | IAT `QueryPerformanceCounter` -> `FUN_1000aa00` | Virtual clock and speed control |
| `0x57e1cc` | IAT `QueryPerformanceFrequency` -> `FUN_1000a750` | >4 GHz fix |
| `0x43e9cb` | `7D` (JGE) -> `EB` (JMP) | **Removes the CD check** (not in the readme) |
| `0x5173d8` | 4 x `NOP` when `DisableBikVideo=1` | Skip opening movies |
| `0x539840` | byte -> `0x20` | *Inferred:* 32-bit Bink fix |
| `0x5174f2` | dword -> `0x80000003` | *Inferred:* 32-bit Bink or save-image fix |
| `0x5398db` | dword -> `0x0FAD15F1` | *Inferred:* 32-bit Bink or save-image fix |
| `0x59b800` | float -> 3.0 when `IncreaseCameraZoomRange=1` | Camera zoom range |
| `0x43ad38` | pointer -> patch-owned float (value / 10) | `BoostMouseOverRange` |
| `0x40e696` | pointer -> patch-owned float (value / 10) | `BoostInteractionRange` |
| `0x580e48`, `0x580e4c` | screen width and height | Custom resolution, plus code patches at `0x43d85f` and `0x43d878` |

The rows marked *Inferred* were not cross-checked against the Sum.exe code at those
addresses.

## The >4 GHz fix

**Observed.** The replacement `QueryPerformanceFrequency` (`FUN_1000a750`):

```c
BOOL Hook_QPF(LARGE_INTEGER *freq) {
    BOOL ok = QueryPerformanceFrequency(freq);
    QueryPerformanceCounter(&real_last);       // seed for Hook_QPC's delta (unshifted)
    shift = 0;
    while (freq->HighPart != 0) {              // halve until it fits in 32 bits
        *freq = *freq / 2;
        shift++;
    }
    return ok;
}
```

The replacement `QueryPerformanceCounter` shifts every reading right by the same `shift`,
so counter and frequency stay consistent. Sum.exe's truncating code (see
[Timing bugs](../sum-exe/timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug)) is left
as it is; it simply never receives a frequency with a non-zero high word.

## The speed control

**Observed.** The replacement `QueryPerformanceCounter` (`FUN_1000aa00`) does not return the
real counter. It returns a **virtual clock**:

```c
BOOL Hook_QPC(LARGE_INTEGER *out) {
    poll_console_hotkeys();                         // ModKey + k### bindings
    if (ResetGameplaySpeed) {                       // edge-triggered key handling
        if (key(Reset))    speed = 100;
        if (key(Increase)) speed = min(speed + 10, 200);
        if (key(Decrease)) speed = max(speed - 10, 50);
    }
    BOOL ok = QueryPerformanceCounter(out);
    real = *out >> shift;
    delta = real - real_last;  real_last = real;
    if (speed != 100) delta = delta * speed / 100;
    virt += delta;
    *out = virt;
    return ok;
}
```

Because `FrameTimer` derives `frame_dt` from this counter, the whole game speeds up or slows
down in 10 % steps between 50 % and 200 %. The 1/120 s minimum and 0.25 s maximum still
apply to the scaled time.

{: .important }
This corrects an earlier guess. The patch does **not** use the game's built-in
`time_scale` (`0x5a5748`), which stays at 1.0.

Two more things happen inside the hook: calls into game code at `0x477780` and `0x43b1d0`.
Ghidra's decompilation of them is unreliable (it mixes up the register calling
convention), so their purpose is **unknown**.

## What the patch does not change

- `timeGetTime` is not hooked, so the [cutscene uptime bug](../sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime)
  remains, and the speed keys do not affect `SequenceTimer`.
- The frame limiter and clamp constants are untouched.

## Default `dinput.ini`

```ini
[Main]
;ScreenWidth=1280
;ScreenHeight=1024
DisableBikVideo=0
ResetGameplaySpeed=192    ; '
IncreaseGameplaySpeed=221 ; ]
DecreaseGameplaySpeed=219 ; [
IncreaseCameraZoomRange=1
BoostMouseOverRange=0
BoostInteractionRange=0
[console]
ModKey=0
k009="add_gold 1000"
k070=show_framerate
```

`k070` (the F key) runs the game's own `show_framerate` console command. The FPS counter
is part of the original game; the patch only adds the key binding.

## Evidence

| Claim | REA evidence ID |
|---|---|
| Patch import list (QPC, QPF, VirtualProtect, GetAsyncKeyState) | `ev_de20c77b4f5f68613575a7e3fe14ef3828917fedf07fc5893bbbdd1143091859` |
| `DllMain` patch list and `Hook_QPF` | `ev_b09b433595fc8d2b4322ef35f32b9e3e4a4c975e78efa43094123d62023c62a3` |
| `Hook_QPC` | `ev_025d75ee145f9d5c4decf021ba4e26e44d584480efb9b884d779ce8f61bb861c` |
| CD-check function in Sum.exe (`FUN_0043e9b0`) | `ev_f6f9548e9e6050af232d21c1bb27b8881d20f964226d0dea400ed5e87a259035` |
| Bytes at `0x43e9cb` (`7D 34`) | `ev_ec57b49df3bf0054ccb34477706b9e901cc31883514faeba51698654a19b18b9` |
