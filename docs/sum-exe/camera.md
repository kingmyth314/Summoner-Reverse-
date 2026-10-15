---
title: Camera
parent: Sum.exe
nav_order: 6.15
---

# Camera
{: .no_toc }

The game camera is one static object, `game_camera` (`0x60a018`). It orbits the party leader at
a fixed distance (6.0), and it has two presets: a low view and a high view. A mode picks the
preset: low, high, automatic (high while an enemy is near), or forced high by the level layer.
The player can only turn the camera and zoom it. Zoom is clamped to `[0.3, camera_zoom_max]`.
Cutscene shots, conversations, slew mode and the script editor take the camera over through a
small state stack. **Observed** in code; the zoom keys were tested in agent mode.

1. TOC
{:toc}

## The object

`camera__camera` (`0x478d70`) builds `game_camera` at startup (`init_game_camera`, `0x438d70`).
`camera_frame` (`0x478990`) runs once per frame from `player_input_frame`. **Observed.**

| Offset | Meaning | Evidence |
|---|---|---|
| `+0x000` | mode (0-5, see below); `+0x004` previous mode | `camera_set_mode` `0x479930` |
| `+0x008` / `+0x009` | "forced high view suppressed" (from the detail level) / "forced high view requested" | `detail_apply_level_settings` `0x4873e3`, `camera_set_mode` `0x47994b` |
| `+0x00c` | flags (table below) | |
| `+0x010` | state stack depth (max 8); slots of `0x24` bytes from `+0x014` | `camera_push_state` `0x4797b0` |
| `+0x134` / `+0x138` | zoom remembered for the high / low view (start 1.0 / 0.8) | `camera__camera`, `camera_frame` `0x478bb1` |
| `+0x13c` | running camera sequence (a cutscene shot) | `camera_start_sequence` `0x4792c0` |
| `+0x140` | look-at point: target position + centre offset, y + `+0x194` | `camera_update_target_pos` `0x478e00` |
| `+0x14c` | target object (the leader) | `camera_set_target` `0x478ff0` |
| `+0x150` | fixed eye position used in mode 5 | `camera_set_target` `0x47907e` |
| `+0x15c` | wanted direction for auto-turning | `camera_auto_rotate_update` `0x477ca0` |
| `+0x180` | yaw (radians, wrapped to ±2π) | `camera_apply_input_deltas` `0x477af0` |
| `+0x184` | zoom: scales both distance and elevation | `camera_compute_eye_pos` `0x477ea0` |
| `+0x188` | elevation of the eye above the target, per unit of zoom | same |
| `+0x18c` | vertical field of view (degrees) | `interface_set_view_clip` `0x412380` |
| `+0x190` | distance | same |
| `+0x194` | look-at height offset | `camera_update_target_pos` |
| `+0x198` / `+0x19c` | eye height offset, current / wanted (moves at 1.5 per second) | `camera_update_eye_position` `0x478325` |
| `+0x1a4` / `+0x1b0` | follow camera eye position / orientation matrix | `camera_snap_to_target` `0x478e70` |
| `+0x234` / `+0x238` | target height, smoothed / wanted | `camera_smooth_height` `0x478700` |
| `+0x23c` / `+0x240` / `+0x244` | this frame's input: yaw, zoom, elevation rates | `camera_set_input` `0x479170` |
| `+0x248` / `+0x254` | origin and rotation applied to camera sequences | `camera_set_sequence_transform` `0x479ad0` |
| `+0x278` / `+0x284` | the rendered view: `camera_view_pos` (`0x60a290`) and `camera_view_orient` (`0x60a29c`) | `0x60a018 + 0x278 = 0x60a290` |

The eye position is (**observed**, `camera_compute_eye_pos`):

```
eye.x = look.x + sin(yaw) * distance * zoom
eye.z = look.z + cos(yaw) * distance * zoom
eye.y = elevation * zoom + height(+0x234) + eye_offset(+0x198)
```

So zooming slides the eye along a fixed line toward or away from the leader. **Observed.**

Flags at `+0x0c` (**observed** unless marked):

