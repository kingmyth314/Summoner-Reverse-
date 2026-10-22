# Experimental build, 2026-10-21: the feature workers' notes

Written by four workers from the SummonerFix code and the decompiled source (docs/experimental-build.md); merged as written, except that the lead added the autosave Load/Save screen changes and the SummonerFix controller forwarding. The dgVoodoo research is docs/future/dgvoodoo.md.

## controller

### Controller tier 1 in the source (experimental build): notes

Feature: `[Controller]` (SummonerFix's controller support, written into the exe's source). Only file changed:
`src/experimental/controller.cpp` (the stub replaced). No game file, header or ini changed.

Compiles with `fixed-compile.mjs ... experimental/controller.cpp --experimental`: one diagnostic, the existing
`include/vsdk/math/vector.h(73) C4244` that every file including `summoner/summoner.h` gets (the GameTick unit shows
the same one). No new warnings.

## What it does

The same behaviour and ini keys as SummonerFix's `controller.c` / `padmap.h` (spec), but on the game's own input
path instead of hooks:

- **Where it runs**: `exp_controller_frame` is called by `exp_frame_begin` in `GameTick` after `mouse_update` and
  `mouse_get_pos`, before the multiplayer frame and the state's frame handler: game thread, every frame, any
  renderer (SummonerFix needs the OpenGL SwapBuffers hook; this does not).
