---
title: "Combat: melee and spell damage"
parent: Sum.exe
nav_order: 6.25
---

# Combat: melee and spell damage

**Summary:** `combat_melee_damage` (`0x464f50`) computes the damage of one melee hit. It takes the weapon's
damage and material, the attacker's level, a random spread, a party-size modifier and per-attack-type
multipliers, then subtracts the target's protection. Read from the disassembly (**observed**); not yet checked
against in-game numbers.

The function is a `thiscall` on an attack record: `+0x00` attacker (`Character*`), `+0x08` pointer to the
target, `+0x20` a bonus count, `+0x3c` the attack or skill id (see [Skills](characters.md#skills)).

## Formula

1. **Evade:** if the target has status 8 and `frand() < 0.25`, the damage is 0. Status 8 has no floating
   text, see [Status effects](characters.md#status-effects).
2. **Protection:** `p = character_get_protection(target) x 0.5`, at most 0.75.
3. **Base damage:**
   - with a weapon (equipment slot 5): `B = weapon.damage x 0.1`, plus `M = materials[weapon.material].damage_mod`.
     The damage type and elemental damage come from the weapon (`+0x8c`, `+0x90`);
   - without one: `B = char_info.damage x 0.1`, `M = 0`.
4. **Power:** if the attacker has status 6 (Power), `B` is doubled.
5. **Level:** `D = (B + M) x (1 + 0.5 x attacker level)`.
6. **Party size:** `n = party_count_active()`. Then `v` is 0 for fewer than 3 members, and 0.2, 0.4 or 0.6 for 3,
   4 or 5. The value is negated when the attacker is a party member (type 7). So a large party deals slightly less
   damage per hit, and monsters deal more to it.
7. **Spread:** `D = D + D x (frand() - 0.5 + v) x 0.6`. Species `0x91` doubles the result.
8. **Attack type** (`+0x3c`), for example:
   - `0x12` backstab: `x (skill(backstab) x 10 + 1.2)`, and `x 2.5` more when the record's flag `+0x50` is set;
   - `0x35`-`0x37` aimed attacks (eyes, arms, legs): `x 0.5`;
   - `0x3d` added blow: `x 0.7`;
   - several chain attacks replace the damage with `level x 4`, `level x 8` or `level x 10`.
9. **Final:** `D x (1 + bonus x 0.1) x (1 - p)`. Here `bonus` is the record's `+0x20`.

After this, a second jump table on the attack type applies the side effects: status effects, life leech
(`0x450c70` with `-0.1` / `0.1` of the damage), mug ("Stole %d gold!") and others.

## Applying damage and healing

`character_add_hp` (`0x450850`, `thiscall (delta, source, ...)`) changes current HP: negative values damage
and positive values heal. Status 16, when active, absorbs damage into its own value first (a shield).
Damage to party members is skipped while `Players_immortal` (console command `immortal`) is set; `Debug_death` is
checked next to it. HP is capped at `character_get_max_stat(0)`, with the excess kept at `Character +0x6e8`.
HP changes go through `character_add_hp` (`0x450850`), which `combat_melee_damage` also uses for the life leech.
At 0 HP it calls `character_kill` (`0x450d90`). `character_kill` records the killer (`+0xa4`),
clears all status effects and sets the dead flag (`0x8` in `+0x320`). The `death` debug option (`Debug_death`) and some
multiplayer and party checks skip that last step (**observed**). Scripts (`script_action_die`), the `health` console
command and the AI die action call it directly.

## Example (from the numbers on Joseph's screen)

Joseph at level 3 with a Short Sword (damage 35, material 13 = iron, damage modifier 0): `B = 3.5`, `M = 0`, so
`D = 3.5 x 2.5 = 8.75` before the spread, party and protection factors. The Character screen
shows the weapon's raw damage (35), not this value.

## Helpers named here

| Address | Name | |
|---|---|---|
| `0x455950` | `character_has_status` | status slot timer is running |
| `0x455a30` | `character_status_value` | strength of an active status |
| `0x434350` | `party_count_active` | party members that exist and are not flagged out |
| `0x5426e0` | `frand` | `rand() / 32768` |

## Spell hits

`living_entity_get_hit_by_spell` (`0x45dea0`) handles a spell hitting a target. Its arguments are the spell index
into [`spells`](game-tables.md#spells) and a source type (`+0x18`; 0 = cast by a character). It then:

1. **Resist chance.** It takes the spell's hit chance (`+0x5c`, with special cases: one species uses level x 2 / 100,
   and source type 8 uses 0.3) and the target's magic resistance (`character_get_resistance(3)`). From those it
   computes a resist value: `1 - (1 - (0.5 + magic resist - power x 0.04)) x hit chance`.
2. **Per-spell handler.** A 108-way switch on the spell index chooses the effect. For example:
   - lightning: `amount = caster level x (1 + energy skill)` dice of size 6 (negative, so damage);
   - heal and vitalize: `amount = caster level` dice of size 12 or 8 (positive, so healing);
   - the status spells (blind, silence, curse, freeze, sleep, ...) share one block that calls `status_effect_add`;
   - resurrect and revive share a block, as do the meteor storms and the death spells.
3. **Roll** (`spell_roll_amount`, `0x47f4d0`):
   - the result is `amount` dice of the spell's `die` size, each `floor(frand() x die + 1)` (fixed `-amount`
     when the die is 0);
   - damage (a negative result) is multiplied by `1 - resistance(spell damage type)` (fire, ice, energy, ... at
     `spell +0x78`) and by `1 - resist`; the `No_resist` console variable skips both;
   - a party-size factor applies as in melee, but smaller: 0.15, 0.30 or 0.45 for 3, 4 or 5 members. Party members
     take more damage, monsters less.
4. **Apply:** `character_add_hp(result)`, then the spell's hit effect.