| Bit | Meaning |
|---|---|
| `0x1` | The follow camera drives the rendered view: `camera_copy_follow_view` (`0x4790d0`) copies `+0x1a4..+0x1d0` to `+0x278..+0x2a4` |
| `0x2` | Follow the target (input, smoothing, auto view run only with a target and this bit) |
| `0x8` | A camera sequence is running |
| `0x10` | Free look (slew); runs only while `0x1` is clear |
| `0x20` | Default view values stored (`camera_set_default_view_params`) |
| `0x40` | Blocks zoom input. `camera_auto_rotate_update` sets it while it turns the camera (`0x477e1f`) and clears it again, so the player cannot zoom during an automatic turn (**observed**) |
| `0x80` | Sequence origin and rotation are set |
| `0x100` | Restore zoom from `+0x1a0` when auto-turning ends |
| `0x200` | Go back to mode 3 after mode 5 |

`camera_resume_follow` (`0x479790`) sets `0x1 | 0x2`. `camera_set_follow_flags(follow, free_look)`
(`0x479740`) clears `0x1`, `0x2` and `0x10`, then sets `0x2` and `0x10` from its two arguments.
**Observed.**

## Modes

`camera_set_mode` (`0x479930`) applies a mode. Low and high are presets set by
`camera_set_view_params(distance, elevation, zoom, look height, eye offset)` (`0x479830`).
**Observed.**

| Mode | What it is | Set by |
|---|---|---|
| 0 | Low view: `(6.0, 2.0, 0.8, 3.3, -1.5)`, fog default 20 / 30 | F12 |
| 1 | High view: `(6.0, 7.0, 1.0, 0.3, 1.0)`, fog default 35 / 50 | F12 |
| 2 | Automatic: low view, high view while an enemy is near | F12, the default (`camera_reset_default`, `0x478f60`) |
| 3 | Forced high view for a level layer | `level_set_current_layer` (`0x482110`), level scripts |
| 4 | Not stored: "go back to the previous mode" (`+0x004`, or 3 after flag `0x200`) | leaving a mode-3 layer, the end of mode 5, `camera_leave_level` |
| 5 | Glide to a fixed eye point, then go back (mode 4) | `camera_set_target` |

- The fog defaults apply only when the layer has no fog distance of its own
  (`fog_set_low_camera_default` `0x4870b0`, `fog_set_high_camera_default` `0x487080`). **Observed.**
- After a change, zoom is set to the value remembered for that view: `+0x138` when the
  elevation is below 4 (low), else `+0x134`. `camera_frame` writes the current zoom back to the
  same slot each frame. So the low and high views keep separate zoom levels. **Observed.**
- **F12** (`hud_key_camera` `0x40e980` → `camera_cycle_mode` `0x479a60`) cycles 0 → 1 → 2 → 0.
  On the world map (`cur_level_index` 4) it does nothing. In mode 3 it does nothing, and
  `hud_key_camera` shows a status message for 3 s and plays sound `0x27`. **Observed.** See
  [Controls](controls.md).
- **Automatic view** (`camera_auto_view_update` `0x4787b0`) checks once a second. It switches to
  high at once when `camera_enemy_near_leader` (`0x4796a0`) is true: the leader's current target
  is an enemy, or a living enemy is within 8 units (squared distance 64). It switches back to low
  after 5 s without one. It does nothing while `game_flags` bit `0x100` is set (*inferred*:
  a conversation, set by `player_talk_to` and `script_object_talk`), while `Ai_paused`, or in
  solo mode (`0x1000`) while the low view is on. **Observed.**
- **Level layers.** `level_vis_parse` (`0x486d40`) reads `$high_camera` per vis set into
  `level_layer_vis_flags` (`0x22ea4a4`). When the leader enters a layer with value 1,
  `level_set_current_layer` asks for mode 3; value 0 asks for mode 4. This is skipped while
  flag `0x1` is clear or a sequence runs. **Observed.** What `$low_camera` stores is *unknown*
  (it writes a register value that was not traced).
- **Detail level.** A flag in the detail record (`+0x24` or `+0x25`) sets `+0x008`. While it is
  set, a request for mode 3 is only remembered in `+0x009`; clearing it later applies mode 3.
  **Observed.** *Inferred:* low detail settings keep the player's own camera in high-view areas.
- **Leader change.** `camera_set_target` snaps the camera when the new target is more than
  10 units away. Closer, it stores the new follow eye in `+0x150` and enters mode 5: the eye
  glides there, and the camera returns to the previous mode once the new leader moves
  (`+0x698` above 0.01) or the glide settles. **Observed.**

## Input: turning and zoom

`camera_process_input` (`0x434660`) runs from `player_input_frame`. **Observed:**

