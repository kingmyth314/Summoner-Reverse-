# decomp50: localize, vsdk\net, vsdk\os

Written from Sum.exe v1.40's disassembly only (`node tools/asm.mjs fn|callers|grep|strings`, plus raw .data /
import-table reads of Summoner/Sum.exe). Summoner 2 was checked for file names only (it has none of these
functions except timer_wait, which it keeps in timer.cpp). Header base: src/decomp/include at commit 11f4aef
(the include tree was re-copied once because the repo moved from 917a09d to 11f4aef during the work; decomp49's
os_foreground declaration is kept). Header decisions D3000-D3009 are in include/DECISIONS.md.

Totals: 75 functions of the three modules (29 + 25 + 21). 72 written (41 exact / 31 likely / 0 unsure), plus 8
compiler-generated or unnamed helpers of these objects; the 3 Mempool functions of vsdk\os are header inlines
already defined in vsdk/os/memory.h (D3008).

## Translation units

| File (all placeholders) | Code | .data / .bss | Evidence |
|---|---|---|---|
| `vsdk/unnamed_localize_0x503f60.cpp` | 0x503f60-0x505547 | .data 0x5a5c30-0x5a6057, .bss 0x25c68b0-0x25d889b | Starts with the three static initialisers (table 0x5887e0-0x5887e8) of the unit's pools; ends with the two COMDATs it calls (0x505530, 0x505540); gr.cpp's initialisers start at 0x505550. Literals in order of use. No path string, no strings.tbl block, no Summoner 2 file |
| `vsdk/unnamed_net_0x517590.cpp` | 0x517590-0x5194ab | .data 0x5a7dcc-0x5a8483, .bss 0x26b63f0-0x2710ebb | After movie_blit_frame (movie .bss 0x26b63e4), before character_instance's initialiser 0x5194b0 (.data from 0x5a8484). Messages name psnet_send, nw_WorkReliable, nw_SendReliable, nw_ReceiveReliable, nw_ConnectToServer but no file. No path string, no strings.tbl block |
| `vsdk/unnamed_os_0x530b90.cpp` | 0x530b90-0x531bea | .bss 0x2cbfce0-0x2cbff27 | After the vsdk\ui inlines; one object although 0x5312c0.. is unoptimised: ccmd_sysinfo 0x2cbff00 sits inside the os .bss, nop (compiler) padding after os_seed_random, int3 (linker) gap only at 0x531beb (D3007). timer_wait 0x530bf0 lies inside |
| `vsdk/unnamed_os_0x531bf0.cpp` | 0x531bf0-0x531c0f | none | The two -1 stubs after the int3 gap; optimised; may share an object with stricmp_wrapper / the debug stubs (module -) that follow |

No unit has file-name evidence, so modules.txt has no lines.

Build: localize is /O2 /Ob0 (it calls the out-of-line empty Mempool() and Mempool::ResetOffset), intrinsic
str*/mem*, no string pooling (duplicate "INVALID", "#end", ","). net and os are /O2; the os object's last part
(CPU code, sysinfo, clipboard) is unoptimised, written as `#pragma optimize("", off)` before
os_clipboard_get_text.

## Functions

### localize (29 + 7 helpers; 20 exact / 9 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x503f60 / 0x503f70 | init_localize_pool_non_level thunk / body (Mempool()) | exact (helper) |
| 0x503f80 / 0x503f90 | init_localize_pool_level thunk / body | exact (helper) |
| 0x503fa0 | init_localize_string_pool | exact (helper) |
| 0x503fb0 | localize_init_string_pool (StringPool(0xa800)) | exact |
| 0x503fc0 / 0x503fd0 | localize_string_pool_register_dtor / localize_string_pool_dtor | exact (helpers) |
| 0x503fe0 | localize_unescape | likely |
| 0x504070 | localize_strdup | exact |
| 0x5040e0 | localize_table_add | likely |
| 0x504190 | localize_set_string_raw | exact |
| 0x5041b0 | localize_table_lookup | exact |
| 0x504200 | localize_hash | exact |
| 0x504230 | localize_load_strings | likely |
| 0x504400 | localize_set_string | likely |
| 0x5044d0 | localize_parse_string_line | exact |
| 0x504520 | localize_load_names | likely |
| 0x5047f0 | localize_load_non_level | exact |
| 0x504830 | localize_add_if_missing | exact |
| 0x504870 | localize_load_level_table | likely |
| 0x504ed0 | localize_clear_level | exact |
| 0x504f10 | localize_level_init | exact |
| 0x504f60 | localize_init | likely |
| 0x505020 | localize_topic_name | exact |
| 0x505040 | localize_item_name | exact |
| 0x505060 | localize_stub_ret0 | likely |
| 0x505070 | localize_quest_name | exact |
| 0x505090 | localize_script_name | exact |
| 0x5050b0 | localize_set_language | exact |
| 0x505120 | localize_get_language_name_buf | exact |
| 0x505160 | localize_get_language_code_buf | exact |
| 0x5051a0 | localize_remap_charset | likely |
| 0x5054e0 | localize_get_string_ptr | exact |
| 0x5054f0 | localize_string_or_invalid | exact |
| 0x505510 | localize_get_language_name | exact |
| 0x505530 / 0x505540 | Mempool() / Mempool::ResetOffset COMDATs (memory.h inlines) | comment only |

