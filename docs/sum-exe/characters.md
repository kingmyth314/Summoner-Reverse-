---
title: "Characters, experience and levels"
parent: Sum.exe
nav_order: 6.2
---

# Characters, experience and levels

**Summary:** the selected party member is `cur_character` (`0x5fcc88`), a pointer to a character
object. That object holds the level, experience, HP and AP at `+0x5f4`..`+0x607`, and ten equipped-item
pointers at `+0x754`. Experience thresholds come from `xp_level_table` (`0x2305420`). Maximum HP and AP are
computed: base value plus equipment bonuses. The layout is `Character` in `symbols/types.h`.

## Evidence

- A runtime dump made in agent mode (2026-10-13), with Joseph at level 3. The Character screen showed
  HP 128/128, AP 26/26, 200 experience and 450 to the next level.
  `cur_character` was `0x9086c8`; the values were found at the offsets below.
- Disassembly of `character_add_xp` (`0x44fab0`), `character_get_max_stat` (`0x462840`),
  `character_equip_bonus` (`0x4627c0`), `xp_for_level` (`0x489a60`) and `party_level_reset`
  (`0x437e50`).
- Disassembly of `init_skill_table` (`0x442ba0`), `characterinfo_parse_table` (`0x4452f0`) and
  `character_fill_locomotion_anims` (`0x46af70`), and the retail `tables.vpp` (2026-10-15).

## Character fields

