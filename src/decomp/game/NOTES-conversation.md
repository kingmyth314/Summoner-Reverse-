# Phase 3: conversation.cpp, conversationctl.cpp (source half, decomp20)

Written from Sum.exe's disassembly only (`node tools/asm.mjs fn`, raw .data / .rdata bytes). No Summoner 2 or
Dash Faction code was read. Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>` with no
warnings. The header tree is the repo's `src/decomp/include` as of 9c82c5a (re-based on it with
`git merge-file` after the repo moved during this work) plus the changes in CHANGED-HEADERS.txt; every
other pilot .cpp of the repo gives the same diagnostics with this tree as with the repo's.

All 76 functions of the two ranges are accounted for: 72 written, plus the four inline-header COMDATs
(0x4d8f30, 0x4d8f40, 0x4d9a80, 0x4da830) which are defined in camera.h / vector.h / living_entity.h.
Nothing is left.

## conversation.cpp (0x4d7b30-0x4d87ce; .data 0x5a0250-0x5a02cb; .bss 0x25461a0-0x25464af)

32 functions: 22 exact, 10 likely, 0 unsure.

| Address | Function | Confidence |
|---|---|---|
| 0x4d7b30 / 0x4d7b40 / 0x4d7b50 / 0x4d7b60 | static_init / init / register_atexit / atexit_dtor of conversation_db (from the definition) | exact |
| 0x4d7b70 | conversation__dtor (implicit: the pool's destructor) | exact |
| 0x4d7b80 | conversation__ctor | likely (list head store order) |
| 0x4d7c20 | conversation_alloc_storage | exact |
| 0x4d7c70 | conversation_clear | exact |
| 0x4d7ca0 | conversation_free | exact |
| 0x4d7cc0 | conversation_add_speaker | exact |
| 0x4d7ce0 | conversation_alloc_speaker | exact |
| 0x4d7d10 | conversation_alloc_topic | likely (`listed == true`) |
| 0x4d7d50 | conversation_set_speaker | likely (store scheduling) |
| 0x4d7e10 | conversation_clear_speaker | exact |
| 0x4d7e40 | conversation_set_topic | exact |
| 0x4d7e90 | conversation_select_greeting | exact |
| 0x4d7eb0 | conversation_apply_topic_effects | likely |
| 0x4d80c0 | conversation_has_speaker | exact |
| 0x4d8100 | conversation_has_speaker_hash | exact |
| 0x4d8140 | conversation_first_topic | exact |
| 0x4d8170 | conversation_next_topic | exact |
| 0x4d81a0 | conversation_add_name | likely (increment form) |
| 0x4d81d0 | conversation_find_name | exact |
| 0x4d8220 | conversation_set_topic_available | exact |
| 0x4d82b0 | conversation_find_speaker | exact |
| 0x4d82f0 | conversation_load | likely |
| 0x4d8390 | conversation_save_state | exact |
| 0x4d83a0 | conversation_save_topics | likely |
| 0x4d84d0 | conversation_restore_state | likely (if / else per bit) |
| 0x4d8620 | conversation_build_topic_list | likely (list_insert store order) |
| 0x4d86f0 | conversation_remove_topic_entry | exact |
| 0x4d8760 | conversation_farewell | likely |

## conversationctl.cpp (0x4d87d0-0x4db248; .data 0x5a02cc-0x5a0451; .bss 0x25464b0-0x254675f)

40 functions: 23 exact, 16 likely, 1 unsure (plus ~conversation_slot, folded into empty_function).

| Address | Function | Confidence |
|---|---|---|
| 0x4d87d0 / 0x4d87e0 / 0x4d87f0 / 0x4d8800 | static_init / init / register_atexit / atexit_dtor of conversation_active | exact |
| 0x4d8810 | conversation_controller__dtor (implicit) | exact |
| 0x4d8880 | conversation_controller__add_talker | exact |
| 0x4d8900 | conversation_controller__topic_talker | exact |
| 0x4d8940 | conversation_controller__line_talker | exact |
| 0x4d8a90 | conversation_update_talk_anims | likely |
| 0x4d8ca0 | conversation_slot__ctor | exact |
| 0x4d8cc0 | conversation_slot__reset | exact |
| 0x4d8ce0 | conversation_controller__ctor | likely |
| 0x4d8db0 | conversation_open_dialogue | likely |
| 0x4d8e10 | conversation_camera_capture | likely (inline transform helper is a guess) |
| 0x4d8f00 | dialog_camera_pose__set (inline in conversation.h) | likely |
| 0x4d8f50 | conversation_camera_compute_shot | unsure (expression forms) |
| 0x4d9aa0 | dialog_camera_pose__assign (inline in conversation.h) | likely |
| 0x4d9ad0 | conversation_camera_blend | likely |
| 0x4d9d90 | conversation_camera_update | likely |
| 0x4da010 | conversation_frame | likely |
| 0x4da860 | character_has_ai_goal_10 | exact |
| 0x4da890 | conversation_camera_begin | likely |
| 0x4daa70 | conversation_end_cleanup | likely |
| 0x4dabb0 | conversation_choose_topic | exact |
| 0x4dabf0 | conversation_request_end | exact |
| 0x4dac10 | conversation_reset_state | exact |
| 0x4dac20 | conversation_is_idle | exact |
| 0x4dac30 | conversation_end | exact |
| 0x4dacb0 | conversation_select_topic | likely |
| 0x4daef0 | conversation_load_for_level | likely |
| 0x4daf50 | conversation_begin | exact |
| 0x4dafe0 / 0x4daff0 | static_init / init of ccmd_dialogue_make_topic_available | exact |
| 0x4db010 | cmd_dialogue_make_topic_available | likely |
| 0x4db0d0 / 0x4db0e0 | static_init / init of ccmd_dialogue_make_topic_unavailable | exact |
| 0x4db100 | cmd_dialogue_make_topic_unavailable | likely |
| 0x4db1c0 / 0x4db1d0 | static_init / init of ccmd_dcamera | exact |
| 0x4db1f0 | cmd_dcamera | likely |

## Findings

- conversation_db is the conversation *file* plus state (class `conversation`); conversation_active is the
  controller (class `conversation_controller`, 0x27c bytes). The byte at 0x254672c after it is a separate
  bool of conversation_frame (DECISIONS.md C1, C4).
- +0x204 / +0x208 are the speaker and the topic (C2). Speaker flags are the Cflag_* word (1 NoCameraMovement,
  2 DontFacePlayer, 4 NoAbort, 8 Important, 0x10 Persistent); topic bits 1 listed, 2 farewell, 3 initial,
  4 fresh, 5 hidden, 6 shop, 0 saved but not read here.
- Savegame: per speaker one bit (Important), per topic four (bit 0, initial, hidden, is-hail-topic);
  Persistent speakers propagate (savegame.md's "exact bit meanings unknown" can be filled in).
- The dialog camera option (Summoner_options.dialogue_camera, `dcamera`): 1 or (2 and the speaker not
  Important) or Cflag_NoCameraMovement keep the game camera.
- Game bug candidates (kept in the code, commented): (1) conversation_apply_topic_effects' inner
  +Remove initial topic: loop advances with the outer entry's next pointer, so it never ends when the
  removed topic is the hail topic, the topic has +Add initial topic: entries and the remove entry is not
  the last; (2) conversation_camera_compute_shot never sets side.y before normalising it (stack garbage).
- The three console commands are placed in conversationctl.cpp (C7).

## Open questions

- Topic flag bit 0 (saved as topic bit kind 0): who sets it (not this unit, not the parser's visible part).
- char_info flags 0x1000 / 0x2000 / 0x4000 (camera framing for big speakers): names unknown.
- ai goal types 9 and 10 (goal_find(1, 9 / 10, 0, 0, -1, 0, 0, 0x6c)) and the meaning of goal_find's last six
  arguments; status 8 cured by conversation_begin (and status_effect_update_fx(handle, 6)).
- dialogue_line +0x14 (nonzero stops the select_topic scroll search from moving to the next line).
- Whether vector3's operators are inline in the original header (the binary expands some and calls others
  in one function, DECISIONS.md C3); decides several "likely" ratings once VC6 is available.
- 0x254672c: file-scope static or function-local static of conversation_frame.