### vsdk\net (25; 11 exact / 14 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x517590 | nw_buffer_receive | likely |
| 0x5175e0 | nw_can_write | likely |
| 0x517630 | nw_reliable_slot_status | exact |
| 0x517680 | nw_send_packet | likely |
| 0x517730 | nw_buffer_init | exact |
| 0x517780 | nw_buffer_push | likely |
| 0x517820 | nw_buffer_pop | likely |
| 0x5178c0 | nw_ras_init | likely |
| 0x517ae0 | nw_set_socket_options | likely |
| 0x517bf0 | nw_reliable_reset | exact |
| 0x517c10 | nw_init_socket | likely |
| 0x517d20 | nw_send_reliable_reply | exact |
| 0x517d70 | nw_send_heartbeat | exact |
| 0x517e00 | nw_WorkReliable | likely |
| 0x518770 | nw_receive_all | likely |
| 0x518880 | nw_close | exact |
| 0x5188d0 | nw_init_networking | likely |
| 0x518a30 | nw_addr_to_string | exact |
| 0x518a90 | nw_send_broadcast | exact |
| 0x518ac0 | nw_send | likely |
| 0x518b80 | nw_receive | exact |
| 0x518ce0 | nw_accept_reliable | likely |
| 0x518f30 | nw_ConnectToServer | likely |
| 0x519370 | nw_addr_equal | exact |
| 0x519440 | nw_flush_receive | exact |

Same object, module -, not written: float_compare 0x517dc0 (declared in net.h, nw_WorkReliable's qsort
callback), nw_NewReceiveReliable 0x518bb0, nw_NewSendReliable 0x518d70, nw_NewCloseSocket 0x5191c0,
net_get_reliable_socket_status 0x5193a0, net_get_reliable_socket_ping 0x5193f0.

### vsdk\os (21: 18 written, 10 exact / 8 likely; 3 header inlines) + 1 unnamed helper

| Address | Function | File | Confidence |
|---|---|---|---|
| 0x505540 | Mempool__ResetOffset | memory.h inline (COMDAT in the localize object) | already defined |
| 0x51dff0 | Mempool__Mempool | memory.h inline (COMDAT in effect_mem.cpp) | already defined |
| 0x51e010 | Mempool__getPoolSpaceLeft | memory.h inline (COMDAT in effect_mem.cpp) | already defined |
| 0x530b90 | os_find_window_callback | unnamed_os_0x530b90 | exact |
| 0x530bf0 | timer_wait | unnamed_os_0x530b90 | exact |
| 0x530c00 | os_get_main_wnd | unnamed_os_0x530b90 | exact |
| 0x530c10 | os_foreground | unnamed_os_0x530b90 | exact |
| 0x530c20 | os_add_msg_handler | unnamed_os_0x530b90 | exact |
| 0x530c40 | os_stub_ret_530c40 | unnamed_os_0x530b90 | likely |
| 0x530c50 | os_stub_ret_530c50 | unnamed_os_0x530b90 | likely |
| 0x530c60 | os_set_window_title | unnamed_os_0x530b90 | exact |
| 0x530d10 | os_init | unnamed_os_0x530b90 | exact |
| 0x530e40 | os_find_previous_instance | unnamed_os_0x530b90 | likely |
| 0x530ee0 | os_init_window | unnamed_os_0x530b90 | exact |
| 0x530ef0 | os_create_window | unnamed_os_0x530b90 | likely |
| 0x530ff0 | os_wndproc | unnamed_os_0x530b90 | likely |
| 0x531250 | os_atexit_nop (no sum.symbols entry; name ours, static) | unnamed_os_0x530b90 | likely (helper) |
| 0x531260 | os_close_main_window | unnamed_os_0x530b90 | exact |
| 0x531280 | os_seed_random | unnamed_os_0x530b90 | exact |
| 0x531b44 | os_clipboard_get_text | unnamed_os_0x530b90 (unoptimised part) | likely |
| 0x531bf0 | os_stub_ret_m1_531bf0 | unnamed_os_0x531bf0 | likely |
| 0x531c00 | os_stub_ret_m1_531c00 | unnamed_os_0x531bf0 | likely |

