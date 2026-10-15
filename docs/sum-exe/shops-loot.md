---
title: Shops, loot and money
parent: Sum.exe
nav_order: 6.45
---

# Shops, loot and money
{: .no_toc }

1. TOC
{:toc}

**Summary:** Shop prices are fixed when the level table is read. A shop sells an item for its full
`$Value` and buys it for `floor(value x 0.7)` (in multiplayer `floor(value x 0.1)`, at least 1).
No skill, stat or difficulty changes a price. Stock never runs out. Buying a chargeable item you
already own recharges it to full for the full price. A killed monster drops gold 25% of the time
(`Chance_to_drop_gold`), `1..10 x level` pieces. Then each `+Drop:` line of its character table
rolls with its percent chance plus 1%. Everything that drops lands in a leather bag or backpack. The
party's gold is one signed 32-bit number with no cap. A new game starts with 30.

{: .note }
Function names are our labels from `symbols/sum.symbols`. The evidence table at the end lists
function addresses, not REA evidence ids. Written 2026-10-15 from the disassembly (`tools/asm.mjs`)
and a read-only look at the retail tables (`tools/vpp.mjs`). Agent draft; the price code was re-checked. The game was not run.

## Where shops come from

- **Observed:** a level's `.tbl` file has a `#Shopkeepers` section. `level_parse_shopkeeper`
  (`0x4cc4d0`) reads `$Shopkeeper: "name"`, then any number of `+Sell List:` and `+Buy List:` blocks
  of quoted item names, each closed by `$End`.
- **Observed:** each shopkeeper is a 0x14-byte record in `level_shopkeepers` (`0x2490ca0`): name,
  sell count, sell entries, buy count, buy entries. The array has room for 15 records. Every entry
  is 8 bytes `{item type index, price}` in `level_shop_items` (`0x2491ac0`), room for 1000 per level.
- **Observed:** the parser checks neither limit. *Inferred:* a level with more would overwrite the
  data after the arrays. The retail tables stay inside them (the largest, `KhosaniStrng`, has 10
  shopkeepers and about 890 list lines).
- **Observed:** a duplicate item inside one list is skipped. An unknown item name prints "Unable to
  locate item %s in any table file." and is skipped. Both lists are sorted by shop category, then by
  type index (`shop_inventory_sort`, `0x46b660`).
- **Observed:** the shop opens from dialogue. When a conversation ends, `conversation_end_cleanup`
  (`0x4daa70`) looks the speaker's name up with `level_shopkeeper_find` (`0x4ce950`). If it is a
  shopkeeper, it calls `shop_set_shopkeeper_from_char` (`0x406360`) and opens screen 7 (buy/sell).
  See [Interface screens](interface.md).

The retail tables define about 50 shopkeepers in 16 levels (towns, three `Rand-*` encounter maps,
and one `Shopkeeper#Multiplayer` in each multiplayer town). **Observed** (table listing).

## Prices

The price is computed once, in `level_parse_shopkeeper`, from the item's `$Value` (`ItemType +0x18`,
see [Game tables](game-tables.md)):

| List | What it means | Price stored |
|---|---|---|
| `+Sell List:` | the shop sells to you | `value` |
| `+Buy List:` | the shop buys from you (single player) | `_ftol(value x 0.7f)` |
| `+Buy List:` | the shop buys from you (multiplayer, `game_flags & 2`) | `max(1, _ftol(value x 0.1f))` |

```
4cc6fe: fild  dword ptr [esp + 0x14]     ; value
4cc702: test  byte ptr [0x5fbfd8], 0x2   ; multiplayer?
4cc70b: fmul  dword ptr [0x580214]       ; 0.1  (then at least 1)
4cc722: fmul  dword ptr [0x58020c]       ; 0.7
4cc728: call  _ftol                      ; truncates
```

- **Observed:** the constants are 0.1 and 0.7 as single floats. `_ftol` truncates.
- *Inferred:* 0.7f is 0.69999999, so when `0.7 x value` is a whole number the shop pays one less.
  A value of 10 sells for 6, and 100 sells for 69. A value of 1 sells for 0 in single player.
- **Observed:** nothing else touches the price. `buysell_buy_item` (`0x4057f0`) and
  `buysell_sell_item` (`0x405900`) read entry `+4` and add or subtract it unchanged. There is no
  haggling skill, no charisma, no reputation and no difficulty factor.
