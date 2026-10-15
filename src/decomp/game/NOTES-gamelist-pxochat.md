# Phase 3: new_gamelist.cpp and new_pxochat.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata / .data bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include` with no warnings.
All headers of the copied tree still compile together, and every existing pilot .cpp of src/decomp still
compiles against this include tree with no errors. Header changes: CHANGED-HEADERS.txt; decisions D57, D58
in include/DECISIONS.md.

## new_gamelist.cpp (0x40a660-0x40bf2b; .rdata 0x57ecc8-0x57edcf; .data 0x58a7a0-0x58ac47; .bss 0x5bc710-0x5bc918)

| Address | Function | Confidence |
|---|---|---|
| 0x40a660 / 0x40a670 | init_realtimetimer_5bc714 pair (compiler: `RealtimeTimer gamelist_repeat_timer`, entry 13) | exact |
| 0x40a680 / 0x40a690 | init_realtimetimer_5bc718 pair (gamelist_cursor_timer, entry 14) | exact |
| 0x40a6a0 | gamelist_load_bitmaps | exact |
| 0x40a850 | gamelist_init | likely |
| 0x40a880 | gamelist_frame | likely (join-result switch, case 4) |
| 0x40a970 | gamelist_update_join_button | likely |
| 0x40a9b0 | gamelist_show_join_error | likely |
| 0x40ab80 | gamelist_update_scrollbar | likely |
| 0x40ac90 | gamelist_password_input_begin | exact |
| 0x40acc0 | empty_frame_hook_40acc0 | exact |
| 0x40acd0 | gamelist_handle_click | likely |
| 0x40b0b0 | mp_server_list_get | likely |
| 0x40b0e0 | gamelist_refresh_tooltip | exact |
| 0x40b120 | gamelist_page_up | likely |
| 0x40b150 | gamelist_page_down | likely |
| 0x40b180 | gamelist_join_selected | exact |
| 0x40b1b0 | gamelist_back | exact |
| 0x40b1d0 | gamelist_password_input_cancel | exact |
| 0x40b1e0 | gamelist_button_action | likely |
| 0x40b340 | gamelist_scroll_up | exact |
| 0x40b360 | gamelist_scroll_down | exact |
| 0x40b390 | gamelist_refresh | exact |
| 0x40b3b0 | gamelist_create_game | exact |
| 0x40b3c0 | gamelist_create_game_cb | exact |
| 0x40b3d0 | gamelist_update_buttons | exact |
| 0x40b4a0 | gamelist_drag_scrollbar | likely |
| 0x40b530 | gamelist_draw | exact |
| 0x40b5c0 | mp_server_list_draw | likely |
| 0x40bca0 | gamelist_draw_scrollbar | likely |
| 0x40bd50 | gamelist_draw_password_input | likely |
| 0x40be80 | mp_draw_join_help | likely |

29 functions + 2 initialiser pairs: 14 exact, 15 likely, 0 unsure.

Facts:
- 18 buttons. Flags: 1 HiLite acting on release through the next entry, 2 acts on press with auto-repeat (100 ms
  after a 500 ms delay), 4 password popup entries (react and draw only while it is open), 8 tooltip (Refresh,
  L800). Join (7) needs a selection (the first game is selected automatically); arrows (1 / 3) need more than
  one page (20 rows).
- mp_join_result: 1 pending (input ignored, "Joining game..." box), 4 opens the password popup (or shows
  "Incorrect password." when it is already open), 3 and 5..13 show L788..L798 in a "Join failure" box.
  Codes 9..13 go through sprintf(buf, text) with no arguments.
- Create warns modem hosts (nw_network_status 1 / 2: L801 / L802), mouse mode 3 until the box closes.
  Back: state 11 (PXO chat) with pxo_cfg_flags bit 0, else state 1. Create: state 13.
- Row colours: closed game red; hovered / selected highlight; else red for level not unlocked
  (mp_player_level_available), full game, locked difficulty (profile bit 1 << (difficulty + 25)) or character
  level outside min..max. Ping is capped at "9999".