- **Cursor** (`move_cursor`): left stick, radial dead zone, speed linear in the deflection beyond it, in game pixels
  per second; dt from QueryPerformanceCounter capped at 50 ms. The position is kept as a double (sub-pixel movement
  adds up), clamped to `0..mouse_screen_w-1` / `0..mouse_screen_h-1` (outside game play, state != 4, to 1024 wide:
  the menu UI is 1024 wide on wider screens). The new position is written into `mouse_x/mouse_y` and
  `game_mouse_x/game_mouse_y` (so this frame's handler sees it) and sent through the game's `mouse_set_pos`, which
  moves the Windows cursor (only in the foreground). The next `mouse_update` reads it back, so the real mouse simply
  carries on from there: no override state as in SummonerFix. If the game's position differs from the last one
  written by more than 1 pixel (rounding through the window scale), the mouse or agent mode moved it and the stick
  restarts from there.
  Why `mouse_set_pos` and not `SetCursorPos`: SummonerFix's ClientToScreen hook maps game pixels to the scaled
  window only for calls from 0x500b00..0x500d00 (the mouse unit), and with agent mode's cursor set it moves the agent
  cursor along. So it works scaled, widescreen, and under agent mode.
- **Keys** (`send`): `key_mark(code, down, 0)` as the window messages do, with `key_layout` set to 0 around the call
  (game thread, as the message handler) so key_remap_layout's AZERTY / QWERTZ swaps do not change the key the pad
  sends (SummonerFix's `g_injecting` does the same). `extern int key_layout;` is declared in controller.cpp (not in
  key.h).
- **Mouse buttons**: `mouse_mark_button(button, down)` writes the live record; mouse_update latches it at the start
  of the next frame, as for a real click (one frame later, as with SummonerFix's SwapBuffers poll). Calling it
  directly also skips SummonerFix's `[KeyBindings]` mouse redirect (wanted: the pad sends game buttons).
- **Right stick**: holds the camera keys (Left/Right/Up/Down by default) past `CameraThreshold`, released below 3/4
  of it (no chatter).
- **Edge detection** over one 32-bit down word (14 XInput buttons with their own bits, LT 0x10000, RT 0x20000,
  right stick 0x40000..0x200000); only changes send anything. Order per frame: other releases, modifier releases,
  modifier presses, other presses (LT+B gives Shift down before the right click, released after).
- **Idle**: stick in the dead zone and nothing held: nothing is touched. With `Enabled=0` the frame function returns
  at its first line (unless agent mode set a synthetic state, see below), so the game's input is exactly the fixed
  build's.
- **XInput**: `LoadLibraryA` xinput1_4, xinput1_3, xinput9_1_0 and `GetProcAddress("XInputGetState")`; the structs
  are declared in the file (no header, no import library). Only loaded with `Enabled=1`. With no controller the four
  slots are probed once a second. A real pad counts only while `os_foreground()`; otherwise, on disconnect, or when a
  synthetic state ends, whatever is held is released on the next frame. XInput only: a PS5 DualSense does not show
  up (as with SummonerFix).
- **Mapping**: the 20 inputs and defaults of padmap.h and the full key-name table of keynames.h are copied into
  controller.cpp (both headers live in src/summonerfix and keynames.h does not compile under VC6's old `for` scope).
  Targets: any keynames.h name, `0xNN`, MouseLeft/MouseRight/MouseMiddle or `none`; Mouse4/Mouse5 or an unknown name
  falls back to the default with a log line.
- **Agent mode test input**: two exports from the exe (`extern "C" __declspec(dllexport) __cdecl`):
  `void ExpControllerSetSynthetic(int lx, int ly, int rx, int ry, unsigned buttons, unsigned long ms)` and
  `int ExpControllerSyntheticDone(void)`, same meaning as SummonerFix's `ControllerSetSynthetic` /
  `ControllerSyntheticDone` (buttons: wButtons bits, 0x10000 LT, 0x20000 RT). Any thread (spin lock); works with
  `Enabled=0` (the mapping is always read at init). Done returns 0 until the first frame ran.
- **Marker**: `// EXPERIMENTAL_BUILD: controller Controller` once, so `Sum-exp.fixes` lists `Controller` and
  SummonerFix's `ControllerInstall` stands down.

Functions: `pad_key_parse` (keynames.h KeyParse), `parse_target`, `load_config`, `load_xinput`, `send`,
`is_modifier`, `move_cursor`, `read_real_pad`, `read_synthetic`, `axis_held`, `exp_controller_init` (config, XInput,
log), `exp_controller_frame` (the per-frame work above), the two exports.

## ini keys ([Controller] in summonerfix.ini, unchanged; read once at the first frame)

| Key | Default | Meaning |
|---|---|---|
| Enabled | 0 | 1 = read an XInput controller |
| DeadZone | 7849 | left stick radial dead zone (0-30000) |
| CursorSpeed | 800 | game pixels per second at full deflection (50-5000) |
| CameraThreshold | 16000 | right stick deflection that holds a camera key (1000-32000) |
| TriggerThreshold | 30 | trigger press threshold (1-254) |
| A B X Y LB RB LT RT Start Back DPadUp DPadRight DPadDown DPadLeft LS RS CameraLeft CameraRight CameraUp CameraDown | MouseLeft MouseRight X I Tab H LShift Space Esc M 1 2 3 4 C F12 Left Right Up Down | what each input presses |

## What the lead must do

1. **SummonerFix (controller.c), or agent `pad` tests SummonerFix's code, not the exe's.** Today, when
   `RebuiltFixInSource("Controller")` is true, `ControllerInstall` only clears `g_enabled`, but `ControllerFrame`
   still plays the agent's synthetic state (it does not check `g_enabled`). Proposed change:
   ```c
   typedef void(__cdecl *PFN_ExpSetSynth)(int, int, int, int, unsigned, unsigned long);
   typedef int(__cdecl *PFN_ExpSynthDone)(void);
   static PFN_ExpSetSynth g_expSetSynth;  // the experimental exe's own controller (src/decomp/experimental/controller.cpp)
   static PFN_ExpSynthDone g_expSynthDone;
   static int g_inSource;
   ```
   In `ControllerInstall`, right after `LoadConfig();` (before the `if (!g_enabled)` return, since `pad` works with
   Enabled=0), move the stand-down there:
   ```c
   if (RebuiltFixInSource("Controller")) {
       HMODULE exe = GetModuleHandleA(NULL);
       g_expSetSynth = (PFN_ExpSetSynth)GetProcAddress(exe, "ExpControllerSetSynthetic");
       g_expSynthDone = (PFN_ExpSynthDone)GetProcAddress(exe, "ExpControllerSyntheticDone");
       g_inSource = 1;
       g_enabled = 0;
       Log("controller: in this exe's source (EXPERIMENTAL_BUILD), not driven by SummonerFix; agent pad %s",
           g_expSetSynth && g_expSynthDone ? "forwarded to the exe" : "not available (exports missing)");
       return;
   }
   ```
   First line of `ControllerFrame`: `if (g_inSource) return;`. First line of `ControllerSetSynthetic`:
   `if (g_inSource) { if (g_expSetSynth) g_expSetSynth(lx, ly, rx, ry, buttons, ms); return; }`. First line of
   `ControllerSyntheticDone`: `if (g_inSource) return g_expSynthDone ? g_expSynthDone() != 0 : TRUE;`.
   (The stand-down only happens when both `Sum-exp.addr` and `Sum-exp.fixes` sit next to the exe.)
2. **Check the exports after linking**: `dumpbin /exports Sum-exp.exe` should list `ExpControllerSetSynthetic` and
   `ExpControllerSyntheticDone` (undecorated, `__cdecl`). Linking an exe with exports also writes `Sum-exp.lib` and
   `Sum-exp.exp` next to it in build/link-exp; harmless, but if link-build copies or checks the folder, ignore them.
3. Optional: SummonerFix's summonerfix.ini comment for `[Controller]` says "Needs the OpenGL renderer" and "Not yet
   tested with a real controller"; for the experimental exe neither holds. Suggested text for the first comment line:
   `; An XInput controller (Xbox-style gamepad) drives the mouse and keys. SummonerFix: needs the OpenGL renderer;`
   `; Sum-exp.exe reads it itself with any renderer. Read when the game starts. 1 = on.`
4. Docs: docs/experimental-build.md table row "Controller, tier 1": status "written, not yet run".

## Differences from SummonerFix, risks

- **[KeyBindings] still apply to the pad's keys** when SummonerFix's key bindings are installed in Sum-exp.exe:
  key_mark's call to key_remap_layout is redirected to SummonerFix's KeyRemap, which applies the binding table unless
  SummonerFix itself is injecting. The source cannot set that flag. With no `[KeyBindings]` entries nothing changes.
  Fix if wanted: SummonerFix's KeyRemap could pass codes through while a flag exported by the exe is set, or the exe
  could call an exported SummonerFix function; not done.
- Cursor movement reaches the game through the Windows cursor: if the window is not in front, `mouse_set_pos` does
  nothing and the next frame snaps back (a real pad is ignored then anyway).
- In game, with SummonerFix's CenterMenus and a full-screen panel open on a wide screen, the cursor can go right of
  the 1024 panel area (SummonerFix's version clamps to the picture instead). Minor.
- Timing: keys take effect at once (key_down_state), mouse buttons one frame later (latched by the next
  mouse_update, which runs before this hook). For modifier + mouse button (LT+B) the press must span at least two
  frames, or Shift is already released when the handler first sees the click. SummonerFix polls after the handler,
  so a one-frame press works there. Real pads and `pad ... 200` are fine. (agent `peek` addresses are Sum.exe
  addresses, translated through Sum-exp.addr.)
- Two inputs on the same target: releasing one releases the target (as in SummonerFix).
- No live reload of `[Controller]` (as in SummonerFix).
- Not tested on a host or in game.

## IN-GAME TEST PLAN (agent mode, tools/agent.mjs; needs item 1 above for the `pad` steps)

Setup: Sum-exp.exe with Sum-exp.addr / Sum-exp.fixes, `[Debug] Agent=1`, `[Debug] ExperimentalLog=1`,
`[Controller] Enabled=1`, a test save. Look in summonerfix.log for "controller: in this exe's source ... agent pad
forwarded to the exe" (SummonerFix stood down), and in experimental.log for the exe's lines.

1. **Start-up**: experimental.log has `controller: using xinput1_4.dll`, `controller: on (dead zone 7849, cursor speed
   800, camera threshold 16000, trigger threshold 30)`, and, with no pad plugged in, `controller: none connected
   (checking once a second)` once. Pass: these lines, no crash.
2. **Enabled=0**: restart with `Enabled=0`: `controller: off ([Controller] Enabled=0)`; normal agent `move`, `click`,
   `key` behave as before. Pass: identical behaviour, no other controller lines. `pad 0 0 0 0 8000` still opens the
   inventory (the synthetic state is played with Enabled=0 too; nothing else is).
3. **Cursor** (Enabled=1): `move 256 192; info` then `pad 32767 0 0 0 0 500; info`. Pass: the cursor moved right by
   about 400 game pixels (`peek 5fbfdc 8` = game_mouse_x / y: x grew by about 400; `info` shows the agent cursor
   moved by the window-scaled amount). Then `pad 0 32767 0 0 0 250` (up about 200), `pad -20000 -20000 0 0 0 300`
   (left-down, slower), `pad 5000 5000 0 0 0 300` (inside the dead zone: no change). `pad 32767 0 0 0 0 3000` in game:
   the cursor stops at the right edge and the camera turns (edge turning). On the main menu with a wide resolution,
   the cursor stops at x 1023.
4. **Real mouse takes over** (by hand, outside agent mode, with a pad): move with the stick, then the mouse: no jump.
5. **Buttons** (capture before and after each; experimental.log shows `controller: <input> down` / `up`):
   `pad 0 0 0 0 1000` (A: leader walks to the cursor), `pad 0 0 0 0 8000` (Y: inventory; again to close),
   `pad 0 0 0 0 10` (Start: play menu), `pad 0 0 0 0 20` (Back: automap), `pad 0 0 0 0 100` (LB: next member),
   `pad 0 0 0 0 10100` (LT+LB: previous member, Shift+Tab), `pad 0 0 0 0 200` (RB: solo), `pad 0 0 0 0 20000`
   (RT: pause), `pad 0 0 0 0 1` (D-pad up: quick slot 1), `pad 0 0 0 0 40` (LS: spells), `pad 0 0 0 0 80` (RS:
   camera mode), `pad 0 0 0 0 4000` (X: interact near a door or chest). Pass: each does what the key does.
6. **Chain attack**: cursor on an enemy, `pad 0 0 0 0 12000 200` (LT+B). Pass: the log shows `LT down`, `B down`,
   `B up`, `LT up` in that order and chain slot 1 starts; `pad 0 0 0 0 2000 200` (B) the default chain.
7. **Camera**: `pad 0 0 -32768 0 0 400`, `pad 0 0 32767 0 0 400` (yaw at `0x60a018+0x180` changes),
   `pad 0 0 0 32767 0 400` (zoom `+0x184` goes down), `pad 0 0 0 -32768 0 400` (up). Pass: the values change and stop
   changing after the pad state ends (key released).
8. **Long hold**: `pad 0 0 0 0 1000 1500` (A held 1.5 s: the leader keeps following the cursor), then the leader
   stops. Pass: `A up` logged once after 1.5 s.
9. **Layout** (only if a non-zero layout occurs: `peek 25daa50 4` = key_layout; LoadSettings passes 0, so usually it
   is 0 and this step is skipped): with key_layout 2 (AZERTY), `pad 0 0 0 0 20` (Back = M) still opens the automap
   (the key is not swapped to comma), and `peek 25daa50 4` is unchanged afterwards.
10. **Other renderer**: `[Display] Renderer` set to Direct3D (or Glide/dgVoodoo) and a real pad: cursor and buttons
    work (the poll does not depend on SummonerFix's OpenGL hooks); agent mode itself may need OpenGL.
11. **Real controller** (Xbox-style): connect / disconnect lines; Alt+Tab with a button held (released, `up` logged);
    unplug with a button held (released); triggers half pressed; feel of speed and dead zone; CPU with no pad.

## tentacle

### tentacle: the Tentacle Beast grab and throw (G250, G251) in the source

Spec: SummonerFix `src/summonerfix/tentacle.c` (every patch and stub, with the slip fixes listed at its top) and
`docs/future/tentacle-grab.md` section 7.3. The code follows tentacle.c step for step; only the form changed (C++
on the decompiled structs instead of offsets and function pointers).

## The one file changed

`src/game/unnamed_tentacle_beast_0x4c6480.cpp` (no other game file, no experimental\ file, no header).

All additions are under `#ifdef EXPERIMENTAL_BUILD`. The marker `// EXPERIMENTAL_BUILD: tentacle TentacleGrab`
appears once, at the top of the new block (after `tentacle_slot::tentacle_slot`).

### How a game function is changed

For the five functions SummonerFix replaces with an entry jump, the original body is untouched; at its start:

```
#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on()) { exp_tentacle_xxx(this, ...); return; }
#endif
    ... the original, unchanged ...
```

so with `TentacleGrab=0` the original code itself runs (not a copy). The two in-line changes use
`#ifdef EXPERIMENTAL_BUILD` new `#else` original `#endif`:

| Site | Original | Change |
|---|---|---|
| `tentacle_beast_ai_select_action` 0x4c6520 | (entry) | on: `exp_tentacle_select_action(this)` |
| `tentacle_beast_ai_grab` 0x4c6850 | (entry) | on: `exp_tentacle_ai_grab(this, victim)` |
| `tentacle_beast_ai_release` 0x4c6a80 | (entry) | on: `exp_tentacle_ai_release(this)` |
| `tentacle_begin_sink` 0x4c6c50 | (entry) | on: `exp_tentacle_begin_sink(t, ms)` |
| `tentacle_beast_frame_hook` 0x4c7960 | (entry, after the `t == NULL` test) | on: `exp_tentacle_frame_hook(c)` |
| hold tick in `tentacle_beast_ai_frame` 0x4c7070 | `v->add_hp(3.0f, owner->handle, 1, false)` | `exp_tentacle_hold_tick(v, owner->handle)`: off +3 (the original call), on -3 |
| `tentacle_collect_area_hits` 0x4c77c0 (G251) | `t->hits[t->num_hits++] = ...` | on: only while `num_hits < 4` |

Checks: `fixed-compile --experimental`: compiles, the same 3 header warnings (vector.h(73), vfx.h(550/551)) as
REPO's unchanged file, no new ones. `VC6_STRICT=1 vc6-sweep --only tentacle_beast --check`: 21 matched there, 21
now, 0 lost.

## The new functions (all `static`, in the block after `tentacle_slot::tentacle_slot`)

- `exp_tentacle_on()`: reads the switches once (first call: the first tentacle code that runs), logs
  `tentacle grab on: chance N% ...` or `tentacle grab off`.
- `exp_tentacle_select_action(ai)`: the original select (busy tests, 2.5..5 s think timer, up to five candidates:
  a live slot handle, NULL-tested, then living type-7 characters without 0x88), then the grab branch
  (`exp_tentacle_try_grab`: gate watchdog; grab_ready, not multiplayer (`game_flags & 2`), move 0x86 loaded, this
  record holds / throws nothing, `frand() < chance`; target = the nearest grabbable candidate within 3.0 on X/Z of
  the live `$tag-26-pickup` point (fallback `pickup_pos`), else the first grabbable within reach). A pick sets
  `move = tentacle_move_86`, `target`, `grab_ready = 0`, returns 3 (tentacle_beast_ai_frame then calls
  tentacle_beast_start_grab). Otherwise the original ladder (its dead `< 0.0` rung kept).
  Grabbable = alive, no 0x88, has "flail" (14, 0x8c, anim != -1) and "throwndown" (8, 1).
- `exp_tentacle_gate_watchdog()`: grab_ready 0, grab timer invalid, nothing held / thrown / in sub-mode 0x35 in any
  record -> arm the grab timer (10 s).
- `exp_tentacle_ai_grab(ai, victim)`: as the original; move end with nothing held arms the grab timer (slip 4);
  trigger 7 only when `move_trigger_fired` returns 1 (slip 8); `exp_tentacle_take_victim`: victim non-NULL, no
  0x88, has the flail pose, edge distance < reach + 1 (slip 5); `release_hp = tentacle HP x 0.75` (slip 7); the
  holding anims NULL-tested (slip 10); the lead moves only off a victim that leads (party_leader or
  cur_character), to the first other member alive and not held (slip 9).
- `exp_tentacle_hold_tick(v, source)`: -3 HP (slip 6) when on, the original +3 when off.
- `exp_tentacle_ai_release(ai)`: trigger 8 tested for 1; `exp_tentacle_throw_victim`: a gone / down victim is put
  down, not thrown (slip 8); otherwise the original throw, plus the start height and flight time per record.
- `exp_tentacle_begin_sink(t, ms)`: no character -> nothing; a held or flying victim -> `exp_tentacle_drop`; then
  the original sink (timer, flag 4, the party's attack goals on it removed) (slip 12).
- `exp_tentacle_frame_hook(c)`: a held victim gone or down -> drop (slip 11), else pinned 1.5 below the grab bone
  (original); a thrown one gone or down -> drop; else `exp_tentacle_fly`: the original flight and ray, plus a fall
  3.0 below both the start and the aim point, or 3 s of flight, lands it (slip 13); landing =
  `exp_tentacle_put_down` and the grab timer 10 s.
- `exp_tentacle_put_down(c, lift_y)`: flags &= ~0x1028 (a body at 0 HP not yet dead keeps 0x8), go_idle if alive,
  face level, onto the navigation mesh (`pf_point_is_navigable` / `pf_find_nearest_navigable_point`, lifted back to
  lift_y if it fell into the water), find_ground, "throwndown" if alive else clear_flag_2_4.
- `exp_tentacle_drop(t, why)`: puts down the held and the thrown victim, clears both, squeeze timer invalid,
  tentacle anims 0x93 / 0x95, grab timer 10 s.

## ini keys (summonerfix.ini, the same keys SummonerFix reads)

| Key | Default | Meaning |
|---|---|---|
| `[Experimental] TentacleGrab` | 0 | 1: the grab, hold (3 damage / 6 s, up to 30 s or until the tentacle lost a quarter of its HP), throw and the fixes; the G251 hit bound |
| `[Experimental] TentacleGrabChance` | 10 | percent per decision (0..100, clamped) that a tentacle allowed to grab tries to |
| `[Debug] TraceTentacle` | 0 | 1: also the detail lines (roll passed with no candidate, impact misses, every hold tick) |
| `[Debug] ExperimentalLog` | 0 | must be 1 for any line in experimental.log |

Always logged (with ExperimentalLog=1): `tentacle grab on/off`, `grab pick`, `grab`, `grab move ended with nothing
held`, `throw`, `release, victim ... nothing thrown`, `landing (floor | fell below the aim point | flight timeout)`,
`drop held|thrown victim (...)`, `grab gate was stuck closed`. Each with the record index 0..3, the handle in hex
and HP.

## What the lead must add

- Headers / ini / SummonerFix: nothing. The keys already exist in summonerfix.ini and SummonerFixConfig;
  SummonerFix's `TentacleGrabInstall` already checks `RebuiltFixInSource("TentacleGrab")`, which the marker
  above feeds through Sum-exp.fixes. Since that check stands SummonerFix down for the whole feature, the G251 bound
  and the trace patch are also in this source (they are).
- `experimental.h` says the grab needs no hook: correct, none used.

## Differences from SummonerFix (none in behaviour intended)

- Logging goes to experimental.log; the main events are logged without TraceTentacle (they are rare), the details
  need it.
- The original's `lobj_ray` is cleared with memset before use, as tentacle.c does.
- The extra `frand()` of the grab roll shifts the random sequence only with the switch on (as in SummonerFix).

## Risks

- Not linked or run here (the lead links). The code paths are tentacle.c's, which was tested in game, but this is a
  rewrite in C++; the in-game test below is needed.
- With the switch on, the select / grab / release / sink / frame hook run the new functions (not the original
  code), so a slip in the transcription would show only with TentacleGrab=1. With it off nothing changes.
- Not yet tried in game (also not with SummonerFix): a throw over the water, a summon as target, the chance-10 feel.

## IN-GAME TEST PLAN (agent mode, Sum-exp.exe, test saves)

Addresses (Sum-exp.exe keeps the retail data layout; check with the link map if not): records `0x248c328`,
`0x248c438`, `0x248c548`, `0x248c658`; per record `+0xbc` victim, `+0xcc` thrown, `+0xe0` release HP;
grab_ready `0x248c770`, battle started `0x248c76c`, move 0x86 `0x248c7a4`, cur_character `0x5fcc88`. Character
flags `+0x320`, HP `+0x5f8`.

0. Off: `session TentacleGrab=0@Experimental ExperimentalLog=1@Debug`. experimental.log has `tentacle grab off`
   once the boss is loaded; play 2 minutes in the boss fight: no grab, every `+0xbc` stays `ff ff ff ff`; the
   squeeze still behaves as retail. Pass: no tentacle lines but the "off" one.
1. On: `session TentacleGrab=1@Experimental TentacleGrabChance=100@Experimental TraceTentacle=1@Debug
   ExperimentalLog=1@Debug`, a save with at least two heroes; `level Sewerboss sewerboss`; `peek 248c7a4 4`
   non-zero; trigger the battle (`navgoto $tbintro`, after TB-Intro `peek 248c76c 1` = 1); `immortal`.
2. Stand near a tentacle (`npcgoto Tentacle Beast#$npc058`). Capture every 300 ms. Pass: log `grab pick` then
   `grab, victim 0x.., release below X` with X = 0.75 x tentacle HP; the victim hangs at the tip in the flail
   pose; victim `+0x320` has 0x1028; if it led, `peek 5fcc88 4` changes to another hero.
3. Hold: `hold tick -3.0 ... HP a -> a-3` every 6 s (TraceTentacle). After 30 s (or hit the tentacle for a quarter
   of its HP): `throw`, then `landing (floor)`; the hero plays "throwndown", flags cleared, AI back. Only one grab at
   a time; the next `grab pick` at least 10 s after the landing.
4. Battle end mid-hold: while held, `npckill Broken Brass Golem#boss1`, `...#boss2`, `Iron Golem#boss` -> log
   `drop held victim (battle end / sink)`; the hero stands on the floor with its AI; the tentacle sinks.
5. Victim death while held (`immortal` off): `callt 450850 VICTIM 0xC47A0000 TENTACLE_HANDLE 1 0` -> `drop held
   victim (held victim dead)`, the body falls to the floor, no AI wake.
6. Tentacle death while holding: `npckill Tentacle Beast#$npc058` -> throw (or drop), then the death move; the
   tentacle counts as lost.
7. Throw over water (stand so the swipe point is over the water): `landing (fell below the aim point)` or
   `(flight timeout)`, the hero ends on the navigation mesh, not in the water.
8. Chance 10 (`TentacleGrabChance=10`): play 3 minutes, count `grab pick` lines; never two held at once; none while
   only one hero lives.
Pass overall: no crash, no hang, no heal while held, the victim always gets its AI back.

## autosave

### Autosave (experimental build) - worker notes, 2026-10-21

Changed file: `src/experimental/autosave.cpp` (the stub replaced). No game file, header, ini or SummonerFix file was
changed. It compiles with `tools/fixed-compile.mjs ... experimental/autosave.cpp --experimental`; the one warning
(`include\vsdk/math/vector.h(73)` C4244) comes from the shared header and appears for every unit that includes it
(for example `game/new_saveload.cpp`), so it is not new.

## Research: how the game saves (from the source)

- **The save** is `savegame::write(filename, 0, description)` (0x4d1c40): header (level file, script name,
  `level_time`, description up to 255 characters), thumbnail (`saveload_thumbnail_pixels`), the tagged sections,
  then one `File::write` into `path_savegame`. Every caller first runs `saveload_capture_thumbnail()` (0x427100;
  the Save screen does it when it opens) and `conversation_db.save_state()`. `quicksave` (0x40e7c0) is the model:
  `savegame_location_allowed`, count `*.sav` and refuse at 32 ("No more savegame slots."), capture, conversation
  state, write `quicksave.sav` with description "Quick save", HUD message.
- **When saving is allowed.** Every save path calls `savegame_location_allowed()` (0x4d4a50): 0 allowed, 1 a
  level-script event is active or a monster in combat mode targets a party member, 2 the area forbids it
  (no level, Khosani lab before `room8`, lab 2 after `pyrulstart`, Jade Temple after `got_laharah_spirit`,
  TempleINT2 after `mach_intro`), 3 the summon is emerging. On top of that the paths have implicit conditions:
  F3 / F4 / Ctrl+S need `game_flags & GF_SINGLE_PLAYER` (the HUD key handler); `cmd_savegame` needs
  `gameseq_get_state() == 4`; all are reached from the HUD only, so no menu screen is open, no message box,
  and the HUD key handler does not run during cutscenes, conversations or scripted input locks. The world map
  (level 4) is allowed by the game (default case 0), so autosave allows it too.
- **The Load screen** (`saveload_scan_saves`, 0x4258c0) lists *every* `*.sav` in the savegame folder
  (`file_find_files` flags 6: newest first, with the extension), at most 32, skipping files whose header level or
  script no longer exists. A row shows the level's display name and the game time (`level_time`, hh:mm); the info
  panel of the selected row shows the file time, the file date, the description and the thumbnail. Loading is
  `set_pending_load(filename)` + `game_change_level`, by file name. **So autosaves with their own file names appear
  and load with no UI change.** The description tells them apart only in the info panel (after selecting the row);
  see "Proposed UI changes" for marking the row itself.
- **Thumbnail:** `saveload_capture_thumbnail` renders the scene into a 201x151 clip of the back buffer and reads
  it back (565), into the buffer `write_thumbnail` copies.
- **Level load order** (`state_game_enter`): `level_load` (its end calls `exp_level_loaded`), then
  `savegame_state.load_pending()` when a savegame is being loaded (so `pending_load_length() != 0` inside
  `exp_level_loaded` means "entered by loading a save"), then level scripts, party placement, and the game runs.

## What was built (function by function)

`autosave.cpp`, all under `#ifdef EXPERIMENTAL_BUILD`:

- `exp_autosave_init()`: reads the four keys once (clamped: minutes 0..600, slots 1..10), resets the state, logs
  the settings.
- `autosave_in_play()`: the clock condition: state 4, no state change pending, not paused, not `game_frozen`, no
  `GF_CUTSCENE` / `GF_CONVERSATION` / `GF_NO_INPUT` / `GF_PARTY_DEAD`, `interface_cur_screen == 0` (HUD). Only then
  does the timer advance, by `reported_frametime` (game time, not wall time).
- `autosave_blocked()`: NULL or the reason a save cannot be made now: multiplayer (`!GF_SINGLE_PLAYER` or
  `GF_MULTIPLAYER`), not in gameplay, state change pending, savegame load pending, no `cur_character`, party dead,
  cutscene (`GF_CUTSCENE`, `GF_ESCAPE_CUTSCENE`), conversation, scripted sequence (`GF_NO_INPUT`, `GF_NO_RENDER`,
  `GF_NO_CLEAR`, `GF_NO_FLIP`, `GF_SCREENSHOT`), paused / frozen, a menu screen open, a message box
  (`ui_message_box_is_active`), HUD hidden (`ui_input_locked`), an item being used (`ui_using_list_item`,
  `ui_using_equipped_item`), and finally `savegame_location_allowed()` != 0 with its code.
- `autosave_pick_slot()`: lists `*.sav` (flags 4: names with extension, unsorted), counts them (as quicksave does)
  and finds `autosave<n>.sav` for n = 1..AutosaveSlots: the first missing slot, else the one with the oldest file
  time (rotation, oldest first). Only exactly `autosave<n>.sav` names are touched; `summoner<n>.sav` and
  `quicksave.sav` never are.
- `autosave_write(why)`: refuses to create a *new* file when there are already 32 savegames (the Load screen shows
  only 32, and a 33rd would push the player's oldest save off the list; overwriting an existing autosave is still
  allowed). Description `Autosave <n> - <level display name> - <h:mm am/pm>` (local clock; the level name from
  `level_format_display_name`, as the "Cannot save in %s." message). Then quicksave's sequence:
  `saveload_capture_thumbnail()`, `conversation_db.save_state()`, `savegame_state.write()`. Because this runs at
  `exp_frame_end` (after the frame was drawn), the thumbnail render leaves a small picture in the back buffer's top
  left; the code sets `GF_NO_FLIP` so that frame is not presented (FrameEnd clears the flag; the cutscene code uses
  the flag the same way), and the next frame's `gr_clear` removes it. HUD message "Autosave" (only when no other HUD
  message is showing, so a game message is not replaced) or "Autosave failed.".
- `exp_autosave_frame()`: advances the clocks; decides "level change" (pending and 2 s in play since the load) or
  "timer" (play time >= AutosaveMinutes); if blocked, waits and logs the reason once per change; else writes and
  resets the timer and the pending level save (also after a failed write, so a failing disk is not retried every
  frame).
- `exp_autosave_level_loaded(ok)`: a failed load clears everything; a savegame load (pending load name set) means
  no autosave and the timer restarts; multiplayer: nothing; else, with AutosaveOnLevelChange, a level-change
  autosave is queued. It waits for 2 s of in-play time, so a level that opens with a cutscene or conversation
  saves after it.

## Ini keys (`summonerfix.ini`, section `[Experimental]`)

| Key | Default | Meaning |
|---|---|---|
| `Autosave` | 0 | 1 = autosave on. 0 = the game's behaviour exactly (every function returns at once) |
| `AutosaveMinutes` | 5 | minutes of game time in play between autosaves; 0 = no timed autosaves (level changes only); max 600 |
| `AutosaveOnLevelChange` | 1 | 1 = autosave once after each level change (not after loading a save) |
| `AutosaveSlots` | 3 | number of rotating files `autosave1.sav` .. `autosaveN.sav` (1..10); each uses one of the Load screen's 32 rows |

### Text for the lead to add to summonerfix.ini (and SummonerFixConfig if it lists the section)

```
[Experimental]
; Autosave (experimental build only): saves into autosave1.sav .. autosaveN.sav, never into your own saves
; or quicksave.sav, and only where the game itself allows saving (no combat, cutscene, menu, multiplayer).
Autosave=0
; Minutes of play (paused time and menus do not count) between autosaves; 0 = only on level changes.
AutosaveMinutes=5
; 1 = also autosave after entering a new level (not after loading a save).
AutosaveOnLevelChange=1
; Number of rotating autosave files (1..10); the oldest is replaced.
AutosaveSlots=3
```

No `// EXPERIMENTAL_BUILD: <feature> <Key>` marker: SummonerFix has no autosave. No header change is needed.

## What I could not do / limits

- Not run (no link, no game). Everything below the research is untested.
- **The Load list row does not say "Autosave".** The row shows level + game time only; the description is in the
  info panel after a click. The file *time* and date in the panel, and the list order (newest first), make the
  autosaves easy to find, but a marker on the row needs a change in `game/new_saveload.cpp` (proposal 1).
- **The Save screen also lists the autosave files.** A player can save over `autosave2.sav` from the Save screen
  (after the overwrite question); that file is then part of the rotation and will be replaced later. Proposal 2
  hides autosave rows in save mode.
- The timer is not reset by the player's own saves (F3 / Ctrl+S): those paths have no hook. It is reset by loading
  a save and by every autosave.
- Saving takes the same time as a quicksave (a 270 KB file write plus one extra scene render): a short hitch, and
  one frame is not presented (GF_NO_FLIP).

## Proposed UI changes (not made; for the lead, in `game/new_saveload.cpp`)

Both need a way to ask whether autosave is on; add to `include/experimental/experimental.h` after the autosave
entry points:

```cpp
bool exp_autosave_enabled();      // [Experimental] Autosave is on (autosave.cpp)
```

and to `autosave.cpp`: `bool exp_autosave_enabled() { return autosave_on; }` (note: `autosave_on` is only read at the
first frame, which is before any screen can open).

**1. Mark autosave rows in the list** (`saveload_draw_list`, after `saveload_format_level_name(buf, e->level);`):

```cpp
            saveload_format_level_name(buf, e->level);
#ifdef EXPERIMENTAL_BUILD
            // [Experimental] Autosave: "(Auto) <level>", shortened to the same width.
            if (exp_autosave_enabled() && !_strnicmp(e->filename, "autosave", 8)) {
                char tmp[256];
                char full[256];
                saveload_format_level_name(tmp, e->level);   // already ellipsized; re-cut with the prefix
                sprintf(full, "(Auto) %s", tmp);
                gr_string_truncate_ellipsis(buf, full, saveload_level_name_w[resolution_index], -1);
            }
#endif
```

**2. Keep the player's saves out of the autosave files** (`saveload_scan_saves`, first thing in the loop body):

```cpp
#ifdef EXPERIMENTAL_BUILD
        // [Experimental] Autosave: the Save screen does not offer autosave files to save over.
        if (saveload_save_mode && exp_autosave_enabled() && !_strnicmp(node->name, "autosave", 8))
            continue;
#endif
```

Both go into byte-matched code, so the sweep check of PROMPT.md applies (`--only "new_saveload"`); under
`#ifdef EXPERIMENTAL_BUILD` the shipped bytes do not change.

## Risks

- The thumbnail capture at `exp_frame_end` (after the frame was drawn and before FrameEnd) instead of in the
  input phase where quicksave runs it. Same calls, but in a different place in the frame; if the D3D path does not
  like a second scene render after the interface, the thumbnail may be wrong/black or the frame may flicker. Check
  the thumbnail on the Load screen. Fallback: move the call to `exp_frame_begin` (needs an `exp_autosave_frame_begin`
  hook in `exp_hooks.cpp`, the lead's file), where it matches quicksave's position (before the state's `gr_clear`).
- `GF_NO_FLIP` for one frame: if the driver needs every frame presented, a visible stutter.
- `savegame_location_allowed` code 1 covers only monsters of type 5 / 9 in combat mode that target a party member;
  a fight against others (or a monster about to attack) does not block, same as F3. The game's own rule.
- `ui_using_list_item` / `ui_using_equipped_item` / `ui_input_locked` as blockers are my addition (conservative).
- The 2 s settle after a level change counts only in-play time; a level that keeps the input locked forever (none
  known) would never get its level-change autosave (the timer one waits the same way).

## IN-GAME TEST PLAN (lead, agent mode, Sum-exp.exe)

Ini for all tests: `[Debug] ExperimentalLog=1`, `[Debug] TestSaves=1` (saves in `Summoner/savetest`),
`[Experimental] Autosave=1 AutosaveMinutes=1 AutosaveOnLevelChange=1 AutosaveSlots=3`. Read `experimental.log` next
to the exe after each step. Before starting, note the `*.sav` files in the save folder with their times and sizes
(the player's saves must stay byte-identical throughout).

1. **Switch off.** `Autosave=0`, load a save, play 2 minutes, change level (`level IonaEXT02 IonaEXT02_v2` or a
   level exit). Pass: log line `autosave: off, ...`, no other autosave lines, no `autosave*.sav` created.
2. **Load does not autosave.** `Autosave=1`. Start, load the newest test save. Pass: log
   `autosave: level N (...) loaded from a savegame: no autosave, timer reset`; no file written in the first 2 s.
3. **Timer, game time.** Stand in a safe spot (town). Pause (`key SPACE`) for 70 s (`wait 10000` x7), unpause.
   Pass: no autosave while paused. Then play ~60 s unpaused: `autosave: wrote autosave1.sav (timer, new slot, ...)`;
   `capture` right after shows the HUD message "Autosave". Open the inventory (`key I`) for 70 s: no save while it
   is open (the clock stops; the log may show nothing until the timer is due).
4. **Rotation.** Keep playing ~3 more minutes. Pass: `autosave2.sav`, `autosave3.sav`, then `autosave1.sav`
   again with "replaced the oldest". File times rise in that order; `summoner*.sav` and `quicksave.sav` unchanged.
5. **Level change.** `level <name> <script>` (or a level exit). Pass: log `level N (...) loaded: autosave after 2 s
   in play`, then `wrote autosaveK.sav (level change, ...)` about 2 s after the level is playable, not during the
   loading screen. For a level that opens with a cutscene (e.g. `play_and_load_cutscene` scenes), `waiting (level
   change): cutscene` first and the save after the cutscene.
6. **Blocked: combat.** Walk into a monster group (or `navgoto` beside one) and let the timer come due during the
   fight. Pass: `waiting (timer): combat or scripted event (savegame_location_allowed 1)` once, the save after the
   fight. **Summon:** `summon 38`, pass `waiting ...: summon emerging (...3)` if it falls due then (optional).
   **Area:** `level` to the Temple of Urath / a forbidden stage (G130 note in savegame.md: "Cannot save in The
   Temple of Urath."): pass `waiting ...: not in this area (savegame_location_allowed 2)` and no file.
7. **Menus / dialogue.** Talk to an NPC when the timer is due: `waiting (timer): conversation` (or `menu open`),
   save after the talk ends.
8. **Load screen.** F4 (or the main menu's Load). Pass: the autosaves are listed (newest first) with the level name
   and game time; selecting one shows `Autosave <n> - <level> - <h:mm am>` and a correct thumbnail (not black, no
   HUD in it, not garbled). Load it: the game continues where the autosave was made (party, gold, position).
   Also `peek 5fbfd8` before/after (game_flags) to see no leftover 0x8000.
9. **32-save limit.** Copy saves in until there are 31 `*.sav` and no autosave file, AutosaveSlots=3. The first
   autosave writes `autosave1.sav` (32 files); the next one would need a new `autosave2.sav`, so the log shows
   `skipped (timer): 32 savegames, the Load screen shows no more`. Pass: never more than 32 `*.sav`.
10. **Multiplayer** (if a test setup exists): log has no `wrote` lines; at most `waiting ...: multiplayer`
   (level-change saves are not even queued).
11. **Screen check.** Watch the window (or record it) around an autosave: no 201x151 picture flashes in the
   top-left corner. Agent `capture` reads the back buffer, so a capture taken in the autosave frame itself could
   show the corner picture even though that frame is never presented; judge by the window, not by one capture.

Pass overall: every autosave is in `autosave1..N.sav`, loads, describes itself, and every skip has a log line; the
player's own save files are unchanged.

## display

### display: frame-rate cap, vsync (field of view: proposal only)

Feature files: `src/experimental/display.cpp` (replaces the stub), `src/vsdk/frametime.cpp` (one
`#ifdef EXPERIMENTAL_BUILD` block in `FrameDone` plus two declarations). Nothing else changed.

## Ini keys (`[Experimental]` in summonerfix.ini)

| Key | Default | Meaning |
|---|---|---|
| `FrameRateCap` | 0 | Frames per second. 0 = the game's own cap (120). 30..240; values outside are clamped (logged). |
| `VSync` | 0 | 1 = wait for the vertical blank on the OpenGL renderer (`wglSwapIntervalEXT(1)`). Direct3D / Glide: logged as not supported; the game's own vsync flag stays in charge there. |

Text for the lead to add to `summonerfix.ini` under `[Experimental]` (I did not edit it):

```
; Experimental build only. Frame-rate cap in frames per second (30-240); 0 = the game's own (120).
; The game's speed does not change with the cap, only the smoothness.
FrameRateCap=0
; Experimental build only, OpenGL renderer: 1 = wait for vertical sync (no tearing; the frame rate then follows
; the monitor's refresh rate, up to FrameRateCap). 0 = as the game (no vsync under OpenGL).
VSync=0
```

## How it works

### FrameRateCap

- The frame timer's floor `min_reported_frametime` (0x5a574c) is the cap: `FrameDone` (fixed build, G1
  FrameLimiter) waits until one floor has passed since the last frame and reports exactly that time, carrying a late
  wake-up into the next frame. So a frame shorter than the floor takes and reports `1 / cap` real seconds; a frame
  longer than it reports its real length, still clamped to 0.25 s (`max_reported_frametime`, untouched). Game time
  = real time at any cap: only the number of frames per second changes (two small per-frame truncations, the level
  fade and the animation ticks, still depend on the rate as in the stock game: see Risks).
- `exp_display_init` (first frame, after the first `FrameDone` has run `frametime_initialize`): reads the key,
  clamps, sets `min_reported_frametime = 1 / cap`, sets `exp_display_exact_ms = 1`.
- `exp_display_frame` (every frame, before `FrameEnd`): puts `1 / cap` back when something else wrote the floor.
  This is needed: `cutscene_cleanup` calls `frametime_restore_limits`, which restores `frametime_saved_min`; that
  static was initialised by the CRT from the .data value (1/120) and is never updated because `frametime_unlimit`
  (the saver) has no caller. So every cutscene end sets the floor back to 1/120.
  - Side finding: SummonerFix's `[Game] MaxFPS` on the original Sum.exe patches the 1/120 immediate in
    `frametime_initialize` only, so on the original game MaxFPS is also lost at the first cutscene end (inferred
    from the code, not tested).
- `vsdk/frametime.cpp`, `FrameDone`: with `exp_display_exact_ms` set, the millisecond count given to `UpdateTicker`
  and `milliseconds_unpaused` carries the fraction the game truncates (`(int)(reported_frametime * 1000)`).
  Without it the millisecond clocks (GameTimer: AI and spell timers; RealtimeTimer; bitmap animations) run slower
  than the float frame time by `frac(ms) / ms`: 4 % at 120 (8.33 -> 8), 13.5 % at 144 (6.94 -> 6), likely 10 % at
  100 (1/100 as a float is 0.0099999998; times 1000 in x87 precision truncates to 9; not checked in the asm), 1 % at 30. With the carry they match real time at every cap. Note: with
  `FrameRateCap=120` set explicitly those clocks therefore run about 4 % faster than with `FrameRateCap=0` (where
  the game's truncation is kept as is) - the cap's value is correct real time, the default is the game's.
- SummonerFix: no stand-down marker. Its `[Game] MaxFPS` is a byte patch in `ApplyDisplayPatches` (main.c), which
  is not run for a rebuilt exe, so on Sum-exp it does nothing and `FrameRateCap` is the only cap. Its speed key
  works through `frametime_divisor` and is unaffected (the carry uses the divided frame time).

### VSync

- Only for `gr_screen.mode == GR_OPENGL` (0x68). The game's own `gr_screen.vsync` (options `cfg_34`, on during
  cutscenes) is read only by `gr_direct3d.cpp` (DDFLIP_NOVSYNC) and `gr_glide.cpp`; the OpenGL back end ignores it.
- `display_vsync_apply`: when a GL context is current (`wglGetCurrentContext`, `wglGetCurrentDC`), looks up
  `wglSwapIntervalEXT` / `wglGetSwapIntervalEXT` once with `wglGetProcAddress`, calls `wglSwapIntervalEXT(1)` and
  logs the result and the read-back interval. Called at init and every frame; it does work only when the context
  or the DC changed (`gr_opengl_init_context` takes a new DC when the window is re-created; it keeps the HGLRC).
- Other renderers: one log line ("not supported on the Direct3D renderer ..."), then the switch is off.
- SummonerFix's `SwapBuffers` hook (display.c) only draws and clears; it sets no swap interval, so no overlap.
- With VSync on, a cap above the refresh rate has no effect; a cap below it gives an uneven frame pacing (frames
  of 1 and 2 refreshes). Game speed stays right either way (frames longer than the floor report their real time).

### Logging (only with `[Debug] ExperimentalLog=1`)

- init: `display: FrameRateCap N (min frame time ...)` or `display: FrameRateCap off ...`; the VSync lines.
- `display: min frame time was set to 0.008333 s; FrameRateCap puts back ...` after each cutscene.
- every 5 s: `display: speed: wall W ms, game real-time ticker T ms (P%), F frames, fps X, min dt D; draw lists
  peak sorted S opaque O (of 500) attach A (of 2000)`. P is the speed check (should be ~100 % while not paused and
  without frames over 0.25 s, i.e. not during loading). The draw-list peaks are for the FieldOfView decision below.

## Function by function

- `display_renderer_name`: renderer id -> name, for the log.
- `display_vsync_apply`: see VSync.
- `exp_display_init`: reads both keys once; sets the cap; applies vsync or logs not supported; starts the report.
- `exp_display_frame`: re-applies the cap and vsync if needed; the 5 s speed/draw-list report.
- `FrameDone` (frametime.cpp): the millisecond carry under `EXPERIMENTAL_BUILD`, original line in `#else`.

## Checks done

- `fixed-compile ... experimental/display.cpp --experimental`: compiles; one diagnostic, the C4244 in
  `include/vsdk/math/vector.h(73)` that every unit including `vsdk/gr/gr.h` gets (vsdk/gr.cpp shows the same), so
  nothing new from this file. I declared the three levelrender counters locally instead of including
  `engine/rendering/levelrender.h`, whose headers add two more C4244s (vfx.h).
- `fixed-compile ... vsdk/frametime.cpp --experimental`: 0 diagnostics.
- `VC6_STRICT=1 vc6-sweep --only frametime --check src/decomp/vc6-matches.txt`: 12 match, 0 lost.

## For the lead

- Header (optional, cleanliness): add to `include/experimental/experimental.h` under the display entry points
  ```
  // Nonzero while [Experimental] FrameRateCap is set: FrameDone (vsdk/frametime.cpp) carries the millisecond fraction.
  extern int exp_display_exact_ms;
  ```
  and then drop the local `extern int exp_display_exact_ms;` in `vsdk/frametime.cpp` (include
  `experimental/experimental.h` there under `#ifdef EXPERIMENTAL_BUILD` instead).
- docs/experimental-build.md feature row: switches `[Experimental] FrameRateCap`, `VSync`.
- No SummonerFix change needed.
- New imports in display.cpp: `wglGetCurrentContext`, `wglGetCurrentDC`, `wglGetProcAddress` (opengl32, already
  linked through gr_opengl.cpp), `GetTickCount` (kernel32).
- Nothing was created outside `src/` except this NOTES.md.

## Field of view: research and proposal (not implemented)

**Where the game sets it (observed in the source).**
- `gr_setup_3d(orient, pos, fov, ...)` (0x50dec0): `fov` is the **horizontal** angle (tan(fov/2) scales the x axis;
  a value below 2.0 is taken as the tangent). It builds `gr_matrix_scale` and the frustum (`gr_setup_frustum`), and
  everything else derives from those: `gr_cull_sphere` / `gr_cull_bounding_box`, the screen projection,
  `gr_screen_to_world_ray` (mouse picking), `gr_get_view_fov` (target decal size). So culling and picking follow a
  changed fov by themselves.
- `interface_init_global` (GameInit) calls `gr_setup_3d` once with `camera_fov` = 73.74 on the 1024 x 672 view and
  stores the resulting **vertical** fov in `interface_view_fov` (0x5bd818). Nothing else writes it.
- Every frame outside cutscenes `interface_set_view_clip` (0x412220) sets `camera_fov =
  gr_fov_to_vertical(interface_view_fov)` (despite its name, it returns the horizontal fov that keeps that vertical
  fov for the current clip), and `state_game_frame` passes `camera_fov` to `gr_setup_3d`. Side panels and wider
  `GameWidth` therefore keep the vertical fov and change the horizontal one.
- Cutscenes set `camera_fov` from the shot and `interface_set_view_clip` leaves it alone (GF_CUTSCENE). The item
  views (inventory, buy/sell, status) use a fixed 60. The camera's `fov` field is saved (CMRA) but overwritten the
  next frame.

**The hook would be one write:** at `exp_display_init` (GameInit has run),
`interface_view_fov = 2 * atan(tan(interface_view_fov / 2) * tan(F / 2) / tan(73.74 / 2))` with F = FieldOfView
in degrees (the horizontal fov of the 4:3 in-game view; the game's is 73.74). It scales the whole frustum, keeps the
side-panel / widescreen behaviour and leaves cutscenes and item views alone. No HUD or 2D UI uses the 3D
projection apart from the overhead texts and target decal, which follow it.

**Why it is not small and safe.**
1. `levelrender_sort_list` and `levelrender_opaque_list` (`LEVELRENDER_MAX_SORTED` = 500 pointers each, 0x22f8d84
   and 0x22fa6a8) are filled with `list[count++] = e` at three sites in
   `game/unnamed_render_scene_0x481680.cpp` (character push ~line 911, level object push ~1165, effect push ~1366)
   with **no bounds check**; only the entry pool (0xc800 bytes, 3200 16-byte entries, shared with the larger effect
   entries) is checked. A wider fov puts more objects in the frustum, and past 500 the pushes write over the .bss
   after the list. Whether the stock game ever gets near 500 is unknown (inferred, not verified in game, so not a
   game-bugs entry yet); the 5 s log line now records the peaks.
2. Vis sets are per layer (`level_layer_vis`), not per view, and levels were built for the stock fov and zoom: a
   wider view can show unfinished geometry or the void at level edges (content, not a crash).

**Proposal.**
1. First collect data with this build: play a few dense levels (town, a big battle, max zoom) with the log on and
   read the `draw lists peak` values at the default fov.
2. Add bounds checks under `EXPERIMENTAL_BUILD` at the three push sites (skip the entry when its list has 500;
   original code in `#else`; vc6-sweep --check on that unit), or raise the two lists' size in the experimental
   build only.
3. Then `[Experimental] FieldOfView` (degrees, 0 = game, clamp e.g. 50..110) with the one write above, logged with
   the game's own `interface_view_fov` value as the baseline.

## Risks

- **Other per-frame integer truncations** (searched `(int)(... frametime ...)` in game/ and vsdk/). The float frame
  time drives the simulation, and the tickers are carried (above); two other places still truncate per frame, so
  their rate depends on the frame rate. This is in the stock game too (frame rates of 4-120 on period hardware);
  a cap does not add it, but a cap at an unlucky value makes it constant:
  - `levelrender_render_scene` level fade: `levelrender_fade_alpha += (int)(reported_frametime * 256)` (both
    directions). Per second: 240 at caps 30 / 60 / 120 / 240, 200 at 100, about 165 at 165, **144 at 144** (1.78
    per frame truncates to 1). So the level fade-in/out (0..255) takes ~1.06 s at 120 and ~1.77 s at 144. Cosmetic.
  - `character_instance::process`: `increment_tick((int)(frametime * 30 * 160))`, 4800 animation ticks a second:
    exact at 30 / 60 / 120 / 240, 1 % slow at 144 (33.3 -> 33), 0 % at 100 (48). Negligible.
  Clean caps are 30, 60, 120, 240. Fixing those two sites is out of scope (game files; could use the same carry).

- The cap's carry changes the millisecond clocks only while `FrameRateCap` is non-zero; with 0 the frame timer is
  the fixed build's byte for byte (the `#else` line).
- A cap of 30 makes input feel slower (one frame later) but does not change timing; very low caps were excluded
  (minimum 30).
- `wglSwapIntervalEXT` may be overridden by the driver's control panel ("application controlled" needed).
- Not run in game (by rule); the lead links and tests.

## IN-GAME TEST PLAN (agent mode, test saves)

Setup for all: `[Debug] ExperimentalLog=1`, `Agent=1`, Renderer=OpenGL, Sum-exp.exe (`SUMMONER_EXE`). Load a test
save in a level (not the world map) and stand still or walk; avoid pausing (the speed line counts only unpaused
frames). Peek addresses are Sum.exe's (`0x5a574c` min frame time float, `0x5a5744` fps float, `0x5a575c` real-time
ticker int); on Sum-exp use the link-exp address table if `peek` does not translate.

1. **Off = game.** `FrameRateCap=0`, `VSync=0`. Log: `FrameRateCap off (the game's min frame time 0.008333 s)`.
   `info` fps about 115-120. Speed lines after 30 s: P about 96 % (the game's own millisecond truncation at 120;
   this is the baseline). Pass: no other display lines; behaviour as the fixed build.
2. **Cap 60.** `FrameRateCap=60`. Log `FrameRateCap 60 (min frame time 0.016667 s ...)`; `peek 5a574c 4` = 0x3c888889;
   `info` fps ~60; speed lines P 99-101 %. Walk a measured route (e.g. a party member from one point to another,
   or a spell's duration) with a stopwatch from `capture` timestamps against cap 0: same real time within a few %.
3. **Cap 30, 144, 240.** Same; fps ~30 / ~144 / ~240 (240 only if the PC renders that fast; otherwise the real
   rate), P 99-101 % in each. Pass: no speed difference in the walk test; the 144 run is the one where the carry
   matters (without it P would read ~86 %).
3b. **Level fade (the known rate dependence).** At cap 120 and at 144, enter a level (or change layers) with
   `capture` every 100 ms during the fade-in; expected ~1.1 s at 120 and ~1.8 s at 144 (see Risks). Pass: matches
   the expectation (it is cosmetic and also in the stock game; record the numbers).
4. **Clamp.** `FrameRateCap=10` and `=1000`: log lines "below 30; using 30" / "above 240; using 240".
5. **Cutscene.** Cap 60, trigger a cutscene (new game intro or a level script cutscene), let it end. Log: `min
   frame time was set to 0.008333 s; FrameRateCap puts back 0.016667 s`; afterwards `peek 5a574c 4` = 0x3c888889 and
   fps ~60 again. Pass: the cap survives.
6. **VSync.** `VSync=1`, cap 0, on a 60 Hz monitor: log `VSync on (OpenGL)` and `wglSwapIntervalEXT(1) = 1, interval
   now 1`; fps ~60 (the refresh rate); P 99-101 % (frames longer than the floor report real time). Captures of a
   camera turn show no tearing. Then with `[Display] Mode=Window`, resize/alt-tab: if a second `wglSwapIntervalEXT`
   line appears the DC changed and was handled; fps stays at the refresh rate.
7. **VSync on Direct3D** (if the D3D path runs at all on the test PC): log "not supported on the Direct3D renderer".
8. **Draw-list data for FieldOfView** (any run above): note the largest `peak sorted` / `opaque` values seen in a
   town, a crowded battle and at maximum zoom-out (CameraZoomMax raised). Report them; anything near 500 means the
   bounds checks are needed even without a wider fov.