- *Inferred:* the multiplayer factor is chosen when the level table is parsed, so it follows the
  mode the game is in at that time.

## Buying and selling

- **Observed:** `buysell_mode` (`0x5b6410`, formerly labelled `buysell_category`) is 0 for buying and 1 for
  selling. Buying lists the shop's `+Sell List`. Selling lists only the party items whose type is
  on the shop's `+Buy List` (`buysell_count_party_items`, `0x4050f0`).
- **Observed:** **buy** is refused with sound `0x24` when `party_gold < price`. The buy button is
  greyed out for the same reason (`buysell_update_button_states`, `0x4048d0`). Otherwise the price is
  subtracted and `party_inventory_add_type(type, 1, -1, 1)` (`0x46b980`) adds one item.
- **Observed:** stock is unlimited. Nothing in the buy path changes the shop list.
- **Observed:** **sell** finds the first inventory entry of that type, adds the price to
  `party_gold` and takes one unit off the stack. The item does not join the shop's stock.
- **Observed:** selling does not check identification. An unidentified item sells for the same
  price. Unidentified types are drawn in blue, and items your selected character cannot use in red
  (`buysell_draw_item_list`, `0x405fd0`). The item-info button needs an identified type.
- **Observed:** gold is subtracted before the item is added. If the inventory is full (0x209 =
  521 entries), `party_inventory_add_type` returns without adding, and the gold is lost. *Inferred:*
  hard to reach in practice.

### Recharging

- **Observed:** if you buy a type you already own, and it is not stackable and has magic info
  (`ItemType +0x58`), `buysell_buy_item` compares the owned item's charges (`item +8`) with the
  maximum (`magic +4`). When full it shows "This item is fully charged." and charges nothing.
- **Observed:** otherwise it charges the full price, and `party_inventory_add_type` adds the
  maximum to the owned item's charges and clamps them at the maximum. No second copy is made.
  Recharging one charge costs as much as recharging all of them.

### Identification

- **Observed:** identification is free. `inventory_identify_item` (`0x416fb0`) picks the party member
  with the highest appraise skill (`0x14`) whose skill timer has run out, and calls
  `character_use_skill` (`0x429410`). No gold is involved.
- **Observed:** the "identified" state is bit 0 of `ItemType +0x5c`, so one success identifies every
  item of that type, in shops too. A new game resets the bit from the `"not identified"` table flag
  (`item_types_reset_identified`, `0x44b250`). See [Characters](characters.md) for the skill list.
- *Unknown:* the success roll inside `character_use_skill` for appraise.

## Monster loot

`monster_drop_loot` (`0x450e30`) runs from the death handler `character_on_death` (`0x4537f0`,
formerly labelled `summon_turn_hostile`) and from the script commands `script_object_drop_loot` /
`script_npc_drop_loot`.

- **Observed:** the death handler skips it for type-7 characters and for characters with `+0x320`
  bit `0x100`. *Inferred:* those are party members and summons. In multiplayer only the host
  (`mp_local_player +0x10` bit 0) rolls loot.

### Gold

```
if (frand() < Chance_to_drop_gold)                       // 0x599690, default 0.25
    gold = _ftol(frand() * level * 10) + 1;              // level = character +0x600
    if (multiplayer) gold *= mp_netgame_num_players;     // 0x6513e4
```

- **Observed:** `frand` is `rand() / 32768`, so a level-5 monster drops 1 to 50 gold, 25% of the time.

### Items

- **Observed:** the item list comes from the monster's character table
  ([Game tables](game-tables.md)). Single player uses `+Drop: "item" percent` (at most 5 lines,
  `char_info +0x274/+0x278/+0x2bc`). Multiplayer uses `+Multiplayer Drop: "item" percent level` (at
  most 17 lines, `+0x300/+0x304/+0x348/+0x38c`). The percent is stored as `percent x 0.01`.
- **Observed:** the single-player test is `game_flags & 1`. That bit is set by
  `state_boot_enter` and `state_mp_lobby_leave`, and `mp_local_player_create` replaces it with 2.
- **Observed:** a multiplayer line is skipped when its level is above the monster's level. Retail
  uses 1, 25 and 50.