- Game list entry (0x50 bytes) laid out in multi.h as mp_gamelist_entry (fields from mp_handle_actv_gm).

## new_pxochat.cpp (0x421e70-0x42540a; .rdata 0x580478-0x580603; .data 0x58f720-0x590067; .bss 0x5c3c60-0x5c595b)

| Address | Function | Confidence |
|---|---|---|
| 0x421e70 | pxochat_open_banner_url | likely |
| 0x421ea0 | pxo_find_chat_banners | likely |
| 0x421fa0 | pxo_load_chat_banners | likely |
| 0x422370 | pxochat_leave | exact |
| 0x4223d0 | pxo_parse_chat_banners_cfg | likely |
| 0x422790 | pxochat_banner_has_url | exact |
| 0x4227c0-0x422810 | three init_realtimetimer pairs (chat cursor, channel cursor, button timer; entries 63-65) | exact |
| 0x422820-0x422970 | eleven init_strptrs pairs (popup texts as `char *&` to localize_strings, entries 66-76) | likely |
| 0x422980 | pxochat_build_popup_text | likely |
| 0x422b40 | pxochat_add_typed_channel | likely |
| 0x422cf0 | pxochat_join_typed_channel | exact |
| 0x422d10 | pxochat_input_cancel | exact |
| 0x422d30 | pxochat_join_selected_channel | exact |
| 0x422d70 | pxochat_update_banner_cursor | exact |
| 0x422dd0 | pxochat_load_bitmaps | likely |
| 0x422fc0 | pxochat_init | likely |
| 0x423050 | pxochat_update_scrollbar | likely |
| 0x4231a0 | pxochat_leave_thunk | likely |
| 0x4231b0 | pxochat_update_banner | exact |
| 0x423220 | pxochat_frame | likely |
| 0x423370 | pxochat_checking_version_popup_frame | likely |
| 0x4233d0 | pxochat_version_error_popup_frame | likely |
| 0x423420 | pxochat_new_version_popup_frame | unsure (Q2) |
| 0x4234a0 | pxochat_validating_popup_frame | exact |
| 0x4234f0 | pxochat_login_error_popup_frame | exact |
| 0x423530 | pxochat_version_check_frame | likely |
| 0x4236c0 | pxochat_login_frame | likely |
| 0x4238c0 | pxochat_update_join_buttons | exact |
| 0x423920 | pxochat_escape_leaves | likely |
| 0x423950 | pxochat_update_status_text | likely |
| 0x423a20 | pxochat_handle_click | likely |
| 0x424120 | pxochat_page_up | likely |
| 0x424170 | pxochat_page_down | likely |
| 0x4241c0 | pxochat_goto_charselect | exact |
| 0x4241d0 | pxochat_button_action | likely |
| 0x424320 | pxochat_input_begin | likely |
| 0x424360 | pxochat_scroll_up | exact |
| 0x424390 | pxochat_scroll_down | exact |
| 0x4243e0 | pxochat_goto_gamelist | exact |
| 0x424400 | pxochat_show_motd | exact |
| 0x424410 | pxochat_update_buttons | likely |
| 0x4244f0 | pxochat_drag_scrollbar | likely |
| 0x4245b0 | pxochat_on_channel_joined | exact |
| 0x424600 | pxochat_draw_channel_input | likely |
| 0x4246d0 | pxochat_draw | likely |
| 0x4247d0 | pxochat_draw_banner_highlight | exact |
| 0x4248c0 | pxochat_draw_channel_list | likely |
| 0x424a60 | pxochat_draw_user_list | likely |
| 0x424b80 | pxochat_draw_chat_lines | likely |
| 0x424e00 | pxochat_draw_scrollbar | likely |
| 0x424ec0 | pxochat_draw_chat_input | likely |
| 0x425010 | pxochat_draw_caption | exact |
| 0x425080 | pxochat_sync_channel_list | likely |
| 0x425110 | pxochat_sync_user_list | likely |
| 0x4251a0 | pxochat_append_motd | likely |
| 0x425250 | pxochat_show_motd_thunk | likely |
| 0x425260 | pxochat_clear_motd | exact |
| 0x425270 | pxochat_add_chat_line | likely |
| 0x4253e0 | pxochat_reset_lists | exact |

