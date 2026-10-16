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
(0x49de5a-0x49de66). vector2_segment_circle_intersect 0x5254d0 takes five arguments (a, b, p, margin, out t;
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
object, would be called (d3d_tcache_slot__ctor 0x54d470 sets the same five fields and is the D3D slot's).

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

**D3400 (engine/ai/ai.h). ai_group +0x1c / +0x20, and ai_think_enemies / ai_think_members hold the other
side.** ai_group_collect_enemies (0x493ac0) writes the enemies it finds to group +0x20[n] (handles) and
counts them in +0x1c (`mov [ecx + 4*eax + 0x20], edx` / `inc [ecx + 0x1c]`, 0x493c97-0x493caf, 10 max): the
old `_unk1c[0x2c]` becomes num_enemies / enemies[10]. The same loop puts the enemy pointers into
ai_think_enemies (0x243e394) and the group's own members into ai_think_members (0x243e3c0,
`mov [4*edx + 0x243e3c0], eax` with living_from_handle(group members[i]), 0x493af4-0x493b13); the
sum.symbols names are kept and the swap is documented. ai_think_members is declared [6] (sum.symbols [5]):
a group holds six members and the unit's .bss ends at 0x243e3d8, where the camera key counts start.
ai_group_remove_member writes its -1 to members[j] after the shift loop with j == 6, i.e. to +0x1c
(`mov [eax + 4*edx + 4], -1` after the loop leaves edx = 6, 0x492d07): a game bug, kept.

**D3401 (engine/camera/camera_sequence.h). time_left and advance belong to a second sequence class.**
camera_blend_sequence__time_left (0x4943f0) returns [+0x14] + [+0x10] + [+0xc] - [+8] and _advance
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
0x4a42e0 (sum.symbols plane__plane) has set_normal_point's body but returns this (`mov eax,
ecx`), and its callers pass EAX straight on to projector_record::add_plane / set (0x4bc157-0x4bc15c,
0x4bc1a6-0x4bc1b1): a constructor of a temporary. The empty default constructor keeps `plane p;`
(gametarget.cpp) compiling.

**D3602 (engine/ai/ai.h). ai_state +0x20 is vector3 approach_pos and +0x2c int approach_layer** (were
`int _unk20; ubyte _unk24[0xc]`). ai_combat_start_approach (0x4bd9c9-0x4bd9e9) and
ai_chase_target_until_in_range (0x4be109-0x4be134) store the target's position and layer there. No other
src/decomp file uses the old names.

**D3603 (engine/ai/ai.h). 0x4bce80 is an ai_state member.** sum.symbols calls it ai_state__within_attack_radius,
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

## decomp55: the script editor unit (game/unnamed_script_editor_0x4de0b0.cpp, placeholder name; 0x4de0b0-0x4ecf90, static initialisers 374-473, .data 0x5a08a8-0x5a1573, .bss 0x2546828-0x256b74f)

**D3500 (summoner/unnamed_script_editor_0x4de0b0.h). The hover and selection records are struct objects.**
0x2546bd0 (hover) and 0x255a008 (selection) are two 0x44-byte records {kind, size, pos, orient, object}
copied whole (`rep movsd` with ecx 0x11, sel = hover); sum.symbols' per-field names become macros on
script_editor_hover / script_editor_sel (type script_editor_pick, names ours). The mouse ray at 0x2552c08
is an ix_ray filled by gr_screen_to_world_ray.

**D3501 (same header). script_action_defaults.** The 33-entry 0xc-byte table built by the inline
constructor 0x4de410 holds arg0c, arg10, flag14, needs_nav (script_editor_default_action_navpoint picks a default navpoint)
and has_params (opens the parameter window). Field names ours.

**D3502. 0x254a8b0 is a matrix, not a position.** It is copied as 9 floats with the selection's orient and
restored the same way; sum.symbols' script_editor_saved_pos names it wrongly. The saved position is the
vector3 at 0x2563a58 (script_editor_saved_pos here, script_editor_saved_orient for 0x254a8b0).

**D3503. Two sum.symbols names are fields of gadgets.** script_editor_name_room 0x254e020 is
script_editor_save_name_inputbox (0x254daa0) +0x580, its max_length; script_editor_ambient_file_list
0x2564d04 is script_editor_ambient_listbox (0x2564a30) +0x2d4, its entry ring. Both stay as macros.

**D3504 (vsdk/ui/ui_gadgets.h). ui_inputbox +0x5ac is a character filter callback.** The editor stores
script_editor_decimal_filter_cb there and the inputbox key handler calls it before inserting a character;
the field (char_filter, name ours) replaces padding.

**D3505. sum.symbols names that describe other functions.** script_editor_add_camera_navpoint (0x4e6a30) adds a
"$camera%.3d" navpoint; script_editor_action_type_allowed (0x4e7a40) returns whether action type is
offered for a character; script_editor_delete_selected_action (0x4e7cd0) deletes the selected action;
script_editor_default_action_navpoint (0x4e8cf0) returns the default navpoint of a new action. Names kept (no rename
without a strings/path source), behaviour stated at each declaration.

**D3506 (vsdk/ui/ui_gadgets.h). ui_listbox::select_by_data returns bool.** It returns AL: false when no
entry has the data (0x52d8d1 / 0x52d8e2); the editor tests it. No existing definition or caller used the
old void result, so nothing else changed.

## decomp57: boss_ai.cpp's last function, handle slots, blob shadows, pathfinding geometry, avoidance (0x4a3580-0x4aa77f)

**D3700 (engine/ai/pathfinding.h). pf_wall derives from seg2d.** pf_layer_from_edges passes the wall
record itself as the segment of seg2d_point_near_middle (`push esi` at 0x4a6651 / 0x4a6678, esi =
&pf_walls[i]) and copies its first two words into a local seg2d for seg2d_point_left_of (0x4a669f). Same
layout (+0x00 a, +0x04 b), so the base class only adds the conversion; pathfinding.cpp compiles unchanged.

**D3701 (engine/ai/unnamed_avoidance_0x4a6710.h, new). avoid_prim is avoid_obstacle (0x88).**
avoid_prim_from_segment builds its primitive in a 0x88-byte frame (avoid_segment_hits_prim `sub esp, 0x88`
at 0x4a6f58) with the obstacle's offsets (+0x00 flags 0x21, +0x08 centre, +0x10 / +0x18 axes, +0x30 / +0x34
half sizes, +0x38 / +0x40 the end points), and avoid_prim_overlap reads both arguments through the same
offsets (+0x10, +0x18, +0x30, 0x4a6e2a-0x4a6f0a). decomp32's `struct avoid_prim { ubyte[0x78]; }` becomes
`typedef avoid_obstacle avoid_prim`. The obstacle's full layout: +0x20 / +0x28 the two extra axes of a
circle (avoid_add_primitive 0x4aa282-0x4aa299), +0x38 eight points (the static initialiser 0x4a6760
constructs 8 x 8 bytes at +0x38 of each 0x88 record), +0x78 the bounds (rect2d, written at 0x4aa339-0x4aa346)
- decomp32's points[10] overlapped the bounds.

**D3702 (unnamed_avoidance_0x4a6710.h). avoid_area_desc::owner is the layer.** avoid_area_add passes
desc +0x2c as avoid_add_primitive's sixth argument (0x4a9c75 / 0x4a9cc0), which stores it as the
obstacle's layer (word +0x06, 0x4a9f09); the fifth argument, 1 or 2, is the size class (+0x04). The field
keeps spells.cpp's name (comment only).

**D3703 (unnamed_avoidance_0x4a6710.h). Segment list layout: 30 segment refs at +0x18, 30 corners at
+0x270.** The static initialiser 0x4a67a0 runs `vector constructor iterator` with (0x2461020 - 0x258, size
0x14, count 30) and (0x2461020, size 0x44, count 30) per list (stride 0xa78 from 0x2460db0): segs at +0x18,
corners at +0x270. A corner is {pos +0x00, pushed +0x08, offset +0x10, pf_vis_node +0x18, offset_state
+0x40}: avoid_seglist_add_segment compares and writes the point at corner +0x00 (0x4a86b9, 0x4a8754), the
miter code writes +0x10 / +0x40 (0x4a8040-0x4a804c), avoid_clip_segments writes +0x08 and the node (+0x18
pos, +0x1c flags 0x200, +0x1e layer, +0x24 the link block; 0x4a92d0-0x4a932b). decomp32's corners[29] at
+0x288 was the node of corner 0 onwards; the node addresses are unchanged, so pathfinding.cpp's
`corners[k].node` still compiles to the same offsets. +0x08 is the polygon index (avoid_poly_get 0x4aa3e4),
+0x0c the segment count (short) and +0xa68 the bounds (0x4a8e53-0x4a8e8d). avoid_poly: +0x04 point count,
+0x06 segment-list index, +0x08 eight obstacle indices, +0x10 / +0x12 size / layer, +0x14 30 points
(initialiser 0x4a6720), +0x104 bounds (avoid_poly_from_obstacle 0x4a7320). The polygon copies are rep movsd
of 0x45 dwords (0x4a96d8, 0x4a9800): written as memcpy, since vector2 has a user operator= (D3706).

**D3704 (unnamed_avoidance_0x4a6710.h). avoid_area_free takes void *.** It only clears the first word
(0x4a7b84) and is called on area slots (avoid_area_remove) and on polygons (avoid_poly_release 0x4a7c9b,
avoid_create_obstacle_polys' failure paths). Name kept (sum.symbols).

**D3705 (unnamed_avoidance_0x4a6710.h). avoid_poly_points_in_box / _in_circle take (shape, other).** The
outline walked is the second argument's (ebx = [esp+0xc] at 0x4a70e1, +0x38 points, +0x02 count), the box
or circle the first (pushed last, 0x4a70f0). sum.symbols' "(polygon, box)" has them the other way round.

**D3706 (vsdk/math/vector.h). vector2 gets an empty default constructor and inline operator= / operator+.**
The three avoidance initialisers pass vector3's empty constructor 0x505530 to `vector constructor iterator`
for vector2 arrays (avoid_poly +0x14, avoid_obstacle +0x38, the segment refs and corners), which a type
without a constructor would not get; the folded empty function is vector3::vector3()'s, as for matrix43 /
quaternion (D500). vector2__assign (0x4aa3a0: copies x, y, `ret 4`, EAX not set) and vector2__operator_add
(0x4aa3c0: hidden result, `ret 8`) are called by avoid_add_primitive; they are inline members whose only
out-of-line copies are this unit's COMDATs (the avoidance and shadow units are built without inline
expansion: vector3's constructor 0x43b580 and operator= 0x413190 are called there too). No vector2 in the
tree is brace-initialised; every src/decomp file compiles unchanged.

**D3707 (engine/ai/pathfinding.h). pf_segment_hits_walls returns true when the segment is clear.** It
returns 1 at the end (0x4a53a9) and 0 on a wall crossing (seg2d_segment_intersect 1 or 4, 0x4a5280) or an
obstacle hit (0x4a538c); pf_straight_walk_clear returns false when it is false (0x4a5656). Name kept.

**D3708 (engine/ai/pathfinding.h). seg2d_line_intersect(seg, line, out, float bias) returns nothing.** The
fourth argument is added to the parameter as a float (`fadd dword [esp+0x10]` at 0x4a5c28; seg2d_closest_point
pushes 0, the same bits); EAX only holds `out` for the stores. Was `int ... int flags`; the only caller
(pathfinding.cpp) passes a literal 0 and ignores the result, so it compiles unchanged.

**D3709 (source-map.md data ranges).** The avoidance unit's .data is 0x59ce88-0x59d34c (four switches, then
its strings) and its .bss 0x2460cc0-0x2476bb7 (splines 0x2460cc0 .. the 500 link blocks 0x246c7a8, reset by
avoid_reset 0x4a9d87); the source-map row gives .data 0x59ce88-0x59cea4 and .bss to 0x246b92c. The
geometry unit has no .data. The shadow unit's .data starts at 0x59cd8c (see D3711).

**D3710 (engine/rendering/levelrender.h). lobj_raycast_draw_lists returns the hit count; the fifth argument
is a bool.** Every path ends with `mov eax, [edi]` (hit->num_hits) or `xor eax, eax`, and the fifth argument
is tested as a byte (`mov al, [esp+0x30]; test al, al` at 0x4a4c06). The fourth (`bool *blocked` at the
callers) is the draw-list stage byte (0, 1, 2; `mov byte [eax], 2` at 0x4a4d9c); the declaration keeps
bool * so the callers compile, the definition writes it through a char pointer. lobj_ray +0x30 is a float
(min_radius, compared with the mesh radius at 0x4a4c36): a union with the old _unk30.

**D3711 (unnamed_shadows_0x4a37d0.cpp). pf_debug_mode (.data 0x59cd8c) and pf_debug_path (.bss 0x245cd8c)
are defined in the shadow / obstacle-debug unit.** Both lie between the handle-slot unit's last items
(0x59cd88, 0x245cd78) and this unit's first (0x59cd90 / 0x245cd90), next to its other debug switches
(0x59cd90 and 0x59cd94 are also ints initialised to 1). Placement likely, not proven.

**D3712 (game/boss_ai.cpp). level_page_in_characters (0x4a3580) is boss_ai.cpp's last function.** sum.symbols
tags it boss_ai.cpp; it follows level_preload_effects with no gap, uses no data of its own, and the next
unit begins with its static initialiser at 0x4a3640 (entry 291, the handle slots, whose .bss follows
boss_ai.cpp's timer 0x245c9e0). decomp29 had left it out only because its task range ended at 0x4a3580.

## decomp58: the goal / door unit and the camera unit (game/unnamed_goals_doors_0x475180.cpp, game/unnamed_camera_0x4778b0.cpp; both file names placeholders)

**D3800 (unit boundaries 0x475180-0x479b5f).** Two objects. (1) 0x475180-0x4778ab: ai.cpp ends with
cast_effect_track_count (D1905, ret 0x475178) and its .data with its literals ("Ai_path_wandering_enabled"
ends 0x59b7c6); the next .data item is an initialised variable, ai_goal_next_id (0x59b7c8 = 1, read by
ai_goal_alloc and the savegame code), followed by this object's one literal, "WARNING: unknown service
request type -- %d\n" (0x59b7cc, mp_handle_srv_req). A variable after another object's literals starts a new
object (as D3309). No .bss; its .rdata floats 0x581a0c-0x581a24 follow ai.cpp's 0x5819fc-0x581a08. The mp
helpers, goals and doors could be several objects (none has other data); kept as one placeholder.
(2) 0x4778b0-0x479b59: opens with the static initialiser thunk of ccmd_zoom_limit (entry 229; 230 / 231
follow), .data 0x59b7fc-0x59b986 starts with its variables (mlook_sensitivity, camera_zoom_max, 1.5f), .bss
0x22e49b8-0x22e4b0c lies between ai.cpp's (to 0x22e49b7) and spells.cpp's (0x22e4b18); spells.cpp starts at
0x479b60 (entry 232, strings.tbl spells.cpp ids). The source map's spells.cpp tag for 0x479ad0
(camera_set_sequence_transform) is a stray. No path string or strings.tbl block names either file.

**D3801 (signatures changed; all callers compile unchanged).** camera::snap_to_target(bool, bool) (was
(int, int): `cmp al, 1` at 0x478e77 / 0x478e9e); camera::cycle_mode returns bool (AL at 0x479a6d / 0x479a89;
was void); camera::start_sequence(cutscene_camera_sequence *) (cutscene_camera_sequence_sample is called on
it directly and +0x08 is its fov; the one caller passes &cutscene_sequences[i]); ai_state::goal_remove
returns bool (`mov al, 1` / `xor al, al`, ret 4; was void); door_set_open(lobj *, float, bool, bool) and
door_toggle_state(lobj *, bool) (their flags are pushed through as dwords to door_start_move, which tests
bytes at 0x476f61 / 0x477030; were int); mp_dequantize_position(ushort, ushort, ushort) (`and eax,
0xffff` before fild at 0x4756f7; was short). Fields: camera::num_saved_states is uint (jae / jbe at
0x4797b9 / 0x4797d5), camera::sequence is camera_sequence * and camera::target obj * (were void *).

**D3802 (engine/camera/camera.h). CAMF_* names for the camera flag bits** (ours, from the doc's table).
camera_set_follow_flags clears 0x1, 0x2 and 0x10 and sets 0x2 from its first and 0x10 from its second
argument (`(b << 3 | a) << 1`, 0x479759-0x47977f); a running sequence ends with restore = !second argument
(sum.symbols says !a).

**D3803 (engine/ai/ai.h). AI_GF_* goal field bits and AI_GOAL_KEEP / LOST / DONE / SUSPEND** (names ours).
ai_goal_type_fields returns 4 target, 8 pos + layer, 0x10 spell, 0x20 the float argument (byte table
0x476008); ai_goal_find adds 1 (type), 2 (flags, compared without 0x30e) and 0x40 (skip the running goal)
and masks with its first argument; ai_goal_add passes 0x1f. ai_goal_evaluate's results 1 / 2 remove the
goal, 3 sets goal flag 8 (suspended), 0 clears it (ai_goals_validate).

**D3804 (engine/rendering/levelrender.h). door_info +0x54 is a bit field** (moving, opening, close_wait,
locked_default, then a 28-bit close_delay = ms + 1), in a union with the old `state` word (s3d.cpp and the
savegame code use the word). Evidence: door_start_move's `sete / and 1 / shl / or` merges and the delay store
`shl 4; xor (state & 0xf)` (0x476fa3-0x476fb2), door_update's `shr ecx, 4; dec ecx` (0x4770b9). +0x08 is the
move duration in seconds (union name `duration`); +0x48 the floor point (accessor floor_point(), no code).

**D3805 (game/unnamed_camera_0x4778b0.cpp). camera_frame's statics are of a vector type with an empty
destructor (cam_vec, ours).** The two statics (0x22e4ac0, 0x22e4ad0) are guarded (0x22e4a38 bits 0 / 1) and
register atexit destructors that are a bare RET (0x478d10 / 0x478d00); camera_frame has an EH frame (handler
0x57c528) whose state goes to 0 after the statics, for the local copy of the target point that is never
destroyed visibly. So the type has an inline empty destructor; vector3 cannot (its globals register no
atexit). Unsure: the type might be the original vector class of another header. The 0x84 bytes
0x22e4a3c-0x22e4abf are referenced by no instruction; kept as camera_unk_22e4a3c.

**D3806 (camera_frame / camera_update_sequence). State-1 sequences are camera_blend_sequence (D3401).**
time_left and advance are called on the running sequence when get_state() == 1; camera_free_sequence deletes
only state-1 sequences (the shots, state 0, live in the cutscene's array and are only dropped).

**D3807 (summoner/multi/multi.h). mp_packed_orient (0x14 bytes, ours) for matrix3_pack_orientation /
matrix3_unpack_orientation; mp_net_handle_map declared (int[1000], 0x22e94f4).** The map's .bss lies after
spells.cpp's, so it is not defined in the handle unit; its owner (probably the object unit 0x47f9a0, whose
mp_net_handle_map_reset fills it) is open. MP_NEW_MON 0x3e and MP_SRV_REQ 0x3f added.

**D3808 (engine/rendering/levelrender.h). 0x476890 is a function with no symbol: lobj_index_compare (ours),
`*a - *b` over ints**, the bsearch comparator of door_from_lobj and lobj_find_textured_record (`push
0x476890` at 0x476900 / 0x477485). It sits in the padding gap sum.symbols leaves before
name_ptr_stricmp_compare.



## decomp61: the vfile units, ui_button and ui_static (D4100-D4107)

**D4100 (vsdk/vfile/file.h). File::write and File::write_line return void (were int).** Neither sets EAX
on every path: write leaves the slot error (early out), 0 (len 0 or a failed fwrite) or the new byte count
(an `add` that happens to end in EAX); write_line leaves the error, 0 or fwrite's result. No caller in
Sum.exe reads EAX after `call 0x526410` / `call 0x5264e0` (grep of build/sum.asm), and every src/decomp
call discards the result. Calls are unchanged; the definitions are new (unnamed_vfile_rw_0x525720.cpp).

**D4101 (vsdk/vfile/file.h). file_find_node is name[0x28], attributes (+0x30), time (+0x34)** (was
name[0x2c], time). file_find_files writes the stack node's +0x30 (0 / 2 read-only / | 4 archive from
_finddata_t.attrib, 0x513494-0x5134b8) and +0x34 (time_write), and copies the node with 14 dwords (0x38
bytes). No src/decomp code uses sizeof(name) or +0x30.

**D4102 (vsdk/vfile/file.h). file_make_path returns char * (out, or NULL)** (was void). 0x512b30 returns
EAX = 0 for an invalid id and for an install path of 256 chars or more, and EAX = out (mov eax, edx) at the
end; file_make_cd_path is the same. The callers that ignore it compile unchanged.

**D4103 (vsdk/vfile/file.h). file_delete returns bool** (was void): AL = 0 when the file does not exist,
else `_unlink(path) == 0` (neg / sbb / inc, 0x5137c9). The callers ignore it.

**D4104 (vsdk/vfile/file.h). file_init(const char *, int) (was bool).** The packfile switch is tested as
a dword (`mov eax, [esp+0x110]; test eax, eax`, 0x512e29); the bool parameters of this unit are tested as
bytes (File::find_at `mov al, [esp+0x3c]; test al, al`). summoner.cpp's file_init(NULL, true) compiles
unchanged.

**D4105 (vsdk/vfile/file.h). File__is_absolute_path (0x513fb0) and File__find_free_slot (0x5140b0) are File
members (File::is_absolute_path, File::find_free_slot), keeping the sum.symbols address comments.** 0x513fb0
ends with `ret 4` and File::find calls it with ECX = this and no stack cleanup (0x5141de); File::open_found
loads ECX = this before calling 0x5140b0 (0x51453a `mov ecx, edi`). Both are thiscall functions that do not
use this. They were declared as free functions only in sum.symbols (no header had them).

**D4106 (File::read, open). packfile_fread is called with a fifth argument 0 (applied at merge: the prototype has a fifth `int unused`)** (0x5260b2 `push edx` with
edx = 0 before the four arguments, `add esp, 0x14`; again at 0x526145) although packfile_fread (0x5348a0)
reads only four. The original prototype probably had a fifth parameter (with a default); file_packfile.cpp's
definition has four. Not changed here (it would turn the existing definition into an overload); the call in
unnamed_vfile_rw_0x525720.cpp passes four, so its code will differ by the push until the prototype gets the
parameter (definition and declaration together).

**D4107 (vsdk/ui/ui_gadgets.h). ui_static +0x374..+0x380 are the state bitmap's clip size and offset
(bm_w, bm_h, bm_u, bm_v; ours), not the justification.** create stores w / h there and 0 / 0
(0x527f46-0x527f64); ui_static::draw passes them as the last four arguments of ui_state_bitmap::draw
(0x528125-0x528150), whose fifth and sixth go to gr::set_clip(x + u, y + v, w, h). The justification
stays at +0x3bc / +0x3c0 (window.cpp's parse_static, draw's switches at 0x5281e6 / 0x52822b). The
sum.symbols names set_justification (0x5280a0) and set_text_offset (0x5280c0) are kept on the two no-caller
setters of these fields, and the sum.symbols comments of ui_static__create / __set_justification /
__set_text_offset (+0x374 justification, +0x378 font, +0x37c / +0x380 text offset) are wrong. The text is
the char[0x100] at +0x274 (set_text), so ui_static's _unk274 is now text + the four ints.

## decomp59: projectors, lamp glows and the boss units 0x4c0250-0x4ca6ff

**D3900 (engine/rendering/unnamed_projectors_0x4c0250.h). The projector unit's declarations; the receiver
list.** The unit is written (game/unnamed_projectors_0x4c0250.cpp). New: geom_bsp_node (the 12-byte BSP node
of s3d_geom_block +0x04: three 10-bit normal fields scaled by 2/1023 - 1 (0x581ba4, 0x57e990), child bits 30 /
31, child index and face list in the third dword; projector_collect_lobj_faces 0x4c029a-0x4c0335,
0x4c03fc-0x4c0447), the four unit functions, and lobj_face_mark_offsets sized 8000 (0x2485564 up to the walk's
single-face word 0x24893e4; lamp_glows starts at 0x24893e8). decomp57's shadow_record (lobj_list[7],
lobj_count : 3) is kept: projector_collect_lobj_faces appends with the 3-bit count
(`and edx, -0x40; add edx, 0x40; ...; and edx, 0x1c0`, 0x4c05c0-0x4c05d0), so an eighth object goes to
+0xfc (decomp57's _unkfc) and a ninth wraps to +0xe0; the `cmp ecx, 0x10; jae` exit after it (0x4c05d2)
tests the old 3-bit count and never fires.

**D3901 (units). The eleven cuts of 0x4c0250-0x4ca6ff.** No path string and no strings.tbl read in the range:
all files are placeholders. Each boss unit opens with its static initialisers and owns one .bss / .data block
(initialiser entries 317-337, 0x5884f8-0x588548): projectors 0x4c0250 (no initialiser; .bss 0x2483564-0x24893e7),
lamp glows 0x4c0a40 (317), Machival 0x4c0c40 (318-319), Titus 0x4c20e0 (320-322), Evil Joseph 0x4c3470
(323-324; boss_target_list_find and boss_resurrect_full are its functions), Evil Urath 0x4c4170 (325-326; its
own boss_target_list_find_b), Murod 0x4c5140 (327), Murod Oni 0x4c5c20 (328), tentacle beast 0x4c6480 (329-332),
Giant Salamanka 0x4c8030 (333-335), Luminar 0x4c8d10 (336-337; ai_predict_owner_position 0x4ca650, without data,
ends it before level_scripts_common.cpp's entry 338 at 0x4ca700). source-map.md's hand cuts agree except for
the projector end (the plane::operator= COMDAT 0x4c0a20-0x4c0a3e belongs to it).

**D3902 (engine/rendering/unnamed_lamp_glow_0x4c0a40.h, new). lamp_glow and the lamp glow functions.**
lamp_glow (0x18: pos, size = radius * 5, alpha, a dword zeroed by lamp_glow_add) and lamp_glows[300] (sum.symbols).
lamp_glow_bitmap stays declared in levelrender.h (decomp57, for level_preload_effects); the new header includes
levelrender.h instead of declaring it again. The empty initialiser 0x4c0a50 is read as lamp_glows' construction
(vector3's empty user constructor).

**D3903 (engine/ai/boss_ai.h). tentacle_info gets its full layout; tentacle_slot.** init_tentacle_records
(0x4c6490) runs vector_constructor_iterator(record, 0x18, 5, tentacle_slot__ctor 0x4c64f0) and five GameTimer
constructors (+0xa8, +0xc0, +0xc4, +0xc8, +0x100) per record: the slots and timers are members with
constructors. The field offsets come from the unit (move +0x7c, flags +0x84 (1 submerged, 2 rising, 4 sinking),
target +0x8c, hits +0x90 / +0x94, death anim +0xa4, think timer +0xa8, grab tag +0xb8, victim +0xbc, thrown
+0xcc, throw velocity +0xd0, release HP +0xe0, reach +0xe4, swipe / squeeze points +0xe8 / +0xf4, sink timer
+0x100, hold / up times +0x104 / +0x108, HP mark +0x10c). The hit list has room for 4 (death_anim follows at
+0xa4). `character` and `victim` keep their names (level_script_sewers.cpp).

**D3904 (engine/ai/unnamed_giant_salamanka_0x4c8030.h, new; boss_ai.h). The Giant Salamanka state is one object.**
giant_salamanka_boss_load clears 0x178 dwords from 0x248c7d0 (`rep stosd`, 0x4c83b6), ending exactly at the
Luminar unit's .bss (0x248cdb0); init_giant_salamanka_timers constructs its GameTimers. As D3403 (serpent, tiger),
the struct giant_salamanka_boss gsal (0x5e0) carries the fields and the sum.symbols names become macros
(giant_salamanka_combat_enabled, _stage, _forced_stage, _stage2_reached / _ack, _stage3_reached / _ack,
_missile_tag, _threat_table, _attack_timer, _offense_point_count, _current_move, _target_handle,
_stage1/2/3_attacks). The six externs of boss_ai.h became comments; boss_ai.h includes the new header after its
guard (with the serpent / phoenix / tiger headers). boss_ai.cpp and level_script_khosani_lab.cpp compile
unchanged. The move rows are indexed from 0x248c844 = &moves[stage - 1] (giant_salamanka_ai_select_action
0x4c89ed-0x4c89fb), which is why sum.symbols speaks of "0x20 bytes from 0x248c844".

**D3905 (engine/ai/boss_ai.h). giant_salamanka_target_in_reach returns int.** `mov eax, 1` / `xor eax, eax`
(0x4c8cb7 / 0x4c8cbd), as tiger_move_in_range (D3405). Its only caller, boss_move_in_range_dispatch
(boss_ai.cpp), returns it as bool; no call changes.

**D3906 (engine/ai/ai.h, boss_ai.h, fx.h, levelrender.h, summoner/multi/multi.h). New declarations.** The
thiscall functions of the eight boss units are ai_state members (each is called with ECX = the AI record, or
calls go_idle / set_mode on ECX): a decomp59 block before `int mode;`. The cdecl functions and the unit globals
are a decomp59 section of boss_ai.h. mp_send_swi_boss (0x4888d0, (boss, mode, sub-mode): pushes at 0x4c1f79,
0x4c9a1f) goes to multi.h next to its handler; titus_rock_erupt_fx (0x48c9a0, (handle) -> effect, 0x4c326a) to
fx.h; levelrender_fx_char (0x22fb6d0) and its three tint floats (sum.symbols names) to levelrender.h. The
empty skin hook character_instance::ca_noop_2arg (0x51a880) is pushed a float (1.8f, 0x4ca4c1) as its second
argument by luminar_beam_hit; the declaration (int, int) is kept and the bits are passed, because changing it
means changing vsdk/character_instance.cpp (to do there: `(int bitmap, float scale)`).

**D3907 (engine/ai/ai.h). luminar_ai_select_action takes the target and returns nothing; predict_owner_position
is the Luminar unit's.** 0x4c9410 is `ret 4` with a pushed character (luminar_ai_idle pushes 0, the frame its
target) and leaves whatever EAX the last call left (the grab branch returns luminar_try_start_grab's AL). The
declared member ai_state::predict_owner_position (0x4ca650, decomp43) is defined in
game/unnamed_luminar_0x4c8d10.cpp.

**D3908 (names).** Move pointer names follow the table index: titus_move_<stage><index> as sum.symbols has them,
machival_move_N / evil_joseph_move_N / murod_oni_move_N for [0][N] (machival_move_melee and the sum.symbols
names kept). murod_oni_move_is_kick (sum.symbols) compares with murod_oni_move_3, not move 0; the name is kept.
evil_urath_unused_moves (0x248bd88, 0x140 bytes after the threat table, never accessed) is named for its size.
Names ending in an address (machival_248b030, titus_248b950, ...) are dwords the code writes but never reads,
or never touches.

**D4200 (units). 0x52bfe0-0x530b86 is four placeholder files.** No vsdk\ui path string, strings.tbl block or
Summoner 2 file covers the range (Summoner 2 has no UI code), so every file is `unnamed_*`. Cuts:
(1) 0x52bfe0 starts an object because window.cpp's two COMDATs (0x52bfa0, 0x52bfc0) end window.obj;
vsdk/unnamed_gadget_0x52bfe0.cpp = ui_gadget + ui_state_bitmap to 0x52ca53, no data, no EH.
(2) vsdk/unnamed_listbox_0x52ca60.cpp: .bss 0x2cbf584-0x2cbf623 (the line tables, after window.cpp's ""
literals ..0x2cbf583), .data 0x5a95e4 (after window.cpp's ..0x5a95e3), one int3-padded EH funclet block
0x57d510-0x57d556 after window.cpp's, and it ends with its COMDATs 0x52f400-0x52f444 (two scalar deleting
destructors, the text entry's implicit destructor). The cut between (1) and (2) is NOT proven (the gadget code
has nothing that could differ); one object 0x52bfe0-0x52f444 is the alternative.
(3) vsdk/unnamed_build_stamp_0x52f450.cpp: .data 0x5a9604-0x5a966f of its own; no .bss, no initialisers. It
may be the head of (4) (which has no .data to compare).
(4) vsdk/unnamed_ui_manager_0x52f570.cpp: the initialiser run 0x52f570-0x52f700 (table 0x588974-0x5889a0,
twelve entries; the next entry 0x531a80 is the os unit's), .bss 0x2cbf628-0x2cbfcd9, and the COMDATs at the
end (colour set constructors 0x5309c0-0x530b50, inline getters 0x530b70 / 0x530b80) before the os unit
(0x530b90). It calls COMDAT copies of inlines, so it is built without inline expansion.

**D4201 (vsdk/ui/ui_gadgets.h). The list box's entry head is a subclass with an inline constructor; the text
entry is a class.** ui_listbox() (0x52d5c0) stores entries.next / entries.prev (+0x2dc / +0x2e0) right after
the ui_listbox_entry constructor on +0x2d4 and before its own vtable (0x52d606-0x52d612), i.e. during member
construction: `ui_listbox_entry_head : ui_listbox_entry { ui_listbox_entry_head() { list_init(this); } }`
(name ours) is the member type. The implicit ~ui_listbox (0x431950) still calls ~ui_listbox_entry directly.
ui_listbox_text_entry (0x13c, vtable 0x581e2c, sum.symbols ui_listbox_text_entry__*) gets its constructor
0x52d480 and set_text 0x52d4a0; its destructor is implicit (0x52f440). All src/decomp uses of `entries`
(`.entries.next`, `e != &x.entries`) compile unchanged (derived-to-base conversion).

**D4202 (vsdk/ui/ui.h). The UI colour sets are structs.** The twelve initialisers (0x52f5d0-0x52f700) call
implicit constructors that construct each Color in turn (0x5309c0-0x530b50, nested: ui_colors5 = ui_colors3
then two Colors, ...); a `Color x[N]` would go through the vector constructor iterator. So: ui_colors2/3/4,
ui_window_color_set (sum.symbols' ui_window_colors__ctor; type name ours because the object has the name),
ui_colors5/6 (deriving from ui_colors3), ui_colors2_x3, _3_x2, _4_x4, _5_x3, _6_x2, ui_colors8 (members c0..,
g0..; names ours). ui_tooltip_colors is three Colors (constructor, ui_init and ui_tooltip::draw use [0]..[2];
sum.symbols says Color[4]). New declarations: ui_checkbox_colors, ui_inputbox_colors, ui_slider_colors,
ui_listbox_colors, ui_listbox_entry_colors. An inline `operator Color *()` (ours, a shim) keeps the existing
users compiling unchanged (`colors = ui_button_colors;`, summoner.cpp `ui_window_colors[2].set(...)`): an
/Ob0 unit would call it, so the new files use `&set.g0.c0` / `set.c2.set(...)`. To do: switch window.cpp,
ui_container.cpp, unnamed_ui_button/static and summoner.cpp to member access and drop the shim.

**D4203 (vsdk/ui/ui.h). ui_process(int key, bool clear_clicks).** The second argument is compared as a byte
(`cmp byte ptr [esp+0x38], 1`, 0x5308c2). Both callers pass 1 (game/unnamed_frame_0x4323e0.cpp,
game/unnamed_states_0x431870.cpp) and compile unchanged.

**D4204 (vsdk/ui/ui.h). The build stamp data.** build_stamp_checksum (int, 0x5a9604, compared signed after an
arithmetic shift), build_stamp_expiry (long = VC6 time_t, 0x5a9608), build_stamp_name (0x5a960c). The first
literal of the unit follows at 0x5a9654 (4-aligned), so the name array is 69 to 72 bytes; 72 is assumed.
build_stamp_get_name keeps its (char *, int) signature: the size is never read.

**D4205 (vsdk/ui/ui_gadgets.h). ui_listbox::select_by_text returns bool.** `mov al, 1` / `xor al, al`
(0x52d914 / 0x52d901), like select_by_data (D3506). Callers ignore the result and compile unchanged.

**D4206 (vsdk/ui/window.h, ui_container.h). get_mask_data / get_mask_buffer are inline members.** The only
copies (0x530b80, 0x530b70) are the last two functions of the UI manager object, after its colour set
COMDATs, and ui_draw is their only caller: COMDATs of header inlines in an /Ob0 unit, as D2869 / D1644
describe for other vsdk units.

**D4207 (vsdk/ui). Field names.** ui_gadget::_unk258 becomes hotspot (set_hotspot / hit_test); ui_state_bitmap's
private _data[0x18] becomes the public state / bm / next fields; ui_listbox's _unk274 / _unk2d0 / _unk310 become
flags, typeahead, typeahead_len, typeahead_timer, max_content_w, max_content_h, content_width,
content_height, user_selected, selected, hscroll_step, vscrollbar, hscrollbar, colors, wrap_width_cb;
ui_listbox_entry's _unk14 becomes width / height. No src/decomp file used the old names.

**D4208 (vsdk/ui/ui_gadgets.h). ui_listbox_entry::wrap_text(int width, int y, int (*width_cb)(int y, int
line_h)).** ui_listbox::layout_entry pushes the callback (+0x31c) first, then y, then the width (0x52cc82-
0x52cc92); the callback gets (y, line height) (0x52ca98-0x52ca9e) and its non-zero result replaces the width.
sum.symbols' "(width, callback, ...)" has the order wrong.

## decomp60: effect_mem.cpp's last functions, the light, attached-light, sound, EAX, quaternion and vector units (0x51d250-0x525710)

**D4000 (vfx.h, effect_mem.h). fx_rec48 is vfx_instance and fx_rec30 is vfx_light_desc.** The record
vfx_instance_create takes from vfx_instance_free_list (0x48 bytes, the 48 records effects_init_pools
threads) is the instance the rest of the code knows as vfx_instance: vfx_load stores the name index << 9
at +0 (the id_bits ai.cpp and the interface read), +0x1c is the list of 0x30-byte records that
ui_3d_begin walks as vfx_instance::lights (next +0x28), and vfx_instance_free hands those records to
vfx_node_free_light. vfx_instance now has the full layout (lists +0x18 / +0x1c / +0x20 / +0x28 / +0x2c, pos
+0x30, next / prev +0x40), vfx_light_desc gets offset +0x14 and prev +0x2c (0x30); effect_mem.h keeps the
old names as typedefs (`typedef vfx_instance fx_rec48; typedef vfx_light_desc fx_rec30;`). Their implicit
constructors are still 0x51df10 and the shared 0x51df60. effects_init_pools' stores to _unk18 / _unk1c /
_unk20 / _unk28 / flags are now nodes / lights / emitters / rec2c_list / id_bits (same offsets).

**D4001 (vfx.h). The 0x70-byte light track is vfx_light_track (name ours).** sum.symbols uses
vfx_light_desc for both records: vfx_light_desc__sample (0x51fdc0) is thiscall on the 0x30-byte instance
light, while vfx_light_desc__relocate / get_position / sample_34 / _40 / _4c / _58 are thiscall on the
0x70-byte track of the base (light_tracks + 0x70 * i; the sample reads its frame count at +0xc and its
channels at +0x10..+0x64). decomp48's vfx_light_desc (the instance light, with sample) is kept; the track is
a new class vfx_light_track whose members carry the vfx_light_desc__* addresses. The vfx_node_* functions
(create_light 0x51fb90, free_light 0x51fc80, update_light 0x51fcb0, get_scale 0x5203f0) are thiscall on the
instance light, so they are vfx_light_desc members. vfx_light_desc::sample keeps its `void *track`
parameter so unnamed_interface_0x411270.cpp compiles unchanged.

**D4002 (gr.h). gr_light is the attached-light layout, 0x98 bytes.** levelrender_sum_point_lights reads
world position +0x20, colour +0x2c, intensity +0x38, has_range +0x48, ranges +0x4c / +0x50: the fields
attached_light_create / attached_light__reset write (attached_light, 24 slots of 0x98 at 0x2cb45d0;
levelrender_lights[16] is the same type). The 400 records of 0x94 bytes at 0x2ca5de0 (light_reset_all's
0x39d0 dwords) are the light unit's `class light` (colour times intensity at +0x34, radii +0x2c / +0x30).
gr.h's comment and size are corrected (0x94 -> 0x98, `_unk54[0x40]` -> `[0x44]`); gr_light stays its own
declaration (levelrender.h forward-declares `struct gr_light`).

**D4003 (vfx.h). attached_light_create(pos, intensity, range_near, range_far, r, g, b).** The third
argument is stored as a float at +0x4c next to the range at +0x50 (0x5201c2), and has_range (+0x48) is set;
it was `int b`. living_entity_init passes 0, which converts to 0.0f (the same bits).

**D4004 (vector.h, fixed.h). float_max, float_min and the fixed-point helpers are inline.** Their only
out-of-line copies are COMDATs at the ends of objects that call them and were built without inline
expansion: float_max 0x50d960 (before clamp_float in the gr unit), float_min 0x51faf0 (after the light
unit's light__light), fixed8::set 0x51e040 and float_to_fixed8 0x51e060 (after effect_mem.cpp's record
constructors and Mempool members), fixed4_to_float 0x520360 and fixed8_to_float 0x520400 (among the
attached-light unit's COMDATs). vector3::dot (0x4b24f0) is added as an inline member for the same reason.

**D4005 (effect_mem.h). effects_load_preload_rfx returns int.** 0 at 0x51dc16, -1 when effect_base_load
fails (0x51dc1b); it was declared void. boss_ai.cpp calls it as a statement.

**D4006 (unnamed_lights_0x51e070.h). Light argument types from the stores.** light_set_all_enabled and
light_set_enabled AND their byte argument with 1 before the bitfield store (an int argument; was bool),
light_create_point and light_set_dynamic store theirs without the AND (bool; was `int flags`).
lights_shade_vertices tests its sixth and seventh arguments as bytes (bool dynamic, bool dynamic_only;
were int count / int a) and never reads the eighth. lights_select_nearest returns the selection count
(was void). The callers compile unchanged.

**D4007 (unit bounds). character_instance::render (0x520420) is a unit of its own.** It follows the
attached-light object's COMDAT block (0x520280-0x520413: the inline constructor, the track samplers,
attached_light_create_from_vfx, get_scale, fixed8_to_float) and precedes the sound unit's first static
initialiser (0x5204c0). Every object in this range ends with its inline functions (effect_mem.cpp, the
light, attached-light, sound, quaternion and vector units), and nothing in the attached-light unit calls
render, so it is not one of that unit's COMDATs: written as vsdk/unnamed_ca_render_0x520420.cpp (likely;
which file it belongs to is not known).

**D4008 (vsdk/snd/ds.h, ours). The DirectSound layer and EAX declarations.** The sound and EAX units call
ds_* (0x55c000-0x55dfe0, not written) and the EAX 2.0 IKsPropertySet interface; ds.h declares the ds_*
prototypes from these call sites and a minimal IKsPropertySet (C++ vtable: Release +0x08, Get +0x0c, Set
+0x10, QuerySupport +0x14), EAXLISTENERPROPERTIES (0x38) and the three GUIDs (shared .rdata 0x5832e0 /
0x5833b0 / 0x5833c0, not this unit's data), standing in for <dsound.h> / <eax.h> like d3d_min.h.

**D4009 (snd.h). snd_play_3d(handle, pos, bool looping, int a, int unused).** The third argument is
compared with 1 as a byte (0x521579) and chooses snd_play_looping or snd_play, and both it and the fourth
go to ds_play_3d's last two arguments; the fifth is never read. It was (handle, pos, float a, float b,
int c). The callers push (sound, pos, 0 / 1, 0, 0) (sound_play_3d_at, sound_emitter_play);
gamesound.cpp's `snd_play_3d(sound, pos, 0.0f, 0.0f, 0)` compiles unchanged (0.0f converts to false / 0)
and should become `(sound, pos, false, 0, 0)`. snd_init (0 / -1), snd_get_frequency (ds_get_frequency's
value or -1) and eax_load_props (1 / 0) return int (were void).

**D4010 (unit bounds). The sound and EAX functions are two objects.** The sound object ends with the
COMDATs Sound::is_used (0x521e10) and clamp_uint (0x521e20) after snd_set_eax_environment; the EAX
functions follow from 0x521e40. The two are built differently: sounds_parse_table's two "#Sounds End"
arguments are one literal (0x5a8954, string pooling), while eax_save_props / eax_load_props use 28
separate "%d\n" / "%f\n" literals (0x5a8ad8-0x5a8b48). Their .bss is contiguous (sound 0x2cb5410-0x2cbf487,
EAX 0x2cbf488-0x2cbf497). source-map.md's "may be the tail of the sound file" is answered: separate files.

**D4011 (quaternion.h). quaternion::to_matrix fills a matrix43.** 0x522bf0 writes twelve floats: the nine
rotation values and zeros at +0x24 / +0x28 / +0x2c (0x522cb0); to_matrix_b (0x522cc0) writes the nine
rotation values only. Every caller of to_matrix passes a matrix43 (matrix43::set, matrix43_interpolate,
..., matrix.cpp casts it to matrix *). The declaration keeps `matrix *` so matrix.cpp compiles unchanged;
the definition writes through a matrix43 pointer. To do: make the parameter matrix43 * together with
matrix.cpp's four calls.

**D4012 (quaternion.h). cquat normalisation uses an integer square.** cquat::normalized (0x523050) and
cquat::normalize (0x5232b0) compute `idiv` 0xfff8001 (16383^2) by the integer |q|^2 before FILD / FSQRT,
so the scale is the root of a whole number (0 for any cquat longer than one unit, 1 near one unit) and a
zero cquat divides by zero. Reproduced as written (game bug, NOTES.md).

**D4013 (headers). quaternion and cquat move to vsdk/math/quaternion.h.** The quaternion unit's header
(file name ours) holds both classes with their members; matrix.h includes it where quaternion was defined
(so every user still sees it) and skeleton.h loses its cquat (it includes matrix.h). decomp61's
make_identity declaration moved with the class.

## decomp64: containers, the experience table, the effect objects and the user tracker login (D4400-D4412)

**D4400 (units). Four placeholder units for 0x489730-0x48db2f.** No path string, no strings.tbl read and module
"-" for every function, so all four file names are placeholders. Cuts: (1) game/unnamed_containers_0x489730.cpp
0x489730-0x48992f: static initialiser entry 250 builds `containers` (0x2302fe0, right after multichat.cpp's
chat_history ring, which ends at 0x2302fdf), .data "backpack" / "leather bag" (0x59c230-0x59c247); (2)
game/unnamed_experience_0x489930.cpp 0x489930-0x489c3f: xp_level_table 0x2305420-0x2305617, "experience.tbl" and
"D" (0x59c248-0x59c25b); (3) game/unnamed_fx_0x489c40.cpp 0x489c40-0x48d7bf: entries 251 / 252 (fx_active_list,
fx_pool) open its code, .data 0x59c25c-0x59c357 ("sky" ends right where levelrender.cpp's level_light_scale
starts), .bss 0x2305618-0x2318e43; the .data order proves fx_obj_destroy and the blob shadow code are one file
("Destroying AA" 0x59c2a0, used at 0x489e0a, lies after shadow_blob_bms 0x59c260, written at 0x48a014); (4)
game/unnamed_utrack_0x48d7c0.cpp 0x48d7c0-0x48db2f: .bss 0x2318e48-0x231906f (after fx_count 0x2318e40), no
.data, then levelrender.cpp (0x48db30, .bss 0x2319070). Between (1), (2) and (3) there is neither a data gap nor
an out-of-order use (xp_level_table follows the pool exactly, fx_num_sky follows the table exactly): they may be
one file. The cuts follow the initialisers and the topics; this is open (NOTES.md).

**D4401 (engine/effects/fx.h). effect_object gets its constructor, destructor and virtual destroy; the +0xbc bit
field is split.** fx_obj__ctor (0x489d30) / fx_obj__dtor (0x489db0, stores vtable 0x581aac then jumps to
obj__dtor) / fx_obj_destroy (0x489dc0, the vtable's one slot, called through `call [edx]` by fx_destroy and
fx_create_instance). +0xbc: the signed 17-bit fx_flags (decomp37), then an unsigned 3-bit hit_count (`shr 0x11;
and 7` at 0x48aa63, 0x48c6dc; `add 0x20000` / `and 0xe0000` to count, 0xc0000 = 6 at 0x48c7e0), a 4-bit field
that is cleared and an 8-bit field set to -1 (`and 0xfffff; or 0xff000000` at 0x489d7a, 0x48bdea). `_bits_hi :
15` is replaced (no .cpp used it). `target` (+0xc0) stays an int: fx_frame reads it as the casting ItemType (its
+0x58 charge effect's +0xc at 0x48b501, 0x48b534; source type 5 when nonzero) and character_equip_item_fx stores
the ItemType there (0x48ca90). The new unit globals and every function of the unit are declared in fx.h.

**D4402 (fx.h, vfx.h). Signatures corrected from the bodies.** All existing calls compile unchanged (int / bool
literals convert); none of them had a definition. vfx_instance::process(int, ...) -> (float frametime, ...):
fx_update_instance pushes its float argument (0x48b766), the creators 0.001f (0x3a83126f at 0x48c0f9, 0x48ca0f);
spells.cpp (lines 551, 695) passes 0. fx_attach_to_character's last two (int a, int b) -> bool (`mov dl, [esp +
0x20]; test dl, dl` / `mov dl, [esp + 0x30]` at 0x48c09a, 0x48c0ac). fx_add_effect (..., int a) -> bool flag
(passed on to fx_create_instance's byte, `mov al, [esp + 0x90]` 0x48c036). fx_end_owned (int, bool) -> (int, int
destroy) (`test edi, edi` on the whole dword, 0x48d138). fx_remove_missiles_from void -> bool (`xor al, al` /
`mov al, 1`, 0x48d0a5 / 0x48d0e2). status_stone_fx void -> int (the effect handle in EAX, 0x48cdcc / 0x48cddf).

**D4403 (fx.h). fx_find_spawn_point fills a matrix, not a position.** sum.symbols: "(out pos, character,
distance) point in front of the character". 0x48d4c6-0x48d643 write three rows into the output (rvec copied,
uvec / fvec through vector3::operator=), and fx_create_spell_effect_near passes the same 0x24-byte local as the
orientation of fx_create_instance (0x48c828 / 0x48c837), with the character's position as the position. The
four candidates are forward, right, back and left at the distance; the chosen side's orientation is the
character's turned toward it. fx_create_spell_effect_near is (spell, caster, owner, ms): four arguments
(0x48c833, 0x48c868-0x48c883).

**D4404 (engine/spells/spells.h). spell_hit +0x10..+0x18 are source type, item and power; inline constructor.**
fx_frame and fx_missile_hit fill +0x10 with `item ? 5 : 0`, +0x14 with the item and +0x18 with 0 (0x48a9ca-
0x48a9f2, 0x48bb54-0x48bb80) where the single-player path passes the same values as get_hit_by_spell's
source_type / item / power (0x48aad5-0x48aaea). `int _unk10[3]` (unused by name) becomes three fields. The
store group -1, -1, then zeros before fx_frame's spell 0x66 loop (0x48adfc-0x48ae36, overwritten inside the
loop) is an inline constructor; at the other uses it is dead-store-eliminated.

**D4405 (vsdk/geom/vfx.h, effect_mem.h). vfx_base and vfx_instance fields the fx unit reads.** vfx_base: +0x00
name (`_stricmp([base], name)` 0x48d693), counts +0x04 / +0x06 / +0x08 / +0x0a / +0x16 (get_num_tags sums +4,
+6, +0xa, +0x16), +0x18 flags (bit 0: random yaw 0x48bfa7; the attached kinds keep their orientation
0x48a4e5), +0x20 node descriptors (0xa8 each, flag 0x40 = lamp glow, radius +0x28; 0x48bea4-0x48bed2), +0x28
emitter descriptors (0xe8, +0x20 frames, +0x24 rate; count +0x08), +0x2c records of 0x21c (+0x4c, +0x50 seconds;
count +0x16), +0x40 (nonzero: linkable, 0x48d1f9). `char _unk00[0x12]` / `_unk14[8]` / `int _unk20` are
replaced (num_frames +0x12, category +0x1c, light_tracks +0x24 keep their offsets). vfx_instance: +0x08
category (2 = destroyed by fx_destroy_spell_effects 0x48d746; fx_disable_by_effect_name only matches 0,
0x48d67a), +0x10 time (compared with 14.0 / 55.25 in fx_frame), +0x14 length (fx_is_finished 0x48d031), +0x18
the fx_rec98 list (next +0x90, glow +0x8c; 0x48bec0 / 0x48becc), +0x20 the fx_recb0 list (next +0xa8, live
count word +0x96; fx_missile_hit 0x48bbe3); fx_rec98 / fx_recb0 in effect_mem.h get the two field names. New
members: stop 0x537230, is_stopped 0x537270, get_node_name 0x5373a0, bind_node 0x537580 (node, character
instance, tag), node_can_bind 0x537700, reset_flags 0x537800, vfx_base::get_num_tags 0x538340. Names follow
decomp60 where the later tree (commit 0c74176, merged after this copy was taken) has the same offsets
(num_nodes, num_lights, num_emitters, num_rec2c, category, length, nodes, emitters, `short flags`); that tree
also declares effect_is_cutscene_base (int), attached_light_slots / MAX_ATTACHED_LIGHTS and
attached_lights_init_slots / _clear_slots, which this copy adds too: keep one of each when merging
(CHANGED-HEADERS.txt). The +0x10 time is `_unk10` there.

**D4406 (engine/characterinfo/characterinfo.h). xp_level and the experience parsers.** types.h's xp_level as a
struct with groups of {hp, ap, hp_random, ap_random}: experience_parse_level calls experience_parse_stat_gain(tf,
+6 + 4i, +8 + 4i) and (tf, +7 + 4i, +9 + 4i) (0x489a04-0x489a15), which stores Accept("D") through the second
pointer and ParseInt through the first (0x489a3c-0x489a53). xp_level_up_gains (0x489b04, 0x489b6f) rolls
floor(frand() * value + 1), at least 3.0 (0x57ef1c), when the flag is set. types.h's hp_extra / ap_extra
("perhaps a random range") are these flags.

**D4407 (engine/ai/unnamed_avoidance_0x4a6710.h, comment not changed). avoid_area_desc::_unk00 is not always 0.**
summon_spawn_effect stores 1 in the first two fields (`mov eax, 1` 0x48cbd7, then +0x10 / +0x14 of its local),
so the summon's area is a prohibited one; the header's "0 at every caller" belongs to the avoidance unit and is
noted in NOTES.md.

**D4408 (summoner/multi/unnamed_utrack_0x48d7c0.h, new; multi.h includes it). The user tracker login.**
utrack_packet (0x204 bytes, packed 17-byte header: type +0, len +1, code +3, sig +9, data +0x11; 0x48d7dd-
0x48d7ee, 0x48da38-0x48da9a), utrack_login_request {login[0x21], password[0x1b]} (the second strcpy writes at
+0x21, 0x48d96d; length 0x4d = 0x11 + 0x3c; pxo_login_name char[33] and pxo_login_password char[27] are
adjacent at 0x5f9670 / 0x5f9691), the sum.symbols globals and pxo_login_data (0x2318e48, ours). pxo_login_request
takes the request pointer and the reply buffer (0x48d87c, 0x48d913).

**D4409 (vsdk/math/ix.h). seg2d_intersect.** (a, b, c, d) with a / b by address and c / d by value: six argument
dwords (`add esp, 0x18` at 0x48d406), the body reads [esp+4], [esp+8] as pointers and [esp+0xc]..[esp+0x18] as
floats (0x53d8f0-0x53d916); returns AL.

**D4410 (fx.h). The blob shadow bitmaps.** shadow_create_blob_bitmap builds a grey palette (0x230561c, 0x300
bytes) and 16 64 x 64 ramps (0x2308e2c, 0x1000 each, up to fx_sky_list 0x2318e2c) and stores 16 bm_create
handles at 0x59c260 (initialised to -1). Nothing else references 0x59c260-0x59c29f, 0x230561c or 0x2308e2c: the
bitmaps are made and never used. Names ours.

**D4411 (fx.h). The two hooks through empty_function.** fx_link_character_obj (0x48d273, with its flag) and
fx_summon_hook_stub (0x48d1c7) call 0x4f13e0 with two arguments; as for the boss stage hooks (decomp56), they are
declared as fx_link_hook (fx, character) and fx_summon_hook (fx, argument) and defined empty, folded.

**D4412 (vsdk/geom/effect_mem.h). effect_is_cutscene_base and the frame counter.** effect_is_cutscene_base (id)
returns AL (bit 26 of the base flags, 0x51dec0-0x51deea); effects_vfx_counter_2c9f968 (defined in
effect_mem.cpp) gets its extern; fx_frame zeroes it (0x48a242).

## decomp63: the render-scene unit, the shadow-table unit and the equip packets (0x481680-0x487b0f; game/unnamed_render_scene_0x481680.cpp, unnamed_shadow_table_0x487440.cpp, unnamed_equip_packets_0x4878a0.cpp, all placeholders)

**D4300 (engine/rendering/levelrender.h). hover_tint_active returns int; levelrender_level_load returns
int.** hover_tint_active ends in `xor eax, eax; ret` / `mov eax, 1; ret` (0x4820a2 / 0x4820a5), the
pattern D3905 reads as int (a bool return loads AL). levelrender_level_load clears EAX before its RET
(0x4868dd) on its only path, so it returns 0; it was declared void. Callers: gameplay_update_cursor
tests the result (an int in a condition), level_load_files (gameplay.cpp) ignores the load result; both
compile unchanged.

**D4301 (levelrender.h). The draw entries and the level-object render records are complete.**
levelrender_entry (0x10) gains depth +0x04 (in a union with the old _unk04) and object +0x0c (types 0, 4,
5); levelrender_attach_entry (0x4c: character +0x10, item +0x14, light +0x18, pos +0x1c, orient +0x28;
levelrender_push_attachment 0x48349a-0x4834fb) and levelrender_effect_entry (0x1c: fx +0x10,
vfx_node_ref +0x14; levelrender_add_effect_entry 0x48432a-0x48435b) derive from it; LRE_* name the six
levelrender_draw_list cases (jump table 0x486398). lobj_render_state +0x00 is the "$seethru" vis set
(levelrender_level_load 0x486811), +0x08 a GameTimer (`new lobj_render_state[num_lobjs]` runs
GameTimer::GameTimer on +8 of each record, 0x486704). The flag bits are LRS_* masks, not bit fields:
the transitions are `and ~1; or 2` / `and ~2; or 1` / `and ~3` / `or 3` (0x482be2, 0x482c7c, 0x482cc1,
0x482b7c), while levelrender_level_load clears bits 0-1 in one RMW (0x486766); no bit-field layout
gives both. gametarget.cpp (`.flags & 0xff0`) and unnamed_shadows_0x4a37d0.cpp (type / dist / lobj)
compile unchanged.

**D4302 (vsdk/geom/vfx.h). The vfx records levelrender reads, under the decomp60 / decomp64 names.**
decomp60 (D4000) named the base records at +0x28 (0xe8 bytes) vfx_emitter_desc and the instance list
+0x20 (fx_recb0, next +0xa8) emitters; decomp64 named the +0x20 node descriptors vfx_node_desc (0xa8)
and the +0x2c records vfx_desc21c (0x21c, count +0x16). decomp63 keeps those names and adds the fields
its code reads: vfx_emitter_desc vis_flags +0x28 (0x400000 / 0x800000, levelrender_effect_apply_vis
0x483fc6), centre +0x3c and radius +0x48 (levelrender_add_effects 0x483f16-0x483f19); vfx_node_desc pos
+0x1c, cull_radius +0x2c and glow_pos +0x48 (glow_radius +0x28 is also the node entry's sort radius,
0x483d92); vfx_desc21c vis_flags +0x6a (0x80 / 0x100, 0x48401b-0x484030). The per-instance records of
the +0x2c descriptors (instance +0x24, next +0xe0, flags word +0x68) are vfx_inst21c, the argument of the
vif_* particle functions (0x483e4d, 0x483ed2, 0x487272). New: vfx_base num_tags +0x10 / tags +0x3c
(vfx_base_tag, 0x14 bytes, refs of kind 0 node / 1 emitter / 2 +0x2c record, 0x48416c-0x48418f), the
instance's render_alpha (+0x04 byte, in a union with _unk04: 0x4824dc, 0x486212) and insts21c (+0x24, in
a union with _unk24), vfx_node_ref (the 8-byte argument of vfx_instance::render_node 0x538260, thiscall
with three pushes). All are unions with or splits of the existing padding; nothing moves.

**D4303 (vfx.h). attached_light::world_pos (+0x20) is object-relative at draw time.**
levelrender_draw_list writes it, for each ranked light, as (light.pos - lobj.pos) rotated by the level
object's orientation (0x485b46-0x485b68) just before the back end (which levelrender_sum_point_lights is
part of) reads it. The name is decomp60's and is kept; the comment says what it holds.

**D4304 (unit boundaries). 0x481680-0x48743f, 0x487440-0x48789f and 0x4878a0-0x487b8f are three files.**
The first ends with detail_apply_level_settings (its RET at 0x487432); its .data 0x59bd70-0x59c11f is
variables then literals, its .bss 0x22ea498-0x22fb71b. A new object starts at 0x487440: the variable
shadows_enabled (0x59c120 = 1) follows the first unit's last literal ("$vis_set" 0x59c114), as in D3309,
and the next function is the first to use that .data (shadow_get_character_anchor reads 0x59c128); its
.bss 0x22fb720-0x22fedd3 follows the first unit's last byte (0x22fb718). The equip packet functions
(0x4878a0-0x487b83) use no .data or .bss of their own (mp_packet_table's counters only); they are put
in a placeholder file of their own because nothing ties them to either neighbour (the shadow unit or
multichat.cpp, whose .data starts with its command table at 0x59c180). If evidence turns up, the file
merges into one of them without any other change.

**D4305 (new headers). engine/rendering/unnamed_render_scene_0x481680.h and
unnamed_shadow_table_0x487440.h, included at the end of levelrender.h.** The declarations of these units
that levelrender.h already had (fog_*, level_vis_force_*, hover_tint_*, levelrender_render_scene,
options_apply_detail, shadows_parse_table, place_ground_decal, move_target_*, ...) stay where they were;
the new headers hold the rest (globals, records, the other functions). levelrender.h also declares the
three back ends levelrender_draw_list calls (0x48e6c0 / 0x48fdb0 / 0x491500, `(lobj, alpha, light count,
lights)` from the pushes at 0x485bc5-0x485bed) and the levelrender.cpp objects the unit touches
(levelrender_lights[16] 0x23df188, the bytes 0x243e038 / 0x243e039, levelrender_wave_vertices). fx.h's
fx_char_link / fx_char_link_used / fx_sky_list / level_light_get (decomp64) are used as they are. gr.h
gains gr_light_desc::reset (0x50f400), Gr_3d_zmult, gr_start_instance / gr_stop_instance /
gr_set_far_clip / gr_line_quad_3d; multi.h MP_INV_EQP (0x33) / MP_INV_UQP (0x41) (mp_packet_table
counters 0x595758 / 0x5958e0 = table + 0x1c * type + 0xc). path_internal (0x59bd74, summoner.h) is
defined in the render-scene unit: it lies in that unit's .data.

**D4306 (game/unnamed_render_scene_0x481680.cpp). render_scope, an inline empty scope object.**
levelrender_render_scene (5 states), levelrender_draw_list (5), levelrender_fade_occluders,
levelrender_build_draw_lists, levelrender_add_lobjs, levelrender_add_effects and lamp_glows_draw open EH
frames (FuncInfo 0x583de0, 0x583ef0, 0x583e28, ...) whose unwind actions are all the folded empty
destructor 0x434650 on a stack slot, with no constructor code and no destructor call on the normal path:
an object whose constructor and destructor are both inline and empty. profile_label (vsdk/os/profile.h)
does not fit: its constructor 0x51b6b0 is out of line and called. The class and its name are ours;
render_scope objects are placed where the state numbers change (state 0 at the function start, 1-4 in
the blocks). gr_light_desc cannot be the state-0 object: items and containers construct gr_light_desc
locals in levelrender_draw_list without an EH state, and buysell_render_item_mesh has no EH frame.

**D4307 (render-scene header). levelrender_push_attachment's alpha argument is a ubyte.** Its caller
stores the byte from character_get_move_fade_alpha over the dead first argument's slot and pushes the
whole dword (0x4833fa-0x483407), so the parameter is a char type (an int parameter would be widened).
The callee never reads it (levelrender_draw_list takes the alpha from the character again).

**D4308 (engine/rendering/unnamed_shadow_table_0x487440.h). shadow_entry is 0x50 bytes with a gap at
+0x40.** shadows_parse_table stores the radius at record +0x44, the scale at +0x48 and clears / sets the
byte +0x4c (0x487575, 0x487593, 0x487560, 0x4875ac), then copies 0x14 dwords from the stack record
(0x4875c3); +0x40 is never written. Nothing reads the table (shadow_entries is referenced only by
0x4875cb).

**D4309 (vfx.h). vfx_node_desc +0x18 is written as a word.** levelrender_effect_apply_vis clears 0x200
with `mov ax, word [..+0x18]; and eax, 0xfdff; mov word [ecx], ax` (0x483ff0-0x484003) and the tag pass
sets it with a byte or on +0x19 (0x48418f); fx_create_instance reads +0x18 as decomp64's int flags
(0x40, lamp glow). Both stay: flags (int) and vis_flags (ushort) share the slot in a union, and the
render-scene unit uses vis_flags for the 0x200 / 0x400 / 0x600 bits.

## decomp65: the character mesh renderer and the 3D view state (0x50af80-0x50f9cf)

**D4500 (unit bounds; gr 3D header, vector.h). Two objects; the fog / depth helpers and clamp_float are header
inlines.** The range is two objects. The character mesh object (vsdk\ca) opens with nine static initialisers
(table 0x588880-0x5888a0, after console.cpp's), its last regular function is ca_render_stub_ret (0x50d7d0), and
its COMDAT block 0x50d7e0-0x50d9ab follows: vector3::cross (a COMDAT, D500), gr_get_fog_alpha, gr_calc_depth,
gr_vertex_depth, gr_get_point_fog_value, gr_get_fog_value, float_max (a COMDAT, D4004) and clamp_float. Every one of
the eight is called by this object (the three renderers, ca_render_mesh, or another of the eight), and none of
the five fog helpers is a regular function of it (they come after cross). A separate fog object does not fit:
clamp_float, which no fog helper calls, follows float_max, and the 3D view object's initialisers start right after
it (0x50d9b0); the helpers' data (gr_depth_scale 0x5a729c between Gr_far_max and Gr_3d_zmult, Gr_3d_zmult,
Gr_3d_zmax) is the 3D view object's .data. So the five fog helpers are inlines of a gr header (written in
vsdk/gr/unnamed_gr_3d_0x50d9b0.h; gr.h's three prototypes for them are removed) and clamp_float is inline in
vector.h (was a prototype). The other objects that call them (the Direct3D and OpenGL back ends, the vfx code,
the sound group volume, the Glide tint code) are vsdk objects built without inline expansion; they call the
copies kept here. The 3D view object opens with its initialisers (table 0x5888a8-0x5888ec), runs from gr_setup_3d
to gr_light__read (0x50f490) and ends with its COMDATs 0x50f590-0x50f9cf; matrix.cpp starts at 0x50f9d0 (table
0x5888f0). .data: mesh 0x5a7280-0x5a7293 (gr_reflection_map_bm, "refmap01.tga": gr_load_reflection_map, vsdk\gr
in sum.symbols, is the mesh object's), 3D view 0x5a7294-0x5a747f (matrix.cpp's Homogeneous_column follows at
0x5a7480). .bss: mesh 0x25db930-0x2682997, 3D view 0x2682998-0x2692e9f (identity_matrix 0x2692ea0 is
matrix.cpp's). The 3D view .bss also holds vfx_mesh_sort_buf / vfx_mesh_sort_count (only vfx_mesh_render uses
them) and two 25-pointer vertex lists of the polygon clipper (0x26926f4, 0x2692758), so the sort helpers
sort_by_float_key / float_key_less (0x50f2e0 / 0x50f380), matrix43_compose, the gr_light_desc members and the light
record members are this object's too. No path string, no strings.tbl block, no Summoner 2 counterpart: both files
are placeholders (vsdk/unnamed_ca_mesh_0x50af80.cpp, vsdk/unnamed_gr_3d_0x50d9b0.cpp). The order of the COMDATs in
either block matches neither the first call nor the header order (open question).

**D4501 (gr.h). gr_setup_3d(.., bool zbuffer, ..) and gr_cull_sphere(.., bool ignore_far).** gr_setup_3d pushes
its fourth argument unchanged (`mov edx, [esp+0x20]; push edx`, 0x50df33) to gr_zbuffer_clear(bool); an int
parameter would be converted (test / setne) first. gr_cull_sphere tests its third argument as a byte (`mov al,
[esp+0x1c]; test al, al`, 0x50ece5 and 0x50ed2f). Both were int. Every caller in src/decomp passes the literal 0 or
1 (ai.cpp, gameplay.cpp, living_entity.cpp, new_buysell.cpp, new_inventory.cpp, new_saveload.cpp, new_status.cpp,
objects.cpp, unnamed_interface_0x411270.cpp) and compiles unchanged; no other definition exists.

**D4502 (vsdk/math/ix.h). The Plane__* members are inline members of plane; two plane assignments.** The plane
constructor from three points (0x50f7d0), set_from_points (0x50f800), distance_to_point (0x50f860, was an
out-of-line prototype) and transform (0x50f880, thiscall (out, pos, orient), returns out) are COMDATs at the end of
the 3D view object; the ix_* functions of another vsdk object (ix_ray_plane, ix_ray_quad, ix_ray_parallelogram)
call these copies. They are written as header inlines of the existing class plane (one plane type, as decomp35 /
decomp56 decided). Open: Sum.exe has two out-of-line plane copy assignments, 0x50f760 (this object: calls
vector3::operator=, copies d, returns this; used by the clip plane setters, gr_get_frustum_planes and
gr_clip_plane::operator=) and 0x4c0a20 (the projector object's, four dword moves), and plane::transform's
`*out = p` calls 0x4c0a20, not its own object's 0x50f760. One implicit operator= cannot have two kept copies, so
the original had two plane-like types (sum.symbols spells them Plane__ and plane__) or one user-declared
assignment; and gr_cull_bounding_box_transformed's local plane has no constructor call (0x50ee70), which `plane p;`
would emit here. Left as one class until a second type shows up elsewhere.

**D4503 (vsdk/gr/unnamed_gr_3d_0x50d9b0.h). gr_light_record.** sum.symbols' gr_light__reset (0x50f420) and
gr_light__read (0x50f490) are thiscall on a 0x8c-byte record: type byte +0, quaternion +4, vector3 +0x14, six
floats +0x20..+0x34, a float +0x38 read only for type 4, bool +0x3c, floats +0x40 / +0x44 (40.0 after reset), bool
+0x48, char[64] name +0x49 (File::read_string(.., 0x40, 0x30007, "")). gr_light is the 0x98-byte attached-light
layout (D4002), so the record is a new class gr_light_record (name ours). Nothing calls either member.

**D4504 (vsdk/gr/unnamed_gr_3d_0x50d9b0.h). gr_check_normal_facing(normal, point) and
gr_get_normal_facing_dot(normal, point); bool result.** In 0x50eb30 the operator- call consumes the second argument
(`push eax; push ecx; push edx; mov ecx, gr_view_pos; call operator-`: the hidden result and [esp+8]), and the dot
takes the first: dot(gr_view_pos - point, normal) > 0. 0x50eba0 is the same with a normalize. Every caller passes
the normal first (the three character mesh renderers: the cross product, then the triangle's first vertex;
levelrender_draw_list_*: the face's +8, then its first vertex, 0x48eba3-0x48eba8). sum.symbols' "(pos, normal)" has
the order reversed. The callers test AL after the call, so the result type is bool (gr_plane_faces_viewer, the same
shape without a caller, too).

**D4505 (behaviour reproduced as written).** (1) The renderers transform the vertex normals with the full bone
matrix, translation included (`fadd [eax + 0x24..0x2c]` for the normal too, 0x50b7e4-0x50b859 Glide, 0x50c514-0x50c58d Direct3D),
then normalise the weighted sum: the lighting normal of a skinned vertex leans towards the bone position. (2)
gr_d3d_render_character_mesh takes the fog alpha of a seam copy from ca_skin_normals[j - d].z (0x50c37f-0x50c39d)
where the vertex it copies used its view depth. (3) gr_cull_sphere tests gr_frustum_near_plane whenever ignore_far
is clear, also when far clipping is off and that plane is stale (or the zero plane, distance 0). (4) The tint grey
is (r + g + b) * 0.75 (0x580e04), clamped to 255, not the average. Written as the code does it; NOTES.md lists them
as game bugs to check.

**D4506 (vsdk/ca/unnamed_ca_mesh_0x50af80.h, new; character.h includes it). The mesh renderer's view of a
vim_submesh and of gr_light_desc.** The renderers read block0 positions, block1 normals, block2 u / v, block3
16-byte triangles (ca_mesh_tri: three int indices, a short flags word: 0x20 two-sided, the other bits hidden when
they meet gr_light_desc +0x1c << 6), block4 a short per vertex (a non-negative value d: copy of vertex j - d, a uv
seam), block5 eight bytes per vertex, _unk18 the texture slot, vertex_extra the vertex of each morph target id,
size0 / size1 the vertex and triangle counts. skeleton.h keeps its generic names. In block5 the bytes +0..+3 are
the weights (0 = unused, they sum to 16: the result is scaled by 0.0625) and +4..+7 the bone indices; character.h's
ca_vertex_weights calls +0 bones and +4 weights (character.cpp uses the names, so they stay; the renderers index
the bytes). gr_light_desc +0x1c is used as an int mask (Character::hero_model_prefix returns the table values 6 / 2
cast to const char *), and the constructor runs vector3() on +0x0c (a vector3 member the header splits into
_unk0c / appearance / f14); both left as they are, noted. gr_light_desc::reset (0x50f400) is declared.

**D4507 (d3d_min.h, gl_min.h, vfx.h, vsdk/math/bezier.h).** IDirect3DDevice7::DrawIndexedPrimitiveVB (+0x80, was
`void *`) gets its prototype (device, type, vb, start, vertex count, WORD indices, index count, flags) and the SDK
macro, D3DPT_TRIANGLELIST 4; gl_min.h gets GL_UNSIGNED_SHORT 0x1403; vfx.h declares vfx_render_alpha (0x2dbfd14)
and vfx_render_bitmap (0x5a9cfc, name ours), the second pass the vfx render state asks for; bezier.h (file name
ours) declares the 0xf4-byte bezier with the three members ca_ramp_curve_eval calls.

## decomp67 (the vfx object 0x536950-0x538647, the ix unit 0x53ac20-0x53debc, the float helpers 0x542500-0x542cc2, ui_element 0x542cd0-0x54302c)

**D4700 (vsdk/geom/vfx.h). vfx_instance signatures.** process returns AL (`xor al, al` 0x537152 for no base,
base flag 0x4000 or done; `mov al, 1` 0x537136, 0x53720c, 0x53721b): bool, was void. reset_flags reads both
arguments as bytes (`mov al, [esp+0xc]; and eax, 1`, 0x53780a): bool, bool. get_node_world_transform compares
its last argument as a byte with 1 (`cmp byte ptr [esp+0x1ac], 1`, 0x537db3) and passes push 1 to its own
recursion: bool c (a and b are only passed down). get_tag_transform (sum.symbols' name kept) hides or shows tags:
the second and third arguments are bytes (`and ecx, 0xff; shl ecx, 0x1f` 0x538003, `cmp byte ptr [esp+0x70], 1`
0x5381d0) and EAX is not set before `ret 0xc`: void (tag, bool, bool), was int (tag, int, int); spells.cpp ignores
the result. init_nodes ends with `xor eax, eax` (0x536bb3): int; its second argument goes as a raw dword to
vfx_node_create_light's bool (0x536acd) and vfx_instance_create pushes its own dword unchanged (0x51d787): bool
no_light (was int a), which suggests vfx_instance_create's `int a` is a bool too. All existing calls compile
unchanged (same functions, argument conversions only).

**D4701 (vfx.h). vfx_instance__ctor 0x538570 and vfx_instance__clear 0x5369c0 are vfx_base's.** The constructor
constructs a vector3 at +0x48 and calls clear, which zeroes the words +0x04..+0x16 except +0x14, keeps the top
three bits of +0x18 and zeroes +0x40: vfx_base's counts, flags, _unk40 and pos (+0x48). vfx_instance is 0x48
bytes, so +0x48 is outside it. Declared as vfx_base::vfx_base() / clear(); the constructor is defined out of
line in the vfx object (it is in the binary with no caller).

**D4702 (vfx.h). vfx_emitter_desc +0x1c is a signed 16-bit bit field.** vfx_instance_create (0x51d5ae),
decode_tag (0x5372d8) and get_num_tags (0x538357) all read it as `mov reg, dword [+0x1c]; shl reg, 0x10;
sar reg, 0x10`, the bit-field extraction; a short would be `movsx`. `int num_particles : 16; int _bits1c_16 :
16` replaces `short num_particles; short _pad1e`. The same field is the emitter's tag count (decode_tag).

**D4703 (unnamed_vfx_0x536950.cpp). gr_vertex_pool has no initialiser here.** The binary's initialiser 0x536950
runs `vector constructor iterator` with gr__Vertex__Vertex (0x5081c0) over 4000 x 0x30 bytes; gr.h's gr_vertex
has no constructor (D1605, open). The definition is plain until D1605 is settled. The pool is defined first:
the initialiser table (0x5889d0, 0x5889d4, 0x5889d8) runs pool, camera position, camera orientation.

**D4704 (vfx.h). vfx_instance +0x0c is a float (elapsed seconds).** process adds the frame time to it and
stores time = +0x0c * 15 (0x536fe4-0x536ffe), init_nodes stores num_frames / 15 there (0x536a8c), reset_flags
zeroes it. It becomes `union { int _unk0c; float elapsed; }` so that unnamed_render_scene_0x481680.cpp's
`vi->_unk0c` still compiles. vif_emitter_update's last argument is this value pushed as its dword (0x5370f9);
its declaration keeps `int unk0c` (decomp63), and the vfx object passes `_unk0c`. Changing the parameter to
float needs render_scene's call changed to `vi->elapsed` (not done here: read-only repo).

**D4705 (unnamed_ix_0x53ac20.cpp). The ix statics and their atexit handlers.** 37 function-local `static
vector3`s are built behind guard bytes and registered with atexit; each handler is a bare `ret`
(0x53ad30-0x53da70). VC6 registers a destructor only for a class with a destructor, but vector.h's vector3 has
none (and vector3 globals elsewhere, e.g. vector3_zero 0x523f30, register nothing). Open: the ix unit may see a
vector3 with an empty inline destructor (another header version). The source writes plain `static vector3`; the
handlers carry their address comments.

**D4706 (unnamed_floatmath_0x542500.cpp). Empty string literals in .bss.** float_format_buf copies one byte from
.bss 0x2ee451c into a two-byte local and zeroes the second (0x54283c-0x54284c), the code VC6 makes for
`char digit[2] = ""`, and strcpy's buf from .bss 0x2ee4520 (0x54285b), i.e. `strcpy(buf, "")`. Both are written
as literals; this disagrees with the cmdline.cpp comment that VC6 puts "" in .data (cmdline_no_options
0x2dc7058 may be such a literal too).

**D4707 (vsdk/ui/ui.h). ui_element::tooltips is a ui_tooltip.** ~ui_element (0x542d40) calls ui_tooltip's
destructor (0x55ef50) on this + 0x124 under its EH frame (state 0 -> -1), so the sentinel at +0x124 is a member
of type ui_tooltip (0x120 bytes, to +0x244 = callback). `ui_tooltip tooltips` replaces `ui_tooltip_head tooltips;
ubyte _unk12c[0x118]`; every existing use (`tooltips.next`, `tooltips.prev`, `(ui_tooltip *)&tooltips`) still
compiles. ui_tooltip_head is unused now.

**D4708 (vfx.h, effect_mem.h). The vfx base and record layouts the vfx object reads.** vfx_base: +0x0e
num_cameras (records at +0x38, 0x64 bytes), +0x14 num_materials (+0x44, 0x4c bytes), +0x30 dummies (count +0x0a,
0x64 bytes), +0x40 a pointer whose byte +0x0c indexes the materials (setup_render_state 0x53844c), the counts
relocate_node caps (0x536dfa-0x536e41). Every descriptor starts with name +0 and parent name +4 (get_tag_name /
get_node_name, jump tables 0x537470 / 0x53755c), relocated by reloc_ptr. Bind flags: mesh byte +0x0c bit 0,
emitter +0x18 bit 31, light track +0x0c bit 0, vif +0x6a bit 9, dummy +0x0c bit 15 (0x53773a-0x5377cc). Instance
records: fx_rec98 +0x14 alpha (float from the instance's byte +4), +0x18 ci, +0x1c tag:28 / level / bound /
placed / hidden; fx_recb0 +0x0c flags (stop sets bit 0), +0x10 tag:28 / bound / placed / hidden, +0x14 ci, +0x7c a
quaternion (was vector3 v7c); fx_rec2c +0 ci, +4 tag:29 / bound / placed / hidden, +0x14 a quaternion (was vector3
v14 and _unk20); vfx_inst21c +0 pos, +0xc matrix, +0x6a tag, +0x6c ci, +0x72 live count, +0xe4 prev. The kinds
follow decode_tag (VFX_TAG_*, names ours). Upstream names (decomp63/65: num_tags, tags, vfx_base_tag, vfx_tag_ref,
vis_flags, vfx_inst21c, insts21c, render_alpha, vfx_node_ref, render_node) are kept.

**D4709 (vfx.h, effect_mem.h). The vfx object's inline helpers.** get_base, the five list walkers and
vif_record_num_live (sum.symbols vfx_get_word_72) are COMDATs after the last regular function (0x538590-
0x538647) and are only called from the object, which is built without inline expansion: header inlines,
defined in effect_mem.h after effect_get_base and the record types. list1c_item steps its count by 2 (`sub ecx,
2`, 0x5385e2), kept as in the binary.

**D4710 (vsdk/math/ix.h). The ix signatures.** From the bodies: ix_sphere_boundingbox is (point, radius, min,
max) (the radius at [esp+8], 0x53c41c; sum.symbols reads "(center, min, max, radius)"); ix_point_linesegment
returns the clamped distance along the segment (0x53db4b-0x53db6c), not the distance from the point;
ix_ray_sphere does not read its length argument; ix_box_oriented_box_oriented takes the rotation as rows 16
bytes apart (B[1][0] at +0x10, 0x53c652); the polygon tests take vertex pointer arrays (ix_ray_quad builds one,
0x53b1ab). sum.symbols' names are kept for ix_ray_polygon (a segment / point-normal plane test), ix_line_line
(two planes) and ix_slab_clip (an outcode).

**D4711 (vsdk/math/vector.h). The scalar vector3 operators.** vector3__plus_scalar / minus_scalar /
add_scalar_assign / sub_scalar_assign (0x53de00-0x53debc) close the ix unit as COMDATs: operator+(float) (it was
declared inline without an address), operator-(float), operator+=(float), operator-=(float).

**D4712 (unit boundaries). 0x542500-0x54302c is two objects.** The float helpers end with two unreferenced
thiscall members (0x542cb0 / 0x542cc0) and the UI element code ends with ui_tooltip's scalar deleting destructor
(0x543010), each the closing COMDATs of an object (as D1601); the functions have nothing in common, and the .data
0x5ac8c8-0x5ac8d9 (formatter strings) / 0x5ac8dc (the UI id counter) are adjacent but separate. 0x543030 (a bare
ret, no reference) is left unattributed.

**D4713 (ix.h). bbox_from_points and bbox_expand_by_vector.** ix_sphere_polygon calls 0x5595e0 (mins, maxs, a,
b) and 0x559810 (mins, maxs, delta) (0x53c0db, 0x53c0eb); declared in ix.h (unnamed_gr_3d_0x50d9b0.h, where the
other bbox helpers are, includes ix.h).

## decomp68 (the level completion unit 0x4b0960-0x4b140f, the AI movement unit 0x4b1410-0x4b513f, the AI think unit 0x4b5140-0x4b77ef, the dialogue box unit 0x4d6660-0x4d7b2f; all placeholders)

**D4800 (levelscripts/conversation.h). The dialogue boxes are polymorphic classes.** dialogue_box_base__ctor
stores vtable 0x581c6c and dialogue_box_base__dtor (0x4d7130) restores it: a user-declared destructor of a
class with virtuals. The base vtable has two slots, [0] 0x4d7140 (sum.symbols conversation_reset) and [1]
_purecall 0x56e903 (the next dword 0x581c74 is the float 1/7); the text box vtable 0x581c4c is {0x4d7650,
0x4d7680, 0x4d69d0}, the topic list's 0x581c5c {0x4d7920, 0x4d7930, 0x4d6a90}: slot 0 reset (set_rect calls it
through `call [eax]`, 0x4d717e), slot 1 draw, slot 2 layout (add_text / add call `[edx+8]`, 0x4d7340,
0x4d785e), new in each derived class. The line (0x581c58) and topic entry (0x581c68) records have one slot,
draw(int scroll), called by the box draws with (int)scroll (`call [ebp]`, 0x4d77cf). So the plain `void
*vtable` members go, the classes declare the virtuals, and: base reset is 0x4d7140 (was declared at 0x4d7920,
which is dialogue_topic_list::reset, `jmp 0x4d7140`); 0x4d7650 (conversation_start) is dialogue_box::reset
(the base reset, then the 32 labels cleared); `start()` stays as an inline alias of it for new_dialogue.cpp
(a direct call on the object, same code). item_visible / item_y index the 0x18-byte entries (`lea eax,
[eax+2*eax]` then `*8 + 0x2c`, 0x4d78b9 / 0x4d7908): members of dialogue_topic_list, where their callers are.
add returns the entry (EAX = edi at 0x4d7861 / 0x4d78a3). Fields: dialogue_line +0x08 and dialogue_topic_entry
+0x08 are the wrap width (gr_split_str's max width, 0x4d6b2c / 0x4d6e16), dialogue_line +0x14 is the text
(NULL for the spacer line add_text inserts; _unk14 kept in a union), base +0x28 is a GameTimer (constructed at
0x4d70f9), the 32 labels are {char used; char text[0x80]} records (alloc returns the record, free compares
`label + 1`, 0x4d6a57) whose inline constructor clears both bytes (the loop 0x4d66c9).

**D4801 (summoner/multi). The level completion unit has its own header; mp_send_living_combat's types.**
mp_level_find_boss, mp_level_complete_check, the L_CMPLT / D_CMPLT / COM_SEQ handlers and
mp_send_living_combat move from multi.h to summoner/multi/unnamed_level_complete_0x4b0960.h (included by
multi.h), with the unit's other functions. mp_send_living_combat stores the variant, hit flag, skill and
special as single bytes (`mov byte ptr [esp+4], al` etc., 0x4b11c6-0x4b11e7) and reads one byte at +0x10 of the
defence move (`mov al, [eax+0x10]`, 0x4b120c: move_info::category), so the parameters are (int, int, ubyte,
move_info *, ubyte, ubyte, bool); were int / void *. combat.cpp's only call compiles unchanged.

**D4802 (engine/ai/ai.h). follow_path takes a float, turn_towards a bool.** ai_follow_path compares its
argument with 0.001f / 0.0001f (`fld dword ptr [esp+0x70]; fcomp [0x581938]`, 0x4b3a2f) and passes it to
character_distance_to_reach_speed: a float speed (the traveling frame passes the leader's speed). ai_turn_towards
reads its second argument as a byte (`mov al, [esp+0xb8]; test al, al`, 0x4b2c15). The existing calls (0, 1,
true) compile unchanged.

**D4803 (engine/ai/ai.h). The goal handlers return int.** goal_do_attack, _follow, _travel, _nothing,
_approach_party, _combat_mode, _cast_spell, _cast_at_point and _move_to set EAX on every path (`or eax, -1`,
`xor eax, eax`, `mov eax, 1`, path_to's result); goal_do_hide, _sneak, _special_attack, _pickup and
_move_to_timed leave EAX as a callee left it and stay void. ai_process_goals ignores the value.

**D4804 (engine/ai/ai.h). ai_target_query is 0x18 bytes.** ai_score_target writes +0x14 with the chosen
candidate's combat_attack_rating (`mov [esi+0x14], eax`, 0x4b5778) and reads it back to scale the next score
(`fmul dword ptr [esi+0x14]`, 0x4b5737); boss_target_list_build's record at esp+0x1c has room for it (+0x14 is esp+0x30,
the last slot of its frame below the saved registers). +0x0c holds the chosen Character (stored from [esi+4]), +0x08 is the attacker limit
(compared with ai_count_engaged_attackers, 0x4b56b7). The second argument is a byte (`mov cl, [esp+0x28];
test cl, cl`, 0x4b53e5): bool.

**D4805 (engine/ai/ai.h). ai_combat_frames_watched is ai_mode_frame_counts[3].** 0x247e84c lies inside the
13-int array at 0x247e840 that ai_do_frame clears; the traveling and cast frames increment [2] (0x247e848) and
[4] (0x247e850) the same way. One object: the name becomes a macro on the array element; the array is defined
in the AI movement unit.

**D4806 (game/unnamed_ai_think_0x4b5140.cpp). A GameTimer with an empty destructor.** ai_party_spread_attackers'
function-static timer 0x247e91c is constructed under a guard bit (0x247e95c) and registers atexit(0x4b71b0), a
bare RET: its type has a destructor that expands to nothing, which GameTimer has not (no GameTimer global
registers one). Written as a local class spread_timer : GameTimer with ~spread_timer() {}, as D3805's cam_vec.

**D4807 (engine/ai/ai.h). ai_state fields.** +0x78 is a float (the wander radius: `fmul dword ptr [esi+0x78]`,
0x4b43b4; goal_do_move_to_timed stores its float argument there), was ubyte[4]. +0x204 and +0x20c are floats
(`fmul dword ptr [esi+0x204]`, 0x4b255d; `fmul dword ptr [esi+0x20c]`, 0x4b2b70): steer_offset and steer_side
in unions with the old int names (ai.cpp clears _unk204). Named in unions: +0x198 turn_move (the turn-in-place
move, 0x4b2de6), +0x1ac formation_slot (0x4b516e, 0x4b5261), +0x1fc prev_path_started (0x4b352f), +0x220
avoid_handle (0x4b1ffa).

**D4808 (unit boundaries). 0x4b0960-0x4b77ef is three objects.** The level completion and COM_SEQ code
0x4b0960-0x4b140f has no initialiser; its .data 0x59d694-0x59d6e3 is one block (the boss names unique to it,
in reverse order of use, then "resetting %s to attack %s\n"), so COM_SEQ belongs with the level code; it follows
multispell.cpp (tagged by strings.tbl id 238, no .data), whose .bss it continues (0x247e830 after 0x247e82f) -
only adjacency, so a placeholder unit, not multispell.cpp. The initialiser runs 0x4b1410 (entries 303 / 304)
and 0x4b5140 (entry 305) open the movement and think units; the .data 0x59d6e4-0x59d7c3 (turn_scaler, the
speed factors, the movement unit's literals) and the .bss blocks 0x247e840-0x247e8c3 / 0x247e8c8-0x247e963 split
the same way, and the .rdata constants of the think unit (the team table 0x581bb0, then -4, 0.0005, 7) follow
the movement unit's (-1.5 0x581ba8, 120 0x581bac). pathfinding_visibility.cpp starts at 0x4b77f0.

**D4809 (summoner/multi/multi.h). Two AI senders.** mp_send_character_attack (0x488390) and mp_send_swi_sneak
(0x488a20) are declared next to mp_send_swi_hide: one pushed Character, `add esp, 4` (0x4b621c, 0x4b680a).

## decomp66 (0x5565e0-0x56c4d0: String / TextBuffer, LineFile, bezier, the clipper, lfsr, FTP / HTTP, vfx channels, BitArray, DirectSound, the slider, the polygon split, the vfx dummies, emitters and materials, the sound streams; all placeholders; libjpeg not rewritten)

**D4600 (vsdk/vfile/linefile.h). LineFile::read_line returns the buffer.** 0x558600 returns its first argument
in EAX for each line read and `xor eax, eax` (0x55879a) at the end of the file: char *, was int. console.cpp's
`while (lf->read_line(line, 256))` compiles unchanged.

**D4601 (vsdk/geom/vfx.h). The +0x0c word of vfx_node_header has two views.** The light-track samplers take
`(ushort >> 1) & 0x7fff` (bit 0 separate), the forces and emitters an arithmetic `sar 8` of the dword
(vfx_force_desc::num_frames); both kept in an anonymous union (num_frames:15 above _bit0, frame_word). The
force kind is the low byte of frame_word (`cmp byte ptr [eax+0xc], 1`, 0x56069d and 0x560b48). The dummies
read the low 15 bits of decomp67's vfx_dummy_desc::flags (`and edx, 0x7fff`, 0x55fcce).

**D4602 (vfx.h). vfx_channel::eval returns short.** 0x55bc00 returns AX (`mov ax, 0xfa0`, 0x55bc10, the broken-
data value 4000) and the callers sign-extend AX (`movsx edx, ax`, 0x56143e): short, was int.

**D4603 (vsdk/snd/ds.h, vsdk/snd/snd.h). The third argument of ds_play_streamed / snd_play_streamed is a float
pan.** ds_play_streamed multiplies it on the FPU (`fld dword ptr [esp+0x1c]; fmul`, 0x55d353) as ds_play does
its pan; the fifth goes to ds_find_free_channel, which does not read it (the sound unit pushes 1.0f): float
priority. snd_play_streamed / _looping pass their third argument through unchanged, so they become (int, float,
float) too (was int arg); every caller passes 0 (gamesound.cpp, summoner.cpp, the cutscene unit), which
converts to 0.0f with the same `push 0`. The decomp66 copy of src/decomp/vsdk/unnamed_snd_0x5204c0.cpp carries
the change.

**D4604 (vsdk/ui/ui_gadgets.h). The slider members that use +0x2a8 belong to ui_slider.** create_scrollbar
(0x55ed30), set_range (0x55eeb0) and the other members reading +0x2a8 are called on ui_slider pointers
(the listbox, window.cpp); the 0x2a8-byte ui_slider_base has no +0x2a8. They move from ui_slider_base to
ui_slider (0x2ac); the existing callers are unaffected.

**D4605 (vfx.h). vfx_emitter_desc +0x28 is a bit-field word.** The emitter unit reads the shape as `shl 0x1e;
sar 0x1e` (0x56022b), the material index as `shl 0x10; sar 0x12` (0x56255f, 0x5612cd) and tests single bits
0x10000 .. 0x1000000; decomp63's vis_flags (0x400000, 0x800000) are bits of the same word: an anonymous union of
vis_flags and the bit fields. The high half of +0x1c (`sar 0x10`, 0x55ff5d) is the first emitting frame
(start_frame, was _bits1c_16), the high half of +0x38 the last one (end_frame, also the loop length the frame
counters are taken modulo). The class moves after the key channels (its channels fill +0x4c..+0xe7).

**D4606 (vfx.h). The effect record kinds share vfx_node's layout.** The dummies (0x55fed0), cameras (0x5655c0),
emitters (0x561190) and tag groups (0x565400) call vfx_node_header::relocate (0x55bbd0) on their own this, and
the dummy and camera code call vfx_node::get_orient (0x55ba30) on their descriptor. decomp67 declared
vfx_dummy_desc, vfx_camera_desc and vfx_base_tag as their own structs with name / parent first; they keep those
names and fields, get their channels (dummy and camera: vec channel +0x10, quat channel +0x34; camera start /
end frames +0x0c / +0x0e), and the code casts to vfx_node_header / vfx_node where the binary calls those
members. decomp66's class vfx_force (vfx_node plus five channels, relocate 0x55b960) and decomp67's struct
vfx_force_desc (relocate 0x55b960, 0xa0 bytes of _unk) are one class: decomp67's name, decomp66's layout
(`class vfx_force_desc : public vfx_node`); fx_rec4c::update / get_transform take vfx_force_desc *.
vfx_node_header keeps its int name / link (decomp67's vfx object reads header.link).

**D4607 (vsdk/geom/effect_mem.h). fx_rec28 +0x08 is the velocity, +0x14 the position.** init_particle stores
the speed vector through 0x562360 and the start point plus speed * age through 0x562340 (0x5602ac, 0x56034e);
apply_forces measures the forces from 0x562400 (+0x14) and ends with new_pos = pos + 0.5 (vel + new_vel) dt
stored through 0x562340 and new_vel through 0x562360 (0x5609da, 0x5609e6); the billboard axes put the triangle
tip at +0x14 - +0x08 / 15. sum.symbols has the names the other way round (vfx_particle__set_pos 0x562360,
set_velocity 0x562340, get_velocity 0x562400, get_pos 0x562420): the members are named by what they hold
(set_vel, set_pos, get_pos, get_vel) with the symbols noted. +0x02 is the lifetime (8.8, compared unsigned
with the age: `cmp ax, [edi+2]; jb`, 0x56185d), +0x04 the 4.12 angle sum.symbols calls the size.

**D4608 (vfx.h). vfx_material flag bits and fields.** eval_curve34 reads bits 0-2 signed (`shl ecx, 0xd; movsx;
sar ecx, 0xd`, 0x564ff9) and switches on 0 / 1 / 2; the curve rate is bits 7-15 (`shr ax, 7`, 0x5650ea); reset
writes `and 0xffc7; or al, 7` then `and 0x7f; or 0x780` (0x564f88-0x564fb7). decomp67's flags keeps its name in
a union with curve34_mode:3, blend:3, additive:1, rate:9. +0x02 / +0x04 / +0x06 are the curve lengths, +0x08 the
rgb bytes of the flat particles, +0x34 / +0x44 / +0x48 the curve pointers (ptr_relocate), +0x38 a bitmap name
(reloc_ptr) and +0x3c its handle (was _unk02[0x0a], _unk34[0x18]).

**D4609 (vsdk/gr/gr.h). gr_bitmap_3d's second argument is a float.** The emitter pushes the particle angle as a
float (`push ecx; fstp dword ptr [esp]`, 0x562082) and gr_bitmap_3d passes the dword through to
gr_bitmap_3d_projected; the snow, lamp-glow and mesh callers push 0, the same bits as 0.0f. float angle, was
int a.

**D4610 (vfx.h). ptr_relocate is inline.** Its copy 0x564d50 sits in the mesh object's block of inline members
(0x564770-0x564d62, after vfx_ptr4_relocate); the vfx and emitter objects call it. Declared inline with its
body (base added unless the pointer is NULL); no definition existed.

**D4611 (unit boundaries). 0x562a30-0x56565c is the mesh object and the material code.** The initialiser table's
last user entry (651) runs 0x562a30 (the dump_mesh_frames console command, .bss 0x2f91a60-0x2f91a6f, .data
0x5ae104-0x5ae1db); the mesh functions (0x562b20-0x564761) are followed by their inline accessors
(0x564770-0x564d62, ending with ptr_relocate). The material, texture-layer, tag-group and camera code
(0x564d70-0x56565c) follows with no initialiser and no .data; its only data is the empty-string literal
0x2f91a70 (vfx_material::reset), the .bss slot after Dump_mesh_frames. Its small members sit after each group
(0x5653e0 / 0x5653f0, 0x565450, 0x5655f0 / 0x565610), so it may be up to three objects; it is one placeholder,
src/decomp/vsdk/unnamed_vfx_material_0x564d70.cpp. The mesh object itself is not written (decomp66 NOTES.md).

**D4612 (effect_mem.h, vfx.h). Emitter instance and texture-layer fields.** fx_recb0 +0x18 is a float time in
seconds (compared with 0 and advanced by dt, 0x561764-0x561831; was _unk18), +0x8c .. +0x94 five shorts (the
4.12 width, depth, size, speed and speed spread blended in get_transform, 0x561466-0x56153d; was _unk8c), +0xa0
the frame of the last update and +0xa4 the spawn accumulator (was _unka0); the top byte of +0x0c is the alpha
(`and edx, 0xffffff; shl eax, 0x18; or`, 0x56155f), in a union with decomp67's flags (bit 0 stopped).
vfx_tex_layer's empty constructor and destructor are the two functions vfx_material's constructor passes to
`eh vector constructor iterator` (0x564f39-0x564f4f): inline in the header. The 0x18-byte zeroing constructor
sum.symbols calls vfx_tex_layer__ctor (0x564ef0) belongs to another type (no caller).

## VC6 compile (D4900-D4999): src/decomp under Visual C++ 6.0 Professional (cl 12.00.8168, `/c /O2 /MT`)

Edits that make the units compile under VC6 as well as mingw. They change no layout and no generated code for
a compiler that already accepted the old spelling; tools/vc6-compare.mjs (docs/tools.md) checks the result.
202 of 204 .cpp files compile; the two left are listed under D4902 and D4904.

**D4900 (vsdk/vsdk.h). Fixed-width integer types without <stdint.h>.** VC6 has no stdint.h (fatal error C1083 in
every unit). vsdk.h defines int8_t .. uint64_t, intptr_t and uintptr_t itself when `_MSC_VER < 1600` (VC6 is
1200; `__int64` for the 64-bit pair) and includes <stdint.h> otherwise. Same sizes as symbols/types.h.

**D4901 (engine/ai/ai.h). ai_state's attacker slots are a named struct.** VC6 cannot generate the default
constructor of an unnamed class whose member has one (C2639, "compiler-generated default constructor required by
unnamed class"): the slot holds a GameTimer. The struct is named `ai_attacker`; the array, its offset (+0x1b0)
and the members are unchanged. This one error stopped 102 units.

**D4902 (new_saveload.cpp, new_inventory.cpp, new_status.cpp, new_tutorial.cpp). The single-reference string
tables get a constructor under VC6.** VC6 does not brace-initialise a struct with a reference member (C2440
"cannot convert char * to struct ..."). With a converting constructor `T(char *&r) : s(r) {}` it accepts the
tables unchanged (`{ XSTR(..) }` per element) and emits the init_strptrs_* pattern of Sum.exe: one
`mov [table+4k], offset localize_strings+4id` per entry in a `$E` function (checked on a test file). The
constructor is `#ifdef _MSC_VER`: C++98 (mingw) rejects brace initialisation of a class with a constructor.
Open: new_help.cpp's `help_line` has two references (key, text), which VC6 accepts neither with braces nor with a
two-argument constructor in braces; `help_line(a, b)` elements compile but load both addresses into registers
first, unlike init_strptrs_5bc928 (0x40bf40). A `[][2]` array of the one-reference struct gives exactly the
binary's stores, so the original was probably a two-dimensional table (`help_mouse_lines[i][0]`), which changes
the drawing code; new_help.cpp is left failing under VC6 until that unit is revisited.

**D4903 (vsdk/character.cpp). CA_FRAMES_TO_TICKS's parameter is not called f.** VC6's preprocessor splits the
float suffix of `1.0f` when the macro has a parameter named `f` (C2059 "invalid suffix on number"). Renamed to
`n`; the expansion is unchanged.

**D4904 (build flags). `/MT`, the full VC98 include names, and the DirectX 7 SDK.** Sum.exe links the
multithreaded static CRT (_beginthread 0x56fff0, _mtinit 0x571adc, _getptd 0x571b43), so units are compiled with
`/MT`: without `_MT`, VC6's stdio.h defines `putc` as a macro (LineFile::putc in vfile/linefile.h fails to parse)
and process.h does not declare `_beginthread` (the FTP and HTTP units). A VC98/INCLUDE copied straight from the
CD has 8.3 names for the C++ headers (XCEPTION, ALGRITHM, FCTIONAL, STDXCEPT, STREAMBF, STRSTREM), which setup
renames; <new> (combat.cpp) needs `exception`. levelrender.cpp includes the system <d3d.h> and uses the
Direct3D 7 C++ interfaces (IDirect3D7, IDirect3DVertexBuffer7, D3DVERTEXBUFFERDESC), which VC6's own DirectX
headers predate: it needs the DirectX 7 SDK include folder ahead of VC98/INCLUDE and is the other unit that
does not compile here.

**D4905 (vsdk/plainstring.cpp). `strcpy(dest, buf); return dest;`, not `return strcpy(dest, buf);`.** The six
number-to-string helpers (int_to_string 0x515660, int_to_hex_string 0x5156b0, float_to_string 0x515700,
byte_count_to_string 0x515760, vector3_to_string_buf 0x515800, ptr_to_hex_string 0x515880) load `dest` into EAX
before the inline copy's `not ecx` (`mov eax, [esp+0x2c]` at 0x515687); `return strcpy(..)` loads it after.
With the two statements all six compile byte for byte (vc6-compare, /O2); the first demonstration of a source
shape found by the compiler.

## decomp71 (the party inventory unit 0x46b460-0x46c5cd, the item info popup 0x418580-0x419166, alt_textures 0x4aac10-0x4ab0ef, the join / tracker unit 0x4ab0f0-0x4abb73, the game tracker client 0x4ae330-0x4ae882, the level-object ray tests 0x4ae890-0x4af335; all placeholders)

**D5200 (no file). 0x43d190-0x43d510 and 0x419170-0x4191a0 need no new source.** The 39 functions of the first range
are compiler output of summoner.cpp's definitions that are already written: init_game_config_defaults is the inline
constructor of `SummonerOptions Summoner_options`, the nine sinit_ / init_ / register_dtor_ / dtor_ quads are the
nine `CmdLineParam cmdline_param_*` objects, init_ccmd_gamma_thunk / init_ccmd_gamma is `console_command
ccmd_gamma(...)`. summoner.cpp lists them in comments (`0x43d180 ...`, `0x43d220-0x43d265`, ...), as new_itemtrade.cpp
does for its two RealtimeTimer pairs 0x419170-0x4191a0. Their source line is the object definition, so nothing is
added; the tool that lists "functions without source" should count those comment ranges.

**D5201 (game/unnamed_party_inventory_0x46b460.cpp). The add_gold command ends the party inventory unit.** Its
initialiser pair 0x46c540 / 0x46c550 and cmd_add_gold 0x46c570 sit between character_wearing_servant_disguise and
item_paging.cpp's initialiser 0x46c5d0; the object 0x970810 is the lowest address of the inventory .bss
(0x970810-0x97186f), and its literals ("add_gold" 0x59a6e4 ...) follow the servant item names in the same .data
block. VC6 emits a dynamic initialiser where the object is defined, so `console_command ccmd_add_gold` is written
after the last inventory function, before cmd_add_gold (as summoner.cpp does for ccmd_gamma). sum.symbols' old
item_paging.cpp tag on 0x46c540-0x46c570 is not followed (source-map.md left it open).

**D5202 (game/unnamed_item_info_popup_0x418580.cpp). The item info popup is a unit of its own.** It starts with its
own dynamic initialiser (0x418580, static_initializers entry 53, the popup's RealtimeTimer 0x5c1544) after
new_inventory.cpp's last function (0x418540) and before new_itemtrade.cpp's initialisers (entries 54-55, 0x419170);
its .rdata 0x57fdbc-0x57fe3f, .data 0x58cbd8-0x58cd9f and .bss 0x5c1470-0x5c156b lie exactly between the two
screens' blocks. No name evidence: placeholder. The six functions other units call stay declared in interface.h;
the rest is in summoner/interface/unnamed_item_info_popup_0x418580.h, which also declares new_buysell.cpp's
buysell_cameras (defined there, declared nowhere before; the popup reads the shop's camera table for both screens).

**D5203 (engine/objects/objects.h). SIGNATURE: two byte parameters.** party_inventory_remove (0x46bb30) reads its second
argument with `mov al, [esp + 8]` and character_unequip_slot (0x46c240) its third with `mov al, [esp + 0x14]`; an
int would be read as a dword. Both are now bool (free_item, update_model). Every caller passes 0 or 1 and compiles
unchanged. Parameter names of party_inventory_add_item / _add_type / _add_by_name (count, charges, sort) from the
code: charges < 0 means the charge effect's maximum, sort calls party_inventory_sort.

**D5204 (game/unnamed_party_inventory_0x46b460.cpp). Breakable charged items.** character_use_inventory_item and
character_use_equipped_item treat ItemType charge_effect->disposable as: >= 1.0 one unit (stack) or charge used, the
item removed at 0; a fraction is the chance that the item breaks on this use, otherwise one charge goes but the
item is kept at 0 charges (it can no longer be used: the first test is charges > 0). Written as the code is.

**D5205 (character_equip_item 0x46bdf0). item_is_identified is called twice.** For a party member (type 7) the code
calls item_is_identified(item) and, when it is true, calls it again (0x46be20, 0x46be31) before refusing. Written as
`(!item_is_identified(item) || !item_is_identified(item))`; the second call may be another function folded with it
by the linker (identical code).

**D5206 (source-map 0x4aac10-0x4abb7f). Two units, cut at 0x4ab0f0.** alt_textures (0x4aac10-0x4ab0ef) uses only
.data 0x59d39c-0x59d3c3 and .bss 0x2476bb8-0x24794bb; static_init_mp_join_timer (0x4ab0f0) starts the join / tracker
code, whose initialised data (0x59d3c4, the accepted checksums) and .bss (0x24794bc-0x2479727) follow. Written as
game/unnamed_alt_textures_0x4aac10.cpp and game/unnamed_mp_join_0x4ab0f0.cpp.

**D5207 (alt_textures_parse_table 0x4aac10). Two unchecked buffers (latent bugs).** The "$Base:" name is parsed with
a limit of 0x28 into a 0x21-byte row of alt_texture_bases (0x4aad01-0x4aad0f), so a name of 33..40 characters runs
into the next row; the texture names of a set are stored without a count check (15 slots: bitmaps +0x2c, names
+0x6a), so a 16th texture overwrites the next set. The shipped table was not checked against either limit.

**D5208 (objects.h equip_page_slots 0x5819d4). Not defined in this unit.** The table sits in .rdata after objects.cpp's
225.0f (0x5819d0) and before shared float constants; nothing in 0x46b460-0x46c5cd reads it (inventory_page_in_equipped
and mp_page_in_player_items do). It may be this unit's or item_paging.cpp's; left extern, owner open.

**D5209 (engine/rendering/levelrender.h). lobj_ray / lobj_hit fields.** lobj_ray +0x28 is a float limit
(lobj_raycast_all: `fld hit.t; fcomp [ray + 0x28]`, 0x4af2fd): union `stop_t` with `_unk28` (existing writes of
_unk28 compile). +0x2c is the face tested with flag 2 (lobj_raycast pushes it as the face argument; living_entity.cpp
already stores a face pointer cast to int): union `face`. lobj_hit +0x14 is the plane normal of the hit (the face
tests copy their normal there): `vector3 normal` replaces `int _unk14[3]` (no code used _unk14). +0x20 is 1 for an
edge / vertex hit of the sphere test. New declarations: lobj_ray_test_geometry / _faces, lobj_sphere_test_faces,
lobj_raycast_bsp, lobj_last_hit_face / _index.

**D5210 (mp_handle_join_request 0x4ab350). The version check cannot fail against a 1.40 client.** The host accepts a
JN_RQST whose last dword equals its items_table_checksum or one of {0xf174baca, 0x6e4c08c4} (0x59d3c4, count
0x59d3cc); the 1.40 sender (mp_send_jn_rqst, 0x470cad) puts the constant 0xf174baca there, not its own checksum. So a
client with a different items.tbl is never refused (deny 8 needs another build). Comment only.

**D5211 (mp_change_level 0x4ab780). Dead slot search.** The loop over the level's seven conversation_save_table slots
looking for "<level>_m1" (0x4ab7d5-0x4ab7ff) leaves its index in ebx, which nothing reads. Written as a loop whose
result is unused.

**D5212 (tracker_start_game 0x4ab960). The channel copy is unchecked.** tracker_game_channel (0x100 bytes) is copied
with strcpy into the 0x24-byte channel field of the 0x58-byte record, which ends where tracker_game_channel itself
begins (0x2479520): a channel name of 36 or more characters overwrites the start of its own source. IRC channel
names of the lobby are at most 0x20 characters (lobby_channel name[0x21]), so it is latent.

**D5213 (source-map 0x4ae330-0x4af320). The game tracker client and the ray tests are two placeholder units.** The
tracker's .bss runs from 0x247b0a0 to 0x247e818 (the last block is the 0x5f1-byte heartbeat packet at 0x247e228; the
source map's 0x247e23d is only its last referenced field), the ray tests' from 0x247e81c; the tracker code has its
own literals (0x59d618-0x59d647) and shares nothing with the geometry code. Whether the tracker is the tail of
multi_chattracker.cpp (its .bss follows that file's directly) stays open. Files: game/unnamed_gtrack_0x4ae330.cpp,
game/unnamed_lobj_raycast_0x4ae890.cpp, headers summoner/multi/unnamed_gtrack_0x4ae330.h (and
unnamed_mp_join_0x4ab0f0.h for the join unit). The code matches the PXO game tracker client of Volition's published
FreeSpace 2 source in structure (packet header len / game_type / addr / type / sig / data, 20 list buffers, the
`i = MAX + 1` loop exit); that is not name evidence under the project rules, so no name is taken from it.

**D5214 (gtrack_send_heartbeat 0x4ae410). The length test.** A received packet is handled only when the datagram size
equals its len field minus one, and the decremented value is stored back (`dec ecx; cmp eax, ecx; mov [len], ecx`,
0x4ae572-0x4ae575). Written as `size != (int)--in.len`; whether the server counts one byte more or this is an
off-by-one is not known.

**D5215 (lobj_ray_test_faces / lobj_sphere_test_faces). Back faces use different flags.** A thin ray accepts a face
seen from behind when face flag 0x200 is set (`test byte [esi + 1], 2`, 0x4aea9b), the swept sphere when flag 0x20
(texture with alpha, s3d_mark_alpha_meshes) is set (`test byte [edi], 0x20`, 0x4aec6f). Written as the code is; one
of the two may be a typo of the other (unsure, comment only).

**D5216 (lobj_raycast_bsp 0x4aeef0). A full node stack drops a subtree (latent).** When 128 crossed nodes are pending
(`cmp edx, 0x80; jae`, 0x4af0a5) the current node is neither pushed nor descended: its faces and both children are
skipped. Needs a tree deeper than 128 crossed levels along one segment; not seen.

**D5217 (engine/characterinfo/characterinfo.h). alt_texture_set is 0x2d4 bytes with a name.** alt_textures_parse_table
allocates n * 0x2d4 and parses the "+Textures:" set name into +0x00 with a limit of 0x28; alt_textures_find_entry
compares +0x00. `ubyte _unk00[0x2c]` becomes `char name[0x28]` + `_unk28[4]`, LAYOUT_CHECK 0x2d4 added. No code used
_unk00.

**D5218 (engine/objects/objects.h, game/unnamed_party_inventory_0x46b460.cpp). The party inventory globals and the two
unreferenced tables are defined here.** party_inventory (0x970820), party_gold (0x971868) and party_inventory_count
(0x97186c) had declarations but no definition. .data 0x59a660 {"Weapons", "Armor", "Magic", "Jewelry", "Misc"} and
0x59a674 {0, 1, 2, 3, 4} have no reference in Sum.exe; they are this unit's initialised data (between the servant
list 0x59a63c and its literals), so they are written as shop_category_names / shop_category_ids (names ours).

## decomp70 (0x550660-0x5534d0, 0x553ba0-0x5548b0, 0x562460-0x562550, 0x562a20-0x564d60, 0x565610-0x5664a0)

**D5100 (unit boundaries).** Six placeholder units. anim_track 0x550660-0x5519b9 (vsdk\ca; no data; its
inline accessors 0x551770-0x55195d and quaternion::unpack_short4 0x551960 are the closing COMDATs), the polygon
clipper 0x5519c0-0x5520c0 (vsdk\geom; opened by initialiser 0x5519c0, table 0x588a20; .bss 0x2f13368-0x2f136e7),
CRC-32 0x5520d0-0x552192 (.bss 0x2f136e8-0x2f13ae8, the next block), ui_inputbox 0x5521a0-0x5534e1 (ends before
ui_container.cpp), ui_checkbox / ui_radio_button 0x553ba0-0x5549b9 (after ui_container.cpp, before targautils.cpp),
and the vfx mesh object 0x562a30-0x564d62 (D4611). The anim_track code and the polygon clipper are separate
objects: the clipper's initialiser opens its own object. Whether the checkbox code is one object with
ui_container.cpp's neighbours cannot be told (no data); it is one placeholder. 0x562460-0x562540 / 0x562a20 (the
emitter accessors) and 0x565610 were already written (inline in vfx.h / effect_mem.h, listed in the emitter and
material files); 0x565660-0x5664a0 is libjpeg (identified in decomp66, not rewritten).

**D5101 (skeleton.h). anim_track's scale keys.** +0x04 is a ushort count (init_tangents and get_scale loop over
`word ptr [esi+4]`), +0x10 a pointer to 0x28-byte keys (scale_key_value reads +0x10, stride 0x28, +4) and +0x24 /
+0x28 their control points (stride 0xc), as the position keys. Were int _unk04, int _unk10, ubyte _unk24[8]; no
code used them. The rotation key bytes +0x0c..+0x0e are tension, continuity and bias (the accessors at 0x551840,
0x551820, 0x551860); the field names (tangent[3], ease_in / ease_out) are kept for the cutscene camera reader.

**D5102 (quaternion.h). unpack_short4 is inline.** Its only copy (0x551960) closes the anim_track object, which
was built without inline expansion (every vector3 / cquat operator is called there); the vfx units call that
copy. Declared inline after cquat (the body needs the complete type); no definition existed.

**D5103 (skeleton.h, SIGNATURE). init_tangents and get_rotation take bools.** Both pass their flag on as the
dword they got (`mov ebx, [esp+0x28]; push ebx`, 0x551004; `mov edx, [esp+0x28]; push edx`, 0x5513f7) and the
callees test a byte (`mov bl, [esp+0x70]` in calc_rot_tangents, `mov al, [esp+0x2c]` in
anim_track_interp_rotation): with an int parameter VC6 would normalise it to a bool first. Callers pass 0 / 1.

**D5104 (ix.h, SIGNATURE). polygon_touches_planes' offset is a float.** It is passed on to plane_classify_point,
which loads it with FLD (0x551e0e). Was int; the three callers pass 0.

**D5105 (ui_gadgets.h, SIGNATURE). ui_inputbox::set_valid_chars / set_invalid_chars / set_text return int.**
`xor eax, eax` / `mov eax, 1` (0x55288e, 0x552b8e; 0x553044, 0x553064): a bool would be returned in AL (as
is_char_allowed does, `xor al, al` / `mov al, 1`). Were void; no caller reads the result. ui_inputbox gets its
four virtual slots (0, 5, 7, 8 of 0x581f1c) and its fields (+0x274 flags, the three 0x100-byte strings, +0x57c the
colour pointer; were _unk274 / _unk57c). ui_checkbox gets its fields (were _unk274[0x120]) and toggle.

**D5106 (vfx.h). The mesh descriptor.** vfx_node_desc's layout from vfx_node_desc::relocate (0x5645b0: the
pointers +0x38 .. +0x6c, the name +0x94 against the header base, the BitArray +0x44, the header +0x00..+0x0b
through vfx_node_header::relocate), update_frame (+0x0c flags and frame rate, +0x0e .. +0x16 counts, +0x30, the
rest pose +0x70 / +0x7c / +0x84, +0x9c) and render_mesh. The header relocation is written as a cast to
vfx_node_header (the call passes `this`); the BitArray as a POD mirror (frames_data ..) with a cast in frame_bit and
relocate, because it shares a union with D5107's alias. The records it points to are new types in vfx.h (names
ours; the accessor names are sum.symbols').