| Input | Effect |
|---|---|
| Left / Right arrows | yaw rate +1 / -1 (`camera_key_yaw` `0x434840`: Left minus Right) |
| Mouse within 3 px of the left / right edge | yaw rate -1 / +1, only with no mouse button down (the opposite sign of the arrows; how that looks on screen is *unknown*). A block meant to hold the edge turn back after a held left-button drag until the cursor leaves the edge (`0x5fccb8`, reset at `0x4347c2`..`0x4347e6`) can never be set: the test reads the held flag it has just stored (`0x4346e7`..`0x43471b`, game bug K191) |
| Up / Down arrows | zoom rate (`camera_key_zoom` `0x434870`: Down minus Up) |
| Mouse wheel | zoom rate -10 per notch forward, +10 back |
| Options: camera speed `0x60ad00` | multiplies all three rates |
| Options: invert `0x60acd1` | negates the yaw rate only |

{: .important }
Up/Down and the wheel both feed the zoom rate `+0x240`, not a tilt. The elevation rate `+0x244`
comes from `camera_key_extra_axis` (`0x4348a0`), which always returns 0. So the player can never
tilt the camera; only the low/high presets change the angle. This corrects "Tilt: Up/Down" on the
[Controls](controls.md) page. **Observed**, and **tested 2026-10-15 in agent mode**: holding Up for 0.4 s
lowered `+0x184` (zoom) from 0.91 to 0.52 while `+0x188` (elevation) stayed 2.0. Up zooms in, Down zooms out.

The input is skipped while `game_flags` bit `0x100` is set (see above), and while the script
editor has a window open ([Script editor](script-editor.md)). On the world map only yaw is read.
**Observed.**

`camera_apply_input_deltas` (`0x477af0`), each frame (**observed**):

- yaw += rate × frametime × 2π / 6 in the low view, / 7.5 in the high view. At speed 1 a held
  arrow turns a full circle in 6 s or 7.5 s.
- zoom += rate × frametime, then clamped to `[0.3, camera_zoom_max]`. Held Down at speed 1 zooms
  from 0.3 to 1.0 in 0.7 s.
- elevation is clamped to `[-2, 9]`. On the world map elevation is fixed at 12 and zoom at 1.0.

## Following and smoothing

`camera_frame`, while there is a target and flag `0x2` (**observed**):

1. `camera_update_target_pos`: the look-at point from the target position.
2. `camera_apply_input_deltas`, then `camera_auto_rotate_update` (turns the yaw toward `+0x15c`).
3. `camera_smooth_height` with the target's y. A new wanted height is taken only when it differs
   by more than 0.3, so small steps in the floor do not move the camera. The height then moves
   at 4 × difference per second and snaps when within 0.001.
4. `camera_update_eye_position` (`0x478130`): each axis of the eye moves toward the wanted eye
   with `camera_approach_value` (`0x478450`), a velocity step that snaps on overshoot.
5. `camera_update_orientation` (`0x477f60`): the view looks at the target; it is set directly
   while the target moves (`+0x168` = 2), else smoothed.
6. `camera_find_occluders` (`0x478520`), then `camera_auto_view_update`, then
   `camera_copy_follow_view` copies the result into the rendered view.

`state_game_frame` then adds the active effect camera offset (`vfx_get_camera`, a camera shake)
to `camera_view_pos` / `camera_view_orient` and passes them and `+0x18c` to `gr_setup_3d`.
**Observed.** See [vsdk gr](vsdk-gr.md).

**Field of view.** The constructor and `camera_level_reset` (`0x478d20`) set 73.74°.
`interface_set_view_clip` (`0x412220`) overwrites it every frame outside cutscenes with
`gr_fov_to_vertical(interface_view_fov)` for the current viewport (side panels make it
narrower). **Observed.**

## Walls and level geometry

There is no camera collision. `camera_find_occluders` casts a ray from the eye to the target
(`lobj_raycast_draw_lists`) and lists up to 32 level objects in the way (`camera_occluders`
`0x22e49b8`). `levelrender_fade_occluders` (`0x4829e0`) fades them out. The eye is never moved.
**Observed.** The console toggle `geom_avoid` sets `Camera_geometry_avoidance` (`0x22e4b0c`), but
nothing else reads that byte. **Observed:** the only references are in `cmd_geom_avoid`.

## Free look and slew

