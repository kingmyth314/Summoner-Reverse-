# Test saves

Two sets of saves for testing the game, SummonerFix and the rebuilt exes. None comes from anyone's own play.
- **The fight saves** (this folder, 2026-11-03): saves placed just before the major fights of Summoner, for combat,
  bosses, their stage changes and cutscenes. Made from a new game with the project's agent tools
  (`tools/testsaves/testsaves.mjs`). Each has the story party at levels that suit the fight, gear from a matching
  tier, healing and AP items, summoning rings and gold. The fight list follows the order of the boss guides on GameFAQs.
- **The play-through set** (`playthrough/`, 2026-11-04): 43 saves of the main story in order (44 since pt-42b, 2026-11-08), from Masad to the last
  fight, each with the story state of a real game (quests, journal, party). See
  [the play-through set](#the-play-through-set-started-2026-11-03).

## Using them

1. Copy a save into the game's `savegame` folder (next to `Sum.exe`) under an unused name such as
   `summoner20.sav`. With SummonerFix's `[Debug] TestSaves=1` the folder is `savetest` instead.
2. Start the game, Load Saved Game: the save's description starts with `TEST` and the fight's number.
3. Do what the table says to start the fight. **Let the boss's intro cutscene play**: skipping Titus's intro with
   Esc left him standing inert in a test (below).

The saves load in the original `Sum.exe` (with or without SummonerFix) and in the rebuilt exes; a save's format is
the same for all (`docs/sum-exe/savegame.md`).

## The saves

Levels are Joseph's; the other heroes are about one level lower. Tiers: 0 early (Fiery Falchion, leather), 1 (Sword
of Spirits, chainmail), 2 (Logaros Longsword, Hero's chainmail), 3 (Sword of Summoners, Summoner gear).

| Save | Fight | Party | Tier | To start it |
|---|---|---|---|---|
| `01-barbarian-fighter.sav` | Barbarian Fighter, Masad | Joseph 3 | 0 | walk 10 units on along the path toward the Barbarian; the intro plays |
| `02-iron-golem.sav` | Iron Golem, two Broken Brass Golems, tentacles; Lenele sewers | Joseph 9, Flece 8 | 0 | walk 20 units toward the Iron Golem (in view, to the right); the intro plays |
| `03-carados.sav` | Carados and the Bone Mages, Iona catacombs | Joseph 11, Flece 10, Rosalind 10 (she joins in the level) | 0 | walk 8 units north into the crypt, round the bones; the intro plays |
| `04-undead-rider.sav` | Undead (Ghost) Rider, night forest | Joseph 13, Flece, Rosalind | 1 | the intro plays on loading; let it play |
| `05-lord-korel.sav` | Lord Korel, grassland encounter (added 2026-11-08) | all four, 14 | 1 | click Korel, 10 units away, and answer "Have you paid for that sword?", "One condition?", "Yes." |
| `06-zane.sav` | Zane and five bandits, grassland encounter (added 2026-11-08) | all four, 14 | 1 | click Zane, 10 units away, and answer "No." |
| `07-fire-salamanka.sav` | the small Fire Salamanka of the apothecary's quest, hills | all four, 15 | 1 | a short walk away; attack it (it does not attack first) |
| `08-giant-salamanka.sav` | Giant Salamanka, Khosani laboratory | all four, 16 | 1 | walk 8 units forward off the ledge; the intro plays |
| `09-phoenix-rider.sav` | Phoenix Rider, hills | all four, 17 | 1 | the intro plays on loading; let it play |
| `10-ikaemos-priest-king.sav` | Ikaemos Priest King | all four, 17 | 1 | walk a short way on to the trigger point; the intro plays |
| `11-serpent-rider.sav` | Serpent Rider, forest | all four, 18 | 2 | the intro plays on loading; let it play |
| `12-tiger-rider.sav` | Tiger Rider, temple (Joseph and Flece, as in the story) | Joseph 18, Flece 18 | 2 | walk a short way on to the trigger point; the intro plays |
| `13-azha-the-archlich.sav` | Azha the Archlich, Wolong caverns (added 2026-11-08) | Joseph 19, Flece 18, Rosalind 18 (Jekhar is away, as in the story) | 2 | click Azha, 5 units away, and choose "Challenge him to combat." |
| `14-general-pijian.sav` | General Pijian, battle of Wolong | Joseph, Rosalind, Jekhar, 20 | 2 | walk 7 units toward Pijian, south down the street (behind the camera); the intro plays |
| `15-serpent-and-phoenix-riders.sav` | Serpent and Phoenix Riders, Liangshan | all four, 20 | 2 | the intro plays on loading; let it play (since 2026-11-10; the party stands on `$sptrigger`) |
| `16-general-wentao.sav` | General Wentao, Liangshan | all four, 21 | 2 | the defence of the shrine: click the Guardian of the Shrine, 12 units away on his platform, and talk to the end ("Destiny?"); three waves of soldiers follow, then Wentao (moved 2026-11-08) |
| `17-undead-and-tiger-riders.sav` | Undead and Tiger Riders, Jade Temple | all four, 21 | 2 | walk a short way on to the trigger point; the intro plays |
| `18-murod.sav` | Murod and Murod Oni, Jade Temple | all four, 22 | 3 | double-click the red carpet just below Joseph, about five times (toward Murod, behind the camera); the intro plays |
| `19-pyrul.sav` | Pyrul, Khosani laboratory | all four, 22 | 3 | walk 7 units toward `$salstart`, Pyrul's door; the intro plays (moved 2026-11-08) |
| `20-luminar.sav` | Luminar, Iona | all four, 23 | 3 | the intro plays on loading; let it play |
| `21-titus.sav` | Titus, Ikaemos palace | all four, 23 | 3 | the intro plays on loading; let it play |
| `22-evil-urath-and-the-demon.sav` | Evil Urath, Evil Joseph and the Demon of Darkness, the temple | all four, 24 | 3 | the act-4 cutscene and the intro play on loading; let them play |

**Correction (2026-11-08):** Lord Korel (5), Zane (6) and Azha the Archlich (13) were missing until 2026-11-08.
Each turns hostile through a conversation choice, and in a save made by jumping to the level Korel stayed hidden
even with his quest flag set. They are now made from play-through saves, not from recipes (**Observed** in the
original `Sum.exe`):
- *05 and 06:* pt-10 (the world map after the Undead Rider) with the flag the Lenele conversation gives
  (`lenele1aa:unhide_korel` for Korel, `lenele1d:unhide_zane` for Zane, set with `savedit`). From there `exitto
  rand-grassland01 rand-grassland01_v3 1` entered the grassland encounter as the world map does, and the boss was
  there: Korel visible with Zane and the bandits hidden, or the other way round. Saved at the arrival point, about
  10 units from him.
- *13:* pt-23 (the Wolong caverns, the hand restored). Azha waits at `$archlich` (plane 7) with no trigger
  condition. The party was put 5 units from him on layer 7 the safe way (below, "19 moved"), and a Bone Knight
  27 units away was killed (see "Thirty seconds after loading").
- Each was then raised with `xpto` (05 and 06: Joseph 14, the others 13; 13: Joseph 19, Flece and Rosalind 18),
  given its tier's kit with `testsaves.mjs gear`, and its HP and AP filled with `savedit`.
- From a fresh load, a click on the boss walked the party up and opened his conversation. Korel turned hostile
  (type 5) after "Have you paid for that sword?", "One condition?", "Yes.", Zane after "No.", and Azha after
  "Challenge him to combat.". In each the party took damage within 25 s. Korel and Zane had left the living list by then (killed, **Inferred**).
  Loaded with no input, none of the three lost HP in 30 s.

## How they were checked

`node tools/testsaves/testsaves.mjs verify all` loads each save in the original `Sum.exe` with SummonerFix, starts
the fight as a tester would (walking to the trigger or the boss, letting the intro play), fights 35 seconds and
compares the party's HP before and after. Nobody steers the heroes in the check, so it shows whether the fight
starts, not whether it can be won. **Observed** 2026-11-03:
- the party took damage in 15 of the 19: 01, 02, 03, 08, 09, 10, 11, 12, 14, 15, 16, 17, 18, 19, 22;
- 04 Undead Rider: the party was killed ("Game Over"), so the fight ran;
- 21 Titus and 20 Luminar: the party in combat (the HUD's combat mark, Titus's health bar in view) but no damage
  within 35 seconds; Titus moved from his start point, which he does only after his intro;
- 07 Fire Salamanka: present next to the party, and it does not attack first (as intended for this quest monster).

They were made with the agent's `level` command, which enters a level as a new game would (state 2 resets the party
inventory and the journal; see `exitto` in `docs/tools.md`): their journal is empty, and the inventory is the kit the
finishing step adds. These saves skip the story before the fight: only the flags a fight needs are set (for example `masad_talked_to_nath`
for the Barbarian, `maindoor_open` for Carados, `ikaemos_main_door` for the Priest King). Quests, doors and NPCs
elsewhere in the level may not match a real play-through.

