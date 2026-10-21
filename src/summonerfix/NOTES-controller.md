# Controller support, tier 1: notes

## Design decisions

- **Per-frame poll in the SwapBuffers hook** (display.c `Hook_SwapBuffers`, first line). It runs once per frame on
  the game thread, so `key_mark` / `mouse_mark_button` are called from the same thread the game's own input uses.
  The frame limiter hook (timing.c `FrameSleep`) is no good: it only runs in frames shorter than MIN_DT. The cost
  is that the controller needs the OpenGL renderer; but agent mode's cursor override has the same limit (the
  ScreenToClient hook is installed only by `DisplayInstall`). With another renderer `ControllerInstall` logs that
  and stays off.
- **Cursor**: the same override agent mode uses (`g_agentX/Y` read by the ScreenToClient hook), with a third
  state. `g_agentCursor` is 0 (real cursor), 1 (agent mode set it: kept for ever, as before) or 2 (the controller
  moved it). In state 2 the real Windows cursor is also moved there (`SetCursorPos`), and the position where it
  landed is remembered. When the game's cursor poll (return address in 0x500b00..0x500d00, as the ClientToScreen
  hook checks) sees the real cursor elsewhere, the mouse has moved: state goes back to 0 and the mouse carries on
  from where the stick left the cursor (no jump). The game's own cursor warp (ClientToScreen at the mouse poll)
  updates the remembered position, so a warp is not taken for a mouse movement. In state 1 (agent mode) the
  controller moves only the agent cursor and never the real one.
- **Cursor movement** in game pixels per second (`CursorSpeed`, default 800: the 1024-wide screen in about 1.3 s
  at full deflection), scaled to window pixels in `DisplayMoveCursor` with fractions carried over, so slow
  movement still moves. Radial dead zone (default 7849, XInput's own left-stick value), speed linear in the
  deflection beyond it (as asked). dt from QueryPerformanceCounter, capped at 50 ms so the cursor does not jump
  after a loading stall. The cursor is clamped to the game picture (not the black bars), so pushing against the
  side edge triggers the game's edge turning, as with the mouse.
- **Right stick = arrow keys** held through `key_mark` (the camera reads them with `key_is_down`), with a
  threshold (16000) and 3/4 hysteresis so a key does not chatter. Binary, like the keys themselves: the game has
  no analogue camera input.
- **Edge detection** over one 32-bit "down" word: the 14 XInput buttons (their own bits), LT/RT (0x10000/0x20000)
  and the four right-stick directions (0x40000..0x200000). Only changed bits send anything, so an idle controller
  sends nothing. Order within a frame: other releases, modifier releases, modifier presses, other presses (LT+B in
  one frame gives Shift down before the right click).
- **Mapping** in `padmap.h` (header-only, shared by the DLL and the config app, like keynames.h). Targets are game
  key names or MouseLeft/Right/Middle, parsed with keynames.h's `KeyParse`; Mouse4/5 are refused (the game has no
  side buttons). Injected keys bypass `[KeyBindings]` (KeysInjectKey sets `g_injecting`), as for agent mode.
- **Default mapping** (from docs/sum-exe/controls.md): A left click, B right click (chain attack), X interact, Y
  inventory, LB Tab (next member), RB H (solo), LT LShift (chain slot 1, Shift+Tab = previous member), RT Space
  (pause), Start Esc (play menu), Back M (automap), D-pad quick slots 1-4, LS C (spells), RS F12 (camera mode).
