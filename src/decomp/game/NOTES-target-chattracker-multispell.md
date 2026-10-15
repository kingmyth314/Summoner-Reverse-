# Phase 3: gametarget.cpp, multi_chattracker.cpp, multispell.cpp (decomp35)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .data / .rdata bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. All three files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include` with no
warnings, every new or changed header compiles on its own, and all 64 existing `src/decomp/{game,vsdk}/*.cpp`
(repo HEAD 99fac83) give exactly the diagnostics against this tree that they give against the repo's (none).

**Base:** the include tree is the repo's at **99fac83** (the repo moved twice while this batch was written;
the header changes are scripted and were re-applied on each new base). Header changes: CHANGED-HEADERS.txt;
decisions D1500-D1512 at the end of include/DECISIONS.md.

All 68 functions of the three units (sum.symbols module column, initialiser thunks counted) are written:
**39 exact, 29 likely, 0 unsure**. Not in sum.symbols but written: the atexit nop 0x43cd40 (exact, comment
only) and world_pick_debug_hook (no address, folded into 0x40acc0; unsure, D1507).

## gametarget.cpp (0x43a330-0x43cd6d; .rdata 0x580e00-0x580e37; .data 0x5943f0-0x594437; .bss 0x60ab60-0x60abab)

| Address | Function | Confidence |
|---|---|---|
| 0x43a330 / 0x43a340 | init_gametimer_60ab70 pair (chain_attack_timer, entry 130) | exact |
| 0x43a350 / 0x43a360 | init_gametimer_60ab60 pair (gold_highlight_timer, entry 131) | exact |
| 0x43a370 | world_pick_set_skill_filter | exact |
| 0x43a390 | world_pick_clear_filter | exact |
| 0x43a3a0 | party_order_attack | likely (`is_leader() == true`) |
| 0x43a460 | character_can_talk_to | likely (sum order; the name test) |
| 0x43a600 | world_tint_gold_object | likely (the out-of-bounds index) |
| 0x43a650 | world_hover_tint_object | exact |
| 0x43a6e0 | object_open_pickup | likely |
| 0x43a770 | world_pick_lobj | likely (loop form) |
| 0x43a940 | world_pick_reset | exact |
| 0x43a960 | world_mouse_frame | likely (big; locals, statics) |
| 0x43b1d0 | world_hover_tint_character | exact |
| 0x43b260 | world_pick_under_mouse | likely (inline vector COMDAT calls) |
| 0x43b5e0 | world_mouse_hover_target | likely (two jump tables) |
| 0x43bba0 | world_pick_ground_point | likely (validity test order, min / max) |
| 0x43be80 | world_click_move | likely |
| 0x43c060 | object_try_unlock_or_open | likely (key search flag) |
| 0x43c420 | container_examine | likely (text size) |
| 0x43c5a0 | player_talk_to | exact |
| 0x43c620 | pick_test_character | likely (term order) |
| 0x43c830 | chain_attack_select | likely |
| 0x43cb60 | object_use_container | exact |
| 0x43cd50 | hud_set_timed_highlight | exact |
| (0x43cd40) | atexit nop of world_mouse_frame's static hit_pos | exact (comment) |

26 functions: 12 exact, 14 likely.

Facts:
- The world mouse runs every frame of state_game_frame with (player_input, camera_view_pos). Input bits:
  1 right click, 4 left click, 8 move flag, 0x10 held, 0x40000000 debug teleport (on lobjs with flag 0x40),
  0x80000000 debug pick (world_mouse_target_handle + ui_input_lock).
- The level object under the mouse is ray-picked only every third frame (world_pick_lobj_counter), up to the
  near fog distance, among drawn opaque objects (render state bits 4-11 all set, lobj flag 0x400 clear).
- Characters are picked against the camera-to-point segment (pick_test_character); party summons in AI
  mode 6, mounts and (with the follow camera, flag 1) characters 15 m or 3 m of height away are ignored.