## The play-through set (started 2026-11-03)

The second set will follow the story in order, so that every save has the story state of a real game: quests,
journal, doors, the heroes joining when they do. It is made by playing in agent mode, with shortcuts only where they
do not change the story: walking inside a level by teleport, boss fights ended through the game's own damage
function, levels raised to what the walkthroughs suggest. A pilot on 2026-11-03 played the opening (**Observed**):
- Masad: Nath's conversation (driven by clicking each page's first line and END; the window's X does not answer
  agent clicks), "Find Yago" in the journal, the Barbarian's intro and his death, the boat to the world map;
- the world map: a random encounter interrupted the walk to Lenele twice;
- Lenele's outskirts, first reached with the agent's `level`: the journal read back empty. The cause: `level` calls
  `game_change_level`, which goes through the title state (state 2) and resets the inventory and the journal. A new
  agent command, `exitto`, leaves a level as the game's own exits do; with it "Find Yago" stayed in the journal. The
  play-through uses only `exitto`.

The opening (one conversation, one boss, one exit) took about 45 minutes of agent work; Lenele alone has fifteen
quests and several districts. The full set is a job of several sessions, kept out of the repository until each save
is checked like the fight saves.

### The set (2026-11-04)

`testsaves/playthrough/`: 43 saves (44 with pt-42b, added 2026-11-08 for the first ending), the main story from Masad to the last fight, played in order in agent mode on
the original `Sum.exe` with SummonerFix, from the opening pilot on. Load them as the fight saves (above); their
descriptions start with `TEST PT` and the number.