- **Observed:** the item's type flags gate it by `mp_game_difficulty` (`0x6513ec`): `"multi easy
  only"` (`0x20`) needs 0, `"multi medium only"` (`0x40`) needs 1, `"multi hard only"` (`0x80`)
  needs 2. The check runs in single player too. *Unknown:* whether `mp_game_difficulty` is always 0
  in single player.
- **Observed:** each line rolls once. `r = frand()`, half the time `r = 1 - r`, then `r -= 0.01`.
  The item drops when `r <= chance`. *Inferred:* the chance is the table percent plus 1%, and 100
  always drops. A `"Rolling %d %d"` debug line is printed.
- **Observed:** in single player a monster also drops the weapon in equipment slot 5 and the
  weapon in slot 9, unless the item type has bit `0x2` at `+0x6a`.
- **Observed:** named bosses always drop a key item (`_stricmp` on the character's script name
  `+0x2f4`). See the table below.

| Character | Item |
|---|---|
| Slave Driver | Slave Pit Key |
| Mortankas | Tome Page 8 |
| Azha the Archlich | Azha's Femur |
| Lord Korel | Jarl's Sword |
| Zane | Zane's Bow |
| Cerval | Silk Garments |
| Haenul | Eraekor Gemstone |
| Guardian of the Shrine | Ring of Forest |
| Orenian Scout `$npc002` in Masad (level 3) | Worn Katana, unless one already dropped |

### The loot bag

- **Observed:** a lone gold drop stays a loose gold pile. Anything more goes into a new container
  from `container_obj_create` (`0x4897f0`): a `"leather bag"` or a `"backpack"` (50/50), with a
  random yaw. Sound `0xd` plays if a weapon dropped, else `0xc`.
- **Observed:** the bag is unlocked when the game is multiplayer, when a line has chance > 0.98, or
  when a boss item dropped. Otherwise it is locked with chance `level / 100`, and the lock value
  (`container +0xe4`) is `frand()`. A container with lock > 0 shows "Locked". Pick lock is skill
  `0x15`; *inferred:* it succeeds when the skill beats the lock value.
- **Observed:** `hud_set_timed_highlight` (`0x43cd50`) highlights the bag for 5 s. In multiplayer
  the host sends it with `container_spawn_loot` (`0x488fc0`).

## Containers, examine spots and gold piles

- **Observed:** the container pool holds 40 containers (`0x2302fe0`, 0xe8 bytes each). Each holds up
  to 16 items. `container_add_item` (`0x46aa30`) merges gold into one gold item kept first.
  **Observed:** when the pool is full, or the bag's item type is missing, `container_obj_create` returns 0
  (`0x48980f`, `0x489879`); `monster_drop_loot` then leaves the drop loose (`0x452ec6`). `level_item_spawn` does
  not test it (game bug C274, latent).
- **Observed:** placed level items come from `level_items.tbl` (`$Item`, `$Navpoint`, `+Count`,
  at most 32 per level). `level_item_spawn` (`0x46a640`) puts each in its own container.
- **Observed:** "examine" spots are `lc_entries` (`0x248ce40`, 50 records): an item name and a count
  `+0x108`. `container_examine` (`0x43c420`) gives them once. It shows "Found %d %s!" or "Found
  %s!", or "Nothing!" the second time. The name `"gold"` adds the count to `party_gold`.
- **Observed:** clicking a loose gold pile shows "Picked up %d Gold" and `item_obj_pickup`
  (`0x46b6b0`) adds `+0x9c` to `party_gold`. The pickup screen does the same per selected row
  (`pickup_take_selected`, `0x420da0`). Party AI can walk to items and gold (`ai_goal_do_pickup`).

## Party gold

- **Observed:** `party_gold` (`0x971868`) is a signed int. `party_inventory_reset` (`0x46b680`) sets
  it to 30 for a new game. It is saved at `INVT +0x04` ([Savegame](savegame.md)) and in the
  multiplayer `.plr` file ([Multiplayer](multiplayer.md)).
- **Observed:** no code clamps it at the top. Only the dialogue "give gold" action clamps a
  negative result to 0.
- **Observed:** other sources and sinks:

