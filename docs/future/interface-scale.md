---
title: Interface scale
parent: Projects
nav_order: 5
---

# Interface scale (2026-11-10)

**Summary.** `[Display] InterfaceScale` (percent, 50-100, default 100) makes the 2D interface (menus, HUD, inventory
and the other panels) smaller relative to the screen while the 3D world keeps the screen's full resolution. It works
by giving the game a taller resolution at the screen's shape (75% on a 16:9 screen: 1820x1024 instead of 1366x768)
and placing the unchanged 1024x768 layout inside it: the HUD at the bottom, menus and full-screen panels centred.
A size **above** 100% is not possible without rewriting the interface (below). The vc6 sweep after the change: 5,618 matched, 0 lost (Observed 2026-11-10). Implemented 2026-11-10 in the
64-bit player build and the 32-bit fixed and experimental builds (through SummonerFix's display layer for those);
the original `Sum.exe` keeps 100%. Written and built; run in the game on 2026-11-10 (the 64-bit build, OpenGL and Vulkan; see the end).

## How the display and the interface interact today (Observed in the source, 2026-11-10)

Three sizes are involved:

1. **The game resolution** (`gr_screen.max_w` x `max_h`): `[Display] GameWidth` / `GameHeight`, through the registry
   and resolution-table slot 2 (`CustomResolution` in `game/summoner.cpp`; SummonerFix's `ApplyDisplayPatches` for the
   original exe). `GameWidth=Auto` (since 2026-10-24) is 768 high at the screen's aspect (`src/summonerfix/screensize.h`).
2. **The window** (the monitor in borderless full screen, or `WindowWidth` x `WindowHeight`).
3. **The output map** (`modern/display.cpp` `get_map`, SummonerFix `display.c` `GetMap`): every `glViewport` the game
   sets in game pixels is scaled by `min(window_w / max_w, window_h / max_h)` and centred. The Vulkan back end uses the
   same map (`modern_display_map`).

The consequence that matters: the game resolution is **only a coordinate space**. Everything is rasterised at the
window's resolution, because the viewports are mapped before drawing (not a small picture stretched afterwards). So
the 3D world is always sharp at the monitor's resolution, and the game resolution decides only how large one *game
pixel* is on the screen.

The 2D interface is drawn 1:1 in game pixels at absolute positions from tables per `resolution_index`, e.g. every
HUD button's `y = d->pos[resolution_index][1]` (`game/new_hud.cpp`, `hud_init`, line 271), and the menus' table rows
([UI tables](../sum-exe/vsdk-ui.md)). With `resolution_index` 2 that is the 1024x768 layout. The only interface code
that reads the screen height is popup centring (`ui_choice_popup_y`, `ui_message_box_y`), tooltips' bottom limit,
the "Game Over" text and the credits' scroll start (`unnamed_interface_0x411270.cpp`, `new_hud.cpp`,
`new_credits.cpp`), and `interface_set_view_clip`, which sizes the 3D view as `height - hud_get_panel_height()`.

So **the interface's size on the screen is `768 / GameHeight` of the screen height**: `GameHeight` already is an
interface-scale knob. What was missing is **placement**: the layout is anchored at the game screen's top-left by its
absolute positions. Widescreen (2026-10-13) solved this in x: the menus and full panels are shifted as a whole picture
by the output map, and the HUD-type screens by shifting the interface's calls to the 2D primitives
(`experimental/ui_shift.h`, `experimental/widescreen.cpp`; SummonerFix `widescreen.c` for the original exe).
Nothing did it in y, so with `GameHeight` above 768 (Inferred from the positions above; not run):

- the HUD bar would sit at lines 672-768 of, say, 1024, with the 3D view (`height - bar`) running behind and below it;
- menus and full panels would sit at the top of the screen, with an empty band under them.

The config app said as much: "all sizes are 768 high". `GameHeight` above 768 was possible but not usable.

## What an interface scale can mean here

**Smaller than 100%: feasible.** A taller game resolution at the screen's shape (`768 * 100 / scale` lines) makes the
layout cover `scale` percent of the screen height. The 3D view keeps the window's pixels and the screen's aspect
(`gr_screen.aspect` follows `width / height`, and `GameWidth` grows with `GameHeight`), but it is not identical: on the
HUD screens it is `height - bar`, so with a smaller bar it covers more of the screen (16:9 at 75%: 1820x928 game
pixels, 98% of the height, against 1366x672, 87.5%), and `camera_fov`, which `interface_set_view_clip` derives from that
clip every frame, follows the slightly wider clip (Inferred from the code). Only the placement in y has to be added,
by the same two mechanisms as in x.