How they were made (**Observed**): every conversation of the main story was played through the dialogue window
(each offered topic clicked, the agent's `dlg` and `talkto` commands; a conversation is read to its end, so side
topics are spent too); levels were left through `exitto` only; the story items were taken from the containers the
game drops or places (`items` / `open`). Shortcuts, none of which changes the story state:
- walking inside a level by teleport (`navgoto`, `npcgoto`), and the world map crossed by `exitto` (no random
  encounters);
- every fight ended through the game's damage function (`npckill` / `npchp`), boss stages and cutscenes in their
  own order. The party gets no experience and almost no loot from those fights, so gold stays at 59 from the
  Catacombs on;
- party levels raised with `xpto` (through the game's XP code) to the level `game_stage_set` gives each story stage,
  from Iona on (stage 2: 7, stage 4: 9 ... the last fight: 25). The game itself never raises them after a load
  ([`level_entered_by_load`](../docs/sum-exe/main-loop.md)); pt-01..05 keep the levels the opening gave (Joseph 3);
- the party made immortal (the `immortal` console flag, `0x941704`) during the Liangshan waves and the act 4 fights,
  and mortal again before each save;
- Lenele's side quests, the Khosani and Wolong side quests, and the optional bosses Lord Korel, Zane and Azha were
  not played. (Since 2026-11-09 the side quests have been played one by one from these saves with
  `tools/testsaves/sidequests.mjs`, each from a fresh load, and nothing was saved back into the set: results on
  [Side quests](../docs/sum-exe/side-quests.md). Korel, Zane and Azha are fight saves 05, 06 and 13.)

| Save | Where | The story so far |
|---|---|---|
| `pt-01` .. `pt-03` | Masad, the world map, Lenele's slums | the opening pilot: Find Yago |
| `pt-04-sewer-flece-joined` | sewer | Flece joined at the market gate |
| `pt-05-palace-ring-of-darkness` | Lenele palace | Flece's sneak, Meeting-With-Yago, the Ring of Darkness |
| `pt-06-iona-rosalind-joined` | Iona | Rosalind joined; the abbot's talk |
| `pt-07-catacombs-ring-of-light` | Iona catacombs | Brother Ovaard's key, the spheres, Carados, the Ring of Light |
| `pt-08-act2-lenele-dock` | Lenele dock (`lenele1d_v2`) | act 2 begins (Nar-Act2) |
| `pt-09-palace-jekhar-joined` | Lenele palace (`lpalaceint_v2`) | Yago and the king; Jekhar joined |
| `pt-10-worldmap-after-undead-rider` | world map | the Ghost (Undead) Rider driven off |
| `pt-11`, `pt-12` | Khosani laboratory | the stronghold's Guardian and Ulsadana; the Giant Salamanka, the Ring of Fire |
| `pt-13-worldmap-after-phoenix-rider` | world map | the Phoenix Rider (killed twice: it is reborn once) |
| `pt-14-ikaemos-ring-of-stone` | Ikaemos palace | the dagger slots, the Priest King, the Ring of Stone |
| `pt-15-worldmap-after-serpent-rider` | world map | the Serpent Rider |
| `pt-16-lenele-besieged` | Lenele gate (`lenele2d`) | Pedrog |
| `pt-17-occupied-lenele` | occupied Lenele | Joseph and Flece only |
| `pt-18-temple-before-tiger-rider` | Temple of Urath | Eavesdropping played; the Tiger Rider next |
| `pt-19-act3-eleh`, `pt-20-eleh-joseph-found` | Eleh | act 3: Flece alone, then Joseph found |
| `pt-21`, `pt-22` | Wolong | Rosalind rejoined; Sizhe's four items, the draught |
| `pt-23-wolong-caverns-hand-restored` | Wolong caverns | Ngaru's Ring of Water, Hand-Restoration |
| `pt-24`, `pt-25` | Wolong under siege | Pijian, Truth-About-Aoqi; Jekhar and the Ring of Four Winds |
| `pt-26`, `pt-27` | Liangshan | the two riders, the shrine's defence, Wentao; the Ring of the Forest |
| `pt-28` .. `pt-30` | Jade Temple | the Tiger and Ghost Riders, Murod, the Ring of Jade |
| `pt-31`, `pt-32` | Khosani stronghold | act 4; Ulsadana (capture the four demons) |
| `pt-33-iona-yago-found` | Iona in the snow | Yago found |
| `pt-34`, `pt-35` | Lenele in the snow, the palace | Rosalind and Jekhar alone; Sornehan and Galienne, the Khosani scrolls |
| `pt-36`, `pt-37` | stronghold, second laboratory | the scrolls for the Ring of Fire; Pyrul trapped |
| `pt-38` .. `pt-41` | stronghold, Iona, Ikaemos | the Rings of Light and Stone, Luminar and Titus trapped, the Ring of Darkness |
| `pt-42-temple-of-urath-machival-fight` | Temple of Urath | the last fight under way (its intro plays on arrival) |
| `pt-42b-temple-machival-nearly-dead` | Temple of Urath | Evil Urath and Evil Joseph dead, Machival at 5% and idle; both endings ahead (added 2026-11-08) |
| `pt-43-temple-before-the-endings` | Temple of Urath | Machival dead; only the second ending ahead (see below) |

**Checked** 2026-11-04: each of the 43 loads in the original `Sum.exe` with SummonerFix and reaches play (game state
4) in the level it names; pt-43 also with its description (the Load Game screen shows it).

**The endings, 2026-11-08** (**Observed** in the original `Sum.exe` and in the 64-bit exe, each run from a fresh
load to game state 9, the credits). The forge ending (End1) starts only when a byte is set that Machival's death
scene sets and every entry to the level clears ([G130](../docs/sum-exe/game-bugs.md)), so no save made after his
death can reach it: the game itself refuses saving there, and pt-43 was saved with the agent's `savenow`.
**Correction:** pt-43's "both endings ahead" was wrong. Loaded, with the party on `$end2trig`, nothing happened;
with the byte poked to 1 End1-EnterForge started at once. pt-43 gives only the second ending: walk back out of the
temple (back toward the entrance, about 23 units, to the `lenele3a` exit) and End2-LeaveTemple, End2-Montage and the credits play.
A save before the first ending must be from before Machival dies, so pt-42b was made from pt-42 (intro played,
Evil Urath and Evil Joseph killed with `npckill` after each transition scene, Machival brought to 5% with `npchp`,
the party put on `$temple_01`, 6.5 units in front of him, saved with `savenow`). Machival loads idle: his combat
switch (`0x248b034`) is not saved, so he does not fight back. The party did not attack him from that spot within
60 s; next to him it killed him in 15 to 20 s. Machival-Death then plays and the party (Joseph) stands at entry 2;
from there `$end2trig`, 25 units back toward where Machival stood, starts End1-EnterForge and End1-Montage, and the exit, 23 units back toward the entrance,
End2. Both endings reached the credits from pt-42b in both exes, and pt-43's End2 in both. The re-save changed
more than the story flags, as 19's did: the item list came back in another order, 1 Cleansing Elixir of 10, Flece
hurt, and three members' AP above the max savinfo shows (**Inferred**: the party used the elixirs against Evil
Joseph and Machival, and AP filled to the max the gear gives, which savinfo does not count). The elixirs and
Flece's HP were set back with `savedit`; flags, party, gear and gold are otherwise pt-42's. **Correction (the same
day):** the elixirs were not used. A re-save of pt-42 with no movement and no fight also came back with 1 Cleansing
Elixir and the list re-sorted (**Observed**): a Cleansing Elixir holds 5 charges and never stacks, so the loader
makes any count 1, and it sorts the list as it adds. See the next paragraph.