`camera_free_look_update` (`0x4791a0`) runs while flag `0x10` is set and `0x1` is clear. With
either Ctrl key held, the mouse delta × `mlook_sensitivity` (`0x59b7fc`, 1.0) × 0.01 radians
turns the rendered view. `movement_read_controls` (`0x531f50`) then moves it. **Observed.**
The console command `slew` (`slew_set_enabled` `0x4348b0`) is the only retail way in: it pushes
the camera state, calls `camera_set_follow_flags(0, 1)` and sets the slew limits
(`movement_init(10.0, 1.0, 0.1, 0.1)`). Turning slew off pops the state. **Observed.**

## Taking the camera over

`camera_push_state` / `camera_pop_state` (`0x4797b0` / `0x4797d0`) save 9 dwords: flags,
distance, yaw, zoom, elevation, fov, look height, mode, previous mode
(`camera_get_state` `0x478890`). The same 9 dwords are the `CMRA` chunk of a
[savegame](savegame.md). **Observed.**

| User | Takes over | Gives back |
|---|---|---|
| Cutscene shot | `camera_start_sequence`: ends the last shot (pop), push, `camera_set_follow_flags(0, 0)`, sets flag `0x8` | `cutscene_cleanup` → `camera_end_sequence(1)` (`0x4797f0`): free the shot, pop, clear flag `0x8` and `game_flags` `0x800` |
| Conversation | `conversation_camera_begin` (`0x4da890`): push, `camera_set_follow_flags(1, …)` | `conversation_camera_update` (`0x4d9d90`): pop or `camera_resume_follow` |
| Slew | `slew_set_enabled(1)` | `slew_set_enabled(0)` |
| Script editor | `script_editor_handle_input`: push, `camera_set_follow_flags` | `script_editor_close` |

While a shot runs, `camera_update_sequence` (`0x479480`) writes the rendered view
(`+0x278`/`+0x284`) from the shot's position and orientation. It sets the fov from the shot
(`shot +0x8 × 1/π`). It is frozen while paused (`game_flags` `0x4`). The follow eye at
`+0x1a4` is not touched, so the pop cuts straight back to the gameplay view. **Observed** code;
the cut is *inferred*. Shot files, timing and events are on [Cutscenes](cutscenes.md).

**Sequence transform.** `camera_set_sequence_transform(pos, matrix)` stores an
origin and a rotation. When flag `0x80` is set, every shot position is unrotated by that matrix
and moved by that origin, and the shot orientation is multiplied by it. **Observed.**
`cutscene_play` clears it. `summon_create` sets it right after `cutscene_play`, to the summon's
position and a rotation about the y axis. So summon cutscenes (the dragons, see
[Summons](summons.md)) are filmed around wherever the summon appears. **Observed** order;
*inferred* purpose. Until 2026-10-15 this function was called `camera_set_focus_override`.

`camera_cut_to_view_stub` (`0x479430`) has an empty body. Three level-script helpers
(`lsf_iona_ext_flag_chick_cam`, `lsf_iona_ext_flag_wood_cam`, `lsf_lenele_flag_cart_played`)
pass it a navpoint view, so those scripted camera cuts never happen in retail. **Observed.**

## UI model cameras (icam)

`new_icam_load` (`0x410760`) does not touch the game camera. It reads `$Name:`, `$Offset:` (3
floats) and `$PBH:` (pitch, bank, heading) entries for items (`item_type_find`) and characters
(`charinfo_find_by_name`). The tables are `bs_cameras.tbl` (shop), `inv_cameras.tbl` (inventory)
and `status_cameras.tbl` (Character screen). `new_icam_get` (`0x410b70`) returns the offset and
matrix for the 3D model views in those screens. **Observed.**

## Console commands

| Command | Function | Effect (**observed**) |
|---|---|---|
| `zoom_limit [on\|off]` | `cmd_zoom_limit` `0x4778e0` | on: `camera_zoom_max` = 1.0; anything else: 1000.0. Not a toggle despite its help text |
| `mlook_sensitivity <f>` | `cmd_mlook_sensitivity` `0x477a80` | sets the free-look scale |
| `geom_avoid` | `cmd_geom_avoid` `0x477990` | toggles an unused byte (see above) |
| `slew` | `cmd_slew` `0x4343d0` | free-flying camera |
| `dcamera` | `cmd_dcamera` `0x4db1f0` | dialogue camera option 0-2 (`0x60acd8`) |