**Larger than 100%: not feasible without rewriting the interface.** At 100% the 768-line layout already fills the
screen height. A larger interface would have to drop or overlap parts of the layout (the HUD bar is at the bottom of
the 768 lines, the menus fill all of them), or be laid out anew. Evidence: the positions are absolute table values per
`resolution_index` (above); the game's own resolution scaling (`ui_scale` = `screen_w / base_w`,
`interface_init_global`) only chooses between the 640, 800 and 1024 layouts, and there is no 1280 or larger layout in
the data.

**A scale applied in the output instead (rejected).** One could keep the game resolution and scale only the 2D pass:
map the viewports of interface draws with a second, smaller transform. This fails on: the full panels' item and
character models, which the 3D renderer draws inside the panel in screen coordinates (the reason widescreen shifts those
panels as whole pictures); 2D drawn by non-interface code (the cursor in `unnamed_frame_0x4323e0.cpp`, the status
message, the frame counter) that would have to be told apart from interface drawing per call; fonts and clip
rectangles, which would need the transform at every `gr::set_clip`; and two back ends (OpenGL and Vulkan) that would
both need it. It is a rewrite of the 2D path, for the same result as the taller resolution.

## The design (implemented 2026-11-10)

- **The size.** `screensize.h` `DisplaySizeForAspectScaled`: height `768 * 100 / scale` (even), width at the
  screen's aspect (even), never narrower than `1024 * height / 768`. 16:9: 100% 1366x768, 90% 1518x854,
  80% 1706x960, 75% 1820x1024, 67% 2038x1146, 50% 2730x1536. Applied only with `GameWidth=Auto`, OpenGL or Vulkan,
  and an exe that places its interface itself: the 64-bit host (`modern/registry.cpp`, `FIXED_BUILD` builds) and, for
  the 32-bit rebuilt exes, SummonerFix when the exe's `.fixes` file lists `InterfaceScale` (`main.c`). An explicit
  `GameWidth` / `GameHeight` above 768 gets the same placement in those exes.
- **In play (state 4), the HUD-type screens** (HUD, pickup, play menu, dialogue, assess): the call wrappers of
  `ui_shift.h` add `height - 768` to y, besides the x shift. The layout is anchored at the **bottom**: the HUD bar ends
  at the screen's bottom edge, and the 3D view above it keeps the real height (`exp_ws_view_height` in
  `interface_set_view_clip`). The play menu, dialogue and pickup boxes are drawn in the same layout, so they sit in the
  lower 768 lines rather than at the screen's centre (a consequence, accepted). Interface code sees a 768-high screen
  (`ui_gr_screen_height`), so its popups centre in the layout.
- **Side panels** (map, spells, skills, item trade): x unshifted as before, y shifted with the HUD, so the panel stands
  on the HUD bar.
- **Full panels in play and the menus**: the output map moves the picture down by `(height - 768) / 2` game lines as
  well as right by `(width - 1024) / 2` (`[Display] CenterMenus`); the bars outside the 1024x768 picture are cleared
  for menus. GL viewports count from the bottom and the mouse from the top, so the map keeps both (`off_y`,
  `off_y_top`). The full panels' 3D view is 1024x768 (`exp_ws_view_width` / `exp_ws_view_height`).
- **The mouse**: the interface's cursor (`game_mouse_x / y`) gets the same x and y shift as the drawing
  (`exp_ws_mouse_tick`, `ui_mouse_get_pos`); world code reads the raw position (`exp_mouse_raw_x / y`), as for x.
  The output map converts window to game pixels with the same offsets the picture uses, so clicks land on what is
  drawn (Inferred).
- **Movies** outside play are centred in the 768 lines as in the 1024 columns (`unnamed_movie_0x5172f0.cpp`).
- **The field of view** is measured once by `interface_init_global` on a `width x 0.875 height` clip; it keeps the real
  height (`exp_ws_real_screen_height`), so the 3D view and `[Display] FieldOfView` are the same at every scale.