**Charges, fixed 2026-11-08** (**Observed**). Our save editors wrote every item they added with 0 charges (the third
word of an inventory entry, [Savegame](../docs/sum-exe/savegame.md)), and the game will not use an item at 0
charges. In the original `Sum.exe`, pt-43 as committed on 2026-11-07: "Use" on the Healing Elixir (50, charges 0
in memory) left 50 and healed nothing; a Health Tonic, an entry the game had written itself (charges 1), healed
Joseph from 124 to 195 HP. 60 of the 63 saves had such items: every healing, AP and cleansing item `gear` and the
recipes added (all 19 fight saves and pt-04 .. 43 with pt-42b; pt-01 .. 03 have only the game's own). Both editors now write the item's full
charges, and all 60 saves were repaired with `savedit --fix-charges`: charges 0 -> 1 (5 for the Cleansing Elixir,
whose count becomes 1); savinfo shows nothing else changed. Repaired pt-43 in the original: all potions at charges 1
and the Cleansing Elixir at 5 after a fresh load, and a Healing Elixir healed Joseph from 124 to 227. `gear` now
asks for one Cleansing Elixir. **Inferred:** the AP above savinfo's max after a re-save (also on the no-fight
re-save) is the max the gear gives, which savinfo does not count. The doubled rings of the 19 re-save are not
explained yet. **Explained (the same day, Observed):** they were in the committed file. `finish` added the tier's
summoning rings to a raw save that already held the rings its story gave, and equipped new copies of the two
Joseph wears while the old ones stayed in the inventory: 16 fight saves had a worn ring listed again (03 .. 22), 13
of them also two of up to six rings (19 had eight rings plus six more). A played game lists each ring once and a
worn ring not at all (pt-42). The 16 were repaired with `savedit` (`--set-item-count "Ring of X=1"` for each
doubled ring, `--remove-item` for each ring Joseph wears); `finish` now adds only the rings the raw save lacks and
takes the worn ones out of the inventory. The play-through set was not affected (`gear` already did both). After the
repair each of the 16 loads into play in the original `Sum.exe`.