See [Console](console.md).

## Tips

- **CameraZoomMax** ([SummonerFix](../summonerfix/index.md)) only raises the clamp at
  `0x59b800`. **Observed.** The eye is `6 × zoom` back and `2 × zoom` (low) or `7 × zoom` (high)
  up. At zoom 3 in the low view the eye is about 19 units from the leader. With the default low
  fog (far 20) and the fog-distance cull in `character_is_in_view` (`0x44f020`), the leader's
  surroundings fade out first. *Inferred.*
- Typing `zoom_limit` in the console overwrites a SummonerFix value with 1.0 or 1000.
  **Observed.**
- Zoom is saved in the `CMRA` chunk. A save made with a high CameraZoomMax loads fine without
  it; the next frame clamps zoom back to 1.0. *Inferred.*
- Objects between the camera and the leader are faded, not avoided, and at most 32 per frame.
  At a high zoom more objects fall on the ray. *Inferred.*

## Open questions

- What `+0x19c` is meant for (the low view lowers the eye by 1.5). (Flag `0x40` is answered above.)
- What `$low_camera` stores, and which detail settings set `+0x008`.
- The byte at `0x5943fd` gates the auto-turn toward a mouse-move target in
  `world_pick_under_mouse` (`0x43b52e`). It is 1, it is never written, and it sits inside the
  skill-`0x26` pick filter table `0x5943f8`. Whether it is a separate setting is *unknown*.
- `game_flags` bit `0x800` cleared by `camera_end_sequence`.

## Evidence

These are function and instruction addresses read with `tools/asm.mjs`, not REA evidence IDs.
Float constants were read from `Sum.exe` (`0x581a38` 6.0, `0x581a34` 7.5, `0x580e18` 4.0,
`0x581970` 0.3, `0x581a28` -2.0, `0x580e20` 9.0, `0x581a2c` 12.0, `0x581a5c` 64.0,
`0x5818ec` 0.01, `0x59b804` 1.5).

| Claim | Where |
|---|---|
| Mode cycle, world map block | `camera_cycle_mode` `0x479a60`-`0x479acb` |
| Mode rules, remembered zoom | `camera_set_mode` `0x479930`-`0x479a51` |
| Presets | `camera_view_low` `0x478960`, `camera_view_high` `0x478930`, `camera_set_view_params` `0x479830` |
| Keys, wheel, edge scroll, invert, speed | `camera_process_input` `0x434660`-`0x434831`, `camera_set_input` `0x479170` |
| Turn rate, zoom clamp, elevation clamp | `camera_apply_input_deltas` `0x477af0`-`0x477c92` |
| Eye formula | `camera_compute_eye_pos` `0x477ea0` |
| Frame order, render copy, mode 5 return | `camera_frame` `0x478a2f`-`0x478cda`, `camera_copy_follow_view` `0x4790d0` |
| Automatic view | `camera_auto_view_update` `0x4787b0`, `camera_enemy_near_leader` `0x4796a0` |
| Layer camera | `level_vis_parse` `0x486def`, `level_set_current_layer` `0x48213a`-`0x482171` |
| Detail suppression | `detail_apply_level_settings` `0x4873e3`-`0x48741a` |
| Leader change | `camera_set_target` `0x479009`-`0x4790ab` |
| Height dead band | `camera_smooth_height` `0x478700` |
| Occluders, unused `geom_avoid` | `camera_find_occluders` `0x478520`, xrefs of `0x22e4b0c` |
| Free look | `camera_free_look_update` `0x4791b9`-`0x4792aa`, `slew_set_enabled` `0x4348b0` |
| State stack | `camera_push_state` `0x4797b0`, `camera_get_state` `0x478890` |
| Shots and transform | `camera_start_sequence` `0x4792c0`, `camera_update_sequence` `0x479480`, `camera_end_sequence` `0x4797f0`, `cutscene_cleanup` `0x4368d0` |
| Summon transform | `summon_create` `0x47ef57`, `0x47f04f`; `cutscene_play` `0x43635a` |
| Fov per frame | `interface_set_view_clip` `0x412364`-`0x412380`, `state_game_frame` `0x43912a` |
| Console | `cmd_zoom_limit` `0x4778e0`, `cmd_mlook_sensitivity` `0x477a80` |
| icam tables | `new_icam_load` `0x410760`, `new_icam_get` `0x410b70` |