**D5107 (vfx.h). glow_pos is not a mesh field.** decomp63 wrote lamp_glow_update_effects as reading
rec->glow_pos (+0x48 of the mesh descriptor), but the code reads `[eax+0x48]` with EAX = effect_get_base's result
(0x484690): the vfx_base's pos. +0x48 of the descriptor is inside the frame BitArray. Kept as a union alias of POD
type (vfx_vector3_alias, convertible to vector3) so unnamed_render_scene_0x481680.cpp compiles; that file should
read base->pos (NOTES.md).

**D5108 (vfx.h). Face normals are signed bit fields.** vfx_unpack_vector_s8 (0x564a20) extracts the three bytes
with SAR (signed) and maps each with * (1 / 127.5) - 1, the unsigned mapping the vertex normals use with SHR
(0x5648c0). Written as `int nx : 8` fields so the code matches; the mismatch is listed as a bug.

**D5109 (vfx.h). The billboard size accessors fold with the particle's.** vfx_mesh_sprite::get_w / get_h (the
words +0 / +2 as 8.8) have the code of fx_rec28::get_age / get_lifetime; the COMDATs 0x564c60 / 0x564c70 sit in
the mesh object's accessor block, so the mesh's copies are the ones the linker kept. The addresses stay on the
fx_rec28 declarations (one declaration per address).