**19 moved, 2026-11-08** (**Observed** in the original `Sum.exe`). **Correction:** 19 as committed was broken
where it stood, not only far away: at the KhosaniLab2 entrance the lab's enemies engaged during the first 20 s, and
Rosalind died with no input (a re-save without any move came back with her dead); `verify` teleported the party
away before that showed. Moving it was hard for two reasons found on the way: `party_teleport_to` (`0x439750`)
lines the followers up 0.75 apart along +x at the leader's height, and right after the move the followers' combat
or path state from the entrance sent them back to (-97.9, -9.8, 45), layer 0, next to four Burning Bone Knights
they could not reach (the knights, idle, did no damage in 120 s; `npckill` leaves those four "dying", and five
others are not script objects). What worked: the party immortal (`0x941704`) from before the load, the leader
teleported to pt-37's spot, each member's position and layer written onto pt-37's four standing spots (the
played save right after the Pyrul fight), immortal off, saved at once. Loaded fresh, all four stood together at full
HP for 30 s (one follower stepped 3.7 units), so the stale state does not survive a load. From that load, double
clicks toward `$salstart` (6.6 units; it fires within 4) started Pyrul-Intro. Against the committed file: the
position, the item order, AP filled to the gear's max, and the party-state word's bit 0x10 (not known what it is).

**16 moved, 2026-11-08** (**Observed** in the original `Sum.exe`). 16 stood at Liangshan's entry, about 270
units from the shrine (two members lost HP in the first 20 s there, then it stopped). It was moved as 19 was (immortal
from before the load, each member written onto a spot, saved at once) onto the four navpoints `$npc197` ..
`$npc201`, 12 to 15 units from the Guardian of the Shrine (layer 17; he stands on a wooden platform, layer 18, inside
the hollow tree). Loaded fresh: all four held their spots at their saved HP. `talkto` (a click on him: the party
walks up by pathfinding) opened his conversation at 8.5 units; clicking "Child of prophecy?" .. "Destiny?" in turn
ended it with `EnemyAttack` (`liangshan_flag_wave1` set, the first soldiers sent). None of the riders' flags is
needed: the conversation has no condition, and the riders' ground is about 200 units away.

