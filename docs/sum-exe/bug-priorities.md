---
title: Bug fix priorities
parent: Sum.exe
nav_order: 6.96
---

# Bug fix priorities
{: .no_toc }

1. TOC
{:toc}

**Summary:** This page ranks the 329 bugs from [Bugs in the original game](game-bugs.md) that SummonerFix does not
already fix or avoid. The score is how much the bug hurts a normal single-player game times how quickly we can
reproduce it in agent mode. At the top are the save list overwriting a save without asking (G4), the Phoenix
Rider's Inferno hitting once per party member (G215) and a crash in the Luminar fight (C250). Only 25 bugs score
9 or more. Most of the other 304 are latent, invisible, multiplayer-only, script-editor-only or minor visual slips.

{: .note }
**Draft.** The scores are a judgement made from the bug paragraphs on 2026-10-16 (*Inferred*). They do not
re-verify any bug. The ease scores assume the agent-mode setup in [Tools](../tools.md#toolsagentmjs-agent-mode).
A score changes when a bug is reproduced or a new tool makes one easier to reach.

{: .note }
**Added after the ranking (2026-10-17):** C314 (Salamanka flinch hang), C315 (multiplayer slot error loop), C316
(avoidance area index overflow) and G311 (scripted "go" lost), found while byte-matching. All four are latent (no
known retail trigger), so they rank near the bottom: impact 1-2, ease 1 (C316 might reach impact 4 if five
overlapping spell areas turn out to be reachable in a retail fight).

{: .note }
**Fixed since the ranking:** G4, G215, C250, G8, G12, G40, G170, G301, C210 with G212, G270, G211 and G210 (2026-10-17, SummonerFix `bugfix.c`, all reproduced in game first (C210: the doubled count, not the crash); C270 and G50 were withdrawn instead (they did not happen); each fix can be switched off in `[Fixes]`: `SaveOverwriteQuestion`, `PhoenixInferno`, `LuminarBeamTarget`, `SummonKeepsLeader`, `SpellRecoveryTimes`, `GoldPileAmount`, `MessageBoxPause`, `CanisTenGold`, `AiBlessCount`, `AreaCasterGone`, `SupportAiTarget`, `DetectionCone`).
The same data is in [`bug-priorities.json`](bug-priorities.json).

## Method

**Scope.** All 355 bugs on the bugs page as of 2026-10-16, minus 26 that SummonerFix players never meet (see
[Not ranked](#not-ranked)). Those are the 14 that SummonerFix fixes or avoids, and 12 that need the Direct3D or
Glide renderer. SummonerFix always runs the game on OpenGL, so OpenGL-only bugs (C50, C90, K50, K51, K310, V50,
V51) stay in the list.

**Impact (1 to 5)** is what a normal single-player player gets when the bug triggers, discounted when normal play
rarely triggers it:

| Impact | Meaning |
|---|---|
| 5 | Crash or lost progress in normal play |
| 4 | Crash that needs an uncommon fight, a lost or overwritten save, a puzzle or fight that can go badly wrong |
| 3 | Wrong game rules that every player meets (cooldowns, dice, AI choices, gold), or a crash on rare hardware |
| 2 | Visible but harmless (UI rows, highlights, music, hit effects); rare-trigger rules slips; multiplayer-only crashes |
| 1 | Multiplayer-only and script-editor-only bugs, latent bugs that retail play cannot reach, invisible bugs |

**Ease (1 to 5)** is how fast we can make the bug show in our test setup:

| Ease | Meaning |
|---|---|
| 5 | Minutes in agent mode: a menu, a key, a console command or a test save |
| 4 | A warp with `play_and_load_cutscene`, a save edit with `tools/savedit.mjs`, a cheat, or one `poke` |
| 3 | A warp plus a set-up fight or a measurement (logging hook, `peek`), or a modded table file |
| 2 | Rare timing, a specific late-game state with no warp, or hardware we can simulate only with a hook |
| 1 | Networked multiplayer, a gone service (PXO), hardware we do not have, or code with no caller |

**Fix size** is our estimate from the bug paragraph: *tiny* is a few bytes (a flipped jump, a constant, an
address), *small* is a short hook or code cave, *medium* is a rewritten function or a new data path, and
*large/unknown* is anything bigger. Several paragraphs leave the intended behaviour open (G8, G15, G100, G229);
for those the fix also needs a decision on what the game should do.

**Priority** is impact x ease. Ties go to the smaller fix, then to the higher impact.

## Ranking

| Rank | Id | Bug | Impact | Ease | Reproduce | Fix size | Priority |
|---|---|---|---|---|---|---|---|
| 1 | G4 | Saving on the top row of a scrolled save list overwrites that save without the "Overwrite?" question | 4 | 5 | Make 10+ test saves, scroll the save list, save on the top row | small | 20 |
| 2 | G215 | Phoenix Inferno hits its target once per party member and knocks the whole party down wherever they stand | 4 | 4 | Phoenix fight (`rand-hills01` or Liangshan), boss stage command, watch Inferno | small | 16 |
| 3 | C250 | Luminar's beam hits a target that died during the 88-frame beam through a NULL pointer: crash | 5 | 3 | Warp to the Luminar fight; let a summon or hurt member die (or dismiss the summon) while the beam charges | small | 15 |
| 4 | G8 | Removing any party record (a summon dies) hands control to the first living member | 3 | 5 | Cast a summon, select another hero, let the summon die or end | small | 15 |
| 5 | G12 | Spell recovery, AI recovery and loop time store the record number instead of the table value (fireball 26 s, not 20) | 3 | 5 | Cast fireball, time the cooldown; compare `spells.tbl` | small | 15 |
| 6 | G15 | Damage dice roll 0 to die-1 while healing dice roll 1 to die (one point less per die); intent unclear | 3 | 5 | Cast lightning repeatedly, log damage with a hook | small | 15 |
| 7 | G40 | A loose gold pile left on the ground holds 1 gold after a load | 3 | 5 | Kill a gold-only monster, leave the pile, save, load, pick it up | small | 15 |
| 8 | G170 | A "Cannot save" warning shown while paused unpauses the game when closed | 3 | 5 | Pause in a fight, press the save key, close the warning | small | 15 |
| 9 | G301 | Giving Canis "10 gold" in occupied Lenele takes 100 | 3 | 4 | Warp to Lenele2ab, take the payoff topic, compare gold | tiny | 12 |
| 10 | C210 | AI healer counts some Bless candidates twice and writes past its 6-entry list into saved registers and the return address | 4 | 3 | Fight with Rosalind on AI, no Protect, four hurt unblessed members (`poke` HP); `immortal` off, watch for the crash | small | 12 |
| 11 | C270 | **Withdrawn 2026-10-17: cannot happen** (at most seven areas exist; see game-bugs, "Claims that did not hold"). Was: More than ten burning fire, blizzard or dark wall areas overflow a stack array in the effect update: crash from the 15th | 4 | 3 | Party casters with `max_skills`, cast fireball / wall of fire repeatedly in one fight; count areas with `peek` | small | 12 |
| 12 | G100 | The "leech" special ability (Blood Elemental, Crystal Rokhul, Joseph's Wraith) does nothing to HP in single player | 3 | 4 | Summon the Wraith (skills via `savedit.mjs`), fight, log HP | small | 12 |
| 13 | G50 | **Withdrawn 2026-10-17: did not happen in game** (the lab cannot be saved in, and the doors come back down on re-entry; see game-bugs, "Claims that did not hold"). Was: After a reload or second visit, the Khosani lab flipper board no longer matches the raised flippers | 4 | 3 | Warp to KhosaniLab, raise flippers, save and load, try the crossing | medium | 12 |
| 14 | G30 | Boss `$Offense point:` tag names parse as -1, so several boss attacks aim from near the world origin and rarely land | 3 | 4 | Warp to a boss (Pyrul, Machival, Giant Salamanka), boss `_stage` command, watch slash / breath hits | medium | 12 |
| 15 | K1 | A hot button used by number key lights up for one frame instead of 100 ms | 2 | 5 | Press 1 in a level; capture frames | tiny | 10 |
| 16 | K292 | Conversation "more" and "end" highlight anywhere below the entry | 2 | 5 | Open a long conversation, hover below "more" | tiny | 10 |
| 17 | K3 | Shop sell list category buttons jump to the wrong row | 2 | 5 | Open a shop in sell mode, click a category button | small | 10 |
| 18 | K4 | Inventory identify mode category buttons jump to the wrong row | 2 | 5 | Add unidentified items with `savedit.mjs`, identify mode, click a category | small | 10 |
| 19 | K5 | A scrolled save list shows the current scene as the top row's thumbnail | 2 | 5 | Make 10+ test saves, scroll the save list | small | 10 |
| 20 | K19 | The launcher's Speed tab setting is overwritten by the config defaults | 2 | 5 | Set `ComputerSpeed` in the registry, start, read detail options | small | 10 |
| 21 | K272 | Every character gets the same blob shadow; `shadows.tbl` sizes are loaded and never used | 2 | 5 | Any level; compare a boss shadow with a hero's | medium (deferred: needs a design) | 10 |
| 22 | G210 | Group detection beyond detection range accepts foes outside the forward cone, not inside: head-on approach noticed late | 3 | 3 | Approach a monster group head-on and from the side; log when it enters the enemy list | tiny | 9 |
| 23 | G211 | Support-style AI (Flece) always overrides its chosen target with a random enemy | 3 | 3 | Fight with Flece on AI; log her targets | tiny | 9 |
| 24 | G270 | One fire / blizzard / dark wall area whose caster is gone switches off every later area and cures their statuses | 3 | 3 | Wall of fire from a summon, dismiss it, walk into later areas | tiny | 9 |
| 25 | G229 | AI healers compare each share against the random number instead of a running sum, so they often cast nothing | 3 | 3 | Fight with Rosalind on AI; log support spell picks | small | 9 |
| 26 | K6 | A 33rd map label replaces the first one | 2 | 4 | Add 33 labels on one map (script the clicks) | tiny | 8 |
| 27 | K30 | Tall bosses use their size class height: health bar and text too low, upper body not clickable | 2 | 4 | Warp to the Luminar or Machival; look at the health bar, click high | tiny | 8 |
| 28 | K100 | A ring total of exactly 60,000 never shows the summon message, and the pop-up lists the summon early | 2 | 4 | Set a ring total to 59,999 / 60,000 by memory, gain power | tiny | 8 |
| 29 | C200 | LAN game list leaks a player record per visit; the fifth visit after four Create Game / Cancel rounds crashes | 2 | 4 | Options: LAN; Multiplayer, then Create Game and Cancel four times. Menus only, no second machine | small | 8 |
| 30 | G5 | Buying with a full inventory (521 item types) takes the gold and gives nothing | 2 | 4 | Fill the inventory with `savedit.mjs`, buy an item type not yet held | small | 8 |
| 31 | K12 | Container pickup list scrollbar thumb is full height and runs past the track when scrolled | 2 | 4 | Open a container with more than a page of items | small | 8 |
| 32 | K63 | Character click radius compares a squared distance with a plain radius | 2 | 4 | Click beside small and large characters; log picks | small | 8 |
| 33 | K110 | Each level leave frees only half the ambient emitters; after enough changes levels lose ambient sounds | 2 | 4 | Change levels repeatedly; count used `sound_emitters` with `peek` | small | 8 |
| 34 | K131 | Each load of a save in the last temple level replays the 29 s narration | 2 | 4 | Warp to the last temple level, save, load | small | 8 |
| 35 | K191 | Releasing a drag at the screen edge starts the edge camera turn at once | 2 | 4 | Drag with the left button to the edge, release | small | 8 |
| 36 | G250 | Tentacle Beast never grabs or throws (chance below zero, pick discarded); the dead code has more slips | 2 | 4 | Warp to the Tentacle Beast; confirm no grab in a long fight | medium | 8 |
| 37 | G6 | At game stages 4-8 the Zane grassland encounter case falls through and is overwritten | 2 | 3 | `gamestage` 4-8, roll world map encounters; check the chosen level by `peek` | tiny | 6 |
| 38 | G32 | Mounted-rider test reads the Serpent's stage for the Phoenix and never sees the Ghost as mounted | 2 | 3 | Phoenix fight on `rand-hills01`; set `serpent_stage` by `poke`, watch party styles | tiny | 6 |
| 39 | G132 | Liangshan's wave 4 tests only two of the three wave-3 enemies | 2 | 3 | Warp to Liangshan, kill #w3b and the mage first | tiny | 6 |
| 40 | G212 | AI healer casts Protect once two members qualify (doubled count of C210) | 2 | 3 | Fight with the healer on AI; log Bless / Protect choices | tiny | 6 |
| 41 | K130 | Liangshan keeps the Boss music after both riders die | 2 | 3 | Warp to Liangshan, kill the riders with cheats, log the track | tiny | 6 |
| 42 | K150 | Ikaemos upper hall Priest King fight never starts the Boss music | 2 | 3 | Warp to the upper hall, open the doors, log the track | tiny | 6 |
| 43 | C2 | Start-up crash when the 3D primary sound buffer cannot be created (NULL interface reused) | 3 | 2 | Needs a sound device without DS3D; fake the failure with a hook on `CreateSoundBuffer` | small | 6 |
| 44 | G2 | Only the low 32 bits of the QPC frequency are kept: game runs far too fast at 4.29 GHz and above | 3 | 2 | Needs such a timer; can be faked with a `QueryPerformanceFrequency` hook. Timeslip's patch fixes it | small | 6 |
| 45 | G13 | Aimed attack status chances use the selected character's skill, not the attacker's | 2 | 3 | Use aimed attack, switch selection before the hit; read the roll with a logging hook | small | 6 |
| 46 | G31 | A boss's `+Plus Spell:` (Serpent poison, Ghost fear / death) lands even when the hit misses on range | 2 | 3 | Serpent stage 1, step out of reach during the sickle swing | small | 6 |
| 47 | G70 | A cutscene starting while a summon waits to emerge rewrites the shared move; that creature skips its emerge until restart | 2 | 3 | Cast a summon right before a scripted cutscene trigger (`navgoto` onto it) | small | 6 |
| 48 | G252 | Murod Oni's backward-bite timer is armed once, so after 5 s it may switch every think | 2 | 3 | Murod Oni fight, stand behind it; log switches | small | 6 |
| 49 | G253 | Murod's leap and the Luminar's teleport can anchor on a dead party member | 2 | 3 | Murod or Luminar fight with a dead member | small | 6 |
| 50 | K7 | A character missing from the font draws a garbage glyph | 2 | 3 | Type a non-font character into a save name | small | 6 |
| 51 | K40 | With 128 saved map labels, deleting or editing a label is not saved | 2 | 3 | Make 128 labels (scripted), delete one, save and load | small | 6 |
| 52 | K80 | "Knocked Down!" is shown for a casting character who stays standing | 2 | 3 | Get knocked down while casting (boss knock-down move) | small | 6 |
| 53 | K190 | A dragon summon cutscene after a story cutscene streams a freed soundtrack handle, sometimes restarting music | 2 | 3 | Boss intro then dragon summon; log the stream | small | 6 |
| 54 | K290 | Skinning adds the bone translation to normals, so character lighting leans toward bones | 2 | 3 | Capture a lit animated character | small | 6 |
| 55 | K300 | Many spells play their hit sound at a leftover stack position | 2 | 3 | Cast Curse or Wrath of God; log the sound position | small | 6 |
| 56 | G41 | More than 32 loose gold piles: the extra piles are gone after a load | 2 | 3 | Leave 33 gold piles (spawn or kill many), save, load | medium | 6 |
| 57 | C40 | `door_open` / `door_close` / `door_toggle` with an unknown door use an uninitialised pointer | 1 | 5 | Console: `door_open nosuchdoor` | tiny | 5 |
| 58 | C313 | `save_map_labels` hangs when `automap_labels.tbl` has no `#End` | 1 | 5 | Console command with an edited table | tiny | 5 |
| 59 | K52 | `help change_texture_res` toggles the low-resolution textures | 1 | 5 | Console: `help change_texture_res` | tiny | 5 |
| 60 | K61 | Dropped item highlight is yellow: the blue channel reads a string instead of 0.51 | 1 | 5 | Drop an item, capture the highlight | tiny | 5 |
| 61 | K93 | First `antialiasing` command with AA on reports a toggle but changes nothing | 1 | 5 | Console: `antialiasing` | tiny | 5 |
| 62 | K194 | Help for `play_and_load_cutscene` prints `play_cutscene`'s usage | 1 | 5 | Console: `help play_and_load_cutscene` | tiny | 5 |
| 63 | K303 | `sysinfo` always reports MMX and four times the RAM speed | 1 | 5 | Console: `sysinfo` | tiny | 5 |
| 64 | V121 | `character` with no argument writes one byte before its buffer | 1 | 5 | Console: `character` | tiny | 5 |
| 65 | V142 | Console `debug` trace prints an int with `%f` | 1 | 5 | Console: `debug` | tiny | 5 |
| 66 | V200 | `load_mlayout` / `save_mlayout` without a name clear a byte of a saved register | 1 | 5 | Console: `load_mlayout` | tiny | 5 |
| 67 | V270 | Three render console toggles change flags nothing reads | 1 | 5 | Console: `render_level` | tiny | 5 |
| 68 | C6 | More than 50 command-line arguments overwrite `cmdline_params` | 1 | 5 | Launch Sum.exe with 51 arguments | small | 5 |
| 69 | C140 | Console word of 128+ characters corrupts the error jump, 133+ crashes; long `character` arguments overflow | 1 | 5 | Console: type a 140-character word | small | 5 |
| 70 | C203 | `load_mlayout` looks the map name up as a character type and uses type -1 | 1 | 5 | Console: `load_mlayout <file>` | small | 5 |
| 71 | C310 | Game folder path of 116+ characters crashes at start-up | 1 | 5 | Copy the game to a deep folder and start it | small | 5 |
| 72 | K15 | TGA Print Screen from a 32-bit back buffer garbles colours | 1 | 5 | Console `targa`, Print Screen | small | 5 |
| 73 | V17 | `-glide` and `-pack` command-line options are parsed and ignored | 1 | 5 | Start with `-glide` | small | 5 |
| 74 | V80 | `ai_state` console command rejects every name | 1 | 5 | Console: `ai_state <name>` | small | 5 |
| 75 | V81 | Three AI console variables are stored and never read | 1 | 5 | Console: set them, no effect | small | 5 |
| 76 | V122 | Two console toggles are stored and never read | 1 | 5 | Console: `Give_all_items` | small | 5 |
| 77 | C280 | After `play_and_load_cutscene` returns to the menu, Start New Game crashes (hits our own warp workflow) | 1 | 5 | Console `play_and_load_cutscene`, let it return, Start New Game | medium | 5 |
| 78 | G213 | Mounted Serpent tests the wrong hero for sleep after changing target | 2 | 2 | Serpent stage 1 with Nightmare sleep; needs the target change at the right moment | tiny | 4 |
| 79 | G224 | Deleting the last cutscene action in the script editor drops the last level script record | 1 | 4 | Script editor: delete a cutscene action, Save | tiny | 4 |
| 80 | K8 | Credits line count can be off when a long line wraps | 1 | 4 | Play the credits; compare line count | tiny | 4 |
| 81 | K9 | Create Game limits are centred with the height instead of the width | 1 | 4 | Multiplayer, Create Game screen | tiny | 4 |
| 82 | K221 | Script editor hover marker uses the selection's kind | 1 | 4 | Script editor: select a level object, hover a character | tiny | 4 |
| 83 | K222 | Script editor door window Enter clears the wrong box's focus | 1 | 4 | Script editor door window | tiny | 4 |
| 84 | K270 | Ghost Rider props are drawn in two cutscenes meant to hide them | 1 | 4 | `play_and_load_cutscene Ghost-Escape` | tiny | 4 |
| 85 | V10 | Parser line numbers count skipped lines twice | 1 | 4 | Break a table, read the error's line number | tiny | 4 |
| 86 | V40 | Game stage 19 sets `trapped_luminar` on the wrong level | 1 | 4 | `gamestage 19`, read the flag | tiny | 4 |
| 87 | G241 | Enemy casters decide meteor storm / inferno from unwritten stack distances | 2 | 2 | Fight an enemy caster with meteor storm; log the area-spell test | small | 4 |
| 88 | G242 | Layer-crossing search uses dx twice (no dz), so paths can pick the wrong floor layer | 2 | 2 | Needs a level with stacked layers and a move across two edges | small | 4 |
| 89 | G271 | Effect slots keep their old spell number, so AI skips fireball while no fire spell burns | 2 | 2 | Needs a status holder to reuse a fireball slot; log `fx` slots | small | 4 |
| 90 | K11 | Dialogue camera side offset uses an uninitialised y | 2 | 2 | Talk to NPCs; compare camera positions | small | 4 |
| 91 | K294 | A sound that ends during a fade leaves the channel flagged; the next sound there is cut off | 2 | 2 | Needs a fade that outlives its sound; log channels | small | 4 |
| 92 | C4 | No bound on the 300-entry lamp glow table | 1 | 4 | `poke` `lamp_glow_count` near 300, then enter a level with glowing effects | small | 4 |
| 93 | C5 | No bound on the 175-entry cutscene saved-actor table | 1 | 4 | Spawn many characters (memory) before a cutscene, or a modded level | small | 4 |
| 94 | C11 | No bound on the 1000-entry item object list | 1 | 4 | Drop or spawn items until the list is full (memory or modded level) | small | 4 |
| 95 | C12 | More than 1224 live items crash the next item allocation | 1 | 4 | Add items with `savedit.mjs` until the pool is full | small | 4 |
| 96 | C50 | OpenGL with window mode 202 (desktop size) crashes at start-up | 1 | 4 | Set window mode 202 in the registry, start with OpenGL. SummonerFix players use OpenGL | small | 4 |
| 97 | C70 | Character screen crashes for Rosalind in heavy body armour (no heavy stat-screen animation); she cannot equip it in retail | 1 | 4 | Equip heavy armour on Rosalind with `savedit.mjs`, open the Character screen | small | 4 |
| 98 | C130 | Missing rider in Liangshan / Jade Temple, or JekharJoin without Jekhar in reserve, dereferences NULL | 1 | 4 | Warp, then remove the rider or Jekhar by memory or save edit | small | 4 |
| 99 | C151 | Missing Tiger Rider, unjoinable Flece, or Joseph / Rosalind not in reserve dereference NULL in four level scripts | 1 | 4 | Warp, then remove the character from the reserve party with `savedit.mjs` | small | 4 |
| 100 | C160 | With `precalc_vis_method`, 50+ path points on one line overwrite the visibility sweep's return address | 1 | 4 | Console setting plus a level with a long straight path line | small | 4 |
| 101 | C220 | Three script editor actions crash (navpoint action with no cutscene navpoints, Die without a death move, Save after unload) | 1 | 4 | Script editor via the editor flag (`poke`) | small | 4 |
| 102 | C221 | Script editor Save loops forever when the level table lacks an exact `#Ambient Sounds` line or `level_items.tbl` lacks `#Level` / `#End` | 1 | 4 | Script editor with an edited table | small | 4 |
| 103 | C222 | Script editor Camera mode New has no bound on the 750 navpoints | 1 | 4 | Script editor, press New 751 times (script it) | small | 4 |
| 104 | C292 | Input box character set with a character above 0x7f overwrites the return address | 1 | 4 | Script editor input box charset; modded UI table | small | 4 |
| 105 | C302 | Missing Ghost or Phoenix Rider in two random-map variants crashes on the first frame | 1 | 4 | Warp to the variant and remove the rider by memory | small | 4 |
| 106 | G7 | A cutscene whose data fails to load leaves input locked and music off (retail files are all present) | 1 | 4 | Rename one `.csc`, trigger that cutscene | small | 4 |
| 107 | G220 | Script editor Save drops `$Name:` for characters on navpoints of another kind | 1 | 4 | Script editor on a script with such a character | small | 4 |
| 108 | G225 | Removing a level item in the script editor leaves later item indices stale | 1 | 4 | Script editor: remove an item, edit another | small | 4 |
| 109 | G226 | Script editor creature window sets every skill to 100 and, with level 0, base HP / AP to 0 | 1 | 4 | Script editor creature window, OK | small | 4 |
| 110 | G227 | Script editor Save drops every `+Prop:` line | 1 | 4 | Script editor on `sewer`, Save, diff the table | small | 4 |
| 111 | K2 | Multiplayer character screen: the info panel's up arrow scrolls down | 1 | 4 | Multiplayer menu, character screen (no network needed) | small | 4 |
| 112 | K171 | A picked lock can be picked again (new roll, sounds, no "already unlocked") | 1 | 4 | Pick a lock, pick it again | small | 4 |
| 113 | K193 | Screenshot numbering repeats at 1000 or goes random with other `screen*` names | 1 | 4 | Put `screenshot.jpg` in the folder, Print Screen | small | 4 |
| 114 | K220 | Script editor door window shows and writes back sound bytes as doubles, silencing door sounds | 1 | 4 | Script editor door window, OK, Save | small | 4 |
| 115 | K264 | Script editor list clicks are 2 pixels off | 1 | 4 | Script editor list boxes | small | 4 |
| 116 | K265 | Page Down onto a tall last list entry selects the list head | 1 | 4 | Script editor list with a tall last entry | small | 4 |
| 117 | K297 | Script editor check boxes clickable only on the first characters; input boxes use the wrong font | 1 | 4 | Script editor | small | 4 |
| 118 | V111 | Script editor level restart respawns other variants' items and taken items | 1 | 4 | Script editor restart | small | 4 |
| 119 | V141 | `.vcs` path cut at the first dot; one-line `/* */` comments too much | 1 | 4 | Install path with a dot, run a `.vcs` | small | 4 |
| 120 | V190 | Each Print Screen leaks a list node per existing screenshot | 1 | 4 | Print Screen repeatedly, watch heap | small | 4 |
| 121 | V222 | Script editor ambient OK takes a new sound slot each time | 1 | 4 | Script editor ambient window, OK repeatedly | small | 4 |
| 122 | V264 | Script editor re-wraps fonted list entries every frame | 1 | 4 | Script editor; profile | small | 4 |
| 123 | G221 | Script editor Save rewrites the shared level table with only the current script's ambients (variants delete them all) | 1 | 4 | Script editor on `sewer_v2`, change an ambient, Save | medium | 4 |
| 124 | G222 | Script editor Save can write other scripts' level items twice on wrong navpoints | 1 | 4 | Play a level with many items, then edit `sewer` and Save | medium | 4 |
| 125 | G223 | Deleting a navpoint in the script editor leaves items, cutscene actions and dead characters on old slots | 1 | 4 | Script editor: delete a navpoint used by a level item, Save | medium | 4 |
| 126 | C9 | Surface table parser passes size 0x100 for a 0x38-byte stack buffer | 1 | 3 | Modded `surfaces.tbl` with a long name; fix is the size constant | tiny | 3 |
| 127 | C73 | A translated chain skill name above about 60 characters overflows "%s learned %s!" | 1 | 3 | Modded `strings.tbl` | tiny | 3 |
| 128 | C251 | Effects array line longer than 1023 characters overflows the loader stack | 1 | 3 | Modded `.arr` | tiny | 3 |
| 129 | C252 | Arrow button with an inner size of exactly 5 pixels divides by zero | 1 | 3 | Modded UI layout | tiny | 3 |
| 130 | C262 | Zero or over-long cutscene rotation key divides by zero or zeroes the tangents | 1 | 3 | Modded camera file | tiny | 3 |
| 131 | G111 | A door `+Key:` that names no item can never be opened with a key | 1 | 3 | Modded level table | tiny | 3 |
| 132 | K17 | A summon's second weapon slot pages the first weapon's model | 1 | 3 | Summon with two weapons (modded) | tiny | 3 |
| 133 | K92 | A fog update with bad distances keeps the old fog | 1 | 3 | Modded fog values | tiny | 3 |
| 134 | K210 | Snow cells scrolling in on the +x side keep old flakes for up to 12 s | 1 | 3 | Warp to lenele3a, walk toward +x, capture | tiny | 3 |
| 135 | K211 | Mounted Phoenix Rider's animation is not synced to its horse | 1 | 3 | Phoenix stage 1; capture frames | tiny | 3 |
| 136 | K223 | A Pyrul model without `$missile01` fires from bone -1 | 1 | 3 | Modded model | tiny | 3 |
| 137 | K252 | Multiplayer character file names of 32+ characters show junk | 1 | 3 | Rename a `.plr` file's character name | tiny | 3 |
| 138 | K273 | Level `+Fog Near` / `+Fog Far` stored swapped as the default fog | 1 | 3 | Cutscene `$Fog` with near 0 (modded) | tiny | 3 |
| 139 | K295 | Force-driven particles integrate at most 1/30 s per update | 1 | 3 | Compare particle motion at low and high frame rates | tiny | 3 |
| 140 | K298 | Streak particle length ignores one size factor | 1 | 3 | Modded effect | tiny | 3 |
| 141 | K299 | Lit effect mesh normals all point into the negative octant (signed / unsigned slip) | 1 | 3 | Modded lit effect mesh | tiny | 3 |
| 142 | V3 | UI table hotkey "Alt+" sets Shift instead | 1 | 3 | Modded UI table (loader has no caller) | tiny | 3 |
| 143 | V4 | TGA reader skips the image ID field from the wrong offset | 1 | 3 | A TGA with an ID field (modded) | tiny | 3 |
| 144 | V12 | Hex number at the very end of a table reads past the text | 1 | 3 | Modded table ending in a hex number | tiny | 3 |
| 145 | V22 | Level colour-record reader clamps the wrong channels | 1 | 3 | Read records after load with `peek` | tiny | 3 |
| 146 | V71 | Skipped rider moves keep skeleton index 1 instead of -1 | 1 | 3 | Warp to masad or Liangshan, `peek` | tiny | 3 |
| 147 | V90 | vsync option is passed as `gr_init`'s far-z argument | 1 | 3 | Toggle vsync, `peek` the argument | tiny | 3 |
| 148 | V131 | The "Charge" action clears a flag nothing reads | 1 | 3 | Wolong2 dialogue; read the flag | tiny | 3 |
| 149 | V151 | Joseph's "not ready to confront Machival" line can never be said | 1 | 3 | Warp to lenele3a, walk the spline | tiny | 3 |
| 150 | V180 | Activation thread priority change uses an unset handle; instance search result unused | 1 | 3 | Log `SetThreadPriority` | tiny | 3 |
| 151 | V184 | Clipboard reader leaves the clipboard open and needs one byte more | 1 | 3 | Paste with no text on the clipboard | tiny | 3 |
| 152 | V191 | Third script editor message box button calls the second's callback | 1 | 3 | No three-button box is shown | tiny | 3 |
| 153 | V211 | Rider flinch errors are formatted and never shown; flinch tables unused | 1 | 3 | Load the Phoenix; log | tiny | 3 |
| 154 | V221 | Navpoint delete copies the record past the last one | 1 | 3 | Script editor, delete a navpoint | tiny | 3 |
| 155 | C8 | No bound on shopkeepers (15) or shop entries (1000) in a level table | 1 | 3 | Modded level table | small | 3 |
| 156 | C10 | Removing two initial topics, the first the current hail topic, makes an inner loop step the wrong list and hang | 1 | 3 | Modded `_dlg.tbl` with the topics in a different order | small | 3 |
| 157 | C14 | A character name of 32+ characters overwrites the character's mesh pointers | 1 | 3 | Modded character table | small | 3 |
| 158 | C30 | Character preload index above 70 entries or 1 MB overflows | 1 | 3 | Modded preload index | small | 3 |
| 159 | C52 | Khosani lab room 4 script writes into the event-name pool when a prisoner is missing | 1 | 3 | Modded level script, or kill / remove the prisoner by memory | small | 3 |
| 160 | C53 | World map without hills, forest or grass navpoints crashes the stage 4-8 encounter roll | 1 | 3 | Modded world map script | small | 3 |
| 161 | C71 | A town character type with a hide or cloak move but no stand move crashes while loading animations | 1 | 3 | Modded character table | small | 3 |
| 162 | C72 | More than three `$Trigger:` lines in a move overwrite its sound reference and later its list link | 1 | 3 | Modded character table | small | 3 |
| 163 | C100 | A party member set to a level with more skill points than its progression takes hangs the game | 1 | 3 | Modded `experience.tbl` | small | 3 |
| 164 | C110 | Entry point without `$player` navpoints crashes when placing the party; the intended error never fires | 1 | 3 | Modded level script | small | 3 |
| 165 | C111 | Clickable object name plus script name of 68+ characters overflows the stack on save / load | 1 | 3 | Modded level | small | 3 |
| 166 | C112 | `$Door:` naming no level object crashes the load or retargets the last object | 1 | 3 | Modded level table | small | 3 |
| 167 | C113 | No bounds on level effects (10), triggers (32), level items (32) or clickables (50) | 1 | 3 | Modded level table | small | 3 |
| 168 | C114 | Level item without `$Navpoint:` crashes the load; unknown `$Item:` uses a stale type | 1 | 3 | Modded `level_items.tbl` | small | 3 |
| 169 | C141 | Game intro crashes when a Joseph move or .mvf is missing | 1 | 3 | Modded intro script or a removed .mvf | small | 3 |
| 170 | C142 | A cutscene taking off more than 16 equipment pieces overflows the record table | 1 | 3 | Modded cutscene script | small | 3 |
| 171 | C143 | No bounds on script actions (54), level script objects (200) or cutscene objects (32) | 1 | 3 | Modded script table | small | 3 |
| 172 | C161 | Navigation BSP node with a type other than 1-3 hangs the next path test | 1 | 3 | Modded `.pfg` | small | 3 |
| 173 | C170 | Message box line of 64+ characters overflows a stack buffer | 1 | 3 | Modded strings, or a long name in a message | small | 3 |
| 174 | C180 | Modded name tables with large ids or long level sections overflow the stack or the 10 KB pool | 1 | 3 | Modded name tables | small | 3 |
| 175 | C190 | `cutscene.tbl` entry without a usable first `$Fog` key crashes on the first frame | 1 | 3 | Modded `cutscene.tbl` | small | 3 |
| 176 | C191 | No bounds on `cutscene.tbl` entries and per-entry camera, animation, minitrack, fog and fade lines | 1 | 3 | Modded `cutscene.tbl` | small | 3 |
| 177 | C202 | Entering the game list or lobby with an unreadable `.plr` writes through NULL | 1 | 3 | Lock or delete the `.plr` file before entering the game list | small | 3 |
| 178 | C211 | More than 250 position or rotation keys in a cutscene's camera files overflow the key pools | 1 | 3 | Modded camera files | small | 3 |
| 179 | C231 | Door with 5+ navigation edges loses one; 36+ overwrite a return address | 1 | 3 | Modded `.pfg` / door | small | 3 |
| 180 | C254 | A fifth Tentacle Beast character overwrites the tentacle fight's variables | 1 | 3 | Modded level with five tentacles | small | 3 |
| 181 | C260 | With all 400 lights in use a new light overruns the selection table; closing the inventory can crash | 1 | 3 | Create lights until full (memory) | small | 3 |
| 182 | C261 | List box entry wrapping into more than 20 lines overflows the line tables and crashes on draw | 1 | 3 | Script editor list with a very long entry | small | 3 |
| 183 | C271 | A sixth world effect with "sky" in its name overruns the five-entry sky list | 1 | 3 | Modded level or cutscene | small | 3 |
| 184 | C272 | Masad tutorial part 2 crashes if an arrow, spell area or status holder exists at that moment | 1 | 3 | Start a new game; have a status effect (cast Bless) as the Orenian Scout falls. Not reached in normal tutorial play | small | 3 |
| 185 | C274 | Level item placed with all 40 containers in use crashes the load | 1 | 3 | Modded level | small | 3 |
| 186 | C291 | `alt_textures.tbl` base name of 33-40 characters or a set of 16+ textures overflows | 1 | 3 | Modded `alt_textures.tbl` | small | 3 |
| 187 | C301 | More than 512 cutscene or 750 level navpoints overwrite the string pool or console command | 1 | 3 | Modded script | small | 3 |
| 188 | G10 | Adding gold to a container that already holds gold loses it and leaves a stale slot | 1 | 3 | No retail path known; needs a hook or modded level | small | 3 |
| 189 | G11 | Using aimed attack, forge, mine or enchant as a skill takes the AP and does nothing | 1 | 3 | Not offered as a skill in retail; call the skill path by hook | small | 3 |
| 190 | G16 | With all 20 missile slots taken, the next spell missile is dropped | 1 | 3 | Many archers and casters at once, or fill slots by memory | small | 3 |
| 191 | G42 | Door name plus script name of 64+ characters makes every save of that level fail to load | 1 | 3 | Modded level | small | 3 |
| 192 | G51 | Once the 1000 path nodes run out, failed searches keep their nodes until the next level | 1 | 3 | Many path failures in one level; watch the node count | small | 3 |
| 193 | G71 | A misspelled `$Skill:` overwrites the up-direction skill or gives id 0x54 | 1 | 3 | Modded character table | small | 3 |
| 194 | G72 | More than two `$Default Weapon:` lines give wrong materials and then junk weapons | 1 | 3 | Modded character table | small | 3 |
| 195 | G110 | `#Clicks` name that is no level object makes object 0 clickable with that entry | 1 | 3 | Modded level table | small | 3 |
| 196 | G130 | A save made in the last temple after Machival's defeat would lose the forge ending; the game refuses saving there | 1 | 3 | Only with a save forced by memory | small | 3 |
| 197 | G131 | A save in the 8 s after the Laharah message on the second Jade Temple visit would load into a level with no exit; saving is refused | 1 | 3 | Only with a save forced by memory | small | 3 |
| 198 | G140 | After a cutscene, a removed item can return to the wrong slot when its type slot is unusual | 1 | 3 | Modded item or cutscene | small | 3 |
| 199 | G230 | With five goal slots full, a low-priority order overwrites another order's target | 1 | 3 | Queue many orders on one character; log goals | small | 3 |
| 200 | G251 | A tentacle swipe hitting five party members makes that tentacle unkillable | 1 | 3 | Tentacle Beast with five members in reach (summon included) | small | 3 |
| 201 | K50 | OpenGL: 16-bit water mipmaps are built from the wrong rows (SummonerFix players use OpenGL) | 1 | 3 | Capture distant water; compare with a fixed filter | small | 3 |
| 202 | K140 | Cutscene "replace armor" with "(none)" decides from memory before the item table | 1 | 3 | Modded cutscene script | small | 3 |
| 203 | K180 | A `strings.tbl` section above 40 KB shows the next string's text | 1 | 3 | Modded `strings.tbl` | small | 3 |
| 204 | K192 | `+Minitrack:` sounds in `cutscene.tbl` never play | 1 | 3 | Modded `cutscene.tbl`; no retail entry | small | 3 |
| 205 | K212 | Tiger stage-2 rider walks in place of its stand / ready animations | 1 | 3 | Tiger fight stage 2 (boss stage command) | small | 3 |
| 206 | K293 | After 64 lines in a conversation, later lines play no talk animation | 1 | 3 | Long conversation (32+ texts) | small | 3 |
| 207 | K296 | In character-bound effects, light tags from the third bind to the wrong light; bone dummies ignore rotation | 1 | 3 | Modded effect | small | 3 |
| 208 | K301 | `+Set Important:` naming a speaker defined later in the same file does nothing | 1 | 3 | Modded `_dlg.tbl` | small | 3 |
| 209 | K302 | Particle interval curves with two or more non-zero keys spawn at the wrong rate | 1 | 3 | Modded effect | small | 3 |
| 210 | K310 | OpenGL: coloured spell light on lightmapped surfaces shows only its red value as grey (not seen in five scenes) | 1 | 3 | `[Debug] TraceGlLights` and `k310` counters during a fire or ice spell | small | 3 |
| 211 | V2 | Only four of the six large streamed-effect slots are ever handed out | 1 | 3 | Log slot use with a hook | small | 3 |
| 212 | V5 | A speed change on one animation is copied into all five speed slots | 1 | 3 | Log speed slots with `peek` | small | 3 |
| 213 | V8 | Freed font slots are never reused; kerning search can match another character's pair | 1 | 3 | Log font slots; kerning needs a specific font | small | 3 |
| 214 | V13 | List parsers ignore their maximum count | 1 | 3 | Modded table with a long list | small | 3 |
| 215 | V15 | Item placed on a pick layer is placed again by the floor probe | 1 | 3 | Log item positions on load | small | 3 |
| 216 | V20 | A mesh file older than version 6 leaks its record | 1 | 3 | Modded mesh | small | 3 |
| 217 | V21 | A mesh with more than 6 textures overwrites its count and flags | 1 | 3 | Modded mesh | small | 3 |
| 218 | V50 | OpenGL texture memory count is wrong and updated textures upload twice | 1 | 3 | Log uploads with a GL hook | small | 3 |
| 219 | V70 | Five hero moves inherit weapon-class bits and load extra anims | 1 | 3 | Log anim loads | small | 3 |
| 220 | V130 | The catacombs "cryptintro" cutscene can never play | 1 | 3 | Unreachable | small | 3 |
| 221 | V152 | Lenele slums actions set Gorbus's topic backwards; Hail resets it | 1 | 3 | Warp, check the topic by `peek` | small | 3 |
| 222 | V170 | Opening a full screen from an empty side panel runs its leave function twice | 1 | 3 | Open a screen from an empty panel; log | small | 3 |
| 223 | V193 | Cutscene unload passes a handle as a play signature and double-frees a slot | 1 | 3 | Play any cutscene; log `snd_stop` | small | 3 |
| 224 | V213 | Tiger Rider axe is leaked across loads | 1 | 3 | Load the Tiger fight twice; watch heap | small | 3 |
| 225 | V254 | An extension of 8+ characters overruns a buffer | 1 | 3 | Not reached in retail | small | 3 |
| 226 | V261 | Sound with minimum range 0 takes a new slot each lookup; long names overflow | 1 | 3 | Modded sound table | small | 3 |
| 227 | G52 | A path to a goal outside the walkable area usually fails instead of stopping at the edge | 1 | 3 | Order a move outside the area (needs a click there) | medium | 3 |
| 228 | G216 | Tiger Rider's target weights assign the base instead of summing it; mild | 1 | 2 | Tiger fight with several heroes; log weights | tiny | 2 |
| 229 | G240 | Wraith Life Drain target scores add to stale values from earlier tests | 1 | 2 | Fight a Wraith; log scores | tiny | 2 |
| 230 | K16 | Animation reset clears only 25 of 100 weights | 1 | 2 | Needs more than 25 weights in use | tiny | 2 |
| 231 | K18 | Paging out popup model slot 18 can drop another slot's model | 1 | 2 | Needs the rare slot use | tiny | 2 |
| 232 | K161 | "Insert CD #2" box sits in the lower half | 1 | 2 | Remove the CD #2 data from the data path | tiny | 2 |
| 233 | V30 | Level free clears two pointers but keeps their counts | 1 | 2 | Not reached in retail | tiny | 2 |
| 234 | V101 | Jade Beam tests for fireball and prints a debug value with the wrong format | 1 | 2 | Cast Jade Beam on a boss | tiny | 2 |
| 235 | V110 | Avoid spline allocation tests the same handle twice and NULL-tests a handle | 1 | 2 | Not reached in retail | tiny | 2 |
| 236 | V210 | Group enemy count is -1 until the next think after a member leaves | 1 | 2 | Kill a group member, `peek` the count | tiny | 2 |
| 237 | V220 | Full attack points make an approacher write into the fifth slot timer | 1 | 2 | Many attackers on one target; `peek` | tiny | 2 |
| 238 | V230 | Full goal slots set a bit in the travel point's z | 1 | 2 | Not reached in retail | tiny | 2 |
| 239 | V250 | Titus's target becomes -1 for one think in stage 3 | 1 | 2 | Titus stage 3 with a paralysed target | tiny | 2 |
| 240 | V251 | Giant Salamanka flinch errors never shown; odd names index the wrong stage | 1 | 2 | Load the Salamanka; log | tiny | 2 |
| 241 | C90 | OpenGL multi-pass polygon of more than 32 clipped vertices overwrites the stack | 1 | 2 | Needs a polygon that clips to 33+ vertices; SummonerFix players use OpenGL | small | 2 |
| 242 | C192 | Script editor message box with a button set above 4 frees garbage pointers | 1 | 2 | No editor path asks for it; needs a hook | small | 2 |
| 243 | C230 | A queued cast-at-target order whose target is gone crashes while a level cutscene script runs | 1 | 2 | Needs a queued order and a target removed during a cutscene; timing | small | 2 |
| 244 | C232 | The game camera crashes without a target | 1 | 2 | No known retail path; needs a hook | small | 2 |
| 245 | G60 | Chat server connection failing at once is never reported; the lobby retries every frame with a new socket | 1 | 2 | Point the chat server at a refusing address | small | 2 |
| 246 | G214 | On-foot Phoenix walks toward a point mirrored through the level origin when the side point has no path | 1 | 2 | Phoenix stage 3, two members down, blocked side point | small | 2 |
| 247 | G243 | A merged avoidance polygon that fails clipping leaves the other polygons' corners off | 1 | 2 | Needs overlapping avoidance areas near walls | small | 2 |
| 248 | G290 | A ray against an object with more than 128 pending tree nodes misses faces | 1 | 2 | Needs a very complex level object | small | 2 |
| 249 | K51 | OpenGL: a tint on a 2D bitmap, line or polygon wraps bright colours | 1 | 2 | No retail tint above 1; SummonerFix players use OpenGL | small | 2 |
| 250 | K250 | A blob shadow over eight level objects is drawn on none | 1 | 2 | Needs a spot with eight objects under a character | small | 2 |
| 251 | K251 | Evil Urath's leap uses the last edge instead of the nearest | 1 | 2 | Evil Urath fight near several edges | small | 2 |
| 252 | K262 | A steeply turned ground node gives a spell ground effect a NaN orientation | 1 | 2 | Needs such a node | small | 2 |
| 253 | K263 | Projected-texture spot lights light outside range and leave the texture locked | 1 | 2 | No retail projected spot light | small | 2 |
| 254 | K271 | An effect lamp glow can shine through one occluding object | 1 | 2 | Torch effect behind a level object; capture | small | 2 |
| 255 | V31 | Waving objects accumulate normals into arrays nothing reads | 1 | 2 | Not observable | small | 2 |
| 256 | V51 | OpenGL view setup looks along the wrong vector with near plane 0 | 1 | 2 | Read the matrix with a hook | small | 2 |
| 257 | V100 | Spell handlers store amounts that the dice roll overwrites | 1 | 2 | Log `amount` with a hook | small | 2 |
| 258 | V102 | Walk-to-point normalises a direction it never uses | 1 | 2 | Not observable | small | 2 |
| 259 | V120 | A fog debug text can never be drawn | 1 | 2 | Unreachable | small | 2 |
| 260 | V140 | Script trigger name copied unbounded; "+No create" copies an unfilled buffer | 1 | 2 | Modded script | small | 2 |
| 261 | V143 | A duplicate or 301st console command is dropped silently | 1 | 2 | Not reached in retail | small | 2 |
| 262 | V171 | Whole-file packfile read uses the bare name and reports CD files as read | 1 | 2 | Not observable with a hard-disk install | small | 2 |
| 263 | V183 | Level section name lowering stops early; duplicates leak | 1 | 2 | Not observable | small | 2 |
| 264 | V252 | Failed `File::find` searches with an uninitialised extension | 1 | 2 | Log a failed find | small | 2 |
| 265 | V255 | A data directory added twice never gets its new extensions | 1 | 2 | Not reached in retail | small | 2 |
| 266 | V262 | With EAX, each 3D buffer played in 2D keeps a reference | 1 | 2 | EAX device; log buffers | small | 2 |
| 267 | C13 | An object still active when returning to the title can hang or corrupt the heap | 1 | 2 | No known retail trigger; needs a hook to leave an object active | medium | 2 |
| 268 | C193 | Debug Load screen (never entered) dereferences NULL with no selection | 1 | 1 | Unreachable in v1.40 | tiny | 1 |
| 269 | C253 | `file_make_path` with the "any path" id reads far past the table | 1 | 1 | No caller passes it in retail | tiny | 1 |
| 270 | G160 | `multi_path_type 2` is accepted and that machine drops all received paths | 1 | 1 | Multiplayer plus console | tiny | 1 |
| 271 | G182 | Multiplayer: a failed first IAMHERE clears the link but the joiner carries on | 1 | 1 | Needs a multiplayer session | tiny | 1 |
| 272 | G228 | Multiplayer: Pyrul's wall of fire falls through into a meteor storm | 1 | 1 | Needs a multiplayer session | tiny | 1 |
| 273 | G310 | Multiplayer: the summon's position update is never sent; the leader's is sent twice | 1 | 1 | Needs a multiplayer session | tiny | 1 |
| 274 | K181 | French text would show the wrong accented letter; v1.40 never selects French | 1 | 1 | Unreachable in v1.40 | tiny | 1 |
| 275 | K230 | Multiplayer `load_mlayout` monsters get a broken orientation on other machines | 1 | 1 | Needs a multiplayer session | tiny | 1 |
| 276 | K231 | Camera height smoothing overshoot moves away from the floor; frame time cap prevents it | 1 | 1 | Unreachable (frame time capped) | tiny | 1 |
| 277 | K261 | A saved `.eax` file would set listener flags from its air absorption; never read | 1 | 1 | Unreachable from `tables.vpp` | tiny | 1 |
| 278 | K274 | Floating text in a cutscene letterbox would sit low; not drawn in cutscenes | 1 | 1 | Unreachable in v1.40 | tiny | 1 |
| 279 | V1 | Skeleton trigger-time setter updates every slot except the named one | 1 | 1 | No caller | tiny | 1 |
| 280 | V11 | Comma formatter prints "-,123,456" for some negatives | 1 | 1 | No caller passes such a value | tiny | 1 |
| 281 | V16 | Two character-instance helpers drop their result | 1 | 1 | No caller | tiny | 1 |
| 282 | V41 | Automap bit functions accept indices 8 times too large | 1 | 1 | No caller passes one | tiny | 1 |
| 283 | V112 | Level scaling reads `cur_character` before its NULL test | 1 | 1 | Not reached in retail | tiny | 1 |
| 284 | V182 | Multiplayer: IAMHERE and ACK send uninitialised bytes | 1 | 1 | Multiplayer | tiny | 1 |
| 285 | V192 | Debug Backspace flush tests a key code that never arrives | 1 | 1 | Unreachable | tiny | 1 |
| 286 | V212 | Phoenix on-foot setup looks up `ready_hurt` and discards it | 1 | 1 | Not observable | tiny | 1 |
| 287 | V214 | Serpent first load clears its seated flag right after setting it | 1 | 1 | Not observable | tiny | 1 |
| 288 | V223 | Segment test reports the wrong meeting point for touching segments | 1 | 1 | Not observable | tiny | 1 |
| 289 | V253 | Memory-mode file reader seeks wrongly | 1 | 1 | No caller uses those seeks | tiny | 1 |
| 290 | V260 | Segment sound emitters would get an out-of-range pan | 1 | 1 | Nothing creates one | tiny | 1 |
| 291 | V272 | Sixteen blob shadow bitmaps are built and never used | 1 | 1 | Not observable | tiny | 1 |
| 292 | V290 | Two uncalled functions: box test extent slip and a particle cull | 1 | 1 | No caller | tiny | 1 |
| 293 | V300 | Colour tint would apply the green factor to red | 1 | 1 | Every tint the game sets is grey | tiny | 1 |
| 294 | V302 | Key queue flush runs before the keyboard is ready; `key_putkey` has no full test | 1 | 1 | Not observable | tiny | 1 |
| 295 | C7 | A window with "+Always a selection" loops forever on the first gadget; no reachable path sets the flag | 1 | 1 | Unreachable in v1.40 (only the uncalled UI table loader sets it) | small | 1 |
| 296 | C51 | OpenGL crashes at start-up when the driver lacks `GL_ARB_multitexture` | 1 | 1 | Needs an old driver; every current driver has the extension | small | 1 |
| 297 | C144 | Multiplayer chat or system message wrapping into more than six lines crashes | 1 | 1 | Needs a multiplayer session | small | 1 |
| 298 | C171 | Multiplayer: Tab with every party character gone hangs | 1 | 1 | Needs a multiplayer session | small | 1 |
| 299 | C181 | Multiplayer: a reliable packet claiming more than 600 bytes overruns the heap buffer | 1 | 1 | Needs a forged packet from a second machine | small | 1 |
| 300 | C201 | Multiplayer: one forged packet of type 0x42 or 0x40 crashes the receiver | 1 | 1 | Needs a forged packet from a second machine | small | 1 |
| 301 | C273 | Network game: spell hit effect with no spell record crashes every machine | 1 | 1 | Needs a multiplayer session | small | 1 |
| 302 | C311 | Multiplayer client crashes when it cannot create a character the host sent | 1 | 1 | Needs a multiplayer session | small | 1 |
| 303 | C312 | Multiplayer respawn packet for an unknown player id crashes | 1 | 1 | Needs a multiplayer session | small | 1 |
| 304 | G14 | Multiplayer: closing a container drops the wrong entries on the other machines | 1 | 1 | Needs a multiplayer session | small | 1 |
| 305 | G17 | Trade completes with an item that is gone; the `add_gold` command gets corrupted | 1 | 1 | Multiplayer trade | small | 1 |
| 306 | G18 | Multiplayer: a lock picked through the host uses an unmapped handle on clients | 1 | 1 | Needs a multiplayer session | small | 1 |
| 307 | G180 | Multiplayer: a packet from an unlinked address is handled as the previous machine's | 1 | 1 | Needs a multiplayer session | small | 1 |
| 308 | G181 | Multiplayer: at receive sequence 0xffb6 a resent packet is stored again and delivered 65,536 packets later | 1 | 1 | Needs a very long multiplayer session | small | 1 |
| 309 | K13 | Multiplayer level select player rows drift from the names | 1 | 1 | Needs players joining | small | 1 |
| 310 | K14 | PXO chat banner never clickable (service gone) | 1 | 1 | Not reproducible: PXO is offline | small | 1 |
| 311 | K62 | Multiplayer chat kick message shows the nick as the reason | 1 | 1 | Needs a chat server | small | 1 |
| 312 | K160 | Multiplayer: overlapping joins can announce a player twice | 1 | 1 | Needs three machines | small | 1 |
| 313 | K200 | PXO lobby: a cut-short channel record leaves the list stale for 10 s | 1 | 1 | Needs a chat server | small | 1 |
| 314 | K213 | Serpent illusion would leave the second copy active; nothing starts it | 1 | 1 | Unreachable in v1.40 | small | 1 |
| 315 | V6 | Small streamed-effect ring restarts only at slots 0, 1, 4 or 5 | 1 | 1 | Not reached in retail | small | 1 |
| 316 | V7 | Registry helpers read past a key name with no backslash; delete prints NULL | 1 | 1 | No retail key name lacks the backslash | small | 1 |
| 317 | V14 | 16-to-16-bit TGA converter writes nothing | 1 | 1 | No caller reaches it | small | 1 |
| 318 | V18 | Angle extraction gets pitch wrong off the 90-degree headings | 1 | 1 | Not reached in retail | small | 1 |
| 319 | V19 | A matrix "transpose" is not one; a skeleton slerp starts from garbage | 1 | 1 | No caller | small | 1 |
| 320 | V82 | Already-down branch of the knock-down can never run | 1 | 1 | Unreachable | small | 1 |
| 321 | V181 | Multiplayer: repeated connection request opens a second link | 1 | 1 | Multiplayer | small | 1 |
| 322 | V201 | Multiplayer host keeps a link from an unlisted machine until it times out | 1 | 1 | Multiplayer | small | 1 |
| 323 | V224 | Two avoidance helpers read the list header as a segment | 1 | 1 | Not observable | small | 1 |
| 324 | V256 | Four File helpers with no caller have slips | 1 | 1 | No caller | small | 1 |
| 325 | V257 | UI table loader tooltip leak and auto-repeat flag | 1 | 1 | No caller (V9) | small | 1 |
| 326 | V263 | Three light / effect helpers with no caller have slips | 1 | 1 | No caller | small | 1 |
| 327 | V265 | Six list box and UI manager slips reached only through the UI table loader | 1 | 1 | No caller (V9) | small | 1 |
| 328 | V271 | Three render flags are read but never set | 1 | 1 | Unreachable | small | 1 |
| 329 | V9 | UI table loader is never called and has its own slips | 1 | 1 | No caller | medium | 1 |

## Top 15 to do first

1. **G4**: the only way the game destroys a save without asking, and ten test saves show it in minutes.
2. **G215**: the Phoenix Rider's Inferno can wipe the party in two fights of the main story; one wrong distance and one wrong target.
3. **C250**: a story boss fight crashes when a party member or summon dies during the beam, which is a normal event in that fight.
4. **G8**: control jumps away from the hero the player chose every time a summon ends, so players meet it all the time.
5. **G12**: every spell cooldown that the tables set is wrong (fireball 26 s instead of 20); one store per field to fix.
6. **G15**: every damage spell loses a point per die; decide first whether the tables were tuned to it.
7. **G40**: gold left on the ground shrinks to 1 after a load; a save edit and a load show it.
8. **G170**: a fight the player paused starts again behind a warning box, so the player can die without input.
9. **G301**: a dialogue choice takes ten times the gold it names; a one-byte constant.
10. **C210**: a live crash in hard fights with Rosalind on AI; the same doubled count causes G212, so one fix covers both.
11. **C270**: withdrawn 2026-10-17. The avoidance pool allows at most seven areas, so the ten-entry list cannot overflow.
12. **G100**: Joseph's Wraith summon and two monsters have a special ability that does nothing; settle what "leech" should do.
13. **G50**: withdrawn 2026-10-17. Saving is refused in the lab, and on a second visit the flipper doors were down like the board.
14. **G30**: most boss offense-point attacks rarely land, so several boss fights are easier than designed; fixing it makes them harder, so test balance with it.
15. **K1**: the cheapest visible fix (one flipped jump) and a good first patch to check the before / after capture workflow.

C280 (Start New Game crashes after `play_and_load_cutscene` returns to the menu) has impact 1 for players, but it
slows our own warp workflow. It is worth fixing in SummonerFix's debug code before the long list of warp tests.

## Not ranked

These 26 bugs are fixed or avoided by SummonerFix, or need a renderer that SummonerFix never selects.

| Id | Why it is not ranked |
|---|---|
| C1 | Fixed (`bugfix.c`) |
| C3 | Fixed |
| C91 | Direct3D only |
| C150 | Glide only |
| C290 | Avoided (`gameopts.c`) |
| C300 | Direct3D only |
| G1 | Fixed |
| G3 | Fixed |
| G9 | Fixed |
| G300 | Fixed (`bugfix.c`) |
| K20 | Fixed (`bugfix.c`, 2026-10-16) |
| K10 | Fixed |
| K60 | Direct3D only |
| K64 | Fixed (`bugfix.c`) |
| K90 | Fixed (`gamma.c`) |
| K91 | Direct3D only |
| K151 | Glide only |
| K170 | Fixed (`bugfix.c`, 2026-10-16) |
| K260 | Fixed (`bugfix.c`) |
| K280 | Fixed (`display.c`) |
| K291 | Direct3D only |
| V60 | Direct3D only |
| V61 | Direct3D only |
| V150 | Glide only |
| V153 | Glide only |
| V301 | Direct3D only |