- **XInput** loaded with LoadLibrary (xinput1_4, xinput1_3, xinput9_1_0) and GetProcAddress("XInputGetState");
  the structs are declared in controller.c, so no header or import library (the DLL's import table is unchanged).
  Only loaded when Enabled=1. `XInputGetState` on an empty slot is slow, so with no controller the four slots are
  checked once a second; connect and disconnect are logged, "none connected" once.
- **Focus**: a real pad counts only while the game's "main window active" byte (0x2cbff1c) is set; otherwise its
  held inputs are released. Agent mode keeps that byte at 1. The synthetic state ignores focus.
- **Disconnect / focus loss / end of a synthetic state**: whatever is still held is released on the next frame.
- **No live reload**: `[Controller]` is read at start-up (as `[Fixes]`). Could be added to keys.c's ini watcher
  later.
- **Agent `pad` command** (agent.c): `ControllerSetSynthetic` stores the state with a start time and length under
  a critical section; the game thread uses it instead of XInput while it lasts. The command sleeps MS, then waits
  (up to 1 s) until the game thread has released the inputs (`ControllerSyntheticDone`), so the next command sees
  a clean state. It works with Enabled=0 (ControllerInstall always reads the mapping) but, like the cursor, needs
  the OpenGL renderer's SwapBuffers hook to be polled.

## File changes (in the copy)

- `src/summonerfix/controller.c` (new): config, XInput loading and polling, synthetic state, edge detection,
  stick handling. `ControllerInstall`, `ControllerFrame`, `ControllerSetSynthetic`, `ControllerSyntheticDone`.
- `src/summonerfix/padmap.h` (new): the input table (ini key, bit, default key, label) and the numeric defaults,
  shared with the config app.
- `src/summonerfix/summonerfix.h`: declarations of the four controller functions and `DisplayMoveCursor`.
- `src/summonerfix/display.c`: `DisplayMoveCursor` (cursor state 2, real cursor moved along, clamped to the game
  picture, sub-pixel remainders); the ScreenToClient hook ends state 2 when the real mouse moved; the
  ClientToScreen hook keeps the remembered real position in step with the game's warp; `ControllerFrame()` at the
  start of `Hook_SwapBuffers`; `#include <stdlib.h>` for `abs`.
- `src/summonerfix/main.c`: `ControllerInstall()` after `KeyBindInstall()` in both the Sum.exe and the rebuilt-exe
  paths (it needs `g_keyMark` / `g_mouseMark` from keys.c); rebuilt-exe log line mentions the controller.
- `src/summonerfix/agent.c`: the `pad LX LY RX RY BUTTONS [MS]` command and its line in the command list comment.
- `src/summonerfix/summonerfix.ini`: the `[Controller]` section (Enabled=0 and all defaults, commented).
- `src/summonerfix-config/config.c`: a **Controller...** button (right column, under Experimental) opening a
  second window like "Choose fixes": "Use a controller" checkbox, the four numbers, one edit box per input with its
  default and action, Save (validates numbers and key names; nothing is written if one is wrong), Defaults (fills
  in the defaults; Save writes them), Close. Window class `SummonerFixPad`, added to the dialog message loop.
- `tools/build-summonerfix.ps1`: `controller.c` added to the file list.

Builds with no errors and no warnings (DLL and config app). The DLL's import table is unchanged (no XInput import).

A host test (`ctl-work/padtest.c`, controller.c with SumVa / Log / KeysInject* / DisplayMoveCursor stubbed) ran:
idle frames send nothing; A gives mouse 0 down then up; LT+B gives LShift down, mouse 1 down, mouse 1 up, LShift
up; the right stick at -30000/+30000 holds Left and Up and releases them; inside the threshold or dead zone nothing
happens; full left stick for 500 ms moves 397 game px (800 px/s); stick up moves the cursor up; a bad mapping
(`B=Mouse4`) falls back to the default with a log line; `A=none` sends nothing.

## To test in game (not done: the game was not run)

Use agent mode with `[Debug] Agent=1`. Start each run with `move X Y`, so the agent owns the cursor.

1. Idle does nothing: `[Controller] Enabled=1` with no controller: the log says "none connected" once; agent
   `move`, `click`, `key` work as before; the real mouse works in a normal (non-agent) session.
2. Cursor: `move 256 192; pad 32767 0 0 0 0 500; info` → cursor about 200 screenshot px to the right (400 game px;
   screenshot px are half the window's client pixels, so the exact number depends on the window size). Then
   `pad 0 32767 0 0 0 250` (up), `pad -20000 -20000 0 0 0 300` (diagonal, slower), `pad 5000 5000 0 0 0 300`
   (inside the dead zone: no movement). Check that the cursor stops at the picture edge and that the camera then
   turns (edge turning).
3. Buttons: in game, on an empty spot, `pad 0 0 0 0 1000` (A: the leader walks there); `pad 0 0 0 0 8000`
   (Y: inventory opens) and again to close; `pad 0 0 0 0 10` (Start: play menu), `pad 0 0 0 0 20` (Back: automap),
   `pad 0 0 0 0 100` (LB: next party member), `pad 0 0 0 0 10100` (LT+LB: previous member, Shift+Tab),
   `pad 0 0 0 0 200` (RB: solo mode), `pad 0 0 0 0 20000` (RT: pause), `pad 0 0 0 0 1` (D-pad up: quick slot 1),
   `pad 0 0 0 0 40` (LS: spells panel), `pad 0 0 0 0 80` (RS: camera mode), `pad 0 0 0 0 4000` (X: interact near
   a door or chest). Capture before and after each.
4. Chain attack: next to an enemy with the cursor on it, `pad 0 0 0 0 12000 200` (LT+B: chain slot 1) and
   `pad 0 0 0 0 2000 200` (B: default slot). Check the chain starts (and the 1 s cooldown).
5. Camera: `pad 0 0 -32768 0 0 400` and `pad 0 0 32767 0 0 400` (turn; peek the yaw at `0x60a018+0x180`), `pad 0 0 0
   32767 0 400` (zoom in: `+0x184` goes down) and `pad 0 0 0 -32768 0 400` (zoom out).
6. Held button across a long press: `pad 0 0 0 0 1000 1500` (A held 1.5 s: the leader keeps following the cursor,
   player_input flag 0x10), then check A is released (the leader stops following).
7. Menus: on the main menu and the play menu, move with the stick and press A on a button.
8. Mixed: after `pad`, a plain agent `click` and `key` still work (the agent cursor stays at the pad's position).
9. Real controller (when one is available, outside agent mode): connect/disconnect lines in the log; Alt+Tab away
   with a button held (it must be released); unplug with a button held (released); move the mouse after using the
   stick (the mouse takes over at that point, no jump); triggers half pressed; check the cursor speed and dead zone
   feel, and the CPU cost with no controller plugged in (XInput is probed once a second).
10. Config app: open **Controller...**, change a value, Save, check the ini; type `Mouse4` or `Foo` for a button
    (refused, nothing written); Defaults then Save.

## Open points

- Two inputs mapped to the same target: releasing one releases the target while the other is still held.
- A key held on the keyboard and pressed on the pad at the same time is released by whichever lets go first.
- Not done (tier 2 ideas): analogue camera, live reload of `[Controller]`, Direct3D/Glide support (would need
  another per-frame hook and cursor path), on-screen hints for the buttons.
