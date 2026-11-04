# Test saves

Saves placed just before the major fights of Summoner, for testing the game, SummonerFix and the rebuilt exes:
combat, bosses, their stage changes and cutscenes. They were made on 2026-11-03 for this purpose, from a new game,
with the project's agent tools (`tools/testsaves/testsaves.mjs`). None comes from anyone's own play. Each has the
story party at levels that suit the fight, gear from a matching tier, healing and AP items, summoning rings and gold.
The fight list follows the order of the boss guides on GameFAQs.

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