**D5110 (vfx.h). The mesh scratch arrays.** vfx_mesh_verts (0x272b900, 1820 vector3) is defined in effect_mem.cpp;
vfx_mesh_face_uvs (0x2c98a90-0x2c9f1af, 2200 x 0xc bytes: the "not placed" hole of effect_mem.cpp's .bss) is
used only by the mesh object but lies in effect_mem.cpp's .bss, so its definition belongs there (not added: the
file is the lead's). Both declared extern in vfx.h.

**D5111 (gr.h, unnamed_gr_3d_0x50d9b0.h).** gr_poly_rgb (0x545610) and gr_bitmap_3d_oriented (0x53f410) declared
(the mesh object calls them). gr_sort_entry's value gets a ushort alias (index): render_mesh stores and reloads it
with word moves (0x563326, 0x5633bb). render_mesh passes the sort key (a float) as gr_poly_begin's second int
argument (`mov eax, [edx]; push eax`, 0x563481): written as a reinterpreting cast; gr_poly_begin's parameters are
not changed.

**D5400 (savegame.h, level_scripts.cpp, spells.h, spells.cpp). Two fixes from the VC6 byte comparison.**
`savegame::layout_two_stage` (0x4d1bb0) uses the section size 0x673f, but `sizeof(savegame_two_stage)` is 0x6740
under VC6 (the int members pad the struct to 4): the code now uses `SAVEGAME_TWO_STAGE_BYTES`, the byte count of
counts, hashes and the 0x607 bit bytes. `spell_spawn_hit_fx` (0x47e590) takes (caster, pos, owner, orient): the
fourth argument goes to `fx_add_effect` as the orientation (`[esp + 0x50]` at 0x47e5a0) and the third into the
avoidance owner (`[esp + 0x4c]` at 0x47e62a); its caller at 0x47cd93 pushes the object's orientation (+0x38) last.

## decomp69 (the 3D drawing helpers 0x53e520-0x54080f, the File array helpers 0x544e50-0x5450bb, the polygon builder 0x5450c0-0x54614a, the sound emitters 0x546150-0x546cf0, Perlin noise 0x546d00-0x54738e, the Direct3D texture cache 0x54bb30-0x54d5b5; all placeholders; gr::Mode inlines 0x5368a0-0x536930)

**D5000 (unit boundaries).** Six new objects, one file each, all `unnamed_<what>_<first address>.cpp` in
src/decomp/vsdk (no path string, no strings.tbl block, no module evidence beyond the folders): the 3D
drawing helpers (0x53e520, after cmdline.cpp's last function 0x53e4e0, up to the gr::Vertex assignment
COMDAT 0x540790; character.cpp opens at 0x540810 with its initialiser), the File arrays (0x544e50, after
stringpool.cpp's SetLength 0x544e30; no data, no initialiser), the polygon builder (0x5450c0, opening with its
two initialisers 0x5889fc / 0x588a00; .data 0x5aca38-0x5aca47, .bss 0x2ee45d8-0x2f076eb), the sound emitters
(0x546150, two initialisers 0x588a04 / 0x588a08; the message 0x5aca48, .bss 0x2f076f0-0x2f0b16f), the noise
(0x546d00, .data 0x5aca88, .bss 0x2f0b170-0x2f0dd9b; gr_glide.cpp follows at 0x547390 with .data 0x5aca8c)
and the Direct3D texture cache (0x54bb30, initialiser 0x588a18; the message 0x5ad7a4, .bss
0x2f132d8-0x2f13317; gr_opengl.cpp follows at 0x54d5c0). Each .data / .bss block sits between its
neighbours' in the same order as the code, and each object starts with its own initialisers or ends with its
own COMDATs, which is the boundary evidence. The gr::Mode accessors 0x5368a0-0x536930 belong to part 1 of
gr_direct3d.cpp (D1601); their markers are in gr_direct3d_additions_0x5368a0.cpp for the lead to fold in.

**D5001 (unnamed_gr_line_0x53e520.cpp). gr_line_3d and gr_line_3d_world are in the same object as the console
command.** The command's initialiser (0x53e670) follows them instead of opening the object: VC6 emits a
dynamic initialiser where its global is defined, as in gr_glide.cpp (ccmd_whee), so the command is defined
after the two functions. They are vsdk\gr like the rest, and cmdline.cpp (vsdk\os) ends before them.

**D5002 (gr.h). gr_vertex keeps no constructor and no vector3 member (D1605 stays open).** This range adds
more evidence for the original's gr::Vertex: every local vertex is constructed with gr__Vertex__Vertex
(0x5081c0), the polygon builder's 64 clip vertices have an initialiser (0x5450c0), and gr_draw_sphere's
vertex assignments call the implicit operator= COMDAT 0x540790, which assigns +0 through
vector3::operator= (0x413190, a hidden-pointer copy) and the other fields one by one. Giving gr_vertex a
`vector3 pos` would change the x / y / z accesses of 80 call sites in other units; as in D4703 the
definitions stay plain and the functions concerned are marked likely.

**D5003 (gr.h). gr_poly is (n, verts, gr::Mode mode, bool fixed_depth, float depth) and returns bool.** It
calls gr__Mode__get_alpha_source with ecx = its argument slot (0x53e773), so the mode is a gr::Mode object;
the last two go unchanged to gr_poly_begin's fixed_depth / depth; it ends with `call gr_poly_end` and
gr_bitmap_3d_projected returns that EAX. Was (int n, gr_vertex **, unsigned int flags, int a, int b), void.
The decals call (0x118c21, 0, 0) still compiles through Mode(gr_mode) (the optimised unit folded the inline
six-field constructor to the constant in the binary).

**D5004 (gr.h). gr_bitmap_3d, gr_draw_sphere, gr_draw_arrow_3d and gr_line_quad_3d return bool.** gr_bitmap_3d
returns gr_bitmap_3d_projected's result or `xor al, al`; the other three end with `mov al, 1`. Were void;
no caller reads the result. sum.symbols' gr_draw_arrow_3d is a wire box (8 corners, 12 edges); the name is
kept.

**D5005 (unnamed_gr_line_0x53e520.cpp). `!pnt->flags & 1`.** gr_bitmap_3d_projected and
gr_bitmap_3d_oriented_quad test the projected point with `test cl, cl; sete dl; test dl, 1` (0x53eb3a,
0x53ee2a): the logical not of the whole byte, then & 1, VC6's code for the precedence slip of
`!(pnt->flags & 1)`. Written as the slip.

**D5006 (unnamed_gr_line_0x53e520.h). The three dead arrow / line helpers take nine scalars.** Each builds its
points with vector3(x, y, z) (0x43b580) from six float arguments, and gr_draw_axes_3d pushes the raw
components and the gr_color bytes; a by-value vector3 would have been copied with vector3's copy
constructor (0x410cd0, out of line in this object, as gr_draw_axes_3d's own copy shows). sum.symbols says
"(a, b by value)".

**D5010 (gr.h). The polygon builder's signatures.** gr_poly_begin(bool buffered, float z, int a2, bool
fixed_depth, float depth): the byte test of the first (0x545151), the float of the second as the buffered
sort key, the third stored as a dword (0x54522b) and pushed as a byte when recorded, the fourth a byte
(0x54524b), the fifth stored to gr_poly_depth. gr_poly_set_texture(gr::Mode, int bm, ubyte r, ubyte g,
ubyte b, ubyte a, int stage): the colours are masked with 0xff before the tint sum (0x54535f-0x545364) and
the last is the pass's stage byte (+0x0b). gr_poly_end returns bool (`mov al, 1` / `xor al, al`). Were
(int, int, int, int, int), (gr::Mode, int, int, int, int, int, int flags) and void. The shadow unit's calls
compile unchanged.

**D5011 (unnamed_gr_poly_0x5450c0.h). The builder's vertex records are their own 0x34-byte type.** The vertex,
clip and cursor arrays step by 0x34 (four passes of 12 bytes after the gr_vertex pointer), while gr.h's
gr_multipass_vertex is declared with one pass (0x10) and other units index past it. gr_poly_vertex_rec has the
same layout with four passes; the lists handed to gr_tmapper_multipass / the clipper are cast to
gr_multipass_vertex **. gr_poly_texture_slots is a gr_tmapper_pass[4] (the 0x14-byte record of gr.h: mode,
bitmap, enabled, vertex_color, vertex_alpha, stage, sectioned, r g b, a); its initialiser runs the implicit
constructor 0x546110 (gr::Mode() on +0), sum.symbols' gr_mode_slot__ctor.

**D5012 (unnamed_gr_poly_0x5450c0.cpp). gr_poly_rgb's tint.** The red result is computed and dropped
(`fstp st(0)` after float_min, no _ftol, 0x54567b), the green result goes to the r argument (0x5456bb) and g
keeps its value: VC6's code for `r = ...tint_r...; r = ...tint_g...;` (the first store is dead, the float
calls stay). Written so; gr_poly_set_texture has all three.

