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

## new_skill.cpp / new_spell.cpp / new_tutorial.cpp (decomp21)

**D100 (new_tutorial.cpp, new_skill.cpp). lobj +0x20 is a matrix; door records; two-stage state is
{open, locked}.** tutorial_cond_combat_part2 copies three dwords from door +0x28 to lobj +0x14 (the
position) and calls quaternion__to_matrix_b (0x522cc0, thiscall on door +0x34) with `lobj + 0x20`
(0x42ebbf-0x42ebd8), so lobj's `_pad20[0x24]` becomes `matrix orient` (size unchanged; lobj already had
vector3 members). New `door_info` (0x8c bytes, door_from_lobj's records): +0x28 open_pos, +0x34
open_orient (quaternion), +0x58 float lock (character_use_skill compares it with the pick lock skill
+ 0.05 at 0x4295fa-0x429626; 0 = not locked). door_from_lobj, lobj_find_by_name, fx_disable_by_effect_name
are declared in levelrender.h (units not known). savegame::two_stage_get(bool *, bool *, lobj *) /
two_stage_set(bool, bool, lobj *): the tutorial opens the door with set(1, second, l) (0x42ebad-0x42ebba),
the pick lock unlocks it with set(first, 0, l) (0x429640-0x42964d), so the first bit is "open" and the
second "locked". quaternion gets to_matrix (0x522bf0) and to_matrix_b.

**D101 (new_tutorial.cpp). level_entered_by_load gates the tutorial the other way round from its name.**
tutorial_frame and tutorial_draw return at once when the byte 0x25443a8 is 0 (0x42ef0b-0x42ef12,
0x42f030-0x42f03e), so the hints run only while it is set. sum.symbols says it is set "when a save is
pending ... blocks the party joins and the tutorial". The code is written as the binary has it
(`if (!level_entered_by_load || Ignore_tutorial) return false`); the symbol's name or comment needs
checking in game_change_level (open question, NOTES).

**D102 (new_skill.cpp). skill_info fields.** `_unk04` is the AP cost (skill_is_available 0x42929c,
character_use_skill 0x429480, character_use_skill_on 0x42b096 / 0x42b140 for skills 0x13 / 0x12);
+0x14 the skills screen group (skills_count_in_group 0x4275c2, skills_set_page 0x427c6e); +0x18 the
spell that carries the skill out (0x4294e1, 0x429577); +0x1c a localize slot, the summon / aimed-attack
popup name (0x428a27, 0x428abc; skill_find_by_display_name compares it); +0x20 flags (bit 0 passive:
0x4290c7, 0x427cf9; bit 1 attack-only: 0x429189; bit 2 summon, bit 3 aimed-attack part: skill_is_summon /
skill_has_flag8); +0x30 a second description slot shown first for active skills (skills_show_tooltip
0x428437-0x428453). +0x0c hp_share from docs/sum-exe/summons.md. Size unchanged (0x34).

**D103 (new_skill.cpp). ui_choice_popup_show's last three arguments.** skills_handle_mouse passes
(4, skills_popup_flags, skills_popup_enabled) (0x428798-0x4287a2); sum.symbols reads them as flags, an
int* of values and a bool* of enabled entries. The prototype keeps `int a, int b, int c` because
new_inventory.cpp already passes its bool array cast to int; new_skill.cpp casts the same way. A later
pass can make them `int flags, int *values, bool *enabled` and drop both casts.

**D104 (new_skill.cpp). skills_popup_skills holds names.** sum.symbols types 0x5f4f60 int[16] "skill
ids", but both menu builders store localize_string_or_invalid(skill_table[s] +0x1c) there (0x428a43,
0x428ad8) and skills_summon_popup_frame passes the chosen entry to skill_find_by_display_name (0x428b98):
it is the `char *items[]` of the popup. The ids go to skills_popup_flags as `skill << 2 | 3`.

**D105 (new_spell.cpp). spell_is_usable(int spell, Character *c, bool ignore_cost),
character_cast_spell(int spell, Character *c).** The second argument of spell_is_usable is loaded into EDI
and replaced by cur_character when 0 (0x42ae23-0x42ae2b), the third (byte) skips the AP and cooldown tests
(0x42ae4b); character_cast_spell's second argument is the caster (0x42af31-0x42af3b). The old prototype
(int, int, int) / (int, int) took 0s from the HUD, which still compiles. character_use_skill_on and
character_cast_at_point (world_mouse_frame 0x43af07-0x43af7c) are declared with (spell, target / pos,
layer, priority, user).

**D106 (new_skill.cpp). character_can_use_summon(int skill, Character *c).** The callee reads the skill at
[esp+4] (0x446ec0) and the character at [esp+8] (0x446eab); sum.symbols' comment says "(character, skill)".

**D107 (interface.h, new_hud.h). Shared .rdata tables.** 0x57ef40 {2,2,2}, 0x57ef4c {8,8,8} and 0x57ef58
{5,5,5} are ui_draw_panel_frame's frame width and insets (0x4134d1-0x41350f; tutorial_layout /
tutorial_draw size the box with them); names ours. hud_hot_button_size (0x57ee00, defined `const` in
new_hud.cpp) is declared `extern const` in new_hud.h: the skills and spells rows use it for their icons.
With the declaration in scope the definition gets external linkage, as it needs in a real build.

**D108 (new_skill.cpp, new_spell.cpp). skills_count_page / spells_count_page take no argument.** Both
callers (skills_init 0x42753c, the refresh functions 0x429055 / 0x42ad95) push nothing and the bodies read
no argument; sum.symbols' "(page)" is wrong. They loop over the 49 buttons with a switch on the index
(byte tables 0x427b3c / 0x42a01c over i - 23).

## multisave.cpp / multitrade.cpp / multichat.cpp / multicontainer.cpp (decomp24)

**D400 (multisave.cpp). The .plr summary is its own type, mp_char_summary (0x12c bytes).**
mp_player_file_read_summary stores +0x20 (char_info, 0x46d591), +0x24 (flags, 0x46d59c), the string at +0x28
(0x46d5b4) and +0x128 (level, 0x46d5d7) of its output; mp_player_file_write keeps one in a local at esp+0x16c
whose description it reads at +0x28 (0x46d146 lea [esp+0x194]). The callers pass mp_char_record + 0x28, so the
record is a file name followed by this. New `mp_char_summary` in `summoner/multi/multisave.h`; mp_char_record
is unchanged (its users read rec->name etc.). mp_player_file_read_summary keeps `void *out` (new_charselect.cpp
passes `rec->name`, a char*); the definition casts. The flags field is `int`: the version-8 conversion uses
`sar` (0x46d5a6, and 0x46d779 for mp_player +0x30).

**D401 (multitrade.cpp, new_itemtrade.cpp). mp_trade_partner is a Player*.** mp_handle_it_rqst passes
player_from_handle's result (0x471096) to mp_send_it_acpt, which stores it with mp_trade_set_state(2, p)
(0x4712b3); mp_handle_it_qury / _stat compare player_from_handle(...) with it (0x471ca6, 0x471dc6); +0xc goes
to mp_handle_to_net as a handle. multi.h's `struct mp_trade_peer {_unk00[0xc]; id; _unk10; mp_player
*player;}` was the same record seen from new_itemtrade.cpp (+0x14 = Player::mp_owner). Now `typedef Player
mp_trade_peer;` (multitrade.h) and `extern Player *mp_trade_partner;`; Player +0x14 becomes an anonymous union
{mp_owner, player} so new_itemtrade.cpp's `mp_trade_partner->player` still compiles unchanged.

**D402 (multi.h). mp_player +0x18 socket, +0x30 profile_flags.** +0x18: mp_forward_cur_packet skips
records with -1 (0x44230e), chat_cmd_ping passes it to net_get_reliable_socket_ping (0x487c85, 0x487cb8).
+0x30: mp_player_file_write writes it after the char_info (0x46d21b), mp_player_file_load reads it back
(0x46d769) and converts version-8 bits with sar (0x46d779), so `int profile_flags`. `_unk18[4]` and the
first half of `_unk2c[8]` are gone (no pilot used them).

**D403 (multi.h). Packet helpers.** Every sender builds {type byte, ushort data size, data} in a 0x200-byte
local and bumps mp_packet_table[type].sent (+0xc); the variable-length ones keep a ushort size in a register
(`and esi, 0xffff` in mp_send_container_close 0x488e21, container_spawn_loot 0x489088) and close with
`size - 3` at +1 (0x488e42, 0x4890e9); chat_build_packet takes the size by address. Written with our macros
MP_BUILD_HEADER / MP_ADD_DATA / MP_END_PACKET; with constant offsets they fold to the stores seen in the
fixed-size senders. MP_IS_HOST() / MP_IS_CLIENT() spell the `(game_flags & 2) && mp_local_player &&
(flags & 1)` test in that order (living_entity.cpp's unoptimised copy at 0x453077-0x45309a shows the three
terms). `struct mp_packet_info` (types.h layout) added with name[8] + 4 bytes: multi_process_packet reads the
bytes +0x18 / +0x19 (0x4418a6, 0x44191d). The handler type is (data, sender address): 0x44196e pushes
mp_cur_packet + 3 and the address argument.

**D404 (multisave.cpp). The PXO settings are one struct, pxo_config pxo_cfg (0x610 bytes at 0xa59a88).**
pxo_read_config clears 0x184 dwords from 0xa59a88 (0x46daa6) before it sets +4 (word 4200), +8 and the six
0x100-byte strings. multi.h's `extern unsigned int pxo_cfg_flags;` becomes `#define pxo_cfg_flags
(pxo_cfg.flags)` (with pxo_utracker_server / pxo_gtracker_server / pxo_chat_server likewise), so the four
pilots that read or write pxo_cfg_flags compile unchanged. The next 0x208 bytes (0xa5a098 word, 0xa5a09c
string; read only by InetGetFile__InetGetFile 0x51b84f-0x51b891, never written) lie before s3d.cpp's .bss
and are defined here as inet_proxy_port / inet_proxy_server[0x200] (names ours).

**D405 (item_paging.cpp, multisave.cpp). mp_player_autosave_timer moves to multisave.cpp.** item_paging.cpp's
last code is the jump table 0x46cc50-0x46cc5f; sum.symbols tags the initialiser pair 0x46cc60 / 0x46cc70
multisave.cpp (strings.tbl id 237 in mp_player_file_write), and only mp_player_autosave_start / _frame /
mp_player_save_on_leave use the timer. The definition and its initialiser comment are now at the top of
multisave.cpp; item_paging.cpp's range is 0x46c5d0-0x46cc5f.

**D406 (multi.h). Unit headers for the four units.** `summoner/multi/multisave.h`, `multitrade.h`,
`multichat.h`, `multicontainer.h` (file names from the module column, directory ours) hold the units'
types, globals and prototypes; multi.h includes them at its end, so every `#include "summoner/multi/multi.h"`
sees the same declarations as before. Moved there: the .plr prototypes and pxo_cfg_flags (multisave.h),
mp_trade_item and the trade globals / prototypes (multitrade.h), chat_send_line (multichat.h),
mp_send_container_close (multicontainer.h). The unit headers use forward declarations only, except
multitrade.h, which includes player.h for the mp_trade_peer typedef.

**D407 (decomp24). Smaller additions, no conflict.** File: read_float (0x525a30), read_string (0x525ee0,
(buf, size, min_ver, default): pushes 0, 0, 0x20, buf at 0x46d56c), write_int16 (0x526230), write_float
(0x526270), write_string_z (0x5263e0). characterinfo: chain_skill_get_points / chain_skill_set_points
(Player*, skill[, points]) and chain_config_reset_defaults. objects: party_inventory_clear,
party_inventory_find_type_index (by type index, pushes the index at 0x471913), OBJ_TYPE_CONTAINER (11: the
container handlers compare +0xc with 0xb). savegame: hot_buttons_clear_member (thiscall, one argument at
0x46d96f). matrix: matrix(r, u, f) (0x50fad0, thiscall, ret 0xc). net: net_get_reliable_socket_ping.
mp_quantize_position(short*, short*, short*, vector3 by value: the vector is copied to the stack first,
0x488fd3) and mp_dequantize_position (hidden result, 0x489202).

## decomp23 (spells.cpp, combat.cpp)

**D300 (combat.cpp). living_combat moves to engine/combat/combat.h with its fields named.** The class
was an opaque 0x54-byte block in living_entity.h; combat.cpp's code gives every field (owner +0x00, target
handle +0x04, opponent record +0x08, GameTimer +0x0c, state +0x14 ... backstab flag +0x50; layout check
0x54 kept). living_entity.h now includes combat.h. The constructor 0x4681b0 is a real constructor
(`GameTimer__GameTimer(this+0xc)`, `living_combat_reset`, `mov eax, esi`), but Character keeps the D1
union (`combat` / `protection`, new_assess.cpp still uses the second name), and a union member cannot
have a constructor. So the GameTimer at +0x0c is declared as `int timer` and the constructor is written
as `living_combat *construct()` (placement-new of the GameTimer, then reset). To do with D1: drop the
union, type +0x0c as GameTimer and turn construct() back into living_combat::living_combat().

**D301 (combat.cpp). Character +0x6D0 is a move record pointer.** `cur_move` was `int`; combat.cpp
dereferences it (`mov eax, [edx+0x6d0]; mov ecx, [eax+0x18]` at 0x465fca, 0x4637e0-0x4637ea), so it is
`move_info *`. New partial `move_info` in characterinfo.h (category +0x10, id +0x14, variant +0x18,
triggers +0x1c, next +0x40, hit tag +0x44; type name ours). No pilot read the field.

**D302 (combat.cpp). character_instance::get_bone_transform takes (pos, orient, bone).** 0x51b460 reads
the bone from `[esp+0xc]`, passes `[esp+8]` to matrix43::get_rotation and `[esp+4]` to get_position;
combat_fire_weapon_missile pushes (bone, &orient, &pos) (0x46399c-0x4639a5). The pilot's declaration
(bone, orient, pos) does not match, but character_instance.cpp defines it, so the correct order is
declared beside it as an overload; combat.cpp calls that one. To do: swap the definition in
character_instance.cpp and drop the old declaration.

**D303 (combat.cpp). character_play_move's first argument is the move record.** Every combat.cpp call
passes a character_find_move / find_move_alt result (0x463635, 0x463702). decomp20 declared it `int move`;
the declaration is kept with the same text for the merge and combat.cpp casts. To do: make it `move_info *`.

**D304 (spells.cpp). spell_info +0x44 is a sound_ref.** spells_reset_runtime sets the handles at +0x14,
+0x24, +0x38 and +0x48 to -1 (0x47a789-0x47a791), so +0x44 is a fourth sound_ref (`banish_sound`), but
the parser never fills it: +Misc effect: and +Banish effect: both pass +0x34 to spells_parse_sound
(0x47a39a). `_unk44[0xc]` becomes `banish_sound`.

