---
title: "Side quests"
parent: Sum.exe
nav_order: 6.33
---

# Side quests

**Summary (2026-11-09):** `events.tbl` lists 56 side quests (journal entries 20 to 75). Each one is opened by a
character's dialogue (`+Unhide Quest:`) and closed by another topic (`+Hide Quest:`). The reward sits in the closing
topic: items (`+Gain Item`), experience (`+AddXP`) and gold (`+AdjustGP`). Every quest that has dialogue was played
in the original `Sum.exe`, in agent mode, from the play-through saves (`testsaves/playthrough/`) with
[`tools/testsaves/sidequests.mjs`](../tools.md#toolstestsavessidequestsmjs-side-quests-played).
- Closed, with the reward: 51 (The King of Talas only at the late Saanavarh visit, below; The Lord Chamberlain's Men
  added on 2026-11-10). Two of them only from their closing topic (32 was already open in the save; 33 has
  no opening topic), and 41 closes with no reward. (Counted 51 until the evening of 2026-11-09, see the correction
  below.)
- Closed later than intended: The King of Talas. The king who pays stays hidden in Saanavarh through the v4 and v5
  stages and shows only in v6 (game stage 20; bug [G313](game-bugs.md), fixed by `[Fixes] TriggeredShow`).
- Opened, with a closing topic that the test did not reach: none since 2026-11-10. Until then The Lord Chamberlain's
  Men: Torvel's double, who pays, is hidden until the lanterns are given to the Master of Revels in the palace, as
  designed, and that part had not been played (spec `torvel2` plays it, below).
- Opened, with no closing topic in the game's data: 2 (The Infestation, Crazy Iven).
- Cannot appear in the journal: 3. Two are developer leftovers (Bleargh, Ajaymes the Pirate), one is cut (Find a
  blue shard for Varyssa).

One quest is broken in the game itself: The King of Talas cannot end until late in act 4 ([G313](game-bugs.md)). The other findings below are about
how the quests work, and one oddity in the data (Torva's 125,000 gold).

**Correction (2026-11-09, evening).** Until then the test's `talkto` command talked to any character by name, hidden
or not, which a click cannot do. It produced a false bug, G312 (Torvel's double paying without the quest; withdrawn,
see [Game bugs](game-bugs.md#claims-that-did-not-hold)), and it closed six quests by talking to a hidden character.
With `talkto` skipping hidden characters (flag `0x2`), the whole run was repeated (**Observed**, original `Sum.exe`)
and those six were played again in the state the game shows the character:
- The King of Talas (40): the full path (the forest king, Uligo the Artisan, "Tell him what the Khosani said.") still
  leaves the closing king hidden in Saanavarh v4 (and v5): bug G313. On 2026-11-10 the v6 visit (from pt-41) showed
  him and he paid; spec `talas6`.
- The Lord Chamberlain's Men (68): the double is hidden until the lanterns (`ls_lenele1ab_init`); not played further
  that evening. **Observed** 2026-11-10 (spec `torvel2`, from pt-08, in the original and the 64-bit exe): Torvel ("How
  can I help you?"), the Master of Revels in the palace ("What do they have to do?"), Krys the Lantern Maker in the
  market ("Gargoyle blood?"; the six Gargoyle Blood given, standing in for the Ikaemos gargoyles; "I have six drams"
  gave the Stonefire Lanterns and 2,000 XP), the Master again ("I have the Stonefire Lanterns!", 500 XP), and back in
  the theatre district the double, now shown, closed the quest on "Hail": 2,000 XP and 5,000 gold. No flag was set
  by hand.
- The Seeds of Aahur, Part 2 (71): Elarh (act 4) is there only with `buy_seeds`, set at the end of part 1; with it set
  and Saanavarh entered again, the quest closed as before.
- The Tome of the Nhuvasarim (33): Gesualdo's second self is chosen when the forest loads with the pages; given first
  and the forest entered again, "Enter the tome." was reached.
- The Emperor's Dragon Scales (41): Gongji shows when the Ring of Jade is taken (his script's label 1) and is hidden
  again once the party leaves the temple and comes back; from the save made after the ring (pt-30) his topic was
  reached. Whether the story brings a player back to the temple later was not checked (**Unknown**). **Answered
  2026-11-10 (code and data):** the hand-in can be missed. Gongji's entry in `jadetemple_script.tbl` starts hidden,
  and only the end of Murod-Death sends him to label 1, where he shows (`level_script_jadetemple.cpp`). After that,
  Qifeng's "Yes, we will leave now." (trigger `EndCutscene`) moves the party to the stronghold
  (`ls_jadetemple_finish`); "No, we will not leave yet." keeps it there. From game stage 14, which the temple sets,
  the world map keeps the temple as an exit (`ls_worldmap1_setup_jadetemple`), so the party can come back
  (**Inferred**), but every later entry runs the start script again and Gongji stays hidden (**Observed** on
  re-entry, above). The scales can all be had before the temple: the Green from Xueli in Eleh, the Blue in the
  desert encounter's v3, which rolls at game stages 9 to 14 (`ls_worldmap1_pick_encounter_stage9`); the Red and the
  Gray are placed in levels not checked here. Gongji takes one to four (`CheckScales`), and the quest closes with no
  reward either way. So it is a quest that closes only in the window between Murod's death and leaving the temple
  (**Inferred** from the code; not a bug: the script does what the data says).
- Curse of the Crone of Savheo (50): which forest character stays is random on each visit (`ls_rand_forest01_init`);
  on the second visit the crone was hidden in the first run and present on a later one.

## How they were played

**Observed** 2026-11-08 and 2026-11-09, in the original `Sum.exe` with SummonerFix, the party immortal. One fresh
load per quest. The tool talks to each character along the shortest path through his topics, saves after each
conversation and compares the save with the one before: journal, inventory, gold, experience. Experience is read
from Joseph, so a reward paid while he is away (Durgan in act 4) shows only on the others.

The objective in between was mostly not played. In its place, each step reported in the results:
- the item a quest sends the party to find was **given** (`party_inventory_add_by_name`);
- a monster to kill was killed with `npckill` (Sir Horum, for the Lord of Logaros);
- where the closing topic waits on a story flag (`+Unhide by event`), the flag was **set** (`flagl`), for example
  `ivane_unhide_evien` for Lady Ivane and `liangshan_mq_placed_coin_locket` for the Family Tradition;
- a quest item flagged "not identified" was **marked identified**, as the Lore skill would (below).

So the results show that each quest's dialogue opens it, accepts its objective and pays its reward. They do not show
that each item can be found where the game puts it.

## The quests

Rewards as observed; XP is the party's (each member gains it). "Given" lists what the test supplied.

| # | Quest | Opened by (level) | Closed by | Given / set | Result |
|---|---|---|---|---|---|
| 21 | The Infestation | Merden (lenele1b) | none in the data | | opened; nothing closes it (a bounty: 50 gold per bacite tail) |
| 22 | The Beggar's Eyes | A Beggar With No Eyes (lenele1b) | the beggar | | Blessed Surcoat, 200 XP |
| 23 | The Salamanka's Tongue | Apothecary, after the beggar's story | the apothecary | Salamanka Tongue | Bottle with Eyes, 50 XP |
| 24 | The Merchant of Orenia | Aravind (lenele1b) | Aravind | | 50 XP, 25 gold |
| 25 | Free Jinyan | Gorbus the Guard (lenele1b) | Gorbus | Forged Document | 100 XP (20 gold paid) |
| 27 | The Salt Smugglers | Pedrog (lenele1b) | Pedrog | Chunk of Salt | 375 XP, 900 gold |
| 28 | The Rag Doll | Weeping Butcher (lenele1b) | the butcher | Rag Doll | High Quality Knife, 400 XP |
| 29 | The Lord of Logaros | Lord of Logaros (lenele2aa) | the Lord | Sir Horum killed | Logaros Longsword, 2,000 XP |
| 30 | Adeslani's Brother | Adeslani (lenele2ab) | Adeslani, after her escort | the three plaza soldiers killed | 4,188 XP (4,000 in the table; the rest presumably the soldiers), quest closed, 2026-11-09 (below) |
| 31 | The Fate of Rhukha | Keeper of Lore (KhosaniStrng_v2) | Keeper of Lore (v4) | | both ends, separate runs: Arcticus Mantellum, 15,000 XP |
| 32 | The Amulet of Prince Yon | (pt-05 had it open) | Drego (lenele1d) | Yon's Amulet | 200 XP, 2,000 gold, 3 Health Tonics |
| 33 | The Tome of the Nhuvasarim | none | Gesualdo (rand-forest01_v6) | tome and eight pages, identified | tome and pages taken, `gesualdo_done` set, the party sent to the Jade Temple (`jadetemple_v2`): the tome is a portal. Gesualdo's second self appears only when the forest is entered with the pages |
| 34 | The Hunting Horn of Vadagar | Saato the Priestess (KhosaniStrng_v4) | Saato | horn, identified | Nobleman's Platemail, 10,000 XP |
| 35 | The Combustible Crook | Uhika (KhosaniStrng_v4) | Uhika | Pipe | 2,000 XP, 10,000 gold |
| 36 | The Staff of Profuse Comestibles | Uhika, after 35 | Uhika | Fishing Pole | 4,000 XP, 20,000 gold |
| 37 | The Curious Device | Uhika, after 36 | Uhika | Spyglass | 8,000 XP, 30,000 gold |
| 38 | The Battle of Quadraticus | Uhika, after 37 | Uhika | Chess Piece | 16,000 XP, 40,000 gold |
| 39 | The Elixir of Olfactory Obfuscation | Uhika, after 38 | Uhika | Perfume | 35,000 XP, 100,000 gold |
| 40 | The King of Talas | King of Talas (rand-forest01_v6) | the King in Saanavarh (v6; hidden in v4, v5) | | closes only at the late Saanavarh visit (v6, game stage 20): Hero's Platemail, 20,000 XP (2026-11-10, from pt-41). In v4 and v5 the paying king stays hidden (G313; with `[Fixes] TriggeredShow` he shows there too). Until 2026-11-09 counted as closed in v4 (hidden king reached by `talkto`) |
| 41 | The Emperor's Dragon Scales | Xueli (eleh) | Gongji (jadetemple) | | closed with no reward (as his dialogue's comments expect); Gongji shows only after Murod's death, until the party leaves the temple with Qifeng, so the hand-in can be missed (2026-11-10, code and data) |
| 42 | The Family Tradition | Fangyin (Wolong2_v2) | Fangyin | flag `liangshan_mq_placed_coin_locket` | Parrying Dagger, 7,000 XP |
| 43 | Longyue's Hat | Longyue (Wolong2_v2) | Longyue | Longyue's Hat | Well-Honed Katana, 5,000 XP |
| 44 | Secrets of the Dakhanim | Ilaru (KhosaniStrng_v2) | Ilaru, act 4: parchment, mirror, three parts | the items, mirror identified | 10,000 + 25,000 + 100,000 XP, Summoner Platemail |
| 45 | Haenul and the Gemstone of Eraekor | Athodi the Priestess (v4) | Athodi | Eraekor Gemstone | Bone Jerkin, Tome Page 5, 30,000 XP |
| 46 | Child of the Spirits | Piaobo (Liangshan) | the box into the pit (WolongCaverns), Piaobo (Wolong2_v2) | | 1,000 XP; Ring of Control, 5,000 XP |
| 47 | The Tome of Gahnis | the heretic monk (IonaEXT) | Iodir the Heretic (IonaExt02_v2) | | both ends, separate runs: Tome of Gahnis; Thermal Medallion, 10,000 XP |
| 48 | Pursued by Death | Ahlang (Rand-Orenia01_v3) | Ahlang | Tree Bark | 2,000 XP |
| 49 | Durgan's Lucky Charm | Durgan the Sailor (lenele1d) | Durgan (act 1; act 4 in lenele3d) | Durgan's Earring | 50 XP; act 4: Breaching Claymore, 100,000 XP |
| 50 | Curse of the Crone of Savheo | Elioma the Priestess (v4) | Elioma, after the crone | Stone Rose | in story order across both levels: 15,000 + 35,000 + 35,000 XP on the way, then Battle Torque, 65,000 XP |
| 51 | The Mystery of Luleva | Athana of Sidaeh (v4) | Athana | | 15,000 XP |
| 52 | The Ohmedilosi | Kiva the Priestess (v4), once she has seen the map | Kiva | map identified, three carvings | Vengeful Katana, 60,000 XP |
| 53 | Miala the Iomani | Sister Miala (IonaEXT) | Miala | Iomani Necklace | Garnet, 800 XP |
| 54 | Qelah of the Muthavi | Sister Qelah (IonaEXT) | Sister Llio | | Ring of Proficiency, 900 XP |
| 55 | Tathal the Wanderer | Sister Bilan of Nahul (IonaEXT) | Tathal (Rand-Desert_v3) | | both ends: Nahul Scroll; Blue Dragon Scale, 6,000 XP |
| 56 | Wid the Terrified | Brother Wid (IonaEXT) | Wid | axe, identified | Ring of Health, 700 XP |
| 57 | Shards of the Ghimaadi | Varyssa (lenele1b) | Varyssa | the three glass shards | Shard Sword |
| 58 | The Statue of Gaodi II | Caishi the Sculptor (Wolong2_v2) | Caishi, after the five parts | head, arms, legs | 1,000 XP for the head, 2,000 for each limb, then Anti-Magus Torque, 3,000 XP |
| 59 | The Sword of Jarl | Jarl the Smith (lenele1aa) | Jarl | Jarl's Sword | Medevan Hachet, 500 XP |
| 60 | Ragneli's Robbers | The Great Ragneli (lenele1d) | Ragneli | Zane's Bow | 500 XP, 10 gold |
| 61 | Torva's Magic Lute | Torva (lenele1aa) | Torva | Torva's Lute | 500 XP, **125,000 gold** (below) |
| 62 | Cerval's Game | Mercer the Tailor (lenele1aa) | Mercer | Silk Garments | Silk Breeches, 500 XP |
| 63 | Lady Ivane's Mystery | Lady Ivane (lpalaceint_v2) | Ivane | flag `ivane_unhide_evien` | 1,000 XP |
| 64 | Crazy Iven | Crazy Iven (lenele1d) | none in the data | | opened; nothing closes it |
| 65 | Pomporo's Heirloom | Pomporo (lenele1ab) | Pomporo | Signet Ring | Berserker Sword, 600 XP |
| 66 | Nandi the Gatherer | Nandi (Wolong2_v2) | Nandi, after three gifts in turn | Sparrow's Egg, Ghimaadi's Cloak, Bane's Kiss | Nandi's Potion |
| 67 | The Ghost of Ikaemos | Munorus (IkaemosTopInt) | Munorus | Vaesura's Necklace | 1,000 XP |
| 68 | The Lord Chamberlain's Men | Torvel (lenele1ab) | Torvel's double | | closed 2026-11-10 by the double after the full lantern path (spec `torvel2`): 2,000 XP, 5,000 gold (on the way: Stonefire Lanterns and 2,000 XP from Krys, 500 XP from the Master of Revels). The double is hidden until the lanterns are given (`ls_lenele1ab_init`). The earlier "paid at once" (G312) was the test command reaching the hidden double: withdrawn |
| 69 | Elodach the Initiate | Jerve the Elder (TempleInt) | Jerve | Urath Feather, two Madog flags | Ring of Talent, 700 XP |
| 70 | The Seeds of Aahur | Beelon (lenele1d) | Elarh the Merchant (KhosaniStrng_v2) | | both ends: the seeds; 400 XP, 500 gold after bargaining |
| 71 | The Seeds of Aahur, Part 2 | Elarh (v4) | Elarh | Dust of Aahur | 16,000 XP, 50,000 gold (Elarh is there only once part 1 has set `buy_seeds`) |
| 72 | Aesik's Sword | Aesik (masad) | Earis (lenele1aa) | | Bastard Sword, 100 XP |
| 73 | The Encyclopedia of Heresies | Zefnat the Scholar (lenele1ab) | Zefnat | Volumes 28 and 67 | Garnet, 1,250 XP |
| 75 | Novice Sama | Brother Eamon (IonaEXT) | Sama, after her book goes to Eamon | | 200 gold (from Eamon) |

Not in the table: 20 Bleargh ("Pick up a spoon", invalidated by `test_end_flag`), 26 Find a blue shard for Varyssa
(a journal text, but no dialogue opens it), 74 Ajaymes the Pirate (only in `Miniquests_dlg.tbl` and a commented-out
`qjournal.tbl` entry). **Inferred** from the tables: none of the three can appear in the journal.

## Findings

- **Quest items must be identified.** `+Unhide by item` counts an item through `party_count_item_by_name`
  (`0x4ce9a0`), which skips items whose type is not identified (`item_is_identified`, `0x44b690`: bit 0 of
  `ItemType+0x5c`). Seven quest items are flagged "not identified" in `items.tbl`: the Axe of Ikus, the Horn of
  Vadagar, the Khosani Map, the Dakhanim Mirror, the Nhuvasarim Tome and its pages. Holding one unidentified, the
  hand-over topic did not appear (Wid, Saato, Kiva: **Observed**); marked identified, each quest closed. A player
  identifies items with the Lore skill ("Identifies items."); the code path of that skill was not traced (**Unknown**).
  The identified state is saved per item type (the ITEM chunk's bit array, `savegame_read_items`).
- **Torva pays 125,000 gold** for his lute in act 1 (`+AdjustGP: 125000`, "All the gold I have is yours"), more than
  any other quest before act 4. The game does as the table says (**Observed**); whether the number is a slip is
  **Unknown**.
- **Adeslani's escort works, if the party stays behind her** (**Observed** 2026-11-09). After "Yes, we'll help." she
  walks from waypoint to waypoint, at each one waiting until the party is within 3 units (`trigger 3`). At the plaza
  she sends three soldiers in (`pass go`); `ls_lenele2ab_frame` moves her on once they are dead
  (`lenele2ab_flag_escort_1`), and at `$adel2-10` it unhides "We made it?" (`lenele2ab_flag_escort_complete`).
  On 2026-11-08 the test put the party on top of her (`npcgoto`) every few seconds and she stopped for good at
  (-98, 188.7): the party blocked her path. The soldiers were still hidden there, and killed that way they only
  reach "dying". With the party kept 2 units behind her (the tool's `follow` step) she reached the plaza, the soldiers
  appeared and were killed, and she walked on to the house; the escort took 45 rounds of 3 s.
- **Torvel's double.** `lenele1ab` holds two characters named Torvel: `Torvel` (opens the quest) and `Torvel#part2`
  (closes it: 2,000 XP, 5,000 gold). In act 1's script the first is hidden; in the act 2 and 3 scripts both stand
  there. Talking to "Torvel" in acts 1 and 2 reached the double, who paid at once with the quest never opened
  (**Observed**, four runs). A comment in the developer copy places the double near the theatre only after the quest
  (`PLACEMENT TRIGGER: +Unhide NPC: "Torvel#part2"`), so the double looks misplaced in the shipped scripts
  (**Inferred**). Whether a player standing at the real Torvel reaches him first is **Unknown**.
  **Answered 2026-11-09 (Observed):** the two stand 0.8 units apart; from behind the first Torvel he answers and
  opens the quest, and the double then pays at once with no step done. Now bug [G312](game-bugs.md).
  **Corrected the same evening (Observed):** all of this went through `talkto`, which reached hidden characters. The
  game hides the double itself: `ls_lenele1ab_init` disables `Torvel#part2` until `lpalaceint_mq_gave_lanterns` is set
  and the first Torvel after (in act 1 both are hidden). In game the double had flag `0x2` set, was not drawn, and the
  real mouse never put him under the cursor. G312 is withdrawn; the shipped placement is the designers' plan.
- **Cross-character unlocks.** Many topics are unlocked from another character's dialogue (`+Unhide topic: {...}
  From: "..."`): the apothecary's barter opens only after the beggar's story, Sama's hand-over only through Brother
  Eamon. Uhika's five quests open one after the other. A quest tested on its own can look stuck for this reason.
- **The Emperor's Dragon Scales** closes at Gongji with nothing taken or given; the comments in his dialogue ask how
  to take scales the game cannot name, and suggest scrapping him (**Observed**).