**D5013 (unnamed_gr_poly_0x5450c0.h, unnamed_poly_split_0x55f110.h). gr_poly_free_clip_verts and
gr_poly_alloc_clip_vertex are header inlines.** They follow the pass record's implicit constructor COMDAT
(0x546110) after the last regular function, in the order of their first use, as the clipper's
gr_clip_free_temp_verts does (D4500). The split unit's declaration of gr_poly_alloc_clip_vertex is replaced by
an include of the new header (no second declaration).

**D5014 (unnamed_gr_poly_0x5450c0.cpp). The buffer helpers and the replay.** put_vector / get_vector copy 12
bytes with the cursor update between the moves, as put_data / get_data's memcpy (rep movsd, cursor store,
rep movsb): memcpy with sizeof(vector3). The replay loop is a do / while on the opcode around the switch: the
default case (an unknown byte) reads the next byte, case 8 leaves; each case's locals are block-scoped (case
4's gr::Mode is constructed inside it, 0x545efa).

**D5020 (gr.h). tcache_slot's four members are header inlines.** Their copies (0x54d490 init_stage, 0x54d4b0
reset, 0x54d4f0 is_bound, 0x54d5a0 invalidate_bitmap) lie after the texture cache's last regular function,
interleaved with the Direct3D slot's own members in the order of first use there; gr_glide.cpp's calls of
is_bound / invalidate_bitmap reach the same copies. Declared out of line before; the bodies are now in the
class. D2502 (no tcache_slot constructor) is unchanged.

