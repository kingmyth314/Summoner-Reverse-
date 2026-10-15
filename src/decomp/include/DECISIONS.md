# Header merge decisions

How each conflict between the old per-batch stub headers (`stubs_*.h`, `ui_screen.h`, `dialogue_box.h`,
the unit headers and `symbols/types.h`) was resolved. Evidence is from Sum.exe v1.40 (`node tools/asm.mjs`,
`symbols/sum.symbols`); "definition" means the pilot `.cpp` that defines the function, whose signature
wins when the callers allow it. Addresses are Sum.exe virtual addresses.

## Classes and layouts

**D1. Character** (types.h C struct; partial classes in stubs_assess_pickup.h, stubs_player_effectmem.h,
stubs_sound_items_registry.h; `Character_M` / `living_combat_M` / `ai_state_M` shims in stubs_status.h).
One class `Character : obj` in `engine/objects/living_entity.h` with every types.h field at the same offset
(`LAYOUT_CHECK` 0x788) and the union of the methods the pilots call (all thiscall per sum.symbols:
character_get_name 0x44ed90 ... character_hero_model_prefix 0x462ec0). Typed sub-objects: `ai` is an
`ai_state` (+0x324, 0x234), +0x558 is a `living_combat` (0x54; character_get_protection 0x464de0 is thiscall
on it), `equipment[10]` are `item_info*` (item +4 = ItemType*, as the assess and status code read them),
`instance` is `character_instance*`, the GameTimer fields are `GameTimer`. Two names for +0x558 exist in the
code (`combat` via the status shim macro, `protection` in new_assess.cpp); since the .cpp code may not change
yet, both are members of an anonymous union (same object). The shim macros remain as identities:
`CHAR_M(c)` = `c`, `COMBAT_M(c)` = `&c->combat`, `AI_M(c)` = `&c->ai`. To do: drop the union alias and the
macros when the code is touched; declare `living_combat`'s constructor (0x4681b0) then (a union member
cannot have one).

**D2. File** (five copies: stubs_cmdline_grfont.h, stubs_container_targa.h, stubs_sound_items_registry.h,
stubs_charselect_levelselect.h, stubs_options_saveload.h, stubs_player_effectmem.h, plus a local class in
timestamp.cpp). One 0x114-byte class in `vsdk/vfile/file.h` with the union of the members, all at their
sum.symbols addresses (find 0x5141c0 ... write 0x526410, plus write_int 0x526250 for GameTimer::write).
Fields from the symbol comments: path id +0 (File__get_path_id), path buffer +5 (File__get_full_path), pack
entry +0x108, slot +0x110 (File__is_open). timestamp.cpp's local declaration is replaced by the include.

**D3. File::read_int8 / read_int16 default argument** (`int` in stubs_cmdline_grfont.h, `char` / `short` in
stubs_container_targa.h). The readers return the default with `mov al, [esp+0xc]` (0x525861) and
`mov ax, [esp+0xc]` (0x525901): the default is the result type, `char` / `short` (read_uint8: `ubyte`).

**D4. TextFile** (four copies). One class in `vsdk/parse/parse.h` with every method. `SkipTo` returned void in
two copies and bool in one: 0x515ba0 returns `xor al, al` / `mov al, 1` (0x515bb5, 0x515c17) and
ui_parse_slider tests AL, so bool.

**D5. ui_element / ui_gadget / ui_tooltip** (stubs_container_targa.h and stubs_window_charinst.h; types.h's C
struct). One declaration in `vsdk/ui/ui.h` with the superset of members (container's `init` 0x52c040 and
`draw_bevel` 0x55e2e0, window's `init` 0x542da0, `get_kind`, ..., `find_nearest_*`). `callback` (+0x244) is
`bool (*)(int, int)`: ui_window::send_event (0x52aff0) calls it with (event, value) and returns its result
(the other copy had `void*`). ui_container: the full class of ui_container.h replaces the partial copy in
stubs_window_charinst.h (same offsets: font +0x380). The widgets went to `vsdk/ui/ui_gadgets.h`.

**D9. matrix / matrix3 / matrix3_t** (types.h `matrix3` C struct, `class matrix` in two stubs, wrapper
`matrix3_t` in stubs_buysell_itemtrade.h, shim `MATRIX3_M` in stubs_status.h). One `class matrix`
(`vsdk/math/matrix.h`, 0x24) with the matrix3__* members (multiply 0x50fbc0, copy 0x4a4320, make_identity
0x511050, extract_angles 0x510240); `matrix3` and `matrix3_t` are typedefs of it, `MATRIX3_M(m)` is `m`.
vector3 / quaternion / matrix43: the operator set of stubs_window_charinst.h (superset), all out of line.

**D10. mp_player** (stubs_menus_dialogue.h: flags +0x10, object +0x28; stubs_player_effectmem.h: next +0,
object `void*`; macros MP_PLAYER_NAME/NEXT/TYPE for +0x34 / +0 / +0x14). One struct in
`summoner/multi/multi.h` with next +0, flags +0x10, type +0x14, object +0x28, name +0x34. `object` is a
`Player*`: interface_set_screen reads `[[mp_local_player] + 0x28] + 0xc` (0x412743-0x41274c), and +0xc is
Player.handle; party_remove_all (player.cpp) clears it. The three macros now expand to the fields.

**D11. mp_char_profile / mp_char_record** (stubs_gamecreate_journal.h: flags +0x4c only;
stubs_charselect_levelselect.h: the full 0x154-byte record). One `mp_char_record`
(`LAYOUT_CHECK` 0x154), `mp_char_profile` is a typedef: new_charselect.cpp's
`mp_charselect_get_selected_profile` returns an element of `mp_char_profiles` (0x154 stride) and new_gamecreate
reads its +0x4c.

**D12. Mempool constructor** (inline body in stubs_sound_items_registry.h, out-of-line declaration in
stubs_player_effectmem.h; `struct` with no name field in stubs_credits_help.h). Inline in
`vsdk/os/memory.h`: item_paging.cpp's static initialiser is 19 bare `Mempool::Copy` calls (the constructor
expanded), and effect_mem.cpp, which the NOTES date as built /Ob0, calls a copy at 0x51dff0 that sits at the
end of its unit, where VC6 places the COMDAT of a non-expanded inline. Same for `getPoolSpaceLeft`
(0x51e010). `destination` is `char*` (effect_mem.cpp stores char pointers; types.h says uint8_t*).