**D305 (spells.cpp). spell_groups[] holds (count, first record).** spells_parse_table stores the record
pointer (`mov [8*ebx+0x22e9264], edi`, EDI = the record's +0x00) and spell_get_group reads
`[first + 4 + 0x80*k]` (the record's index); `first` is `spell_info *`.

**D306 (spells.cpp, combat.cpp). Output-argument order of the tag / node transforms.** The call sites
push the node or tag last: vfx_instance::get_node_transform(pos, orient, node) (0x47e348-0x47e35c, the
position is unrotated afterwards), vim_get_tag_transform(vim, tag, pos, orient) (0x463975-0x463987, the
outputs feed matrix43(pos, rot)). sum.symbols' comments list the index first; the declarations follow
the pushes.

**D307 (combat.cpp). charinfo_collect_moves(out, category, info, a, exclude, class).** The only call
(0x463eb4-0x463ec9) pushes class, exclude (+0x18), 0, info, 0xa, &out; sum.symbols lists
"(out[], info, category, exclude, bit)". The declaration follows the pushes.

**D308 (combat.cpp). Small signatures from the call sites.** mp_send_living_combat(attacker net,
defender net, variant, defence move, state argument, skill, special) (0x4635b4-0x4635c7);
boss_counter_hit_dispatch takes two arguments (the `add esp, 0xc` at 0x467e98 also pops
character_break_paralysis_on_hit's); lighting_sample_stub's outputs are bytes (combat_spawn_blood reads
them with `and eax, 0xff`); spell_fire_missile(spell, missile, caster, target, start, orient) and
spell_place_ground_effect(spell, caster, pos, owner, orient) from their bodies (sum.symbols' argument
lists differ).

**D309 (spells.cpp). spell_effect records are bit fields.** The pool reset does `and ecx, -2; or ecx,
0x7e` and spell_effect_alloc `or ecx, 0x7f`: bit 0 "used" and a 6-bit field set to all ones; declared as
`unsigned used : 1, slot : 6`. Names ours.

**D310 (spells.cpp, combat.cpp). Declarations shared with decomp20.** next_arg, ARG_STRING,
normalize_safe, vector2::len, operator*(float, const vector3 &), size_radius, distance_between_edges,
status_effect_cure, is_moving, play_move and move_finished are declared with decomp20's text (its copy of
the headers is newer than this one), so the merge only has to drop duplicates. combat.cpp defines
operator*(float, const vector3 &) (0x468a90 lies in its range).

## summoner.cpp (decomp26)

**D600 (summoner.h). console_sound_buffer is an int.** sum.symbols types 0x60ad78 `void*`, but cmd_play_sound
stores snd_get_handle's EAX there (0x43d6b0) and pushes the same value as the handle argument of
snd_play_streamed (0x43d6af); console_sound_stop only tests it and writes 0 (0x43d731-0x43d73a). Declared
`int` like the other sound handles.

**D601 (summoner.cpp). WinMain's __try / __except.** The SEH frame (push 0x580e50 scope table, push
0x56de44 _except_handler3, the filter at 0x43e625 calling RecordExceptionInfo(GetExceptionInformation(),
"Summoner Main Thread")) is written as `__try { } __except (...) { }` under `#ifdef _MSC_VER`; mingw has
no __try, so the syntax check sees only the plain MainLoop call in the `#else` branch.

**D602 (localize.h). XSTR_SLOT.** level_display_names holds the localize_strings slot, not the string:
init_level_display_names stores immediates 0x25cddc8.. = &localize_strings[571].. (0x43cd80). The image
holds only entry 0's name ("test" at 0x594440 -> 0x5945d8); everything from entry 0's slot on is
written by the initialiser, so to VC6 the slot address was not a constant expression. The macro
`XSTR_SLOT(text, id)` (name ours) gives `&localize_strings[id]`; how the original spelled it is open.

**D603 (level_scripts.h). level_display_names has 51 entries.** It was `[52]` ("51 used"). The table is
0x594440-0x5945d7: the "test" literal starts right after it at 0x5945d8, both search loops stop at
`cmp esi, 0x5945d8` (saveload_format_level_name 0x426c48, level_format_display_name 0x4d63e2), and the
initialiser's last store is 0x5945d4. sum.symbols' `char*[104]` is one pair too long.

**D604 (summoner.h). config_solo_confirm is a macro.** It was `extern bool config_solo_confirm` at
0x60acd5, which is Summoner_options.solo_confirm (+0xd). With summoner.cpp defining Summoner_options a
separate extern would have no definition; `#define config_solo_confirm (Summoner_options.solo_confirm)`
keeps new_hud.cpp's spelling.

**D605 (summoner.h). SummonerOptions::set_defaults.** summoner_cfg_load writes the 21 default values
(0x43e295-0x43e320) in the same order and with the same values as the inline constructor
(init_game_config_defaults 0x43d190). Both are now one inline member, set_defaults(), that the
constructor calls; code unchanged for new_options.cpp's options_edit.

**D606 (summoner.h). resolution_table is unsigned.** sum.symbols types 0x580e38 `int[3][2]`; LoadSettings
halves the entries for mouse_move_to with SHR (0x43db6f / 0x43db71), which VC6 emits for an unsigned
`/ 2` (a signed one adds the sign bit first). Declared `const uint resolution_table[3][2]` and defined
in summoner.cpp (.rdata, next to WinMain's scope table 0x580e50).

## matrix.cpp / skeleton.cpp (decomp25)

**D500 (matrix.cpp). The small vector3 / matrix / quaternion / matrix43 members are inline; this settles
D42.** The math code shows two kinds of members. (1) Out of line in their unit: vector3::normalize
0x524430, normalized 0x524270, is_valid 0x524220, dot_xyz 0x5241c0, ... sit in the vector unit
(0x523f80-0x524930), and the matrix / matrix43 constructors, get_rotation, operator* sit in matrix.cpp;
optimised game code calls them (combat_get_weapon_tag_position calls 0x511b70 / 0x511450,
conversation_camera_capture calls 0x5113a0). (2) Members whose only copy lies among another unit's
code: vector3() 0x505530 (localize), (x, y, z) 0x43b580 (gametarget), copy 0x410cd0, operator=
0x413190 (interface code), + / - 0x45b680 / 0x45b6b0, += 0x48e440, *= / * / -= 0x50f6a0 / 0x50f790 /
0x50f840 and quaternion(x, y, z, w) / = 0x50f990 / 0x50f9b0 (the gr unit before matrix.cpp), cross
0x50d7e0 (after ca_render), matrix() 0x50f660 (gr), matrix::operator= 0x4a4320 (shadows), and six
functions after matrix.cpp's last function 0x511bb0: vector3::set 0x511c10, operator== 0x511c30,
matrix43 transform 0x511c70, set_position 0x511cf0, get_position (by value) 0x511d10, set_rotation
0x511d30, the same tail pattern as character_instance.cpp's 0x51b6b0-0x51b75f. These are COMDATs of
header inlines: matrix.cpp, compiled without inline expansion, calls them for every use (0x50f9f0,
0x50fafd, 0x5106f8, ...), while the optimised conversationctl.cpp expands transform in place with the
same expression tree as 0x511c70 (x = (m10 v.y + m20 v.z) + v.x m00 + m30 at 0x4d8e45-0x4d8e59 and
0x511c83-0x511c97), and objects.cpp expands matrix::operator= with a call of 0x413190 inside (D66).
So vector.h / matrix.h now define these members in the class body (bodies from the COMDATs); the
declarations and signatures are unchanged except D501. With inline definitions the /Ob1 units' inline
3-dword copies of D42 and the /Ob0 vsdk units' calls both follow from the same source. The vector-unit
members stay declared out of line. matrix::operator= is written as three row assignments (objects.cpp
calls 0x413190 inside it; 0x4a4320, compiled in an optimised unit, has them expanded).

**D501 (matrix.cpp). Signatures.** quaternion::operator= returns void: 0x50f9b0 ends with EAX = q.w
(`mov eax, [eax+0xc]`), not this (the old header returned quaternion &; no .cpp used the result).
matrix43 has two get_position members: 0x511d10 returns by value (inverse, anim_pos_key's constructor)
and the existing 0x51b6e0 takes an out pointer (character_instance.cpp); different signatures, so both
COMDATs survive. vector3::negated_b 0x524340 (vector unit, only caller matrix43::inverse) is a second
negation besides the inline operator- 0x51b700 (name ours). New members: matrix copy / (r, u, f) /
sin-cos / angle / axis-angle constructors, multiply_by_transpose, multiply_transposed, rows_match,
set_from_sincos, set_from_angles, has_zero_axis, is_valid, determinant, the four orthogonalize
variants, transpose, inverse, invert, transpose_in_place, set_random, set_from_unit_forward,
set_from_forward_up / _right, rotate_between; matrix43 equals, make_identity, set_product_4x4,
transpose_in_place, set_rotation; quaternion(const matrix &) 0x522610 (returns this: a constructor;
sum.symbols quaternion__set_from_matrix), normalize 0x522f20; cdecl matrix43_interpolate,
matrix43_interpolate_quat, matrix43_set_interpolated; globals identity_matrix, identity_matrix43,
Homogeneous_column. extract_angles and get_axis_angle became const.

**D502 (skeleton.cpp). Layouts.** skeleton is 0xd0 bytes (skeleton_mem_used +/- 0xd0 in read /
~skeleton): +0 / +4 the rotation / position key arrays, +8 file name (strcpy in the constructor), +0x48 /
+0x4c key totals, +0x50 / +0x54 list links (list_remove in ~skeleton), +0x58 name (ca_string_pool),
+0x68 vector3 root displacement, +0x74 quaternion root rotation (read_quaternion, slerp operand; 16
bytes up to num_bones +0x84), +0x88..+0x9c weights, tracks, morph counts and arrays. sum.symbols'
skeleton_add 0x543d50 is the five-argument constructor (thiscall, constructs +0x68 / +0x74, returns
this, ret 0x14; character_add_skeleton calls it on operator new(0xd0)); skeleton_read 0x544090 is
skeleton::read(FileReader *, name, path_id) (ECX = the new skeleton, ret 0xc). anim_track gets its
counts (+0 position, +2 rotation, ushort) and key pointers (+8 / +0xc). The key classes anim_key,
anim_pos_key (0x28), anim_rot_key (0x14, with a cquat of four int16), anim_vec_key (0x10) and
skeleton_morph (0xc) are new; anim_key's and anim_pos_key's constructors and get_size are inline (the
COMDATs 0x544ad0-0x544b3f follow skeleton.cpp's last function), 0x544b40 / 0x544b60 / 0x544b70 are
compiler-generated (implicit constructors, new[]'s default constructor closure). vim is 0x68 bytes
(operator new(0x68) in vim_alloc) with the data block, sub-meshes (vim_submesh, 0x30), flags +0x2c,
header copies, tags (vim_tag, 0x60) and ca_skin at +0x38: ca_bone, ca_bone_ext and ca_skin moved from
character.h to skeleton.h (vim_read fills +0x38..+0x4c with the ca_skin fields; ca_mesh +4 points
there). ca_bone_ext +0 is a name, not an int: vim_read stores ca_string_pool.AddUnique there and
vim_write_header writes it with write_string_z. vim_header (0x24) is new. The vim_get_tag_transform
declaration of decomp23 matches 0x543c70.

**D503 (skeleton.cpp). FileReader, other additions.** New class FileReader (0x14 bytes, 0x513a20-0x513e5f)
in vsdk/vfile/file.h, with the fields of sum.symbols' constructor comment; File gains write_vector
(0x526280). bmpman.h gains bm_get_filename (0x503b60), character.h declares ca_string_pool (0x2dc7070,
character.cpp .bss), skeleton.h declares the skeleton_* counters (0x2ee4598-0x2ee45a8, defined in
skeleton.cpp: they lie between its literals 0x2ee4578-0x2ee4590 and character.cpp's 0x2ee45ac).

**D504 (matrix.cpp). x87 order.** VC6 without /Op commutes and reassociates float sums here:
matrix43::operator* (0x5114b0) adds the four terms of the three columns of one row in three different
orders ((a2 b20 + h b30) + a0 b00 + a1 b10, (h b31 + a2 b21) + a0 b01 + a1 b11, (a2 b22 + a0 b02) +
h b32 + a1 b12), and matrix::determinant's products come out as fz rx uy. So the order of the source
terms cannot be recovered from the instructions; the expressions are written in natural index order.
What the code does fix: which partial results are reused (2yy, 2zz, 1 - 2xx in set_from_axis_angle;
1 - t in the interpolations), which are spilled, and `2 * a` compiled as `a + a`.

**D1000 (character.cpp, decomp30). ca_character layout and members.** ca_character is 0x1f8 bytes, not
0x1e8: the initialiser 0x540820 calls the EH vector constructor with (0x2dc7088, 0x1f8, 200, 0x540950,
0x540960), and ca_init / ca_close / ca_find_character step by 0x1f8 to 0x2ddfa48. +0x44 is a flags word
(`test byte [esi+0x44], 8` in character_free_skeletons, `or al, 4` in character_load_shadow, bit 0 in
ca_close), so the name is 0x44 bytes (was 0x48). +0x4c is the count saved by character_lock_skeleton_count,
+0x50 and +0x1f0 are only cleared (character_clear), +0x1e8 are two LOD mesh slots (character_get_lod_index
and character_unload loop twice from +0x1e8), +0x1f4 is the model Mempool (character_create_base stores
its third argument there; character_reload_mesh calls ResetOffset on it). The character_* functions that
take ECX (character_add_skeleton ... character_get_lod_index, 0x540970-0x541270, ret n with ECX = the
character) are members with short names, the flat symbol in the comment (as living_entity.h's
find_move); those with the character on the stack (character_clear, character_unload, character_release)
stay free functions. Constructor 0x540950 (`mov eax, ecx; ret`) and destructor 0x540960 (`ret`) are
user-declared and defined out of line in character.cpp: an implicit pair would not exist as functions for
the vector constructor. num_anims keeps its name (character_instance.cpp uses it).

**D1001 (character.cpp). ca_mesh, ca_bone name.** ca_mesh is the 0x30-byte slot of the table at 0x2ddfa48
(205 entries to 0x2de20b8; ca_preload_alloc_slot: `lea eax, [ecx+2*ecx]; shl eax, 4`): +0 owner (NULL =
free; character_create_base stores the character, character_unload clears it), +4 skin (= vim + 0x38),
+8 the stripped file name (strcpy in character_create_base; character_reload_mesh passes slot + 8 to
load_shadow), +0x28 the mesh (cleared by ca_preload_alloc_slot), +0x2c the shadow mesh (load_shadow).
sum.symbols types ca_preload_slots as uint8[0x300]; it is ca_mesh[205]. +0x1f0 of ca_character is kept as
an int, not a third LOD: every LOD loop runs twice, although character_create_base stores lods[i] for every
name i < num_names without a bound. ca_bone (skeleton.h) +0 is a name: character_find_mesh_tag
`_stricmp(name, skin->bones + i * 0xa0)`; its length (0x50, up to bind) is ours.

**D1002 (character.cpp). ca_vertex_weights.** 0x5408f0 (sum.symbols ca_vertex_weights__clear) returns
`this` (`mov eax, ecx` before zeroing the four bytes): a constructor, not a void clear. 0x540900, a bare
`ret` directly after it, is taken as the empty destructor (unsure). 0x540940 zeroes the same bytes and
pops two unread arguments (`ret 8`) without returning this: an ordinary member clear_b(int, int). The
struct is 8 bytes (add stores the weight at +4 + index). None of the four has a caller.

**D1003 (character.cpp). Preload index.** ca_load_preload_index reads four ints (the first two are
dropped; ca_preload_count is the third, ca_preload_size the fourth), then per entry 0x40 name bytes into a
stack buffer (dropped) and two ints into 0x2ee20c4 + 4i / 0x2ee21dc + 4i (70 entries each up to
0x2ee22f4: ca_preload_offsets / ca_preload_sizes, names ours), then seeks to 0 and reads the whole file
into ca_preload_data. ca_preload_find compares names at 0x2de20d4 + 0x48 i, i.e. inside the data block
after the 0x10-byte header: ca_preload_header / ca_preload_entry (0x48) describe that block. ca_preload_data
is 0x100000 bytes (0x2de20c4 to the offsets array; character_add_skeleton only uses the preload data for
sizes below 0x100000).

**D1004 (character.cpp). Signatures.** character_load's third argument is a bool (`cmp byte [esp+0x14],
1`) that sets flag 2, which character_release_all skips (keep_loaded); the fourth is never read.
character_add_skeleton(filename, name, is_state, unused, check_dup, root_rotation): it passes arguments
1, 2, 3, 4, 6 to the skeleton constructor 0x543d50 (whose order decomp25 fixed) and compares argument 5 as
a byte with 1 before the duplicate search (callers charinfo_load_move_anim / charinfo_load_script_anim
pass 1). The trigger functions take a float frame number (callers push it with fstp). New declarations:
Mempool::ResetOffset (inline in memory.h like getPoolSpaceLeft, D12: the COMDAT 0x505540 `mov [ecx+8], 0`
is called by character_reload_mesh), bm_replace_file 0x5031c0 (int; -1 on failure), charinfo_find_by_name
0x446b80 (characterinfo.h; character_load_shadow tests `char_infos[i].flags & 1`, +0x258).

## s3d.cpp / levelrender.cpp (decomp28)

**D800 (s3d.h, levelrender.h). The level is one object, s3d_level level_s3d (0xe0 bytes at 0xa5a2a0).**
sum.symbols types 0xa5a2a0 `uint8[0x28]` and names its later fields as separate globals (num_lobjs
0xa5a2c8, lobjs 0xa5a2cc, level_num_navpoints 0xa5a2d8, ..., level_texture_bms 0xa5a320). But
level_load_files passes 0xa5a2a0 to level_load_s3d (0x438708), s3d_level_clear zeroes 0x38 dwords of it
(0x46e1a5), every reader addresses the counts and arrays as level +0x20..+0xdc, and level_page_in_textures
ORs bit 1 into the byte at 0xa5a2a0 itself (0x46ff38-0x46ff46). New `struct s3d_level` in the new header
engine/s3d/s3d.h (Engine\s3d\s3d.cpp, path literal 0x59ad10). levelrender.h's `extern int num_lobjs`,
`extern lobj *lobjs` and `extern int *level_texture_bms` become macros on its fields, as D404 did for
pxo_cfg_flags (`#define num_lobjs (level_s3d.num_meshes)`, and likewise num_doors, doors,
level_num_navpoints, level_navpoints, level_num_vis_records, level_vis_records, level_num_textures,
level_face_bitmaps), so gamesound.cpp, new_hud.cpp, new_tutorial.cpp and objects.cpp compile unchanged. The
field names differ from the macro names on purpose. levelrender.h includes s3d.h. `extern bool
S3d_load_lightmaps` (0x59aa58, added to levelrender.h by decomp26) moves to s3d.h: the byte is the first
.data of s3d.cpp, right before the unit's literals ("level" 0x59aa5c).

**D801 (levelrender.h). lobj_face +4 is a 24-bit lightmap index and a signed byte (bit fields).**
s3d_read_mesh_ref writes the pair with `sar ecx, 0x18` and `shl eax, 8 / sar eax, 8` (0x470056-0x470062)
and stores it back as `(hi & 0xff) << 24 | (lo & 0xffffff)` (0x4700a3-0x4700c3); s3d_read_meshes tests the
low part's sign with `shl ecx, 8 / js` (0x46ed4c) and sets it to -1 with `or edi, 0xffffff` (0x46f0ab,
0x46f0dc). So `int lightmap : 24; int _unk7 : 8;` in place of `int _unk4` (no pilot used it). The low
part indexes level_face_bitmaps (level +0x7c, the lightmap bitmaps of s3d_load_lightmap).

**D802 (levelrender.h). door_info fields from s3d_read_door.** 0x470220-0x4702e1 reads +0x00 int (the
level object), +0x04 / +0x06 int16 (+0x04 indexes the named paths: level_load_s3d 0x46e783-0x46e796),
+0x08 float, +0x0c vector, +0x18 quaternion, +0x28 vector, +0x34 quaternion, then invalidates the
GameTimer at +0x44 and resets +0x48..+0x76. +0x44 is a real GameTimer member: `new door_info[n]` in
s3d_alloc_level_arrays constructs it per element (0x46fbcb-0x46fbdf, EH state 2). `_unk10[6]` becomes
closed_pos (+0x0c) / closed_orient (+0x18); door_update's +0x10 and +0x1c reads fall inside them. open_pos,
open_orient and lock keep their offsets and names.

**D803 (levelrender.h). lobj has a constructor.** lobj_clear_layers (0x4775e0) sets the dword +0x6c to -1
and returns this; `new lobj[n]` in s3d_alloc_level_arrays calls it for every element inside EH state 1
(0x46fa77-0x46fa85), the pattern of an out-of-line constructor. Declared `lobj();`. Also from
s3d_read_meshes: +0x04 is the s3d_geometry (was void *mesh, same name), +0x10 the lightmap boxes (was
_unk10), +0x68 the per-vertex RGB bytes and +0x6c the four layer bytes (was _pad68[8]);
get_layer_n / set_layer_n (0x4775f0 / 0x4776a0) added.

**D804 (file.h). File::close returns int.** 0x514740 ends with `mov eax, [esi+0xc]` (the slot error,
0x5147af) and level_load_s3d returns that value (0x46e764 `call File__close / mov ebp, eax`, 0x46e8e8).
Was `void close()`; the other callers ignore the result.

**D805 (vfx.h). attached_light's constructor is inline.** levelrender.cpp's initialiser for the 16 records
at 0x23df188 (0x48db40) loops over attached_light__reset (0x520210) with no constructor call, while
init_attached_lights (0x51fb10) calls the out-of-line attached_light__ctor: the D500 pattern. New class
attached_light in vsdk/geom/vfx.h with `attached_light() { reset(); }`; only the type byte (+0x00, -1 free)
is named.

**D806 (matrix.h). quaternion(const matrix *) is 0x522790.** quaternion__set_from_matrix_b returns this
(0x52292a, ret 4) like the constructor 0x522610, and s3d_read_navpoint builds a temporary with it and then
copies it with the inline operator= (0x46f343-0x46f374) before File::write_quaternion. Declared as a second
constructor taking the matrix by pointer (the reference one is 0x522610's).

**D900 (boss_ai.cpp, decomp29). ai_state +0x34 is a float.** `int _unk34` becomes `float attack_range`:
boss_get_move_range (0x4a26f0) copies a boss move record's second $Range: float there (`mov eax, [ecx];
mov [esi+0x34], eax` after `*max = m->range_max`), and the Titus / Pyrul cases store an x87 result
(`fstp dword ptr [esi + 0x34]` at 0x4a27c3 / 0x4a27d3). No existing .cpp read `_unk34`.

**D901 (boss_ai.cpp). char_info +0x188 is a short count.** `ubyte _unk188[0x12]` becomes
`short num_target_points; ubyte _unk18a[0x10];`: character_target_point_count (0x4a21b0) tests
`cmp word ptr [eax + 0x188], 0 / jle` and returns `movsx eax, word ptr [eax + 0x188]`. Offsets and
size unchanged (LAYOUT_CHECK 0x3d0).

**D902 (boss_ai.cpp). The per-boss AI functions are ai_state members.** boss_ai_check_stage loads ECX with
the AI record before every per-boss call (`mov ecx, esi; call 0x49a6c0 tiger_check_stage`, 0x4a075f) and
the load / idle / frame dispatchers tail-jump with ECX unchanged; the callees read it (`mov esi, ecx`,
`mov eax, [ecx + 0x4c]` at 0x49ad27, 0x4c56cc, 0x49b041, ...). So they are declared in ai_state (ai.h)
next to the dispatchers boss_ai_load / _unload / _check_stage / _idle_dispatch / _frame,
boss_threat_decay, boss_turn_rate_override, boss_get_attack_move_id and boss_get_move_range (all
thiscall: sum.symbols "thiscall on ai"; boss_get_move_range ends `ret 8`). The unload callees never read
ECX (luminar_free_bitmap, tiger_unload, ...: no ECX use in their first instructions) and
ghost_revive_minions is called without loading ECX (0x4a079f), so those are free functions in boss_ai.h.
luminar_ai_idle and tentacle_beast_ai_idle do not read ECX either; they stay members for uniformity
(the tail jump is the same either way).

**D903 (boss_ai.cpp). Boss move records.** New `struct boss_attack` (0x20 bytes) from boss_ai_parse_table's
stores (+0 offense point, +4 / +8 $Range:, +0xc cos($Max angle:), +0x10 = 1, +0x14 +Spell:, +0x18
$Variation:, +0x1c flags 1 Trip, 2 Push, 4 Projectile, 8 Spell Only, 0x10 Plus Spell). The bosses'
current-move pointers are `boss_attack *`, and so is serpent_part_busy (sum.symbols: int[3]): the
dispatchers dereference it (`test byte ptr [eax + 0x1c], 0x8` at 0x4a0b9a, `mov eax, [edi + 0x14]` at
0x4a0cf2, `cmp dword ptr [eax + 0x18], 0xbb` at 0x4a1cd1). serpent_part_move (0x2452390) stays int[3]: it is
returned without a dereference (0x4a20f5). The table argument is `boss_attack table[][10]`:
the record pointer is `table + (stage * 5 - 5) << 6` = (stage - 1) * 0x140, and serpent_attack_table
(0x2451ed8) + 3 * 0x140 is serpent_timer_attack (0x2452298), so the serpent's table is [3][10].

**D904 (boss_ai.cpp). boss_threat_add argument order.** sum.symbols says "(boss, attacker, amount, melee)".
The body reads the handle from the third argument (`mov ebp, [esp + 0x10]` after one push, then
living_from_handle(ebp)), the amount from the second (`fld dword ptr [esp + 0x10]` after two pushes) and
the flag from the fourth; character_add_hp pushes (a, source, delta, boss) (0x4509ff-0x450a0e). Declared
`boss_threat_add(Character *boss, float amount, int attacker, int flag)`. boss_threat (types.h) gets a
GameTimer at +0x14 (GameTimer::SetTimeout on `lea ecx, [esi + 0xc]` with esi = record + 8).

**D905 (boss_ai.cpp). boss_counter_hit_dispatch's arguments.** The decomp23 header names (attacker,
defender) are right: the function tests the first argument for type 9 and the Ghost / Serpent species
and casts the move's spell on the second (`mov ecx, [esp + 0x28]` = argument 2 before
living_entity_get_hit_by_spell, source = argument 1's handle). sum.symbols' "(boss, attacker)" and
bosses.md's "hits the melee attacker" read it the other way round: the boss's own hit carries its
+Plus Spell: to the defender. Only the comment changed.

**D906 (boss_ai.cpp). fx_create_spell_effect_at.** Declared in fx.h as (spell, caster, owner, ms, pos,
orient): the body stores argument 2 at +0xb0, argument 3 (resolved with living_from_handle, default
position +0x10 and orientation +0x38) at +0xb4 and passes argument 4 to GameTimer::SetTimeout on +0xdc
(0x48cb10-0x48cb31); arguments 5 / 6 are the pos / orient overrides. sum.symbols lists the time last.

**D907 (boss_ai.cpp). Folded per-boss functions.** Several dispatcher cases call or jump to
empty_function (0x4f13e0) or ls_test_use (0x4f29c0, `xor al, al; ret`). In the source these are separate
per-boss functions merged by identical-code folding at link time, so they are declared with names of
ours (titus_unload, murod_oni_check_stage, evil_joseph_check_stage_b, giant_salamanka_ai_idle,
murod_oni_fire_attack, murod_oni_cast_special, the four *_script_hook) and no address. Their calling
conventions follow the call sites (ECX loaded = ai_state member; pushed Character * = cdecl).

**D908 (boss_ai.cpp). Jump-table bounds.** Four switches have a byte table wider than their real cases:
boss_cast_special_dispatch (0x51..0xc9, lowest real case 0x58), boss_attack_stuns_dispatch and
boss_attack_knocks_down_dispatch (0x51..0xc9, cases 0xac / 0xb8), boss_stage_attack_enabled_dispatch
(0x51..0xc5, case 0xac). The bounding species are written as explicit cases that share the default
body; which other species the original listed cannot be told (they all map to the default).

**D909 (boss_ai.cpp). Other additions.** living_entity.h: Character::get_mount (0x462d20), get_rider
(0x462d40), get_698 (0x456f80), `character_size_heights` (0x5818d8, .rdata, name ours) and
`character_size_radii` (0x59d7c8). characterinfo.h: num_char_infos (0x714e54). ai.h:
ai_state::add_move_to_timed (0x474190) and ai_score_target (0x4b53c0) with its 0x14-byte query record
(ai_target_query, names ours). vector.h: vector2::len_sq (0x523fa0, out of line: it is in the vector unit
0x523f80-0x524930). pathfinding.h: pf_point_lists[3][20] (0x245f460, 0x60-byte records, index
size * 20 + layer; three sizes fit before 0x2460ae0; names ours). effect_mem.h: effects_load_preload_rfx
(0x51dbb0). spells.h: SPELL_NIGHTMARE 0x48, SPELL_GHOST_DEATH 0x54 (spells.tbl records 72 and 84).
boss_ai.h: the boss species constants, boss_attack, boss_threat, boss_target (0x10-byte candidate
record of boss_target_list_build), every boss_ai.cpp function and the per-boss functions and data the
unit calls. tentacle_get_attack_kind takes the Character (both dispatchers push `[esi]`, the owner),
not a handle as sum.symbols says.

## level_scripts.cpp (decomp27)

**D700 (savegame.h). The savegame object's layout.** `savegame` was a method list without fields ("layout
not needed yet"). It is now the 0x10e8-byte object from the layout functions and the size reports:
+0 buffer, +4 reserved size (savegame_init_buffer 0x4cfbb0), +8 used (every layout function adds to it),
then {data, size} pairs: header +0xc (0x4d1802 / 0x4d180d), thumbnail +0x14 (0x4d1847), visit counts +0x1c
(0x4d1871), 51 level blocks of 0x18 bytes at +0x20 {data, num_flags, num_bits, num_ints, num_floats, size}
(savegame_layout_level_data 0x4d1889-0x4d18bd; savegame_set_bit's message calls them
`data_pointers[%d(%s)].size`), the conversation base +0x4e8 and 51 x 7 {data, size} at +0x4ec (0x4d18fc,
0x4d191a), then items +0x1014 .. hot buttons +0x105c (0x4d19ed .. 0x4d1c1f), the pending load name +0x1064
(0x80, savegame_set_pending_load 0x4d387e) and slot +0x10e4. num_flags / num_bits are unsigned
(`shr edi, 3` at 0x4d18b5, `jae` at 0x4cff4a). The members that would clash with the globals level_items
(0x24e3b50) and automap_labels (0x5f9838) are named level_item_hashes and map_labels (ours). New types:
savegame_section, savegame_level_data, savegame_party_state (the LENT dword as bit fields: masks
0x4cf522-0x4cf565 and 0x4d2e3c / 0x4d3029 / 0x4d2ee9), savegame_hot_button (6 bytes, `#pragma pack(1)`),
savegame_automap_label (0x34), savegame_two_stage (0x673f: counts, 51 x 0x79 hashes, bits at +0x6138).
savegame() is the constructor 0x4cfb80 (sum.symbols savegame_state__clear: init_savegame_state jumps to it
with ECX = the object); the destructor is empty and inline (atexit 0x4ceef0 jumps to empty_function).

**D701 (savegame.h). bool values.** set_flag's third argument is tested as a byte (`mov cl, [esp+0x1c];
test cl, cl` at 0x4cfdb4), so `bool value` (was int; the callers pass 1). set_bit and set_speaker_bit /
set_topic_bit likewise (0x4cfff7, 0x4d07bd); set_quest_bit compares with true (`cmp dl, 1` at 0x4d0c63).

**D702 (savegame.h). Unused arguments, return types.** write (0x4d1c40) never reads its second argument
and read (0x4d25b0, ret 8) its second (load_pending passes the pending slot). read_thumbnail_file returns
int (EAX 0 / 1 at 0x4d29c0 / 0x4d29c9; was void, the return is still unused). The other members were
added from their definitions; the free functions savegame_refresh_object_flags / _character_flags are
cdecl (savegame_set_flag calls them with ECX not set, 0x4cfdd2).

**D703 (level_scripts.h). level_script_info is the initialised table of level_scripts.cpp.**
level_script_table (0x59f128) lies in this unit's .data (between its literals 0x59e798-0x59f11c and
level_table); the image holds the name, the five hooks, +0x14, the load steps and the music name. +0x18 is
`void (*)(dialogue_action *)` (conversation_execute_action pushes the action and calls it, 0x4d59bf; was
void *). The hash lists are written by events_parse_table (`mov [eax + 4*edx], edi` at 0x4d3cf0), so
`unsigned int *` (was const). empty_function in the action slot is cast (identical-code folding: the
original had per-level empty functions).

**D704 (level_scripts.h). level_script_use returns char.** It returns the hook's AL or `xor al, al`
(0x4d5e34); was int (new_skill.cpp ignores the result).

**D705 (level_scripts.h). level_items and the other .bss of 0x2493a18-0x25443af are level_scripts.cpp's.**
sum.symbols tags level_items (0x24e3b50) items.cpp, but items.cpp's .bss is 0x8b9ea0-0x8ec9.. and the
address sits between this unit's Preparse_scripts (0x24e3b48) and navpoints (0x24e4950). Defined in
level_scripts.cpp with conversation_save_table, quest_table / quest_count (sum.symbols: new_journal.cpp,
doubtful), level_entered_by_load (summoner.h keeps the extern), script_objects and the three StringPools.
level_item_rec (0x70) is read from level_items_parse_table (0x4cb524-0x4cb65e).

**D706 (levelrender.h). door_info +0x54.** decomp28 (D802) named it `state`; bit 3 is the door's default
locked state (savegame_two_stage_get, `shr eax, 3; and al, 1` at 0x4d1287). Comment only. num_doors /
doors are decomp28's macros; level_scripts.cpp includes engine/s3d/s3d.h.

**D707 (ai.h). ai_group_create returns the group.** savegame_read_groups keeps EAX (0x4cfa93) and passes it
to ai_group_add_member and stores +0x4c / sets the +0x50 timer through it. Was void (player.cpp ignores
it). New: ai_groups[10] (0x243e040), ai_groups_reset (0x492b30), ai_goal_next_id (0x59b7c8).

**D708 (conversation.h). dialogue_action fields.** conversation_execute_action reads +0 as the topic / flag
name, +4 as a flag name, +0xc as the amount / item type / flag level, +0x10 as a species (-1 = the party).
Added as anonymous-union aliases (name, flag, value, species) so conversationctl.cpp's `_unk` spellings
still compile.

**D709 (savegame.h, level_scripts.h). conversation_save_table records.** The table stays `void *[357 * 4]`
(new_mainmenu.cpp, new_saveload.cpp, conversationctl.cpp index it); level_scripts.cpp reaches the 0x10-byte
records {name, speakers, topics, level cap} through `CONVERSATION_SAVE_INFO(level, slot)` (struct
conversation_save_info; +0xc is +Levelcap:, 100 by default, 0x4d6286 / 0x4d628e).

**D710 (level_scripts.h). level_add_navpoint returns bool.** decomp28 declared `void
level_add_navpoint(s3d_navpoint *)`; the function ends `mov al, 1` (0x4d58c9). The argument type is kept
(s3d_navpoint: name +0, orient +4, pos +0x28 match the reads at 0x4d5740, 0x4d579e, 0x4d5774).

**D711 (decomp27). Other additions, no conflict.** Character: save_state 0x457150, load_state 0x458800,
set_ai_active 0x462200, add_xp 0x44fab0; character_save_state_size 0x4587f0; CF_SUMMON 0x100. camera:
get_state / set_state (9 dwords), reset_default, set_mode, snap_to_target, sync_yaw_to_target. objects.h:
containers[40] (0x2302fe0). levelrender.h: lobj::get_layer_n (decomp28 had it too), door_find_by_name
(lobj ** / door_info ** outputs, 0x4772d9 / 0x477300), door_set_open, door_set_closed, door_toggle_state,
lobj_from_index. fx.h: fx_create_world(name, flag). snd.h: sound_emitter_pause / _unpause. summoner.h:
level_time (0x60a654), party_member_join. vector.h: vector3_zero (0x2cbf4b8). new_saveload.h:
saveload_thumbnail_pixels. level_scripts.h: script_object (0x550, GameTimer at +0x518), navpoint (0x30;
+0x10 is a heading, not sum.symbols' handle: extract_angles h is stored there, 0x4d57ac), level_effect,
level_ambient, the level-script hook globals of level_scripts_common.cpp, the 138 ls_* hooks, and the
unit's functions and globals.

## bmpman.cpp (decomp31)

**D1100 (bmpman.cpp). bm_lock's third parameter is the palette.** bmpman.h had `bm_lock(bm, ubyte **data,
int *format)` returning the type. The body (0x5036ac-0x5036c2) stores the entry's +0x20 (data) through the
second argument, +0x28 (palette) through the third and returns +0x18 (format); the failure paths return 0.
Now `int bm_lock(int bm, ubyte **data, ubyte **palette)` returning the format. ui_container::set_mask_bitmap
passes 0x25c2ff8 as the third argument, so `bm_lock_format_unused` (name kept for ui_container.cpp) becomes
`ubyte *` and is defined in bmpman.cpp (first dword after bm_bitmaps, 0x2591c78 + 0xaf0 * 0x48).

**D1101 (bmpman.cpp). The m2v readers take one more argument.** bm_read_header pushes four arguments to
m2v_read_header (`push 0; push h; push w; push name`, 0x502d63-0x502d67, `add esp, 0x10`) and bm_lock_m2v
three to m2v_read_bitmap (`push 0` at 0x503b24, `add esp, 0xc`). Neither body reads the extra stack slot
(both open with PATH_ANY), so targautils.h declares a trailing `int unused`. targautils.cpp's definitions
still have the old parameter lists (they compile as separate overloads) and should get the parameter when
merged.

**D1102 (bmpman.cpp). bm_get_total_size has two parameters.** sum.symbols gives (handle). The body
(0x503ba0) passes the address of its own first argument slot as num_pixels to bm_get_mipmap_info and then
multiplies num_pixels * frames by `[esp+0x1c]` after the cleanup, which is the second stack argument. Declared
`int bm_get_total_size(int bm, int bytes_per_pixel)` (name of the second parameter ours: the product is
the byte size of all frames and levels at that depth). No caller in Sum.exe v1.40.

**D1103 (bmpman.cpp). milliseconds_unpaused is read as unsigned.** bm_get_current_frame converts it with
`mov [esp+8], edx; mov [esp+0xc], 0; fild qword` (0x50258a-0x50259c), VC6's unsigned-to-float sequence.
frametime.h keeps `int` (frametime.cpp's definition); bmpman.cpp casts to unsigned int.

**D1104 (bmpman.cpp). Other header changes.** bmpman.h: the BM_TYPE_* / BM_ANIM_* / BM_FORMAT_* and
size constants (format names after Dash Faction's bm::Format, numbers from bm_bytes_per_pixel's jump
table 0x50247c and bm_has_alpha), every bmpman.cpp function and global; bm_get_num_frames' out parameter
is the whole animation's length (num_frames / frames_per_ms * 0.001), renamed anim_secs. gr.h:
gr_tcache_free (0x508020). Kept as declared although the call sites differ: tga_read_bitmap's fourth
argument is a bool in bm_lock_tga (pushed as a dword loaded from a byte slot, 0x503880) and
tga_read_header's sixth is 1 for the mip probes (0x502c6b), both left as targautils.h's int / void *.

## pathfinding.cpp (decomp32)

**D1200 (pathfinding.h). The graph types; pf_bounds is one rect2d.** decomp29's pf_point_list /
pf_point_ref (boss_mount_pick_flee_point) are the .pfg graph (0x60, pf_graphs[3][20] at 0x245f460) and
its visibility node (0x28): renamed pf_graph / pf_vis_node with typedefs and `#define pf_point_lists
pf_graphs`, keeping decomp29's field names count / points / pos so boss_ai.cpp compiles unchanged. The
other fields come from pathfinding_load_graph (0x49d2c5-0x49d573: +0x04 / +0x14 second node list, +0x0c /
+0x1c BSP, +0x20 cells, +0x24 the wall grid read by pf_read_region) and pf_graph_search (+0x18 open list,
+0x1c parent, +0x20 g, +0x24 f). The four floats 0x245c998..0x245c9a4 (sum.symbols pf_bounds_max_x,
_min_x, _min_z, _max_z) are written as one 16-byte copy of a cell's bounds in pf_compute_graph_bounds
(`lea edx, [ecx + 0xc]` and four dword moves, 0x49cb7b-0x49cb9a), so they are `rect2d pf_bounds` with the
four symbol names as macros on its fields. The symbols swap x and z: mp_quantize_position scales the
position's x by 0x245c9a0 / 0x245c9a4 and its z by 0x245c99c / 0x245c998 (0x475660-0x4756bc), and
pf_find_nearest_navigable_point compares cell +0x14 / +0x18 with pos.x and +0x0c / +0x10 with pos.z
(0x49e972-0x49e9bd). rect2d's fields are named by that use (y = world z); the macros keep the symbols'
names (fixing sum.symbols is left open).

**D1201 (pathfinding.h). pf_straight_walk_clear(a, a_layer, b, b_layer, flags).** sum.symbols says
"(from, to, size, layer)". ai_path_smooth pushes (7, next->layer, &to, start_layer, &from)
(0x49f6bd-0x49f6ce), where start_layer is ai_path_find's second argument (`mov ebp, [esp+0x3c]`
0x4a0209, passed on at 0x4a022e), and its backward pass pushes (7, nodes[0]->layer, &from,
nodes[i]->layer, &to) (0x49f7ac-0x49f7bf). The same order in pf_graph_search (0x49fc3d-0x49fc4b) and
pf_mark_nodes_visible_from_point (0x49f240-0x49f253).

**D1202 (pathfinding.h). ai_path_find(from, from_layer, to, to_layer, size, float cost_bias), `to`
not const.** The sixth argument is passed through to pf_graph_search, which loads it with `fld dword
[esp+0x274] / fchs` into the goal node's cost (0x49fc09-0x49fc12): a float (conversationctl's literal 0
has the same bits). pf_graph_search writes to->x / to->z when the goal is outside every cell
(0x49fb2a-0x49fb2c), so `to` loses its const in both.

**D1203 (pathfinding.h). Which unit defines what.** pathfinding.cpp's .bss is 0x2452cb8-0x245c9df
(pf_layer_links .. the search stamp 0x245c9dc; boss_ai.cpp's timer is 0x245c9e0). The graph tables at
0x245f3a0-0x2460cbb (sum.symbols tags several of them pathfinding.cpp) lie after the obstacle-debug unit's
.bss and are the geometry unit's (docs/sum-exe/source-map.md, 0x4a4ee0 row), so they are extern in
pathfinding.h and not defined in pathfinding.cpp. 0x245cd8c (freed by pathfinding_atexit_free, not
referenced elsewhere) and the .data int 0x59cd8c (compared with 7 before the search hook) belong to other
units; both are declared with names of ours.

**D1204 (pathfinding.h). ai_path is 0x28 and the pool holds 1000.** ai_path_pool_init threads
0x2452d58..0x245c998 in 0x28 steps (0x49d932-0x49d944) = 1000 nodes, and ai_path_alloc_node prints
"current max is %d" with 0x3e8 (0x4a02d9); sum.symbols' "500 path nodes" is the initial value of
ai_path_peak_used (0x1f4, 0x49d956). Fields: +0x00 pos, +0x0c layer (pf_graph_search passes the node's
layer word, 0x4a0180), +0x10 dir (ai_path_find), +0x1c cost, +0x20 flags (bit 1 tested by
ai_path_smooth), +0x24 next. ai_path_node_create takes the point as a vector2 by value (x to +0, y to +8;
the caller pushes the node's two floats, 0x4a0184-0x4a018e), ai_path_node_create_b a vector3.

**D1205 (ai.h). Avoidance records.** pathfinding.cpp reads the avoidance unit's tables
(0x4a6710-0x4aa760, file not known): the 14 segment lists 0x2460db0 (flags +4, corner count +0xe, size
+0x10, layer +0x14, corners of 0x44 from +0x288 whose first 0x28 bytes are used as pf_vis_node: point,
flags, links, A* parent +0x1c; 0x49fd3e, 0x49ff3b-0x49ff6a), the obstacles 0x246a080 (0x88: flags,
point count, size, layer words, outline from +0x38; 0x49e227-0x49e296), the polygons (avoid_poly_get,
count +2, obstacle bytes +8, bounds +0x104), the splines and their points (wall record at point +4,
search stamp at +0x10, 0x49e396-0x49e3a6). Declared in ai.h next to avoid_area_add, which now includes
pathfinding.h for rect2d / pf_edge / pf_vis_node. avoid_prim's 0x78 bytes are what fits in
pf_add_avoidance_nodes' frame (unsure).

**D1206 (vector.h, file.h, levelrender.h). Other declarations.** vector2::negate 0x524000,
to_vector3 0x4a4ec0 (hidden result), from_xz 0x4a4ee0 (`ret 4`, no this returned, so a member, not a
constructor), static nearly_equal 0x4a4e60 (cdecl, two pointers); vector3::length_quick 0x5240e0;
vector2_closest_point_on_segment 0x525410 is (p, a, b, out) returning the squared distance, not
sum.symbols' "(out, p, a, b)": the body forms (b - a) . (p - a) with its first argument as p and writes
the fourth (0x525413-0x5254b0), and pf_sweep_test_wall passes (&corner copy, &from, &to, &tmp)
(0x49de5a-0x49de66). vector2_project_onto_segment 0x5254d0 takes five arguments (a, b, p, margin, out t;
`add esp, 0x14` at 0x49df08). File::read_uint16(min_ver, def) 0x525940 (an overload of FileReader's
name only, File had none) and File::read_bool 0x525800; door_is_open 0x476940 (door, fraction) -> AL.
pf_cells_along_segment is (layer, size, a, b): it indexes 0x245f460 with arg0 + 20 * arg1
(0x4a64b0-0x4a64c4), the reverse of sum.symbols' "(size class, layer, ...)".

**D1300 (gr_opengl.cpp, decomp33). gr::Mode is declared.** gr.h gets `class gr::Mode` (one gr_mode dword,
`value`) with the sum.symbols members as header inlines: default constructor = gr__Mode__Mode_invalid
(0x5368a0, value -1; it is what init_gr_opengl_current_mode 0x54efc0 calls on the global), Mode(gr_mode)
= gr__Mode__Mode_value (0x506810, thiscall, ret 4), the six-field constructor 0x508160 (ret 0x18, fog
in bits 25-29 down to the texture source in bits 0-4), operator== 0x5368b0 (`cmp` of the dwords, ret 4),
the six get_* accessors 0x5368c0-0x536910, invalidate 0x536920 and is_multitexture 0x536930. Inline
because every out-of-line copy is a COMDAT in another object (the accessors among the Direct3D code,
module "vsdk\gr" in sum.symbols) and the vsdk units call them, as with vector3 (D500). gr_opengl.cpp
passes it by value: gr_opengl_set_mode takes `lea ecx, [esp + 0x1c]` (its own argument slot) as `this`
for operator== and the getters (0x54efec-0x54f049), and gr_opengl_clear builds a temporary with
0x508160 and pushes its dword (0x54f7ed-0x54f7f8). D7's `typedef unsigned int gr_mode` stays for the rest
of the tree (gr.cpp's front end and all pilots pass the dword); Mode(gr_mode) converts implicitly. Only
the gr_opengl_* functions are declared with gr::Mode.

**D1301 (gr_opengl.cpp). The bound textures are one int[2].** sum.symbols has gr_opengl_bound_texture0
(0x5ad7c8) and gr_opengl_bound_texture1 (0x5ad7cc); gr_opengl_tcache_set indexes them by the stage
(`mov dword ptr [4*eax + 0x5ad7c8], -1` at 0x54eda2, `mov eax, [4*ebx + 0x5ad7c8]` at 0x54edcc, the store
at 0x54ee99). Defined as `int gr_opengl_bound_texture[2] = { -1, -1 }` (name ours, from the pair); the
other functions use [0] / [1]. The -1 at 0x5ad7d0 has no reference and is a separate global.

**D1302 (gr_opengl.cpp). gr_color_gamma_lut is float[256].** sum.symbols types 0x25d88d0 uint32[256].
gr_set_gamma clamps `fild i` against two float constants and stores with `fstp dword ptr [ecx]`
(0x506b1f-0x506b4f), and gr_opengl_tmapper reads it with `fld dword ptr [4*edx + 0x25d88d0]` + _ftol
(0x550150). Declared `extern float gr_color_gamma_lut[256]` in gr.h.

**D1303 (gr_opengl.cpp). gl_convert_1555_to_rgba8888_scaled decodes 565.** The name is kept, but the
body extracts (p >> 5) & 0x3f scaled by the /63 magic 0x82082083 (0x54dca3-0x54dcb9), p & 0x1f and
p >> 11 by the /31 magic, and ORs a constant 0xff alpha (`or dh, 0xff` at 0x54dce9): an opaque
565 -> RGBA converter. No caller.

**D1304 (gr_opengl.cpp). gr_vertex_depth takes a gr_vertex.** sum.symbols: (vertex*), reading only z at
+8; gr_opengl_tmapper / line_3d pass gr_vertex pointers. gr_opengl_set_fog passes a local vector3
(`lea ecx, [esp + 0x4]; push ecx; call 0x50d8b0` at 0x55042f, after vector3::vector3(0, 0, z)), written
as a cast in the source. gr_get_aspect (0x506950), gr_get_view_fov (0x50e5a0: two float* outputs, called
with (&h, &v) and the second one used as gluPerspective's fovy) and gr_scale_by_far_clip (0x50ecb0)
are added to gr.h with gr_color, gr_texture, gr_texture_2, gr_fog_enabled / color / near / far,
gr_eye_pos, gr_eye_matrix and gr_far_clip_dist (sum.symbols names and addresses).

**D1305 (gr_opengl.cpp). GL linkage.** No system GL header: vsdk/gr/opengl/gl_min.h declares the
OPENGL32.dll functions `extern "C" __declspec(dllimport) APIENTRY` (every call in the unit is
`call dword ptr [0x57e2xx]`, stdcall) and gluLookAt / gluPerspective without dllimport (they are called
through the linker thunks 0x57b5ca / 0x57b5c4, `call 0x57b5ca` at 0x550586). The wgl* functions and the
pixel-format calls come from <windows.h>. The GL_ARB_multitexture pointers are `void (APIENTRY *)(GLenum)`
(no `add esp` after `call dword ptr [0x2f13344]`).

## level_script_khosani_lab.cpp / level_script_worldmap.cpp (decomp34)

**D1400 (scheme). The per-level script units have no header of their own.** The level_script_table hooks
of every level are already in `levelscripts/level_scripts.h` (the gathering header, decomp27), and nothing
outside the two units calls their helpers or reads their globals (`tools/asm.mjs grep` over every
reference to 0x257eb10-0x257ecf9 and 0x257ed08-0x257ed5b and the helper addresses: only callers inside the
unit). So each `.cpp` declares its own helpers at the top (not `static`: the binary cannot tell) and keeps
its unit-local types (`ls_khosanilab_cell`). sum.symbols says ls_khosanilab_timer is also used by
ls_ionaext_action and ls_worldmap1_encounter_timer by ls_lenele1b_action; neither reads them (the only
references are in their own units, 0x4f77d0-0x4f9d15 and 0x4fb500-0x4fbef5).

**D1401 (levelscripts/script_internal.h, new). navpoint_get_position.** sum.symbols puts 0x4dc600 in
script_internal.cpp; one small header for it (directory guessed, as the other levelscripts units). It
returns bool (`mov al, 1` at 0x4dc6d5, `xor al, al` at 0x4dc6db); (name, vector3 *, matrix *) from the
caller ls_khosanilab_puzzle_use (pos at its local +0, the 0x24-byte matrix at +0xc, 0x4f9ba1-0x4f9bab).

**D1402 (engine/ai/ai.h). avoid_spline_def.** sum.symbols types ls_khosanilab_path (0x257ebc0) uint8[0xc4]
"(count, kinds, points at +0x7c)". avoid_spline_create reads +0 as the count and per point i
`avoid_spline_point_create(&def[0x44 + 8i], &def[0x84 + 8i], def[4 + 4i], def[0x24 + 4i])` (0x4a79bb-0x4a79cd),
and ls_khosanilab_puzzle_add_point writes the same offsets (1 to +4 + 4i, 2 to +0x24 + 4i, the two
points to +0x44 / +0x84; 0x4f885f-0x4f8892): so `{count, cell_x[8], cell_y[8], vector2 p0[8], p1[8]}`, 0xc4.
There is nothing at +0x7c. ai.h now includes vsdk/math/vector.h for vector2. avoid_spline_create returns
the spline index or -1 (0x4a79a3, 0x4a7a09-0x4a7a21), avoid_spline_free takes the index.

**D1403 (engine/rendering/levelrender.h). The level-script door helpers.** door_apply_saved_state's
second argument is a byte (`mov cl, [esp+0x1c]` compared with the saved state byte at 0x477397-0x47739f),
so `bool open`; callers pass the bool flipper state unconverted (0x4f9ce7). door_is_open_by_name returns
door_is_open's AL (0x47743b) or `xor al, al` (0x477424); its second argument is a float fraction (callers
push 0x3c23d70a / 0x3a83126f). lobjs_hide_by_prefix / lobjs_show_by_prefix take the prefix only.

**D1404 (engine/objects/living_entity.h). Character::place_at_party_start, set_speed_scale.** Both
thiscall: `mov ecx, esi; call 0x44f1a0` with one pushed slot (0x4f9c15), `push 1; push <float>; call
0x462260` (0x4fb64f, ret 8 per sum.symbols). The flag is pushed as 0 / 1, typed bool.

**D1405 (summoner/summoner.h). level_player_starts, level_entry_point, GF_CUTSCENE.** sum.symbols types
level_player_starts (0x60a32c) void*[4]. ls_worldmap1_init stores the address of its own 0x30-byte record
there (0x4fb5db-0x4fb5ef) whose +0x10 / +0x14 / +0x20 are written from the saved heading, position and
plane, the navpoint layout (level_scripts.h, D711), and character_place_at_party_start reads +0x14 / +0x10 /
+0x20: so `navpoint *[4]` (forward-declared in summoner.h, next to the other level globals 0x60a2c8-0x60a328).
GF_CUTSCENE is game_flags bit 0x20 (sum.symbols "cutscene running"; `test byte ptr [0x5fbfd8], 0x20` at
0x4f93c9 / 0x4f94fe before the Salamanka music changes). Name ours.

**D1406 (levelscripts/level_scripts.h). level_scripts_common.cpp helpers.** New declarations, no conflict:
level_find_player_starts, script_play_cutscene (name, continuation), level_trigger_set_exit /
level_trigger_set_script_use (name, script; the script is compared only when not NULL, 0x4ccfbd),
script_object_say (index, text, 5.0f), script_npc_set_disabled, script_npc_send_go,
script_player_near_navpoint (navpoint, distance; the callers test AL), script_npc_is_dead (AL from
script_object_is_dead, `xor al, al` at 0x4ce61b), script_party_gain_xp.

**D1407 (engine/ai/boss_ai.h, engine/ai/ai.h). Giant Salamanka flags, Ai_paused.** The five byte flags
0x248c7d0 / 0x248c7dc-0x248c7df (sum.symbols names and bool types; read with `mov al, byte ptr` at 0x4f9328
/ 0x4f9374, written with `mov byte ptr` at 0x4f952b / 0x4f9560 / 0x4f9570) and Ai_paused (0x22e499c, a
console toggle; `mov al, byte ptr [0x22e499c]` at 0x4fbf1b). Declarations only.

## gametarget.cpp / multi_chattracker.cpp / multispell.cpp (decomp35)

**D1500 (spells.h). spell_cast_request +0x14 is the casting item.** mp_spell_cast_sync sends
`(+0x14 - item_types) >> 8` or -1 (0x4b0391-0x4b03ae: `sub eax, 0x8cbec0 / sar eax, 8`), and
mp_handle_release_spell rebuilds `item_types + (index << 8)` (0x4b057c-0x4b0585). The field is now an
anonymous union {int _unk14; ItemType *item;}: combat.cpp and the inline constructor keep clearing _unk14.

**D1501 (spells.h). spell_cast_params decoded (was int data[13]).** The first word is sent as three
values: bits 1-6 sign-extended (`shl 0x19 / sar 0x1a`, 0x4b03e3 / 0x4afd6b), bits 7-28 (`shr 7 / and
0x3fffff`) and bits 29-31 (`shr 0x1d`); the reader packs them back over the old bit 0 (0x4b05b7-0x4b05ed:
`and edx, 1 / or eax, edx`). The target mode (bits 1-6) selects the words +0x2c.. through the jump tables
0x4aff80 / 0x4b0100 (modes 1..10). Fields: flag0 : 1, int mode : 6, value : 22, kind : 3, int target[12];
LAYOUT_CHECK 0x34. Nothing used data[]. The same first-word split as spell_effect (bits 1-6) suggests one
type; not merged.

**D1502 (spells.h). summon_dismiss takes a handle.** All five callers push one dword and clean it
(mp_game_frame 0x441365, spell_cast_execute 0x47cddc, spell_cast_launch_effects 0x47de9a,
mp_handle_summon_quick 0x4b090f, ai_think_party_update 0x4b75d2); the body (0x47f270) never reads it.
Declared `summon_dismiss(int handle)`; spells.cpp's definition now takes the (unused)
parameter too (merge, 2026-10-15).

**D1503 (levelrender.h). hover_tint_set_character tints a level object.** levelrender_draw_list compares
hover_tint_char (0x22f8d0c) with the record it draws, whose orientation is at +0x20 (0x485b5c-0x485b87:
`lea eax, [esi + 0x20]` ... `cmp esi, ecx`), a lobj; world_mouse_frame (0x43ae07) and
world_mouse_hover_target (0x43ba87) pass level objects. Declared with `lobj *`; the sum.symbols names
(hover_tint_set_character, world_hover_tint_character) are kept.

**D1504 (levelrender.h). door_toggle_state returns bool.** object_try_unlock_or_open tests AL after the
call (0x43c27b-0x43c285, `test al, al / jne`); level_scripts.cpp ignores the result.

**D1505 (levelrender.h). door_info +0x5c is the key item type.** object_try_unlock_or_open passes it to
party_inventory_find_type (0x43c0aa) and character_find_equipped_type (0x43c0d1), both ItemType *.
Anonymous union {int _unk5c; ItemType *key;} so s3d.cpp's `door->_unk5c = 0` stays.

**D1506 (summoner.h, summoner/gametarget.h). gametarget.cpp gets its unit header.** The gametarget block
of summoner.h (world_pick_set_skill_filter, world_pick_clear_filter, the five .bss externs,
object_open_pickup) moved unchanged to summoner/gametarget.h, which summoner.h includes, so every unit
that used them keeps its include. Added there: the unit's functions and data, WORLD_CLICK_* (the switch of
world_mouse_frame, jump table 0x43b194), and two outside variables the unit writes: world_mouse_target_handle
(0x593f50, sum.symbols) and world_last_click_pos (0x609ff8, name ours; world_click_move 0x43c03b).

**D1507 (gametarget.h). world_pick_debug_hook.** world_mouse_frame calls 0x40acc0 with two arguments
(0x43ad69-0x43ad7d: `push ecx / push edx / call 0x40acc0 / add esp, 8`), the level object and the face.
0x40acc0 is a bare ret shared by identical-code folding and declared without parameters
(empty_frame_hook_40acc0, D57), so the call is written to an empty function of its own (name ours),
defined in gametarget.cpp; the linker would fold it into 0x40acc0.

**D1508 (multi.h, summoner/multi/multispell.h). multispell.cpp's unit header.** mp_spell_cast_sync and
mp_summon_claim_release moved from the tail of multi.h to multispell.h (included by multi.h like the other
unit headers); the packet ids MP_CST_ANM .. MP_SMN_QCK (0x34-0x3a, names from mp_packet_table) and
mp_forward_cur_packet_buffered (0x442230, mp_handle_summon_quick) were added to multi.h.

**D1509 (summoner/multi/multi_chattracker.h).** New unit header for the IRC client; included by multi.h.
The record types are from the malloc sizes and the list links: irc_command 0x6c (next +0x68, 0x4ac032),
irc_user 0x28 (next +0x24, 0x4adb82), irc_channel 0x8c (users int16 +0x22, topic +0x24, next +0x88,
0x4ad9f4). irc_add_channel's second parameter is stored with `mov word ptr [ecx + 0x22], dx` (short).

**D1510 (vsdk/math/ix.h).** New header: plane {normal, d} with the inline set_normal_point (its COMDAT
0x43b5a0 lies in gametarget.cpp's range and its -1.0f, 0x580e1c, in gametarget.cpp's .rdata; d =
dot(p, n) * -1.0f, `fmul dword ptr [0x580e1c]` rather than fchs), ix_ray {origin, dir} (the ray that
world_pick_under_mouse builds at its +0x18 and passes to ix_ray_plane; gr_screen_to_world_ray writes the
same layout), ix_ray_plane, ix_point_linesegment. world_pick_under_mouse's `ground = facing` is the
implicit plane assignment (vector3::operator= call 0x413190 for the normal, then d).

**D1511 (gametarget.h). character_can_talk_to returns int.** `mov eax, 1` / `xor eax, eax` (0x43a5c8,
0x43a5ed) and world_mouse_hover_target tests EAX (0x43b735).

**D1512 (levelrender.h, comments only). lobj_hit +0x24 / +0x28.** +0x24 is the level object index
(world_pick_lobj indexes lobjs and levelrender_lobj_states with it, 0x43a85d-0x43a887; level_scripts.cpp
already calls lobj_from_index(hit.face)); +0x28 is the s3d_face pointer (minus mesh->faces, divided by 20,
0x43a8b0-0x43a8f3). The member names stay.

## gr_direct3d.cpp (decomp36)

**D1600 (vsdk/gr/directx/d3d_min.h). DirectX 7 without the system headers, as C-style COM.** The header
declares the DirectDraw 7 / Direct3D 7 structures, the seven interfaces as `{ Vtbl *lpVtbl; }` with
`__stdcall` function-pointer tables, the SDK's C call macros (`IDirect3DDevice7_SetRenderState(p, a, b)` =
`(p)->lpVtbl->SetRenderState(p, a, b)`), the IIDs and the constants. C style, not C++ virtuals: in
gr_d3d_set_bump_env the vtable of d3d_device is loaded before the d3d_f2dw call and the global is loaded
again afterwards for `this` (`mov esi, [eax]` at 0x5367b7, `mov edx, [0x2dc6c40]; push edx` at 0x5367be /
0x5367cc), which is the macro's double evaluation; a C++ call evaluates the object once. Slots from the
calls: device GetCaps +0xc, EnumTextureFormats +0x10, BeginScene +0x14, EndScene +0x18, Clear +0x28,
SetTransform +0x2c, SetViewport +0x34, SetRenderState +0x50, DrawPrimitive +0x64, DrawPrimitiveVB +0x7c,
SetTexture +0x8c, SetTextureStageState +0x94; IDirect3D7 EnumDevices +0xc, CreateDevice +0x10,
CreateVertexBuffer +0x14, EnumZBufferFormats +0x18, EvictManagedTextures +0x1c; IDirectDraw7 CreateClipper
+0x10, CreateSurface +0x18, GetCaps +0x2c, RestoreDisplayMode +0x4c, SetCooperativeLevel +0x50,
SetDisplayMode +0x54, GetAvailableVidMem +0x5c, GetDeviceIdentifier +0x6c; surface AddAttachedSurface +0xc,
Blt +0x14, Flip +0x2c, GetAttachedSurface +0x30, GetBltStatus +0x34, GetFlipStatus +0x48, GetSurfaceDesc
+0x58, IsLost +0x60, Lock +0x64, Restore +0x6c, SetClipper +0x70, Unlock +0x80; clipper SetHWnd +0x20; gamma
Get / SetGammaRamp +0xc / +0x10; vertex buffer Lock / Unlock +0xc / +0x10. The IIDs (0x583350 HAL device,
0x583360 IDirect3D7, 0x583370 gamma control, 0x583380 IDirectDraw7, checked against their bytes) come from
dxguid.lib and are extern. DirectDrawEnumerateA is called through the import-library thunk (`call
0x57b564`), so it has no dllimport. The type names are the SDK's, so the header cannot be included with
<ddraw.h> / <d3d.h> (levelrender.cpp includes <d3d.h>; it does not include this header).

**D1601 (gr_direct3d.h, gr_direct3d.cpp). The tagged code is two objects; the vertex pool is the vfx
object's.** sum.symbols tags 0x534a20-0x536944 and 0x538650-0x53ac18 gr_direct3d.cpp; the vfx code
0x536980-0x538630 (vsdk\geom) lies between. Three things put the boundaries at 0x536950 and 0x538650:
(1) the initialiser table runs 0x5889cc (gr_d3d_current_mode), 0x5889d0 (init_gr_d3d_vertex_buf),
0x5889d4 / 0x5889d8 (vfx camera position / orientation), 0x5889dc-0x5889e4 (the three CmdLineParams);
(2) the .bss blocks are 0x2d90ed4-0x2d90ee7 (texture scales, current mode), 0x2d90ee8-0x2dbfd1b
(vfx_camera_pos 0x2d90ee8, vfx_camera_orient 0x2d90ef8, gr_vertex_pool 0x2d90f10, vfx_active_instance
0x2dbfd10, vfx_render_alpha 0x2dbfd14) and 0x2dbfd20-0x2dc6cbb, and the .data blocks 0x5a9cf8-0x5a9cf9,
0x5a9cfc-0x5a9d23 (the vfx -1 and "$vis", ">>> FAILED TO GET TAG %s\n") and 0x5a9d24-; (3) the first block
ends with out-of-line header inlines (gr::Mode 0x5368a0-0x536930, then d3d_f2dw 0x536940, the SDK's F2DW),
which is where a vsdk object's COMDAT inlines go (imin at the end of bmpman.cpp; IsEqualGUID, sum.symbols
guid_equal 0x53ac00, at the end of the second block). The pool is used by the vfx mesh renderer. So the
first block is a separate file of vsdk\gr\directx (name not known), the second is gr_direct3d.cpp (its
__FILE__ is pushed at 0x53956c), and init_gr_d3d_vertex_buf / gr_vertex_pool are left to the vfx unit.
Both blocks are in one .cpp for now, split at the "PART 2" line.

**D1602 (gr_direct3d.h). gr_d3d_texture_scale_u / _v are float[2] and the names are swapped.** sum.symbols
has two floats 0x2d90ed4 (_u) and 0x2d90edc (_v). gr_d3d_tmapper_multipass passes 0x2d90ee0 and 0x2d90ed8
for the second stage (0x5365d0 / 0x5365da), so each is a pair indexed by stage. gr_d3d_tcache_set's fourth
argument is its u scale (sum.symbols' own signature) and receives 0x2d90edc (`push 0x2d90edc` is the
fourth argument at 0x535530); gr_d3d_bitmap multiplies the source x by [0x2d90edc] and divides by the
bitmap width (0x5358d1-0x5358f6), the source y by [0x2d90ed4] over the height (0x535916-0x53591c). The
symbol names are kept; the code passes &gr_d3d_texture_scale_v[s] as the u scale (fixing sum.symbols is
left open).

**D1603 (gr_direct3d.h). d3d_primary_surface is the render target, d3d_back_surface the primary.**
Windowed, gr_d3d_init_device creates the DDSCAPS_PRIMARYSURFACE (0x200) surface into 0x2dc6c48
(0x53975d-0x53976a) and the DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE (0x2040) surface into 0x2dc6c44
(0x5397b1-0x5397be); fullscreen, the 0x2218 flipping chain goes into 0x2dc6c48 and GetAttachedSurface(
DDSCAPS_BACKBUFFER) into 0x2dc6c44 (0x5398af-0x5398cd). The device renders into 0x2dc6c44 (CreateDevice
at 0x5399c2), gr_d3d_flip flips / blits into 0x2dc6c48. sum.symbols' names are kept with a comment.

**D1604 (gr.h). d3d_direct3d stays void \*.** gr.h declares it `void *` and levelrender.cpp casts it to the
system IDirect3D7 (it includes <d3d.h>), so gr.h cannot use d3d_min.h's type. gr_direct3d.cpp defines it
`void *` and reads it through `#define D3D_DIRECT3D ((IDirect3D7 *)d3d_direct3d)` (same code). The other
interface pointers are typed in gr_direct3d.h. d3d_use_vertex_buffer (gr.h) is defined in gr_direct3d.cpp.

**D1605 (gr.h, levelrender.h). gr state and helpers the back end reads.** Added under "gr_direct3d.cpp"
in gr.h: gr_vsync (0x25d8dd0, name ours; gr_d3d_flip turns it into Flip flags 1 / 9 with `neg dl; sbb edx,
edx; and edx, -8; add edx, 9` at 0x53a718), Gr_far_min / Gr_far_max (0x5a7294 / 0x5a7298, scaled by 0.1 for
S3), gr_calc_depth (0x50d880, float(float)), gr_get_fog_alpha (0x50d830; the result's AL is stored, int
kept), gr_poly_get_section (0x55fb00) with eight arguments (nv, verts, pass, bm, &u_offset, &v_offset,
&u_scale, &v_scale): the pushes at 0x5364b6-0x5364c9, and the body walks arg1[i] + arg2 * 12 + 4 / + 8
(0x55fb64-0x55fb7f); sum.symbols' "(bitmap, u, v, ...)" describes the inner part. gr_tmapper_pass (0x14) and
gr_multipass_vertex (a gr_vertex pointer, then 12 bytes of u, v, r, g, b, a per pass) are read from
gr_d3d_tmapper_multipass (0x5363f5-0x536742; names ours). levelrender.h: levelrender_d3d_vertex_buffer_pos
(0x240d9e8, name ours; reset by gr_d3d_init_device 0x53a0d7 and gr_d3d_flip 0x53a7d4, used by
levelrender_draw_list_d3d). gr_vertex is not given a constructor although the original has one
(gr__Vertex__Vertex 0x5081c0 constructs a vector3 at +0; `vector constructor iterator` in 0x536950, gr_rect,
gr_bitmap_float): adding it would construct levelrender.cpp's levelrender_face_verts, which has no
initialiser in the binary to check against. Left open.

**D1606 (gr_direct3d.h). Signatures from the call sites.** gr_d3d_set_bitmap(mode, bm, stage): three
arguments pushed (`push -1` at 0x535706, 0x535da8, 0x5361af), the third unused (as gr_opengl_set_bitmap's
stage). gr_d3d_is_mode_supported takes the gr::Mode by pointer (`mov ecx, [esp + 4]` is `this`,
0x53aadc), like gr_is_mode_supported. gr_d3d_set_default_transforms(dev, float zfar) returns the HRESULT
(the failures return EAX at 0x538b03 / 0x538b17); zfar is read twice as the second argument
(0x538b19 / 0x538b2f; gr_d3d_init_device pushes 250.0, 0x5399f6). gr_d3d_set_fog(int set_range): the
range is recomputed only when its argument is set (`mov eax, [esp + 0xc]; test eax, eax` at 0x53a903).
gr_d3d_read_backbuffer(x, y, w, h, buf) returns 3 (565) or 0. The DirectDraw enumeration records are 0x428
bytes (d3d_device_record: GUID, GUID *, GUID, GUID *, name[0x400]; arrays of 8 at 0x2dc07c8 and 16 at
0x2dc2990, from the strides and the neighbouring globals).

## characterinfo.cpp (decomp38)

**D1800 (engine/characterinfo/characterinfo.h). skill_names has 84 entries.** The header declared
`skill_names[36]` ("0x595bf4 ends it"), sized by the loop of items_parse_table. The table at 0x595b64 holds
84 pointers ("hand to hand" .. "serpent whip multi"), followed by the dword 84 at 0x595cb4, and every lookup
loop in characterinfo_parse_table ends at that address (`cmp esi, 0x595cb4` at 0x445c0b / 0x445c6d /
0x445ccf / 0x445d31 / 0x445d93). Now `skill_names[MAX_SKILLS]` plus `int num_skill_names` (no reader; the
same sizeof pattern as items.cpp's num_weapon_flag_names). items.cpp keeps its explicit 36.

**D1801 (vsdk/os/memory.h). Mempool gets an inline empty default constructor.** characterinfo.cpp defines
15 Mempools with no initialiser (Copy is called at run time: characterinfo_load_party, ..._alloc_level_buffers,
..._load_party_anims, statscreen_load_anim), and its static initialiser list has exactly 15 empty thunk /
`ret` pairs (init_empty_444850 .. init_empty_444a10, entries 162-176, between resistance_level_names and
ccmd_All_summons). An inline `Mempool() {}` gives both: the definitions compile and VC6 emits one empty
dynamic initialiser per object (as for ai_path_nodes, pathfinding.cpp). The 3-argument inline constructor
(D12) is unchanged; no other unit defines a Mempool without arguments.

**D1802 (engine/characterinfo/characterinfo.h). move_info, char_info, skill_info completed.** From the
parser stores: move_info +0x00 action (AddUniqueQuoted after "$Action:", 0x4461dd), +0x04 anim_file
(AddUnique after "$Animation:", 0x446163; the first argument of character_add_skeleton, 0x4477bd), +0x34
sound (sound_ref_parse(&m+0x34), 0x4461da), +0x48 effect ("$Effect:", 0x44621a); size 0x4c (the free list
stride, 0x4451cb). The names of decomp23 (anim, anim_class, category, id, variant, triggers, next, hit_tag)
are kept; id is the "$Variation:" index, variant the "$Location:" index. char_info: +0x18a short[8] attack
point tags (0x446a47), +0x19c short count and +0x21c int* of the cast-move spells (0x445253 / 0x44525f),
+0x25c head bone / +0x260 head height (0x446a87 / 0x446b54), replacing _unk18a / _unk19c / _unk21c /
_unk25c. skill_info `_unk24[8]` is two ints (-1 in every record; init_skill_table stores dwords).

**D1803 (engine/characterinfo/characterinfo.h). charinfo_collect_moves: the fourth argument is the list.**
The body indexes `info->moves` with [esp+0x18] (0x446d9c, the fourth argument) and never reads the second;
combat.cpp passes (moves, 0xa, info, 0, ...), so the second is the array size (not checked) and the fourth
the category (0, attacks). Parameter names changed only; the call is unchanged.

**D1804 (engine/characterinfo/characterinfo.h). charinfo_find_move's flag is a bool.** It is read as a
byte (`mov bl, byte ptr [esp+0x20]`, 0x446cc4; the same in charinfo_find_move_alt 0x446d24) and only tested.
The callers pass 0 / 1; `int flag` became `bool loaded_only`.

**D1805 (engine/characterinfo/characterinfo.h). species_info, chain_skill, multi_charinfo_slot_t.** New
types from the unit: species_table entries are 0xc bytes {name, species, byte load} (the stride 0xc and
`cmp byte ptr [edi+8], 1` at 0x4451e6; types.h's `int _unk08` is the load byte plus padding); chain_skill
0x14 bytes {name slot, species, skill, chain slot, byte points} (init_chain_skill_names stores dword, dword,
dword, dword, byte; character_learn_chain_skill compares the byte at +0x10, 0x4497a2); the multiplayer slot
0x3ec bytes {used, char_info copy, int textures[6]} (characterinfo_find_multi_slot copies 0xf4 dwords to
slot+4, charinfo_create_character_instance writes bm_reload results from slot+0x3d4).

**D1806 (vsdk/os/memory.h, engine/characterinfo/characterinfo.h). summon_mempool and player_instance_mempool
are defined in characterinfo.cpp.** Both lie in this unit's .bss (0x7592c0, 0x744680, between char_infos and
the move records) and are among the 15 objects of D1801. memory.h keeps its `extern summon_mempool`;
player_instance_mempool (sum.symbols module "-") is declared in characterinfo.h.

**D1807 (none; vsdk/ca/character.h unchanged). character_load's fourth argument.** charinfo_create_character_instance
passes the +Alternate character: texture entry there (`push [esi+0x268]`, 0x448e2b) and 0 otherwise; the
header calls it `int unused`. Written as `(int)info->alt_textures` with a NOTES question; the header is
not changed because character.cpp (decomp30) found no reader.

**D1808 (engine/ai/ai.h, engine/effects/fx.h, summoner/summoner.h, engine/objects/living_entity.h). New
declarations, no conflict:** ai_state::begin_emerge (0x473970, thiscall on cur_summon+0x324, 0x448068) and
ai_state::summon_emerge_update (0x4b4660, the same `lea ecx, [edx+0x324]`, 0x4480d2); fx_active_list
(0x2305920 sentinel, walked through +0x98 at 0x4480dd), fx_destroy(handle) and fx_end_owned(handle, bool)
(push 1 / push handle, 0x448222); alt_textures_find(name, int*) and alt_textures_find_entry(base, name)
(0x4465cf / 0x446653); Character::apply_hero_model(bool) (0x4628a0, `mov ecx, esi` with one pushed byte,
0x448aeb-0x448af3) as one line in living_entity.h.

## ai.cpp (decomp39)

**D1900 (engine/ai/ai.h). The AI profiler is one object, ai_profile ai_prof (0x22e48e0, 0x50 bytes).**
sum.symbols names the timer 0x22e4908 and the nine accumulators 0x22e490c-0x22e492c separately, but one
static initialiser (init_realtimetimers_22e48e0, entry 220) constructs both RealtimeTimers 0x22e48e0 and
0x22e4908 (`mov ecx, 0x22e48e0; call 0x5019e0; mov ecx, 0x22e4908; jmp 0x5019e0`, 0x472040-0x47204f): two
separate globals would get two initialisers. The layout is {timer, nine averages, timer, nine
accumulators}: ai_do_frame stores acc * 1000 / 500 of 0x22e490c + 4i into 0x22e48e4 + 4i (0x47497d-
0x474ae9). The sum.symbols names stay as macros (ai_prof_timer, ai_prof_path_us, ...). The first timer
has no reader (sum.symbols' "used by mp_handle_kick_pl" does not hold: no reference outside the
initialiser).

**D1901 (engine/ai/ai.h). ai_frame and ai_update_perception are cdecl on the character.** sum.symbols calls
both thiscall ai_state; ai_do_frame pushes the character and pops it (`push eax; call 0x472a90; add esp,
4`, 0x474bee-0x474c39), ai_frame reads it at [esp+0x28] after its EH prologue (0x472aac) and passes it to
0x472e80 the same way (0x472b51 / 0x472b5d). Declared as free functions (Character *).

**D1902 (engine/ai/ai.h). The goal slots are ai_goal[5].** `ubyte goals[5][0x34]` becomes a struct from
ai_goal_alloc's stores (type +0, flags +4 or-ed with 2, id +8 from ai_goal_next_id, level_session_time
+0xc, priority +0x10, pos +0x14, layer +0x20, spell +0x24, target +0x28; 0x47609e-0x47611e), the
ai_state_init clears (+0, +0x10 = -1, +4, +0x30) and ai_state_save, which writes +0xc, +0x14..+0x1c and
+0x2c through fld / fstp (floats) and the others with mov (0x474700-0x4747b4). +0x30 is the goal's path
(ai_goal_remove frees it). goal_add's eighth argument is a float (boss_release_mount's 30.0f reaches it
unconverted through ai_goal_add_move_to_timed: `push edx` of the raw argument, 0x4741a4); goal_alloc
takes seven. The goal_find parameters keep their names (conversationctl.cpp), with a comment. Nothing
else read `goals` as bytes.

**D1903 (engine/ai/ai.h). The goal_add wrappers return int; add_move_to_timed's walk is a bool.** The
wrappers end in `call 0x476190; ret n` and their callers in ai.cpp test the result (ai_goal_add_hide's
`xor eax, eax` path), so add_cast_spell / add_cast_at_target / add_cast_at_point / add_skill_from_behind
/ add_hide / add_sneak / add_move_to_timed are `int` (they were void with "return unused"; the existing
callers ignore the value). add_move_to_timed tests its fourth argument with `cmp dl, 1` (0x474196), so
bool; its fifth goes to goal_add's priority slot (0x4741b4), the parameter name `ms` is kept.

**D1904 (engine/ai/ai.h). +0x214 is the path's start point; +0x54 and +0x214 are vector3; path is
ai_path *.** ai_path_find_new and ai_path_find_append copy the search's from point there
(`mov edx, [edi]` with edi = from, 0x473cd1; 0x473dce), and ai_path_begin drops a one-node path whose
node lies within 0.05 of it (0x473ad7-0x473af2), so `path_dest` became `path_start`. No other unit used
the old names. ai_path_find_new adds char_info +0x260 (head_height, D1802) to the cost bias
(`fadd dword ptr [ecx + 0x260]`, 0x473cae); the field name is kept.

**D1905 (engine/ai/ai.h). cast_effect_tracks_reset / _add / _count are ai.cpp's.** They follow
ai_can_be_knocked_down without a gap (0x475090-0x475178), their table 0x22e4950 lies in this unit's
.bss (between ccmd_attack_point 0x22e4930 and ccmd_ai_frame_update_rate 0x22e4990), ai_do_frame counts it
down (0x474b47-0x474b79), and cast_effect_track_add's 1/15 (0x581a08) follows this unit's .rdata
constants 0x5819fc-0x581a04. sum.symbols has module "-" for them. The table type is cast_effect_track
{spell, handle, float time}. vsdk/geom/vfx.h gets vfx_instance::id_bits (+0, `shr eax, 9` at 0x4750d6)
and vfx_base::num_frames (short +0x12, `movsx edx, word ptr [eax + 0x12]` at 0x47512f).

**D1906 (engine/ai/ai.h). The AIS_* name table is five entries short from 0x22.** ai_substate_names
(0x59ad94, 55 names and 10 NULLs) is read by nothing. By index it matches the sub-modes up to 0x21
(9 / 0xb enter / leave hiding, 0xc / 0xd sneaking, 0xf close and 0x10 ranged combat, 0x11 backstab,
0x15 choose spell with ai_set_mode's random 500-1500 ms timeout, 0x16..0x1d the positioning modes that
set flag 4), but the cast sub-modes 0x27 / 0x28 / 0x29 (ai_goal handlers) and the disabled ones (0x2c
ai_knock_down_local, 0x2d ai_is_knocked_down, 0x2f pushed, 0x31 stunned) match it only with an offset of
5 (AIS_SPELL is entry 0x22, AIS_TRIPPED_FALL 0x27, AIS_STUNNED 0x2c). ai.cpp defines the values it uses
as AIS_* macros with the shifted names; 0x1f and 0x3f have no name.

**D1907 (summoner/multi/multi.h, engine/objects/living_entity.h, others). New declarations, no
conflict.** mp_player gets +0x64 move_update_timer (a GameTimer: `lea ecx, [edx + 0x64]; call
GameTimer::SetTimeout` on mp_local_player, 0x472d8f) and the flag bits 8 / 0x10 that ai_frame keeps;
multi.h includes vsdk/os/timestamp.h. The senders mp_send_new_path, mp_send_obj_update,
mp_broadcast_ai_disable, mp_send_swi_2dir / _ndir / _hide (argument counts from the pushes and `add esp`).
living_entity.h: kill, on_death, set_flag_2_4, ai_move_frame, ai_suspended (thiscall, `mov ecx, esi`).
objects.h: handle_slot_release (0x4a36a0). level_scripts.h: cutscene_script_is_active (0x4ed770).
vector.h: frand_range (0x542770). ai.h: the ai.cpp data, the mode handlers of the later AI units, goal_add,
goal_alloc, goal_remove, ai_groups_frame, ai_mode_frame_counts (13 ints at 0x247e840, cleared by
`rep stosd` with ecx = 0xd, 0x474b39).

**D1908 (none; engine/combat/combat.h unchanged). living_combat::set_target's first argument.** Every
ai.cpp caller pushes the owner pointer there (`push edi; push eax` with eax = owner, 0x4744b4); the
declaration keeps `int unused` (combat.cpp's definition), so ai.cpp passes `(int)owner`.

## gr.cpp (decomp40)

**D2000 (gr.h). gr__Screen is 0xa8 bytes and holds the colour, texture, fog and tint state.** sum.symbols
names gr_color 0x25d8d84, gr_texture / _2, gr_fog_enabled / _color / _near / _far, gr_fog_z_*,
gr_fog_alpha_*, gr_fog_vertex_*, gr_tint_enabled / _r / _g / _b and (D1605) gr_vsync 0x25d8dd0 as separate
globals. They lie inside gr_screen (0x25d8d30): gr_init clears 0x2a dwords from it with `rep stosd`
(0x505e03-0x505e15), which reaches 0x25d8dd7, and then sets current_bitmap / _2 to -1 and the fog fields to 0
(0x505e8c-0x505ea4) after the clear; the implicit constructor gr__Screen__Screen (0x5081a0, called by
init_gr_screen 0x505b00) constructs exactly two sub-objects, at +0x54 and +0x61 (`lea ecx, [esi + 0x54]` /
`[esi + 0x61]`, call 0x505530), which are the two Colors (gr_color, gr_fog_color; the byte-aligned +0x61
follows the bool fog_enabled at +0x60). 0x505530 is the empty constructor that vector3() and Color() share
by identical-code folding (sum.symbols calls them "the vector3 members"). The next object after +0xa8 is
gr_mode_25d8dd8, built by its own initialiser. The struct gets the fields (names ours where sum.symbols has
only the global's name), LAYOUT_CHECK 0xa8, and the old names become macros on them, as D800 did for
level_s3d (`#define gr_color (gr_screen.current_color)`, ...); levelrender.cpp, gr_opengl.cpp and
gr_direct3d.cpp compile unchanged. dual_monitors (+0x14) is a bool (gr_check_dual_monitors writes the
byte, 0x506134 / 0x50614e). +0xa4 (0 from gr_init, pushed to gr_poly_begin by gr_poly 0x53e808) is
poly_arg_a4 (name ours). The `extern bool gr_vsync` of the "gr_direct3d.cpp" group in gr.h was left as it
was (that group belonged to another agent); a `#define gr_vsync (gr_screen.vsync)` after the group
supersedes it, and the extern line should be deleted at merge.

**D2001 (gr.h). The mode globals are gr::Mode objects and the front end takes
gr::Mode; this revises D7 / D1300 for gr.cpp.** Every one of the 30 globals has a dynamic initialiser
`mov ecx, &global; call 0x508160` (gr::Mode's six-field constructor, ret 0x18; table 0x5887ec-0x588860),
so each is a gr::Mode object, not a dword (a gr_mode global cannot get such an initialiser). The front end
hands the mode to the back ends without conversion: gr_tmapper pushes its argument dword straight to
gr_opengl_tmapper(gr::Mode) (0x507a30-0x507a3f), and gr.cpp, built without inline expansion (it calls the
inline 0x508160 out of line), would otherwise call Mode(gr_mode) 0x506810, which has no caller in the
binary. So gr.h now declares the 30 globals `extern gr::Mode` (12 of them have no reader: gr_mode_<address>,
names ours) and gr_bitmap_ex / _stretched / gr_bitmap / gr_bitmap_float / gr_line / gr_rect / gr_circle /
gr_tmapper with gr::Mode; gr_is_mode_supported takes gr::Mode * (it passes the pointer to
gr_d3d_is_mode_supported(gr::Mode *), D1606). The rest of the tree still stores and passes dwords
(gr_string's int mode, ui_bitmap_scaled(gr_mode), locals): gr::Mode gets an inline `operator gr_mode()`
(ours; no copy exists in the binary, so only the optimised game units, which expand it, may use it).
GR_MODE_AUTOMAP_MASKED becomes `gr::Mode(1, 1, 2, 5, 0, 2)`, the six fields of 0x4028821. All src/decomp
.cpp compile unchanged (same 11 warnings). Open: gr_string and ui_bitmap_scaled probably take gr::Mode in
the original too; the vsdk callers of gr_string (built without inline expansion) would call the operator.

**D2002 (gr.h). gr::Mode::Mode(gr_mode) is out of line, defined in gr.cpp.** Revises D1300's inline.
0x506810 sits between Color::Color (0x5067f0) and Color::set (0x506820), which gr.h already declares out of
line, after gr_close and before gr::set_clip, i.e. among gr.cpp's own functions in source order; the
unit's COMDAT inlines are all at its end (0x508160 Mode(6 fields), 0x5081a0 the gr__Screen constructor,
0x5081c0 gr_vertex's constructor), in the order of their first use. It has no caller anywhere, which a
COMDAT copy would not be emitted without (the vsdk objects emit only the inlines they use). So it is a
plain member defined in gr.cpp. Implicit conversions from gr_mode in the tree now call it (compiles; no
such conversion exists in the binary).

**D2003 (gr.h). gr_init returns int; the seventh argument is vsync.** gr_init ends with `xor eax, eax`
(0x5060fb) and LoadSettings ignores EAX. The byte argument goes to 0x25d8dd0 (0x505eaa), the byte
gr_set_vsync writes (0x506164) and gr_d3d_flip / gr_glide_flip read (D1605); the old name "dual" came from
the neighbouring dual-monitor field (+0x14, gr_check_dual_monitors). LoadSettings pushes 0.

**D2004 (vsdk/gr/opengl/gl_min.h). glFinish without dllimport for gr.cpp.** gr_finish ends with `jmp
0x5504f0`, the linker's import thunk for glFinish (`jmp [0x57e318]`), not `jmp [0x57e318]` itself, so gr.cpp
saw glFinish declared without __declspec(dllimport), unlike gr_opengl.cpp (D1305). gl_min.h's GL_MIN_API
drops dllimport when GL_MIN_NO_DLLIMPORT is defined; gr.cpp defines it before including gr_opengl.h.

**D2005 (new vsdk/gr/glide/gr_glide.h; gr_direct3d.h; targautils.h; gr.h). Declarations for gr.cpp's
calls.** New gr_glide.h: the 24 Glide entry points gr.cpp dispatches to (sum.symbols names, module
vsdk\gr\glide\gr_glide.cpp), signatures from gr.cpp's pushes (gr_glide_page_in gets both of gr_page_in's
arguments, 0x506a00-0x506a0a; gr_glide_bitmap the ten of gr_bitmap_stretched; gr_glide_is_mode_supported
the pointer). gr_direct3d.h, texture-cache section: gr_d3d_mark_texture_dirty (0x54d3f0) and
gr_d3d_tcache_free (0x54d430), one argument each. targautils.h: jpeg_write_file(name, rgb, w, h) (0x5565e0;
pushes at 0x5064b2-0x5064bd). gr.h: gr_set_alpha_test takes bool (it forwards the argument dword to
gr_d3d_set_alpha_test(bool), 0x50812b-0x508130, as gr_set_antialias forwards its bool, which
cmd_antialiasing builds with `sete al; push eax`, upper bytes undefined); a new "gr.cpp (decomp40)" group
with the unit's remaining functions and globals (gr_check_dual_monitors, gr_set_vsync, gr_deactivate,
gr_init_context, gr_screenshot, gr_get_fade_level, gr_draw_fade, gr_set_aspect, gr_flip, gr_zbuffer_clear,
gr_set_texture, gr_circle, gr_tmapper, gr_tmapper_multipass, gr_noop_507d20, gr_set_fog, gr_set_tint,
gr_finish, gr_tcache_flush, gr_set_antialias, gr_flush_pipeline, the four stubs, gr_poly_buffer_flush
0x545d70; Gr_alpha_fog_ratio, Gr_level_fog_multiplier, gr_fade_level, gr_gamma, gr_gamma_percent,
gr_fullscreen_rect, gr_fade_duration, gr_fade_time, gr_initialized, Gr_show_stats, cmd_antialiasing_on,
gr_fade_state, gr_zbuffer_enabled). None of these was declared before (searched by address).

**D1700 (living_entity.cpp). living_entity.h rewritten from the unit.** The class keeps every offset the
pilots use; members the unit reads are named and typed (velocity / +0x6c0 as vector3, move_start_time
float, death_move move_info *, CF_* flag names from the tests in the unit: 1 combat ready, 0x4000 hurt,
0x100000 on ground, 0x4000000 head look done, 0x80000000 no ground). The status ids 7, 8 and 14-17 are
used (status_effect_add 0x454700 cases; Light, Invisible, Firewall, Blizzard, Ice coffin, Dark wall),
so NUM_STATUS_EFFECTS is 18 with names for all of them.

**D1701 (living_entity.cpp). XSTR is redefined in the unit.** The /Od code tests the constant 0 of
`(0 ? text : localize_strings[id])` before every lookup (`xor ecx, ecx; test ecx, ecx; je`, status_effect_add
0x454c79). localize.h's XSTR stays an lvalue (other units take its address), so living_entity.cpp
`#undef`s it and defines the conditional form locally.

**D1702 (living_entity.cpp). combat stays a union.** Character::Character (0x44e060) constructs members in
declaration order (status slots, timer2ec, ai, combat, timer6d4, net_update_timer). combat (+0x558) is the
D1 union with `protection` (new_assess.cpp), and a union member with a constructor is not allowed, so the
source calls combat.construct() in the body; the order differs from the binary by that one call.

**D1703 (living_entity.cpp). status_effect replaces status_slots.** `ubyte status_slots[18][0x20]` becomes
`status_effect status_effects[18]`: kind +0, GameTimer +4, fx +8, source +0xc, spell / item_info * +0x10,
tick GameTimer +0x14, tick_ms +0x18, value +0x1c (float: fld / fstp at +0x1c in character_status_value).
The constructor's loop builds the timers at +4 and +0x14, stride 0x20 (0x44e0a6).

**D1704 (living_entity.cpp). skill is 0x5c bytes; +0x670 is the light.** `skill[0x60]` overlapped an int
the unit uses as an attached light handle: set to -1 in init (`mov dword ptr [edx + 0x670], -1` 0x44e23a),
stored from attached_light_create (0x44ed04), tested against -1 and freed (0x44ee2d / 0x44ee39). skill ids
stop below 0x5c.

**D1705 (living_entity.cpp). get_hit_by_spell arguments.** (int spell, int source, vector3 *pos, matrix
*orient, int source_type, ItemType *item, float power, bool preloaded) from the uses in 0x45dea0 (pos
and orient passed to the effect creators, item read for its effect values in cases 3-5, power fld'd as
the chain-hit multiplier). The header had eight ints.

**D1706 (living_entity.cpp). add_hp / status_effect_add parameter types.** add_hp(float delta, int
source, int a, bool from_net) -> bool (the last is a byte test, it suppresses mp_broadcast_hp);
status_effect_add(int status, int ms, int spell, float value, int source, bool permanent): +0x10 gets
the spell, +0x1c the float value, the last byte picks kind 2. Callers in combat.cpp keep compiling
(int arguments convert).

**D1707 (living_entity.cpp). move_info fields.** name +0 (stricmp in charinfo_find_move_alt callers),
sound +0x38 (passed to the sound start in play_move), effect +0x48 (fx name). prop_tag_bones (+0x168,
monster_drop_loot) shares a union with combat.cpp's `_unk168`.

**D1708 (living_entity.cpp). xp_level_up_gains argument order.** character_add_xp pushes (species,
&level, &max_hp, &max_ap, &points, &cap) (0x44fc77-0x44fca8); sum.symbols names the pointer
parameters in another order. The push order is used.

**D1709 (living_entity.cpp). container lock is a float.** container_obj_create / container_spawn_loot
get the lock value pushed from a float local (monster_drop_loot, `push ecx` of ebp-8 at 0x452eaf, stored
with fstp before), matching container +0xe4 `float lock`.

**D1710 (living_entity.cpp). lobj_hit fields.** pos (vector3) at +8, was _unk08[6] (the hit point character_probe_lobj_ground copies); +0x24 is
face (copied to item_obj +0xa8) or lobj (read as a word by character_probe_lobj_ground), a union with both
names; +0x28 is a union of _unk28 and s3d_face *face_ptr (the face record find_ground reads).

**D1711 (living_entity.cpp). hero_model_prefix returns the render mask.** 0x462ec0 returns an int mask
(the defaults {6, 6, 2, 2} at 0x5996a4, or the helmet's +RenderMask byte) in eax; new_status.cpp and
new_inventory.cpp call it as const char *, so the declaration keeps that type and the body casts.

**D1712 (living_entity.cpp). 0x461dc0-0x46353f is optimised.** character_say ... cmd_death have no ebp
frames and 0x90 padding, unlike the rest of the unit. They stay in living_entity.cpp: strings.tbl ids
314-352 (living_entity.cpp's block) are used there (character_get_display_name 0x462070 reads 351 / 352)
and the .data / .bss of both parts interleave (Debug_death 0x59a16c between the unit's other globals).
Either `#pragma optimize` or a second object; recorded, not resolved.

**D1713 (merge of ai.cpp and living_entity.cpp). ai_state::travel_to_xyz takes one vector3 by value.** ai.cpp (decomp39) declared three floats, living_entity.cpp (decomp37) a vector3. The stack layout is the same; Character::ai_travel_offset (0x455e00) builds the sum in the argument slot with the inline copy constructor, which is how a vector3 argument is passed, so the vector3 form is kept. The merge also mapped living_entity.cpp onto the names other units had fixed: move_info::action (+0x00) and sound.handle (+0x38), char_info::target_points (+0x18a) and head_bone (+0x25c, the bone kept on the navigation mesh for CIF_ELONGATED records), lobj_hit::face (+0x24), vfx_instance::id_bits >> 9 and vfx_base::num_frames, ai_state::add_travel.

## level_scripts_common.cpp (decomp41)

**D2100 (levelscripts/level_scripts.h). lc_entry::flags is a byte.** Every access to +0x104 is a byte
operation: `mov byte ptr [edx + 0x248cf44], bl` (lc_entry_add 0x4cd3f3 / 0x4cd447), `mov byte ptr
[esi + 0x248cf44], 0x0` (level_script_parse_table 0x4cdbec), `mov dl, byte ptr ...; or dl, 1; mov byte ptr
...` (lc_entry_mark_used 0x4cd4f1), `test byte ptr [esi + 0x248cf44], 0x1` (container_examine 0x43c468) and the
byte read-modify-write of savegame_read_two_stage (0x4d3670 / 0x4d367b). An `unsigned int` would give dword
stores for `= 0`. Now `ubyte flags` plus `_pad105[3]`; the existing `& 1` / `|= 1` uses compile unchanged.

**D2101 (levelscripts/level_scripts.h). script_object::actions is typed.** The 54 actions were raw
`ubyte[54][0x18]`. level_script_restart reads `type` (+0x00) and clears +0x0c of type 0xb actions
(`mov dword ptr [ecx + eax + 0x10], ebx` with the action base at +4, 0x4cb193) and script_object_receive_go
sets the same field (`mov dword ptr [ebp + 0xc], 0x1`, 0x4ce2c9); script_object_goto_label builds a type 15
action with the label at +0x0c (0x4ce350 / 0x4ce358). New `struct script_action` (0x18 bytes, the fields of
types.h script_action) and SCRIPT_ACTION_WAIT_GO / LOOP / GOTO. No other unit used `actions`.

**D2102 (levelscripts/level_scripts.h). level_item_rec::navpoint is a navpoint pointer; navpoint +0x2c.**
level_items_parse_table stores navpoint_find's result (a record of navpoints / cutscene_navpoints, or 0;
0x4dc5d3 / 0x4dc5e2) at +0x48, then writes the item index to `[navpoint + 0x2c]` (0x4cb6b6), reads the layer
at +0x20 (0x4cb6d8) and passes the pointer itself as the name to navpoint_get_position (0x4cb6c2; the name
is at +0). Was `int navpoint`; now `struct navpoint *navpoint`. navpoint's `ubyte _unk28[8]` became
`int _unk28` (level_script_restart skips characters whose start navpoint has it nonzero, 0x4cb0d6) and
`int level_item`. No other unit used either field.

**D2103 (engine/rendering/levelrender.h). door_info +0x60 / +0x6c are sound_refs.** level_script_parse_table
stores a level_script_stringpool name at +0x60 / +0x6c, or 0 with -1 at +0x64 / +0x70 (0x4cdad2 / 0x4cdb2f),
and three parameter bytes at +0x68..+0x6a / +0x74..+0x76 (0x4cdab9-0x4cdacd), then calls
sound_ref_load(&door+0x60, 1) / (&door+0x6c, 1) (0x4cdd2d / 0x4cdd42): the 12-byte sound_ref layout of
gamesound.h. s3d.cpp resets the same bytes through `_unk60`..`_unk74`, so the old fields stay in an
anonymous struct of a union with `sound_ref open_sound, close_sound` (levelrender.h now includes
gamesound.h).

**D2104 (levelscripts/level_scripts.h). level_trigger, and +0x50 is +Index:.** The 0x64-byte record of
level_triggers (0x2490dd0) is laid out from level_parse_trigger (name +0x00, script name +0x20, +RefName:
+0x24, +Id: +0x48 via ParseTypeString over {"load level", "boss", "sound"} at 0x59e0f0, +Type: +0x4c over
{"spline", "location"} at 0x59e0e8, +Index: +0x50, flags +0x54 = 1 then |= 4 for +Multi, +Plane: +0x58,
the path or navpoint +0x5c, +Radius: +0x60). sum.symbols names the copy of +0x50 `level_exit_trigger_index`
(0x2490c60) and level_apply_pending_change "tests bit 2" of it; the code stores it into level_entry_point
(`mov dword ptr [0x60a33c], ecx`, 0x4ccda6), so it is the entry point of the next level. The global keeps
its sum.symbols name; the field is `index`. The flag bit 0x80000000 is level_trigger_trip's "fired once".

**D2105 (levelscripts/level_scripts.h). level_script_init takes a bool.** It reads a byte argument (`mov cl,
byte ptr [esp + 0x24]; test cl, cl`, 0x4cc854) that gates the monster level scaling; state_game_enter pushes
`sete cl` of its "loaded from a save" byte (0x43823c). Declared `level_script_init(bool scale_levels)`.

**D2106 (levelscripts/level_scripts.h). lc_entry_add / lc_entry_clear / lc_entry_exists take the level
object.** sum.symbols calls the argument a hash; the functions store it at lc_entry +0x00 (0x4cd3d0) and
compare it with +0x00, the `lobj *object` that lc_find_by_name dereferences for the name (0x4cd53d). Declared
with `lobj *`.

**D2107 (none). Character::set_armour_alt_texture keeps (int, int).** living_entity.cpp (decomp37) declares
`set_armour_alt_texture(int on, int force)`; the body reads both arguments as bytes (`mov al, byte ptr
[esp + 0x4]` 0x462c91, `mov al, byte ptr [esp + 0x8]` 0x462cc7), so bool fits better, but changing it means
changing living_entity.cpp's definition; script_eleh_strip_party_gear passes (strip, false) through the int
form. Left for the merge.

**D2108 (several headers). New declarations, no conflict:** level_scripts.h the unit's functions, globals
and types (level_trigger, script_avoid_spline, LEVEL_TRIGGER_*, the shop arrays); script_internal.h
navpoint_find, script_parse_navpoints / _objects, script_action_goto, script_entry_start / _advance /
_check_done, script_build_avoid_spline, str_copy_append_32, script_ambients_dirty / script_level_items_dirty;
levelrender.h level_marker_find, level_trigger_record_find (returns the s3d_named_path record of
level_s3d.named_paths, 0x477580) and the fog calls, with neutral parameter names (fog_set_level_default
stores its first argument at 0x59bd88, `mov dword ptr [0x59bd88], eax` 0x486ba1, which sum.symbols now
calls fog_level_near_distance, while level_parse_table passes "+Fog Far:" first, 0x4cbf02-0x4cbf09; open in
NOTES); snd.h the three emitter calls and the EAX file calls; pathfinding.h
seg2d_point_left_of and segments_intersect_xz; vector.h distance_squared; ai.h has_goal_without_flag20;
objects.h shop_inventory_sort, character_has_equipped_type, containers_reset / _free_all; fx.h
fx_level_restart; summoner.h camera_reset_to_player, cutscene_play, cutscene_script_table, US_open_demo;
multi.h mp_netgame_host_level, mp_netgame_level_scale, mp_change_level (one pushed argument, 0x4ccdd9);
boss_ai.h titus_combat_enabled and pyrul_combat_enabled / machival_combat_enabled (names ours: the bytes
0x247fd38 / 0x248b034 that cmd_pyrul_combat / cmd_machival_combat set). level_pending_exit_name became
`const char *` (it holds trigger +0x20, a pooled name or "").

## gameplay.cpp (decomp42)

**D2200 (summoner/gameplay.h). combat_mode_set takes a bool.** It was `void combat_mode_set(int on)` in
summoner.h. The function compares the byte combat_mode with the argument's low byte (`mov ebx, [esp + 0xc]`;
`mov al, [0x5fccb4]`; `cmp al, bl`, 0x439595-0x43959e), stores BL (0x4395a7) and passes the register on to
ai_set_combat_mode_goal(bool, int) (`push ebx`, 0x4395db / 0x439603). Declared `void combat_mode_set(bool on)`
in gameplay.h; the callers (combat.cpp, conversationctl.cpp, gametarget.cpp, living_entity.cpp,
new_hud.cpp: 0 / 1 / true / !combat_mode) compile unchanged.

**D2201 (summoner/gameplay.h). party_toggle_hold returns the solo mode bit.** It was `void`. The function
ends with `mov eax, [game_flags]; and eax, 0x1000; ret` (0x4396b9-0x4396c4) and returns `xor eax, eax` in a
town (0x439636). Declared `int party_toggle_hold()`; every caller ignores the value (hud_key_h_cb even
tail-jumps to it, 0x40e388), so the existing calls are unchanged.

**D2202 (engine/objects/living_entity.h). layer_lobj_list is the level's s3d_vis_set.** level_load_layers
looks the names "$layer01".."$layer20" up in level_vis_records (`_stricmp(name, [vis_records + 0xc*j])`,
0x4388b7-0x4388c6) and stores the record's address (`lea ecx, [esi + 2*esi]; lea eax, [edx + 4*ecx]; mov
[edi], eax`, 0x4388e1-0x4388ed) into level_layer_ground[i] (0x9414b8). The old struct {int _unk00; ushort
count; ushort _pad06; int *indices} is that record ({char *name; ushort count; int *indices}, 0xc bytes).
`typedef s3d_vis_set layer_lobj_list;` replaces it and living_entity.h includes engine/s3d/s3d.h;
living_entity.cpp reads only count and indices and compiles unchanged.

**D2203 (summoner/gameplay.h). party_member_join returns the character; leader and place_at_start are
bools.** It was `void party_member_join(const char *name, int a, int b, int c, int d)` ("meaning not
checked"). EAX is the character: living_from_handle of a known member (0x439c36 / 0x439c4d), player_create's
result (0x439cad-0x439cb6, returned at 0x439d37) or 0; level_spawn_party uses it (`mov edi, eax; test
edi, edi; ... mov edx, [edi + 0x80]`, 0x438b89-0x438b96). The third argument is compared as a byte
(`mov ebx, [esp + 0x11c]` ... `cmp bl, 1`, 0x439cc3; also passed on to player_create's bool leader) and so
is the fourth (`cmp byte ptr [esp + 0x120], 1`, 0x439cfa); the second is the level (`cmp eax, 1; jle`,
then xp_for_level(level), 0x439cd5-0x439cdf), the fifth the char_infos index (`test esi, esi; jge`,
charinfo_find_by_name when negative, 0x439bc7-0x439bda). Declared `Character *party_member_join(const char
*name, int level, bool leader, bool place_at_start, int char_index)`; level_scripts.cpp's calls
`(name, 1, 0, 1, -1)` compile unchanged (0 / 1 convert to bool).

**D2204 (summoner/gameplay.h). camera_reset_to_player takes a bool.** decomp41 declared `void
camera_reset_to_player(int snap)` in summoner.h. The function tests a byte (`mov al, [esp + 4]; test al, al`,
0x439510-0x439514) and state_game_enter passes its bool local. The declaration moved to gameplay.h as
`void camera_reset_to_player(bool snap)`; level_scripts_common.cpp's call with 0 is unchanged.

**D2205 (engine/camera/camera.h). camera_view_pos, camera_view_orient, camera_fov and game_camera_heading are
macros on game_camera.** gameplay.cpp defines `camera game_camera` (0x60a018: sinit_game_camera 0x438d60,
init_game_camera `mov ecx, 0x60a018; jmp camera::camera` 0x438d70, the atexit destructor 0x438d90), and the
four externs are its +0x278 (0x60a290), +0x284 (0x60a29c), +0x18c (0x60a1a4) and +0x180 (0x60a198), so
separate externs would have no definition. They become `#define camera_view_pos (game_camera.view_pos)` and
so on, as D604 did for config_solo_confirm; the readers and writers (conversationctl.cpp, gametarget.cpp,
new_saveload.cpp, new_automap.cpp, gameplay.cpp) compile unchanged. camera gets its constructor (0x478d70)
and destructor (0x478df0) declared.

**D2206 (several headers). New declarations for gameplay.cpp, no conflict.** summoner/gameplay.h: the unit's
functions and data, moved out of summoner.h with the level globals (level_filename .. level_layer_automap_index,
all in this unit's .bss 0x60a2c8-0x60a3cb), party_toggle_hold / party_select_member / party_member_join /
party_teleport_to / combat_mode_set / camera_reset_to_player; summoner.h includes it where the party block
was (level_entered_by_load, level_scripts.cpp's, stays in summoner.h). summoner.h also gets
gameseq_get_prev_state / _next_state, game_pause_count_dec, ui_poll_and_process (key handler called with
the key, return ignored: `push esi; call edi; add esp, 4`, 0x432887), input_flush, cursor_visible, the
cursor bitmaps, the cutscene and player input calls, the cd calls and GF_ESCAPE_CUTSCENE (0x10),
GF_CONVERSATION (0x100), GF_NO_RENDER (0x400), GF_NO_CLEAR (0x4000), GF_PARTY_DEAD (0x40000; names ours).
mp_player +0x2c is `int char_info` (level_spawn_party pushes `[ebp + 0x2c]` as party_member_join's index,
0x438b6d). ai_state::process_goals (0x4763a0, thiscall: `lea esi, [eax + 0x324]; mov ecx, esi`, 0x4395d0).
fx_frame(float, bool) (`mov al, [esp + 8]; test al, al`, 0x48a230). memory.h declares the six level_mem_*
counters (D202) that level_load zeroes. The others are plain additions (interface.h, automap.h, multi.h,
objects.h, ai.h, levelrender.h, camera.h, fx.h, boss_ai.h, level_scripts.h, localize.h, file.h,
file_packfile.h, effect_mem.h, snd.h, vfx.h, bmpman.h).

**D2207 (gameplay.cpp). state_game_load_failed is set only when the load failed.** sum.symbols says "set
once the level is loaded". The only store of 1 is 0x43821b, on the shared tail of the two failure paths of
state_game_enter (level_load returned false at 0x4381bb, or savegame_load_pending returned 0 at 0x43820d);
the success path never writes it after the clear at 0x438132. state_game_leave saves the conversation state
(0x438cb8) and calls game_pause_count_inc (0x438d2e) only when it is 0, i.e. after a good load. The
sum.symbols name is kept, with a comment.

**D2208 (gameplay.cpp). world_last_click_pos and world_mouse_target_handle are defined here; init_empty_438db0
is world_last_click_pos (unsure).** Both were extern only (gametarget.h, summoner.h). world_last_click_pos
(0x609ff8) lies in this unit's .bss, and world_mouse_target_handle (0x593f50, -1) in its .data between
level_load_layers' "$layer%02d" (0x593f44) and game_render_frame's "Fog_near" literal (0x593f54). The empty
static initialiser entry 123 (0x438da0 / 0x438db0, a bare ret) sits between game_camera's (122) and
ccmd_render_3d_obstacles' (124); a vector3 global (empty inline constructor, D500) is the only class object
of the unit's .bss without a constructor call, so it is assigned to world_last_click_pos.

## level_script_sewers / templeint2 / jadetemple / wolong2 / lpalaceint / liangshan / wolong_caverns / catacombs (decomp43)

**D2400 (level_script_lpalaceint.cpp). The guard line tables hold string-table slots; the single servant
line is a one-entry table.** sum.symbols types ls_lpalaceint_guard_halt_lines / _servant_lines char*[7] /
char*[5] and ls_lpalaceint_guard_servant_line char*. The initialisers store slot addresses
(`mov dword ptr [0x257ea20], 0x25cdcf4` = &localize_strings[518], 0x4f2a00; 0x25cdd24 = slot 530 at
0x4f2ab0) and every reader passes the table entry to localize_string_or_invalid(char **) (0x4f32fc /
0x4f336a / 0x4f3412), so the tables are `char **[n]` with XSTR_SLOT initialisers (as ai.cpp's
ai_combat_style_names). The single line is read after a `call rand` whose result is dropped (0x4f3365): an
index `rand() % 1` folds to 0 and leaves the call, so it is written as `char **[1]` read with
`[rand() % 1]`.

**D2401 (engine/ai/boss_ai.h). The tentacle records and the level scripts' boss calls.** New, no
conflict: tentacle_info (0x110 bytes, Character * at +0x80: `mov eax, [esi]` with esi = 0x248c3a8 =
0x248c328 + 0x80, stride 0x110, end 0x248c7e8, 0x4ee2d2-0x4ee303; +0xbc compared with -1 in
tentacle_begin_sink 0x4c6c56) and tentacle_beast_tentacles[4] (sum.symbols uint8[0x440]); the loop in
ls_sewerboss_frame ends at 0x248c768 = the array's end (sum.symbols names that address
tentacle_beast_grab_timer, the next object). tiger/serpent/phoenix/ghost_set_solo take the rider
(`mov esi, [esp + 8]` then `[esi + 0x320]`, tiger_set_solo 0x49c7c1); tentacle_begin_sink(record, ms)
(0x4ee1fb pushes 7000 and the record); ghost_minions_deactivate, evil_joseph_destroy_effects and
tentacle_beast_battle_update take nothing (no pushes at 0x4f03dc / 0x4ef38a / 0x4ee0e5);
evil_joseph_combat_enabled (0x248b9b4) / evil_urath_combat_enabled (0x248bd0c) are byte stores of 1.
machival_combat_enabled was already declared (decomp41).

**D2402 (engine/ai/ai.h). ai_state::predict_owner_position.** 0x4ca650 is thiscall on cur_character + 0x324
(`lea ecx, [edx + 0x324]`, 0x4f3454) with (vector3 *out, float seconds) (`push 0x3f000000 / push ecx` =
&local, 0x4f3447-0x4f3453); sum.symbols "(out, seconds)". The result is the position the guard walks to
(character_ai_add_goal9 gets &out, 0x4f3470).

**D2403 (summoner/summoner.h). GF_NO_INPUT, the script input lock, 0x5fc014.** script_disable_player_input
(0x432f50) / script_enable_player_input (0x432f70) take no arguments (no pushes before the calls at
0x4f3483 / 0x4f307b); sum.symbols: game_flags |= / &= ~0x10000, named GF_NO_INPUT here. 0x5fc014 is a
byte that no instruction writes (`tools/asm.mjs grep 0x5fc014`: one reader, ls_catacombs_frame 0x4fe1e6)
in .bss (zero at load), next to the console command objects 0x5fbfe8-0x5fc018: game_debug_5fc014, name
ours. 0x5fbf19 is decomp42's cheat_gold_5fbf19 (lsf_catacombs_boss_dead reads it at 0x4fe0a0).

**D2404 (level_script_catacombs.cpp). ls_catacombs_ball1_lobj / _ball2_lobj are lobj *.** sum.symbols:
void*. They are `lobjs + index * 0x70` (`lea ecx, [8*eax] / sub ecx, eax / shl ecx, 4 / add ecx, edx`,
0x4fdeb8) and the code reads their +0x14 position (0x4fe018), lobj's pos.

**D2405 (level_script_wolong_caverns.cpp, level_script_lpalaceint.cpp). Two timers are function-local
statics; the spike positions.** sum.symbols names ls_wolongcaverns_demilich_timer (0x257ea7c) and
ls_lpalaceint_sneak_frame_timer (0x257ea08) as globals; both are constructed under a guard bit
(0x257ea9c / 0x257ea0c, `test al, 1 / or dl, 1`) on the first pass with an atexit of an empty handler
(0x4f6de0 / 0x4f3510), which is VC6's function-local static: written as `static GameTimer timer;` inside
ls_wolongcaverns_demilich_sequence (after its early return: the guard test follows the step test,
0x4f6c4b) and at the top of ls_lpalaceint_sneak_frame (the guard test is its first instruction). The
unnamed .bss 0x257eab0-0x257eaf7 is six vector3 positions ($spike1a-c, $spike2a-c; navpoint_get_position
writes 0x257eab0 + 0xc * i, 0x4f670f-0x4f6773; trap 1 walks the first three, trap 2 the last three):
ls_wolongcaverns_spike_pos[6], name ours. Its empty initialiser and ls_wolongcaverns_ptrap_pos's are the two
init_empty functions; which is which is not visible (unsure).

**D2406 (level_script_jadetemple.cpp). One initialiser for two timers.** init_ls_jadetemple_timers
(0x4effc0) constructs 0x257e9c8 and then 0x257e9c0 in one function (no second initialiser entry), so the
two are one declaration with two declarators, fade timer first (likely).

**D2407 (the eight units). Fixed level indices.** Several helpers read other levels' flags with a
constant level (`push 0x9 / push "tb_killed"`, ...). Each unit #defines the ones it uses after the
level_script_table record at that index (read from 0x59f128 + 0x40 * i): 0 Wolong, 5 lenele1b, 9 sewer,
0xc IkaemosTopInt, 0xe KhosaniLab, 0x10 WolongCaverns, 0x18 LPalaceInt, 0x23 lenele1aa. No shared header
constant exists yet; when one is added these should move there.

## script_internal.cpp / console.cpp / norlsallocpool.cpp / new_multichat.cpp (decomp44)

**D2300 (engine/characterinfo/characterinfo.h). charinfo_find_move_any takes a third argument.** All four
callers push three (`push 1; push anim; push info`, `add esp, 0xc`: 0x4dc940-0x4dc953, 0x4dc9a1-0x4dc9b5,
0x4dd234-0x4dd24a, 0x4e49b1-0x4e49c0); the body reads only [esp+4] and [esp+0x14] (after three pushes:
the second argument). So the prototype the callers saw had a third parameter that the function ignores,
named loaded_only after charinfo_find_move's (both callers pass true). characterinfo.cpp's definition
must get the unused parameter (`bool loaded_only`), otherwise it becomes a second overload.

**D2301 (summoner/multi/multichat.h). chat_format_entry is (out, size, entry).** Both callers push three
arguments: chat_update_lines `push entry; push 0x200; push buf` (0x41e514-0x41e51f) and chat_history_pump
(0x4ab13f-0x4ab14b); the function reads the entry at [esp+0x10] after one push (0x487e21), the third
argument. The size is not read. multichat.cpp's definition must take `(char *out, int size, chat_entry *e)`
(it has no caller in src/decomp, so the old two-argument definition would compile as an overload).

**D2302 (engine/objects/living_entity.h). Character::set_ai_active takes an int.** The member tests the
whole dword (`mov eax, [esp+4]; test eax, eax`, 0x462200-0x46220a), and script_entry_start's show/hide
passes the action's int argument with no conversion (`mov ecx, [esi+0xc]; push ecx`, 0x4dd470-0x4dd47d);
a bool parameter would be `test al, al` and a `setne` at that caller. living_entity.cpp's definition must
become `void Character::set_ai_active(int on)` (a member definition without a declaration does not
compile, so this one is caught). The callers that pass true / false compile unchanged.

**D2303 (levelscripts/level_scripts.h). script_action: a navpoint at +0x04, a text at +0x08, signed
arguments.** script_parse_objects stores navpoint_find's result at +0x04 (0x4dc166) and the teleports read
nav->pos / heading / layer through it (0x4dd3db-0x4dd408); the text actions store the string pool's result
at +0x08 (0x4dc355) that say / play effect / pass go / follow pass on. `name` and `_unk08` stay as
the first members of anonymous unions (`nav`, `text`), so level_scripts_common.cpp compiles unchanged.
arg0c and arg10 become int: the actions test them with jl / jge (0x4dc939, 0x4dc999, 0x4dcd00, 0x4dce04),
which an unsigned field would not produce; nothing in src/decomp depends on the unsigned type.

**D2304 (levelscripts/script_internal.h). script_action_goto returns bool.** It ends with `mov al, 1` /
`xor al, al` (0x4dca7d, 0x4dca69) and script_entry_start keeps AL as the action's result (`mov bl, al`
at 0x4dd265 after the call at 0x4dd3bb). The decomp41 declaration said void (return unused there).

**D2305 (levelscripts/script_internal.h). script_parse_objects's second argument is a bool.** It is read
as a byte (`mov al, [esp + 0x39c]; test al, al`, 0x4dbfaa and 0x4dc312): true for a level script
(script_objects, end mode 0, the say texts remapped), false for a cutscene script. The level passes 1, cutscene_script_load
EBX (0x4ed2cf); level_scripts_common.cpp compiles unchanged.

**D2306 (engine/ai/ai.h). ai_state::goal_find's sixth argument is a vector3 pointer.** decomp39 already
names it the goal's position; script_entry_check_done passes `&navpoint->pos` (`add eax, 0x14; push eax`,
0x4dd80b-0x4dd80f). Every other caller in src/decomp passes 0, which stays valid.

**D2307 (levelscripts/level_scripts.h). navpoint +0x24 / +0x28.** The script navpoint parsers store a
float at +0x24 (the height above the floor found below, `fsub; fstp [esi+0x24]` at 0x4db67e, cleared to 0
when the navpoint is moved onto the floor) and the $Type: index at +0x28 (0x4db47a). Both become
anonymous unions with the old names first (`_unk24` / `ground_height`, `_unk28` / `type`).

**D2308 (summoner/interface/interface.h, new summoner/interface/new_multichat.h). The chat unit gets its
own header.** chat_input_cancel (0x41ed50) moves from interface.h to new_multichat.h with the rest of the
unit; interface.h includes new_multichat.h at the old place, so new_itemtrade.cpp compiles unchanged.
interface_screen_is_fullscreen was already declared by decomp42; chat_get_origin uses that declaration.

**D2309 (none). sum.symbols types char_types wrong.** char_types (0x25dae14) is int[256], not uint8[256]:
every access is `[4*eax + 0x25dae14]` (0x50a6ef, 0x50a9c4, 0x50aabd, 0x50ab88), and the next global,
current_char, is 0x400 bytes on (0x25db214).

**D2310 (several headers). New declarations, no conflict:** console.h the rest of the unit (ARG_QUOTED /
ARG_HEX / ARG_PLUS / ARG_MINUS / ARG_COMMA, TOKEN_*, CHAR_*, original_arg, rest_of_command_line,
current_command_count, Console_commands, debug_command_prefixed, ccmd_print_to_file, ccmd_script,
cmd_print_to_file, execute_command_line, cmd_script, load_script); new vsdk/vfile/linefile.h (LineFile,
0x1c bytes, and LineFile_open; file name ours); norlsallocpool.h Grow, GetUsed_thunk, SetOffset;
script_internal.h the unit's functions, SCRIPT_ACTION_*, NAVPOINT_TYPE_*, script_action_names,
script_navpoint_type_names, cutscene_saved_item / cutscene_saved_equipment (this unit's .bss, 0x2546760);
level_scripts.h the cutscene unit's globals this unit reads (cutscene_data with a one-field
cutscene_entry, cutscene_path_id, cutscene_cur_sequence, cutscene_navpoints, cutscene_script_objects,
cutscene_script_stringpool); ai.h ai_state::goals_remove_unmarked (0x4766c0).



## gr_glide.cpp (decomp45)

**D2500 (new vsdk/gr/glide/glide_min.h). Glide 2, through pointers.** Sum.exe imports nothing from Glide:
glide_load_dll (0x5473b0) calls LoadLibraryA("glide2x.dll") (0x5473b5) and GetProcAddress for 112
decorated __stdcall names ("_grDrawLine@8" .. "_guFogGenerateLinear@12"), storing each result in the
sum.symbols glide_gr* / glide_gu* dword. So the API is Glide 2.x, not Glide 3, and the unit calls every
entry point through its pointer (`call dword ptr [0x2f0e9c8]`). glide_min.h stands in for glide.h: the
pointer types follow the argument bytes of the decorated names (all 112 agree with the Glide 2
prototypes), and only the constants the call sites push are defined. The pointer globals are declared in
gr_glide.h (levelrender_draw_list_glide and gr_glide_render_character_mesh call them too).

**D2501 (vsdk/gr/glide/gr_glide.h). glide_poly_vertex_ptrs is GrVertex[25].** sum.symbols types 0x2f0ec08
as void*[32]. gr_glide_tmapper writes 0x3c-byte records from it (`add esi, 0x3c`, 0x54b37f; colour at
+0xc, x at +0, oow at +0x20, sow / tow at +0x24 / +0x28) and passes the address itself to
grDrawPolygonVertexList (0x54b390), and the next global (gr_glide_current_mode) is at 0x2f0f1e4 =
0x2f0ec08 + 25 * 0x3c. The name is kept.

**D2502 (vsdk/gr/gr.h, gr_glide.h). tcache_slot and glide_record.** gr_glide_tcache_set calls
tcache_slot__is_bound (0x54d4f0) and the mark-dirty / free functions call tcache_slot__invalidate_bitmap
(0x54d5a0) with ECX = &glide_tmus[i] (0x5495a2, 0x549706), and both read +0x8 .. +0x14 of it, the fields
that glide_record::load_section writes (bitmap +8, section +0xc, scales +0x10 / +0x14, 0x549b1c-0x549b38).
So glide_record (0x48) derives from a 0x18-byte tcache_slot (declared in gr.h: its members are in the
Direct3D texture cache object, vsdk\gr). tcache_slot gets no constructor: glide_record::glide_record
(0x549780) stores +0, +8, +0xc, +0x10, +0x14 itself; a base constructor, being out of line in another
object, would be called (opengl_record__ctor 0x54d470 sets the same five fields and is the D3D slot's).

**D2503 (vsdk/gr/glide/gr_glide.h). glide_tmus is glide_record[2].** sum.symbols: uint8[0x90]. The static
initialiser 0x5479a0 runs `vector constructor iterator`(0x2f0eb70, 0x48, 2, glide_record__ctor) and the
loops step 0x48 up to 0x2f0ec00 (0x547ac2-0x547acb).

**D2504 (gr_glide.cpp). The .bss gap after glide_hwconfig and the initialised fog table.** grSstQueryHardware
fills 0x2f0f210 (fbRam read at +8, nTexelfx at +0x10: 0x549e58, 0x549e82); the Glide 2 GrHwConfiguration
is 0x94 bytes, and 0x2f0f2a4-0x2f132a3 (0x4000 bytes) has no reference before glide_initialized at
0x2f132a4. It is written as an unreferenced ubyte[0x4000] (name ours). The fog table 0x5ad500 (64 bytes,
pushed to guFogGenerateLinear / grFogTable 0x54ba3d / 0x54ba4d) is in .data, all zero except its last five
bytes 3, 0x42, 0x81, 0xc0, 0xff, so it has an initialiser in the source; it is written with those values.

**D2505 (vsdk/gr/glide/gr_glide.h). The texture cache records.** glide_tcache_entry (0x10; tcache_init
0x547a68-0x547a72 sets +0 = -1, +4 = 0, +0xc = 0; `shl eax, 4` indexing) and glide_section (0x38; the
section loops step 0x38) are ours, from gr_glide_create_texture(_section), load_section and
get_section_rect (ushort x / y / w / h at +0x30 .. +0x36, 0x54952a-0x549556). +0x8 holds one dirty byte per
TMU (`mov byte ptr [edi+ebp+8], 0` with EDI = the TMU, 0x5495dd); gr_glide_create_texture clears both
with one `mov word ptr [edi+8], 0` (0x549390), written as a ushort store. Nothing in the unit sets a
dirty byte, so the reload loop in gr_glide_tcache_set is dead code.

**D2700 (level_script_lenele3d.cpp, level_script_iona_ext2.cpp). The folded leave hook 0x4f0f00 is defined in
lenele3d.cpp under its sum.symbols name.** 0x4f0f00 is `jmp snow_stop` and is the leave slot of both
level_script_table records 0x16 (IonaExt02) and 0x21 (lenele3d) (read from 0x59f128 + 0x40 * i, +0x0c). It
lies between ls_lenele3d_frame (0x4f0eb0) and ls_lenele3d_use (0x4f0f10), so the copy the linker kept is
lenele3d's; IonaExt02's identical hook (in iona_ext2.cpp, 0x4f4550-0x4f4905) was folded into it and has no
address of its own. Written as `void ls_ionaext02_leave() { snow_stop(); }` in level_script_lenele3d.cpp (the
sum.symbols name and the existing level_scripts.h declaration are kept); iona_ext2.cpp defines no leave. A
rename to ls_lenele3d_leave would describe the code better.

**D2701 (level_script_lenele2aa.cpp). empty_function (0x4f13e0) is defined in lenele2aa.cpp.** The bare RET
that every empty function of the game was folded into lies between lsf_lenele2aa_flag_vengence_done (ends
0x4f13d8) and ls_lenele2aa_action (0x4f13f0), inside this unit's code. Nothing else in src/decomp defines it
(summoner.h declares it). Which empty function of the unit it was is not visible (lenele2aa's leave slot is
0x4f13e0; likely). D907 / D703 (callers keep their own names for the folded copies) are unchanged.

**D2702 (engine/camera/camera.h). camera::cut_to_view_stub (0x479430), new.** thiscall on game_camera
(`mov ecx, 0x60a018`, 0x4efe6e), body `ret 0x14`: five dword arguments. lsf_lenele_flag_cart_played pushes
2.0, 2.0, 17.0, &orient, &pos (0x4efe55-0x4efe6d; pos and orient are navpoint_get_position's outputs), so
(vector3 *, matrix3 *, float, float, float). sum.symbols says "17.0, 4.0, 4.0" for the iona_ext callers;
lenele1aa passes 2.0, 2.0.

**D2703 (level_script_eleh.cpp). The four eleh choice bytes keep their sum.symbols names although they are
shifted.** ls_eleh_action sets 0x257e9ef for "JoinJoseph", 0x257e9ec for "TopFloor", 0x257e9ed for
"GroundFloor" and 0x257e9ee for "BottomFloor" (0x4f18a6-0x4f18fa); ls_eleh_fade_place_party_frame maps 9ec ->
entry 5, 9ed -> 3, else 4 (0x4f176a-0x4f178a); ls_eleh_frame's JoinJoseph block reads 9ef. sum.symbols calls
them go_ground / go_bottom / go_other / go_top. Written with the sum.symbols names and the real choice in a
comment; renames proposed in NOTES (go_top -> join_joseph, go_ground -> go_top, go_bottom -> go_ground,
go_other -> go_bottom).

**D2704 (level_script_courtyard.cpp). lenele1d is part of the courtyard unit; 0x4fab90 gets a name.**
ls_lenele1d_init / _frame / _use (0x4fa890-0x4faa57, module -) lie between ls_lenele1e_use (0x4fa850) and
ls_lenele1c_init (0x4faa60), both level_script_courtyard.cpp, and their literals (0x5a46ec-0x5a4760) lie
between lenele1e's / ikaemosu's and lenele1c's (0x5a4770-0x5a4834): one object file. ls_lenele1c_frame
(0x4fab80) is `jmp 0x4fab90`; 0x4fab90 (after 11 bytes of padding) is a function with no sum.symbols entry
(Flece joins at $fleceintro): lsf_lenele1c_flag_found_flece, name ours after its flag, declared at the top of
the .cpp (D1400).

**D2705 (the 13 units). More fixed level indices (as D2407).** Read from level_script_table: 2 "test" (eleh's
"extra_4", 0x4f15f8; events.tbl: "used by eleh script to keep door shut"), 0x17 KhosaniStrng ("open_yago",
0x4f4633), 0x18 LPalaceInt ("got_ring_of_darkness" 0x4efa95, "got_dagger_quest" 0x4f5efc), 9 sewer
("sewer_flag_found_crate", 0x4fb326), 0x1c Wolong2 ("got_ring_of_wind", 0x4fe913). Per-unit #defines.

**D2706 (engine/rendering/levelrender.h, engine/ai/boss_ai.h). New declarations, no conflict.** snow_start
(0x494ab0) / snow_stop (0x494b80): no pushes before the calls (0x4f0da0, 0x4f0f00, 0x4f1000, 0x4f4570), unit
not known. tiger_find_rider (0x49a010, returns eax = the last living entity whose char_info +0x10 is 0x57, 0
when none) and the bytes tiger_dismount_stage_reached / tiger_stage_trigger / tiger_combat (0x2452a98..a9a,
byte compares and stores in ls_templeint_frame / the cutscene ends).

## new_multi / summoner_cd / multiendgame / multiplayerdata / pathfinding_visibility (decomp47)

**D2600 (summoner_cd.cpp). The shipped Sum.exe carries a no-CD edit in cd_find_drive / cd_set_root; the
source is the unpatched reading.** Bytes no compiler emits: `0f 85 00 00 00 00` (jne +0) at 0x43e6b0 (after
GetDriveTypeA, `cmp eax, 5`) and 0x43e6e3 (after GetVolumeInformationA, `cmp eax, 1`), `75 00` at 0x43e872
(after the second GetDriveTypeA); `mov dl, bl / b2 2e / 90` at 0x43e695 (a `mov dl, bl` whose result is
overwritten, then `mov dl, '.'` and a nop filling the 3 bytes of `add dl, 0x41`) and `b0 2e 90` at 0x43e914
(`mov al, '.'`, nop: the 3 bytes of `add eax, 0x41`); the strings 0x594f50 `2e 5c 5c 00` and 0x594f70 `2e 5c
00 00` sit where "A:\" and "%c:\" belong (the pushed letter is still pushed at 0x43e917 and ignored). This
is on disk, unlike Timeslip's in-memory jge -> jmp at 0x43e9cb (docs/patches/timeslip.md). summoner_cd.cpp
writes `root[0] = 'A' + i`, "A:\", "%c:\" with `drive + 'A'`, and the three tests as `continue` (the first
two) and "skip result = i, break" (the third: `found` is stored only on the SUMMONER_1 path, so the found
block cannot loop back; the other matches jump into it with the store dropped). The original jump targets
are lost: unsure. A byte-matching build has to re-apply the patch. summoner/summoner.h: cd_select_drive
returns int (`mov eax, esi` = cd_set_root's result at 0x43e8d4; WinMain ignores it), new cd_find_drive,
cd_set_root, cd_root (sum.symbols char[64]; the .bss gap to 0x60ae58 is 0xdc bytes).

**D2601 (summoner/multi/multiendgame.h, multi.h). mp_player_disconnected is (reason, bool quit); LV_GAME is
{reason, id}; the declaration moved to multiendgame.h.** multi.h had (int id, int reason) with a note that the
first argument looks like a reason. The host path passes argument 1 to mp_send_player_left and argument 2 to
mp_leave_game (`mov eax, [esp+4] / push / call 0x481260`, `mov edx, [esp+0xc] / push / call 0x4812a0` at
0x481514-0x481526), the client path argument 1 to mp_show_disconnect_popup (0x48152f-0x481534), whose jump
table 0x481430 maps 1-6 to the strings.tbl reasons 225-230. mp_leave_game reads its argument as a byte (`mov
al, [esp+4] / test al, al`, 0x48130b): bool; the dword is passed through unchanged, so the parameter is bool
too. Only the type of the second parameter changes (all callers pass literal 0: gameplay.cpp, new_hud.cpp,
new_levelselect.cpp, new_playmenu.cpp compile unchanged). mp_send_player_removed stores argument 1 at
data + 3 and argument 2 at data + 7 (0x481474 / 0x481491) and is called as (reason, mp_local_player->id)
(0x48154d-0x481558); mp_handle_leave_game reads data[0] as the reason (compared with 1, 4, 6) and data[1] as
the id (mp_player_find_by_id, 0x4815cc-0x4815d2): sum.symbols' "(id, reason)" is reversed. mp_send_player_left's
4-byte word is the reason, not an id (sum.symbols "(id)"); its handler mp_handle_game_over never reads it.

**D2602 (engine/ai/pathfinding_visibility.h, pathfinding.h). Argument orders and the BSP node types.**
pathvis_insert_sorted reads the list index from [esp+8] before its pushes and the node from [esp+0xc] after
two (0x4b7df0-0x4b7e0c), the caller pushes the layer, then the node (0x4b7c92-0x4b7c9f): (node, layer), not
sum.symbols' "(list index, node)"; pathvis_insert_by_side the same (0x4b8190 / 0x4b81d2, caller 0x4b8122);
pathvis_bsp_contains takes (point, node): `mov esi, [esp+0xc]` after one push is argument 2, tested and
dispatched as the node, argument 1 is the point (0x4b86e1-0x4b86f2; caller 0x4b82e8 pushes the graph's BSP
then &mid). pf_bsp type 1 is not a leaf: pathvis_bsp_contains and pf_bsp_node_find (0x4a5ce1) compare the
point's x with the node point's x and go front / back (both when within 0.0001); pathfinding.h's comment
corrected. New: pathvis_edge (0x28, derived from seg2d: a / b at +0 / +4 are passed to
seg2d_segment_intersect and seg2d_point_left_of), the unit's .bss, PFN_COLLINEAR (0x2, set and cleared by
sort_points_clockwise, tested by determine_point_visibility), float_lerp_range (0x4a4f00), read_tsc (timer.h).

**D2603 (pathfinding_visibility.cpp). do_single_vis_check returns 1 for blocked; add_or_remove_edge is
unoptimised.** 0x4b825e returns 1 when 0x247ecdc is clear, 0x4b82f8 / 0x4b832b return 1 when the midpoint is
in the BSP or the segment crosses an edge, the end returns 0; determine_point_visibility sets PFN_VISIBLE
when the result is 0 (`cmp ebp, ebx / jne`, 0x4b8455-0x4b8465). The name (sum.symbols) is kept, the header
says what the value means. 0x4b7e80 has an ebp frame, reloads every variable from memory, `imul eax, eax,
0x28` and the `xor reg, reg / test / jne` of the list macros' while (0) (0x4b7f37, 0x4b80d3, 0x4b8110):
written between `#pragma optimize("", off)` / `on` (guarded by _MSC_VER for the mingw check). Its locals are
declared in frame order (first = highest address, as objects.cpp).

**D2604 (multiendgame.cpp). The kick functions 0x471df0-0x471fef stay out.** mp_remove_player,
mp_send_removed_notice and mp_handle_kick_pl read strings.tbl 233-234 of the `// multiendgame.cpp` block, but
they follow multitrade.cpp at 0x471df0 and multiendgame.cpp is the contiguous block 0x4811a0-0x481673
(static initialiser entry 238 at its front). One translation unit's code is contiguous in this link, so they
are another file (or multitrade.cpp's tail; source-map.md leaves it open). Not written here.

**D2605 (summoner/multi/multi.h). mp_player +0x08 addr, +0x54 display_name; flags and packet names.**
+0x08 (8 bytes) is the net address: nw_addr_equal(&p->addr, &to->addr, 1) in mp_send_net_info (0x4af69d-0x4af6a2)
and the two dwords of mp_server_addr copied there in mp_handle_net_info (0x4af798-0x4af7a6). +0x54 (16 bytes)
receives gr_string_truncate_ellipsis(p + 0x54, name, 50, -1) in mp_add_new_player (0x4af637-0x4af645). No
pilot used _unk08 / _unk54. New macros (names ours): MP_PF_DATA_RECEIVED 4 (`or [eax+0x10], 4` after PLY_DAT,
0x4afbaf / 0x4afbcb), MP_PF_TRACKED 0x20 (`test al, 0x20` before tracker_end_game, 0x4812be), MP_PF_INFO_SENT
0x40000000 (0x4af37f), MP_PF_LEAVING 0x80000000 (0x4814e9 / 0x4814f4, ui_screen_stop_player 0x41e392);
MP_NET_INF .. MP_LV_GAME 5-9 (mp_packet_table, the send counters 0x595250 / 0x59526c / 0x595288 / 0x5952a4 /
0x5952c0 = 0x5951b8 + 0x1c * type + 0xc); MP_ADD_FLOAT (the float copy through a stack temporary at
0x4af8c6-0x4af8d0). multi.h also declares mp_player_alloc (0x441610), mp_player_disconnect (0x441660),
tracker_end_game (0x4aba20), mp_server_addr / mp_server_player_id, and includes the two new unit headers
multiplayerdata.h and multiendgame.h. vsdk/net/net.h: nw_NewCloseSocket(int *) (the callers push
&mp_player::socket and store -1 afterwards, 0x481565-0x48157a) and nw_addr_equal (0 = equal, 0x519394).

## the interface manager, game/unnamed_interface_0x411270.cpp (decomp48; the file name is a placeholder)

**D2800 (summoner/interface/interface.h). party_cycle returns bool.** Was `void party_cycle(int)`. Both
exits set EAX: `mov eax, 1` after party_select_member in the multiplayer branch (0x4133bf), `xor eax, eax`
when the leader did not change (0x4133ca), and `cmp edi, ecx / setne al` (the old leader against
party_leader, 0x41344f-0x413453) at the end of the single-player branch. The callers (new_hud.cpp
hud_handle_keys, new_spell.cpp) ignore the result; a return-type change cannot form an overload, so no call
needed changing.

**D2801 (interface.h). statscreen_show_next_member / statscreen_show_prev_member take bool skip_summon.**
Were `(int a)`. Both pass their argument unchanged (`mov eax, [esp+4] / push eax`, 0x413320, 0x413340) to
party_find_next_member / party_find_prev_member, which read it as a byte (`mov al, [esp+0x10] / test al,
al`, 0x413241, 0x4132d1) and then add CF_SUMMON (0x100) to the skip mask. An int parameter passed to a bool
one would need a `test / setne` conversion, which is not there. The calls (new_inventory.cpp,
new_status.cpp) pass 1 and are unchanged; there is no other definition.

**D2802 (interface.h). ui_bitmap_scaled takes gr::Mode.** Was `gr_mode`. The body pushes its fourth
argument straight on to gr_bitmap_stretched (`mov edx, [esp+0x24] / push edx`, 0x41450e), whose parameter is
gr::Mode (D2001); a gr_mode parameter would need gr::Mode::Mode(gr_mode) (0x506810, out of line, D2002),
which the function does not call. The callers pass gr_bitmap_wrap_mode (a gr::Mode) and compile
unchanged; there is no other definition.

**D2803 (interface.h, new_inventory.h, game/new_inventory.cpp). ccmd_show_ui and cmd_show_ui belong to the
interface manager.** The object (0x5bd780) lies in the manager's .bss (between ui_choice_popup_flags 0x5bd77c
and ui_choice_popup_mouse_mode 0x5bd78c), its strings (0x58bfcc-0x58c054) follow the manager's literals
("v%d.%02d" 0x58bfc0) and come before new_inventory.cpp's first .data object (0x58c080), and its initialiser
(0x414c80 / 0x414c90, static_initializers entry 49) and cmd_show_ui (0x414cb0) end the manager's code
before new_inventory.cpp's first initialiser (0x414d30). The definitions move to the end of the manager's
file (the initialiser's position puts the object after the last function and before cmd_show_ui), and the
cmd_show_ui prototype moves from new_inventory.h to interface.h (declared once).

**D2804 (vsdk/geom/vfx.h). vfx_instance::lights (+0x1c), vfx_base::light_tracks (+0x24) and the new class
vfx_light_desc.** ui_3d_begin (0x412400) walks the circular list from `[vfx + 0x1c]` through `[esi + 0x28]`
until it is back at the head (0x412579-0x412583), reads the light's offset (floats +0x08..+0x10), colour
bytes (+0x20..+0x22) and range (`movsx edx, word ptr [esi+0x24]`, times 1/256), and calls the thiscall
vfx_light_desc__sample (0x51fdc0, ret 8) with `base->+0x24 + 0x70 * index` and reported_frametime; the base
is effect_get_base(id_bits >> 9) (0x412483-0x412489). interface_frame clears bit 2 of the instance's first
dword (`and dword ptr [eax], -5`, 0x4119f9). The fields are added to the existing partial classes (nothing
in the tree takes their sizeof or embeds them); vfx.h now includes vsdk/math/vector.h for the vector3 field.

**D2805 (levelscripts/level_scripts.h). cutscene_entry::flags (+4).** interface_frame ends the cutscene on
Esc only when `test byte ptr [eax+4], 1` is clear (0x4119dd), cutscene_data being the loaded entry; bit 0 is
+NoAbort in symbols/types.h. Appended to the partial struct.

**D2806 (interface.h). ui_bitmap_suffix_tga / ui_bitmap_suffix are defined in the interface manager.** The
two arrays (0x58bda4, 0x58bdac) point to "640.tga" / "1024.tga" / "640" / "1024" at 0x58bf10-0x58bf2f,
inside the manager's literal pool (between the message popup names 0x58bed4 and the cursor names 0x58bf30).
VC6 puts a unit's string literals after its own initialised data, so the arrays are the manager's first
.data objects; the previous unit's pool ends with "rt" at 0x58bda0. The manager's .data therefore starts at
0x58bda4 (docs/sum-exe/source-map.md says 0x58bdb4).

**D2807 (interface.h). status_character is the manager's.** The header said new_status.cpp; the dword
(0x5bd810) lies in the manager's .bss and interface_init_global / interface_load_all / statscreen_refresh_anim
write it. No other unit defines it.

**D2808 (gr.h, effect_mem.h, levelrender.h, summoner.h, new_itemtrade.h, interface.h). New declarations, no
conflict.** light_set_all_enabled (0x51e100, `push 0` / `push 1`), light_delete (0x51e540),
lights_set_max_per_object (0x51f530), lights_max_per_object / light_ambient_r/g/b (0x5a8900-0x5a890c),
gr_fov_to_vertical (0x50e5e0, float in, ST0 out), vfx_instance_free (0x51d7b0) and vfx_load (0x51dd00, (name,
1), result stored as the instance), overhead_label_slots_clear (0x4872b0), cutscene_cleanup (0x436620),
itemtrade_gold_input_active (0x5c1590, already defined by new_itemtrade.cpp),
worldmap_load_overlay / worldmap_frame (key in, key out) /
worldmap_draw_locations (0x42f320 / 0x42f500 / 0x42fa80), item_info_popup_load_bitmaps (0x4185a0), and the
manager's own functions and globals. elevator_popup_frame is called from new_multi.h (decomp47), not redeclared.

## decomp49: file_packfile.cpp, mouse (vsdk/mouse.cpp), exceptionhandler (vsdk/exceptionhandler.cpp), multi_door.cpp, new_int_multi_update.cpp

**D2900 (vsdk/vfile/packfile/file_packfile.h). packfile_add returns int.** decomp42 declared it void (level_load
ignores the result). It returns 1 for an already loaded packfile (`mov eax, 1` at 0x53449c), 0 when the file
is not found or there is no room (0x53458c), and otherwise the result of packfile_load_cd / packfile_load_hdd
(tail at 0x534558 / 0x534576, EAX kept). Only the return type changes: the one definition is new
(file_packfile.cpp) and gameplay.cpp's three calls are statements, so no call or definition needed a change.

**D2901 (file_packfile.h). Packfile / PackfileEntry layouts differ from symbols/types.h.** packfile_add stores
+0x20 as a dword (`sete cl` / `mov [esi+0x20], ecx`, 0x53453b) and clears the dword +0x24 (0x53453e): types.h
has `uint8 flags; uint8 _pad[7]`; the header keeps two ints (`flags` is still tested as a byte everywhere, which
is how VC6 tests `int & 1`). +0x30 is not the directory size (types.h, the sum.symbols comment of
packfile_check_header) but header dword 3, the size of the whole .vpp: packfile_read_whole_file compares the
caller's buffer size with it (0x534927) and freads that many bytes of the .vpp from offset 0 (0x534972); named
`file_size` (docs/sum-exe/vsdk-packfile.md already calls it "total size"). PackfileEntry::hash is int (signed
`cdq / idiv` at 0x533f1f and 0x534619; packfile_hash returns abs) and ::size is unsigned (`shr esi, 0xb` after
the 0x7ff round-up, 0x5340fb). hdd_file / cd_file are FILE * (fclose / fread / fseek on them). types.h is the C
reference and was not edited; the differences are noted for its maintainer.

**D2902 (vsdk/os/mouse.h, vsdk/gr/directx/gr_direct3d.h). clamp_int moves to mouse.h and is defined in
mouse.cpp.** gr_direct3d.h declared it under "not in a written unit yet" (module "-"). The function
(0x501450-0x501468) follows the mouse unit's implicit mouse_button constructor (0x501440) and precedes
frametime.cpp's first initialiser (0x501470, table entry 0x5887cc right after the mouse unit's 0x5887c4 /
0x5887c8), so it is the mouse object's last function (possibly an out-of-line header function the object
emitted). The declaration moved to mouse.h; gr_direct3d.h includes vsdk/os/mouse.h so gr.cpp and
gr_direct3d.cpp still see it. mouse.h includes only vsdk.h and vsdk/os/timestamp.h (no windows.h or dinput.h,
which would clash with d3d_min.h).

**D2903 (mouse.h). DirectInput is never used in v1.40.** mouse_use_cursor (0x5a5734) is initialised to 1 in
.data and its only writer is mouse_init (`mov dword ptr [0x5a5734], 1`, 0x500e59); mouse_di_init returns -1 at
once when it is 1 (0x500e96). So DirectInputCreateA (import 0x57e02c) is never called, mouse_di_read is dead,
and mouse_update always takes the GetCursorPos path. The mouse-look re-centring is dead as well:
keep_mouse_centered (0x257f031) is written only by mouse_keep_centered_enable / _disable, and mouse_hidden only
by mouse_set_hidden / mouse_set_visible, none of which has a caller. Comment only.

**D2904 (mouse, exceptionhandler). File names are placeholders.** Neither unit has an assert or a path string
(neither calls DebugError). The mouse unit is written as vsdk/mouse.cpp (sum.symbols tag vsdk\os\mouse) and the
crash reporter as vsdk/exceptionhandler.cpp (tag "exceptionhandler", the name of Bruce Dawson's
ExceptionHandler.cpp, whose strings and structure the object matches); both file comments say so. The
headers vsdk/os/mouse.h and vsdk/os/exceptionhandler.h keep their names.

**D2905 (exceptionhandler). The report helpers are file-static.** hprintf, RecordModuleList, ShowModuleInfo,
PrintTime, RecordSystemInformation, GetExceptionDescription and GetFilePart are called only from this unit
(direct calls, no other references), so the .cpp makes them static and exceptionhandler.h keeps only
RecordExceptionInfo. `static` is ours (it does not change VC6's code). RecordExceptionInfo returns
EXCEPTION_CONTINUE_SEARCH (0) on every path (0x541a99, 0x541ea9): WinMain's __except body is never entered and
the exception continues to the system's unhandled-exception handling after errorlog.txt is written; the
summoner.cpp comment "the result stays -1 then" describes a path that is not taken. Comment only.

**D2906 (os.h, file.h, multi.h, new_int_multi_update.h, file_packfile.h, mouse.h). New declarations, no
conflict.** os_foreground (0x530c10, returns the byte is_main_wnd_active); file_get_filename (0x513160,
(out, path)), install_path (0x26b4168) and cd_root_path (0x26b627c, char[256]) in file.h (summoner.cpp's
LoadSettings has a local install_path[260], which only shadows the global); MP_DOOR_TG 0x28 / MP_DOR_PCK 0x29
and the five other multi_door.cpp functions in multi.h (send counters 0x595624 / 0x595640 = mp_packet_table[t]
+0x0c); the version check's remaining functions and globals (version_check_need_registry 0x58bd34 and
version_check_need_fetch 0x58bd38, names ours); the packfile unit's records, globals and functions; the mouse
unit's globals (those of windows / dinput types stay in the .cpp) and functions. door_find_by_mesh,
version_check_run, version_check_status_text, packfile_read_whole_file, packfile_find_in_packfile and the nine
old mouse declarations keep their signatures.

## decomp50: localize, vsdk\net, vsdk\os (vsdk/unnamed_localize_0x503f60.cpp, vsdk/unnamed_net_0x517590.cpp, vsdk/unnamed_os_0x530b90.cpp, vsdk/unnamed_os_0x531bf0.cpp; all file names are placeholders)

**D3000 (vsdk/localize/localize.h). localize_remap_charset returns char *, not void.** Every exit leaves the
text pointer in EAX: `mov eax, [esp + 8]` for other languages (0x5051b0), `mov eax, esi` after the French and
German loops (0x50526a, 0x5052c9) and at the shared empty-string exit (0x505316). The five callers in src/decomp
(items.cpp, level_scripts.cpp, level_scripts_common.cpp, new_credits.cpp, script_internal.cpp) call it as a
statement, so they compile unchanged; no other definition exists. The other new declarations of the unit (every
localize_* in address order, localize_node, localize_language_info, the four tables, localize_language) did not
exist before; the existing ones keep their signatures.

**D3001 (localize unit). localize_pool_used is a char *, the write pointer into a 0xa000-byte text buffer.**
sum.symbols says data:int "bytes used". Its .data value is 0x025ce47c (the byte after localize_strings[1000]),
localize_set_string stores it into localize_strings[id] (0x504417), compares the length with
0x25d847d - pointer (0x50442e: the buffer ends at localize_item_table 0x25d847c) and advances it by length + 1
(0x5044a2). The buffer gets a name of ours (localize_text_buffer, 0x25ce47c); the symbol keeps its name.

**D3002 (localize.h). Typed tables.** localize_topic / script / quest / item_table are `localize_node *[0xfd]`
(sum.symbols void*[0xfd]): localize_table_add allocates 12-byte nodes and stores hash +0, text +4, next +8
(0x504118-0x50412d). localize_language_names is `localize_language_info[3]` {name[0x21], code[0x21]}
(sum.symbols char[3][0x42]): localize_get_language_code_buf copies from 0x5a5c59 + 0x42 * language (0x505170),
and the .data holds "gr" / "fr" at +0x21 of the German / French entries. The second table at 0x5a5d00
(char[3][20]: "\"Farewell.\"", "\"Leb wohl.\"", "\"Au revoir.\"", indexed with lea eax, [eax + 4*eax] at
0x504ccf) and the flags 0x5a5d3c (init 1, English), 0x25d8874 (German), 0x25d887c (pools created) get names of
ours.

**D3003 (localize unit). XSTR is the conditional form of D1701 in this unit.** The /O2 object still emits the
English literal of each XSTR next to the used key: "Golem" at 0x5a5eac (no reference) before the key 0x5a5eb4,
the same for the other eleven summon names and "Spirit of Laharah" 0x5a5fcc before the key "Spirit Of Laharah"
0x5a5fe0. The unit #undefs XSTR and uses `(0 ? (char *)(text) : localize_strings[id])`; localize.h's lvalue
XSTR stays (other units take its address).

**D3004 (vsdk/net/net.h). Packet buffer, reliable header and reliable socket layouts.** sum.symbols types them
as byte arrays. nw_recv_buffers (0x26b63f0) is `nw_packet_buffer[5]`: the stride is 0x1050c (the
*7, *29, *3, <<6, +1, *3, *4 sequence at 0x5175ab-0x5175c3), nw_init_networking initialises them up to
0x2707d2c (0x518a13), each holds 0x60 slots of 0x2b8 bytes {seq, size, addr[8], data[0x2a8]} and the counters
+0x10500 / +0x10504 / +0x10508 (nw_buffer_init 0x517730). The four buffers 0x2708130 / 0x2708398 / 0x2708600 /
0x2708868 are `reliable_header` (0x262 bytes, packed: type +0, seq +2, data_len +4, send_time +6, data +0xa;
nw_buffer_receive's size 0x262 and the 0x98-dword + word memset at 0x518f78); 0x2708130 has no sum.symbols
entry (nw_flush_buf, ours). nw_reliable_sockets is `reliable_socket[10]` of 0xd2d bytes (packed): sbuffers +0,
ssequence +0x258, timesent +0x384, send_len +0x5dc, rbuffers +0x708, recv_len +0x960 (dword stores at
0x5183a0), rsequence +0xbb8, last received +0xce4, last sent +0xce8, status +0xcec, their / our sequence
+0xcf0 / +0xcf2, addr +0xcf4, pings[10] +0xcfc, ping_pos +0xd24 (byte), num_ping_samples +0xd25 (unsigned:
jb at 0x5181d9), mean_ping +0xd29 (net_get_reliable_socket_ping 0x51942e). net_addr {ip, port} (8 bytes,
host order) is new. The socket globals are `unsigned int` in the header (no winsock include); the .cpp uses
SOCKET, the same type.

**D3005 (net.h). Kept names whose sum.symbols comments are off.** nw_reliable_slot_status(type) returns 1
when the packet buffer of that type holds a packet (first_seq != -1 and <= last_seq, 0x51765b-0x51766d), not
"a connected socket". nw_send_heartbeat sends the IAMHERE packet (type 6, seq 0xfebd, 0x517d7b); the real
heartbeat (type 5) is built inline in nw_WorkReliable (0x5186e6). nw_send_reliable_reply's arguments are
(addr, seq, send_time) (0x517d2d-0x517d57), not (socket, seq, addr). nw_reliable_resend_time is, on the client,
the connect time that the LIMBO time-out counts from (set at 0x51916b). nw_addr_equal keeps its decomp47
`const void *` parameters (the callers pass mp_player::addr, a ubyte[8]).

**D3006 (vsdk/os/os.h). os_init's parameters, os_init_window's argument, the title size.** os_init copies
argument 1 to the mutex name and the window class and argument 2 to the title (0x530d16-0x530d87; called as
os_init("Volition", "Summoner")): the parameters are renamed (name, title), the types are unchanged.
os_init_window takes an int: os_init pushes 0 and pops it with the atexit argument (add esp, 8 at 0x530e34).
os_window_title is char[0x80], not 0x98: the critical section passed to InitializeCriticalSection is
0x2cbfe60. os_foreground keeps the decomp49 declaration (one line); the other declarations are new.

**D3007 (units). One os object 0x530b90-0x531bea that ends in unoptimised code; 0x531bf0 starts another.**
The CPU / sysinfo code and os_clipboard_get_text (0x5312c0-0x531bea) are /Od-style (frame pointer, locals
reloaded, no 16-byte function alignment: cpu_identify at 0x5312d6), but ccmd_sysinfo (0x2cbff00) lies in the
middle of the os .bss (0x2cbfce0-0x2cbff27), and the padding after os_seed_random is the compiler's nop fill,
not the linker's int3 fill seen after 0x531beb. So it is one object with optimisation switched off part way
(written as `#pragma optimize("", off)` before os_clipboard_get_text; intrinsics stay on, the strlen is
repne scasb). The -1 stubs 0x531bf0 / 0x531c00 follow the int3 gap and are optimised: a separate placeholder
file. timer_wait (0x530bf0, module vsdk\os) is in the os object, between os_find_window_callback and
os_get_main_wnd; its declaration stays in vsdk/os/timer.h.

**D3008 (memory.h). The three vsdk\os Mempool functions need no new code.** Mempool__ResetOffset (0x505540),
Mempool__Mempool (0x51dff0) and Mempool__getPoolSpaceLeft (0x51e010) are the out-of-line COMDATs of the inlines
in vsdk/os/memory.h (D12), emitted by the /Ob0 objects that call them (localize: localize_clear_level
0x504ee2; effect_mem.cpp). 0x505530 (vector3__vector3_default) is the localize object's empty Mempool() COMDAT
(both pool initialisers jump to it), folded with every other empty constructor.

**D3009 (net unit). FD_SET is <winsock.h>'s.** Both selects build the fd_set with two stores (count 1,
array[0] = socket; 0x5175f2, 0x518790): the WinSock 1.1 macro (no duplicate scan), matching WSAStartup(0x101).
The RAS structures are written out in the .cpp (Windows 95 sizes: RASCONN 0x19c, RASCONNSTATUS 0xa0, RASPPPIP
0x28) because the SDK's <ras.h> sizes depend on WINVER.

## the editor dialogs, the boot / load-game states and the frame end (decomp51; game/unnamed_editor_dialogs_0x430bc0.cpp, unnamed_states_0x431870.cpp, unnamed_frame_0x4323e0.cpp, all three file names placeholders)

**D3100 (vsdk/ui/ui_gadgets.h). ui_state_bitmap has an out-of-line constructor and destructor;
ui_listbox_entry is a class and ui_listbox embeds one at +0x2d4.** The functions sum.symbols calls
ui_static__dtor (0x431780), ui_button__dtor (0x4317e0) and ui_listbox__dtor (0x431950) are implicit
destructors: each calls ui_state_bitmap__dtor (0x52c950) on exactly the members the constructor builds, in
reverse order (ui_static +0x3a4, +0x38c; ui_button +0x3f4 .. +0x394), or ui_listbox_entry__dtor (0x52d560)
on +0x2d4, then ~ui_gadget (0x52c410), with an EH frame and no vtable store; they are emitted right after
their first use in the unit (editor_dialog_destroy's deletes; dtor_ui_listboxes_5faac0), which is where VC6
puts a COMDAT it generates. So ~ui_state_bitmap() and ~ui_listbox_entry() are declared (with the
constructors 0x52c930 / 0x52d540 that ui_button() / ui_listbox() call), and the three destructors are not
declared members. ui_listbox_entry (0x3c bytes, vtable 0x581e30 with one slot, set_text 0x52d5b0) gets the
fields add_entry writes (+0x1c indent, +0x20 wrap indent, +0x24 line spacing, +0x28 font, +0x30 text, +0x38
colours; 0x52dacb-0x52daeb) and the ring links +0x8 / +0xc (the list box's +0x2dc / +0x2e0 point to its own
+0x2d4 after the constructor, 0x52d606-0x52d60c). ui_listbox's `_unk2d0[0x50]` becomes `_unk2d0[4]`,
`entries`, `_unk310[0x10]` (LAYOUT_CHECK 0x320 unchanged). New members (decomp51): ui_listbox get_selected /
select_by_text / select_entry / find_by_text / add_entry / clear, ui_button::press, ui_gadget::set_focus.
No existing file defines a ui_button / ui_static / ui_listbox object, and all src/decomp files compile as
before.

**D3101 (ui_gadgets.h). ui_static +0x388 is `Color *colors`.** Was `_unk388[4]`. editor_dialog_show stores
`&editor_dialog_colors[0]` there next to font = -1 (`mov [edi+0x388], 0x5fa280`, 0x430f65, 0x431029). No
file used _unk388.

**D3102 (summoner.h). The calls with an argument to the shared bare rets get functions of their own.** The
load-game screen calls 0x40acc0 with the level name four times (`push; call 0x40acc0; add esp`, 0x432051,
0x432176, 0x432224, 0x432311) and screenshot_save calls 0x4f13e0 with its message (0x43264f); both are
declared without parameters (empty_frame_hook_40acc0, D57; empty_function). As in D1507, each is written as
an empty function of the unit that calls it (loadgame_debug_hook, screenshot_debug_hook; names ours,
confidence unsure); identical-code folding would merge them into one bare ret, so they get no address.

**D3103 (vsdk/os/key.h). KEY_DEBUGGED is 0x8000 in PollInputEvents.** PollInputEvents masks the key with
`and ah, 0x7f` before comparing it with KEY_LAPOSTRO (0x432789) and has a case 0x800e (0x4327d9, key_flush),
but key_mark adds 0x800, not 0x8000, for a key pressed with the debug key held (`or ch, 0x8`, 0x509508), and
no other modifier sets bit 15. The macro keeps the value the code tests (name ours); the debug Backspace
flush is unreachable. KEY_LAPOSTRO (0x29), KEY_SYSRQ (0xb7, key_on_keydown 0x50968b), key_flush and
key_get_press_count are new.

**D3104 (summoner.h). gr_info counters, game_status_message.** FrameEnd prints six counters as percentages of
gr_info_total_time (0x2396c90) and zeroes them, but nothing else writes gr_info_total_time (its seven
references are all in FrameEnd), so `gr_info` shows nothing in v1.40. The counters are declared extern with
names ours (owner unit not known; 0x24268f0 / 0x24268f4 may be the start of a levelrender vector array,
since levelrender_draw_list_opengl stores `[esi + 0x24268f4]` beside `[esi + 0x24268f8]`, 0x492313). The status
message (0x5fbf54) and its RealtimeTimer (0x5fbf48) have no writer either: FrameEnd only reads them, and
screenshot_save formats "Screenshot written to ..." into a local buffer that it passes to the empty hook.

**D3105 (summoner.h). game_change_level keeps `(const char *filename, void *arg)`.** The body copies the
second argument into level_script_name as a string (0x431af3-0x431b11), so `const char *` is likely the
original type, but new_mainmenu.cpp (184) and new_saveload.cpp (689) pass conversation_save_table elements
(`void *`), which would no longer convert; the definition casts instead. Change it together with those two
calls if the table gets a typed declaration.

**D3106 (summoner.h, snd.h, multi.h, objects.h, ui.h). New declarations, no conflict.** The three units'
functions and globals (summoner.h: editor_dialog, EDITOR_DIALOG_*, the state handlers, the frame-end
globals, GF_PAUSED 0x4 / GF_SCREENSHOT 0x40 / GF_NO_FLIP 0x8000), snd_pause_all(int, int) (only the second
argument reaches ds_pause_all, 0x5207a1) and snd_unpause_all, multi_set_netgame_state(bool send_now)
(0x440fb5-0x440ff1: mp_broadcast_packet when set, else the buffered send), party_inventory_reset (0x46b680),
ui_draw(void (*)()), ui_process(int, int), build_stamp_check / build_stamp_get_name(char *, int) (the caller
pushes 0x200, 0x431cd8), debug_noop_frame_end(int, int, int), the UI_EVENT_PRESSED (0xc) /
UI_EVENT_LIST_SELECT (0x18) / UI_EVENT_LIST_DBLCLICK (0x19) values the callbacks compare. editor_dialog_init
moved from the "other units" list into the editor dialog section (declared once).

**D3107 (source-map.md cuts). The three units' .data boundaries.** VC6 puts a unit's initialised data
before its own literal pool. build_stamp_pending (0x5930b8, initialised 1) follows the editor dialogs'
literals ("No" .. "OK", 0x59309c-0x5930b6), and Gamewide_show_net_stats (0x5931b0, initialised 1) follows
the states' last literal ("Load", 0x5931a8), so 0x431870 and 0x4323e0 start new units. player.cpp's first
object (hero_model_names, 0x5934d0) follows the frame-end pool, whose end holds the four ccmds' strings
(0x5933c0-0x5934c9): the ccmds 0x432f80-0x433280 belong to the frame-end unit. The editor dialog unit has no
initialised data, so its start (0x430bc0) rests on the initialiser run and .bss only.

## decomp52: the player input unit and the cutscene file (game/unnamed_input_0x4343a0.cpp, game/unnamed_cutscene_0x434900.cpp; both file names are placeholders)

**D3200 (summoner/unnamed_cutscene_0x434900.h, levelscripts/level_scripts.h). The cutscene object 0x601e68 is
one class; its field symbols are macros.** init_cutscene_601e68 is `mov ecx, 0x601e68; jmp 0x4350b0`, and
cutscene__ctor (0x4350b0) is thiscall: the eh vector constructor of 50 cutscene_camera_sequence (0x70 bytes)
at this + 0x18, then +0x1644, +0, +4, +0x14, +0x15f8, +0x15fc, +0x1600[2], +0x1608[10], +0x1630, +0x1634[4],
+8, +0xc through this; the destructor 0x4351c0 (atexit 0x434930) runs the eh vector destructor. The members
load_files / bind_actors / load_sounds / unload / set_data_path / clear_data_path / load_data /
get_fog_distances / apply_vis_overrides take ECX and read the same offsets through it. So `class cutscene`
(0x1648 bytes, class, member and object names ours: the_cutscene) replaces the separate externs, and
cutscene_data (0x601e68), cutscene_path_id (+4), cutscene_time_offset (+8), sequence_start_time (+0x10),
cutscene_num_sequences (+0x14), cutscene_sequences (+0x18), cutscene_cur_sequence (+0x15f8),
cutscene_cur_fade_event (+0x15fa), cutscene_sequence_started (+0x15fc) and cutscene_fx_handles (+0x1608)
become macros onto it (as D2205 for game_camera): separate externs inside the object would have no
definition. level_scripts.h's head-only `struct cutscene_entry` (const char *name, flags) becomes the full
0x140-byte class with its constructor (0x4349b0) in the cutscene header, which level_scripts.h now includes;
the string fields are `char *` (cutscene_parse_table strcpy's into them). script_internal.cpp and
unnamed_interface_0x411270.cpp compile unchanged.

**D3201 (units). The cutscene file starts at 0x434900, not 0x4349b0, and switches optimisation off.** The
optimised static initialisers 0x434900-0x4349a5 (entries 111-113) construct the cutscene object 0x601e68,
cutscene_entries 0x6034c8 and cutscene_alloc_pool 0x5fccc0. Those three lie inside one .bss block with
variables that only the unoptimised code uses: cutscene_saved_actors 0x5fcce8 (cutscene_play / cleanup) sits
between the pool and the object, ccmd_play_cutscene 0x6034b8 (initialised by the unoptimised 0x4374b0) between
the object and the entries, and 0x609f08-0x609f38 after them. One object's .bss is contiguous in this link,
so the three objects are the cutscene file's; an object with both optimised and unoptimised code is
`#pragma optimize("", off)` part way (as D3007 for the os object): the three definitions come first, then the
pragma. The padding follows the function (nop after 0x4349a5, int3 after 0x4349b0's body). The input unit
keeps 0x4343a0-0x4348f6 (.bss 0x5fcca8-0x5fccb8, .data 0x593948-0x593999). Whether the input code is the same
object as the cutscene file (adjacent in .text, .data and .bss, both optimised up to 0x4349a5) is open; it is
written as its own placeholder file.

**D3202 (summoner/summoner.h). The cutscene and input declarations moved to the two unit headers.**
cutscene_play, cutscene_cleanup, cutscene_sequence_update, cutscene_events_frame, cutscene_play_escape,
cutscene_parse_table and cutscene_shutdown_stub (summoner.h) went to unnamed_cutscene_0x434900.h,
player_input_frame to unnamed_input_0x4343a0.h; summoner.h includes both. No signature changed.

**D3203 (engine/camera/camera_sequence.h). The camera sequence classes.** The base vtable 0x581ae4 has the
scalar deleting destructor 0x493f40 and _purecall (0x56e903) in slots 1 and 2; the shot's vtable 0x581af0 has
0x493fa0, get_pos 0x4943d0 (this + 0x40) and get_orient 0x4943e0 (this + 0x4c). cutscene_camera_read_track
(0x493fd0) takes (File *, float start): cutscene::load_files pushes the running start, then &f
(0x434d65-0x434d8a), and the reader compares [esp + 0x28] with 0. The camera methods frame, set_input,
start_sequence, end_sequence (bool: `mov al, [esp + 8]` at 0x4797f8) and set_sequence_transform (ret 8) are
added to camera.h.

**D3204 (cutscene file). The two saved-fog names were swapped in sum.symbols. Update after merge: sum.symbols
now has 0x609f08 cutscene_saved_fog_near and 0x609f1c cutscene_saved_fog_far, and the source uses those; the
evidence below was written with the old labels.** cutscene_play stores fog_get_far_current into 0x609f1c (`saved_fog_near`, 0x4362e1) and
fog_get_near_current into 0x609f08 (`saved_fog_far`, 0x4362ec); cutscene_cleanup passes 0x609f08 first
(0x4368db-0x4368e2) to fog_set_distances (near, far) (0x486b10: [esp + 4] goes to fog_near_distance 0x59bd80,
per sum.symbols). The code is consistent; only the two labels are the wrong way round. The source uses the
sum.symbols names with a comment; renaming them is for symbols/sum.symbols.

**D3205 (input unit). combat_mode is defined in the input unit; player_input stays in player.cpp; 0x434650 is
profile_label's destructor COMDAT.** combat_mode (0x5fccb4, declared in player.h, defined nowhere so far) lies
between ccmd_slew (0x5fcca8) and player_mouse_held (0x5fccb6), so it is defined there; its users are other
units (hud, gameplay.cpp, savegame). player_input (0x5fcc98) stays in player.cpp, where it is written (it
lies before ccmd_slew). The bare RET 0x434650 (91 callers) between player_read_mouse_input and
camera_process_input is the inline ~profile_label emitted in this unit after its first user (the unwind
funclet 0x57c020 calls it), kept by identical-code folding; profile.h's out-of-line declaration is unchanged.

**D3206 (summoner/summoner.h). config_invert_camera and config_camera_speed are macros onto
Summoner_options** (+9 and +0x38; camera_process_input reads 0x60acd1 and 0x60ad00), as D604.

**D3207 (headers). New declarations, no conflicts:** key_down_time (key.h),
script_editor_no_window_open (summoner.h; PollInputEvents and frame_show_loading_icon are decomp51's), goals_mark_keep (ai_state),
serpent_target_stage / tiger_target_stage / phoenix_target_stage / serpent_set_cutscene_walk_anim (boss_ai.h),
fog_set_distances, level_vis_force_begin / reset / add / end, levelrender_fade_reset (levelrender.h),
fx_create_at_camera, fx_pause_for_cutscene, fx_resume_after_cutscene, fx_destroy_spell_effects (fx.h),
effects_release_cutscene_memory (effect_mem.h), snd_free (snd.h), file_remove_path (file.h),
cutscene_script_load / cutscene_script_unload (level_scripts.h; the load's return is not used, declared
void), movement_init (input unit header, unit not known), Snow_enabled (cutscene header, unit not known).

## decomp53: the multiplayer menu, lobby, state 6 / monster layout, effect stream, net session and packet units (placeholders)

**D3300 (engine/effects/effect_stream.h). effect_stream_queue_add's third argument is the target; effect_stream_start
returns int.** The header had (name, Mempool *pool, int arg) and void effect_stream_start(). effect_stream_queue_add
stores its third argument at entry +0x2c (`mov edx, [esp + 0x14]` after two pushes, 0x4404d2 / 0x4404d9) and its second
at +0x30 (0x4404f9 / 0x440503); +0x30 is the mempool every reader pushes (0x440739, 0x44098a, 0x440a0f), +0x2c the
object a streamed file is read into (`mov ecx, [4*eax + 0x64fe64]` / skeleton_read 0x440a41; the vim * store 0x440ad3).
So the order (name, pool, target) was right and only the type changes to void *. effect_stream_start returns 0 / 1 in
EAX on every path (0x440569, 0x440589, 0x440598, 0x4405b4, 0x4405c3, 0x4405ce). The one call of each in src/decomp
(effect_mem.cpp's `effect_stream_queue_add(b->name, pool, 0)` and the statement call of effect_stream_start) compile
unchanged. The rest of the unit (queue, buffer, state, the open / finish / next / flush steps) is new in the header.

**D3301 (summoner/multi/unnamed_netpackets_0x441820.h, unnamed_netsession_0x440b40.h). The last argument of
multi_send_packet / multi_send_packet_buffered is a flush flag, mp_broadcast_packet's is "skip the flagged player",
multi_process_client_status's is "reliable"; all bool.** multi.h declared `multi_send_packet(..., int reliable)`,
`mp_broadcast_packet(..., int reliable)` and `multi_process_client_status(mp_player *, int a)`. multi_send_packet
always sends with nw_NewSendReliable (0x442189, 0x442201); its fourth argument (`mov al, [esp + 0x34]; test al, al`,
0x4421e6) only decides whether the buffer goes out now. multi_send_packet_buffered is the same with nw_send
(0x442010). mp_broadcast_packet's third argument (`mov bl, [esp + 0x14]`, 0x441ab8) makes a player with
MP_PF_INFO_SENT lose the flag instead of getting the packet (0x441ad7-0x441aed); it always flushes. The client status
sender tests a byte (0x44119c) and picks the reliable or the buffered send. All existing calls pass 0 or 1 and
compile unchanged; there was no definition before. Parameter names changed only in the comments of the callers'
units (none edited).

**D3302 (summoner/multi/multi.h). mp_player is 0xbc bytes with GameTimers at +0x24 and +0x68.** multi.h had
`ubyte _unk20[8]` and no size. init_gametimer_array_651604 (0x440ba0) constructs +0x24, +0x64 and +0x68 of four
records 0xbc apart from 0x6515a0 (the implicit constructor inline, as the two list heads' initialisers 0x440b50 /
0x440be0 do at 0x651418 / 0x6514d8); mp_session_reset clears 0xbc dwords of the array (0x4414a7) and steps 0xbc
(0x4414f1). +0x24 is SetTimeout by mp_accept_join_request (0x4ab69b, `join_timer`), +0x68 is used by
mp_handle_swi_2dir (0x488659, `swi_timer`); names ours. `_unk20` is now an int. LAYOUT_CHECK 0xbc added. No code
used `_unk20`.

**D3303 (multi.h). mp_packet_info::flag is a byte.** It was an int. multi_process_packet compares it as a byte
(`cmp byte ptr [ebp + 0x5951c0], 1`, 0x441959), which VC6 does not emit for an int compare. Three pad bytes keep
+0x0c. +0x18 / +0x19 (rate_flags[0] / [1]) are documented: [0] the type has rate info (cmd_disp_rate's help lists
those, 0x442582), [1] its display is on (toggled at 0x44242e-0x442439, listed by the status at 0x44253c). No
written code read `flag`.

**D3304 (summoner/multi/unnamed_mpmenus_0x43eb00.h). mp_player_level_available's mask is int.** multi.h had
`unsigned int mask`. The function shifts it with `sar eax, 0x1a` (0x43eb83), signed. Its only written caller,
mp_level_available_to_all, passes mp_player::profile_flags (int). The callers mp_server_list_draw and
mp_handle_join_request are not written yet.

**D3305 (summoner/multi/unnamed_netsession_0x440b40.h). The netgame globals are one struct, mp_netgame (0x6513a8, 0x6c
bytes); the old names are macros.** mp_host_game clears 0x1b dwords from mp_netgame_name (`mov ecx, 0x1b; mov edi,
0x6513a8; rep stosd`, 0x43ede2-0x43edf1), exactly the bytes up to mp_update_timer (0x651414), so the fourteen
globals multi.h declared separately (name, password, level, flags, players, max players, difficulty, host level,
levels below / above, state, server address and id, level scale) and two unreferenced dwords (+0x34, +0x58) are one
object. As multisave.h does for pxo_cfg (`#define pxo_cfg_flags (pxo_cfg.flags)`), the old names stay as macros
(`#define mp_netgame_level (mp_netgame.level)`), so every existing use compiles unchanged, including
multiplayerdata.cpp's `sizeof(mp_server_addr)` (8). Type and object names ours; the memset is
`memset(&mp_netgame, 0, sizeof(mp_netgame))`.

**D3306 (game/unnamed_lobby_0x43ef30.cpp). irc_connect's -2 is a dead case.** state_mp_lobby_frame treats -2 like
1 (`cmp eax, -2; je`, 0x43f17b), but multi_chattracker.cpp's irc_connect returns only -1, 0 and 1. Comment only.

**D3307 (unnamed_netpackets_0x441820.h). The byte averages are crosswise.** mp_net_stats_update stores the average of
net_stats_sent_bytes_history into net_stats_avg_sent_bytes (0x441d73) and that of net_stats_recv_history into
net_stats_avg_recv_bytes (0x441d90). sum.symbols' comments say the opposite. Nothing reads either average (only
mp_reset_net_stats and mp_net_stats_update touch them), so the code keeps the sum.symbols names and the stores as
they are. Comment only; a note for the symbols maintainer.

**D3308 (game/unnamed_mlayout_0x43f9d0.cpp). monster_layout_load spawns every monster with the map name's char_info.**
charinfo_find_by_name is called once, on the map-name string (0x43ff07), and its result stays in EBP for every
living_entity_create (0x440111); the per-record name read into the same buffer (0x44001e) is used only to find a
type-7 party member. So load_mlayout creates its monsters with char_info charinfo_find_by_name(level filename)
(normally -1). Game bug of a debug command (state 6 and the layout editor are unreachable; the command is not).

**D3309 (source-map). 0x440b40-0x442380 is two objects; the cut 0x43ef20 / 0x43ef30 has data evidence.** Each
object's .data is its initialised variables, then its literals (the interface manager, the effect streamer and the
lobby all show this). The literals of mp_handle_gm_stat and mp_handle_ct_stat (0x595158-0x5951b7) are followed by the
variable mp_packet_table (0x5951b8), so a new object starts there; its literals follow the table (0x5958f0..: those
of multi_process_packet, the senders and disp_rate). The .bss agrees: 0x440b40-0x441817 use only 0x6513a8-0x65188f
(netgame block, timer, list heads, records), 0x441820-0x4425a6 only 0x651890-0x65e967 (receive buffers, send buffers,
statistics, ccmd_disp_rate) besides calls and the extern player list. Written as game/unnamed_netsession_0x440b40.cpp
and game/unnamed_netpackets_0x441820.cpp. Likewise the lobby's .data begins with two variables (0x594fcc = 1, no
reference; lobby_join_channel_index 0x594fd0 = -1) right after the menu unit's literals ("All_multi_levels",
0x594fb8), which supports the source map's "inferred" cut at 0x43ef30. The cut lobby / state 6 (0x43f9d0) still rests
on the initialiser run (the .bss has an 8-byte gap 0x60f1c8-0x60f1cf; no .data variable).

**D3310 (game/unnamed_netpackets_0x441820.cpp). multi_process_packet's type test is off by one; DMY_PCK has no
handler.** `cmp cl, 0x42; ja` (0x441869) rejects only types above 66, so type 0x42 indexes mp_packet_table[66], the
literal "multi_process_packet: Invalid packet type..." at 0x5958f0: its "handler" dword is "i_pr" (0x72705f69), its
flag byte 'o' (not 1), so a packet of type 0x42 from a listed player calls 0x72705f69. DMY_PCK (0x40) has handler 0
(0x5958bc) and is called the same way. Neither is checked; the sizes are not checked against the datagram length
either. Game bugs, comment only (written as the code is).

**D3311 (multi.h and the four new placeholder headers). Declarations moved out of multi.h, new ones added.** The
declarations of functions and globals these units define moved from multi.h to the unit headers
(unnamed_mpmenus_0x43eb00.h: mp_host_game, the level checks, All_multi_levels; unnamed_lobby_0x43ef30.h: lobby_channel
and every lobby_* global and function; unnamed_netsession_0x440b40.h: mp_player_list, mp_local_player, mp_net_port,
mp_player_alloc / _disconnect, mp_session_reset, mp_game_frame, multi_do_frame, mp_level_start, mp_set_local_status,
multi_process_client_status, multi_set_netgame_state (decomp51's declaration, `send_now`), pxo_read_config_thunk and the
netgame block; unnamed_netpackets_0x441820.h: mp_packet_table, multi_send_packet, mp_broadcast_packet, the two
forwards). multi.h includes the four headers at its end, so every unit that includes multi.h sees the same names;
the netsession and netpackets headers include multi.h themselves (guarded) because they need the complete mp_player.
MP_MAX_PLAYERS (4) is in multi.h. New, no conflict: MP_GM_STAT / MP_CT_STAT, MP_NUM_PACKET_TYPES, MP_PF_ANNOUNCED
(0x2: mp_host_update_session 0x440ddf, mp_handle_gm_stat 0x441081), mp_player_find_by_addr (address, check_port; the
callers push 0 or 1), mp_all_players_have_status, mp_broadcast_packet_3e, the object unit's mp_send_local_updates /
mp_send_monster_list / mp_send_new_lep / mp_net_handle_map_reset, the join / tracker unit's mp_join_timer_reset,
chat_history_pump, mp_unpause, tracker_init / _start_game / _frame and its globals (0x24794bc, 0x24794c0, 0x2479520,
0x2479620; names ours: mp_join_accept_player / _timer, tracker_game_channel / tracker_filter_channel),
gtrack_send_channel_query with its 0x28-byte request, mp_level_find_boss / mp_level_complete_check, the game list
state's mp_gamelist_init / _net_frame / _free, pxo_login_reply, and the 34 packet handlers of mp_packet_table that no
header had (all (data, sender), as the table's call 0x441970 passes them). Elsewhere: nw_NewReceiveReliable,
nw_NewSendReliable, net_get_reliable_socket_status (net.h, same object as decomp50's unit, module -); File::is_open,
get_async_status, write_matrix, read_matrix and file_get_ext (file.h); effects_base_is_resident (effect_mem.h);
ui_element::set_size, ui_push_group / ui_pop_group (ui.h); ui_listbox::select_by_data (ui_gadgets.h);
debug_return_zero, debug_noop_mp_frame, debug_noop_disp_rate (debug.h); pxochat_logged_in (new_pxochat.h, defined in
new_pxochat.cpp but not declared). living_entity.h keeps character_xp_death_penalty (defined in the net session unit).

**D3312 (game/unnamed_*_0x43eb00 .. 0x441820.cpp). File names are placeholders.** None of the six units has an assert
path, a strings.tbl block (no localize_strings read in 0x43eb00-0x4425a6) or a Summoner 2 counterpart name; all are
module "-" in sum.symbols. Written as game/unnamed_mpmenus_0x43eb00.cpp, unnamed_lobby_0x43ef30.cpp,
unnamed_mlayout_0x43f9d0.cpp, unnamed_effectstream_0x440400.cpp, unnamed_netsession_0x440b40.cpp and
unnamed_netpackets_0x441820.cpp; effect_stream.h keeps its name (engine/effects/, directory ours).

**D3400 (engine/ai/ai.h). ai_group +0x1c / +0x20, and ai_think_members / ai_think_enemies hold the other
side.** ai_group_collect_enemies (0x493ac0) writes the enemies it finds to group +0x20[n] (handles) and
counts them in +0x1c (`mov [ecx + 4*eax + 0x20], edx` / `inc [ecx + 0x1c]`, 0x493c97-0x493caf, 10 max): the
old `_unk1c[0x2c]` becomes num_enemies / enemies[10]. The same loop puts the enemy pointers into
ai_think_members (0x243e394) and the group's own members into ai_think_enemies (0x243e3c0,
`mov [4*edx + 0x243e3c0], eax` with living_from_handle(group members[i]), 0x493af4-0x493b13); the
sum.symbols names are kept and the swap is documented. ai_think_enemies is declared [6] (sum.symbols [5]):
a group holds six members and the unit's .bss ends at 0x243e3d8, where the camera key counts start.
ai_group_remove_member writes its -1 to members[j] after the shift loop with j == 6, i.e. to +0x1c
(`mov [eax + 4*edx + 4], -1` after the loop leaves edx = 6, 0x492d07): a game bug, kept.

**D3401 (engine/camera/camera_sequence.h). time_left and advance belong to a second sequence class.**
cutscene_camera_sequence_time_left (0x4943f0) returns [+0x14] + [+0x10] + [+0xc] - [+8] and _advance
(0x494400) blends between [+0x48]->eye_pos / view_orient (camera +0x1a4 / +0x284) and +0x4c / +0x58,
writing +0x18 / +0x24; in a cutscene shot +0x14 is the anim_track's key counts. camera_frame calls
time_left and camera_update_sequence calls advance only for state 1 (camera_sequence_get_state == 1,
0x478c3b, 0x4794bb); cutscene shots are state 0 (cutscene_camera_sequence__ctor pushes 0). The two
methods move to `camera_blend_sequence : camera_sequence` (0x7c bytes, names ours). Nothing in Sum.exe
constructs a state-1 sequence (camera_sequence__ctor has one caller), so it has no vtable in the binary.
No existing caller in src/decomp.

**D3402 (vsdk/ca/skeleton.h). anim_track +0x14..+0x20.** cutscene_camera_read_track hands each track two
0xc-byte arrays per position key (+0x14, +0x18, from cutscene_cam_keys12) and two 8-byte arrays per
rotation key (+0x1c, +0x20, from cutscene_cam_keys8) before anim_track__init_tangents (0x4940a4-0x4941b0).
`_unk10[0x1c]` becomes _unk10, pos_aux_a / _b (vector3 *), rot_aux_a / _b (cquat *), _unk24[8]; same size.
cutscene_camera_sequence::track is now an anim_track (was ubyte[0x2c]; the cutscene file does not touch it).

**D3403 (engine/ai/unnamed_serpent_0x495290.h, unnamed_phoenix_0x497ac0.h, unnamed_tiger_0x499e50.h,
boss_ai.h). Each rider boss's state is one object; the sum.symbols names are macros.** One static
initialiser per boss constructs all of its GameTimers (init_serpent_timers 0x4952a0: 13, init_phoenix_timers
0x497cd0: 11, init_tiger_timers 0x499e60: 7), the rule of D1900. serpent_boss_load clears 0x190 dwords from
0x2451d98 (`rep stosd`, 0x4959cc), which ends exactly at ccmd_serpent_solo (0x24523d8): serpent_boss
0x2451d98-0x24523d7. The phoenix (0x2452420-0x24529f7) and tiger (0x2452a90-0x2452c97) objects are cut the
same way (stage first, up to the next console object); their first 0x140 / 0x30 bytes repeat the serpent's
field order (stage, target stage, four flags, the anim remap, offense points). The externs of boss_ai.h
(serpent_stage, serpent_target_stage, serpent_handle, serpent_timer_attack, serpent_part_busy,
serpent_part_move, serpent_threat_table, phoenix_stage, phoenix_target_stage, phoenix_cur_attack,
phoenix_threat_table, tiger_stage, tiger_target_stage, tiger_dismount_stage_reached, tiger_stage_trigger,
tiger_combat, tiger_threat_table, tiger_cur_move) became macros in the unit headers, which boss_ai.h now
includes after its guard; boss_ai.cpp, the cutscene file and the level scripts compile unchanged.
serpent_handle is handles[0] of a three-handle array (the loops run from 0x24522ac over num_extra_parts + 1
entries, serpent_part_index 0x4975ba); serpent_whip_reach (0x2452160) is attacks[2][0].range_max. The
record of "$OffenseNN" tags (0x1c bytes) is shared by serpent and phoenix: serpent_offense_point in
boss_ai.h.

**D3404 (engine/combat/combat.h; applied at merge). living_combat::update takes a move id, not a frame
time.** Every caller passes an int: boss_get_attack_move_id's result (ai_combat_ranged_attack,
`push eax`, 0x4be82b) or 0xb8 (tiger_combat_update, `push 0xb8`, 0x49b78b), and update_boss_attack
compares it with +0x18 (move_variant, 0x46837c). The declaration (float frametime, combat.cpp's
definition) is left as it is because changing it means changing combat.cpp; tiger_combat_update passes
the int through the float parameter (*(float *)&id). To do when combat.cpp is touched: `bool update(int
move_id)` and `update_boss_attack(int move_id)`.

**D3405 (ai.h, boss_ai.h). Return types from the binary.** ai_state::serpent_seat_rider, phoenix_seat_rider
and tiger_seat_rider return bool (`mov al, 1` / `xor al, al`, 0x495f1d, 0x4983aa, 0x49afbd; were void);
tiger_move_in_range returns int (`mov eax, 1` / `xor eax, eax`, 0x49c6f2; was bool). ai_state::combat_approach
(0x4bda50) returns bool (tiger_wander_update tests al, 0x49bcb0). The callers (boss_ai.cpp) ignore or
convert the results; no definition existed. ai_state +0x68 is the cast point (phoenix_cast_special writes
Character +0x38c..+0x394; phoenix_special_strike passes +0x398 to spell_place_ground_effect): `_unk68[0x14]`
becomes cast_point, cast_arg, _unk78[4].

**D3406 (units). The six cuts of 0x492b00-0x49c83a hold.** Each starts with its static initialisers and
owns one .bss / .data block: AI groups 0x492b00 (entries 263-264), camera sequences 0x493e50 (265-268,
.bss 0x243e3d8-0x2444597), snow 0x4946a0 (269-272: the console commands end the snow unit, as
source-map.md says), serpent 0x495290 (273-277), phoenix 0x497ac0 (278-282), tiger 0x499e50 (283-287;
pathfinding.cpp starts at 0x49c840). No assert path, no strings.tbl read: all six are placeholder files.
snow_start / snow_stop moved from levelrender.h to the snow unit header (levelrender.h includes it);
Snow_enabled moved there from the cutscene header (which includes it).

## decomp56: Pyrul, Ghost Rider, decals, AI combat (0x4b87c0-0x4c0245)

**D3600 (engine/ai/ai.h). ai_state::ghost_seat_rider returns bool.** It sets AL on every path (`mov al, 1`
0x4ba9ea / 0x4baa50, `xor al, al` 0x4baa58), like the serpent / tiger / phoenix seat functions (D3405). The
only caller, boss_ai_check_stage, ignores the value, so no call changes.

**D3601 (vsdk/math/ix.h). plane gets plane(const vector3 *n, const vector3 *p) and an empty plane().**
0x4a42e0 (sum.symbols plane__set_normal_point_b) has set_normal_point's body but returns this (`mov eax,
ecx`), and its callers pass EAX straight on to projector_record::add_plane / set (0x4bc157-0x4bc15c,
0x4bc1a6-0x4bc1b1): a constructor of a temporary. The empty default constructor keeps `plane p;`
(gametarget.cpp) compiling.

**D3602 (engine/ai/ai.h). ai_state +0x20 is vector3 approach_pos and +0x2c int approach_layer** (were
`int _unk20; ubyte _unk24[0xc]`). ai_combat_start_approach (0x4bd9c9-0x4bd9e9) and
ai_chase_target_until_in_range (0x4be109-0x4be134) store the target's position and layer there. No other
src/decomp file uses the old names.

**D3603 (engine/ai/ai.h). 0x4bce80 is an ai_state member.** sum.symbols calls it charinfo_within_attack_radius,
but every caller passes the AI record in ECX (0x4bd2cb, 0x4bdc1f, 0x4be6c7, 0x4be7dd) and +0x34 is
ai_state::attack_range; it is declared ai_state::within_attack_radius (dist < attack_range + 0.2).

**D3604 (engine/ai/boss_ai.h). The Pyrul / Ghost Rider .bss cut is 0x24802e0, not 0x2480300.** Pyrul's
last move pointer is 0x24802dc (pyrul_init_move_ptrs) and ghost_init_move_ptrs writes 0x24802e0-0x24802f8,
so the ghost unit's .bss starts at 0x24802e0 (source-map.md gives 0x2480300, the first command object). The
ghost minion handles are three arrays of two (servants 0x24808b0, mages 0x24808b8, undead 0x24808c0):
ghost_revive_minions has three separate loops and ghost_choose_move_target picks one pair per summon spell.
The offense-point record type boss_offense_point (0x1c) is shared by both units.

**D3605 (engine/rendering/unnamed_decals_0x4bbe80.h, unnamed_projectors_0x4c0250.h; new placeholder
headers).** decal_record (0x128) embeds the projector record (0xe0) at +0x38; the projector keeps world
planes at +0x14 (add_plane 0x4c070b) and object-space planes at +0x80 (to_lobj_space 0x4c0740), which the
decal draw and projector_collect_lobj_faces clip against (0x4bc8ee, 0x4c0574). Flag words are bitfields
(lifetime : 29 at decal +0x24; plane count bits 1-3 at projector +0). The rings are vsdk.h's plist_* lists;
decal_recycle's body is expanded again in decals_draw_for_lobj and decals_recycle_unused.

**D3606 (engine/ai/ai.h). The AI combat unit uses decomp54's names.** ai_group +0x1c / +0x20 are
num_enemies / enemies (D3400): ai_combat_cast_frame reads them for the support candidates (the target's
group) and a party member's offensive candidates. combat_approach, turn_towards, follow_path,
alert_guards_of_target, combat_close_in / _melee_attack / _ranged_attack and choose_support_spell keep the
decomp54 declarations; living_combat::update(int move_id) is D3404 (the AI combat callers push
boss_get_attack_move_id's result, -1 or 0).