Same object, module -, not written: os_poll 0x530ca0, os_pump_messages 0x530cb0, cpu_read_tsc 0x5312c0,
cpu_identify 0x5312d6, PrintSystemInfo 0x53171f, init_ccmd_sysinfo_entry 0x531a80, init_ccmd_sysinfo 0x531a8a,
cmd_sysinfo 0x531aa8.

## Globals defined (names ours marked)

- localize: localize_pool_used (char *, D3001), localize_language_names, localize_farewell (ours),
  localize_is_english (ours), the three pools, the four tables, localize_level_buffer / non_level_buffer /
  text_buffer (ours), localize_strings, localize_french_punct (module - but in this .bss), localize_is_german
  (ours), localize_language, localize_pools_created (ours), the five load flags, localize_max_string_id,
  localize_current_pool.
- net: nw_server_socket, nw_udp_socket, nw_recv_buffers, nw_connection_speed, nw_network_status (module -),
  nw_flush_buf (ours, 0x2708130), nw_reliable_recv_buf, nw_connect_reply_buf, nw_connect_request,
  nw_reliable_sockets, nw_reliable_resend_time, nw_last_heartbeat_time, nw_networking_ok, nw_socket,
  nw_winsock_started, the three RAS pointers (ours, 0x2710eb0-0x2710eb8).
- os: os_mutex_name, os_msg_handlers, os_window_title, os_critical_section (ours), os_window_class,
  os_prev_instance_found, main_wnd, os_main_thread (ours, 0x2cbff14), os_instance_mutex, is_main_wnd_active,
  os_inited (ours, 0x2cbff20), os_num_msg_handlers.

## Open questions

- The localize file may be game code linked into the vsdk range (it hard-codes Summoner names); its directory
  is unknown either way.
- nw_can_write's `or eax, eax` after `cmp eax, -1` and nw_WorkReliable's unused `cmp eax, 0x258` look like
  expression-only debug macros (written as NW_DEBUG_CHECK); VC6 may drop the written form.
- nw_init_networking: the "none" / unknown-speed exits share one block; written as two returns, an if chain
  with a goto for "fast". A switch on nw_network_status after the chain would also fit.
- nw_receive_all clears from_len before the select (dead store kept by the address-taken local); the source
  may instead initialise another variable there.
- The os object's switch to unoptimised code: `#pragma optimize("", off)` is inferred (D3007); it may also
  have been a separate build rule for part of the file (not possible in one object) or an inline-asm side
  effect (cpu_read_tsc has rdtsc, but os_clipboard_get_text has no asm and is still /Od).
- 0x530c40 / 0x530c50 / 0x505060 / 0x531bf0 / 0x531c00 have no reference: their signatures are guesses.
- 0x2710ea8-0x2710eaf (net .bss) and 0x5a5c34 (localize .data) have no reference.

## Doc disagreements

- docs/sum-exe/localize.md: "28 functions" - 29 in module localize plus 7 initialiser helpers; the range starts
  at 0x503f60 (not 0x503fb0) and ends at 0x505547. The pool names are "loc: non-level" and "loc: level" (the doc
  and sum.symbols write "loc  non level"). The per-level tables are not "topic_names section" / "script_names
  section": they come from topic_names.tbl / script_names.tbl, section `<level>_dlg.tbl` / `<level>_script.tbl`,
  English lines first, then the `#<language>` copy. Each language entry also has a code ("gr", "fr"). French
  remapping is one-way (to_font ignored); German maps both ways but not symmetrically (ß -> 0xbc, 0xe1 -> ß).