**D5021 (gr_direct3d.h). d3d_tcache_slot.** The two stage records (0x2f132d8, 0x18 bytes each) are built by a
constructor (0x54d470: initialized 0, bitmap / section -1, scales 0, stage untouched) that the Glide record
does not call, so they are a class of their own deriving from tcache_slot, with the constructor and the
Direct3D bind / unbind as inline members (0x54d4c0, 0x54d530). sum.symbols names the constructor
d3d_tcache_slot__ctor and its initialiser init_d3d_tcache_slots_2f132d8; the OpenGL back end has its own cache
(gl_tcache_entries) and does not use it. d3d_unbind skips SetTexture while d3d_s3_depth (0x2dc6c30) is set.

**D5022 (gr_direct3d.h). gr_d3d_tcache_set's last argument is bool bind.** It is tested as a byte
(`mov al, [esp + 0x28]; test al, al`, 0x54d0f3) and only decides whether the section is bound; a missing or
dirty texture is created either way. Was int fail_on_full; the callers pass 0 / 1.

**D5023 (gr_direct3d.h). The cache records.** d3d_tcache_entry (0x10: bitmap handle, section count, dirty
byte, section array) and d3d_tcache_section (0x20: surface, byte size, u / v scale, x, y, w, h) from
gr_d3d_create_texture, gr_d3d_create_texture_section and gr_d3d_get_section_rect; names ours (the Glide
cache's names where they match). The two dwords 0x2f1330c / 0x2f13310 have no reference.

**D5024 (unnamed_d3d_tcache_0x54bb30.cpp). gr_d3d_copy_bitmap_to_surface.** One branch per bitmap format in
the order of the binary (8 alpha, paletted, 888, 4444, 1555, 565, 8888, 88), as gr_glide_create_texture_section;
the row tails and the rows below the bitmap are cleared with loops (rep stosd / adc / rep stosw after a
`count > 0` test, the shape gr_glide.cpp also writes as loops). The or'ed channel terms are in the binary's
order. The loops run over bm_w x bm_h (the arguments the section function passes: the whole bitmap's size)
from the section's corner, not over the section's w x h: written as the binary does (bug, NOTES.md).

**D5030 (snd.h). sound_create_emitter's second argument is the update interval.** It is stored at +0x48 and
sound_emitter_update_position re-arms the record's RealtimeTimer with it (0x546911); nothing treats it as a
volume. Renamed update_ms (was volume); the callers pass 100.

**D5031 (snd.h). sound_emitters_init and sound_emitter_delete return int.** Both paths of sound_emitters_init
end with `xor eax, eax`; sound_emitter_delete returns -1 (emitters off) or 0. Were void; no caller reads them.

**D5032 (snd.h). Emitter flag 8.** sound_emitter_stop, sound_emitter_play (after a successful start) and
sound_emitter_delete clear bit 3 of +0x50 and sound_emitter_update_volume / update_position only restart a
sound while it is clear, but nothing sets it: kept as SOUND_EMITTER_FLAG8 with no meaning given.

**D5033 (snd.h, vector.h). The emitter record and vector3::operator!=.** sound_emitter (0x74 bytes) has no
user constructor: 0x546c80 constructs exactly the five vector3, the RealtimeTimer and the GameTimer members,
the implicit constructor, emitted as a COMDAT after the unit's functions. vector3::operator!= (0x546cc0,
exact compare of the three components, ret 4) is a new inline member of vector3; its only copy is this
object's COMDAT.

**D5034 (snd.h). sound_emitter_set_pos_vel is (emitter, a, b, vel, scale).** It stores the segment ends +0x5c /
+0x68 (each only when not NULL) and calls sound_emitter_update_position(emitter, &seg_a, vel, scale); the
sum.symbols comment "(idx, pos, vel)" and "+0x5c and +0x68 vectors" describe the same stores. No caller.

**D5040 (unnamed_perlin_0x546d00.h, levelrender.h). The noise unit.** The float version of Perlin's reference
noise: noise1 takes and returns a float (fld dword, no double), noise2 / noise3 take their vector by value
(the callers of noise3 store three floats on the stack; noise2 reads two). perlin_noise3's declaration moves
from engine/rendering/levelrender.h to the new header, which levelrender.h includes, so its callers compile
unchanged.

**D5050 (file.h). The File array helpers return the first File::error.** The readers and writers return the
error (`test eax, eax; jne` to the exit with EAX kept) or 0 after the last element, so they are int; the two
syncs end with the callee's EAX and no caller reads it, so they stay void. 0x544f80 (an int array reader
with no symbol and no caller) is file_read_int_array (name ours).

## decomp72 (the keyboard 0x5093a0-0x509ffb, the debug stubs / debug.cpp / free movement 0x531c10-0x532353, the time formatter / timer.cpp / vif particles 0x532660-0x533dae, the dialogue table 0x4feb50-0x500a22, the cutscene scripts 0x4ed050-0x4ed7be, eleven level-script units)