| Code | Effect |
|---|---|
| `combat_melee_damage` (`0x464f50`), mug | adds `_ftol(damage x 0.5)`, "Stole %d gold!" ([Combat](combat.md)) |
| `conversation_execute_action` (`0x4d5990`) | adds or takes gold ("<party received %d gold>"), and topics that need `gold >= N` |
| `ls_lenele1b_action`, `ls_lenele2ab_action` | Lenele scripts compare with 20 and 25, take gold or empty it |
| `mp_trade_complete` (`0x4718d0`) | moves gold between players (trade screen 19 has a gold row) |
| `state_game_enter` (`0x4382ea`) | 9,999,900 gold if byte `0x5fbf19` is set; nothing writes it (*inferred* dead) |
| `state_game_enter` (`0x43830a`) | 100 gold and a katana when `US_open_demo` is set |

## Console commands

| Command | Function | Effect |
|---|---|---|
| `add_gold <n>` | `cmd_add_gold` (`0x46c570`) | `party_gold += n`, prints "Added %d gold to party." *Inferred:* a negative `n` takes gold and can go below 0 |
| `chance_gold <f>` | `cmd_chance_gold` (`0x463080`) | sets `Chance_to_drop_gold` (default 0.25); 1.0 makes every kill drop gold |

See [Console](console.md).

## Player tips

- Sell gear worth a multiple of 10 last: it fetches one gold less than 70%.
- Recharge only when a charged item is nearly empty. Any recharge costs the full price.
- Selling unidentified items costs nothing extra, and appraising is free.
- Shops never run out. You can buy a whole stack at the same price.
- Higher-level monsters drop more gold (up to 10 x level). In single player their bags are
  more often locked, so bring a lockpicker.
- An item listed at 100% always drops; 5% is really 6%.

## Evidence

Function and data addresses (no REA evidence ids).

| Claim | Where |
|---|---|
| shop records, list parsing, price factors, dedup | `level_parse_shopkeeper` `0x4cc4d0`..`0x4cc7d2`; floats `0x58020c` 0.7, `0x580214` 0.1 |
| shop opened after dialogue | `conversation_end_cleanup` `0x4dab66`..`0x4dab8b` |
| buy check, deduction, fully-charged warning | `buysell_buy_item` `0x40581e`..`0x4058c4` |
| recharge to max | `party_inventory_add_type` `0x46ba53`..`0x46ba94` |
| sell adds price, takes one unit | `buysell_sell_item` `0x40593b`..`0x405969` |
| mode 0 = shop list, 1 = party items on the buy list | `buysell_count_party_items` `0x4050f0`, `buysell_get_list_item` `0x404960` |
| colours, info button | `buysell_draw_item_list` `0x406074`..`0x4060b0`, `buysell_update_button_states` `0x40491d` |
| appraise identifies | `inventory_identify_item` `0x417017`..`0x41705e` |
| gold roll and amount | `monster_drop_loot` `0x450e5f`..`0x450ff7`; floats `0x599690` 0.25, `0x57eb7c` 10 |
| drop tables and limits | `characterinfo_parse_table` `0x4463d2`..`0x446527`; float `0x5818f0` 0.01 |
| SP/MP table, difficulty flags, roll | `monster_drop_loot` `0x451061`..`0x4514de`; floats `0x5818ec` 0.01, `0x58191c` 0.98 |
| flag names | string table `0x599024` |
| equipped weapon drops | `monster_drop_loot` `0x4514e3`..`0x4518f6` |
| boss items | `monster_drop_loot` `0x4518f9`..`0x452db5` |
| bag, lock chance, highlight | `monster_drop_loot` `0x452db8`..`0x4530bc`; `container_obj_create` `0x489831` |
| who drops loot | `0x4537f0` death handler `0x453ef6`..`0x453f5d` |
| locked check | `object_use_container` `0x43cc7c` |
| examine spots | `container_examine` `0x43c45a`..`0x43c4df` |
| gold pickup | `item_obj_pickup` `0x46b6f4`..`0x46b70d` |
| single-player bit | writes at `0x431c65`, `0x43f7ae`, `0x441763` |
| other gold writers | `grep 0x971868]` in `tools/asm.mjs` (list above) |

## Open questions

- *Unknown:* the appraise and pick-lock success formulas inside `character_use_skill`.
- *Unknown:* whether `mp_game_difficulty` can be non-zero in single player.
- *Unknown:* what `+0x320` bit `0x100` and `ItemType +0x6a` bit `0x2` are called in the tables.
