# Phase 3: new_buysell.cpp and new_itemtrade.cpp

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata/.data bytes of Sum.exe only (a small
scratch byte reader for the jump tables, button tables and layout tables). No Summoner 2 or Dash Faction
code was read. Both .cpp files pass the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings ...` with no warnings.

Headers: `stubs_menus_dialogue.h`, `ui_screen.h` and `stubs_gamecreate_journal.h` (existing; RealtimeTimer,
gr clip calls, key_to_ascii, mouse_get_delta, MP_PLAYER_NAME, the KEY_B/I/R/BACKSPACE/ENTER codes,
shop_set_shopkeeper_from_char, ui_has_owner_character, statscreen_refresh_anim), `symbols/types.h`
(ItemType, shopkeeper, shop_item, Character, vector3, matrix3, gr__Screen), plus the new
`stubs_buysell_itemtrade.h` (guard STUBS_BUYSELL_ITEMTRADE_H) and the unit headers `new_buysell.h`,
`new_itemtrade.h`. `item_paging.h` and `stubs_assess_pickup.h` are not included: they pull in their own
Color / ItemType / party_inventory definitions that clash with the set above.

## new_buysell.cpp (0x4048b0-0x4063a2; .rdata 0x57e99c-0x57ea1f; .data 0x588e08-0x589822; .bss 0x5b2db8-0x5b6803)

| Address | Function | Confidence |
|---|---|---|
| 0x4048b0 | init_realtimetimer_5b640c_thunk (compiler, no source) | exact |
| 0x4048c0 | init_realtimetimer_5b640c (compiler: `RealtimeTimer buysell_repeat_timer`) | exact |
| 0x4048d0 | buysell_update_button_states | likely (merged tails) |
| 0x404960 | buysell_get_list_item | exact |
| 0x4049c0 | shop_find_item_type | exact |
| 0x404a00 | bs_cameras_parse_table | exact |
| 0x404a20 | buysell_load_bitmaps | exact |
| 0x404bd0 | buysell_init | likely (living-list loop shape) |
| 0x404cc0 | buysell_set_category | exact |
| 0x404dc0 | buysell_update_category_buttons | likely (loop + switch on the index, constant records) |
| 0x404f70 | buysell_category_first_index | exact |
| 0x404fe0 | buysell_update_scrollbar | likely (Q2) |
| 0x4050f0 | buysell_count_party_items | likely |
| 0x4051a0 | buysell_leave | exact |
| 0x4051e0 | buysell_frame | exact |
| 0x405260 | buysell_handle_click | likely (shared key tails) |
| 0x4056b0 | buysell_category_tooltip | exact |
| 0x405790 | buysell_page_up | exact |
| 0x4057c0 | buysell_page_down | exact |
| 0x4057f0 | buysell_buy_item | likely (button array in the argument slot) |
| 0x405900 | buysell_sell_item | exact |
| 0x4059a0 | buysell_button_action | exact |
| 0x405ba0 | buysell_goto_status | exact |
| 0x405bc0 | buysell_goto_inventory | exact |
| 0x405c00 | buysell_scroll_up | exact |
| 0x405c20 | buysell_scroll_down | exact |
| 0x405c50 | buysell_goto_weapons | exact |
| 0x405c90 | buysell_goto_armor | exact |
| 0x405cd0 | buysell_goto_magic_items | exact |
| 0x405d10 | buysell_goto_jewelry | exact |
| 0x405d50 | buysell_goto_misc | exact |
| 0x405d90 | buysell_show_item_info | exact |
| 0x405db0 | buysell_update_buttons | exact |
| 0x405e80 | buysell_drag_scrollbar | likely |
| 0x405f10 | buysell_draw | exact |
| 0x405fd0 | buysell_draw_item_list | likely (locals) |
| 0x406150 | buysell_draw_scrollbar | exact |
| 0x406200 | buysell_draw_item_model | exact |
| 0x406280 | buysell_model_view_begin | exact |
| 0x4062c0 | buysell_model_view_end | exact |
| 0x4062e0 | buysell_render_item_mesh | likely (local order) |
| 0x406360 | shop_set_shopkeeper_from_char | likely (parameter type, see below) |
| 0x406390 | ui_has_owner_character | exact |
| 0x4063a0 | ui_return_true | unsure (Q1) |

44 functions: 32 exact, 11 likely, 1 unsure.

Facts:
- Button table (40 x 0x40, *new* name `buysell_button_defs`): action id = HiLite record + 1. 6 status (screen 2),
  8 journal in single player (10) / inventory page 0 in multiplayer (1), 10 / 12 toggle buy and sell, 14 / 16
  scroll, 19 / 22 / 25 / 27 / 31 jump to weapons / armour / magic / jewelry / misc, 35 buy or sell the
  selected row (no sound of its own, buy/sell play 0x2b or 0x24), 37 item info popup, 39 close. Def flag 4
  (category hot spots) shows the localized category name as a tooltip (L640-644).
- Keys: Esc and B close (0x24), I inventory page 0, Q journal (single player only), R status (0x27). The
  mouse wheel pages the list. Double click on a row buys / sells it at once.
- Selecting a row pages its mesh into item slot 0x12 (`item_page_in(type, 0x12, 0)`), drawn with the
  bs_cameras.tbl camera offset by `buysell_model_view_offset` (0x57e9f0).
- Row colours: highlight when hovered or selected, blue (0x5bd328) unidentified, red (0x5bd954) when
  `character_meets_item_requirements(status_character, type)` fails (never for kind 2), else text.
- buysell_page_up tests `jl` (pos >= 0), unlike the other screens' `> 0`; harmless.
- In sell mode `buysell_category_first_index` returns an index into the shop's buy list, but the rows are
  only the owned items on that list, so the category jump buttons can scroll to the wrong row in sell mode
  (behaviour of the binary, not a decomp error).
- .bss holes: 0x5b2dbc (4 bytes between buysell_scroll_pos and buysell_cameras, never referenced; probably
  alignment of the camera table). `buysell_cameras` is 0x3628 bytes (same as status_cameras).

## new_itemtrade.cpp (0x419170-0x41a932; .rdata 0x57fe40-0x57ff23; .data 0x58cda0-0x58d4b3; .bss 0x5c1570-0x5c1881)

| Address | Function | Confidence |
|---|---|---|
| 0x419170 | init_realtimetimer_5c1858_thunk (compiler) | exact |
| 0x419180 | init_realtimetimer_5c1858 (compiler: `RealtimeTimer itemtrade_repeat_timer`) | exact |
| 0x419190 | init_realtimetimer_5c1860_thunk (compiler) | exact |
| 0x4191a0 | init_realtimetimer_5c1860 (compiler: `RealtimeTimer itemtrade_cursor_timer`) | exact |
| 0x4191b0 | itemtrade_load_bitmaps | likely |
| 0x419380 | itemtrade_init | likely |
| 0x419420 | itemtrade_update_scrollbar | likely (Q2) |
| 0x419530 | itemtrade_count_party_rows | exact |
| 0x419550 | itemtrade_leave | exact |
| 0x419560 | itemtrade_frame | exact |
| 0x419610 | itemtrade_update_trade_buttons | likely (case order) |
| 0x419670 | itemtrade_update_scroll_buttons | exact |
| 0x419710 | itemtrade_handle_click | likely (gold input tails, byte compare) |
| 0x419bb0 | itemtrade_party_page_up | exact |
| 0x419be0 | itemtrade_party_page_down | exact |
| 0x419c10 | itemtrade_cancel | exact |
| 0x419c20 | itemtrade_submit_gold | exact |
| 0x419c40 | itemtrade_clear_gold_input | exact |
| 0x419c50 | itemtrade_button_action | exact |
| 0x419dc0 | itemtrade_list1_scroll_up | exact |
| 0x419de0 | itemtrade_list1_scroll_down | exact |
| 0x419e10 | itemtrade_list2_scroll_up | exact |
| 0x419e30 | itemtrade_list2_scroll_down | exact |
| 0x419e60 | itemtrade_party_scroll_up | exact |
| 0x419e80 | itemtrade_party_scroll_down | exact |
| 0x419eb0 | itemtrade_accept | exact |
| 0x419ec0 | itemtrade_update_buttons | exact |
| 0x419f90 | itemtrade_drag_scrollbar | likely |
| 0x41a020 | itemtrade_list1_handle_mouse | exact |
| 0x41a0c0 | itemtrade_party_handle_mouse | likely (duplicated mode reset) |
| 0x41a1d0 | itemtrade_get_party_row | exact |
| 0x41a200 | itemtrade_begin_gold_input | exact |
| 0x41a230 | itemtrade_draw | exact |
| 0x41a360 | itemtrade_draw_list1 | likely |
| 0x41a490 | itemtrade_draw_list2 | likely |
| 0x41a5a0 | itemtrade_draw_party_list | likely |
| 0x41a700 | itemtrade_draw_scrollbar | exact |
| 0x41a7b0 | itemtrade_draw_gold_input | likely |
| 0x41a8b0 | itemtrade_get_panel_width | exact |
| 0x41a8c0 | itemtrade_mouse_in_panel | exact (asm.mjs splits it at 0x41a8f0, which is not a function start) |

40 functions: 29 exact, 11 likely, 0 unsure.

Facts:
- Button table (28 x 0x40 at 0x58cda0; sum.symbols' `itemtrade_button_defs` 0x58cddc is the flags of entry 0,
  as with gamecreate). Ids: 2 / 4 list 1, 6 / 8 list 2, 10 / 12 party list (all scroll functions update the
  party scrollbar), 16 accept (`mp_send_it_okay`, 0x23), 18 cancel trade and 20 close (`mp_trade_cancel`,
  0x24), 23 gold OK (submit + close box, 0x23), 25 / 27 gold cancel / close (0x24). Def flag 8 marks the gold
  box records 21-27: drawn after the lists and clickable only while the box is open.
- Records 13 / 14 ("MultiTrade-Ready") light while mp_trade_state is 3 (we accepted; accept hot spot 15 is
  disabled) or 4 (the partner accepted). Offer list arrows are disabled while the list is shorter than its
  3-row page.
- Party row 0 is a pseudo entry {party_gold copied at init, &itemtrade_gold_item (type = "Gold")} while
  party_gold > 0; clicking it opens the gold box. Digits build the amount (at most min(party_gold, 9999999)),
  Backspace divides by 10, Enter submits (`mp_send_it_rmvi(Gold)` then `mp_send_it_addi(&gold_item, amount)`),
  Esc closes. The cursor "|" blinks every 500 ms. Clicking an item row offers one more unit while the
  count not yet offered is > 0; clicking a list 1 row takes the offer back (`mp_send_it_rmvi(type)`).
- The gold row's count is the init-time snapshot of party_gold, not the live value.
- `itemtrade_gold_input_size` (0x57ff0c) holds {154,20}, {303,25}, {246,32}: the 800x600 width 303 looks like
  a data slip (it would be about 192), but it is what the binary has.
- .bss hole 0x5c1844 (4 bytes after itemtrade_gold_item, never referenced). mp_send_it_addi reads the gold
  item's +8 (charges), so itemtrade_gold_item is at least 12 bytes (0x5c1838-0x5c1843), not int[2].

## Wrong or incomplete shared declarations (not changed)

- `src/decomp/game/item_paging.h` / `item_paging.cpp`: `int item_page_in(int slot, int type)` has the
  parameters swapped. 0x46c940 compares its second argument with 17 and passes the first to
  item_type_page_in; every call site (buysell, inventory, items_level_load) pushes (type, slot, 0). The third
  argument is never read by the callee. Declared here as `item_page_in(int type, int slot, int unused)`.
- `stubs_gamecreate_journal.h`: `shop_set_shopkeeper_from_char(void *character)` is really a Character*
  (it reads the name at +0x2f4); kept as void* here so the declarations match.
- `symbols/types.h`: ItemType `+0x10 _unk10` is the paged mesh pointer (buysell_draw_item_model, item_paging's
  `type->mesh`). matrix3 is a C struct but matrix3__make_identity is a thiscall member (declared on the
  wrapper `matrix3_t` in the stub header).
- `stubs_assess_pickup.h`: `party_inventory` as `int[521][2]` and its `item_info`; here it is
  `party_inventory_entry {count, inv_item *}` with `inv_item {?, ItemType *type, int charges}`. Merge needed.
- sum.symbols comments to fix: `ui_tooltip_set` is (x, y, w, h, text), not (x, y, text);
  `itemtrade_submit_gold` / `itemtrade_draw_gold_input` describe 0x5c1838 / 0x5c1878 as "gold text, length",
  but they are the gold pseudo item and the typed amount; `buysell_info_item` (0x5b6800) is the pressed
  button index (button_action(id + 1), update_buttons); `buysell_list_entries` is a `shop_item*` and
  `buysell_shop` a `shopkeeper*`; `living_list_first` is `living_list.live_next`; `itemtrade_gold_item` is not
  int[2] (above).

## Open questions

- Q1: 0x4063a0 `ui_return_true` (mov al, 1 / ret) is the identical-code-folded mouse-in-panel at the end of
  this unit; its original name (buysell_mouse_in_panel?) and parameters are guesses.
- Q2: buysell_update_scrollbar / itemtrade_update_scrollbar test the row count (> 0) for the proportional
  thumb, where pickup tests the scroll position; the count is always above the page there, so the simple
  branch is dead. Source spelling unknown.
- Q3: `mp_trade_peer` (mp_trade_partner): only +0xc (id) and +0x14 (an mp_player, name at +0x34) are known.
- Q4: `gr_light_desc` +0x14 = 0.15f: which light parameter it is (the ctor builds a vector3 at +0xc).
- Q5: buysell_update_category_buttons: the loop over 40 records with a switch on the index may have been
  written with `buysell_buttons[i]` / `[i + 1]`; VC6's code uses constant addresses either way?
