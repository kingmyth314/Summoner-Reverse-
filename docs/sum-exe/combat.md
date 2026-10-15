---
title: "Combat: melee and spell damage"
parent: Sum.exe
nav_order: 6.25
---

# Combat: melee and spell damage

**Summary:** `combat_melee_damage` (`0x464f50`) computes the damage of one melee hit. It takes the weapon's
damage and material, the attacker's level, a random spread, a party-size modifier and per-attack-type
multipliers, and subtracts the target's protection. A second step then applies the skill's side effects, the
target's resistances, a position bonus, curse and critical hits. Read from the disassembly (**observed**); not
yet checked against in-game numbers.

The function is a `thiscall` on the attacker's combat record (`living_combat`): `+0x00` attacker
(`Character*`), `+0x08` the opponent's combat record (its `+0x00` is the target `Character`), `+0x20` the
chain count, `+0x3c` the attack or skill id (see [Skills](characters.md#skills)). A normal attack has id
`0x55` ("none"). Its stack argument is not read (all three callers in `combat_apply_melee_hit` pass 0).

## Formula

1. **Evade:** if the target has status 8 and `frand() < 0.25`, the damage is 0. Status 8 is invisibility
   (**observed**: `damage_type_to_status` (`0x47a8c0`) maps spell 11, invisibility, to it, and
   `character_update_status_effects` fades a character that has it). It has no floating text, see
   [Status effects](characters.md#status-effects).
2. **Protection:** `p = character_get_protection(target) x 0.5`, at most 0.75.
3. **Attack type, first pass** (byte table `0x465d40` for ids `0xa`..`0x55`). Only some ids compute weapon
   damage (steps 4-9). The others set the damage directly:
   - kick (`0xf`): `D = level x 4`, damage type blunt (6);
   - serpent whip multi (`0x53`): `D = level x 8`; serpent whip (`0x50`): `D = level x 10`;
   - push (`0xe`): `D = 0`, and the record's `+0x24` grows by 2;
   - trip, earthshaker, mend, steelskin, overpower, bless, revitalize, prayer, serpent whip trip and push: `D = 0`;
   - every other id, also hand to hand (`0x0`), aimed attack (`0x11`), medevan technique (`0x40`) and weak
     (`0x54`): `D = 0`, with no damage type.

   The ids with weapon damage are counter attack, double attack, backstab, the aimed attacks (eyes, arms,
   legs), blindness stab, arm slice, leg swipe, desperation, stamina burst, added blow, life leech, mind drain,
   icy touch, burning hands, shocking grasp, sacrifice, confusion, silence, axespin, mug and none.
4. **Base damage:**
   - with a weapon (equipment slot 5) and without char_info flag `0x200`: `B = weapon.damage x 0.1`, plus
     `M = materials[weapon.material].damage_mod`. The damage type and elemental damage come from the weapon
     (`+0x8c`, `+0x90`);
   - otherwise: `B = char_info.damage x 0.1`, `M = 0`. The damage type is blunt (6), or slashing (4) for
     species `0xb0`. There is no elemental damage.
5. **Power:** if the attacker has status 6 (Power), `B` is doubled.
6. **Level:** `D = (B + M) x (1 + 0.5 x attacker level)`.
7. **Party size:** `n = party_count_active()`. Then `v` is 0 for fewer than 3 members, and 0.2, 0.4 or 0.6 for 3,
   4 or 5. The value is negated when the attacker is a party member (type 7). So a large party deals slightly less
   damage per hit, and monsters deal more to it.
8. **Spread:** `D = D + D x (frand() - 0.5 + v) x 0.6`. Species `0x91` doubles the result.
9. **Attack type multipliers:**
   - `0x12` backstab: an attacker of type 9 or with flag `0x40` (`Character +0x320`) only doubles `D`. Any
     other attacker multiplies by `skill(backstab) x 10 + 1.2`, then by 2.5 when the record's flag `+0x50` is
     set and by **0.5** when it is not. `living_combat_update_special` sets `+0x50` when the target is not in
     combat mode (`0x467d83`, `0x467d89`);
   - `0x35`-`0x37` aimed attacks (eyes, arms, legs): `x 0.5`;
   - `0x3d` added blow: `x 0.7`.
10. **Chain and protection:** `D x (1 + chain x 0.1) x (1 - p)`. Here `chain` is the record's `+0x20`, the
    number of attacks in the current chain (*inferred* from `chain_attack_check_window`).
11. **Side effects** (second jump table `0x465d8c`, ids `0x38`..`0x4f`). They act on this `D`. The heals,
    AP changes and area hits do not run on a multiplayer client.
    - blindness stab, arm slice, leg swipe: `x 0.3`; silence (`0x46`): `x 0.15`;
    - desperation: `x (1.3 - HP / max HP)`; stamina burst: `x AP / max AP`;
    - icy touch, burning hands, shocking grasp: the elemental type becomes 1, 0 or 2;
    - life leech (`0x3e`): `D` is halved and the attacker heals by it (`character_add_hp(+D)`);
    - mind drain (`0x3f`): the target loses `0.1 x D` AP, the attacker gains `0.1 x D` AP
      (`character_add_ap`, `0x450c70`), and `D = 0`;
    - sacrifice (`0x44`): `D` is doubled and the attacker loses 2/3 of the doubled value as HP;
    - confusion (`0x45`): the target loses `0.25 x D` AP, and `D = 0`;
    - mug (`0x4f`): `D` is halved, and `(int)D` gold goes to `party_gold` ("Stole %d gold!"). The target still
      takes the halved damage;
    - axespin (`0x47`): every living character within 5 units, except the attacker and party members (type 7),
      takes `0.75 x D`. Earthshaker (`0x48`) casts spell `0x3b` (trip) on each instead. Then `D = 0`. The loop
      does not skip the target, so the target is hit through the loop;
    - mend, steelskin, overpower, bless, revitalize, prayer: the attacker casts spell 0 (heal), `0x13` (bless),
      `0x10` (empower), `0x15` (protect), 4 (vitalize) or `0x16` (faith) on itself, source type 6.
12. **Resistance:** `r1 = 1 - resistance(damage type)` and `r2 = 1 - resistance(elemental)`. A type of -1 gives
    resistance 0 (`0x4626bb`). If `r1 < 0` the factor is `r1`; else if `r2 < 0` it is `r2`; else it is
    `r1 x r2`. `D = D x factor`.
13. **Position:** `D = D x (1 + m)`, with `m = combat_position_modifier(attacker, target)` (`0x464d00`), stored at
    the record's `+0x48`. `m` is +0.25 when the attacker is more than 0.25 above the target and -0.25 when it is
    more than 0.25 below. It gains +0.5 when the target is knocked down (AI mode 8 / `0x2d`), else +0.15 when
    the attacker is behind it.
14. **Curse:** if the target has status 3, `D = D x (1 + curse value)`.
15. **Critical hit:** if `character_skill_allowed(0xc)` and `frand() < skill(critical hit)`, `D` is doubled.
16. **Super power:** with the console toggle `Super_power` set and a party member attacking, the result is 200.
17. The function returns `floor(D)`.

## Applying damage and healing

`character_add_hp` (`0x450850`, `thiscall (delta, source, a3, a4)`) changes current HP: negative values damage
and positive values heal. It is **observed** to:

- do nothing to a character with flag `0x80` (returns false), and ignore type 4 (returns true);
- set `Character +0x608` to 1 when the source is a party member (type 7) and to 0 otherwise, on every call
  (`0x4508b7`). `character_on_death` pays experience only when this byte is set, so the last HP change before
  a death decides it ([Party](party.md#experience-sharing));
- in multiplayer, when `a4` is 0, multiply damage from a source with flag `0x40` or of type 9 by 4, and damage
  from any other source that is not a party member by 2 (`0x4508bd`..`0x450922`). Melee and spell hits pass 0;
- keep the change for the floating number (`levelrender_draw_float_text`) at `Character +0x6e8`, with a 2 s
  timer at `+0x6f0`. Changes less than 0.2 s apart add up. A change of exactly 0 starts a "0" only when no
  number is showing (`0x450925`..`0x45095a`); the draw prints `|+0x6e8|` whenever the timer runs (`0x48543a`);
- set flag `0x4000` on damage, then return at once for a change of 0;
- feed a boss's threat table (`boss_threat_add`, type 9), cure sleep (status 9) on damage, send the character
  back to idle from AI mode 8 / `0x31`, and give a sneaking character (flag `0x400`) a sneak goal;
- while status 16 (ice coffin) is active, change no HP at all. Damage with a source lowers the status value
  instead (not below 0). Damage beyond that value is lost, and heals do nothing. The coffin ends when the
  value reaches 0 (`character_update_status_effects`);
- skip all damage while the `death` console option (`Debug_death`, on by default) is off, and damage to party
  members while `Players_immortal` (console command `immortal`) is set. A multiplayer client always applies;
- cap HP at `character_get_max_stat(0)`. A capped heal stores only the HP actually gained at `+0x6e8`.

At 0 HP or below it calls `character_kill(0x22)` (`0x450d90`) and returns the opposite of its result. This
happens after every call that gets that far, also when the ice coffin blocked the change.

`character_kill` (**observed**, `0x450d90`..`0x450e29`):

- stores its argument at `+0xa4`. That is the id of the death move (category 6) that `character_on_death`
  plays later, not the killer. `character_add_hp`, the AI die action (`ai_class_die`) and the `health`
  command pass 0x22; `script_action_die` passes the script's value;
- sets HP to 0 and clears all status effects;
- returns false without setting a flag when, on a single-player game or a multiplayer host, the death option
  is off, or the character is a boss (type 9) in single player. On a multiplayer client these tests are
  skipped;
- returns false for a party member (type 7);
- otherwise sets flag `0x8` in `+0x320` and returns true.

Flag `0x8` marks a character that is dying (the party code also uses it to hide members). The dead flag
`0x80` is set by `character_on_death` (`0x4537f0`, at `0x45393a`), which `ai_frame` calls (`0x472dd6`). It
acts on any character with HP at 0 or below, party members included. It sets `0x8` too (`0x453870`), waits while the
current move's trigger 9 is still ahead, plays the death move, pays experience and turns 5 into 6, 9 into 10
and a summon's 7 into 8. In single player it does nothing for a boss (type 9).

## Example (from the numbers on Joseph's screen)

Joseph at level 3 with a Short Sword (damage 35, material 13 = iron, damage modifier 0): `B = 3.5`, `M = 0`, so
`D = 3.5 x 2.5 = 8.75` before the spread, party, chain, protection and the later factors. The Character screen
shows the weapon's raw damage (35), not this value.

## Helpers named here

| Address | Name | |
|---|---|---|
| `0x455950` | `character_has_status` | status slot timer is running |
| `0x455a30` | `character_status_value` | strength of an active status |
| `0x434350` | `party_count_active` | party members that exist and are not flagged out |
| `0x5426e0` | `frand` | `rand() / 32768` |
| `0x4626b0` | `character_get_resistance` | 0 for type -1 |
| `0x462400` | `character_skill_allowed` | true for non-party characters; party members ask `player_skill_unlocked` |
| `0x450c70` | `character_add_ap` | used by mind drain and confusion |
| `0x464d00` | `combat_position_modifier` | height, knock-down and behind bonus |

## Spell hits

`living_entity_get_hit_by_spell` (`0x45dea0`) handles a spell hitting a target in single player (in
multiplayer, `spell_prepare_cast` and `spell_apply_hit_effects` do this work). It is a `thiscall` on the
target with eight stack arguments: the spell index into [`spells`](game-tables.md#spells) (`[ebp+0x8]`), the
caster's handle, the hit position, the hit orientation, the source type (`[ebp+0x18]`, the fifth; 0 = cast
by a character), the item for item sources, a strength value (`[ebp+0x20]`, used only by leech and conversion, and passed on to the nested hits) and a
"preloaded effect" flag.
**Observed:** it returns without effect

- for a dead target (flag `0x80` or HP <= 0), except for spells 3, 5 and `0x69`;
- when every party member is dead, or when `serpent_part_on_hit` takes the hit (`0x45df5a`);
- when the caster's handle no longer resolves;
- for a target farther than the spell's `+Distance:` (when it is above 0).

Spell `0x48` (Nightmare) applies spells 7, 8 and `0x43` instead. An offensive spell that hits a party member
from a caster not on team 0 turns combat mode on (`combat_mode_set`, `0x45e0d0`). It then:

1. **Power.** A switch on the source type (table `0x460075`) sets the power: the caster's level for 0 and 8,
   100 for 7, values from char_info, an item or an ability for 1 to 5, and for 6 (a combat skill) the level
   divided by 3 (heal), 4 (vitalize) or 2, times `1 + 0.15 x Character +0x578`.
2. **Resist value.** It takes the spell's hit chance (`+0x5c`, with special cases: a caster of species `0xb6` uses
   level x 2 / 100, spell `0x50` in ghost stage 3 uses 0.5, and source type 8 uses 0.3) and the target's magic
   resistance (`character_get_resistance(3)`). From those it computes
   `resist = 1 - (1 - (0.5 + magic resist - power x 0.04)) x hit chance`. A spell with no `+Hit Chance:` line
   has hit chance 1 (the parser's default, `0x47a294`), so its resist value is `0.5 + magic resist - power x 0.04`.
   The handlers that can be resisted test it in two ways (**observed**; `No_resist` skips both):
   - the shared status block resists an **offensive** spell when `frand() <= resist` (`test ah, 0x41` at
     `0x45e9c3`);
   - the other handlers resist when `frand() < resist` (`test ah, 0x1`), whether or not the spell is
     offensive: Failed Dark void's five statuses, leech, conversion, apdrain, pushback, trip, Titus stone gaze,
     power leech, Laharah's scream, the death spells and Life Drain (eleven tests, `0x45eccc`..`0x45fe39`);
   - the sleep part of icy sleep (spell `0x21`) is tested against the target's magic resistance instead, after
     the damage: it lands unless `frand() <= magic resistance` (`0x45ffac`).
3. **Per-spell handler.** A 108-way switch on the spell index chooses the effect. The handlers set a dice
   count and a die for the roll. For example:
   - lightning: `count = power x (1 + energy skill)` for a cast by a character, else `power`; the die is the
     table's 6, **negated** (damage);
   - heal and vitalize: `count = power` dice of the table's size, 12 or 8 (positive, so healing);
   - the status spells (blind, silence, curse, freeze, sleep, ...) share one block that calls `status_effect_add`
     when `spell_target_value` is above 0.01;
   - resurrect and revive share a block, as do the meteor storms and the death spells;
   - Jade Beam (`0x18`) on a target that is not a boss and has no flag `0x40` takes half the target's current
     HP at once (`character_add_hp(-hp / 2, caster, 1, 0)`, `0x45f1c4`..`0x45f209`), with no resist test;
   - the death spells (death, Ghost death, Urath death, Dark void) do nothing to bosses and `0x40`-flagged
     characters. If not resisted they take all current HP, and a target that is not a party member loses its
     equipment slots 0 to 8 (freed, so they do not drop; `0x45fdf0`). A resisted Dark void casts Failed Dark
     void on the target;
   - Life Drain (`0xd`) takes one level from a target above level 1 and rescales its stats; it deals no HP;
   - power leech (`0x11`) takes `AP x power x 0.01` AP (all AP with chance 0.05); the caster gains at most 20;
   - leech (`0x3e`, the special ability of the Blood Elemental, Crystal Rokhul and Wraith) computes
     `strength x 0.05 x power` but sets no dice, so the roll gives 0 and no HP changes (`0x45f471`..`0x45f47d`).
     The multiplayer path keeps the same value (`spell_prepare_cast` `0x47baa6`..`0x47babb`). See
     [Game bugs](game-bugs.md).
4. **Roll** (`spell_roll_amount`, `0x47f4d0`):
   - for `count > 0` the result is `count` dice, each `floor(frand() x die + 1)`. A positive die gives 1 to
     `die`. A negated die gives 0 to `-(die - 1)`, so damage dice average `(die - 1) / 2`, not `(die + 1) / 2`.
     With die 0 the result is `-count` at once, with no resistance or party factor (`0x47f4ec`..`0x47f4fe`);
     with `count <= 0` it is 0;
   - damage (a negative result) on a target is multiplied by `1 - resistance(spell damage type)` (fire, ice,
     energy, ... at `spell +0x78`). A resistance above 1 turns the damage into healing;
   - damage that is still negative is then multiplied by `1 - magic resistance`. This is the target's
     resistance 3, not the resist value of step 2. Some handlers turn this step off. If the product is not
     negative, it becomes 0. `No_resist` skips both multipliers;
   - a party-size factor applies as in melee, but smaller: 0.15, 0.30 or 0.45 for 3, 4 or 5 members. Party members
     (and a missing target) take more damage, others less. Healing gets no party factor.
5. **Apply:** `character_add_hp(result, caster, 0, 0)` for every handler that reaches the end, also with a
   result of 0 (`0x45fecd`); a flinch for damage; the spell's hit effect; and, for the damage spells, the AI's
   retaliation against the caster (`ai_retaliate_on_hit`).

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Evidence |
|---|---|
| Evade | `0x464f7a`..`0x464fb1` |
| Protection cap | `0x464fb4`..`0x464fd3` |
| First attack-type pass | `0x464fe3`..`0x464ffd`; byte table `0x465d40`, targets `0x465d24` |
| Weapon, flag `0x200`, unarmed type | `0x465004`..`0x465099` (`test byte [eax+0x259], 2` at `0x465016`) |
| Power, level, party, spread | `0x4650a1`..`0x46515a` |
| Backstab halved without `+0x50` | `je 0x4651b1` at `0x465193`; `fmul 0.5` at `0x4651b1` |
| Kick, serpent whips, push, zero cases | `0x4651c9`, `0x465222`, `0x465206`, `0x4651f3`, `0x4651e9` |
| Chain and protection | `0x46523c`..`0x465266` |
| Side effects | jump table `0x465d8c`; life leech `0x4652af`..`0x4652eb`; mind drain `0x4652f0`..`0x465336`; sacrifice `0x465343`..`0x465384`; confusion `0x46539c`..`0x4653cc`; mug `0x46543c`..`0x46549f`; axespin / earthshaker `0x4654a4`..`0x465617` |
| Resistance, position, curse, critical, Super power, floor | `0x465bfe`..`0x465c64`, `0x465c68`..`0x465c82`, `0x465c86`..`0x465ca5`, `0x465ca9`..`0x465cdb`, `0x465ce1`..`0x465cf5`, `0x465cfb`..`0x465d13` |
| Resistance of type -1 | `character_get_resistance` `0x4626bb`..`0x4626cb` |
| Multiplayer damage x2 / x4 | `character_add_hp` `0x4508bd`..`0x450922` |
| Floating number at `+0x6e8` | `0x450925`..`0x4509b0`, `0x450bf5`; `levelrender_draw_float_text` `0x48543a` |
| Status 16 blocks every HP change | `0x450abc`..`0x450b1d` (both paths `jmp 0x450c3c`) |
| Death option and immortal | `0x450b4e`..`0x450b97` |
| `+0x608` party hit byte | `character_add_hp` `0x450892`..`0x4508b7`; read at `character_on_death` `0x453a4a` |
| Zero change shows "0" | `0x450928`..`0x45095a` (`delta == 0` with the timer at 0 or below), early return `0x4509df`..`0x4509f1` |
| Kill: death move id, flag `0x8` | `character_kill` `0x450d9d` (`mov [eax+0xa4], ecx`), conditions `0x450db8`..`0x450e0d`, `or edx, 8` at `0x450e18`; callers pass `push 0x22` (`0x450c52`, `0x461a3c`, `0x473a03`) or the script value (`0x4dc9d0`) |
| Dead flag `0x80` | `character_on_death` `0x453870` (`or al, 8`), `0x45393a` (`or dl, 0x80`); called from `ai_frame` `0x472dd6` |
| Dead targets, range, Nightmare | `0x45deb6`..`0x45def7`, `0x45e0d8`..`0x45e123`, `0x45df93`..`0x45e078` |
| Dead party, serpent part, combat mode | `0x45df5a` (`serpent_part_on_hit`), `0x45e0d0` (`combat_mode_set`) |
| Power by source type | `0x45e128`, table `0x460075` |
| Hit chance and resist value | `0x45e3ac`..`0x45e450`; default hit chance 1.0 `spells_parse_table` `0x47a294` |
| Resist tests | `<=` in the status block `0x45e9b3`..`0x45e9c6`; `<` at `0x45eccc`, `0x45f467`, `0x45f4ab`, `0x45f4fc`, `0x45f551`, `0x45f596`, `0x45f5ef`, `0x45f6a7`, `0x45fc67`, `0x45fd25`, `0x45fe39`; icy sleep against magic resistance `0x45ff8f`..`0x45ffb4` |
| Jade Beam half HP | `0x45f1c4`..`0x45f209` |
| Death spells strip equipment | `character_unequip_slot` loop, `0x45fdf0` |
| Leech value dropped | `0x45f471`..`0x45f47d`, then `spell_roll_amount` result stored over it at `0x45febe` |
| Lightning die negated | `0x45e4ca`..`0x45e4d8` |
| Roll passes magic resistance | `mov ecx, [ebp-0x20]` at `0x45fe87` (stored from `character_get_resistance(3)` at `0x45e424`) |
| Dice, resistances, clamp, party | `spell_roll_amount` `0x47f4df`..`0x47f64a` |

## Changes (2026-10-15)

{: .important }
Checked again against the disassembly with `tools/asm.mjs`. Corrected: backstab without flag `+0x50` is
**halved**, not left as is; type 9 and flag `0x40` attackers only double. The "chain attacks" with
`level x 4 / x 8 / x 10` are kick, serpent whip multi and serpent whip; push, trip and the self-cast skills deal 0.
`0x450c70` is `character_add_ap`, used by **mind drain**, not by life leech; life leech halves the damage and
heals the attacker with `character_add_hp`. Added the steps after the side effects (resistance, position, curse,
critical hit, Super power, floor), the weapon flag `0x200`, the unarmed damage type and the ids that deal 0.
`+0x6e8` is the floating-number value, not excess HP; status 16 blocks heals and drops overflow damage; the
death option stops all damage; multiplayer doubles damage from non-party sources. Spell hits: the damage
multiplier is `1 - magic resistance`, not `1 - resist value` (the resist value only decides whether an
offensive spell lands); damage dice roll 0 to `-(die - 1)`; healing gets no party factor; the hit-chance cases
include spell `0x50`; the species case is the caster's.

## Changes (2026-10-15, living_entity.cpp)

{: .important }
Checked against the disassembly of the `living_entity.cpp` functions. Corrected: `character_kill` stores the
**death move id** at `+0xa4`, not the killer; the flag it sets, `0x8`, means dying, and the dead flag `0x80`
comes later from `character_on_death`. On a multiplayer client the death-option and boss tests are skipped,
so the old "only on a single-player game or a multiplayer host" was wrong. Spell hits: the source type is
the **fifth** stack argument, not the fourth; most handlers resist with `frand() < resist` and test it even
for spells without the offensive flag, and only the shared status block uses `<=` for offensive spells; icy
sleep's sleep is tested against magic resistance. Status 8 is now **observed** as invisibility and status 16
named as the ice coffin. Added the `+0x608` "hit by a party member" byte, the "0" for a zero change, the early
returns of `living_entity_get_hit_by_spell`, Jade Beam's half-HP hit, the death spells' equipment loss,
Life Drain, power leech and leech (which changes no HP), and die 0 skipping resistances in the roll.