**D5300 (game/level_script_*.cpp). File names of the level scripts without a strings.tbl block.** lenele2d,
lenele2ab, IonaExt, rand-forestnite1, rand-forest01, rand-hills01, IkaemosBottomInt, masad and test read no id of
a `// level_script_*.cpp` block of strings.tbl and no path string names them (lenele2d's refusal reuses id 502 of
the lenele2aa block, the same English line). They are named level_script_<record>.cpp after the convention of the
tagged units (lower case, the record's '-' as '_'), which the lead asked for; the names are not proven and are not
in modules.txt. The random-encounter records (0x28-0x2f: Rand-HillsNite01 .. Rand-GrasslandNite01) have no single
level name, so their code (0x4ee840-0x4eee32) is the placeholder game/unnamed_rand_levels_0x4ee840.cpp; whether it
is one file, seven, or lenele2d's tail is not decidable (no .bss, literals without a gap).

**D5301 (unit boundaries). A 4-byte step in .bss between two runs of bools is taken as a unit boundary.** A unit's
bool globals are packed byte by byte (eleh 0x257e9ec-0x257e9ef, templeint 0x257ea6c-0x257ea76); each object's .bss
starts 4-aligned. rand-forestnite1's bools end at 0x257ea65 and rand-forest01's start at 0x257ea68; masad's end at
0x257ed5d and test's start at 0x257ed60; liangshan's end at 0x257ea61 before rand-forestnite1's 0x257ea64. So
rand-forestnite1 / rand-forest01 and masad / test are two units each (source-map.md had them as one run). A hint,
not a proof: units with no bools in .bss (lenele2d, ikaemosbottomint, the random levels) cannot be cut this way.

**D5302 (level scripts). Two bool stores of the same constant are written as two statements.** The code stores
some pairs from AL (mov al, 1 / two byte stores: ls_rand_forestnite1_init, the cutscene ends) and others as two
immediates (ls_rand_forest01_init, ls_rand_hills01_init). level_script_temple_inside.cpp writes the AL form as two
statements and calls it exact (ls_templeint_tiger_intro_cs_end 0x4f62b0); the same here. The init whose two paths
both use AL is marked likely.

**D5303 (game/level_script_ikaemosbottomint.cpp). IkaemosBottomInt's init is a unit of its own.** It has no data and
no own literal; it lies between level_script_ikaemosu.cpp (IkaemosTopInt) and rand-hills01 and could be either's.
By the pair ikaemosu.cpp / ikaemosl2.cpp (IkaemosBottomInt2's file) its original may have been
level_script_ikaemosl.cpp; that is a guess, so the D5300 convention is used.

**D5304 (level_script_rand_hills01.cpp). 0x4fa6f0 is a function: ls_rand_hills01_goto_khosanistrng (name ours).**
No sum.symbols entry. ls_rand_hills01_frame reaches it only by a tail jump and it sits after the three cutscene
callbacks with its own push esi / edi and ret, so it is a separate function defined after them (rand-forest01 has
the same level change inline in its frame).

**D5305 (level_script_masad.cpp). ls_masad_init's three door blocks are written out.** Each is lobj_find_by_name,
door_from_lobj, two_stage_get / two_stage_set and the snap to the open pose, as new_tutorial.cpp's
tutorial_cond_combat_part2 writes the same code. They may have been a macro; a function would be a call under /Ob1.

**D5306 (game/unnamed_cutscene_script_0x4ed050.cpp). The cutscene-script unit owns the cutscene tables.** The unit
(initialiser entries 474-476) has no name; its .bss 0x256b750-0x257e997 holds cutscene_navpoints,
cutscene_script_stringpool and its 0x2800-byte buffer, cutscene_script_objects (the implicit array constructor is
init_cutscene_object_timers), the two counts, cutscene_time and cutscene_script_name; they are defined there (no
other unit defined them). The declaration order follows the three initialisers (script objects, the empty
initialiser, the string pool), the same three as level_scripts.cpp's 0x4d4ff0-0x4d5080. The empty initialiser
0x4ed090 is taken to be the navpoint array's (navpoint holds a vector3, whose constructor is inline and empty);
likely, not visible in the code.

**D5307 (levelscripts/level_scripts.h). SIGNATURE: cutscene_script_load returns bool (was void).** It returns AL 0
or 1 (0x4ed239 / 0x4ed36d). Its callers (cutscene_play, script_editor_load_selected_script) ignore the result and
compile unchanged.

**D5308 (levelscripts/level_scripts.h). MAX_CUTSCENE_NAVPOINTS is 0x200 (was 0x2d6).** sum.symbols sizes
cutscene_navpoints 0x8818 bytes, the distance to cutscene_script_objects, but cutscene_script_stringpool (0x2571750)
and its buffer (0x2571764-0x2573f63) lie in between: the table is 0x6000 bytes, 0x200 records, and the script
editor caps the count at 0x200 (cmp dword ptr [0x257e96c], 0x200 at 0x4e5d59). cutscene_script_load does not check
it (see NOTES).

**D5309 (levelscripts/conversation.h). conversation_topic::add_line has no address of its own.** parse_table appends
a line with mov ecx, topic / call 0x4fec50, conv_speaker__add_topic: the topic's lines (+0x10 / +0x14, next +0)
and the speaker's topics have the same layout, so the two identical members were folded into one copy. The member
is declared (with no address, "folded with conversation_speaker::add_topic") and defined after add_topic. Also
declared: dialogue_action::init (0x500990, dialogue_rec__init4) and conversation::parse_cflags (0x4fed30,
dialogue_parse_cflags; thiscall with an unused this, ret 4) with class TextFile forward-declared.

**D5310 (game/unnamed_dialogue_table_0x4feb50.cpp). The dialogue table unit is a placeholder.** No path string, no
strings.tbl block, no reference name for 0x4feb50-0x500a22 (the record members and conversation::parse_table);
conversation.cpp (0x4d7b30-) is another unit.

**D5311 (unnamed_dialogue_table_0x4feb50.cpp). The shape of conversation::parse_table.** The line-kind test
(+PCText: .. +JekharText:) and the topic-reference test (+Add topic: .. +Set hail topic:) each appear twice in the
code, before the first pass and after each, both ending in the same blocks: a rotated while loop over an inline
helper (dialogue_accept_line_kind / dialogue_accept_ref_kind, names ours). The reference loop's no_arg flag is
cleared once before the outer loop (the second copy of the test has no store). Every error inside the file is
{ tf.Close(); return -1; } (merged by the compiler into 0x5008c3), the errors after the Close return -1. The
function has no return statement at its end: the good path returns whatever EAX holds after the destructors,
which is 0 (the TextFile destructor's Close loads the already-NULL buffer pointer). Marked unsure as a whole.

**D5312 (vsdk/os/key.h). SIGNATURE: key_init(int layout, bool fix_numlock) (was int fix_numlock).** key_init stores
the argument as a byte and compares it with 1 (0x5097c5-0x5097cb). The caller (summoner.cpp, key_init(0, 1))
compiles unchanged. Also declared in key.h: the rest of the unit's functions (with windows.h for key_msg_handler).

**D5313 (vsdk/unnamed_key_0x5093a0.cpp). The keyboard unit is a placeholder; its data is defined there.** No path
string or assert. Its .data is key_repeat_enabled (0x5a64bc, name ours), two int[128] ASCII tables (0x5a64c0,
0x5a66c0: key_to_ascii reads 127 of each) and key_names[256] (0x5a68c0, window.cpp's hotkey parser reads it); its
literals are not pooled ("Shift" and "Alt" twice), and each "" of key_names points to a separate 4-byte .bss slot
(0x25daa7c-0x25dace4), zero-filled literals placed in .bss. The time arrays and timestamps are unsigned (key_mark
compares with jae, key_down_time with ja); key_last_released / key_last_pressed (0x25da948 / 0x25daa4c, written
only) are named here.

**D5314 (vsdk/unnamed_debug_stubs_0x531c10.cpp, vsdk/debug.cpp). The stubs before DebugError are a unit of their
own.** sum.symbols tags only DebugError and its helper debug_error_deactivate_gr with debug.cpp. stricmp_wrapper and
the seven bare-ret / return-0 hooks (0x531c10-0x531c90, module -) may be debug.cpp's head or the tail of
unnamed_os_0x531bf0.cpp's object; nothing says which, so they get a placeholder. debug.cpp holds the two tagged
functions; it is a new file (the include tree had only debug.h).

**D5315 (vsdk/unnamed_movement_0x531d20.cpp). The free-movement unit.** A placeholder (no name evidence). It calls
the vector3 constructors and operators out of line, as /Ob0 builds do. sum.symbols' movement_velocity (0x2cbff68) is
the velocity the keys ask for and movement_rotvel (0x2cbff98) the smoothed linear velocity (smooth_approach_exp
writes it); the names are kept. Names ours: movement_rot_tau (0x2cbff30), movement_turn_rate (0x2cbff38),
movement_turn_target (0x2cbff48), movement_tau (0x2cbff84), movement_inited (0x2cbffa4). smooth_approach_exp's
second argument is the wanted value and the third the current one (the code: new = second + (third - second) * e);
sum.symbols' comment has them the other way round.

**D5316 (vsdk/timer.cpp, vsdk/unnamed_time_format_0x532660.cpp). timer.cpp holds the four tagged functions and the
TSC conversion.** time_format_hms (0x532660, module -) precedes timer_close but has literals and timer.cpp has none,
so nothing ties it to timer.cpp: a placeholder. 0x5329c1 (no sum.symbols entry, no caller: ticks / timer_tsc_ticks
* 0.25) is written as timer_tsc_to_seconds (name ours). timer_get and read_tsc keep their __asm blocks (the EBP
frames show inline assembly), with a non-MSVC branch for the syntax check as in unnamed_floatmath_0x542500.cpp.

**D5317 (vsdk/geom/vfx.h, vsdk/unnamed_vif_0x5329f0.cpp). The vif records' layouts.** vfx_desc21c is filled in
(orient +0x14, size, spreads, speeds, the size / alpha ramps, count +0x68, bitmap +0x6c / name +0x70, the attract
offset, the force direction, two vif_curve key curves +0x90 / +0xf4 and the vif_spawn_path +0x158; was _unk14 /
_unk54 / _unk6c). vfx_inst21c gets emit_pos / emit_orient (+0x30 / +0x3c), time, interval, count, spawn_timer, first,
particles and the two list sentinels (+0x80 live, +0xb0 free; was _unk30 / _unk70 / _unk74) and a LAYOUT_CHECK of
0xe8. vif_particle (0x30) uses plain floats: the record constructor 0x533c70 builds only the two vectors and two
matrices, so the sentinels have no constructed members. Every existing user (unnamed_vfx_0x536950.cpp,
unnamed_render_scene_0x481680.cpp) reads only fields that kept their names and offsets. vif_emitter_update keeps
D4704's int last argument (the effect time's raw dword) and reads it as a float. The unit is a placeholder, built
with the inline members out of line (/Ob0).

## decomp74 (the unwritten runs of runs-b: initialiser and COMDAT markers, seven level-script units, the loading progress, Bink movies, InetGetFile, the sysinfo code, the reliable-socket getters)

**D5600 (game/level_scripts.cpp, game/unnamed_ai_think_0x4b5140.cpp). The empty initialisers stay unwritten.**
init_empty_4d5030 and init_empty_4b5150 are a `jmp` thunk and a bare `ret`. Tested with VC6 /O2 /Ob1 /GX: a global
array of a class whose inline constructor is empty (`struct V { float x, y, z; V() {} }; V a[4];`, as vector3) gives
exactly this pair (one per definition). So each is such a definition; which object is not known (the think unit's
two vector3 arrays would give two pairs, the binary has one), so none is written and the markers are unsure.

**D5601 (game/script_internal.cpp). str_copy_append_32 (0x4db250) and filename_has_cscript_suffix (0x4db2c0) are
written at the head of script_internal.cpp.** Untagged, no data of their own ("_cscript.tbl" 0x5939b4 is pooled),
between conversationctl.cpp's last ccmd (0x4db248) and script_internal.cpp (0x4db340); levelscripts/script_internal.h
already declares them. Both match VC6 byte for byte; the suffix test needs `if (strcmp(...) == 0) return true;
return false;` (the `return strcmp(...) == 0` form adds an instruction).

**D5602 (game/level_script_ikaemosext2.cpp, _lenele1ab, _khosanilab2, _tancredhouse, _khosanistrng, _rand_desert,
_ikaemosext). Seven level-script units named by the D5300 convention.** None has a strings.tbl block or a path
string; the names are level_script_<record>.cpp (lower case, '-' as '_'), not proven. ikaemosext2's two literals
continue level_script_ikaemosl2.cpp's .data (source-map.md), so it may be that file's tail; kept separate like
ikaemosbottomint (D5303). khosanilab2 is a unit of its own by its .bss (0x257e9f8-0x257e9f9, D5301).

**D5603 (game/level_script_khosanilab2.cpp). 0x4f22b0 is a function: lsf_khosanilab2_flag_pyrulstart (name ours).**
ls_khosanilab2_frame is `jmp 0x4f22b0` (as ls_lenele1c_frame, D2704); the body has no sum.symbols entry and is named
after its first flag. Both match. The first music start needs a temporary (`music = music_find_by_name("Pyrul");
music_play(music, true)`: the original pushes `true` after the inner call), the second is the nested form.

**D5604 (game/level_script_tancredhouse.cpp). ls_test_use (0x4f29c0) is defined in the tancredhouse unit.** Its body
(`xor al, al; ret`) ends the tancredhouse run, and every record whose use hook does nothing points at it (identical
code folding); sum.symbols names the folded copy after the "test" record, so the definition keeps that name. The
sum.symbols comment ("reacts (opens gates, speaks a refusal)") does not describe this body.

**D5605 (vsdk/unnamed_progress_0x503dd0.cpp, vsdk/bmpman/bmpman.h). The loading progress is a unit of its own; two
signature changes.** bmpman.cpp's object ends with its imin COMDAT (0x503dc0), and the progress data follows
bmpman's (.data 0x5a5c24, .bss 0x25c6898), so 0x503dd0-0x503f54 is a separate object (placeholder name). SIGNATURE:
`bool progress_update(int flags)` (was int: it returns AL and the callers test AL, e.g. 0x4387e4; gameplay.cpp's
0x4386c0 now matches) and `progress_set_callback(..., bool abortable)` (was int arg: `mov cl, [esp+0x14]`). Both
functions match.

**D5606 (vsdk/bmpman.cpp, vsdk/character_instance.cpp). imin and imax are defined out of line in the objects whose
COMDATs they are.** Both are header inlines in the original (copies at object ends), but vector.h declares them out
of line and every caller calls them; defining them inline would let /Ob1 expand them in the callers. So imin is
defined at the end of bmpman.cpp and imax at the end of character_instance.cpp (both match).

**D5607 (vsdk/unnamed_movie_0x5172f0.cpp). The Bink movie unit.** Placeholder name (movie_play's header vsdk/os/movie.h
is a guess too). The Bink entry points are declared `__declspec(dllimport) __stdcall` from the import table
(binkw32.dll 0x57e488-0x57e4a4); BinkSetSoundSystem gets the import address of BinkOpenDirectSound. movie_blit_frame
matches. movie_play does not: its File local (constructed, never used) sits at [esp] in the original, while VC6
places it above the path buffer in every declaration order tried (and in an inner scope); the while-loop form gives
the original's size.

**D5608 (vsdk/unnamed_os_0x530b90.cpp). The CPU / sysinfo code is written under `#pragma optimize("", off)`.** The
__asm blocks have a non-MSVC branch (D5316). cpu_read_tsc matches. In cpu_identify and PrintSystemInfo the code
follows the binary but VC6 /Od lays the locals out in an order that is not the declaration order (it changes with
the names), so the frame offsets differ: likely. The zeroed CPUID result is a struct (its first member is stored with
an immediate, the rest from EAX, as /Od does for `= { 0 }`); in __asm its vendor dwords are written as
`dword ptr [id + 4]` (`dword ptr id.vendor[0]` is C2443 to VC6).

**D5609 (vsdk/skeleton.cpp). ui_stub_ret_543030 (a bare ret, no reference) is skeleton.cpp's first function.** It
follows ui_element's closing COMDAT (ui_tooltip's scalar deleting destructor 0x543010), and an unreferenced COMDAT
would have been dropped by the linker, so it is an ordinary function at the start of the next object. Placement
unsure; the body matches.

**D5610 (vsdk/unnamed_net_0x517590.cpp). net_get_reliable_socket_status / _ping are the net unit's.** Module - in
sum.symbols, but they lie between nw_addr_equal and nw_flush_receive and index nw_reliable_sockets (status +0xcec,
mean_ping +0xd29). Both match (the id test is unsigned).

**D5611 (vsdk/unnamed_inetgetfile_0x51b760.cpp, vsdk/inetfile/inetgetfile.h). InetGetFile is a unit of its own.**
Between character_instance.cpp's last COMDAT (imax) and effect_mem.cpp, ending with its own COMDATs (the scalar
deleting destructors of http_get and ftp_get). Header: the getter fields are typed and renamed `ftp_get *ftp` /
`http_get *http` (were `void *ftp_get` / `*http_get`, which hid the class names; no other code uses them);
GetErrorCode (0x51ba70, no sum.symbols entry, name ours) and the INET_ERROR_* values (from the code: 1 bad
parameters, 3 neither "http:" nor "ftp:", 7 new failed; names ours) are added. Eight of ten functions match; the
destructor differs because the original calls the scalar deleting destructors where VC6 here expands
`delete` into the destructor and operator delete (why is not known), and the constructor in its EH layout.

**D5612 (game/level_script_khosanistrng.cpp). One stack cleanup in ls_khosanistrng_init is not reproduced.** In the
v4 branch the original adds `add esp, 0x10` after the two door calls, before printf("enabled kl2\n"); VC6 defers it.
A block, a `do { } while (0)` and a label before the printf change nothing; same size, likely.

**D5613 (marker convention). Every function address starts a comment line.** The earlier pilots described many
compiler-generated functions and COMDAT copies in running text ("0x4d5040 / 0x4d5050 / ..."), which the
unwritten-function survey does not count. decomp74 splits those comments so that each address begins a line
(`// 0xADDR name: ...`), one per line, in address order; the first line of a block still names the following
definition for vc6-compare.mjs, the later ones are notes. Bodies of header inlines stay in the headers.

## decomp73 (the unwritten run list A: static initialisers, header-inline COMDATs and helpers of existing units; the state sequencer 0x401000-0x4011ad and the icam tables 0x410760-0x410ce6 as placeholders; combat_trigger_reflex_ability; spell_prepare_cast, spell_cast_execute, spell_cast_launch_effects)

**D5500 (game/unnamed_gameseq_0x401000.cpp, summoner/summoner.h). The state sequencer is a unit of its own.** The first
seven functions of the executable (gameseq_init .. GameStateSwap, module -) end at 0x4011ad; new_assess.cpp's
static initialiser follows at 0x4011c0. Its .data is gameseq_state_table (0x588a70, 15 x 0xc) followed by its only
literal ("Packet type %d created %d time(s)\n", 0x588b24, read by GameTick), and new_assess.cpp's .data starts at
0x588b50: so the table is this unit's, and it is defined there (initialised from the 35 state handlers). Its .bss is
gameseq_state_cur / prev / next / pending (0x5b27e0-0x5b27ec), read by nobody else. A placeholder name (no assert
path, no strings.tbl block). `GameStateHandlers` (symbols/types.h) moves into summoner.h with the table and the four
variables; GameStateSwap and the state 7 / 9 handlers 0x430ad0-0x430b90 (not written yet; the run lists missed them
because nothing marks them) are declared there. All seven functions match VC6 byte for byte.

**D5501 (GameTick 0x401070). The packet counters are formatted and thrown away.** In multiplayer GameTick zeroes
mp_packet_table[i].sent before multi_do_frame and, after the state's frame handler, sprintf's "Packet type %d created
%d time(s)" into string_buffer for every counter above 0, with no output call: a debug print whose output was
compiled out. Kept as written (byte match).

**D5502 (game/new_automap.cpp). `vector constructor iterator` (0x404880) is marked in new_automap.cpp.** The
compiler-generated helper (??_H) sits between new_automap.cpp's last function (0x404870) and new_buysell.cpp's first
initialiser (0x4048b0). The linker keeps the first COMDAT copy in link order, so new_automap.obj is the first object
that defines it, although none of the unit's functions calls it in v1.40 (the empty initialiser 0x404870 is the only
candidate for an array of objects with an out-of-line constructor that VC6 later reduced). A marker comment only (no
source); confidence likely for the placement.

**D5503 (game/unnamed_icam_0x410760.cpp, summoner/interface/interface.h). The icam tables are a unit of their own.**
new_hud.cpp ends at 0x41075a (NOTES-hud.md) and new_int_multi_update.cpp's version check starts at 0x410cf0; the three
functions and their literals (0x58bcbc-0x58bd1f, between new_hud.cpp's .data and version_check_result 0x58bd3c) form
one unit. The function prefix suggests new_icam.cpp, but no string names the file: a placeholder. The table layout
(new_icam_data, 0x3628 bytes: count, a 0x21-byte file name, is_item[0x200], index[0x200] (ushort), offset[0x200],
pbh[0x200]) is from new_icam_load's stores; field names ours. interface.h keeps new_icam_table as the byte array
(D18) and the functions keep `void *`; the struct is local to the unit. new_icam_find (0x410af0) is declared in
interface.h. The message buffer of new_icam_get is block-scoped: VC6 shares its stack slot with the second matrix
product (frame 0xe4, which a function-scope buffer makes 0x108). All three match VC6 byte for byte.

**D5504 (unnamed_icam_0x410760.cpp). The vector3 copy constructor COMDAT 0x410cd0 is marked in the icam unit.**
new_icam_get's `*orient = pm * (bm * hm)` expands matrix::operator= inline, and the last row's vector3::operator=
returns a copy through the out-of-line copy constructor (0x410cd0): its first out-of-line use, and the COMDAT follows
the unit's last function. Marker only (vector.h has the inline body).

**D5505 (game/living_entity.cpp). The vector3 operator+ / operator- COMDATs 0x45b680 / 0x45b6b0 are marked there.**
They follow character_clip_move_to_navmesh (0x45ab00, their first out-of-line use, NOTES-living-entity.md) inside the
unit's range. Their code is optimised (no ebp frame, z first) although this part of living_entity.cpp is built /Od,
so either the COMDATs come from another object that was linked first or the optimised copy won; the markers only
record where they are.

**D5506 (game/combat.cpp, engine/spells/spells.h). combat_trigger_reflex_ability (0x467670).** The four casts (the
defender's reflex ability, then each worn item's armour triggered effect; each beneficial or not) are written out in
the binary; here one macro, COMBAT_TRIGGER_CAST (as COMBAT_SELF_CAST). Their request objects store spell = -1 and
caster = 0 before the five fields of spell_cast_request's default constructor (0x467727-0x467743), which the self-cast
and melee requests do not; VC6 removes no such dead stores (tested), so the requests here use a second constructor
`spell_cast_request(int no_spell)` that sets all seven fields (the argument is always -1; ours). With it VC6 produces
the same stores and size (1605 bytes) but keeps opponent->owner in callee-saved registers across frand, where the
original reloads it and keeps -1 in edi: likely. The item branch's single-player spell_cast_execute call of an
offensive, not hit-only effect passes the defender's own handle as the target (0x467c7c: `mov edx, [eax+0x80]` with
eax = opponent->owner) together with the attacker's position and layer; every other branch targets the attacker
(candidate bug K300 for game-bugs.md: an armour's offensive triggered spell is aimed at its wearer in single player).
spell_cast_execute keeps its `int a` (party_inventory passes `(int)type`); the macro casts the item type.

**D5507 (engine/spells/spells.h, game/spells.cpp). spell_prepare_cast (0x47abe0): SIGNATURE change.** Was (Character *,
int spell, int target, int a, int source_type, int b). The fourth argument is the casting item's type (the source
type switch reads its +0x94 weapon / +0xf8 armour triggered effect and +0x58 charge effect), the sixth a float:
combat_trigger_reflex_ability passes its float damage bit for bit and spells 0x3e / 0x3f multiply by it (0x47baae).
Now (Character *, int, int, ItemType *item, int source_type, float power); the existing callers pass 0 for both, which
compiles the same. spell_cast_params gets an anonymous union `ftarget[12]` over target[12] for the float slots (same
layout). spell_spawn_creature (0x47bd30, not written) is declared from the call 0x47b81c: (Character **out[4],
caster, spell), a plain out pointer (the callee does not return it). The case bodies are written in the binary's
order (VC6 emits them in source order). Behaviour notes: with No_resist set, spell 0x65 does nothing (the case returns
before its hit roll); the curse case with source types 1-4 and the drain case read the target's handle / type without
the NULL test the other cases make (latent: spells with SPELL_TARGET_CHARACTER return earlier without a target).
VC6: same size class (1316 against 1264 instructions), the differences are tail merges and register choices.

**D5508 (game/spells.cpp). spell_cast_execute (0x47c5d0): the EH frame and `where`.** The function opens an EH frame
(handler 0x57c598) whose state becomes 0 at the start of the single-player part and is never unwound visibly: a local
with an empty inline destructor. `where` (the hit sound position, [esp+0x10]) is the only candidate, written as a
local class spell_vec : vector3 with ~spell_vec() {} (as D3805's cam_vec), declared after the multiplayer branch (that
gives the original prologue order). Several cases (0x17 / 0x58 areas, the creatures 0x1c / 0x4f / 0x64, 0x36, 0x4c,
the spells without a case, the non-party side buff without cur_character) reach the hit sound without setting
`where`: the sound plays at whatever the stack holds (candidate bug K301; the same cases in
spell_cast_launch_effects play no sound unless an effect was created). The 0x69 / 0x6b party loop calls
get_hit_by_spell on living_from_handle's result without a NULL test (latent). VC6 builds a larger frame (0x190 against
0x12c: it does not overlap the case-scoped vectors and matrices as the original does); likely.

**D5509 (game/spells.cpp). spell_cast_launch_effects (0x47d420).** Two switches: the first creates the effects (and
records spell_effect_add for the effect objects, through the macro SPELL_FX_RECORD: obj_from_handle before
spell_effect_add, as the binary calls them), the second applies the hits that need no effect object and the summons
(0x26-0x31 play the hit sound unconditionally). The weapon / armour triggered missile computes a copy of the target's
orientation that nothing reads (the hit passes &t->orient, 0x47d870); the creature effects of 0x4f and 0x64 are
spell 0x1c's (push 0x1c at 0x47da55); 0x0c / 0x65 / 0x67 read the target's position without a NULL test after creating
the effect (latent). In the binary the second switch's dispatch and its first case sit between the first switch's
missile case and its remaining cases; VC6 lays the source order out sequentially, so the layout differs: likely.

**D5510 (combat.cpp, spells.cpp, levelrender.cpp, and the initialiser comments of 30 units). Markers one per line.**
The run lists (and tools/vc6-compare.mjs) read a `// 0xADDR` at the start of a comment line; several units described
an initialiser pair on one line ("// 0x404590 init_..._thunk / 0x4045a0 init_..."), which marked only the first
address, and where such a line opened the comment block of the following function, vc6-compare took that address
for the function (new_automap.cpp's cmd_map_grid, cmd_show_coordinates, cmd_reset_map_labels; new_buysell.cpp,
new_inventory.cpp, new_skill.cpp, new_spell.cpp's goto / page functions): they are now one marker per line, with a
blank line before the function's own marker, and those 21 functions are compared (all match). Conversely, comments
that start with the address of a function that is NOT written ("// 0x4670a0 combat_trigger_special_ability: not
written yet.") hid it from the run lists; in the files touched here they now read "// name (0xADDR): not written
yet." (0x4670a0, 0x468360, 0x468760, 0x47e900, 0x48e6c0, 0x48fdb0, 0x491500). Other units still have such lines
(unnamed_net_0x517590.cpp, unnamed_os_0x530b90.cpp: "module -, not written here").

**D5700 (game/combat.cpp). combat_trigger_special_ability (0x4670a0) and update_boss_attack (0x468360).** The
attacker's char_info special ability (+0x3c, chance +0x40, source type 1) and then its weapon's (equipment[5]) +0x94
triggered effect (source type 3, the weapon type as the item) are cast with D5506's COMBAT_TRIGGER_CAST macro: a
beneficial spell at the attacker (position + 1.2 up), any other at the defender. Unlike the reflex function every
branch targets the matching side, so K300 has no twin here. VC6: same size (1479 bytes), the prologue register order
differs; likely. update_boss_attack is written from its jump table (0 idle, 1 attacking, 2-3 clear, 6 the boss
special cast, the rest wait for trigger 6) and matches byte for byte.

**D5701 (game/combat.cpp). blood_splot_place (0x468760).** The decal's v half-axis is written as `0 - u.z, 0,
u.x - 0` (the original's fsubr / fsub with the 0.0 constant: a cross product with (0, 1, 0) whose products by 0 and 1
VC6 folded); u.cross() adds a temporary and a copy. The three size cases expand the vector operators less in each
later case (the large case calls vector3::operator= and the free operator*(float, vector3) out of line); the
operator form here gives 869 bytes against 810, the explicit vector3(x * s, ...) form 659: likely. The grey level
goes to decal_place's int colour argument bit for bit (`*(int *)&grey`); that argument is probably a float
(lights_shade_vertices' last argument), not changed here.

**D5702 (game/spells.cpp, engine/spells/spells.h). spell_spawn_creature (0x47bd30) creates nothing.** It sorts the
visible living entities within the spell's +Distance: into two local Character list heads (friends: team 0 with a
team-0 caster, or the caster itself; others), picks four strike targets into out[4] (7/8 an other while any is listed
or already picked, else a friend; a new one unless rand() % (2 x remaining) is 0) and moves every list back to the end
of living_list. The sum.symbols name is kept (header comment corrected). Four local Character objects (the 0x788-byte
constructor / destructor for each, the EH frame); 2016 bytes as the original; likely.

**D5703 (engine/spells/spells.h, game/spells.cpp). summon_create (0x47e900): SIGNATURE change.** Was
`Character *summon_create(Character *, vector3 *, int a, int spell, int b)`; now `void summon_create(Character
*caster, vector3 *pos, int layer, int spell, int existing)`: no path sets EAX, no caller reads it, the third argument
is stored to +0x68c and passed as living_entity_create's layer, and the fifth is a living-entity handle to reuse (-1:
create). The ring xp chain is an if / else-if chain in the binary's order (0xc0 / 0xcd share a body with ||, the other
pairs are separate tests with repeated bodies); VC6 tail-merges the repeated get_int calls, so 2195 against 2415
bytes; likely.

**D6000 (vsdk/parse/parse.h, vsdk/parse.cpp). bool_to_cstr (0x517000): SIGNATURE change.** Was `void
bool_to_cstr(char *dest, bool b)`; now `void bool_to_cstr(char *dest, int size, bool b)`: the flag is read from
the third argument slot ([esp + 0xc] before the two pushes) and dest from the first, as in its neighbours
vector3_to_cstr / matrix3_to_cstr (0x517060 / 0x5170d0, size unused). No caller in Sum.exe or in the tree.
VC6: matches.

**D6001 (vsdk/gr/glide/gr_glide.h, vsdk/gr_glide.cpp). glide_pending_activations (0x2f132ac) and
glide_pending_deactivations (0x2f132b0) are `volatile int`.** Every access in the binary is a load into a
register followed by a compare or an `inc reg` / store (`mov eax, [x]; cmp eax, -1`, `mov ecx, [x]; inc ecx;
mov [x], ecx`, the double read in gr_glide_flip), never a memory operand, while the neighbouring bool and int
globals are compared and incremented in memory (`cmp byte ptr [glide_close_on_deactivate], 1`, `inc dword ptr
[glide_swap_enabled]`). The window procedure (glide_msg_handler) writes them. VC6: +7 functions in gr_glide.cpp
(glide_set_active, gr_glide_set_mode, the clear / zbuffer_clear / deactivate / set_zbias / set_fog wrappers);
the other user, unnamed_ca_mesh_0x50af80.cpp, keeps its 8 matches.

**D6002 (vsdk/gr/unnamed_lights_0x51e070.h). light::dynamic is `short dynamic : 1` (signed).** Every test of the
bit is `mov ax, [l + 8]; and eax, 0xffff8000; test ax, ax`, the code VC6 gives a signed one-bit field (an
unsigned one is `test byte ptr [l + 9], 0x80`); `enabled` (bit 14) stays unsigned (light_is_enabled matches only
that way). Only lights code touches the field, and only for truth tests or bool stores. VC6: +6 functions
(light_create_point, light_set_pos, light_set_radius, light_set_color, light_set_spot_dir / _angles);
light_is_dynamic is written `? true : false`.

**D6003 (vsdk/unnamed_snd_0x5204c0.cpp). The backend tests are one-case switches.** `mov eax, [snd_backend];
dec eax; jne` (not `cmp dword ptr [snd_backend], 1`) is VC6's code for `switch (snd_backend) { case 1: ... }`;
the functions that return the backend's result keep it in a variable initialised to the failure value
(`or eax, -1` / `xor al, al` before the `dec`). snd_get_frequency alone returns from inside the case. VC6: +17
functions.

**D6004 (vsdk/unnamed_snd_0x5204c0.cpp). snd_load's path buffer is 0x200 bytes and get_packfile_path gets 0x80.**
The frame is 0x418 bytes (File 0x114, filename 0x100, path 0x200, offset) and the size pushed for
File::get_packfile_path is the constant 0x80, not sizeof(path). The stripped name is two strcpy calls in an
if / else (the copy code is shared, the destination address is set in both arms). VC6: the function now
differs only at its last bitfield store (`or [mem], imm` where the original loads, ors and stores).

**D6005 (all ten units of this pass). Source shapes that make cl 12.00.8168 emit build 8966's code.** Recorded
so that later passes use the same forms (details and counter-examples in the pass notes):
- bool results: `if (f(..) == 0) return true; return false;` (also for `||` chains and for a returned
  comparison after a call) gives the original `test / setcc al` or `mov al, 1 / xor al, al`; `return a != b`
  gives `neg / sbb / neg` or `xor / setcc / mov al`. A plain member comparison (`return slot >= 0`) is already
  right as an expression.
- 8966 keeps globals and fields in registers across stores through other pointers (and loop bounds across
  calls); where the original does so, a local copy written in the source gives the same code: `n =
  light_count` before a loop that calls out, `d = lights_selection_depth - 1`, a local alias of
  gl_tcache_entries for an init loop, `next = free_head` read after evict_block, the free-list unlink in
  bm_alloc_entry through `prev` / `next` locals.
- Counted loops over a global array whose index is passed on (snd_level_page_in, gr_opengl_tcache_frame) are
  `for (i = 0; i < N; i++, p++)` with a walking pointer: the original compares the counter, not the pointer.
- An argument that is itself a call (gr_get_aspect() for gluPerspective) is evaluated first by 8966: a local
  before the call reproduces it.
- `x ? 1 : 0` passed on as an argument was an if / else of two calls (gr_glide_flip); `n > 128 ? 256 : 128`
  is `n <= 128 ? 128 : 256` (the setg / dec / and al, 0x80 / add 0x100 sequence).
- Not reproducible with 8168 (left as they were): return-value optimisation of `return T(args)` (8168 builds a
  temporary and calls the copy constructor), strcat of a one-character literal as a word move, `and dh, imm`
  partial-register bitfield stores, the `or eax, eax` return of an already -1 EAX, the rotate idiom as `rol`,
  and most register-allocation and scheduling differences.

**D6100 (game/living_entity.cpp, game/combat.cpp, vc6-flags.txt). The optimised tail of living_entity.cpp is the
same object; the bloodspots and death commands are combat.cpp's.** The survey proposed a separate unit for
0x461dc0-0x463480. The data says otherwise. (1) VC6 puts all of an object's initialised globals before its
literals, whatever their position in the source (tested with VC6's compiler and linker: a global defined after two
functions, and after a `#pragma optimize("", off)`, still lands before their literals in the linked image). The
tail's own tables character_speech_times (0x599698) and hero_model_prefix_table (0x5996a4) lie among the unit's
other initialised globals (0x599644-0x5996b3), before the literals of the unoptimised part, and the tail's literals
(0x599eb0-) follow the unoptimised part's (up to 0x599eac) directly. (2) An object's .bss is one contiguous block:
the tail's console command objects (ccmd_chance_gold 0x8eca30, ccmd_Spell_ap 0x8eca40, ccmd_kill_summon 0x8eca50,
ccmd_no_resist 0x940428), whose initialisers are tail code, lie before and between the living lists, the pool and
spell_cooldowns, whose initialisers are unoptimised code. So 0x44d9f0-0x46335f is one object compiled with /O2 that
turns optimisation off at the top and on again at character_say, as the cutscene file does (D3201): the
unoptimised part keeps /Oi (inline strcpy / strlen: two more matches) and the /O2 string pooling (its literals fold
with other objects', e.g. "fire_ring_xp" 0x593e54), and its static initialisers are frame-pointer wrappers while the
tail's are tail jumps, exactly where the pragmas sit. (3) Debug_death = 1 (.data 0x59a16c) sits after the Spell_ap
literals and before the bloodspots literals, so another object's .data starts there; its .bss (ccmd_death
0x941710, ccmd_bloodspots 0x941730, Bloodspots 0x94173c) interleaves with combat.cpp's ccmd_Super_power 0x941720
(initialised at 0x464020, combat code) and Super_power 0x94173d, and living_entity's .bss ends with
move_play_serial 0x941708. So 0x463360-0x46353f (init_ccmd_bloodspots_thunk .. cmd_death, sum.symbols
"living_entity.cpp") opens combat.cpp's object: moved there with Debug_death and Bloodspots (the declarations stay
in living_entity.h). No new unit. Flags: living_entity.cpp /O2 /GX /MT (was /Od /Ob1 /GX /MT: 49 -> 67 of 132
before any other source change), the cutscene file /O2 /GX /MT (the default; /Od /Ob1 ties).