**D13. obj is 0x98 bytes** (stubs_player_effectmem.h checked `sizeof(obj) == 0xc`). The derived objects all
start their own fields at +0x98 (container items, item_obj item, effect_object next, Character team), and
obj_active_list (0x970770), the list head object, reaches 0x970808 (next symbol objects_initialized
0x970809). obj_link_new (0x469b60) writes +0xc, +0x34, +0x80, +0x84 of the base. So `obj` holds types.h
Character's +0x00..+0x97 (vtable, links, type, pos, orient, handle, ...), and `container` / `item_obj` /
`Character` derive from it (container 0xe8 checked).

**D14. savegame and conversation_storage** (savegame: get_quest_bit in stubs_gamecreate_journal.h, get_flag
in stubs_player_effectmem.h, read_info / read_thumbnail_file / write / set_pending_load in
stubs_options_saveload.h; conversation_storage: layout in dialogue_box.h, save_state in
stubs_options_saveload.h). One class each (`levelscripts/savegame.h`, `levelscripts/conversation.h`) with the
union of the members.

**D15. file_find_files list** (head `file_find_entry` = a whole node in stubs_charselect_levelselect.h,
head `file_find_list {next, prev}` in stubs_options_saveload.h; node name 0x30 vs 0x2c + time_t). The nodes
are 0x38 bytes (operator new size) with the file time at +0x34 (new_saveload.cpp reads it), so the name is
0x2c. `file_find_node` derives from `file_find_list`, the function takes `file_find_list*`, and
`file_find_entry` is a typedef of the node: both call sites (a node head and a bare head) then compile
unchanged.

**D16. item_info / inv_item and party_inventory** (stubs_assess_pickup.h: `item_info {?, type}` and
`int party_inventory[521][2]`; stubs_buysell_itemtrade.h: `inv_item {?, type, charges}` and
`party_inventory_entry party_inventory[521]`; stubs_status.h: item_info again). One `item_info {_unk00, type,
charges}` (`engine/objects/items.h`; `inv_item` is a typedef) and `party_inventory_entry {count, item_info
*item}` (`engine/objects/objects.h`): itemtrade_get_party_row returns 0x970820 + 8 * row and buysell reads
`entry.item->type`. `party_inventory_sort` takes `party_inventory_entry*` (new_pickup passes the array).

**D17. Button records** (ui_screen.h `ui_button_def` with `int pos[3][2]` / `ui_button_state`;
stubs_assess_pickup.h `screen_button_def` with `{x, y} pos[3]` / `screen_button`). Same 0x40 / 0x18 layouts.
`screen_button` is a typedef of `ui_button_state` (same field names); the two def structs stay separate
because the code indexes the position differently (`pos[i][0]` vs `pos[i].x`). Both are in
`summoner/interface/interface.h`; to merge when the assess / pickup code may change.

**D19. game_camera** (types.h `typedef struct game_camera` made the object name unusable; player.cpp's
stub had `class camera` with set_target). `class camera` with the types.h layout (0x2a8) and
`camera_set_target` 0x478ff0; `extern camera game_camera` (0x60a018). The aliases `camera_view_pos`,
`camera_view_orient` (sum.symbols) and `camera_fov` (0x60a1a4, ours) stay because new_saveload.cpp reads them.