- `[Display] CenterHud=0` turns the in-play placement off (the original placement), and with it the taller size, so
  `InterfaceScale` then does nothing; `CenterMenus=0` turns off the menus' centring.
- The config app's "Match my screen: W x H" row shows the 100% size whatever `InterfaceScale` is (cosmetic).
- Screenshots above 1024 lines: Print Screen writes a Targa there (game-bugs C290), in these builds too
  (`experimental/gameopts.cpp`), so 50-74% are safe.

Where it is not done: the original `Sum.exe` under SummonerFix. Its in-play placement is SummonerFix's call patching by
address range (`widescreen.c`), which would also need `gr_screen_height` (`0x506180`) hooked for the interface's
callers, the y argument of every patched primitive, the mouse's y, and `interface_set_view_clip`'s height; without
all of them the HUD would float mid-screen, so SummonerFix does not give the original exe a taller size.

Known open points (Unknown until run):

- The world map (level 4) places its location labels at projected world positions and draws them through the
  interface wrappers, so they get the interface shift. That was already so in x at a wide `GameWidth`; in y it adds
  `height - 768`. Not checked in the game for either.
- Things the game draws over the whole screen from interface code (the item-info popup's dimming rectangle) cover
  only the 1024x768 layout, as in x today.

## Test recipe (for the lead, not yet run)

Screen 2560x1440 (capture = half the window, 1280x720). `summonerfix.ini`: `[Display] GameWidth=Auto`,
`Renderer=OpenGL` (and once `Vulkan`), `[Debug] Agent=1`, `TestSaves=1`.
The positions follow from the map: window scale `s = min(2560 / W, 1440 / H)`; a menu point at layout `(lx, ly)` is at
capture `(s/2 (dx + lx), s/2 (dy + ly))` with `dx = (W - 1024) / 2`, `dy = (H - 768) / 2`; a HUD point at
`(s/2 (dx + lx), s/2 (H - 768 + ly))`.

| | 100% (1366x768, s = 1.874) | 75% (1820x1024, s = 1.406) |
|---|---|---|
| Main menu, Load Saved Game | (858, 368) | (804, 366) |
| HUD, Joseph's portrait | (275, 660) | (366, 675) |

1. `InterfaceScale=100`: start, `capture is100-menu`, `click left 858 368`, `info` (the load screen), load a play-through
   save, `capture is100-hud`, `click left 275 660`, `info` (screen 2, the character screen), `key Escape`, `key I`,
   `capture is100-inv`.
2. `InterfaceScale=75`: the same with the 75% positions; the log (`experimental.log` with `[Debug] ExperimentalLog=1`)
   says `widescreen: 1820x1024, interface shift 398, 256`, and `modern.log` `registry: InterfaceScale 75%`.
3. Check: the HUD bar at the bottom edge, the 3D world above it to the top; the inventory and main menu centred with
   black bars; each click confirmed by `info` (the load screen; screen 2 after the portrait); the cursor's tip on what
   it clicks; one click on the ground at 75% walks the party to the spot under the cursor (`party` before and after),
   since world clicks use the raw position.
4. The 32-bit fixed or experimental exe needs its `.addr` table for SummonerFix's rebuilt mode, so use a build from
   the main checkout (where the hybrid table exists), with the SummonerFix of this change.

## Run 2026-11-10 (Observed)

The 64-bit player build in the test folder (2560x1440 screen, `GameWidth=Auto`):
- `InterfaceScale=100`: `widescreen: 1366x768, interface shift 171, 0`, as before the change.
- `InterfaceScale=75`: `registry: InterfaceScale 75%`, `widescreen: 1820x1024, interface shift 398, 256`. The main menu
  and the Load screen centred with bars, clicked at the positions from the formula above; a save loaded; the HUD bar
  smaller on the bottom edge with the world above it; a click on the third portrait selected Rosalind; I opened the
  inventory, centred; a click on the ground walked Rosalind forward (z 38.5 -> 27.3).
- The same at 75% under Vulkan with the validation layer on: no messages; the map panel opened on the left above the
  HUD bar.

Not checked: whether the cursor's tip and a ground click land on the exact spot (only the direction was seen), the
world map's labels, conversations, and 50%. The recipe's "screen 2 after the portrait" was wrong: a portrait click
selects the hero, it opens no screen.