**D6101 (game/living_entity.cpp, game/objects.cpp, game/unnamed_cutscene_0x434900.cpp). VC6 12.00.8168 orders /Od
locals by a hash of their names; locals renamed to give the binary's frames.** Observed with the compiler on probe
functions: the frame slots of one scope go from ebp-4 downwards in bucket order, bucket = (h ^ (h >> 16)) & 15 with
h = h * 4 + (h >> 4) + c over the name's characters (32-bit; it predicts every probe, 80+ names), and within a
bucket the most recently declared local comes first. Block-scope locals follow the function scope's (each block in
source order), `this` comes last, the EH record sits at the top; /Z7, /Zi, /Ob, /GX, /Gf, /Oi do not change it (/Og
does: it orders by first use). The rule in the file headers ("the first declared local gets the highest address")
held only where the names happened to hash in that order. The locals of 38 functions (25 / 4 / 9) are renamed (and tied ones
re-declared newest-first) so that 8168 gives the binary's layout; the names are ours anyway (no local name is in the
binary). **Inferred, open:** the original compiler (8966) may lay locals out differently:
refresh_item_status_effects' 11 locals are in first-use order in the binary, which a hash order gives by chance with a probability of
about 1e-6, while save_state initialises `ofs` before the status table that lies above it, which neither pure
declaration order nor first-use order gives. The renames are kept because they make the available compiler
reproduce the bytes.

**D6102 (include/vsdk/os/profile.h). HEADER CHANGE: profile_label's constructor and destructor are inline.** Only
the /Ob0 units (character_instance.cpp, the ix unit 0x53ac20) call 0x51b6b0 / 0x434650 directly; every /Ob1 unit,
optimised or not, keeps only the EH states (objects.cpp's character_process_anim), and the unwind funclets call the
destructor's COMDAT. Defined empty in the class. All 13 users rechecked with VC6: 0 lost, 9 won (ai.cpp 0x472a90,
pathfinding.cpp 0x4a01d0, unnamed_ai_move 0x4b4590, unnamed_ai_think 0x4b6940 / 0x4b6a30, unnamed_pfgeom 0x4a5d40,
unnamed_shadows 0x4a44b0, character_instance 0x51a6f0 / 0x51a780); mingw: 253 files, 0 errors.

**D6103 (game/living_entity.cpp). place_at_party_start reads the start back through start_navpoint.** After
`start_navpoint = level_player_starts[slot]` the position, heading and layer come from this+0x34 (cast to
navpoint *, the member is a void *), not from the table again. Same behaviour; now exact.

**D6104 (game/living_entity.cpp). BEHAVIOUR FIX: play_move refuses a new move once the current move's trigger 9
time is (nearly) reached, not while it is pending.** The binary's test is `fcomp 0.0001; test ah, 0x41; je go_on`:
it returns -1 when move_trigger_time_left(9) <= 0.0001. The source had `> 0.0001f`, the inverse. Now exact.

**D6105 (include/engine/objects/living_entity.h). HEADER CHANGE: three bool parameters.** Character::init's fifth
argument (not read) is a bool (living_entity_create pushes it with `mov cl, byte ptr`); move_trigger_fired's
`latch` is stored as a byte (`mov byte ptr [ebp+0xc], 1`); set_armour_alt_texture's `on` / `force` are read as
bytes. Every caller passes a constant or a bool expression; the callers' units were rechecked with VC6 (combat, the
five AI / boss units, level_scripts_common, script_internal, the cutscene file): 0 lost.

**D6106 (game/living_entity.cpp, game/unnamed_cutscene_0x434900.cpp). Source shapes read from the code order.**
status_effect_add: the switch on the status kind has case 2 first. LE_PUT_STATUSES: the record is written inside
`case 0: case 2:` (a kind that matches neither skips to the loop end), not after a `default: continue`.
save_state: `ofs = 0` is declared (and initialised) before the status order table, renamed sort_order.
can_cast_spell: `spells[spell].cost > ap` (the table address is computed first). cmd_kill_summon: `for (p =
head.next, next = p->next; p != &head; p = next, next = p->next)` (the next link is read before each test).
set_floating_text: a signed length test (`jle`). get_display_name: the empty-name case is the else branch (its code
comes last). hero_model_prefix: a chain of `!=` tests instead of a switch. ai_suspended: one `||` condition with one
return. skill_equip_mod: the multiplayer skill remap is a switch with pick lock first. cutscene::load_files:
`num_sequences = num_sequences + num` (an int add stored back as a short; `+=` gives a 16-bit add).
objects.cpp character_process_anim: `(cur_character->pos - pos).len_sq()` read in place (z, y, x), no named copy.

