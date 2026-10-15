# player.cpp and vsdk\geom\effect_mem.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and a byte dump of .data only. Summoner 2 and Dash
Faction were not read. Syntax-checked with `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings`
and the include path from the task (clean). Not compiled with VC6.

Files: `player.h`, `player.cpp`, `effect_mem.h`, `effect_mem.cpp`, `stubs_player_effectmem.h`.
Only `timestamp.h` (GameTimer) of the shared headers is included; the reasons are in the header comment
and under "Shared headers" below.

## player.cpp (0x433340-0x43439f)

The 22 tagged functions plus the six initialiser functions in front of them and three untagged
Player members in the range (`player_input_reset`, `player_skill_unlocked_at_level`,
`player_init_from_character`). Previous unit ends at 0x433331 (gr_info command), next starts at
0x4343a0 (init_ccmd_slew_thunk).

| Address | Function | Confidence |
|---|---|---|
| 0x433340 / 0x433350 | init_gametimer_arrays_5fc378 (thunk + body): `Player party_list, player_free_list;` | likely |
| 0x433390 / 0x4333a0 | init_gametimer_array_5fc058: `Player player_list_other;` | exact |
| 0x4333c0 / 0x4333d0 | init_gametimer_arrays_5fc500: `Player Players[5];` | likely |
| 0x433400 / 0x433410 | init_ccmd_max_skills (thunk + body): `console_command ccmd_max_skills(...)` | exact |
| 0x433430 | cmd_max_skills | exact |
| 0x4334f0 | player_alloc_slot | likely |
| 0x433580 | player_clear_handle_of | exact |
| 0x4335a0 | party_remove_player | likely |
| 0x4336b0 | party_remove_all | likely |
| 0x433730 | player_input_reset (Player::input_reset) | exact |
| 0x433750 | player__set_ai_control | exact |
| 0x433790 | player_create (Player::create) | likely |
| 0x433960 | player__make_leader | exact |
| 0x4339a0 | player__drop_leader | likely |
| 0x4339e0 | player__is_leader | likely |
| 0x4339f0 | player_skill_unlocked (Player::skill_unlocked) | likely |
| 0x433b10 | player_skill_unlocked_at_level | exact |
| 0x433b70 | player_init_from_character | likely |
| 0x433ca0 | party_lists_init | likely |
| 0x433d10 | party_lists_atexit_cleanup (static) | exact |
| 0x433d60 | party_set_leader | likely |
| 0x433eb0 | player_from_handle | exact |
| 0x433ef0 | player_start_skill_cooldown | exact |
| 0x433f40 | party_remove_member | likely |
| 0x4340d0 | party_add_member | likely |
| 0x434270 | party_sort_by_species | likely |
| 0x434350 | party_count_active | exact |

27 entries (31 functions counting thunks): 12 exact, 15 likely, 0 unsure.

Data: .data 0x5934d0-0x5937eb (14 model-name pointers, the four skill progression tables with their
counts; literals follow up to 0x593948), .bss 0x5fc030-0x5fcca3.

## effect_mem.cpp (0x51bb70-0x51d240 written; unit runs to 0x51e018)

The 11 tagged functions, the 24 untagged functions between them, and the static initialisers before
them, which construct the record arrays effects_init_pools threads.

