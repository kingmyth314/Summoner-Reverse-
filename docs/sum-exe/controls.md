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
3. Once per frame, `PollInputEvents` (`0x432710`) takes one code with `key_inkey` (call at
   `0x432754`):
   - **Shift+Esc** (`0x1001`) quits (`gameseq_set_state(3)`);
   - **PrtSc** (`0xb7`) sets a flag in `game_debug_flags`;
   - any other code is returned.
4. `interface_frame(code)` (`0x4119a0`) offers the code to the open screen's handler. With no
   screen open, `hud_frame` → `hud_handle_keys` handles it.

**Key codes:** the DirectInput-style scan code, plus `0x1000` for Shift and `0x4000` for Alt.
Held keys are polled separately with `key_is_down` / `key_down_time`. Examples: the camera
arrows, and the modifier keys in the mouse input.

The keyboard does **not** use DirectInput; only the [mouse](vsdk-gr.md#mouse-0x500a90-0x501090)
does.

## Default keyboard map (`hud_handle_keys`)

| Key | Code | Action | Function |
|---|---|---|---|
| Esc | `0x01` | Play menu (screen 9); first closes a pending prompt | `hud_key_playmenu` |
| 1-6 | `0x02`-`0x07` | Use quick slot 1-6 (spell or item) | inline |
| Tab / Shift+Tab | `0x0f` / `0x100f` | Next / previous party member | `party_cycle` |
| R | `0x13` | Status screen (2) | `hud_key_status` |
| I | `0x17` | Inventory (1) | `hud_key_inventory` |
| S | `0x1f` | Toggle screen 6 (*inferred*: skills) | `hud_key_screen6` |
| H | `0x23` | Toggles `game_debug_flags` bit `0x1000` or shows a prompt (*unknown* meaning) | `hud_key_h` |
| X, middle mouse | `0x2d` | Interact with the nearest usable object (within 3 units) | `hud_interact_nearest` |
| C | `0x2e` | Toggle screen 5 (*inferred*: spells) | `hud_key_screen5` |
| M | `0x32` | Toggle automap (3) | `hud_key_automap` |
| Space, NumLock | `0x39`, `0x45` | Pause / unpause | `hud_key_pause` |
| F1 | `0x3b` | Help (22) | `hud_key_help` |
| F2 | `0x3c` | Screen 12 | inline |
| F5-F9 | `0x3f`-`0x43` | Select party member 1-5 | `hud_select_party_member` |
| F12 | `0x58` | Cycle camera mode (0, 1, 2) | `hud_key_camera` → `camera_cycle_mode` |
| Q, Enter, F3, F4, Alt+S, Alt+L | | Debug only (`game_debug_flags & 1`) | |

Screens 5 and 6 sit between the save/load, skill and spell code in the
[screen list](interface.md). C for spells and S for skills is therefore likely.

## Mouse and camera

`player_read_mouse_input` (`0x4344b0`) fills `player_input` (`0x5fcc98`) every frame:

| Flag | Meaning |
|---|---|
| `0x04` | Left click (move / act); `0x08` added with Ctrl |
| `0x10` | Left button held for more than 400 ms |
| `0x01` / `0x02` | Right button pressed / clicked |
| `0x20` / `0x40` | Shift / Alt held |

`camera_process_input` (`0x434660`) controls the camera:
- **Rotation:** Left/Right arrows, or the mouse within 3-4 pixels of the left/right screen
  edge (`gr_screen_width`).
- **Tilt:** Up/Down arrows.
- **Zoom:** the mouse wheel, in steps of 10.
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

## Chain attacks

When a chain attack is possible, `chain_attack_select` (`0x43c830`) checks which modifier key is
held and picks one of the character's four chain-skill slots
(`character_get_chain_skill(char, slot)`, `0x449440`):

| Held key | Slot |
|---|---|
| Shift (left or right) | 1 |
| Ctrl (left or right) | 2 |
| Alt (left or right) | 3 |
| CapsLock | 0 |
| none | The character's default slot (`+0x184`). After a chain made this way it rotates 0 → 3 → 1 → 2 (`character_cycle_default_chain`) |

A slot with no skill shows "No chain skill configured!". Skills are assigned per slot with
`character_set_chain_skill` (`0x4493e0`).

These are held-key checks (`key_is_down`). The [key-binding hook](#hook-for-custom-bindings)
runs before the key-down state is stored, so a binding such as `Q=LShift` makes Q select
chain slot 1. It also makes Q act as Shift everywhere else, for example in Shift+Tab and
Shift+click.

## Switching away from the game (Alt+Tab)

`os_wndproc` (`0x530ff0`) answers `WM_ACTIVATE`/`WM_ACTIVATEAPP` with `SetWindowPos(main_wnd,
HWND_TOPMOST or HWND_NOTOPMOST, 0, 0, screen width, screen height, 0)`. Flags 0 include
activation, so the call made on *losing* focus brought the game straight back. SummonerFix
skips that call and adds `SWP_NOACTIVATE` to the other one (see
[SummonerFix](../summonerfix/index.md)).
