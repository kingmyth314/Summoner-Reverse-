---
title: Summons
parent: Sum.exe
nav_order: 6.22
---

# Summons
{: .no_toc }

In the single-player game, Joseph's summons are unlocked by the eight summoning rings, not by
his level. A ring gains "ring power" while it is worn. Its power sets the level of the creature,
and at 60,000 power a ring unlocks its second summon. Only one summon can exist at a time. It
borrows part of Joseph's maximum HP until it dies. When it dies, all summoning waits 30 seconds.
The four dragons are different. Each one plays an in-engine cutscene, casts one big spell, and
costs current HP instead of borrowing max HP. **Observed** in code, not tested in game (the 60,000 threshold was re-checked).

1. TOC
{:toc}

## The skills

Summoning uses the general skill **summon** (`0x1b`). Each creature has its own skill id, `0x24`
(golem) to `0x33` (forest dragon). **Banish summon** (`0x34`) dismisses the current summon. See
[Characters](characters.md#skills) for the full skill list. Each summon skill has a record in
`skill_table` (`0x8b3f88`, 0x34 bytes per record, filled by `init_skill_table` `0x442ba0`).
**Observed:**

| Offset | Meaning for a summon skill |
|---|---|
| `+0x04` | AP cost (`skill_is_available` checks it, `character_perform_spell_cast` pays it) |
| `+0x08` | cooldown in seconds (`player_start_skill_cooldown` multiplies it by 1000); 30 for skill `0x1b` |
| `+0x0c` | HP share (a fraction of max HP; see [Cost](#cost-ap-and-hp)) |
| `+0x18` | spell id: always skill id + 2 (`0x26`..`0x35`), the matching entry of `spells.tbl` |
| `+0x1c` | creature name, shown in the summon pop-up menu |
| `+0x20` | flags: `0x4` = summon skill (`skill_is_summon` `0x446e70`), `0x10` = dragon |

The summon spells are the first entries of the "Skils and monster spells" part of `spells.tbl`
([Game tables](game-tables.md)). They are class `summon` with cost 0 there, so the real AP cost
comes from the skill record. **Observed.** Spell 40 is still named "summon blur" in `spells.tbl`,
while skill `0x26` is "summon Blade of Urath". **Observed.**

## Summonable creatures

`summon_skill_creature_name` (`0x446c40`) maps each skill to a creature name (pointer table at
`0x595b24`), and `summon_find_charinfo` (`0x446c00`) loads that creature's `char_info`.
The ring rules come from `character_can_use_summon` (`0x446e90`), single-player branch.
**Observed**, except the "power ring" column, which is *inferred* from the species switch in
`summon_create` (see [Level](#level-of-the-creature)).

| Skill | Creature | Needs (single player) | AP | HP share | Power ring |
|---|---|---|---|---|---|
| `0x28` | Blackfire Elemental | Ring of Darkness in the party | 10 | 0.2 | darkness |
| `0x2e` | Spirit of Laharah | story flag `got_laharah_spirit` | 50 | 0.8 | darkness |
| `0x2b` | Mist Elemental | Ring of Light | 10 | 0.3 | light |
| `0x2c` | Wraith | Ring of Light and `light_ring_xp` > 60,000 | 30 | 0.5 | light x 0.7 |
| `0x25` | Red Minotaur | Ring of Fire | 15 | 0.2 | fire |
| `0x29` | Blood Elemental | Ring of Fire and `fire_ring_xp` > 60,000 | 30 | 0.4 | fire x 0.7 |
| `0x24` | Golem | Ring of Stone | 7 | 0.1 | stone |
| `0x26` | Blade of Urath | Ring of Stone and `stone_ring_xp` > 60,000 | 25 | 0.5 | stone x 0.7 |
| `0x27` | Blue Imp | `got_ring_of_water` or Ring of Water | 15 | 0.3 | water |
| `0x31` | Water Dragon | Ring of Water and `water_ring_xp` > 60,000 | 50 | 0 | water |
| `0x2a` | Poison Elemental | `got_ring_of_forest` or Ring of Forest | 20 | 0.3 | forest |
| `0x33` | Forest Dragon | Ring of Forest and `forest_ring_xp` > 60,000 | 30 | 0.5 | forest |
| `0x2f` | Celestial Samurai | `got_ring_of_wind` or Ring of Four Winds | 30 | 0.5 | wind |
| `0x32` | Four Winds Dragon | Ring of Four Winds and `wind_ring_xp` > 60,000 | 30 | 0.5 | wind |
| `0x2d` | Jade Golem | `got_ring_of_jade` or Ring of Jade | 14 | 0.6 | jade |
| `0x30` | Jade Dragon | Ring of Jade and `jade_ring_xp` > 60,000 | 30 | 0.8 | jade |

"In the party" means `party_count_item_by_name` (`0x4ce9a0`) finds the ring anywhere in the
party's items. The ring does not have to be worn to allow the summon. **Observed.** It must be
worn to gain power (next section). In the level `templeint2`, Blackfire, Poison, Blade of Urath
and Blood Elemental are always refused. **Observed** (`0x446fa3`).

Where the rings come from is only partly mapped. **Observed** in level scripts: the Ring of Fire
from the Giant Salamanka fight (`ls_khosanilab_salamanka_intro_cs_end` `0x4f9520`), the Ring of
Jade after Murod's death (`ls_jadetemple_murod_death_cs_end` `0x4f0880`). Scripts also show
"Joseph acquired Summon ..." messages for Mist Elemental (`lsf_catacombs_boss_dead` `0x4fe0a0`),
Blue Imp (`lsf_wc_nali_used` `0x4f6df0`) and Spirit of Laharah (`lsf_tiger2_dead` `0x4f0360`).
The other rings are *unknown* here.

## Ring power

`character_add_xp` (`0x44fab0`) gives each ring worn in equipment slots 1 and 2 (`+0x758`,
`+0x75c`) a share of every experience award, through `ring_add_xp` (`0x44ffc0`). **Observed:**

- gain = experience x 2 x summon skill (the float value, so summon level 1 = 0.10 gives 20%);
- the new total is capped at twice the character's total experience (`+0x604` x 2);
- the total is a savegame int named after the ring: `dark_ring_xp`, `light_ring_xp`,
  `fire_ring_xp`, `stone_ring_xp`, `water_ring_xp`, `wind_ring_xp`, `forest_ring_xp`,
  `jade_ring_xp`.

So the summon skill's description, "Speed at which summons gain power", is literal.
`ring_check_summon_unlock` (`0x450220`) shows "Joseph acquired Summon Wraith!" (and the Blood
Elemental, Blade of Urath and four dragon versions) when a total crosses 60,000 (`0xea60`).
The dragon messages also need the ring in the party. The Ring of Darkness has no second
summon. **Observed.**

The item pop-up for a ring comes from `ring_format_info` (`0x44b7a0`): "Summoning Ring", "Ring
Power: " with the total, and "Available Summons:" with the creatures unlocked so far. The Ring
of Darkness lists Spirit of Laharah once `got_laharah_spirit` is set. **Observed.**

## When summoning is allowed

`skill_is_available` (`0x4291d0`) and `player_skill_unlocked` (`0x4339f0`) add story gates in
single player (`game_debug_flags` bit `0x1`). **Observed:**

- no summon skill before `got_ring_of_darkness`;
- after `act2_done`, none until `got_ring_of_water`;
- none after `mach_killed` (`skill_is_available` only).

`skill_is_available` also needs current AP >= the skill's cost, a summon skill above 0, and the
summon cooldown timer (`Player +0x94`, that is `+0x28 + 4 x 0x1b`) to have run out. Then it
calls `character_can_use_summon`. `skill_is_usable` (`0x4290a0`) returns 0 for every summon skill while
`cur_summon` (`0x5fcc8c`) is set, and in towns (`level_is_town`). So there is one summon at a
time. **Observed.** *Inferred* story reading: the `act2_done` gate is the part of the story
where Joseph has lost his rings.

In multiplayer (`game_debug_flags` bit `0x2`) the rings do not matter. A summon is allowed when
the caster's level (`+0x600`) reaches a fixed value per skill: Blackfire 1, Mist 4, Blue Imp 8,
Red Minotaur 12, Golem 16, Samurai 20, Poison 24, Jade Golem 28, Blade of Urath 32, Blood 36,
Wraith 40, Laharah 44. Dragons are never allowed. **Observed** (jump table `0x44739c`). The game
also allows only one summon per game, through the SMN packets and `mp_summon_try_claim`
(`0x4b0750`); see [Multiplayer](multiplayer.md).

The `All_summons` cheat skips the ring, flag and level checks. **Observed.**

## Casting

1. The skills panel opens a pop-up of the allowed creatures (`skills_build_summon_menu`
   `0x428980`, at most 16 entries). The pick goes through `skill_find_by_display_name` to
   `character_use_skill` (`0x429410`). Hot buttons call it too (`hud_use_hot_button`).
2. `character_use_skill` does not take AP for summons (byte table `0x4297d8` is 0 for
   `0x24`..`0x33`). It sets `hud_target_mode = 1` and stores the spell id in
   `ui_pending_use_skill`, so the player picks a spot on the ground. **Observed.**
3. The cast runs as a spell. `character_perform_spell_cast` (`0x45da60`) pays the AP of
   `skill_table[spell - 2]` for spells `0x26`..`0x35` when `Use_spell_ap` is on (default 1).
   `spell_cast_execute` (`0x47c5d0`) finds the summon skill whose `+0x18` is the spell and
   calls `summon_create`. **Observed.**

## `summon_create`

`summon_create` (`0x47e900`) takes the caster, the target point, a flag, the spell id and an
existing handle (-1 = create a new creature). **Observed** steps:

1. Work out the creature's level (next section) and create it with `living_entity_create`.
   For the dragons, the spawn point is moved along the caster-to-target line by 7 (jade),
   13 (water), 35 (four winds) or 14 (forest) units (table `0x59b9f0`).
2. Set `Character +0x320 |= 0x103` (`0x100` marks a summon) and give it the caster's team.
3. Red Minotaur: its weapon (slot 5) is removed and freed. Wraith: its instance alpha byte
   (`+0xb4`) is set to `0x80`, *inferred* half transparent.
4. For a party caster (type 7): allocate a `Player` slot (`summon_player`, `0x5fcc84`) with
   flag `0x2` (summon) and flag `0x4` in solo mode; add the creature to the caster's AI group
   (`ai_group_add_member`); set `cur_summon`.
5. Normal summon: lend `HP share x max HP` from the caster. The amount goes to
   `Player +0x178` and is taken off the caster's base max HP (`+0x5f4`); current HP is cut to
   the new max.
6. Dragon (flag `0x10`): see [Dragons](#dragons).
7. Start the arrival effect (`summon_spawn_effect` `0x48cb40`) using the spell's emerge time.
   With no room for it, the game prints "no room for summon target effect!!" and lets the AI
   play the emerge instead (`ai_begin_emerge`, AI mode 6 SUMMONING, see [AI](ai.md)).

A summon from a non-party caster gets flag `0x40000` when the caster's species is `0x5a` or
`0xc9`. *Unknown* which creatures those are.

## Level of the creature

**Observed** in `summon_create` (`0x47e968`-`0x47ec20`):

| Case | Creature level |
|---|---|
| Single player, party caster | highest L (2 to 29) with ring power >= `xp_for_level(L)`, else 1 |
| Second-tier creature (Wraith, Blood, Blade) | the same with 0.7 x ring power |
| Multiplayer | round(caster level + 5 x summon skill), so + half the summon level |
| `All_summons` on, or a non-party caster | round((summon skill + 0.5) x caster level) |

The ring is chosen by the creature's species (`char_info +0x10`): `0xc0`/`0xcd` darkness,
`0xcc`/`0xb9` light, `0x90`/`0xcb` fire, `0x4`/`0xb0` stone, `0xa4`/`0xb5` water,
`0xce`/`0xb6` wind, `0xca`/`0xc3` forest, `0x9`/`0xc2` jade. The dragon species (`0xb5`,
`0xb6`, `0xc2`, `0xc3`) are confirmed by `summon_emerge_update`. The other pairings are
*inferred* from the ring order. The creature's stats then come from its own table at that level
([Characters](characters.md)).

## Cost: AP and HP

- **AP:** paid once, at cast time (table above). **Observed.**
- **Max HP (normal summons):** the share stays lent while the summon lives. When it dies,
  `summon_return_lent_max_hp` (`0x4536c0`) gives `Player +0x178` back to every species-0 party
  member and clears hot-button member 4. **Observed.**
- **Current HP (dragons):** current HP is capped at `(1 - share) x max HP`. Jade Dragon leaves at
  most 20%, Four Winds and Forest 50%, Water Dragon costs nothing. **Observed** (`0x47f054`).
- **Cooldown:** when the summon dies, the death handler starts Joseph's summon timer for 30 s.
  **Observed** (`0x453ece`).

## Behaviour, dismissal and death

The summon follows like a party member: it is in Joseph's AI group and its `Player` is in
`party_list`. The party state can even select it. See [AI](ai.md) for follow and attack logic.

It ends in these ways (**observed**):

| Cause | Path |
|---|---|
| Banish summon (spell `0x36`) | `spell_cast_execute` case `0x47cdd5` -> `summon_dismiss` (`0x47f270`) |
| More than 50 units from Joseph (not in solo mode) | `ai_think_party_update` (`0x4b75ae`) -> `summon_dismiss` |
| `kill_summon` console command | `party_remove_player` on every non-leader `Player` with flag `0x2` |
| Killed | death handler `character_on_death` (`0x4537f0`): lent HP back, summon `Player` removed, 30 s cooldown |

`summon_dismiss` kills the creature through `ai_class_die`, so it also goes through the death
handler (*inferred*). If **Joseph** dies while a summon is out, the summon leaves the party,
switches to team 3 and type 5, goes idle, and the HUD shows "Summoned creature has turned
against you!". **Observed** (`0x453d87`-`0x453e6c`).

## Dragons

The four dragon skills (`0x30`..`0x33`) have flag `0x10`. **Observed** in `summon_create`:

1. Destroy an effect owned by the caster for this cast, then set flags `0x1820` on the dragon. Bit
   `0x800` keeps it in the world during the cutscene.
2. `cutscene_play(skill_names[skill])`: the cutscene is named after the skill ("summon jade
   dragon" and so on). These are the four `"any"`-level entries of `cutscene.tbl`
   ([Cutscenes](cutscenes.md)).
3. `levelrender_fade_out_start`, then every living entity that is not a party character and not
   in Joseph's group gets flag `0x10` and its AI turned off. `cutscene_cleanup` later removes the
   dragon's effects.
4. Point the game camera at the dragon (`camera_set_focus_override`), and cut Joseph's HP.

The dragon spells have an emerge time of 20 in `spells.tbl`. When the arrival effect has run
out, `summon_emerge_update` (`0x4b4660`) makes the dragon cast its own spell at the target:
Jade Dragon spell 104 (fire), Water 105 (ice), Four Winds 106 (energy), Forest 107. **Observed.**
What removes the dragon afterwards is *unknown*.

## Cutscenes, levels and saving

- **Story cutscenes** stash the summon: `summon_stash_for_cutscene` (`0x448000`) saves it in
  `stashed_summon` (`0x8b3c34`) and `summon_restore_after_cutscene` (`0x448500`) puts it back.
  Details are on [Cutscenes](cutscenes.md).
- **Level change:** `level_unload` -> `party_level_reset` (`0x437e50`) sets `cur_summon` and
  `summon_player` to 0. **Observed.** That the creature is not carried to the next level is
  *inferred*.
- **Saving** is refused while the summon is still emerging (`savegame_location_allowed`
  `0x4d4a50` returns 3, "during summon"). A save stores the summon's species and name in the
  LENT party state, and loading recreates it ([Savegame](savegame.md)). **Observed.**

## Console commands

| Command | Effect |
|---|---|
| `All_summons` | toggle; skips ring/flag/level checks and uses the skill-and-level formula |
| `summon golem` / `summon blackfire` | `cmd_summon` (`0x47f730`): `summon_create` 4 units in front of the current character; any other name prints "Unknown summon type" |
| `kill_summon` | removes the friendly summon |
| `Spell_ap` | toggles `Use_spell_ap`, so summons cost no AP |
| `max_skills` | skips skill cooldowns, including the summon timer |

See [Console](console.md) for the full list.

## Open questions

- Where the Rings of Darkness, Light, Stone, Water, Forest and Four Winds are given.
- What happens to a dragon after its spell, and whether it ever counts as `cur_summon` for long.
- Which creatures species `0x5a` and `0xc9` are.

## Evidence

These are function and instruction addresses read with `tools/asm.mjs`, not REA evidence IDs.
Skill record values were read from `init_skill_table`; spell data from `spells.tbl`, listed with
`tools/vpp.mjs` (not committed).

| Claim | Where |
|---|---|
| Ring and flag rules, `templeint2`, MP levels | `character_can_use_summon` `0x446e90`, jump table `0x44739c` |
| Skill records (AP, share, spell, flags) | `init_skill_table` `0x44378d`-`0x443db5` (records `0x8b46d8`..`0x8b4a38`) |
| Story gates | `skill_is_available` `0x42920c`-`0x42926e`, `player_skill_unlocked` `0x433a3d` |
| One at a time, no towns | `skill_is_usable` `0x4290d5`-`0x429170` |
| AP paid at cast | `character_use_skill` `0x42948f`, `character_perform_spell_cast` `0x45dcd3` |
| Level formulas | `summon_create` `0x47e968`-`0x47ec20` |
| HP lend / dragon HP cap | `summon_create` `0x47f090`-`0x47f103`, `0x47f054`-`0x47f085` |
| Dragon cutscene and freeze | `summon_create` `0x47ef3e`-`0x47efd1` |
| Dragon spells | `summon_emerge_update` `0x4b47ff`-`0x4b4899`, table `0x4b4908` |
| Ring power | `character_add_xp` `0x44fb6b`-`0x44fc12`, `ring_add_xp` `0x4501b9` |
| 60,000 unlock messages | `ring_check_summon_unlock` `0x450226` |
| 50-unit leash | `ai_think_party_update` `0x4b7559`-`0x4b75d3` |
| Death, hostile summon, cooldown | `character_on_death` `0x4537f0` (`0x453a06`, `0x453d87`-`0x453ece`) |
| Cooldown length | `player_start_skill_cooldown` `0x433ef0` |
| Console | `cmd_summon` `0x47f730`, `cmd_kill_summon` `0x463140` |