| Offset | Type | Field | How it is known |
|---|---|---|---|
| `+0x00c` | int | type: 7 for player party members; 6, 8 and 10 are the dead forms of 5, 7 and 9 | `character_add_xp` (type 7), `character_resurrect` (`0x454070`) |
| `+0x010` | float[3] | world position | |
| `+0x098` / `+0x2f0` | int / mask | team, and the mask of teams it fights (defaults at `0x581904`) | |
| `+0x324` | ai_state | AI sub-object: mode (`AI_STATE_*`, names from the game's table at `0x59ad60`: inactive, idle, traveling, combat, cast spell, patrol, summoning, banishing, disabled, hiding, sneaking, mount travel, mount idle), previous mode, timer, owner, travel destination `+0x54`, target handle `+0x60`, five goal slots `+0x8c`, current goal `+0x190`, path `+0x210` | `ai_set_mode` (`0x473320`) |
| `+0x69c` / `+0x6bc` | float[3] / float | velocity, speed scale | |
| `+0x6cc` / `+0x6d0` | int / pointer | current animation, and the current move record (a `move_info`, see [Game tables](game-tables.md#character-info-and-moves)) | `character_play_move` (`0x460f20`) stores the move at `0x460ff6` and its start time at `+0x9c` (`0x461005`) |
| `+0x6e0` / `+0x6e4` | handle / flags | linked mount or rider; flags bit 0 rider, bit 1 mount | `character_get_mount`, `rider_unlink_mount` |
| `+0x6f8` | char[16] | floating text over the character (colour `+0x709`, scale `+0x70c`) | `0x461fe0`, used for "%d AP" |
| `+0x71c` | char_info* | the character's record from the character tables | `statscreen_unload_anim` reads its species at `[+0x71c]+0x10` (`0x449106`) |
| `+0x080` | handle | the object's handle | written by `obj_create`; `player_from_handle(handle)` returns the player record |
| `+0x2f4` | char[] | name | argument of `"%s gained level %d!"` |
| `+0x320` | flags | `0x80` = dead | `party_level_reset` revives characters with this flag set |
| `+0x5ac`-`+0x5f0` | int[18] | locomotion animation ids, in the order of `locomotion_anim_names` (`0x599648`): stand, norm_walk, norm_run, ready, walk, run, stand_hurt, norm_walk_hurt, norm_run_hurt, ready_hurt, walk_hurt, run_hurt, sneak_ready, hide_ready, tripped_liedown, cloaked_ready, cloaked_sneak, statscreen (`+0x5f0`) | `character_fill_locomotion_anims` (`0x46af70`) fills each with `charinfo_find_anim`; the dump values 31, 30, 29, 26 (first four slots) are animation indices |
| `+0x5f4` | float | base max HP (without equipment) | written from the level table; read by `character_get_max_stat(0)` |
| `+0x5f8` | float | current HP | 119 code references; set to max on level-up and restore |
| `+0x5fc` | short | base max AP | `character_get_max_stat(1)` |
| `+0x5fe` | short | current AP | set to max on level-up and restore |
| `+0x600` | short | level | dump (3); `character_add_xp` |
| `+0x604` | int | experience | dump (200); `character_add_xp` adds to it |
| `+0x558` | sub-object | protection helper; its first field points back at the character | `character_get_protection` |
| `+0x614` | byte[] | skill and attribute values by skill id; value = byte x 0.01 | `character_get_skill`: id 2 = speed (11), id 1 = base protection |
| `+0x754` | item*[10] | equipment slots; slots 1 and 2 are rings (they gain experience through `ring_add_xp`), slot 5 (`+0x768`) is the weapon, slots 3, 4, 6, 7 and 8 are armour | `character_equip_bonus`; the dump had a Short Sword in slot 5 and a Tunic in slot 7 |

{: .important }
**Corrected 2026-10-15.** The locomotion row used to read "ready, ready_hurt, walk, run, tripped_liedown"
for `+0x5ac`-`+0x5e4`. The name table at `0x599648` has 18 entries and starts with stand, norm_walk, norm_run
and ready. The last one, statscreen, is `+0x5f0`.

**Locomotion fallbacks (observed).** After the lookups, `character_fill_locomotion_anims` fills a missing
(-1) id from another slot (`0x46afc3`..`0x46b125`):

- stand and ready stand in for each other; so do norm_walk and walk, and norm_run and run;
- stand_hurt takes stand, and ready_hurt takes ready;
- walk_hurt takes norm_walk, norm_walk_hurt takes walk, run_hurt takes norm_run, and norm_run_hurt takes run.

The tables drop every "hurt" move of a non-boss record when they load (see
[Game tables](game-tables.md#character-info-and-moves)). *Inferred:* for every character except the
bosses, all six hurt slots therefore come from these fallbacks.

Maximum HP is `base_max_hp + character_equip_bonus(0)`, with a minimum of 1. Maximum AP is
`base_max_ap + character_equip_bonus(1)`, with a minimum of 0. `character_equip_bonus(stat)` adds up
the modifiers of every equipped item. Each item type has two `{stat, value}` modifiers at `+0x40`.

## Character screen stats

`status_draw` (`0x42c9c0`) draws the screen through helpers: `status_draw_level` (level, experience, bar),
`status_draw_stats` (below), `status_draw_skills` (skill list and chain keys), `status_draw_skill_points`,
`status_draw_resistances` and `status_draw_combat_style`.

`status_draw_stats` (`0x42cff0`) draws the stat panel for `status_character` (`0x5bd810`). Each value
is coloured by its equipment bonus: one colour for a positive bonus, another for a negative one, and
the normal colour for none.

| Shown | Computed as | Joseph at level 3 |
|---|---|---|
| HP, AP | current / `character_get_max_stat` | 128/128, 26/26 |
| Speed | `character_get_skill(2)` x 1000 | skill byte 11 → 0.11 → 110% |
| Damage | weapon item type `+0x68` (slot 5) | Short Sword: 35 |
| Protection | `character_get_protection` x 100 | Tunic 0.07 + level 3 x 0.01 = 0.10 → 10 |

`character_get_skill(id)` (`0x462440`) returns `skill[id] x 0.01`, adjusted by equipment
(`character_skill_equip_mod`, four `{skill, value}` entries at item type `+0x20`). It returns -999999 when
`character_skill_allowed` says the skill is locked (`player_skill_unlocked` on the player record, which checks story flags).

`character_get_protection` (`0x464de0`) works in one of two ways:

- with no armour worn, it returns `get_skill(1) + level x 0.01`;
- otherwise it adds up `materials[type +0x08].protection_mult x type +0xf0` for each worn armour piece (`materials` is the `materials.tbl` table at `0x8b9d4c`).

**Resistances** (`status_draw_resistances`) come from `character_get_resistance(type)` (`0x4626b0`). The types
are 0 fire, 1 ice, 2 energy, 3 magic, then 4-6 slashing, piercing and blunt in the screen's order. This is the
order of `damage_type_names` (`0x598fc4`). Equipment adds
its `+Resist Mod:` entries (`character_resist_equip_bonus`, item type `+0x30`). Magic resistance also
grows with skill `0x23` x 0.5 and level x 0.02, which is Joseph's 6% at level 3.

The record's own resistance levels come from its `+Resistances:` block. `characterinfo_parse_resistances`
(`0x446930`) stores seven level indices at char_info `+0xf8` (default 2, standard). The level names are
weak, vulnerable, standard, strong, invulnerable and strengthen (`0x595cb8`).
`resistance_level_damage_mult` (`0x448c80`) returns `1 - m` for the damage multipliers 2, 1.5, 1, 0.5, 0
and -0.5, so it gives -1, -0.5, 0, 0.5, 1 and 1.5 (**observed**: constants `0x580200`, `0x580210`,
`0x57e6b0`, `0x57eb84`, `0x580df4`, each subtracted from 1.0 at `0x57e990`).

The item type layout is `ItemType` in `symbols/types.h`. It is mapped from the `items.tbl` parser (`items_parse_table`), where each
`$Field:` is stored at a fixed offset: for example kind `+0x04`, material `+0x08`, value `+0x18`, slot `+0x1c`, damage `+0x68`, weapon skill `+0x6c` and protection `+0xf0`.

## Skills

`Character.skill[id]` stores each skill or attribute as hundredths: byte 10 = 0.10, shown as level 1 (value x 10). Speed is shown as value x 1000, so byte 11 = 110%.

`skill_table` (`0x8b3f88`) has **84 records** of 0x34 bytes, ids `0x0`..`0x53`. It is filled by
`init_skill_table` (`0x442ba0`), whose last store is `0x8b5094`. The next table, `resistance_level_names`,
starts at `0x8b5098` (**observed**). The enum is `skill_id` in `symbols/types.h`.

Fields of a record (**observed** in `init_skill_table`):

| Offset | Field |
|---|---|
| `+0x00` | localized name (a `localize_strings` slot, L33 "hand to hand" first) |
| `+0x18` | for the summon skills, the summon spell (`0x26`, spell 38 "summon golem", for summon golem) |
| `+0x1c` | second name: the creature for the summon skills (L114 "Golem"), the empty string L34 for the others |
| `+0x20` | flags: bit 2 = summon skill (`skill_is_summon`), bit 3 read by `skill_has_flag8` |
| `+0x2c` | localized description (L35 "Inc. % to hit in hand to hand combat."; L34, empty, for the summon skills) |

The tables name skills through `skill_names` (`0x595b64`): 84 strings, then the count 84 at `0x595cb4`
(**observed**). The character table parser matches `$Skill:` and the four `$Up/$Down/$Left/$Right Skill:`
lines against it (`cmp esi, 0x595cb4` at `0x445c0b`). A missing direction line stores `0x55`, "none"
(`0x445c1b`), which `character_get_chain_skill` reports as "No chain skill configured!". A name that is
not in the list leaves the search at 84 (`0x54`), one past the table. Every skill name in the retail tables
is in the list (checked 2026-10-15).

Joseph at level 3 has non-zero speed, sword weapons, dodge, push, summon and heal, which
matches his Character screen.

{: .important }
**Corrected 2026-10-15.** This section used to say 85 records and listed id `0x54` as "weak". There is
no skill `0x54`: "weak" is the first resistance level name (`0x595cb8`), which follows the skill name
count in memory. `SKILL_WEAK = 0x54` in `symbols/types.h` is not a skill.

| Id | Skill | Id | Skill | Id | Skill |
|---|---|---|---|---|---|
| `0x0` | hand to hand | `0x1c` | heal | `0x38` | blindness stab |
| `0x1` | protection | `0x1d` | dark | `0x39` | arm slice |
| `0x2` | speed | `0x1e` | energy | `0x3a` | leg swipe |
| `0x3` | sword weapons | `0x1f` | holy | `0x3b` | Desperation |
| `0x4` | axe weapons | `0x20` | fire | `0x3c` | stamina burst |
| `0x5` | blunt weapons | `0x21` | ice | `0x3d` | added blow |
| `0x6` | staff weapons | `0x22` | divination | `0x3e` | life leech |
| `0x7` | bow weapons | `0x23` | magic resist | `0x3f` | mind drain |
| `0x8` | heavy arms | `0x24` | summon golem | `0x40` | medevan technique |
| `0x9` | parry | `0x25` | summon red minotaur | `0x41` | icy touch |
| `0xa` | counter attack | `0x26` | summon Blade of Urath | `0x42` | burning hands |
| `0xb` | dodge | `0x27` | summon blue imp | `0x43` | shocking grasp |
| `0xc` | critical hit | `0x28` | summon blackfire elemental | `0x44` | sacrifice |
| `0xd` | double attack | `0x29` | summon blood elemental | `0x45` | confusion |
| `0xe` | push | `0x2a` | summon poison elemental | `0x46` | silence |
| `0xf` | kick | `0x2b` | summon mist elemental | `0x47` | axespin |
| `0x10` | trip | `0x2c` | summon wraith | `0x48` | earthshaker |
| `0x11` | aimed attack | `0x2d` | summon jade golem | `0x49` | mend |
| `0x12` | backstab | `0x2e` | summon laharah | `0x4a` | steelskin |
| `0x13` | assess | `0x2f` | summon samurai | `0x4b` | overpower |
| `0x14` | appraise | `0x30` | summon jade dragon | `0x4c` | bless |
| `0x15` | pick lock | `0x31` | summon water dragon | `0x4d` | revitalize |
| `0x16` | sneak | `0x32` | summon wind dragon | `0x4e` | prayer |
| `0x17` | hide | `0x33` | summon forest dragon | `0x4f` | mug |
| `0x18` | forge | `0x34` | banish summon | `0x50` | serpent whip |
| `0x19` | mine | `0x35` | aimed attack eyes | `0x51` | serpent whip trip |
| `0x1a` | enchant | `0x36` | aimed attack arms | `0x52` | serpent whip push |
| `0x1b` | summon | `0x37` | aimed attack legs | `0x53` | serpent whip multi |

### Chain skills

`chain_skill_table` (`0x8b3c68`) has 40 records of 0x14 bytes, ten per hero, filled by
`init_chain_skill_names` (`0x4425c0`). **Observed** fields: `+0x00` localized name ("Chain Added Blow" ...),
`+0x04` species, `+0x08` skill, `+0x0c` chain slot (0 to 3), `+0x10` byte: the chain points needed.
Slot 0 is "added blow" (skill `0x3d`, threshold 0). Slots 1 to 3 hold three skills each, learned at 0, 10
and 30 points (the threshold bytes are 0 sixteen times, 10 and 30 twelve times each). The points per slot are the persistent variables `"chainpts<species><slot>"` (0 to 100);
the chosen chain per direction is `"chainconfig<species><slot>"` (`chain_points_set` `0x449290`,
`chain_config_set` `0x449200`).

**Observed** in the code of `character_learn_chain_skill` (`0x449650`), not tested in game: a landed chain
costs AP, and with chance 0.13 gives the slot a point. The `Quick_combo` console toggle makes it always
give 10 points instead of 1. Crossing a threshold shows "%s learned %s!".

## Status effects

`Character +0xac` holds 18 status slots of 0x20 bytes, indexed by status id. Each slot has a state at `+0` and
`GameTimer`s at `+4` and `+0x14`. `status_effect_add` (`0x454700`) checks immunities, sets the slot and shows
floating text. `status_effect_cure` (`0x4553a0`) clears it ("-Silence" ...).

| Id | Effect | Id | Effect |
|---|---|---|---|
| 0 | Blind | 6 | Power |
| 1 | Silence | 9 | Sleep |
| 2 | Faith | 10 | Paralyze |
| 3 | Curse | 11 | Poison |
| 4 | Bless | 12 | Cripple |
| 5 | Regen | 13 | Slow |

Ids 7, 8 and 14-16 exist (the maximum is 0x10) but show no text.

## Player record

Each party member also has a `Player` record (`player.cpp`), kept in the circular list `party_list` (`0x5fc350`).
`player_from_handle` (`0x433eb0`) finds it from the character's object handle. The record was confirmed in a runtime
dump of Joseph's record at `0x5fcaf8` (2026-10-14).

| Offset | Field | Joseph |
|---|---|---|
| `+0x000` / `+0x004` | next / prev in `party_list` | |
| `+0x008` | index of the member's record in `char_infos` | (read by `statscreen_unload_anim` at `0x4491c4`) |
| `+0x00c` | character object handle | `0x2658d` (= `Character +0x80`) |
| `+0x020` / `+0x024` | count and pointer of the skill progression: `{skill, level, value}` entries | 17 entries at `0x593508` |
| `+0x17c` | **unspent skill points** | 4 (matches the Character screen) |
| `+0x180` | set on level-up from the level table | 3 |
| (`+0x190`) | not a field: the record is 0x188 bytes ([Party](party.md#records-and-lists)). For Joseph's record, the last of the five, `+0x190` lands on `cur_character` (`0x5fcc88`), which held his `Character` | `0x9086c8` |

On a level-up, `character_add_xp` adds the level table's skill points to `+0x17c`. For each progression
entry whose level equals the new level, it also sets `Character.skill[skill] = value`.

## Objects and handles

`obj_create` (`0x469c40`) takes a 12-byte slot from `obj_free_list` and links it into `obj_used_list`. The
slots are in `obj_slots` (`0x9438b0`). It stores the handle (slot index + a running base) at object `+0x80`,
and a network handle at `+0x8c`. `obj_from_handle` (`0x46b130`) reverses this. `living_from_handle`
(`0x46b1a0`) also checks that the object is a living entity.

## Experience and levels

`character_add_xp` (`thiscall`, ECX = character) is called by the console command `add_xp`. In order, it:

1. stops at the level cap, `xp_max_level` (50, or 75 with debug flag 2);
2. adds the experience to `+0x604`;
3. gives a share of the experience to the rings in equipment slots 1 and 2 (`ring_add_xp`, `0x44ffc0`), which counts it per ring (`dark_ring_xp`, `fire_ring_xp`, ...);
4. checks `xp_level_reached(level, xp)`, which means `xp >= xp_for_level(level + 1)`;
5. on a level-up, calls `0x489ac0` with the class. That call updates the level, base max
   HP, base max AP and the skill points;
6. sets current HP and AP to the new maximum;
7. for party members, applies the player record's skill progression and shows "+Level!".

`xp_level_table` has 0x18-byte records indexed by level. The first field is the experience needed:
level 2 = 100, 3 = 200, 4 = 450 (confirmed in game). Beyond level 20, each level adds 50,000, then
60,000, and so on. Debug flag 2 multiplies the thresholds by 5.

The rest of each record (`xp_level` in `symbols/types.h`, read by `experience_parse_level`) is:
- the skill points gained at that level (2);
- the level number;
- four 4-byte groups, one per species (`char_info +0x10`), with the HP and AP gained.

For Joseph's group, level 1 gives 64 HP and 5 AP, and levels 2 and 3 each add 32 HP and 3 AP. That makes
128 HP at level 3, and 2 + 2 = 4 skill points, both as the Character screen shows.

The heroes are species 0 to 3. `species_is_hero` (`0x449a20`) is `species <= 3`; every id in
`species_table` (`0x580e60`, 223 entries) is between 0 and 225 (**observed**).

## Open questions

- The rest of the stat panel after protection (`0x42d58f` onwards).
- Whether the crossed hurt fallbacks (walk_hurt from norm_walk, norm_walk_hurt from walk) are meant.
  Which locomotion slot the game picks for a hurt character was not traced.

## Changes (2026-10-15)

- Skills: 84 records (`0x0`..`0x53`), not 85; "weak" removed from the id table. Added the record
  fields, the `skill_names` list and what the parser stores for a missing (`0x55`) or unknown (`0x54`) name.
- Locomotion ids: 18 slots `+0x5ac`..`+0x5f0` in the order of `locomotion_anim_names`, with the fallbacks.
  The old row named the wrong slots.
- Added `+0x71c` (char_info pointer), Player `+0x008`, the char_info resistance levels and
  `resistance_level_damage_mult` (it returns 1 minus the multiplier), and a short chain skill section.
