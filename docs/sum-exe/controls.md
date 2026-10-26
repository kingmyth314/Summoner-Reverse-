---
title: "Controls and keyboard map"
parent: Sum.exe
nav_order: 6.1
---

# Controls and keyboard map

**Summary:** Summoner's controls are **hard-coded**. There is no binding table and no options
setting. Mouse actions are read every frame by `player_read_mouse_input` (`0x4344b0`). Keyboard
keys arrive as queued key codes and are dispatched by one big `switch` in `hud_handle_keys`
(`0x40d930`). Every key passes through `key_remap_layout` (`0x509560`) first, which is the
natural hook for [custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions).

## How a key press travels

1. Windows sends `WM_KEYDOWN` to the game window. `key_on_keydown` (`0x509660`) is
   registered with `os_add_msg_handler` and receives it.
2. `key_mark(code, down, latency)` (`0x5093a0`):
   - calls **`key_remap_layout(code)`** (`0x509560`), which swaps scan codes for French
     and German keyboards;
   - updates `key_down_state` and `key_press_counts`;
   - pushes the code to the key queue.
3. In game, `input_drain_events` (`0x434440`) calls `PollInputEvents` (`0x432710`) until it returns 0, and
   passes each code to step 4. While the game window is in the background, `PollInputEvents` first waits
   (`os_poll` and 100 ms `timer_wait` steps, with `game_pause_count` raised). Then it takes one code with
   `key_inkey` (call at `0x432754`):
   - **Shift+Esc** (`0x1001`) quits (`gameseq_set_state(3)`) in single player. In multiplayer it leaves the
     game through `mp_player_disconnected` (reason 2 when the local player's flag `0x1` is set, else 0), and
     does nothing while the game is paused;
   - **PrtSc** (`0xb7`) sets `game_flags` bit `0x40`; `FrameEnd` saves the screenshot at the end of the frame;
   - a debug Backspace (`0x800e`) would flush the key queue, but the debug modifier that `key_mark` adds is
     `0x800`, so that code never arrives (dead code);
   - these three return 0; any other code is returned.
4. `interface_frame(code)` (`0x4119a0`) offers the code to the open screen's handler. With no
   screen open, `hud_frame` → `hud_handle_keys` handles it.

**Key codes:** the DirectInput-style scan code, plus `0x1000` for Shift, `0x2000` for Alt and `0x4000` for Ctrl (`key_mark`
and `key_get_shift_status` set them from scan codes `0x2a`/`0x36`, `0x38`/`0xb8` and `0x1d`/`0x9d`; **observed**,
corrected 2026-10-15: this page said `0x4000` was Alt).
Held keys are polled separately with `key_is_down` / `key_down_time`. Examples: the camera
arrows, and the modifier keys in the mouse input.

Nothing in v1.40 uses DirectInput, neither the keyboard nor the [mouse](vsdk-gr.md#mouse-0x500a30-0x501468).
The mouse always runs in cursor mode: `mouse_use_cursor` (`0x5a5734`) is 1 in `.data` and nothing clears it,
so `mouse_di_init` returns -1 before it reaches `DirectInputCreateA` (`0x500e90`..`0x500e9b`). **Observed.**
(Corrected 2026-10-15; this page said the mouse used DirectInput.)

## Default keyboard map

The keys `hud_handle_keys` handles.

| Key | Code | Action | Function |
|---|---|---|---|
| Esc | `0x01` | Play menu (screen 9); first closes a pending prompt | `hud_key_playmenu` |
| 1-6 | `0x02`-`0x07` | Use quick slot 1-6 (spell or item) | inline |
| Tab / Shift+Tab | `0x0f` / `0x100f` | Next / previous party member | `party_cycle` |
| R | `0x13` | Character screen (2) | `hud_key_status` |
| I | `0x17` | Inventory (1) | `hud_key_inventory` |
| S | `0x1f` | Toggle the Skills panel (screen 6) | `hud_key_screen6` |
| H | `0x23` | Solo mode on/off: `party_toggle_hold` toggles `game_flags` bit `0x1000` and makes the other party members hold (cleared in towns). When the byte at `0x60acd5` is set, it first asks "Enter solo mode?" / "Leave solo mode?" (**observed**) | `hud_toggle_solo_mode` |
| X, middle mouse | `0x2d` | Interact with the nearest usable object (within 3 units) | `hud_interact_nearest` |
| C | `0x2e` | Toggle the Spells panel (screen 5) | `hud_key_screen5` |
| M | `0x32` | Toggle automap (3) | `hud_key_automap` |
| Space, NumLock | `0x39`, `0x45` | Pause / unpause | `hud_key_pause` |
| F1 | `0x3b` | Help (22) | `hud_key_help` |
| F2 | `0x3c` | Options (screen 12) | inline |
| F5-F9 | `0x3f`-`0x43` | Select party member 1-5 | `hud_select_party_member` |
| F12 | `0x58` | Cycle camera mode (0, 1, 2) | `hud_key_camera` → `camera_cycle_mode` |
| Q, Enter, F3, F4, Ctrl+S, Ctrl+L | | Single player only (`game_flags & 1`; bit `0x1` means single player, set by `state_boot_enter` at `0x431c65`, not "debug"). These include the journal (`hud_key_journal`), the combat-mode toggle (`hud_toggle_combat_mode`) and save (`hud_key_save`) (**observed** in `hud_handle_keys`); which key does which is not mapped yet, except Ctrl+S: quick save (seen in game 2026-10-17, "Quick save successful.") | `hud_handle_keys` |

Confirmed from in-game frame captures (2026-10-13).

## Mouse and camera

`player_read_mouse_input` (`0x4344b0`) fills `player_input` (`0x5fcc98`) every frame:

| Flag | Meaning |
|---|---|
| `0x04` | Left button pressed outside the panels (move / act). It is assigned, not added, so it drops the Shift, Alt, right-button and held bits set earlier in that frame; `0x08` is then added with Ctrl |
| `0x10` | Left button still held at least 400 ms after a press outside the panels |
| `0x01` / `0x02` | Right button pressed / released (outside the panels) |
| `0x20` / `0x40` | Shift / Alt held |
| `0x40000000` / `0x80000000` | Left press while the grave key (`0x29`) is held, with / without Shift (`0x434609`..`0x434633`) |

`camera_process_input` (`0x434660`) controls the camera:
- **Rotation:** Left/Right arrows, or the mouse within 3-4 pixels of the left/right screen
  edge (`gr_screen_width`).
- **Zoom:** Up/Down arrows (Up zooms in). The game has no tilt key: the arrows feed the zoom rate, and only
  the low/high camera presets change the angle (tested 2026-10-15; see [Camera](camera.md#input-turning-and-zoom)).
- **Zoom (mouse):** the mouse wheel, in steps of 10.
- **Settings:** a speed multiplier at `0x60ad00` and an invert flag at `0x60acd1`.

## Hook for custom bindings

`key_remap_layout(code) -> code` is called once per key event, before anything else sees
the key. So far it holds only a few `if` swaps for the keyboard layouts. To support custom
bindings, SummonerFix can replace it (a 5-byte `JMP` at `0x509560`) with a function that
looks up a 256-entry table loaded from `summonerfix.ini`. Remapped keys would then reach:
- the key queue, and so the keyboard map above;
- `key_is_down` and `key_down_time` (camera arrows, modifiers).

The layout remapping still has to be kept (it depends on `key_layout`, `0x25daa50`), and
console keys stay separate. Mouse buttons do not pass through this function; remapping
them would need a hook in `mouse_update`.

## Mouse buttons

The mouse message handler (`0x500f60`, registered by `mouse_init`) maps `WM_LBUTTONDOWN`...`WM_MBUTTONUP` to `mouse_mark_button(button 0-2, down)` (`0x501100`). That function updates the live button state, the press/release counters read by `mouse_was_pressed`/`mouse_was_released` (`0x501360`/`0x501390`), and a double-click flag, under `mouse_lock`. Double clicks are the game's own: a second press within 400 ms
(`mouse_double_click_time`, `0x5a5738`, a RealtimeTimer per button) sets the flag; `WM_LBUTTONDBLCLK`,
`WM_RBUTTONDBLCLK` and `WM_MBUTTONDBLCLK` go to the handler's default case and are ignored (jump table
`0x500fec`). **Observed.** The mouse wheel (`0x20a`) is added to `mouse_wheel`. Side buttons (`WM_XBUTTON*`) are ignored by the game.

## Text entry

Save names, the console, chat and input boxes read the same key queue, and turn codes into characters with `key_to_ascii` (`0x509ca0`, 23 callers). Bindings must not apply there: SummonerFix's replacement translates each code back to the physical key that produced it before the lookup.

## Chain attacks

A chain attack is started with a **right-click**: `chain_attack_select` (`0x43c830`) runs only in a frame where the right mouse button was pressed (`player_input` flag `0x01`). There is also a 1-second cooldown (a `GameTimer` of 1000 ms). The function checks which modifier key is held at that moment and picks one of the character's four chain-skill slots
(`character_get_chain_skill(char, slot)`, `0x449440`):

| Held key | Slot |
|---|---|
| Shift (left or right) | 1 |
| Ctrl (left or right) | 2 |
| Alt (left or right) | 3 |
| CapsLock | 0 |
| none | The character's default slot (`+0x184`). After a chain made this way it rotates 0 → 3 → 1 → 2 (`character_cycle_default_chain`) |

Holding the key alone does nothing; hold it and right-click. A slot with no skill shows "No chain skill configured!". Skills are assigned per slot with
`character_set_chain_skill` (`0x4493e0`).

These are held-key checks (`key_is_down`). The [key-binding hook](#hook-for-custom-bindings)
runs before the key-down state is stored, so a binding such as `Q=LShift` makes Q select
chain slot 1. It also makes Q act as Shift everywhere else, for example in Shift+Tab and
Shift+click.

## Switching away from the game (Alt+Tab)

`os_wndproc` (`0x530ff0`) answers `WM_ACTIVATE`/`WM_ACTIVATEAPP` with `SetWindowPos(main_wnd,
HWND_TOPMOST or HWND_NOTOPMOST, 0, 0, screen width, screen height, 0)`. Flags 0 include
activation, so the call made on *losing* focus brought the game straight back. SummonerFix
replaces the focus-loss call with one that sends the window to the bottom without activating it, and adds `SWP_NOACTIVATE` to the focus-gain call. It also adds an in-game hotkey (`[Game] ConfigKey`, default F10) that opens `SummonerFixConfig.exe` (see
[SummonerFix](../summonerfix/index.md)).
