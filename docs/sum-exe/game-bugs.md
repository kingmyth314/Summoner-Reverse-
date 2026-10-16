---
title: Bugs in the original game
parent: Sum.exe
nav_order: 6.95
---

# Bugs in the original game
{: .no_toc }

1. TOC
{:toc}

**Summary:** This page lists the defects in `Sum.exe` v1.40 that we have found so far: 355 in all.
Three of them hurt every player on a modern PC: double game speed, intros that skip after about 25 days
of Windows uptime, and a crash on Start New Game when no audio device is connected. SummonerFix fixes all
three. Most of the rest are small UI slips. Others are latent: they need modded data or an unusual
setup before they show. Every entry below was checked again in the disassembly
(`node tools/asm.mjs fn <name>`) on 2026-10-15.

{: .note }
**Draft.** Severity says what the player gets when the bug triggers. **Observed** means we read the
deciding code, or saw the bug at runtime where the entry says so. *Inferred* means the in-game effect
comes from reading the code and has not been tested. "Latent" means we know of no trigger in retail play.

## Overview

| Severity | Live | Latent | Fixed by SummonerFix |
|---|---|---|---|
| Crash or hang | 7 | 79 | 2 |
| Gameplay | 51 | 24 | 4 |
| Cosmetic | 59 | 35 | 5 |
| Invisible | 42 | 58 | 0 |

Two bugs only matter with the OpenGL renderer. The game never selects that renderer, but SummonerFix
does (see [SummonerFix](../summonerfix/index.md)).

## Crash or hang

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| C1 | Start New Game crashes when Windows has no audio output device | `snd_init` `0x5205b0`, `ds_channel_release` `0x55c5b0` | **Observed**, reproduced in agent mode | Fixed (`bugfix.c`) |
| C2 | Crash at start-up when the 3D primary sound buffer cannot be created | `ds_init` `0x55c1a0` | **Observed** code, *inferred* crash | No |
| C3 | "Failed to allocate 1073741824 bytes" with the OpenGL renderer | `GlTextureInit` `0x54e690` | **Observed** at runtime | Fixed |
| C4 | Latent: more than 300 lamp glows corrupt memory | `lamp_glow_add` `0x4c0a90` | **Observed** code, *inferred* effect | No |
| C5 | Latent: a cutscene with more than 175 living characters overwrites memory | `cutscene_play` `0x4361d0` | **Observed** code, *inferred* effect | No |
| C6 | Latent: more than 50 command-line arguments overwrite memory | `cmdline_split` `0x53e060` | **Observed** code, *inferred* effect | No |
| C7 | Latent: a UI window with "+Always a selection" can hang the game | `ui_window__process` `0x52bc20` | **Observed** code, *inferred* hang | No |
| C8 | Latent: a level with more than 15 shopkeepers or 1000 shop entries overwrites memory | `level_parse_shopkeeper` `0x4cc4d0` | **Observed** code, *inferred* effect | No |
| C9 | Latent: a surface name longer than about 56 characters in the surfaces table overflows the stack | `surfaces_parse_table` `0x468e40` | **Observed** code, *inferred* effect | No |
| C10 | Latent: a dialogue topic that removes two initial topics, the first of them the current hail topic, hangs the game | `conversation_apply_topic_effects` `0x4d7eb0` | **Observed** code and retail data, *inferred* hang | No |
| C11 | Latent: more than 1000 item objects in a level overwrite the object list | `item_obj_spawn` `0x46a1b0` | **Observed** code, *inferred* effect | No |
| C12 | Latent: more than 1224 live items crash on the next item created | `item_alloc` `0x44b380` | **Observed** code, *inferred* crash | No |
| C13 | Latent: an object still in the active list when the game returns to the title hangs or corrupts the heap | `objects_free_all` `0x469fc0` | **Observed** code, *inferred* effect | No |
| C14 | Latent: a character whose name has 32 or more characters overwrites its own mesh pointers | `character_create_base` `0x541500` | **Observed** code and retail data, *inferred* effect | No |
| C30 | Latent: a character preload index with more than 70 entries or more than 1 MB overwrites memory | `ca_load_preload_index` `0x541790` | **Observed** code and retail data | No |
| C40 | Latent: the `door_open`, `door_close` and `door_toggle` console commands with an unknown door name use an uninitialised pointer and can crash | `cmd_door_open` `0x4d5e70`, `cmd_door_close` `0x4d6580`, `cmd_door_toggle` `0x4d6600` | **Observed** code, *inferred* crash | No |
| C50 | Latent: the OpenGL renderer with window mode 202 (desktop size) crashes at start-up | `gr_opengl_init` `0x54f6f0` | **Observed** code, *inferred* crash | No |
| C51 | Latent: the OpenGL renderer crashes at start-up on a driver without `GL_ARB_multitexture` | `gl_init_multitexture` `0x54f370`, `gl_setup_multitexture_state` `0x54ffa0` | **Observed** code, *inferred* crash | No |
| C52 | Latent: in the Khosani lab's room 4, a missing prisoner makes the script write into the event-name pool | `ls_khosanilab_room4_frame` `0x4f8d70` | **Observed** code and retail data, *inferred* effect | No |
| C53 | Latent: a world map without `$hills`, forest or grass navpoints crashes the stage 4 to 8 encounter roll | `ls_worldmap1_pick_encounter_stage4` `0x4fc4b0` | **Observed** code, *inferred* crash | No |
| C70 | Latent: opening the Character screen while Rosalind wears heavy body armour crashes, because she has no heavy stat-screen animation | `statscreen_load_anim` `0x448f60` | **Observed** code and retail data, *inferred* crash | No |
| C71 | Latent: in a town, a character type with a hide or cloaked state move but no "stand" state move crashes while its animations load | `charinfo_remap_town_moves` `0x447850` | **Observed** code and retail data, *inferred* crash | No |
| C72 | Latent: a move with more than three `$Trigger:` lines overwrites its sound reference, and from the fifth trigger on its move list link | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data, *inferred* effect | No |
| C73 | Latent: a translated chain skill name of more than about 60 characters overflows the "%s learned %s!" message on the stack | `character_learn_chain_skill` `0x449650` | **Observed** code and retail data, *inferred* effect | No |
| C90 | Latent: with the OpenGL renderer, a multi-pass polygon of more than 32 vertices after clipping overwrites the stack | `gr_tmapper_multipass` `0x507a80` | **Observed** code, *inferred* effect | No |
| C91 | Latent: Print Screen with the Direct3D renderer quits the game with "see your graphics guy" when the back buffer cannot be locked | `gr_screenshot` `0x5061f0`, `gr_d3d_read_backbuffer` `0x53a7f0` | **Observed** code, *inferred* trigger | No |
| C100 | Latent: a party member who joins or is set to a level with more skill points than its progression can take (for example with a modded `experience.tbl`) hangs the game | `character_add_xp` `0x44fab0` | **Observed** code, retail data and progression tables, *inferred* hang | No |
| C110 | Latent: a level entered at an entry point that has neither `$player<n>-01` nor `$player01` crashes when the party is placed; the "Find Allender" error that should catch it never fires | `level_find_player_starts` `0x4caf40`, `character_place_at_party_start` `0x44f1a0` | **Observed** code and retail data, *inferred* crash | No |
| C111 | Latent: a clickable object whose name plus the level script name has 68 or more characters overflows the stack when the level's door and chest states are saved or loaded | `lc_find_by_hash` `0x4cd570` | **Observed** code and retail data, *inferred* effect | No |
| C112 | Latent: a `$Door:` name that is not a level object crashes the level load or changes the last level object's door | `level_script_parse_table` `0x4cd5f0` | **Observed** code and retail data, *inferred* effect | No |
| C113 | Latent: a level with more than 10 level effects, 32 triggers, 32 level items or 50 clickable objects overwrites memory | `level_parse_table` `0x4cb7b0`, `level_parse_trigger` `0x4cc190`, `level_items_parse_table` `0x4cb430`, `level_script_parse_table` `0x4cd5f0` | **Observed** code and retail data, *inferred* effect | No |
| C114 | Latent: a level item of the current script with a missing `$Navpoint:` crashes the level load; one with an unknown `$Item:` uses a stale item type | `level_items_parse_table` `0x4cb430` | **Observed** code and retail data, *inferred* effect | No |
| C130 | Latent: a level script whose rider (Liangshan, Jade Temple) is missing from the level, or a Wolong2 "JekharJoin" while Jekhar is not in the reserve party, dereferences NULL and crashes | `ls_liangshan_init` `0x4f4930`, `ls_jadetemple_init` `0x4effe0`, `ls_jadetemple_tiger_ghost_intro_cs_end` `0x4f0320`, `ls_wolong2_frame` `0x4f2140` | **Observed** code and retail data, *inferred* crash | No |
| C140 | Latent: a console line with a word of 133 or more characters crashes the console, and one of 128 or more corrupts its error jump; `character` with more than 128 characters of arguments overflows the stack. No console in v1.40: only a patch or a `.vcs` script can send such a line | `read_word` `0x50aab0`, `make_lowercase_token` `0x50aa50`, `execute_command_line` `0x50a210`, `cmd_character` `0x439930` | **Observed** code, *inferred* crash | No |
| C141 | Latent: in the game intro, a Joseph "play action" or "play .mvf" whose move is missing (an unknown move or a .mvf that fails to load) crashes the cutscene | `script_entry_start` `0x4dd0b0` | **Observed** code and retail data, *inferred* crash | No |
| C142 | Latent: a cutscene that takes off more than 16 pieces of equipment overwrites the record count and then memory after it | `script_action_replace_weapon` `0x4dccb0`, `script_action_replace_armor` `0x4dce60` | **Observed** code and retail data, *inferred* effect | No |
| C143 | Latent: a script object with more than 54 actions, a level script with more than 200 scripted objects or a cutscene with more than 32 overwrites memory | `script_parse_objects` `0x4dba30` | **Observed** code and retail data, *inferred* effect | No |
| C144 | Latent: in multiplayer, a chat or system message that wraps into more than six lines crashes the game | `chat_add_line` `0x41ed60` | **Observed** code, *inferred* crash | No |
| C150 | Latent: with the Glide renderer (a 3dfx card or a Glide wrapper), a polygon of more than 25 vertices overwrites the renderer's mode, the `whee` console command object, the cursor clip rectangle and the hardware configuration | `gr_glide_tmapper` `0x54b240`, `gr_glide_tmapper_multitexture` `0x54b3a0` | **Observed** code, *inferred* effect | No |
| C151 | Latent: in the Temple of Urath (`templeint_v2`) a missing Tiger Rider, in the Lenele courtyard a Flece who cannot join (a load pending, or no Flece character), and in Eleh and Wolong a Joseph or Rosalind who is not in the reserve party dereference NULL and crash | `ls_templeint_frame` `0x4f5f80`, `lsf_lenele1c_flag_found_flece` `0x4fab90`, `ls_eleh_frame` `0x4f1560`, `ls_wolong_frame` `0x4fe9a0` | **Observed** code and retail data, *inferred* crash | No |
| C160 | Latent: with the `precalc_vis_method` console setting on, more than 50 path graph points on one straight line from a path's start or end point overwrite the exception record and then the return address of the visibility sweep | `determine_point_visibility` `0x4b8350` | **Observed** code, *inferred* crash | No |
| C161 | Latent: a navigation BSP node with a type other than 1, 2 or 3 in a level's `.pfg` path graph hangs the game at the next path or position test | `pf_bsp_node_find` `0x4a5c60`, `pathvis_bsp_contains` `0x4b86e0`, `pf_read_typed_refs` `0x49ca20` | **Observed** code, *inferred* hang | No |
| C170 | Latent: a message box line of 64 or more characters overflows a stack buffer, and from about 69 characters on it corrupts the lengths of the lines after it | `ui_message_box_draw` `0x414270` | **Observed** code, *inferred* effect | No |
| C171 | Latent: in multiplayer, Tab or Shift+Tab hangs the game when the leader's character and every other party record's character are gone | `party_cycle` `0x413360` | **Observed** code, *inferred* hang | No |
| C180 | Latent: a modded name table overflows the stack or a buffer: an `item_names.tbl` / `quest_names.tbl` id of 1000 or more, a level section of `topic_names.tbl` / `script_names.tbl` with more than 500 English lines, or a `strings.tbl` id of 1000 or more; a level whose topic and script names need more than the 10 KB level pool crashes | `localize_load_names` `0x504520`, `localize_load_level_table` `0x504870`, `localize_set_string` `0x504400`, `localize_strdup` `0x504070` | **Observed** code and retail data, *inferred* effect | No |
| C181 | Latent: in multiplayer, a reliable data packet whose length field says more than 600 bytes overwrites the heap after its 600-byte receive buffer (only a malformed or hostile packet; the game's own senders stay below the limit) | `nw_work_reliable` `0x517e00`, `nw_NewReceiveReliable` `0x518bb0` | **Observed** code, *inferred* effect | No |
| C190 | Latent: a `cutscene.tbl` entry without a `$Fog:` line, or whose first `$Fog` key is for a later shot, crashes the cutscene on its first frame | `cutscene_get_fog_distances` `0x4354a0` | **Observed** code and retail data, *inferred* crash | No |
| C191 | Latent: a `cutscene.tbl` with more than 85 entries, or an entry with more than 4 `$Camera`, 2 plain or 10 shot `$Animation`, 4 `+Minitrack`, 16 `$Fog` or 16 `$Fade` lines, or camera files with more than 50 shots in all, overwrites memory | `cutscene_parse_table` `0x4355e0`, `cutscene_load_files` `0x434b40` | **Observed** code and retail data, *inferred* effect | No |
| C192 | Latent: a script editor message box asked for a button set above 4 deletes garbage pointers when it closes, and one with an empty text shows uninitialised stack text | `editor_dialog_show` `0x430c80`, `editor_dialog_destroy` `0x4316a0` | **Observed** code, *inferred* effect | No |
| C193 | Latent: in the debug "Load" screen (state 8, never entered in v1.40), a level-list message with no selected entry dereferences NULL | `loadgame_level_list_cb` `0x432200` | **Observed** code, *inferred* crash | No |
| C200 | In LAN multiplayer, going from the game list to Create Game and back with Cancel four times crashes the game | `mp_local_player_create` `0x441760`, `state_mp_gamelist_enter` `0x43eed0` | **Observed** code, *inferred* crash | No |
| C201 | Latent: in multiplayer, one forged packet of type 0x42 crashes the receiving machine whoever sends it, and one of type 0x40 (DMY_PCK) from a machine in the game does the same; the game's own senders never use either type | `multi_process_packet` `0x441820` | **Observed** code and data, *inferred* crash | No |
| C202 | Latent: entering the multiplayer game list or lobby when the chosen character's `.plr` file can no longer be read writes through a NULL pointer and crashes | `mp_local_player_create` `0x441760`; lobby connect step in `state_mp_lobby_frame` `0x43f080` | **Observed** code, *inferred* crash | No |
| C203 | Latent: the `load_mlayout` console command creates every monster of the layout with character type -1 (the map name looked up as a character type), so the memory before `char_infos` is used as its type. No console in v1.40: only a patch or a `.vcs` script can run it | `monster_layout_load` `0x43fe00` | **Observed** code, *inferred* effect | No |
| C210 | In a hard fight, an AI-controlled Healer or Healer/Caster party member (Rosalind by default) who can cast Bless but not Protect can crash the game when four or five party members are unblessed and hurt or under attack: its candidate count runs past its six-entry list and overwrites the caller's saved registers and return address | `ai_group_think_caster` `0x4936c0`, `ai_group_combat_think` `0x492db0`, `ai_groups_frame` `0x492d30` | **Observed** code, *inferred* crash | No |
| C211 | Latent: a cutscene whose camera files hold more than 250 position keys or 250 rotation keys in all overwrites the next key pool, and past the last pool the snow state (largest retail: Machival-Death, 179 rotation keys) | `cutscene_camera_read_track` `0x493fd0` | **Observed** code and retail data, *inferred* effect | No |
| C230 | Latent: a monster or party member that has a cast-at-target order waiting behind another order crashes the game when that order's target no longer exists while a level cutscene script runs | `ai_goal_evaluate` `0x475830` | **Observed** code, *inferred* crash | No |
| C231 | Latent: a door with five or more navigation edges loses its fifth edge when the level's door floor points are restored, and one with 36 or more overwrites the return address of `door_find_floor_point` (a crash) | `door_find_floor_point` `0x476ab0` | **Observed** code, *inferred* effect; retail navigation data not counted | No |
| C232 | Latent: the game camera crashes if it is ever left without a target, or if its target is set to none while it has one | `camera_frame` `0x478990`, `camera_set_target` `0x478ff0` | **Observed** code, *inferred* crash; no retail path found | No |
| C220 | Latent: three script editor actions crash the game: adding an action that takes a navpoint while the Cutscene Script button is latched and the cutscene script has no navpoints; choosing the Die action for a character type without a death move; and Save while the Cutscene Script button is still latched after the cutscene script was unloaded | `script_make_filename` `0x4e8cf0`, `script_editor_apply_action_params` `0x4e84b0`, `script_editor_show_script_dialog` `0x4e6fe0` | **Observed** code, *inferred* crash | No |
| C221 | Latent: the script editor's Save hangs (and fills the disk with one repeated line) when the level table has no line that is exactly `#Ambient Sounds`, or when `level_items.tbl` has neither the level's own `#Level:` line nor `#End` | `script_write_level_table` `0x4ea1e0`, `level_items_write_table` `0x4ea500` | **Observed** code and retail data, *inferred* hang | No |
| C222 | Latent: in the script editor's Camera mode, New adds a `$camera` navpoint with no check against the 750-entry navpoint table; the 751st overwrites the memory after it | `script_editor_delete_navpoint` `0x4e6a30` (adds, despite the name) | **Observed** code, *inferred* effect | No |
| C250 | In the Luminar fight, the game crashes when the target of the Luminar's lightning beam dies (or is gone) between the start of the beam and its hit | `luminar_beam_hit` `0x4ca3a0`, `luminar_ai_frame` `0x4c95f0` | **Observed** code and retail data, *inferred* crash | No |
| C251 | Latent: an effects array (`.arr`) line longer than 1023 characters overflows the stack of the effects loader | `File__read_line` `0x525f60`, `effects_parse_arr_line` `0x51ca60` | **Observed** code and retail data, *inferred* effect | No |
| C252 | Latent: drawing an arrow button whose inner height or width is exactly 5 pixels divides by zero | `ui_button__draw_arrow` `0x527440` | **Observed** code, *inferred* crash | No |
| C253 | Latent: `file_make_path` called with the "any path" id reads far past the path table | `file_make_path` `0x512b30` | **Observed** code, *inferred* crash | No |
| C254 | Latent: a level with five or more Tentacle Beast characters writes the fifth tentacle's record over the tentacle battle's own variables | `tentacle_beast_boss_load` `0x4c6d10` | **Observed** code and retail data, *inferred* effect | No |
| C260 | Latent: with all 400 lights in use, a new point or spot light is written over the end of the light selection table; if it was one of the inventory model's lights, closing that screen can crash in `light_delete` | `light_create_point` `0x51e290`, `light_create_spot` `0x51e3f0`, `light_alloc` `0x51e380` | **Observed** code and retail data, *inferred* crash | No |
| C261 | Latent: a list box entry that wraps into more than 20 lines overwrites the list box line tables and, from line 22, the UI manager's mouse flags; the next draw copies the first line with a pointer as its length and crashes | `ui_listbox_entry__wrap_text` `0x52ca60`, `ui_listbox__draw` `0x52f080` | **Observed** code, *inferred* crash | No |
| C262 | Latent: a cutscene camera rotation key of (0, 0, 0, 0) divides by zero when the camera tangents are built; a key whose packed length is above one unit makes them zero | `cquat__normalized` `0x523050`, `cquat_log_of_delta` `0x523c60`, `anim_track__calc_rot_tangents` `0x5506e0` | **Observed** code and retail data, *inferred* effect | No |
| C270 | In a fight where more than ten fireball, wall of fire, Pyrul fireball, blizzard or dark wall areas burn at the same time, the effect update overwrites its own saved registers and, from the fifteenth area, its return address (a crash) | `fx_frame` `0x48a230` | **Observed** code, *inferred* crash; not reproduced, and whether a retail fight reaches eleven areas was not measured | No |
| C271 | Latent: a level or cutscene that creates a sixth world effect whose name contains "sky" writes past the five-entry sky list; the frame then draws a pointer built from the effect counter (a likely crash) | `fx_create_world` `0x48cea0`, `levelrender_render_scene` `0x482200` | **Observed** code and retail data, *inferred* effect | No |
| C272 | Latent: in the Masad combat tutorial, the "part 2" step (after the Orenian Scout is defeated) crashes if an arrow is in flight, a spell area is burning or any character's status effect holder exists at that moment; the level-start callers crash the same way if such an effect already exists | `fx_disable_by_effect_name` `0x48d660`, `tutorial_cond_combat_part2` `0x42ead0` | **Observed** code and retail data, *inferred* crash | No |
| C273 | Latent: in a network game, a spell "near" or "at" hit effect with no spell record crashes every machine that updates it: the Ghost Rider's stage-3 scythe hit (no retail multiplayer script has a Ghost Rider), or any such cast while all 64 spell records are in use | `fx_frame` `0x48a230` (kinds 12 and 14), `boss_counter_hit_dispatch` `0x4a2230`, `spell_effect_add` `0x47f660` | **Observed** code and retail data, *inferred* crash | No |
| C274 | Latent: a level item placed while all 40 containers are in use crashes the level load | `level_item_spawn` `0x46a640`, `container_obj_create` `0x4897f0` | **Observed** code and retail data, *inferred* crash | No |
| C280 | Latent (console only): after `play_and_load_cutscene` returns to the main menu, Start New Game crashes (access violation) | `cmd_play_and_load_cutscene` `0x437590`, `cutscene_change_level_for` `0x437400` | **Observed** in game once, cause not analysed | No |
| C290 | Latent: Print Screen at a screen height above 1024 lines (only with a resolution patch, for example SummonerFix's `GameHeight`) overwrites the JPEG writer's return address and crashes | `jpeg_write_file` `0x5565e0` | **Observed** code, *inferred* crash | Avoided (`gameopts.c`) |
| C291 | Latent: an `alt_textures.tbl` `$Base:` name of 33 to 40 characters runs into the next base, and a texture set with more than 15 textures overwrites the next set | `alt_textures_parse_table` `0x4aac10` | **Observed** code and retail data, *inferred* effect | No |
| C292 | Latent: an input box character set with a character above `0x7f` (script editor or UI table) writes past its 128-entry stack table into the return address | `ui_inputbox__set_valid_chars` `0x552870` | **Observed** code, *inferred* crash | No |
| C300 | Latent: with the Direct3D renderer on a card whose largest texture is smaller than a bitmap (256 on Voodoo-class cards; current cards take 8192 or more), loading that bitmap writes past the locked texture surface and reads past the bitmap | `gr_d3d_copy_bitmap_to_surface` `0x54c0a0`, `gr_d3d_create_texture_section` `0x54bca0` | **Observed** code, *inferred* effect | No |
| C301 | Latent: a cutscene script with more than 512 navpoints overwrites the cutscene string pool, and a level script with more than 750 navpoints overwrites the `door_toggle` console command and the memory after it | `cutscene_script_load` `0x4ed0f0`, `script_parse_navpoints` `0x4db700` | **Observed** code and retail data, *inferred* effect | No |
| C302 | Latent: in `Rand-ForestNite1_v2` and `rand-hills01_v2`, a Ghost Rider or Phoenix Rider missing from the level crashes the game on the first frame | `ls_rand_forestnite1_frame` `0x4f56e0`, `ls_rand_hills01_frame` `0x4fa4a0` | **Observed** code and retail data, *inferred* crash | No |
| C310 | Latent: a game folder whose path is 116 characters or longer crashes the game at start-up | `packfile_add` `0x534420`, `packfile_fopen` `0x534690` | **Observed** code, *inferred* crash | No |
| C311 | Latent: in multiplayer, a new character from the host that the client cannot create (for example with all living-entity slots in use) crashes the client | `mp_handle_new_lep` `0x47ff10` | **Observed** code, *inferred* crash | No |
| C312 | Latent: in multiplayer, a respawn packet for a player id the machine does not know crashes it | `mp_respawn_player` `0x481090` | **Observed** code, *inferred* crash | No |
| C313 | Latent: the `save_map_labels` console command hangs the game when `automap_labels.tbl` has no `#End` line | `automap_labels_save` `0x430580` | **Observed** code and retail data, *inferred* hang | No |

**C1. No `snd_disabled` after a failed `ds_init`.** When `ds_init` fails, `snd_init` returns -1 without
setting `snd_disabled` (`jne 0x520672` at `0x520644` jumps past `mov byte [0x2cbf480], 1`). Later
`ds_channel_release` enters a stream lock that was never initialised. See [Sound](sound.md#start-up) and
[SummonerFix](../summonerfix/index.md#fixes).

**C2. Retry through a NULL pointer.** When the 3D primary buffer fails, `ds_init` releases DirectSound and
stores 0 (`mov [0x2f79e3c], eax` with `eax = 0` at `0x55c2eb`). It then calls the next method through
that pointer (`mov edx, [eax]` at `0x55c2fa`). Without DS3D, the same failure goes on to `SetFormat` on the
primary buffer that was never created (`jne 0x55c337` at `0x55c2f0`). See [Sound](sound.md#start-up).

**C3. Texture buffer sized from the driver.** The OpenGL init allocates `size x size x 4` bytes, where
`size` is `GL_MAX_TEXTURE_SIZE` (`imul ecx, eax; shl ecx, 2` at `0x54e71c`). A driver that reports 16384
asks for 1 GB. See [SummonerFix](../summonerfix/index.md#opengl-failed-to-allocate-1073741824-bytes).

**C4. No bound in `lamp_glow_add`.** The array holds 300 records of 0x18 bytes (`0x24893e8`..`0x248b008`).
The function writes at `lamp_glow_count` with no compare. Record 301 lands on the count itself.
`fx_create_instance` (`0x48beb8`) adds glows for every effect instance with flag `0x40`. They are only
cleared on a level change. See [Effects](effects.md#lamp-glows).

**C5. No bound on the saved actors.** `cutscene_play` writes one 8-byte entry per living character into
`cutscene_saved_actors` (`0x5fcce8`, 175 entries) and increments `cutscene_num_saved_actors` (`0x436556`)
without a compare. See [Cutscenes](cutscenes.md#what-cutscene_play-changes).

**C6. No limit in `cmdline_split`.** Each argument goes to `cmdline_args[cmdline_num_args++]`
(`0x53e17b`, `0x53e1a9`) with no test against 50. Argument 51 overwrites `cmdline_params` (`0x2dc7038`).

**C7. A loop that never advances.** With window flag `0x10` and no selection, the loop at
`0x52bcd8`..`0x52bce5` calls the first gadget's "selectable" method. It repeats while that returns 0,
but `edi` never moves to the next gadget. Flag `0x10` ("+Always a selection") is set only by
the UI table loader, `ui_load_table`, which has no caller (V9). The script editor builds its windows with constant
flags that do not include `0x10`. The only other path is `editor_dialog_show`, which builds its flags at run time
from its last argument (`or ecx, 0x20` at `0x430ebc`). Its three callers (`script_write_objects` `0x4ea0b8`,
`script_save` `0x4ead94` and `0x4eadcb`) all pass `0xb`, so its windows get flags `0x2b` and no path sets `0x10`.

**C8. Shop tables.** The parser stores into `level_shopkeepers` (`0x2490ca0`, 15 records) and
`level_shop_items` (`0x2491ac0`, 1000 entries) without checking either count. The retail tables fit. See
[Shops and loot](shops-loot.md#where-shops-come-from).

**C9. Wrong buffer size.** The second `ParseString` call passes size 0x100 (`push 0x100` at `0x468f68`)
for a stack buffer at `esp+0xa4`. The next local starts at `esp+0xdc`, so the buffer has 0x38 bytes.

**C10. Inner loop steps the wrong list.** For each `+Remove Initial Topic:` entry that removes the speaker's
current hail topic, the game picks a new hail topic from the `+Add Initial Topic:` list. That inner loop advances
with the *outer* entry's next pointer (`mov ecx, [eax+8]` at `0x4d7fcd`, with `eax` the remove entry). It ends only
when the remove entry is the last in its list. The parser prepends entries (`conversation_topic__push_list24`
`0x4fecb0`), so the list runs in reverse file order. Four retail topics remove two initial topics: "I have your
book!" in `lenele1ab_dlg.tbl`, its `_v2`/`_v3` copies and `Miniquests_dlg.tbl`. In each, the hail topic in effect
at that point (`"Hail"#2`) is the first line in the file, so it is last in the list, and the loop exits after one
pass. A modded table that swaps the two lines would hang. Found while writing
[`conversation.cpp`](../../src/decomp/game/conversation.cpp).

**C11. No full test on the item pool.** The search loop for a free `item_obj` stops at 1000
(`cmp [ebp-0x88], 0x3e8; jge` at `0x46a1db`). It then uses `item_obj_pool[i]` anyway (`0x46a200`..`0x46a20c`).
The pool holds 1000 records of 0xac bytes (`0x946790`..`0x970770`), so record 1000 is `obj_active_list`
(`0x970770`) itself. `obj_link_new` (`0x469b60`) then links the list head into its own list, and the following
stores write over the globals after it, including `objects_initialized` (`0x970809`). Item objects are freed when
they are picked up. `items_init_pool` only runs from `objects_reset` (title screen and multiplayer reset), so the
limit is per level visit. We know of no retail level that comes near it.

**C12. No test for an empty free list.** `item_alloc` takes `item_free_list` and reads its `next` with no NULL
test (`mov ecx, [eax]` at `0x44b385`). `items_init_pool` (`0x44b280`) builds the list from 1224 records of 0x10
bytes (`0x8b50c8`..`0x8b9d48`). The last record's `next` is 0, so item 1225 dereferences NULL. Every `item_create*`
goes through it: party inventory, equipment of every living character, loot and shop stock.

**C13. The delete loop does not advance.** `objects_free_all` repeats `delete obj_active_list.next` until the
list is empty (`0x469fc6`..`0x46a01b`). `obj::~obj` (`0x469b40`) only stores the vtable and does not unlink, and
`operator delete` (`0x512570`) does not unlink either. So any object still linked is deleted again and again.
Item objects live in a static pool (`0x946790`), so deleting one also passes a non-heap pointer to the heap. Callers:
`objects_reset` (`0x46b203`, from `state_title_enter` and `mp_session_reset`) and `game_cleanup` (`0x43e228`).
*Inferred:* the level unload always empties the list first, because returning to the title works in retail
play. A path that leaves an object linked would hang.

**C14. Name copied into a 32-byte field.** `character_create_base` strips the extension into a 41-byte stack buffer
(`0x54150c`..`0x54151b`, `file_strip_ext` at `0x5415a0`) and copies it into the character with no length test
(`0x5415b1`..`0x5415c7`). After `character_load_shadow` has stored the mesh at slot `+0x28` (`0x540e18`) and the
shadow mesh at `+0x2c` (`0x540e98`), it copies the name again into slot `+8` (`0x54160e`..`0x541636`). The slots are
0x30 bytes (`ca_preload_alloc_slot`, `0x2ddfa48`..`0x2de20b8`, 205 slots), so a name of 32 characters or more writes
over both mesh pointers, and one of 41 or more also overruns the stack buffer. The longest retail character name has
28 characters ("Guardian of the Stone Portal"). Found in the [`character.cpp`](../../src/decomp/vsdk/character.cpp) notes.

**C30. No bounds on the preload index.** `ca_load_preload_index` reads the entry count (`ca_preload_count`,
`0x54180e`) and the data size (`ca_preload_size`, `0x541818`) from the `.svf` file and uses both as they are.
The loop stores each entry's offset and size into two 70-slot arrays (`[esi - 0x118]` and `[esi]` from
`0x2ee21dc`, `0x541859`..`0x541864`) until the count. It then reads `size` bytes into `ca_preload_data`
(`0x2de20c4`, 1 MB, `0x541894`). Entry 71 writes past the arrays, and a file over 1 MB writes past the buffer
into the arrays that follow. Retail: the largest of the 156 `.svf` files in `chars.vpp` is `evil7724.svf`
with 61 entries and 626,685 bytes. When the file does not open, the message is formatted into a stack buffer
and dropped (`0x5418b0`..`0x5418c2`). Found in the `character.cpp` notes.

**C40. The lookup result is not tested.** Each command calls `door_find_by_name(next_arg, &door, &lobj)` and passes the
`lobj` local straight on (`mov edx, [esp+0x10]` at `0x4d5e97`, `0x4d65a7`, `0x4d6627`). `door_find_by_name` returns -1
without writing either output when the `bsearch` fails or the index is out of range (`0x477308`). The local is never
initialised, so `door_set_open` (`0x476e00`), `door_set_closed` or `door_toggle_state` reads `[lobj+8]` from whatever
was left on the stack. A missing argument has the same effect. These are developer console commands (see
[Console commands](console.md)), so no retail path reaches them.

**C50. Mode 202 skips the context.** `gr_opengl_init` switches on the window mode (`0x54f6f0`..`0x54f700`): 200 calls
`gr_opengl_init_context` (`0x54f739`), 201 loops in `DebugError` ("OpenGl can only be used in windowed modes"), and 202
jumps straight to `0x54f741`. There it calls `glGetString(GL_VERSION)` and `glGetString(GL_EXTENSIONS)` with no current
context, which return NULL, and passes the result to `strstr` (`0x54f761`). The error for any other mode names
`gr_direct3d_init` (`0x54f702`, a copied line). The only `gr_init` call pushes 201 (`push 0xc9` at `0x43db45`; SummonerFix
patches it to 200), so no unpatched game reaches this.

**C51. Entry points left NULL.** `gl_init_multitexture` looks for `GL_ARB_multitexture` in the extension string
(`0x54f382`..`0x54f38c`). When it is missing, the `je 0x54f3e0` skips both `wglGetProcAddress` calls (`0x54f3c0`..`0x54f3da`)
but still calls `gl_setup_multitexture_state` (`0x54f3e0`), whose first instructions call `gl_active_texture_arb` and
`gl_client_active_texture_arb` (`0x54ffa7`, `0x54ffb2`). Both are 0. The "does not support multitexturing" box only covers
fewer than two units (`cmp [esp], 2` at `0x54f39e`). Every vendor driver exposes the extension. *Inferred:* Windows' own
software OpenGL 1.1 ("GDI Generic", used without a vendor driver, for example in some virtual machines) does not, so
SummonerFix's OpenGL setup would crash there.

**C52. No test for -1.** When the player reaches `$platform` (once, before `room4_started` is set, `0x4f8b4a`..`0x4f8b64`), the
function opens `prisonlid01`..`08` and sets `end_mode = 1` on each prisoner through
`script_objects[script_object_find("Khosani Prisoner#n")]` (`0x4f8e0b`..`0x4f8e9a`). `script_object_find` returns -1 when no
living or dead character of that name is found (`or eax, -1` at `0x4cde57`). Index -1 writes the dword 1 at `0x24f5ce0`,
0x550 bytes before `script_objects` (`0x24f6230`), inside the event-name buffer of `events_string_pool` (`0x24ede14`, 0x8400
bytes). The four prisoners are in the retail level scripts (`tables.vpp`) and are still locked in at that moment.

**C53. NULL name compared.** `ls_worldmap1_nearest_hills_nav` returns NULL when no navpoint starts with `$hills` (`0x4fc332`..`0x4fc337`)
and reports distance 10000 (`0x461c4000`, `0x4fc2ad`). The picker switches to the forest or grass result only when that one
is strictly nearer (`0x4fc685`..`0x4fc6c2`). With no terrain navpoint nearer than 10000 units it keeps the NULL and calls
`_stricmp(NULL, "$hills001")` (`0x4fc6e2`..`0x4fc6e8`). The retail world map has these navpoints.

None.

**C70. No NULL check after the move search.** `statscreen_refresh_anim` (`0x412e50`) asks for the heavy
version when the item in equipment slot 7 has slot type 8, "heavy" (`cmp dword ptr [eax+0x1c], 0x8` at
`0x412ee8`). `statscreen_load_anim` walks the state list for variation `0x9f` ("statscreen heavy") and
leaves `m` NULL when there is none. It then reads `m->anim` for the party member (`mov ebx, [ebx+0x8]` at
`0x4490dc`). The multiplayer branch does the same with the result of `charinfo_find_move(info, 0xe, 0x9f, 0)`.
Joseph, Jekhar and Flece have both stat-screen moves; Rosalind has only "statscreen light". All 48 heavy
items in `items.tbl` are `+Restricted:` to Joseph, Flece and Jekhar, so no retail item triggers it.

**C71. The town remap does not check its source.** In a town, or in templeint without the full set,
`charinfo_remap_town_moves` points the state moves hide, ready hurt, hide_ready, ready_to_cloaked,
cloaked_to_ready, cloaked_ready and cloaked_sneak at the anim of `charinfo_find_move(info, 0xe, 0x92, 1)`,
"stand". The result is used without a test (`mov ecx, [eax+0x8]` at `0x447af8`). The function returns
early only when the record has no movement move of variation 1. In the retail tables the only record with
such state moves and no "stand" is Serpent Rider, which has no movement variation 1 move.

**C72. No bound on the triggers.** `$Trigger:` lines fill `move_info +0x1c` in 8-byte steps with no
counter (`0x4462e0`..`0x446318`). A fourth trigger lands on the sound reference at `+0x34` (name and
handle), which was parsed just before. A fifth reaches `+0x3c` and the list link at `+0x40`, so the next
walk of that move list follows a float as a pointer. No retail move has more than three triggers.

**C73. Fixed message buffer.** The message is built with `sprintf` at `[esp+0x60]` (`0x4497d8`). The frame
is 0xa0 bytes (`sub esp, 0xa0` before the three register pushes), so the buffer has 76 bytes before the
return address. The name comes from
`character_get_display_name` (at most 8 characters for the heroes), the skill name from
`localize_string_or_invalid`. The longest retail chain skill names are 19 (English, "Chain Stamina
Burst"), 13 (German) and 17 (French) characters. It needs a modded `strings.tbl`.

**C90. No bound on the pass vertex list.** For every renderer other than Direct3D and Glide (in practice
OpenGL), `gr_tmapper_multipass` copies the `nv` vertex pointers into a 32-entry stack array (`sub esp, 0x8c`
at `0x507a85`, array at `[esp+0x18]`, copy loop `0x507aab`-`0x507aca`) without testing `nv`. Its callers are
`gr_poly_clip_and_draw` (`0x54595c`) and `gr_poly_split_sections_draw` (`0x55f556`). `gr_poly_vertex`
(`0x5452f0`-`0x54530c`) appends to a 32-record pool (`gr_poly_vertices`, 0x680 bytes of 0x34-byte records) with
no check either, and the clipper (`gr_poly_clip_planes` `0x545970`) can add one vertex per clip plane, up to
six. So a polygon of 27 or more vertices that is clipped can pass 33 or more vertices. *Inferred:* retail
geometry uses far smaller polygons, so this is latent. Direct3D and Glide take their own path
(`0x507ce3`, `0x507cb1`).

**C91. A failed read-back is fatal.** `gr_screenshot` switches on the format `gr_read_backbuffer` returns: 3,
6 and 7 (565, 888, 8888) are handled, and anything else calls `DebugError` with "The current screen mode
doesn't support print screen... see your graphics guy." (`0x50625d`-`0x506279`). `DebugError` shows a message
box and calls `exit(1)` (`0x531d08`, `0x531d10`). `gr_d3d_read_backbuffer` returns 0 when the back buffer's
`Lock` fails (`test eax, eax; je` at `0x53a827`, `xor eax, eax` at `0x53a82b`), so a screenshot taken while the
surface cannot be locked ends the game. Glide and OpenGL always return 3 and 7. *Inferred:* a lock should fail
only while the surface is lost (for example around a task switch), so this is latent. SummonerFix's OpenGL
renderer is not affected. The failed-lock path also skips `gr_d3d_begin_scene` (`0x53a8ae`); see Still unclear.

**C100. Skill points with nowhere to go.** With `quiet` set, `character_add_xp` spends a party member's
unspent points at once: it walks the progression round-robin and gives +10 to a skill whose entry is at or
below the character's level and below 100 (`0x44ff15`..`0x44ffa6`). The loop ends only when the points reach 0
(`cmp [edx+0x17c], 0; jle` at `0x44ff18`). If every eligible skill is at 100, it never ends. With a
progression count of 0 it would divide by zero instead (`idiv [ecx+0x20]` at `0x44ffa0`). `party_member_join`
and `party_set_level` pass `quiet` = 1. In retail the highest level they set is 30 (`game_stage_set`,
`push 0x1e` at `0x4d45e2`), which gives 71 points (`experience.tbl`: 2 per level for 2-6 and from 20, 3 for 7-19).
The smallest capacity is Jekhar's, 13 entries of about 9 points each, 119 in all (`0x5935d8`; base skills 10
from `Jekhar.tbl`). So retail data cannot trigger it. `party_set_level` has its own copy of the loop
(`0x4cecf0`..`0x4ced47`) with the same exit test.

**C110. The test reads the wrong slot.** `level_find_player_starts` fills `level_player_starts[0..3]` (`0x60a32c`)
from `$player<entry>-01`..`04` and, when the first is missing, from `$player01`..`04`. The error test after the
fallback loop uses the loop counter, which is 4 there: `mov eax, [4*edi + 0x60a32c]` at `0x4cafcc` reads
`0x60a33c`, which is `level_entry_point` (always 1 or more), so the `DebugError` "Unable to locate player starts
for level %s ... Find Allender." is never reached. A level without the starts keeps NULL pointers, and
`character_place_at_party_start` reads the position through them (`add ecx, 0x14` at `0x44f1d5`). The fallback is
also only tried when start 0 is missing, so a group with `-01` but without `-02`..`-04` leaves members 2-4 on
NULL. In the retail tables every single-player "load level" trigger's `+Index:` has a full group of four in
the target's `_script.tbl` (and `.s3d`). The only gap, `lenele3a.tbl`'s exit to `lenele3d` with `+Index: 2`, uses
the spline `$loadarea01`, which `lenele3a` does not have, so the parser drops that trigger (count decremented at
`0x4cc4c5`). The elevator callbacks set entry points 3 to 5 (`0x41dec0`..`0x41e2c0`, multiplayer only), and
the groups exist: 3 and 4 in `IonaEXT_script.tbl` (level 15), 3 to 5 in `eleh_script.tbl` (level 29), and the
same in their `_m1` copies. The entry points set by scripts were not checked against the navpoints.

**C111. A 0x44-byte buffer.** `lc_find_by_hash` builds `level_script_name + object name` with
`strncpy(buf, level_script_name, 0x40)` and `strncat(buf, name, 0x40)` into a 0x44-byte stack buffer
(`sub esp, 0x44` at `0x4cd575`, calls at `0x4cd59b` and `0x4cd5ac`). Both two-stage paths call it
(`savegame_two_stage_set` `0x4d0fac`, `savegame_read_two_stage` `0x4d3618`), so the overflow would happen on a save
or load in that level. G42 is the same name pair in another function. The longest retail pair (script name plus
`#Clicks` name) is 36 characters (`IkaemosBottomInt_m1` + `MedPriestStoned13`).

**C112. No test after the search.** The `#Doors` loop compares the `$Door:` name with every level object
(`0x4cd982`..`0x4cd9c5`) and keeps the last one compared when nothing matches. `door_from_lobj` returns 0
for an object without flag `0x100` (`0x4768d4`), and the parser writes the lock bit through it
(`mov [esi+0x54], edx` at `0x4cd9ec`). If the last object is a door, that door gets the other door's lock and
sounds. Every retail `$Door:` name is in its level's `.s3d`.

**C113. Four tables without a bound.** `level_parse_table` writes `$Level_fx:` records at `level_effect_count`
(`0x4cb8fd`..`0x4cb92d`) into 10 records of 0x28 bytes (`0x24902c8`; `level_ambients` follows at `0x2490458`).
`level_parse_trigger` writes `$Trigger:` records into 32 of 0x64 bytes (`0x2490dd0`), `level_items_parse_table`
level items into 32 of 0x70 bytes (`0x24e3b50`), and the `#Clicks` loop of `level_script_parse_table` into 50
`lc_entries` of 0x10c bytes (`0x248ce40`, `0x4cdbe2`). None compares the count. The ambient sounds do (`cmp esi,
0x80` at `0x4cd257`). Retail maxima: 7 effects (`IonaExt02.tbl`), 25 triggers (`Lenele1aa.tbl`), 18 level items
(`sewer` in `level_items.tbl`), 26 clickable objects (`IkaemosExt_script.tbl` and others).

**C114. Two unchecked look-ups.** For a record of the current script, `navpoint_find` stores its result in
`+0x48` (`0x4cb5f8`) and, after `savegame_level_item_has`, the parser writes through it
(`mov [eax+0x2c], ecx` at `0x4cb6b6`) with no NULL test. An unknown `$Item:` skips the store at `0x4cb5a1`, so
`+0x40` keeps whatever an earlier level left in that record (or 0), and `item_create_by_name` gets its name from
there (`mov eax, [edx]` at `0x4cb6ed`). Every retail item and navpoint of `level_items.tbl` resolves.

None.

**C130. Lookups used without a NULL test.** `ls_liangshan_init` passes `script_object_find_character("Serpent Rider#1")`
and `("Phoenix Rider#1")` straight to `serpent_set_solo` / `phoenix_set_solo` (`0x4f4946`..`0x4f4961`).
`ls_jadetemple_init` does the same for the four riders of `jadetemple_v2` (`0x4f0006`..`0x4f0041`) and
`ls_jadetemple_tiger_ghost_intro_cs_end` for the Tiger and Ghost Riders (`0x4f0334`..`0x4f034f`). The lookup
returns NULL when no character has that name, and `serpent_set_solo` writes `[esi + 0x320]` at once (`0x497a4b`).
`ls_wolong2_frame` reads `[eax + 0xc]` from the result of `party_add_member("Jekhar")` (`0x4f21a7`..`0x4f21ac`), which
returns NULL when Jekhar is not in the reserve list `player_list_other` (`0x43411d`, `0x434131`). The retail
script tables place every rider (`Liangshan_script.tbl`, `jadetemple_script.tbl`, `jadetemple_v2_script.tbl`), and
`ls_wolong2_init` moves Jekhar to the reserve before the "Join us." topic can run (`0x4f20b7`..`0x4f20bc`), so this
needs modded data. Found in the `NOTES-level-scripts-8.md` notes (oddity 10).

**C140. Unbounded console tokens.** `read_word` and `read_quoted_string` copy characters through
`token_write_ptr` into `token` (`0x25db894`, 128 bytes) with no count (`0x50aacd`..`0x50aadf`), and
`make_lowercase_token` copies the result into `lowercase_token` (`0x25db21c`, 128 bytes, `0x50aa52`..`0x50aa75`).
`execute_command_line` accepts lines of up to 254 characters (`0x50a22b`..`0x50a244`). After `token` come
`rest_of_command_line`, `current_command_count` (`0x25db918`, word characters 132-135), `char_types_initialized`
and `debug_command_prefixed`. A word of 133 or more characters therefore sets the command count to a large
number, and the next look-up (`0x50a3d8`..`0x50a406`) reads past the 149 used entries of `Console_commands` into
NULL pointers. After `lowercase_token` comes `console_error_jmp` (`0x25db29c`), so a word of 128 or more
characters corrupts the jump buffer that every argument error `longjmp`s to (`0x50a985`). The same line limit
answers the open question on `cmd_character`: up to 244 characters of arguments fit after `character `, so its
128-byte stack buffer can overflow (`0x43995e`..`0x4399c9`). The retail game has no console. SummonerFix's
`[ConsoleKeys]` cuts each command to 95 characters, so only a `.vcs` script started by a patch (or Timeslip's
`[console]` lines, whose length limit was not read) could send such a line. No `.vcs` file ships.

**C141. The game-intro hold test.** When a Joseph "play action" / "play .mvf" in a cutscene is not followed by a
"synch" action, `script_entry_start` checks for the game intro and then compares
`charinfo_find_move_any(info, move, 1)->action` with "gi-Josp-Sneak-Ready.mvf" (`0x4dd211`..`0x4dd245`). The
look-up returns NULL when no move list holds that index (`xor eax, eax` at `0x446ca2`), and `mov eax, [eax]` at
`0x4dd23d` does not test it. The parser stores -1 for an unknown "play action" move, and the .mvf index comes from
`charinfo_load_script_anim`. In the retail `Game-Intro_cscript.tbl`, Joseph's five actions are all "play .mvf"
with files that ship (`gi-Josp02.mvf`, `gi-Josp04.mvf`, `gi-Josp-Sneak-Ready.mvf`), and the intro plays, so this
needs modded data or a missing file.

**C142. 16 saved-equipment records without a bound.** `script_action_replace_weapon` and `_armor` append to
`cutscene_saved_equipment` (`0x2546760`, 16 records of 12 bytes) at `cutscene_saved_equipment_count` with no
compare (`0x4dcd3d`..`0x4dcd87` and the armour copies up to `0x4dd08b`). The 17th record would land on the count
itself (`0x2546820`), and the next ones beyond it. A weapon removal saves at most two records per character, an
armour change two plus one for every further armour action. The retail maximum is 14
(`King-Audience-B_cscript.tbl`: four weapon and three armour changes); no cutscene with these actions has a
"goto". The count is cleared by `cutscene_script_unload` (`0x4ed417`) and `cutscene_script_rewind_actors`
(`0x4ed5e0`).

**C143. Script objects without a bound.** `script_parse_objects` appends each `+Action:` at
`so->num_actions` (`0x4dc0fb`..`0x4dc128`) into 54 records of 0x18 bytes (`+0x4`..`+0x513`, followed by the
path pointer and the counters), and the first action of an object takes the next of 200 `script_objects`
(`0x4dbfb9`..`0x4dbfeb`, followed by `ccmd_door_open` at `0x25388b0`) or 32 `cutscene_script_objects`
(`0x4dbff6`..`0x4dc042`, followed by `cutscene_script_object_count` at `0x257e968`). None compares the count.
Retail maxima: 50 actions (Joseph in `Act2-Outro_cscript.tbl`), 123 scripted objects (`WolongCaverns_script.tbl`),
19 cutscene objects (`Game-Intro_cscript.tbl`).

**C144. Negative scroll in the chat box.** `chat_add_line` wraps the text into up to 16 pieces of the chat width
and then makes room among the six lines: `excess = chat_num_lines + n - 6`, then
`memmove(chat_lines, &chat_lines[excess], (chat_num_lines - excess) * 72)` (`0x41edb4`..`0x41eddc`). For
`n > 6` the size is negative, so `memmove` gets a size near 4 GB, and the loop after it writes past the six
lines. The local input is limited to 63 characters, which gives two or three pieces with a name in front;
only a longer remote or system text (up to 255 characters, as received) could reach seven. Needs a modified peer.

**C150. No bound on the Glide vertex buffer.** `gr_glide_tmapper` builds one Glide 2 `GrVertex` (0x3c bytes) per
input vertex in the static buffer at `0x2f0ec08` (`esi = 0x2f0ec14`, `add esi, 0x3c` at `0x54b37f`, loop on `nv`
from `0x54b289` to `0x54b383`) and passes it to `grDrawPolygonVertexList(nv, ...)` (`0x54b390`..`0x54b396`) without
testing `nv`. The buffer holds 25 vertices: the next variable, `gr_glide_current_mode`, is at `0x2f0f1e4`, 0x5dc =
25 x 0x3c bytes on. Behind it lie `ccmd_whee` (`0x2f0f1e8`, a console command object), `glide_cursor_clip_rect`
(`0x2f0f1f8`), `gr_glide_video_memory_mb` (`0x2f0f208`) and the `grSstQueryHardware` result (`0x2f0f210`).
`gr_glide_tmapper_multitexture` writes into the same buffer (`0x2f0ec18`.., `0x2f0ec24`.., `0x2f0ec28`) with no test
either; its own stack vertex list has 32 entries. Callers: `gr_tmapper` (`0x507a57`) and `gr_tmapper_multipass`
(`0x507c75`, `0x507cd1`), fed by the 32-record `gr_poly_vertices` pool and the clipper of C90. *Inferred:* retail
geometry uses far smaller polygons, so this is latent, like C90 for OpenGL. It needs the Glide renderer: a 3dfx
card or a Glide wrapper.

**C151. More lookups used without a NULL test** (the same kind as C130). In `templeint_v2`, while `tiger_killed` is
not set, `ls_templeint_frame` reads `[eax + 0x5f8]` (the rider's HP) from `tiger_find_rider()` (`0x4f6244`..`0x4f6249`)
on every frame; `tiger_find_rider` (`0x49a010`) returns 0 when no living entity of the rider species is in the
level. Retail `TempleInt_v2_script.tbl` places "Tiger Rider#1". `lsf_lenele1c_flag_found_flece` passes the result of
`party_member_join("Flece", ...)` (`0x4fabf9`..`0x4fac08`) to `character_equip_item` (`0x4fac1d`), which reads
`[esi + 0xc]` from it at once (`0x46be19`); `party_member_join` returns 0 while a save is being loaded
(`savegame_pending_load_length`, `0x439bb1`..`0x439bbd`) or when the name is not a character type (`0x439bd2`..`0x439beb`).
`ls_eleh_frame` reads `[eax + 0xc]` from `party_add_member("Joseph")` (`0x4f1684`..`0x4f1689`, on the "JoinJoseph"
trigger) and `ls_wolong_frame` from `party_add_member` for Rosalind (`0x4fea05`..`0x4fea0a`); `party_add_member`
returns 0 when the name is not in the reserve party (`0x43411b`..`0x434126`, `0x43412f`..`0x43413a`). *Inferred:* in
retail play the rider is there, the Flece trigger runs on a level frame (no load pending) and Joseph and Rosalind are
in the reserve party when those dialogues run, so all four are latent.

**C160. The pending array has no bound.** `determine_point_visibility` walks the graph points that
`pathvis_scan_cells` returns sorted clockwise. Points on one ray from the query point are flagged `PFN_COLLINEAR`
(`0x4b7a09`, `0x4b7a21`). They are collected in a 50-entry stack array (`esp+0x54`, cleared by `rep stosd` of 0x32
dwords at `0x4b839a`..`0x4b83a6`), and their edges are inserted when the group ends (`0x4b8542`..`0x4b8585`). At a
count of 50 (`cmp [esp+0x10], 0x32` at `0x4b84ac`) the code inserts the current point's two edges at once
(`0x4b84c0`..`0x4b84e4`). It still stores the point at `pending[count++]` (`0x4b84e7`..`0x4b84f0`). The
DebugError "Too many points on same sweep line. Get DaveA" (`0x4b86bf`) is on the other branch, which needs flag
`0x247fce8`, and nothing writes that flag. Entry 50 is the saved exception-list link (`esp+0x11c`), 51 and 52 the
handler and try level, and 53 the return address. The flush writes 0 into every entry it used (`0x4b8577`), and the
epilogue restores `fs:[0]` from entry 50 (`0x4b86a3`..`0x4b86ad`). So 51 points on one ray leave a broken exception
chain, and 54 or more make the function return to address 0, or into a graph node when the group runs to the end
of the list. The 51st point's edges are also inserted twice, which removes them again, because `pathvis_insert_edge`
drops an edge it already holds (`0x4b7f41`). The function runs only with `Precalc_vis_method` (`0x245c9d8`, .bss) set
(`pf_compute_endpoint_visibility` `0x49f36b`..`0x49f385`). Only the `precalc_vis_method` console command writes it
(`0x49c88f`..`0x49c8a9`), and v1.40 has no console (only a patch or a `.vcs` script can send a command). Whether a
retail `.pfg` has 51 points on one line was not checked.

**C161. No default case.** Both BSP walks switch on the node type (`dec eax` / `je` at `0x4a5c72`..`0x4a5c7b` and
`0x4b86f2`..`0x4b86fb`). Any other type jumps back to the loop test with the same node (`jmp 0x4a5d16`, `jmp
0x4b8795`), so the walk never ends. `pf_read_typed_refs` stores the type from the file without a test
(`0x49ca44`..`0x49ca53`). `pf_bsp_node_find` is behind `pf_bsp_find_cell`, which `pf_point_is_navigable`,
`pf_graph_search` and `pf_straight_walk_clear` call, so such a node would hang ordinary play. Retail levels play,
so no walked retail node has another type. The `.pfg` files were not scanned.

**C170. A 64-byte line buffer.** `ui_message_box_draw` splits the text with `gr_split_str` (`0x414360`, at most 16
lines, width 280 / 350 / 448 pixels from `0x57efd0`) into start offsets (`[esp+0xa0]`) and lengths (`[esp+0x60]`).
For each line it does `strncpy(line, text + start, len)` into a 64-byte local at `[esp+0x20]` (`0x414370`..`0x414388`)
and writes the terminator at `line[len]` (`0x414399`), with no clamp. The next local is the length array, so a line of
64 or more characters overwrites it, and from 68 + 4 characters on the length of the following line; that line's
`strncpy` then pads up to a garbage length and overwrites the stack. The box holds 255 characters
(`ui_message_box_show`, `0x413e71`). A line needs an average glyph under about 4.4 pixels at 640x480 to get there,
or a word that `gr_split_str` cannot break (not traced). We know of no retail message that comes near it.

**C171. A loop that only stops on a live character.** In multiplayer (`game_flags & 2`) `party_cycle` walks
`party_list` from `party_leader` and calls `living_from_handle` for each record. A NULL result jumps straight back
to the next record (`test eax, eax; je 0x413376` at `0x413392`); only a non-NULL one is compared with the starting
record (`cmp esi, edi` at `0x413396`). So when the leader's own record and every other record have no living
character, the loop never ends. The single-player branch tests for the start record first (`0x4133fa`) and always
ends. We know of no retail sequence that leaves the leader's character gone while the Tab keys still work.

**C180. Ids and counts taken on trust.** `localize_load_names` fills a 1000-entry hash table on the stack
(`[esp+0x39c]`, `rep stosd` of 0x3e8 dwords at `0x504594`) and stores `hashes[id]` with the parsed id and no compare
(`mov [esp+4*esi+0x39c], eax` at `0x50464c`). The table ends at the saved registers and return address, so an id of
1000 to about 1004 or a negative one overwrites the frame. `localize_load_level_table` writes one hash per English
line of the level section into a 500-entry table (`[esp+0x360]`, 0x1f4 dwords at `0x5049c9`) with a moving pointer and
no count (`mov [esi], eax; add esi, 4` at `0x504acd`..`0x504ad3`); line 501 reaches the frame. `localize_set_string`
only tests `id >= 0` before `localize_strings[id] = dest` (`0x50440f`..`0x504417`); `localize_strings` has 1000 slots
and slot 1000 is the start of the text buffer `0x25ce47c`, so a higher id overwrites the first strings' text (and
`localize_load_strings` puts "NULL Format String" at the last id + 1, `0x504389`..`0x504396`). In the level pool
`localize_strdup` copies into the result of `allocate_in_mempool` without a NULL test (`0x5040ac`..`0x5040d2`;
`allocate_in_mempool` returns 0 when the 16-byte-rounded size does not fit, `0x5127d3`..`0x5127da`). Retail data:
the highest ids are 520 (items) and 81 (quests), the longest level sections have 127 topics (`khosanistrng_v6_dlg.tbl`)
and 77 script names (`lenele1b_v2_script.tbl`), `strings.tbl` ends at id 981, and the largest level needs about 7 KB
of the 0x2800-byte level pool. Needs a modded table.

**C181. The length field decides the copy.** For a data packet `nw_work_reliable` takes the length from the
packet's own header (`mov eax, [0x270839c]; and eax, 0xffff` at `0x518391`..`0x518396`), compares it with 0x258 and
ignores the result (`cmp eax, 0x258` at `0x51839b`, no branch), allocates 0x258 bytes (`0x5183b5`..`0x5183ba`) and copies
that many bytes from the receive buffer into it (`rep movsd` / `movsb` at `0x5183e4`..`0x5183ee`). Neither the size
that `nw_buffer_receive` returned (`0x517f1f`) nor the 0x262-byte receive buffer limits it. `nw_NewReceiveReliable`
later copies the same length into the caller's buffer (`0x518c6d`..`0x518c8f`). The game's senders put at most 0x200
bytes into a reliable buffer before sending it (`multi_send_packet`), so only a corrupt or forged datagram on the
game port does this.

**C190. `fogs[i - 1]` after the loop.** `cutscene_get_fog_distances` walks the entry's `$Fog` keys while the current
shot is not below the key's shot, then reads `fogs[i - 1]` (`0x4354eb`..`0x435510`). With no key reached yet (`i` = 0)
that is the dword at entry `+0x74`, the `num_fogs` word and its padding, used as a key pointer: NULL for an entry
without `$Fog`, a small number otherwise. `cutscene_events_frame` calls it on every cutscene frame (`0x4370ea`) and passes
the result to `fog_set_distances`. All 84 retail entries have their first `$Fog` key at shot 0, in shot order, so the
first frame (shot 0) always finds one. *Inferred:* an access violation on the first cutscene frame.

**C191. No list bounds in the cutscene table.** `cutscene_parse_table` stores each `$Camera`, `$Animation` /
`$Character Animation`, `+Minitrack`, `$Fog` and `$Fade` line at the next index of a fixed array in the `0x140`-byte
entry (4, 2 plain and 10 shot effects, 4, 16, 16 slots) and the entries into `cutscene_entries` (85) without comparing
any count; only `cutscene_parse_visset` checks its 16 sets and 16 names. The next field or the next entry is
overwritten; an 86th entry (`0x609f08`) covers the saved fog distances, the `play_and_load_cutscene` and
`stop_cutscene` command objects (`0x609f10`, `0x609f20`) and `cutscene_num_entries` (`0x609f30`). At load time `cutscene_bind_actors` and `cutscene_events_frame`
use the counts as indices into the object's 2 and 10 effect-handle slots, and `cutscene_load_files` fills the
50 `cutscene_sequences` records with every shot of every camera file (only a count of 0 or less fails, `0x434d0e`); a
51st shot overwrites the current-shot index (`0x603460`) and the fields after it. Retail maxima: 84 entries, 4 cameras,
1 plain and 7 shot effects, no minitrack, 13 fogs, 16 fades (exactly at the limit), 13 vissets, and 48 shots
(Act2-Outro).

**C192. Unchecked arguments of `editor_dialog_show`.** The button switch (jump table `0x4315a0`) has no default: a value
above 4 jumps past it (`ja 0x431563` at `0x431039`), so the three button pointers of the `operator new` record keep
whatever the heap held, and `editor_dialog_destroy` deletes them. With an empty text `gr_split_str` returns 0 lines, the
copy loop never runs and the uninitialised 1024-byte stack buffer is measured and shown. All three callers
(`script_write_objects`, `script_save`) pass button set 0 and a formatted message, and the script editor is a
developer tool.

**C193. NULL test that does not guard.** For message `0x18`, `loadgame_level_list_cb` gets the selected entry, skips
only the empty hook when it is NULL (`test esi, esi; je 0x43222c` at `0x43221c`) and then reads `[esi + 0x30]` anyway
(`0x43222c`). The callback is installed only by `state_loadgame_enter` (`0x431f73`), the enter handler of state 8,
which has no `gameseq_set_state` caller ([Main loop](main-loop.md)).

**C200. Every game list entry takes a player record.** With the LAN setting (the Options radio button that clears
`pxo_cfg_flags` bit 0, `options_apply_and_save` `0x41fac7`), `state_mp_gamelist_enter` calls
`mp_local_player_create` each time state 12 is entered (`0x43eed0`..`0x43eed9`). That function takes a record from
the free list (`mp_player_alloc` `0x441610`) and stores it in `mp_local_player` without freeing the previous one
(`0x44176d`..`0x441774`). Create Game (`gamelist_create_game`, state 13) and its Cancel (`gamecreate_cancel`
`0x40a020`, state 12) free nothing, and the game list's leave function only frees the game entries
(`mp_gamelist_free` `0x470820`). Records go back only through `mp_session_reset` (from `mp_charselect_init`
`0x40659a`), `mp_leave_game` or the host's `mp_server_check_clients`. There are four records (`0x6515a0`, 0xbc
bytes each). The first entry from the character screen takes one; after the fourth Cancel the fifth entry gets NULL.
`mp_local_player_create` then only requests state 10 (`gameseq_set_state` just sets the pending state, `0x401020`)
and writes `[eax + 0x10]` with `eax = 0` (`0x4417a6`..`0x4417aa`). With the default Internet (PXO) setting the game
list skips the call, and the lobby skips it when it comes back from the game list (`0x43eff5`), so only LAN mode
leaks. Hosting after one to three Cancels still works: the host's `mp_server_check_clients` frees every listed
record without a socket on its first frame (`0x440c85`..`0x440cdd`).

**C201. Type 0x42 passes the range check.** `multi_process_packet` rejects only types above 0x42 (`cmp cl, 0x42;
ja` at `0x441869`), and the table has 66 records (0..0x41). Record 66 is the string "multi_process_packet: Invalid
packet type: %d\n" at `0x5958f0`. Its rate byte `+0x18` is 'I', non-zero, so the net-rate path copies "INC R " / "INC
U " and then the record's "name" (`+0x10`, "cket: Invalid packet type: %d\n", 31 bytes) to `esp+0x1e`, in a 16-byte
buffer that ends at `esp+0x28` (`0x4418b4`..`0x44191b`). The copy overwrites the return address and the arguments
`data`, `len` and `from`. For a sender in the player list the handler call (`call [ebp + 0x5951bc]`, `0x441970`) then
jumps to 0x72705f69 ("i_pr"). For any other sender (flag byte 'o', not 1, `0x441959`) the loop goes on with
`data` = 0x6b636170 and `len` = 0x74207465 from the copied text (`0x441979`..`0x44198f`), and the function returns
to 0x2064696c. DMY_PCK (0x40) has handler 0 (`0x5958bc`) and flag 0, so it is called for a listed sender only. The
packet's size word is not checked against the datagram length either. The packets arrive through
`multi_process_incoming` (`0x441b10`), which `multi_do_frame` runs whenever `mp_local_player` is set
(`0x4413d0`..`0x4413eb`): in the game list and lobby as well as in a game. Only a crafted datagram in the game's
format on the game port, or a modified peer, sends these types.

**C202. A failed profile read is not handled.** `mp_local_player_create` asks for the chosen profile with
`mp_charselect_get_selected_profile(1)` (`0x441787`), which reads the `.plr` summary again and returns NULL when that
fails (`0x408568`..`0x40857b`). On NULL it requests state 10 and then reads `[esi + 0x48]` with `esi = 0`
(`0x441793`..`0x4417c5`). The character screen only moves on with a selection (`mp_charselect_save_choice`
`0x407466`), so the file has to become unreadable (deleted, locked) between the choice and the next screen. A second
NULL use in the lobby connect step (`mp_charselect_get_selected_profile(0)`, then `lea esi, [eax + 0x28]` after
recording the error, `0x43f12b`..`0x43f150`) needs a selection below 0, which the lobby's own entry already turns
into C202's crash.

**C203. The map name's character type for every record.** `monster_layout_load` calls `charinfo_find_by_name` once,
on the map name read from the header (`0x43feff`..`0x43ff0c`), keeps the result in `ebp`, and passes it to
`living_entity_create` for every record that is not type 7 (`0x4400fd`..`0x440113`). The record's own name, read into
the same buffer (`0x44001e`), is only used to find a type 7 party member by name. A map name is not a character type,
so `charinfo_find_by_name` returns -1 (`0x446bb9`), and `characterinfo_find_multi_slot` / `charinfo_create_character_instance`
then use the 0x3d0 bytes before `char_infos` (`0x6d7e50`) as the type.

**C210. A candidate count that counts twice.** [notes 4] On its second pass (`first_pass` 0), when the enemies'
summed `combat_attack_rating` is more than three times the group's, `ai_group_think_caster` scores every member
that is not blessed (status 4) for Bless (spell `0x13`). It increments the candidate count `ebx` once for such a
member (`inc ebx` at `0x49395c`) and again when its score `10 x attackers + 30 x (1 - HP fraction)` is above 0
(`0x493988`). The count is used as a number of list entries: with Protect (`0x15`) castable and a count above 2
it casts Protect (`0x4939c5`); otherwise it divides the scores of the first `count` entries by the total
(`0x4939f2`..`0x493a05`). The list is six 0x10-byte records at `[esp+0x30]`..`[esp+0x90]` (the 0x80-byte frame of
`sub esp, 0x80`; the notes' "five entries" is wrong, six members fit). A count of 7 rewrites the score slot of
record 6, which is the function's own `group` argument (`[esp+0x98]`); a count of 8 rewrites `ai_group_combat_think`'s
saved `ebp` (`[esp+0xa8]`, the timer pointer of `ai_groups_frame`, which calls `GameTimer__SetTimeout` through it at
`0x492d8b` / `0x492d7d` right after); 9 or more rewrite `ai_group_combat_think`'s return address (`[esp+0xb8]`). The
value written is the old word read as a float and divided by the total, so a pointer becomes a tiny float bit
pattern. A count of 8 needs four members (the party group holds the four heroes and the summon) that are not
blessed and hurt or attacked. The pass runs for style 3 and 5 members that the player does not control, are not
attacked themselves, have more than 25 % AP and got no spell from `ai_choose_support_spell`; Rosalind is
Healer/Caster by default ([Party](party.md#combat-behaviour)). *Inferred:* the crash (or heap write) in game; not
reproduced. G212 is the everyday side of the same count.

**C211. Camera key pools without a bound.** `cutscene_camera_read_track` takes each shot's keys from four pools
that `cutscene_camera_tracks_clear` (`0x494680`, called by `cutscene_load_files` at `0x434ba8`) empties once per
cutscene: 250 position keys of 0x28 bytes (`cutscene_cam_keys40` `0x243e3e8`, count `0x243e3dc`), 250 rotation
keys of 0x14 bytes (`0x2440af8`, count `0x243e3e0`) and two tangent pools of twice that (`0x2441e80`, count
`0x24435f0`; `0x24435f8`, count `0x243e3d8`). The counts are only added to (`0x49408e`..`0x4940d1`,
`0x494174`..`0x4941ba`), never compared. Past 250 keys the position pool runs into the rotation pool, which runs
into the tangent pools, and the last pool ends at `snow_grid_x` (`0x2444598`). The retail `.CSC` files of each
`cutscene.tbl` entry, summed: Machival-Death 14 position / 179 rotation keys, Act2-Outro 109 / 99, summon jade
dragon 97 / 16; every other entry less. **Observed** in code and data; the effect is *inferred*. C191 covers the
shot count, not the keys.

**C230. The missing return.** `ai_goal_evaluate` treats goal types 3, 4 and 5 (casts) together (jump table
`0x475cd4`, entries 2-4 = `0x475989`). For a cast goal that is not the running one (`goal +0x08` != `ai_state
+0x190`), type 4 (cast at a target) looks the target up with `living_from_handle` (`0x4759c4`). A missing target
only sets the result to 1 (`mov ebx, 1` at `0x4759d2`). The code then goes on: when `cutscene_script_is_active`
(`0x4759d7`) is true, it reads the target's object type (`cmp dword ptr [esi + 0xc], 7` at `0x4759e4`) with ESI = 0.
That reads address 0xc and crashes. The intent was to suspend casts at party members during cutscene scripts
(result 3 at `0x475cb8`). The attack and follow cases return as soon as the target is missing (`0x4758a9`,
`0x47591f`). The function is called once per used goal slot by `ai_goals_validate` (`0x47662c`), first thing in
every `ai_process_goals`. A type-4 goal is added by `character_use_skill_on` (the player's cast order),
`ai_group_think_caster`, `ai_combat_cast_frame` and the boss casters. The goal must be waiting, not running, and its
target's handle must no longer resolve, all while `game_flags` `0x20` is set or a cutscene script name is loaded
(`cutscene_script_is_active` `0x4ed770`). *Inferred:* the crash. We have not found a retail sequence that leaves a
waiting cast goal with a deleted target.

**C231. A four-entry edge list without a bound.** `door_find_floor_point` collects the navigation edges of the door
(flag `0x2` at edge `+0x8`, door index at edge `+0xa`) into `[esp + 0x3c]` (`0x476b47`..`0x476b61`). That loop runs
over all `pf_edge_count` edges with no limit. The next local is the cell count `[esp + 0x4c]`, written at `0x476bb5`.
From `[esp + 0x50]` on lies the ray block that the code fills only after a match (`0x476ce4`..`0x476d55`). The loop
variables are below the array (`[esp + 0x10]`..`[esp + 0x1c]`), so the notes' "overwrites the loop variables" does
not hold. What happens:
- With 5 to 35 edges, the fifth entry is replaced by the cell count before it is compared (`cmp esi, [edx]` at
  `0x476bee`). The point between that edge's two ends is never found through it. If no other edge matches, the door
  keeps its old floor point (`door +0x48`..`+0x50`).
- With 36 or more edges, the 36th entry falls on the return address (locals end at `[esp + 0xc8]`).

The only caller is `level_restore_doors` (`0x438a14`). Doors with a path (`door +4` >= 0) take this path. *Inferred:*
the effect. We did not count door edges per door in the retail navigation data.

**C232. The target is read without a test.** `camera_frame` runs the follow steps only when there is a target
(`+0x14c`) and flag `0x2` is set (`0x478a37`..`0x478a42`). It then reads `target->pos` (`0x478a75`..`0x478a90`) and
`target +0x88` (`0x478b5e`..`0x478b6c`) on every frame, with no test. `camera_set_target(NULL)` with a current target
passes `NULL + 0x10` to `distance` (`0x479013`..`0x47901d`). All 21 callers pass `cur_character`, a living entity
from a release path, or a saved target, and `camera_frame` runs only in the game state (`player_input_frame`
`0x4344a6`). So we know no case with no target. *Inferred:* a crash if one happens.

**C220. Three unchecked pointers in the editor.** `script_make_filename` picks the navpoint a new action starts
with. With the Cutscene Script button latched and no cutscene navpoints it falls back to a loop over the level
navpoints that tests `[0x28]`, a field read through a pointer that is still NULL (`mov ecx, dword ptr [0x28]` at
`0x4e8d34`): an access violation whenever the level has any navpoint. (In level-script mode with no level navpoint at
all it returns `&navpoints[-1]`, `0x4e8d43`..`0x4e8d4f`; every retail level has navpoints.) For the Die action
(`script_editor_action_list_kind` 9) `script_editor_apply_action_params` ignores the list choice and calls
`charinfo_find_move(info, 6, 0x22)` again, then reads `+0x8` of the result with no NULL test (`0x4e87ad`..`0x4e87b2`);
the action window offers the category-8 moves exactly when that move is missing (`script_editor_fill_action_lists`).
`script_editor_show_script_dialog` copies `cutscene_script_name` and writes 0 through `strstr(name,
"_cscript.tbl")` (`0x4e7025`..`0x4e7032`). The name is empty after `cutscene_script_unload` (`0x4ed51d`), and the
Cutscene Script button's second action (event `0xf`) unloads the script and only hides the button
(`script_editor_cutscene_button_cb` `0x4e7931`..`0x4e7936`, `script_editor_sync_cutscene_button` `0x4e28f0`); it
stays latched, so Save writes through NULL. Retail cutscene script names all end in lower-case `_cscript.tbl`.

**C221. Copy loops that ignore end of file.** `script_write_level_table` copies the old level table line by line
until a line equals `#Ambient Sounds` (`0x4ea269`..`0x4ea2c9`) and never looks at `TextFile__ReadLine`'s result.
At the end of the file `ReadLine` returns 0 without touching the buffer (`0x516017`..`0x516020`), so the last line
is written to the temp file again and again. `level_items_write_table` does the same until a line starts with
`#End` or equals `#Level: "<level>"` (`0x4ea5b2`..`0x4ea63a`). `ReadLine` also fails without advancing on a line of
0x200 characters or more (`0x51603d`..`0x516047`), and it ends a line only at CR, so a table saved with bare LF
line ends reads as one line. All 49 retail level tables have the exact line and CRLF ends, and `level_items.tbl`
ends with `#End`; a hand-edited table can trigger it.

**C222. No limit for camera navpoints.** The function that sum.symbols calls `script_editor_delete_navpoint` is what
New runs in Camera mode (`script_editor_on_new` `0x4e6a14`). It takes `&navpoints[navpoint_count++]` and fills it
from `camera_view_pos` (`0x4e6a30`..`0x4e6a76`) without the `MAX_NAVPOINTS` compare that
`script_editor_add_navpoint` makes for level and ambient navpoints ("Limit reached - can't add any more level
navpoints").

**C250. A beam hit with no NULL test.** `luminar_beam_start` plays move `0x89` (`Luminar.tbl` "attack_beam":
`$Trigger: "impact start" 88`, `"no interrupt" 0`) and sets AI mode `0x32` (`0x4ca32a`..`0x4ca330`). Each frame
`luminar_ai_frame` takes the target from `ai_state +0x60` and drops it when it is gone or has flag `0x8`
(dying) set (`0x4c95f8`..`0x4c9613`). Mode `0x32` goes to case 2 of the jump table (byte table `0x4c977c`,
entry `0x23`; target `0x4c9722`), which calls `luminar_beam_hit(edi)`. `luminar_beam_hit` tests the pointer only
for the turn toward the target (`test esi, esi` at `0x4ca3b4`). When trigger 7 fires (`0x4ca4d9`) it calls
`character_add_hp(-125)` with ECX = the target pointer and no test (`0x4ca4f0`..`0x4ca502`), and
`character_add_hp` reads `[ecx + 0x320]` first (`0x45085c`). `luminar_ai_beam_move_end` does test the target
for flag `0x8` before a new beam (`0x4ca206`..`0x4ca211`). *Inferred:* a party member who dies of other damage,
or a summon that disappears, during the beam's 88 frames crashes the game at the impact frame.

**C251. A line reader that ignores its size.** `File::read_line(buf, size, min_ver)` copies bytes until NUL, CR
or LF and never looks at `size` (`0x525f81`..`0x525fab`). `effects_parse_arr_line` reads into a 0x404-byte
stack frame and passes 0x400 (`0x51ca77`, `0x51cba8`). Retail `effects.arr` and `ccrunch.arr`
(`summoner.vpp`) have at most 206 and 241 characters per line, so only a modded file triggers it.

**C252. Division by an empty interior.** `draw_arrow` computes the margin `m = (h - 4) / 4 + 2` and the inner
size `h - 2m - 1` (`0x527448`..`0x5274ad`). It skips only a negative inner size (`jl` at `0x5274d9`) and then
divides by it (`idiv edi` at `0x527504` / `0x52750b`; the width branch at `0x5275dd` / `0x5275e6`). A 5-pixel
side gives 0. The script editor's two arrow buttons are 17 x 16 (`0x4e0e45`, `0x4e0e9e`); the slider arrow
buttons take the slider's thickness (`0x55e8ea`..`0x55e9e2`), which was not traced.

**C253. "Any path" passes the path check.** `file_path_id_valid` returns true for `0x98967f`
(`0x512cd4`..`0x512cdd`), and `file_make_path` then reads `file_path_table[0x98967f]`
(`mov edi, [8*ebx + 0x2693068]` at `0x512b8d`). No caller pushes `0x98967f` to it; most pass the path id in a
register, and those were not all traced.

**C254. No bound on the tentacle records.** The free-record search stops at the fourth record
(`cmp eax, 0x248c7e8` at `0x4c6d88`) and then uses the index anyway (`0x4c6d8f`..`0x4c6d9f`): a fifth record lands on
`tentacle_beast_grab_timer` (`0x248c768`) and the battle variables after it. Retail `sewerboss_script.tbl` has
exactly four Tentacle Beasts (`#$npc057`..`#$npc060`).

**C260. light_alloc's -1 is not tested.** `light_alloc` returns -1 when the 400 records at `lights` (`0x2ca5de0`)
are all in use (`or eax, -1` at `0x51e3a4` and `0x51e3af`). `light_create_point` passes the result straight to
`light_get` and writes the record (`0x51e295`..`0x51e2bf`); `light_create_spot` does the same (`0x51e3f6`..`0x51e43d`).
`lights[-1]` is the last 0x94 bytes of `lights_selection` (`0x2c9f9e0`, 16 x 0x640 bytes, ending at `0x2ca5de0`).
`levelrender_setup_level_lights` skips a -1 (`cmp eax, -1` at `0x482df5`), but `ui_3d_begin` stores it in `ui_3d_lights`
(`0x412565`) and `ui_3d_end` hands it to `light_delete` (`0x4125b5`). `light_delete(-1)` finds a type in the
overwritten record and unlinks it through its `+0x00` / `+0x04`, which `light_alloc` never set: two dwords of the
selection table, zero unless that level was used (`0x51e5b5`..`0x51e5c8`). *Inferred:* a write to address 0. In retail
data the largest level has 325 lights (`LPalaceInt.s3d`, header count at file offset 0x1c), plus at most 24 dynamic
lights and the inventory lights, so the table never fills.

**C261. No bound on the line tables.** `ui_listbox_entry__wrap_text` stores each line at
`ui_listbox_line_starts[n]` and `ui_listbox_line_lengths[n]` (`0x52cad3`, `0x52cadc` / `0x52caf5`) with no compare
against 20. Line 21 writes its start pointer into `ui_listbox_line_lengths[0]` (`0x2cbf5d4`), and from line 22 the
lengths land on the UI manager's `.bss` (`ui_left_pressed` `0x2cbf628` on). `ui_listbox__draw` re-wraps every visible
entry and copies each line with `strncpy(buf, start, length)` into a stack buffer of its 0x214-byte frame
(`0x52f308`..`0x52f330`). *Inferred:* with a pointer as the length, `strncpy` pads zeros far past the frame. Only
the script editor fills list boxes in v1.40, with one-line names in boxes 0x6f to 0x17e pixels wide.

**C262. Integer normalisation of packed quaternions.** `cquat__normalized` computes `0xfff8001 / len_sq` with
`idiv` (`0x523088`..`0x523094`) and scales by the square root of the quotient. A key with all four components 0 divides
by zero; a key even one unit longer than 0x3fff gets quotient 0 and becomes zero; a shorter one is left unchanged.
`cquat_log_of_delta` normalises the conjugate of a key (`0x523c7b`), and `anim_track__calc_rot_tangents` uses it for the
squad inner quaternions. Only cutscene camera tracks reach it: `cutscene_camera_read_track` packs the `.csc` floats with
`_ftol` (truncation, `0x4941f7`..`0x49424b`) and calls `anim_track__init_tangents` (`0x4942fc`), and
`cutscene_camera_sequence_sample` samples with squad on (`push 0x1` at `0x49438f`). `skeleton_read` calls
`anim_track__init_tangents` only for `.mvf` versions below 2 (`cmp [esp+0x18], 0x2; jge` at `0x544643`); all 671 retail
`.mvf` files are version 5 or 8. The 1,744 rotation keys of the 94 retail `.csc` files pack to a squared length of
at most 0x3fff squared (exactly that for the identity keys), and none is zero, so the division never fails and never
yields 0. `cquat__normalize` (`0x5232b0`, same `idiv` at `0x5232f7`) has no caller.

**C270. No bound on the area list.** In its first pass `fx_frame` collects every effect object whose spell
(`+0xa8`) is 0x1a, 0x1d, 0x1f, 0x23 or 0x4e (fireball, wall of fire, dark wall, blizzard, Pyrul fireball; case
table `0x48b64c`) and that owns an avoidance area (`+0x100` != -1) into a stack array at `esp+0x230`
(`mov [esp + 4*eax + 0x230], ebp; inc eax` at `0x48a40a`..`0x48a416`). There is no compare. The array has room
for ten entries (`esp+0x230`..`0x257`); the eleventh lands on the saved `edi`, the next three on `esi`, `ebp` and
`ebx`, the fifteenth on the return address (`0x268`) and the sixteenth on the frame time argument. The areas
come from wall of fire (`spell_place_ground_effect` `0x47e100`), blizzard (`spell_spawn_hit_fx` `0x47e590`) and
every fireball hit on a character (`living_entity_get_hit_by_spell` `0x45efe7`..`0x45f022` ->
`spell_effect_obj_set_timer` `0x47e4d0`, kind 7). The effect pool holds 48 objects (`fx_alloc` `0x489cf9`), so
up to 48 can be listed. *Inferred:* several casters with wall of fire or fireball (party AI, Fire Imps, Pyrul)
can reach eleven in a long fight; how long each area lives (its vfx length) was not measured.

**C271. No bound on the sky list.** `fx_create_world` stores every effect whose name contains "sky"
(`stristr` at `0x48cef3`) at `fx_sky_list[fx_num_sky++]` (`0x48ceff`..`0x48cf33`). The list has five entries
(`0x2318e2c`..`0x2318e3f`, cleared by `fx_sky_list_clear` `0x48a1a0`); the sixth lands on `fx_count`
(`0x2318e40`), which `fx_alloc` and `fx_obj_destroy` then step by one, and the eighth on the PXO login pointer
(`0x2318e48`). `levelrender_render_scene` draws `fx_num_sky` entries (`0x482429`..`0x48243a`). Retail: the most
"sky" `$Level_fx` lines in one level table is 2 (`test-water.tbl`), and no `cutscene.tbl` effect name contains
"sky"; the level-script trap callers use fixed fire effects.

**C272. No NULL test on the instance.** `fx_disable_by_effect_name` reads `instance->category` (`mov ecx, [eax + 8]`
at `0x48d67a`, with `eax` = `+0xa0`) for every active effect object. `fx_new` (`0x48c41b`) leaves `+0xa0` at 0,
and only the instance creators fill it, so arrows (`fx_create_missile`'s weapon path stores 0 at `0x48c2bf`),
spell areas (`fx_create_spell_area`, kind 11), status holders (`fx_get_status_holder`, kind 9) and item effects
(kind 16, until loaded) have none. The level-start callers (`level_script_init` `0x4cca5b`..`0x4cca6f`,
`ls_masad_init`, `ls_wolongcaverns_init`, `ls_lenele3d_init`, `ls_masad_reset_fire`) run after
`fx_level_leave` emptied the list, but `tutorial_cond_combat_part2` (`0x42ebe2`, "masad-firedoor2.vfx") runs
mid-level when the Masad Scout `"Orenian Scout#$npc002"` is in the inactive list and has no move. A status
holder exists for every character with an active status (`character_update_status_effects` `0x45bee8`).
*Inferred:* Joseph alone in the first fight rarely has a status, so we know of no retail run that hits it. A
second slip: an effect without a base (`effect_get_base` 0, `0x48d691`) ends the whole search instead of skipping.

**C273. Spell record not checked.** In a network game (`game_flags` `0x2`) kinds 12 and 14 copy 13 dwords from
the spell record at `+0x108` with `rep movsd` (`0x48abcf`, `0x48ae06`, `0x48af99`); kind 11 tests it first
(`0x48a925`..`0x48a92d`). `fx_create_instance` never sets `+0x108`, and `fx_obj_destroy` clears it
(`0x489dda`), so only `spell_cast_launch_effects` fills it (`0x47d978`, `0x47da04`, `0x47da84`, `0x47db6d`),
from `spell_effect_add` (`0x47f660`), which returns 0 when all 64 records (`spell_effects` `0x22e8528`) are used.
`boss_counter_hit_dispatch` creates a kind 14 effect for the Ghost Rider's "+Plus Spell" moves other than ghost
death (`0x4a2293`..`0x4a22ab`; "ghost paralyzing fear" on the stage-3 scythe in `ghost_rider_boss.tbl`) and
never sets it. Retail: the Ghost Rider and Serpent Rider appear in `masad_m1`, `jadetemple_m1` and
`Liangshan_m1` only inside `/* Level script creatures: */` comments or as navpoints; no multiplayer script
creates them.

**C274. Missing NULL test.** `container_obj_create` returns 0 when all 40 containers (`0x2302fe0`) are used
(`0x48980f`) or the bag item type is missing (`0x489879`). `monster_drop_loot` (`0x452ec2`),
`mp_handle_item_drop` (`0x4892dc`) and `savegame_read_item_objects` (`0x4d32b4`) test it; `level_item_spawn`
passes it straight to `container_add_item` as `this` (`0x46a654`..`0x46a681`). Its callers are
`level_items_parse_table`, `level_script_restart` (after `containers_free_all`) and the script editor. Retail
levels place at most 18 items (`sewer` in `level_items.tbl`); whether containers restored from a save can fill
the pool before the level items are placed was not traced.

**C280. New game after the console cutscene viewer.** `play_and_load_cutscene` loads the cutscene's level, sets
`game_flags |= 0x18` and plays the cutscene; bit `0x8` sends the game back to state 1 (the main menu) when it ends
([Cutscenes](cutscenes.md#console-cutscenes-and-pending-playback)). In agent mode on 2026-10-16 (Tiger-Ghost-Intro,
Jade Temple), clicking Start New Game from that menu ended the process with `0xC0000005` about 20 s later
(SummonerFix log: `ExitProcess(3221225477)`). The same command used for TempleINT2, with bit `0x8` cleared by hand
before the cutscene ended, played on normally. *Inferred:* some level or cutscene state is not reset on the way back
to the menu. Not reachable without the console. Cause not analysed yet.

**C290. A 1024-row table on the stack.** `jpeg_write_file` reserves `0x11fc` bytes (`0x5565e0`..`0x5565e5`) and fills
one row pointer per image line at `[esp + 0x20c + 4*i]` for `h` lines with no limit (`0x55661f`..`0x55663a`). The
table ends at `esp + 0x120c`, where the return address sits, so it holds 1024 rows; row 1025 replaces the return
address with a pointer into the pixel buffer, and rows after it overwrite the arguments. `gr_screenshot` passes the
screen clip height (`0x25d8d68`, `0x506214`) for all three JPEG calls (`0x5064bd`, `0x506518`, `0x5065a7`), and
`screenshot_save` takes the JPEG path unless `Print_screen_as_targa` is set (`0x43251e`). The retail resolution
table stops at 1024x768 ([Video settings](video-settings.md)), so the original game cannot reach it. SummonerFix
reads any `GameWidth` / `GameHeight` from `summonerfix.ini` (`main.c`). *Inferred:* with a height above 1024,
Print Screen ends the game when `jpeg_write_file` returns. Confirmed in the code on 2026-10-16: after its four pushes
the frame is `0x120c` bytes and the table starts at `+0x20c`. SummonerFix avoids it: with `GameHeight` above 1024
it switches screenshots to Targa (`Print_screen_as_targa`, `0x5fc022`), whose writer has no row table. Not
tested in game.

**C291. Two unchecked sizes in the alternate textures table.** `alt_textures_parse_table` parses `$Base:` with a
limit of 0x28 characters into rows of 0x21 bytes (`n * 33 + 0x2476fb8`, `0x4aacf3`..`0x4aad0f`), so a name of 33
characters or more runs into the next row. The texture names of a `+Textures:` set are read in a loop that stops
only at the next keyword or `#End` (`0x4aaea4`..`0x4aaee8`), with a word counter at `+0x68` and no test against the 15
bitmap slots. Retail `alt_textures.tbl` has 27 bases, the longest name has 21 characters, and no set has more than
two textures. Found in the `NOTES-unnamed-inventory-popup-join-gtrack.md` notes (D5207).

**C292. Character codes above 127 index a 128-byte table.** `ui_inputbox__set_valid_chars` builds the allowed set in a
128-byte table at `[esp + 0x1c]` (frame `sub esp, 0x8c` plus four pushes) and marks each character with
`xor eax, eax; mov al, [ebx + ebp - 1]; mov byte [esp + eax + 0x1c], 1` (`0x552952`..`0x55295d`, also `0x5528d0`,
`0x5529e0`..`0x5529eb`). The table ends at the return address (`esp + 0x9c`), so a character from `0x80` writes into
it. The script editor's sets are ASCII ("A-Za-z0-9$_-", "01234567890.", "0123456789"); the other caller,
`ui_parse_inputbox` (`0x529576`), only runs for UI tables, which nothing loads (V9). Found in the
`NOTES-unnamed-anim-clip-ui-vfxmesh.md` notes.

**C300. The whole bitmap is copied into each section.** `gr_d3d_create_texture` splits a bitmap into sections of at
most `gr_screen.max_bitmap_w` x `max_bitmap_h` (`0x25d8d6c`, `0x25d8d70`; `idiv` at `0x54d16f`, `0x54d187`), which
`gr_d3d_init_device` takes from the device's maximum texture size, 256 when the driver reports less than 16
(`0x539ba6`..`0x539c54`). Each section gets its own texture of the section's size (`gr_d3d_create_texture_section`
`0x54bcd8`..`0x54bd8f`), but the section function passes the whole bitmap's width and height to the copy
(`0x54bf51`..`0x54bf8e`; the mip loop halves them at `0x54bfe9`..`0x54c006`). In the palettised path the copy starts
at the section's corner (`+0x10`, `+0x14`, `0x54c327`..`0x54c35e`), writes `bm_w` texels per row and `bm_h` rows one
surface pitch apart (`0x54c388`..`0x54c417`), and pads only when the bitmap is narrower or shorter than the texture
(`0x54c3e3`, `0x54c432`). With two or more sections each row runs into the next, the rows past `tex_h` land after the
locked surface, and the source index `(y + row) * bm_w + x + col` runs past the bitmap by `y * bm_w` bytes. The 8-bit
alpha path ignores the section position altogether (`0x54c1ab`..`0x54c28f`, see V301). With one section, which is the
case for every bitmap on a card with 2048 or more, nothing goes wrong. Found in the
`NOTES-unnamed-gr-line-poly-emitter-tcache.md` notes.

**C301. No bound on either navpoint table.** `cutscene_script_load` parses navpoints until `#Objects` and stores each at
`cutscene_navpoints[cutscene_navpoint_count++]` with no compare (`0x4ed281`..`0x4ed2b7`). The table holds 512 records
of `0x30` bytes: `cutscene_script_stringpool` starts at `0x2571750`, `0x6000` bytes after `cutscene_navpoints`
(`0x256b750`), and the script editor stops at `0x200` ("Limit reached - can't add any more cutscene navpoints",
`0x4e5d59`). Record 513 overwrites the string pool object. `script_parse_navpoints` does the same for a level script's
`#Navpoints` (`0x4db776`..`0x4db78a`, called in a loop by `level_script_parse_table` at `0x4cd718`..`0x4cd735`); its
table has 750 records (`navpoints` `0x24e4950`, editor limit `0x2ee` at `0x4e5d7d`), and record 751 lands on
`ccmd_door_toggle` (`0x24ed5f0`). Retail scripts stay well under both: at most 72 cutscene navpoints
(`Game-Intro_cscript.tbl`) and 566 level navpoints (`lenele1b_v2_script.tbl`, `_v3`). Found in the
`NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

**C302. The rider is read without a test.** While `ghost_killed` is not set, the `Rand-ForestNite1_v2` frame handler
calls `ghost_find_rider` and reads the result's hit points (`fld dword ptr [eax + 0x5f8]` at `0x4f583e`). The
`rand-hills01_v2` handler does the same with `phoenix_find_rider` (`0x4fa5fe`). Both finders return 0 when no living
entity has the rider's species (`xor eax, eax` at `0x4b9fe6`, `0x497be6`). The check runs from the first frame, before
the intro cutscene, and stops once the rider's hit points reach 0 and the flag is set (`0x4f5887`, `0x4fa64b`), so only
a script without the rider triggers it. Both retail scripts place their rider. Same kind as C130 and C151. Found in the
`NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

**C310. Two 128-byte path buffers.** `packfile_add` builds `install_path` + the packfile name with
`sprintf("%s%s")` into a 128-byte stack buffer (`sub esp, 0x80` at `0x534425`, `sprintf` at `0x53446c`). The
buffer ends at the return address, and nothing checks the length. `packfile_init` adds the eight packfiles at
start-up; the longest names are `summoner.vpp` and `cutscene.vpp`, 12 characters (`0x533e30`..`0x533e7d`). So an
`install_path` of 116 characters or more (with its trailing `\`) makes the string 128 bytes or longer. The
terminating zero, or the rest of the path, then lands on the saved return address, and the `ret` at the end of
`packfile_add` jumps to a broken address (*inferred*: a crash before the title). `packfile_fopen` repeats the
copy for every packed file it opens, with no check, into its own 128-byte buffer (`sub esp, 0x80` at
`0x534690`, copies `0x534741`..`0x534796`). `file_init` accepts up to 255 characters for the folder (`push 0xff`
at `0x512dc6`). Latent: typical install folders are much shorter.

**C311. The NULL entity is used.** When `living_entity_create` fails (`0x47ff88`; it returns 0, for example with
"No more space for living entities"), `mp_handle_new_lep` calls `mp_player_disconnected(3, 0)` (`0x47ff96`) and
then carries on with the same NULL pointer: `mov edx, [esi + 0x80]` at `0x47ffa1`, then `ai_reset` on
`esi + 0x324` (`0x47ffc1`). *Inferred:* an access violation on the client.

**C312. Resurrecting nothing.** `mp_respawn_player` looks up the player by id (`mp_player_find_by_id`,
`0x481097`). For an unknown id it sets the character to NULL (`xor esi, esi` at `0x4810b6`) and still calls
`character_resurrect`, `character_get_max_stat` and a store to `[esi + 0x5fe]` (`0x4810b8`..`0x4810cb`). It is
reached from `mp_handle_respawn` (packet 0x19) and `mp_respawn_and_notify`. *Inferred:* a crash when a respawn arrives
for a player who has already left.

**C313. ReadLine's result is ignored.** `automap_labels_save` copies the old `automap_labels.tbl` to
`automap_labels_temp.tbl` line by line (`TextFile__ReadLine` at `0x430791`). The loop ends only on a line that
starts with `#End` (`strncmp` at `0x4307a5`) or on the current level's `#Script:` line (`0x4307d1`..`0x4307ff`).
It never tests whether ReadLine read anything, so a file without `#End` makes it write the same line forever.
The only caller is the `save_map_labels` console command (`cmd_save_map_labels`). Retail: the shipped
`automap_labels.tbl` is just `#Automap Labels` and `#End`, so it is latent.

## Gameplay

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| G1 | The game runs at double speed on modern PCs | `FrameTimer` `0x501510` | **Observed** at runtime | Fixed |
| G2 | The game runs many times too fast when the QPC frequency is 4.29 GHz or more | `timer_init` `0x532840` | **Observed** code | No (Timeslip's patch fixes it) |
| G3 | Cutscenes skip almost at once after about 24.9 days of Windows uptime | `cutscene_sequence_update` `0x436a90` | **Observed**, reproduced | Fixed |
| G4 | With the save list scrolled, saving on the top row overwrites a save without asking | `saveload_try_save` `0x426320` | **Observed** code, *inferred* effect | No |
| G5 | Buying with a full inventory takes the gold and gives no item | `buysell_buy_item` `0x4057f0` | **Observed** code, *inferred* effect | No |
| G6 | At game stages 4 to 8, the Zane grassland encounter gets no extra chance | `0x4fc4b0` (grass picker) | **Observed** code, *inferred* effect | No |
| G7 | If a cutscene's data fails to load, input stays locked and the music stays off | `cutscene_play` `0x4361d0` | **Observed** code, *inferred* effect | No |
| G8 | When any party record is removed (for example a summon dies), control may jump to the first member | `party_remove_player` `0x4335a0` | **Observed** code, *inferred* effect | No |
| G9 | Alt+Tab does not work: the game takes the focus straight back | `os_wndproc` `0x530ff0` | **Observed** at runtime | Fixed |
| G10 | Latent: adding gold to a container that already holds gold loses the gold and leaves a stale slot | `container_add_item` `0x46aa30` | **Observed** code, *inferred* effect | No |
| G11 | Latent: using aimed attack (or forge, mine, enchant) as a skill takes the AP and does nothing | `character_use_skill` `0x429410` | **Observed** code, *inferred* effect | No |
| G12 | Spell cooldowns are the spell's record number in seconds, not the table's recovery time (fireball 26 s instead of 20; monsters' AI recovery times likewise) | `spells_parse_table` `0x47a190` | **Observed** code and retail data, *inferred* effect | No |
| G13 | The blind / cripple / slow chance of an aimed attack (and of blindness stab, arm slice, leg swipe) uses the selected character's aimed attack skill, not the attacker's | `combat_apply_melee_hit` `0x465df0` | **Observed** code, *inferred* effect | No |
| G14 | Multiplayer: when a player closes a container after taking some items, the other machines drop the container's last entries instead of the taken ones | `mp_handle_container_close` `0x488eb0` | **Observed** code, *inferred* effect | No |
| G15 | Damage spells roll each die from 0 to die-1 (lightning d6 averages 2.5), while healing dice roll 1 to die | `living_entity_get_hit_by_spell` `0x45dea0`, `spell_roll_amount` `0x47f4d0` | **Observed** code and retail data; intent *inferred* | No |
| G16 | Latent: with all 20 missile slots taken, the next spell missile is not fired | `spell_launch_missile` `0x47e6c0` | **Observed** code, *inferred* effect | No |
| G17 | Latent: if an offered item is gone when a trade completes, the partner gets it anyway and the `add_gold` console command is corrupted | `mp_trade_complete` `0x4718d0` | **Observed** code, *inferred* effect | No |
| G18 | Multiplayer: a lock picked through the host can stay locked on the other machines, or another container unlocks there | `mp_handle_container_lockpick` `0x489660` | **Observed** code, *inferred* effect | No |
| G30 | Boss moves aimed by offense point are measured from about the world origin: Pyrul's slash and fire breath, Machival's area attacks and every Giant Salamanka melee hit should rarely land; the Ghost's sweep hits only its main target | `boss_ai_parse_table` `0x4a1790` | **Observed** code and retail data, *inferred* effect | No |
| G31 | A boss's `+Plus Spell:` (Serpent sickle poison; Ghost paralysing fear and "ghost death") lands even when the hit itself is dropped as out of range | `living_combat_resolve_hit` `0x467df0` | **Observed** code, *inferred* effect | No |
| G32 | Melee party members fight the mounted Phoenix Rider in melee (unless the Serpent is in its stage 1), and the mounted Ghost Rider always | `boss_species_mounted_stage` `0x4a2480` | **Observed** code, *inferred* effect | No |
| G40 | A loose gold pile on the ground holds only 1 gold after the game is loaded | `savegame_read_item_objects` `0x4d31e0` | **Observed** code, *inferred* effect | No |
| G41 | With more than 32 loose gold piles in a level, the extra piles are gone after a load | `savegame_write_item_objects` `0x4d2370` | **Observed** code, *inferred* effect | No |
| G42 | Latent: a door whose name plus the level script name has 64 characters or more makes every save of that level fail to load | `savegame_read_two_stage` `0x4d34e0` | **Observed** code, *inferred* effect | No |
| G50 | After a reload or a second visit to the Khosani lab, the flipper puzzle's board no longer matches the flippers: raised flippers show no lava platform, and the next presses move the wrong flippers | `ls_khosanilab_init` `0x4f8920`, `ls_khosanilab_set_flippers` `0x4f9ca0` | **Observed** code, *inferred* effect | No |
| G51 | Latent: once the 1000 path nodes run out, every failed path search keeps its nodes until the next level load | `pf_graph_search` `0x49fa70` | **Observed** code, *inferred* effect | No |
| G52 | Latent: a path to a goal outside the walkable area usually fails instead of ending at the area's edge | `pf_find_cell_crossing` `0x49ecb0` | **Observed** code, *inferred* effect | No |
| G60 | Latent: when the chat server connection fails at once (any error except "would block"), the multiplayer lobby never reports it, retries every frame and opens a new socket each time | `irc_connect` `0x4abc10` | **Observed** code, *inferred* effect | No |
| G70 | If a cutscene starts while a summon is still waiting to emerge, every later summon of that creature type ends its emerge at once, until the game is closed | `summon_stash_for_cutscene` `0x448000` | **Observed** code, *inferred* effect | No |
| G71 | Latent: a misspelled `$Skill:` name in a character table overwrites the record's up-direction skill; a misspelled direction skill becomes id `0x54`, past the skill table | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data | No |
| G72 | Latent: a character with more than two `$Default Weapon:` lines gets wrong materials on its first weapons and, from the eighth line on, junk weapons | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data, *inferred* effect | No |
| G100 | In single player the "leech" special ability of the Blood Elemental, Crystal Rokhul and Wraith (Joseph's summon) does nothing to HP | `living_entity_get_hit_by_spell` `0x45dea0` | **Observed** code and retail data, *inferred* effect | No |
| G110 | Latent: a `#Clicks` name that is not a level object makes level object 0 clickable with that entry's message and gold | `level_script_parse_table` `0x4cd5f0` | **Observed** code and retail data, *inferred* effect | No |
| G111 | Latent: a door `+Key:` that names no item can never be opened with a key | `level_script_parse_table` `0x4cd5f0` | **Observed** code and retail data, *inferred* effect | No |
| G130 | Latent (the game refuses saving there, tested): after loading a game saved in the last temple level after Machival's defeat, walking to the forge no longer starts the first ending; only the temple exit (the second ending) still ends the game | `ls_templeint2_init` `0x4ef190`, `ls_templeint2_enter_forge` `0x4ef5d0`, `savegame_location_allowed` `0x4d4b5e` | **Observed** code and in game (save refused) | No |
| G131 | Latent (the game refuses saving there): on the second Jade Temple visit, a game saved in the 8 s after "Joseph acquired Summon Spirit of Laharah!" loads into a temple that never fades out to the world map, and the level has no exit | `lsf_tiger2_dead` `0x4f0360`, `ls_jadetemple_init` `0x4effe0`, `savegame_location_allowed` `0x4d4b45` | **Observed** code and retail data, *inferred* effect | No |
| G132 | In Liangshan, General Wentao's wave can arrive, and "That must be the last of them." can be said, while Orenian Samurai#w3a of the third wave is still fighting | `lsf_liangshan_flag_wave4` `0x4f5280`, `lsf_liangshan_flag_finished_quest` `0x4f5320` | **Observed** code and retail data, *inferred* effect | No |
| G140 | Latent: after a cutscene, an item it took off goes back into a wrong equipment slot when the item's type slot is not the usual one for that hand or body part | `script_action_replace_weapon` `0x4dccb0`, `script_action_replace_armor` `0x4dce60`, `cutscene_script_unload` `0x4ed390` | **Observed** code and retail data, *inferred* effect | No |
| G160 | Latent: `multi_path_type 2` is accepted (and reported as "Unknown"), and a machine set to it drops every path it receives, so other players' characters stop on that screen | `cmd_Multi_path_type` `0x4811d0`, `mp_handle_new_path` `0x4801f0` | **Observed** code, *inferred* effect | No |
| G170 | A "Cannot save..." warning shown while the player has paused the game resumes the game when it is closed | `ui_message_box_show` `0x413e40`, `ClearFrameFlag5bd504` `0x414020` | **Observed** code, *inferred* trigger | No |
| G180 | Latent: in multiplayer, a reliable packet from an address with no link (for example a machine whose link has timed out) that comes after a connected machine's packet in the same update is handled as that machine's: an ACK can free its unacknowledged packets, and data enters its stream | `nw_work_reliable` `0x517e00` | **Observed** code, *inferred* effect | No |
| G181 | Latent: in multiplayer, once a link's receive sequence number reaches 0xffb6, a resent copy of a packet already delivered is stored again, and it is delivered in place of the real packet with that number when the sequence comes round (about 65,536 packets later) | `nw_work_reliable` `0x517e00`, `nw_NewReceiveReliable` `0x518bb0` | **Observed** code, *inferred* effect | No |
| G182 | Latent: in multiplayer, when the first IAMHERE send of a join fails, the link is cleared but the joining machine carries on as connected | `nw_ConnectToServer` `0x518f30`, `mp_handle_acpt_jn` `0x470a80` | **Observed** code, *inferred* effect | No |
| G210 | Monster groups (and the party's group) notice a foe beyond their detection range only when it is to the side of or behind a member, never when it is straight ahead: a party walking up to monsters head-on is noticed later (Archlich: within 3 m instead of 10 m) | `ai_group_collect_enemies` `0x493ac0` | **Observed** code and retail data, *inferred* effect | No |
| G211 | A party member on the Support combat style (Flece by default) ends up attacking a random enemy: its group order always adds an attack on the first enemy of the shuffled list after the target it chose | `ai_group_think_style1` `0x493290` | **Observed** code, *inferred* effect | No |
| G212 | An AI healer casts Protect on the party as soon as two members are unblessed and one of them is hurt or attacked, not when more than two need it | `ai_group_think_caster` `0x4936c0` | **Observed** code, *inferred* effect | No |
| G213 | In the mounted Serpent fight (stage 1), after the Serpent changes target its "is this target asleep" test looks at the first or second hero in its list instead, so it may knock back a sleeping new target or leave an awake one for another | `serpent_choose_attack` `0x496330` | **Observed** code and retail data, *inferred* effect | No |
| G214 | In the on-foot Phoenix fight (stage 3), when two party members lie knocked down and the point beside them has no path, the Phoenix walks for up to 5 s toward the mirror image of that point through the level origin | `phoenix_reposition_update` `0x497da0` | **Observed** code, *inferred* effect | No |
| G215 | The Phoenix Rider's Inferno (stages 1 and 3) hits its target once for every party member, and knocks every party member down for 5-10 s wherever they stand | `phoenix_special_strike` `0x499890` | **Observed** code and retail data, *inferred* effect | No |
| G216 | The Tiger Rider's choice among several heroes it sees uses only the last hero's threat as the common base of the weights, not the sum, so it leans harder toward the heroes that hurt it than the other boss pickers do | `tiger_pick_target` `0x49b0a0` | **Observed** code, *inferred* effect | No |
| G230 | Latent: when all five goal slots of a character are in use, a new order whose priority is not above the lowest one is not stored but overwrites that order's target, point, spell or radius, so the character can attack, follow or walk to the wrong place | `ai_goal_alloc` `0x476020`, `ai_goal_add` `0x476190` | **Observed** code, *inferred* effect | No |
| G220 | Latent: the script editor's Save writes a character's actions without its `$Name:` record when the character starts on a navpoint of the other kind (a camera, ambient or cutscene navpoint in a level script; a camera or ambient one in a cutscene script); on the next load those actions belong to the character before it in the file | `script_write_objects` `0x4e9ce0` | **Observed** code, *inferred* effect | No |
| G221 | After an ambient sound edit, the script editor's Save rewrites the level table, which all script variants of the level share, with only the ambients whose navpoints are in the script being edited: saved from a variant such as `sewer_v2`, `masad_v2`, `Wolong2_v2`, `LPalaceInt_v2` or `TempleInt_v2` it deletes every ambient sound of the level, and from a base script it drops the 8 retail `$Ambient:` lines that no script defines | `script_write_level_table` `0x4ea1e0` | **Observed** code and retail data | No |
| G222 | In a level whose `level_items.tbl` section serves more than one script (`sewer`, `IonaExt02`, `LPalaceInt`, `Wolong2`, `Rand-Iceland01`, `Rand-OreniaNite01`), the script editor's Save can write the other scripts' level items a second time, under a navpoint of the current level | `level_items_write_table` `0x4ea500`, `level_items_parse_table` `0x4cb430` | **Observed** code and retail data, *inferred* trigger | No |
| G223 | Deleting a navpoint in the script editor shifts the later navpoints down but leaves level items, cutscene script actions and dead characters pointing at the old slots, so Save names the neighbouring navpoint for them; the "in use" check does not look at any of the three | `script_editor_on_delete` `0x4e6ad0`, `script_replace_navpoint_references` `0x4e6f10`, `script_editor_navpoint_in_use` `0x4e6d90` | **Observed** code, *inferred* effect | No |
| G224 | In the script editor, deleting the last action of a character's cutscene script drops the last level script record instead, so Save writes that character without its level-script actions | `script_editor_show_action_params` `0x4e7cd0` (deletes the action, despite the name) | **Observed** code, *inferred* effect | No |
| G225 | In the script editor, removing a level item leaves the indices of the later level items stale, so editing another navpoint's item afterwards changes the wrong record (or reads past the list) | `script_editor_level_item_ok` `0x4ecbe0` | **Observed** code, *inferred* effect | No |
| G226 | The script editor's creature window sets every skill except skill 2 to 100 on OK, whatever the level, and with level 0 ("party leader's level") sets the creature's base HP and AP to 0 | `script_editor_creature_ok` `0x4ec730` | **Observed** code, *inferred* effect | No |
| G227 | The script editor's Save drops every `+Prop:` line (the item a scripted character holds): 18 lines in 7 retail scripts (`sewer`, `IonaEXT`, `lenele2aa`, `lenele2aa_m1`, `Rand-grassland01_v3`, `_v4`, `_v5`) | `script_write_objects` `0x4e9ce0`, `script_parse_objects` `0x4dba30` | **Observed** code and retail data | No |
| G228 | In multiplayer, Pyrul's wall of fire is always followed by a meteor storm | `pyrul_fire_attack` `0x4b9670` | **Observed** code and data, *inferred* effect | No |
| G229 | An AI healer (party Healer / Healer-Caster members and enemy healers) casts a support spell only with a chance equal to the largest single share of the scores, and prefers the first candidates in its list | `ai_choose_support_spell` `0x4bea80` | **Observed** code, *inferred* effect | No |
| G240 | Wraiths choose the target of Life Drain from scores that still hold the previous spell test's values, which pushes the choice towards the first candidates | `ai_consider_life_drain` `0x4bf5a0` | **Observed** code, *inferred* effect | No |
| G241 | Enemy casters decide whether to cast meteor storm or inferno from stale stack values instead of the party members' distances | `ai_combat_cast_frame` `0x4bfaa0`, `ai_consider_area_spell` `0x4befe0` | **Observed** code, *inferred* effect | No |
| G242 | When a straight move crosses more than one layer-transition edge, the game takes the crossing nearest in x only, which can put a path, a walk test or a scripted avoid spline on the wrong floor layer | `pf_first_edge_crossing` `0x4a53d0` | **Observed** code, *inferred* effect | No |
| G243 | Latent: when a new avoidance obstacle that merges two or three avoidance polygons cannot be clipped against the walls, all but the first merged polygon keep their corners switched off, so paths no longer go around those areas until they are rebuilt | `avoid_create_obstacle_polys` `0x4a9440` | **Observed** code, *inferred* effect | No |
| G250 | The Tentacle Beast's tentacles never pick up and throw a party member, although the move ("Luminar grab" / "Luminar throw") is in `Tentacle Beast.tbl`; they only swipe and squeeze | `tentacle_beast_ai_select_action` `0x4c6520` | **Observed** code and retail data, *inferred* effect | No |
| G251 | Latent: a tentacle swipe that hits five party members at once makes that tentacle unkillable: at 0 HP it never plays its death move or counts as lost | `tentacle_collect_area_hits` `0x4c77c0`, `tentacle_beast_check_stage` `0x4c7360` | **Observed** code, *inferred* effect | No |
| G252 | After the first 5 s of the fight, Murod Oni may switch to a backward bite at a hero behind it on every think (30% each time), not once per timer period | `murod_oni_ai_select_action` `0x4c5de0` | **Observed** code and retail data, *inferred* effect | No |
| G253 | Murod's leap can land him beside a dead party member, and the Luminar's teleport can bring it next to a dead hero and make that hero its target | `murod_start_leap` `0x4c51a0`, `luminar_teleport_out` `0x4c9da0` | **Observed** code, *inferred* effect | No |
| G270 | When the caster of a fire wall, fireball fire, blizzard or dark wall is gone, a character standing in that area stops being burnt or frozen by it, and by every area listed after it; their burning, freezing or dark wall statuses are cured at once | `fx_frame` `0x48a230` | **Observed** code, *inferred* effect | No |
| G271 | AI casters often skip fireball although no fire spell is burning: a status effect holder or an item effect that reuses the slot of an earlier fireball, wall of fire or Pyrul fireball effect still carries that spell | `fx_new` `0x48c3e0`, `fx_count_spells_1a_1d_4e` `0x48bd40`, `ai_choose_offensive_spell` | **Observed** code, *inferred* effect | No |
| G290 | Latent: a ray against a level object whose geometry tree has more than 128 pending crossed nodes misses the faces under the next node | `lobj_raycast_bsp` `0x4aeef0` | **Observed** code, *inferred* effect | No |
| G300 | In single player, the Robe of Restoration's Heal (20% when Rosalind is hit or parries) heals the enemy who hit her, not Rosalind | `combat_trigger_reflex_ability` `0x467670` | **Observed** code and retail data, *inferred* effect | Fixed (`bugfix.c`) |
| G301 | Choosing "Give him 10 gold pieces." for Canis the Mercenary in occupied Lenele takes 100 gold (all of it below 100) | `ls_lenele2ab_action` `0x4f1220` | **Observed** code and retail data | No |
| G310 | In multiplayer, a player's summon never gets its once-a-second position update to the other machines; the leader's update is sent twice instead | `mp_send_local_updates` `0x47f9c0` | **Observed** code, *inferred* effect | No |

**G1. Frame limiter.** `FrameTimer` stores `last = now` (`0x501577`) before it sleeps (`0x5015bc`). The
next frame counts the same sleep again. See
[Timing bugs](timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed).

**G2. QPC frequency.** `timer_init` keeps only the low 32 bits of `QueryPerformanceFrequency`
(`mov ecx, [esp+8]` at `0x532859`, stored at `0x532868`). Modern Windows reports 10 MHz, so most PCs are
not affected. See [Timing bugs](timing-bugs.md#1-qpc-frequency-truncation-4-ghz-bug).

**G3. Signed subtract.** The start time is subtracted as a signed 32-bit value (`fisub dword` at
`0x436afc`). After 2^31 ms of uptime every shot looks overdue. See
[Timing bugs](timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime).

**G4. Row index instead of list index.** The overwrite test reads
`saveload_entries[selected_row].level` (`cmp [edx+0x5e3640], -1` at `0x426352`) without adding
`saveload_scroll_pos`. Entry 0 is the `<New Savegame>` row, so when the list is scrolled the top visible
row skips the "Overwrite existing savegame?" box. `saveload_save_to_slot` does add the scroll position
(`0x4263a0`), so it writes over the real save. See [Interface screens](interface.md).

**G5. Gold first, item second.** `buysell_buy_item` subtracts the price (`0x4058b7`) and then calls
`party_inventory_add_type`. That function returns without adding a new item type when the inventory
has 0x209 entries (`cmp edx, 0x209` at `0x46b9e4`). Hard to reach. See
[Shops and loot](shops-loot.md#buying-and-selling).

**G6. Missing jump.** The Zane case copies `rand-grassland01` and script `_v3` (`0x4fc9d8`..`0x4fca20`)
and then falls through into the general rolls at `0x4fca22`, which overwrite both names. See
[World map](worldmap.md#encounter-table).

**G7. Early return.** `cutscene_play` locks input and stops the music, then returns when
`cutscene_load_data` fails (`jmp 0x4365d9` at `0x43634c`) without setting `game_flags` bit `0x20`.
`cutscene_cleanup` does nothing while that bit is clear (`0x436630`), so nothing is restored. Retail
files are all present, so this needs a missing or broken `.csc` file. See
[Cutscenes](cutscenes.md#what-cutscene_play-changes).

**G8. Leader re-picked every time.** After unlinking any record, single player sets the leader to the
first living member of `party_list` (`call party_set_leader` at `0x433638`), whether or not the removed
record led. In multiplayer it calls `party_set_leader` for every local member (no break, `0x433623`).
The intent is unknown. See [Party](party.md#leaving-and-coming-back).

**G9. Focus loss re-activates the window.** The `WM_ACTIVATE` handler calls `SetWindowPos` with flags 0
on losing focus too (`0x5310e9`), and flags 0 include activation. See
[Controls](controls.md#switching-away-from-the-game-alttab).

**G10. The merge branch falls into the insert.** When slot 0 already holds gold, the new count is added to it
(`0x46aa79`..`0x46aa9a`) and the new gold object is deinit-ed (`call [edx]` at `0x46aaa8`). The code then jumps
to the common tail (`jmp 0x46aae5`). That tail stores the deinit-ed object in slot 0 over the merged gold
(`mov [edx+0x98], eax` at `0x46aaf7`) and increments `num_items` (`0x46ab06`). The merged sum is lost. The new
last slot was never written, and the slot-0 object is free in the pool again. Callers: `monster_drop_loot`
(`0x452f00`), `level_item_spawn` (`0x46a681`, always a new container), `mp_handle_item_drop` (`0x4892fa`) and
`savegame_read_item_objects` (`0x4d3341`). A monster drop creates at most one gold object (`item_create_misc_by_name("Gold")`
at `0x450fee`), so we know of no retail path that adds two.

**G11. AP first, action second.** `character_use_skill` pays the AP for every skill outside the 0x12..0x33
switch range, so also for 0x11 (`lea ecx, [edi-0x12]; cmp ecx, 0x21; ja` at `0x42948f`, then `character_add_ap`
at `0x4294b6`). The action switch has no case for 0x11 or 0x18..0x1a, so it returns false. The skills panel
sends aimed attack to its popup (`skills_handle_mouse`), and the popup uses the per-part skills (eyes, arms,
legs). The panel also refuses to drag 0x11 or 0x1b onto a hot button (`cmp esi, 0x11` at `0x4288aa`,
`cmp esi, 0x1b` at `0x4288af`). The creation skills have no icon (`skills_icon_names[24..26]` = 0), so no retail
path reaches the bad case. Found in the [`new_skill.cpp`](../../src/decomp/game/new_skill.cpp) notes.

**G12. Record index stored for three fields.** `spells_parse_table` parses `+Recovery time:`, `+AI Recovery time:`
and `+Loop Time:` and then stores the loop counter `bp`, not the parsed value (`mov word [esi-0x12], bp` at
`0x47a49c`, `[esi-0x10]` at `0x47a4c1`, `[esi-0xe]` at `0x47a50b`; `ebp` is the record index, `inc ebp` at
`0x47a648`). `TextFile__Accept` compares with `_strnicmp` (`0x516134`), so the table's "+AI Recovery Time:" and
"+Loop time:" spellings do match. `character_perform_spell_cast` passes `+0x66` to
`character_start_spell_recovery` (`0x45dc7a`), which starts the cooldown as the cast effect's length plus that
value (`0x44dcf0`..`0x44dd1f`). For type 5 characters (the type of a summon that turned hostile, see
[Summons](summons.md)) it uses `+0x68` instead (`0x44dce6`); `ai_consider_area_spell` also reads `+0x68`
(`0x4bf068`). Retail `spells.tbl` has `+Recovery time:` on six spells: invisibility 0 -> 11, fireball 20 -> 26,
wall of fire 20 -> 29, dark wall 20 -> 31, blizzard 25 -> 35, Pyrul fireball 20 -> 78. Twenty spells have
`+AI Recovery Time:`, for example vitalize 10 -> 4, death 30 -> 12, curse 10 -> 23, ice coffin 25 -> 36.
Spells without the line keep their `+Duration:` (or -1, no cooldown) as before. `+Loop Time:` is read only by
`status_effect_duration` for status 8 (`movsx eax, word [0x22e5512]` at `0x47ab90`, spell 11 invisibility,
10 -> 11 s). That is the tick period of the status, and `character_update_status_effects` has no tick action
for status 8 (case byte 3 at `0x45bf3c + 3`), so this part has no effect. See
[Spells](game-tables.md#spells).

**G13. `cur_character` instead of the attacker.** The three status branches load `mov ecx, [0x5fcc88]` before
`character_get_skill(0x11)` (`0x4663c0`, `0x466468`, `0x466510`) and compare the result with `frand()`. The
cases also serve blindness stab, arm slice and leg swipe. In multiplayer every machine resolves other players'
hits with its own selected character. In single player the aimed attacks are started through
`character_use_skill` for the selected character, so the wrong skill is used only when the selection changes
before the hit lands. Monsters never pick these skills (`living_combat::ai_choose_special` only takes ids below
`0x24`).

**G14. The container handle is read for every item.** The loop pushes `ebp`, the container's net handle from
`data + 4`, for each taken item (`push ebp` at `0x488f00`), not the item handles that `mp_send_container_close`
appends after the count (`0x488e08`..`0x488e2a`). So `container_remove_item` gets the container itself. It does
not find it, but still decrements `num_items` (`0x46abf5`..`0x46ac04`). Each taken item therefore drops the
*last* entry on the receiving machines (the host too, which then forwards the packet). When every item was
taken the count reaches 0 and the container is deleted, as intended. Otherwise the taken items stay listed
there and as many untaken ones vanish from the list.

**G15. Negated die.** For damage the handlers pass the table's die negated (`neg eax` at `0x45e4d6` for
lightning; nine `neg` sites in all), and `spell_roll_amount` rolls `floor(frand() x die + 1)` per die
(`0x47f51e`..`0x47f549`). With die 6 that is 1..6 for a heal but 0..-5 for damage. Lightning (d6) and the other
damage spells with a `+Die:` in retail `spells.tbl` lose one point per die on average. A designer may have tuned the tables to this, so the intent is *inferred*. See
[Combat](combat.md#spell-hits).

**G16. Both exits return.** When the free-slot search fails (`0x47e705`..`0x47e716`), the function clears the
slots whose effect object is gone (`0x47e71d`..`0x47e745`) and then searches again, but both outcomes return
(`je 0x47e821` at `0x47e74f`, `ret` at `0x47e765`). The missile is never recorded, so `spell_fire_missile`
finds no slot (`spell_missile_remove` returns -1) and creates no missile. Slots are also freed when a missile
fires, when its caster dies or plays a new move (`fx_remove_missiles_from`, callers `0x4538cf`, `0x460fcd`) and
on entering a level (`spells_reset_runtime`), so 20 pending missiles at once are needed. We know of no retail
fight that reaches that.

**G17. No test for -1.** For each offered item that is not gold, `mp_trade_complete` subtracts the count at
`party_inventory[party_inventory_find_type_index(type)]` (`0x471914`..`0x471929`) without testing for -1. Slot
-1 is `0x970818`, the handler pointer of `ccmd_add_gold` (`0x970810`, set by `console_command__console_command`
at `0x50a1dc`). The pointer stays positive, so `party_inventory_remove` is not called. The offered item stays in
the inventory, the partner still gets it, and "add_gold" later jumps into the wrong code. The offer is built
from the inventory with merged types and the count checked (`itemtrade` click handler), and the trade is
cancelled when the character dies, hides or leaves AI mode 1 (`NOTES-multi.md`). We found no way to lose an offered item while the
trade screen is open.

**G18. Host handle used without mapping.** On success the host clears the lock (`mov [ebx+0xe4], 0` at `0x489564`)
and sends its own object handle (`container_lockpick` `obj_from_handle` at `0x4894b9`, stored by
`mp_send_lockpick_result` at `0x489614`). The receiver looks it up with `obj_from_handle` (`0x4896a2`), where
`mp_handle_container_open` uses `mp_obj_from_net_handle` (`0x488d5b`). That function maps the handle through
`mp_net_handle_map` (`0x22e94f4`) on a client (`0x475265`..`0x47526b`). When the handles differ, the lookup misses
and the lock stays on that machine, or it finds another container (type test `0xb` at `0x4896b0`) and clears that
one's lock (`0x4896ce`). `object_use_container` tests the local lock (`fld [ecx+0xe4]` at `0x43cc7c`), so on that
machine the container still reports itself locked. *Inferred:* how often host and client handles differ was not
measured. Found in the `NOTES-multi.md` notes (bug 3).

**G30. Tag names parsed as numbers.** `boss_ai_parse_table` reads `$Offense point:` as a string and stores
`atoi(string) - 1` in the move record (`0x4a18b7`..`0x4a18c4`). All 89 records of the nine loaded retail
tables give a tag name (`"$offense01"`..`"$offense07"`), so `atoi` returns 0 and every move has point -1.
The loaders keep the found `$offenseNN` tags in packed 0x1c-byte records, with the count just before the array
(Pyrul `0x247fdb4`). The point tests index that array with the move's number and no check (`lea ecx,
[8*eax]; sub ecx, eax` at `0x4b9c2b` and in the four other tests below). Point -1 is therefore the 0x1c bytes
before the array: its position `+0x10` / `+0x18` is a word nobody writes and the count (an integer read as a
float, about 0); for Pyrul it is the fifth threat record. Its direction is near zero. The tests are
`pyrul_point_in_attack_cone` (Pyrul move classes 0 and 2, slash and fire breath), `ghost_point_in_attack_cone`
(Ghost class 4), `machival_offense_point_reaches` (Machival's filter and kinds 3 and 7),
`giant_salamanka_target_in_reach` (every Giant Salamanka single-target hit, through
`boss_move_in_range_dispatch`, and its action choice at `0x4c8a32`) and `murod_oni_offense_point_reaches`
(only Murod Oni's "in reach" target flag). For the cone tests, the target must be within the move's range of
about (0, 0) and, with a zero direction, the move needs a max angle of 90 degrees or more (`dot >= cos`). When
the test finds nobody, the resolver still returns true for Pyrul (`0x4a2c36`) and Machival (`0x4a2c9d`), so no
fallback hit is made. The Ghost's resolver returns false (`0x4a2be1`), so its main target is still hit through
`living_combat_resolve_hit` with the plain edge distance (`boss_move_target_in_range`); only the extra victims
are lost. *Inferred:* the Pyrul, Machival and Giant Salamanka moves deal no melee damage unless the target
stands near the world origin. This needs a
runtime check (agent mode). See [Bosses](bosses.md#boss-table-files).

**G31. Spell before the range test.** `living_combat_resolve_hit` calls `boss_counter_hit_dispatch(attacker,
defender)` at `0x467e93` and only then, for a type-9 attacker, `boss_move_in_range_dispatch` (`0x467ea8`),
which ends the hit when it fails. The dispatcher hits the defender with the move's spell when the Serpent
Rider's or Ghost Rider's current move has flag `0x10` (`+Plus Spell:`): spell `0x54` through
`living_entity_get_hit_by_spell` (`0x4a228c`), others through `fx_create_spell_effect_at` (`0x4a22ab`). The
hit count `+0x40` is set when the swing starts (`living_combat_roll_defense` `0x464112`), and the range is
tested at the hit frame, so a target that steps out of range during the swing still gets the spell. Retail
moves with the flag: Serpent stage 1 sickle ("poison"), Ghost stage 3 scythe ("ghost paralyzing fear") and
claw ("ghost death").

**G32. Wrong stage variable.** `ai_get_combat_style` returns style 2 (Range) instead of 0 (Melee) while the
character's target is a mounted rider (`0x473644`). `boss_species_mounted_stage` tests `tiger_stage == 1 or 2`
for species `0x57`, but `serpent_stage` (`0x2451d98`) `== 1` for both `0x58` (Phoenix) and `0x59` (Serpent),
and returns false for `0x5a` (Ghost; `jg 0x4a249f` at `0x4a248e`). `phoenix_stage` is `0x2452420`, and
`character_target_point_count` in the same file reads it correctly (`0x4a21e8`). So in the mounted Phoenix
fight (`rand-hills01`, stage 1) the result depends on whatever `serpent_stage` holds from an earlier Serpent
fight, and the Ghost is never treated as mounted. In `Liangshan` both riders are on foot (forced stage 3), so
the result is right there. The callers include the group orders, `ai_score_target`, `ai_combat_approach` and
`character_get_attack_range`.

**G40. The amount is skipped.** `savegame_write_item_objects` saves each loose gold pile as x, y, z and its amount
`+0x9c` (`0x4d2550`..`0x4d2559`). The reader takes x, y and z, then skips z and the amount together (`add esi, 8` at
`0x4d33a7`), and creates the pile with `item_obj_spawn(item, pos, matrix, 1, -1, -1)` (`push 1` at `0x4d33c2`).
`item_obj_spawn` stores its fourth argument at `+0x9c` (`0x46a619`..`0x46a622`), and `item_obj_pickup` adds `+0x9c`
to `party_gold` (`0x46b6ff`..`0x46b70d`). Nothing else writes `+0x9c` of the pile after the load. A loose pile is
what `monster_drop_loot` leaves when gold is the monster's only loot (`0x452e38`..`0x452e48`); the amount is
`_ftol(rand x level x constant) + 1` (`0x450f80`..`0x450fa4`). So a pile that was not picked up before saving gives
1 gold after loading. Gold inside a container keeps its amount, because the container items are read with their
saved count (`0x4d3307`, `0x4d3326`). The loaded pile is also dropped to the floor again by `item_obj_spawn`'s
probe.

**G41. A silent limit of 32.** The gold-pile loop stops at 32 (`cmp ebp, 0x20; jge` at `0x4d24e3`) and the save still
succeeds. The level does not recreate monster drops, so piles 33 and later are lost on load.

**G42. A length check that fails the load.** For every saved two-stage hash of the current level, the reader walks
the doors and builds `level_script_name + door name` in a 64-byte buffer. Before each comparison it tests the
combined length and returns 0 when it is 64 or more (`cmp ebx, 0x40; jae 0x4d3730` at `0x4d3588`, repeated at
`0x4d35ac`). `savegame_read` then prints `"12a"` and the load ends in the main menu (`gameseq_set_state(1)` at
`0x438211`). The test runs for every door it compares, not only the matching one, and only when the level has at
least one saved two-stage entry. `savegame_two_stage_get` and `savegame_two_stage_set` build the same string with
`strncpy` / `strncat` and no test. We did not check the retail door names.

**G50. The board is cleared, the doors are not.** `ls_khosanilab_init` clears the 10 x 7 board `ls_khosanilab_flippers` on every
entry (`rep stosd` of 0x46 bytes, `0x4f895c`..`0x4f897e`) and then hides the platform of every cell
(`ls_khosanilab_apply_lava_platforms`, `0x4f8980`). The flippers are doors, and a door's state is its two-stage bit in
`savegame_state`, which `door_start_move` flips (`savegame_two_stage_set` at `0x476fe4`). That bit is kept across level changes
and saved. A press toggles the used cell (`0x4f9b66`..`0x4f9b79`) and its four neighbours through `set_flippers`, which
toggles the cell and calls `door_apply_saved_state(name, cell)` (`0x4f9cc4`..`0x4f9cf4`). That moves the door only when its
bit differs from the new value (`0x47739f`..`0x4773ad`). So after a reload, a flipper that was raised keeps its door raised
while its cell reads 0: its platform stays hidden, the beam grid (`ls_khosanilab_puzzle_build_grid`) is built from the board
and not from the doors, and the first press on a neighbour leaves that door where it is while the cell becomes 1. The board
catches up only as cells are toggled. *Inferred:* how far this blocks the crossing was not tested; it needs agent mode.

**G51. `ai_path_free(NULL)`.** When the graph search has found the goal, it builds the path from the goal backwards with
`path = ai_path_node_create(x, z, layer, path)` (`0x4a0180`..`0x4a019b`). When the pool is empty `ai_path_alloc_node` returns 0
(`0x4a02d7`..`0x4a02f2`), the loop jumps to `0x4a01b5` and calls `ai_path_free(0)` (`push 0` at `0x4a01b5`), which does nothing
(`test eax, eax; je` at `0x4a0484`). The nodes already chained, including the goal node from `ai_path_node_create_b`
(`0x49fbec`), are lost. Only `ai_path_pool_init` refills the pool, from `pathfinding_load_graph` (`0x49ce04`). So each failure
shrinks the pool for the rest of the level and makes the next failure likelier. We know of no retail fight that uses 1000
nodes at once.

**G52. The z test is reversed.** For a goal outside every cell, `pf_graph_search` looks for the nearest crossing of the
start-to-goal segment with a cell edge (`0x49fafd`..`0x49fb08`) and gives up when there is none (`0x49fb3d`).
`pf_find_cell_crossing` tests each cell's bounding box first. A cell keeps `max z` at `+0x0c`, `min z` at `+0x10`, `min x`
at `+0x14` and `max x` at `+0x18` (`pf_compute_graph_bounds` `0x49cb17`..`0x49cb74`, `pf_find_nearest_navigable_point`
`0x49e972`..`0x49e9ad`). The x test is a normal overlap (`seg min x < cell max x`, `seg max x > cell min x`, `0x49ed42`..`0x49ed60`),
but the z test is `seg min z < cell min z` and `seg max z > cell max z` (`0x49ed66`..`0x49ed84`): only cells whose whole z
extent lies inside the segment's are examined. A segment that runs mostly along x finds no crossing, and the search returns
no path. Clicked destinations are pulled onto the mesh first (`world_click_move` calls `pf_find_nearest_navigable_point`,
`0x43bf26`), as are most AI targets, so *inferred:* this needs a goal that no caller clamps (see "Still unclear").

**G60. An error returned as "in progress".** `irc_connect` makes a non-blocking socket and calls `connect`
(`0x4abdb6`). When that returns -1 it sets `irc_connecting` (`0x247a4dc`) only for `WSAEWOULDBLOCK` (10035,
`cmp eax, 0x2733` at `0x4abdc5`), but returns 0 for every error (`xor eax, eax` at `0x4abdd3`). Its only caller,
the lobby connect step that `state_mp_lobby_frame` jumps to every frame until lobby flag 1 is set (`jmp 0x43f110`
at `0x43f0bb`), treats 0 as "still connecting" and returns (`test eax, eax; je` at `0x43f181`). Only -1 sets
`lobby_connect_state` to the error state (`0x43f18e`). With `irc_connecting` still 0, the next frame runs the whole
connect again: a new `socket` (`0x4abce5`) whose handle overwrites `0x2479740`, and the old socket is never closed.
The usual failures still get through: a refused or timed-out connection shows up later in `select` on the
connecting socket (`0x4abea7`..`0x4abed4`). Only an immediate error (no route to the host, an address that cannot
be used, a firewall that rejects the call) takes this path. PXO's chat servers are gone, so this needs a
replacement server. Found in the `NOTES-target-chattracker-multispell.md` notes.

**G70. The shared move record is edited.** When the summon has not yet started its emerge (object flag bit
0 clear, AI not in mode 6), `summon_stash_for_cutscene` starts it with `ai_begin_emerge` (`0x448068`),
which plays the emerge move and stores its start time (`character_play_move`, `0x461005`). It then
fast-forwards the anim and subtracts `character_move_trigger_time_left(id) + 0.1` from each of the three
trigger times (`fsubr`/`fstp [esi-0x8]` at `0x4480c4`..`0x4480c7`). The record is `cur_move`, the move in
`move_infos` that every character of that char_info (and its alternates) shares. The tables load once per
session, so the edit lasts until the game exits. Elapsed time is 0 at this point, so each trigger becomes
about -0.1 s. A later stash of the same type subtracts 0, so the times do not drift further (the note's
"shifts again" does not hold). `summon_emerge_update` (`0x4b4660`) ends the emerge when the move is finished
or trigger 6, "critical end", has fired (`0x4b46e9`..`0x4b4701`). *Inferred:* with that trigger at -0.1 s,
each later summon of that creature leaves its emerge state on the first update instead of at frame 80 to
499 (for example the Golem's 304). The window is the delay before the emerge (`ai_frame` waits for the
GameTimer at `ai_state +0x1a0`, `0x472c38`), so it needs a cutscene trigger right after a summon is cast.

**G71. Search end used as an index.** The skill name search stops at the count, 84 (`cmp esi, 0x595cb4` at
`0x445d93`), and the value is stored at `[edi+ebp+0x94]` (`0x445dad`). `skill` is 0x54 bytes
(`mov ecx, 0x15; rep stosd` at `0x445bad`), so index 84 is `+0xe8`, the up skill. The four direction lines
store the same 84 when the name is unknown (`0x445c13`). Id 84 is past `skill_table` and reads
`resistance_level_names` (`0x8b5098`) as its first fields. Every skill name in the retail tables matches.

**G72. No bound on the default weapons.** `$Default Weapon:` indexes `char_info +0x116` (two shorts) and
`+0x11c` (two materials) by the running count `+0x114` with no compare (`0x44583f`, `0x44587e`..`0x44589c`).
The third line writes the padding at `+0x11a` and `+0x124`. The fourth to seventh write their item indices
over the two materials already stored at `+0x11c`/`+0x120`. Materials from the third line on land in
`+0x124`..`+0x168`, which the prop tag code resets afterwards (`0x445b07`..`0x445b26`: prop tag 0x11, tag
names 0). So weapons 8 and 9 read back as item types 0x11 and 0, and later ones as 0. `living_entity_init`
creates one item per counted weapon (`0x44e8e9`..`0x44e911`). No retail record has more than two
`$Default Weapon:` lines.

**G100. Leech value overwritten.** Case `0x3e` (spell 62, "leech") stores
`strength x 0.05 x power` in `amount` (`fld [ebp+0x20]; fmul [0x58021c]; fimul [ebp-0x3c]; fstp [ebp-0x24]` at
`0x45f471`..`0x45f47d`) but leaves the dice count and die at 0. After the switch, every case calls
`spell_roll_amount(target, spell, count, die, ...)` and stores its result over `amount` (`fstp [ebp-0x24]` at
`0x45febe`). With count 0 the roll returns 0 (`test eax, eax; je 0x47f653` at `0x47f4ff`, `fld [0x57eb84]` at `0x47f653`), so `character_add_hp(0)` changes nothing.
In multiplayer, `spell_prepare_cast` computes the same value and keeps it in the cast request
(`0x47baa6`..`0x47babb`), so only single player loses it. `combat_trigger_special_ability` takes the
single-player path through `living_entity_get_hit_by_spell` (`test dl, 2` at `0x46714a`). Retail users:
`+Special Ability: "leech"` in `Blood Elemental.tbl` (100 10), `Crystal Rokhul.tbl` (40) and `Wraith.tbl` (100).
*Inferred:* what leech was meant to do (drain the target or heal the caster) is not known; `spells.tbl` marks it
`+Beneficial`, `+Target character`, `+Hit only`.

**G110. Index 0 for "not found".** `lobj_find_by_name` returns -1 for an unknown name, and the parser then
pushes 0 instead of the index (`push ebx` with `ebx = 0` at `0x4cdbb6`) and stores `lobj_from_index(0)` as the entry's
object (`0x4cdbe2`). Every retail `#Clicks` name appears in its level's `.s3d` (a string search, not a parse of
the object list).

**G111. Type -1 stored as the key.** With `+Locked:` and a non-empty `+Key:`, the parser stores
`&item_types[item_type_find(key)]` (`shl eax, 8; add eax, 0x8cbec0` at `0x4cda11`) without a test for -1, so an
unknown key name becomes the address 0x100 bytes before the item table. The 13 retail key names are all in
`items.tbl`.

None.

**G130. The forge ending depends on a byte the level init clears.** `ls_templeint2_enter_forge` starts the first
ending (party back, "End1-EnterForge", then "End1-Montage" and the credits) only when `mach_killed` is set, the
player is near `$end2trig`, and `ls_templeint2_end2_ready` (`0x257e9b8`) is set (`0x4ef5ed`..`0x4ef5f4`). That byte is
in .bss, cleared by `ls_templeint2_init` on every entry (`0x4ef1cf`) and set only by
`ls_templeint2_machival_death_cs_end` (`0x4ef588`). The level sets `level_script_event_active` to 0 when the act 4
narration ends (`0x4ef287`) and never back to 1, so saving is allowed after the fight. A game saved after
Machival-Death therefore loads with `mach_killed` set and the byte 0: the forge does nothing. The lenele3a exit tests only
`mach_killed` (`ls_templeint2_use` `0x4ef790`..`0x4ef7a2`) and still plays "End2-LeaveTemple" and "End2-Montage".
*Inferred:* the player gets only the second ending after such a load. New in this check.

**Not reachable in retail play (tested 2026-10-16).** `savegame_location_allowed` also has a per-level rule
(jump table `0x4d4b7c`, index `0x4d4b98` by `cur_level_index` + 1): in TempleINT2 (level 37) it refuses saving once
the story flag `mach_intro` is set (`0x4d4b5e`..`0x4d4b76`), that is from Machival's introduction on, through the
fight and after it. In agent mode (TempleINT2 entered with `play_and_load_cutscene Nar-TempleAct4`) the Save button
showed "Cannot save in The Temple of Urath." after the introduction. So no save made after Machival's death
exists, and the cleared byte never matters. With the byte set, standing on `$end2trig` started End1-EnterForge
at once; with only `mach_killed` set (the state a load would give) nothing happened, as the code says. A fix is
not needed.

**G131. A timer that does not survive a load.** In `jadetemple_v2`, once the four riders are dead,
`lsf_tiger2_dead` sets `got_laharah_spirit`, shows the summon message, starts the 8 s `ls_jadetemple_tiger2_timer` and
clears `level_script_event_active` (`0x4f053e`..`0x4f0574`). On later frames the set flag leads only to the timers:
when the 8 s are up it starts the 5 s fade (`0x4f057c`..`0x4f05a8`), and when the fade ends it loads `worldmap1` at
entry 5 (`0x4f05d9`..`0x4f0670`). `ls_jadetemple_init` invalidates both timers on every entry (`0x4f0049`..`0x4f0058`),
and an invalid `GameTimer` never reports elapsed (`0x501914`). A game saved in those 8 s loads with the flag set and
both timers invalid, so the level change never comes. `Jadetemple.tbl` has an empty `#Triggers` section, so the
level has no exit of its own. *Inferred:* the player is stuck in the Jade Temple and must load an older save. The
fade closes screen 4 every frame (`0x4f05c3`..`0x4f05d1`), so only the 8 s before the fade are a risk. New in this
check.

**Not reachable in retail play.** The per-level rule of `savegame_location_allowed` refuses saving in the Jade
Temple (level 34, read in agent mode) once `got_laharah_spirit` is set (`0x4d4b45`..`0x4d4b5d`), and
`lsf_tiger2_dead` sets that flag in the same call that starts the 8 s timer (`0x4f053e`..`0x4f0574`). So the 8 s
window cannot be saved in. **Observed** code; the refusal itself was seen in game for TempleINT2 (G130), not yet
for the Jade Temple. A fix is not needed.

**G132. Wave 4 checks two of the three wave 3 enemies.** `lsf_liangshan_flag_wave3` sends `Orenian Samurai#w3a`,
`#w3b` and `Orenian Mage#w3` (`0x4f5248`..`0x4f5261`). `lsf_liangshan_flag_wave4` starts the next wave (Greater
Mage, two soldiers, General Wentao, and Flece's "Here comes Wentao and his soldiers!") when `#w3b` and the mage are
dead (`0x4f5299`..`0x4f52b9`); `#w3a` is not tested. `lsf_liangshan_flag_finished_quest` tests only the wave 4 enemies
(`0x4f533d`..`0x4f5383`). In `Liangshan_script.tbl`, `#w3a` is a hidden level 21 Elite Samurai that shows itself and
runs to `$npc197` on "go", like `#w3b`. *Inferred:* when the player kills `#w3b` and the mage first, the fourth wave joins
a fight that is still going on, and the end-of-quest remark can come with `#w3a` still alive. Found in the
`NOTES-level-scripts-8.md` notes (bug 3).

**G140. A stale slot.** When a cutscene takes off an item, the record's slot is set from the item type's slot:
weapon 4 or two-handed 9 goes back to hand 5, shield 5 to hand 6 (`0x4dcd55`..`0x4dcd87`); torso 6 or heavy 8
to body slot 7, legs 7 to slot 8 (`0x4dcf94`..`0x4dcfb9`, `0x4dcffe`..`0x4dd023`). Any other type slot leaves
the record's slot as it was: 0 (neck) for an unused record, or the slot of an earlier cutscene's record.
`cutscene_script_unload` puts the item back into that slot (`character_equip_item` at `0x4ed3bf`). In retail
data the items taken off always have the usual slots: every "replace weapon" is "(none)", and the armour
actions use "Tunic", "Leather Jerkin" (torso) and "Bougu-Dou" (heavy) from `items.tbl`.

(none)

**G160. A third path mode with no code.** `cmd_Multi_path_type` accepts 0 to 2 (`jl` / `cmp eax, 2` / `jg` at
`0x4811e8`..`0x4811ef`) and prints 2 as "Unknown" (`0x481222`). `mp_handle_new_path` first frees the character's
current path (`ai_path_free`, `0x48026b`). It then builds the received nodes for type 0 (`0x4802d2`..`0x4802da`), or
for type 1 calls `ai_path_find_new` / `ai_path_find_append` (`0x480381`..`0x480401`). Type 2 does neither, and the
type-0 tail is skipped too (`0x480425`..`0x480434`), so the character is left with no path. `multi_path_type`
(`0x59bca8`) is a local setting of each machine, and only the console sets it. The help text also says "machiens".

**G170. Unpause without a pause.** A message box with flag 1 pauses a single-player game: `ui_message_box_show`
calls `game_pause` (`0x413f96`), which does nothing when the game is already paused (`game_is_paused`, `jne` at
`0x432667`). Closing the box (a button or Esc, `ui_message_box_handle_click` `0x4140e8`) runs
`ClearFrameFlag5bd504`, which calls `game_unpause` whenever flag 1 was set (`0x41402c`..`0x414034`), without
remembering whether its own pause took effect. The HUD keys still work while the game is paused (Space unpauses
through the same `hud_handle_keys`, `0x40deea`), and the save key and the quick-save key have no pause test
(`0x40df22`..`0x40df2f`, `0x40dff5`..`0x40e002`). Their warnings "Cannot save in combat.", "Cannot save during
summon.", "Cannot save now.", "No more savegame slots." and the other quick-save errors use flag 1 (`push 0x1` at
`0x40e1af`, `0x40e862`, `0x40e910`, `0x40e92b`). So a player who pauses a fight and presses the save key gets the
warning, and the fight goes on as soon as it is closed.

**G180. A peer pointer kept from the last packet.** `nw_work_reliable` clears its link pointer (`ebx`) once, before
the packet loop (`xor ebx, ebx` at `0x517e14`). For each packet that is not a connection request it searches the nine
links for the sender's address and sets `ebx` only on a match (`0x518044`..`0x518081`). The "unknown sender" test
then only checks `ebx` (`test ebx, ebx` at `0x518088`), and the loop goes back to `0x517f0d` without clearing it. So the
first stray packet of an update is only logged ("Received from %s", `0x5180b2`), but a stray packet after a known one
runs the known link's code: it refreshes its time-out (`0x5180cc`), an ACK frees the link's sent buffer with that
sequence number (`0x518232`..`0x518283`) and adds a ping sample, and data is stored in the link's receive buffers and
ACKed to the known machine's address (`0x518391`..`0x518413`). Stray packets come from a machine whose link this side
has dropped (time-out, `0x518734`) but which still sends.

**G181. The wrap test lets old packets in.** The receive test for data (`0x5182f8`..`0x51834a`) first drops packets
more than 148 away from the expected number (`cmp ecx, 0x95; jge` at `0x518319`). Below 0xff6a it rejects numbers
below the expected one. From 0xff6a on it computes `expected + seq + 0x96` in 16 bits (`add ax, di; add eax, 0x96`
at `0x51833c`..`0x51833f`) and accepts the packet when that is at most 0x95. That sum needs `0x10000 - expected`
instead: as written it accepts an old number when `expected + seq >= 0x1ff6a`, which happens from expected 0xffb6 on.
The duplicate test only looks at packets still buffered (`0x51835f`..`0x518372`), so a resend of a packet already
read (its ACK was lost) is stored with its old number and ACKed. `nw_NewReceiveReliable` delivers the buffer whose
number equals the expected one (`0x518c33`..`0x518c50`) and then increments it (`0x518cc5`), so the stale copy waits
until the number comes round again and is delivered then; the real packet with that number is refused as a
duplicate but still ACKed (`0x518364`..`0x518369`, `0x5183ff`..`0x518413`), so its data is lost.

**G182. The indices survive the clean-up.** `nw_ConnectToServer` takes a free slot, puts it in limbo (status 5), and
stores the slot in `nw_server_socket` and in the caller's `*socket` (`0x519118`..`0x519128`) before it sends IAMHERE.
When `nw_send_heartbeat` returns -1 it clears the whole slot (`0x519137`..`0x519150`) but not the two indices.
`mp_handle_acpt_jn` ignores the return value and tests only `mp_local_player +0x18 != -1` (`0x470ab9`), so it sets the
join result 2 and goes on (`0x470ac4`..`0x470ad3`). Every later `nw_NewSendReliable` on that slot fails ("Can't send packet
because of status 0", `0x518dd7`..`0x518df3`). *Inferred:* the join never completes. It needs `sendto` to fail
right after the connect request to the same address succeeded.

(none)

**G210. The cone test is the wrong way round.** [notes 2] A member sees a living enemy when it is within its
`$Field of view range:` (`char_info +0x220`) and in line of sight. Inside `$Detection range:` (`+0x224`) that is
enough (`0x493be8`). Beyond it, unless the member is blind, the game normalises the direction to the enemy and
dots it with the member's forward vector (`orient.fvec`, object `+0x50`): `fcomp [0x581ae0]` (0.71) and
`jne 0x493c71` on C0 (`0x493c51`..`0x493c5c`) accept the enemy when the dot product is **below** 0.71, i.e. outside
the 45-degree cone ahead. `script_guard_sees_player` uses the same constant the other way: below 0.71 it returns
false (`0x4ce882`..`0x4ce893`). Retail characters have, for example, field of view 10 and detection 3
(`Archlich.tbl`); most detection ranges are 4-5. *Inferred:* in game, monsters ahead of the party between those
ranges are not put in their group's enemy list until the party is inside detection range.

**G211. A missing return.** [notes 3] After the "protect a Range / Healer / Caster member" loop (which returns
properly at `0x49344c`), `ai_group_think_style1` picks the nearest enemy nobody attacks, else the enemy of the most
hurt member, else one with attackers, else the first, and orders the attack (`0x493545`). It then falls through to
`ai_group_order_attack(member, ai_think_members[0], group, 1)` (`0x49354d`..`0x49355f`), the "first enemy"
fallback. `ai_goal_add_attack` adds a type-1 goal and `ai_goal_add` removes the old type-1 goals first
(`0x47619f`), so the second order replaces the first. `ai_group_shuffle_enemies` shuffles the list on every think
(`0x493ce0`), so the first enemy is a random one. The member then keeps that target (the function returns early
while it has a living target). Species 2 (Flece) gets style 1 by default (`living_entity_init`, see
[Party](party.md#combat-behaviour)).

**G212. Protect at two.** [notes 4] The doubled count of C210 also decides the group spell: `cmp ebx, 2; jle` at
`0x4939c5`. One member that is not blessed and hurt (or attacked) already gives 2; two such members, or one plus
one unhurt, give 3 or more, and the healer casts Protect (`0x15`) instead of Bless on one member. *Inferred:* the
test was meant as "more than two members".

**G213. An index that is a flag.** [notes 7] In stage 1, on the normal-attack path, a target re-pick by
`boss_target_list_pick` is followed by `boss_target_list_find_b` for the new index (`0x496678`) and then
`serpent_part_is_asleep(list, asleep)`: the argument is the previous asleep flag (`mov eax, [esp+0x2c]; and eax,
0xff; push eax` at `0x49667f`..`0x496688`), so candidate 0 or 1 is tested instead of the new index in `esi`. The
result feeds `serpent_part_is_busy` (`0x4966a2`) and the choice between the stage-1 knockback (move `0xba`, "Rider
Stun", only when not busy, `0x4966ce`) and the sickle (`0xbb`), and whether it re-picks away from a sleeping
target (`0x496706`..`0x496751`). Asleep means sleep with more than 10 s left and no poison; the Serpent's own
stage-1 Nightmare (`serp_boss.tbl` "stage1 nightmare", spell "Nightmare") is what puts heroes to sleep.

**G214. A negated point.** [notes 10] With the dive flag (`phoenix_flags` 2) in stage 3, when two or more party
members are knocked down (AI sub-mode `0x2d`), `phoenix_reposition_update` paths to a point 4.5 m beside their
midpoint (`ai_path_find_new` at `0x497f42`). When that fails, the second try negates all three coordinates of that
world point (`fchs` at `0x497f5f`, `0x497f65`, `0x497f75`) and paths there (`0x497fae`); a path found sets the
travel mode and a 5 s wander timer. The mirrored side point (`mid - dir x 4.5`) was presumably meant. G215 makes
two members down at once a common case.

**G215. One distance for everyone.** [notes 12] For spell `0x1e` (`inferno`) the move trigger walks
`party_list`, and for every living member compares the squared distance **between the target and the boss**
(`[esi+0x10]` minus `[edi+0x10]`, `0x499900`..`0x49992f`) with the spell's distance squared (`spells[0x1e]`
`+Distance:` 6, `0x22e5e98`). Inside it, it creates the effect on the target and calls
`living_entity_get_hit_by_spell(target, 0x1e, boss)` (`0x499962`), then knocks **the member** down for
`frand_range(5, 10)` s (`ai_knock_down` `0x49998b`). `phoenix_boss.tbl` gives Inferno a range of 0-3 m in stages
1 and 3 ("phoenix inferno1", "stage3 inferno"), so the test always passes: the target takes the hit once per
party member (up to five with the summon) and the whole party is knocked down. Members on a mount or already down
are refused by `ai_can_be_knocked_down`. *Inferred:* each member's own distance and one hit each were meant.

**G216. Assigned, not summed.** [notes 14] With two or more candidates, `tiger_pick_target` starts the base at
10.0 (`0x49b205`) and, for every candidate with a threat record, sets it to `-(threat[1] + threat[0])`
(`fld; fadd; fchs; fstp [esp+0x18]` at `0x49b235`..`0x49b243`): the last such candidate wins. Each weight is then
`base x 0.1 + 10 - (3 x threat[0] + threat[1])`. The generic `boss_target_list_pick` sums the same quantity
(`fsubr [esp+0x14]; fstp [esp+0x14]` at `0x4a3266`..`0x4a326a`), so this looks like `=-` for `-=`. Threats are HP
deltas, negative for damage, so the base is a common offset; a smaller base makes the pick lean more toward the
heroes that hit it. *Inferred* intent and effect; the effect is mild.

**G230. A slot index for a slot that was not taken.** `ai_goal_alloc` looks for the slot with the lowest priority
that has no flag `0x1` (`0x476048`..`0x476069`). Empty slots have priority -1 (`ai_goal_remove` `0x47632c`), so they
come first. When the chosen slot holds a goal, the new goal is stored only if its priority is higher or it has flag
`0x1` (`cmp ecx, [edi + 4*eax]; jle 0x476124` at `0x476088`..`0x47608b`). Otherwise the stores are skipped, but ESI
still holds the index, and the function returns it (`0x476178`). `ai_goal_add` takes any index >= 0 as a new slot
(`0x476272`). It writes the new goal's fields into it, chosen by the new type's `ai_goal_type_fields`: target
`+0x28` (`0x47628d`), point and layer (`0x4762a7`..`0x4762c3`), spell `+0x24` (`0x4762d7`), float `+0x2c` (`0x4762ec`).
The old goal keeps its type, flags, id and priority. Example: a new attack overwrites a waiting cast-at-target goal's
target, and a new move-to overwrites a wander goal's point. `ai_set_combat_mode_goal` calls `ai_goal_alloc`
directly and stores nothing after it, so it is not affected. For the bug to show, all five slots must be full.
Attack, travel and cast orders first remove the old goals of their own kind (`0x47619f`..`0x4761e9`), which frees a
slot. So the five slots must hold other kinds: follow, combat mode, move-to, wander, backstab, sneak or hide. *Inferred:*
the effect; no retail sequence was confirmed.

**G220. A header that depends on the navpoint type.** In a level script `script_write_objects` writes `$Name:` ...
`$Start position:` only when the start navpoint has type 0 (`cmp [eax+0x28]; je` at `0x4e9e00`..`0x4e9e05`); in a
cutscene script only for type 1, and type 0 gives the `+No create` header (`0x4e9e3d`..`0x4e9e47`,
`0x4e9ff5`..`0x4ea075`). Any other type jumps straight to the action loop (`0x4e9e07`, `0x4e9ff7`), which writes the
`+Action:` lines when the character has a script object. `script_parse_objects` reads `+Action:` lines with
`TextFile__Accept` after a record (`0x4dbf8c`), so they join the record before. Creature mode places a character on
any hovered navpoint (`script_editor_place_creature` `0x4e58a0` tests only "already the start" and "same point"), so
the trigger is a deliberate placement plus an action.

**G221. Only the current script's ambient navpoints are written back.** The new `#Ambient Sounds` section has one
`$Ambient:` per navpoint of type 3 in the loaded script that has an emitter at exactly its position
(`0x4ea2ff`..`0x4ea344`); an emitter of -1 is skipped (`0x4ea359`). Everything else in the old section is replaced.
`level_parse_ambient` looks a name up in the current script's navpoints and then among the level-file markers
(`level_marker_find`, `0x4cc0ea`..`0x4cc105`); marker positions are not in the navpoint table. The level table is one
file per level, but the ambient navpoints are defined per script, and many variant scripts define none of them
(retail `tables.vpp`: `sewer_v2`/`_v3`/`_m1`/`_m2` 37 of 37 missing, `masad_v2` 37/37, `Wolong2_v2` 38/38,
`LPalaceInt_v2` 24/24, `TempleInt_v2`/`_v3`/`_m1` 9/9, `lenele1d_v2`/`_v3` 8/8, `lenele1e_v2` and `tancredhouse_v2`/`_v3`
4/4, `Rand-HillsNite01_v3` 19/36, `lenele1ab_v2`/`_v3` 7/26, `lenele1b_v2`/`_v3` 2/41). Eight names are in no script
at all (`IkaemosExt` `$torch_15`, `IkaemosBottomInt2` `$ambient015`, `IonaExt` `$ambient001`, `Rand-HillsNite01`
`$ambient034`..`036`, `TempleInt` `$ambient007`, `$ambient008`): level markers, or names that never resolve. The
editor writes the table only when an ambient was changed (`script_ambients_dirty`, `script_save_files` `0x4e8e80`).
If the table has no `#Triggers` text, `TextFile__SkipTo` leaves the cursor after the ambient header (`0x515bcf`),
and the old `$Ambient:` lines are copied again after the new ones.

**G222. Navpoint pointers that only the current script fills.** `level_items_parse_table` reads the whole level
section but sets `+0x48` (the navpoint) only for records of the current script (`_stricmp` and `mov [esi+0x48], eax`
at `0x4cb5dc`..`0x4cb5f8`). `level_items_write_table` first copies the records of the other scripts as they are
(`0x4ea6ae`), then writes every `level_items` record with a non-NULL `+0x48` (`0x4ea9cd`..`0x4ea9d7`). A record of
another script keeps the pointer its slot held in an earlier level (an address inside the shared `navpoints` table,
now a navpoint of this level); `level_items` is never cleared (no other writer of `0x24e3b98`). *Inferred:* after
playing a level with more level items in the same session, each such record appears twice, the second time on a
wrong navpoint, and that script variant then spawns the item twice. The retail sections named hold records of two or
three scripts (`sewer`, `sewer_v2`, `sewer_v3`; `LPalaceInt`, `LPalaceInt_v2`; ...).

**G223. References that the shift does not visit.** Del moves every later record of the table down one slot
(`0x4e6b63`..`0x4e6bcc` cutscene, `0x4e6c2b`..`0x4e6c96` level) and calls
`script_replace_navpoint_references(&np[i + 1], &np[i])` for each. That function updates the first live character
that starts there, the level script actions (`script_objects` only) and the location triggers
(`0x4e6f10`..`0x4e6f9c`). It does not visit `living_list_inactive` (dead characters), `cutscene_script_objects`
or `level_items[].navpoint`, and `script_editor_navpoint_in_use` checks the same three things only
(`0x4e6d93`..`0x4e6ede`). Save writes dead characters (`script_write_level_script` walks the inactive list,
`0x4e9592`), cutscene actions and level items by navpoint name, so they come out under the next navpoint's name; a
reference to the deleted navpoint itself moves onto its successor.

**G224. The cutscene record is removed from the level table.** When the deleted action was the last one,
`script_editor_show_action_params` computes the record's index from `script_objects` (`sub eax, 0x24f6230` at
`0x4e7d4c`) even when `script_editor_cur_script` is a `cutscene_script_objects` record (`0x2573f68`). That index is
379 or more, so no record moves, but `script_object_count` still drops by one (`0x4e7d9d`..`0x4e7da8`). The last
level script record falls out of the table: `script_write_objects` no longer finds it by handle and writes that
character without actions; the emptied cutscene record stays counted.

**G225. A removal without renumbering.** "No item" or count 0 removes the record with `memmove` and decrements
`level_item_count` (`0x4eceb8`..`0x4ecee4`); only the selected navpoint's `+0x2c` is set to -1 (`0x4eceee`). Every
other navpoint's `+0x2c` and every spawned item object's `+0xa0` (set by `level_items_parse_table` `0x4cb6b6` and
`level_item_spawn`) still hold the old indices. Save itself writes from the records and is right until such a
later edit. (The "<level>-%.3d" name search in the add path always ends: a clash at index i gives the name for
i + 1, and that name can only clash again at a different index.)

**G226. A clamp that always fires.** The skill loop multiplies each skill by `level / char_info level` and then
replaces it with 100 when `v > 0` (`fcomp [0x57eb84]` = 0.0, `test ah, 1; jne` at `0x4ec983`..`0x4ec990`) or when
`100 > v` (`0x4ec992`..`0x4ec99f`), so every value becomes 100; only index 2 is skipped (`0x4ec963`). The scale uses
the typed level (`fild [esp+8]` at `0x4ec893`) even when level 0 has just set the creature's level to the leader's,
so base HP (`+0x5f4`) and base AP (`+0x5fc`) become 0 (`0x4ec8ab`..`0x4ec922`). Save writes only `+Level:` (and
only for a level above 0), so the reloaded table is not affected; the creature in the running session is.

**G227. A parsed field the writer does not know.** `script_parse_objects` accepts `+Prop:` and equips the named
item (`0x4dbdbd`..`0x4dbf84`, "Unable to find prop %s for character %s"). `script_write_objects` writes `$Name`,
`$Character`, `+Monster`/`+Boss`, `+Miniboss`, `+Hidden`, `+Trigger`, `+Team`, `+Level`, `$Start position` and the
actions, but no `+Prop:` (the whole function, `0x4e9ce0`..`0x4ea1c0`).

**G228. A missing break.** In `pyrul_fire_attack` the wall-of-fire case (move spell `0x1d`, jump table `0x4b99b0`
slot 0) computes a point 7 ahead and, in multiplayer (`test byte [0x5fbfd8], 2` at `0x4b98b5`), casts spell `0x1d`
there (`0x4b98be`..`0x4b98da`) and runs on into the next case body, which casts spell `0x1c` ("meteor storm") with no
position and the boss's layer (`0x4b98dd`..`0x4b98f6`; that body is the case of spell `0x4f`, "Pyrul meteor storm"). The
single-player branch places the ground effect and returns (`0x4b9902`). Pyrul's level `KhosaniLab2` is in the
multiplayer level list (`level_table` entry `0x59ff18`, flags `0x801`, bit 0 = multiplayer, `init_level_table`).

**G229. A pick against single shares.** `ai_score_support_targets` stores each candidate's own score and returns the
sum (`0x4bea45`..`0x4bea58`). `ai_choose_support_spell` divides each score by the sum (`0x4beb0d`..`0x4beb26`) and
then takes the first candidate whose share is at least one random number (`frand`, `test ah, 0x41; jne` at
`0x4beb3d`..`0x4beb62`), instead of comparing with the running sum. With two equal candidates it casts half of the
time, with three a third, and a lone candidate always. `ai_consider_blind_or_silence` in the same unit builds the
running sum correctly. Callers: `ai_group_think_caster` (`0x4937f7`) and `ai_combat_cast_frame` (`0x4bfce2`).

**G240. Scores that are added to, not set.** `ai_consider_life_drain` (Wraith, species `0xb9`) adds the running
total to each record's `+0x08` (`fld st(0); fadd [eax+8]; fstp [eax+8]` at `0x4bf66f`..`0x4bf674`) where the other
tests first store 0 (`ai_consider_blind_or_silence` `0x4beee9`). `ai_choose_offensive_spell` runs it on the same list
after blind/silence, the two area spells, power leech and death (`0x4bf854`..`0x4bf916`), whose normalised values are
still in `+0x08`.

**G241. Distances that are never written.** For a caster that is not a party member, `ai_combat_cast_frame` builds
the offensive candidates from `cur_character`'s group and stores only the character (`mov [ebp], eax`,
`0x4bfda4`..`0x4bfdde`); the healer branch above stores a distance at `+0x04` as well (`0x4bfc6d`..`0x4bfc7d`).
`ai_consider_area_spell` counts the candidates whose `+0x04` is below the spell's range and needs two
(`0x4bf019`..`0x4bf040`); it is called for spells `0x1c` (meteor storm) and `0x1e` (inferno) (`0x4bf862`..`0x4bf87c`)
and only for non-party casters (`0x4befe6`). The values are whatever the stack frame held, or an enemy healer's
distances to its own allies.

**G242. x twice.** `pf_first_edge_crossing` keeps the crossing with the smallest "squared distance" from the
segment start, computed as `dx * dx + dx * dx` (`fsub [esp+0x14]; fst [esp+0x38]; fld st(0); fmulp; fld [esp+0x38];
fmul [esp+0x38]; faddp` at `0x4a5447`..`0x4a5462`): the z term is the x term again. Callers: `pf_trace_edge_crossings`,
`pf_straight_walk_clear`, `pf_mark_nodes_visible_from_point`, `ai_probe_move_for_walls`, `script_build_avoid_spline`.
Only crossings out of the given layer count (`0x4a5441`), so it matters where a move leaves a layer over two edges.

**G243. Flags set for all, cleared for one.** Before clipping the merged polygon, `avoid_create_obstacle_polys` sets
flag 8 (`AVOID_SEGLIST_OFF`) on the segment list of every merged polygon (`or ecx, edi` with `edi` = 8,
`0x4a9a29`..`0x4a9a6c`). When `avoid_clip_segments` fails it prints "create_obstacle_poly_from_avoid_poly failed" and
clears the flag only on the first one (`and dword ptr [8*eax + 0x2460db4], -9` at `0x4a9b16`). We know of no
retail geometry that makes the clip fail.

**G250. A chance of below zero.** `tentacle_beast_ai_select_action` picks the grab move when `frand() < 0`
(`0x4c666d`..`0x4c667f`; `0x57eb84` is 0.0). `frand` is `rand() * k` (`0x5426e0`), never negative. The pick is
also lost: the function clears the tentacle's move (`+0x7c`) right after (`0x4c66a5`) and stores only
`tentacle_move_2` or `tentacle_move_1` (`0x4c6753`, `0x4c67de`). `tentacle_beast_ai_frame` starts the grab only
when `+0x7c` equals `tentacle_move_86` (`0x4c72b6`..`0x4c72cf`), and `tentacle_beast_boss_load` loads that move
(`0x4c6fe5`). So `tentacle_beast_start_grab`, `_ai_grab`, the throw and the hold tick never run. The dead code
has two more slips: the 6-second hold tick calls `character_add_hp(+3.0)`, a heal (`push 0x40400000` at
`0x4c714b`; negative amounts are damage, `0x4508d7`), and `tentacle_beast_ai_release` uses the victim without a
NULL test (`0x4c6b53`).

**G251. Four hit slots.** `tentacle_collect_area_hits` adds every member of the leader's AI group within the
swipe's reach whose defence roll fails to `hits[num_hits++]` with no bound (`0x4c7870`..`0x4c7887`). The record has
four slots (`+0x94`), so a fifth hit writes a character handle into `+0xa4`, the death move (-1 until the death,
`0x4c6e6b`). AI groups hold up to six members (`cmp edx, 0x6` at `0x492c59`), the party at most five. At 0 HP
`tentacle_beast_check_stage` plays the death move, sets `0x88` and counts the tentacle lost only when `+0xa4` is
-1 (`0x4c73fe`..`0x4c7452`); otherwise it treats `+0xa4` as a playing move, finds it finished, and only sets the
record's "down" bit 0 (`0x4c745c`..`0x4c7487`). *Inferred:* the tentacle stays at 0 HP and is never killed. The
fight's end does not depend on it (`ls_sewerboss_frame` waits for the golems, `0x4ee165`). Latent unless all four
heroes and a summon fight in the sewer boss room and one swipe hits all five.

**G252. A timer armed once.** `murod_oni_init_timers` (called only from `murod_oni_boss_load`, `0x4c5ccf`) sets
`murod_oni_timer_b` to 5 s (`0x4c5dcf`..`0x4c5dd9`). `murod_oni_ai_select_action` tests it (`0x4c5ec7`) and, when a
target with flag 4 exists, switches to it with `murod_oni_move_2` on `frand() < 0.3` (`0x4c5efa`..`0x4c5f27`), but
never sets the timer again. The crowd timer next to it is re-armed for 20 s (`0x4c5ea5`..`0x4c5eb5`). Flag 4 means
"in reach of move 2" (`murod_oni_evaluate_target` `0x4c6205`..`0x4c621e`); move 2 is the third
`murod_oni_boss.tbl` entry, "bite backward" (`$offense02`). Because of G30 that flag is rarely set, so the effect
shows less often than the code allows.

**G253. Anchors without a life test.** `murod_start_leap` collects every party record's character with only a NULL
test (`0x4c51cb`..`0x4c51e9`), picks one at random and tries up to five points 15 units away in a random horizontal
direction (`0x57ef14` = 15, `0x4c5204`..`0x4c52dd`). Dead members stay in `party_list` with `0x80` set
([Party](party.md)). `luminar_teleport_out` collects every living entity of type 7 (or any entity with game flag 8)
with no flag test (`0x4c9dc5`..`0x4c9de6`), picks one, lands 14 units from it (`0x581ab8`) and stores it as the
AI target (`mov [esi + 0x60], ecx` at `0x4c9f9f`). After the teleport the beam test drops a dying target
(`0x4ca20a`), so the Luminar goes idle and picks again.

None.

**G270. `break` instead of `continue`.** For each living character, the second pass walks the collected areas
(C270). When the area's caster no longer resolves (`living_from_handle` of `+0xb0`), the loop ends
(`test ebx, ebx; je 0x48b5a1` at `0x48b408`) instead of going to the next area (`0x48b581`). The code after the
loop then cures status 14 (firewall), 15 (blizzard) and 17 (dark wall) unless an area of that kind was seen
(`0x48b5a1`..`0x48b5db`). So one area whose caster is gone switches off all areas after it in the effect list
for every character, and their statuses end at once. *Inferred:* a caster is "gone" when its object is removed
(a summon that leaves, a removed corpse), not merely dead; the list order is creation order.

**G271. `fx_new` keeps the slot's spell.** `fx_new` sets the kind, flags, `+0xa0`, `+0xc0`, `+0xe0`, `+0x100`
and `+0x108` (`0x48c3f4`..`0x48c461`) but not the spell `+0xa8`, and neither does `fx_obj_destroy` nor
`fx_obj__ctor` (the pool starts at 0, spell 0). `fx_get_status_holder` (kind 9, `0x48ccdc`) and
`character_equip_item_fx` (kind 16, `0x48ca76`) set no spell either. Every other creator stores one
(`fx_create_instance` -1 at `0x48c01e`, the spell creators their spell). `fx_count_spells_1a_1d_4e` counts
objects whose `+0xa8` is 0x1a, 0x1d or 0x4e (`0x48bd50`..`0x48bd65`); `ai_choose_offensive_spell` rates
fireball (0x1a) only when that count is 0 (`0x4bfa0b`..`0x4bfa2b`), and `pyrul_choose_attack` (`0x4b932c`) and
`murod_ai_select_action` (`0x4c5959`) read it too. `fx_alloc` takes the first free slot, so a holder created
right after a fireball area ended often gets its slot. *Inferred* effect, not measured. The same stale spell
makes `fx_destroy_spell_effects` (cutscene start, `0x48d733`) destroy status holders, which
`character_update_status_effects` recreates (harmless), and item effects, which only an equip recreates; no
retail item has `+Animation:` (the item effect file, `items_parse_table` `0x44a402`), so that part is latent.

**G290. A full node stack drops the subtree.** `lobj_raycast_bsp` keeps the crossed nodes on a 128-entry stack. When
it is full (`cmp edx, 0x80; jae` at `0x4af0a5`..`0x4af0ab`) the current node is neither pushed nor descended, so its
faces and both children are skipped and the ray can pass through that part of the object. The only caller is
`lobj_raycast` (`0x4ae9e2`). The retail level geometry was not measured; a tree with 10-bit node indices would need
more than 128 crossings along one segment, which we have not seen. Found in the
`NOTES-unnamed-inventory-popup-join-gtrack.md` notes (D5216).

**G300. Wrong handle for the armour's spell.** After the defender's reflex ability, `combat_trigger_reflex_ability`
walks the defender's nine equipment slots (`+0x754`..`+0x774`, `0x46794a`..`0x467c97`) and, for armour (item type 1,
`items_parse_table` `0x44a2b9`) with a `+Triggered Effect:` record (`+0xf8`), rolls `frand()` against the chance (the
table value times 0.01, `0x44ad00`). In single player a spell that is beneficial (`+0x20`, "+Beneficial") but not
"+Hit only" (`0x80`) goes to `spell_cast_execute` with the **attacker's** handle as target and the defender's position
(`[ebp]` is the attacker, `0x467b0f`..`0x467b1d`, `0x467c83`..`0x467c86`). The reflex-ability branch above it targets
the defender for the same kind of spell (`0x467815`..`0x467829`), and the multiplayer branch does too (`[eax + 0x80]` of
the defender, `0x467a4b`..`0x467a68`). An offensive spell gets the opposite slip: the defender's own handle as target
at the attacker's position (`0x467c64`..`0x467c86`). The only retail armour with a triggered effect is Rosalind's
Robe of Restoration, `+Triggered Effect: "heal" 20 20` ("may cast a Heal spell upon the wearer"); `heal` is spell 0,
"+Beneficial" without "+Hit only". `spell_cast_execute`'s case for spell 0 calls `living_entity_get_hit_by_spell` on
`living_from_handle(target)` (`0x47c9a6`..`0x47c9d4`), so the enemy gets the Heal. Found in the
`NOTES-sweep-below-0x4a0000.md` notes (their K300 candidate, the offensive case, has no retail item).

**Reproduced and fixed (2026-10-16).** In agent mode (Ikaemos exterior reached with `play_and_load_cutscene
Nar-Ikaemos`, the Robe added with `tools/savedit.mjs --add-item`, Rosalind alone in Solo mode against a Bacite
Shaman, `[Debug] TraceArmourSpells` logging the call at `0x467c86`) the Robe cast "spell 0 cast by Rosalind on
Bacite Shaman#1az (team 2)" right after the Shaman's hit. SummonerFix (`bugfix.c`) changes `mov ecx, [ebp]` at
`0x467b0f` to `mov ecx, eax` (eax holds the wearer there), and in the same test the next three casts went "on
Rosalind".

**G301. 100 for 10.** In `Lenele2ab_dlg.tbl` the topic {Give him 10 gold pieces.} under {"Massacre?"} triggers
"payoff". `ls_lenele2ab_action` then sends Canis away and subtracts 100 (`sub eax, 0x64` at `0x4f124d`), clamping the
party's gold at 0 (`0x4f1255`..`0x4f1257`). The topic has no other line. The table's own
comment after the topic ("Need to provide for the transfer of funds here.") suggests the amount was never matched up.
Found in the `NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

**G310. The leader is pushed twice.** `mp_send_local_updates` sends an OBJ_UPD for the local leader (`esi`) when its
timer has run out (`mp_send_obj_update` at `0x47fa00`). The second block tests `cur_summon`: it must be
locally controlled and its own timer at `+0x784` must have run out. It then pushes `esi`, the leader, again
(`push esi` at `0x47fa55`) and restarts the summon's timer (`0x47fa64`). The summon's OBJ_UPD is never sent
from here. The other callers of `mp_send_obj_update` are the AI frame (only for `cur_character`, `0x472db1`),
the elevators and two boss releases. *Inferred:* the summon's position and heading on the other machines are not
corrected every second, so they drift from the owner's view.

## Cosmetic

| # | What the player sees | Function | Evidence | SummonerFix |
|---|---|---|---|---|
| K1 | A hot button used with a number key lights up for one frame instead of 100 ms | `hud_frame` `0x40cf40` | **Observed** code | No |
| K2 | In the multiplayer character screen, the info panel's up arrow scrolls down | `mp_charselect_button_action` `0x4074e0` | **Observed** code | No |
| K3 | In a shop's sell list, the category buttons jump to the wrong row | `buysell_category_first_index` `0x404f70` | **Observed** code | No |
| K4 | In the inventory's identify mode, the category buttons jump to the wrong row | `inventory_category_first_index` `0x4153d0` | **Observed** code | No |
| K5 | With the save list scrolled, the top row shows the wrong thumbnail | `saveload_load_thumbnail` `0x425c10`, `saveload_draw_thumbnail` `0x426fd0` | **Observed** code, *inferred* effect | No |
| K6 | Adding a 33rd label to a level's map replaces the first label | `automap_label_alloc` `0x430230` | **Observed** code, *inferred* effect | No |
| K7 | A character missing from the font is drawn as a garbage glyph | `gr_string` `0x508c60` | **Observed** code, *inferred* effect | No |
| K8 | The credits line count can be off when a long line wraps | `credits_parse_table` `0x408620` | **Observed** code, *inferred* effect | No |
| K9 | The player count and level limits on the Create Game screen are a few pixels off centre | `gamecreate_draw_player_count` `0x40a4f0` | **Observed** code | No |
| K10 | Windowed mode stretches the 3D view to the desktop's aspect ratio | `gr_init` `0x505e00` | **Observed** code and at runtime | Fixed |
| K11 | The dialogue camera's side offset depends on an uninitialised value | `conversation_camera_compute_shot` `0x4d8f50` | **Observed** code, *inferred* effect | No |
| K12 | In a container's pickup list, the scrollbar thumb fills the whole track and runs past its end | `pickup_update_scrollbar` `0x420850` | **Observed** code | No |
| K13 | In the multiplayer level select, the player list's hover and click rows drift from the drawn names | `mp_levelselect_handle_click` `0x41c450` | **Observed** code | No |
| K14 | The PXO chat banner is never clickable | `pxo_parse_chat_banners_cfg` `0x4223d0` | **Observed** code | No |
| K15 | Latent: Print Screen saved as TGA (console `targa`) from a 32-bit back buffer gives garbled colours | `tga_convert_pixels` `0x555020` | **Observed** code, *inferred* effect | No |
| K16 | Latent: resetting a character's animations clears only the first 25 of 100 weights | `character_instance__reset_anims` `0x519850` | **Observed** code, *inferred* effect | No |
| K17 | Latent: a summon's second weapon (slot 9) pages the first weapon's model | `items_level_load` `0x44d540` | **Observed** code, *inferred* effect | No |
| K18 | Latent: paging out the popup model slot 18 can drop another slot's item model | `item_page_out_all` `0x46ca50` | **Observed** code, *inferred* effect | No |
| K19 | The launcher's Speed tab ("Default Detail Level") has no effect on the game's detail settings | `GameInit` `0x43df40`, `summoner_cfg_load` `0x43e250` | **Observed** code and launcher strings, *inferred* effect | No |
| K20 | Brass, Steel and Iron Golems show no hit effect when struck (stone golems do) | `combat_spawn_hit_vfx` `0x4669c0` | **Observed** code and retail data, *inferred* effect | No |
| K30 | Tall bosses (Tentacle Beast, Evil Urath, Luminar, Machival, Giant Salamanka, Titus, Pyrul) get their size class's height: health bar and floating text sit too low, and clicking the upper body misses | `character_size_height` `0x4542f0` | **Observed** code, *inferred* effect | No |
| K40 | With 128 map labels saved, deleting or editing a label is not saved, and the old label comes back on the next visit or load | `savegame_automap_label_remove` `0x4d1390` | **Observed** code, *inferred* effect | No |
| K50 | With the OpenGL renderer, the smallest mipmaps of the 16-bit water textures are built from the top half of the larger level only | `gl_build_mipmaps` `0x54e0e0` | **Observed** code and retail data, *inferred* effect | No |
| K51 | Latent: with the OpenGL renderer, a tint on a 2D bitmap, line or polygon wraps bright colours round to dark | `gr_opengl_apply_tint` `0x54f9a0` | **Observed** code, *inferred* effect | No |
| K52 | Latent: `help change_texture_res` (or `? change_texture_res`) switches the low-resolution textures on or off | `cmd_change_texture_res` `0x501c90` | **Observed** code | No |
| K60 | With the Direct3D renderer, the depth clear misses the last column and the last row of the clip area, so those pixels keep the previous frame's depth | `gr_d3d_zbuffer_clear` `0x5355e0` | **Observed** code, *inferred* effect | No |
| K61 | A dropped item's 5-second highlight gets no blue at all: it is yellow instead of the pale gold in the data | `world_tint_gold_object` `0x43a600` | **Observed** code, *inferred* effect | No |
| K62 | In the multiplayer chat, a kick message shows the kicked player's name where the reason should be | `chat_process_irc_message` `0x4ac240` | **Observed** code | No |
| K63 | Clicking a character tests the horizontal distance squared against the radius: small and normal-sized characters can be clicked further from their middle, characters with a radius above 1 only nearer to it | `pick_test_character` `0x43c620` | **Observed** code, *inferred* effect | No |
| K64 | With the OpenGL renderer, a start without `summoner.cfg` (a fresh install) switches on the low-resolution textures | `summoner_cfg_load` `0x43e250`, `gr_get_video_memory_mb` `0x5079f0` | **Observed** code, *inferred* effect | Fixed (`bugfix.c`) |
| K80 | A knock-down that hits the selected character while it casts shows "Knocked Down!", but the character stays on its feet | `ai_knock_down_local` `0x4742b0`, `ai_set_mode` `0x473320` | **Observed** code, *inferred* effect | No |
| K90 | With the OpenGL renderer, the gamma (brightness) slider does not change the brightness | `gr_set_gamma` `0x506a90`, `gr_opengl_tmapper` `0x550080` | **Observed** code, *inferred* effect | Fixed (`gamma.c`) |
| K91 | With the Direct3D renderer and a hardware gamma ramp, lit polygons get the gamma curve twice | `gr_d3d_tmapper` `0x5361a0`, `gr_d3d_tmapper_multipass` `0x536360`, `gr_d3d_set_gamma` `0x53ab20` | **Observed** code, *inferred* effect | No |
| K92 | Latent: a fog update with a negative near distance or a far distance below the near one keeps the old fog distances | `gr_set_fog` `0x507db0` | **Observed** code, *inferred* effect | No |
| K93 | Latent: when anti-aliasing is on in the options, the first `antialiasing` console command reports "antialiasing toggled!" but changes nothing | `cmd_antialiasing` `0x505db0` | **Observed** code | No |
| K100 | A ring whose power lands on exactly 60,000 never shows "Joseph acquired Summon ...!", and its pop-up lists the summon one gain before it can be cast | `ring_check_summon_unlock` `0x450220`, `ring_format_info` `0x44b7a0` | **Observed** code, *inferred* effect | No |
| K110 | After enough level changes, levels lose their ambient sounds (and ground effects their sound), until the game is restarted: every level leave frees only half of the ambient sound emitters | `level_script_leave` `0x4ccc70`, `level_ambient_remove` `0x4cd2c0` | **Observed** code and retail data, *inferred* effect | No |
| K130 | In Liangshan the "Boss" music keeps playing after the Serpent and Phoenix Riders are dead, until the level is left or a game is loaded | `ls_liangshan_init` `0x4f4930`, `ls_liangshan_frame` `0x4f4a20` | **Observed** code and retail data, *inferred* effect | No |
| K131 | Every load of a game saved in the last temple level replays the 29 s "Nar-TempleAct4" narration | `ls_templeint2_init` `0x4ef190`, `ls_templeint2_frame` `0x4ef1e0` | **Observed** code and retail data, *inferred* effect | No |
| K140 | Latent: a cutscene "replace armor" with "(none)" or an unknown item name, on a character wearing armour, decides whether to act from the memory just before the item table, so the armour may stay on | `script_action_replace_armor` `0x4dce60` | **Observed** code and retail data, *inferred* effect | No |
| K150 | In the Ikaemos palace's upper hall the Priest King fight keeps the level's "IkaemosPalace" music; the "Boss" track never plays | `ls_ikaemostopint_init` `0x4f9ec0`, `lsf_ikaemos_found_boss` `0x4fa050`, `lsf_ikaemos_boss_killed` `0x4fa140` | **Observed** code and retail data, *inferred* effect | No |
| K151 | Latent: with the Glide renderer (a 3dfx card or a Glide wrapper), an 8-bit alpha bitmap wider or taller than 256 texels would repeat its top-left corner in every 256 x 256 section; no bitmap loader in v1.40 produces that format | `gr_glide_create_texture_section` `0x5484f0` | **Observed** code, *inferred* effect | No |
| K160 | In multiplayer, a player who joins while another join is still going on can see "%s has entered the game." twice for the other new player | `mp_add_new_player` `0x4af4e0`, `mp_handle_new_info` `0x4af4b0`, `mp_handle_net_info` `0x4af760` | **Observed** code, *inferred* trigger | No |
| K161 | The "Insert Summoner CD #2" box is centred across the screen but starts at the middle, so it sits in the lower half | `cd_prompt_disc2` `0x43ea20` | **Observed** code | No |
| K170 | In the "You have died." popup (and the other titled choice popups), the highlight and the click area of each entry sit 5 pixels above its text: the bottom of "Restart" selects "Quit" | `ui_choice_popup_hit_test` `0x413970` | **Observed** code, *inferred* effect | No |
| K171 | A lock that was already picked can be picked again: the skill rolls again, plays the unlock or the failure sound, and "This door already unlocked." is never shown for it | `character_use_skill` `0x429410`, `door_pick_by_mesh` `0x4aaa10` | **Observed** code, *inferred* trigger | No |
| K180 | Latent: when a `strings.tbl` language section has more than 40 KB of text, each string that does not fit shows the text of the next string that does (or nothing) | `localize_set_string` `0x504400` | **Observed** code and retail data, *inferred* effect | No |
| K181 | Latent: French text would show `Ä` as `É`; v1.40 never selects French | `localize_remap_charset` `0x5051a0` | **Observed** code and retail data | No |
| K190 | A dragon summon's cutscene, cast after any story cutscene, streams a stale soundtrack handle: usually silence as intended, but when a music track has since taken that sound slot (for example boss music started right after a boss intro), that track restarts under the dragon scene and its slot is freed when the scene ends | `cutscene_unload` `0x4351e0`, `cutscene_load_sounds` `0x435000`, `cutscene_sequence_update` `0x436a90` | **Observed** code and retail data, *inferred* effect | No |
| K191 | Releasing a long left-button drag with the cursor at the left or right screen edge starts the edge camera turn at once; the code meant to hold it back until the cursor leaves the edge never engages | `camera_process_input` `0x434660` | **Observed** code, *inferred* effect | No |
| K192 | Latent: a `+Minitrack:` sound in `cutscene.tbl` never plays | `cutscene_load_sounds` `0x435000`, `cutscene_events_frame` `0x436c00` | **Observed** code and retail data | No |
| K193 | Latent: after `screen999`, every Print Screen writes `screen1000` again over the last one; a `screen*.jpg` (or `.tga`) whose name sorts after the numbered ones, such as `screenshot.jpg`, makes the number random | `screenshot_save` `0x4324f0` | **Observed** code, *inferred* effect | No |
| K194 | Latent: the help of the `play_and_load_cutscene` console command prints `play_cutscene`'s usage line. No console in v1.40 | `cmd_play_and_load_cutscene` `0x437590` | **Observed** code | No |
| K200 | Latent: in the PXO lobby, a channel list reply with a cut-short record leaves the channel list unchanged on screen until the next refresh 10 s later | `lobby_parse_channel_list` `0x43f450` | **Observed** code | No |
| K210 | In the snowy levels (`lenele3a`, `lenele3d`, `ionaext02`), when the camera moves one 5 m cell toward +x the new row of cells at the +x edge keeps the flakes of the row that left on the far side, so that edge has thin or misplaced snow for up to 12 s | `snow_update` `0x494ba0` | **Observed** code, *inferred* effect | No |
| K211 | The mounted Phoenix Rider's animation is not kept in step with its horse (the Serpent and Tiger Riders' are) | `phoenix_seat_rider` `0x4982a0` | **Observed** code, *inferred* effect | No |
| K212 | In the Tiger fight's mounted axe stage (stage 2), the rider plays its stage-2 walk animation even when it should stand; the stage-2 ready animation `tigr_stage2_ready.mvf` is loaded but never used | `tiger_set_stage2_anims` `0x49a140` | **Observed** code and retail data, *inferred* effect | No |
| K213 | Latent: the Serpent's illusion, if it ever ran, would leave the second copy visible and active while the first one vanishes; in v1.40 nothing starts the illusion | `serpent_illusion_start` `0x496ba0`, `serpent_ai_frame` `0x4973e0` | **Observed** code, *inferred* effect | No |
| K230 | Latent: in multiplayer, monsters that a `load_mlayout` monster layout creates on the other machines get a broken orientation (their up row is a copy of their right row), until the first position update turns them by heading. No console in v1.40: only a patch or a `.vcs` script can run it | `matrix3_pack_orientation` `0x475380` | **Observed** code, *inferred* effect | No |
| K231 | Latent: an overshooting step of the camera's height smoothing moves the camera away from the floor height instead of onto it; it cannot happen because the frame time is capped at 0.25 s | `camera_smooth_height` `0x478700` | **Observed** code and constants | No |
| K220 | The script editor's door window shows the door sounds' range, volume and roll-off as 0.0 (or garbage), and OK writes those values back, so editing any door with sounds silences them, also in the saved `_script.tbl` | `script_editor_list_door_sounds` `0x4e53a0`, `script_editor_door_ok` `0x4ec270` | **Observed** code, *inferred* values | No |
| K221 | In the script editor, while a level object is selected, the white hover marker of a character or navpoint is drawn as a box from the wrong fields | `script_editor_frame` `0x4e01f0` | **Observed** code, *inferred* effect | No |
| K222 | In the script editor's door window, Enter in the sound name box takes the focus from the key item box instead of leaving the sound box | `script_editor_door_sound_inputbox_cb` `0x4ebb20` | **Observed** code | No |
| K223 | Latent: a Pyrul model without a `$missile01` tag would fire his missiles from bone -1 (memory before the bone array) instead of from his own frame | `pyrul_get_missile_origin` `0x4b95f0` | **Observed** code and retail data, *inferred* effect | No |
| K250 | Latent: a blob shadow that falls on eight level objects is drawn on none of them, and on nine to fifteen only on the last few | `projector_collect_lobj_faces` `0x4c0250`, `shadows_render` `0x4a4360` | **Observed** code, *inferred* effect | No |
| K251 | When Evil Urath leaps across more than one obstacle edge, his rise is fitted to the last edge found, not the nearest, so the arc can be too flat or too steep | `evil_urath_leap_check_path` `0x4c4200` | **Observed** code, *inferred* effect; retail path data not checked | No |
| K252 | Latent: a multiplayer character file whose name has 32 or more characters shows the name with junk after it in the character list | `File__read_string` `0x525ee0`, `mp_player_file_read_summary` `0x46d450` | **Observed** code, *inferred* effect | No |
| K260 | With the `DirectSound` sound setting (the default, no DirectSound3D), sounds placed in the world (footsteps, weapon hits, spells, moves, doors, loot, voices) play at the sound-effects volume squared: at half volume they play at a quarter, while menu clicks follow the slider; level ambient loops start that way until their next volume update | `snd_play_3d` `0x521480`, `snd_calc_volume_3d` `0x520fa0`, `snd_play` `0x520d50` | **Observed** code, *inferred* effect | Fixed (`bugfix.c`) |
| K261 | Latent: a saved EAX settings file (`<level>.eax` in `data\internal`) sets the listener flags to the bits of its air-absorption value; all 49 retail `.eax` files carry -5.0 there, but the game never reads them from `tables.vpp` | `eax_get_listener_props` `0x522240`, `eax_save_props` `0x5220e0`, `eax_load_props` `0x522420` | **Observed** code and retail data, *inferred* effect | No |
| K262 | Latent: a spell ground effect whose ground node is turned more than 90 degrees about a tilted axis gets a wrong (or NaN) orientation when it is placed on the floor | `quaternion__set_from_matrix_b` `0x522790`, `vfx_instance__set_node_transform` `0x537a10` | **Observed** code, *inferred* effect; retail `.vfx` node orientations not checked | No |
| K263 | Latent: a spot light with a projected texture lights points outside its range, shows an undefined colour for one texture format, and leaves the texture locked | `light_sample_projected_bitmap` `0x51f0d0`, `lights_shade_point` `0x51ec70`, `light_delete` `0x51e540` | **Observed** code and retail data, *inferred* effect | No |
| K264 | In the script editor's lists, a click in the 2 pixels above an entry selects that entry instead of the one under the mouse, and a click on the last 2 pixels of the last entry selects nothing | `ui_listbox__process` `0x52d080` | **Observed** code, *inferred* effect | No |
| K265 | Latent: Page Down in a list box whose selected last entry is taller than the box selects the list's own head record (no text, data -1), which the list's callback then receives | `ui_listbox__select_page_down` `0x52e050` | **Observed** code, *inferred* effect | No |
| K270 | In the Ghost-Escape and Ghost-Dismount cutscenes (Rand-ForestNite1) the Ghost Rider's attached props are drawn, although the code means to hide them there | `levelrender_add_character` `0x483260` | **Observed** code and retail data, *inferred* effect; not seen in play | No |
| K271 | A lamp glow on an effect (a torch or brazier effect) can shine through one level object that should hide it | `lamp_glow_update_effects` `0x484600` | **Observed** code, *inferred* effect | No |
| K272 | Every character gets the same blob shadow: the per-character shadow sizes in `shadows.tbl` (0.47 to 25.55) are loaded and never used | `shadows_parse_table` `0x4874c0`, `shadow_draw_character` `0x487630` | **Observed** code and retail data | No |
| K273 | Latent: a level table's `+Fog Near:` and `+Fog Far:` are stored swapped as the level's default fog, so a cutscene `$Fog` key with a near distance of 0 starts the fog at the far value | `level_parse_table` `0x4cb7b0`, `fog_set_level_default` `0x486b90` | **Observed** code and retail data, *inferred* effect | No |
| K274 | Latent: floating text drawn inside a cutscene's letterbox would sit lower by the letterbox height; health bars, speech text and overhead icons have the same slip but are not drawn in cutscenes | `levelrender_draw_float_text` `0x485380`, `levelrender_draw_health_bars` `0x4848c0`, `levelrender_draw_speech_text` `0x484ae0`, `character_draw_overhead_bitmap` `0x481c40` | **Observed** code, *inferred* effect | No |
| K280 | With SummonerFix's widescreen output (a game width above 1024), the black strip right of the centred main menu showed old frames of the last level in turn with black after a cutscene had run (seen after the console cutscene viewer and after skipping story cutscenes) | SummonerFix `display.c` (`Hook_SwapBuffers`), not the game | **Observed** by the user and in agent-mode captures (2026-10-16) | Fixed (`display.c`) |
| K290 | Animated characters are lit as if their normals leaned towards each bone's position: the skinning moves the normals by the bone translation too | `gr_glide_render_character_mesh` `0x50b4c0`, `gr_d3d_render_character_mesh` `0x50c130`, `gr_opengl_render_character_mesh` `0x50cd00` | **Observed** code, *inferred* effect | No |
| K291 | With Direct3D in a fogged level, character vertices on texture seams are not fogged while the vertices around them fade | `gr_d3d_render_character_mesh` `0x50c130` | **Observed** code, *inferred* effect | No |
| K292 | In a conversation, the "more" and "end" entries light up for the wrong mouse positions | `dialogue_topic_entry__draw` `0x4d6e70` | **Observed** code, *inferred* effect | No |
| K293 | In a conversation of more than 64 text lines and spacers, the later lines play no talk animation | `dialogue_box__add_text` `0x4d7190` | **Observed** code, *inferred* effect | No |
| K294 | A sound that is fading out and ends during the fade leaves its channel marked as fading; the next sound put on that channel is cut off within 75 ms, and the old sound buffer is not released | `ds_channel_update_fade` `0x55d6f0`, `ds_channel_release` `0x55c5b0`, `ds_find_free_channel` `0x55d0a0` | **Observed** code, *inferred* effect | No |
| K295 | Effect particles pushed by forces (gravity, wind) move by at most 1/30 of the step per update, so they move slower when updates are long | `vfx_emitter_instance__move_particles` `0x560a00` | **Observed** code, *inferred* effect | No |
| K296 | Latent: in an effect bound to a character, light tags from the third on bind to (and place, hide or show) the wrong light, and dummies bound to a bone never take the bone's rotation | `vfx_instance__list1c_item` `0x5385d0`, `vfx_dummy_state__update` `0x55fc90` | **Observed** code, *inferred* effect | No |
| K297 | Latent: in the script editor, check boxes and radio buttons are clickable only over the first few characters of their label, and input boxes place the scroll and cursor with the wrong font | `ui_checkbox__create` `0x553d00`, `ui_radio_button__create` `0x5548a0`, `ui_inputbox__draw` `0x5530b0` | **Observed** code, *inferred* effect | No |
| K298 | Latent: velocity-aligned (streak) particles have a length that ignores one of their size factors | `vfx_particle_get_billboard_axes` `0x562840` | **Observed** code, *inferred* effect | No |
| K299 | Latent: lit effect meshes get vertex normals that all point into the negative octant, because face normals are unpacked as signed bytes with the unsigned mapping | `vfx_unpack_vector_s8` `0x564a20`, `vfx_mesh_render` `0x562f60` | **Observed** code, *inferred* effect | No |
| K300 | The hit sound of many spells (Black Cloud, Recall, Destroy Undead, Wrath of God, Burning Hands, Curse, the Meteor Storms, Banish Summon and others) plays at a leftover stack position, so it can come from the wrong place or be out of earshot | `spell_cast_execute` `0x47c5d0` | **Observed** code, *inferred* effect | No |
| K301 | Latent: a `+Set Important:` or `+Clear Important:` line naming a speaker defined later in the same `_dlg.tbl` does nothing, so that character's quest icon is not shown or not removed | `dialogue_parse_table` `0x4fee50` | **Observed** code and retail data | No |
| K302 | Latent: an effect whose particle interval curve has two or more non-zero keys spawns at the wrong rate between the keys | `vif_curve__evaluate` `0x533ca0` | **Observed** code, *inferred* effect | No |
| K303 | Latent: the `sysinfo` console command always reports MMX, and shows the RAM speed four times too high | `cpu_identify` `0x5312d6`, `PrintSystemInfo` `0x53171f` | **Observed** code | No |
| K310 | With the OpenGL renderer, coloured light from spells and effects on lightmapped walls and floors shows only its red value, as grey: fire and torch light turns white, blue light (Icicle, the Catacombs effects) all but disappears | `levelrender_draw_list_opengl` `0x491500` | **Observed** code and retail data, *inferred* effect | No |

**K1. Wrong timer test.** `hud_frame` clears `hud_hot_button_used` when the timer is valid *or* elapsed
(`jne 0x40cf74` after `GameTimer__valid` at `0x40cf5d`). A running timer is valid, so the highlight that
`hud_handle_keys` sets for 100 ms (`0x40dd1c`) is gone on the next frame. It was probably meant to be
`!valid() || elapsed()`.

**K2. Two buttons, one case.** The case table at `0x407630` maps button 31 (up) and button 33 (down) to the
same body, `mp_charselect_scroll_down(2)` (`0x4075d0`). There is no `scroll_up(2)` case. See
[Multiplayer](multiplayer.md).

**K3, K4. Index into the wrong list.** In sell mode the shop list shows only owned items, in
party-inventory order (`buysell_get_list_item` `0x40497d`). The jump returns an index into the shop's buy list
(`0x404f98`..`0x404fcc`). In identify mode the inventory shows only unidentified items, but
`inventory_category_first_index` returns a party-inventory index (`mov eax, esi` at `0x41543e`).

**K5. Row index again.** In save mode `saveload_load_thumbnail` loads no file for row 0 (`test ebx, ebx;
jle` at `0x425c65`). `saveload_draw_thumbnail` then shows the current scene for row 0 (`0x426fe5`). Both
use the row, not scroll + row, so a scrolled top row shows the scene instead of its save.

**K6. Full returns 0.** With 32 labels, `automap_label_alloc` returns 0 (`xor eax, eax` at `0x430245`)
instead of -1. `automap_add_label_at_mouse` tests for a negative result (`jl` at `0x403180`), so it
writes over label 0. The other three callers make the same test: `automap_labels_parse_table` (`0x42fe19`,
every label after the 32nd in `automap_labels.tbl` goes into label 0; the retail table has none),
`automap_labels_reset_default` (`0x430342`, console command `reset_map_labels` only) and
`savegame_read_automap_labels` (`0x4d3806`).

**K7. No check for -1.** `gr_font_get_char` returns -1 for a character outside the font (`or eax, -1` at
`0x508503`). `gr_string` uses the result as an index without a test (`mov edx, [edx+4*eax]` at `0x508d21`).

**K8. Wrong buffer.** For wrapped lines the `&` (right column) test reads `credits_lines[start]`, the
output buffer at the input offset (`cmp byte [edx+esi], 0x26` at `0x408862`). It was probably meant to
read `line[start]`. Only the line count changes.

**K9. Height used twice.** `gr_get_string_size` returns the width and the height, but both centring
offsets read the height (`mov eax, [esp+0x30]` at `0x40a541` and `mov eax, [esp+0x34]` at `0x40a553`
are the same local after one push). The width is never read. `gamecreate_draw_limits` (`0x40a570`) does
the same.

**K10. Desktop aspect.** In windowed mode `gr_init` takes the aspect ratio from
`GetSystemMetrics(SM_CXSCREEN) / GetSystemMetrics(SM_CYSCREEN)` (`0x505ef5`..`0x505f0f`), not from the
window. Only windowed mode is affected, and the retail game runs fullscreen. See
[SummonerFix](../summonerfix/index.md#fixes).

**K11. Missing component.** The shot code builds a side vector from the speaker-to-player direction. It writes only
its x and z (`[esp+0x54]` and `[esp+0x5c]`, `0x4d8fcb`..`0x4d8fd7`), never y (`[esp+0x58]`), and then normalises it
(`vector3__normalize_safe` at `0x4d8fe4`). Whatever the stack held decides how much of the camera offset goes up
instead of sideways. *Inferred:* the value is often the same from call to call, so this may not show.

**K12. Full-height thumb.** Both branches set the thumb height to the full track height
(`mov [0x5c3b34], edx` at `0x4208f8` and `mov [0x5c3b34], edi` at `0x420943`). The scrolled branch still moves
the thumb down by `scroll_pos / num_items x track` (`0x420901`..`0x420941`). `pickup_draw_scrollbar` (`0x4212c0`)
tiles the thumb bitmap over that height, so once the list is scrolled the thumb runs past the bottom of the track.
The hit area grows with it. The assess screen scales the height with `page / count` (`0x4017a6`..`0x4017d7`).
This shows for any container with more items than one page.

**K13. Row counter steps for every player.** The click loop adds the font height for every entry of
`mp_player_list` (`add ebp, esi` at `0x41c7e0`), but tests only type-3 players (`cmp [edi+0x14], 3` at
`0x41c79d`). `mp_levelselect_draw_players` moves down only after drawing a type-3 player (`0x41d522`..). A
player in another state, for example one who is still joining (state 2, `mp_accept_join_request` `0x4ab6a8`),
shifts every later name by one row. The hover highlight then lands on another line, and a click can select a
different player. Kick uses the selected *name*, which is drawn highlighted, so the host can see the mistake
before kicking. Only the first four rows are hit-tested (the box is `font_h * 4` high).

**K14. Link parsed into a scratch buffer.** Each `+Link:` value is parsed into one stack buffer
(`lea eax, [esp+0x294]` at `0x4225e3`) and never copied into `links[count]`. `pxo_chat_banner_link_url` is
therefore always empty, and `pxochat_banner_has_url` (`0x422790`) is always false. This was live while PXO
served banners. The service is gone, so it no longer shows.

**K15. Copy size and advance disagree.** For a 4-byte source pixel, `tga_convert_pixels` falls through to the
plain copy. It copies `count x src_bpp` bytes (`imul ecx, esi` at `0x5550ab`) but returns `count x dst_bpp`
(`imul eax, edx` at `0x5550ae`). `gr_screenshot` always asks for 3-byte output (`push 3` at `0x5063fa`) and uses
`src_bpp` 4 when `gr_read_backbuffer` reports format 7 (`0x506283`). In raw packets of more than one pixel, each
BGRA quad is then read back as a BGR triple, so the channels shift. 16-bit (format 3) and 24-bit back buffers
convert correctly. By default screenshots are JPEG (`Print_screen_as_targa` `0x5fc022` tested at `0x43251e`), so
this needs the console command `targa` and a renderer that reports 32 bits.

**K16. Byte count passed as an element count.** `reset_anims(true)` clears `weights` with `mov ecx, 0x19; rep
stosd` (`0x51986b`), which is 25 floats of the 100 that `alloc_buffers` allocates. `character_reinit_anims`
(`0x447f20`) clears every weight anyway through `clear_state_anims` and `clear_action_anims`. `level_load_preload`
(`0x447ce9`) does not: it resets every living character of a char_info after `charinfo_remap_town_moves`, so a
nonzero weight at index 25 or above keeps blending into the new animation set. *Inferred:* it needs an animation
with index 25 or above playing at that moment, and it would only last until that animation ends.

**K17. Index 5 for slot 9.** For characters with flag `0x100` (summons) the loop tests `equipment[9]`
(`[esi+0x778]` at `0x44d72d`) but pages `equipment[5]`'s type (`mov ecx, [esi+0x768]` at `0x44d737`) into paging
slot 9. The second weapon's model is never paged, and if slot 5 is empty while slot 9 is not, `[ecx+4]` reads
through NULL. Whether any retail summon carries a slot-9 item is not checked.

**K18. The popup branch evicts the wrong slot.** When slot 18 owns a mesh that another slot (0..16) also shows,
`item_page_out_all(18)` clears the *other* slot and sets the type's mesh to NULL (`0x46caab`..`0x46caca`). It then
returns without clearing slot 18 (`ret` at `0x46cad2`). The general branch swaps the pools instead
(`0x46cae4`..`0x46caf9`). Slot 18 keeps its type with a NULL mesh, so a later `item_page_in(type, 18)` returns at
once (`s->type == type`) with no mesh, and the other slot's item loses its model until it is paged again. Callers
of slot 18 are the shop model view, the inventory drag and the item info popup (`buysell_handle_click` `0x405507`,
`inventory_drag_to_equip` `0x417442`, `item_info_popup_close` `0x418c22`). We found no retail sequence where slot
18 owns a mesh that another slot shares.

**K19. Presets reset straight away.** `GameInit` reads `ComputerSpeed` (`os_registry_read_dword` at `0x43e080`),
clamps it to 0..2 (`0x43e090`..`0x43e09e`) and presets fog and three detail options (`0x43e0ac`..`0x43e0fe`). It then
calls `summoner_cfg_load` (`0x43e104`), which first stores every default (`0x43e295`..`0x43e320`, among them
`config_detail_14` `0x43e2d4`, `config_fog_quality` `0x43e2f9`) and then reads `summoner.cfg`. The presets are lost
whether or not the file exists. The launcher (`Summoner.exe`) writes `ComputerSpeed` from its Speed tab (strings
"SpeedTab", "ComputerSpeed", "Default Detail Level:"), and `launcher_help.txt` describes that tab as setting "the
default Detail Level". The presets themselves are also odd: value 1 gives `config_detail_14` 2 and value 2 gives 1.
Found in the [`summoner.cpp`](../../src/decomp/game/summoner.cpp) notes.

**K20. The name switch and the spawn tests disagree.** `combat_spawn_hit_vfx` picks a name from the target's skin
(char_info `+0x218`, case bytes at `0x466a64`): flesh and leather (1, 2) get `blood.vfx`, stone, ice and the metals
(3, 9..16) get `sparkhit.vfx`, the rest none. It then spawns only for skin 1 (`blood.vfx`, `cmp edi, 1` at
`0x466a0c`) and skins 3 and 9 (`sparkhit01.vfx`, `0x466a2f`..`0x466a37`). Leather and the metals (copper, gold,
bronze, iron, silver, admantium, volitionite; `material_names` `0x598f5c`) get nothing, and `sparkhit.vfx` is never
used. Retail tables: Brass Golem, Broken Brass Golem and Steel Golem are "bronze", Iron Golem is "iron"; no
character is "leather". Both effect names exist in `effects.vpp`. All 13 callers are in `combat_apply_melee_hit`.
Found in the `NOTES-spells-combat.md` notes.

**K30. Boss height dropped.** For type 9, `character_size_height` calls `charinfo_boss_height` and discards
the result (`fstp st(0)` at `0x45430f`), then returns the size-class height (`fld [4*edx + 0x5818d8]` at
`0x454323`; 0.9, 1.8, 2.5). The radius function next to it returns the boss value (`jmp 0x4542ea` at
`0x4542d2`). The lost heights are Tentacle Beast 15, Evil Urath 8, Luminar 5.5, Machival 5, Giant Salamanka 4,
Titus and Pyrul 3 (the riders, Murod, Murod Oni and Evil Joseph return the size table anyway). Callers:
`levelrender_draw_health_bars`, `_draw_speech_text`, `_draw_float_text`, `pick_test_character`,
`character_has_line_of_sight_to` / `_to_point`, `character_find_overlap`, `script_guard_sees_player`, the
conversation camera and the script editor. *Inferred:* the visible part is the bar and text placement and
picking; the line-of-sight and overlap use may also change what a boss can see or touch, which was not traced.

**K40. Remove returns at the limit.** `savegame_automap_label_remove` returns at once when the list holds 128 labels
(`cmp eax, 0x80; jge` at `0x4d13ca`), and so does `savegame_automap_labels_reset_level` (`0x4d1526`). The test was
probably meant for the add. Deleting a label (`automap_handle_click`, `0x402bf3`) removes it from the live table
(`automap_label_delete`) but not from the saved list. Editing one calls remove and then add (`0x40345e` /
`0x403470`, `0x4034e6` / `0x4034f9`), and the add also refuses at 128 (`0x4d12d6`), so the new text is dropped.
`savegame_read_automap_labels` rebuilds the level's labels from the saved list after a load and on every level entry
(`state_game_enter`, `0x438233`), so the deleted label or the old text comes back. The list counts the labels of all
levels together, and the editor refuses new labels at 128 (`0x40316c`), so the player can reach the limit.

**K50. The 565 filter never skips a row.** Each generated 565 texel averages `src[0]`, `src[1]`, `src[w]` and `src[w + 1]`
(`0x54e4f9`..`0x54e50d`) and the source pointer then moves on by two pixels (`add eax, 4` at `0x54e5a8`). At the end of an
output row it is not moved past the second source row (`0x54e5c2`..`0x54e5cb`), so output row k reads source rows k and
k + 1. The 888 and 8888 cases add two rows (`0x54e49f`..`0x54e4af`, `lea eax, [8*edi]` at `0x54e348`). The filter runs for
the levels below the stored ones when a bitmap has more than one level (`cmp ecx, 1; jle` at `0x54e9e3`, call at
`0x54ea00`). TGAs load as formats 5, 6 and 7 only (`bm_read_header` `0x502b4a`..`0x502b99`); a VBM with format code 2 or
more loads as 565 (`0x501df8`..`0x501e1b`). Retail: 7 of the 104 VBMs are 565 with stored mipmaps (`grslnd-h2o-nite.vbm`,
`ks-h2o.vbm`, `S-WatClean.vbm`, `water-test.vbm`, `water-testsmall.vbm`, `whirl.vbm` and `wlng-h2o.vbm`, all in `levelt.vpp`, 64 or 128 pixels with 1 to 3 stored
levels), so only their 16x16 and smaller levels are wrong. Those levels are drawn only at a distance (trilinear filter
`0x2703`, `0x54ebed`..`0x54ec0b`), so this is hard to see. Direct3D and Glide do not use this code.

**K51. Truncated before the clamp.** `gr_opengl_apply_tint` computes `(r + g + b) x tint_r` and stores it as a byte
(`mov byte [esi], al` at `0x54f9dd`) before `clamp_ubyte(.., 0, 255)` (`0x54f9df`), which can then change nothing. Same for
green and blue (`0x54f9fb`, `0x54fa19`). `gr_d3d_apply_tint` clamps the int (`clamp_int`, `0x5354ba`..`0x5354f8`) and
`gr_glide_apply_tint` the float (`0x54a56d`..`0x54a5a1`). So a white vertex (sum 765) with a tint of 0.5 gives 126 instead of
255. The function runs from `gr_opengl_bitmap`, `gr_opengl_line`, `gr_opengl_line_3d` and `gr_opengl_tmapper` while
`gr_tint_enabled` is set (`0x5501a4`..`0x5501af`). The hover and effect tints of `levelrender_draw_list` reach meshes
(the character mesh renderers, which clamp the brightness first, OpenGL in `gr_opengl_render_character_mesh` at `0x50d3a1`..`0x50d3c3`) and level geometry (`levelrender_face_vertex_colors`), and
`gr_poly_buffer_flush` clears the flag while it replays (`0x545d9e`, `0x546028`). We found no draw that reaches this
function with a tint set.

**K52. No `calling_a_command_function` test.** `execute_command_line` also calls a command's function for a status query
("?", `0x50a2ed`..`0x50a319`, flags at `0x50a4af`..`0x50a4bb`) and for help ("help", "man" or a "?" in the line,
`0x50a324`..`0x50a389`, flags at `0x50a4ed`..`0x50a4fd`; call at `0x50a50a`), with `calling_a_command_function` 0.
`cmd_load_mipmaps` tests it (`0x501b50`..`0x501b57`). `cmd_change_texture_res` does not: it calls
`bm_set_reduce_textures(!bm_reduce_textures)` and prints "Low resolution textures are now %s!" every time
(`0x501c90`..`0x501cd3`), and it has no help text. A developer console command, so no retail path reaches it.

**K60. No + 1 on the depth clear.** The clip rectangle keeps its right and bottom edges inclusive: `gr__set_clip`
stores `w - 1` and `h - 1` (`lea edx, [ecx - 1]` at `0x50684f`, `dec ecx` at `0x50685b`, stored at `0x50686f` and
`0x50687b`). `gr_d3d_clear` turns them into an exclusive `D3DRECT` with `+ 1` (`lea eax, [edx + eax + 1]` at
`0x5355a3`, `lea eax, [edx + ecx + 1]` at `0x5355b3`), and so does `gr_opengl_clear` (`0x54f828`, `0x54f839`).
`gr_d3d_zbuffer_clear` builds the same rectangle without it (`add edx, eax` at `0x53562f`, `add eax, ecx` at
`0x535640`) and clears only the z buffer (`push 2`, `D3DCLEAR_ZBUFFER`, at `0x535654`). So column `x2` and row `y2`
of the clip area are never cleared. `gr_zbuffer_clear` runs once per scene, from `levelrender_render_scene`
(`0x482545`) and `gr_setup_3d` (`0x50df38`). `gr_opengl_zbuffer_clear` clears the whole buffer with `glClear` and has
no rectangle (`0x54f8af`). *Inferred:* with the reversed depth test (`D3DCMP_GREATEREQUAL` against a clear value of
0.0 on every card except S3, `0x539f04`..`0x539f0e`), a new pixel in that column or row is drawn only when it is at
least as near as last frame's, so the right and bottom edge of the 3D view can show a one-pixel line of the clear
colour while the camera moves. Not tested. Direct3D only. Found in the `NOTES-gr-direct3d.md` notes.

**K61. Index 3 of a three-float colour.** `world_tint_gold_object` passes red `[0x594424]` (0.98), green
`[0x594428]` (1.0) and blue `[0x594430]` (`0x43a62e`..`0x43a63b`). The colour's third float, 0.51, is at `0x59442c`,
and nothing reads that address. `0x594430` is the string "gold" (`646c6f67`, about 1.7e22 as a float; `obj_is_gold`
pushes it as a string at `0x46a7b2`). `world_hover_tint_object` scales each channel by `(pulse + 1) x 0.5`
(`0x43a681`..`0x43a6cc`), and `levelrender_draw_list` hands the result to `gr_set_tint` for the tinted object
(`0x48628c`..`0x4862a2`). The mesh renderers multiply the vertex brightness by the tint and convert with `_ftol`
without a clamp (Direct3D `0x50c8f2`..`0x50c90c`, OpenGL `0x50d3f3`..`0x50d40e`). `_ftol` uses `fistp qword`
(`0x56c7fd`), so a product of about 1e22 gives the 64-bit "integer indefinite" `0x8000000000000000`, whose low byte
is 0. The highlight is started for 5 s by `hud_set_timed_highlight`, whose only caller is `monster_drop_loot`
(`0x45306f`). *Inferred:* every loot drop flashes yellow (red and green only) instead of a pale gold. Any renderer
that goes through these mesh paths. Found in the `NOTES-target-chattracker-multispell.md` notes.

**K62. The reason pointer is a shared buffer.** For `KICK <channel> <nick> :<reason>`, the parser copies word 0
(channel) and word 1 (nick) into stack buffers (`0x4ace72`..`0x4ace9b`, `0x4acea4`..`0x4acecd`) but keeps word 2
as the pointer `irc_get_word` returns (`mov esi, eax` at `0x4acedf`). That pointer is always `irc_word_buf`
(`0x247999c`, `mov eax, 0x247999c` at `0x4ad982`). The next call, `irc_get_word(1)` for the "were we kicked" test
(`0x4acee4`), overwrites that buffer with the nick. `sprintf` then gets `esi` as the last `%s` of
"*** %s has kicked %s from channel %s (%s)" (string 241, `push esi` at `0x4acf18`). This needs a working chat
server (PXO is offline). Found in the `NOTES-target-chattracker-multispell.md` notes.

**K63. Squared distance against a plain radius.** For a character, the pick radius is `2 x character_size_radius`
(`fadd st(0), st` at `0x43c6eb`). After the sphere test against the closest point on the camera ray (`r^2 > d^2`,
`0x43c78c`..`0x43c79d`), the radius is halved (`0x43c79f`..`0x43c7a9`) and compared with `dz^2 + dx^2`, which is not
square-rooted (`fld [esp+0x14]; fadd st, st(1); fcomp [esp+0x48]` at `0x43c7ad`..`0x43c7b3`). So the horizontal limit
is `sqrt(R)` instead of `R`. The size radii are 0.15, 0.47, 1.0 and 99.9 (`character_size_radii` `0x59d7c8`):
for 0.47 the limit is 0.69, and for 0.15 the sphere test (0.30) is what stops it. Bosses take their radius from
`charinfo_boss_radius` (`0x4542ca`), so a boss radius above 1 gets a narrower test than intended. Type 3 objects
and type 11 (`0x43c6c2`, r = 2.0, limit 1.0) go through the same code. *Inferred:* the effect on clicks was not
tested. Found in the `NOTES-target-chattracker-multispell.md` notes.

**K64. OpenGL reports a constant 8 MB.** `summoner_cfg_load` sets texture quality 2 (`0x43e2e0`) and, when
`summoner.cfg` is missing (`jne 0x43e36c` at `0x43e32e`) or shorter than 64 bytes (`0x43e366`..`0x43e36a`), lowers it
to 0 when `gr_get_video_memory_mb() <= 8` (`0x43e36c`..`0x43e376`). For OpenGL that function returns the constant 8
(`mov eax, 8` at `0x507a03`), so the test is always true. Texture quality 0 is `bm_set_reduce_textures(1)`
(`sete al` at `0x4872d8`, `0x4872dc`). `summoner.cfg` is written only by `summoner_cfg_save` from
`options_apply_and_save` (`0x41faa3`), so this happens on a fresh install until the player saves the options, and
the saved file then keeps 0. Glide stores the small value it tests
against 4 and 12, probably MB (`gr_glide_init` `0x549e5d`..`0x549ea2`). Direct3D: see V60. Found while checking V60. SummonerFix now patches the constant to 64 (`bugfix.c`, 2026-10-15; the patch applies, checked in the log; the texture choice itself was not tested because that needs a missing `summoner.cfg`).

**K80. The message does not check the result.** `ai_can_be_knocked_down` (`0x475030`) refuses sub-modes `0xe`,
`0xc` and `0xd` (`0x475047`), but not casting. `ai_knock_down_local` then stores the duration, invalidates the
disable timer and calls `ai_set_mode(8, 0x2c)` (`0x4742fb`..`0x474316`). `ai_set_mode` refuses `AI_STATE_DISABLED`
while `ai_is_casting` (`0x47333e`..`0x47335e`) and returns 0. `ai_knock_down_local` ignores that result and still
shows "Knocked Down!" for `cur_character` (`0x47431b`..`0x47432f`). It also returns 1, so `ai_knock_down` sends
`mp_broadcast_ai_disable` in multiplayer (`0x47435d`..`0x474377`). The other machines run the same test on their own
copy (`mp_handle_ai_disable` `0x480f9f`), so a copy that is not casting there would fall (*inferred*, not traced).
Melee knock-downs come from `combat_apply_melee_hit` (3000-7000 ms, `0x466286`..`0x4662a4`), spells and the
Titus and Machival party knock-downs. Pushes (`ai_disable_2f_local`) and stuns (`ai_disable_31`) are dropped the
same way, without a message. Whether the game means casting to be uninterruptible is *unknown*; the message is the
slip.

**K90. No gamma under OpenGL.** `gr_set_gamma` builds `gr_gamma_ramp` with the curve
`255 * (i/255)^(1/gamma)` (`0x506ac1`-`0x506b10`). The second table, `gr_color_gamma_lut`, gets only `i` clamped
to 0..255 (`0x506b12`-`0x506b63`): an identity table. The back-end call has cases for Direct3D and Glide only
(`0x506b65`-`0x506b7a`). Direct3D uploads the ramp with `IDirectDrawGammaControl::SetGammaRamp` (`0x53ab6d`) and
Glide calls `grGammaCorrectionValue` (`0x54bac6`). The OpenGL back end has neither, and Sum.exe imports no gamma
ramp function (the only gamma string in the file is `grGammaCorrectionValue`). `gr_opengl_tmapper` maps
vertex colours through the identity table (`0x550150`-`0x550182`). The ramp reaches OpenGL only in
`gl_convert_a8_to_rgba8888` (`0x54d5f3`): the alpha of 8-bit-alpha bitmaps, at upload time. The option slider
(0.7 to 2.2, `options_apply_gamma` `0x41f380`) therefore changes only how opaque those textures are, and only for
textures uploaded after the change: nothing flushes the texture cache. Fonts are built in `GameInit`
(`0x43df76`-`0x43df9e`) before the saved gamma is applied (`0x43e110`), so they use gamma 1.0. The identity table
is right for Glide, where the hardware applies the curve; the OpenGL renderer never got a replacement.

Live with SummonerFix, which runs the game on OpenGL, until its 2026-10-16 fix (`gamma.c`; options 2 and 1 below,
see [SummonerFix](../summonerfix/index.md)). **The ways a fix could work:**

1. *Hardware ramp, smallest change.* Build a `WORD[3][256]` ramp from `gr_gamma_ramp` (`0x25d8e0c`, 256 ints,
   shifted left by 8 as `gr_d3d_set_gamma` does at `0x53ab3d`-`0x53ab61`) and pass it to GDI
   `SetDeviceGammaRamp` on the window's DC. SummonerFix already hooks `SwapBuffers` (`display.c`), so it can
   compare `gr_gamma` (`0x5a6064`) with the last value each frame without patching game code. Alternatively
   patch the OpenGL fall-through of `gr_set_gamma` (`0x506b71`, `jne 0x506b7f`) into a call to a stub. It needs
   the original ramp saved with `GetDeviceGammaRamp` and restored on exit, on focus loss and in the crash path.
   It changes the whole monitor, like the Direct3D and Glide renderers do. Windows refuses ramps that differ
   too much from identity, so the 2.2 end of the slider may fail; HDR displays and some drivers ignore it.
2. *Post-process in the window.* In the `SwapBuffers` hook, before the real call, copy the scaled back buffer
   to a texture (`glCopyTexSubImage2D`) and draw it back over the game area through the same curve: a fragment
   program with a 256-entry lookup texture (through `wglGetProcAddress`), or, with only OpenGL 1.1, `glPixelMapfv`
   with `GL_MAP_COLOR` and `glCopyPixels`. Skip it when `gr_gamma` is 1.0. This touches only the game window and
   needs no restore. It is more code, and the `glCopyPixels` path is slow on modern drivers. Savegame thumbnails
   are read earlier in the frame (`saveload_capture_thumbnail`), so they stay uncorrected, as with a hardware
   ramp.
3. *Vertex colours only.* Fill `gr_color_gamma_lut` with the ramp values (patch the second loop or write the
   table after each `gr_set_gamma`). That matches what the Direct3D software path does to vertex colours, but it
   leaves textures, the UI and bitmaps unchanged, and it would double the curve under Glide. Only a partial fix.

Option 1 matches the other two renderers. Option 2 is the cleanest for a windowed or borderless game.

**SummonerFix's fix.** Option 2 with a GLSL fragment shader (`pow(c, 1/gamma)`, the curve of `gr_gamma_ramp`), and
option 1 as the fallback when OpenGL 2.0 is missing. Nothing runs at gamma 1.0. Tested in agent mode on 2026-10-16
at gamma 0.7 and 2.0 (the main menu darkens and brightens; the black bars stay black). Not tested: the ramp fallback,
and the slider itself (we wrote `gr_gamma` directly). The 8-bit-alpha textures uploaded after a change still get the
curve in their alpha as before, so those few textures change opacity as in the original game.

**K91. Gamma twice on Direct3D.** `gr_d3d_tmapper` passes a vertex's own colour through `gr_gamma_ramp`
(`0x536247`-`0x536265`), and `gr_d3d_tmapper_multipass` does the same for the pass colours (`0x5366b6`-`0x5366fa`).
`gr_d3d_set_gamma` also loads the ramp into the display hardware when the gamma-control interface exists
(`0x53ab27`-`0x53ab6d`). `gr_d3d_init_device` gets that interface only when DirectDraw reports caps bit `0x20000`
(DDCAPS2_PRIMARYGAMMA, `test [0x2dc0610], 0x20000` at `0x539d8a`). On such a device lit polygons get the curve
in software and again on the screen, while textures and the UI get it once. Without the cap only those lit
polygons get any gamma. Glide avoids this by using the identity table for vertex colours (`0x54b2bf`).
*Inferred:* the visual effect was not tested. Live with the retail Direct3D renderer at any gamma other than 1.0.

**K92. Stale fog distances.** `gr_set_fog` stores the fog colour and the flag first (`0x507dc9`, `0x507dd4`). With
fog on it limits `near` to half of `far` (a quarter on Glide), then jumps to the back-end call when `near < 0`
(`0x507e1c`) or `far < near` (`0x507e2f`). The fog stays on with the previous call's distances and ranges, and
the back end is told to apply them (`push 1` at `0x507f4f`, `0x507f5a`, `0x507f65`). The game's callers are
`levelrender_fog_enable` and `levelrender_fog_disable` (fog off). `fog_set_distances` replaces values `<= 0` and
keeps far above near (`0x486b10`-`0x486b7a`), but `level_set_current_layer` copies a layer's two fog values
straight into the globals (`0x48218a`-`0x4821b6`), and only the first is tested `> 0`. So a level whose layer fog
has a negative distance would trigger it. *Inferred:* no retail level is known to do so (latent). A far distance
of exactly 0 with a positive near one is the related case in Still unclear.

**K93. Console anti-aliasing flag.** `cmd_antialiasing` toggles its own flag `cmd_antialiasing_on`
(`0x25d9224`, 0 at start) and passes the new value to `gr_set_antialias` (`0x505db9`-`0x505dda`). The options
setting goes through `config_antialias` (`0x60acf4`) and `options_apply_detail` (`0x487330`-`0x48733c`) and never
touches that flag. With anti-aliasing on in the options, the first command calls `gr_set_antialias(1)` again and
prints "antialiasing toggled!", and the options screen keeps showing the old setting after a real toggle. Only
reachable with the Direct3D renderer on a device that sets `gr_antialias_enabled` (`0x539dd5`); elsewhere the
command prints "not supported". A developer console command, so latent.

**K100. Three tests, two meanings.** `character_can_use_summon` unlocks a ring summon when the ring's total
is above 60,000 (`cmp eax, 0xea60; jle` at `0x44738b`). `ring_check_summon_unlock` shows the message only
when the old total is below 60,000 and the new one above it (`jge` at `0x45022d`, `jle` at `0x45023a`). A gain
that lands on exactly 60,000 shows nothing, and the next gain starts from 60,000, so it fails the "old below
60,000" test: the message is never shown, although the summon becomes usable. `ring_format_info` lists the
summon under "Available Summons:" from 60,000 on (`cmp eax, 0xea60; jl` at `0x44bb34`, seven times), so at
exactly 60,000 the pop-up offers a summon that cannot be cast yet. Ring gains are integers
(`(int)(xp x 2 x summon skill)` and its eighth), capped at twice the character's experience, so landing on
60,000 is possible but rare.

**K110. Removing by index from a table that shrinks.** `level_script_leave` runs
`for (i = 0; i < level_ambient_count; i++) level_ambient_remove(i)` (`0x4cccbb`..`0x4ccccc`).
`level_ambient_remove` deletes the emitter, moves the later records down and decrements the count
(`0x4cd2e0`..`0x4cd320`), so the loop deletes records 0, 2, 4, ... of the original table and stops halfway.
`level_script_reparse` then sets the count to 0 (`0x4cb3d9`), and the other half keep flag `0x1` in
`sound_emitters` (128 records) for good: only `sound_emitter_delete` clears it, and `sound_emitters_delete_all`
has no reachable caller. `snd_stop_all` in `state_game_leave` silences them, so the leak is not heard. Retail
levels have up to 44 `$Ambient:` lines (`Lenele1aa.tbl`; `sewer.tbl` and `Liangshan.tbl` 42), so a few visits
to the big towns fill the pool. After that `sound_emitter_find_free` returns -1, `sound_create_emitter`
fails and `level_ambient_add` drops the sound (`0x4cd2a2`), and so does every ground effect sound
(`fx_create_ground_effect` `0x48c584`). *Inferred:* loading a save leaves the level through the same
`state_game_leave`, so it does not help. Not seen at runtime;
an agent-mode test can watch `level_ambient_count` (`0x2491a58`) after entering Lenele1aa a few times.

None.

**K130. A "done" byte set before the event.** `ls_liangshan_frame` plays the looping "Boss" track once the
`phoenix_serpent` flag is set (after SERP-PHNX-Intro, `0x4f4a91`..`0x4f4aac`). The branch that plays `level_music`
again when `phoenix_dead` and `serpent_dead` are both set runs only while `ls_liangshan_phoenix_dead_done`
(`0x257ea61`) is 0 (`0x4f4ab4`..`0x4f4b09`). `ls_liangshan_init` sets that byte to 1 on every entry, whatever the flags
(`0x4f49be`), so the branch never runs. Nothing else switches the music back: the level's own restart
(`level_script_frame`, `level_music_stop_timer`) runs once after the level load, and the riders die without a
cutscene (`script_npc_goto_label(..., 2)`, `0x4f4b9e`, `0x4f4bf0`). *Inferred:* the boss music plays on until the player
leaves Liangshan. Found in the `NOTES-level-scripts-8.md` notes (bug 2).

**K131. "Played" kept in .bss.** `ls_templeint2_frame` plays "Nar-TempleAct4" while `ls_templeint2_act4_played`
(`0x257e9b7`) is 0 (`0x4ef1e0`..`0x4ef1fb`), and `ls_templeint2_init` clears it on every entry (`0x4ef1c3`). There is no
saved flag for it (events.tbl lists only `mach_intro`, `urath_killed`, `joseph_killed` and `mach_killed` for
templeint2). The end callback replays Machival-intro only when `mach_intro` is not set (`0x4ef230`..`0x4ef237`), so
only the narration repeats. In cutscene.tbl it is a 29 s camera flight with its own soundtrack
(`Temple_Urath_Act_4.wav`, fade out at 27.3 s). Saving is allowed in the level (see G130). *Inferred:* each load
inside the temple, before or after the Machival fight, starts with the act 4 narration again. Found in the
`NOTES-level-scripts-8.md` notes (oddity 9).

**K140. `item_types[-1]`.** The parser stores -1 for "(none)", and `item_type_find` returns -1 for an unknown
name. With body armour in slot 7, `script_action_replace_armor` first reads
`item_types[a->arg0c].slot` to skip a change to the same kind of armour (`0x4dcea9`..`0x4dcecf`), without the
`arg0c < 0` test that the no-armour branch has (`0x4dcee8`). For -1 that reads `0x8cbddc`, 0x100 bytes before
`item_types`. If the value there is 8 (heavy armour worn) or 6, the action returns at once and the armour stays
on. The retail cutscenes use "replace armor" only with real item names.

**K150. Both "done" bytes preset to 1.** `ls_ikaemostopint_init` loads `ebx = 1` (`0x4f9ef1`) and stores it into
`ls_ikaemostopint_found_boss_done` and `ls_ikaemostopint_boss_killed_done` on every entry, whatever the saved flags
(`0x4f9f2e`, `0x4f9f34`). `lsf_ikaemos_found_boss` starts the "Boss" music only while the first byte is 0
(`0x4fa050`..`0x4fa094`), and `lsf_ikaemos_boss_killed` restores `level_music` only while the second is 0
(`0x4fa140`..`0x4fa180`). Both branches are dead. The fight itself still works: IkmsPKing-Intro plays at
`$ipktrigger` once the four dagger slots have opened Door02-01 (`0x4fa0b1`..`0x4fa102`), and the Priest King's death
with the Ring of Stone in the party gives "Joseph acquired Summon Golem!" (`0x4fa1ba`..`0x4fa225`). Retail
`cutscene.tbl` gives IkmsPKing-Intro only its own soundtrack (`ikpk-intro.wav`), no music. The level music is
"IkaemosPalace" (`0x4f9ec1`). Same pattern as K130 (Liangshan), where only one byte is preset; LPalaceInt02 presets
its two bytes from the saved `bothdead` flag instead (`ls_lpalaceint02_init` `0x4f2519`..`0x4f2543`). *Inferred:* the
player hears the level track through the Priest King fight.

**K151. The alpha path ignores the section position.** `gr_glide_create_texture` cuts a bitmap into 256 x 256
sections and passes each one's rectangle in the section record (`+0x30` x, `+0x32` y, `+0x34` w, `+0x36` h,
`0x549424`..`0x549455`) to `gr_glide_create_texture_section`. The paths for the other formats start their 16.16
source position from `+0x30` / `+0x32` (for example format 6 at `0x54893b`..`0x5489c9`). The format 2 path (8-bit alpha
through `gr_gamma_ramp`, `0x548666`..`0x5486ee`) starts at the bitmap's first byte and steps by the full width
(`add ecx, edx` at `0x5486e0`), bounded by the full width and height (`0x5486aa`..`0x5486b5`), so every section gets
the top-left corner. No loader makes format 2: `bm_read_header` maps TGA depths 8, 16, 24, 32 to formats 1, 5, 6, 7
(byte table `0x502e08`, jump table `0x502df4`), PCX is 1 (`0x502add`), M2V 5 (`0x502d9b`), and `bm_read_vbm_header`
gives 5, 4 or 3 (`0x501df8`..`0x501e1b`); the `bm_create` callers pass formats 3, 4, 5, 6 and 1. So this is latent
even with modded data. It needs the Glide renderer: a 3dfx card or a Glide wrapper.

**K160. Duplicate join line.** For an id that is already known, `mp_add_new_player` returns the existing record
(`0x4af5a6`..`0x4af5d4`). Its "Received new player packet but player %s is already connected" text goes only into
`string_buffer`. Both `mp_handle_new_info` (`0x4af4cb`..`0x4af4d0`) and `mp_handle_net_info` (`0x4af77f`..`0x4af784`)
call `mp_announce_player_joined` (L236 "%s has entered the game.") for any non-NULL result. *Inferred trigger:*
when the host accepts a player, it sends that player NET_INF with every other player (`mp_send_net_info`, called by
`mp_server_accept_connections` `0x440d4f`). Once the player reports status 3, the host broadcasts NEW_INF for it
(`mp_host_update_session` `0x440dc8`..`0x440de1`) to every connected player except the new one (`mp_broadcast_packet`
skips flag `0x40000000` once, `0x441ad7`..`0x441aed`). A player C who is accepted while player B is between those two
steps gets B in both packets.

**K161. Half a centring.** `cd_prompt_disc2` sets x = (screen width - box width) / 2 (`0x43ea6f`..`0x43ea7d`) but
y = screen height / 2 (`0x43ea7f`..`0x43ea89`). It draws `ui_draw_text_box(x, y, w, h)` and the text at (x + 3, y + 2)
(`0x43ea9c`, `0x43eab3`). `level_load` shows the box when `eleh.s3d` is not on disk and `levelm.vpp` is not in the
data path (`0x4385cf`..`0x4385f0`), that is, when the level data is on CD #2.

**K170. The title gap is drawn but not tested.** Below a title, `ui_choice_popup_draw` moves down by the title
height plus `[0x57ef7c]`, 5 pixels at every resolution (`0x413d45`..`0x413d4e`), and `ui_choice_popup_show` adds the
same 5 to the height (`0x41380a`). `ui_choice_popup_hit_test` adds only the title height (`0x4139b3`..`0x4139d1`), so
every row's rectangle starts 5 pixels above its text. The hovered row is the one drawn highlighted, so the player sees
the wrong entry light up over the bottom 5 pixels of each entry, and the strip above the first entry selects it.
Titled popups: `death_popup` (`0x40f690`, "You have died." with Restart / Quit, flags `0xb`, shown by `hud_party_dead`
when the party dies) and the two PXO popups (`pxochat_version_check_frame` `0x42368c`, `pxochat_login_frame`
`0x42387e`). The drag rectangle of `ui_choice_popup_update_buttons` ignores the title entirely (`0x413b07`..`0x413b24`),
but only the untitled skills and status popups set the drag flag `0x4` (`0x4287a2`, `0x428816`, `0x42be85`), so that
part never shows.

**K171. The lock value stays.** Both pick paths, single player (`character_use_skill` `0x4295fa`..`0x429693`) and the
multiplayer host (`door_pick_by_mesh` `0x4aaa87`..`0x4aab57`), test the door's lock value (`door +0x58`) against 0
first: 0 means "no lock" (single player returns at once, the host sends result 2, which the client shows as "This
door already unlocked.", `0x4aa8bc`). Otherwise they roll `character_get_skill(0x15)` against it. A success clears
the two-stage locked flag (`savegame_two_stage_set(.., 0, ..)`, `0x42964d`, `0x4aaaf6`), runs the level's use hook,
plays sound 6 and the feedback, but never writes `door +0x58`. So the pick skill used again on the same door rolls
again: sound 6 and the feedback once more, or the failure sound 5 on a door that is already open, and result 2 is
only ever sent for doors that never had a lock. Inferred trigger: the skill takes the clicked
`world_interact_object` without a locked test (`0x4295d3`..`0x4295e8`); the world pick filter was not traced. The
use hook is harmless twice: `door_toggle_state` runs it on every open too (`0x476f21`).

**K180. The slot is set before the room test.** `localize_set_string` stores the current write pointer in
`localize_strings[id]` (`0x504417`) and the id in `localize_max_string_id` (`0x504439`), and only then tests whether the
text fits before the end of the 0xa000-byte buffer (`mov eax, 0x25d847d; sub eax, edx; cmp ebp, eax` at
`0x50442e`..`0x504437`). A string that does not fit returns NULL without advancing the pointer (`0x50443f`..`0x504448`),
so its id points at unwritten (zero) bytes until the next string that fits is copied to the same place
(`0x504477`..`0x5044a7`), and then shows that string. The retail sections need 24,673 (English), 27,668 (German) and
28,389 (French) bytes.

**K181. A capital A for a capital E.** The French table maps 0xC4 (`Ä`) to 0x90 (`mov byte [ecx], 0x90` at `0x505210`,
jump table `0x50531c` / `0x505384`), which is `É` in CP437, while `É` (0xC9) itself goes to 0x82 like `é` (`0x505206`).
0xC4 was probably meant to be 0xC9. The retail French text has no `Ä` (counted in `strings.tbl` and the four name
tables), and v1.40 never selects French: the only call of `localize_set_language` passes 0 (`0x504f64`), and nothing
else writes `localize_language`.

**K190. A soundtrack handle that is freed but kept.** `cutscene_unload` stops and frees the soundtrack handle in the
cutscene object (`+0x1630`, `0x603498`; `snd_stop` and `snd_free` at `0x4352c4`..`0x4352f1`) but never sets it back to -1.
`cutscene_load_sounds` writes it only when the entry has `$Soundtrack` (`0x43502b`..`0x435056`), and shot 0 always
streams it (`snd_play_streamed` at `0x436bbb`). Seven retail entries have no `$Soundtrack`: the four dragon summons
(level `"any"`, played by `summon_create` at `0x47ef57` on every dragon cast), "test", "Test" and "WolongFlythrough". Story
cutscenes with a soundtrack play from the start of a new game on ("Game-Intro" has `intro-final.wav`), so by the time a
dragon is cast the handle is stale. Streams take sound slots
from 0 (`snd_get_handle` with its last argument 1, `0x520805`..`0x520816`), other sounds from 8, and only `music_play`
(`0x4690f9`) and the cutscene soundtrack ask for stream slots. `snd_level_release` in `cutscene_cleanup`
(`0x436660`) frees the soundtrack's slot, so the stale slot is the lowest free stream slot afterwards. Two cases
(*inferred*): if the slot is still free, `snd_load` fails on the empty name and the dragon scene is silent, as it would
be with a reset handle. If a later `music_play` took it, for example a boss intro's continuation that starts the boss
track at once (`ls_templeint2_machival_intro_cs_end` `0x4ef2c2`, `ls_wolong2_pijian_intro_cs_end` `0x4f1e2d`), the dragon
scene restarts that track from its beginning, unlooped, under the scene (`cutscene_play` has just stopped the music).
At the end `cutscene_cleanup` fades the stream out over 200 ms, restarts the saved track (`0x4366aa`) and then
`cutscene_unload` frees the slot that track is now playing from (`snd_free` clears the name and drops the buffer,
`0x520c10`). A failed `cutscene_load_data` also calls `cutscene_unload` (`0x435472`), which frees the stale slot again.

**K191. The edge-turn block reads its own store.** `camera_process_input` stores the held-drag bit of the current
input (`(flags >> 4) & 1`, `0x4346e7`..`0x4346f1`) into `0x5fccb7` and, with the cursor within 3 pixels of an edge, at
once tests `(~flags >> 4) & 0x5fccb7` (`0x43470f`..`0x43471b`). That is "held now and not held now", always 0, so the
branch that clears `0x5fccb7` and sets the block byte `0x5fccb8` (`0x43471d`..`0x434724`) never runs. `0x5fccb8` stays 0;
its test (`0x43472d`) and its reset when the cursor leaves the edge (`0x4347c2`..`0x4347e6`) are dead. The intended
test was evidently "held last frame, not held now". *Inferred:* holding the left button to walk with the cursor at
the screen edge, then letting go, turns the camera at once, because the edge turn needs only that no mouse button is
down (`0x434736`..`0x434750`).

**K192. Minitrack handles stay -1.** `cutscene_load_sounds` sets `minitrack_sounds[i]` (`0x60349c`, 4 slots) to -1 for
every `+Minitrack` (`0x43508a`..`0x435090`) and never loads the file. `cutscene_events_frame` passes that value to
`snd_play` at the track's shot (`0x436fa0`..`0x436fa8`), which does nothing for -1. No retail entry has `+Minitrack`.

**K193. A name-sorted scan picks the number.** `screenshot_save` lists `screen*.tga` or `screen*.jpg` in the install
directory with `file_find_files` mode 1 (`push 0x1` at `0x432539`), which inserts every name in `_stricmp` order
(`0x513507`..`0x513519`), and reads the number from the last node (`list.prev`, `0x432587`..`0x43259c`), then adds 1.
Up to `screen999` that is the highest number. `screen1000` sorts before `screen999`, so from then on every screenshot is
`screen1000` and replaces the previous one. A matching name that sorts last and does not parse as `screen%d` (for
example `screenshot`) leaves the number uninitialised: `sscanf` writes nothing into the stack slot `[esp]`
(`0x43258e`..`0x4325a9`).

**K194. Copied usage line.** `cmd_play_and_load_cutscene` prints the same literal as `cmd_play_cutscene`, "Usage:
play_cutscene <cutscene name>" (`push 0x593b90` at `0x43753d` and `0x4375fd`).

**K200. Early return skips the refresh.** `lobby_parse_channel_list` updates each channel's user count or adds the
channel (`0x43f504`..`0x43f528`) and calls `pxochat_sync_channel_list` after the last record (`0x43f544`). When a
record has a name but no count or no topic (`strtok` returns NULL at `0x43f4bb` or `0x43f4dc`), it returns at once
(`0x43f549`), so the counts and channels taken so far are not shown until the next `/list` (every 10 s,
`lobby_request_channel_list`). Needs a chat server that sends such a line; the PXO servers are gone.

**K210. An inverted test in one of four scroll loops.** [notes 5] When the camera moves one cell, `snow_update`
shifts the 15 x 15 grid and moves the row that left to the other edge, then calls `snow_cell_update` on it, which
returns true when the cell has no flake block (culled or the pool is empty). For -x, +z and -z it refills the cell
when that returns false (`jne` at `0x494dfc`, `0x494e50`, `0x494e9c`); for +x it refills when it returns true
(`je 0x494dbf` at `0x494db6`), when `snow_cell_fill` returns at once for lack of a block. So the 15 cells that
scroll in on the +x side keep their block's old flakes, which belonged to cells 75 m away (or, for a block just
taken from the pool, to whatever cell had it before). The per-frame loop then treats them as filled and only
replaces `n` flakes per frame (`snow_cell_fill_count`, 2.67 flakes per second), so a block is renewed after about
12 s. *Inferred* for what is seen: the grid reaches 35 m from the camera, so the edge row may be in fog.

**K211. Time read from and written to the mount.** [notes 11] `serpent_seat_rider` and `tiger_seat_rider` read the
mount's animation time and write it to the rider's instance (`get_set_time(&t, 0)` on `[mount+0x314]`, then
`get_set_time(&t, 1)` on `[rider+0x314]`, `0x495e61` / `0x495e76`). `phoenix_seat_rider` uses the mount's instance
for both calls (`mov ecx, [esi+0x314]` at `0x4982e5` and `0x4982f6`), so the rider's time is never set. *Inferred:*
the rider's ride cycle runs out of step with the horse's.

**K212. Walk in the stand slots.** [notes 15] `tiger_set_stage2_anims` (called twice by `tiger_stage_move_update`,
`0x49c38f`, `0x49c3d9`) writes `tiger_anim_stage2_walk` into the stand, ready, stand-hurt and ready-hurt slots
(`+0x5ac`, `+0x5b8`, `+0x5c4`, `+0x5d0`, `0x49a144`..`0x49a15c`) as well as the walk slots. `tiger_load_anims`
stores `ride_horse_ready_axe` at `0x2452c7c` (`0x49abaa`), the only reference to that address. Retail
`Tiger Rider.tbl` has `ride_horse_ready_axe` with `tigr_stage2_ready.mvf`. *Inferred:* the axe-stage rider never
uses its ready pose. Stage 2 is reached after the four spear throws in `templeint`.

**K213. One part short, in code that never runs.** [notes 8] The first branch of `serpent_illusion_start`
deactivates `serpent_part_handles[0 .. num - 2]` (`edi` from 1, `cmp edi, eax; jl` with `eax` =
`serpent_num_extra_parts` 2, `0x496bdc`..`0x496c0d`): only the first copy. Every other loop over the parts runs
`num + 1` times from the serpent's own handle. But sub-mode `0x38` is set only by `serpent_ai_frame` when the
serpent's pending sub-mode (`0x24523a8`) is `0x38` (`0x49741c`..`0x497430`), and the only write to that word stores
`0x20` (`serpent_choose_attack` `0x4965ec`). So `serpent_place_parts`, the illusion start and update, and the
copies' appearance never run in v1.40; the two copies made by `serpent_spawn_rider` stay hidden. `serp_boss.tbl`
still lists "stage2 illusion" (variation 7).

**K230. Two terms packed twice.** `matrix3_pack_orientation` keeps five sign bits (`out +0x10`, from `m[2]`, `m[5]`,
`m[8]`, `m[6]` and `m[7]`). It then stores `m[0]`, `m[1]`, `m[0]`, `m[1]` as the four packed terms
(`0x4753f3`..`0x475425`). The receiver, `matrix3_unpack_orientation`, reads terms 3 and 4 as `uvec.x` / `uvec.y` (`m[3]`,
`m[4]`, `0x47547d`, `0x475484`) and rebuilds the other four terms from them with square roots. So `uvec` comes out
as `rvec` with only its z sign different, and `fvec` comes from `sqrt(|1 - 2·m0²|)`-style terms. A yaw-only monster
gets a forward row that points mostly up. The only sender is `mp_send_new_lep` (`0x47fe7f`), and its only caller is
`monster_layout_load` (`0x44012b`), the `load_mlayout` console command (C203 already covers its wrong character type).
`mp_handle_new_lep` builds the entity with that matrix (`living_entity_create` `0x47ff88`). `mp_handle_obj_update`
later calls `matrix3__set_from_angles(0, 0, heading)` (`0x480849`) whenever the heading it extracts differs from
the received one (`0x48080a`..`0x48081f`), so the next update repairs it. *Inferred:* how it looks; not run.

**K231. `fchs` on the clamp.** `camera_smooth_height` computes `step = frame_dt × (wanted - height) × 4.0`
(`0x580e18`; `0x478736`..`0x478750`). When `|difference| < |step|`, it replaces the step with the *negated* difference
(`fld [esp+4]; fchs` at `0x47876f`..`0x478773`). The height then lands at `height - difference`, twice as far from
the wanted height as before. It snaps only once within 0.001 (`0x581938`). The overshoot needs `frame_dt × 4 > 1`.
`FrameDone` clamps `frame_dt` (`0x5a5740`) to `max_reported_frametime`, 0.25 (`frametime_initialize` `0x5014ee`).
The only writer that raises it, `frametime_unlimit` (`0x501750`, 10 s), has no caller. At exactly 0.25 the step equals
the difference and is not replaced. So retail never runs the `fchs`.

**K220. Bytes printed as doubles.** The six sound parameters are bytes (`+0x68`..`+0x6a`, `+0x74`..`+0x76`); the
function pushes each as one dword and formats it with `"%.1f"` (`mov dl, [esi+0x69]; push edx; push 0x5a0c48` at
`0x4e5539`..`0x4e5543`, and five more to `0x4e565e`). `sprintf` reads 8 bytes, the byte and the caller's saved `edi`
above it, which as a double is almost always a tiny number shown as "0.0". `script_editor_door_ok` stores
`(int)atof(min)`, `(int)(atof(volume) * 255)` and `(int)atof(rolloff)` back into the bytes
(`0x4ec33c`..`0x4ec384`, `0x4ec42a`..`0x4ec472`), and Save writes them into `+Open sound:` / `+Close sound:`.
(Volume is kept as 0-255, so even a correct "%d" would need the /255 that the script writer applies.)

**K221. The selection's kind decides the hover's shape.** The hover block tests `script_editor_sel_kind == 4`
(`0x4e035d`..`0x4e0367`) and then reads the HOVER object as a level object (`+0x50`..`+0x60` bounds, `0x4e0372`..);
for a hovered character or navpoint those offsets hold other fields.

**K222. The wrong gadget.** On Enter (`cmp eax, 0x1c` at `0x4ebc60`) the handler calls `ui_gadget__clear_focus` on
`script_editor_door_key_inputbox` (`0x2559688`, `0x4ebc65`..`0x4ebc6a`), not on its own box `0x25490d8`.

**K223. A missing else.** With `pyrul_missile_bone` (`0x247fd34`) at -1 the function copies the boss's frame into
the outputs and then still calls `character_instance__get_bone_world_transform` with bone -1, which overwrites them
(`0x4b95fd`..`0x4b9665`). `pyrul_boss_load` looks the tag up once; the retail `Pyrul.vim` has it.

**K250. A 3-bit count.** The receiver count is bits 6-8 of the record's first dword. The append stores at
`+0xe0 + 4*count` and adds `0x40` inside the mask `0x1c0` (`0x4c05b3`..`0x4c05d5`), so the eighth object goes to
`+0xfc` and the count wraps to 0; the ninth overwrites the first. The `cmp ecx, 0x10; jae` exit after it
(`0x4c05d2`) tests the old count (0-7) and never fires. `shadows_render` draws the shadow on `count` objects
(`shr eax, 6; and eax, 7` at `0x4a4394`..`0x4a439a`).

**K251. The minimum is kept and never used.** For every edge crossing along the leap,
`evil_urath_leap_check_path` writes the distance to `*out` (`fst`/`fstp dword [edx]` at `0x4c4321`..`0x4c4327`) and
keeps the smallest in a local (`[esp + 0x10]`, `0x4c4329`..`0x4c4334`) that is never read again. The return value
(`*out < FLT_MAX`, `0x4c435b`..`0x4c4374`) is still right. `evil_urath_ai_leap_rise` computes the rise speed and the
peak from `*out` (`0x4c48f3`..`0x4c491c`, `0x4c49e6`..`0x4c4a64`).

**K252. The terminator goes one past.** `File::read_string(buf, max, ...)` stores up to `max` characters and then
writes the NUL at `buf[max]` (`0x525f4a`..`0x525f54`); `FileReader::read_string` does the same
(`0x513e29`..`0x513e35`). The `.plr` readers pass 0x20 for the name (`0x46d570`, `0x46d73d`). In the summary the
byte after the name is `+0x20`, which the next `read_int` overwrites (`0x46d591`), so a 32-character name has no
terminator. The character creation screen accepts at most 31 characters (`cmp esi, 0x1f; jge` at `0x406e05`), so
only an edited `.plr` triggers it. `vim_read` passes 0x7f for 0x80-byte buffers and is safe.

**K260. The group volume applied twice.** `snd_calc_volume_3d` returns `group volume * sound volume * attenuation`
(`fmul [4*edx + 0x2cb542c]` at `0x52108b`, `[esi + 0x28]` at `0x521008`). Without DirectSound3D, `snd_play_3d` divides
only the sound volume out (`fdiv [esi + 0x28]` at `0x521572`) and passes the rest to `snd_play` or `snd_play_looping`.
`snd_play` multiplies by `snd_get_volume`, which is group volume times sound volume again (`0x520df5`..`0x520dff`), so
the group volume is squared. The DirectSound3D path passes `group * volume` once (`0x521525`..`0x52152c`). The
sound-effects slider sets group 0 (`options_set_sfx_volume` `0x41f25f`), and `sound_ref_load` and the footstep and
ambient loaders get their handles in group 0. `sound_play_3d_at` (`0x468de0`) has 14 callers (footsteps, weapon hits,
spell cast and hit sounds, moves, doors, loot drops, `feedback_play`). Emitters start through `snd_play_3d` too
(`sound_emitter_play` `0x5463a7`, `0x5463e6`) and are later set by `sound_emitter_update_volume` with
`snd_set_volume` (`0x5464d4`), which takes the value as it is. `snd_play_3d` takes this path whenever DS3D is off
(`snd_is_ds3d_enabled` at `0x5214f7`). DS3D is on only with `Soundcard=EAX` (`0x43d9bb`..`0x43d9cd`), `-directsound3d` or
`-eax` ([Sound](sound.md#start-up)); the default `Soundcard=DirectSound`, the only sound value SummonerFix offers besides
`No Sound`, leaves it off.
At full volume (the default 1.0) nothing changes. The 1.75 that `snd_play_3d` passes as scale (`push 0x3fe00000` at
`0x5214c3`) is applied only below 1.0 in the distance case (`0x52100b`..`0x52101f`), but unclamped when the sound is
exactly at the listener (`0x520fe3`); the notes say "never".

**SummonerFix's fix** (`bugfix.c`, 2026-10-16): the `fld` / `fdiv [esi + 0x28]` / `fstp` at `0x52156e` becomes a call
that divides out the group volume as well (zero when the group volume is 0), so world sounds follow the slider
linearly, as on the DirectSound3D path. At full volume nothing changes. Checked in the log (the patch applies); not
yet judged by ear.

**K261. Property 14 twice.** EAX 2 listener properties are numbered 2 (ROOM) to 15 (FLAGS) and fill
`EAXLISTENERPROPERTIES` in that order. `eax_get_listener_props` reads them one by one with `IKsPropertySet::Get`, but
the last two calls both ask for 14 (air absorption HF): into `+0x30` (`push 0xe` at `0x5223e4`) and into `+0x34`, the
flags (`push 0xe` at `0x522405`). `eax_save_props` writes the flags with `"%d\n"` (`0x52221f`), so the file holds the
float's bits, and `eax_load_props` reads them back and sets all parameters at once (`eax_set_listener_props`, property 1,
size 0x38, `0x522596`). All 49 `.eax` files in `tables.vpp` end in `-5.000000` and `-1063256064` (`0xC0A00000`, the bits of
-5.0), so they were saved with this bug. They are not read: `level_parse_table` builds the name with
`file_make_path(path_internal_copy, "<level>.eax")` (`0x4cbf82`..`0x4cbfaf`) and `eax_load_props` opens it with the C
runtime `fopen` (`0x52243e`), which does not see packfiles; the retail install has no `data\internal` directory, and every
level keeps the environment "generic" set just before (`0x4cbf75`). `cmd_eax_save_props` (`0x4ca970`) is a console
command, and v1.40 has no console. *Inferred:* loaded, the flags `0xC0A00000` clear the six EAX scale and clip flags
and set reserved bits; a driver may also reject the whole set.

**K262. m[0][1] read for m[0][0].** For a negative trace with `m[2][2]` the chosen diagonal,
`quaternion__set_from_matrix_b` computes `sqrt(m[2][2] - (m[0][1] + m[1][1]) + 1)` (`fld [ecx+0x4]; fadd [ecx+0x10];
fsubr [ecx+0x20]` at `0x522841`..`0x522847`); the other two cases use the right diagonals (`0x522892`, `0x5228e2`). The
result is normalised (`0x522887`), so the rotation is wrong, and NaN when the root's argument is negative. Case 2 is
taken whenever `m[2][2] > m[1][1]` (`0x5227fa`..`0x522829`), so a turn about the up axis alone (where `m[1][1] = 1` is
the largest) never reaches it. The live caller is `vfx_instance__set_node_transform` (`0x537c3a`, `0x537d9e`),
which in fact writes the given position and orientation into a node's track record and moves the children with it.
Its only outside caller is `spell_probe_ground_point` (`0x47e4a9`), which puts a `$groundNN` node of a ground spell
effect on the floor with the node's own orientation. The other caller, `s3d_read_navpoint` (`0x46f348`), runs it only
when writing a `.s3d`, and `level_load_s3d` only reads.

**K263. Projected spot-light textures.** A spot light (type 3, `light_create_spot` `or ecx, 0x3` at `0x51e436`) keeps
its texture at `+0x68`. `light_sample_projected_bitmap` locks it with `bm_lock` when `bm_get_data` has no data
(`0x51f10b`..`0x51f128`) and nothing unlocks it: `light_delete` unlocks only types 1 and 4 (`0x51e57a`..`0x51e5ad`),
which no create function sets. The format jump table (`0x51f38c`) sends format 2 to the end (`0x51f36f`) and formats
outside 1..7 skip it (`ja 0x51f36f` at `0x51f172`), so the colour returned is the uninitialised vector of
`0x51f0db`. In `lights_shade_point` the sampled texel replaces the attenuated colour (`0x51ef8f`..`0x51ef93`), also when
the point is beyond the outer radius, where the attenuation step is skipped (`je 0x51ee4a` at `0x51edad`). Levels make
spot lights from record types 1 and 4 with an optional texture index at `+0x48` (`level_lights_setup` `0x48e60a`).
In the 51 retail `.s3d` files that have lights (2,879 records: 2,582 type 0, 173 type 1, 124 type 4), no spot record
has a texture index other than -1.

**K264. The margin counted twice.** `ui_listbox__process` takes the click y relative to the gadget
(`mouse_y - screen_y`, `0x52d1a3`..`0x52d1a9`), checks it against the vertical margin `+0x2c0`, and calls
`entry_at(0, vscroll + y, 0)` (`0x52d1d1`..`0x52d1de`). `ui_listbox__draw` draws the entries inside a clip that starts
at `screen_y + v_margin` (`0x52f1da`..`0x52f218`), at `entry y - vscroll` (`0x52f2a7`..`0x52f2b0`). So the hit test is
`v_margin` pixels low. `ui_listbox__create` sets both margins to 2 (`0x52d6ea`, `0x52d6f0`), and only the table parser
changes them. The script editor's list boxes (`script_editor_create_ui`) are the only ones in use.

**K265. Page Down onto the ring head.** When the selected entry is taller than the view, `select_page_down` takes
`selected->next` (`mov edi, [edi + 0x8]` at `0x52e09b`) and tests it only for NULL. For the last entry that is the ring
head at list box `+0x2d4`, a `ui_listbox_entry` built by the constructor (`0x52d5fd`) with data `+0x34` = -1 and no
text. It becomes the selection and message 0x18 goes to the callback (`0x52e0e3`..`0x52e0ee`). Entries in v1.40 are
one-line names (script editor), so no entry is taller than a list box.

**K270. A test that can never be true.** For species 0x5a in a cutscene (`game_flags` `0x20`) the function
returns before the attachment list only when `_stricmp(cutscene_script_table(), "Ghost-Escape_cscript.tbl")`
*and* `_stricmp(..., "Ghost-Dismount_cscript.tbl")` are both 0 (`0x48333d`..`0x483369`: the first `jne` goes on
to draw, only the second `je` returns). One name cannot equal both, so the list at `+0x728` (pairs from
`+0x72c`, slot 0x11 skipped) is always drawn. *Inferred* intent: or. Both scripts are in `tables.vpp`
(`$Level: "Rand-ForestNite1"`). Whether the drawn props look wrong in those two cutscenes was not checked.

**K271. Index of the wrong list.** `lamp_glow_update_lobjs` skips the occluder whose attach-list index equals
its own (`cmp edi, [esp + 0x14]` at `0x48451f`). `lamp_glow_update_effects` copies the test
(`cmp edi, [esp + 0x20]` at `0x484785`), but its outer index walks the sorted list (`levelrender_sort_list`,
`0x48461d`..`0x484815`) while `edi` walks the attach list. So for the glow at sorted index i, attach entry i is
never ray-tested (`lobj_raycast` `0x4847b4`), and the glow fades in although that object blocks it.

**K272. `shadows.tbl` unused.** `shadows_parse_table` stores up to 175 records of 0x50 bytes (name, `$Radius:`,
`$Scale:`, `+Prop`) at `0x22fb720` (`0x4875b4`..`0x4875d8`); no instruction reads `0x22fb720`..`0x22fedcf`
(only the count `0x22fedd0`, inside the parser). `shadow_draw_character` reads only its character argument and
passes the constants `0x59c12c` / `0x59c130` / `0x59c134` (0.15, 1.0, 5.0) and `shadow_bm` for everyone
(`0x487677`..`0x48769f`); the 20.0 radius from `shadows_draw_characters` (`0x487734`) is ignored. Retail
`shadows.tbl` has 151 entries, radius 0.47 to 25.55 (Boat), Giant Salamanka 10.37, Joseph 1.49.

**K273. Arguments pushed in the wrong order.** `level_parse_table` pushes `level_fog_near` first and
`level_fog_far` last (`0x4cbefc`..`0x4cbf09`), so `fog_set_level_default` gets the far value as its first argument
and stores it in `fog_level_near_distance` (`0x59bd88`, `0x486ba1`), which `fog_restore_level_default` (called
next, `0x4cbf11`) and `fog_set_distances` (for a distance <= 0, `0x486b55`) copy into the near distance.
`levelrender_fog_enable` passes that as `gr_set_fog`'s start (`0x4829a9`). `level_set_current_layer` and the
camera defaults overwrite the fog on the next layer change, so the swap shows only through the <= 0 fallback.
Retail: no level table has `+Fog Near:` or `+Fog Far:`; four `cutscene.tbl` `$Fog` keys have near 0.0. The
per-layer `+Distance:` values are *not* swapped (see "Checked, not bugs").

**K274. Clip x subtracted, y not.** `gr_project_vertex` adds both clip offsets (`fiadd [0x25d8d5c]` at
`0x50eb0e`, `fiadd [0x25d8d60]` at `0x50eb1f`). The four overhead drawers call `gr_get_clip` and subtract only the
x offset before the clipped `gr_bitmap` / `gr_string` (`0x481d9c`, `0x4849ce`..`0x4849e1`, `0x484c0a`..`0x484c1c`,
`0x4854c8`..`0x4854d0`). Outside cutscenes the 3D view clip starts at y 0 (`interface_set_view_clip`
`0x412220`; only the left panels move x). In a cutscene it is the centred 2:1 letterbox, but health bars,
speech text and speaker icons return on `game_flags` `0x20` (`0x4848e2`, `0x484aeb`, `0x48581f`).
`levelrender_draw_float_text` only tests its own flag (`0x59bd78`, `0x485380`); whether retail cutscenes ever
show floating text (damage, "+Level!") was not checked.

**K280. Flickering right bar after a cutscene (a SummonerFix bug).** With `CenterMenus=1` and a game width above
1024, SummonerFix shifts the 1024-wide menu layout to the middle of the window (`GetMap` in `display.c`), so the
game's area right of x = 1024 becomes the right part of the picture. The game draws menus only inside 1024 x 768
and does not clear the rest, and SummonerFix cleared only the bars outside the game area, not that strip. After a
level had been drawn, the two back buffers kept old frames of it there, shown in turn with black: in four agent-mode
captures taken 70-110 ms apart on the main menu (Jade Temple reached with `play_and_load_cutscene`, then back to
the menu), one showed the Jade Temple in the strip. Fixed 2026-10-16: on menu screens (game state not 4)
`Hook_SwapBuffers` also clears the area right of the 1024-wide layout. The same four captures with the fix were
all black there. In-game full-screen panels are not changed (the world may show beside them).

**K290. The normals get the bone translation.** All three character renderers skin each vertex with up to four
bone matrices of 0x30 bytes. They transform the position with the rotation rows and add the translation
(`fadd [eax + 0x24]` / `+0x28` / `+0x2c`, OpenGL `0x50d00e`..`0x50d056`). They then transform the normal the same way,
translation included (`0x50d088`..`0x50d0de`), sum the weighted results into `ca_skin_normals` and normalise them
(`0x50d33b`) before `lights_shade_vertices` (`0x50d366`). Direct3D does the same at `0x50c514`..`0x50c58d` and
`0x50c846`, Glide at `0x50b7e4`..`0x50b859` and `0x50bb00`. A normal is a direction and should only be rotated.
*Inferred:* when a bone moves away from its rest position, the lighting normals of its vertices lean towards the
bone's offset. Characters drawn without an instance (`0x2682990` = 0) copy the mesh normals and are lit correctly
(`0x50d1c0`..`0x50d1f1`). Found in the `NOTES-unnamed-ca-mesh-gr-3d.md` notes.

**K291. The seam copy reads the normal's z.** For a vertex that copies an earlier one (a texture seam,
`ca_vertex_done` set), `gr_d3d_render_character_mesh` takes the fog alpha from `ca_skin_normals[j - d].z`
(`add ecx, 0x2622e48; mov eax, [ecx + 8]; call gr_get_fog_alpha`, `0x50c37f`..`0x50c39d`) and copies the position
from `ca_skin_positions` (`0x50c3a6`..`0x50c3b3`). Every other vertex uses its view-space depth
(`[esi + 0x25e74b8]`, `0x50c778`..`0x50c77e`). The result goes into the fog byte of the vertex (`+0x17`, `0x50c3a8`).
`gr_get_fog_alpha` returns 255 for a depth at or below the fog start (`0x50d839`..`0x50d848`), and a unit normal's z is
at most 1, so seam vertices are never fogged. Glide copies the whole vertex and OpenGL draws without a per-vertex fog
alpha, so only Direct3D, the retail default, is affected. Found in the `NOTES-unnamed-ca-mesh-gr-3d.md` notes.

**K292. The hover test compares x with a y bound.** For the "more" (kind 5) and "end" (kind 6) entries,
`dialogue_topic_entry__draw` tests the mouse against the topic box's corner (`0x2545b6c`, `0x2545b70`) and the text
size. The last test compares the mouse x with `box y + text height` (`mov edx, [esp + 0x18]; add ecx, edx;
cmp eax, ecx` with `eax` still the x, `0x4d6fe8`..`0x4d6ff0`; "end" `0x4d707c`..`0x4d7084`), so there is no lower
bound on the mouse y. The click test in `dialogue_frame` uses `point_in_rect` with the same rectangle and is right
(`0x4094d3`..`0x4094f0`, `0x409560`..`0x40957d`). *Inferred:* the highlight colour shows anywhere below the entry
within its width (or never, if the box's x is beyond its y plus the text height), while clicks work only on the
text. Found in the `NOTES-unnamed-ai-move-think-dialogue.md` notes.

**K293. The full ring drops the speaker.** `dialogue_box__add_text` keeps 64 lines (`cmp eax, 0x40` at `0x4d7262`).
While the ring is not full it passes the speaker (its fourth argument) to `dialogue_line__init` (`0x4d73cc`..`0x4d73ea`),
which stores it at `+0x18` for text lines (`0x4d6b06`..`0x4d6b0d`). Once the ring is full it pushes 0 instead
(`0x4d72fa`..`0x4d7316`). `conversation_update_talk_anims` picks the talking character from `+0x18` of the visible
lines (`0x4d8b49`, `0x4d8bb5`). The ring is cleared only when a conversation opens (`dialogue_init` -> `0x4d7650` ->
`0x4d7140`). Each text adds a spacer line as well, so about 32 texts fill it. *Inferred:* in a long conversation the
speakers stop moving their lips and hands. Found in the `NOTES-unnamed-ai-move-think-dialogue.md` notes.

**K294. The fade flag outlives a sound that ends by itself.** `ds_stop_fade` sets flag 8 on the channel and a
75 ms timer (`0x55d673`..`0x55d6ab`); `ds_do_frame` then calls `ds_channel_update_fade` for every channel with that
flag. If the sound has stopped, the update returns at once (`ds_channel_is_playing`, `0x55d6f8`..`0x55d702`); flag 8
is cleared only when the volume reaches 0 (`0x55d742`..`0x55d74c`). `ds_channel_release` does nothing for a channel
with flag 8 (`0x55c5c5`..`0x55c5d5`). `ds_find_free_channel` takes any stopped channel without flags 1 or 4, calls
`ds_channel_release` on it and returns it (`0x55d0b4`..`0x55d0f7`). `ds_play` then duplicates the new buffer into the
channel and sets its volume (`0x55d170`, `0x55d21e`), so the old buffer is never released and flag 8 stays. On the
next frame the fade timer has long run out, and the update subtracts the old step (`volume / (ms / 75)`) from the new
volume and releases the channel when it reaches 0. `character_play_move` fades the previous move sound over 100 ms
(`0x46112b`..`0x461137`), a step of the whole volume, so a sound that ends in that window leaves a channel that cuts
the next sound on it at once. *Inferred:* now and then a sound effect is missing. Found in the
`NOTES-unnamed-vsdk-tail.md` notes.

**K295. The sub-step counter advances by 0.5.** `vfx_emitter_instance__move_particles` integrates the force records in
steps: total = elapsed effect time x 1/15 (`[0x581a08]`, `0x560b85`..`0x560b9e`), each step min(total - done, 1/30)
(`[0x582338]`, `0x560bb7`..`0x560bde`), then done += 0.5 (`[0x57e6b0]`, `0x560c16`..`0x560c2d`). So one step runs and
any update longer than 1/30 integrates only 1/30. The same loop is in the three other branches (`0x560d9a`, `0x560f01`,
`0x561131`). *Inferred:* force-driven particles move by less at low frame rates and differently at each frame rate.
Found in the `NOTES-unnamed-vsdk-tail.md` notes.

**K296. Two slips in effect nodes bound to a character.** `vfx_instance__list1c_item`, the light list walker, steps
its index by 2 (`sub ecx, 2` at `0x5385e2`) where `list18_item` and `list20_item` step by 1 (`dec ecx`, `0x5385c5`,
`0x538605`). `vfx_base__decode_tag` returns the plain light index (kind 2, `0x53734b`..`0x537365`), so light 2 resolves to
light 1, light 3 to light 2 and so on. `vfx_instance__bind_node` (light case `0x537600`..`0x537634`, from
`fx_bind_character_tags`), `get_node_transform`, `set_node_transform` and `get_tag_transform` use it. Separately,
`vfx_dummy_state__update` builds the bone product for a bound dummy and stores the position (`0x55fe8c`), then calls
`quaternion__to_matrix` on its own orientation into that product (`0x55feb5`..`0x55febc`) where the emitter converts
the product back into its orientation (`quaternion__set_from_matrix` and `quaternion__assign`, `0x56164b`..`0x56165b`).
The retail `.rfx` files were not checked for effects with three or more lights or bone-bound dummies. Found in the
`NOTES-unnamed-vfx-ix-floatmath-ui.md` and `NOTES-unnamed-vsdk-tail.md` notes.

**K297. Font and length swapped.** `gr_get_string_size` takes (w, h, text, length, font). `ui_checkbox__create` and
`ui_radio_button__create` push -1 as the font and the gadget's font as the length (`0x553d0e`..`0x553d1c`,
`0x5548af`..`0x5548bd`), and the font is what they store at `+0x380` for drawing (`0x553dba`). So the gadget width
covers as many characters as the font handle's value, measured in the default font. `ui_inputbox__draw` measures
its text with the default font (`push -1` at `0x553217`); the notes say it draws and processes with the box font. The callers are
`script_editor_create_ui` and the UI table parsers, which nothing loads (V9). Found in the
`NOTES-unnamed-anim-clip-ui-vfxmesh.md` notes.

**K298. The tip is computed twice.** In `vfx_particle_get_billboard_axes` the velocity path builds a vector divided by
15 and scaled by 1.6, by the function's argument at `[esp + 0xc0]` and by `vfx_emitter__get_fixed8_32`, and stores it
in the output (`0x562947`..`0x5629ad`). It then builds it again without the argument factor and stores it over the
first (`0x5629b2`..`0x562a04`). The notes read the argument as the particle size. Which retail emitters use this
path was not checked. Found in the `NOTES-unnamed-vsdk-tail.md` notes.

**K299. Signed bytes, unsigned mapping.** `vfx_unpack_vector_s8` extracts the three high bytes of a face's word with
`sar` and maps each as `b / 127.5 - 1` (`0x564a28`..`0x564a7c`, `[0x582340]` = 1/127.5). `vfx_unpack_normal` uses the
same mapping on unsigned bytes (`shr`, `0x5648c7`..`0x5648fc`), and `vfx_pack_normal` packs `(x + 1) * 127.5`. With
signed bytes every component comes out between -2 and 0. `vfx_mesh_render` averages these face normals for each
vertex, normalises and packs the result and lights the vertex with it (`0x5631c7`..`0x56325a`), unless the mesh is
full-bright (`test al, 0x10` at `0x563193`) or its material value is 254 or more (`0x56319b`). Which retail effect
meshes are lit was not checked. Found in the `NOTES-unnamed-anim-clip-ui-vfxmesh.md` notes (D5108).

**K300. The sound position is set only on some paths.** In single player `spell_cast_execute` ends at
`spell_play_hit_sound(spell, &pos)` (`0x47c9ee`..`0x47c9f4`), which plays the spell's hit sound with
`sound_play_3d_at` (`0x47e072`..`0x47e078`; only spells `0x32`..`0x35` play it flat). `pos` (`[esp + 0x10]`) is written
only by the summon search (`0x47c7c6`), the dragon spells (`0x47c82a`), the target-character case (`0x47c9d9`) and
some other cases. The jump to the sound comes without a write from the default case (black cloud, recall, destroy
undead, wrath of god, burning hands and every spell without a case, `0x47c84b`, table `0x47d348` / `0x47d3a8`), from
curse / Urath curse (`0x47ca5b`), the three meteor storms (`0x47ced2`), banish summon (`0x47cde5`), Titus stone gaze
(`0x47d08c`), and vitalize, protect and faith cast by a character outside the party when there is no current
character or group (`0x47c8c6`, `0x47c8d4`). The slot then holds whatever an earlier call left there. Not checked in
play. Found in the `NOTES-sweep-below-0x4a0000.md` notes (their K301).

**K301. Pending speakers are only checked.** `+Set Important:` / `+Clear Important:` with a quoted name look the speaker
up while parsing (`conversation_find_speaker` at `0x5000dc`) and store the result in the reference (`0x5000e3`). When
the speaker comes later in the file the result is NULL, and the name goes on a pending list
(`0x5000e7`..`0x500152`). At the end the pending list is only checked for existence (`0x500835`..`0x500854`); the
reference keeps NULL, and `conversation_apply_topic_effects` skips NULL speakers when it sets or clears bit 8
(`0x4d807a`..`0x4d80b6`). Bit 8 is `Cflag_Important` (`dialogue_parse_cflags` `0x4fedf1`), the flag that draws the
quest icon over the character (`levelrender_draw_speaker_icons` `0x48588c`). Retail: all 22 such lines name a later
speaker. Three are in `LPalaceInt_v2_dlg.tbl` (Lady Ivane's "How can I help?" sets Evien, Brissa and Dali), but those
three start with `Cflag_important` and nothing clears it; the other 19 are in `Miniquests_dlg.tbl`, which no level
script loads. Found in the `NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

**K302. The key time is replaced by the reciprocal.** In reciprocal mode, which `vif_emitter_update` uses for the
curve at `+0xf4` (`push 0x1` at `0x533187`), `vif_curve__evaluate` builds `1/v0` and then subtracts that from `t`
instead of the key time `t0` (`fld dword ptr [esp + 0x8]; fsub st, st(1)` at `0x533d3c`..`0x533d40`). It returns
`1 / (1/v0 + (t - 1/v0) / (t1 - t0) * (1/v1 - 1/v0))`. The linear mode uses `t - t0` (`0x533d61`..`0x533d65`). A curve with
one key, or one where either key is 0 (`0x533cff`..`0x533d1e`), is not affected. Whether a retail effect file has such a
curve was not checked. Found in the `NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

**K303. A flag that is never cleared and a 256 KB copy.** `cpu_identify` sets a local to 1 (`mov dword ptr [ebp - 0x1c],
1` at `0x5312e2`) and then stores 0 or 1 in `*mmx` from that local, not from the CPUID bit (`0x5313e9`..`0x5313fd`). The
AMD and Intel name strings test the pointer instead of `*mmx` (`cmp dword ptr [ebp + 0xc], 0` at `0x531453`, `0x53157b`, `0x5315bb`).
`PrintSystemInfo` measures RAM speed with copies of `0x10000` dwords (256 KB, `0x53199b`) and prints copies per
second as MB/s (`0x531a29`..`0x531a68`). Only `cmd_sysinfo` calls `PrintSystemInfo` (`0x531af4`), and v1.40 has no
console (see C140). Found in the `NOTES-sweep-0x4a0000.md` notes.

**K310. Red three times.** The OpenGL draw list lights lightmapped faces in two passes. When moving point lights
reach the object (count in the third argument, `[esp + 0x98]`), `levelrender_sum_point_lights` stores each
vertex's light as three floats: red at `+0`, green at `+4`, blue at `+8` of
`levelrender_gl_vert_colors` (`0x24268f8`, 12 bytes per vertex; `0x48fcc7`..`0x48fd0e`). The second pass adds
that colour on top of the lightmap (`gr_mode_level_alpha_add`, `0x491c0a`). For a face with no clipped corner,
the copy into the GL colour array reads the red float three times: `fld dword ptr [ebx + 0x24268f8]` at
`0x491f9e`, `0x491fb5` and `0x491fcd`, each times 255. Byte 0 (red) gets red, byte 1 (green) gets red, byte 2
(blue) gets red, and byte 3 keeps the fog alpha (`0x491f90`). Faces with a clipped corner take their bytes
from the clipper's vertices (`+0x2c`..`+0x2e`, `0x491e06`..`0x491e14`), so they are coloured correctly. A face
can therefore change colour as it crosses the screen edge. The other renderers are right: Glide reads `+0`, `+4`, `+8`
(`0x48ee57`, `0x48ee73`, `0x48ee86`), and Direct3D reads `+8`, `+4`, `+0` into its BGRA bytes (`0x4907cd`..`0x490805`).
The non-lightmapped OpenGL path also reads the three channels (`0x492669`, `0x492694`, `0x4926c4`). The lights
come from `attached_light_slots` (`levelrender_rank_light`), filled by effect light nodes with red, green
and blue from tracks `+0x40`, `+0x4c`, `+0x58` (`vfx_node_create_light` `0x51fbe0`..`0x51fc5b`,
`attached_light_create` `0x5201da`..`0x5201e0`); no retail character table has `+LightColor:` (V13).
Retail: 84 of the 146 light nodes in 42 of the 193 `.rfx` files have channels more than 24 apart. Examples:
`icicle.rfx` (126, 228, 254), `catacombs.rfx` (36, 0, 254), `klab.rfx` (254, about 200, about 60), `lenele1b.rfx`
(254, 204, 96), `fireball.rfx`, `Murod-Death.rfx` (28, 184, 66) and `preload.rfx` (34, 64, 156 and 254, 0, 0).
The files were read with the layout of `vfx_node_relocate` (light count `+6`, array `+0x24`, 0x70 bytes each) and
`vfx_channel__eval`. *Inferred:* with OpenGL, a blue light gives a dim grey glow on the walls, an orange one a
white glow. Characters and effect meshes are lit through `lights_shade_vertices` and are not affected. The
game never selects OpenGL, but SummonerFix always does, so every SummonerFix player sees this.

**Not yet seen in game (2026-10-16).** Four agent-mode before/after captures (Catacombs, Temple of Urath, Icicle and
Ice Coffin cast by Rosalind in the Ikaemos exterior) showed no difference beyond frame noise. A counter on the first
of the three loads (`[Debug] TraceGlLights`, agent command `k310`) explained it: in the Ikaemos tests no face took
this path at all. It runs only when `levelrender_draw_list_opengl`'s third argument, the count of point lights near
the object (`[esp + 0x1c]` of `levelrender_draw_list`, `0x485b6d`), is non-zero, and spell effect lights are not
among them. The level's own point lights are. The fix stays out of SummonerFix until a scene with one is found. The burning village of the Masad tutorial (walked up to the fires) also
gave zero faces on this path, so its fires are effect lights too. *Inferred:* the path may be rare in retail play. A second counter on the entry of `levelrender_draw_list_opengl` (same
setting) narrowed it down. Its point lights are the 24 `attached_light_slots` (`level_light_get` `0x48d180`), filled
by effects with a light node (`vfx_node_create_light`) and by characters with `+LightColor:` (`living_entity_init`
`0x44ecf6`); no retail character table has `+LightColor:`. The Masad fires create none. Rosalind's Icicle and Light
spells do: with Light active in the Ikaemos exterior, 25,618 of 180,140 draw-list calls got 1-3 lights, but still
no face took the unclipped branch, so every lit face there went through the clipped branch, which reads all three
channels correctly. The faulty branch needs a light-node effect next to a level object whose faces are drawn
unclipped; no such scene has been found yet.

## Invisible

| # | Effect | Function | Evidence |
|---|---|---|---|
| V1 | Sets the time of every trigger slot whose name does *not* match | `character_set_skeleton_trigger_time` `0x540d70` | **Observed**; no caller |
| V2 | Only 4 of the 6 large streamed-effect slots are used | `effects_alloc_stream_slot` `0x51c520` | **Observed** code, *inferred* effect |
| V3 | Latent: "Alt+" in a UI table hotkey gives Shift | `ui_parse_hotkey` `0x528430` | **Observed** code |
| V4 | Latent: a TGA with an image ID field is read from the wrong offset | `tga_read_bitmap` (`File__seek` at `0x5556f7`) | **Observed** code |
| V5 | A speed change on one animation fills all 5 speed slots | `character_instance__start_anim` `0x51aee0` | **Observed** code, *inferred* effect |
| V6 | Latent: the ring of small streamed-effect slots restarts at slot 0, 1, 4 or 5 only | `effects_find_stream_slot` `0x51c5c0` | **Observed** code |
| V7 | Latent: the registry helpers read past an "HKEY_..." string without a backslash; delete formats its error with NULL | `os_registry_resolve_key` `0x511e50`, `os_registry_delete` `0x5123e0` | **Observed** code |
| V8 | Freed font slots are not reused, and the kerning search can apply another character's pair | `gr_font_create` `0x508560`, `gr_font_get_char` `0x508440` | **Observed** code and retail data |
| V9 | The UI table loader has no caller; it also ignores a missing file, never sets window mask data and drops several button options | `ui_load_table` `0x52b200`, `ui_window__set_mask_bitmap` `0x52b930`, `ui_parse_button` `0x528530` | **Observed**; no caller |
| V10 | Every line skipped by `SkipToNextLine` is counted twice in parser line numbers | `TextFile__SkipToNextLine` `0x515af0` | **Observed** code |
| V11 | Latent: a negative number with 3k digits is printed as "-,123,456" | `int_to_string_with_commas` `0x5152a0` | **Observed** code |
| V12 | Latent: a hex number at the very end of a table reads past the text | `TextFile__ParseInt` `0x516290`, `TextFile__ParseUInt` `0x516380` | **Observed** code |
| V13 | Latent: the list parsers ignore their maximum count | `TextFile__ParseIntList` `0x516940`, `TextFile__ParseFloatList` `0x516a30`, `TextFile__ParseStringList` `0x5168b0` | **Observed** code and retail data |
| V14 | The 16-to-16-bit TGA converter writes nothing to its output | `tga_convert_pixels` `0x555020` | **Observed**; no caller reaches it |
| V15 | An item placed on a pick layer is always placed again by the general floor probe | `item_obj_spawn` `0x46a1b0` | **Observed** code, *inferred* effect |
| V16 | Two character-instance helpers lose their result | `character_instance__get_anim_root_velocity` `0x51b1e0`, `character_instance__get_bone_bind_inverse` `0x51ae00` | **Observed**; no caller |
| V17 | Latent: the `-glide` and `-pack` command-line options are accepted but never read | `cmdline_read_params` `0x43dc30`, `LoadSettings` `0x43d7f0` | **Observed** code |
| V18 | Latent: `extract_angles` gets the pitch wrong for a pitched matrix whose heading is not a multiple of 90 degrees | `matrix3__extract_angles` `0x510240` | **Observed** code; its only pitch user is unreachable |
| V19 | `matrix43::transpose_in_place` is not a transpose; `skeleton::get_root_rotation` slerps from an uninitialised quaternion | `matrix43__transpose_in_place` `0x511bb0`, `skeleton__get_root_rotation` `0x544010` | **Observed**; no caller |
| V20 | Latent: a mesh file older than version 6 (or one that fails to read) leaks its mesh record | `vim_load_file` `0x5438b0`, `vim_alloc` `0x543040`, `vim_free` `0x5430b0` | **Observed** code and retail data |
| V21 | Latent: a mesh with more than 6 textures overwrites its texture count and flags | `vim_read` `0x543430` | **Observed** code and retail data |
| V22 | The level colour-record reader clamps green into blue and never clamps green | `s3d_read_color_record` `0x470300` | **Observed** code; nothing reads the records |
| V30 | Latent: `level_free_s3d` clears the navpoint and colour-record pointers but keeps their counts | `level_free_s3d` `0x46fdc0` | **Observed** code |
| V31 | Latent: waving level objects add their corner normals into two arrays that are never cleared or read | `levelrender_wave_lobj` `0x48de60` | **Observed** code |
| V40 | Game stage 19 sets `trapped_luminar` on level 14 (KhosaniLab) instead of 22 (IonaExt02), which does nothing | `game_stage_set` `0x4d42e0` | **Observed** code and retail data |
| V41 | Latent: the automap bit functions accept an index up to 8 times too large | `savegame_get_bit` `0x4cff20`, `savegame_set_bit` `0x4cff80` | **Observed** code |
| V50 | The OpenGL texture memory count is wrong, and an updated texture is uploaded twice | `gr_opengl_create_texture` `0x54e840` | **Observed** code |
| V51 | `gr_opengl_setup_view` looks at the forward vector instead of eye + forward and uses a near plane of 0 | `gr_opengl_setup_view` `0x550500` | **Observed**; no caller |
| V60 | Latent: Direct3D reports its video memory in bytes, so the start-up check that lowers texture quality on cards with 8 MB or less never fires | `gr_d3d_init_device` `0x5395c0`, `gr_d3d_get_video_memory_mb` `0x53aaf0` | **Observed** code |
| V61 | Latent: the DirectDraw driver list stops after the first driver, so only the primary display driver's Direct3D devices are ever listed | `d3d_ddraw_enum_callback` `0x5389f0` | **Observed** code |
| V70 | Five hero moves inherit weapon-class bits from a dropped hurt move, so their anims also load for those weapon classes | `characterinfo_parse_table` `0x4452f0` | **Observed** code and retail data |
| V71 | On masad (not masad_v2), Liangshan and JadeTemple, the rider moves that are skipped keep skeleton index 1 instead of -1 | `move_used_on_level` `0x444bb0` | **Observed** code, *inferred* effect |
| V80 | Latent: the `ai_state` console command rejects every state name ("Error: Invalid state name") | `cmd_ai_state` `0x474d10` | **Observed** code |
| V81 | Latent: the `ai_frame_update_rate`, `attack_point` and `offense_point` console variables are stored and never read; `attack_point` also accepts an index one past the target's last attack point | `cmd_ai_frame_update_rate` `0x4720e0`, `cmd_attack_point` `0x473260`, `cmd_offense_point` `0x473300` | **Observed** code |
| V82 | The branch of `ai_knock_down_local` for a character that is already down can never run; if it did, it would restart the lying phase with the first knock-down's duration | `ai_knock_down_local` `0x4742b0`, `ai_can_be_knocked_down` `0x475030` | **Observed** code |
| V90 | `LoadSettings` passes the vsync option as `gr_init`'s far-z argument (0.0 or 1.0) and a constant 0 as its vsync argument | `LoadSettings` `0x43d7f0`, `gr_init` `0x505e00` | **Observed** code, *inferred* intent |
| V100 | Several spell handlers store an `amount` that the dice roll overwrites; Laharah's scream's 10-unit test only guards that store | `living_entity_get_hit_by_spell` `0x45dea0` | **Observed** code and retail data |
| V101 | Jade Beam's boss branch tests for fireball (`0x1a`), which it can never be, and the other branch prints "jade dmg: %d" with a double | `living_entity_get_hit_by_spell` `0x45dea0` | **Observed** code |
| V102 | `character_walk_to_point` normalises the direction and never uses it; its "already facing" test compares a raw dot product with 1.0 | `character_walk_to_point` `0x455e70` | **Observed** code |
| V110 | Latent: allocating a script avoid spline tests "already in use" twice with the same handle and NULL-tests the target's handle instead of the target character | `script_alloc_avoid_spline` `0x4cedf0` | **Observed** code |
| V111 | The script editor's level restart respawns level items of other script variants (navpoint never looked up) and items already taken, and frees the avoid splines without clearing their slots | `level_script_restart` `0x4cb060` | **Observed** code, *inferred* effect |
| V112 | Latent: the level-scaling code reads `cur_character`'s level before testing `cur_character` for NULL | `level_script_init` `0x4cc7e0` | **Observed** code |
| V120 | The "Fog_near = %.2f, Fog_far = %.2f" debug text in the game view can never be drawn: its timer is started only inside the branch that already needs it running, and the flag that starts it is never set | `game_render_frame` `0x438dc0`, `levelrender_get_flag_22fb6f7` `0x486be0` | **Observed** code |
| V121 | Latent: the `character` console command with no argument writes a zero one byte before its name buffer, then fails the lookup | `cmd_character` `0x439930` | **Observed** code |
| V122 | Latent: the `Dump_sound_usage` and `Give_all_items` console toggles are stored and never read | `cmd_dump_sound_usage` `0x4378f0`, `cmd_Give_all_items` `0x438070` | **Observed** code |
| V130 | The catacombs' "cryptintro" cutscene can never play: its guard byte is never written, the flag test is inverted, and cutscene.tbl has no such cutscene | `ls_catacombs_frame` `0x4fe1c0` | **Observed** code and retail data |
| V131 | The "Charge" dialogue action clears `wolong2_charge`, which `ls_wolong2_init` sets on every Wolong2 entry; nothing in Sum.exe or the retail tables reads the flag | `ls_wolong2_force_alert_action` `0x4f1eb0`, `ls_wolong2_init` `0x4f2060` | **Observed** code and retail data |
| V140 | Latent: a script object's trigger name is copied with `strcpy` into a 32-byte field; a "+No create" object copies it from a stack buffer that is never filled | `script_parse_objects` `0x4dba30` | **Observed** code and retail data |
| V141 | Latent: a `.vcs` console script path is cut at the first `.` of the whole path, so a `.` in the install directory breaks every script; a `/* ... */` comment on one line comments out the lines after it | `load_script` `0x50acd0` | **Observed** code |
| V142 | Latent: the console's `debug` trace prints an integer argument with `%f` | `process_command` `0x50a5b0` | **Observed** code |
| V143 | Latent: a console command whose name is already registered, or the 301st, is dropped without a message and its object keeps NULL fields | `console_command__console_command` `0x50a180` | **Observed** code and retail data |
| V150 | Latent: the Glide texture cache's in-place reload never runs (no code sets its per-TMU dirty bytes); it would also copy unconverted texels from the start of the bitmap | `gr_glide_tcache_set` `0x549560` | **Observed** code |
| V151 | Joseph's "We're not ready to confront Machival yet." in Lenele (lenele3a) can never be said: the use hook tests the name "templeint", the trigger is "templeint2", and in single player the palace exit on the same spline fires first anyway | `ls_lenele3a_use` `0x4f1030`, `ls_lenele3a_init` `0x4f0f70` | **Observed** code and retail data |
| V152 | In the Lenele slums every level-script dialogue action other than Gorbus's own "CheckReleaseOrder" sets his "Give him the release order." topic to the opposite of the party holding the Forged Document; his Hail resets it, so the player sees no difference | `ls_lenele1b_action` `0x4fb160` | **Observed** code and retail data, *inferred* effect |
| V153 | With the Glide renderer (a 3dfx card or a Glide wrapper), a texture marked dirty (texture quality change, save thumbnail) of an animated bitmap is built twice: the rebuild stores the frame number as the owner, so the next use rebuilds it again | `gr_glide_mark_texture_dirty` `0x5496e0`, `gr_glide_tcache_set` `0x549560` | **Observed** code |
| V170 | Opening a full-screen screen from a side panel with no character to show runs the side panel's leave function twice | `interface_set_screen` `0x412620` | **Observed** code, *inferred* trigger |
| V171 | `packfile_read_whole_file` opens the `.vpp` by its bare name (not under the install path) and reports success for a CD packfile without reading it | `packfile_read_whole_file` `0x534900` | **Observed**; no caller |
| V180 | The main window's thread priority is never changed on activation: `SetThreadPriority` gets a thread handle that nothing sets; the previous-instance window search runs at every start and its result is never read | `os_wndproc` `0x530ff0`, `os_find_previous_instance` `0x530e40` | **Observed** code |
| V181 | Latent: in multiplayer, a repeated connection request opens a second link for the same machine, which holds a slot for 100 s | `nw_work_reliable` `0x517e00` | **Observed** code, *inferred* effect |
| V182 | Latent: in multiplayer, IAMHERE packets send four uninitialised stack bytes as their send time and ACKs two as their sequence number | `nw_send_heartbeat` `0x517d70`, `nw_send_reliable_reply` `0x517d20` | **Observed** code |
| V183 | Latent: the level section name is lowered only while the index is below the character's code, and a duplicate name leaks its node and its copy in the pool | `localize_load_level_table` `0x504870`, `localize_table_add` `0x5040e0` | **Observed** code and retail data |
| V184 | The clipboard reader leaves the clipboard open when there is no text on it, and needs one byte more than its size argument | `os_clipboard_get_text` `0x531b44` | **Observed**; no caller |
| V190 | Every Print Screen leaks one 56-byte list node for each screenshot file already in the folder | `screenshot_save` `0x4324f0` | **Observed** code | No |
| V191 | Latent: the third button of a script editor message box calls the second button's callback | `editor_dialog_handle_event` `0x4315c0` | **Observed** code | No |
| V192 | Latent: the debug Backspace key flush in `PollInputEvents` tests a key code that can never arrive | `PollInputEvents` `0x432710` | **Observed** code | No |
| V193 | At the end of every cutscene, `cutscene_unload` passes the soundtrack's sound handle to `snd_stop`, which takes a play signature, and frees a slot that `snd_level_release` has already freed | `cutscene_unload` `0x4351e0` | **Observed** code | No |
| V200 | Latent: `load_mlayout` and `save_mlayout` with no file name clear the top byte of the console code's saved EDI register before trying the file `.mlo` | `cmd_load_mlayout` `0x4401a0`, `cmd_save_mlayout` `0x4402f0` | **Observed** code |
| V201 | Latent: in multiplayer, the host accepts a reliable link from a machine that is not in its player list and neither stores nor closes it, so the link keeps one of the nine slots until it times out | `mp_server_accept_connections` `0x440d20` | **Observed** code, *inferred* effect |
| V210 | Nothing known: when a group member dies or leaves, its group's enemy count becomes -1 until the group's next think (up to 2.5 s) | `ai_group_remove_member` `0x492ca0` | **Observed** code | No |
| V211 | Nothing: a rider flinch move with an unknown `stage<N>_flinch_*` action formats an error message that is never shown (the Phoenix's `stage1_flinch_high` and `stage1_flinch_mid` do so on every load), and the per-stage flinch tables are never read | `serpent_rider_set_flinch` `0x4954c0`, `phoenix_rider_set_flinch` `0x498720`, `tiger_rider_set_flinch` `0x49a1a0` | **Observed** code and retail data | No |
| V212 | Nothing: the Phoenix Rider's on-foot setup looks up `ready_hurt` and throws the result away (its `ready_hurt` uses the same `phnx_ready.mvf` as `ready`) | `phoenix_set_foot_anims` `0x499be0` | **Observed** code and retail data | No |
| V213 | Nothing known: unloading the Tiger Rider frees its axe without clearing `tiger_axe`, and every Tiger load makes a new axe without freeing the one not yet handed over | `tiger_unload` `0x49ae90`, `tiger_equip_weapon` `0x49a2c0` | **Observed** code, *inferred* effect | No |
| V214 | Nothing: the Serpent's first load clears its "rider seated" flag right after setting it; the nested loads of its two copies set it again | `serpent_boss_load` `0x495960` | **Observed** code | No |
| V230 | Latent: with all five goal slots kept (flag `0x1`), a new goal of the party leader sets bit `0x100` in `ai_state +0x5c`, the z of the travel point (`goals[-1].flags`) | `ai_goal_alloc` `0x476020` | **Observed** code |
| V220 | When every attack point of a target is taken, an approaching AI character stores its handle into the target's fifth attacker-slot timer (`attack_point_users[-1]`) | `ai_approach_target_point` `0x4bcff0` | **Observed** code | No |
| V221 | Deleting a navpoint in the script editor copies the record after the last one into the last slot before the count drops; with a full table that reads past `navpoints[]` | `script_editor_on_delete` `0x4e6ad0` | **Observed** code | No |
| V222 | Each OK in the script editor's ambient window with a new sound or new parameters takes a further `sounds[]` slot (640 in all) and never releases the old one | `script_editor_ambient_ok` `0x4eb3d0` | **Observed** code, *inferred* effect | No |
| V223 | Latent: for two segments that share an end point (the first segment's end equal to the second's start), the segment test reports the far end of the second segment as the meeting point; only the cell-crossing search uses that point | `seg2d_segment_intersect` `0x4a5750` | **Observed** code | No |
| V224 | Latent: two avoidance helpers read the segment before a list's first segment (the list header) when a corner is on no segment, or when the previous segment does not end at the corner | `avoid_seglist_ensure_corner_offset` `0x4a82a0`, `avoid_merge_shared_segments` `0x4a8b80` | **Observed** code | No |
| V250 | In Titus's stage 3, when the stage timer runs out with a paralysed target and no unparalysed candidate, Titus's target becomes -1; he idles for one think and then picks again | `titus_ai_select_action` `0x4c2790` | **Observed** code |
| V251 | Latent: as V211, a Giant Salamanka flinch move whose `$Action:` is not `stage<N>_flinch_area/status/left/right/front` formats an error that is never shown; a name that does not start with `stage<N>_flinch_` indexes the tables with a wrong stage | `giant_salamanka_set_flinch` `0x4c8280` | **Observed** code and retail data |
| V252 | Every failed `File::find` with an explicit path id runs its fallback search with an uninitialised extension buffer and may leave a wrong path in the File object; `File::open` searches again and overwrites it | `File__find` `0x5141c0` | **Observed** code |
| V253 | Latent: a memory-mode `FileReader` cannot seek to 0 or to the end, and origin 2 counts back from the current position | `FileReader__seek` `0x513a90` | **Observed** code |
| V254 | Latent: an extension of 8 or more characters overruns the 8-byte last-extension buffer onto `File::find`'s " " constant and the "wt" / "wb" mode strings | `File__open` `0x514630` | **Observed** code |
| V255 | Latent: adding a data directory a second time never adds its new extensions | `file_add_path` `0x512e60` | **Observed** code and data |
| V256 | Four File helpers with no caller are wrong: `read_matrix43` writes `m[i][3]` of a 4x3 matrix, `find_on_cd` passes the name pointer's address as the packfile out pointer, `file_list_files` splices its own stack list head into the caller's list on no match, `read_String8` copies without bound into a 256-byte stack buffer | `File__read_matrix43` `0x525ba0`, `File__find_on_cd` `0x513f70`, `file_list_files` `0x513590`, `File__read_String8` `0x525ce0` | **Observed**; no caller |
| V257 | Latent (UI table loader only, V9): `set_tooltip` unlinks the old tooltip without freeing it (0x120 bytes), and every button created inside a container gets the auto-repeat flag | `ui_button__set_tooltip` `0x5271c0`, `ui_static__set_tooltip` `0x527fa0`, `ui_button__create_in_parent` `0x527100` | **Observed**; no caller outside `ui_load_table` |
| V260 | Latent: segment sound emitters would get a pan far outside -1..1 (hard left or right); nothing creates a segment emitter | `sound_emitter_calc_pan` `0x521270`, `sound_emitter_create_segment` `0x546670` | **Observed**; no caller |
| V261 | Latent: a sound whose minimum range is 0 or less never matches its own slot and takes a new one of the 640 on every lookup; a sound name of 256 or more characters overflows the stack before the length test | `snd_get_handle` `0x5207d0` | **Observed** code and retail data |
| V262 | With EAX on, every 3D sound buffer played in 2D keeps one reference that is never released, so its duplicate buffer is never freed; a failed `QueryInterface` would be used anyway | `eax_buffer_init` `0x521fd0`, `ds_play` `0x55d1cd` | **Observed** code, *inferred* effect |
| V263 | Three light and effect helpers with no caller are wrong: `lights_debug_draw` draws the first `light_count` records, not the used ones; `lights_calc_point_specular_color` reads a NULL normal inside the inner radius; `vfx_all_resident` is unused | `lights_debug_draw` `0x51fa70`, `lights_calc_point_specular_color` `0x51f560`, `vfx_all_resident` `0x51dcb0` | **Observed**; no caller |
| V264 | In the script editor, every list entry given a font is re-wrapped, and its list box's sizes and scroll bars recomputed, in every frame's process and draw: the dirty flag is never cleared | `ui_listbox__relayout_dirty` `0x52cf10`, `ui_listbox_entry__set_font` `0x52d5a0` | **Observed** code |
| V265 | Nothing in v1.40: six list box and UI manager slips are reached only through the UI table loader (V9) or have no effect because no gadget has a tooltip and no UI sound is loaded | `ui_listbox__set_tooltip` `0x52d7e0`, `ui_parse_listbox` `0x5298c0`, `ui_state_bitmap__draw` `0x52c9a0`, `ui_listbox__remove_entry` `0x52dd00`, `ui_listbox__set_vscroll_fraction` `0x52ed70`, `ui_process` `0x530330`, `ui_listbox__process` `0x52d080` | **Observed**; no reachable trigger |
| V270 | Nothing in v1.40: the `render_level`, `render_characters` and `render_effects` console toggles change flags that nothing reads. No console in v1.40: only a patch or a `.vcs` script can run them | `cmd_render_level` `0x481890`, `cmd_render_characters` `0x481980`, `cmd_render_effects` `0x481a70` | **Observed** code | No |
| V271 | Nothing: three render flags are read but never set, so the frame-rate fog scaling, a "no lighting" mode (no level lights, distance culling of level objects, no lamp glows, LOD 2) and a level-light rebuild are dead code | `levelrender_fog_enable` `0x482850`, `fog_get_near_current` `0x486bf0`, `fog_get_far_current` `0x486c20`, `levelrender_render_scene` `0x482200` | **Observed** code | No |
| V272 | Nothing: sixteen 64 x 64 blob shadow bitmaps are built at start-up and never used | `shadow_create_blob_bitmap` `0x489f10` | **Observed** code | No |
| V290 | Latent: two functions with no caller have slips: the oriented-box test multiplies one term by the wrong extent, and the Direct3D particle path culls the top 100 pixels of the screen | `ix_box_oriented_box_oriented` `0x53c610`, `vfx_emitter_instance__render_d3d` `0x561900` | **Observed** code | No |
| V300 | Latent: a polygon colour tint whose red and green factors differ would tint red with the green factor and leave green untinted; every tint the game sets is grey | `gr_poly_rgb` `0x545610` | **Observed** code | No |
| V301 | Latent: Direct3D 8-bit alpha textures read past the bitmap when it is smaller than its texture, and every section of a multi-section alpha bitmap gets the top-left texels | `gr_d3d_copy_bitmap_to_surface` `0x54c0a0` | **Observed** code | No |
| V302 | Nothing: `key_init` flushes the key queue before the keyboard is marked ready, so the flush returns at once; `key_putkey` (no caller) does not check for a full queue | `key_init` `0x509770`, `key_putkey` `0x509d40` | **Observed** code | No |

**V1.** The `strcmp` result decides the store the wrong way round (`je 0x540dde` at `0x540dbe` skips a
match). Nothing calls the function, so it never runs.

**V2, V6. Mask 5 instead of a modulo.** `effects_alloc_stream_slot` indexes its 6-slot ring with
`and eax, 5` (`0x51c533`), which only gives 0, 1, 4 and 5. Slots 2 and 3 are never used, so large
streamed effects are evicted sooner than intended. `effects_find_stream_slot` scans its 8 slots with
`and eax, 7` but stores the next start with `and edx, 5` (`0x51c613`), so all 8 slots are still used.

**V3.** "Alt" sets the same flag as "Shift" (`mov bl, 1` at `0x5284a1` and `0x528481`). Only `ui_load_table`
uses this parser, and nothing calls it (V9).

**V4.** With a non-zero ID length, the reader seeks to that length from the start of the file (origin 0)
instead of past the 18-byte header. A seek is only made when the length is non-zero (`jbe` at `0x5556e4`).
*Inferred:* the retail TGAs have no ID field.

**V5.** With a speed other than 1.0, the loop at `0x51af53`..`0x51af65` stores `{anim, speed}` in every free
one of the 5 slots, with no break after the first.

**V7.** After the root name, `os_registry_resolve_key` reads `[esi+1]` (`0x511eb4`) whether `esi` stopped on the
`\` or on the terminator. The root-name copy into the 256-byte stack buffer has no length check
(`0x511e9b`..`0x511ea8`). `os_registry_delete` with `name == NULL` formats "Error removing registry key '%s'"
with that NULL name. No caller passes an "hkey..." string (the only such literals are `os_registry_hkey_from_string`'s
own, `0x5a7554`..), and `os_registry_delete` has no caller.

**V8.** `gr_font_create` remembers the first free slot but loads into slot `gr_num_fonts`. `gr_close_fonts`
(`0x5081d0`, only from `gr_close`) does not reset that count. The game loads its three fonts once in `GameInit`
(`0x43df76`..`0x43df9e`), so no slot is ever freed and reloaded. After the kern loop, `gr_font_get_char` compares
only `ch1` (`cmp edx, edi` at `0x5084d9`), not `ch0`. When the loop runs off the character's pairs, it can apply
the next character's pair. If it runs off the end of the table, it reads one record past it. None of the 7 retail
fonts has any kern pairs (`num_kern_pairs` 0), so this never runs.

**V9.** `ui_load_table` has no call and no pointer in the image (we searched the bytes for `0x52b200`). Every
`ui_parse_*` function, `ui_parse_hotkey` (V3) and `ui_window__load_mask_bitmap` (`0x52b42a`) is reached only from
it. The defects inside it are therefore dead code:
- a missing file only formats a message (`0x52b25e`);
- without "+Name:" it copies an uninitialised stack buffer (`0x52b3a3`..`0x52b3ca`);
- `set_mask_bitmap` never writes `mask_data` (`+0x4d8` is written only by the constructor, `0x52b07c`);
- `ui_parse_button` reuses the previous target for an unknown state name.

`ui_window__init` (live, used by the script editor and two debug states) calls `get_flags` in both list walks and
ignores the result (`0x52b1a1`..`0x52b1d3`). Every window is appended last, so a modeless window created while a
modal one exists goes after it. No retail path creates such a pair.

**V10.** The loop stops on the CR (`cmp byte [eax], 0xd`). It counts the line (`0x515b13`) and then tests the
same CR for LF (`cmp dl, 0xa` at `0x515b19`), so the cursor stays on the CR. The next `SkipWhitespace` counts the
CR again. Only `localize_load_level_table` calls it (`0x504a37`, `0x504bb9`), and the following `Accept` skips the
CR, so only the line numbers in parse error messages change. Live (every level string table), invisible.

**V11.** The digits before the first comma are `(len - neg) % 3 + (neg != 0)`, and 3 only when the sum is 0
(`0x515331`..`0x515344`). For -123456 this gives "-", then ",123,456". The size check counts
`(len - neg - 1) / 3` commas (`0x5152e3`..`0x5152ef`), one too few for that case. Callers are `status_draw_level`
and `ring_format_info`, which pass experience values, never negative.

**V12.** The hex loop tests each character with `strchr("0123456789abcdefABCDEF", c)` (`0x5162d4`..`0x5162e3`).
`strchr` also finds the terminator, so a NUL is taken as a digit ('A'-'F' branch, `0x516314`) and the cursor goes
past the end. It needs a `0x` number as the last bytes of a file (retail tables end with `#End`).

**V13.** The loops end only on `)` or NUL (`0x5169fe`..`0x516a02`, `0x516a83`..`0x516a8a`). The max-count
argument (8 for `+RenderMask:` at `0x44b13d`, 3 for `+LightColor:` at `0x44659b`) is never read. Retail: the
longest `+RenderMask:` list has 5 parts, and no character table has `+LightColor:`. `ParseStringList` has no caller.

**V14.** For 16-bit to 16-bit, `tga_convert_pixels` toggles bit 15 of the *source* in place (`xor word [eax],
0x8000` at `0x555042`), and the caller still advances by the returned size. Its only callers are
`tga_rle_encode_row` from `gr_screenshot`, which always asks for 3-byte output (`0x5063fa`), and the
unreferenced `tga_write_rle` (`0x555970`).

**V15.** The layer loop over all level objects has no break, so `i >= num_lobjs` is always true after it, and
`layer` becomes -1 (`0x46a52d`..`0x46a535`). The general `lobj_raycast_all` probe (`0x46a5d8`) then places the item
again from 3 units above the layer floor. It was probably meant to run only when the layer probe found nothing.
*Inferred:* the result differs only where another surface lies within 3 units above the layer floor.

**V16.** `get_anim_root_velocity` stores zero for an action animation and then overwrites it unconditionally
(`0x51b1fc` jumps only past the zero, `0x51b21a`..). `get_anim_root_speed` (`0x51b250`) does return 0 there.
`get_bone_bind_inverse` inverts a temporary copy and drops it. Neither function has a caller.

**V17.** `cmdline_read_params` stores whether `-pack` was found at `0x60ad70` (`0x43dc8b`), and nothing reads that
byte. The `-glide` object (`0x60ad50`) is only referenced by its constructor and destructor (`0x43d37c`, `0x43d3a0`).
Glide is the default renderer, but a Direct3D choice from the registry (`0x43dade`..`0x43daf0`) is not overridden by
`-glide`; only `-direct3d` is tested (`0x43daf5`..`0x43db03`). The launcher passes typed options (`CmdLineOptions`),
so a player could give them. The two bytes `0x60ad6d`/`0x60ad6e` that `LoadSettings` tests (`0x43d95b`, `0x43d968`)
are never written either (removed options, no effect).

**V18.** `set_from_sincos` builds `fvec = (cos p sin h, -sin p, cos p cos h)` (`0x510086`, `0x5100b0`, `0x5100a7`).
`extract_angles` estimates cos p as `sin h x fvec.x` or `cos h x fvec.z` (`fmul` at `0x5102b2` / `0x5102bd`), where a
divide is needed. That gives cos p x sin^2 h, so tan(pitch) comes out up to twice too large. Heading and bank are
right. Of the 12 call sites, 11 use only the heading: `character_save_state` (`0x4572b8`), `character_ai_move_frame`
(`0x45c688`), `camera_sync_yaw_to_target` (`0x4798d7`), `mp_send_obj_update` (`0x480584`), `mp_handle_obj_update`
(rebuilds with pitch 0 and bank 0, `0x48083e`), `level_add_navpoint` (`0x4d57a6`), `script_parse_navpoint(s)`
(`0x4db553`, `0x4db8ab`), `ls_worldmap1_frame` (`0x4fbdbb`) and `build_bone_matrices` (`0x51a05f`). The one that uses
the pitch, `camera_blend_sequence__advance` (`0x4944b9`, `0x4944d0`, then `set_from_angles` at `0x4945f2`), runs only
for a camera sequence in state 1 (`0x4794bb`..`0x4794ce`). The only sequence class Sum.exe builds is the cutscene shot,
with state 0 (`push 0` at `0x493f83`); the blend class that owns this method has no constructor in the image, so that
branch is dead.

**V19.** `transpose_in_place` moves `m[1] -> m[4] -> m[5] -> m[9] -> m[3] -> m[1]` and
`m[2] -> m[8] -> m[10] -> m[7] -> m[6] -> m[2]` (`0x511bb1`..`0x511c02`). A transpose would swap 1/3, 2/6 and 5/7, and
`m[9]`, `m[10]` belong to the position. `get_root_rotation` passes a default-constructed quaternion (`0x544051`, no
initialisation) as the slerp start (`quaternion_slerp` at `0x54405c`). Neither function has a call, and their addresses
do not occur in the image.

**V20.** `vim_load_file` returns 0 for a version below 6 (`0x5438e8`) and for a failed `vim_read`
(`0x543908`..`0x54390d`) without freeing the record from `vim_alloc`. `vim_alloc` writes `+0x2c` even when `new`
returned NULL (`0x54307c`..`0x543086`), and `vim_free` frees nothing when the data pointer is NULL (`0x5430bc`).
Retail: 820 of the 825 meshes are version 6. The other five (GHorse1.vim version 2; Pijan.vim, Pijan_shadow.vim,
prop-minosickle_shadow.vim and prop-sword-bastard_shadow.vim version 3) are never loaded. Characters load the mesh
named after their table (Horse1 uses Horse1.vim, version 6; `charanim.tbl`'s "GHorse1.v3d" is not read, since Sum.exe
has no "+State:" literal). No table defines a Pijan character. Items page only the "prop-*.v3d" model
(`item_page_in_mesh` `0x46c7e2`), and only characters get the "_shadow" suffix (`character_load_shadow` `0x540e60`).

**V21.** The texture loop (`0x543835`..`0x543896`) stores each `bm_load` handle at `+0x10 + 4i` and re-reads the
count at `+0x28` on every pass (`0x54388a`). With 6 slots, texture 7 overwrites the count and texture 8 the flags at
`+0x2c`. The largest retail count is exactly 6 (Flece, Heavy Flece, Rosalind, their shadows, Prop-Legs-Leather02).

**V22.** The third clamp loads green (`fld [ebp]`, `+4`, at `0x4703bf`), clamps it to the constant at `0x5819f8`
and stores the result into blue (`fstp [ebx]`, `+8`, at `0x4703f2`). `level_load_s3d` reads the records into level
`+0x5c` (count `+0x58`, `0x46e6f5`..`0x46e70d`). There is no other reference to `0xa5a2f8` / `0xa5a2fc`, and
`level_free_s3d` only clears the pointer (`0x46fed0`), so nothing reads the colours. Found in the `s3d.cpp` notes.

**V30.** `level_free_s3d` stores 0 at `+0x3c` (`level_navpoints`, `0x46fea9`) and `+0x5c` (colour records,
`0x46fed0`) but not at `+0x38` (`level_num_navpoints`) or `+0x58`. `level_marker_find` (`0x4774ac`) and
`script_editor_draw_navpoints` (`0x4e3678`) loop the count over the pointer, so a call between `level_unload`
(only caller `state_game_leave`) and the next load would read through NULL. Nothing calls them there, and the
next `s3d_read_header` sets the counts again. Found in the `s3d.cpp` notes.

**V31.** For each face of an object with draw flag `0x20`, `levelrender_wave_lobj` adds the three corner normals
into `levelrender_wave_vertex_normals` (`0x2396ca8`) and 1.0 into `levelrender_wave_vertex_face_count`
(`0x238a170`) (`0x48e342`..`0x48e3d7`), then normalises every one of the 12000 entries with a non-zero count
(`0x48e3fa`..`0x48e426`). No other code reads or clears either array (no other reference to `0x238a1xx` or
`0x2396cxx` past `0x2396ca0`), so the work is wasted and the counts only grow. Whether a retail level object has
flag `0x20` was not checked, so this is counted as latent. Found in the `levelrender.cpp` notes.

**V40. A copied level number.** Stage 19 reads `trapped_luminar` on level 22 (`push 0x16` at `0x4d4343`). If it is set
and `trapped_titus` is not, it sets `trapped_luminar` again, on level 14 (`push 0xe` at `0x4d437a`). In `events.tbl`
the flag is only in `$Level: "ionaext02"` (level 22); `khosanilab` (level 14) has `got_ring_of_fire`. Stage 6 has the
same "if not A set B, else if not B set A" shape on levels 14 and 12, so the 14 was probably copied from there.
`savegame_set_flag` ignores a name that is not in the level's list (`0x4cfd90`), and the branch runs only when level
22's flag is already set, so nothing changes.

**V41. Bound in bytes, index in bits.** Both functions load the level's automap bit count (`[esi+0x28]`, layers x
4096) and compare the index against it shifted left by 3 (`shl edi, 3` at `0x4cff44`; `shl edx, 3` at `0x4cffae` in `savegame_set_bit`).
An index past the real count reaches the level's int and float variables and the next levels' blocks. The callers
`automap_is_cell_revealed` (`0x4300c0`) and `automap_reveal_cell` (`0x430110`) check the layer but not x and y,
so a cell outside 0..63 would get through. We know of no caller that passes one.

**V50.** The byte count is `2 x w x h` of the last level the upload loop processed (`imul esi, ebp; shl esi` at
`0x54ece0`..`0x54eced`), so a mipmapped texture counts 2 bytes (its 1x1 level) and a 32-bit one half its size. It feeds
`gl_texture_mem_frame` and `gl_texture_mem_used` (`0x54ecfc`..`0x54ed08`), which `gr_opengl_tcache_flush` only prints into
`string_buffer`. With `update` set, level 0 goes up with `glTexSubImage2D` (`call [0x57e30c]` at `0x54ebd0`) and then again
with `glTexImage2D` (`call [0x57e31c]` at `0x54ec2b`), because the sub-image branch jumps to the common upload
(`jmp 0x54ec11` at `0x54ebd6`). Only the time is lost.

**V51.** `gluLookAt` gets `gr_eye_pos` as the eye and `gr_eye_matrix.fvec` (`0x26927d8`..`0x26927e0`) as the centre, not
eye + fvec (`0x55051a`..`0x550586`), and `gluPerspective` gets near 0 (`push 0; push 0` at `0x5505a8`). Neither the address
nor a call to it occurs in the image.

**V60. Bytes, not MB.** `gr_d3d_init_device` asks `IDirectDraw7::GetAvailableVidMem` (vtable `+0x5c`, `0x539fa6`) for
texture memory (`DDSCAPS_TEXTURE`, `0x1000` at `0x539f75`) and stores the free amount, in bytes, into
`gr_d3d_video_memory_mb` (`0x53a04d`). `gr_d3d_get_video_memory_mb` returns it unchanged (`0x53aaf0`), and so does
`gr_get_video_memory_mb` (`jmp 0x53aaf0` at `0x507a0e`). `summoner_cfg_load` compares the result with 8
(`cmp eax, 8; jg` at `0x43e371`) when `summoner.cfg` is missing or short (K64), so with Direct3D the test that is
meant for 8 MB cards is false on any card the game accepts (it needs 5 MB free, see "Checked, not bugs"). Glide
returns its small value (probably MB). On any card of more than 8 MB the MB test is false too, so this only matters on 1999 hardware.
Direct3D only. Found in the `NOTES-gr-direct3d.md` notes.

**V61. `DDENUMRET_CANCEL` after the first driver.** `d3d_ddraw_enum_callback` copies the GUID and names into the
next of up to several driver entries and increments `d3d_num_ddraw_devices` (`0x538a02`), but returns 0
(`xor eax, eax` at `0x538a58`, `ret 0x10` at `0x538a61`), which is `DDENUMRET_CANCEL`. `DirectDrawEnumerateA`
(`0x538867`) calls it first for the primary display driver, so the loop in `d3d_enumerate_ddraw_devices`
(`0x538884`..`0x5388f6`) always sees one driver. Of its devices, those with hardware transform and lighting are
skipped (`test [ecx], 0x10000` at `0x53892c`, `D3DDEVCAPS_HWTRANSFORMANDLIGHT`), and the last one listed is used
(`lea eax, [8*edx + 0x2dc2568]` at `0x538914`, the entry before `d3d_num_3d_devices`). `DirectDrawEnumerate`
without `Ex` lists only the primary driver and 3D-only add-in cards, and the 3dfx ones among those are refused in
Direct3D mode anyway (`0x539e73`..`0x539ec4`). With the enumeration continuing, a Voodoo 1 or 2 would be the last
device and the game would quit with message 943, so the early stop has no bad effect we know of. Direct3D only.
Found in the `NOTES-gr-direct3d.md` notes.

**V70. Bits ORed into a recycled record.** `+Animation class:` does `or ebx, edx` into `move_info +0xc`
(`0x44618d`..`0x4461a4`) and never clears it; only a move without the line gets a fresh 1 (`0x4461b2`). The
hurt-move drop frees the record at once (`0x44635f`), and the next `+Move:` gets the same record back from
the free list. Replaying the load order over the retail tables gives five such moves:

- Joseph's staff stand_to_ready (`josp_stv_stand_to_ready.mvf`) also gets pole arms;
- Rosalind's staff trip (`rosl_trip.mvf`) also gets swords, bows, crossbows and daggers;
- Rosalind's sword ready (`rosl_swd_ready.mvf`) also gets bows and crossbows;
- two Flece hurt transitions get bows, but they are dropped themselves.

In each case a later move of the same variation with the right class is nearer the list head, so
`charinfo_find_move` and `charinfo_find_anim` still pick the right anim. The trip and stand moves are not
attack variations 1-5, so `charinfo_collect_moves` does not see them. What remains is that
`characterinfo_load_party_anims` loads the extra anim into the hero's pool for those classes (the class test at
`0x4475fd`..`0x447601`).

**V71. Store before the test.** On these levels `move_used_on_level` writes `m->anim = 1`
(`mov dword ptr [esi+0x8], 0x1` at `0x444db3` and `0x444fb8`) before its anim-name tests. When a test
then skips the move (serp_, ghrd_, tigr_ and phnx_ stage anims), `charinfo_load_move_anim` returns without
loading, and the move keeps 1. The earlier skip tests (low parry and flinch, non-combat flinch, towns) return
before the store and keep -1. *Inferred:* the rider records are not heroes, so the loaded-only lookups ignore
`anim` for them. A lookup by skeleton index (`charinfo_find_move_any`, used by the level script actions) could
return a skipped move for skeleton 1; this was not traced.

**V80. Pointer compare instead of a string compare.** After `process_command(2)` the loop compares each
entry of `ai_state_names` (`0x59ad60`, 13 pointers into .data, `0x59b404`..`0x59b4e8`) with the address of the
`next_arg` buffer: `cmp dword ptr [eax], 0x25dacec` (`0x474d4f`). Other commands pass the same address to string
functions (`push 0x25dacec` in `cmd_play_cutscene` `0x437513`), so `next_arg` is a char array, and no table
pointer can equal it. The loop always ends with 13, and the command prints "Error: Invalid state name" and returns
(`0x474d62`..`0x474d77`). The help text lists the names correctly. A developer console command, so no retail path
reaches it.

**V81. Variables without a reader.** `tools/asm.mjs grep` finds `ai_frame_update_rate` (`0x59af5c`) only in
`cmd_ai_frame_update_rate` (`0x472102`, `0x472121`), `Ai_attack_point` (`0x59af60`) only in `cmd_attack_point`
(`0x4732b0`, `0x4732b7`) and `Ai_offense_point` (`0x59af64`) only in `cmd_offense_point` (`0x473318`). The help text
of `ai_frame_update_rate` says it sets the update rate for off-screen creatures, but the throttle in
`ai_update_perception` uses a 500-750 ms timer (`0x47301d`). `cmd_attack_point` stores the index and resets it to
-1 only when it is greater than the target's point count (`cmp eax, edx; ... jle` at `0x4732ae`..`0x4732b5`), so
an index equal to the count is kept. Nothing reads it, so this has no effect. Developer console commands only.

**V82. A test made twice.** `ai_can_be_knocked_down` returns false when `ai_is_knocked_down` (sub `0x2d`) is true
(`0x47506a`..`0x475073`). `ai_knock_down_local` returns 0 on that result (`0x4742b3`..`0x4742bf`) and only then
tests `ai_is_knocked_down` again (`0x4742c5`). Nothing in between changes the mode, so the branch at
`0x4742ce`..`0x4742f8` never runs. A second knock-down on a lying character is simply ignored, and is not sent in
multiplayer. If the branch ran, it would compare the new duration with the time left (`disable_ms` `+0x1a4` minus
the mode timer) and on a longer one restart the mode timer (`SetTimeout(0)` at `0x4742ec`) without storing the
new duration, so the character would lie for the first duration again. The note in `NOTES-ai.md` described that
second half; it has no effect in the game.

**V90. vsync in the wrong argument.** `LoadSettings` pushes 0 as `gr_init`'s seventh argument (`push 0x0` at
`0x43db1e`), and builds the sixth from `config_cfg_34 != 0` converted to a float (`0x43db08`-`0x43db30`).
`gr_init` stores the seventh argument to `gr_screen.vsync` (`0x25d8dd0`, `0x505eaa`) and the sixth to
`gr_screen.far_zvalue` (`0x505e87`). `config_cfg_34` is the vsync option: `options_apply_detail` copies it to
`0x25d8dd0` (`0x48734d`-`0x48735a`), which `gr_d3d_flip` (`0x53a718`) and `gr_glide_flip` (`0x54a762`) read. The
two arguments look swapped. Nothing shows: `far_zvalue` has no reader, and `GameInit` calls
`options_apply_detail` (`0x43e141`), which sets the real vsync value before play. Only the clear and flip
inside `gr_init` run with vsync 0.

**V100. Dead stores.** Cases `0x40`-`0x42` (ice, fire and energy shield), `0x68` and `0x6a`/`0x6b` (dragons,
non-party targets), `3`/`5`/`0x69` (non-party targets of `0x69`), `0x66` (Laharah's scream) and `0xc`, `0x54`,
`0x57`, `0x65` (the death spells) set `amount` and a count with die 0. The roll with die 0 returns `-count`
(`0x47f4ec`..`0x47f4fe`), which is the same value (or `-ceil(hp)` for the death spells), and is stored over
`amount` at `0x45febe`. So nothing changes. In Laharah's scream the store of `-level` happens only within 10 units
(`distance` at `0x45fc88`, `fcomp` at `0x45fc90`), but the count and die are set either way; `spells.tbl` gives the scream `+Distance: 10`, and
the function already returns for a target farther than that (`0x45e0d8`..`0x45e123`). Leech is the one case
where the lost store matters (G100).

**V101. Jade Beam leftovers.** For a boss or a `0x40`-flagged target, case `0x18` copies the fireball code,
including `if (spell == 0x1a)` before `spell_effect_obj_set_timer` (`cmp [ebp+0x8], 0x1a` at `0x45f0dd`), which
is always false in case `0x18`. For other targets it takes half the current HP and prints it with
`printf("jade dmg: %d\n", (double)dmg)` (`fstp qword [esp]` then `push 0x599c54` at `0x45f1df`..`0x45f1e7`). The
`%d` reads the low half of the double. `printf` has no console in the retail build. The Jade Golem
(`Jade Golem.tbl`, move "jade beam") is the caster.

**V102. Unused direction.** `character_walk_to_point` computes `p - pos`, normalises it into a local
(`vector3__normalize_safe` at `0x455eba`), computes `p - pos` again and dots that unnormalised copy with the
forward vector. It skips the travel order only when the product is exactly 1.0 (`fcomp [0x57e990]; test ah, 0x40`
at `0x455f24`..`0x455f2f`). *Inferred:* the test was meant for the unit vector; as written the order is
practically always given. Callers: `conversation_frame`, `conversation_end_cleanup`, `script_action_wait_at`.

**V110. The second test repeats the first.** After `living_from_handle(handle)` the function calls
`script_avoid_spline_in_use(handle)` twice (`push esi` with the same `esi` at `0x4cee23` and `0x4cee30`).
*Inferred:* the second was meant for the target. It then calls `living_from_handle(target)` and tests
`esi`, the handle (`test esi, esi` at `0x4cee5a`), not `eax`, so a stale target would reach
`script_build_avoid_spline` as NULL. Both callers (`script_entry_start` `0x4dd5e2`, `ls_lpalaceint_sneak_frame`
`0x4f3327`) pass `cur_character`'s handle as the target.

**V111. A debug path that trusts every record.** `level_script_restart` (only caller
`script_editor_on_time_back`) loops over all `level_item_count` records and calls
`navpoint_get_position(record +0x48)` (`0x4cb2cc`..`0x4cb2da`). The parser fills `+0x48` only for records of the
current script (`0x4cb5e6`), so the others hold a pointer from an earlier level or 0. It spawns the item
whether or not the save marks it as taken. At the end it calls `avoid_spline_free` for each used slot
(`0x4cb31a`..`0x4cb339`) but leaves the keys, so a later `script_free_avoid_spline` for that character frees
the spline again and `script_alloc_avoid_spline` refuses a new one. `level_script_leave` frees them the same way,
but there `level_script_reparse` resets the slots (`script_avoid_splines_reset`, `0x4cb427`) before they are used.

**V112. The NULL test comes too late.** In single player the code multiplies the roll by `cur_character +0x600`
(`0x4cc893`..`0x4cc8b2`) and only then, inside the loop, tests `cur_character` (`test eax, eax` at `0x4cc8cc`).
The branch for no current character (characters keep their `char_info` level) is unreachable.

**V120. A timer that never starts.** `game_render_frame` keeps a static `GameTimer` (`0x60a2c0`), constructed
invalid (`-1`, `GameTimer__GameTimer` at `0x501862`). It draws the fog text only while `GameTimer__valid` is true
and less than 10,000 ms have passed (`0x438e65`..`0x438e81`); otherwise it calls `GameTimer__invalidate` and
returns (`0x438f07`). The only `SetTimeout(0)` (`0x438e97`) is inside that drawing branch, so the timer can never
become valid. That call also waits for byte `0x22fb6f7` (`levelrender_get_flag_22fb6f7`), whose only write is the
clear in `levelrender_render_scene` (`0x482392`). *Inferred* intent: the text was meant to show for 10 s after a
fog change, with the flag set by the fog code. The labels are right: the first value comes from `0x486bf0`, which
returns the near distance (the getter names are swapped in sum.symbols, see the symbol fixes).

**V121. Empty name.** `cmd_character` concatenates every argument plus a space into a 128-byte buffer at
`[esp+0x38]`, then removes the last space with `name[strlen(name) - 1] = 0` (`mov [esp+ecx+0x37], al` at
`0x4399da`). With no argument the name is empty, `strlen - 1` is -1 and the zero goes to `[esp+0x37]`, the top
byte of a local that the command only fills later (the copy of `cur_character +0x10..+0x58`, `0x439a55`..`0x439aa5`).
`charinfo_find_by_name("")` then returns -1 (`0x446bb9`) and the command prints "Unable to find character  in
character table". Nothing changes. The usage text is printed only for `help`.

**V122. Toggles with no reader.** `Dump_sound_usage` (`0x609fe1`) is written only by its command
(`0x43790f`..`0x437929`); `Give_all_items` (`0x609fec`) by its command (`0x43808f`..`0x4380a9`) and by
`ls_test_init`, which sets it for the test level (`0x4fde10`). No instruction reads either byte except the
commands' status prints, and the raw byte scan finds no other reference. The cheat that gives all items does not
exist in retail.

**V130. Inverted test, missing cutscene.** `ls_catacombs_frame` goes on only when the byte `0x5fc014` is set
(`0x4fe1e6`..`0x4fe1ed`) and then when `catacombs_flag_intro_played` is already set (`je` on a zero result at
`0x4fe206`). It then sets the flag again and plays "cryptintro" with no callback (`0x4fe208`..`0x4fe227`). Nothing
writes `0x5fc014`; its only reference is this read. Even if the byte were set, the inverted test would play the cutscene
on every frame after the flag is set, never before. cutscene.tbl has no "cryptintro" entry (the catacombs have only
"Carados-Intro"), and no `.vpp` holds a file of that name. Found in the `NOTES-level-scripts-8.md` notes (bug 1).

**V131. Flag cleared, never read.** `ls_wolong2_init` sets `wolong2_charge` to 1 when the script is `wolong2`
(`0x4f208c`..`0x4f209e`). The "Charge" trigger of the Jade Temple Priest's "How can we help?" topic (`wolong2_dlg.tbl`)
reaches `ls_wolong2_force_alert_action`, which sets it to 0 (`push eax` with eax = 0 from `_stricmp`, `0x4f1f06`..`0x4f1f17`).
The string has only these two references (`0x5a26e0`), and no dialogue, script or events table uses the flag
except the `+Flag:` declaration in events.tbl. Either the polarity is backwards or the flag is unused; nothing changes
for the player. Found in the `NOTES-level-scripts-8.md` notes (bug 6).

**V140. The trigger copy.** `script_parse_objects` copies the 256-byte `trigger` buffer into the object's
32-byte field at `+0x530` with an inline `strcpy` (`0x4dc06c`..`0x4dc0b0`); the field is the last of the
0x550-byte record, so a longer name runs into the next script object. A normal object fills the buffer from
`+Trigger:` or with "" (`0x4dbc30`..`0x4dbc74`). The "+No create" path (`0x4dba7f`..`0x4dbb78`) never writes it,
so it copies whatever an earlier call left on the stack (`trigger_level` is -1 there, so the text is not used).
In retail data the longest trigger name is 32 characters ("lenele_slums_got_salamanka_quest", the Fire
Salamanka of `rand-hills01_script.tbl`): its terminating zero lands in the first byte of the next, unused
record and nothing changes. The next record is set up when it is allocated, so even a longer copy would mostly
be overwritten.

**V141. `.vcs` path and comments.** `load_script` builds `install_path` + `\` + name, then cuts at
`strchr(path, '.')` (`0x50ad05`..`0x50ad49`) before appending ".vcs": a `.` in the install directory cuts the path
there. A line that contains `/*` sets the comment flag even when it also contains `*/` (`0x50ae24`..`0x50ae43`),
so the following lines are skipped up to the next `*/` line; a line with `/*` after a command is skipped whole.
The script's line numbers count only empty and executed lines (`inc ebx` at `0x50aeac`). No `.vcs` file ships
and the console has no output.

**V142. `%f` for an int.** With the `debug` prefix, `process_command` formats "Found int number! %f" with the
integer `command_argument_int` (`0x50a7d7`..`0x50a7e8`). The text goes to the empty `print_to_console`.

**V143. Silent duplicates.** The constructor returns when 300 commands are registered (`cmp eax, 0x12c` at
`0x50a188`) or `_stricmp` finds the name (`je 0x50a1df` at `0x50a1b1`), before it stores the name, description
and callback (`0x50a1d7`..`0x50a1dc`). The 149 retail names are all different.

**V150. A reload switch nothing reaches.** `gr_glide_tcache_set` reloads the sections of an existing texture
entry only when the entry's byte `[entry + 8 + tmu]` is set (`0x5495e2`..`0x5495e8`). The only stores to those bytes
in the unit write 0: `gr_glide_create_texture` (`mov word [edi+8], 0` at `0x549390`) and `gr_glide_tcache_set` itself
(`0x5495dd`); `glide_record::load_section` clears the section bytes of the same layout (`0x549acb`, `0x549aec`). All
references to the entry table `0x2f0ec00` are in this unit (asm grep), and none writes a non-zero value.
`gr_glide_mark_texture_dirty` rebuilds the texture instead (`0x549718`). If it did run, the reload switch
(`0x549612`..`0x549686`) copies from the start of the locked bitmap for every section and only flips the 1555 alpha bit
or copies raw bytes, without the paletted / 888 / 8888 conversions of `gr_glide_create_texture_section`.

**V151. Name mismatch on a shared spline.** `ls_lenele3a_use` speaks L503 only when the used name is "templeint"
(`_stricmp` at `0x4f1034`..`0x4f1044`). A trigger reaches the use hook with its `+RefName:`, which defaults to the
trigger name (`level_parse_trigger` `0x4cc310`..`0x4cc32f`; `level_check_triggers` passes `+0x24`, `0x4cac75`..`0x4cac79`).
Retail `lenele3a.tbl` has no `+RefName:`, and its temple trigger is "templeint2". Its "lpalaceint02" (trigger 1) and
"templeint2" (trigger 2) both use `$loadarea06`. Before game stage 21 `ls_lenele3a_init` makes "templeint2" a
script-use trigger and "lpalaceint02" an exit (`0x4f0fb7`..`0x4f0fca`, `0x4f0fd3`..`0x4f0fed`); from stage 21 the other way
round (`0x4f0f91`..`0x4f0fa4`). `level_check_triggers` stops at the first exit it crosses (`0x4cac9a`..`0x4cad2c`), and
trigger 1 comes first, so in single player the doorway leads to the palace before stage 21 and to the temple after;
the temple trigger's use call never comes before the exit. With the right name the line would still only be reached
where no exit shares the spline (see "Still unclear" for the `+Multi` copies).

**V152. An else that toggles the wrong way.** `ls_lenele1b_action` handles "CheckReleaseOrder" with
`script_topic_toggle_by_item` for "The Keep?" and `script_topic_toggle_by_item_inv` for "Give him the release order."
(`0x4fb1fa`..`0x4fb222`). Every other action name takes the `else` (`jne 0x4fb227` at `0x4fb1f8`), which calls the
non-inverted `script_topic_toggle_by_item` for "Give him the release order." (`0x4fb227`..`0x4fb236`): the topic becomes
available when the party lacks the Forged Document and hidden when it has it (`0x4ceb80`..`0x4cebbe`). The other
actions of `lenele1b_dlg.tbl` (CheckEyes, CheckTongue, CheckGold, CheckGold2, CheckShards, CheckCrate, CheckSalt,
CheckDoll, SalamankaActive, TradeBacite) all pass through it. *Inferred:* no visible effect. Every conversation
with Gorbus starts with Hail, whose `+Trigger: "CheckReleaseOrder"` sets the topic right again. Inside his
conversation the only other trigger is CheckGold, run by "Give him the release order." itself. It hides that topic
while the document is held, after the guard has examined it. The three answers ("Give him 20 gold.", "From the
palace.", "From the forger.") stay. The `else` was probably meant to be absent.

**V153. The frame number stored as the owner.** `gr_glide_mark_texture_dirty` maps the handle to the current frame
(`bm_get_current_frame`, `0x5496e8`) and calls `gr_glide_create_texture(frame, entry)` (`0x549716`..`0x549718`), which
stores its first argument as the entry's bitmap (`mov [edi], ebp` at `0x549337`). `gr_glide_tcache_set` rebuilds the
entry whenever that differs from the handle it is given (`0x5495bc`..`0x5495c9`). For a single bitmap the frame is the
handle; for an animation it is not, so the next draw builds the texture a second time. Callers of
`gr_mark_texture_dirty`: `bm_set_reduced` (`0x5022f3`, texture quality), `bm_replace_file` (`0x50339c`) and the two
thumbnail functions (`0x427215`, `0x4d2997`). Only time is lost.

**V170. The restart keeps the old screen.** When the screen being left is not full-screen and the new one is 1, 2, 7
or 10, `interface_set_screen` picks a `status_character` if it is NULL. When it finds none it restarts with screen 0
by jumping back to its top (`jmp 0x41262c` at `0x412770`, `0x4127b9`, `0x4127d3`), but `interface_cur_screen` is
only written later (`0x4127f8`), so the old screen's leave runs a second time. From the HUD the leave is empty. From
a side panel the leaves are idempotent (`skills_leave`, `assess_leave`, `itemtrade_leave`), except `pickup_leave`,
which in multiplayer sends `mp_send_container_close` again (`0x420999`). It needs `status_character` NULL while the
current character is not the main hero and no type-7 living entity exists on the level, which we know of no retail
case for.

**V171. A whole-file reader that is never used.** `packfile_read_whole_file(buf, size, vpp)` finds the packfile by
name, refuses a buffer smaller than its size (`"B : (%d %d)"`, `0x53492e`), then `fopen`s the record's name as it is
(`push esi` at `0x534951`), relative to the current directory instead of `install_path`, and reads the whole `.vpp`.
For a CD packfile (`+0x20` bit 0) it returns 1 without reading anything (`0x534946`..`0x53498c`). Its only caller,
`file_blast_into_buffer` (`0x517220`), has no caller and no pointer reference in v1.40.

**V180. A NULL thread and an unread flag.** On activation `os_wndproc` calls `SetWindowPos` and then
`SetThreadPriority([0x2cbff14], 2)`, on deactivation `SetThreadPriority([0x2cbff14], 0)` (`0x5310b7`..`0x5310bf`,
`0x5310ef`..`0x5310f8`). `0x2cbff14` is read only there and never written (no other reference in the image), so the
call gets a NULL handle and fails. `os_init` runs `os_find_previous_instance` after the single-instance mutex test
(`0x530e25`); its callback sets `0x2cbff0c` (`0x530bc5`), which is only ever cleared (`0x530e46`) and never read. The
mutex test is what actually stops a second copy.

**V181. The ignore mark is never tested.** For a connection request (type 3) the first loop finds a link in status 1
or 5 with the same address, logs "Received duplicate connection request", ACKs it and sets the packet type to 7
(`0x517f49`..`0x517f91`). The free-slot loop that follows does not test the type (`0x517fa7`..`0x517fbe`), so it takes
another slot for the same address, puts it in limbo (status 5) and replies (`0x517fc2`..`0x518013`, `0x518028`..`0x518413`).
The client resends its request every 2 s until it gets the reply (`nw_ConnectToServer` `0x519034`..`0x51904c`). Later
packets from that address match the first slot (`0x518044`..`0x518081`), so the extra one only waits for the limbo
time-out of 100 s (`0x518461`..`0x5184b0`). There are nine slots.

**V182. Stack bytes on the wire.** `nw_send_heartbeat` fills type 6, sequence field 0xfebd and length 0 of a local
packet (`0x517d7b`..`0x517d87`) and sends 10 bytes (`push 0xa` at `0x517da8`); bytes 6 to 9, the send time, are never
written. `nw_send_reliable_reply` fills type, length 4, send time and the acknowledged number at +0xa
(`0x517d3d`..`0x517d57`) but not the header sequence at +2. The receivers use only the data field of an ACK and echo
the IAMHERE time (`0x51812a`..`0x51814c`). *Inferred:* one exception is an ACK for a second IAMHERE (sent every 750 ms
while the link is in limbo, `0x517e60`..`0x517e8a`) that arrives after the link is up: it goes through the normal ACK
path and its echoed garbage time becomes one ping sample (`0x5181a6`..`0x5181c9`); the median of ten hides it.

**V183. Two localize slips.** The lowering loop of the section name compares the index with the current character
(`movsx eax, byte [esp+edi+0x60]; cmp edi, eax; jl` at `0x5049bc`..`0x5049c7`), so it stops once the index reaches the
character's code: the smallest code in a level name is `-` (45), so only names of more than 45 characters can keep
upper-case letters (the retail section names have at most 32 characters, and the file's section lines are lower case). `localize_table_add` allocates the 12-byte node and copies the text
(`0x5040f4`, `0x504119`) before it walks the bucket for the same hash and text (`_stricmp` at `0x50414f`, `0x50416d`),
so a duplicate returns with both allocated and unused. The retail item, quest and level tables have no duplicates.

**V184. Early returns.** `os_clipboard_get_text` calls `OpenClipboard(0)` without testing it, returns without
`CloseClipboard` when `GetClipboardData(CF_TEXT)` or `GlobalLock` returns 0 (`jmp 0x531be6` at `0x531b6a`, `0x531b7f`),
copies at most `max` characters and writes the terminator at `buf[n]` (`0x531bcd`..`0x531bd3`). There is no call to it
and no pointer to `0x531b44` in the image. Its code is unoptimised, unlike the rest of the object before `0x5312c0`.

**V190. The file list is never freed.** `file_find_files` allocates a 0x38-byte node per match with `operator new`
(`0x5134d7`) into a list whose head is a local of `screenshot_save` (`0x432552`). `screenshot_save` reads the last node
and returns without freeing any of them; `quicksave` frees its list after the same call (`operator_delete` at
`0x40e82d`). *Inferred:* a few kilobytes per hundred screenshots, never noticeable.

**V191. Second callback for the third button.** `editor_dialog_handle_event` tests `callbacks[2]` (`mov eax, [esi +
0x2c]` at `0x43161b`) but calls `callbacks[1]` (`call [esi + 0x28]` at `0x431628`). Every caller passes no callbacks and
only the one-button OK set, so neither callback exists.

**V192. `0x8000` instead of `0x800`.** The case compares the key with `0x800e` (`0x4327d9`), Backspace with bit
`0x8000`. `key_mark` adds `0x800` for a key pressed while the grave key is down with `key_debug_modifier` set
(`or ch, 0x8` at `0x509508`), and no code adds `0x8000`, so `key_flush` (`0x4327e1`) is never reached from here. The
debug-key tracking above it masks the same wrong bit (`and ah, 0x7f` at `0x432789`) and only writes two bytes nothing
reads (`0x5fc028`, `0x5fc029`).

**V193. A handle used as a signature.** `snd_stop` (`0x5215d0`) passes its argument to `ds_stop`, which looks for a
channel with that play signature (`ds_get_channel` `0x55db00`); `music_play` uses it with `music_play_signature`
(`0x4690bc`). `cutscene_unload` gives it the sound slot index instead (`0x4352da`). Signatures count up from the first
play of the session (`ds_next_signature`, `0x55d3d3`..`0x55d3e6`), so a slot index matches only a channel still
holding one of the first few signatures; normally the call does nothing. The stream itself was already stopped
with `snd_stop_fade` on its real signature (`0x43667a`). `snd_free` on the slot that `snd_level_release` freed is a
no-op unless a stream took the slot in between (see K190).

**V200. `buf[strlen - 1]` on an empty name.** Both commands join their words with a space after each and then
remove the last character with `mov byte ptr [esp + ecx + 0x7], al`, where `ecx` is the length and the buffer is at
`esp+0x8` (`0x44023d`..`0x440250`, `0x44038d`..`0x4403a0`). With no argument the length is 0 and the store hits
`esp+0x7`, the top byte of the EDI that the command pushed (`0x4401b8`) and pops back for `execute_command_line`
(`0x440272`), which in turn restores its own caller's EDI from it (`0x50a54d`). The messages these commands print go
nowhere: `print_to_console` is a bare `ret` (`0x50a5a0`).

**V201. The socket of an unknown address is dropped.** `mp_server_accept_connections` takes a pending link with
`nw_accept_reliable` and looks its address up with `mp_player_find_by_addr` (`0x440d29`..`0x440d44`). When there is no
record it returns (`je 0x440d62` at `0x440d47`) without `nw_NewCloseSocket`, so the link stays open in the reliable
layer, which times links out only after 100 s without a packet (see V181). A machine whose join record was already
freed (for example by `mp_server_check_clients` when the join timer ran out, `0x440c96`..`0x440cad`) would connect
this way.

**V210. The loop variable after the loop.** [notes 1] `ai_group_remove_member` shifts the members after the removed
one down and then stores -1 at `[group + 4*edx + 4]` with `edx` = 6 after the loop (`0x492d07`), which is `+0x1c`,
the group's enemy count, not `members[5]`. A removed member in the last slot gives the same `edx`. The next
`ai_group_collect_enemies` rewrites the count (`0x493b22`), so the -1 lasts until the group thinks again. Readers in
between (`ai_group_count_helpless`, the style functions, `ai_combat_cast_frame` `0x4bfc40` / `0x4bfd4b`) compare
signed (`jle`), so -1 acts as 0. Callers: `character_on_death`, `party_remove_member`, `party_remove_player`,
`cmd_character`, `script_entry_check_done`. *Inferred:* no visible effect.

**V211. A message nobody prints, tables nobody reads.** [notes 9] Each `*_rider_set_flinch` walks the flinch move
list and, for an action that starts with `stage` but is neither `stage<N>_flinch_area` nor `_status`, calls
`sprintf` into a stack buffer and does nothing with it ("Unable to set Serpent / Phoenix / Tiger Rider flinch for %s
-- bad $Action", `0x495587`, `0x4987e7`, `0x49a267`; no print call in the three functions). `Phoenix Rider.tbl`
has flinch moves `stage1_flinch_high` and `stage1_flinch_mid`, so the Phoenix's message is built on every load.
The tables themselves (`0x24523c0` / `0x24523cc` Serpent, `0x24529dc` / `0x24529e8` Phoenix, `0x2452c4c` /
`0x2452c58` Tiger) have no reference outside these three functions. The Serpent and Phoenix copy stage 2 into
stage 3 at the end; the Tiger copies stage 2 into stage **1** (`0x49a27a`..`0x49a28f`; it has stage 2 and 3
flinches only). The digit is not range-checked either: `stage0_` or `stage4_`..`stage9_` would write outside the
three-entry tables (latent, modded data only).

**V212. A discarded lookup.** [notes 13] `phoenix_set_foot_anims` calls `charinfo_find_anim(info, "ready_hurt")`
(`0x499c29`) and overwrites `eax` with the `walk` lookup (`0x499c3a`) without a store; the four stand and ready
slots already got `ready`. `Phoenix Rider.tbl` maps both `ready` and `ready_hurt` to `phnx_ready.mvf`, so nothing
changes. `serpent_set_foot_anims` stores its `ready_hurt` (`0x497650` body).

**V213. A stale and a leaked axe.** [notes 16] `tiger_unload` (`boss_ai_unload` case `0x57`, from
`ai_state_shutdown` in the entity destructor) calls `item_free(tiger_axe)` and leaves the pointer
(`0x49ae90`..`0x49ae9f`). Every `tiger_boss_load` (each `ai_reset`: level load, cutscene actor reset, savegame
load) runs `tiger_equip_weapon`, which overwrites `tiger_axe` with a new `item_create_weapon` (`0x49a2c0` body)
without freeing an axe that was not handed over. A second free needs a second Tiger unload without a load in
between (no retail case known); the leak costs one item per extra load from the 1224-item pool (C12).

**V214. Set, then cleared.** [notes 6] `serpent_boss_load` sets `serpent_has_rider` (`0x2451da3`) by level
(`0x49599a` / `0x4959a5`) and only then, on the first load, clears the 0x640-byte state from `0x2451d98`
(`rep stosd` at `0x4959cc`), which includes the flag. `serpent_spawn_rider` then creates the two copies as type-9
species `0x59` (`living_entity_create` at `0x49574b` / `0x4957d6`); `living_entity_create` calls `ai_reset`,
which runs `boss_ai_load` and so `serpent_boss_load` for each copy. Those nested calls skip the setup but set the
flag again (1 outside `Liangshan` / `Jadetemple`). So the flag is right by the time `serpent_seat_rider` runs
(`0x495b73`). Only if both copies failed to be created would the mounted rider not be seated.

**V230. Index -1 used.** When every slot has flag `0x1`, the slot search leaves ESI = -1 and jumps over the stores
(`0x47606d`). The party-leader branch that follows (`0x476124`..`0x476176`) has no test of ESI. It ORs `0x100` into
`[edi + 4*(13*-1) + 0x90]` = `ai_state +0x5c`, then returns -1. `+0x5c` is the travel point's z (`ai.md` Paths). Bit
8 of a float is a low mantissa bit, so the point moves by a few millionths of its value. Only follow orders
(`ai_think_party_update` flags 1, `script_action_follow` flags `0x21`) and caller-chosen travel and move-to flags set
`0x1`. Five kept goals at once are not known to happen.

**V220. Index -1 used as a slot.** `ai_approach_target_point` starts with index -1 (`0x4bd008`) and keeps it when no
attack point is free (or the boss stage disables them, `0x4bd054`..`0x4bd150`). It stores the index at
`ai_state +0x7c` (`0x4bd162`), paths to the target's own position, and on success writes the handle to
`[target + 4*index + 0x4fc]` (`0x4bd279`..`0x4bd285`): with -1 that is target `+0x4f8` = `ai_state +0x1d4`, the
GameTimer of `attackers[4]` (`ai_state +0x1b0`, 8 bytes each). *Inferred:* that slot's remaining time is garbage until
it is claimed again; `ai_claim_attack_slot` compares remaining times when it takes a slot over.

**V221. One past the end.** The move loops run while `i < count` and copy `[i + 1]` into `[i]`
(`cmp ebp, eax; jl` at `0x4e6bca`, `0x4e6c27`..`0x4e6c96`), so the last pass copies `navpoints[count]` and calls
`script_replace_navpoint_references` for it; the slot is dropped by the decrement. With 750 level navpoints the
read is the 0x30 bytes after the table.

**V222. Emitters freed, sounds kept.** OK removes the old ambient (`level_ambient_remove`, which only deletes the
emitter, `0x4cd2d8`) and calls `snd_get_handle` for the new one (`0x4eb4ad`). `snd_get_handle` reuses a slot only for
the same name and parameters (`0x52082d`..`0x52086b`) and otherwise takes the first free one of 640
(`0x520818`..`0x520884`); nothing releases the old slot. *Inferred:* only many edits in one level matter.

**V223. The wrong end.** For `a->b == b->a` the function writes `b->b` (`mov edx, [edi]` with `edi` = `b->b`,
`0x4a57b0`..`0x4a57dd`) and returns 3. `pf_first_edge_crossing`, `pf_segment_hits_walls`, `segments_intersect_xz`,
`ai_find_nearest_wall_hit` and `do_single_vis_check` only use results 1 or 4 or none; `pf_find_cell_crossing` uses the
point for any positive result (`test eax, eax; jle` at `0x49edb5`). It needs two exactly equal floats.

**V224. Unguarded lookups.** `avoid_seglist_ensure_corner_offset` searches the list's segments for the corner and,
with no second segment, reads `segs[first].wall` (`lea ecx, [ecx + 4*ecx]; mov edx, [edx + 4*ecx + 0x1c]` at
`0x4a8306`..`0x4a8309`) also when `first` is still -1 (`or ecx, -1` at `0x4a82cb`). `avoid_merge_shared_segments`
takes `segs[index - 1]` as the segment before a reference and falls back to `segs[index - 2]` when that one does not
end at the corner (left branch from `0x4a8c5c`); for index 1 that is `segs[-1]`. The lists are built so that every
corner lies on a segment; we found no path that breaks it.

**V250. A failed pick stored.** In stage 3, when `titus_stage_timer_a` has run out and the target has status 10
(paralyse), Titus asks for a candidate with flag 8 (stage 3, not paralysed: `titus_filter_target`
`0x4c2275`..`0x4c228c`) and keeps the result in ESI (`0x4c2c4a`..`0x4c2c7b`). When there is none, ESI is -1; the
timer is set to 15 s and the code goes on to store ESI as `titus_target_handle` with move 21, 22 or 23
(`0x4c2cf0`, `0x4c2d4e`, `0x4c2d61`). `titus_ai_frame` finds no target and goes idle unless the move is 21
(`0x4c2680`..`0x4c271c`); the rock eruption (move 21) does not need the target. On the next think
`titus_ai_select_action` sees no target and picks one with flags 0 (`0x4c2bb9`).

**V251. The Giant Salamanka's copy of V211.** `giant_salamanka_set_flinch` skips `strlen("stage")`, reads one
digit with `atoi`, skips the digit and `_flinch_` and compares the rest with "area", "status", "left", "right" and
"front" (`0x4c82a3`..`0x4c8369`). Any other name goes to `sprintf` into a local buffer that is never printed
(`0x4c836b`..`0x4c837e`). Retail `Giant Salamanka.tbl` has only matching names (stages 2 and 3 have no "left").

**V252. A buffer filled only on the other branch.** The extension buffer `[esp + 0x10]` is written only for
`PATH_ANY` (`0x5142ad`..`0x5142f6`). An explicit path id that is not found goes through the packfile test to the
write-path loop, which calls `stristr(extensions, buffer)` on it (`0x51436f`..`0x514395`) and sets the path id and
full path from the result (`0x5143a5`..`0x5143c0`). `File::open` repeats the find with `PATH_ANY` when the explicit
one fails (`0x5146e5`..`0x5146ed`).

**V253. Memory-mode seek limits.** Origin 0 and origin 1 accept only a positive offset that stays below the size
(`0x513abb`..`0x513ad9`); origin 2 subtracts the offset from the current position (`0x513aa4`..`0x513ab3`). The
callers (`skeleton_read` `0x54425a`, `0x5442a0`, `0x5442d5`; `vim_read` `0x543463`) use origins 0 and 1 with
positive offsets or ignore the result.

**V254. strcpy into 8 bytes.** `File::open` copies the extension from `file_get_ext` into `0x5a7818` ("none" plus
padding, 8 bytes) without a length test (`0x51465c`..`0x51467a`, `0x51470c`..`0x514728`). The next bytes are the
" " that `File::find` appends to the extension (`0x5a7820`, read at `0x5142ef`) and the mode strings "wt" / "wb"
(`0x5a7824` / `0x5a7828`). No retail file name has such an extension.

**V255. A dropped result.** When the directory is already in the table, `file_add_path` walks the new extension
list and calls `stristr` for each entry (`0x512fc1`..`0x51302c`) but never uses the result and never adds the
extension. `setup_data_paths` adds each directory once (`0x43dca0`..`0x43df04`); `cutscene_set_data_path` removes
its directory first (`0x43533f`).

**V256. Dead File helpers.** No call and no pointer to `0x525ba0`, `0x513f70`, `0x513590`, `0x525ce0` or
`File__sync_matrix43` (`0x526840`) exists in the image (byte search). `read_matrix43` stores at `+0xc`, `+0x18`
and so on (`0x525bd9`..`0x525c42`). `read_String8` copies bytes up to the NUL into `[esp + 0x4]` of a 0x100-byte
frame (`0x525d34`..`0x525d5c`).

**V257. Only the UI table loader reaches them.** `ui_button__set_tooltip` unlinks the first tooltip node and
allocates a new one (`0x5271d6`..`0x5271f2`); `ui_static__set_tooltip` is the same. `ui_button__create_in_parent`
ORs 2 into the flags (`or al, 0x2` at `0x52713e`). Their only callers are `ui_parse_button` and `ui_parse_static`,
reached only from `ui_load_table` and the container / slider / listbox parsers it calls, and `ui_load_table` has no
caller (V9).

**V260. An unnormalised direction.** For emitters with flag 4, `sound_emitter_calc_pan` takes the closest point of
the segment `+0x5c`..`+0x68` (`ix_point_linesegment` at `0x521290`) and, beyond 0.5 units, returns
`dot(point - listener, listener right)` (`0x5212e7`..`0x5212f1`) without dividing by the distance. `snd_set_pan`
clamps it to -1..1 (`0x521769`..`0x521797`). Flag 4 is set only by `sound_emitter_create_segment` (`or al, 0x4` at
`0x5466d4`), which has no caller; level ambients are point emitters (`level_ambient_add` uses `sound_create_emitter`).
`snd_calc_pan` (point emitters, `snd_play_3d`) uses the normalised direction (`vector3_normalized_dir` at `0x52122f`).

**V261. Compared before it is fixed.** `snd_get_handle(name, min_range, volume, rolloff, group, stream)` reuses a slot
only when `snd_params_equal` matches volume `+0x28`, minimum range `+0x2c` and roll-off `+0x34`
(`0x520848`..`0x52086b`), but it replaces a minimum range of 0 or less by 1.0 only when filling a new slot
(`0x52091a`, `0x520962`..`0x520987`), so such a sound never matches. The name is copied into a 256-byte stack
buffer (`0x5207ed`..`0x520803`) before the 0x28 length test (`0x5208e5`). Callers with a table value as the minimum
range: `sound_ref_load` (`$Sound:` byte `+9`), `level_parse_ambient` (`$Ambient:`, first float), and
`characterinfo_parse_table` (`+Footstep sounds:`, first float). In the retail tables the smallest value is 2.0
(3,138 `$Sound:`, 1,984 `$Ambient:` and 28 `+Footstep sounds:` lines).

**V262. A reference per play.** For a 3D buffer played in 2D, `ds_play` calls `eax_buffer_init` when
`ds_eax_enabled` is set (`0x55d1bd`..`0x55d1cd`). It calls `QueryInterface` for the property set (`call [ecx]` at
`0x521ff3`), ignores the result, sets buffer property 4 (ROOM) to -10000 through the returned pointer
(`0x522008`..`0x52200e`) and never calls `Release`. Each play duplicates the buffer (`ds_duplicate_buffer` `0x55d170`),
so each duplicate keeps one reference after its channel releases it. *Inferred:* a small leak per sound while EAX is
on; a failed `QueryInterface` leaves NULL and crashes. EAX needs `Soundcard=EAX` or `-eax` and a working
`eax.dll` listener property set (`eax_init`).

**V263. Dead helpers.** `lights_debug_draw` loops `light_count` times from `lights + 0xc` (`0x51fa84`..`0x51fab2`),
which are not the listed lights once one is deleted. `lights_calc_point_specular_color` tests the normal pointer for
NULL (`test edi, edi` at `0x51f69d`) but, inside the inner radius, calls `vector3__dot` with it (`mov ecx, edi` at
`0x51f701`). `vfx_all_resident` indexes the record table `0x2c8f068` with a `vfx_find_name` index
(`0x51dcbf`..`0x51dcce`). None of the three has a call or a pointer.

**V264. The dirty byte.** `ui_listbox_entry__set_font`, `set_indent`, `set_wrap_indent` and `set_line_spacing` set
`+0x04` to 1 (`0x52d5a4` and the others); only the constructor clears it (`0x52d55c`). `ui_listbox__relayout_dirty`
re-wraps every entry with the byte set and then recomputes the content size and both scroll bars
(`0x52cf2a`..`0x52cf66`), and it runs at the start of every `process` and `draw` (`0x52d09a`, `0x52f098`). The script
editor calls `set_font` on its entries (`script_editor_create_ui` `0x4e10c2` and others, `script_editor_fill_action_list`
`0x4e4eb3`, `script_editor_list_ambient_sounds` `0x4e52f3`). Performance only.

**V265. Unreachable in v1.40.**
- `ui_listbox__set_tooltip` unlinks the old tooltip without freeing it (0x120 bytes, `0x52d7f6`..`0x52d812`) and copies
  the text into `+0xc` without a length test; its only caller is `ui_parse_listbox` (`0x529d15`).
- `ui_parse_listbox`: "+Add both scrollbars" sets only the vertical flag `0x2` (`0x529b58`), and "+Add horizontal
  scrollbar" skips the other two tests (`jmp 0x529b5e` at `0x529b36`).
- `ui_state_bitmap__draw` returns before `gr__reset_clip` for state 1 without a bitmap (`0x52ca00`..`0x52ca0d`); state 1
  is set only by `ui_state_bitmap__set`, whose callers are `ui_parse_button` and `ui_parse_static`.
- `ui_listbox__remove_entry` and the bottom-aligned branch of `ui_listbox__add_entry` set the scroll position
  (`0x52ddab`, `0x52ddba`, `0x52db57`) without `ui_slider__set_pos`, so the next `sync_scrollbars` sets it back to the
  slider's position (`0x52d03f`..`0x52d062`); `ui_slider__set_range` leaves the position alone (`0x55eeb0`). Their only
  callers are `add_entry` with a content-size limit and `ui_listbox__remove_by_text` (no caller), and no script editor
  list box is bottom-aligned (flag `0x80`) or size-limited.
- `get_vscroll_fraction` / `set_vscroll_fraction` step lines by the font height without the line spacing
  (`0x52e9c4`..`0x52e9d7`); neither has a caller.
- `ui_process` keeps searching the windows after a tooltip hit (`0x530587`..`0x530652`), so the last window in the list
  order sets `ui_tooltip_gadget`; every `set_tooltip` caller is a table parser.
- A hidden list box plays UI sound 7 when clicked or on its hotkey (`0x52d108`..`0x52d13d`); `ui_sounds` (`0x2cbf684`)
  is set to -1 by `ui_init` and written nowhere else, so `ui_play_sound` plays nothing.

**V270. Toggles without a reader.** The three commands set, clear or toggle `Render_level` / `Render_characters` /
`Render_effects` (`0x59bd79`..`0x59bd7b`, `0x4818af`..`0x481aea`) and print them; no other instruction references
the three bytes.

**V271. Flags never set.** `fog_fading` (`0x22fb6f8`) is read at `0x482897`, `0x486bf0` and `0x486c20`; `0x22fb714`
at `0x482d9b`, `0x4839f4`, `0x4841fa`, `0x48438e` and `0x485f89`; `levelrender_lights_dirty` (`0x243e038`) at
`0x48236a` and cleared at `0x482381`. No instruction writes a non-zero value to any of them.

**V272. Bitmaps without a reader.** `shadow_create_blob_bitmap` fills sixteen alpha ramps at `0x2308e2c`
(0x1000 bytes each) and stores the `bm_create` handles at `0x59c260`..`0x59c29f` (`0x48a014`..`0x48a045`). No
instruction reads `0x59c260`..`0x59c29f`, the palette `0x230561c` or the ramps; shadows use `Shadow01.tga`
(`shadow_bm`, `0x48a078`).

**V290. Slips in dead code.** In `ix_box_oriented_box_oriented` the third separating-axis test computes
`(|B[1][2]| + |B[1][0]|) * b.z + |B[1][1]| * b.y` (`0x53c741`..`0x53c75a`), so `|B[1][0]|` multiplies the third
extent instead of the first; the notes list three more tests of the same kind. In
`vfx_emitter_instance__render_d3d` the screen test keeps x above -100 (`[0x580e34]`, `0x561ba6`) but y above +100
(`[0x57e6b4]`, `0x561bd6`). Neither function has a caller (`tools/asm.mjs callers`, no address reference). Found
in the `NOTES-unnamed-vfx-ix-floatmath-ui.md` and `NOTES-unnamed-vsdk-tail.md` notes.

**V300. Green goes to red.** With `gr_tint_enabled` set, `gr_poly_rgb` computes `(r + g + b) * tint` for each channel,
like `gr_poly_set_texture`. It drops the red result (`fstp st(0)` at `0x54567b`), stores the green result in the red
argument (`mov byte ptr [esp + 0x30], al` at `0x5456bb`) and keeps the untinted green in `ebx` (`0x545617`, used at
`0x545706` and `0x54574c`). The tints come from `hover_tint_set_character` / `hover_tint_set_object` (the three
channels from one `ui_apply_pulse_rgb` value, `world_hover_tint_character` `0x43b1d0`, `world_hover_tint_object`
`0x43a650`) and the fx character tint (one `fst` for all three, `murod_ai_leap` `0x4c54ec`..`0x4c54f8`,
`luminar_teleport_in` `0x4ca166`..`0x4ca172`). With equal factors the swapped results are the same. Found in the
`NOTES-unnamed-gr-line-poly-emitter-tcache.md` notes.

**V301. Texture size against bitmap stride.** The 8-bit alpha branch (format 2) loops `tex_h` rows of `tex_w` bytes and
steps the source by `bm_w` (`0x54c1ab`..`0x54c28f`), without the section position the other formats read (C300). A
bitmap narrower or shorter than its power-of-two texture is read for up to `(tex_h - bm_h) * bm_w + tex_w - bm_w`
bytes past its end; those texels lie outside the drawn u/v range. Which retail bitmaps use this format was not
checked. Found in the `NOTES-unnamed-gr-line-poly-emitter-tcache.md` notes.

**V302. Two keyboard queue slips.** `key_init` calls `key_flush` at `0x5097b5`, but `key_flush` returns at once while
`key_inited` is 0 (`0x509ab0`..`0x509aba`), and `key_inited` is set only at `0x5097fa`. The queue is still the zeroed
`.bss` then. `key_putkey` advances the tail with no compare against the head (`0x509d54`..`0x509d76`), so a 16th
pending key would make the queue read as empty; it has no caller. Found in the
`NOTES-unnamed-key-timer-dialogue-levelscripts.md` notes.

## Claims that did not hold

| Claim | Where it was made | What the code does |
|---|---|---|
| A load sets `level_time` from header field `0xa8` (always 0) | [Savegame](savegame.md#header-observed) note | `savegame_read` pushes `&level_time` as the **fourth** argument (`push 0x60a654` at `0x4d2627`). `savegame_read_header` copies field `0xa8` into the third (a local, `[esp+0x20]` at `0x4d2ad7`) and field `0xac`, the saved time, into `&level_time` (`[esp+0x24]` at `0x4d2aea`). No swap; the savegame page needs a correction |
| `hud_portrait_get_color` case 3 computes red from green | HUD decompilation notes | True (`0x40f987`, `0x40f98b`), but `color_pulse_scale` (`0x414ae0`) scales three equal 1.0 inputs the same way, so the result does not change. Not counted |
| Loot gold is lost when the inventory is full | earlier notes | The gold loss is in the shop **buy** path (G5), not in loot pickup |
| Level select button 31 scrolls the wrong way | earlier notes | The button is in the multiplayer **character** select screen (K2) |
| Only slots 0, 1, 4 and 5 of the 8-entry stream ring are reachable | [`src/decomp` notes](../../src/decomp/README.md) | True for the 6-slot ring (V2). The 8-slot ring reaches all 8 (V6) |
| Invisibility lasts 11 s instead of 10 because of the parser | `src/decomp/game/NOTES-spells-combat.md`, `spells.cpp` comment at `status_effect_duration` | `+Loop Time:` sets status 8's tick period (`0x454b29`..`0x454b3a`), not its length, and status 8 has no tick action. The real effect of the parser slip is on cooldowns (G12) |
| After a container close the other machines keep the taken items and never delete an emptied container | `src/decomp/game/NOTES-multi.md` bug 1 | `container_remove_item` decrements the count even on a miss, so the last entries go and an emptied container is deleted (G14) |
| Life leech is `0x450c70` with -0.1 / 0.1 of the damage | [Combat](combat.md) | `0x450c70` is `character_add_ap`, used by mind drain; life leech heals with `character_add_hp` (corrected on the page) |
| Melee calls `character_get_resistance(-1)`, which may read out of bounds | `NOTES-spells-combat.md` open question 7 | Type -1 returns 0 before any table access (`0x4626bb`..`0x4626cb`) |

Not counted as bugs: `camera_key_extra_axis` (`0x4348a0`) always returns 0 and `camera_cut_to_view_stub`
(`0x479430`) is empty. They look like cut features ([Camera](camera.md)). The other oddities in the decompilation notes were checked again on 2026-10-15. Those that are bugs are in
the tables above; the rest are below.

**Checked, not bugs:**

- `ui_container::draw` one-pixel lines: both calls pass the same point twice on purpose (`0x5539fd`..`0x553a02`,
  `0x553b2c`..`0x553b43`). They are the corner dots of the label gap in the etched frame.
- `skills_handle_mouse` reading `skills_list[-1]`: mouse mode 2 is entered only after `skills_handle_click` has
  set `skills_hover_row` to the hit row, so the index is never -1 there (`0x428711`).
- `spells_update_member_buttons` without a multiplayer test: the buttons call `ui_party_prev` / `ui_party_next`, and
  `party_cycle` stops only on members of the local player in multiplayer ([Party](party.md)), so they do nothing harmful.
- `hud_interact_nearest` skipping items that are "in view": `gr_cull_sphere` returns 1 when the sphere is
  *outside* a plane (`mov al, 1` at `0x50ed2b`), so `item_obj_is_in_view` means "culled" and the skip is right.
- `pxochat_new_version_popup_frame` leaving for every choice above 0: the popup has only two buttons.
- `status_apply_skill_points` dead sum and `inventory_destroy_selected_item` testing `item_can_destroy` twice:
  no effect on the result.
- `gamecreate_levels_below_dec` stopping at 0: the code is consistent with `levels_above_dec`. Only the
  sum.symbols comment ("min 1") is wrong.
- `buysell_page_up` testing `jl`: equal in effect to the other screens' `> 0`.
- Loops without a break in `itemtrade_party_handle_mouse` and `pxochat_handle_click`: the matches are unique,
  so the last match is the only one.
- `cmdline_take_argument` formatting a message and dropping it: the debug print is compiled out.
- `gr_fit_string_String` returning "" when the string fits: no caller (`0x508df0`).
- `mainmenu_start_new_game`'s argument from `conversation_save_table[84]`: a naming problem, not a code fault.
- `assess_update_scrollbar` inner test on `line_count`: a dead branch, with no effect.
- `hud_portrait_get_color` case 3 (already on the page): not counted.
- `game_abandon_to_menu` dereferencing `mp_local_player` when `cur_character` is dead (`0x421d7e`). In single
  player that pointer is NULL, but an all-dead single-player party goes to HUD mode 5 (`0x4394ee`), which runs no
  key handling. Multiplayer has the pointer. *Unclear* rather than disproved, because we have not proved that the
  play menu can never open in mode 5.

- Backstab halved when the target is in combat mode (`je 0x4651b1` at `0x465193`): shared with the aimed
  attacks' `x 0.5`, and consistent with x2.5 against an unaware target. A design choice, not a slip.
- Status 16 blocking heals and dropping damage beyond its value (`0x450abc`..`0x450b1d`): consistent for a full
  shield; intent unknown, not counted.
- `spell_roll_amount` letting a resistance above 1 turn spell damage into healing (`0x47f583`..`0x47f592`): it
  looks deliberate (absorption), not counted.

- `cmd_stop_sound`'s help text says "Stops all sounds that are playing" (`0x5949cc`), but it stops only the `play_sound` stream (`console_sound_stop` `0x43d710`..`0x43d72e`). This is a wording mismatch in a console-only command. The handle that `console_sound_stop` forgets is no growing leak: `snd_get_handle` reuses a loaded sound by name (`_stricmp` at `0x52083c`). Not counted.
- `cfg_34` reaches `gr_init` as the constructor default (`0x43db08`), because `summoner_cfg_load` runs later in `GameInit`. `options_apply_detail` does apply the saved value (`0x48734d`), and no options screen writes it (only `init_game_config_defaults` and `summoner_cfg_load` do).
- `s3d_read_door` returns -1 on success and 0 on error (`neg; sbb; neg; dec` at `0x4702e9`..`0x4702f1`): `level_load_s3d` ignores the result (`0x46e63b`).
- `spell_target_value` (cure) tests status 0 (blind) twice (`0x47f2f3`, `0x47f304`). The id it skips, 2, is Faith, a buff, so the result is right.
- `levelrender_face_vertex_colors` takes the tint brightness from the sum of R+G+B clamped to 255 (`0x48f99e`..`0x48f9ee`). `levelrender_draw_list_glide` does the same (`0x48f3d4`..`0x48f418`), and character meshes use the sum x 0.75 (`0x50bb45`..`0x50bb6d`, constant `0x580e04`), so this is the tint model, not a slip.
- `skeleton::get_root_velocity` (`0x543fc0`) has no caller either (no pointer in the image); nothing wrong was found in it.

- Evil Urath's melee "never lands" (`boss_move_in_range_dispatch` has no `0xb1` case, table `0x4a100c`): true, but Urath never makes a melee attack. A type-9 character only acts through `boss_ai_frame` (`0x4bffad`), and `evil_urath_ai_select_action` only casts one of seven spells (`0x55`..`0x5b`, `evil_urath_cast_spell` `0x4c50cb`) or leaps (`0x4c4eff`). Not counted.
- `character_unload` never freeing the shadow mesh (`0x5413b0`..`0x5413c0`), moved from "Still unclear": every character load runs with a mempool active. `GameInit` sets each party mempool before its `character_load` (`0x43e146`..`0x43e1d2`), `charinfo_create_character_instance` sets `charinfo_select_mempool`'s pool (`0x448e0e`), and that function always returns a pool (player, summon or `level_mempool`, `0x448f20`..`0x448f50`). So `character_create_base` always sets flag `0x8` (`0x5416db`..`0x5416e9`) and the free branch never runs.
- `s3d_read_color_record` green into blue: already V22.
- `s3d_read_door` returning -1 on success: already in "Checked, not bugs" (`0x4702e9`..`0x4702f1`).
- Tint from the sum of R, G and B: already in "Checked, not bugs" (`0x48f99e`..`0x48f9ee`; the Glide list and character meshes use the same model).
- Character name overflow in `character_create_base`: already C14.

- The LENT "inactive list" rule on the savegame page was reversed in the documentation, not in the game: entities of `living_list_inactive` (the dead list, `character_on_death` `0x45401b`..`0x454054`) are written unless they are summons (`0x100`) without `0x40000` (`0x4d2274`..`0x4d2286`). Dead summons are not saved on purpose.
- The script-table checksum is written (`0x4cf227`..`0x4cf244`) and skipped on load (`add edx, 4` at `0x4cf4e3`; the notes give `0x4d24e3`, which is in `savegame_write_item_objects`). Not a bug, but nothing protects a save from a different `script_filenames` table.
- The `CONT` lock byte is `lock x 100` truncated (`_ftol` at `0x4d243a`) and read back x 0.01 (`0x5818f0`). It loses at most 0.01 of the lock value. Not counted.
- `savegame_find_flag_level` (`0x4d0240`) returns a flag value, not a level; its only caller `quest_get_current_step` (`0x4d60dd`) uses it as a bool. The name is ours. Not a bug.
- The thumbnail preview misses its last 8 bytes (`0x4d2977`..`0x4d298b`); already on the savegame page as cosmetic. Not counted.
- `level_time` swap: already listed under "Claims that did not hold".

- `bm_is_user_bitmap` "inverted": it returns true for type-3 entries **not** named "USERBMAP" (`0x5026c0`..`0x5026d0`). `bm_create` names every entry "USERBMAP" (`0x503d15`) and `bm_create_missing` renames its own to the missing file (`0x5030e6`..`0x5030ec`), so the function consistently means "is a missing-file placeholder" (and for handle -1 "was any placeholder made", `0x502699`). Only the name and the sum.symbols comment are wrong. It has no caller and no pointer in the image.
- `bm_create_missing` never reusing its placeholder: each missing name needs its own entry, because later loads find the bitmap by that name (`bm_find` at `0x50273d`); the entries share the static pixels `bm_missing_bitmap_pixels` (`push 0x25c5c8c` at `0x5030b7`), so each costs one table slot. `bm_missing_created` is only a flag for `bm_is_user_bitmap(-1)`. The sum.symbols "once" is wrong.
- `ai_path_node_create` never writing `pos.y` (`0x4a03d6`..`0x4a03f7` store x, z, layer and next only): the height of a path node is never used. `ai_path_find` sets each step's `dir.y` to 0 (`0x4a027c`), `ai_follow_path` overwrites it with 0 after computing it (`0x4b3181`, `0x4b33ca`, `0x4b3407`), `ai_path_begin` compares x and z only (`0x473ad7`..`0x473ae5`), and the copy to `ai+0x218` (`0x4b3127`) is never read (only writes at `0x473beb`, `0x473cdc`, `0x473dd9`, `0x4b3127`, `0x4b3882`).
- Red and blue in OpenGL screenshots and thumbnails: no swap. `gr_opengl_read_backbuffer` reads `GL_RGBA` / `GL_UNSIGNED_BYTE` (`push 0x1908` at `0x5502f0`), keeps the byte order in its row flip (`0x550340`..`0x550363`) and returns 7 (`0x55039a`). Only OpenGL returns 7 (`gr_d3d_read_backbuffer` 3 at `0x53a8b3`, Glide 3). The JPEG path copies bytes 0, 1, 2 as R, G, B (`0x506492`..`0x5064a5`; the 24-bit case swaps bytes 0 and 2 at `0x5064ec`..`0x5064f3`, so `jpeg_write_file` wants RGB), and the thumbnail writer puts byte 0 into the 565 red bits (`0x4271c6`..`0x4271eb`). SummonerFix's `glReadPixels` hook keeps RGBA order (`display.c`, `Hook_glReadPixels`). Format 7 means R, G, B, A for a read-back buffer, while 8888 *bitmaps* are B, G, R, A in the texture converters; the two never meet. The exception is the console `targa` path (K15): TGA wants B, G, R, so under OpenGL its colours are also red/blue swapped on top of K15's channel shift, and OpenGL is the renderer that "reports 32 bits" there (SummonerFix makes that the normal renderer).
- `gr_opengl_init`'s error text naming `gr_direct3d_init` (`0x54f702`): a copied message; part of C50.

- The "At least 8 MB of available video memory required." check tests 5 MB (`cmp eax, 0x500000` at `0x53a01f`): `GetAvailableVidMem` runs after the primary, back and z-buffer surfaces are created (`CreateSurface` `0x539771`, `0x5397c5`, `0x539898`, `0x539998`), so it measures free texture memory. An 8 MB card at 800x600 in 16 bits keeps about 5.1 MB after three 0.96 MB buffers. Glide shows the same message when its memory value at `0x2f0f218` is below 4 (`0x549e58`..`0x549e76`, unit not traced). The threshold matches an 8 MB card; only the wording is loose. Not counted.
- Direct3D device choice: skipping hardware T&L devices and taking the last remaining one (the HAL when there is one, else the software device) is deliberate (see V61). Not counted.
- `gr_d3d_read_backbuffer` returning format 3 and the reversed depth test on non-S3 cards: already noted on the page or design choices (the depth test is set on purpose per vendor, `0x539ed6`..`0x53a016`).

- B7 `species_is_hero` has no lower bound (`cmp ecx, 0x3; setle al` at `0x449a26`): every species id in `species_table` (`0x580e60`) is 0 to 225 and `characterinfo_parse_name` only takes ids from it, so no negative species reaches it.
- B8 console command `charinfo` does nothing: `init_ccmd_charinfo` registers `empty_function` (`0x4f13e0`, `0x449bb0`). Its help text promises a dump; like the other empty commands it looks like a debug feature compiled out. Not counted.
- B10 the hurt-drop loop reads `m->id` after `move_info_free`: the free only writes `+0x40` (`0x446770`), and the ten names are distinct, so no second match and no second unlink.
- B6 first part, `+Drop:` keeps 5 of 17: a compare against 5 (`0x446420`), no overrun. No retail record has more than five `+Drop:` lines. `+Multiplayer Drop:` stops at 17, the array size (`0x4464c4`).
- B3 second part, `statscreen_unload_anim` without a NULL check (`0x44917d`): it runs only when `Character +0x5f0` is not -1 (`0x412e76`). The stat-screen moves have no class line (class 1), and every hero mask includes bit 0 (`mask |= 1` in `characterinfo_load_party_anims`), so the loaded light or heavy move is always found. A weapon-class reload resets `+0x5f0` through `character_reinit_anims` and `character_fill_locomotion_anims`.
- The hurt-move drop itself (`0x44631a`..`0x446372`): an explicit list of ten names, applied to every non-boss record. It drops 354 of 3302 retail moves, but it reads as a design choice, not a slip. Not counted; documented in game-tables.md.
- B2 for heroes in multiplayer (move_used_on_level skips their "stand" move there, so the loaded-only lookup would fail): *inferred* unreachable, since `level_is_town` is forced to 0 with flag 2 and multiplayer loads the full set, so the templeint branch does not run either.

- `ai_pick_attack_slot` (`0x472780`) returns the best direction whether or not the slot is taken. Its only caller,
  `ai_approach_attack_slot`, first lists the slots that are free or its own (`obj_from_handle` and the handle
  compare at `0x4bd44a`..`0x4bd4f7`), takes the nearest of those (`0x4bd51b`..`0x4bd575`), and uses the picked slot
  only to route around the target when the two are two slots apart (`0x4bd5a3`..`0x4bd5d3`). `ai_claim_attack_slot`
  does take a slot over only when the occupant has more time left (`0x472966`..`0x47297a`). By design.
- Goal targets after a load: `ai_state_load` gives a goal the handle of the first living entity whose name hash
  matches (`0x47486d`..`0x4748e5`). `savegame_read_character` matches the whole character record the same way
  (`0x4cf7ac`..`0x4cf7e2`), and the script editor flags duplicate names ("*** DUPLICATE NAME ***",
  `script_write_level_script`). With duplicate names the record load would already go wrong; the goal target adds
  nothing. Not counted.
- `ai_set_mode` not freeing the path: no leak. Every new path frees the old one first (`ai_set_path` `0x473bca`,
  `ai_path_find_new`, `ai_path_to` with a bad layer), and `ai_state_shutdown`, `ai_reset` and
  `ai_reset_party_member` free it.
- `ai_do_frame` has no time budget: a documentation error, not a game bug. Off-screen characters are throttled in
  `ai_update_perception`.
- `slowdown_scaler` (`0x59af54`) is read by `ai_frame` (`0x472ce9`), `Ai_path_wandering_enabled` (`0x59af58`) by
  `ai_follow_path` (`0x4b355b`) and `Ai_paused` (`0x22e499c`) by `character_ai_suspended` (`0x462de0`) and two
  others. They are not dead.
- `ai_goal_next_id` (`0x59b7c8`) is saved and loaded (`savegame_write_living` `0x4d218d`,
  `savegame_read_living` `0x4d2e16`), so loaded goal ids do not collide with new ones.

- NOTES item 7: `gr_draw_fade` calls `gr_set_color` twice with the same value (`0x5066b0`); the second call returns early because the colour did not change (`0x506b90`). Harmless.
- NOTES item 8, first part: `gr::set_clip` tests the right edge with `>=` and the bottom edge with `>` (`0x506840`); when the values are equal the assignment gives the same number, so the result is identical.
- NOTES item 1, second part: the windowed `gr_init` window is centred horizontally but placed at the top of the work area (`0x505f9f` branch). A layout choice, retail runs fullscreen, and SummonerFix replaces the window position (`display.c` `Hook_SetWindowPos`).
- `gr_is_mode_supported` under Glide: `glide_multitexture_supported` (`0x2f132cd`) looks inverted, but the flag is set to 1 when there are fewer than 2 TMUs (`0x549e8d`, `ebx = 1`), so the test is right. Only the name is wrong (symfix.txt).
- Gamma applied to the alpha of 8-bit-alpha textures on all renderers (`0x54c233`, `0x5486bd`, `0x54d5f3`): for the white additive textures this changes their brightness, which is probably the intent.
- `cmd_gamma` (`0x43d520`) only stores `config_gamma`, so `gamma <x>` in the console shows no change until the options are accepted or a level loads. The help text says only that it "Sets" the value; working as written.

- `character_load_state` setting `attachments[0].slot = 1` for species `0xce` (`0x459df0`..`0x459dff`): not a slip
  for the count. `summon_emerge_update` sets the same slot to 1 for species `0xce` when its emerge move's trigger
  2 fires (`0x4b4743`..`0x4b475b`), after `living_entity_init` gave it slot 4 (`0x44ea0e`). The load restores the
  attach point the summon has after it emerged.
- `character_move_has_trigger` without a NULL test (`mov edx, [ecx+0x6d0]` at `0x461644`): `cur_move` is NULL
  only together with `cur_anim = -1` (`living_entity_init` `0x44e688`, `ai_reset` `0x47258e`). Both callers test
  first: `character_on_death` needs `cur_anim >= 0`, `character_play_move` needs `get_current_move() != NULL`.
  `character_move_trigger_time_left` has its NULL test (`0x46167c`). Reading an ended move's trigger time gives a
  negative value, which the callers treat as "passed".
- `character_size_height` dropping the boss height (notes bug 2): already K30.
- The resist tests mixing `<=` (status block, `0x45e9c3`) and `<` (eleven other handlers): the difference is a
  single `frand()` value out of 32,768. Not counted.
- The leech, conversion and apdrain handlers testing the resist value without a `+Hit Chance:` line: the parser's
  default hit chance is 1.0 (`mov [esi-0x1c], 0x3f800000` at `0x47a294`), so the resist value is the normal
  `0.5 + magic - 0.04 x power`, not 1.
- `character_get_resistance(3)` uses `character_get_skill(0x23)`, which returns -999999 for a locked skill. Magic
  resist is in all four hero progression tables (Joseph level 13, Jekhar 14, Flece 16, Rosalind 5), and
  `player_skill_unlocked` does not test the level, so it is never locked for a hero.
- From "Still unclear" on the page: "Dead party members: `character_save_state` and `character_load_state`
  have no case for type 8". A dead hero is not type 8. `character_on_death` changes the type only for a
  character that is not type 7 or has the summon flag `0x100` (`0x453f00`..`0x453f16`); the only store of 8 is
  `0x453f94`, for a dying summon. A dead hero stays type 7 with flag `0x80` and is saved by the type 7 case. Dead
  summons are not saved anyway (see the LENT line above). This item can move here.

- `party_set_level` "loops forever": its round-robin spend loop (`0x4cece0`..`0x4ced47`) never runs with points. It calls `character_add_xp(xp, 1, 0)`, which already spends all points for type-7 characters at +10 each with the same loop (`0x44fecb`..`0x44ffa6`). That loop could only hang if the points outnumbered the room left in the unlocked skills. At level 30 (the highest `game_stage_set` asks for) a member has 71 points (`experience.tbl`), and 13 to 17 progression entries starting at 10 leave 117 or more.
- `party_count_item_by_name` counts equipment slots 0..8 only: those are the slots a party member saves (`savegame` 9 x 4 at `+0x06c`). Slot 9 is the monster and summon second weapon (K17).
- `level_script_leave` frees the avoid splines without clearing the slots: `level_script_reparse` clears them on the next load (`0x4cb427`). Only the restart path (V111) keeps them.
- `script_object_talk` calls `conversation_is_idle` twice in a row (`0x4ce646`, `0x4ce654`): the second call gives the same answer. Not counted.
- The messages that are only formatted into a buffer ("\"%s\" not found.", "No level item entry ...", "Couldn't find level trigger", "Unable to locate character ... for shopkeeper support", "Ambient sound navpoint not found", "Tripped trigger", the string pool usage) are debug output that the release build does not print. Not counted.
- `level_check_triggers` compares only 4 characters of `"rand-"`: every retail level name starting with `rand` is an encounter map, so the effect is as intended (corrected on [World map](worldmap.md)).
- The door sound third value (0.1 becomes 1.0, then truncated to a byte): every retail `+Open sound:` / `+Close sound:` line has 1.0 there.
- `script_party_remove_item` indexes `item_types[-1]` for an unknown name before its `type >= 0` test (`0x4cdfa6`..`0x4cdfc7`): `party_inventory_find_type` only compares the pointer (`0x46bd19`), and the test then skips the removal.

- Revival only in towns (notes item 3): `party_level_reset` revives and heals only when the new level is a town or
  `worldmap1` (`0x437f72`..`0x437fa6`). This is a consistent rule (the towns are the rest stops), not a fault; the
  [Party](party.md#death) page said every level change revives and has been corrected.
- `party_member_join` keeping a `Player` slot when `player_create` fails (notes item 8): the `index < 0` test after
  `player_alloc_slot` (`0x439c7d`) repeats one already passed at `0x439bce`..`0x439bf5`, and `player_create` can only
  fail when `living_entity_create` returns 0, which it never does (it loops on `DebugError` when the pool is full,
  `0x461bba`..`0x461bda`, and always returns the record, `0x461daa`). Both branches are dead.
- `level_load_report` formatting three lines and never printing them (notes item 6, `0x437da3`..`0x437de4`): a debug
  print compiled out, like `cmdline_take_argument`.
- `level_load_layers` discarding `lobj_get_layer_n(1)` (notes item 7, `0x43896f`): a dead call. The logic it keeps is
  consistent: an object listed in a second `$layerNN` record gets that layer as its layer 1 unless it is already its
  layer 0 (`0x438974`..`0x438983`).
- Every door opened on a multiplayer level load (notes item 9, `level_restore_doors` `0x438a21`..`0x438a70`): done on
  purpose for every door, open and stored in the two-stage state; the `savegame_two_stage_get` results at `0x438a58`
  are unused, which changes nothing.
- `gameplay_no_render_60a630` never written (notes item 5): its two readers (`0x43275b`, `0x43930d`) only guard a
  debug branch (`frame_sleep_when_inactive`); a cut feature, not counted.
- States 5 to 8 never entered (help screen, a character type picker, debug help text, debug load screen): no
  `gameseq_set_state` caller passes 5, 6, 7 or 8. Cut debug screens, like `camera_cut_to_view_stub`; not counted.
  State 6 only fills a list box with the `char_infos` names; the monster layout files are handled by the
  `load_mlayout` / `save_mlayout` console commands (C203, V200), not by that state.
- `Full_party` cleared on `masad` (`level_spawn_party` `0x438adb`..`0x438af4`): the story starts with Joseph alone in
  Masad Village, so the test is deliberate. It does mean the cheat never works on a new game ([Party](party.md)).
- The help screen's debug key table (`0x5940ec`, six `<debug>-x` pairs) is never drawn: only the one page function
  at `0x59412c` exists. Part of the unreachable state 5 anyway.

- Wolong caverns, Joseph's "Not yet! I must find the Tail of the Rokhul." / "The Ring of Water must be around here somewhere." (notes, bug 5): `lsf_wc_nali_used` does call `script_party_member_say` every frame at `$healpoint` (`0x4f6e5a`..`0x4f6ed3`), but `character_say` only rewrites the speaker's text slot and its 5 s time (`0x461e3b`..`0x461e59`, no sound call). The line simply stays up while the party stands there and fades 5 s after it leaves.
- Wolong caverns poison trap loop (notes, bug 4, second half): the three-pass loop compares the same stored distance (`0x4f6bc4`, `0x4f6bce`..`0x4f6be3`). A target in range breaks out on the first pass, so each target is hit once.
- Flag-only remarks (notes, item 7): `lsf_catacombs_flag_way_comment` (`0x4fe330`) and `lsf_liangshan_flag_lair_comment` (`0x4f4f40`) set their flag and say nothing. No dialogue or script table reads either flag, so this is a cut line with no effect. `lsf_sewer_flag_scene_a` and `lsf_sewer_flag_found_crate` are flag-only by design.
- Palace servant line (notes, item 8): `ls_lpalaceint_sneak_frame` calls `rand()` and ignores the result before it says the single servant line (`0x4f3365`..`0x4f336a`). Harmless.
- `ls_sewer_init` comparing `level_prev_filename` with "lenele1d" (`0x4ed86f`..`0x4ed88b`): both outcomes call `game_stage_set(0)`, so the test has no effect. Probably removed code, not a bug.
- Catacombs king tombs (notes, item 11): using `cry_kinge` or `cry_kingf` while the other is open opens `cry_tombcen` whatever the other tombs show; the 2000 XP needs A-D and G-M closed and `main_crypt_open` unset (`0x4fe674`..`0x4fe7a2`). The level has no `cry_kingj` (catacombs.s3d), so the list is complete. A bonus for the exact solution, not a broken test.
- `ls_sewerboss_golems_done` not reset by `ls_sewerboss_init`: it is cleared by the TB-Intro end callback (`0x4ee36c`) before every fight, and in a fresh session it only repeats `camera_set_mode(4)` and the "Sewer" music once after the golems are already dead. No effect.

- S2 `+Plane:` in `script_parse_navpoint` / `script_parse_navpoints`: the parsed value is always replaced (level and cutscene navpoints get `lobj_pick_layer`'s layer of the floor under them, `0x4db6a0`; camera and ambient navpoints -1, `0x4db6d3`; `ParseTypeString` errors on an unknown type, so no other type reaches the `ja` at `0x4db560`). This is not a lost setting: the script editor writes `+Plane:` from the same computed layer (`script_write_navpoint`, `0x4e9c14`..`0x4e9c26`, only when it is not negative), so the retail `+Plane:` lines (about 14,800 in the script tables) are a record of what the loader computes again.
- S6 `script_entry_start` "die": end mode -1 is set even when `script_action_die` fails (`0x4dd3a5`), so the 5-second retry never runs. It fails only for a move index of -1 (`0x4dc999`), which a retry would not change. "Unable to start script entry ..." and "Unable to find prop ..." are formatted into buffers and never shown (debug text). The retail "die" actions pass 34, 1 or 18, and the parser falls back to the death state move (`0x22`) before it stores -1; whether every such character has one was not checked.
- M2 `chat_draw_lines`: a whisper line without a space would be drawn all blue instead of name-highlighted, but `chat_format_entry` always writes "<name> " in front of a whisper.
- The `antialiasing`, `lmap_off`, `lmap_reset` and `whee` callbacks (`0x505db0`, `0x46e0c0`, `0x46e140`, `0x54b960`) and `cmd_summon` do not test `calling_a_command_function`, so `? <command>` and `help <command>` also run them. Developer commands with no description, and there is no console; not counted.
- `init_char_types` tests `char_types_initialized` (`0x25db91c`), which nothing sets; it has one caller (`LoadSettings` `0x43da51`), so the table is built once.
- `make_lowercase_token` lower-cases one byte before it tests for the end (a do-while), so an empty token also lower-cases stale bytes after its NUL; they are past the terminator and never read.
- `chat_format_entry` ignores its size argument (D2301): its output is a name plus at most 255 characters, inside the 512-byte buffer of `chat_update_lines`.

- The Glide "At least 8 MB of available video memory required." test: `gr_glide_init` quits when `fbRam`, the frame buffer memory in MB of SST 0 (`GrHwConfiguration` +8, `0x2f0f218`, filled by `grSstQueryHardware` at `0x549dd8`), is below 4 (`0x549e58`..`0x549e7d`). That is the frame buffer alone, not the whole card. *Inferred:* a 3dfx card with a 4 MB frame buffer has 8 MB or more in all, except boards with less texture memory such as a 6 MB Voodoo 1. Only the wording is loose, as for Direct3D. The value is the same `gr_glide_video_memory_mb` that K64 calls "probably MB": it is MB (`0x549ea2`). The Glide renderer needs a 3dfx card or a Glide wrapper.
- `gr_glide_init`'s `glide_clip_cursor(1)` when the close-on-deactivate byte `0x2f132b8` is set (`0x549d9a`..`0x549da3`), before it sets that byte (`0x549e9c`) and the clip rectangle (`0x549ee2`..`0x549f01`): on the first init the byte is 0, so nothing happens; on a later init it uses the previous rectangle of the same mode. Not counted. The byte needs 2 or more TMUs and `fbRam >= 12` (`0x549e93`..`0x549e9c`). *Inferred:* no Voodoo 2 reports a 12 MB frame buffer, so this is not "Voodoo2 only" as the gr_glide notes say. Which cards and wrappers set it was not checked.
- `gr_glide_deactivate` calls `grSstControl(2)` and then increments the pending-activation counter `0x2f132ac` (`0x54a6b9`..`0x54a6c7`), which the next `gr_glide_flip` would turn into `grSstControl(1)`. Its only caller is the fatal error path (`gr_deactivate` `0x5061a2` from `debug_error_deactivate_gr` `0x531ca0`), which does not flip again. Not counted.
- Titus and Luminar intros replay on re-entry: `ls_ikaemosbottomint2_init` clears `titus_intro`, `titus_combat_enabled` and both done bytes on every entry until `trapped_titus` (`0x4eeea2`..`0x4eeec7`), and `ls_ionaext02_init` on `Ionaext02_v2` clears `luminar_intro` until `trapped_luminar` (`0x4f45cb`..`0x4f45dd`). `titus_combat_enabled` is only set when Titus-Intro ends (`0x4ef0a2`), so the replay is how the fight is re-armed. A deliberate restart of the boss sequence; the intro on the first frame (no navpoint, `0x4eeef0`..`0x4eef2e`) is how the level is built.
- LPalaceInt02 world map exit after a reload: `ls_lpalaceint02_init` leaves `worldmap1` an exit when `sornehan_dead` and `galienne_dead` are both set, even without `got_scrolls` (`0x4f2499`..`0x4f2511`). But no such save can exist. Saving is refused while `level_script_event_active` is set (`savegame_location_allowed` `0x4d4a50`..`0x4d4a5a`). The frame sets the byte when SG-Intro has played (`0x4f2587`) and clears it only at `bothdead` (`0x4f25d3`), which needs the scrolls (`0x4f275b`..`0x4f2773`). The two death flags are set only after SG-Intro (`0x4f25e9`..`0x4f26c7`), and `worldmap1` is the level's only exit (retail `LPalaceInt02.tbl`). Not reachable.
- IkaemosTopInt dagger slots: `ls_ikaemostopint_use` sets `ikaemos_slotN` on any use of Puzzle-SlotN (`0x4fa230`..`0x4fa36d`), but the slots are doors with `+Locked: 0.00` and `+Key:` Brazen / Cupric / Argent / Auric Dagger (retail `IkaemosTopInt_script.tbl`). `object_try_unlock_or_open` unlocks such a door only with its key in the party (`0x43c0a3`..`0x43c105`) and says "Locked, Cannot Pick" for a zero lock value with a key (`0x43c1a1`..`0x43c1be`). "We've unlocked it already." on a second use is right.
- `ls_templeint_act2_outro_cs_end`'s "Switching char." / "No character!" `printf` lines: debug output with no console in retail; harmless.
- lenele3d's 1 s `ls_lenele3d_timer` (set by `ls_lenele3d_init`, never read) and the dword `0x257e9dc` (written 0, never read): remains of a removed sequence, no effect.
- The Eleh choice bytes `0x257e9ec`..`0x257e9ef`: the code is right (TopFloor to entry 5, GroundFloor to entry 3, BottomFloor to entry 4, `0x4f176a`..`0x4f178a`; JoinJoseph handled in `ls_eleh_frame`); only the sum.symbols names are shifted (symfix.txt).

- `mp_send_player_data` sending PLY_DAT "unreliably" (`multi_send_packet(..., 0)`, `0x4af96d`): `multi_send_packet` has no unreliable path. Every packet goes into the receiver's reliable buffer and out through `nw_NewSendReliable` (`0x442175`..`0x4421a5`). The last argument only sends the buffer at once (`0x4421e6`..`0x44221d`), so PLY_DAT goes out with the next flush or when the buffer fills.
- `mp_handle_player_data` clamping HP (`0x4afb23`..`0x4afb3e`) and AP (`0x4afb65`..`0x4afb75`) at 0 but not at the maximum: the values are the sender's own current HP and AP (`+0x5f8`, `+0x5fe`, `0x4af8e3`, `0x4af917`), so the sender has already capped them.
- `mp_handle_net_info` not reaching `mp_set_local_status(3)` when `mp_add_new_player` returns NULL (`0x4af781`): NULL comes only from a full player pool, which also starts the leave with reason 3 (`0x4af5e0`..`0x4af5e3`), or from the receiver's own id, which `mp_send_net_info` never sends to that player (`nw_addr_equal`, `0x4af6a2`..`0x4af6ac`).
- PLY_DAT sending 0x54 of the skill bytes at `+0x614` (`0x4af867`..`0x4af881`): there are 84 (0x54) skills ([Characters](characters.md)); the rest of the 0x60-byte array is not skills.
- `determine_point_visibility` not flushing the pending points when the last listed point is in a group (`0x4b8530`..`0x4b8532` jump to the end): the sweep is over, and the edge lists are freed right after (`0x4b8595`..`0x4b85d3`).
- `cd_has_level_data` using its buffer as both the destination and the `%s` argument of `sprintf` (`0x43e95a`..`0x43e969`): with this CRT the string is copied onto itself and "\eleh.s3d" is appended.
- `cd_root` declared `char[64]` with a 0xdc-byte .bss gap: `cd_set_root` writes at most 4 bytes.

- `RecordExceptionInfo` returning `EXCEPTION_CONTINUE_SEARCH` on every path (`0x541a99`, `0x541ea9`): this is what Bruce Dawson's original `ExceptionHandler.cpp` does on purpose, so Windows' own crash handling (or a debugger) still runs after `errorlog.txt` is written. Reporting only the first exception (`0x2ee250c`) is also the original's design. Only the docs and the `summoner.cpp` comment were wrong.
- `interface_frame` reading `cutscene_data +4` without a NULL test when `game_flags & 0x20` (`0x4119d8`..`0x4119dd`): `cutscene_play` sets bit `0x20` (`0x43659b`) only after `cutscene_load_data` succeeded and the data was already used (`0x43635f`..`0x436364`), and `cutscene_cleanup` clears the bit (`0x436691`). No write sets `0x601e68` to NULL while the bit is set.
- `packfile_add` testing `name == NULL` and the name length only after the `_stricmp` loop and the `fopen`s (`0x5344f8`..`0x53450f`): all eleven callers pass constant `.vpp` names (`packfile_init` `0x533e30`..`0x533e7d`, `level_load` `0x4385f5`..`0x438609`).
- `packfile_fseek` adding the entry's offset for every origin: its only caller, `File__seek`, converts `SEEK_CUR` / `SEEK_END` to an absolute position itself and always passes origin 0 (`push 0x0` at `0x5149ac`).
- `packfile_fclose` / `packfile_fseek` / `packfile_fread` not testing the CD stream for NULL: `packfile_fopen` returns 0 when the CD `fopen` fails (`0x534737`), and its one caller `File__open_found` stops on 0 (`0x51457d`), so no CD entry is used with a NULL stream.
- `mp_handle_door_toggle` / `mp_handle_door_lockpick` indexing `lobjs` with the packet's value without a range check (`0x4aa784`..`0x4aa79b`, `0x4aa813`..`0x4aa824`): needs a modified peer; not counted, like the other handlers that trust the peer's counts.
- `version_read_nfo_file` processing the last line twice at end of file (`0x411135`..`0x4111c0`): it keeps the last non-comment line, and the repeat copies the same line again.
- The version check ending in result 3 ("There was an error verifying your version ...", `0x41103a`): the pxo.net URL (`version_check_fetch` `0x410cf0`) no longer exists; that is the service, not the code. The `SkipVerify` registry DWORD skips it (`0x410f6f`..`0x410f96`).
- The mouse button accessors not range-checking the button and `mouse_time_since_press` having no `mouse_inited` test: of the 189 calls to the seven accessors, 153 push a constant 0, 1 or 2 directly and the rest pass register copies of such constants; the static button records are valid before `mouse_init`.
- The message box's 1024 background `MessagePopupBckgrnd21024.tga` against the 640 `MessagePopupBkgrnd2640.tga`: both names exist in `summoner.vpp`.
- `0x58be28` (screen width / 640) written by `interface_init_global` (`0x41173b`) and read nowhere: a dead store. Not counted.

- Notes item 5, pings sorted with a float compare: the samples are `last_packet_received - echoed send time` in ms
  (`0x5181b9`..`0x5181c9`), non-negative unless a peer echoes garbage, and non-negative ints compare the same as the
  floats with their bit patterns, so `float_compare` (`0x517dc0`) sorts them correctly.
- Notes item 9, French without a font-to-Windows direction: all 11 callers of `localize_remap_charset` pass 1
  (`push 0x1` before `0x408787`, `0x44b0a6`, `0x4cdc8d`, `0x4d3ef8`, `0x4dc32c`, `0x4ff481`, `0x50446f`, `0x504633`,
  `0x50476a`, `0x504ab8`, `0x504c61`), so the German reverse table is unused as well.
- Notes item 11, no bound on the 32 handler slots: `os_add_msg_handler` (`0x530c20`) has three callers, `mouse_init`
  (`0x500e68`) and `key_init` (`0x5097e8`) from `LoadSettings`, and `gr_glide_init` (`0x549ecd`) from `gr_init`, itself
  called once from `LoadSettings` (`0x43db4d`).
- Notes item 11, `ReleaseMutex` on the `OpenMutexA` handle and no `CloseHandle`: the path ends in `exit(1)`
  (`0x530d95`..`0x530db9`), so nothing is left behind.
- Notes item 12, "Spirit Of Laharah": the summon's character table is `Spirit Of Laharah.tbl` with
  `$Character: "Spirit Of Laharah"` (tables.vpp). `living_entity_create` names characters `"%s#%d"` from the character
  info name (`0x461c94`), and `character_get_display_name` cuts at `#` / digits before `localize_script_name`
  (`0x4620d0`..`0x4621a5`). So the capital O key is the one that matches; "Spirit of Laharah" is only the display text
  (string 134).
- `localize_table_lookup` compares the hash only (`0x5041b0`): no two different names in the retail item, quest or
  any level's topic or script section share a hash.
- `nw_init_networking` with "fast": skipping RAS leaves `nw_networking_ok` at 0, but `mainmenu_check_network` only
  tests it for status 1 and 2 (`0x41dae8`..`0x41dafa`).

- `gr_info`, `show_hud_info` and `net_stats` show nothing, and neither does the "Screenshot written to" message. True, but these are compiled-out debug features, like `camera_key_extra_axis`: nothing writes `gr_info_total_time` (`0x2396c90`) except `FrameEnd`'s reset (`0x432c70`), so the `gr_info` block (`0x432a78`..`0x432c9b`) never draws; `Gamewide_show_hud_info` (`0x5fc021`) and `Gamewide_show_net_stats` (`0x5931b0`) have no reader outside their own console commands (`0x43302a`, `0x43320a`); `screenshot_save` hands its message to the bare `ret` `0x4f13e0` (`0x43264f`), and `game_status_message` (`0x5fbf54`) and its timer (`0x5fbf48`) are only read by `FrameEnd`. No console in v1.40. Not counted.
- `screenshot_save` "takes the number from the last name the scan returns, not the highest": the scan is sorted (mode 1, `0x432539`; `0x513507`..`0x513519`), so up to `screen999` the last name is the highest. The real numbering defect is K193.
- `cutscene_active_count` leaking on a failed load: `cutscene_load_sounds` increments it (`0x435009`), but every failure in `cutscene_load_data` returns before that call (`0x4353f7`, `0x435417`, `0x435437`), and `cutscene_cleanup` decrements it (`0x436647`) only after a load that set bit `0x20`.
- C7 through `editor_dialog_show`: all three callers pass window flags `0xb` (`0x4ea0b8`, `0x4ead94`, `0x4eadcb`), `editor_dialog_show` adds `0x20` (`0x430ebc`), so its windows never get `0x10` and C7 cannot be reached from it either.
- The `short` length read with `"%d"` in `cutscene_parse_table`: `sscanf` writes four bytes into the slot at `ebp-0x270`, and the next local (`shot`) starts at `ebp-0x26c`, so nothing is overwritten.
- `cutscene_events_frame` testing the shot range twice in a row: the second test is redundant and harmless.
- Shot effects and minitracks stop at the first record whose shot is later than the current one, so an unsorted list would skip effects; every retail entry lists its shot effects in shot order. Not counted.
- `loadgame_window_unused` (`0x5fb600`), constructed and destroyed only: a dead object, no effect.

- disp_rate refusing its help and status in single player and listing the names without a separator (notes item 7): true in the code (`game_flags` test before the help test, `0x442390`; one `print_to_console` per name, `0x44253c`..`0x442557`), but `print_to_console` is a bare `ret` (`0x50a5a0`), so nothing is shown in either mode, and v1.40 has no console. Not counted.
- "The NEW_INF skip eats a flush" (notes item 8): `mp_broadcast_new_info` sets `0x40000000` and calls `mp_broadcast_packet(..., 1)` at once (`0x4af37f`..`0x4af39b`), which clears the bit for a connected player in the same call (`0x441ad1`..`0x441aed`), so the frame-end `mp_broadcast_packet(NULL, 0, 1)` (`0x4413c0`..`0x4413c6`) finds no flagged player. Only a new player without a socket at that moment keeps the bit (the socket test comes first, `0x441ad1`), and then the next skip call skips one flush of a buffer that is sent with the next one. Not counted.
- The lobby connect step's NULL profile (notes item 4, second part): reachable only after C202's crash on lobby entry; folded into C202.
- C200's leaked records do not block a hosted game: `mp_server_check_clients` frees every listed non-local record with socket -1 on the host's first frame (`0x440c85`..`0x440cdd`), before `mp_levelselect_start_if_ready` tests `mp_all_players_have_status(3)` (`0x43ec71`).
- The net-rate copy of `multi_process_packet` for valid types: "INC R " plus a name of at most 7 letters ends at `esp+0x25`, inside the 16-byte buffer (`0x4418be`..`0x44191b`); only record 66 (C201) overflows.
- `effect_stream_next` returns 1, never a negative value, at the end (`0x440644`), so the `< 0` tests of `effect_stream_start` (`0x440594`) are dead code; the cursor is compared with the count right after (`0x44059b`..`0x4405a8`). Harmless.
- `irc_connect`'s -2 case in the lobby connect step (`cmp eax, -2`, `0x43f17b`, D3306): dead, since `irc_connect` returns -1, 0 or 1. Harmless.
- `multi_send_packet`'s last argument (notes, multi.h): a flush flag, every packet is reliable; already listed in "Claims that did not hold" (PLY_DAT line). Nothing new.

- Notes 4, "the five-entry candidate array overflows for a six-member group": the list in `ai_group_think_caster` has six 0x10-byte records (`[esp+0x30]`..`[esp+0x90]` of an 0x80-byte frame), so six members fit; the overflow comes only from the doubled count (C210).
- Notes 6 as a live bug: the nested `serpent_boss_load` calls of the two copies restore `serpent_has_rider` (V214).
- The decompiled `tiger_rider_set_flinch` copies `flinch_area[1]` / `flinch_status[1]` into index 2; the binary copies them into index 0 (`mov [0x2452c58], ecx` / `mov [0x2452c4c], edx` at `0x49a289` / `0x49a28f`). A decomp error, not a game bug (the tables are never read, V211).
- `ai_group_collect_enemies` filters enemies by height against `cur_character` (`0x493c71`), not the group leader as the decomp comment says; party members stay near `cur_character`, so no effect is known.
- The Phoenix Rider's on-foot `ready_hurt` slot (V212) and the empty flinch tables (V211) change nothing on screen.

- `door_find_floor_point` overflow "overwrites the cell count and the loop variables": the loop variables are below the array (`[esp+0x10]`..`[esp+0x1c]`); the count at `[esp+0x4c]` is rewritten at `0x476bb5` before it is read. The overflow itself is real (C231).
- `camera_smooth_height` "at frame times above 0.25 s": the frame time never exceeds 0.25 s (`FrameDone`, `frametime_initialize` `0x5014ee`; `frametime_unlimit` has no caller), so the wrong sign is latent (K231), not an occasional visible step.
- `ai_goal_compare` returning -1 both ways for two combat-mode / sneak goals (`0x475d6c`..`0x475d82`), and 1 both ways for equal priority and time (`0x475d97`..`0x475daa`): qsort over 5 entries still ends, and the order of two such goals does not matter. Not counted.
- `door_start_move`'s test of bit `0x4` (`0x476ff0`): always false, because `0x476f85` clears it and the value written back keeps it clear. Dead code, not counted.
- The SRV_REQ "unknown service request type" warning is only formatted into a buffer; `geom_avoid` / `Camera_geometry_avoidance`, `camera_occluders_unk` (`0x22e4acc`) and `camera_set_default_view_params`' values are never used; `zoom_limit`'s texts lack a newline. Dead or debug code, like the entries under "Not counted" on game-bugs.md.
- `ai_service_request` having no single-player branch (`0x4766fb`..`0x47670e` skips everything outside a multiplayer client): `hud_key_pause`'s request only matters to a multiplayer client; a symbol correction, not counted.

- `script_write_objects` writes `+Trigger:` with its own `"\n"` (format `0x5a1290`, `0x4e9f2f`): an extra empty line in the record; the parser skips blank lines. Not counted.
- `script_editor_level_item_ok`'s name loop (notes: "possible endless name loop"): a clash at index i always gives the name for i + 1, and only one index can produce a given name, so the search ends after at most `level_item_count` restarts (`0x4ecd47`..`0x4ecdbe`).
- `script_editor_show_door_lock` reading a NULL record: its only caller adds an `lc_entries` record first when none exists (`lc_entry_exists` / `lc_entry_add`, `script_editor_handle_input`), and `lc_entry_add` has no limit and always returns a record (`0x4cd390`..; its overflow is C113). Not reachable.
- `script_replace_navpoint_references` updating only the first character that starts at a navpoint: the editor refuses a second start on the same navpoint (`script_editor_place_creature`) and no retail `_script.tbl` gives two objects the same `$Start position`. The real gap is the dead characters (G223).
- `cmd_ghost_unlink` (first Ghost Rider of the live list, `0x4b9f00`..`0x4b9f2f`) against `ghost_find_rider` (last, `0x4b9fe0`..`0x4ba011`): every retail script that has a Ghost Rider has one (masad, jadetemple, jadetemple_v2, Rand-Forestnite1_v2), only "Ghost Rider" has species `0x5a`, and the console commands cannot be typed in v1.40.
- `obstacles_debug_render`'s offset-line mode draws the second corner's offset from the first corner (`fadd [esi + 0x270]` at `0x4a3c7c`): true, but the mode switch `0x245ce08` has no writer (`asm.mjs grep`), so the branch never runs.
- `ai_consider_blind_or_silence` "inflating every later record" (notes bug 5): the scores there are a running sum (`0x4bef2a`..`0x4bef2f`), so multiplying the total by 1.5 at `cur_character` (`0x4bef16`..`0x4bef24`) widens only that character's own band (by half the total before it); the later bands keep their width of 10. See the unclear list for the intent.
- `pf_bsp_node_find` looping on a node type other than 1-3 (notes bug 7): already C161.

- `titus_cast_special` reading `titus_current_move` without a test (`0x4c32e5`): its only caller chain is `living_combat_update_boss_attack` (`0x468423`) right after it started a move whose id `boss_get_attack_move_id` read from the same pointer (`0x4a20ba`), so the pointer is set; the shared reads of every boss's current move there are not separate bugs.
- `evil_joseph_get_attack_move_id` (`0x4c40c5`): the same shared pattern as the other cases of `boss_get_attack_move_id` (`0x4a209d`..`0x4a20eb`); the melee attack runs only in the attack mode chosen together with the move.
- `evil_urath_filter_target` / `murod_filter_target` reading the handle before the NULL test (`0x4c4482`, `0x4c5b82`): their only caller `boss_target_filter_dispatch` gets entries from `boss_target_list_build`, which stores only non-NULL characters (`test edi, edi` at `0x4a2fbf`, store `0x4a3053`). The later NULL test is dead.
- `boss_target_list_find` / `_b` with unchecked candidates (`0x4c3c84`): same lists, built by `boss_target_list_build`.
- `tentacle_beast_ai_release` without a victim test (`0x4c6b53`): real, but dead with the grab (part of G250).
- The tentacle squeeze tick healing (`0x4c714b`): real, dead with the grab (part of G250).
- Giant Salamanka forced stage outside 1-3 setting the stage to 0 (`0x4c8589`): `cmd_gs_stage` accepts only 1-3 (`0x4c80b8`..`0x4c80bf`), and the forced stage is cleared at boss load (`0x4c83df`); Titus also keeps an in-range forced stage (`0x4c24ce`..`0x4c253e`). Unreachable, and there is no console in v1.40.
- "Invalid stage for titus rider" (`0x59dc38`) and "Sets dark josesph in combat state": console text; no console in v1.40.
- F624 `ui_button__process` looping for ever above state 9: every store to `+0x278` is a constant 0-9 or a computed 6 / 0 (`0x526ce1`..`0x526cf2`) or 7 / 0 (`0x526da7`..`0x526dba`), so the default never runs.

- F6203 `ui_listbox__recalc_content_size` gives an empty list a content height of minus the entry spacing (`0x52ce08`..`0x52ce16`): `entry_at` returns NULL for an empty list first (`0x52ce2f`), and `set_vscroll` clamps to 0 (`0x52ea78`). Harmless.
- F6210 `ui_listbox__snap_scroll_bottom` reading the ring head at the end of the content (`0x52ec8f`..`0x52eca4`): the head is a constructed `ui_listbox_entry` (`0x52d5fd`) whose line count (list box `+0x300`) nothing writes. Every list box in v1.40 is a static object (zero `.bss`), so the count is 0 and the code falls back to the last entry (`jle 0x52eccd`), which is right. Only `ui_parse_listbox` uses `new`, and it has no caller.
- L1, second half: `quaternion__set_from_matrix_b` choosing case 2 when `m[2][2] > m[1][1]` even if `m[0][0]` is larger: for a rotation matrix every case gives the right quaternion while its root is positive; this only costs precision.
- L4, the 1.75 scale: it is applied when the sound is at the listener position (`0x520fe3`), so it is not dead; in the distance case it is used only below 1.0 (`0x52100f`..`0x52101f`), which looks like a min-style clamp. Not counted on its own.
- `eax_buffer_init` setting ROOM to -10000: a 3D buffer played in 2D (`ds_play` sets `DS3DMODE_DISABLE`, `push 0x2` at `0x55d1b7`) gets no reverb on purpose.
- `cquat_log_of_delta` / `quaternion_log_of_delta` using a local without a constructor call: the constructors are empty, so nothing is uninitialised that is read before it is written.
- `lights_get_selection_count`, `lights_shade_vertices` and `lights_calc_point_specular_color` read `[4*depth + 0x2c9f99c]`, which `asm.mjs` labels `effects_num_names`: it is `lights_selection_counts[depth - 1]` (the counts start at `0x2c9f9a0` and the selection depth is 1 or more), not a read of the effect name count.
- `s3d_read_navpoint` converting a matrix to a quaternion: that branch runs only when the `File` writes (`File__check_flags(1)` false, `0x46f31c`), and `level_load_s3d` only reads.

- Per-layer fog distances swapped (render notes): `fog_set_layer` stores the first `+Distance:` number in `0x22fae78` and the second in `0x22ea4f4` (`0x487119`, `0x48712e`; `level_parse_table` `0x4cbd41`..`0x4cbddc`), and `level_set_current_layer` copies `0x22fae78` to the near and `0x22ea4f4` to the far distance (`0x48218a`..`0x4821b6`). Retail lines are written near, far ("30 50", "60 140"), so the fog is right; the two `sum.symbols` names are swapped (symfix).
- `fx_is_finished` with no instance (`0x48d02c`): all five callers pass the handles of effects made by `fx_create_instance` (status effects via `fx_attach_to_character`, move effects, kinds 5 / 6 in `fx_frame` `0x48a7f3`, Phoenix and Titus effects); handles carry a serial (`obj_from_handle` `0x46b141`), so a stale handle returns NULL first.
- `fx_create_spell_area` calls `character_is_enemy` on the caster without a test (`0x48c77e`): its three callers are the cast paths of that caster (`spell_cast_execute` `0x47ca4c` / `0x47d007`, `spell_cast_launch_effects` `0x47d4e4`), so the caster exists.
- `fx_frame`'s area exit `duration < frame time` (`0x48b4d7`..`0x48b4e7`): every collected area matches one of the five spells, so the duration is 11,000 or 20,000 ms; dead code, not a defect.
- `fx_destroy_spell_effects` destroying status holders at a cutscene start: `character_update_status_effects` recreates them (folded into G271).
- `camera.md` fog defaults 20 / 30 and 35 / 50: confirmed (`0x4870c9`..`0x4870d3`, `0x487099`..`0x4870a3`).

- The character tint replaces the lit colour by tint x clamp((r + g + b) x 0.75, 0, 255) in all three renderers (`[0x580e04]` = 0.75: `0x50bb6d`, `0x50c8bb`, `0x50d3b9`), and level geometry uses the plain clamped sum (`levelrender_face_vertex_colors` `0x48f9a2`..`0x48f9ee`). Neither is an average, and the two paths agree on a bright, flat highlight, so this reads as intended.
- `gr_cull_sphere` tests `gr_frustum_near_plane` (`0x26924bc`, `0x50ed2f`..`0x50ed4b`) whenever `ignore_far` is 0, also with far clipping off, and `gr_setup_frustum` updates that plane only while far clipping is on (`0x50e4e4`..`0x50e591`). In retail every level view fogs (`fog_set_high_camera_default` / `fog_set_low_camera_default` store 35/50 and 20/30, `0x487099`, `0x4870c9`), so `levelrender_fog_enable` (`0x482399`) sets the plane before `levelrender_add_lobjs`, `levelrender_build_draw_lists` and `levelrender_add_effects` cull; `worldmap_frame` (`0x42f5b1`) runs after the scene and reads that frame's plane. The plane is the fog distance, not a near plane (symbol fix).
- The Glide, Direct3D and OpenGL character renderers drop vertices at different screen bounds (Glide 1224 x 1000, Direct3D 2048, OpenGL depth only): a renderer difference, not a slip.
- `ix_ray_sphere` never reads its length argument (`[esp + 0x5c]` is overwritten by the dot product at `0x53d7e7` before any read); its only caller, `script_editor_ray_hit_object`, passes 1000.0 (`0x4e2f56`), farther than any visible object.
- The dialogue box draws compare the visible height with the scroll offset instead of each line's top (`dialogue_text_box_draw` `0x4d779e`, `dialogue_topic_box_draw` `0x4d7a48`), so lines below the box are drawn and clipped by `gr__set_clip` (`0x4d775a`): wasted draws only.
- `character_use_inventory_item` / `character_use_equipped_item` keep an item with a fractional `+Disposable:` value at 0 charges when the break roll fails (D5204): that is the break chance working, and items.tbl 1.40 uses only plain `+Disposable` (1.0).

- Level vif particles are not leaked: `vfx_instance_create` makes `level_mempool` current and passes level = 1 only for a level effect (`0x51d76f`..`0x51d79f`), so the `operator new` in `vif_instantiate_level` (`0x5337ed`) allocates from that pool (`operator_new_dbg` `0x5124c0`), and `s3d_alloc_level_arrays` empties it on the next level load (`mov dword ptr [0x22e43c8], ebp` with `ebp = 0`, `0x46f9a9`..`0x46f9b0`). `vif_record_release` only has to clear the pointer.
- `dialogue_parse_table` (conversation::parse_table) does return 0 on success: `xor eax, eax` at `0x5008a6`, after the destructors and before the shared exit at `0x5008a8`; the failure paths load -1 (`0x50091c`) or `TextFile::Open`'s result (`0x4feff2`).
- `ls_test_use` (`0x4f29c0`) is `xor al, al; ret`, a shared empty use hook; no gate or refusal code.

- `snd_load` passing 0x80 to `File__get_packfile_path` (`push 0x80` at `0x520b17`): the call fails when `install_path` + packfile name is longer than 127 characters (`0x514059`..`0x51405b`), and then `ds_load_buffer` gets the plain name with offset 0. That length is the same one at which `packfile_add` already overflows at start-up (C310), so it cannot be reached on its own.
- `automap_labels_parse_table` (`0x42fd40`) overwriting label 0 after 32 labels: the same `automap_label_alloc` return of 0 as K6, tested with `jl` (`0x42fe19`). The other callers, `automap_labels_reset_default` (`0x430342`) and `savegame_read_automap_labels` (`0x4d3806`), test it the same way. Retail `automap_labels.tbl` holds no labels. It is part of K6 (see the docfix).
- `automap_reveal_all` (`0x42fc00`) calling `automap_reveal_cell(level, layer, x, y, 0)` for every cell (`push 0x0` at `0x42fc1c`): it clears the reveal bits. Its only caller is `GameInit` (`0x43e06a`), at start-up, where clearing is harmless. The name is wrong, not the code (symfix).
- `automap_labels_reset_default` matching shopkeepers by name pointer (`cmp [ebp], eax` at `0x430317`): `level_parse_shopkeeper` stores the pointer `character_get_name()` returned for the same character (`0x4cc56a`..`0x4cc575`, `0x4cc5b8`..`0x4cc5bd`), so the pointer test works.
- `decal_place`'s colour argument receiving a float from `blood_splot_place` (`push ecx` at `0x468a64`): decal `+0x30` is read only by `decals_draw_for_lobj`, which passes it as the eighth argument of `lights_shade_vertices` (`0x4bca96`). That function never reads its eighth argument (`0x51f3b0`..`0x51f524`). The value has no effect, whatever its type.
- `character_play_move`'s trigger-9 test (D6104): trigger 9 is "no interrupt" (`anim_event_names` `0x5960b4`). The binary refuses a new move once that point is reached (time left <= 0.0001, `0x460fae`..`0x460fbe`), and `character_on_death` waits at the same point (`0x4538b0`..`0x4538bd`). Before it, both remove the move's missiles and go on (`fx_remove_missiles_from`). The lock ends when the move stops playing (`character_move_playing` sets `cur_anim` to -1, `0x4614ba`). This is the intended no-interrupt window. Only our source had the test inverted. combat.md describes the death wait the wrong way round (docfix).
- `Give_all_items` (`0x609fec`) written only by `cmd_Give_all_items` and `ls_test_init` and read nowhere: confirmed (`0x43808f`..`0x4380a9`, `0x4fde10`). It is already V122.
- `nw_NewCloseSocket` clearing `rsequence[i]` instead of `ssequence[i]` (D5806): the record is cleared afterwards when the socket exists, so there is no effect.
- `levelrender_fan_tables[n]` indexed without an `n <= 8` test in the plain Direct3D and OpenGL paths (`0x492872`): `gr_clip_polygon` clips against plane bits 1..0x10, five planes, so a clipped triangle has at most 8 corners and the 9-entry table is enough.
- Direct3D lightmapped batch restart keeping `levelrender_d3d_vertex_buffer_pos`, and the OpenGL restart testing count + 3 (`0x491c39`..`0x491c42`, limit 4000): D5804 already shows the writes stay inside the buffers (0x2140 entries and 4512).
- `spell_spawn_creature` (`0x47bd30`) storing the list head when the random index runs past the end: the index is below the count, so it cannot happen.
- `worldmap_frame` setting location flags 2 and 4 together (`or al, 6`) and `automap_reveal_cell` revealing twin levels 7/49, 30/35, 31/36: design, not defects.

**Still unclear (left out):**

- `objects_update_frame` animates objects of type 1 as characters (`0x46b3d0`). We found no `obj_link_new` caller
  with type 1, but `living_entity_init` passes its type from a register (`0x461c05`, not traced).
- `automap_label_margin` read at +0xc instead of +8 for the cursor y, `pickup_list_origin[2] = {210, 293}`,
  `itemtrade_gold_input_size` 303 at 800x600 and the larger 640 values in pxochat: possible data slips. They need
  screenshots to judge.
- `build_bone_matrices` keeping the fade factor only for state animations: intent unknown.
- The other parse.cpp remarks in `NOTES-memory-parse.md` (Error's 40-byte copy, ParseMultiline's trim,
  ParseFlags' NULL entry, AcceptQuoted's advance, `wide_charmap_to_string`, `stristr`,
  `allocate_in_commonblock`'s 2 slots) were not checked again.
- Medevan technique (`0x40`) plays an attack move (`living_combat::play_skill_attack`) but deals 0 at the first
  attack-type pass (byte 6 at `0x465d40 + 0x36`) and has no case in either side-effect table (`0x465d8c`,
  `0x46659c + 0x36` = default). Whether the skill does its work elsewhere was not traced.
- `s3d_read_meshes` compares "s1-sewercanal" with length 7 (`push 7` at `0x46efc0`), so mesh flag `0x100000` (`0x46efea`) also goes to the `S1-SewerCol-A*/B*` meshes in `levelm.vpp`. What that flag does on a level mesh was not traced.
- `character_create_base` does not test `ca_preload_alloc_slot`'s NULL (`mov [eax], ebx` at `0x5415da`; 205 slots). The peak number of loaded meshes in retail play was not measured.
- `character_unload` frees slot `+0x28` but never the shadow mesh at `+0x2c` (`0x5413b0`..`0x5413c0`). This matters only outside a mempool (flag `0x8`, `0x541372`). `charinfo_create_character_instance` sets a mempool first (`0x448e0e`), but whether `charinfo_select_mempool` can return none was not traced.
- `skeleton::read` reads each trigger name as 255 raw bytes and runs `strlen` on them. The retail `.mvf` files were not checked for a name with no NUL.
- `cmd_summon` has no `calling_a_command_function` test (`0x47f730`). What it does during the console's help pass depends on `next_arg` at that time, which was not traced.
- From the notes, not checked again: `container_spawn_loot` reading `+0xe0` of a gold item_obj (`0x4890c3`), `mp_handle_it_addi` / `mp_handle_item_drop` trusting the peer's counts (needs a modified peer), `level_free_s3d` keeping the navpoint and colour counts, `levelrender_wave_lobj`'s arrays that are never cleared or read, `tga_read_bitmap` not decoding grey images (types 3, 11), `tga_rle_encode_row` comparing `dst_bpp` bytes of pixels `src_bpp` apart, `ca_load_preload_index` without bounds, `skills_mouse_mode` staying 2 after identify / pick lock / banish, and the "mach_killed" summon gate.
- `character_create_base` not testing `ca_preload_alloc_slot`'s NULL (`0x5415da`): already listed. The slots are freed by `character_unload` (`mov [eax], 0` at `0x5413e1`), so the 205 slots must be held at once; the peak was not measured.
- The Ghost's class-4 moves: `boss_attack_resolve_dispatch` runs `ghost_resolve_hit` on the cone victims and then returns false (`0x4a2bd9`..`0x4a2be1`), so `living_combat_resolve_hit` also hits the main target. Whether that is a second hit on a target already in the cone list (`living_combat_resolve_hit_vs` and `living_combat_resolve_hit` both dealing damage) was not traced. With G30 the cone list is usually empty anyway.
- Evil Urath's fallback `rand() % 6` when `rand_weighted_index` fails (`0x4c5064`..`0x4c5071`) never picks the seventh spell (`0x5b`). When the weighted choice can fail was not traced (all weights are positive, so probably never).
- Dead party members: `character_save_state` and `character_load_state` have no case for type 8 (tables `0x4587d7`, `0x45a103`), so a party member who is dead at save time keeps no level, XP, HP, equipment, skills or AI state in the save. What the member has after the load and a resurrection was not traced (the party bits create a fresh type-7 entity, `0x4d2e48`..`0x4d2eb5`).
- `savegame_read_character` matches records only against `living_list` (`0x4cf7ac`), but the writer also saves dead characters from `living_list_inactive`. Whether the level always recreates those by name in the active list (else the load fails with `"7a"`) was not checked.
- `savegame_two_stage_get` / `_set` (`0x4d1110`, `0x4d0e20`) `strncat` a door or object name into a 64-byte stack buffer after `level_script_name` with count 0x40; a combined length of 64 or more would overflow it. Retail names were not checked (see G42).
- `two_stage_set` with an existing container entry stores both bits as 0 (`0x4d0fdf`..`0x4d1070`, notes open question 1); the path looks unreachable.
- `INVT` entry count is not clamped on load (`0x4d2cd5`); only a hand-edited save could exceed 0x209.
- G52: which retail callers pass `ai_path_find` a goal outside every cell. `world_click_move`, `ai_follow_character`, `conversation_frame` and `ai_approach_attack_slot` clamp with `pf_find_nearest_navigable_point` first; `script_action_walk_to` (`0x4dc760`, `0x4dc7b3`), `ai_path_find_new` / `_append` (`0x473cc6`, `0x473dbe`, `0x473e40`) and `mp_handle_swi_ndir` (`0x48885a`) were not traced.
- K51: whether any 2D bitmap, line or `gr_tmapper` draw runs while `gr_set_tint(1)` is in effect under OpenGL (none found; the tints we traced go through clamped mesh and level paths).
- G50: the in-game effect (whether the hidden platforms or the out-of-step board block the crossing) needs agent mode.
- V60: `cmp eax, 8; jg` at `0x43e374` is a signed compare. With Direct3D, a driver that reports 2 GiB or more of free texture memory (`0x80000000` or above) would read as negative, and a first start would switch on the low-resolution textures as in K64. What modern Windows reports through DirectDraw's `GetAvailableVidMem` for large cards was not measured.
- K60: whether the one-pixel edge is visible depends on what the colour buffer holds there (the frame's `gr_clear`, `state_game_frame` `0x43905a`) and on the clip area that `gr_setup_3d` sets; needs a Direct3D run.
- K61: that dropped items reach `gr_d3d_render_character_mesh` / `gr_opengl_render_character_mesh` through `ca_render_set_mesh` (`0x4862b7`) was read from `levelrender_draw_list`, not traced per object type. The Glide mesh path (`0x50bba3`) and level geometry (`levelrender_face_vertex_colors` `0x48fa11`) were not checked for the same conversion.
- K63: the intended shape (a cylinder of radius R inside a sphere of radius 2R) is a guess from the two tests.
- `mp_write_spell_targets` sending `target[0..1]` of modes other than 4 and 5 as local handles (notes, last bullet): not checked in this batch.
- `statscreen_unload_anim` leaves the freed light move's `anim` set (no store to `+0x8` after `character_free_skeleton`, `0x449194`). After light, then heavy, the next unload finds the stale light move first and frees its index. `character_free_skeleton` lowers the skeleton count when it frees the last slot (`0x540bf5`..`0x540bfd`), so the heavy anim normally gets the same index and the right skeleton is freed. Whether anything adds a skeleton to a hero between two Character screen visits without a full anim reload was not traced.
- `character_fill_locomotion_anims` fills walk_hurt from norm_walk and norm_walk_hurt from walk (also run_hurt from norm_run, norm_run_hurt from run; `0x46b0b6`..`0x46b125`), while stand_hurt takes stand and ready_hurt takes ready. Non-boss characters always use these fallbacks. Whether the crossing is intended depends on which slot a hurt character uses; not traced.
- V71: whether any script action or boss stage ever plays skeleton 1 through a skipped rider move.
- A goal without a target is saved with `name_hash_nocase("")` (`ai_state_save`). On load the search takes the
  first living entity whose name has the same hash. `living_entity_init` clears the name (`mov byte [eax+0x2f4], 0`
  at `0x44e6dd`), so an entity left without a name would become the target of every such goal, instead of -1.
  Whether any living entity in retail play has an empty name was not checked.
- `ai_state_load` neither saves nor clears a goal's path pointer (goal `+0x30`; the loop writes `+0x00`..`+0x2c`).
  If a level gives a monster goals with paths before the save is applied, a loaded goal keeps the old goal's path.
  Not traced.
- A knock-down during the fall (sub `0x2c`) passes `ai_can_be_knocked_down`, overwrites `disable_ms` with the new
  duration (which can be shorter), shows "Knocked Down!" again and is sent again in multiplayer. Whether that is
  intended was not decided.
- `ai_do_frame` runs `ai_frame` for type 4 (NPCs) even when their dead flag is set (`cmp ecx, 4; je` at
  `0x474bdd`). Whether NPCs can be dead there was not traced.
- `ai_state_init` calls `ai_set_mode(-1, -1)` (`0x47238f`) before it sets the handle slot `+0x80` to -1
  (`0x4723e5`), so `handle_slot_release` gets whatever `+0x80` held. Whether `living_entity_init` (its only
  caller, `0x44e22f`) clears that memory first was not checked.
- The `0x30` sub-mode (`AIS_PUSH_STUNNED`, `ai_mode_disabled_frame` `0x4b4ca6`) has no setter in `ai.cpp`. Who sets
  it was not traced.
- `gr_set_fog` with `far = 0` and `near > 0` (possible through `level_set_current_layer`, `0x4821b6`): `near` is clamped to 0, both checks pass, and the ranges become `1 / 0` (`gr_fog_alpha_inv_range` `0x507ead`, `gr_fog_z_inv_range` `0x507ee6`). With x87 exceptions masked that is +infinity; what the back ends draw then was not traced.
- `gr_d3d_read_backbuffer` calls `gr_d3d_end_scene` (`0x53a7f7`) and on a failed lock returns without `gr_d3d_begin_scene` (`0x53a82b`). The thumbnail caller `saveload_capture_thumbnail` (`0x42718a`) would then draw the rest of the frame outside a scene. Whether it checks the returned 0 was not traced.
- `gr_d3d_read_backbuffer` always reports 565 and copies `2 * w` bytes per row (`0x53a850`, `0x53a8b3`). If the Direct3D renderer can run with a 32-bit back buffer, screenshots and thumbnails would be garbled. Whether it ever picks one was not checked.
- `gr::set_clip` with an origin right of or below the screen sets `clip_left` / `clip_top` to `max - origin - 1`, a negative value, and the clip becomes one pixel wide at the screen edge instead of empty (`0x506840`-`0x50689f`). No caller with such an origin was found.
- The per-layer fog arrays (`0x22ea4f4`, `0x22fae78`) may be named the wrong way round, like `0x59bd7c` / `0x59bd80`; depends on the "+Distance:" order in `level_parse_table` (`0x4cbddc`).
- `character_add_hp` with a change of exactly 0 starts the floating number at 0 when none is showing
  (`delta == 0` at `0x450946`, store `0x450951`), and `levelrender_draw_float_text` prints `|+0x6e8|` whenever the
  timer runs (`0x48543a`). `living_entity_get_hit_by_spell` calls `character_add_hp` after every handler, also
  status spells, cures and resisted spells (`0x45fecd`). So such hits should show a "0" over the target for 2 s.
  Whether that is meant as a "no damage" marker was not decided; needs a screenshot.
- `rings_add_xp_all` feeds all eight rings, owned or not (`0x450420`). The Light, Fire and Stone messages have no
  inventory test, so "Joseph acquired Summon Wraith!" could show for a Ring of Light the party does not have. The
  four dragon messages need the ring in the party, so a dragon ring that passes 60,000 before it is found never
  gets its message (the summon still unlocks). Whether retail ring totals reach 60,000 before the ring is found
  was not measured.
- `party_set_level` has its own skill-spending loop that gives Joseph +20 per point and the others +10
  (`0x4ced1b`..`0x4ced26`). It runs after `character_add_xp(..., quiet = 1)`, which has already spent every point
  with +10, so the +20 branch never runs (`mov eax, [edi+0x17c]` at `0x4cece0`, `cmp eax, ebx; jle` at
`0x4cece8`). Whether Joseph was meant
  to get +20 was not decided.
- The death spells unequip and free every equipment slot of a non-party target (`0x45fdf0`), so its equipment
  cannot drop as loot. Intent unknown.
- `party_set_level`'s own spend loop gives Joseph (species 0) +20 per point, while `character_add_xp`, which really spends the points, gives everyone +10. Whether the stage jumps were meant to give Joseph +20 was not decided.
- `level_trigger_set_exit_by_refname` / `level_trigger_set_script_use_by_refname` change only the first trigger with that `+RefName:`. In the retail tables the single-player copy always comes first and its `+Multi` copy second, so in multiplayer `ls_sewer_init`'s toggles never reach the trigger that is used. Whether multiplayer is meant to keep every sewer exit live was not checked.
- `script_play_cutscene` saves `level_script_frame_fn` without checking that it is already `script_cutscene_done`. Two calls in one frame (for example two of `ls_templeint_frame`'s eight `urathmyth` flags set at once) would leave the level's `frame` slot replaced by `script_cutscene_done`, which then calls the second continuation every frame. Whether two of those flags can be set before one frame runs was not traced. The Giant Salamanka pair in `ls_khosanilab_salamanka_fight` cannot: stage 3 needs the stage 2 acknowledgement (`giant_salamanka_check_stage` `0x4c85fa`).
- `script_eleh_strip_party_gear` puts Joseph's old weapon and the slot 6 / 1 / 2 items into the party inventory (`party_inventory_add_item`), so they can be put back on at once. Whether the prison was meant to take them away was not checked.
- `party_check_all_dead` picks the first party member without `0x80` (`0x439494`..`0x4394a5`), but
  `party_select_member` refuses one with `0x8` as well (`test [eax+0x320], 0x88` at `0x4396e5`). If that member were
  hidden while the leader is dead, the selection would fail every frame (error sound `0x24`) and the death screen
  would never come. At least nine places set `0x8` (`0x450e18`, `0x453870`, `0x495788`, `0x495816`, `0x4b8cd3`,
  `0x4ba4f5`, `0x4c2603`, `0x4c35b6` and others); whether any of them can hide a living member while the leader is
  dead was not traced.
- `cmd_character` appends every argument to its 128-byte stack buffer with no length check (`0x43995e`..`0x4399c9`).
  Whether a console line can be long enough to overflow it depends on the console input limit, which was not read.
- `state_game_enter` in multiplayer: when `level_load` fails it reports the local player as disconnected
  (`mp_player_disconnected(4, 0)`, `0x4381cc`) but does not leave state 4. What the game does next was not traced.
- Wolong caverns poison mist trap (notes, bug 4): `ls_wolongcaverns_ptrap_fire` deals 15 damage and adds status 3 (Curse) with value 0.5 for 40 minutes (`push 0x3f000000`, `push 0x249f00`, `push 0x3` at `0x4f6bfd`..`0x4f6c09`). The trap is named poison (navpoint `$poisontrap`, `wolong_caverns_poisonmist.wav`, `wc-ptrap.vfx`), and the Liangshan marsh uses status 11 (Poison) with the same call shape and -24 (`0x4f4d42`..`0x4f4d56`). But 0.5 is a Curse value (damage x 1.5, magic resistance -0.5): as Poison it would add 0.5 HP at each tick. So the id and the value agree with each other, and the name does not. Whether the designers meant a curse is unknown. In game the player sees "+Curse" and a blue tint for 40 minutes, not "+Poison".
- V140 / C143: what the stack holds at `trigger` (`[esp+0x194]` of `script_parse_objects`) on the first "+No create" object of a cutscene. In `cutscene_script_load` the previous calls are `script_parse_navpoint`, whose frame covers only the upper part of that buffer (its `orient`, `hit` and `ray` locals); the first 0x24 bytes come from deeper calls. Whether 32 bytes without a zero can occur needs a runtime read (agent mode).
- `script_parse_navpoints` has no ray-miss test (the cutscene parser has a DebugError at `0x4db5e5`): a level navpoint with no `$layer` floor under it reads the zeroed hit, so it is raised by 2 units and gets level object 0's pick layer. Whether any retail level navpoint misses the floor needs the level geometry.
- The `cmd_character` line in the existing list ("depends on the console input limit, which was not read") is answered by C140: the limit is 254 characters (`0x50a23e`), so the overflow is possible from a `.vcs` script; that line can be removed.
- Timeslip's `[console]` binding length (its `dinput.dll` holds `0x50a210`; its line buffer was not read).
- lenele3a in multiplayer: `ls_lenele3a_init` changes every trigger of a name, including the `+Multi` copies of "templeint2" (`level_trigger_set_script_use` loops over all records, `0x4cd00a`..`0x4cd047`). Retail `lenele3a.tbl` has a `+Multi` "templeint2" on `$loadarea06` with no `+Multi` palace exit there. If this level script runs in multiplayer before stage 21, that doorway would do nothing silently, which is where the "templeint" refusal (V151) would have been heard. Whether multiplayer runs `ls_lenele3a_init` with those stages was not checked.
- Glide animated textures after a texture quality change: `gr_glide_mark_texture_dirty` rebuilds and invalidates only the current frame (`0x5496e8`..`0x549718`). The other frames' entries keep their old size, and their owner still matches, so `gr_glide_tcache_set` will not rebuild them. Whether that shows (UVs from the reduced bitmap size against a full-size texture) was not traced.
- What a Glide wrapper reports in `fbRam` and the TMU count (`0x2f0f218`, `0x2f0f220`) decides whether the 8 MB test passes and whether the focus code closes the Glide window. Not checked for any wrapper.
- On the edited `Sum.exe` only (not v1.40 as shipped): `cd_prompt_disc2` calls `cd_find_drive("SUMMONER_2")`. That needs the volume label of `.\` to read SUMMONER_2 (`0x43e825`..`0x43e839`), so it may never return a drive and the "Insert CD #2" box may loop when `levelm.vpp` is missing. Not tested. It depends on what `GetVolumeInformationA(".\\")` returns.
- `party_cycle` ignores its direction in multiplayer and always walks forward (`0x413370`..`0x4133ac`), so Shift+Tab acts like Tab. It only matters when the local player owns three or more members that the walk accepts; we did not establish whether a multiplayer player can (summons are not excluded by the `0x8a` test).
- `door_find_by_mesh` for the host's own click calls `door_set_open(l, -1, 0, 1)` / `door_set_closed(l)` on a door that `door_toggle_state` has already set moving; whether the third argument (0 here, 1 in the DOOR_TG handler `0x4aa7ab`) restarts the host's door movement was not traced.
- Whether multiplayer can be started in retail v1.40: `mainmenu_button_action` (`0x41dca0`) has a multiplayer case
  through `mainmenu_check_network`, while source-map.md says the mode was not offered. If it is reachable, C181,
  G180-G182, V181 and V182 are live in multiplayer (as G14 / K160 are counted) rather than latent.
- G182's visible effect after the failed IAMHERE (what the join screen shows while every send fails) was not traced.
- German `ß` goes to 0xBC (`0x50530b`) while the reverse table takes 0xE1 (CP437 `ß`, `0x5052be`) back to `ß`: right
  or wrong depends on the font's glyph order, which was not checked; v1.40 never selects German.
- K190: what the player hears when `cutscene_unload` frees the slot of the music track that is playing (`snd_free` -> `snd_unload` -> `ds_unload_buffer`, `0x55ced0`, drops the buffer's reference count): whether the stream keeps playing to its end, stops, or breaks the next `music_play` of that track. Needs a test in agent mode (a dragon summon in Machival's fight after his intro).
- `player_read_mouse_input` assigns `0x4` on a left press (`0x4345e0`), dropping Shift `0x20` and Alt `0x40` set earlier in the same call. Whether a reader of `player_input` needs Shift or Alt on the click frame (a Shift+click action) was not traced.
- C190: whether the dword at entry `+0x74` has nonzero padding above the count word (the constructor and the parser write only the word), so whether the read is NULL or a small address; either way it faults.
- 0x24268f0 / 0x24268f4 (two of the `gr_info` counters) are written as `esi`-indexed elements by `levelrender_draw_list_opengl` (`0x492313`), so they may be the first elements of a levelrender array rather than counters (D3104).
- C200 before the crash, as a client: the leaked records stay in `mp_player_list` with status 0. When the player then joins another LAN game, `multi_do_frame`'s status 6 step waits for `MP_PF_DATA_RECEIVED` on every listed record (`0x441435`..`0x441458`), and `mp_add_new_player` gives up when the free list is empty (`mp_player_alloc` `0x4af5d5`, then `mp_player_disconnected`). So a join after one to three Cancels may stall at "linking players" or drop out; the join path (record ids, `mp_player_find_by_id`) was not traced.
- C201 for an unlisted sender: whether the first fault is the read at 0x6b636170 plus the offset (`0x44185e`..`0x441863`) or the return to 0x2064696c depends on whether that address is mapped; both crash.
- C201 port: `mp_net_port` defaults to 10234 (sum.symbols), while vsdk-os.md says `nw_init_socket` falls back to 4200 for values up to 1023 or 65535; which port a default install listens on was not re-checked.
- C203's visible result: what the 0x3d0 bytes before `char_infos` (other characterinfo.cpp variables) make of the new monsters; a crash is likely but not traced.
- V200: whether the console executor's caller uses EDI after the command returns.
- V201: whether the unstored link lives on with heartbeats from the other machine or times out after 100 s.
- Lobby user counts: `lobby_handle_irc_command` adds 1 to the joined channel's count for every join event, even for a user already listed, and subtracts 1 for every part, even for an unknown user (`0x43f5fe`..`0x43f61a`, `0x43f63b`..`0x43f658`). Whether the IRC layer can send such events was not traced; the next `/list` overwrites the count (`0x43f516`).
- `.peg` stream entries: `effect_stream_open_current` starts a zero-byte read of the file object without opening it (jump table `0x440840` entry 1 -> `0x440825`, `edi = esi = 0`). Only effect base names are queued (`effect_base_queue_stream` `0x51c6f7`); whether `effects.arr` names any `.peg` (or `.mvf` / `.vim`) was not checked.
- `ai_group_think_style2` (Range, Caster) returns without any order when it meets a helpless enemy in the shuffled list before an engaged one (`jne 0x4936b5` at `0x49362f`), where `ai_group_think_style1` skips it; a design choice or a slip is not known.
- `ai_group_collect_enemies` returns before resetting anything when the leader handle is not living (`0x493add`), and `ai_groups_frame` still calls `ai_group_combat_think`, which then works on the previous group's `ai_think_*` lists; no retail path that leaves a group with a dead or deleted leader was found.
- The Tiger's pending sub-mode word `0x2452b7c` has one reference, a read in `tiger_ai_frame` (`0x49bfab`, compared with `0x21`); no write was found, so that branch may be dead like the Serpent's illusion.
- Notes 14 intent: the `=-` reading of G216 rests on the generic picker; the Tiger's own constants (base 10, weight `base x 0.1 + 10`) differ from it anyway.
- C230: whether any retail sequence leaves a waiting type-4 goal whose target handle no longer resolves while a cutscene script is active (corpses may keep their handles; `ai_goals_mark_keep` keeps goals across `cutscene_play`).
- C231: the largest number of door edges (`pf_edges` flag `0x2`) per door in the retail levels.
- G230: how often five goal slots are full in play (move-to and wander orders with different points each add a new goal, since `ai_goal_find` compares the point).
- K230: whether the host sends an OBJ_UPD for an idle layout monster soon, so how long the broken orientation lasts.
- `script_write_level_script` counts the inactive characters for the duplicate-name list without the serpent-part test that the fill applies (`0x4e9904`..`0x4e993a` against `0x4e99cf`..`0x4e9a24`), so a dead serpent part with a level start navpoint would leave an unset pointer for `qsort`. The extra parts are made by `serpent_spawn_rider` through `living_entity_create` without a start navpoint (no `+0x34` store in `0x4955e0`..`0x49580b`); whether the constructor leaves `+0x34` at 0 was not traced. Probably unreachable.
- `ai_combat_cast_frame` reads the target's (`0x4bfc36`) and the own (`0x4bfd43`) AI group pointer of a party member without a NULL test, while the `cur_character` branch tests it (`0x4bfd9b`). Whether a type-7 character can be without a group there (after `party_remove_member` or `ai_group_remove_member`) was not settled.
- `ai_consider_blind_or_silence`: whether the 1.5 on the running total (`fmul [0x580210]` = 1.5 at `0x4bef24`, weight 10 at `0x57eb7c`) was meant as "the selected character counts 1.5 times". As built, that character's chance grows with its position in the list.
- `ghost_revive_minions` (called every frame from `boss_ai_check_stage` `0x4a079f`) resurrects every dead, inactive minion and decrements the summon counter `0x24808c8` without testing it (`0x4bbc80`..`0x4bbd45`); the summon gate `< 2` is in `ghost_choose_attack` (`0x4bad95`, `0x4baec1`). If dead minions keep flag `0x80` with `+0x84` bit 0 clear, they come back at once whether or not the Ghost cast a summon, and the counter goes negative. Not checked in game.
- `avoid_area_remove` returns before `avoid_area_free` when one of the area's obstacles is no longer used (`test cl, 1; je 0x4a9e5c` at `0x4a9e3c`), keeping its slot (14 in all) until `avoid_reset`. `avoid_area_rebuild` frees the rebuilt obstacle itself (`0x4a7d99`) and recreates the others of its polygon; whether a failed rebuild can free another area's obstacle was not traced.
- Retail play in the variant scripts of G221 (`sewer_v2`, `masad_v2`, `Wolong2_v2`, ...): `level_parse_ambient` finds their level's `$ambientNNN` names in neither the script's navpoints nor (unless the level file has markers of those names) `level_marker_find`, and then skips the line (`0x4cc116`..`0x4cc125`, the "Ambient sound navpoint not found" text is only formatted). If the level files have no such markers, those variants play without ambient sounds. The level `.s3d` marker names were not checked.
- Save on an install whose tables are only in `tables.vpp` (the retail layout): `File__find` falls back to the packfile when the loose file is missing (`0x51434c`..`0x51435a`), `File__delete` then returns -3 (`0x514aa5`..`0x514ab4`), and the table writers return that error before closing and renaming the temp file (`0x4ea47c`..`0x4ea485`, same in `level_items_write_table`). *Inferred:* Save reports an error for the level table or `level_items.tbl` and leaves `<n>.int` behind (if the folder exists), after the script file was already written.
- F612 `FileReader::read*` in memory mode: real off-by-one (`lea edx, [eax + 4]; cmp edx, esi; jl` at `0x513bbd`..`0x513bc2`, the same in `read` `0x513b53`..`0x513b58` and the float, int16, int8, vector, quaternion and string readers), and a failed read returns 0 without moving. It is used for preloaded skeletons and effect streams (`character_add_skeleton` `0x540ab1`, `effect_stream_finish_entry` `0x440a28`, both into `skeleton_read`). Whether a retail `.mvf` ends exactly with the last field `skeleton_read` reads (the tag key floats, `0x544924`..`0x544963`) was not checked.
- The party-list walks without a NULL test: `titus_rock_erupt_damage` (`0x4c3384` multiplayer, `0x4c340a` single player), `tentacle_collect_area_hits` (`0x4c7830`) and `tentacle_beast_battle_update` (`0x4c7696`). They crash only if a party record or AI group member handle can be stale; C171 shows records can lose their characters in multiplayer, but no single-player path was found.
- ui_button / ui_static draw return at once when slot 4 (`is_disabled`) is true (`0x52769c`..`0x5276a1`) and use the disabled look when `ui_gadget__is_hidden` (`0x52c4b0`) is true (`0x5276af`..`0x5276b6`): either the names of `+0x1c` / `+0x1d` or of the two tests are swapped. Needs a runtime check.
- Game flag 8 widening the Luminar's teleport and grab candidates to every living entity (the boss included, `0x4c9dcb`): what sets flag 8 is not known.
- L10 `vfx_node_create_light` passes the size channel as a raw int16 (8.8 fixed, `fild` at `0x51fc51`) as the range (`attached_light_create` `+0x50`), 256 times too large; `vfx_node_update_light` overwrites `+0x50` with `fixed8_to_float` of the sampled value (`0x51fd45`..`0x51fd4d`). Whether any frame renders an effect light between `vfx_instance_init_nodes` and its first `vfx_instance_process` (`0x53717b`) was not traced, so whether a one-frame light flash is visible is unknown.
- L11 `character_instance__render` returns with its instance still set through `ca_render_set_instance` when bit 0 of the parameter byte is set (`test al, 0x1; jne` at `0x520462`). Who sets that bit (callers `inventory_render_character` `0x41807d`, `status_draw_model` `0x42ccab`, `levelrender_draw_list` `0x4860ad`) and whether anything reads the stale `0x268298c` / `0x2682990` before the next set was not traced.
- C270: how long a fireball, wall of fire or blizzard area lives (the vfx length decides how many can burn at once); a test with several fire casters would settle whether the crash is live.
- K270: what the Ghost Rider's attachment list (`+0x728`) holds in Ghost-Escape / Ghost-Dismount, so whether the drawn props are visibly wrong.
- K274: whether any retail cutscene shows floating text.
- C274: whether a loaded save can restore containers before `level_items_parse_table` places the level items.
- `detail_apply_level_settings` (`0x487370`) stores "+UnlockMedium" / "+UnlockHigh" into `game_camera +0x08`, which the camera code treats as "defer high camera requests": either the camera field's name is inverted or the flags lock the high camera (render notes, open question; not a bug candidate yet).
- `experience.tbl` gains marked "D" are never below 3 (`fld 3.0` at `0x489b3a`..`0x489b4b`), even "D1" or "D2"; no retail line uses "D", so the intent cannot be compared.
- `ai_think_party_update` reads `char_info` of each AI group member from `living_from_handle` with no NULL test (`0x4b756b`..`0x4b7574`), for summoned creatures in the party group. Whether a group can hold a stale member handle (a dead or departed party member) was not traced; compare the party-list walks in "Checked, not bugs".
- `vfx_instance__set_node_transform`'s child loop (`0x537db3`..`0x537ee4`): the notes say mesh children of a light, dummy, emitter or vif parent are placed at the parent's displacement instead of their own position moved by it, and that vif children are measured from the file position (`0x537cb3`). Only `spell_probe_ground_point` (`0x47e4a9`) reaches it; not traced against an effect file.
- `vfx_mesh` xform branch of the frame update scales by 64 for mesh flag 4 after adding the translation (`0x564292`..`0x56429f`); the notes say the other branches scale first. Whether any retail mesh sets flag 4 on an xform mesh was not checked.
- `ui_radio_button::process` clears byte `+0x274` of every sibling gadget on a hotkey or Enter (`0x5547bf`, per the notes); the script editor's radio containers hold only radio buttons, mixed containers not checked.
- `anim_track__get_rotation` before the first key and `anim_track__calc_rot_tangents`' wrap interval (notes: harmless at t = 0; loop always 0): not checked.
- Multiplayer: `mp_mark_difficulty_complete` assigns the difficulty bits to `+0x30` (`mov [eax + 0x30], edx` at `0x4b0b1e`) when they are not all set, which clears bits 0..25. Verified in the code; whether that wipe of the level progress is intended (start the next difficulty from the first level) is not known.
- Multiplayer, unchecked: `ftp_get::abort_get` spinning when the worker never sets done; `http_get` never seeing a failed asynchronous host lookup; the items-checksum version check accepting the constant 1.40 value (D5210); `mp_change_level`'s dead search for "<level>_m1" (D5211); `tracker_start_game` copying the 0x100-byte channel into a 0x24-byte field (D5212); `gtrack_send_game_over` state 2 never set; the tracker's packet size test (D5214).
- `+NPCText:` with a quoted speaker name that `conversation_find_name` does not know yet (`0x4ff3c7`..`0x4ff418`) adds the name but leaves the line's speaker 0 (`0x4ff369`); no retail `+NPCText:` names a speaker, and what a 0 speaker does to the line was not traced.
- `gr_d3d_create_texture_section`: `Lock` on a mip level with flags 0 (`0x54bf46`); on failure it releases only that level and returns -1, which `gr_d3d_create_texture` ignores (`0x54d2ce`). The `CreateSurface` failure path calls `bm_unlock` (`0x54c088`) and `gr_d3d_create_texture` unlocks again (`0x54d308`). Whether either path is reachable or harmful was not traced.
- `gr_d3d_round_texture_size` rounds every size above 128 to 256 (`0x54d371`..`0x54d384`), so `gr_d3d_create_texture` refuses ("D3D: Not drawing %dx%d bitmap") any mip-mapped bitmap larger than 256 (`0x54d1b3`..`0x54d1f9`); whether a retail bitmap has mip levels and is larger than 256 was not checked.
- `spell_cast_launch_effects` creates the Pyrul and Mach Meteor Storm effects with spell `0x1c` (`push 0x1c` at `0x47da55`); all three have the same hit effect file in `spells.tbl`, and whether damage or sounds come from spell `0x1c` was not traced.
- The decomp73 notes' latent NULL reads (`spell_prepare_cast` `0x47b094`, `0x47b9dc`; `spell_cast_launch_effects` `0x47d987`; `spell_cast_execute` `0x47d2c9`) and the No_resist slip for spell `0x65` (`0x47b31a`) were not checked.
- D5804 item 4: the Glide and OpenGL plain paths append clipped-polygon vertices after the mesh's vertices with no bound (OpenGL index from `[esp + 0x20]`, `0x491765`..`0x4917b5`). It needs the largest vertex count of a retail level object (from the `.s3d` files), and we have no reader for that yet. With no object near 4000 vertices it stays harmless.

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Where |
|---|---|
| C1 `snd_disabled` skipped | `snd_init` `0x520642`..`0x52066b`; `ds_channel_release` `0x55c5db`..`0x55c5e7` |
| C2 NULL retry | `ds_init` `0x55c2d5`..`0x55c314` |
| C3 texture buffer size | `0x54e70b`..`0x54e722` |
| C4 lamp glows | `lamp_glow_add` `0x4c0a90`..`0x4c0ae4`; callers `0x4868b8`, `0x48beb8` |
| C5 saved actors | `cutscene_play` `0x43646c`..`0x43655f` |
| C6 arguments | `cmdline_split` `0x53e16b`..`0x53e1a9` |
| C7 loop | `ui_window__process` `0x52bcc8`..`0x52bce9` |
| C8 shop tables | `level_parse_shopkeeper` `0x4cc4f5`..`0x4cc755` |
| C9 buffer size | `surfaces_parse_table` `0x468f35`..`0x468f72` |
| C10 topic loop | `conversation_apply_topic_effects` `0x4d7f9a`..`0x4d7fd9`; `conversation_topic__push_list24` `0x4fecb0` |
| G1 limiter | `FrameTimer` `0x501540`, `0x501577`, `0x5015bc` |
| G2 QPC | `timer_init` `0x532853`..`0x532868` |
| G3 cutscene timer | `cutscene_sequence_update` `0x436ad9`..`0x436aff` |
| G4 overwrite test | `saveload_try_save` `0x426340`..`0x42635b`; `saveload_save_to_slot` `0x426395`..`0x4263a0` |
| G5 buy | `buysell_buy_item` `0x4058aa`..`0x4058c4`; `party_inventory_add_type` `0x46b9e4` |
| G6 Zane | `0x4fc99c`..`0x4fca22` |
| G7 load failure | `cutscene_play` `0x4362d7`..`0x43634c`; `cutscene_cleanup` `0x436626`..`0x43663b` |
| G8 leader | `party_remove_player` `0x4335d9`..`0x43363d` |
| G9 focus | `os_wndproc` `0x5310a9`..`0x5310e9` |
| K1 hot-button timer | `hud_frame` `0x40cf58`..`0x40cf7e`; `hud_handle_keys` `0x40dd17`; `GameTimer__valid` `0x501990` |
| K2 arrow | case bytes `0x407630`, jump table `0x4075f0` |
| K3 sell list | `buysell_category_first_index` `0x404f98`; `buysell_get_list_item` `0x40497d` |
| K4 identify mode | `inventory_category_first_index` `0x4153dc`..`0x41543e` |
| K5 thumbnail | `saveload_load_thumbnail` `0x425c61`..`0x425c67`; `saveload_draw_thumbnail` `0x426fda`..`0x426fe9` |
| K6 labels | `automap_label_alloc` `0x430236`..`0x430245`; `automap_add_label_at_mouse` `0x40317e`; `automap_labels_parse_table` `0x42fe12`..`0x42fe19`; `automap_labels_reset_default` `0x43033b`..`0x430342`; `savegame_read_automap_labels` `0x4d37ff`..`0x4d3806` |
| K7 glyph | `gr_font_get_char` `0x5084ee`..`0x508503`; `gr_string` `0x508d0e`..`0x508d36` |
| K8 credits | `credits_parse_table` `0x408858`..`0x408868` |
| K9 centring | `gamecreate_draw_player_count` `0x40a515`..`0x40a564` |
| K10 aspect | `gr_init` `0x505eee`..`0x505f0f` |
| K11 side vector | `conversation_camera_compute_shot` `0x4d8fbd`..`0x4d8fe4` |
| V1 trigger time | `character_set_skeleton_trigger_time` `0x540dbc`..`0x540ddb` |
| V2, V6 masks | `effects_alloc_stream_slot` `0x51c533`, `0x51c573`; `effects_find_stream_slot` `0x51c5d3`, `0x51c613` |
| V3 hotkey | `ui_parse_hotkey` `0x52847a`..`0x5284a3` |
| V4 seek | `tga_read_bitmap` `0x5556de`..`0x5556f7`; `File__seek` `0x51492f` (origin 0 = absolute) |
| V5 speed slots | `character_instance__start_anim` `0x51af3d`..`0x51af65` |
| `level_time` claim | `savegame_read` `0x4d261d`..`0x4d263f`; `savegame_read_header` `0x4d2ac6`..`0x4d2aea` |
| C11 item pool | `item_obj_spawn` `0x46a1db`..`0x46a20c`; pool `0x946790` + 1000 x 0xac = `obj_active_list` `0x970770` |
| C12 item records | `item_alloc` `0x44b380`..`0x44b387`; `items_init_pool` `0x44b282`..`0x44b295` |
| C13 delete loop | `objects_free_all` `0x469fc6`..`0x46a01b`; `obj__dtor` `0x469b40`; callers `0x46b203`, `0x43e228` |
| G10 gold merge | `container_add_item` `0x46aa79`..`0x46aaaa`, `0x46aae5`..`0x46ab0c` |
| G11 skill AP | `character_use_skill` `0x42948f`..`0x4294b6`; `skills_handle_mouse` `0x4288aa`..`0x4288b2` |
| K12 pickup thumb | `pickup_update_scrollbar` `0x4208f8`, `0x420901`..`0x420943`; `assess_update_scrollbar` `0x4017a6`..`0x4017d7` |
| K13 player rows | `mp_levelselect_handle_click` `0x41c79d`..`0x41c7e8`; `mp_levelselect_draw_players` `0x41d522` |
| K14 banner link | `pxo_parse_chat_banners_cfg` `0x4225cd`..`0x4225f4` |
| K15 32-bit TGA | `tga_convert_pixels` `0x5550a0`..`0x5550c6`; `gr_screenshot` `0x50625d`..`0x50629d`, `0x5063fa` |
| K16 weights | `character_instance__reset_anims` `0x51986b`..`0x519872`; `level_load_preload` `0x447cd9`..`0x447ce9` |
| K17 slot 9 | `items_level_load` `0x44d72d`..`0x44d74e` |
| K18 slot 18 | `item_page_out_all` `0x46caa2`..`0x46cad2` |
| V7 registry | `os_registry_resolve_key` `0x511e8d`..`0x511ecf`; `os_registry_delete` `0x5123e0` |
| V8 fonts | `gr_font_get_char` `0x5084b6`..`0x5084e7`; `GameInit` `0x43df76`..`0x43df9e` |
| V9 UI tables | `ui_load_table` `0x52b25e`, `0x52b3a3`..`0x52b3ca`; `ui_window__init` `0x52b18f`..`0x52b1d5` |
| V10 line count | `TextFile__SkipToNextLine` `0x515af4`..`0x515b1f` |
| V11 commas | `int_to_string_with_commas` `0x5152e3`..`0x5152f4`, `0x515331`..`0x515344` |
| V12 hex | `TextFile__ParseInt` `0x5162d0`..`0x5162e3`, `0x516314` |
| V13 lists | `TextFile__ParseIntList` `0x5169e7`..`0x516a02`; `TextFile__ParseFloatList` `0x516a6f`..`0x516a8a` |
| V14 16-bit TGA | `tga_convert_pixels` `0x55502f`..`0x555052` |
| V15 floor probe | `item_obj_spawn` `0x46a52d`..`0x46a535`, `0x46a5d8` |
| V16 helpers | `character_instance__get_anim_root_velocity` `0x51b1f7`..`0x51b226` |
| G12 recovery times | `spells_parse_table` `0x47a49c`, `0x47a4c1`, `0x47a50b`, `0x47a648`; `character_start_spell_recovery` `0x44dce6`..`0x44dd1f`; `character_perform_spell_cast` `0x45dc7a`; `status_effect_duration` `0x47ab90` |
| G13 aimed attack skill | `combat_apply_melee_hit` `0x4663c0`..`0x4663df`, `0x466468`..`0x466487`, `0x466510`..`0x46652f` |
| G14 container close | `mp_handle_container_close` `0x488f00`..`0x488f15`, `0x488f4f`..`0x488f77`; `container_remove_item` `0x46abf5`..`0x46ac04`; `mp_send_container_close` `0x488e08`..`0x488e2a` |
| G15 damage dice | `living_entity_get_hit_by_spell` `0x45e4ca`..`0x45e4d8`; `spell_roll_amount` `0x47f513`..`0x47f551` |
| G16 missile slots | `spell_launch_missile` `0x47e700`..`0x47e765`; `spell_fire_missile` `0x47e883` |
| G17 trade slot | `mp_trade_complete` `0x471913`..`0x47193e`; `party_inventory_find_type_index` `0x46bd91`; `console_command__console_command` `0x50a1dc` |
| C14 character name | `character_create_base` `0x54150c`..`0x5415c7`, `0x54160e`..`0x541636`; `character_load_shadow` `0x540e18`, `0x540e98`; `ca_preload_alloc_slot` `0x541700`..`0x541730` |
| G18 lockpick handle | `mp_handle_container_lockpick` `0x4896a2`..`0x4896ce`; `container_lockpick` `0x4894b9`, `0x489564`; `mp_send_lockpick_result` `0x489614`; `mp_obj_from_net_handle` `0x475265`..`0x47526b`; `object_use_container` `0x43cc7c` |
| K19 ComputerSpeed | `GameInit` `0x43e06f`..`0x43e104`; `summoner_cfg_load` `0x43e295`..`0x43e327` |
| K20 hit effects | `combat_spawn_hit_vfx` `0x4669d2`..`0x466a54`, case bytes `0x466a64`, jump table `0x466a58`; `material_names` `0x598f5c`; `characterinfo_parse_table` `0x445f42`..`0x445f64` |
| V17 options | `cmdline_read_params` `0x43dc7c`..`0x43dc8b`; `init_cmdline_glide` `0x43d37c`; `LoadSettings` `0x43dade`..`0x43db03`, `0x43d95b`, `0x43d968` |
| V18 extract_angles | `matrix3__extract_angles` `0x51028f`..`0x5102c0`; `set_from_sincos` `0x510086`..`0x5100b0`; `camera_blend_sequence__advance` `0x4944b9`..`0x4945f2`; `camera_update_sequence` `0x4794ad`..`0x4794ce`; `cutscene_camera_sequence__ctor` `0x493f83` |
| V19 dead math helpers | `matrix43__transpose_in_place` `0x511bb1`..`0x511c02`; `skeleton__get_root_rotation` `0x544051`..`0x54405c` |
| V20 mesh leak | `vim_load_file` `0x5438e5`..`0x5438f1`, `0x543908`..`0x54390d`; `vim_alloc` `0x54307c`..`0x543086`; `vim_free` `0x5430b9`..`0x5430be` |
| V21 textures | `vim_read` `0x543835`..`0x543896` |
| V22 colour clamp | `s3d_read_color_record` `0x4703bf`..`0x4703f2`; `level_load_s3d` `0x46e6f5`..`0x46e70d`; `level_free_s3d` `0x46fed0` |
| C30 preload index | `ca_load_preload_index` `0x54180e`..`0x541894`, `0x5418b0`..`0x5418c2`; retail `chars.vpp` `evil7724.svf` (61 entries, 626,685 bytes) |
| G30 offense point | `boss_ai_parse_table` `0x4a18ad`..`0x4a18c4`; `pyrul_boss_load` `0x4b8a5a`..`0x4b8a9f`; `pyrul_point_in_attack_cone` `0x4b9c29`..`0x4b9c4c`; `ghost_point_in_attack_cone` `0x4bbb89`; `machival_offense_point_reaches` `0x4c0dd9`; `murod_oni_offense_point_reaches` `0x4c6239`; `giant_salamanka_target_in_reach` `0x4c8c4c`..`0x4c8c61`; `boss_attack_resolve_dispatch` `0x4a2c1e`..`0x4a2c58`; retail `tables.vpp` `*_boss.tbl` |
| G31 plus spell | `living_combat_resolve_hit` `0x467e8a`..`0x467eb8`; `boss_counter_hit_dispatch` `0x4a2254`..`0x4a22ea`; `living_combat_update_boss_attack` `0x4683ea`..`0x4683fc` |
| G32 mounted test | `boss_species_mounted_stage` `0x4a2484`..`0x4a249e`; `ai_get_combat_style` `0x473644`..`0x473653`; `character_target_point_count` `0x4a21e8` |
| K30 boss height | `character_size_height` `0x45430a`..`0x454323`; `character_size_radius` `0x4542ca`..`0x4542d2`; `charinfo_boss_height` `0x4a15a0`, case bytes `0x4a1620` |
| V30 counts kept | `level_free_s3d` `0x46fe97`..`0x46fee2`; `level_marker_find` `0x4774ac` |
| V31 wave arrays | `levelrender_wave_lobj` `0x48e342`..`0x48e426`; caller test `test cl, 0x20` at `0x48fe4b` |
| C40 door commands | `cmd_door_open` `0x4d5e83`..`0x4d5ea5`; `cmd_door_close` `0x4d6593`..`0x4d65ac`; `cmd_door_toggle` `0x4d6613`..`0x4d662e`; `door_find_by_name` `0x4772ad`..`0x47730b`; `door_set_open` `0x476e07` |
| G40 gold amount | `savegame_write_item_objects` `0x4d24ec`..`0x4d255f`; `savegame_read_item_objects` `0x4d337f`..`0x4d33cb`; `item_obj_spawn` `0x46a619`..`0x46a622`; `item_obj_pickup` `0x46b6f6`..`0x46b70d`; `monster_drop_loot` `0x450fcd`..`0x450ff7`, `0x452e38`..`0x452e48` |
| G41 32 piles | `savegame_write_item_objects` `0x4d24e3`..`0x4d24e6` |
| G42 name length | `savegame_read_two_stage` `0x4d3566`..`0x4d35af`, `0x4d3730`; `savegame_read` `0x4d2771`..`0x4d277f`; `state_game_enter` `0x43820d`..`0x438213` |
| K40 labels | `savegame_automap_label_remove` `0x4d13bf`..`0x4d13ca`; `savegame_automap_labels_reset_level` `0x4d151d`..`0x4d1526`; `savegame_automap_label_add` `0x4d12d4`..`0x4d12dc`; `automap_handle_click` `0x402bd2`..`0x402bff`; `automap_label_text_input` `0x40343a`..`0x403470`, `0x4034c3`..`0x4034f9`; `state_game_enter` `0x438233` |
| V40 stage 19 | `game_stage_set` `0x4d433e`..`0x4d4381`; `savegame_set_flag` `0x4cfd74`..`0x4cfd94`; `events.tbl` (`tables.vpp`) `$Level: "ionaext02"`, `$Level: "khosanilab"` |
| V41 bit bound | `savegame_get_bit` `0x4cff3c`..`0x4cff4a`; `savegame_set_bit` `0x4cffae`..`0x4cffb3`; `automap_is_cell_revealed` `0x4300ce`..`0x4300f8`; `automap_reveal_cell` `0x430129`..`0x430166` |
| C50 window mode 202 | `gr_opengl_init` `0x54f6f0`..`0x54f700`, `0x54f741`..`0x54f761`, `0x54f702`; `LoadSettings` `0x43db45` |
| C51 multitexture | `gl_init_multitexture` `0x54f382`..`0x54f3e0`, `0x54f39e`; `gl_setup_multitexture_state` `0x54ffa2`..`0x54ffb2` |
| C52 room 4 prisoners | `ls_khosanilab_room4_frame` `0x4f8e0b`..`0x4f8e9a`; `script_object_find` `0x4cde54`..`0x4cde57`; `ls_khosanilab_frame` `0x4f8b4a`..`0x4f8b64`; `events_string_pool` `0x24f6218` (buffer `0x24ede14`, 0x8400) |
| C53 hills navpoint | `ls_worldmap1_nearest_hills_nav` `0x4fc2ad`, `0x4fc327`..`0x4fc337`; `ls_worldmap1_pick_encounter_stage4` `0x4fc672`..`0x4fc6e8` |
| G50 flipper board | `ls_khosanilab_init` `0x4f895c`..`0x4f8980`; `ls_khosanilab_puzzle_use` `0x4f9b66`..`0x4f9b97`; `ls_khosanilab_set_flippers` `0x4f9cc4`..`0x4f9cf4`; `door_apply_saved_state` `0x47737d`..`0x4773ad`; `door_start_move` `0x476fd3`..`0x476fe4`; `ls_khosanilab_use` `0x4f95d4`..`0x4f95dc`; `ls_khosanilab_apply_lava_platforms` `0x4f89e4`..`0x4f89ff` |
| G51 path leak | `pf_graph_search` `0x4a0180`..`0x4a019b`, `0x4a01b5`..`0x4a01b7`, `0x49fbec`; `ai_path_free` `0x4a0484`; `ai_path_alloc_node` `0x4a02d7`..`0x4a02f2`; `pathfinding_load_graph` `0x49ce04` |
| G52 cell crossing | `pf_find_cell_crossing` `0x49ed02`..`0x49ed84`; `pf_compute_graph_bounds` `0x49cb17`..`0x49cb74`; `pf_find_nearest_navigable_point` `0x49e972`..`0x49e9ad`; `pf_graph_search` `0x49fae6`..`0x49fb3d`; `world_click_move` `0x43bf26` |
| K50 565 mipmaps | `gl_build_mipmaps` `0x54e4f9`..`0x54e5cb`, `0x54e49f`..`0x54e4af`, `0x54e348`; `gr_opengl_create_texture` `0x54e9df`..`0x54ea00`, `0x54ebed`..`0x54ec0b`; `bm_read_vbm_header` `0x501df8`..`0x501e68`; `bm_read_header` `0x502b4a`..`0x502b99`; retail VBMs in the `.vpp` files |
| K51 OpenGL tint | `gr_opengl_apply_tint` `0x54f9b6`..`0x54fa1b`; `clamp_ubyte` `0x550640`; `gr_d3d_apply_tint` `0x535491`..`0x5354f8`; `gr_glide_apply_tint` `0x54a546`..`0x54a5a1`; `gr_opengl_tmapper` `0x5501a4`..`0x5501af`; `gr_poly_buffer_flush` `0x545d9e`, `0x546028`; `gr_opengl_render_character_mesh` `0x50d3a1`..`0x50d3c3` |
| K52 change_texture_res | `cmd_change_texture_res` `0x501c90`..`0x501cd3`; `cmd_load_mipmaps` `0x501b50`..`0x501b57`; `execute_command_line` `0x50a2ed`..`0x50a389`, `0x50a4af`..`0x50a50a` |
| V50 texture count | `gr_opengl_create_texture` `0x54ece0`..`0x54ed08`, `0x54ebb4`..`0x54ebd6`, `0x54ec11`..`0x54ec2b`; imports `0x57e30c` (`glTexSubImage2D`), `0x57e31c` (`glTexImage2D`) |
| V51 setup_view | `gr_opengl_setup_view` `0x55051a`..`0x5505bc`; `gr_eye_matrix` `0x26927c0` |
| G60 connect error | `irc_connect` `0x4abdb6`..`0x4abddc`, `0x4abce5`..`0x4abcee`, `0x4abea7`..`0x4abed4`; lobby connect step `0x43f173`..`0x43f19a`; `state_mp_lobby_frame` `0x43f0b2`..`0x43f0bb` |
| K60 z clear | `gr_d3d_zbuffer_clear` `0x535600`..`0x53565a`; `gr_d3d_clear` `0x535570`..`0x5355ca`; `gr__set_clip` `0x50684f`..`0x50687b`; `gr_opengl_clear` `0x54f81d`..`0x54f84f`; `gr_opengl_zbuffer_clear` `0x54f8af`; callers `0x482545`, `0x50df38` |
| K61 gold tint | `world_tint_gold_object` `0x43a62e`..`0x43a644`; .data `0x594424`..`0x594430`; `world_hover_tint_object` `0x43a681`..`0x43a6d0`; `levelrender_draw_list` `0x48628c`..`0x4862a2`; `gr_d3d_render_character_mesh` `0x50c8f2`..`0x50c90c`; `gr_opengl_render_character_mesh` `0x50d3f3`..`0x50d40e`; `_ftol` `0x56c7fd`; `monster_drop_loot` `0x45306f` |
| K62 kick reason | `chat_process_irc_message` `0x4ace6c`..`0x4acf35`; `irc_get_word` `0x4ad940`, `0x4ad982` |
| K63 pick radius | `pick_test_character` `0x43c6e4`..`0x43c6ef`, `0x43c74d`..`0x43c7be`; `character_size_radius` `0x4542ca`, `0x4542e3` (`0x59d7c8`) |
| K64 OpenGL 8 MB | `summoner_cfg_load` `0x43e2e0`, `0x43e32e`, `0x43e366`..`0x43e376`; `gr_get_video_memory_mb` `0x507a03`; `options_apply_detail` `0x4872d0`..`0x4872dc`; `options_apply_and_save` `0x41faa3` |
| V60 bytes | `gr_d3d_init_device` `0x539f6a`..`0x539fab`, `0x53a01b`..`0x53a04d`; `gr_d3d_get_video_memory_mb` `0x53aaf0`; `gr_get_video_memory_mb` `0x507a0e`; `summoner_cfg_load` `0x43e36c`..`0x43e376`; `gr_glide_init` `0x549ea2` |
| V61 enumeration | `d3d_ddraw_enum_callback` `0x538a02`, `0x538a58`..`0x538a61`; `d3d_enumerate_ddraw_devices` `0x538867`, `0x538884`..`0x5388f6`, `0x538907`..`0x538914`; `d3d_enum_devices_callback` `0x53892c`; `gr_d3d_init_device` `0x539e73`..`0x539ec4` |
| C70 stat-screen heavy NULL | `statscreen_refresh_anim` `0x412edb`..`0x412ef9`; `statscreen_load_anim` loop `0x449044`..`0x44907a`, read `0x4490dc`; retail `Rosalind.tbl` (no "statscreen heavy"), `items.tbl` (48 heavy items, none for Rosalind) |
| C71 town remap NULL | `charinfo_remap_town_moves` `0x447ab2`..`0x447af8` |
| C72 trigger bound | `characterinfo_parse_table` `0x4461da`..`0x4461ee` (sound parsed first), `0x4462bb`..`0x446318` (triggers) |
| G72 default weapon bound | `characterinfo_parse_table` `0x4457fb`..`0x4458b3`, `0x445b07`..`0x445b26`; `living_entity_init` `0x44e8e9`..`0x44e911` |
| C73 chain message buffer | `character_learn_chain_skill` `0x449650` (`sub esp, 0xa0`), `0x4497ac`..`0x4497d8`; `strings.tbl` ids 7-32 in three languages |
| G70 emerge triggers | `summon_stash_for_cutscene` `0x448038`..`0x4480ca`; `character_move_trigger_time_left` `0x461670`; `character_play_move` `0x460ff6`, `0x461005`; `summon_emerge_update` `0x4b46df`..`0x4b4701`; `ai_frame` `0x472c1f`..`0x472c45` |
| G71 unknown skill name | `characterinfo_parse_table` `0x445ba7`..`0x445bb7`, `0x445be9`..`0x445c1b`, `0x445d71`..`0x445dad`; `skill_names` count `0x595cb4` = 0x54 |
| V70 recycled class bits | `characterinfo_parse_table` `0x4460fb`, `0x44617d`..`0x4461b2`, `0x446354`..`0x44635f`; `characterinfo_add_move` `0x446740`; `characterinfo_load_party_anims` `0x4475ec`..`0x447601`; retail `Joseph.tbl`, `Rosalind.tbl`, `Flece.tbl` replayed in `species_table` order |
| V71 anim = 1 | `move_used_on_level` `0x444db3`, `0x444fb8`; `charinfo_load_move_anim` `0x447792` |
| B7 species ids | `species_is_hero` `0x449a20`; `species_table` `0x580e60`..`0x5818d4` |
| B8 empty command | `init_ccmd_charinfo` `0x449bb0` |
| B3 unload guard | `statscreen_refresh_anim` `0x412e76`; `statscreen_unload_anim` `0x449149`..`0x44917d` |
| K80 cast knock-down | `ai_can_be_knocked_down` `0x475030`..`0x475087`; `ai_knock_down_local` `0x4742fb`..`0x47433e`; `ai_set_mode` `0x47333e`..`0x47335e`; `ai_knock_down` `0x47435d`..`0x474377`; `mp_handle_ai_disable` `0x480f8e`..`0x480f9f`; `combat_apply_melee_hit` `0x466286`..`0x4662a4` |
| V80 ai_state compare | `cmd_ai_state` `0x474d48`..`0x474d77`; .data `0x59ad60`..`0x59ad94`; `next_arg` as a buffer `0x437513`, `0x43995e` |
| V81 dead variables | `cmd_ai_frame_update_rate` `0x472102`, `0x472121`; `cmd_attack_point` `0x47329d`..`0x4732b7`; `cmd_offense_point` `0x473318`; `ai_update_perception` `0x47301d` |
| V82 unreachable branch | `ai_can_be_knocked_down` `0x47506a`..`0x475073`; `ai_knock_down_local` `0x4742b3`..`0x4742f8`; `ai_mode_disabled_frame` `0x4b4b6e`..`0x4b4b88` |
| C90 multipass bound | `gr_tmapper_multipass` `0x507a85`, `0x507aab`-`0x507aca`; `gr_poly_vertex` `0x5452f0`-`0x54530c`; `gr_poly_clip_and_draw` `0x5458a7`, `0x54593b`; callers `0x54595c`, `0x55f556` |
| C91 screenshot fatal | `gr_screenshot` `0x506255`-`0x506279`; `gr_d3d_read_backbuffer` `0x53a824`-`0x53a834`, `0x53a8b3`; `DebugError` `0x531d08`-`0x531d10`; `screenshot_save` `0x43263f` |
| K90 OpenGL gamma | `gr_set_gamma` `0x506ac1`-`0x506b7f`; `gr_opengl_tmapper` `0x550127`-`0x550182`; `gl_convert_a8_to_rgba8888` `0x54d5f3`; `options_apply_gamma` `0x41f380`-`0x41f3c1`; `GameInit` `0x43df76`, `0x43e110`; `gr_font_create` `0x5088c1` |
| K91 D3D double gamma | `gr_d3d_tmapper` `0x53622f`-`0x536265`; `gr_d3d_tmapper_multipass` `0x5366b6`-`0x5366fa`; `gr_d3d_set_gamma` `0x53ab27`-`0x53ab6d`; `gr_d3d_init_device` `0x539d8a`-`0x539dc4`; `gr_glide_tmapper` `0x54b2bf` |
| K92 stale fog | `gr_set_fog` `0x507dc9`-`0x507e2f`, `0x507f3f`-`0x507f6d`; `levelrender_fog_enable` `0x482850`-`0x4829c6`; `fog_set_distances` `0x486b10`-`0x486b7a`; `level_set_current_layer` `0x482176`-`0x4821b6` |
| K93 antialias flag | `cmd_antialiasing` `0x505db0`-`0x505dda`; `options_apply_detail` `0x487330`-`0x48733c`; `gr_set_antialias` `0x5080c0`; `gr_d3d_init_device` `0x539dd5` |
| V90 vsync argument | `LoadSettings` `0x43db08`-`0x43db4d`; `gr_init` `0x505e52`, `0x505e87`, `0x505eaa`; `options_apply_detail` `0x48734d`-`0x48735a`; `gr_d3d_flip` `0x53a718`; `gr_glide_flip` `0x54a762` |
| C100 quiet loop | `character_add_xp` `0x44fecb`..`0x44ffa6` (`jle` at `0x44ff1f`, `idiv` at `0x44ffa0`); quiet callers `party_member_join` `0x439cda`..`0x439cea`, `party_set_level` `0x4cecc4`..`0x4cecd3`; highest level `game_stage_set` `0x4d45e2`; progression tables `0x593508`, `0x5935d8`, `0x593678`, `0x593740` |
| G100 leech store | `0x45f441`..`0x45f480`; roll stored over it `0x45feb6`..`0x45febe`; count 0 `spell_roll_amount` `0x47f4ff`; multiplayer `spell_prepare_cast` `0x47ba86`..`0x47babb`; single-player path `combat_trigger_special_ability` `0x46714a`..`0x46721a` |
| K100 thresholds | `character_can_use_summon` `0x44738b`; `ring_check_summon_unlock` `0x450226`..`0x45023a`; `ring_format_info` `0x44bb34` (and six more `cmp eax, 0xea60; jl`) |
| V100 dead stores | die-0 return `spell_roll_amount` `0x47f4ec`..`0x47f4fe`; store `0x45febe`; distance return `0x45e0d8`..`0x45e123` |
| V101 Jade Beam | `0x45f0dd` (`cmp [ebp+0x8], 0x1a`), `0x45f1c4`..`0x45f209` |
| V102 walk_to_point | `0x455eba`, `0x455f24`..`0x455f42` |
| C110 player starts | `level_find_player_starts` `0x4caf91`..`0x4cafd5`; `character_place_at_party_start` `0x44f1c2`..`0x44f1ed`; `level_parse_trigger` `0x4cc4a2`..`0x4cc4c6`; retail `lenele3a.tbl`, `Lenele3a.s3d`, `lenele3d_script.tbl`, `Lenele3d.s3d` |
| C111 0x44 buffer | `lc_find_by_hash` `0x4cd575`, `0x4cd58f`..`0x4cd5ac`; callers `0x4d0fac`, `0x4d3618`; retail `*_script.tbl` `#Clicks` |
| C112 door name | `level_script_parse_table` `0x4cd971`..`0x4cd9ec`; `door_from_lobj` `0x4768ce`..`0x4768df`; retail `*_script.tbl` `#Doors` against `levelm.vpp` `.s3d` |
| C113 table bounds | `level_parse_table` `0x4cb8ed`..`0x4cb9cc`; `level_parse_trigger` `0x4cc1c2`..`0x4cc1d2`; `level_items_parse_table` `0x4cb51f`..`0x4cb540`, `0x4cb735`..`0x4cb745`; `level_script_parse_table` `0x4cdbb1`..`0x4cdbe2`, `0x4cdcd6`..`0x4cdce0`; `level_ambient_add` `0x4cd257` |
| C114 level items | `level_items_parse_table` `0x4cb597`..`0x4cb5ab`, `0x4cb5e8`..`0x4cb5f8`, `0x4cb6a9`..`0x4cb6b6`, `0x4cb6e1`..`0x4cb6f1`; retail `level_items.tbl`, `items.tbl` |
| G110 click object 0 | `level_script_parse_table` `0x4cdba5`..`0x4cdbe2` |
| G111 key -1 | `level_script_parse_table` `0x4cd9ea`..`0x4cda1c`; retail `+Key:` names against `items.tbl` |
| K110 ambient leak | `level_script_leave` `0x4cccaf`..`0x4ccccc`; `level_ambient_remove` `0x4cd2c0`..`0x4cd320`; `level_script_reparse` `0x4cb3d9`; `level_ambient_add` `0x4cd26e`..`0x4cd2ab`; `sound_emitter_delete` `0x5467e4`..`0x54682d`; `sound_create_emitter` `0x546703`..`0x546773`; `sound_emitters_delete_all` `0x5461cd` (caller `0x546650` only); retail `$Ambient:` counts in `tables.vpp` level `.tbl` files |
| V110 avoid spline | `script_alloc_avoid_spline` `0x4cee10`..`0x4cee5c`; callers `0x4dd5c7`..`0x4dd5e2`, `0x4f3314`..`0x4f3327` |
| V111 restart | `level_script_restart` `0x4cb2bc`..`0x4cb339`; `level_items_parse_table` `0x4cb5d6`..`0x4cb5f8`; `level_script_reparse` `0x4cb427` |
| V112 cur_character | `level_script_init` `0x4cc88e`..`0x4cc8ce` |
| V120 fog text | `game_render_frame` `0x438e35`..`0x438f11`; timer constructor `0x501862`, `GameTimer__valid` `0x501990`; flag `0x22fb6f7` read `0x486be0`, only write `0x482392` |
| V121 empty name | `cmd_character` `0x4399cc`..`0x4399e3` (`mov [esp+ecx+0x37], al` at `0x4399da`); lookup failure `charinfo_find_by_name` `0x446bb9`, message `0x4399f2`..`0x439a0d` |
| V122 toggles | `Dump_sound_usage` writers `0x43790f`, `0x43791c`, `0x437929`; `Give_all_items` writers `0x43808f`, `0x43809c`, `0x4380a9`, `ls_test_init` `0x4fde10`; byte scan of the image for `0x609fe1` / `0x609fec` |
| C130 unchecked lookups | `ls_liangshan_init` `0x4f4946`..`0x4f4961`; `ls_jadetemple_init` `0x4f0006`..`0x4f0041`; `ls_jadetemple_tiger_ghost_intro_cs_end` `0x4f0334`..`0x4f034f`; `serpent_set_solo` `0x497a4b`; `ls_wolong2_frame` `0x4f21a7`..`0x4f21ac`; `party_add_member` `0x43411d`, `0x434131`; `ls_wolong2_init` `0x4f20b7`..`0x4f20bc` |
| G130 forge ending | `ls_templeint2_init` `0x4ef1cf`; `ls_templeint2_enter_forge` `0x4ef5d5`..`0x4ef5f4`; `ls_templeint2_machival_death_cs_end` `0x4ef588`; `ls_templeint2_act4_cs_end` `0x4ef287`; `ls_templeint2_use` `0x4ef790`..`0x4ef7f9`; `savegame_location_allowed` `0x4d4a50`..`0x4d4a58` |
| G131 Laharah fade | `lsf_tiger2_dead` `0x4f053e`..`0x4f0574`, `0x4f057c`..`0x4f05a8`, `0x4f05d9`..`0x4f0670`; `ls_jadetemple_init` `0x4f0049`..`0x4f0058`; `GameTimer__elapsed` `0x501914`; retail `Jadetemple.tbl` `#Triggers` empty |
| G132 wave 4 | `lsf_liangshan_flag_wave3` `0x4f5248`..`0x4f5261`; `lsf_liangshan_flag_wave4` `0x4f5299`..`0x4f52b9`; `lsf_liangshan_flag_finished_quest` `0x4f533d`..`0x4f5383`; retail `Liangshan_script.tbl` `Orenian Samurai#w3a` |
| K130 boss music | `ls_liangshan_init` `0x4f49be`; `ls_liangshan_frame` `0x4f4a91`..`0x4f4aac`, `0x4f4ab4`..`0x4f4b09`; `level_script_frame` `0x4ccabe`..`0x4ccaf4` |
| K131 act 4 narration | `ls_templeint2_init` `0x4ef1c3`; `ls_templeint2_frame` `0x4ef1e0`..`0x4ef1fb`; `ls_templeint2_act4_cs_end` `0x4ef230`..`0x4ef237`; retail cutscene.tbl "Nar-TempleAct4", events.tbl templeint2 flags |
| V130 cryptintro | `ls_catacombs_frame` `0x4fe1e6`..`0x4fe227`; only reference to `0x5fc014` is `0x4fe1e6`; retail cutscene.tbl (no "cryptintro") |
| V131 wolong2_charge | `ls_wolong2_init` `0x4f208c`..`0x4f209e`; `ls_wolong2_force_alert_action` `0x4f1ef2`..`0x4f1f17`; references to `0x5a26e0`: `0x4f1f0c`, `0x4f2093`; retail `wolong2_dlg.tbl` "Charge" trigger |
| C140 console tokens | `read_word` `0x50aacd`..`0x50aadf`; `read_quoted_string` `0x50ab00`; `make_lowercase_token` `0x50aa52`..`0x50aa75`; `execute_command_line` `0x50a22b`..`0x50a244`, `0x50a3d8`..`0x50a406`; `process_command` `0x50a985`; `cmd_character` `0x43995e`..`0x4399c9`; `src/summonerfix/keys.c` `CONSOLE_CMD_LEN` 96 |
| C141 game-intro hold | `script_entry_start` `0x4dd211`..`0x4dd245`; `charinfo_find_move_any` `0x446ca2`; retail `Game-Intro_cscript.tbl` |
| C142 saved equipment | `script_action_replace_weapon` `0x4dcd3d`..`0x4dcd87`; `script_action_replace_armor` `0x4dcf94`..`0x4dd08b`; `cutscene_script_unload` `0x4ed417`; `cutscene_script_rewind_actors` `0x4ed5e0`; retail `*_cscript.tbl` "replace weapon" / "replace armor" |
| C143 script object bounds | `script_parse_objects` `0x4dbfb9`..`0x4dc042`, `0x4dc0fb`..`0x4dc128`; retail `*_script.tbl`, `*_cscript.tbl` `#Objects` |
| C144 chat scroll | `chat_add_line` `0x41ed8f`..`0x41eddc` |
| G140 stale slot | `script_action_replace_weapon` `0x4dcd55`..`0x4dcd87`; `script_action_replace_armor` `0x4dcf94`..`0x4dcfb9`, `0x4dcffe`..`0x4dd023`; `cutscene_script_unload` `0x4ed3a6`..`0x4ed3bf`; retail `items.tbl` `+Slot:` |
| K140 item_types[-1] | `script_action_replace_armor` `0x4dcea9`..`0x4dcecf`, `0x4dcee8`; `item_types` `0x8cbec0` |
| V140 trigger copy | `script_parse_objects` `0x4dba7f`..`0x4dbb78`, `0x4dbc30`..`0x4dbc74`, `0x4dc06c`..`0x4dc0b0`; readers `savegame_refresh_character_flags` `0x4cfed5`..`0x4cfeed`, `0x4dc0d8`..`0x4dc0e6`; retail `rand-hills01_script.tbl` |
| V141 .vcs path | `load_script` `0x50ad05`..`0x50ad49`, `0x50ae24`..`0x50aeac` |
| V142 %f | `process_command` `0x50a7d7`..`0x50a7e8` |
| V143 duplicates | `console_command__console_command` `0x50a188`, `0x50a1b1`, `0x50a1d7`..`0x50a1dc` |
| C150 Glide vertex buffer | `gr_glide_tmapper` `0x54b284`..`0x54b383`, `0x54b390`..`0x54b396`; `gr_glide_tmapper_multitexture` `0x54b3d9`, `0x54b496`, `0x54b520`; next symbols `0x2f0f1e4`, `0x2f0f1e8`, `0x2f0f1f8`, `0x2f0f208`, `0x2f0f210`; callers `gr_tmapper` `0x507a57`, `gr_tmapper_multipass` `0x507c75`, `0x507cd1` |
| C151 unchecked lookups | `ls_templeint_frame` `0x4f622b`..`0x4f6249`; `tiger_find_rider` `0x49a010`; `lsf_lenele1c_flag_found_flece` `0x4fabf1`..`0x4fac1d`; `character_equip_item` `0x46be19`; `party_member_join` `0x439bb1`..`0x439beb`; `ls_eleh_frame` `0x4f1684`..`0x4f1689`; `ls_wolong_frame` `0x4fea05`..`0x4fea0a`; `party_add_member` `0x43411b`..`0x43413a`; retail `TempleInt_v2_script.tbl` "Tiger Rider#1" |
| K150 Priest King music | `ls_ikaemostopint_init` `0x4f9ef1`, `0x4f9f2e`..`0x4f9f34`; `lsf_ikaemos_found_boss` `0x4fa050`..`0x4fa094`, `0x4fa0b1`..`0x4fa102`; `lsf_ikaemos_boss_killed` `0x4fa140`..`0x4fa180`, `0x4fa1ba`..`0x4fa225`; retail `cutscene.tbl` IkmsPKing-Intro, `music.tbl` IkaemosPalace |
| K151 alpha sections | `gr_glide_create_texture_section` `0x548666`..`0x5486ee` (format 2), `0x54893b`..`0x5489c9` (format 6 with section origin); `gr_glide_create_texture` `0x549424`..`0x54947b`; `bm_read_header` `0x502b4a`..`0x502ba8`, tables `0x502df4`, `0x502e08`; `bm_read_vbm_header` `0x501df8`..`0x501e1b`; `bm_create` callers `0x42567f`, `0x46f1a4`, `0x48a02c`, `0x4d3993`, `0x5030c2`, `0x508934` |
| V150 dead reload | `gr_glide_tcache_set` `0x5495bc`..`0x5495e8`, `0x549612`..`0x549686`; stores `0x549390`, `0x5495dd`, `0x549acb`, `0x549aec`; `gr_glide_tcache_init` `0x547a57`..`0x547a7c`; references to `0x2f0ec00` |
| V151 temple refusal | `ls_lenele3a_use` `0x4f1034`..`0x4f105c`; `ls_lenele3a_init` `0x4f0f87`..`0x4f0fed`; `level_parse_trigger` `0x4cc2ec`..`0x4cc32f`; `level_check_triggers` `0x4cac75`..`0x4cac79`, `0x4cac9a`..`0x4cad2c`; retail `lenele3a.tbl` `#Triggers` |
| V152 release order topic | `ls_lenele1b_action` `0x4fb1e6`..`0x4fb23b`; `script_topic_toggle_by_item` `0x4ceb80`..`0x4cebbe`; `script_topic_toggle_by_item_inv` `0x4cebc0`; retail `lenele1b_dlg.tbl` Gorbus the Guard |
| V153 animated rebuild | `gr_glide_mark_texture_dirty` `0x5496e8`..`0x549718`; `gr_glide_create_texture` `0x549337`; `gr_glide_tcache_set` `0x5495bc`..`0x5495c9`; `gr_mark_texture_dirty` callers `0x427215`, `0x4d2997`, `0x5022f3`, `0x50339c` |
| C160 sweep pending | `determine_point_visibility` `0x4b839a`..`0x4b83a6`, `0x4b84ac`..`0x4b84f0`, `0x4b8542`..`0x4b8585`, `0x4b86a3`..`0x4b86ad`, `0x4b86bf`; `pathvis_scan_cells` `0x4b7a09`, `0x4b7a21`; `pathvis_insert_edge` `0x4b7f41`; `pf_compute_endpoint_visibility` `0x49f36b`..`0x49f385`; `cmd_precalc_vis_method` `0x49c88f`..`0x49c8a9` |
| C161 BSP type | `pf_bsp_node_find` `0x4a5c72`..`0x4a5c81`, `0x4a5d16`; `pathvis_bsp_contains` `0x4b86f2`..`0x4b8701`, `0x4b8795`; `pf_read_typed_refs` `0x49ca44`..`0x49ca53` |
| G160 path type 2 | `cmd_Multi_path_type` `0x4811e8`..`0x4811f1`, `0x481215`..`0x481222`; `mp_handle_new_path` `0x48026b`, `0x4802d2`..`0x4802da`, `0x480381`..`0x480401`, `0x480425`..`0x480434` |
| K160 join line | `mp_add_new_player` `0x4af5a6`..`0x4af5d4`; `mp_handle_new_info` `0x4af4cb`..`0x4af4d0`; `mp_handle_net_info` `0x4af77f`..`0x4af784`; `mp_server_accept_connections` `0x440d4f`; `mp_host_update_session` `0x440dc8`..`0x440de1`; `mp_broadcast_packet` `0x441ad7`..`0x441aed` |
| K161 disc 2 box | `cd_prompt_disc2` `0x43ea6f`..`0x43ea89`, `0x43ea9c`, `0x43eab3`; `level_load` `0x4385cf`..`0x4385f0` |
| C170 message box line | `ui_message_box_draw` `0x414339`..`0x414360` (`gr_split_str`, 16 lines, width `0x57efd0`), `0x414370`..`0x414399` (`strncpy` into `[esp+0x20]`, terminator), locals `[esp+0x60]` / `[esp+0xa0]`; `ui_message_box_show` `0x413e71`..`0x413e84` |
| C171 multiplayer cycle | `party_cycle` `0x413370`..`0x4133ac` (NULL loop `0x413392`, start test `0x413396`); single-player branch `0x4133d8`..`0x413408` |
| G170 pausing box | `ui_message_box_show` `0x413f73`..`0x413f96`; `game_pause` `0x432660`..`0x432667`; `ClearFrameFlag5bd504` `0x414020`..`0x414039`; `ui_message_box_handle_click` `0x4140d8`..`0x4140e8`; `hud_handle_keys` `0x40deea`, `0x40df22`..`0x40df2f`, `0x40dff5`..`0x40e002`; `hud_key_save` `0x40e1aa`..`0x40e1c4`; `quicksave` `0x40e857`..`0x40e873`, `0x40e910`, `0x40e925`..`0x40e934` |
| K170 title gap | `ui_choice_popup_hit_test` `0x413988`..`0x4139d1`; `ui_choice_popup_draw` `0x413d39`..`0x413d4e`; `ui_choice_popup_show` `0x413805`..`0x413811`; gap table `0x57ef7c` = {5, 5, 5}; `death_popup` `0x40f690`..`0x40f6c9`; `ui_choice_popup_update_buttons` `0x413ae2`..`0x413b24`; titled callers `0x40f6c9`, `0x42368c`, `0x42387e` |
| K171 lock value kept | `character_use_skill` `0x4295d3`..`0x429693`; `door_pick_by_mesh` `0x4aaa81`..`0x4aab57`; `mp_handle_door_lockpick` `0x4aa838`..`0x4aa8c3`; `door_toggle_state` `0x476f21` |
| V170 double leave | `interface_set_screen` `0x41262c`..`0x4126ee` (leave switch, table `0x4129d8`), `0x4126ee`..`0x41270f` (case bytes `0x412a3c`), restarts `0x412770`, `0x4127b9`, `0x4127d3`, store `0x4127f8`; `pickup_leave` `0x420960`..`0x420999` |
| V171 whole-file reader | `packfile_read_whole_file` `0x534900`..`0x534993`; caller `file_blast_into_buffer` `0x517299`; no reference to `0x517220` |
| C180 name tables | `localize_load_names` `0x504588`..`0x504594`, `0x50464c`; `localize_load_level_table` `0x5049c9`..`0x5049d8`, `0x504a52`, `0x504acd`..`0x504ad3`; `localize_set_string` `0x50440f`..`0x504417`; `localize_load_strings` `0x504389`..`0x5043c6`; `localize_strdup` `0x5040a9`..`0x5040d2`; `allocate_in_mempool` `0x5127c8`..`0x5127da`; retail `item_names.tbl`, `quest_names.tbl`, `topic_names.tbl`, `script_names.tbl`, `strings.tbl` |
| C181 reliable length | `nw_work_reliable` `0x518391`..`0x5183ee`, `0x517f0d`..`0x517f2f`; `nw_NewReceiveReliable` `0x518c6d`..`0x518c8f` |
| G180 kept peer | `nw_work_reliable` `0x517e14`, `0x518044`..`0x518088`, `0x5180b2`, `0x5180cc`, `0x518232`..`0x518283`, `0x518391`..`0x518413`, loop `0x51841b`..`0x518437` |
| G181 sequence wrap | `nw_work_reliable` `0x5182f8`..`0x51834a`, `0x51835f`..`0x518372`, `0x5183ff`..`0x518413`; `nw_NewReceiveReliable` `0x518c33`..`0x518c50`, `0x518cc5` |
| G182 kept indices | `nw_ConnectToServer` `0x519118`..`0x519150`; `mp_handle_acpt_jn` `0x470aac`..`0x470ad3`; `nw_NewSendReliable` `0x518dd0`..`0x518df3` |
| K180 text buffer | `localize_set_string` `0x504401`..`0x504448`, `0x504477`..`0x5044a7`; `localize_pool_used` `.data 0x5a5c30` = `0x25ce47c` |
| K181 French Ä | `localize_remap_charset` `0x5051b6`..`0x5051dc`, `0x505206`, `0x505210`, tables `0x50531c`, `0x505384`; `localize_init` `0x504f61`..`0x504f65`; writes of `0x25d8878` only at `0x5050b7` |
| V180 thread priority | `os_wndproc` `0x5310b7`..`0x5310bf`, `0x5310ef`..`0x5310f8`; imports `0x57e1dc` (`SetThreadPriority`), `0x57e3bc` (`SetWindowPos`); `os_init` `0x530e25`; `0x2cbff0c` at `0x530bc5`, `0x530e46` |
| V181 duplicate request | `nw_work_reliable` `0x517f35`..`0x517f91`, `0x517fa7`..`0x518013`, `0x518028`..`0x51803f`, `0x518461`..`0x5184b0`; `nw_ConnectToServer` `0x519034`..`0x51904c` |
| V182 stack bytes | `nw_send_heartbeat` `0x517d70`..`0x517dab`; `nw_send_reliable_reply` `0x517d20`..`0x517d5b`; `nw_work_reliable` `0x517e60`..`0x517e8a`, `0x51812a`..`0x51814c`, `0x5181a6`..`0x5181c9` |
| V183 localize slips | `localize_load_level_table` `0x5049a1`..`0x5049c7`; `localize_table_add` `0x5040f2`..`0x504119`, `0x50414f`, `0x50416d` |
| V184 clipboard | `os_clipboard_get_text` `0x531b51`..`0x531be0`; imports `0x57e384` (`OpenClipboard`), `0x57e388` (`GetClipboardData`), `0x57e1bc` (`GlobalLock`), `0x57e1c0` (`GlobalUnlock`), `0x57e3b8` (`CloseClipboard`) |
| C190 fog key -1 | `cutscene_get_fog_distances` `0x4354a9`..`0x4354e9` (loop), `0x4354eb`..`0x435510` (`fogs[i - 1]`); caller `cutscene_events_frame` `0x4370dd`..`0x4370f7`; retail `cutscene.tbl`: 84 entries, first `$Fog` at shot 0 in each |
| C191 table bounds | `cutscene_parse_table` `0x4355e0` (no compare against 85, 4, 2, 10, 4, 16); `cutscene_parse_visset` `0x435ff0` (its 16 checks); `cutscene_load_files` `0x434d0e`..`0x434d22`, `0x434d57`..`0x434da9`; retail maxima 84 / 4 / 1 / 7 / 0 / 13 / 16 fades / 13 vissets, 48 shots (Act2-Outro) |
| C192 dialog arguments | `editor_dialog_show` `0x431033`..`0x43103f` (`ja 0x431563`, jump table `0x4315a0`); `editor_dialog_destroy` `0x4316a0`; callers `0x4ea0b8`..`0x4ea0d4`, `0x4ead94`..`0x4eadad`, `0x4eadcb`..`0x4eade4` |
| C193 load-list NULL | `loadgame_level_list_cb` `0x432215`..`0x43222c`; installed by `state_loadgame_enter` `0x431f73`; state 8 has no `gameseq_set_state` caller |
| K190 stale soundtrack | `cutscene_unload` `0x4352c4`..`0x4352f1`; `cutscene_load_sounds` `0x43502b`..`0x435056`; `cutscene_sequence_update` `0x436bb4`..`0x436bc3`; `cutscene_cleanup` `0x436660`, `0x43667a`, `0x4366aa`, `0x4369fa`; `cutscene_load_data` `0x435472`; `snd_get_handle` `0x520805`..`0x520816`; `snd_free` `0x520c10`..`0x520c45`; `snd_level_release` `0x520ce0`..`0x520d32`; `summon_create` `0x47ef57`; `ls_templeint2_machival_intro_cs_end` `0x4ef2c2`; `ls_wolong2_pijian_intro_cs_end` `0x4f1e2d`; retail `cutscene.tbl`: 7 entries without `$Soundtrack` |
| K191 edge block | `camera_process_input` `0x4346e7`..`0x43471b`, `0x43471d`..`0x434724`, `0x43472d`, `0x434736`..`0x43476e`, `0x4347c2`..`0x4347e6`; only references to `0x5fccb7` / `0x5fccb8` |
| K192 minitracks | `cutscene_load_sounds` `0x43506c`..`0x43509b`; `cutscene_events_frame` `0x436f57`..`0x436fd0` |
| K193 screenshot number | `screenshot_save` `0x432539` (mode 1), `0x432587`..`0x4325a9`; `file_find_files` `0x5134f4`..`0x513519`, `0x513574`..`0x513586` |
| K194 usage | `cmd_play_cutscene` `0x43753d`; `cmd_play_and_load_cutscene` `0x4375fd` (both `push 0x593b90`) |
| V190 screenshot list | `screenshot_save` `0x432552`..`0x43264f` (no `operator_delete`); `file_find_files` `0x5134d5`..`0x5134d7`; `quicksave` `0x40e82d` |
| V191 third callback | `editor_dialog_handle_event` `0x431616`..`0x431628` |
| V192 debug Backspace | `PollInputEvents` `0x432787`..`0x43278f`, `0x4327d9`..`0x4327e1`; `key_mark` `0x5094f4`..`0x509508` |
| V193 handle as signature | `cutscene_unload` `0x4352d3`..`0x4352ec`; `snd_stop` `0x5215d0`..`0x5215f1`; `ds_stop` `0x55d620`; `ds_get_channel` `0x55db00`..`0x55db27`; `ds_play_streamed` `0x55d3d3`..`0x55d3e6`; `music_play` `0x4690b2`..`0x4690c4` |
| C200 game list record leak | `state_mp_gamelist_enter` `0x43eed0`..`0x43eed9`; `mp_local_player_create` `0x44176d`..`0x4417aa`; `mp_player_alloc` `0x441610`..`0x44161e`; `gamecreate_cancel` `0x40a020`; `gamelist_create_game` `0x40b3b0`; `mp_gamelist_free` `0x470820`; `mp_session_reset` `0x4414a7`..`0x4414fb` (only caller `mp_charselect_init` `0x40659a`); lobby skip `0x43eff0`..`0x43eff8`; LAN setting `options_apply_and_save` `0x41faad`..`0x41fad3` |
| C201 type 0x42 / DMY_PCK | `multi_process_packet` `0x441869`..`0x441877` (range), `0x4418a6`..`0x44191b` (rate copy), `0x44194f`..`0x441970` (handler call), `0x441979`..`0x44198f` (loop); record 66 at `0x5958f0`; DMY_PCK record `0x5958b8`; `multi_process_incoming` `0x441b4e`..`0x441c5d`; `multi_do_frame` `0x4413d0`..`0x4413eb` |
| C202 NULL profile | `mp_local_player_create` `0x441787`..`0x4417c5`; `mp_charselect_get_selected_profile` `0x408540`..`0x40857e`; lobby connect step `0x43f129`..`0x43f150` |
| C203 layout char_info | `monster_layout_load` `0x43feff`..`0x43ff0c`, `0x44001e`, `0x440063`..`0x440084`, `0x4400fd`..`0x440113`; `charinfo_find_by_name` `0x446bb7`..`0x446bbd` |
| K200 channel list | `lobby_parse_channel_list` `0x43f4b9`..`0x43f4dc`, `0x43f544`, `0x43f549` |
| V200 empty mlayout name | `cmd_load_mlayout` `0x4401b8`, `0x44023d`..`0x440250`, `0x440272`; `cmd_save_mlayout` `0x44038d`..`0x4403a0`; `execute_command_line` `0x50a50a`..`0x50a54d`; `print_to_console` `0x50a5a0` |
| V201 unstored link | `mp_server_accept_connections` `0x440d29`..`0x440d62`; `mp_server_check_clients` `0x440c96`..`0x440cad` |
| C210 doubled count, list overflow | `ai_group_think_caster` `0x4936c0` (frame), `0x4937ea` (list at `esp+0x30`), `0x49395c`, `0x493988`, `0x4939c5`, `0x4939f2`..`0x493a05`; `ai_group_combat_think` `0x492db5`..`0x492dbf`, `0x492fd4`..`0x492fd8`; `ai_groups_frame` `0x492d30`..`0x492d38`, `0x492d7d`, `0x492d8b` |
| C211 camera key pools | `cutscene_camera_read_track` `0x494076`..`0x4940d1`, `0x49415c`..`0x4941ba`; `cutscene_camera_tracks_clear` `0x494680`; `cutscene_load_files` `0x434ba8`; retail `cutscene.tbl` and `cutscene.vpp` `.CSC` files |
| G210 cone | `ai_group_collect_enemies` `0x493bac`..`0x493be8`, `0x493bf9`..`0x493c5c`; `script_guard_sees_player` `0x4ce882`..`0x4ce893`; `Archlich.tbl` |
| G211 second order | `ai_group_think_style1` `0x49353c`..`0x49355f`; `ai_goal_add_attack` `0x473fc5`; `ai_goal_add` `0x476197`..`0x4761c8`; `ai_group_shuffle_enemies` `0x493ce0` |
| G212 Protect threshold | `ai_group_think_caster` `0x49395c`, `0x493988`, `0x4939b5`..`0x4939d7` |
| G213 asleep index | `serpent_choose_attack` `0x496678`..`0x4966aa`, `0x4966ce`, `0x496706`..`0x496751`; `serpent_part_is_asleep` `0x496060`; `serp_boss.tbl` stage 1 |
| G214 negated point | `phoenix_reposition_update` `0x497f42`..`0x497fae` |
| G215 Inferno | `phoenix_special_strike` `0x4998c0`..`0x499999`; `boss_attack_trigger_dispatch` `0x4a0d5f`; `phoenix_boss.tbl`; `spells.tbl` "inferno" `+Distance:` 6 |
| G216 threat base | `tiger_pick_target` `0x49b1fe`..`0x49b250`, `0x49b276`..`0x49b28f`; `boss_target_list_pick` `0x4a3257`..`0x4a326a` |
| K210 snow +x refill | `snow_update` `0x494d7e`..`0x494dbf` (+x), `0x494df5`..`0x494e00`, `0x494e49`..`0x494e54`, `0x494e95`..`0x494ea0`; `snow_cell_update` `0x494a85`, `0x494a9b`..`0x494a9f` |
| K211 Phoenix rider time | `phoenix_seat_rider` `0x4982e5`..`0x498303`; `serpent_seat_rider` `0x495e55`..`0x495e76` |
| K212 Tiger stage 2 anims | `tiger_set_stage2_anims` `0x49a144`..`0x49a15c`; `tiger_load_anims` `0x49abaa`; callers `0x49c38f`, `0x49c3d9`; `Tiger Rider.tbl` |
| K213 illusion | `serpent_illusion_start` `0x496bd7`..`0x496c0d`; `serpent_ai_frame` `0x497417`..`0x497430`; only pending write `0x4965ec` |
| V210 enemy count -1 | `ai_group_remove_member` `0x492ce8`..`0x492d07`; `ai_group_collect_enemies` `0x493b22` |
| V211 flinch | `serpent_rider_set_flinch` `0x495579`..`0x4955af`; `phoenix_rider_set_flinch` `0x4987d9`..`0x49880f`; `tiger_rider_set_flinch` `0x49a259`..`0x49a28f`; `Phoenix Rider.tbl` |
| V212 ready_hurt | `phoenix_set_foot_anims` `0x499c23`..`0x499c3a`; `Phoenix Rider.tbl` |
| V213 Tiger axe | `tiger_unload` `0x49ae90`..`0x49ae9f`; `tiger_equip_weapon` (`item_create_weapon` for "TigerAxe"); `boss_ai_unload` `0x4a04f8`; `ai_state_shutdown` `0x47243b` |
| V214 has_rider | `serpent_boss_load` `0x49599a`..`0x4959cc`, `0x495b73`; `serpent_spawn_rider` `0x495745`..`0x49574b`; `living_entity_create` `0x461c23`; `ai_reset` `0x47245a`..`0x47246e` |
| C230 cast goal target | `ai_goal_evaluate` `0x4759b7`..`0x4759e8`, jump table `0x475cd4`; `ai_goals_validate` `0x47662c`; `cutscene_script_is_active` `0x4ed770` |
| C231 door edges | `door_find_floor_point` `0x476b2f`..`0x476b61` (unbounded fill), `0x476bb5` (count), `0x476bee` (compare), `0x476ce4`..`0x476d55` (ray block); caller `level_restore_doors` `0x438a14` |
| C232 camera target | `camera_frame` `0x478a37`..`0x478a42`, `0x478a75`..`0x478a90`, `0x478b5e`..`0x478b6c`; `camera_set_target` `0x479009`..`0x47901d` |
| G230 unstored goal | `ai_goal_alloc` `0x476048`..`0x47608b`, `0x476178`; `ai_goal_add` `0x47625e`..`0x4762ec`; `ai_goal_remove` `0x47632c`; `ai_goal_type_fields` table `0x476008` |
| K230 orientation packing | `matrix3_pack_orientation` `0x4753f3`..`0x475425`; `matrix3_unpack_orientation` `0x475437`..`0x475517`; `mp_send_new_lep` `0x47fe7f`; `monster_layout_load` `0x44012b`; `mp_handle_new_lep` `0x47ff6d`..`0x47ff88`; `mp_handle_obj_update` `0x480805`..`0x480849` |
| K231 height sign | `camera_smooth_height` `0x478736`..`0x47879c`; constants `0x580e18` 4.0, `0x581938` 0.001; `frametime_initialize` `0x5014ee`; `frametime_unlimit` `0x501750` (no caller) |
| V230 goals[-1] | `ai_goal_alloc` `0x47606b`..`0x47606d`, `0x47615f`..`0x476176` |
| C220 editor NULL pointers | `script_make_filename` `0x4e8d18`..`0x4e8d3c` (`[0x28]` at `0x4e8d34`), `0x4e8d43`..`0x4e8d4f`; `script_editor_apply_action_params` `0x4e878d`..`0x4e87c0`; `script_editor_show_script_dialog` `0x4e6ff3`..`0x4e7032`; `cutscene_script_unload` `0x4ed51d`; `script_editor_cutscene_button_cb` `0x4e7929`..`0x4e7945`; caller `script_create` `0x4e8a38` |
| C221 copy loops | `script_write_level_table` `0x4ea269`..`0x4ea2c9`; `level_items_write_table` `0x4ea5b2`..`0x4ea63a`; `TextFile__ReadLine` `0x516017`..`0x516047`; retail `tables.vpp`: 49 level tables with `#Ambient Sounds` and CRLF, `level_items.tbl` ends with `#End` |
| C222 camera navpoints | `script_editor_delete_navpoint` `0x4e6a30`..`0x4e6a76`; only caller `script_editor_on_new` `0x4e6a14` (jump table `0x4e6a20`, mode 2) |
| G220 orphan actions | `script_write_objects` `0x4e9df4`..`0x4e9e07`, `0x4e9e3d`..`0x4e9e47`, `0x4e9ff5`..`0x4ea075`, `0x4ea0e0`..`0x4ea1b9`; `script_parse_objects` `0x4dbf8c` |
| G221 shared level table | `script_write_level_table` `0x4ea2ff`..`0x4ea359`, `0x4ea40e`..`0x4ea462`; `level_parse_ambient` `0x4cc0ea`..`0x4cc10f`; `TextFile__SkipTo` `0x515bc1`..`0x515bd3`; retail `tables.vpp`: each level `.tbl`'s `$Ambient:` names against the `$Name:` records of every `<level>_*script.tbl` |
| G222 stale level item navpoints | `level_items_parse_table` `0x4cb5dc`..`0x4cb5f8`; `level_items_write_table` `0x4ea6ae`, `0x4ea9cd`..`0x4ea9d7`; references to `level_items` (`asm.mjs grep 0x24e3b50`); retail `level_items.tbl` sections with several `$Script:` values |
| G223 navpoint delete | `script_editor_on_delete` `0x4e6b63`..`0x4e6bcc`, `0x4e6c2b`..`0x4e6c96`; `script_replace_navpoint_references` `0x4e6f10`..`0x4e6f9c`; `script_editor_navpoint_in_use` `0x4e6d93`..`0x4e6ede`; `script_write_level_script` `0x4e9592`..`0x4e95c0` |
| G224 wrong table | `script_editor_show_action_params` `0x4e7d42`..`0x4e7da8` |
| G225 stale indices | `script_editor_level_item_ok` `0x4ece78`..`0x4eceee`; `level_items_parse_table` `0x4cb6b6` |
| G226 creature OK | `script_editor_creature_ok` `0x4ec801`..`0x4ec922`, `0x4ec961`..`0x4ec9b9`; constants `0x57eb84` = 0.0, `0x57e6b4` = 100.0 |
| G227 `+Prop:` | `script_parse_objects` `0x4dbdbd`..`0x4dbf84`; `script_write_objects` `0x4e9ce0`..`0x4ea1c0` (no `+Prop` string); retail `*_script.tbl` `+Prop:` counts |
| G228 Pyrul fall-through | `pyrul_fire_attack` jump table `0x4b99b0` / index bytes `0x4b99c4`; `0x4b98b5`..`0x4b98f6`, `0x4b9902`; spells.tbl ids `0x1c` meteor storm, `0x1d` wall of fire, `0x4f` Pyrul meteor storm; `init_level_table` store `0x59ff18` = 27 (KhosaniLab2), flags `0x801` |
| G229 support pick | `ai_score_support_targets` `0x4bea45`..`0x4bea58`; `ai_choose_support_spell` `0x4beb0d`..`0x4beb62`; callers `0x4937f7`, `0x4bfce2` |
| G240 life drain scores | `ai_consider_life_drain` `0x4bf631`..`0x4bf684`; `ai_choose_offensive_spell` `0x4bf854`..`0x4bf916`; species `0xb9` Wraith (`species_table` entry 181) |
| G241 area spell distances | `ai_combat_cast_frame` `0x4bfc6d`..`0x4bfc7d`, `0x4bfd8c`..`0x4bfdde`; `ai_consider_area_spell` `0x4befe6`, `0x4bf019`..`0x4bf040`; calls `0x4bf862`..`0x4bf87c` |
| G242 x twice | `pf_first_edge_crossing` `0x4a5441`..`0x4a5462`; callers `0x49f0a6`, `0x4a5516`, `0x4a562c`, `0x4b1964`, `0x4dde71` |
| G243 seglist flags | `avoid_create_obstacle_polys` `0x4a9a29`..`0x4a9a6c`, `0x4a9ae5`..`0x4a9b16` |
| K220 door parameters | `script_editor_list_door_sounds` `0x4e5539`..`0x4e565e`; `script_editor_door_ok` `0x4ec33c`..`0x4ec384`, `0x4ec42a`..`0x4ec472` |
| K221 hover marker | `script_editor_frame` `0x4e035d`..`0x4e0383` |
| K222 door Enter | `script_editor_door_sound_inputbox_cb` `0x4ebc60`..`0x4ebc6a` |
| K223 missile bone | `pyrul_get_missile_origin` `0x4b95f0`..`0x4b9665`; `$missile01` in retail `Pyrul.vim` |
| V220 attack point -1 | `ai_approach_target_point` `0x4bd008`, `0x4bd054`..`0x4bd162`, `0x4bd279`..`0x4bd285` |
| V221 one past | `script_editor_on_delete` `0x4e6b5f`..`0x4e6bcc`, `0x4e6c27`..`0x4e6c96` |
| V222 sound slots | `script_editor_ambient_ok` `0x4eb480`..`0x4eb4c5`; `level_ambient_remove` `0x4cd2d8`; `snd_get_handle` `0x520818`..`0x520884` |
| V223 shared end point | `seg2d_segment_intersect` `0x4a57b0`..`0x4a57e6`; `pf_find_cell_crossing` `0x49edb5`; other callers `0x4a527d`, `0x4a55dc`, `0x4b16fe`, `0x4b182c`, `0x4b8328` |
| V224 segs[-1] | `avoid_seglist_ensure_corner_offset` `0x4a82c7`..`0x4a8309`; `avoid_merge_shared_segments` `0x4a8c5c`..`0x4a8cef` |
| C250 beam hit | `luminar_beam_hit` `0x4ca3b4`, `0x4ca4d9`..`0x4ca502`; `luminar_ai_frame` `0x4c95f8`..`0x4c9613`, `0x4c9615`..`0x4c962c`, `0x4c9722`; byte table `0x4c977c`; `luminar_beam_start` `0x4ca28c`, `0x4ca32a`..`0x4ca330`; `luminar_ai_beam_move_end` `0x4ca206`..`0x4ca211`; `character_add_hp` `0x45085c`; `Luminar.tbl` "attack_beam" |
| C251 arr line | `File__read_line` `0x525f81`..`0x525fab`; `effects_parse_arr_line` `0x51ca60`, `0x51ca77`, `0x51cba8`; `summoner.vpp` `effects.arr` (206) and `ccrunch.arr` (241) |
| C252 arrow divide | `ui_button__draw_arrow` `0x527448`..`0x5274ad`, `0x5274d9`, `0x527504`, `0x52750b`, `0x5275dd`, `0x5275e6`; `ui_button__draw` `0x527dce`..`0x527de3`; arrow buttons `0x4e0e45`, `0x4e0e9e`, `0x55e8ea`..`0x55e9e2` |
| C253 any path | `file_path_id_valid` `0x512cd4`..`0x512cdd`; `file_make_path` `0x512b37`, `0x512b8d` |
| C254 fifth tentacle | `tentacle_beast_boss_load` `0x4c6d77`..`0x4c6d9f`; `tentacle_beast_grab_timer` `0x248c768`; `sewerboss_script.tbl` lines with `Tentacle Beast#$npc057`..`#$npc060` |
| G250 tentacle grab | `tentacle_beast_ai_select_action` `0x4c666d`..`0x4c66b4`, `0x4c6753`, `0x4c67de`; `frand` `0x5426e0`; `tentacle_beast_ai_frame` `0x4c72b6`..`0x4c72cf`, `0x4c7122`..`0x4c7150`; `tentacle_beast_boss_load` `0x4c6fe5`; `tentacle_beast_ai_release` `0x4c6b53`; `character_add_hp` `0x4508d7`; `Tentacle Beast.tbl` "Luminar grab" / "Luminar throw" |
| G251 tentacle hits | `tentacle_collect_area_hits` `0x4c77f1`, `0x4c7870`..`0x4c7887`; `tentacle_beast_boss_load` `0x4c6e6b`; `tentacle_beast_check_stage` `0x4c73fe`..`0x4c7452`, `0x4c745c`..`0x4c7487`; `ai_group_add_member` `0x492c59`; `ls_sewerboss_frame` `0x4ee165` |
| G252 Murod Oni timer | `murod_oni_init_timers` `0x4c5dcf`..`0x4c5dd9` (only caller `0x4c5ccf`); `murod_oni_ai_select_action` `0x4c5ea5`..`0x4c5eb5`, `0x4c5ec7`..`0x4c5f27`; `murod_oni_evaluate_target` `0x4c6205`..`0x4c621e`; `murod_oni_boss.tbl` third entry |
| G253 dead anchors | `murod_start_leap` `0x4c51cb`..`0x4c51e9`, `0x4c5204`..`0x4c52dd`; `luminar_teleport_out` `0x4c9dc5`..`0x4c9de6`, `0x4c9f9f`; `luminar_ai_beam_move_end` `0x4ca20a` |
| K250 shadow receivers | `projector_collect_lobj_faces` `0x4c0595`..`0x4c05d7`; `shadows_render` `0x4a438e`..`0x4a439a` |
| K251 Urath leap | `evil_urath_leap_check_path` `0x4c420b`, `0x4c4321`..`0x4c4338`, `0x4c435b`..`0x4c4374`; `evil_urath_ai_leap_rise` `0x4c48e7`..`0x4c491c`, `0x4c49d6`..`0x4c4a64` |
| K252 32-character name | `File__read_string` `0x525f4a`..`0x525f54`; `FileReader__read_string` `0x513e29`..`0x513e35`; `mp_player_file_read_summary` `0x46d570`, `0x46d591`; `mp_player_file_load` `0x46d73d`; `mp_charselect_handle_click` `0x406e05` |
| V250 Titus target -1 | `titus_ai_select_action` `0x4c2c4a`..`0x4c2cb2`, `0x4c2cf0`, `0x4c2d4e`, `0x4c2d61`, `0x4c2bb9`; `titus_filter_target` `0x4c2275`..`0x4c228c`; `titus_ai_frame` `0x4c2680`..`0x4c271c` |
| V251 Salamanka flinch | `giant_salamanka_set_flinch` `0x4c82a3`..`0x4c837e`; caller `giant_salamanka_boss_load` `0x4c84b8`; `Giant Salamanka.tbl` |
| V252 find fallback | `File__find` `0x5142ad`..`0x5142f6`, `0x51434c`..`0x5143c0`; `File__open` `0x5146e5`..`0x5146ed` |
| V253 reader seek | `FileReader__seek` `0x513a99`..`0x513ae0`; callers `0x54425a`, `0x5442a0`, `0x5442d5`, `0x543463` |
| V254 last extension | `File__open` `0x51465c`..`0x51467a`, `0x51470c`..`0x514728`; data `0x5a7818`..`0x5a782b`; `File__find` `0x5142ef` |
| V255 add path again | `file_add_path` `0x512f68`..`0x512f92`, `0x512fc1`..`0x51302c`; `setup_data_paths` `0x43dca0`..`0x43df04`; `cutscene_set_data_path` `0x43533f` |
| V256 dead File helpers | `File__read_matrix43` `0x525bd9`..`0x525c42`; `File__find_on_cd` `0x513f7d`..`0x513f96`; `File__read_String8` `0x525d34`..`0x525d5c`; no reference to `0x525ba0`, `0x513f70`, `0x513590`, `0x525ce0`, `0x526840` |
| V257 tooltip and repeats | `ui_button__set_tooltip` `0x5271d6`..`0x5271f2`; `ui_button__create_in_parent` `0x52713e`; callers `ui_parse_button` `0x528a10`, `0x52877d`, `ui_parse_static` `0x52ab9a`; `ui_load_table` `0x52b505`, `0x52b5d4` (no caller) |
| C260 light -1 | `light_alloc` `0x51e3a4`, `0x51e3af`; `light_create_point` `0x51e295`..`0x51e2bf`; `light_create_spot` `0x51e3f6`..`0x51e43d`; `levelrender_setup_level_lights` `0x482df5`; `ui_3d_begin` `0x412554`..`0x412565`; `ui_3d_end` `0x4125b5`; `light_delete` `0x51e5b5`..`0x51e5c8`; `LPalaceInt.s3d` 325 lights |
| C261 line tables | `ui_listbox_entry__wrap_text` `0x52cad3`..`0x52cafc`; `ui_listbox__draw` `0x52f2b6`, `0x52f308`..`0x52f330`; `ui_left_pressed` `0x2cbf628` |
| C262 cquat division | `cquat__normalized` `0x523088`..`0x523094`; `cquat_log_of_delta` `0x523c7b`; `cutscene_camera_read_track` `0x4941f7`..`0x49424b`, `0x4942fc`; `cutscene_camera_sequence_sample` `0x49438f`; `skeleton_read` `0x544643`; retail `.mvf` versions 5 / 8, `.csc` keys max length 0x3fff |
| K260 group volume | `snd_calc_volume_3d` `0x520fd8`..`0x520fe3`, `0x521008`, `0x52108b`; `snd_play_3d` `0x5214c3`, `0x521525`..`0x52152c`, `0x52156e`..`0x521575`, `0x5215af`; `snd_get_volume` `0x520df5`..`0x520dff`; `sound_emitter_update_volume` `0x5464d4` |
| K261 EAX flags | `eax_get_listener_props` `0x5223e4`, `0x522405`; `eax_save_props` `0x52221f`; `eax_load_props` `0x52243e`, `0x52255d`; `eax_set_listener_props` `0x522596`; `level_parse_table` `0x4cbf75`, `0x4cbf82`..`0x4cbfbc`; 49 `tables.vpp` `.eax` files |
| K262 matrix case 2 | `quaternion__set_from_matrix_b` `0x5227fa`..`0x522829`, `0x522841`..`0x522887`; `vfx_instance__set_node_transform` `0x537c3a`, `0x537d9e`; `spell_probe_ground_point` `0x47e4a9`; `s3d_read_navpoint` `0x46f31c`, `0x46f348` |
| K263 projected bitmaps | `light_sample_projected_bitmap` `0x51f10b`..`0x51f128`, `0x51f172`, table `0x51f38c`; `lights_shade_point` `0x51edad`, `0x51ee4a`, `0x51ef7b`..`0x51ef93`; `light_delete` `0x51e57a`..`0x51e5ad`; `light_create_spot` `0x51e436`; `level_lights_setup` `0x48e60a`; 51 retail `.s3d` light tables |
| K264 click margin | `ui_listbox__process` `0x52d1a3`..`0x52d1de`; `ui_listbox__draw` `0x52f1da`..`0x52f218`, `0x52f2a7`..`0x52f2b0`; `ui_listbox__create` `0x52d6ea`, `0x52d6f0` |
| K265 page down | `ui_listbox__select_page_down` `0x52e096`..`0x52e0ee`; `ui_listbox__ui_listbox` `0x52d5fd`; `ui_listbox_entry__ui_listbox_entry` `0x52d559` |
| V260 segment pan | `sound_emitter_calc_pan` `0x521290`, `0x5212e7`..`0x5212f1`; `snd_set_pan` `0x521769`..`0x521797`; `sound_emitter_create_segment` `0x5466d4` (no caller); `sound_emitter_update_pan` `0x546518` |
| V261 sound slots | `snd_get_handle` `0x5207ed`..`0x520803`, `0x520848`..`0x52086b`, `0x5208e5`, `0x520962`..`0x520987`; `snd_params_equal` `0x520570`; `sound_ref_parse` `0x446859`..`0x446875`; `level_parse_ambient` `0x4cc062`..`0x4cc142`; `characterinfo_parse_table` `0x445df6`..`0x445e50` |
| V262 EAX reference | `eax_buffer_init` `0x521fe2`..`0x52200e`; `ds_play` `0x55d170`, `0x55d1b7`, `0x55d1bd`..`0x55d1cd` |
| V263 dead light helpers | `lights_debug_draw` `0x51fa84`..`0x51fab2`; `lights_calc_point_specular_color` `0x51f69d`, `0x51f701`; `vfx_all_resident` `0x51dcbf`..`0x51dcce`; no reference to `0x51fa70`, `0x51f560`, `0x51dcb0` |
| V264 dirty flag | `ui_listbox_entry__set_font` `0x52d5a4`; `ui_listbox_entry__ui_listbox_entry` `0x52d55c`; `ui_listbox__relayout_dirty` `0x52cf2a`..`0x52cf66`; `ui_listbox__process` `0x52d09a`; `ui_listbox__draw` `0x52f098`; `script_editor_create_ui` `0x4e10c2` |
| V265 unreachable UI slips | `ui_listbox__set_tooltip` `0x52d7f6`..`0x52d812`; `ui_parse_listbox` `0x529b36`, `0x529b58`, `0x529d15`; `ui_state_bitmap__draw` `0x52ca00`..`0x52ca0d`; `ui_listbox__remove_entry` `0x52ddab`, `0x52ddba`; `ui_listbox__add_entry` `0x52db57`, `0x52dbbd`; `ui_listbox__sync_scrollbars` `0x52d03f`..`0x52d062`; `ui_process` `0x530587`..`0x530652`; `ui_listbox__process` `0x52d108`..`0x52d13d`; `ui_init` `0x52f7a5` |
| C270 area list | `fx_frame` `0x48a3e4`..`0x48a416`, case table `0x48b644` / `0x48b64c`; `living_entity_get_hit_by_spell` `0x45efe7`..`0x45f022`; `spell_effect_obj_set_timer` `0x47e4f3`..`0x47e583`; `spell_spawn_hit_fx` `0x47e5f6`, `0x47e66b`; `fx_alloc` `0x489cf9` |
| C271 sky list | `fx_create_world` `0x48ced2`..`0x48cf33`; `fx_sky_list_clear` `0x48a1a0`..`0x48a1bb`; `levelrender_render_scene` `0x482429`, `0x48243a`; `fx_alloc` `0x489d01`; `pxo_login_request` `0x48d938` |
| C272 effect name search | `fx_disable_by_effect_name` `0x48d674`..`0x48d691`; `fx_new` `0x48c41b`; `fx_create_missile` `0x48c2bf`; `tutorial_cond_combat_part2` `0x42eb43`..`0x42ebe2`; `character_update_status_effects` `0x45bee8` |
| C273 spell record | `fx_frame` `0x48a925`..`0x48a92d`, `0x48abcf`..`0x48ac03`, `0x48ae06`..`0x48aeb9`, `0x48af99`..`0x48afcd`, jump table `0x48b600`; `spell_effect_add` `0x47f660`..`0x47f667`; `spell_effect_alloc` `0x47f694`..`0x47f6aa`; `boss_counter_hit_dispatch` `0x4a2293`..`0x4a22ab`; `fx_obj_destroy` `0x489dda` |
| C274 container pool | `container_obj_create` `0x4897fb`..`0x48980f`, `0x489871`..`0x489879`; `level_item_spawn` `0x46a64c`..`0x46a681`; `monster_drop_loot` `0x452ec2`; `mp_handle_item_drop` `0x4892dc`; `savegame_read_item_objects` `0x4d32b4` |
| G270 area loop | `fx_frame` `0x48b3f1`..`0x48b40a`, `0x48b581`..`0x48b59b`, `0x48b5a1`..`0x48b5db` |
| G271 stale spell | `fx_new` `0x48c3e0`..`0x48c467`; `fx_obj__ctor` `0x489d30`..`0x489da1`; `fx_get_status_holder` `0x48ccdc`..`0x48cd09`; `character_equip_item_fx` `0x48ca74`..`0x48ca96`; `fx_count_spells_1a_1d_4e` `0x48bd50`..`0x48bd65`; `ai_choose_offensive_spell` `0x4bfa0b`..`0x4bfa2b`; `fx_destroy_spell_effects` `0x48d733`..`0x48d753` |
| K270 Ghost cutscenes | `levelrender_add_character` `0x483329`..`0x483369`; `cutscene_script_table` `0x4ed760` |
| K271 glow occluder | `lamp_glow_update_effects` `0x48461d`, `0x484785`, `0x4847b4`, `0x4847f9`..`0x484815`; `lamp_glow_update_lobjs` `0x48451f` |
| K272 shadows.tbl | `shadows_parse_table` `0x4875b4`..`0x4875d8`; `shadow_draw_character` `0x487677`..`0x48769f`; `shadows_draw_characters` `0x48772f`..`0x48773a` |
| K273 level fog | `level_parse_table` `0x4cbefc`..`0x4cbf11`; `fog_set_level_default` `0x486b99`..`0x486ba6`; `fog_restore_level_default` `0x486bc0`..`0x486bd0`; `fog_set_distances` `0x486b55`; `cutscene_events_frame` `0x4370f7` |
| K274 clip y | `gr_project_vertex` `0x50eb0e`, `0x50eb1f`; `character_draw_overhead_bitmap` `0x481d83`..`0x481dec`; `levelrender_draw_health_bars` `0x4849c9`..`0x4849e1`; `levelrender_draw_speech_text` `0x484c05`..`0x484c1c`; `levelrender_draw_float_text` `0x48541b`, `0x4854c8`..`0x4854d0`; `interface_set_view_clip` `0x41222a`..`0x412251`, `0x41235f` |
| V270 render toggles | `cmd_render_level` `0x4818af`..`0x48190a`; `cmd_render_characters` `0x48199f`..`0x4819fa`; `cmd_render_effects` `0x481a8f`..`0x481aea` |
| V271 dead flags | `levelrender_fog_enable` `0x482897`; `fog_get_near_current` `0x486bf0`; `fog_get_far_current` `0x486c20`; `0x22fb714` readers `0x482d9b`, `0x4839f4`, `0x4841fa`, `0x48438e`, `0x485f89`; `levelrender_render_scene` `0x48236a`, `0x482381` |
| V272 blob bitmaps | `shadow_create_blob_bitmap` `0x489f15`..`0x48a045`; `shadows_init` `0x48a06f`..`0x48a085` |
| C290 JPEG row table | `jpeg_write_file` `0x5565e0`..`0x5565e5`, `0x55661f`..`0x55663a`; `gr_screenshot` `0x506214`, `0x5064bd`, `0x506518`, `0x5065a7`; `screenshot_save` `0x43251e` |
| C291 alt textures | `alt_textures_parse_table` `0x4aacf3`..`0x4aad0f`, `0x4aae91`, `0x4aaea4`..`0x4aaee8` |
| C292 valid chars | `ui_inputbox__set_valid_chars` `0x552870`, `0x5528d0`, `0x552952`..`0x55295d`, `0x5529e0`..`0x5529eb`; `ui_parse_inputbox` `0x529576` |
| G290 BSP stack | `lobj_raycast_bsp` `0x4af0a5`..`0x4af0ab`; `lobj_raycast` `0x4ae9e2` |
| K290 skinned normals | `gr_opengl_render_character_mesh` `0x50d00e`..`0x50d0de`, `0x50d33b`, `0x50d366`, `0x50d1c0`..`0x50d1f1`; `gr_d3d_render_character_mesh` `0x50c514`..`0x50c58d`, `0x50c846`; `gr_glide_render_character_mesh` `0x50b7e4`..`0x50b859`, `0x50bb00` |
| K291 seam fog | `gr_d3d_render_character_mesh` `0x50c37f`..`0x50c3b3`, `0x50c778`..`0x50c77e`; `gr_get_fog_alpha` `0x50d839`..`0x50d848` |
| K292 more / end hover | `dialogue_topic_entry__draw` `0x4d6fc3`..`0x4d6ff0`, `0x4d7057`..`0x4d7084`; `dialogue_frame` `0x4094d3`..`0x4094f0`, `0x409560`..`0x40957d` |
| K293 talker | `dialogue_box__add_text` `0x4d7262`, `0x4d72fa`..`0x4d7316`, `0x4d73cc`..`0x4d73ea`; `dialogue_line__init` `0x4d6b06`..`0x4d6b0d`; `conversation_update_talk_anims` `0x4d8b49`, `0x4d8bb5`; `dialogue_init` `0x408fbb` |
| K294 fade flag | `ds_stop_fade` `0x55d673`..`0x55d6ab`; `ds_channel_update_fade` `0x55d6f8`..`0x55d702`, `0x55d742`..`0x55d74c`; `ds_channel_release` `0x55c5c5`..`0x55c5d5`; `ds_find_free_channel` `0x55d0b4`..`0x55d0f7`; `ds_play` `0x55d170`, `0x55d21e`; `character_play_move` `0x46112b`..`0x461137` |
| K295 force steps | `vfx_emitter_instance__move_particles` `0x560b85`..`0x560c2d`, `0x560d9a`, `0x560f01`, `0x561131` |
| K296 light tags, dummies | `vfx_instance__list1c_item` `0x5385e2`; `vfx_instance__list18_item` `0x5385c5`; `vfx_instance__list20_item` `0x538605`; `vfx_instance__bind_node` `0x537600`..`0x537634`; `vfx_dummy_state__update` `0x55fe8c`, `0x55feb5`..`0x55febc`; emitter `0x56164b`..`0x56165b` |
| K297 editor gadgets | `ui_checkbox__create` `0x553d0e`..`0x553d1c`, `0x553dba`; `ui_radio_button__create` `0x5548af`..`0x5548bd`; `ui_inputbox__draw` `0x553217`..`0x55322a` |
| K298 billboard tip | `vfx_particle_get_billboard_axes` `0x562947`..`0x5629ad`, `0x5629b2`..`0x562a04` |
| K299 face normals | `vfx_unpack_vector_s8` `0x564a28`..`0x564a7c`; `vfx_unpack_normal` `0x5648c7`..`0x5648fc`; `vfx_mesh_render` `0x563190`..`0x5631a0`, `0x5631c7`..`0x56325a` |
| V290 dead code slips | `ix_box_oriented_box_oriented` `0x53c741`..`0x53c75a`; `vfx_emitter_instance__render_d3d` `0x561ba6`, `0x561bd6` |
| C300 section copy | `gr_d3d_copy_bitmap_to_surface` `0x54c327`..`0x54c35e`, `0x54c388`..`0x54c417`, `0x54c3e3`, `0x54c432`; `gr_d3d_create_texture_section` `0x54bcd8`..`0x54bd8f`, `0x54bf51`..`0x54bf8e`, `0x54bfe9`..`0x54c006`; `gr_d3d_create_texture` `0x54d16f`, `0x54d187`, `0x54d280`..`0x54d2c9`; `gr_d3d_init_device` `0x539ba6`..`0x539c54` |
| C301 navpoint tables | `cutscene_script_load` `0x4ed281`..`0x4ed2b7`; `script_parse_navpoints` `0x4db776`..`0x4db78a`; `level_script_parse_table` `0x4cd718`..`0x4cd735`; `script_editor_add_navpoint` `0x4e5d59`, `0x4e5d7d`; `cutscene_script_stringpool` `0x2571750`; `ccmd_door_toggle` `0x24ed5f0`; retail `Game-Intro_cscript.tbl`, `lenele1b_v2_script.tbl` |
| C302 rider NULL | `ls_rand_forestnite1_frame` `0x4f581f`..`0x4f5887`; `ls_rand_hills01_frame` `0x4fa5e5`..`0x4fa64b`; `ghost_find_rider` `0x4b9fe0`..`0x4ba012`; `phoenix_find_rider` `0x497be0`..`0x497c12` |
| G300 armour target | `combat_trigger_reflex_ability` `0x46794a`..`0x467990`, `0x4679a9`..`0x4679b4`, `0x467a4b`..`0x467a68`, `0x467afc`..`0x467b1d`, `0x467c64`..`0x467c86`, reflex branch `0x467815`..`0x467829`; `spell_cast_execute` `0x47c9a6`..`0x47c9d4`; `items_parse_table` `0x44a2b9`, `0x44acd2`..`0x44ad41`; `spells_parse_table` `0x47a56c`, `0x47a61d`; retail `items.tbl` (Robe of Restoration), `spells.tbl` (heal) |
| G301 Canis payoff | `ls_lenele2ab_action` `0x4f1239`..`0x4f1257`; retail `Lenele2ab_dlg.tbl` |
| K300 hit sound position | `spell_cast_execute` `0x47c7c6`, `0x47c82a`, `0x47c84b`, `0x47c8c6`, `0x47c8d4`, `0x47c9d9`, `0x47c9ee`..`0x47c9f4`, `0x47ca5b`, `0x47cde5`, `0x47ced2`, `0x47d08c`; jump tables `0x47d348`, `0x47d3a8`; `spell_play_hit_sound` `0x47e072`..`0x47e078` |
| K301 important speakers | `dialogue_parse_table` `0x5000dc`..`0x500152`, `0x500835`..`0x500854`; `conversation_apply_topic_effects` `0x4d807a`..`0x4d80b6`; `dialogue_parse_cflags` `0x4fedf1`; `levelrender_draw_speaker_icons` `0x48588c`; retail `LPalaceInt_v2_dlg.tbl`, `Miniquests_dlg.tbl` |
| K302 reciprocal curve | `vif_curve__evaluate` `0x533d20`..`0x533d60`, `0x533d61`..`0x533d7a`; `vif_emitter_update` `0x533187`..`0x533191` |
| K303 sysinfo | `cpu_identify` `0x5312e2`, `0x5313e9`..`0x5313fd`, `0x531453`, `0x53157b`, `0x5315bb`; `PrintSystemInfo` `0x53199b`, `0x531a29`..`0x531a68`; `cmd_sysinfo` `0x531af4` |
| V300 tint channels | `gr_poly_rgb` `0x545617`, `0x54567b`, `0x5456bb`, `0x545706`, `0x54574c`; `world_hover_tint_character` `0x43b1d0`; `world_hover_tint_object` `0x43a650`; `murod_ai_leap` `0x4c54ec`..`0x4c54f8`; `luminar_teleport_in` `0x4ca166`..`0x4ca172` |
| V301 alpha stride | `gr_d3d_copy_bitmap_to_surface` `0x54c1ab`..`0x54c28f` |
| V302 key queue | `key_init` `0x5097b5`, `0x5097fa`; `key_flush` `0x509ab0`..`0x509aba`; `key_putkey` `0x509d54`..`0x509d76` |
| C310 install path | `packfile_add` `0x534425`, `0x53445c`..`0x53446c`, `0x53447b`; `packfile_init` `0x533e30`..`0x533e7d`; `packfile_fopen` `0x534690`, `0x534741`..`0x534798`; `file_init` `0x512dc6`; `File__get_packfile_path` `0x514045`..`0x51405b`; `snd_load` `0x520b17`, `0x520b47` |
| C311 new LEP NULL | `mp_handle_new_lep` `0x47ff88`..`0x47ffc1`; `living_entity_create` `0x461b80` |
| C312 respawn NULL | `mp_respawn_player` `0x481097`..`0x4810cb`; callers `0x481057`, `0x481136` |
| C313 labels save loop | `automap_labels_save` `0x430780`..`0x4307ff`, `0x43082a`; retail `automap_labels.tbl` |
| G310 summon update | `mp_send_local_updates` `0x47f9fd`..`0x47fa00`, `0x47fa14`..`0x47fa6f`; `ai_frame` `0x472db1` |
| K310 GL point-light colour | `levelrender_draw_list_opengl` `0x491c0a`..`0x491c2e`, `0x491e06`..`0x491e14`, `0x491f90`..`0x491fe2`, `0x492669`..`0x4926c4`; `levelrender_sum_point_lights` `0x48fcc7`..`0x48fd0e`; `levelrender_draw_list_glide` `0x48ee57`..`0x48ee92`; `levelrender_draw_list_d3d` `0x4907cd`..`0x490805`; `vfx_node_create_light` `0x51fbe0`..`0x51fc5b`; `attached_light_create` `0x5201b2`..`0x5201e0`; `vfx_node_relocate` `0x536c74`..`0x536ca0`; retail `effects.vpp` `.rfx` files |