**D6107 (notes only). What still differs in the unoptimised code is mostly the compiler build.** With the names and
shapes above, the remaining /Od differences reproduce in small tests with every flag set tried and look like
12.00.8168 against the original 8966: (1) `x = c ? a : b` writes both arms straight into x in Sum.exe (no
temporary: ring_add_xp, rings_add_xp_all, scale_stats_to_level, add_ap, ...), 8168 always uses a temporary; (2) an
inline function returning a vector3 by value is used in place in Sum.exe, 8168 copies the result once more (12
bytes per use: process_anim, walk_to_point, distance_between_edges, is_in_view, ...); (3) `b = (byte == 1)` is
`cmp ecx, 1; sete` in Sum.exe, `sub / neg / sbb / inc` in 8168 (save_state's script flag); (4) in the optimised tail,
switch lowering (apply_hero_model's 4-entry jump table), cross-jumping (set_armour_alt_texture) and byte narrowing
(is_mount / is_rider) differ the same way. None is worked around in the source.

**D5900 (summoner/interface/interface.h, game/unnamed_interface_0x411270.cpp). party_cycle returns int, not bool
(amends D2800).** Both early exits of the multiplayer branch set the whole register (`mov eax, 1` at 0x4133bf,
`xor eax, eax` at 0x4133ca); VC6 compiles `return true` in a bool function as `mov al, 1`. The single-player exit
(`xor eax, eax / setne al`) is the same code for both types. Every caller (new_hud.cpp, new_multi.cpp,
new_spell.cpp) ignores the result, so nothing else changes; with the int type and the multiplayer exit written
`if (p != party_leader) { party_select_member(p); return 1; } return 0;` the function matches byte for byte.

**D5901 (engine/characterinfo/characterinfo.h, game/characterinfo.cpp). charinfo_select_mempool's second
parameter is int, not bool.** 0x448f33 reads it as `mov eax, [esp+8] / test eax, eax`; a bool parameter is read as
`mov al, [esp+8] / test al, al`. Its only caller (charinfo_create_character_instance) passes its own `summon`.

**D5902 (engine/characterinfo/characterinfo.h, game/characterinfo.cpp). level_load_preload and
characterinfo_load_party_anims take `int summon`.** level_load_preload tests the argument as a dword (0x447b20
`mov eax, [esp+0x54] / test eax, eax`), and characterinfo_load_party_anims forwards its own argument to it
unchanged (0x44746d `mov eax, [esp+0x28] / push eax`); with a bool parameter there VC6 inserts `and eax, 0xff`.
The other callers (living_entity.cpp, unnamed_party_inventory_0x46b460.cpp) pass a constant or their own argument and still
compile; their VC6 match counts are unchanged (49 / 134 and 19 / 30, checked with the old and the new headers).
level_load_preload also picks its pool into a separate local (the original stores the chosen pool, even the
argument itself, into the argument's slot: `if (!pool) {...} else mp = pool;`).

**D5903 (game/unnamed_goals_doors_0x475180.cpp). mp_player_find_by_addr: logic error fixed.** The source returned
the first player whose address DIFFERS from the argument: nw_addr_equal returns 0 for equal addresses (as its other
callers test, `== 0`), and 0x4751a9 branches to the `return p` block on `je` (eax == 0). Now `== 0`; the function
matches byte for byte.

**D5904 (VC6 source shapes found while matching, match1 pilot).** What made VC6 12.00.8168 emit the original code
in these ten units (each verified by a byte match): a bool function whose binary sets AL (`mov al, 1` / `xor al, al`)
is written `if (x) return true; return false;`, one that sets EAX (`mov eax, 1` / `xor eax, eax`) is written
`return a || b;`; VC6 lays the then-block of an if out as the fall-through, so the binary's branch polarity gives
the source's (`if (x != 3) A; else B;`, `if (ok) { ...; return true; } return false;`); a duplicated loop test
(`while ((i = f()) != -1)`) becomes one call with `while (1) { i = f(); if (i == -1) return; ... }` (`for (;;)`
keeps the duplicate); a struct element accessed through a local pointer (`T *p = &arr[i];`) gives the original's
biased pointer and register choice; a case body is never shared with an identical one (case 0 and default each
have their own body); struct copies (`pt->p0 = *a;`, `out->points[i] = ob->points[i];`) instead of member copies;
two separate float assignments instead of `a = b = c` (integer moves instead of the FPU); a float local splits a
product of constants that VC6 would otherwise fold (`t = x * 0.001f; t * 204.0f`), and float locals for converted
integers (`float fx = x;`) give `fild / fmulp` instead of `fimul`; `vector3 d = vector3(...)` (temporary plus copy)
instead of `vector3 d(...)`; fabsf instead of fabs on floats (float constant in the compare); `int n = f(); g(n);`
does NOT change the stack-pop merging. Declaration order of locals moves their registers (hud_draw_health_bars);
an object with a destructor declared after an early return moves the EH state store (lamp_glows_draw).

**D5905 (match1 pilot). Differences that look like the compiler build, not the source** (8168 here, 8966 in the
Rich header; none of these could be changed by any source form tried): a strength-reduced loop's pointer compare
uses `jb` where the original has `jl` (and the reverse in other loops); `rep stosd` setup order (`mov ecx` before
`xor eax`); sums of products are reordered (no permutation of the terms changes 8168's order, the original keeps
the source order); `max(x, 0)`'s `setle / dec / and` ANDs into the other register; a bool local sits in the top
byte of its slot (`[esp+3]`) where the original uses the bottom (`[esp]`); stack pops after a call are merged
across the next call; a named Color initialised from a function result is copied instead of being constructed
in place.

**D5800 (game/levelrender.cpp). The unit compiles with VC6: d3d_min.h instead of <d3d.h>.** levelrender.cpp included
the DirectX 7 SDK's <d3d.h>, which VC6's own INCLUDE does not have (D3DVERTEXBUFFERDESC, IDirect3D7 undeclared),
so the unit never compiled with VC6. It now includes vsdk/gr/directx/gr_direct3d.h (d3d_min.h) and the Glide and
OpenGL back-end headers (as unnamed_ca_mesh_0x50af80.cpp does), and the two vertex-buffer calls use the C macros
(IDirect3D7_CreateVertexBuffer on D3D_DIRECT3D, lpVtbl->Release): levelrender_create_vertex_buffer and
levelrender_free_d3d_vertex_buffer match byte for byte, as do the three console commands. mingw still compiles it.

**D5801 (engine/s3d/s3d.h, game/levelrender.cpp). The geometry's render groups and the duplicate-vertex offsets.**
The draw lists read geometry +0x4c / +0x50 as an array of {short num_faces, short num_vertices} with its count
(`movsx ecx, word ptr [eax + 2]` per group, faces and vertices stored group after group, the group drawn with the
texture of its first face): s3d_render_group, s3d_geometry::groups / num_groups. +0x18 (one int16 per vertex) is a
backward offset: >= 0 the vertex repeats the one that many entries before (its clip codes, view position, colours
and projection are copied), < 0 a vertex of its own: s3d_geometry::vertex_dup. Both are unions with the old _unk
names (s3d.cpp keeps them).

**D5802 (game/levelrender.cpp). The three draw lists (0x48e6c0, 0x48fdb0, 0x491500) and their .bss.** Written from the
whole disassembly; the call sequences match the original one to one in all three (29, 59 and 59 calls), and VC6
gives 1277 / 1342, 1627 / 1704 and 1480 / 1553 instructions: likely. The per-vertex bytes sum.symbols calls
levelrender_*_vert_lit hold gr_code_point's clip codes (the names are kept); 0x2400ec8 (view-space vertices, passed
to levelrender_face_vertex_colors by all three) is levelrender_view_verts, 0x2433424 / 0x23f81e4 the Direct3D and
OpenGL index lists, 0x2395cf0 the OpenGL clip codes (names ours). Array sizes: 4000 for the per-vertex vector3 and
byte arrays (the 0xbb80 spacing), 4512 (0x11a0) for the Glide polygon vertices and the OpenGL client arrays, 9000
for the index lists. levelrender_d3d_vertex_buffer_pos (0x240d9e8) and levelrender_lights_dirty / _initialized
(0x243e038 / 9), declared by earlier batches, are defined here. Inline pieces as macros (clip codes, projection,
fog alpha, far fade) and one inline function, levelrender_depth (gr_calc_depth with the products in the other
order, which several sites use).

**D5803 (game/levelrender.cpp). The six function-local statics are the per-function projected and colour arrays.** A
VC6 test shows that `static vector3 a[4000];` in a function gets a guard bit and an empty atexit handler, as the
three guards (0x23eb68c, 0x23d3600, 0x23c5b38) and their six handlers show; each draw list has exactly two
per-function vector3 arrays besides the shared ones (Glide 0x23c7a80 / 0x23d3608, Direct3D 0x23dfb08 / 0x235b1f0,
OpenGL 0x240d9f0 / 0x24268f8), so those are the statics. levelrender_view_verts and levelrender_gl_vertex_array are
globals, so with the two wave arrays four of the six empty initialisers 0x48db70-0x48dc10 are accounted for; two
remain open. Unsure (which array has which guard bit).

**D5804 (game/levelrender.cpp). Draw-list defects.** (1) OpenGL, lightmapped face without a clipped corner
(0x491f9e-0x491fe2): with point lights the three colour bytes all take the red component (`fld [ebx +
0x24268f8]` three times), so coloured dynamic lights on lightmapped level geometry are grey in the OpenGL renderer
(SummonerFix's); live candidate. (2) Direct3D, lightmapped: when the next face would pass 8000 vertices the batch
restarts with a discarding lock but levelrender_d3d_vertex_buffer_pos stays (harmless: the writes stay below the
0x2140 capacity). (3) OpenGL, lightmapped: the restart tests count + 3 instead of count + the face's vertices
(harmless: the client arrays hold 4512). (4) Glide and OpenGL plain paths: the vertices of clipped polygons are
appended after the mesh's vertices for the whole object without a bound (latent overrun of the 4512 entries for a
mesh near 4000 vertices). (5) The plain Direct3D / OpenGL paths index levelrender_fan_tables[n] without the n <= 8
test of the lightmapped ones (latent: a triangle clipped by five planes has at most 8 corners).

**D5805 (game/unnamed_worldmap_automap_0x42f1e0.cpp, summoner/unnamed_worldmap_automap_0x42f1e0.h). The unit
0x42f1e0-0x430baf.** One unit by its contiguous .rdata 0x580d98-0x580df3, .data 0x592c8c-0x5930b7 and .bss
0x5f9670-0x5fa273 (decomp75's analysis); the name is a placeholder. 35 functions: 31 match byte for byte, four are
likely (worldmap_frame, worldmap_marker_clamp_to_edge, automap_labels_parse_table, automap_labels_save:
register and frame-slot choices). Findings: the PXO login name and password are the two fields of one
utrack_login_request (pxo_login_info; the sum.symbols names are macros); worldmap_location is 0x2c bytes (flags,
navpoint, scale, angle, marker x / y, label x / y / w / h, name) and an off-screen location gets flags 2 and 4
together (`or al, 6`); automap_clear_all writes 0 into every cell (it hides the grid at GameInit); the automap
labels' default set matches shopkeepers by name pointer; automap_reveal_cell's value is a bool passed on to
savegame::set_bit (a dword push of the byte, as set_bit tests it); the twin levels are 7 / 49, 30 / 35 and
31 / 36. VC6 folds `f * 0.5f + 0.5f` into (f + 1) * 0.5, so worldmap_marker_scale_for_distance halves in its own
statement. Latent: automap_label_alloc's 0 when full overwrites label 0; automap_labels_save's first loop does
not test ReadLine.

**D5806 (vsdk/unnamed_net_0x517590.cpp, vsdk/unnamed_os_0x530b90.cpp, vsdk/net/net.h, vsdk/os/os.h). The last
functions of the net and os objects.** float_compare (exact), nw_NewReceiveReliable (exact), nw_NewSendReliable and
nw_NewCloseSocket (likely: register choices, same sizes), os_poll and os_pump_messages (exact; the pump returns 0:
`xor eax, eax` on the empty queue, PeekMessage's 0 after the loop). RNT_DISCONNECT (4) is the close packet's type.
nw_NewCloseSocket's send-buffer branch clears rsequence[i] instead of ssequence[i] (harmless: the record is cleared
afterwards when the socket exists). The header comments no longer start with addresses (D5510).

**D5807 (game/unnamed_gamelist_0x4705b0.cpp, summoner/multi/unnamed_gamelist_0x4705b0.h, multi.h). The client's game
list and join requests, 0x4705b0-0x470e2f.** Its own unit by the initialiser run 0x4705b0 and the .bss
0x22e4428-0x22e462f (source-map.md); placeholder name. 14 functions: 10 exact, 4 likely. SIGNATURE:
mp_send_jn_rqst's last parameter is `bool retry` (was `int flag`): `mov al, [esp + 0x26c] / test al, al`; the only
callers pass 1 and 0. The kept character is an mp_char_summary (0x12c bytes, `rep movsd` 0x4b) at 0x22e4430, the
password a 16-byte copy (strncpy 15); the request's +0x40 field is the int at 0x68e1bc, which no written unit
defines (declared as mp_join_unk_68e1bc). mp_handle_actv_gm reads the six ints and two bytes through memcpy (VC6
then keeps them in memory, as the original does).

**D5808 (game/unnamed_kick_0x471df0.cpp). mp_remove_player, mp_send_removed_notice, mp_handle_kick_pl as their own
unit.** D2604 left open whether 0x471df0-0x471fe5 ends multitrade.cpp or is a file of its own; no data decides it.
Written as a placeholder unit (moving the code into multitrade.cpp later only means appending). 2 exact, 1 likely
(mp_remove_player: the original pops the first call's arguments at once). MP_KICK_PL (0x3b).

**D5809 (game/unnamed_swi_0x488230.cpp, multi.h). The SWI packets 0x488230-0x488bf9.** Between multichat.cpp and
multicontainer.cpp; no .data or .bss of its own beyond two printf literals; placeholder unit. 12 of 14 match byte
for byte. mp_player +0x6c is a vector3 (the position kept at SWI2DIR / OBJ_UPD), +0x78 sixteen sampled update
intervals and +0xb8 their count (mp_remote_travel_update, D5810; were _unk6c[]). MP_ADD_FLOAT's initialised temporary
makes VC6 load a member through the FPU (fld / fstp), while the original copies with mov: the senders that read
members use a plain `float f;` assigned before each MP_ADD_DATA (mp_send_swi_ndir and the object packets).

**D5810 (game/unnamed_mp_objects_0x47f860.cpp, summoner/multi/unnamed_mp_objects_0x47f860.h, multi.h). The object
packets 0x47f860-0x481196.** Placeholder unit; 31 functions: 17 exact, 12 likely, 2 unsure (mp_handle_new_path,
mp_handle_obj_update). It may be the head of multiendgame.cpp: the code runs straight into multiendgame.cpp's
static initialiser (0x4811a0), mp_handle_new_path reads multiendgame's multi_path_type (0x59bca8), and
mp_net_handle_map (0x22e94f4, defined here; multi.h had left its owner open) lies 0xc bytes after
ccmd_Multi_path_type (0x22e94e8). Not merged: multiendgame.cpp's header and D2604 say its block starts with the
initialiser, and VC6's .bss order is not strictly the definition order (the worldmap unit's own .bss shows that).
Defects: mp_send_local_updates sends the leader's OBJ_UPD again where it means the locally controlled summon's
(`push esi` at 0x47fa55; live candidate in multiplayer: a client's summon is never position-updated by this path);
mp_handle_new_lep uses the NULL result of a failed living_entity_create after mp_player_disconnected (latent);
mp_respawn_player resurrects a NULL character for an unknown id (latent).

**D5811 (game/new_pxo_banner.cpp; marker splits). open_url, mp_handle_unequip_item and the initialiser markers.**
open_url (0x421de0) is new_pxo_banner.cpp by strings.tbl id 834 and sum.symbols, a unit of one function plus the
two unreferenced rectangles at 0x58f6f8 that precede its literal "open" (unsure ownership); VC6 byte match (the
switch includes SE_ERR_DLLNOTFOUND, 32, which the < 32 test excludes). mp_handle_unequip_item (0x487b10) completes
unnamed_equip_packets_0x4878a0.cpp (byte match). The initialiser pairs of new_pxochat.cpp (0x4227c0-0x422970),
multitrade.cpp (0x470e30-0x470e60), multisave.cpp (0x46cc60 / 0x46cc70), unnamed_mp_join_0x4ab0f0.cpp
(0x4ab0f0 / 0x4ab100) and level_scripts.cpp's 0x4ceef0 are one marker per line (D5510).

**D5812 (open). A register choice VC6 does not reproduce.** In six small handlers (mp_handle_deny_jn,
mp_handle_swi_sneak, mp_handle_respawn, mp_handle_forcepr and two more) the original loads the packet's first int
with `mov eax, [eax]` (reusing the pointer's register) where VC6 emits `mov ecx, [eax]`, for every source form tried
(pointer casts, array index, locals, memcpy, other parameter types) and every /G and /O variant. A different
compiler build (service pack) is one explanation; the functions are marked likely.

**D6300 (vsdk/matrix.cpp, unnamed_vector_0x523f30.cpp, unnamed_ix_0x53ac20.cpp, unnamed_ca_mesh_0x50af80.cpp; match5
pass). Source shapes that make cl 12.00.8168 emit build 8966's code (/O2 /Ob0 vsdk units), each verified by a byte
match.** (1) A fabs compared with a float constant is `(float)fabs(x) < 0.0001f`: plain `fabs(x) < 0.0001f` compares
in double (`fcomp QWORD PTR`), fabsf is an out-of-line call under /Ob0; the cast keeps the intrinsic and the dword
constant (rows_match, vector3_near_equal, ix_sphere_polygon_plane, and `(float)fabs(a) > (float)fabs(b)` in
extract_angles, which also gives the binary's `fxch / fxch / fcompp`). (2) A swap of two values in memory is written
with the temporary taking the value that is stored LAST in the binary: `t = uvec.x; uvec.x = rvec.y; rvec.y = t`
(matrix::transpose_in_place, ix_sphere_polygon_plane's t0 / t1). (3) An early return whose code the binary places
after the body is the body in a then-block (`if ((float)fabs(v) > 1e-30f) { ... } return false;`). (4) A product of
constants that 8168 folds (`atan(..) * 2.0f * RAD_TO_DEG`) keeps the binary's `fadd st, st / fmul` through a float
local (`a = atan(..) * 2.0f; *h = a * RAD_TO_DEG;`, gr_get_view_fov, gr_fov_to_vertical); a converted int divisor is
a float local (`float w = (float)clip_width;` gives `fild / fdivp`, not `fidiv`). (5) A value stored to a global and
reused is a local (`d = fog * dist; gr_far_clip_dist = d; gr_depth_scale = 1.0f / d;`: `fst` then `fdiv st, st(1)`).
(6) bool functions whose binary sets EAX (`mov eax, 1` / `xor eax, eax`) after a float compare are a returned
expression (`return a->key < b->key;`); the if-form gives `mov al, 1`. A bool function that sets AL after `test /
setne` on an int global is the if-form (`if (eax_reverb_muted) return true; return false;`). (7) Locals: a global
loop bound read once (`int next = effects_stream_slot_b_next;` before both scans of effects_alloc_stream_slot /
effects_find_stream_slot); declaration order picks the registers (effects_hash_name: `sum, bits, i, len`); a call
argument that is a call goes to a local first (`tag = get_tag_name(i); _stricmp(tag, name)`), and so does a float
argument the binary stores to a stack slot (`t = ...; return float_max(0.0f, t);`). (8) `return value - rem + 0x40;`
inside the if (align_up_64) gives `sub / add`; the assignment form gives `mov edx, 64 / sub / add`. (9) A pointer local
to an array element (`b = &effect_bases[index]; if (b->resident || b->streamed)`) gives the binary's dead `lea`.

**D6301 (vsdk/gr_direct3d.cpp). gr_d3d_set_default_transforms: element stores and the projection constant.** The
binary stores world / view _11, _22, _33, _44 in that order (one assignment each; the chained `a = b = c = 1.0f`
stores _44 first), computes `zfar / (zfar - 1.0f)` with the constant 1.0 (the source re-read view._44), and stores
proj _11, _22, _44 (= 0, though the memset cleared it), _34, _33, _43. Same behaviour; now exact.

**D6302 (vsdk/gr_direct3d.cpp). gr_d3d_begin_scene / gr_d3d_end_scene: the depth test as a range test.** The binary
tests depth < 0, depth > 1, then depth == 1 (begin) or == 0 (end) before changing it: `if (d >= 0 && d <= 1 && d !=
1)`. This is likely build 8966's lowering of a two-case switch (case 0 / case 1, one empty), which 8168 folds into one
compare; the if-chain reproduces the binary and does the same. Exact.

**D6303 (vsdk/unnamed_vfx_channel_0x55b440.cpp). vfx_channel::clear is a memset of the record.** Three dword stores
of 0 (+0, +4, +8) clear data, size, base_value, cursor and the frame / done / bytes word together;
`memset(this, 0, sizeof(*this))` (12 bytes, intrinsic) is the code, member stores give five word stores. Exact.

**D6304 (vsdk/unnamed_vfx_0x536950.cpp). render_node reads the node index unsigned; setup_render_state reads the
material flags once.** render_node zero-extends vfx_node_ref::index (`xor eax, eax / mov ax, [edi]`) for both the
mesh and the emitter tables, as unnamed_render_scene_0x481680.cpp already casts it ((ushort)); the header keeps
`short` (other writers store a short), the reads are cast. For an index >= 0x8000 the old source indexed
backwards; the binary does not (no such index exists in practice). The mesh case is the else branch (`if (kind !=
MESH) { if (kind == EMITTER) ... } else ...`, the binary's block order); the rest differs by scheduling.
setup_render_state loads mat->flags as one word into a local (`unsigned int flags = mat->flags;`) before the
bitmap store; still differs (the binary loads vi->time above the vfx_render_alpha store).

**D6305 (vsdk/unnamed_ix_0x53ac20.cpp). ix_linesegment_boundingbox: unsigned clip masks and computed-once
differences.** `test ebx, eax / ja` is an unsigned `(ca & cb) > 0` (a signed or `!= 0` test gives `jne`); the face
coordinate is an if / else of two integer copies (the ternary loads through the FPU); the segment difference and the
offset along the face axis are computed once per face (`d`, `s`), kept on the FPU stack as the binary does (the old
source re-read out->x after storing out->y, which differs only if out aliases a or b). Still differs: build 8966
moves the out->z store between fcomp and fnstsw.

**D6306 (notes only; match5 pass). Differences that are the compiler build.** In addition to D5905 / D6005 / D6107,
seen in these ten units and not reproducible with 8168: (1) `vector3 v = a - b;` and `return vector3(...)` /
`return matrix(...)`: 8966 constructs the result in place, 8168 builds a temporary and calls the copy constructor
(vfx_node::get_pos / get_orient, vfx_vec_channel::eval, vector3::normalized / rotated / unrotated / negated_b,
matrix::operator* and the multiply variants, ix_ray_sphere, ix_sphere_boundingbox, gr_get_normal_facing_dot: about
25 functions); direct initialisation `vector3 v(a - b)` gives the same as copy initialisation. (2) A copy of 9 or 3
dwords (matrix43 constructors, get_rotation) loads the last two before storing them in 8966; element stores and POD
struct copies (`movsd`) do not give it. (3) Integer stores and loads scheduled between `fcomp` and `fnstsw`, or above
a store through another pointer (get_axis_angle, effect_base_set_info, effect_base_finish_stream, get_tag_name, gr_d3d_init). (4) 8966 reuses dead argument slots for locals (seg2d_intersect, gr_setup_3d); 8168
adds a frame. (5) 8168 sorts the terms of a sum of products (matrix::determinant: a source with
the terms and factors reordered gives identical code). (6) 8966 tail-merges identical calls at the end of if / else
branches (matrix::orthogonalized: the two fvec.normalize() calls share one); 8168 keeps both. (7) A bool from a
one-bit field is narrowed with `and al, 1` in 8966 (effects_base_is_resident, effect_is_cutscene_base).

**D6200 (include/engine/ai/ai.h, game/unnamed_ai_combat_0x4bce60.cpp). HEADER CHANGE: ai_score_support_targets
takes `int heal, int cure`.** choose_support_spell keeps both can_cast_spell results as dwords (`and eax, 0xff /
mov edi, eax`, `mov ebx, eax`) and pushes the registers; the scorer reads them as dwords (`mov ebx, [esp+0x14]`,
`mov eax, [esp+0x24] / test eax, eax`). Its only caller is choose_support_spell (heal, cure and vitalize are int
locals there). VC6: ai_score_support_targets now matches; mingw: 259 files, 0 errors.

**D6201 (include/levelscripts/level_scripts.h, game/level_scripts_common.cpp). HEADER CHANGE:
level_trigger_point_left_of_line takes the trigger's s3d_named_path, not the trigger.** The caller (level_check_triggers,
0x4cabae) pushes the register that holds the path (`mov eax, [edi + 0xc]` reads the points just before), and the
callee reads `[arg + 0xc]` as the points array. Same behaviour (the source read t->path->points); the only caller is
in the same file. The function still differs (frame 0x28 against 0x20, D6206).

**D6202 (include/summoner/multi/multi.h, game/unnamed_swi_0x488230.cpp, game/unnamed_ai_think_0x4b5140.cpp).
HEADER CHANGE: mp_send_character_walk's second parameter is bool.** goal_do_move_to passes its bool `walk` with
`push ebx` and no `and ebx, 0xff` (an int parameter makes VC6 widen it), and the callee only reads the low byte
(`mov dl, [esp+0x20c]`). The definition in unnamed_swi_0x488230.cpp changes its signature only (12 / 14 matches
before and after); the other callers pass 0 or a comparison (unnamed_ai_move 4 / 19 before and after, ai_combat and
ai_think: no match lost). goal_do_move_to and goal_do_move_to_timed now match (with the speed-scale call written as
an if / else of two calls).

**D6203 (include/engine/combat/combat.h, game/combat.cpp, game/unnamed_ai_think_0x4b5140.cpp). HEADER CHANGE:
living_combat::has_weapon_skill returns bool.** Both callers (goal_do_attack 0x4b600e, pick_attack_target 0x4b592f)
test AL (`test al, al`); the callee's `mov eax, 1 / xor eax, eax` is what a bool `return x > 0.0f;` gives (D5904).
combat.cpp: `return weapon_skill() > 0.0f;` (was `? 1 : 0` in an int function), still a byte match (35 / 63 before
and after). goal_do_attack now matches.

**D6204 (game/unnamed_tiger_0x499e50.cpp). LOGIC FIX: tiger_anim_remap_out reads the character's own loco anim
table, not the mount's.** 0x49c1e0 keeps the argument in EBX (`mov ebx, [esp+8] / mov ecx, ebx / call get_mount`) and
copies `[ebx + 4*slot + 0x5ac]`: the mount is only tested for NULL. The source read mount->loco_anims. Now exact.
serpent_anim_remap_out (0x495db0, unnamed_serpent_0x495290.cpp, not part of this pass) has the same code (`mov edi,
[ebx + 4*edi + 0x5ac]` with EBX the argument) and the same source error (its comment "puts the mount's current anims
back" is wrong); left for its owner.

**D6205 (the ten units of this pass). Source shapes that make cl 12.00.8168 emit build 8966's code** (each verified
by a byte match; details in the pass notes):
- Call arguments computed into locals first. Where the original evaluates the complex arguments of a call before the
  simple ones (point_in_rect / ui_tooltip_set rectangles, automap_is_cell_revealed's cell indices, a float argument
  through a memory temporary), `int x = ...; int y = ...; f(..., x, y, ...)` reproduces it: status_show_skill_tooltip,
  automap_label_box_hover, automap_labels_hover, automap_map_point_revealed, skills_row_hit_text and _icon (all four
  rectangle values), follow_character (`float dt = ...` for predict_owner_position), script_party_acquire_item (the
  format string first).
- A value read once from a global struct and stored into several fields goes through a local (`int walk =
  tiger_anim_stage2_walk; c->a = walk; c->b = walk;`): the original does not re-read it after each store; a second
  local re-reads it once (tiger_set_stage2_anims). `anim = tiger.x; if (anim != -1) {...}` for the guarded groups
  (tiger_equip_weapon, tiger_setup_anims); `item_info *weapon = ...; tiger_weapon = weapon;` before the equip call.
- A float local that is assigned in several places stays in memory in the original (`fst [esp+8]` of its 0.0 start,
  `mov eax, [esp]` of 0.4): an assignment plus `break` and one return gives it (boss_turn_rate_override,
  tiger_cache_move_b8, rand_weighted_index's `result`).
- A loop search followed by `if (i == n || i == -1)` is the original's `if (r <= score) { if (i == -1) return -1;
  ...; return x; }` inside the loop, with `return -1` after it (choose_offensive_spell, choose_support_spell).
- `(float)a / (float)(b * 1000)` with ints: 8168 emits fidiv, the original loads both with fild and divides in
  float: `(float)((float)a / (double)(b * 1000))` (the six consider_* spell timer factors).
- Sums of products: 8168 emits the terms of `a*b + c*d + e*f` in reverse source order, so the natural `x, y, z` order
  gives the original's z-first code (follow_character); a vector subtraction instead of three component differences
  (script_object_near_player_dist, script_navpoint_near_player_dist), d.len() for the square root of the squares
  (goal_do_pickup).
- `v = vector3(a.x + b.x, ...)` instead of member assignments (combat_face_target), `target->pos +
  target->centre_offset` (operand order of the inline add: update_target_pos), `vector2 d(x - p->x, z - p->y)`.
- Element pointers again (D5904): `ui_button_state *b = &buttons[i]` in the button draw loops (status, automap,
  skills) and the skills category tooltip, `script_action *l = &so->actions[i]`, `boss_threat *p = &t[i]`, the
  tooltip's entry pointer (its dead lea is in the original), `vector3 *fwd = &t->orient.fvec` shared by two uses.
- Bool shapes: `if (x) return true; return false;` (within_attack_radius, character_is_not_boss,
  automap_mouse_to_map_cell); `return f() ? true : false;` for the `test al, al / setne al` of a bool call
  (script_player_has_equipped, skills_row_hit); `bool r = f() == true; return r;` for `cmp al, 1 / sete al`.
- int instead of bool where the binary keeps a flag in a full register (`and eax, 0xff / mov ebx, eax`):
  consider_blind_or_silence, choose_support_spell; their target record read again through `*out` after its store.
- Store and statement order: `y += font_h;` before `gr_set_color_ptr` (status), the flag store after the counter
  (tiger_move_done_hook), `layer` before `text[0] = 0` (automap_add_label_at_mouse), the natural parameter order of
  set_default_view_params, `height[1] = height[0] = h`, separate `r = 1.0f; g = 1.0f; b = 1.0f;`, the loop increments
  `i++, y += font_h` and `p = p->next, slot++`, `int level = ...` before the cap call (status_skill_increment), the
  progression read without a pointer (status_build_skill_list).
- Branch layout: nested ifs with one shared `return -1` at the end (consider_area_spell), `if (a->arg0c >= 0) {
  ...; return true; } return false;` (script_action_play_action), the else-part first where the binary falls into it
  (status_update_scrollbar,
  status_handle_skill_buttons, update_target_pos, skills_init), an if / else-if chain instead of a switch where the
  binary compares in source order (consider_death, choose_offensive_spell's species specials), `case 0` with its own
  body in a switch the original tests explicitly (automap_get_zoom), `while (1) { if (!goal_remove(...)) break; }`.
- Clamps with the bound first: `max((short)lo, c->level)` / `min((short)hi, c->level)` with short operands give the
  16-bit `cmp di, ax` of level_script_init; `fabsf` with float constants (D5904) in auto_rotate_update and
  smooth_height, `fabsf(step) > fabsf(d)` (the operand order of the original compare).
- Smaller ones: tiger_rider_set_flinch's message buffer is 0x80 bytes (frame 0x84), choose_offensive_spell's choice
  array 38 entries (frame 0x144), combat_ranged_attack reuses `range` for the edge distance, the operand order
  `cutscene_cur_sequence == a->arg0c`, `int press = b + 1` (automap_update_buttons), the masks `((b != 0) & 1)` of
  set_follow_flags, no `c` local in boss_turn_rate_override, no `info` local in level_page_in_characters, `int lines`
  declared first (automap_draw_label), the summon pointer for each party member (script_place_party_at_starts).

**D6206 (notes only). Differences of this pass that no source form changed** (8168 against 8966, as D5905 / D6107):
- Stack pops: 8966 never merges the pending `add esp` of a call across a varargs call (sprintf), 8168 does
  (status_draw_stats, status_draw_level, script_party_acquire_item: the rest of those functions matches).
- Jump tables whose cases repeat the default target (boss_attack_knocks_down_dispatch, boss_attack_stuns_dispatch,
  boss_stage_attack_enabled_dispatch: 8966 builds a byte table over 0x79 values, 8168 two compares); a ternary whose
  value stays on the FPU stack in the original (`zoom = max(zoom, 0.3f)` in apply_input_deltas); the
  store-then-overwrite of `x = c ? a : b` (status_page_down, status_drag_scrollbar).
- Copy elision of a by-value vector3 result into a named local (update_orientation, snap_to_target): 8168 copies.
- Spill slots and frames: the original reuses a dead argument slot for a temporary where 8168 picks another
  (goal_do_travel, tiger_anim_remap_in), frame sizes 4-8 bytes apart (level_trigger_point_left_of_line,
  combat_toggle_ready_stance), callee-saved registers pushed late (shrink-wrapped) for two registers
  (script_action_die, camera::auto_view_update; 8168 does it for one).
- Register choice and scheduling: ebx / ebp and esi / edi swaps (choose_support_spell, consider_life_drain,
  script_action_wander, group_assign_formation, combat_approach), 8168 sinking computations into the branch that
  uses them (automap_center_on_player), `mov eax, imm / push eax` for a constant argument (camera::cycle_mode),
  inline argument evaluation order of a vector3 constructor (boss_mount_pick_flee_point's result).

**D6207 (notes only). Markers that say exact but differ under 8168.** Marked exact by reading before the VC6 tool
existed, in this pass's units: camera::auto_view_update, camera::snap_to_target, status_load_bitmaps,
status_goto_pending_screen, status_page_up, status_page_down, level_trigger_point_left_of_line, level_ambient_find_at,
script_object_receive_go, script_party_acquire_item, script_action_die, script_action_wander,
automap_center_on_player, ai_state::boss_ai_unload, boss_dismount_dispatch, boss_threat_find,
skills_update_species_tab (and serpent_anim_remap_out outside the pass, D6204). Their remaining differences are of
the D6206 kinds; the words were left as they were.

**D6600 (vsdk/character_instance.cpp, unnamed_gadget_0x52bfe0.cpp, window.cpp, unnamed_string_0x5566e0.cpp,
unnamed_sndstream_0x566560.cpp, skeleton.cpp, console.cpp, registry.cpp, unnamed_checkbox_0x553ba0.cpp; match8 pass).
Source shapes that make cl 12.00.8168 emit build 8966's code** (each verified by a byte match; details in the pass
notes):
- Products of two float constants are not folded in the binary (`fmul c1 / fmul c2`): a float local for the first
  product (`float t = ticks * (1.0f / 160); return t * (1.0f / 30);`), a second one when a third factor follows
  (get_anim_elapsed's `* 30`); the int argument through an `int` local first (`int len = s->end - s->start;`) gives
  the original's register choice (get_anim_length, get_anim_time_left). Same for frametime * 30 * 160 (process).
  The folded and unfolded forms can round differently in the last bit: the new source computes what the binary does.
- Loops over character->num_anims: `ca_character *ch = character; int n = ch->num_anims;` and, where the binary loads
  the array pointer after the count test, `if (n > 0) { int *t = times; for (...) t[i] = ...; }` (reset_anims,
  clear_state_anims, clear_action_anims); a walking `float *w` with `w++, i++` (get_active_anims).
- Declaration order of initialised locals is the order of their stores (and of the register variable's xor) in the
  prologue: parse_container / parse_static / parse_inputbox / parse_listbox (type, parent_x, parent_y, ...);
  report_memory (num_anims, num_bones, time_size, pct_size); a hoisted `int miss = 0;` before the if in the four
  find_nearest_* (it is also the zero pushed for get_screen_x(0)).
- `while (1) { if (!tf->Accept(..)) break; ... }` for the binary's top-tested loop, with the loop's locals
  initialised before the first call of the body (parse_button's state bitmap loop).
- Stores through locals where the binary does not reload: `char *p = buf; p[0] = c; p[1] = 0;`
  (String::operator=(char), assign_char), `char *p = token_write_ptr; *p++ = c; token_write_ptr = p;` (console's
  read_word / read_quoted_string), `int min_size = textbuffer_min_size;` (TextBuffer::TextBuffer), a local result
  assigned in both branches (snd_stream_get_writable); closer but not matching: `ui_gadget *last =
  win->gadgets.window_prev;` (ui_gadget::init), `DWORD rate = src->nSamplesPerSec;` (acm_stream_open).
- A global or field read before an unrelated store (8966 schedules the load first): `int f = flags;` before
  `ui_window_processing = false` (ui_window::process), `bool pressed = ui_left_pressed;` before `state = 3`
  (ui_checkbox::process, ui_radio_button::process).
- Branch layout, the then-block is the fall-through: `if (buf != NULL) return strlen(buf); return 0;` (String::size),
  `if (i >= 0) buf[i + 1] = 0; else free();` (trim_right), `if (fits) { ... } else { overflow }`
  (TextBuffer::append), `if (strlen(next_arg) != 0) load_script(..); else { help; return; }` (cmd_script),
  `if (read(..) == 0) return 0;` (os_registry_read_dword / _int: the binary returns the zero EAX), `if (*cursor == 0)
  current_char = *cursor; else current_char = *cursor++;` (get_next_char), `switch (state) { case 1: ...; default:
  ... }` for `dec eax / je` (ui_state_bitmap::draw), switch cases in the binary's block order (get_token: QUOTE, END,
  LETTER / DIGIT).
- bool returns: `if (win->selected == this) return true; return false;` gives the `cmp / sete al` of is_selected /
  is_mouse_over; the two-store hotkey clear is a counted loop (clear_hotkeys).
- `if (callback) { bool handled = callback(e, v); if (handled) return handled; }` gives the binary's jump into the
  shared epilogue with the callback's AL (ui_gadget::send_event; the registers still differ).
- Signed compares: `(int)acmGetVersion() < 0x3320000` (acm_init, `jge`); `n <= 0` on an unsigned for `jbe`
  (snd_stream_service, with `n -= written;` before the flag clear for the register choice).
- `hkey = NULL; rval = -1;` as two statements after the declarations (os_registry_read, os_registry_delete).
- The dword rep movsd behind a count test is `if (n > 0) memcpy(d, s, n * 4)` (vim_restore_bitmaps matches;
  vim_copy_bitmaps / vim_save_bitmaps keep the form, 8966 loads the destination before the pushes).

**D6601 (include/vsdk/ui/ui.h, vsdk/unnamed_ui_element_0x542cd0.cpp). HEADER CHANGE: ui_element::hidden and
ui_element::disabled are bool, not ubyte.** is_hidden (0x542fe0) and is_disabled (0x543000) return the byte as it is
(`mov al, [ecx + 0x1c] / ret`); a ubyte field returned as bool is normalised (`test / setne`). Every store is a bool
or 0 / 1 (set_hidden / set_visible / set_disabled / set_enabled, init, ui_window's constructor). All 21 .cpp that
include ui.h or name ui_element were VC6-compared before and after (game/summoner, unnamed_editor_dialogs,
unnamed_frame, unnamed_mlayout, unnamed_script_editor, unnamed_states; vsdk skeleton, ui_container, build_stamp,
checkbox, floatmath, gadget, inputbox, listbox, movement, ui_button, ui_element, ui_manager, ui_slider, ui_static,
window): no match lost, ui_element 9 -> 11. mingw: 259 files, 0 errors.

**D6602 (notes only; compiler flags, vc6-flags.txt not changed). Units that would gain from other flags.** (1)
skeleton.cpp with `/Ox /Ob0 /GX /MT` (/O2 without /GF and /Gy): 17 -> 19 (vim_write_cache, vim_reload_bitmaps). The
binary reads the "" initialisers of its name buffers from separate .bss literals (0x2ee4578-0x2ee4590, one per
buffer), which only an unpooled compile reproduces; the other five units of the pass are equal with /Ox or lose
(sndstream 17 -> 16, acm_driver_enum_callback). (2) Not in vc6-flags.txt but better with /Ob0:
unnamed_anim_track_0x550660 1 -> 8, unnamed_d3d_tcache_0x54bb30 3 -> 7, unnamed_sound_emitter_0x546150 14 -> 17,
unnamed_vfx_mesh_0x562a30 3 -> 6, unnamed_poly_clip_0x5519c0 1 -> 3, unnamed_gr_line_0x53e520 2 -> 3. console.cpp
(with /Ob0) and registry.cpp gain one each with /Ox. Left for the owner of the flags file.

**D6603 (notes only; match8 pass). Differences of this pass that no source form changed** (8168 against 8966, as
D5905 / D6005 / D6107 / D6306): return-value and copy elision (character_instance::compute_bone_matrix,
get_bone_transform, get_bone_world_transform, get_root_velocity, sum_root_velocity, skeleton::get_root_velocity:
8966 hands the temporary's address straight to the copy constructor); dead argument or local slots reused for other
locals (skeleton_get_bone_blend_weight, increment_tick, snd_stream_fill_acm / _pcm / _silence, ui_checkbox::create,
parse_slider's frame 0x304); stores scheduled after the pushes of the next call (ui_gadget::set_focus / clear_focus,
ui_element::set_hidden / set_visible / set_disabled / set_enabled, wav_descend_data, wav_read's fail path) or into
an inline strcpy (the set_tooltip functions, skeleton::skeleton); loads hoisted above stores (start_anim,
alloc_buffers' loop registers, acm_stream_open, acm_convert's rep stosd setup, String's copy constructor); register
choices (set_parent's else branch, ui_gadget::send_event, ui_window::set_tooltip, handle_arrow_key's frame slots,
wav_open's mmioDescend in a register); a dead copy of String::buf in the frame (to_lower / to_upper); the
shrink-wrapped early ret of init_char_types; the byte `and al, -0x41` of ui_window::init; strcat of "\\" as a word
move (os_registry_set_app_key); ~skeleton's list_remove (vsdk.h macro) without the reload of next / prev;
ui_element::dtor's direct call to the scalar deleting destructor.

**D6400 (game/unnamed_serpent_0x495290.cpp). LOGIC FIX: serpent_anim_remap_out reads the character's own loco anim
table, not the mount's (as D6204 for the tiger).** 0x495db0 keeps the argument in EBX (`mov ebx, [esp+8] / mov ecx,
ebx / call get_mount`), tests the mount only for NULL, and copies `[ebx + 4*edi + 0x5ac]` (c->loco_anims[slot]). The
source read mount->loco_anims; its "exact" marker was by reading. Now a VC6 byte match; the remap_in comment no longer
says the mount's anims are put back. In the same unit serpent_rider_set_flinch's message buffer is 0x80 bytes (frame
0x84, as the tiger's; predicted by the match4 notes): now exact.

**D6401 (include/engine/ai/pathfinding.h, game/unnamed_pfgeom_0x4a4ee0.cpp). HEADER CHANGE: seg2d_point_near_middle
returns int.** Its only caller, pf_layer_from_edges, tests the result as a dword (`test eax, eax` after both calls; a
bool result is tested with `test al, al`), and the callee sets EAX (`mov eax, 1` / `xor eax, eax`).
pf_layer_from_edges now matches; mingw: 259 files, 0 errors. The callee still differs (8168 orders the factors of its
second dot product, D6306 (5)) and is marked likely.

**D6402 (game/unnamed_pfgeom_0x4a4ee0.cpp, game/unnamed_ai_move_0x4b1410.cpp). The local segments are 0x10-byte
pf_edge records.** Every function of the geometry unit with a seg2d local has a frame 8 bytes larger per segment than
an 8-byte seg2d gives (pf_trace_edge_crossings 0x20 against 0x18, segments_intersect_xz 0x40 with its two segments at
+0x20 and +0x30, pf_first_edge_crossing, pf_straight_walk_clear, pf_layer_from_edges 0x18 against 0x10, and
ai_find_nearest_wall_hit). Declared as pf_edge (the 0x10-byte seg2d record of the cells; only a / b are set), the
frames match: pf_trace_edge_crossings, segments_intersect_xz and pf_layer_from_edges are exact. pf_layer_from_edges
copies only a / b of the wall (`s.a = w->a; s.b = w->b;`). ai_probe_move_for_walls keeps its 8-byte seg2d (its frame
matches with it). The header is unchanged; the original probably had one segment type for both.

**D6403 (the ten units of this pass). Source shapes that make cl 12.00.8168 emit build 8966's code** (each verified by a
byte match; details in the match6 notes):
- Unrolled source. A loop over constant button indices that the binary runs as straight-line code is written out in
  the source: spells_update_species_tab (16 `&= ~1` stores), spells_set_page's six clears (not yet exact),
  spells_count_page (one switch case per class with its own count call and button pair, a local macro: 168 -> 508
  bytes, exact). skills_count_page and the skills species tab (new_skill.cpp) have the same shape (not in this pass).
- Element pointers for both arrays of a button loader (`ui_button_state *b = &x_buttons[i]; const ui_button_def *d =
  &x_button_defs[i];`): spells / journal / itemtrade_load_bitmaps exact (likely the same for the automap, skills and
  status loaders). Also spells_draw, spells_category_tooltip, journal_draw, journal_update_buttons, itemtrade_draw.
- Index loops instead of pointer loops where the binary compares with `jl` (fx_lists_init, itemtrade_frame,
  write_groups); a pointer loop where it uses `jb` (the shadow blob palette).
- Case order: 8168 emits the case bodies of a jump-table switch in source order; ordering the cases as the binary's
  blocks gives the table and the code (damage_type_to_status, spell_get_duration_ms).
- `if (a < b) b = a; g = b;` for a clamp stored to a global whose binary keeps the result in the bound's register
  (journal_desc_page_down, itemtrade_party_page_down, itemtrade_drag_scrollbar); the ternary stores twice.
- Rectangle / call arguments through locals (D6205) for spells_row_hit_icon / _text, itemtrade_list1 / party
  handle_mouse; a call argument that is a call goes into a local first (localize_topic_name, find_move in
  mode_sneaking_frame, the quest step text in journal_select_quest); `height = fh * n` with n from gr_split_str
  (dialogue_line::init, dialogue_topic_entry::init).
- Block layout: `if (x <= 0) A(); else B();` (spells_init), `if (i != -1) { ...; if (...) return true; } return
  false;` (serpent_part_is_busy), a while loop instead of if + do-while (ai_probe_move_for_walls), the then-block
  where the binary falls through (fx_end_owned, journal / itemtrade_update_buttons with the release branch first,
  level_script_load_steps, layout_thumbnail's `!(used & 0xf) ? 0 : ...`), `if (*p) { do ... while (*p); }`
  (split_quotes), `case 1: ...; return key;` so that the default shares case 2's return (journal_frame), a switch for
  the two skill ids (character_use_skill_on, not yet exact).
- Bool shapes (D5904 / D6205): `? true : false` for spells_row_hit and character_can_cast_now, the if-form for
  fx_is_finished, `return a && b;` for dialogue item_visible (EAX), `return x <= y;` with the element pointer for
  line_fits, `bool r = f() == 1; return r;` (segments_intersect_xz), `(float)fabs(d) < 1e-6f` (D6300).
- Locals: a bitfield read into an int before the range test keeps the signed extract (read_living), `index` / `bit`
  computed before the value test (set_quest_bit), the record size computed once (layout_living), the ring index
  `(i + first)`, float bits copied as an int (get_float: `*(int *)&value = ...`), `step = first < last ? 1 : -1`,
  `float pct = level * 2.0f; hit = pct * 0.01f;` (spell_compute_resist), a vector3 built from its components
  (`vector3 end(p.x + m.x, ...)` is constructed in place; `vector3 end = a + b` is not, D6306).
- Operand order of an int sum of globals: `thumb + (mouse_y - start_y)` gives the binary's load order;
  itemtrade_drag_scrollbar matched with swapped relocation targets in the natural order (a warning of the tool, now
  clean), journal_drag_scrollbar likewise (still differs elsewhere).

**D6404 (game/level_scripts.cpp). automap_label_add copies the label text with memcpy(dst, text, strlen(text)).** The
binary's copy after the 0x20-byte memset is `repne scasb / not ecx / dec ecx` and `rep movsd / rep movsb` of that
length: no terminator is copied (the memset provides it). The source had strcpy, which also writes the NUL; the result
is the same for texts below 32 characters. Still differs (the coordinates go through the FPU, D6405).

**D6405 (notes only). Differences of this pass that no source form changed** (8168 against 8966, besides D5905 /
D6206 / D6306):
- Jump tables that start at 0 although the smallest case is 2 or 5 (status_effect_modifier, status_effect_duration):
  8966 indexes the table with the value, 8168 subtracts the minimum; empty explicit cases are dropped by 8168.
- One call with a selected constant argument (`push 1 / jmp / push 0 / call`): quest_journal_set_entry,
  savegame_refresh_character_flags; 8168 gives setne or neg / sbb, or two calls for an if / else (8966 merges them).
- Float members copied through the FPU into a stack temporary and stored as an int (write_character's lent_max_hp,
  automap_label_add's x / z, spell_effect_obj_set_timer's descriptor); 8168 copies with mov for every form tried
  (float local, memcpy, int pun).
- `x = a op b` with the inline operator= that returns a copy: 8168 calls the vector3 copy constructor out of line
  (steer_jitter, path_closest_point, the other ai_move EH functions), 8966 expands it.
- Register choice and scheduling (dialogue first / last_visible_line, level_item_has / add / remove, get_int,
  spells_set_page, use_skill_on's shrink-wrapped push, fx_new's stores), a NULL return value kept in EAX from the
  start (spell_effect_alloc), the loop rotation of shadow_create_blob_bitmap, the FPU-kept cross product of
  seg2d_point_left_of, serpent_part_is_busy_or_asleep (the binary has serpent_part_is_busy's body inlined there only).

**D6406 (notes only). Markers that say exact but differ under 8168** (by reading, before the VC6 tool; left as they
were, as D6207): level_scripts.cpp report_sizes, write_character, data_offset, refresh_character_flags, set_bit,
get_int, set_float, has_conversation_data, get_speaker_bit, set_speaker_bit, get_topic_bit, set_topic_bit,
level_item_add / remove / has, automap_label_add, hot_button_get, write_living, write_item_objects, read_info,
read_header, read_items, read_inventory, read_automap_labels, savegame_location_allowed, quest_journal_set_entry,
script_filenames_parse_table; new_itemtrade.cpp count_party_rows, party_page_up; new_journal.cpp desc_page_up,
goto_inventory; new_spell.cpp spells_set_page; spells.cpp status_effect_modifier, status_effect_duration,
spell_effect_obj_set_timer, spell_spawn_hit_fx, spell_fire_missile; the dialogue box first_visible_line,
last_visible_line, topic_list::add; seg2d_point_left_of. seg2d_point_near_middle, whose source was rewritten, is now
marked likely.
