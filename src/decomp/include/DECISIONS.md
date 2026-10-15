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