- docs/sum-exe/vsdk-os.md "Networking (0x5175e0-0x519370)": the unit is 0x517590-0x5194ab. ConnectionSpeed
  "none" / "slow" / "56k" / "fast" give nw_network_status 0 / 1 / 2 / 3; only slow and 56k probe RAS; any other
  value disables networking like "none". nw_ras_init returns the dial-up IP (0 if none). The reliable layer's
  own names are in its messages (nw_WorkReliable, nw_SendReliable, nw_ReceiveReliable, nw_ConnectToServer).
- docs/sum-exe/vsdk-os.md says nothing about the os window code; controls.md's os_wndproc description matches.
- sum.symbols comments: nw_reliable_slot_status ("connected socket": it tests a packet buffer), nw_send_reliable_reply
  args ("(socket, seq, addr)": (addr, seq, send_time)), nw_send_heartbeat (it sends IAMHERE; the type-5 heartbeat
  is inline in nw_WorkReliable), nw_addr_to_string format ("IP: %s:%d": "IP: %s %d"), nw_recv_buffers
  (uint8[0x10000]: nw_packet_buffer[5], 0x5193c bytes), nw_connect_request (uint8[10]: a full reliable_header of
  which 10 bytes are sent), localize_pool_used (int: char *), os_window_title (char[0x98]: 0x80), the
  localize_pool_* comments ("loc  non level"). docs/sum-exe/multiplayer.md has nothing on this layer.

## Game bugs (all latent; multiplayer is unreachable in retail, the rest needs modded tables or odd setups)

1. nw_WorkReliable keeps `rsocket` from the previous packet: a reliable packet from an unknown address that
   follows one from a known peer is processed as that peer's (0x517e14 sets it once, 0x518088 tests it).
2. A duplicate connection request is acknowledged and marked RNT_IGNORE, but the code then still opens a new
   LIMBO link for the same peer (no type test before the free-slot loop, 0x517fa7).
3. The sequence wrap test adds theirsequence (`add ax, di; add eax, 0x96`, 0x51833c) where the distance needs
   0x10000 - theirsequence, so near the wrap old packets can be accepted again.
4. nw_ConnectToServer: when the IAMHERE send fails the slot is cleared but nw_server_socket and *socket keep the
   index (0x519122-0x51914e).
5. nw_send_heartbeat sends 10 bytes with an uninitialised send_time (0x517d70); pings are int milliseconds sorted
   with the float compare (works only while all are non-negative).
6. localize_load_names / localize_load_level_table: ids / line counts are not checked against the stack tables
   (1000 / 500 ints): a modded table overflows the stack (0x50464c, 0x504acd).
7. localize_load_level_table lowers the section name with `i < section[i]` (compares the index with the
   character, 0x5049bc-0x5049c7): stops early only for names longer than their character codes (harmless).
8. localize_set_string stores the slot pointer and the max id before its room test (0x504417, 0x504439): a full
   text buffer leaves the id pointing at unwritten bytes. localize_table_add allocates the node and the string
   before its duplicate test, leaking both on a duplicate.
9. localize_remap_charset (French) maps Ä 0xc4 to 0x90 (É in CP437) and has no font-to-Windows direction.
10. os_clipboard_get_text returns without CloseClipboard when GetClipboardData or GlobalLock fails, and writes
    buf[max_len] (needs max_len + 1 bytes); it has no caller.
11. os_add_msg_handler has no bound on the 32 handler slots; os_wndproc's SetThreadPriority uses os_main_thread
    (0x2cbff14), which nothing sets (NULL handle, the call fails). os_init calls ReleaseMutex on the handle that
    OpenMutex returned in the second copy (it owns nothing) and never closes it; the "[run] - %s -" window search
    result (os_prev_instance_found) is never read.
12. localize_load_level_table's fixed key "Spirit Of Laharah" (capital O) differs from the string table's
    "Spirit of Laharah"; since the hash is case-sensitive the fallback pair is added even when a level lists
    the spirit under its usual spelling (an extra, unused entry; harmless).