**D20. ItemType / char_info** (types.h; partial copies in stubs_assess_pickup.h and
stubs_sound_items_registry.h). Full types.h layouts (0x100 / 0x3d0). Names as in types.h after the
2026-10-15 update: ItemType +0x10 `mesh` (typed `vim*`: item_paging stores vim_load's result, buysell copies
it to a `void*`), `damage` +0x68 and char_info `damage` +0x38 `ushort` (every read is zero-extended).
ItemType +0x7c / +0x80 are `drop_tag1` / `drop_tag2` (item_paging.cpp's names; types.h `_unk7c/_unk80`),
+0x58 is `item_charge_effect*`. char_info +0xf8 is the `resistance_set` (7 ints, copied with rep movsd 7 by
assess_format_info; types.h `_unkf8[0x1c]`).

**D21. vim / ca_vim** (stubs_sound_items_registry.h `vim`, stubs_window_charinst.h `ca_vim`: same .vim header,
textures +0x10, count +0x28). One `vim` in `vsdk/ca/skeleton.h` (also center +0x50, radius +0x5c);
`ca_vim` is a typedef.

**D22. skill_info** (stubs_player_effectmem.h recovery +0x08; stubs_status.h name +0, description +0x2c).
One 0x34-byte struct with the three fields.

**D23. gr_light_desc** (stubs_buysell_itemtrade.h: `float f14`; stubs_status.h: named fields with `_unk14`).
stubs_status.h's layout with +0x14 named `f14` (the only name the code uses for it).

**D24. Player** (types.h struct, player.h class, a 0x10-byte struct in stubs_sound_items_registry.h).
player.h's class (0x188, GameTimer cooldowns) only, moved to `summoner/player/player.h`.

**D25. Color** (`class` in stubs_menus_dialogue.h, `struct` in four others). One `struct Color {r, g, b, a}`
in `vsdk/gr/gr.h`: no constructor is called by the pilots (the colour objects are built in other units).

**D26. RealtimeTimer / GameTimer** (four copies). `vsdk/os/timestamp.h` (the pilot unit header) only.

**D27. String / StringPool / console_command.** String: the full class of stubs_cmdline_grfont.h (the other
copy had only `operator=(const char*)`). StringPool: both constructors exist (0x544dc0 default, used with
`Init` by effect_mem.cpp; 0x544de0 with arguments, used by gamesound.cpp's static object), so both are
declared. console_command: 0x10 bytes (the objects are 0x10 apart in .bss) with types.h's three fields first
(superseded by D40: 0xc bytes).

**D28. character_instance** (stubs_status.h stub with `process(float, int, int)` and `render`; the pilot
class with `process(float, bool, int)`). The pilot class (its definition wins; new_status passes 0, 0) plus
`render(vector3*, matrix*, gr_light_desc*)` 0x520420 (thiscall, `ret 0xc`).

**D29. ai_state / ai_group** (types.h; stubs_player_effectmem.h partial with `group` at +0x1a8). types.h
layout with +0x1a8 named `group` (`ai_group*`; player.cpp follows it to the group's leader +0x48), the
timer fields as GameTimer, and the ai_* thiscall members of both stubs.

## Functions

**D6. interface_set_screen(int, ?)** (`int` in stubs_credits_help.h, `void*` in stubs_menus_dialogue.h).
`void*`: 0x412620 keeps the argument in EBP, null-tests it and reads a byte or dword through it
(0x41281a `test ebp, ebp` / 0x412822 `mov al, [ebp]`, 0x412863 `mov ecx, [ebp]`). Every call site passes 0 or
an address.

**D7. gr__Mode arguments** (`unsigned int`, `gr_mode` typedef and `int` in different stubs). One
`typedef unsigned int gr_mode` in gr.h for everything outside gr_font.cpp; gr_font.cpp's own definitions
(`gr_string`, `gr_string_at_text_pos`) keep `int mode` because the definition wins. Both are the 4-byte
by-value gr::Mode class in the original; replace when it is declared.

**D8. gr::set_clip / gr__set_clip** (namespace in stubs_assess_pickup.h / stubs_container_targa.h, flat name
in the others). The symbol is the namespace function (`gr__set_clip` = `gr::set_clip`), declared in
`namespace gr`; `#define gr__set_clip gr::set_clip` (and `gr__reset_clip`) keeps the other spelling.

**D18. new_icam_load / new_icam_get table argument** (`new_icam_table*` in stubs_status.h, `void*` in
stubs_buysell_itemtrade.h). The table is a stack argument (0x410760 builds its TextFile in ECX, so it is not
thiscall), new_status.cpp passes a `new_icam_table` and new_buysell.cpp a byte array: `void*`.

**D30. game_play_sound** (`(int, float, int, int, int)` in two stubs, `(int, float, float pan, int, int)` in
gamesound.h). gamesound.cpp's definition; 0x468d90 forwards the five arguments unchanged to snd_play, whose
third is the float pan.

**D31. sound_play_2d / music_play** (`void` / `int loop` in the stubs). gamesound.cpp's definitions: `int`
result, `bool loop`.

**D32. hud_show_message** (`char*` vs `const char*`). `const char*` (callers pass literals and localized
strings; the screens never pass a buffer they need changed).

**D33. gr_split_str string argument** (`char*` in gr_font.cpp's definition, `const char*` in the stubs).
`const char*`, and the definition in gr_font.cpp was changed to match: the body (0x509130) only reads `s`,
and new_journal / new_status pass `const char*` quest and status texts. (The one code change outside
includes, besides moved declarations.)

**D34. bm_load(name, a, b)** vs `(name, path_id, unk)`. `(name, path_id, unk)`: the screens pass
path_interface as the second argument.

**D35. item_page_in** (`(slot, type)` in an early item_paging.h). `(type, slot, unused)` as item_paging.h now
has it: 0x46c940 compares its second argument with 17 and every call pushes (type, slot, 0).

**D36. Duplicate plain declarations** (gr_get_clip, key_to_ascii, mouse_*, ui_* helpers, party / player
functions, localize, ...): identical in every stub, declared once in the header of their unit.

**D37. shop_set_shopkeeper_from_char(void*)**: kept as in its definition (new_buysell.cpp), although the
function reads a Character's name (+0x2f4). **mp_charselect_get_selected_profile(int)**: kept as in its
definition, although the binary tests only the low byte.

**D38. Declarations local to a .cpp** that belong to another unit moved to that unit's header:
timestamp.cpp's `class File` (D2), frametime.cpp's `timer_get` / `timer_wait` (`vsdk/os/timer.h`),
gr_font.cpp's `extern gr__Screen gr_screen` (`vsdk/gr/gr.h`), window.cpp's `KEY_*` and `UI_EVENT_*` macros
(`vsdk/os/key.h`, `vsdk/ui/ui.h`).

**D39. The C runtime**: stubs_credits_help.h / stubs_assess_pickup.h declared strcpy, sprintf, ... by hand
(`unsigned int` sizes). `vsdk/vsdk.h` includes the CRT headers instead; `<stdint.h>` is included for the
types.h integer names (VC6 has none: the real build needs a small shim header).

## Pilot units new_automap / new_inventory / new_hud (stubs_automap.h, stubs_inventory.h, stubs_hud.h)

The three units were written against the old stub headers and converted afterwards. Their `.cpp` code is
unchanged except where a decision below says so.

**D40. console_command is 0xc bytes** (D27 said 0x10; stubs_automap.h wrapped types.h's 0xc C struct as
`console_command_t`, stubs_inventory.h used a 0x10-byte shim `console_command_M`). The constructor 0x50a180
writes only +0, +4, +8 (0x50a1d7-0x50a1dc), and command objects are followed directly by other variables at
+0xc: ccmd_map_grid 0x5b2a00 / automap_centered 0x5b2a0c, ccmd_show_coordinates 0x5b2b60 /
automap_mask_scale 0x5b2b6c, ccmd_show_ui 0x5bd780 / ui_choice_popup_mouse_mode 0x5bd78c. The 0x10 spacing
of player.cpp's objects is not the size. `console_command_t` and `console_command_M` are typedefs of the
class (`vsdk/os/console.h`).

**D41. Color has constructors** (D25 kept a plain struct; stubs_automap.h / stubs_hud.h added
`Color_ctor : Color`). The static colours are built by Color::Color(r, g, b, a) (0x5067f0, thiscall,
`ret 0x10`; init_colors_5b29e0, init_color_5bd2ec, ...), and hud_portrait_get_color (0x40f880) returns its
Color through a hidden pointer (the caller pushes `lea eax, [esp+0xc]` at 0x40f775, the callee stores
`[eax]` and returns it): VC6 returns a 4-byte POD struct in EAX, so Color is not a POD, i.e. it has a
user-declared constructor. `Color()` is declared inline and empty (uninitialised locals and the colour
arrays emit no call); `Color_ctor` is a typedef.

**D42. vector3 fields, not subscripts** (types.h had `float pos[3]`; the pilots wrote `c->pos[0]`,
`*(vector3 *)c->pos`). Code change: `->pos[0/1/2]` became `->pos.x/.y/.z` (new_automap.cpp 4 calls,
new_hud.cpp hud_interact_nearest) and the `*(vector3 *)` casts were dropped (new_inventory.cpp 0x416dc0 /
0x416eb0, new_hud.cpp hud_level_exit_step_back). Same loads and stores. Open: those copies are inline
3-dword moves in these /Ob1 units (0x416e2d, 0x40d855-0x40d89e, the second through `lea eax, [edx+0x10]`
as an inlined `this`), while vector3.h declares operator= out of line (vector3__assign 0x413190, 159
callers); the assignment may be an inline member that the /Ob0 units call as a COMDAT.

**D43. Where the three units' declarations went.** Unit headers `summoner/interface/new_automap.h`,
`new_inventory.h`, `new_hud.h` (old guards kept; new_inventory.h no longer includes types.h). New headers:
`summoner/automap.h` (automap.cpp, 0x42ffe0-0x4302b0: `automap_label` 0x30, the label array, map corners,
reveal grid), `engine/spells/spells.h` (spells.cpp: `spell_info` 0x80 from types.h, `spells`,
`spell_display_names`), and declaration-only unit headers `summoner/interface/new_skill.h`, `new_spell.h`,
`new_tutorial.h` (new_skill.cpp / new_spell.cpp / new_tutorial.cpp per sum.symbols). `hud_key_help`
(0x40e260, new_hud.cpp) moved from interface.h to new_hud.h; new_mainmenu.cpp includes new_hud.h for it.
The rest went to the header of its unit (as D36): key codes and `key_is_down` (key.h), gr_bitmap_float,
gr_cull_bounding_box, the automap gr modes (gr.h), bm_get_num_frames, localize_item_name, the items.cpp and
party-inventory functions (items.h / objects.h, with `equip_page_slots` 0x5819d4 next to objects.cpp's
.rdata), level layers, gameseq_get_state, gameplay party control and gametarget world picking
(summoner.h), lc_entries and the level-exit state (level_scripts.h), the pick overrides, popups, panel
helpers and interface colours (interface.h), ai_service_request (ai.h), mp_send_(un)equip_item,
mp_respawn_and_notify, mp_netgame_state (multi.h), combat_mode (player.h).

**D44. Key modifiers: 0x1000 Shift, 0x2000 Alt, 0x4000 Ctrl.** key_mark ORs 0x10 / 0x20 / 0x40 into CH
(0x5094c3 / 0x5094da / 0x5094f1) from the key_down_state bytes of scan codes 0x2a / 0x36, 0x38 / 0xb8 and
0x1d / 0x9d (base 0x25da94c), as docs/sum-exe/controls.md now says. stubs_hud.h defined `KEY_ALTED` as 0x4000
from the old controls page. key.h has `KEY_ALTED 0x2000` and `KEY_CTRLED 0x4000`; code change in hud_handle_keys:
`KEY_S | KEY_ALTED` / `KEY_L | KEY_ALTED` became `KEY_CTRLED` (same value; quicksave / quickload are Ctrl+S /
Ctrl+L).

**D45. The thiscall shims of the two units are members** (as D1): `Character_HM` (get_display_name,
get_current_move 0x461500, set_6ac 0x4570d0, set_locomotion_speed 0x45a6e0, update_locomotion_anim 0x45a120)
on Character; `ai_state_HM` (is_mode 0x473200, get_mode 0x473600, is_combat_e_or_sneak_attack 0x473120,
goals_clear 0x476380, go_idle) and `ai_goal_M` (add_cast_spell 0x4740c0, ai_goal_add_cast_spell, thiscall on
Character +0x324) on ai_state; `character_instance_HM` / `_M` on character_instance. The macros
`CHAR_HM`, `AI_HM`, `AI_GOAL_M`, `CHARINST_HM`, `CHARINST_M` and `INV_EQUIP(c, i)` (`(c)->equipment[i]`) stay
as identities. Two code changes in hud_interact_nearest, where the old stub's Character stood for any object:
`CHAR_HM(obj)->is_in_view()` became `((item_obj *)obj)->is_in_view()` (0x46a7e0 is item_obj_is_in_view,
declared on item_obj), and the walk uses `obj::next` (`(Character *)obj_active_list.next`, `obj->next`)
instead of types.h's `_link04`.

**D46. savegame members** (stubs_automap.h `savegame_automap`, stubs_hud.h `savegame_HM`). On `class
savegame`: automap_label_add / remove / labels_reset_level / label_count (0x4d12b0-0x4d1620) and the hot
buttons with the callee's argument order (sum.symbols CORRECTION notes, all call sites): hot_button_get
(int *kind, int *id, member, slot) 0x4d1770 returns `setne al` (bool), hot_button_set (kind, id, member,
slot) 0x4d16a0, hot_button_clear (member, slot) 0x4d1660. `SAVEGAME_AUTOMAP` is `(savegame_state)`,
`SAVEGAME_HM` is `(&savegame_state)`. write / read_info / set_pending_load: see D52.

**D47. world_hover_portrait_character is a Character*** (sum.symbols type; stubs_hud.h `int
world_pick_target_mode`). hud_frame stores `hud_portrait_slots[n].character` into it; code change: the
five `(int)` casts were dropped.

**D48. skill_is_available / character_use_skill** (stubs_inventory.h `(skill, Character *)` and
`(skill, void *target, Character *user)`; stubs_hud.h `(skill, int)` and `(skill, Character *, int)`).
skill_is_available 0x4291d0 uses arg 2 as the character (`mov ebp, [esp+0x10]`, 0 -> cur_character, then
`mov ecx, ebp` for character_get_skill): `(int, Character *)`. character_use_skill 0x429410 takes the user
from arg 3 (0x429412 `mov esi, [esp+0x14]`, 0 -> cur_character) and the skill from arg 1: `(int skill, void
*target, Character *user)`; the HUD's Banish call pushes (0x34, cur_summon, 0) (0x40ea5e-0x40ea93), so the
summon is the target. The HUD's calls compile unchanged against the inventory's signatures.

**D49. The camera object** (stubs_hud.h `game_camera_obj` + `camera_HM`, because types.h's typedef took the
name). The code now uses `game_camera` (D19; renamed in new_hud.cpp), `camera::cycle_mode` (0x479a60) is a
member and `CAMERA_HM` is `(&game_camera)`. Aliases kept like camera_fov (D19): `game_camera_heading`
(0x60a198 = game_camera.yaw, new_automap.cpp) and `config_solo_confirm` (0x60acd5 =
Summoner_options.solo_confirm, sum.symbols name, new_hud.cpp).

**D50. mp_player +0x14 / +0x1c** (stubs_hud.h macros `MP_PLAYER_STATE`, `MP_PLAYER_ID`; D10's record has
`type` at +0x14). One field: +0x14 is a state (3 listed on the level select screen, 4..9 the loading steps
mp_draw_loading_status shows); it keeps the name `type` and `MP_PLAYER_STATE(p)` expands to it. +0x1c is
`id` (hud_confirm_quit passes it to mp_respawn_and_notify).

**D51. lobj** (levelrender.h: faces +0xc only; stubs_hud.h: name +0, mesh +4, flags +8, pos +0x14, centre
+0x44, bbox +0x50 / +0x5c). One 0x70-byte struct with all of them (no overlap). num_lobjs (0xa5a2c8),
lobj_get_bbox (0x4776c0) and door_in_use_range (0x476a30, unit not known) are declared next to it.

**D52. savegame::read_info's fifth argument is float*** (new_saveload.cpp: `float *level_time`; stubs_hud.h:
`int *b`). The callee only copies dwords (savegame_read_header 0x4d2ac6-0x4d2aea), new_saveload.cpp reads the
value as seconds (`level_time / 3600.0f`), quickload ignores it. Code change in quickload: the local `int b`
became `float b` (same stack slot, passed by address). write is (filename, int, description) as before.

**D53. file_find_files list** (stubs_hud.h `file_list {next, prev}` / `file_entry` 0x38): the D15 types;
`file_list` and `file_entry` are typedefs of `file_find_list` / `file_find_node`. quicksave's fifth argument 0
converts to the callback pointer.

**D54. gr_light_desc +0x14** (stubs_inventory.h used stubs_status.h's `_unk14`): D23's `f14`. Code change:
`light._unk14` became `light.f14` (new_inventory.cpp, two stores).

**D55. item_charge_effect +0x10** (stubs_inventory.h `bool self_only`, items.h `ubyte self_only`): ubyte
kept; the inventory only tests it (`!charge->self_only`).

**D56. Names renamed in sum.symbols since the pilots** (entries with "(formerly OLD)"): every old name in
src/decomp code and headers now uses the current name; NOTES keep the old one as `NEW (formerly OLD)` (table
rows: the new name only). Not renamed: `level_display_name` (level_scripts.h) is our struct type name, not
the old name of 0x43cd50; the shared virtual slot 0 `dtor()` of the ui classes keeps its name although
0x52c3d0 / 0x553530 are now ui_gadget__destroy / ui_container__destroy (ui_element__dtor 0x542d40 and
ui_checkbox__dtor 0x553cd0 are the same slot). `automap_label_colors` (a pilot name) became the sum.symbols
name `automap_label_type_colors`; character_instance::get_pos became set_pos (0x51b390).

**D57. new_gamelist / new_pxochat: empty_frame_hook_40acc0** (declared in summoner/summoner.h as a shared
empty function; its body, a bare RET at 0x40acc0, lies inside new_gamelist.cpp's code range and is its mouse
mode 3 handler). Kept declared in summoner.h only, defined in new_gamelist.cpp: `node tools/asm.mjs callers
0x40acc0` lists 13 call sites in 11 functions of other units (automap_draw, mainmenu_frame, pxochat_frame,
state_loadgame_enter, game_render_frame, two level scripts, ...), i.e. identical-code folding made the
gamelist copy the one RET they all reach. new_gamelist.h includes summoner.h instead of declaring it again.

**D58. new_gamelist / new_pxochat: the declarations they need from other units** went into the headers of
those units: mp_gamelist_entry, the game-list and join functions, mp_player_level_available and the PXO lobby
(lobby_channel, lobby_* globals and functions, pxo_login_*) into summoner/multi/multi.h (the gathering header;
the lobby units are not known); file_make_path / file_exists_on_disk / file_delete / file_delete_small_files
into vsdk/vfile/file.h; bm_release into bmpman.h; trim_trailing_whitespace into plainstring.h; path_multicache
and cursor_set_type / cursor_url_bm / cursor_walk_bm into summoner/summoner.h; ui_choice_popup_margin_y and
ui_draw_panel_frame into interface.h. New unit headers: vsdk/inetfile/inetgetfile.h (InetGetFile, 0x18 bytes:
operator new(0x18) at 0x422038; directory guessed), summoner/interface/new_pxo_banner.h (open_url, strings.tbl
section new_pxo_banner.cpp id 834), summoner/interface/new_int_multi_update.h (version_check_run, strings.tbl
ids 932 / 933). mp_gamelist_used is declared as a whole mp_gamelist_entry, not void*[2]: the next symbol
(mp_join_retry_timer 0x22e45c0) is exactly 0x50 bytes later, and mp_gamelist_free_list (0x22e45c8) is followed
0x50 bytes later by mp_gamelist_count (0x22e4618), the same head-is-a-record layout as mp_player_list (D10).

## Checked

- Every header compiles on its own, and all headers compile together in one translation unit (no
  conflicting overloads among the 730 free functions).
- Every pilot .cpp passes `-fsyntax-only -Wall` with only the warnings it had before (new_status.cpp 1,
  character_instance.cpp 4, window.cpp 1), and `-Wmissing-prototypes` finds no definition without a
  matching header declaration except the unreferenced `ui_stub_ret_52afe0` (now declared in window.h).
- 2026-10-15, after D40-D56: the 77 headers compile on their own and together; all 32 .cpp files (with
  new_automap.cpp, new_inventory.cpp, new_hud.cpp) pass with the same three warnings and no
  `-Wmissing-prototypes` warning.


## objects.cpp / items.cpp pilot (decomp19)

**D59 (objects.cpp). List macros are `do { } while (0)`.** vsdk.h's `list_init` / `list_insert` /
`list_append` / `list_remove` had `{ }` bodies. The unoptimised objects.cpp keeps a `xor reg, reg / test
reg, reg / jne <start>` after every expansion (obj_link_new 0x469c12, obj__deinit 0x469e5d, objects_init
0x469918 / 0x469932 / 0x46994c), which is a `while (0)` test. Optimised units (player.cpp) drop it, so
their code is unchanged; the do-while form also accepts `if (x) list_init(a); else ...`. New in vsdk.h:
`plist_init` / `plist_append` / `plist_remove` for lists whose head is a pointer (NULL = empty; the
obj_slot free / used lists). `plist_remove` leaves the do-while with `break` when it unlinks the last
element: obj_create 0x469cb1 jumps past the while (0) test (0x469cf2), which an if / else could not do.

**D60 (objects.cpp). Character +0x5ac locomotion animations are also an array.**
character_fill_locomotion_anims (0x46af70) stores `[edx + 4*ecx + 0x5ac]` in a loop over the 18
locomotion_anim_names; the rest of the code uses the field names. `union { int loco_anims[18]; struct {
anim_stand ... anim_statscreen }; }` (anonymous struct: an MS extension VC6 accepts); offsets and
LAYOUT_CHECK unchanged.

**D61 (objects.cpp). obj slot 0 is `virtual void deinit()`; obj has a non-virtual destructor.** The three
vtables 0x5819c4 / 0x5819c8 / 0x5819cc have one slot each (0x469e10 obj__deinit, 0x46a710
item_obj__deinit, 0x46a920 container_deinit; the dword after 0x5819cc is the float 225.0). The header's
`vfunc0` is renamed `deinit`, so **player.cpp:191 `o->vfunc0()` must become `o->deinit()`** (its only
use). `~obj()` (0x469b40) is declared because obj_active_list's atexit destructor and objects_free_all's
expanded `delete` call it directly. mingw warns -Wdelete-non-abstract-non-virtual-dtor at those two
deletes; VC6 does not. item_obj gets +0xa0 level_item, +0xa4 in_container, +0xa8 ground_face
(LAYOUT_CHECK 0xac); constructors, destructors and deinit overrides of obj / item_obj / container are
declared.

**D62 (items.cpp). item_info is 16 bytes and +0 is the free-list link.** items_init_pool links the
records 0x8b50c8..0x8b9d48 with stride 0x10 through +0, item_alloc clears +0 and sets +0xc = -1,
item_free pushes through +0. `_unk00` (unused by the pilots) becomes `item_info *next`, `_unk0c` is added,
LAYOUT_CHECK 0x10.

**D63 (items.cpp). ItemType: typed mod pointers, render masks, the +0x68 union.** items_parse_table
allocates 8-byte {int, float} +Skill Mod: / +Resist Mod: records, 8-byte {int, int} +Stat Mod: records
and 0xc-byte {spell, float chance, int} +Triggered Effect: records, so the `void *` arrays become
`item_mod *` / `item_stat_mod *` / `item_triggered_effect *`. `_unk60[7]` is `render_mask[7]`
(+RenderMask:, indexed by hero_model_index). +0x68..+0xe7 is a union of the flat weapon fields (names
unchanged), `weapon_info weapon` (weapon_is_ranged and character_get_attack_range 0x45d947 pass
`type + 0x68`) and `char *armour_textures[2][4][4]` ([heavy][hero][part], from `(hero + 4*heavy) << 4`
at 0x44ab5d-0x44ab96). The old flags comment was wrong: bit 0 means NOT identified (item_is_identified
returns `!(flags & 1)`; item_flag_names[0] = "not identified").

**D64 (objects.cpp). lobj +0x08 is the flags word.** objects_update_frame tests `[lobj + 8] & 0x100`
(doors) and lobj_raycast ANDs it with the ray's +0x20 / +0x24 masks. `_pad0[0xc]` becomes `_pad0[8]` +
`flags`; lobj_pick_layer is a thiscall member. New in levelrender.h: `lobj_ray` (0x4c bytes, with an
inline constructor that clears +0x30: item_obj_spawn's only store before its first statement),
`lobj_hit` (0x2c), `lobj_raycast`, `lobj_raycast_all`, `door_update`, `num_lobjs`.

**D65 (objects.cpp). gr_cull_sphere returns bool.** item_obj_is_in_view returns its result with no
conversion after the call, while container_is_in_view's other path is `mov al, 1`.

**D66 (objects.cpp). profile_label is expanded inline in objects.cpp.** character_process_anim has an EH
frame whose only unwind action (funclet 0x57c3f0) calls 0x434650 (~profile_label) on ebp-0x10, but the
normal path calls neither constructor nor destructor. profile.h keeps them out of line (the optimised
units call them); objects.cpp's build expanded inline functions, as it also expanded vector3's operators.
Header not changed; noted for the real build.

**D67 (items.cpp). New headers.** `vsdk/os/norlsallocpool.h` (NoRlsAllocPool, 0x1c bytes; file name from
sum.symbols) and `engine/spells/spells.h` (spell_find_by_name, damage_type_to_status,
spell_display_names; directory guessed).

## conversation.cpp / conversationctl.cpp pilot (decomp20)

**C1 (conversation / conversationctl). Class names from sum.symbols' constructors.** The header had
`conversation_storage` for conversation_db (0x25461a0) and `class conversation` for conversation_active
(0x25464b0). sum.symbols names the constructors `conversation__ctor` (0x4d7b80, called with ecx = 0x25461a0
by init_conversation_25461a0 0x4d7b40) and `conversation_controller__ctor` (0x4d8ce0, ecx = 0x25464b0 from
0x4d87e0). So `class conversation` is now the 0x310-byte object and `class conversation_controller` the
running one; `typedef conversation conversation_storage` keeps the old name. No .cpp used either class name
(only the objects), so nothing else changes.

**C2 (conversation). conversation +0x204 / +0x208 are the speaker and the topic, not two "nodes".**
conversation_set_speaker (0x4d7d50) stores conversation_find_speaker's result at +0x204 and
conversation_set_topic (0x4d7e40) stores its topic argument at +0x208. new_dialogue.cpp reads them as
`cur_topic->flag2` / `cur_line->flag2`, `flag6`: kept as anonymous-union aliases (`conversation_node`)
beside the typed `speaker` / `topic`. The bits match: speaker +4 bit 2 is Cflag_NoAbort
(dialogue_parse_cflags 0x4fed30 returns 4 for "Cflag_NoAbort"; dialogue_frame tests `byte [eax+4], 4`),
topic +4 bit 2 is +Farewell (`or al, 4` at 0x500612) and bit 6 is +Shop (`or [esi+4], 0x40` at 0x4ff290).
The speaker's flag word is 16 bits (`mov word ptr [ebp+4], ax` at 0x4ff139; `or byte`/`and word` in
conversation_restore_state), the topic's 32 (bitfield reads `shr; test cl, 1`). To do: switch
new_dialogue.cpp to `speaker->flags & CFLAG_NO_ABORT`, `topic->farewell`, `topic->shop` and drop the aliases.

**C3 (conversationctl). Inline header functions emitted as COMDATs in this range.** 0x4d8f30
game_camera__get_pos / 0x4d8f40 game_camera__get_orient (`lea eax, [ecx+0x1a4/0x1b0]; ret`), 0x4d9a80
vector3__len_sq and 0x4da830 character_get_center are inline members of camera.h / vector.h /
living_entity.h: conversation_frame expands len_sq and get_center in place (0x4da170-0x4da1b1,
0x4da1d9-0x4da206) while conversation_camera_compute_shot / conversation_end_cleanup call the copies.
dialog_camera_pose::set (0x4d8f00) and operator= (0x4d9aa0) are treated the same way (inline in
conversation.h): conversation_end_cleanup copies poses with 6 inline moves (0x4daacf-0x4dab32) while
compute_shot calls 0x4d9aa0 for the same assignment. vector3's operators keep their out-of-line
declarations (the source is the same); the binary shows they are inline too (the same subtraction is
expanded in one place and called as 0x45b6b0 in another of the same function), to revisit with the
compiler.

**C4 (conversationctl). conversation_active is 0x27c bytes; 0x254672c is a separate variable.** The
controller's last member is the 16-slot array at +0xfc (16 x 0x18 = +0x27c, conversation_controller__ctor
0x4d8d3c-0x4d8d8c). conversation_frame reads and writes 0x254672c by absolute address (`mov byte ptr
[0x254672c], al` at 0x4da6c1, `mov al, [0x254672c]` at 0x4da48b), never through `this`, so it is a file-scope
bool after the object (conversation_dest_found, name ours). The old LAYOUT_CHECK of 0x280 was the gap to the
next symbol.

**C5 (conversationctl / new_dialogue). Topic box entries and choose_topic take a conversation_topic*.**
dialogue_topic_entry +0x14 was `int topic`. conversation_farewell (0x4d8760) passes the topic pointer from
the offered list ([entry+8]) to dialogue_topic_box_add_kind3, and conversation_choose_topic (0x4dabb0)
passes its argument to conversation_set_topic, which dereferences it (+4 flags). Now
`conversation_topic *topic`, `dialogue_topic_list::add(conversation_topic*, int)` and
`conversation_controller::choose_topic(conversation_topic*)`; new_dialogue.cpp compiles unchanged
(`add(0, kind)` passes a null pointer).

**C6 (conversationctl). pf_trace_edge_crossings takes (pos, layer, dest by value).** sum.symbols' comment
says "(pos, dest, size, layer)"; the call at 0x4da30b-0x4da348 pushes a 12-byte vector, then the layer, then
&pos, and cleans 0x14 bytes. Declared in the new engine/ai/pathfinding.h with the call's order; the
callee's own reading is not checked.

**C7 (conversationctl). The three console commands belong to conversationctl.cpp.** dialogue_make_topic_
available / _unavailable / dcamera (initialiser entries 371-373, 0x4dafe0-0x4db248) are untagged. Their
objects follow conversation_active in .bss (0x2546730-0x254675f, then cutscene_saved_equipment of the next
unit), their strings follow conversation.cpp's in .data (0x5a02cc-0x5a0451 after 0x5a0250-0x5a02cb), the
initialiser entries follow entry 370 (the controller), and the commands only call conversation_db. Defined
in conversationctl.cpp in initialiser order (the "unavailable" object is the lower .bss address).

**C8 (conversation). NoRlsAllocPool fields.** The header (items.cpp pilot) had `ubyte _data[0x1c]`. The
fields are from the member code: +0 block chain (Release walks [eax]), +4 start, +8 current (Alloc bumps
it), +0xc block size (InitBlocks stores its first argument), +0x10 remaining (Alloc compares the size with
it), +0x14 last allocation, +0x18 two bits (InitBlocks sets bit 0 and bit 1 = can_grow; Alloc tests 2,
Release tests 1). AddBlock / InitBlocks / Align / GetUsed added. `Alloc(int)` kept as declared.

**C9 (conversationctl). Smaller additions, no conflict.** camera: set_follow_flags / resume_follow /
push_state / pop_state. ai_state: is_traveling_sub0, goal_find (8 arguments: `push 0x6c ... push 1`),
goals_clear (already there). Character: size_radius, size_height, distance_between_edges,
status_effect_cure, walk_to_point (vector3 by value: `sub esp, 0xc` before the call), ai_add_goal9,
stopping_distance, clip_move_to_navmesh, play_move, move_finished, pick_talk_move, is_moving. matrix:
set_from_forward, get_axis_angle, set_from_axis_angle; matrix43(const matrix&). vector3: normalize_safe,
`operator*(float, const vector3&)` (0x468a90 is cdecl with the scalar before the vector), distance;
vector2::len. savegame: the conversation bit accessors. console: next_arg, command_argument_int, ARG_STRING
(2) / ARG_INT (8) as the commands pass them. level_scripts: script_object_suspend / resume_index,
conversation_execute_action, quest_find_by_name, quest_journal_set_entry. plainstring: stristr. summoner:
combat_mode_set. living_entity: status_effect_update_fx (handle, status).

## memory.cpp / stringpool.cpp / plainstring.cpp / parse.cpp pilot (decomp22)

**D200 (memory.cpp). The global operators are defined, not declared.** memory.cpp defines
`operator new(size, file, line)` (0x5124c0), `operator new[](size, file, line)` (0x512530), `operator
new(size)` (0x512550), `operator delete` (0x512570) and `operator delete[]` (0x512580). memory.h keeps
only the existing `operator_new` alias. The array forms stay out of the header: the only caller of
0x512530 is allocate_in_commonblock (same unit) and 0x512580 has no caller, while other units' `delete[]`
(targautils.cpp, stringpool.cpp Release) call 0x512570, so a declared `operator delete[]` would change
their code. The two deletes are written `throw()` so that the C++11 checker accepts them.

**D201 (memory.cpp). allocate_in_mempool is a Mempool member.** It is thiscall (`mov esi, [ecx+4]` at
0x5127c5, `ret 4`), so it cannot be Summoner 2's free function; it is declared
`void *Mempool::allocate_in_mempool(unsigned int)` keeping the sum.symbols name. operator new calls it on
`Pool` (0x5124d8).

**D202 (memory.cpp). Level counters placed in memory.cpp.** 0x2693004-0x2693020 (eight ints zeroed by
level_load 0x43859f-0x4385c9; level_texture_mem and level_s3d_mem named in sum.symbols) and
mem_bytes_allocated 0x2693034 lie between registry.cpp's last .bss object (0x2693000) and memory.cpp's
report globals (0x269303c), so they are defined in memory.cpp; the six unnamed ones get names of ours
(`level_mem_<addr>`), only the two named and mem_bytes_allocated are declared in memory.h.

**D203 (stringpool.cpp). StringPool members completed.** Allocate, Release, GetLength, GetRemaining,
GetBuffer, Add, AddQuoted, AddUniqueQuoted, SetLength and the size constructor 0x544e00 are added.
Add returns `char *` (the pointer into the pool), AddQuoted / AddUniqueQuoted take the TextFile (ecx of
the ParseString call is the argument, 0x544d51). SetLength's compare is unsigned (`ja` at 0x544e39), so
its argument is `unsigned int`. The constructors / destructor / SetLength follow the other members
(0x544dc0-0x544e30) and may be inline functions emitted last; they stay out of line.

**D204 (plainstring.cpp). Signatures.** gr_string_truncate_ellipsis and int_to_string_with_commas return
`char *` (EAX = dest on every return path, NULL for the too-small case of the latter); format_indented_column
takes (dest, indent, width, label, fmt, ...) for both variants (0x5150c9 strcats arg 3, 0x5150fc compares
arg 2 with the length; the sum.symbols comment of 0x515030 lists label before width). The 4th argument of
gr_string_truncate_ellipsis lands in gr_get_string_size's length slot (0x5153e5 pushes it after -1);
every caller passes -1, the header keeps the name `font`.

**D205 (parse.cpp). TextFile +0x08 is the file name.** TextFile::Open copies the name to this + 8
(0x515d5e-0x515d81), Error prints this + 8 as the file (0x515f56), packfile_find_in_packfile gets it
(0x515d96). `flag08` / `_unmapped[0x3f]` (no users) become `char filename[0x40]`. Open returns 1 when a
blasted packfile lacks the file, File::open's negative error, else 0; ParseString, CopyUntil and
ParseDelimitedString return the length; ParseIntList's third argument is a lookup function
`bool (*)(int *, int, const char *)` (called at 0x5169c4; the callers' 0 still converts).

**D206 (parse.cpp). file_blast_into_buffer is in parse.cpp.** Its code (0x517220) follows TextFile::AtEnd
and precedes the movie code, its literal 0x5a7d78 closes parse.cpp's .data run that starts with the -1
at 0x5a7950 (written only by it: `file_blast_size`, name ours), and its "" literals 0x26b63dc / 0x26b63e0
follow SkipLine's 0x26b63d4 in .bss. file_blast_filename is taken as 0x30 bytes (0x26b63a4-0x26b63d3)
because 0x26b63d4 is a separate literal. New header `vsdk/vfile/packfile/file_packfile.h` declares
packfile_read_whole_file and packfile_find_in_packfile(vpp, name, &size, &offset): Open stores the third
into TextFile::size and adds the fourth to the buffer (sum.symbols says &offset, &size).

**D207 (parse.cpp). Crc32 and the String additions.** New header `vsdk/os/crc32.h`: Checksum calls
0x514eb0 (value = 0, vfile's COMDAT) and then 0x552120 (value = 0 again, sum.symbols Crc32__Crc32), and
name_hash_nocase stores 0 inline before calling 0x552120, so the constructor is an inline `value = 0` and
0x552120 is a reset (`init`, name ours). String gains assign_n (0x5566e0), static from_float (0x557c00,
cdecl with the hidden result) and the three cdecl `operator+` (0x556f70 String + String, 0x557070
String + char*, 0x557180 char* + String), used by vector3_to_string / matrix3_to_string. File gains
write_line (0x5264e0).
