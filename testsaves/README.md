# Test saves

Two sets of saves for testing the game, SummonerFix and the rebuilt exes. None comes from anyone's own play.
- **The fight saves** (this folder, 2026-11-03): saves placed just before the major fights of Summoner, for combat,
  bosses, their stage changes and cutscenes. Made from a new game with the project's agent tools
  (`tools/testsaves/testsaves.mjs`). Each has the story party at levels that suit the fight, gear from a matching
  tier, healing and AP items, summoning rings and gold. The fight list follows the order of the boss guides on GameFAQs.
- **The play-through set** (`playthrough/`, 2026-11-04): 43 saves of the main story in order, from Masad to the last
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
| `01-barbarian-fighter.sav` | Barbarian Fighter, Masad | Joseph 3 | 0 | walk a short way on to the trigger point; the intro plays |
| `02-iron-golem.sav` | Iron Golem, two Broken Brass Golems, tentacles; Lenele sewers | Joseph 9, Flece 8 | 0 | walk a short way on into the boss room; the intro plays |
| `03-carados.sav` | Carados and the Bone Mages, Iona catacombs | Joseph 11, Flece 10 (Rosalind joins in the level) | 0 | walk a short way on to the crypt; the intro plays |
| `04-undead-rider.sav` | Undead (Ghost) Rider, night forest | Joseph 13, Flece, Rosalind | 1 | the intro plays on loading; let it play |
| `07-fire-salamanka.sav` | the small Fire Salamanka of the apothecary's quest, hills | all four, 15 | 1 | a short walk away; attack it (it does not attack first) |
| `08-giant-salamanka.sav` | Giant Salamanka, Khosani laboratory | all four, 16 | 1 | walk a short way on to the trigger point; the intro plays |
| `09-phoenix-rider.sav` | Phoenix Rider, hills | all four, 17 | 1 | the intro plays on loading; let it play |
| `10-ikaemos-priest-king.sav` | Ikaemos Priest King | all four, 17 | 1 | walk a short way on to the trigger point; the intro plays |
| `11-serpent-rider.sav` | Serpent Rider, forest | all four, 18 | 2 | the intro plays on loading; let it play |
| `12-tiger-rider.sav` | Tiger Rider, temple (Joseph and Flece, as in the story) | Joseph 18, Flece 18 | 2 | walk a short way on to the trigger point; the intro plays |
| `14-general-pijian.sav` | General Pijian, battle of Wolong | all four, 20 | 2 | walk a short way on to the trigger point; the intro plays |
| `15-serpent-and-phoenix-riders.sav` | Serpent and Phoenix Riders, Liangshan | all four, 20 | 2 | walk a short way on to the trigger point; the intro plays |
| `16-general-wentao.sav` | General Wentao, Liangshan | all four, 21 | 2 | the boss is a short walk away |
| `17-undead-and-tiger-riders.sav` | Undead and Tiger Riders, Jade Temple | all four, 21 | 2 | walk a short way on to the trigger point; the intro plays |
| `18-murod.sav` | Murod and Murod Oni, Jade Temple | all four, 22 | 3 | walk a short way on to the trigger point; the intro plays |
| `19-pyrul.sav` | Pyrul, Khosani laboratory | all four, 22 | 3 | walk a short way on to the trigger point; the intro plays |
| `20-luminar.sav` | Luminar, Iona | all four, 23 | 3 | the intro plays on loading; let it play |
| `21-titus.sav` | Titus, Ikaemos palace | all four, 23 | 3 | the intro plays on loading; let it play |
| `22-evil-urath-and-the-demon.sav` | Evil Urath, Evil Joseph and the Demon of Darkness, the temple | all four, 24 | 3 | the act-4 cutscene and the intro play on loading; let them play |

Not here: **Lord Korel (5), Zane (6) and Azha the Archlich (13)**. Each turns hostile through a conversation choice
after a quest, and in a save made by jumping to the level Korel stayed hidden even with his quest flag set. They
belong to the play-through set, made by playing the story in order.

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

`testsaves/playthrough/`: 43 saves, the main story from Masad to the last fight, played in order in agent mode on
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
  not played.

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
| `pt-43-temple-before-the-endings` | Temple of Urath | Machival dead; both endings ahead |

**Checked** 2026-11-04: each of the 43 loads in the original `Sum.exe` with SummonerFix and reaches play (game state
4) in the level it names; pt-43 also with its description (the Load Game screen shows it).

Found while making them (**Observed**, not yet in the game's bug list):
- killing the Tiger Rider while his dismount scene was still ending lost Tiger-Escape's end callback, so act 2 did
  not end (`act2_done` stayed clear); with the kill after the scene the outro ran;
- in Wolong under siege the street-fight chain (the alert soldier, the priest, the lieutenant) stopped before the
  lieutenant's "go" in this run; Pijian and what follows him do not depend on it, so pt-25 has the four street
  fights unplayed (`wolong2_fight1_won` .. `4` clear). The cause is not known (the party arrived by `exitto` and was
  moved by teleport).