57 functions + 14 initialiser pairs: 20 exact, 36 likely, 1 unsure. Nothing left.

Facts:
- Banner: registry ChatBanners != 0 refetches PXO_BANNER_BASE_URL/new_chatbanners.cfg into multicache (state
  machine 0..6 in pxo_load_chat_banners, InetGetFile), picks one of up to 10 $Pcx_name entries at random,
  downloads it, bm_load. **Bug kept:** the +Link: value is parsed into a temporary buffer and never stored, so
  pxo_chat_banner_link_url stays empty and the banner is never clickable (banner_has_url always false).
- Version check (version_check_run 2 / 3 / 4 → popup states 5 / 4 / 3), then login (pxo_login_step 1 / 2 / 3 /
  4 → states 7 / 9 / 6 / 8). "Yes" on the new-version popup: state 3 with pxo_run_updater_on_exit = 1.
- 34 buttons: three lists (channels 0, users 1, chat 2) with arrows 1-12, Join channel (13, needs a
  selection), Join private (15, opens the channel-name popup 27-33), Game list (17, needs a joined channel,
  state 12), MOTD (19), Exit (21) and the 1024-only close (23) to state 10, banner (25, its own hit area).
- Typed channels become "+name" and reuse the first '+' slot of lobby_channels or are appended
  (lobby_flags bit 8 makes pxochat_join_selected_channel join the typed one).
- Chat lines (64 x 0x44, type + 64 chars): 0/5 speaker lines (speaker blue / highlighted), 1/6 their
  continuations, 2 MOTD gold, 3 magenta, 4 green. MOTD lines arrive as "!MOTD$#!" + separator + text.
- The 640 entries of pxochat_channel_users_right / pxochat_channel_count_x ({211,108,152}, {223,117,165}, .data)
  and pxochat_channel_input_pos ({285,253},{230,316},{294,407}) are larger than the 800 ones; kept as stored.

## Open questions

1. **gamelist_frame case 4** pushes the constant 4 into the default case's show_join_error call; written as
   show_join_error(result) folded by VC6. A literal `gamelist_show_join_error(4)` would give the same code.
2. **pxochat_new_version_popup_frame**: for choices > 0 the binary compares with 1 and then calls
   gameseq_set_state(1) unconditionally; the source condition is unknown. pxochat_version_error_popup_frame
   has a compare with 0 that no branch uses (an empty if?).
3. **Unused globals** 0x5c3e97 (bool, set by pxochat_input_begin) and 0x5c5930 (int, cleared by the two login
   popups) are never read in this unit; named pxochat_channel_input_unk / pxochat_login_unk_5c5930 (*new*).
4. **pxochat_update_buttons' state index** (`button + (pressed == 25 ? -1 : 0)`) is a guess at the source
   that gives "xor ecx,ecx / cmp / or ecx,-1 / lea [ecx+edi]".
5. **pxochat_open_banner_url** tests the array address for NULL (not folded): the source probably went
   through a char* (a parameter of an inline helper?).
6. **.bss order**: the timers and string references are defined in their initialiser order (entries 63-76), but
   their .bss addresses are not in that order (0x5c423c, 0x5c57d0, 0x5c426c; strptrs at 0x5c5938.., 0x5c57cc,
   0x5c3e9c). Same open question as new_help.cpp Q4.
7. **Unit split**: open_url (0x421de0) is new_pxo_banner.cpp (only declared here); the banner functions
   are tagged new_pxochat.cpp and their data sits inside new_pxochat's .data / .bss, so they stay here.