**Thirty seconds after loading, 2026-11-08** (**Observed** in the original `Sum.exe`). Every save, all 63, was
loaded with no input and the heroes' HP read from memory at once and 30 s later, against the HP in the file.
The 44 play-through saves: no loss. A member reported missing there is one who is away at that point of the story
(Jekhar in act 3, for example). The fight saves:
- *04, 09, 11 (the riders whose intro plays on loading):* HP lost, as intended: the fight starts with the intro.
- *03 and 17, fixed:* ordinary enemies stood next to the party and attacked at once: a Bone Mage and two Bone
  Knights in 03, four Orenian Samurai in 17. In 17, Joseph died within seconds of loading (the file held a running
  fight). None of them is a level script object (`npckill` answers "script object -1"), so each was killed through
  `callt 450850 0xCHAR 0xC9742400 -1 0 0` (`character_add_hp(-1000000)`; the pointer needs `0x`, as `callt` reads
  THIS with base 0, and a pointer without it crashed the game), its current move cleared, then the game saved.
  As a player would have cleared them before the boss. No enemy came back within 30 s. Carados's two guards
  (type 4, waiting for the intro) were left alone.
- *Companions too low, fixed:* the sweep also showed that in 03, 04 and 08 to 12 the companions were 1 to 7 levels
  below the recipe (08 to 11: Jekhar, Flece and Rosalind at 10 or 11 beside Joseph at 16 to 18). `join NAME LEVEL`
  returns a hero who is already in the party (one the level's story added) as he is, so the recipe level was ignored.
  Each save was loaded with the party immortal, each companion raised with `xpto` (HP, AP and skills as from play)
  and saved. `build` now runs `xpto` after every `join`.
- *HP and AP refilled:* after those saves, and in 16 and 22 (Joseph at 416/464 and 301/488), every member's HP and AP
  were set to their maximum with `savedit`, the description put back, and in 04, 09 and 11 the intro flag cleared
  again (`ghost_intro_played`, `phoenix_intro_played`, `serpent_intro_played`), so loading plays the intro.
- *15 and 18:* the same enemy problem; see the next paragraph.
- Afterwards 03, 08, 10, 12, 15, 16, 17 and 18 lost no HP in 30 s with no input. `verify` started each fight from a
  fresh load: 03, 04, 08, 09, 10, 11, 12 and 17 all took damage from their boss.

**15 and 18, still open (2026-11-08, Observed in the original `Sum.exe`).** Both stay where they were. Each lost
HP in the first 20 s after loading (15: Jekhar 598 to 400; 18: Jekhar 608 to 551). The cause was the same as in 03
and 17, enemies beside the party: seven Onif in 15, two OG Mages in 18 (a third already dead). They were killed the
same way, the party made immortal from before the load (in a first try without it, Flece died in 15 during the
load), and the saves made again with HP and AP refilled. **Fixed:** neither loses HP in 30 s now, and `verify`
starts both fights. Two Onif in 15 and both OG Mages in 18 stay "dying" (HP 0, flag `0x8`) and do nothing. Still
open is the walk to the trigger. From 18's spot, 58 units from `$murodtrigger` (which is up on a raised floor, y 17.9),
the straight walk stops against a wall between two stairways. A walk by waypoints along x = -32 to -35 hardly moved
the party (3.4 units per click straight on, 0.3 sideways).
- *15:* every layer-15 spot near `$sptrigger` that was tried, 10 units north or north-west of it, was moved by
  the game to (-127.2, -0.5, 125.3), among the graves, 12.2 units from the trigger. From there a rock blocks the
  straight walk, and the party, in combat mode, did not move in 25 double clicks. A spot on layer 14 east of the
  trigger was moved to (-98.9, ?, 110.2), 22 units away, and the party did not move from there either. How a player
  reaches the riders' ground is not known yet.
- *18:* `$murodtrigger` (radius 8) is inside entry 2's four starts, so in the story Murod-Intro plays on arrival.
  A party put 13 units south of it on layer 4 walked into the trigger by itself within 5 s: the leader kept a move
  goal from before the teleport (a click at his feet cancelled it). The straight walk from there stops against a
  wall at 13 units; the leader's own walk had gone round through (-34.9, 134.2) and (-34.6, 143.9), but clicks along
  those points did not get past 14 units, and `talkto Murod` (6 units away, behind the wall) did not move the
  party.

**15 and 18, closed (2026-11-10, Observed in the original `Sum.exe` and in the 64-bit exe, each from a fresh load
with no help).** Both were moved with the method above (party immortal before the load, `posgoto`, each member
written onto its spot, Joseph selected again, saved at once) and given back their descriptions; `savinfo` shows
nothing else changed but the level time and AP filled to the gear's max (as in every re-save).
- *18:* the party stands on the raised floor (layer 4, y 17.8), 12 units north of `$murodtrigger`, Joseph at
  (-43.0, 158.8), on the red carpet that leads to Murod. Spots all round the trigger at 10 and 12 units were tried
  first; on this floor every one held. The camera faces the far wall, so Murod is behind it. Four double clicks on
  the carpet just below Joseph moved him a unit each, the fifth crossed the 8-unit radius and Murod-Intro played
  (both exes).
- *15:* how a player reaches the riders' ground is still not known. Spots on layer 15 that held were 21 to 59 units
  from the trigger, and from 31 units south (-130.7, 85.0) the party did not get closer in 25 calibrated double
  clicks: the ground ends at an edge there (the cursor turns into an arrow). So 15 is now made like 11: the party
  stands on `$sptrigger` and the intro flag is clear. To save there without the intro firing first,
  `phoenix_serpent` (level 20) was set in memory with `flagl` before the party was placed, and cleared in the file
  afterwards (`savedit --flag Liangshan:phoenix_serpent=0`). Loaded, SERP-PHNX-Intro plays at once and the riders
  fight; with no input the party lost (game over within 25 s of the intro's end in one run, the riders dead in
  another). Teleported onto the trigger from the old save, the idle party fared the same (only Joseph alive 25 s
  after the intro), so that is the fight, not the spot.

**Gear and positions, fixed 2026-11-06** (**Observed**; a tester reported heroes in starting clothes and a broken
camera or position in some saves):
- *Gear:* the fights were ended through the damage function, so no hero picked up loot: all 43 saves had the
  starting weapons and clothes (Joseph's Short Sword and Tunic even at level 25), no ring worn, 59 gold. pt-04 .. 43
  now carry the fight saves' kit by story stage (`testsaves.mjs gear`): tier 0 to pt-07, 1 to pt-18, 2 to pt-30,
  3 after; Joseph wears the two newest summoning rings the party has, and the healing items and gold are topped up
  to the tier's. pt-01 .. 03 keep the starting gear, which is what a player has there. In the fight saves Rosalind
  in 03 and Jekhar and Rosalind in 12 had kept their starting gear (`finish` took the party from the recipe); fixed.
- *Positions:* the agent's `navgoto` / `npcgoto` wrote only the leader's position, not the layer, the others or
  the camera. Loaded and walked in the original `Sum.exe` (four clicks around the leader), nine saves were broken:
  the leader under the ground or in a void (03, pt-20), in a pit below the party (pt-32), inside a wall (08),
  above water or ground he could not leave (01, 02, 14, pt-25), against a tree (pt-14); in 14 the others stood
  200 units away. Each was loaded, the party moved with `posgoto` (the game's `party_teleport_to`) onto a
  follower's spot or a nearby point of the same level, saved, and given back its description (pt-32 also its
  `khosani_strng_ulsadana2` flag, cleared during the re-save); 08 first went to the laboratory entrance (pt-11's
  spot). **Correction (2026-11-06):** that is the first room of the labyrinth, about 100 units and a layer away from
  the Giant Salamanka, not outside its room. 08 now stands on the rock ledge 8 units before `$salstart`
  (`posgoto -118.65 -16.44 43.78 1`); loaded in the original, two double clicks forward start the intro and the
  fight follows (flags, party and gear unchanged). All nine then walked, with the party together. The other saves walked
  as they were; in combat mode (pt-29) a double click is needed to move. The agent's teleports now go through
  `party_teleport_to` (docs/tools.md). After the changes, `testsaves.mjs verify` on the six changed fight saves
  (01, 02, 03, 08, 12, 14) in the original: each fight started and the party took damage. **Correction
  (2026-11-07):** `verify` teleports the party to the trigger (`navgoto`) and to the boss (`npcgoto`) before it
  watches the fight, so it does not test where a save stands. Measured on loading, the repairs had put five of the
  six far from their fight, on a follower's spot that was itself far away: 02 124 units from `$tbintro`, 03 788
  from `$ctrigger`, 08 103 and a layer from `$salstart`, 14 507 from Pijian; 01's Joseph, 7 units from
  `$cuttrig`, stood over the water and did not move. Each was moved again with `posgoto` near its trigger, on open
  floor with no intro started, and saved with its description back (flags, party, gear and gold unchanged).
  **Observed** in the original, from a fresh load, double clicks toward the trigger only:

  | Save | Spot (`posgoto X Y Z LAYER`) | Walk to the intro |
  |---|---|---|
  | 01 | `-212.07 -14.24 72.65 0`, on the path 10 units before `$cuttrig` (2.5 units) | 7 clicks |
  | 02 | `218.31 22.59 -150.80 2`, 20 units west of `$tbintro`, the Iron Golem in view | 2 clicks, toward the golem |
  | 03 | `17.10 -56.56 452.21 9`, 8 units south of `$ctrigger` (4 units; needs `maindoor_open`, set) | 12 clicks round the bones |
  | 08 | `-118.65 -16.44 43.78 1`, the rock ledge 8 units before `$salstart` | 2 clicks forward |
  | 12 | unchanged, 11 units from `$tigertrigger` | 2 clicks |
  | 14 | `-2.58 2.48 -55.15 0`, the street 22 units north of Pijian (within 15 starts it) | 12 clicks |

  The camera often faces away from the fight: walk toward the boss, not up the screen.

  The same measure on the fight saves the repairs did not touch (**Observed**, 2026-11-07): 07 20 units from the
  Fire Salamanka, 10 15 from `$ipktrigger`, 17 36 from `$tgtrigger` (24 starts it); still to fix (15 and 18 fixed
  on 2026-11-10, see "15 and 18, closed" above), their rows'
  "a short way" is wrong: 15 252 units from `$sptrigger` (the riders' ground is layer 15, the land east of it
  layers 13 and 14 below; a party put on layer 15 off that ground slides into the intro), 18 58 units and 13 below
  `$murodtrigger`, 19 79 units and 55 above `$salstart` (the laboratory entrance, as 08 was). 16 is a defence in
  waves that the Guardian of the Shrine's conversation starts (`EnemyAttack`: one soldier, three, two samurai and a
  mage, then Wentao), not a walk to Wentao; the save has none of the wave flags. A first re-save of 19 on 08's ledge
  started Pyrul's intro by walking but came back with each ring counted twice, 1 Cleansing Elixir instead of 10 and
  three members hurt; it was not kept. pt-11 and pt-19, which the four-click probe had
  passed only on captures (its clicks hit walls and pt-11's exit strip), walked 4.5 to 6.4 units per double click on
  open floor.

Found while making them (**Observed**, not yet in the game's bug list):
- killing the Tiger Rider while his dismount scene was still ending lost Tiger-Escape's end callback, so act 2 did
  not end (`act2_done` stayed clear); with the kill after the scene the outro ran;
- in Wolong under siege the street-fight chain (the alert soldier, the priest, the lieutenant) stopped before the
  lieutenant's "go" in this run; Pijian and what follows him do not depend on it, so pt-25 has the four street
  fights unplayed (`wolong2_fight1_won` .. `4` clear). The cause is not known (the party arrived by `exitto` and was
  moved by teleport).

Rechecked 2026-11-05 in the original `Sum.exe` (**Observed**). Neither is a game bug; both stay out of the bug list:
- *Tiger Rider:* from pt-18, the rider was brought to 40% (`npchp`, Tiger-Dismount) and killed (`npckill`) at
  0 s, 3 s and 6 s into the dismount scene, in the same command as the damage, and after the scene. Each time
  Tiger-Escape played, its end callback ran, `act2_done` was set and Act2-Outro took the game to Eleh. The code
  agrees: two `script_play_cutscene` calls in one frame leave the second's callback in place, and
  `script_cutscene_done` runs it. The loss seen on 2026-11-04 was not reproduced; its cause is unknown.
- *Wolong street fights:* from pt-24 the whole chain ran (`wolong2_fight1_won` .. `fight4_won` set, the lieutenant
  on action 30 of 33) when the party followed the lieutenant and stood next to each enemy before `npckill`. Two
  ways to stall it, both from the agent tools: (1) with teleports after Yishi's talk the priest spoke minutes late,
  only after the party was moved back near him (**Inferred**: a conversation walking up, `conversation_active` state
  3, keeps `ls_wolong2_force_recruit`'s `is_idle()` false; the state was not sampled then); (2) a character killed
  more than about 35 units from the party never finishes dying: the renderer puts the AI of a type 4 or 5
  character in view but farther than √1200 from the current character to sleep each frame (`ai.flags` 0x40,
  `unnamed_render_scene_0x481680.cpp`), and its death waits for an AI frame. The 2026-11-04 run used both.