| Address | Function | Confidence |
|---|---|---|
| 0x51bb70 | init_vertex_buf_272b900 (`vector3 vfx_mesh_verts[1820]`) | exact |
| 0x51bba0 | init_fx_rec48_array | exact |
| 0x51bbd0 | init_fx_rec98_array | exact |
| 0x51bc00 | init_fx_rec30_array_2730e50 | exact |
| 0x51bc30 | init_fx_rec2c_array_2c8ff60 | exact |
| 0x51bc60 | init_fx_rec4c_array | exact |
| 0x51bc90 | init_fx_rec28_array_2c42690 | exact |
| 0x51bcc0 | init_fx_recb0_array | exact |
| 0x51bcf0 / 0x51bd00 | init_effects_mempool | likely |
| 0x51bd20-0x51bd50 | init_effects_string_pool, _ctor, _register_dtor, _dtor | exact |
| 0x51bd60 / 0x51bd70 | init_ccmd_show_effect_mem | exact |
| 0x51bd90 | cmd_show_effect_mem | likely |
| 0x51bda0 | effect_base_alloc_stream_buffer | likely |
| 0x51be80 | align_up_64 | exact |
| 0x51bea0 | effect_base_finish_stream | likely |
| 0x51bf20 | vfx_preload_by_name | exact |
| 0x51bf70 | vfx_find_name | exact |
| 0x51bfd0 | rfx_preload | likely |
| 0x51c370 | effect_base_release | likely |
| 0x51c490 | effect_base_load | likely |
| 0x51c520 | effects_alloc_stream_slot | likely |
| 0x51c5c0 | effects_find_stream_slot | likely |
| 0x51c660 | effects_preload_slot_ptr | exact |
| 0x51c680 | effects_stream_slot_ptr | exact |
| 0x51c6a0 | effect_base_queue_stream | likely |
| 0x51c720 | effect_get_base | likely |
| 0x51c800 | effects_get_num_bases | exact |
| 0x51c810 | effects_find_base | exact |
| 0x51c860 | effect_base_set_info | exact |
| 0x51c8b0 | effect_base_read | likely |
| 0x51c9c0 | vfx_preload | exact |
| 0x51c9f0 | vfx_relocate (effect_base::relocate) | exact |
| 0x51ca30 | vfx_relocate_entry (vfx_node_entry::relocate) | likely |
| 0x51ca60 | effects_parse_arr_line | likely |
| 0x51ccd0 | effects_reset_level | likely |
| 0x51cd40 | effects_init_pools | likely |
| 0x51cf60 | effects_load_bases | likely |
| 0x51d240 | (no symbol) effects_show_memory_usage, empty | likely |

38 entries: 20 exact, 18 likely, 0 unsure. The five compiler-generated record constructors
(0x51df10-0x51dfee) come from the struct definitions in effect_mem.h.

**Not written, same unit:** 0x51d250 vfx_instance_create, 0x51d7b0 vfx_instance_free, 0x51db50
effects_hash_name (declared), 0x51dbb0 effects_load_preload_rfx, 0x51dc20 effects_release_level_memory,
0x51dc50 effects_release_range, 0x51dca0 effects_stub_ret1, 0x51dcb0 vfx_all_resident, 0x51dd00
vfx_load, 0x51de90 effects_base_is_resident, 0x51dec0 effect_is_cutscene_base, 0x51def0
effects_release_cutscene_memory, and the bodies of Mempool::Mempool (0x51dff0) and
Mempool::getPoolSpaceLeft (0x51e010), which only have declarations here. The .bss gaps 0x2c90a70-0x2c98a6f
and 0x2c98a8c-0x2c9f1af belong to those functions.

## Facts used

- player.cpp: DebugError line 421 (0x1a5); species jump tables at 0x433950, 0x433c84 and 0x434338 map
  0..3 to Joseph, Jekhar, Flece, Rosalind in order. Player::create reads the four
  `*_skill_progression_count` ints from memory, player_init_from_character uses the constants 17, 13,
  16, 14, so the latter used the array sizes. With Max_skills, Player::create writes 100 to the skill
  and **1 into the progression's level field** (the tables are writable).
- `player_input_reset` is called with ECX loaded (ECX = record in player_alloc_slot, = party_leader in
  party_set_leader) though it only zeroes globals: a Player member.
- `player_free_list` is a full Player (0x5fc1c8..0x5fc350, GameTimers at 0x5fc1f0), not void*[2].
- effect_mem.cpp: DebugError lines 1095 (0x447) and 1783 (0x6f7). Effect base +8 is a bitfield
  (XOR/AND/XOR store of the 23-bit size, category read as mask compare and as SHR 27 / AND 7). Sizes:
  preload area 0x4c4c0, stream slots 8 x 0x3d0c0 from 0x277d910, big slots 6 x 0x7a140 from 0x2965f10,
  together 0x4c4d80; the Effects Mempool is built over 0x511240 bytes = both areas.