- Click results (WORLD_CLICK_*, gametarget.h): select a party member, talk (single player) / shop (network
  game) to an NPC, attack a team 2 / 3 monster, gold pile within 3 m, container within 3 m, door or level
  container (within 5 m) in single player, lock picking (target mode 2), another player's character within
  5 m (trade, IT_RQST).
- A held left click walks in the mouse direction (character_ai_travel_offset); speed from the cursor's
  distance to the screen centre, or with the high camera (elevation >= 4) from the point's distance / 4.
- Ground target decal: valid on lobj flag 0x40, navigable, < 5 m of height, 3 m <= horizontal distance <
  13 m, the surface allowed by the pick filter (skill 0x26 has its own: surfaces 1 and 2 only) and in sight.
- Doors: a key in the party inventory or equipped by any party member unlocks a locked door for good.
  Messages 623-630; a client sends service request 4 / 5 instead of toggling.
- Chain attacks: right click picks the slot by the held key (Shift 1, Ctrl 2, Alt 3, CapsLock 0, none: the
  leader's default, which then cycles 0 -> 3 -> 1 -> 2), 1 s between picks.

## multi_chattracker.cpp (0x4abb80-0x4ae32d; .data 0x59d41c-0x59d617; .bss 0x2479728-0x247b09b)

| Address | Function | Confidence |
|---|---|---|
| 0x4abb80 | irc_send_raw | exact |
| 0x4abba0 | irc_chat_reset | exact |
| 0x4abc10 | irc_connect | likely (local sizes) |
| 0x4abee0 | irc_flush_command_queue | exact |
| 0x4abf20 | irc_disconnect | exact |
| 0x4abfc0 | irc_remove_all_users | exact |
| 0x4ac020 | irc_queue_command | likely |
| 0x4ac0e0 | irc_get_chat_text | exact |
| 0x4ac100 | irc_read_line | likely (loop form) |
| 0x4ac240 | chat_process_irc_message | likely (0x1670 bytes; buffer order and initialisers) |
| 0x4ad8b0 | irc_get_word | likely |
| 0x4ad9a0 | irc_flush_channel_list | exact |
| 0x4ad9e0 | irc_add_channel | likely |
| 0x4adb40 | irc_add_user | likely |
| 0x4adc40 | irc_remove_user | exact |
| 0x4adcd0 | irc_process_input | likely |
| 0x4adfb0 | irc_get_chat_command | exact |
| 0x4adfd0 | irc_pop_command | exact |
| 0x4ae010 | irc_join_channel | exact |
| 0x4ae120 | irc_get_channel_list | likely |

20 functions: 11 exact, 9 likely.

Facts:
- TCP to pxo_chat_server port 10555, non-blocking; USER "user user user :<info>" and NICK once the socket is
  writable; logged in after RPL_ENDOFMOTD (376). 433 (nick in use) retries the asked-for nick plus a counter.
- The lobby polls three things: irc_get_chat_text (lines for the chat box; MOTD lines start "!MOTD$#!",
  the end "!EMOTD$#!"), irc_get_channel_list (after 323, "$name users topic " per channel) and
  irc_get_chat_command (IRC_CMD_* records: user joined / left, disconnected, kicked, nick changed, the
  channel "#autoselect" resolved to).
- Slash commands: /msg, /me, /xyz (raw), /list; any other is sent raw only with quiet set, else
  "Unrecognized command" (245). Our own messages are echoed through the parser (mode 1).
- Shown: channel "[nick] text", private "<nick> text", actions "* nick text", CTCP, kicks, nick changes,
  401. Not shown: joins (formatted, NULL returned), parts, quits, topics, whois replies (stored only).

## multispell.cpp (0x4afca0-0x4b0957; no .data; .bss 0x247e824-0x247e82f)

| Address | Function | Confidence |
|---|---|---|
| 0x4afca0 | mp_write_spell_target_point | likely (temporaries) |
| 0x4afd10 | mp_read_spell_target_point | exact |
| 0x4afd60 | mp_write_spell_targets | likely (jump table 0x4aff80) |
| 0x4affb0 | mp_read_spell_targets | likely (jump table 0x4b0100) |
| 0x4b0130 | mp_send_summon_deny | exact |
| 0x4b0180 | mp_send_cast_anim | exact |
| 0x4b0210 | mp_handle_cast_anim | likely |
| 0x4b02a0 | mp_spell_cast_sync | likely |
| 0x4b04a0 | mp_handle_release_spell | likely |
| 0x4b0640 | mp_handle_summon_deny | exact |
| 0x4b0660 | mp_broadcast_summon_allowed | exact |
| 0x4b06a0 | mp_handle_summon_allowed | exact |
| 0x4b06b0 | mp_send_summon_request | exact |
| 0x4b0700 | mp_handle_summon_request | exact |
| 0x4b0730 | mp_summon_claim_reset | exact |
| 0x4b0750 | mp_summon_try_claim | exact |
| 0x4b07d0 | mp_broadcast_summon_handle | exact |
| 0x4b0820 | mp_handle_summon_handle | exact |
| 0x4b0850 | mp_summon_map_handle | exact |
| 0x4b0870 | mp_send_summon_quick | exact |
| 0x4b08f0 | mp_handle_summon_quick | exact |
| 0x4b0940 | mp_summon_claim_release | exact |

22 functions: 16 exact, 6 likely.

Facts:
- RLS_SPL (0x35): caster net handle, spell, target net handle, layer + point or -1, item_types index or -1,
  source type, the extra point (spells 0x1d / 0x1f), the three fields of the params' first word, the targets
  by mode. The receiver calls spell_cast_launch_effects with the rebuilt request; the host forwards.
- One summon per game: the host owns mp_summon_owner; SMN_RQS -> SMN_DNY ("Another summon already active",
  238, and the caster goes idle) or SMN_ALD to all. SMN_HND maps the summon's net handle, SMN_QCK dismisses.

## Game bugs noticed

- **Gold highlight colour** (world_tint_gold_object 0x43a62e): the blue component is read from 0x594430, one
  float past the three-float tint (the literal "gold", about 1.7e22 as a float). The 0.51 at 0x59442c is read
  nowhere. The loot highlight of monster_drop_loot gets an absurd blue (clamped by the renderer, if at all).
- **IRC kick reason** (chat_process_irc_message 0x4acedb-0x4acf18): the reason pointer is irc_word_buf from
  irc_get_word(2), overwritten by the next irc_get_word(1); the "(reason)" of "*** %s has kicked %s from
  channel %s (%s)" shows the kicked nick.
- **IRC connect errors** (irc_connect 0x4abdc0): a connect() failure other than WSAEWOULDBLOCK returns 0 ("in
  progress") without setting irc_connecting, so the lobby retries every frame and never reports it.
- **pick_test_character** (0x43c7ad): the horizontal test compares a squared distance with the plain
  (halved) radius, so the horizontal tolerance is sqrt(radius), larger than the radius for small characters.
- **RLS_SPL handles** (mp_write_spell_targets): only the handle lists of modes 4 and 5 are converted to net
  handles; target[0..1] of the other modes go out as the sender's local values (bug or not depends on what
  those modes hold; open).

## Open questions

- Definition order of the .bss: static or common (all three units define their .bss in address order;
  gametarget's two timers in initialiser order).
- world_mouse_frame's three statics (0x60ab64, 0x60ab78, 0x60ab98) are function-local by their use; only
  0x60ab78 has a guard (a class type), so the other two are plain ints.
- ix_point_linesegment's parameter list (4 here; sum.symbols mentions more outputs); door_find_by_mesh's
  meaning (it is the host's door packet of multi_door.cpp by its use here).
- What skill 0x26's filter is for; what the CST_ANM flag byte and the params' value / kind fields mean.
- 0x247ad08 / 0x247ad2c (two 0x24-byte buffers), 0x247ae20, 0x247ae3c, 0x247ae40 are only cleared.