- effects.arr categories: level 0, preload 1, spell 2, cutscene 3, summon 4, character 5; rfx_preload
  sends category 0 to level_mempool and 5 (character) to summon_mempool when no pool is current.

## Open questions

1. **.bss order** (both files). The initialiser order is definition order, so the definitions are
   written in that order; their addresses are not in that order (player_list_other < ccmd_max_skills <
   player_free_list < party_list < Players; effect_mem's arrays likewise). Same question as frametime.cpp.
2. **One initialiser for two objects.** 0x433350 builds party_list and then player_free_list in one
   function. Written as `Player party_list, player_free_list;`; whether VC6 makes one $E function per
   declaration needs the compiler.
3. **effect_mem.cpp build flags.** No string pooling (two __FILE__ literals, four " \r\n\t:,") and no
   inline expansion: cmd_show_effect_mem is a JMP to an empty function, the record constructors call
   vector3() out of line, Mempool's constructor and getPoolSpaceLeft are emitted and called. Fits /O2
   without /Gf and with /Ob0 for vsdk\geom. player.cpp uses pooled "TRUE"/"FALSE" (/Gf).
4. **effects_mem_buffer**: one 0x511240-byte array (written so) or two adjacent arrays with a summed
   size constant. symbols/sum.symbols gives 0x4c4c0 for it and a separate effects_stream_buffer.
5. **Slot masks.** effects_alloc_stream_slot masks with 5 for a 6-slot ring and effects_find_stream_slot
   scans 8 slots with mask 7 but stores the cursor masked with 5: source bugs reproduced as is (only
   slots 0, 1, 4, 5 of the big ring are reachable).
6. **effect_base +4** is added to the 23-bit size for the buffer and used as the relocation end
   (data + it); named table_size, meaning not confirmed. Bit 23 (`level`) and bit 24
   (`bitmaps_pending`) names are ours.
7. **party_remove_player** picks a new leader even when the removed record did not lead; in
   multiplayer it calls party_set_leader for every locally owned member (no break). Reproduced; intent
   unknown. party_remove_member also always re-picks the leader.
8. **game_flags 0x200** gates the leader-change sound 0x28 in party_set_leader; the bit is not in the
   game_flags comment of sum.symbols.
9. **Labels ours:** hero_model_names (0x5934d0), Players (0x5fc4d8), party_lists_initialized
   (0x5fcc90, written once, never read), effects_string_buffer (0x27288d8), effects_name_order
   (0x2c8e980, set to 0..439, no reader), effects_name_map (0x2c9f288, set to -1, no reader),
   effects_vfx_counter_2c9f968, effects_show_memory_usage (0x51d240).
10. **player_input** (0x5fcc98) is tagged gameplay.cpp but lies in this unit's .bss before ccmd_slew
    (next unit); defined here.
11. effect_get_base compares the node pointer with the base data signed (JLE): written as int casts.

## Shared headers (not changed)

- `stubs_sound_items_registry.h` declares `Mempool(char*, int, const char*) { Copy(...); }` inline;
  in effect_mem.cpp the constructor is an out-of-line function (0x51dff0) that is called. Either the
  header body is right and this unit was built /Ob0 (question 3), or the constructor is not inline.
  The same header's `struct Character` / `struct Player` (0x10 bytes) and `sound_ref`-style partial
  layouts collide with full definitions, so it cannot be included together with them.
- `stubs_menus_dialogue.h` declares `int game_play_sound(int sound, float volume, int a, int b, int c)`;
  gamesound.h (and the code) has a float pan as the third argument
  (`game_play_sound(int, float, float, int, int)`).
- `symbols/types.h` gives console_command as 0xc bytes; the objects are 0x10 apart in .bss.
- `symbols/sum.symbols`: `player_free_list` is typed void*[2] (it is a Player, 0x188 bytes);
  `effects_mem_buffer` is given 0x4c4c0 bytes (the Mempool spans 0x511240).
